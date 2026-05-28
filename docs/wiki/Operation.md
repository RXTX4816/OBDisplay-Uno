# Operation

## Navigation

| Button | Action |
|---|---|
| LEFT / RIGHT | Previous / next menu |
| UP / DOWN | Previous / next screen within the current menu |
| SELECT | Context action (varies per menu — see below) |

Menu order: **Cockpit → Experimental → Debug → DTC → Settings**

## Cockpit screen (ADDR_INSTRUMENTS 0x17)

Displays all vehicle data on a single 64×128 screen. All fields update simultaneously at ~177 ms intervals.

```
Row  0: SPD:xxx  RPM:xxxx
Row  1: CLT:xxx  OIL:xxx
Row  2: OLV:x OPR:x AMB:xx
Row  3: ODO:xxxxxxxxxx
Row  4: FUL:xx  FSR:xxxxx
Row  5: TIME:xxxxxxxxx
Row  6: L/100:xxxxx L/h:xxxx
Row  7: km:xxxxx  L:xxxxx
```

| Field | Description |
|---|---|
| `SPD` | Vehicle speed (km/h) |
| `RPM` | Engine speed (rev/min) |
| `CLT` | Coolant temperature (°C) |
| `OIL` | Oil temperature (°C) |
| `OLV` | Oil level OK — 1=OK, 0=low |
| `OPR` | Minimum oil pressure (bar) |
| `AMB` | Ambient temperature (°C) |
| `ODO` | Odometer (km) |
| `FUL` | Fuel level (L) |
| `FSR` | Fuel sender resistance (Ω) |
| `TIME` | ECU uptime (seconds) |
| `L/100` | Fuel consumption rate (L/100 km) since connection |
| `L/h` | Hourly fuel consumption (L/h) |
| `km` | Trip distance (km) since connection |
| `L` | Fuel burned (L) since connection |

SELECT has no action on the Cockpit screen — data is always live.

## Cockpit screen (ADDR_ENGINE 0x01)

Two screens, UP/DOWN to switch:

```
Screen 0:                Screen 1:
RPM:xxxx  V:xxxxx        TBa:xxxxx STa:xxxxx
T1:xx T2:xx T3:xx        mbar:xxxx bits:xxxxxxxx
LAM:xxx LAM2:xxx
LOAD:xxx
```

## Experimental screen

Shows raw measurement group values. SELECT toggles between measurement slots 0/1 and 2/3.

## Debug screen

Internal diagnostic readouts (only present in `uno_debug` builds with `OBD_EXPERIMENTAL_SCREENS` defined).

## DTC screen

| Screen | SELECT action |
|---|---|
| DTC screen 0 | Read fault codes from ECU |
| DTC screen 1 | Clear fault codes on ECU |

After reading, the count of stored DTCs is shown briefly. After clearing, a confirmation overlay appears.

> **Caution:** Do not clear DTCs on address `0x15` (airbag) if an electrical fault is present — on some affected ECUs this can deploy the airbag.

## Settings screen

| Screen | SELECT action |
|---|---|
| Settings screen 0 | Exit ECU session and return to setup |
| Settings screen 1 | Cycle KWP mode: ACK → GROUP → SENSOR |

### KWP modes

| Mode | Behaviour |
|---|---|
| ACK | Keepalive only — no sensor data |
| GROUP | Read measurement groups (most data) |
| SENSOR | Full sensor read (slower, more detailed) |
