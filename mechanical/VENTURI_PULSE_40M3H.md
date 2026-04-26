# 40 m3/h Venturi + Pulse Valve Skid

This is the first parametric CAD target for the commercial irrigation dosing
unit.

## Hydraulic Target

- System capacity: 40 m3/h total main-line flow
- Equivalent flow: 666.7 l/min
- Main line: DN110 compatible pipe, modeled as 110 mm OD / 101.6 mm ID
- Main-line design velocity: approximately 1.37 m/s
- Bypass architecture: DN50 Z-bypass, so the full 40 m3/h does not pass through
  the Venturi body
- Venturi branch target: nominal 8 m3/h bypass injector capacity
- Field pressure assumption: 1.5 atu minimum / 2 atu nominal
- Dosing channels: 4 fertilizer + 1 acid
- Average Venturi suction target: 400 l/h per active dosing channel
- Single-channel injection ratio at 40 m3/h: 1.0%
- 5-channel simultaneous theoretical total: 2000 l/h, equal to 5.0% of
  40 m3/h main flow

## Modeled Components

- DN110 main line with inlet and outlet isolation valves
- DN50 Z-bypass risers and upper bypass run
- Bypass manual isolation valves
- Bypass master pulse valve before the injector
- Main-line and bypass pressure gauge placeholders for the 1.5/2 atu design
  points
- Y-strainer before the Venturi
- DN50 Venturi injector with 26 mm throat representation
- 5-channel pulse dosing valve bank
- Suction filter, service valve, pulse valve, check valve, and calibration tube
  per dosing channel
- Static mixer after injection return
- pH/EC sample flow cell after the static mixer
- IP65 control box placeholder for STM32 pulse outputs
- Skid frame rails and feet
- Separate printable Venturi concept: two-piece clamshell body, O-ring seal
  representation, M8 clearance bolt holes, suction fitting boss, and
  22/24/26/28 mm changeable throat inserts

## Files

- `venturi_pulse_40m3h_model.py`: source of truth for the parametric FreeCAD
  model
- `Irrigation_Venturi_Pulse_40m3h.FCStd`: generated native FreeCAD model
- `Irrigation_Venturi_Pulse_40m3h.step`: generated STEP export for sharing
- `printable_venturi_stl/`: generated STL files for the printable prototype
  pieces

## FreeCAD Tree Organization

- `Venturi_Assembly`: grouped Venturi package containing the nearby bypass
  isolation valves, bypass master pulse valve, bypass pressure gauge,
  Y-strainer, Venturi body, and suction port.
- `Venturi_Printable_Assembly`: grouped 3D-printable Venturi concept containing
  the upper/lower clamshell halves, O-ring representation, suction port, and
  changeable throat insert set.

## Printable STL Parts

- `Printable_venturi_upper_half.stl`
- `Printable_venturi_lower_half.stl`
- `Printable_venturi_suction_port.stl`
- `Printable_venturi_suction_thread.stl`
- `Printable_throat_insert_22mm.stl`
- `Printable_throat_insert_24mm.stl`
- `Printable_throat_insert_26mm.stl`
- `Printable_throat_insert_28mm.stl`
- `Venturi_Printable_Assembly_reference.stl` is for visual reference, not the
  preferred print file.

## Regeneration

Run from the repository root:

```sh
printf '%s\n' \
  'import runpy; _=runpy.run_path("mechanical/venturi_pulse_40m3h_model.py", run_name="__main__")' \
  | /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c
```

The script stores the main sizing values in the `PARAMS` dictionary and also
writes them into a `Design_parameters` spreadsheet inside the FreeCAD document.

## Engineering Notes

- This is a layout and packaging model, not a final pressure-rated fabrication
  drawing.
- The current dosing target assumes one active channel averages 400 l/h. If all
  five pulse valves are allowed to dose at the same time, the theoretical total
  chemical flow is 2000 l/h, so firmware should normally limit simultaneous
  duty or recipe overlap.
- The DN50 Venturi branch is intentionally modeled as a bypass injector. Final
  injector selection must be matched to the 1.5/2 atu field pressure cases,
  actual pressure differential, desired 400 l/h suction curve, fertilizer
  viscosity, and acceptable head loss.
- Wetted materials should remain Schedule 80 PVC, CPVC, PVDF, or equivalent
  chemically resistant parts.

## 3D Printing Notes

- Treat the printable Venturi as a prototype/test article first, not a final
  pressure-rated field component.
- Preferred prototype path: print the two clamshell halves, install an
  EPDM/Viton seal, use stainless M8 fasteners, and test the 22/24/26/28 mm
  inserts against the 400 l/h suction target at 1.5 atu and 2 atu.
- Avoid PLA for wetted chemical service. PETG/ASA can be used for early water
  tests; PA12 SLS/MJF, PP, PVDF, or machined chemical-resistant plastic is more
  appropriate for serious field validation.
- Pressure-test with water above normal working pressure before any fertilizer
  or acid exposure.
