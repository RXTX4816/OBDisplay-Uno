# Operation

> For full screen layouts, field tables, and DTC decoding see [Screen Reference](Screen-Reference).

## Buttons

| Button | Action |
|---|---|
| LEFT / RIGHT | Previous / next menu |
| UP / DOWN | Previous / next screen within the current menu |
| SELECT (MID) | Context action (varies per screen) |

UP/DOWN auto-repeat after ~400 ms, then every ~120 ms.

## Menu order

```
Cockpit  ←→  Experimental  ←→  Debug  ←→  DTC  ←→  Settings
```

> Experimental and Debug only appear in `uno_debug` builds.

## Cockpit screens

### `0x17` Instruments cluster — 4 screens

| Screen | Content |
|---|---|
| 0 | Main dashboard: speed, RPM, oil temp, coolant, fuel level, ambient |
| 1 | Second dashboard: speed, oil/coolant temps, km remaining, L/100km, fuel level, oil level % |
| 2 | Bar gauges: coolant °C, oil temp °C, oil level %, fuel L |
| 3 | Warning summary: lists all active warnings, or ALL OK |

### `0x01` Engine ECU — 7 screens

| Screen | Content |
|---|---|
| 0 | Main dashboard: speed, RPM, coolant, load, throttle, voltage, lambda, intake air |
| 1 | OBD readiness bits (PASS/FAIL, polled every ~2 min) |
| 2 | Basic-setting requirement bits (Y/N) |
| 3 | Engine diagnostics: voltage, coolant, intake air, load, manifold pressure, lambda ×2 |
| 4 | Trip computer: L/100km, L/hr, km remaining, fuel burned |
| 5 | Bar gauges: coolant °C, engine load %, lambda %, battery V |
| 6 | Warning summary: lists all active warnings, or ALL OK |

Other addresses show "no data" in Cockpit — use the Experimental menu to browse raw groups.

## DTC menu

Cursor menu (UP/DOWN to move, SELECT to execute): **Read** → **Clear** → **Show**. Show opens a 4-DTCs-per-page list; UP/DOWN pages, SELECT returns.

## Settings menu

Cursor menu: **Exit** · **KWP:** (cycles ACK → Grp → Sensor) · **AutoRcn:** (Y/N) · **Fuel:** (0x17 only — saves fuel level to EEPROM for 0x01 trip computer range).
