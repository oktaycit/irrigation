"""Generate the 40 m3/h Venturi + pulse-valve dosing skid FreeCAD model.

Run with:
    printf '%s\n' \
        'import runpy; _=runpy.run_path("mechanical/venturi_pulse_40m3h_model.py", run_name="__main__")' \
        | /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c
"""

from __future__ import annotations

import math
from pathlib import Path

import FreeCAD as App
import Part


OUT_DIR = Path(__file__).resolve().parent
DOC_NAME = "Irrigation_Venturi_Pulse_40m3h"


PARAMS = {
    "system_flow_m3h": 40.0,
    "system_flow_lpm": 40.0 * 1000.0 / 60.0,
    "main_pipe_od_mm": 110.0,
    "main_pipe_id_mm": 101.6,
    "main_pipe_length_mm": 1800.0,
    "bypass_pipe_od_mm": 63.0,
    "bypass_pipe_id_mm": 54.0,
    "bypass_nominal_size": "DN50 / 2 inch",
    "venturi_throat_id_mm": 26.0,
    "venturi_nominal_flow_m3h": 8.0,
    "field_pressure_options_atu": "1.5 / 2.0",
    "nominal_field_pressure_atu": 2.0,
    "minimum_field_pressure_atu": 1.5,
    "dosing_channels": 5,
    "dosing_channel_average_lph": 400.0,
    "fertilizer_channel_target_lph": 400.0,
    "acid_channel_target_lph": 400.0,
    "simultaneous_5_channel_total_lph": 5.0 * 400.0,
    "single_channel_injection_ratio_at_40m3h_percent": 400.0 / 40000.0 * 100.0,
    "five_channel_injection_ratio_at_40m3h_percent": 5.0 * 400.0 / 40000.0 * 100.0,
    "main_design_velocity_mps": 40.0 / 3600.0 / (math.pi * (0.1016 / 2.0) ** 2),
}


COLORS = {
    "main": (0.12, 0.36, 0.72, 0.0),
    "bypass": (0.08, 0.56, 0.44, 0.0),
    "venturi": (0.93, 0.54, 0.12, 0.0),
    "valve": (0.12, 0.12, 0.12, 0.0),
    "pulse": (0.86, 0.12, 0.12, 0.0),
    "check": (0.35, 0.35, 0.35, 0.0),
    "sensor": (0.15, 0.64, 0.84, 0.0),
    "chemical": (0.58, 0.22, 0.74, 0.0),
    "acid": (0.92, 0.22, 0.24, 0.0),
    "frame": (0.42, 0.42, 0.42, 0.0),
    "transparent": (0.62, 0.85, 1.0, 0.55),
    "print_body": (0.91, 0.91, 0.84, 0.0),
    "print_insert": (0.98, 0.74, 0.22, 0.0),
    "seal": (0.02, 0.02, 0.02, 0.0),
    "fastener": (0.70, 0.70, 0.70, 0.0),
}


VENTURI_ASSEMBLY_NAMES = [
    "Bypass_manual_isolation_inlet",
    "Bypass_manual_isolation_inlet_handle",
    "Bypass_master_pulse_valve",
    "Bypass_master_pulse_valve_coil",
    "Bypass_pressure_gauge_stem",
    "Bypass_pressure_gauge_dial",
    "Bypass_pressure_gauge_needle",
    "Bypass_set_for_400_l/h_avg_su",
    "Bypass_Y_strainer",
    "DN50_Venturi_injector_8m3h_bypass",
    "Venturi_suction_port_to_pulse_manifold",
    "Bypass_manual_isolation_return",
    "Bypass_manual_isolation_return_handle",
]

PRINTABLE_VENTURI_NAMES = [
    "Printable_venturi_upper_half",
    "Printable_venturi_lower_half",
    "Printable_venturi_oring_upper",
    "Printable_venturi_oring_lower",
    "Printable_venturi_suction_port",
    "Printable_venturi_suction_thread",
    "Printable_venturi_flow_arrow",
    "Printable_throat_insert_22mm",
    "Printable_throat_insert_24mm",
    "Printable_throat_insert_26mm",
    "Printable_throat_insert_28mm",
]

