# Irrigation Gateway

Lightweight Linux edge gateway for the STM32 irrigation controller.

The first MVP intentionally uses only Python's standard library so it can run on
small boards such as Banana Pi BPI-M1 without extra package setup.

## Default Serial Port

```text
/dev/serial/by-id/usb-OktayCit_Irrigation_Controller_Config_Port_207E377C5232-if00
```

Override it with:

```bash
IRRIGATION_SERIAL_PORT=/dev/ttyACM1 python3 app.py
```

## Local API

```text
GET /health
GET /api/ping
GET /api/device
GET /api/telemetry
GET /api/fault
GET /api/runtime
GET /api/programs
GET /api/programs/<id>
POST /api/programs/<id>
```

The device, telemetry, fault, and runtime endpoints proxy the STM32 `INFO`,
`TELEMETRY`, `FAULT`, and `RUNTIME` USB CDC commands and return both parsed JSON
and raw controller response lines.

`POST /api/programs/<id>` accepts JSON matching the STM32 `SET` fields:

```json
{
  "enabled": 1,
  "start_hhmm": 630,
  "end_hhmm": 730,
  "valve_mask": 3,
  "irrigation_min": 12,
  "wait_min": 2,
  "repeat_count": 1,
  "days_mask": 62,
  "ph_x100": 640,
  "ec_x100": 175,
  "fert_a_pct": 25,
  "fert_b_pct": 50,
  "fert_c_pct": 20,
  "fert_d_pct": 30,
  "pre_flush_sec": 60,
  "post_flush_sec": 120
}
```
