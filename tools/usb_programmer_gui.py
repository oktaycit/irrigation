#!/usr/bin/env python3
"""Simple USB config GUI for irrigation program parameters."""

from __future__ import annotations

import argparse
import errno
import glob
import os
import queue
import select
import termios
import threading
import time
import tkinter as tk
from dataclasses import dataclass
from tkinter import messagebox, ttk


PORT_PATTERNS = (
    "/dev/cu.usbmodem*",
    "/dev/cu.usbserial*",
    "/dev/ttyACM*",
    "/dev/ttyUSB*",
)

DAY_LABELS = ("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
PROGRAM_COUNT = 8
VALVE_COUNT = 8


def discover_ports() -> list[str]:
    ports: set[str] = set()
    for pattern in PORT_PATTERNS:
        ports.update(glob.glob(pattern))
    return sorted(ports)


def format_hhmm(value: int) -> str:
    return f"{value // 100:02d}:{value % 100:02d}"


def parse_hhmm(text: str) -> int:
    raw = text.strip().replace(":", "")
    if len(raw) != 4 or not raw.isdigit():
        raise ValueError("Time must be HHMM or HH:MM")
    hours = int(raw[:2])
    minutes = int(raw[2:])
    if hours > 23 or minutes > 59:
        raise ValueError("Time must be between 00:00 and 23:59")
    return hours * 100 + minutes


@dataclass
class ProgramConfig:
    program_id: int
    enabled: int
    start_hhmm: int
    end_hhmm: int
    valve_mask: int
    irrigation_min: int
    wait_min: int
    repeat_count: int
    days_mask: int
    ph_x100: int
    ec_x100: int
    fert_a_pct: int = 25
    fert_b_pct: int = 25
    fert_c_pct: int = 25
    fert_d_pct: int = 25
    pre_flush_sec: int = 60
    post_flush_sec: int = 120

    @classmethod
    def from_protocol_line(cls, line: str) -> "ProgramConfig":
        parts = [item.strip() for item in line.strip().split(",")]
        if parts[:1] == ["PROGRAM"]:
            payload = parts[1:]
        elif parts[:2] in (["OK", "PROGRAM"], ["OK", "SET"]):
            payload = parts[2:]
        else:
            raise ValueError(f"Unexpected program line: {line}")

        if len(payload) not in (11, 15, 17, 19, 20):
            raise ValueError(f"Expected 11, 15, 17, 19 or 20 fields, got {len(payload)}")

        values = [int(item) for item in payload]
        if len(values) == 11:
            values.extend([25, 25, 25, 25, 60, 120])
        elif len(values) == 15:
            values.extend([60, 120])
        elif len(values) in (19, 20):
            values = values[:15] + [60, 120]
        return cls(*values[:17])

    def to_set_command(self) -> str:
        return (
            "SET,"
            f"{self.program_id},{self.enabled},{self.start_hhmm:04d},"
            f"{self.end_hhmm:04d},{self.valve_mask},{self.irrigation_min},"
            f"{self.wait_min},{self.repeat_count},{self.days_mask},"
            f"{self.ph_x100},{self.ec_x100},{self.fert_a_pct},"
            f"{self.fert_b_pct},{self.fert_c_pct},{self.fert_d_pct},"
            f"{self.pre_flush_sec},{self.post_flush_sec}"
        )

    def summary(self) -> str:
        state = "ON" if self.enabled else "OFF"
        return (
            f"P{self.program_id}  {state:<3}  "
            f"{format_hhmm(self.start_hhmm)}-{format_hhmm(self.end_hhmm)}  "
            f"V:{self.valve_mask:03d}  D:{self.days_mask:03d}"
        )


class UsbConfigClient:
    def __init__(self) -> None:
        self.fd: int | None = None
        self.port: str | None = None
        self.lock = threading.Lock()

    def open(self, port: str) -> None:
        with self.lock:
            self.close()
            try:
                fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            except OSError as exc:
                if exc.errno == errno.ENXIO:
                    raise RuntimeError(
                        f"{port} is no longer configured. Refresh ports and reconnect."
                    ) from exc
                raise
            attrs = termios.tcgetattr(fd)
            attrs[0] = 0
            attrs[1] = 0
            attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attrs[3] = 0
            attrs[4] = termios.B115200
            attrs[5] = termios.B115200
            termios.tcsetattr(fd, termios.TCSANOW, attrs)
            termios.tcflush(fd, termios.TCIOFLUSH)
            self.fd = fd
            self.port = port

    def close(self) -> None:
        if self.fd is not None:
            os.close(self.fd)
            self.fd = None
        self.port = None

    def ensure_open(self) -> int:
        if self.fd is None:
            raise RuntimeError("USB port is not connected")
        return self.fd

    def _drain_input(self, quiet_window: float = 0.12) -> None:
        fd = self.ensure_open()
        quiet_deadline = time.monotonic() + quiet_window

        while True:
            remaining = quiet_deadline - time.monotonic()
            if remaining <= 0:
                return

            ready, _, _ = select.select([fd], [], [], remaining)
            if fd not in ready:
                return

            try:
                chunk = os.read(fd, 512)
            except BlockingIOError:
                return

            if not chunk:
                return

            quiet_deadline = time.monotonic() + quiet_window

    def transact(
        self,
        command: str,
        *,
        timeout: float = 2.0,
        idle_timeout: float = 0.25,
        until_line: str | None = None,
    ) -> list[str]:
        with self.lock:
            fd = self.ensure_open()
            termios.tcflush(fd, termios.TCIOFLUSH)
            self._drain_input()
            os.write(fd, f"{command}\r\n".encode("utf-8"))
            time.sleep(0.02)

            deadline = time.monotonic() + timeout
            last_rx: float | None = None
            buffer = ""
            lines: list[str] = []

            while time.monotonic() < deadline:
                wait_time = max(0.0, min(0.2, deadline - time.monotonic()))
                ready, _, _ = select.select([fd], [], [], wait_time)

                if fd in ready:
                    try:
                        chunk = os.read(fd, 512)
                    except BlockingIOError:
                        continue
                    if not chunk:
                        continue
                    buffer += chunk.decode("utf-8", errors="replace")
                    last_rx = time.monotonic()

                    while "\n" in buffer:
                        line, buffer = buffer.split("\n", 1)
                        line = line.rstrip("\r")
                        if not line:
                            continue
                        lines.append(line)
                        if until_line is not None and line == until_line:
                            return lines
                elif lines and last_rx is not None and time.monotonic() - last_rx >= idle_timeout:
                    break

            if buffer.strip():
                lines.append(buffer.strip())

            return lines

    @staticmethod
    def _raise_if_error(lines: list[str]) -> None:
        for line in lines:
            if line.startswith("ERR,"):
                raise RuntimeError(line)

    def ping(self) -> str:
        lines = self.transact("PING", timeout=2.0, idle_timeout=0.35)
        self._raise_if_error(lines)
        if "OK,PONG" not in lines:
            raise RuntimeError("PING response not received")
        return "OK,PONG"

    def list_programs(self) -> list[ProgramConfig]:
        lines = self.transact(
            "LIST", timeout=4.0, idle_timeout=0.35, until_line="OK,LIST,END"
        )
        self._raise_if_error(lines)

        begin_seen = False
        end_seen = False
        programs: list[ProgramConfig] = []

        for line in lines:
            if line == "OK,LIST,BEGIN":
                begin_seen = True
                continue
            if line == "OK,LIST,END":
                end_seen = True
                break
            if line.startswith("PROGRAM,"):
                programs.append(ProgramConfig.from_protocol_line(line))

        if begin_seen and end_seen and programs:
            programs.sort(key=lambda program: program.program_id)
            return programs

        return [self.get_program(program_id) for program_id in range(1, PROGRAM_COUNT + 1)]

    def get_program(self, program_id: int) -> ProgramConfig:
        for _ in range(3):
            lines = self.transact(f"GET,{program_id}", timeout=3.0, idle_timeout=0.4)
            self._raise_if_error(lines)
            for line in lines:
                if line.startswith("OK,PROGRAM,"):
                    program = ProgramConfig.from_protocol_line(line)
                    if program.program_id == program_id:
                        return program
            time.sleep(0.05)

        raise RuntimeError(f"GET response not received for program {program_id}")

    def set_program(self, program: ProgramConfig) -> ProgramConfig:
        lines = self.transact(program.to_set_command(), timeout=3.0, idle_timeout=0.4)
        self._raise_if_error(lines)
        for line in lines:
            if line.startswith("OK,SET,"):
                return ProgramConfig.from_protocol_line(line)
        raise RuntimeError("SET response not received")


class UsbProgrammerApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("Irrigation USB Programmer")
        self.geometry("1080x720")
        self.minsize(980, 640)

        self.client = UsbConfigClient()
        self.programs: dict[int, ProgramConfig] = {}
        self.busy = False
        self.job_token = 0
        self.connected_port: str | None = None
        self.result_queue: queue.Queue = queue.Queue()

        self.port_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Not connected")
        self.selected_program_var = tk.IntVar(value=1)
        self.enabled_var = tk.IntVar(value=1)
        self.start_var = tk.StringVar(value="06:00")
        self.end_var = tk.StringVar(value="07:00")
        self.irrigation_var = tk.StringVar(value="5")
        self.wait_var = tk.StringVar(value="0")
        self.repeat_var = tk.StringVar(value="1")
        self.ph_var = tk.StringVar(value="650")
        self.ec_var = tk.StringVar(value="180")
        self.fert_ratio_vars = [tk.StringVar(value="25") for _ in range(4)]
        self.pre_flush_var = tk.StringVar(value="60")
        self.post_flush_var = tk.StringVar(value="120")
        self.valve_vars = [tk.IntVar(value=0) for _ in range(VALVE_COUNT)]
        self.day_vars = [tk.IntVar(value=1) for _ in range(7)]

        self._build_ui()
        self.refresh_ports()
        self.after(100, self._poll_worker_results)
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def _build_ui(self) -> None:
        self.columnconfigure(0, weight=1)
        self.rowconfigure(2, weight=1)
        self.rowconfigure(3, weight=0)

        top = ttk.Frame(self, padding=12)
        top.grid(row=0, column=0, sticky="ew")
        top.columnconfigure(1, weight=1)

        ttk.Label(top, text="USB Port").grid(row=0, column=0, sticky="w", padx=(0, 8))
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky="ew", padx=(0, 8))
        ttk.Button(top, text="Refresh Ports", command=self.refresh_ports).grid(
            row=0, column=2, padx=4
        )
        ttk.Button(top, text="Connect", command=self.connect_selected_port).grid(
            row=0, column=3, padx=4
        )
        ttk.Button(top, text="Ping", command=self.ping_device).grid(row=0, column=4, padx=4)
        ttk.Button(top, text="Read All", command=self.load_all_programs).grid(
            row=0, column=5, padx=4
        )
        ttk.Button(top, text="Save / Kaydet", command=self.save_selected_program).grid(
            row=0, column=6, padx=(8, 0)
        )

        status = ttk.Frame(self, padding=(12, 0, 12, 8))
        status.grid(row=1, column=0, sticky="ew")
        status.columnconfigure(1, weight=1)
        ttk.Label(status, text="Status").grid(row=0, column=0, sticky="w", padx=(0, 8))
        ttk.Label(status, textvariable=self.status_var, foreground="#0f4c81").grid(
            row=0, column=1, sticky="w"
        )

        content = ttk.Frame(self, padding=(12, 0, 12, 12))
        content.grid(row=2, column=0, sticky="nsew")
        content.columnconfigure(0, weight=0)
        content.columnconfigure(1, weight=1)
        content.rowconfigure(0, weight=1)

        left = ttk.LabelFrame(content, text="Program List", padding=12)
        left.grid(row=0, column=0, sticky="nsw", padx=(0, 12))
        left.rowconfigure(0, weight=1)
        self.program_list = tk.Listbox(left, width=38, height=24, exportselection=False)
        self.program_list.grid(row=0, column=0, sticky="ns")
        self.program_list.bind("<<ListboxSelect>>", self.on_program_selected)
        ttk.Button(left, text="Read Selected", command=self.load_selected_program).grid(
            row=1, column=0, sticky="ew", pady=(10, 0)
        )

        right = ttk.LabelFrame(content, text="Program Editor", padding=12)
        right.grid(row=0, column=1, sticky="nsew")
        for column in range(4):
            right.columnconfigure(column, weight=1)

        ttk.Label(right, text="Program ID").grid(row=0, column=0, sticky="w", pady=4)
        self.id_spin = ttk.Spinbox(
            right,
            from_=1,
            to=PROGRAM_COUNT,
            textvariable=self.selected_program_var,
            width=8,
            command=self.on_id_spin_changed,
        )
        self.id_spin.grid(row=0, column=1, sticky="w", pady=4)
        ttk.Checkbutton(right, text="Enabled", variable=self.enabled_var).grid(
            row=0, column=2, sticky="w", pady=4
        )

        ttk.Label(right, text="Start").grid(row=1, column=0, sticky="w", pady=4)
        ttk.Entry(right, textvariable=self.start_var, width=14).grid(
            row=1, column=1, sticky="ew", pady=4
        )
        ttk.Label(right, text="End").grid(row=1, column=2, sticky="w", pady=4)
        ttk.Entry(right, textvariable=self.end_var, width=14).grid(
            row=1, column=3, sticky="ew", pady=4
        )

        ttk.Label(right, text="Irrigation (min)").grid(row=2, column=0, sticky="w", pady=4)
        ttk.Entry(right, textvariable=self.irrigation_var).grid(
            row=2, column=1, sticky="ew", pady=4
        )
        ttk.Label(right, text="Wait (min)").grid(row=2, column=2, sticky="w", pady=4)
        ttk.Entry(right, textvariable=self.wait_var).grid(
            row=2, column=3, sticky="ew", pady=4
        )

        ttk.Label(right, text="Repeat Count").grid(row=3, column=0, sticky="w", pady=4)
        ttk.Entry(right, textvariable=self.repeat_var).grid(
            row=3, column=1, sticky="ew", pady=4
        )
        ttk.Label(right, text="").grid(row=3, column=2, sticky="w", pady=4)

        ttk.Label(right, text="pH x100").grid(row=4, column=0, sticky="w", pady=4)
        ttk.Entry(right, textvariable=self.ph_var).grid(row=4, column=1, sticky="ew", pady=4)
        ttk.Label(right, text="EC x100").grid(row=4, column=2, sticky="w", pady=4)
        ttk.Entry(right, textvariable=self.ec_var).grid(row=4, column=3, sticky="ew", pady=4)
        ttk.Label(right, text="Example: pH 6.50 -> 650, EC 1.80 -> 180").grid(
            row=5, column=0, columnspan=4, sticky="w", pady=4
        )

        valves = ttk.LabelFrame(right, text="Valve Mask", padding=10)
        valves.grid(row=6, column=0, columnspan=4, sticky="ew", pady=(10, 6))
        for index, variable in enumerate(self.valve_vars):
            ttk.Checkbutton(valves, text=f"Valve {index + 1}", variable=variable).grid(
                row=index // 4, column=index % 4, padx=6, pady=4, sticky="w"
            )

        days = ttk.LabelFrame(right, text="Days Mask", padding=10)
        days.grid(row=7, column=0, columnspan=4, sticky="ew", pady=(6, 10))
        for index, label in enumerate(DAY_LABELS):
            ttk.Checkbutton(days, text=label, variable=self.day_vars[index]).grid(
                row=0, column=index, padx=6, pady=4, sticky="w"
            )

        ratios = ttk.LabelFrame(right, text="Fertilizer Mix (%)", padding=10)
        ratios.grid(row=8, column=0, columnspan=4, sticky="ew", pady=(0, 10))
        for index, label in enumerate(("A", "B", "C", "D")):
            ttk.Label(ratios, text=label).grid(row=0, column=index * 2, padx=(6, 4))
            ttk.Entry(ratios, textvariable=self.fert_ratio_vars[index], width=6).grid(
                row=0, column=(index * 2) + 1, padx=(0, 10), sticky="w"
            )

        flush = ttk.LabelFrame(right, text="Flush Timing", padding=10)
        flush.grid(row=9, column=0, columnspan=4, sticky="ew", pady=(0, 10))
        ttk.Label(flush, text="Pre-flush (sec)").grid(row=0, column=0, padx=(6, 4), sticky="w")
        ttk.Entry(flush, textvariable=self.pre_flush_var, width=8).grid(
            row=0, column=1, padx=(0, 16), sticky="w"
        )
        ttk.Label(flush, text="Post-flush (sec)").grid(row=0, column=2, padx=(6, 4), sticky="w")
        ttk.Entry(flush, textvariable=self.post_flush_var, width=8).grid(
            row=0, column=3, padx=(0, 10), sticky="w"
        )

        button_row = ttk.Frame(right)
        button_row.grid(row=10, column=0, columnspan=4, sticky="ew")
        button_row.columnconfigure(0, weight=1)
        ttk.Button(button_row, text="Kaydet", command=self.save_selected_program).grid(
            row=0, column=0, sticky="ew", padx=(0, 8)
        )
        ttk.Button(button_row, text="Reload Selected", command=self.load_selected_program).grid(
            row=0, column=1, sticky="ew"
        )

        log_frame = ttk.LabelFrame(self, text="Log", padding=(12, 8))
        log_frame.grid(row=3, column=0, sticky="nsew", padx=12, pady=(0, 12))
        log_frame.rowconfigure(0, weight=1)
        log_frame.columnconfigure(0, weight=1)
        self.log_text = tk.Text(log_frame, height=6, wrap="word", state="disabled")
        self.log_text.grid(row=0, column=0, sticky="nsew")

    def refresh_ports(self) -> None:
        ports = discover_ports()
        self.port_combo["values"] = ports
        if ports:
            current = self.port_var.get()
            self.port_var.set(current if current in ports else ports[-1])
            self.log(f"Detected ports: {', '.join(ports)}")
        else:
            self.port_var.set("")
            self.log("No USB serial ports detected.")

    def connect_selected_port(self) -> None:
        port = self.port_var.get().strip()
        if not port:
            messagebox.showerror("USB Port", "Select a USB serial port first.")
            return

        def worker() -> str:
            self.client.open(port)
            try:
                self.client.ping()
                return port
            finally:
                self.client.close()

        self.run_job(
            "Connecting",
            worker,
            on_success=lambda connected_port: self._set_connected(connected_port),
        )

    def _set_connected(self, port: str) -> None:
        self.connected_port = port
        self.status_var.set(f"Ready: {port}")
        self.log(f"Connected to {port}")

    def _require_connected_port(self) -> str:
        port = self.connected_port or self.port_var.get().strip()
        if not port:
            raise RuntimeError("Connect to a USB port first")
        return port

    def _with_fresh_connection(self, fn):
        port = self._require_connected_port()
        self.client.open(port)
        try:
            return fn()
        finally:
            self.client.close()

    def ping_device(self) -> None:
        self.run_job(
            "Pinging",
            lambda: self._with_fresh_connection(self.client.ping),
            on_success=lambda _: self.log("PING -> OK,PONG"),
        )

    def load_all_programs(self) -> None:
        self.run_job(
            "Reading all programs",
            lambda: self._with_fresh_connection(self.client.list_programs),
            on_success=self._apply_program_list,
        )

    def _apply_program_list(self, programs: list[ProgramConfig]) -> None:
        self.programs = {program.program_id: program for program in programs}
        self.program_list.delete(0, tk.END)
        for program in programs:
            self.program_list.insert(tk.END, program.summary())

        if programs:
            target_id = self.selected_program_var.get()
            index = next(
                (
                    idx
                    for idx, program in enumerate(programs)
                    if program.program_id == target_id
                ),
                0,
            )
            self.program_list.selection_clear(0, tk.END)
            self.program_list.selection_set(index)
            self.program_list.see(index)
            self.populate_form(programs[index])

        self.status_var.set(f"Loaded {len(programs)} programs")
        self.log(f"Loaded {len(programs)} programs from device.")

    def load_selected_program(self) -> None:
        program_id = self.selected_program_var.get()
        self.run_job(
            f"Reading program {program_id}",
            lambda: self._with_fresh_connection(
                lambda: self.client.get_program(program_id)
            ),
            on_success=self._apply_single_program,
        )

    def _apply_single_program(self, program: ProgramConfig) -> None:
        self.programs[program.program_id] = program
        self.populate_form(program)
        self._refresh_listbox_entry(program)
        self.status_var.set(f"Loaded program {program.program_id}")
        self.log(f"Program {program.program_id} loaded.")

    def save_selected_program(self) -> None:
        try:
            program = self.read_form()
        except ValueError as exc:
            self.log(f"Validation failed: {exc}")
            messagebox.showerror("Validation Error", str(exc))
            return

        self.log(f"SET -> {program.to_set_command()}")
        self.run_job(
            f"Saving program {program.program_id}",
            lambda: self._with_fresh_connection(lambda: self.client.set_program(program)),
            on_success=self._after_program_saved,
        )

    def _after_program_saved(self, program: ProgramConfig) -> None:
        self.programs[program.program_id] = program
        self.populate_form(program)
        self._refresh_listbox_entry(program)
        self.status_var.set(f"Program {program.program_id} saved")
        self.log(f"Program {program.program_id} written to device.")

    def on_program_selected(self, _event: object) -> None:
        selection = self.program_list.curselection()
        if not selection:
            return

        line = self.program_list.get(selection[0])
        try:
            program_id = int(line.split()[0][1:])
        except (ValueError, IndexError):
            return

        self.selected_program_var.set(program_id)
        program = self.programs.get(program_id)
        if program is not None:
            self.populate_form(program)

    def on_id_spin_changed(self) -> None:
        program_id = self.selected_program_var.get()
        program = self.programs.get(program_id)
        if program is not None:
            self.populate_form(program)

    def populate_form(self, program: ProgramConfig) -> None:
        self.selected_program_var.set(program.program_id)
        self.enabled_var.set(program.enabled)
        self.start_var.set(format_hhmm(program.start_hhmm))
        self.end_var.set(format_hhmm(program.end_hhmm))
        self.irrigation_var.set(str(program.irrigation_min))
        self.wait_var.set(str(program.wait_min))
        self.repeat_var.set(str(program.repeat_count))
        self.ph_var.set(str(program.ph_x100))
        self.ec_var.set(str(program.ec_x100))
        self.pre_flush_var.set(str(program.pre_flush_sec))
        self.post_flush_var.set(str(program.post_flush_sec))
        for variable, value in zip(
            self.fert_ratio_vars,
            (
                program.fert_a_pct,
                program.fert_b_pct,
                program.fert_c_pct,
                program.fert_d_pct,
            ),
        ):
            variable.set(str(value))

        for index, variable in enumerate(self.valve_vars):
            variable.set(1 if (program.valve_mask & (1 << index)) else 0)
        for index, variable in enumerate(self.day_vars):
            variable.set(1 if (program.days_mask & (1 << index)) else 0)

    def read_form(self) -> ProgramConfig:
        program_id = int(self.selected_program_var.get())
        if program_id < 1 or program_id > PROGRAM_COUNT:
            raise ValueError(f"Program ID must be 1..{PROGRAM_COUNT}")

        valve_mask = 0
        for index, variable in enumerate(self.valve_vars):
            if variable.get():
                valve_mask |= 1 << index

        days_mask = 0
        for index, variable in enumerate(self.day_vars):
            if variable.get():
                days_mask |= 1 << index

        try:
            irrigation_min = int(self.irrigation_var.get().strip())
        except ValueError as exc:
            raise ValueError("Irrigation minutes must be a whole number") from exc

        try:
            wait_min = int(self.wait_var.get().strip())
        except ValueError as exc:
            raise ValueError("Wait minutes must be a whole number") from exc

        try:
            repeat_count = int(self.repeat_var.get().strip())
        except ValueError as exc:
            raise ValueError("Repeat count must be a whole number") from exc

        try:
            ph_x100 = int(self.ph_var.get().strip())
        except ValueError as exc:
            raise ValueError("pH must be x100 whole number, example 6.50 -> 650") from exc

        try:
            ec_x100 = int(self.ec_var.get().strip())
        except ValueError as exc:
            raise ValueError("EC must be x100 whole number, example 1.80 -> 180") from exc

        try:
            pre_flush_sec = int(self.pre_flush_var.get().strip())
        except ValueError as exc:
            raise ValueError("Pre-flush seconds must be a whole number") from exc

        try:
            post_flush_sec = int(self.post_flush_var.get().strip())
        except ValueError as exc:
            raise ValueError("Post-flush seconds must be a whole number") from exc

        fert_ratios: list[int] = []
        for label, variable in zip(("A", "B", "C", "D"), self.fert_ratio_vars):
            try:
                ratio = int(variable.get().strip())
            except ValueError as exc:
                raise ValueError(f"Fertilizer {label} percent must be a whole number") from exc
            fert_ratios.append(ratio)

        if valve_mask == 0:
            raise ValueError("Select at least one valve")
        if days_mask == 0:
            raise ValueError("Select at least one day")
        if irrigation_min < 1 or irrigation_min > 999:
            raise ValueError("Irrigation minutes must be 1..999")
        if wait_min < 0 or wait_min > 999:
            raise ValueError("Wait minutes must be 0..999")
        if repeat_count < 1 or repeat_count > 99:
            raise ValueError("Repeat count must be 1..99")
        if ph_x100 < 0 or ph_x100 > 1400:
            raise ValueError("pH x100 must be 0..1400")
        if ec_x100 < 0 or ec_x100 > 2000:
            raise ValueError("EC x100 must be 0..2000")
        if pre_flush_sec < 0 or pre_flush_sec > 900:
            raise ValueError("Pre-flush seconds must be 0..900")
        if post_flush_sec < 0 or post_flush_sec > 900:
            raise ValueError("Post-flush seconds must be 0..900")
        if any(ratio < 0 or ratio > 100 for ratio in fert_ratios):
            raise ValueError("Fertilizer mix percentages must be 0..100")
        if sum(fert_ratios) == 0:
            raise ValueError("At least one fertilizer mix percentage must be greater than 0")

        return ProgramConfig(
            program_id=program_id,
            enabled=1 if self.enabled_var.get() else 0,
            start_hhmm=parse_hhmm(self.start_var.get()),
            end_hhmm=parse_hhmm(self.end_var.get()),
            valve_mask=valve_mask,
            irrigation_min=irrigation_min,
            wait_min=wait_min,
            repeat_count=repeat_count,
            days_mask=days_mask,
            ph_x100=ph_x100,
            ec_x100=ec_x100,
            fert_a_pct=fert_ratios[0],
            fert_b_pct=fert_ratios[1],
            fert_c_pct=fert_ratios[2],
            fert_d_pct=fert_ratios[3],
            pre_flush_sec=pre_flush_sec,
            post_flush_sec=post_flush_sec,
        )

    def _refresh_listbox_entry(self, program: ProgramConfig) -> None:
        entries = list(self.program_list.get(0, tk.END))
        target_prefix = f"P{program.program_id} "
        for index, entry in enumerate(entries):
            if entry.startswith(target_prefix):
                self.program_list.delete(index)
                self.program_list.insert(index, program.summary())
                self.program_list.selection_clear(0, tk.END)
                self.program_list.selection_set(index)
                return

        self.program_list.insert(tk.END, program.summary())

    def run_job(self, label: str, worker, *, on_success=None) -> None:
        if self.busy:
            messagebox.showinfo("Busy", "Another USB operation is already running.")
            return

        self.busy = True
        self.job_token += 1
        job_token = self.job_token
        self.status_var.set(f"{label}...")
        self.log(f"{label}...")

        def task() -> None:
            try:
                result = worker()
            except Exception as exc:  # noqa: BLE001
                self.result_queue.put(("error", job_token, label, exc, None))
                return

            self.result_queue.put(("success", job_token, label, result, on_success))

        threading.Thread(target=task, daemon=True).start()
        self.after(self._job_timeout_ms(label), lambda: self._job_timed_out(job_token, label))

    def _job_timeout_ms(self, label: str) -> int:
        if "Reading all programs" in label:
            return 15000
        if "Reading program" in label:
            return 7000
        if "Saving program" in label:
            return 7000
        return 5000

    def _job_finished(self, job_token: int, label: str, result, on_success) -> None:
        if job_token != self.job_token:
            return
        self.busy = False
        if on_success is not None:
            on_success(result)
        elif label:
            self.status_var.set(f"{label} complete")

    def _job_failed(self, job_token: int, label: str, exc: Exception) -> None:
        if job_token != self.job_token:
            return
        self.busy = False
        self.status_var.set(f"{label} failed")
        if "no longer configured" in str(exc):
            self.refresh_ports()
        self.log(f"{label} failed: {exc}")
        messagebox.showerror("USB Operation Failed", str(exc))

    def _job_timed_out(self, job_token: int, label: str) -> None:
        if job_token != self.job_token or not self.busy:
            return

        self.client.close()
        self.busy = False
        self.job_token += 1
        self.refresh_ports()
        self.status_var.set(f"{label} timed out")
        self.log(f"{label} timed out. USB connection was reset.")
        messagebox.showerror(
            "USB Operation Timed Out",
            f"{label} did not complete in time. The USB connection was reset.",
        )

    def log(self, message: str) -> None:
        timestamp = time.strftime("%H:%M:%S")
        self.log_text.configure(state="normal")
        self.log_text.insert("end", f"[{timestamp}] {message}\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def _poll_worker_results(self) -> None:
        while True:
            try:
                status, job_token, label, result, on_success = self.result_queue.get_nowait()
            except queue.Empty:
                break

            if status == "success":
                self._job_finished(job_token, label, result, on_success)
            else:
                self._job_failed(job_token, label, result)

        self.after(100, self._poll_worker_results)

    def on_close(self) -> None:
        self.client.close()
        self.connected_port = None
        self.destroy()


def run_cli(port: str, *, do_ping: bool, do_list: bool) -> int:
    client = UsbConfigClient()
    try:
        client.open(port)
        if do_ping:
            print(client.ping())
        if do_list:
            for program in client.list_programs():
                print(program.summary())
        return 0
    finally:
        client.close()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="GUI and CLI utility for irrigation USB program parameters."
    )
    parser.add_argument("--port", help="USB serial port path")
    parser.add_argument("--ping", action="store_true", help="Send PING and print response")
    parser.add_argument("--list", action="store_true", help="Read and print all programs")
    args = parser.parse_args()

    if args.ping or args.list:
        if not args.port:
            parser.error("--port is required with --ping or --list")
        return run_cli(args.port, do_ping=args.ping, do_list=args.list)

    app = UsbProgrammerApp()
    app.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