PRINTABLE_STL_NAMES = [
    "Printable_venturi_upper_half",
    "Printable_venturi_lower_half",
    "Printable_venturi_suction_port",
    "Printable_venturi_suction_thread",
    "Printable_throat_insert_22mm",
    "Printable_throat_insert_24mm",
    "Printable_throat_insert_26mm",
    "Printable_throat_insert_28mm",
]


def vec(x: float, y: float, z: float) -> App.Vector:
    return App.Vector(float(x), float(y), float(z))


def add_shape(doc: App.Document, name: str, shape: Part.Shape, color=None):
    obj = doc.addObject("Part::Feature", name)
    obj.Shape = shape
    view = getattr(obj, "ViewObject", None)
    if color is not None and view is not None:
        view.ShapeColor = color[:3]
        if len(color) > 3:
            view.Transparency = int(color[3] * 100)
    return obj


def cylinder_between(p1, p2, radius: float) -> Part.Shape:
    p1 = App.Vector(p1)
    p2 = App.Vector(p2)
    axis = p2 - p1
    return Part.makeCylinder(radius, axis.Length, p1, axis)


def cone_between(p1, p2, radius1: float, radius2: float) -> Part.Shape:
    p1 = App.Vector(p1)
    p2 = App.Vector(p2)
    axis = p2 - p1
    return Part.makeCone(radius1, radius2, axis.Length, p1, axis)


def pipe_between(doc, name: str, p1, p2, od: float, id_: float, color):
    p1 = App.Vector(p1)
    p2 = App.Vector(p2)
    axis = p2 - p1
    unit = axis.normalize()
    outer = Part.makeCylinder(od / 2.0, axis.Length, p1, axis)
    inner = Part.makeCylinder(id_ / 2.0, axis.Length + 2.0, p1 - unit, axis)
    return add_shape(doc, name, outer.cut(inner), color)


def ring_between(doc, name: str, p1, p2, od: float, id_: float, color):
    return pipe_between(doc, name, p1, p2, od, id_, color)


def box_center(doc, name: str, center, size, color):
    cx, cy, cz = center
    sx, sy, sz = size
    shape = Part.makeBox(sx, sy, sz, vec(cx - sx / 2.0, cy - sy / 2.0, cz - sz / 2.0))
    return add_shape(doc, name, shape, color)


def add_label(doc, text: str, pos):
    obj = doc.addObject("App::Annotation", text[:28].replace(" ", "_"))
    obj.LabelText = text
    obj.Position = vec(*pos)
    return obj


def add_valve(doc, name: str, center, axis: str, nominal_od: float, pulse=False):
    color = COLORS["pulse"] if pulse else COLORS["valve"]
    cx, cy, cz = center
    if axis == "X":
        body = Part.makeBox(110, 86, 76, vec(cx - 55, cy - 43, cz - 38))
        bore = cylinder_between(vec(cx - 65, cy, cz), vec(cx + 65, cy, cz), nominal_od / 2.0)
        bonnet = cylinder_between(vec(cx, cy, cz + 36), vec(cx, cy, cz + 112), 24)
    else:
        body = Part.makeBox(76, 86, 96, vec(cx - 38, cy - 43, cz - 48))
        bore = cylinder_between(vec(cx, cy - 65, cz), vec(cx, cy + 65, cz), nominal_od / 2.0)
        bonnet = cylinder_between(vec(cx, cy, cz + 40), vec(cx, cy, cz + 104), 20)
    shape = body.cut(bore).fuse(bonnet)
    obj = add_shape(doc, name, shape, color)
    if pulse:
        coil = box_center(doc, f"{name}_coil", (cx, cy, cz + 122), (58, 48, 30), COLORS["pulse"])
        coil.Label = f"{name} solenoid coil"
    return obj


