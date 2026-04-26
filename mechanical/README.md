# Mechanical Design Target

This folder is reserved for the commercial mechanical design of the irrigation
dosing unit.

## Current Product Direction

The mechanical target is a 4 fertilizer + 1 acid Venturi/Z-bypass dosing skid
for the first commercial `Core` product.

Target envelope:

- 110 mm main-line compatibility
- Z-bypass installation, without forcing the full main-line flow through the unit
- 4 fertilizer channels + 1 acid channel
- 400 l/h average Venturi suction target per active dosing channel
- 1.5 atu minimum / 2 atu nominal field pressure assumption
- calibration tube connection per channel
- suction filter, service valve, check valve, and isolation valve per channel
- static mixer after injection
- pH/EC flow cell after complete mixing
- optional booster pump control output for low-pressure installations
- Schedule 80 PVC, CPVC, PVDF, or equivalent chemically resistant wetted parts

Future CAD work should start from this Venturi/Z-bypass architecture.

## Generated CAD

The first 40 m3/h FreeCAD concept model is now available:

- `VENTURI_PULSE_40M3H.md` documents the design assumptions.
- `venturi_pulse_40m3h_model.py` is the parametric FreeCAD source.
- `Irrigation_Venturi_Pulse_40m3h.FCStd` is the generated native FreeCAD model.
- `Irrigation_Venturi_Pulse_40m3h.step` is the generated STEP export.

This model uses a DN110 main line, DN50 Z-bypass, Venturi injector, bypass
master pulse valve, and 5 pulse dosing valves for 4 fertilizer channels plus
1 acid channel. The current sizing assumption is 40 m3/h main flow, 1.5/2 atu
field pressure, and 400 l/h average suction per active dosing channel.

The FreeCAD file also includes `Venturi_Printable_Assembly`, a separate
prototype-oriented 3D-printable Venturi concept with two clamshell body halves,
O-ring representation, bolt holes, suction port, and 22/24/26/28 mm throat
insert options.
