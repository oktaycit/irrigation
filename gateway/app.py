#!/usr/bin/env python3
"""Small HTTP gateway for the STM32 irrigation USB CDC protocol."""

from __future__ import annotations

import argparse
import glob
import json
import mimetypes
import os
import select
import threading
import time
import termios
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import urlparse


DEFAULT_PORT = "/dev/serial/by-id/usb-OktayCit_Irrigation_Controller_Config_Port_207E377C5232-if00"
DEFAULT_BAUD = 115200
APP_DIR = Path(__file__).resolve().parent
STATIC_DIR = APP_DIR / "static"


class SerialProtocolError(RuntimeError):
    pass


class IrrigationSerialClient:
    def __init__(self, port: str, baud: int = DEFAULT_BAUD, timeout: float = 2.5) -> None:
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self._lock = threading.Lock()

    def command(self, command: str, timeout: float | None = None, end_marker: str | None = None) -> list[str]:
        if not command or "\n" in command or "\r" in command:
            raise ValueError("command must be a single line")

        with self._lock:
            fd = os.open(self.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            try:
                self._configure(fd)
                os.write(fd, command.encode("ascii") + b"\r\n")
                return self._read_lines(fd, timeout or self.timeout, end_marker)
            finally:
                os.close(fd)

    def _configure(self, fd: int) -> None:
        attrs = termios.tcgetattr(fd)
        baud_const = getattr(termios, f"B{self.baud}", termios.B115200)
        attrs[4] = baud_const
        attrs[5] = baud_const
        attrs[2] |= termios.CLOCAL | termios.CREAD
        attrs[3] &= ~(termios.ICANON | termios.ECHO | termios.ECHOE | termios.ISIG)
        attrs[0] &= ~(termios.IXON | termios.IXOFF | termios.IXANY | termios.ICRNL | termios.INLCR)
        attrs[1] &= ~termios.OPOST
        termios.tcsetattr(fd, termios.TCSANOW, attrs)

    def _read_lines(self, fd: int, timeout: float, end_marker: str | None) -> list[str]:
        end = time.time() + timeout
        buf = b""
        lines: list[str] = []

        while time.time() < end:
            ready, _, _ = select.select([fd], [], [], 0.1)
            if not ready:
                continue

            try:
                chunk = os.read(fd, 4096)
            except BlockingIOError:
                continue

            if not chunk:
                continue

            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                line = raw.decode("utf-8", "replace").strip("\r")
                if line:
                    lines.append(line)
                if end_marker and line == end_marker:
                    return lines
                if not end_marker and lines:
                    return lines

        if buf:
            line = buf.decode("utf-8", "replace").strip()
            if line:
                lines.append(line)
        return lines


def parse_program_line(line: str) -> dict[str, Any]:
    parts = line.split(",")
    if parts[0] == "OK" and len(parts) > 1 and parts[1] == "PROGRAM":
        fields = parts[2:]
    elif parts[0] == "PROGRAM":
        fields = parts[1:]
    else:
        raise SerialProtocolError(f"not a program line: {line}")

    if len(fields) != 17:
        raise SerialProtocolError(f"unexpected program field count: {len(fields)}")

    keys = [
        "id",
        "enabled",
        "start_hhmm",
        "end_hhmm",
        "valve_mask",
        "irrigation_min",
        "wait_min",
        "repeat_count",
        "days_mask",
        "ph_x100",
        "ec_x100",
        "fert_a_pct",
        "fert_b_pct",
        "fert_c_pct",
        "fert_d_pct",
        "pre_flush_sec",
        "post_flush_sec",
    ]
    return {key: int(value) for key, value in zip(keys, fields, strict=True)}


def make_set_command(program_id: int, payload: dict[str, Any]) -> str:
    keys = [
        "enabled",
        "start_hhmm",
        "end_hhmm",
        "valve_mask",
        "irrigation_min",
        "wait_min",
        "repeat_count",
        "days_mask",
        "ph_x100",
        "ec_x100",
        "fert_a_pct",
        "fert_b_pct",
        "fert_c_pct",
        "fert_d_pct",
        "pre_flush_sec",
        "post_flush_sec",
    ]
    missing = [key for key in keys if key not in payload]
    if missing:
        raise ValueError(f"missing fields: {', '.join(missing)}")

    values = [program_id] + [int(payload[key]) for key in keys]
    return "SET," + ",".join(str(value) for value in values)


def parse_prefixed_line(line: str, prefix: str, keys: list[str]) -> dict[str, Any]:
    parts = line.split(",")
    expected_prefix = ["OK", prefix]
    if parts[:2] != expected_prefix:
        raise SerialProtocolError(f"not a {prefix.lower()} line: {line}")

    fields = parts[2:]
    if len(fields) != len(keys):
        raise SerialProtocolError(f"unexpected {prefix.lower()} field count: {len(fields)}")

    payload: dict[str, Any] = {}
    for key, value in zip(keys, fields, strict=True):
        try:
            payload[key] = int(value)
        except ValueError:
            payload[key] = value
    return payload


def first_line_with_prefix(lines: list[str], prefix: str) -> str:
    expected = f"OK,{prefix},"
    for line in lines:
        if line.startswith(expected):
            return line
    raise SerialProtocolError(f"no {prefix.lower()} response: {lines}")


def parse_device_line(line: str) -> dict[str, Any]:
    return parse_prefixed_line(
        line,
        "DEVICE",
        [
            "device_name",
            "firmware_version",
            "protocol_version",
            "mcu",
            "program_count",
            "parcel_valve_count",
            "dosing_valve_count",
            "uptime_ms",
        ],
    )


def parse_telemetry_line(line: str) -> dict[str, Any]:
    return parse_prefixed_line(
        line,
        "TELEMETRY",
        [
            "uptime_ms",
            "control_state",
            "program_state",
            "is_running",
            "active_program_id",
            "current_parcel_id",
            "remaining_sec",
            "ph_x100",
            "ec_x100",
            "ph_status",
            "ec_status",
            "active_valve_mask",
            "last_error",
            "alarm_active",
        ],
    )


def parse_fault_line(line: str) -> dict[str, Any]:
    return parse_prefixed_line(
        line,
        "FAULT",
        [
            "active",
            "latched",
            "manual_ack_required",
            "acknowledged",
            "last_error",
            "valve_error_mask",
            "recommended_state",
            "alarm_text",
        ],
    )


def parse_runtime_line(line: str) -> dict[str, Any]:
    return parse_prefixed_line(
        line,
        "RUNTIME",
        [
            "valid",
            "active_program_id",
            "active_valve_index",
            "repeat_index",
            "program_state",
            "active_valve_id",
            "remaining_sec",
            "ec_target_x100",
            "ph_target_x100",
            "error_code",
        ],
    )


class GatewayHandler(BaseHTTPRequestHandler):
    client: IrrigationSerialClient

    def do_HEAD(self) -> None:
        path = urlparse(self.path).path.rstrip("/") or "/"
        if path == "/":
            self._file_head(STATIC_DIR / "index.html")
        elif path.startswith("/static/"):
            self._file_head(STATIC_DIR / path.removeprefix("/static/"))
        else:
            self.send_response(HTTPStatus.OK.value)
            self.end_headers()

    def do_GET(self) -> None:
        path = urlparse(self.path).path.rstrip("/") or "/"

        try:
            if path == "/":
                self._file(STATIC_DIR / "index.html")
            elif path.startswith("/static/"):
                self._file(STATIC_DIR / path.removeprefix("/static/"))
            elif path == "/health":
                self._json({"ok": True, "serial_port": self.client.port, "time": int(time.time())})
            elif path == "/api/ping":
                lines = self.client.command("PING")
                self._json({"ok": lines == ["OK,PONG"], "response": lines})
            elif path == "/api/device":
                lines = self.client.command("INFO")
                self._json({"device": parse_device_line(first_line_with_prefix(lines, "DEVICE")), "raw": lines})
            elif path == "/api/telemetry":
                lines = self.client.command("TELEM")
                self._json({"telemetry": parse_telemetry_line(first_line_with_prefix(lines, "TELEMETRY")), "raw": lines})
            elif path == "/api/fault":
                lines = self.client.command("FAULT")
                self._json({"fault": parse_fault_line(first_line_with_prefix(lines, "FAULT")), "raw": lines})
            elif path == "/api/runtime":
                lines = self.client.command("RUNTIME")
                self._json({"runtime": parse_runtime_line(first_line_with_prefix(lines, "RUNTIME")), "raw": lines})
            elif path == "/api/programs":
                lines = self.client.command("LIST", timeout=5.0, end_marker="OK,LIST,END")
                programs = [parse_program_line(line) for line in lines if line.startswith("PROGRAM,")]
                self._json({"programs": programs, "raw": lines})
            elif path.startswith("/api/programs/"):
                program_id = self._parse_program_id(path)
                lines = self.client.command(f"GET,{program_id}")
                program_lines = [line for line in lines if line.startswith("OK,PROGRAM,")]
                if not program_lines:
                    self._json({"error": "no program response", "raw": lines}, HTTPStatus.BAD_GATEWAY)
                    return
                self._json({"program": parse_program_line(program_lines[0]), "raw": lines})
            else:
                self._json({"error": "not found"}, HTTPStatus.NOT_FOUND)
        except Exception as exc:
            self._json({"error": str(exc)}, HTTPStatus.INTERNAL_SERVER_ERROR)

    def do_POST(self) -> None:
        path = urlparse(self.path).path.rstrip("/") or "/"
        try:
            if not path.startswith("/api/programs/"):
                self._json({"error": "not found"}, HTTPStatus.NOT_FOUND)
                return

            program_id = self._parse_program_id(path)
            length = int(self.headers.get("Content-Length", "0"))
            payload = json.loads(self.rfile.read(length) or b"{}")
            command = make_set_command(program_id, payload)
            lines = self.client.command(command)
            self._json({"ok": bool(lines and lines[0].startswith("OK,SET,")), "response": lines})
        except Exception as exc:
            self._json({"error": str(exc)}, HTTPStatus.BAD_REQUEST)

    def log_message(self, fmt: str, *args: Any) -> None:
        print(f"{self.address_string()} - {fmt % args}", flush=True)

    def _parse_program_id(self, path: str) -> int:
        try:
            program_id = int(path.rsplit("/", 1)[1])
        except ValueError as exc:
            raise ValueError("program id must be numeric") from exc
        if program_id < 1 or program_id > 8:
            raise ValueError("program id must be 1..8")
        return program_id

    def _json(self, payload: dict[str, Any], status: HTTPStatus = HTTPStatus.OK) -> None:
        body = json.dumps(payload, ensure_ascii=False, indent=2).encode("utf-8")
        self.send_response(status.value)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _file(self, path: Path) -> None:
        resolved = path.resolve()
        if STATIC_DIR.resolve() not in (resolved, *resolved.parents) or not resolved.is_file():
            self._json({"error": "not found"}, HTTPStatus.NOT_FOUND)
            return

        content_type = mimetypes.guess_type(str(resolved))[0] or "application/octet-stream"
        body = resolved.read_bytes()
        self.send_response(HTTPStatus.OK.value)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        self.wfile.write(body)

    def _file_head(self, path: Path) -> None:
        resolved = path.resolve()
        if STATIC_DIR.resolve() not in (resolved, *resolved.parents) or not resolved.is_file():
            self.send_response(HTTPStatus.NOT_FOUND.value)
            self.end_headers()
            return

        content_type = mimetypes.guess_type(str(resolved))[0] or "application/octet-stream"
        self.send_response(HTTPStatus.OK.value)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(resolved.stat().st_size))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()


def auto_port() -> str:
    preferred = os.environ.get("IRRIGATION_SERIAL_PORT")
    if preferred:
        return preferred
    if os.path.exists(DEFAULT_PORT):
        return DEFAULT_PORT

    matches = sorted(glob.glob("/dev/serial/by-id/usb-OktayCit_Irrigation_Controller_Config_Port_*-if00"))
    if matches:
        return matches[0]
    return DEFAULT_PORT


def main() -> None:
    parser = argparse.ArgumentParser(description="STM32 irrigation HTTP gateway")
    parser.add_argument("--host", default=os.environ.get("IRRIGATION_GATEWAY_HOST", "0.0.0.0"))
    parser.add_argument("--port", type=int, default=int(os.environ.get("IRRIGATION_GATEWAY_PORT", "8080")))
    parser.add_argument("--serial-port", default=auto_port())
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    args = parser.parse_args()

    GatewayHandler.client = IrrigationSerialClient(args.serial_port, args.baud)
    server = ThreadingHTTPServer((args.host, args.port), GatewayHandler)
    print(f"irrigation gateway listening on http://{args.host}:{args.port}", flush=True)
    print(f"serial port: {args.serial_port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