def add_ball_valve(doc, name: str, center, axis: str, nominal_od: float):
    obj = add_valve(doc, name, center, axis, nominal_od, pulse=False)
    cx, cy, cz = center
    if axis == "X":
        handle = Part.makeBox(120, 16, 10, vec(cx - 10, cy - 8, cz + 105))
    else:
        handle = Part.makeBox(16, 120, 10, vec(cx - 8, cy - 10, cz + 100))
    add_shape(doc, f"{name}_handle", handle, COLORS["valve"])
    return obj


def add_check_valve(doc, name: str, center, axis: str, nominal_od: float):
    cx, cy, cz = center
    if axis == "Y":
        inlet = cone_between(vec(cx, cy - 36, cz), vec(cx, cy, cz), nominal_od / 2.0, nominal_od * 0.35)
        outlet = cone_between(vec(cx, cy, cz), vec(cx, cy + 36, cz), nominal_od * 0.35, nominal_od / 2.0)
    else:
        inlet = cone_between(vec(cx - 36, cy, cz), vec(cx, cy, cz), nominal_od / 2.0, nominal_od * 0.35)
        outlet = cone_between(vec(cx, cy, cz), vec(cx + 36, cy, cz), nominal_od * 0.35, nominal_od / 2.0)
    return add_shape(doc, name, inlet.fuse(outlet), COLORS["check"])


def add_pressure_gauge(doc, name: str, center, label: str):
    cx, cy, cz = center
    stem = cylinder_between(vec(cx, cy, cz), vec(cx, cy, cz + 58), 6)
    dial = Part.makeCylinder(34, 10, vec(cx, cy - 5, cz + 72), vec(0, 1, 0))
    needle = Part.makeBox(28, 2, 3, vec(cx - 2, cy - 12, cz + 71))
    needle.rotate(vec(cx, cy - 11, cz + 72), vec(0, 1, 0), 32)
    add_shape(doc, f"{name}_stem", stem, COLORS["sensor"])
    add_shape(doc, f"{name}_dial", dial, COLORS["sensor"])
    add_shape(doc, f"{name}_needle", needle, COLORS["pulse"])
    add_label(doc, label, (cx - 58, cy - 76, cz + 118))


def add_venturi(doc, center_x: float, y: float, z: float):
    inlet_r = PARAMS["bypass_pipe_id_mm"] / 2.0
    throat_r = PARAMS["venturi_throat_id_mm"] / 2.0
    x0 = center_x - 170
    parts = [
        cone_between(vec(x0, y, z), vec(x0 + 70, y, z), inlet_r, throat_r),
        cylinder_between(vec(x0 + 70, y, z), vec(x0 + 130, y, z), throat_r),
        cone_between(vec(x0 + 130, y, z), vec(x0 + 340, y, z), throat_r, inlet_r),
    ]
    venturi = parts[0].fuse(parts[1]).fuse(parts[2])
    obj = add_shape(doc, "DN50_Venturi_injector_8m3h_bypass", venturi, COLORS["venturi"])
    suction = pipe_between(
        doc,
        "Venturi_suction_port_to_pulse_manifold",
        vec(center_x - 70, y - 12, z + 8),
        vec(center_x - 70, y - 150, z + 8),
        16,
        10,
        COLORS["chemical"],
    )
    suction.Label = "Venturi suction port"
    return obj


def venturi_internal_void(x0: float, y: float, z: float, throat_id: float) -> Part.Shape:
    inlet_r = PARAMS["bypass_pipe_id_mm"] / 2.0
    throat_r = throat_id / 2.0
    return (
        cone_between(vec(x0, y, z), vec(x0 + 95, y, z), inlet_r, throat_r)
        .fuse(cylinder_between(vec(x0 + 95, y, z), vec(x0 + 145, y, z), throat_r))
        .fuse(cone_between(vec(x0 + 145, y, z), vec(x0 + 390, y, z), throat_r, inlet_r))
    )


def printable_venturi_body(x0: float, y: float, z: float) -> Part.Shape:
    outer = (
        cone_between(vec(x0, y, z), vec(x0 + 95, y, z), 50, 36)
        .fuse(cylinder_between(vec(x0 + 95, y, z), vec(x0 + 145, y, z), 36))
        .fuse(cone_between(vec(x0 + 145, y, z), vec(x0 + 390, y, z), 36, 50))
    )

    for x in (x0 + 18, x0 + 115, x0 + 230, x0 + 372):
        outer = outer.fuse(Part.makeBox(58, 178, 28, vec(x - 29, y - 89, z - 14)))

    inner = venturi_internal_void(x0 - 1, y, z, PARAMS["venturi_throat_id_mm"])
    inner = inner.fuse(cylinder_between(vec(x0 + 118, y - 6, z + 4), vec(x0 + 118, y - 112, z + 76), 7))

    body = outer.cut(inner)
    for x in (x0 + 18, x0 + 115, x0 + 230, x0 + 372):
        for bolt_y in (y - 70, y + 70):
            body = body.cut(cylinder_between(vec(x, bolt_y, z - 90), vec(x, bolt_y, z + 90), 4.3))
    return body


def add_printable_venturi(doc, center_x: float, y: float, z: float):
    x0 = center_x - 195
    body = printable_venturi_body(x0, y, z)
    upper_box = Part.makeBox(560, 260, 130, vec(x0 - 70, y - 130, z))
    lower_box = Part.makeBox(560, 260, 130, vec(x0 - 70, y - 130, z - 130))

    upper = add_shape(doc, "Printable_venturi_upper_half", body.common(upper_box), COLORS["print_body"])
    upper.Label = "Printable venturi upper half"
    lower = add_shape(doc, "Printable_venturi_lower_half", body.common(lower_box), COLORS["print_body"])
    lower.Label = "Printable venturi lower half"

    upper_seal = Part.makeBox(435, 166, 4, vec(x0 - 22, y - 83, z + 2))
    lower_seal = Part.makeBox(435, 166, 4, vec(x0 - 22, y - 83, z - 6))
    add_shape(doc, "Printable_venturi_oring_upper", upper_seal.cut(Part.makeBox(390, 122, 8, vec(x0, y - 61, z))), COLORS["seal"])
    add_shape(doc, "Printable_venturi_oring_lower", lower_seal.cut(Part.makeBox(390, 122, 8, vec(x0, y - 61, z - 8))), COLORS["seal"])

    suction = pipe_between(
        doc,
        "Printable_venturi_suction_port",
        vec(x0 + 118, y - 8, z + 5),
        vec(x0 + 118, y - 128, z + 86),
        26,
        12,
        COLORS["print_body"],
    )
    suction.Label = "Printable suction port, tap for fitting"
    thread = cylinder_between(vec(x0 + 118, y - 130, z + 86), vec(x0 + 118, y - 160, z + 106), 14)
    add_shape(doc, "Printable_venturi_suction_thread", thread, COLORS["fastener"])

    arrow_body = cylinder_between(vec(x0 - 30, y + 118, z + 78), vec(x0 + 110, y + 118, z + 78), 5)
    arrow_head = cone_between(vec(x0 + 110, y + 118, z + 78), vec(x0 + 144, y + 118, z + 78), 15, 0)
    add_shape(doc, "Printable_venturi_flow_arrow", arrow_body.fuse(arrow_head), COLORS["pulse"])

    for i, throat_id in enumerate((22, 24, 26, 28)):
        insert_x = x0 + 30 + i * 78
        sleeve = pipe_between(
            doc,
            f"Printable_throat_insert_{throat_id}mm",
            vec(insert_x, y + 250, z - 18),
            vec(insert_x + 48, y + 250, z - 18),
            34,
            throat_id,
            COLORS["print_insert"],
        )
        sleeve.Label = f"Changeable throat insert {throat_id} mm"
        add_label(doc, f"{throat_id} mm", (insert_x - 12, y + 286, z + 16))

    add_label(doc, "Printable 2-piece Venturi: clamshell + O-ring + inserts", (x0 - 20, y - 170, z + 118))
    add_label(doc, "Bolt holes: M8 clearance, stainless fasteners", (x0 - 18, y + 104, z + 114))
    add_group(doc, "Venturi_Printable_Assembly", PRINTABLE_VENTURI_NAMES, "Venturi Printable Assembly")


def add_static_mixer(doc, center_x: float, y: float, z: float):
    body = pipe_between(
        doc,
        "Static_mixer_after_injection",
        vec(center_x - 130, y, z),
        vec(center_x + 130, y, z),
        PARAMS["main_pipe_od_mm"] + 8,
        PARAMS["main_pipe_id_mm"],
        COLORS["transparent"],
    )
    for i in range(6):
        x = center_x - 95 + i * 38
        vane = Part.makeBox(70, 8, 52, vec(x - 35, y - 4, z - 26))
        vane.rotate(vec(x, y, z), vec(1, 0, 0), 35 if i % 2 == 0 else -35)
        add_shape(doc, f"Static_mixer_vane_{i + 1}", vane, COLORS["sensor"])
    return body


def add_flow_cell(doc, center):
    cx, cy, cz = center
    body = box_center(doc, "pH_EC_flow_cell_after_mixer", center, (160, 62, 78), COLORS["transparent"])
    pipe_between(doc, "Flow_cell_sample_in", vec(cx - 80, cy - 90, cz), vec(cx - 80, cy - 31, cz), 12, 8, COLORS["sensor"])
    pipe_between(doc, "Flow_cell_sample_out", vec(cx + 80, cy - 31, cz), vec(cx + 80, cy - 90, cz), 12, 8, COLORS["sensor"])
    for i, label in enumerate(("pH", "EC")):
        cylinder = cylinder_between(vec(cx - 32 + i * 64, cy, cz + 38), vec(cx - 32 + i * 64, cy, cz + 105), 14)
        add_shape(doc, f"{label}_probe", cylinder, COLORS["sensor"])
    return body


def add_channel(doc, index: int, name: str, x: float, color):
    tank_y = -560
    channel_y = -330
    z = 640
    tank = cylinder_between(vec(x, tank_y, 160), vec(x, tank_y, 430), 42)
    add_shape(doc, f"{name}_chemical_canister", tank, color)
    pipe_between(doc, f"{name}_suction_tube", vec(x, tank_y, 430), vec(x, channel_y - 92, z), 12, 8, color)
    filter_body = cylinder_between(vec(x, channel_y - 120, z), vec(x, channel_y - 80, z), 15)
    add_shape(doc, f"{name}_suction_filter", filter_body, COLORS["check"])
    add_ball_valve(doc, f"{name}_service_valve", (x, channel_y - 28, z), "Y", 18)
    add_valve(doc, f"{name}_pulse_dosing_valve", (x, channel_y + 58, z), "Y", 18, pulse=True)
    add_check_valve(doc, f"{name}_check_valve", (x, channel_y + 148, z), "Y", 18)
    pipe_between(doc, f"{name}_feed_to_suction_manifold", vec(x, channel_y + 185, z), vec(-70, -150, 618), 12, 8, color)
    tube = cylinder_between(vec(x + 44, channel_y - 5, z - 84), vec(x + 44, channel_y - 5, z + 76), 10)
    add_shape(doc, f"{name}_calibration_tube", tube, COLORS["transparent"])
    add_label(
        doc,
        f"{name.replace('_', ' ')} 400 l/h avg",
        (x - 70, tank_y - 52, 460),
    )


def add_frame(doc):
    z = 62
    for y in (-650, 180):
        box_center(doc, f"Skid_long_rail_y_{y}", (0, y, z), (1900, 34, 34), COLORS["frame"])
    for x in (-850, -300, 300, 850):
        box_center(doc, f"Skid_cross_rail_x_{x}", (x, -235, z), (34, 865, 34), COLORS["frame"])
    for x in (-850, 850):
        for y in (-650, 180):
            box_center(doc, f"Skid_foot_{x}_{y}", (x, y, 22), (110, 90, 28), COLORS["frame"])


def add_group(doc, name: str, object_names: list[str], label: str | None = None):
    group = doc.addObject("App::DocumentObjectGroup", name)
    if label is not None:
        group.Label = label
    for object_name in object_names:
        obj = doc.getObject(object_name)
        if obj is not None:
            group.addObject(obj)
    return group


def build_model():
    doc = App.newDocument(DOC_NAME)

    main_z = 320
    bypass_z = 610
    main_y = 0

    add_frame(doc)

    main_start = vec(-900, main_y, main_z)
    main_end = vec(900, main_y, main_z)
    pipe_between(
        doc,
        "DN110_main_line_40m3h",
        main_start,
        main_end,
        PARAMS["main_pipe_od_mm"],
        PARAMS["main_pipe_id_mm"],
        COLORS["main"],
    )
    ring_between(doc, "Inlet_flange_DN110", vec(-910, 0, main_z), vec(-870, 0, main_z), 170, PARAMS["main_pipe_id_mm"], COLORS["valve"])
    ring_between(doc, "Outlet_flange_DN110", vec(870, 0, main_z), vec(910, 0, main_z), 170, PARAMS["main_pipe_id_mm"], COLORS["valve"])
    add_ball_valve(doc, "Main_inlet_isolation_valve", (-720, 0, main_z), "X", PARAMS["main_pipe_id_mm"])
    add_ball_valve(doc, "Main_outlet_isolation_valve", (720, 0, main_z), "X", PARAMS["main_pipe_id_mm"])
    add_pressure_gauge(doc, "Main_line_pressure_gauge", (-610, -62, main_z + 35), "Field pressure 1.5/2 atu")

    tap_left = vec(-540, main_y, main_z)
    tap_right = vec(540, main_y, main_z)
    pipe_between(doc, "Z_bypass_left_riser_DN50", tap_left, vec(-540, main_y, bypass_z), PARAMS["bypass_pipe_od_mm"], PARAMS["bypass_pipe_id_mm"], COLORS["bypass"])
    pipe_between(doc, "Z_bypass_upper_run_DN50", vec(-540, main_y, bypass_z), vec(540, main_y, bypass_z), PARAMS["bypass_pipe_od_mm"], PARAMS["bypass_pipe_id_mm"], COLORS["bypass"])
    pipe_between(doc, "Z_bypass_right_riser_DN50", vec(540, main_y, bypass_z), tap_right, PARAMS["bypass_pipe_od_mm"], PARAMS["bypass_pipe_id_mm"], COLORS["bypass"])
    ring_between(doc, "Bypass_inlet_tee_reinforcement", vec(-565, 0, main_z), vec(-515, 0, main_z), 150, PARAMS["main_pipe_id_mm"], COLORS["bypass"])
    ring_between(doc, "Bypass_return_tee_reinforcement", vec(515, 0, main_z), vec(565, 0, main_z), 150, PARAMS["main_pipe_id_mm"], COLORS["bypass"])

    add_ball_valve(doc, "Bypass_manual_isolation_inlet", (-390, 0, bypass_z), "X", PARAMS["bypass_pipe_id_mm"])
    add_valve(doc, "Bypass_master_pulse_valve", (-230, 0, bypass_z), "X", PARAMS["bypass_pipe_id_mm"], pulse=True)
    add_pressure_gauge(doc, "Bypass_pressure_gauge", (-160, 72, bypass_z + 30), "Bypass set for 400 l/h avg suction")
    strainer = cylinder_between(vec(-95, 0, bypass_z), vec(-35, 0, bypass_z), 40)
    add_shape(doc, "Bypass_Y_strainer", strainer, COLORS["check"])
    add_venturi(doc, 110, 0, bypass_z)
    add_ball_valve(doc, "Bypass_manual_isolation_return", (420, 0, bypass_z), "X", PARAMS["bypass_pipe_id_mm"])

    add_static_mixer(doc, 360, 0, main_z)
    add_flow_cell(doc, (630, -150, 375))
    pipe_between(doc, "Flow_cell_sample_takeoff", vec(560, 0, main_z + 45), vec(550, -150, 375), 14, 8, COLORS["sensor"])
    pipe_between(doc, "Flow_cell_sample_return", vec(710, -150, 375), vec(735, 0, main_z + 30), 14, 8, COLORS["sensor"])

    channel_names = ["Fert_A", "Fert_B", "Fert_C", "Fert_D", "Acid"]
    channel_colors = [COLORS["chemical"], COLORS["chemical"], COLORS["chemical"], COLORS["chemical"], COLORS["acid"]]
    for i, name in enumerate(channel_names):
        add_channel(doc, i + 1, name, -420 + i * 180, channel_colors[i])

    add_printable_venturi(doc, -280, 470, 560)

    controller = box_center(doc, "IP65_control_box_STM32_pulse_outputs", (690, -520, 520), (260, 90, 190), COLORS["sensor"])
    controller.Label = "IP65 controller: 5 dosing pulse outputs + bypass master pulse"

    add_label(doc, "40 m3/h DN110 main line", (-830, 82, 435))
    add_label(doc, "DN50 Z-bypass with Venturi injector", (-505, 76, 725))
    add_label(doc, "5 pulse dosing valves: 400 l/h avg each", (-455, -450, 765))
    add_label(doc, "Static mixer + pH/EC flow cell after injection", (235, 110, 460))
    add_label(doc, f"Main velocity approx {PARAMS['main_design_velocity_mps']:.2f} m/s", (-835, 82, 390))
    add_label(doc, "Pressure assumption: 1.5 atu min / 2 atu nominal", (-60, 118, 780))
    add_group(doc, "Venturi_Assembly", VENTURI_ASSEMBLY_NAMES, "Venturi Assembly")

    spreadsheet = doc.addObject("Spreadsheet::Sheet", "Design_parameters")
    for row, (key, value) in enumerate(PARAMS.items(), start=1):
        spreadsheet.set(f"A{row}", key)
        spreadsheet.set(f"B{row}", str(value))

    doc.recompute()

    fcstd_path = OUT_DIR / f"{DOC_NAME}.FCStd"
    step_path = OUT_DIR / f"{DOC_NAME}.step"
    doc.saveAs(str(fcstd_path))

    try:
        import Import

        export_objects = [obj for obj in doc.Objects if hasattr(obj, "Shape")]
        Import.export(export_objects, str(step_path))
    except Exception as exc:  # pragma: no cover - FreeCAD export availability differs.
        print(f"STEP export skipped: {exc}")

    print(f"Saved {fcstd_path}")
    if step_path.exists():
        print(f"Saved {step_path}")

    try:
        import Mesh

        stl_dir = OUT_DIR / "printable_venturi_stl"
        stl_dir.mkdir(exist_ok=True)
        for object_name in PRINTABLE_STL_NAMES:
            obj = doc.getObject(object_name)
            if obj is not None:
                Mesh.export([obj], str(stl_dir / f"{object_name}.stl"))
        printable_objects = [doc.getObject(name) for name in PRINTABLE_STL_NAMES]
        printable_objects = [obj for obj in printable_objects if obj is not None]
        if printable_objects:
            Mesh.export(printable_objects, str(stl_dir / "Venturi_Printable_Assembly_reference.stl"))
        print(f"Saved printable STL files under {stl_dir}")
    except Exception as exc:  # pragma: no cover - FreeCAD export availability differs.
        print(f"STL export skipped: {exc}")


if __name__ == "__main__":
    build_model()
