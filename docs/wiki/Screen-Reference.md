# Screen Reference

## Navigation overview

The display is a 64×128 px portrait panel. When connected, content is organised into **menus** (horizontal) and **screens** (vertical within a menu). The cockpit screens use a pixel-doubled big font; all other screens use the small font (10 columns × 16 rows).

```
← LEFT / RIGHT →    switches menu
↑ UP / DOWN ↓       switches screen within the current menu
SELECT              context action (varies per screen)
```

### Menu order

```
Cockpit  ←→  Experimental  ←→  Debug  ←→  DTC  ←→  Settings
```

> Experimental and Debug screens only appear in `uno_debug` builds compiled with `-D OBD_EXPERIMENTAL_SCREENS`.

---

## Warning flash

When a new warning condition is detected (oil pressure, overheating, low fuel, etc.) the display briefly interrupts any screen with a flashing overlay for ~3 seconds:

```
CRIT          ← severity (CRIT / CAUT / ALRT)

!!! OIL PR    ← warning name, split across two rows
```

The overlay alternates on/off at ~177 ms intervals. If multiple warnings fire simultaneously, the flash cycles through them. After 3 seconds the normal screen resumes.

---

## Cockpit

Live sensor data displayed in a pixel-doubled big font (12 px/char, 16 px/row). No SELECT action — data updates automatically every ~177 ms.

### Instruments cluster (address `0x17`)

**Screen 0** — main dashboard, six values:

```
130
2200

99 O
99 C

33 L
20AIR
```

| Row | Field | Format | Notes |
|---|---|---|---|
| 0 | Vehicle speed | `NNN` km/h | |
| 1 | Engine RPM | `NNNN` | |
| 2 | Oil temperature | `NN O` | Shows `-WARN-` at ≥ 100 °C |
| 3 | Coolant temperature | `NN C` | Shows `-WARN-` at ≥ 100 °C |
| 4 | Fuel level | `NN L` | Smoothed (EMA filtered) |
| 5 | Ambient temperature | `NNAIR` | |

**Screen 1** — trip computer dashboard:

```
130
99 O
99 C
450K
8.3L
33 F
```

| Row | Field | Format | Notes |
|---|---|---|---|
| 0 | Vehicle speed | `NNN` km/h | |
| 1 | Oil temperature | `NN O` | Shows `-WARN-` at ≥ 100 °C |
| 2 | Coolant temperature | `NN C` | Shows `-WARN-` at ≥ 100 °C |
| 3 | Estimated range | `NNNK` km | Shows `---` until fuel burn data is available |
| 4 | Fuel consumption | `N.NL` L/100 km | |
| 5 | Fuel level | `NN F` | Smoothed |

### Engine ECU (address `0x01`)

Groups polled every cycle: **1** (RPM, coolant-proxy, lambda, basic-setting bits) and **5** (vehicle speed, engine load). Groups **4** (voltage, coolant, intake air) and **3** (pressure, throttle angle) are polled on a slower rotation.

**Screen 0** — main dashboard (big font):

```
120
1200
88 C
35%
5.5T
12.3V
3%
24I
```

| Row | Field | Source | Notes |
|---|---|---|---|
| 0 | Vehicle speed | Group 5 | km/h |
| 1 | Engine RPM | Group 1 | |
| 2 | Coolant temperature | Group 4 | `NN C`; `-WARN-` at ≥ 100 °C |
| 3 | Engine load | Group 5/6 | `NN%` |
| 4 | Throttle body angle | Group 3 | `N.NT` ×10 |
| 5 | Battery voltage | Group 4 | `NN.NV` |
| 6 | Lambda controller | Group 1 | `NN%`; spec −15 to +15 % |
| 7 | Intake air temperature | Group 4 | `NNI` °C |

**Screen 1** — OBD readiness bits (small font). Polled automatically every ~2 minutes. Navigating to this screen triggers an immediate poll.

```
EGR   PASS
O2Htr PASS
O2Sns FAIL
A/C   PASS
2Air  PASS
Evap  PASS
CtHtr PASS
Cat   PASS
```

1 = FAIL (test not complete), 0 = PASS.

**Screen 2** — basic-setting requirement bits (small font). Shows whether conditions for ECU basic settings are currently met (Y/N).

| Bit | Label | Meaning |
|---|---|---|
| 7 | `CoolWarm` | Coolant above 80 °C |
| 6 | `RPM<2000` | Engine speed below 2000 rpm |
| 5 | `TBclosed` | Throttle valve closed |
| 4 | `LambdaOK` | Lambda regulation active |
| 3 | `Idle` | Idle state active |
| 2 | `A/Coff` | A/C compressor off |
| 1 | `Cat>300` | Catalyst above 300 °C |
| 0 | `NoFault` | No self-diagnosis malfunction |

**Screen 3** — engine diagnostics (small font):

```
V:  12.3
Co: 88
IA: 24
Ld: 35
mb: 1012
L1: 3
L2: -1
```

| Label | Field | Notes |
|---|---|---|
| `V:` | Battery voltage | ×10, displayed as X.X V |
| `Co:` | Coolant temperature | °C (group 4) |
| `IA:` | Intake air temperature | °C (group 4) |
| `Ld:` | Engine load | % |
| `mb:` | Manifold pressure | mbar (group 3) |
| `L1:` | Lambda controller (group 1) | % |
| `L2:` | Heights correction lambda (group 6) | % |

**Screen 4** — trip computer (big font):

```
8.3L
8.4H
450K
12 B
```

| Row | Field | Notes |
|---|---|---|
| 0 | Fuel per 100 km | `X.XL`; `---L` until driving |
| 1 | Fuel per hour | `X.XH` L/hr |
| 2 | km remaining | `NNNK`; `---K` until fuel start set (see Settings→Fuel) |
| 3 | Fuel burned | `NN B` L since session start |

The fuel algorithm computes consumption from `RPM × engine load` (calibrated for ~1.4 L petrol; see `ENGINE01_FUEL_DENOM` in `Config.h`).

**Screen 5** — bar gauges:

Four vertical bars (fills from bottom):

| Bar | Field | Range | Tick |
|---|---|---|---|
| C | Coolant °C | 0–120 | 90 °C |
| L | Engine load | 0–100 % | 80 % |
| λ | Lambda % | −15 to +15 % | 0 % |
| V | Battery V | 10.0–16.0 V | 12.0 V |

---

## Experimental

Raw measurement group values. Only visible in `uno_debug` builds.

```
Grp: XX

V1: XXXXXXX
U1: UUUUUU

V2: XXXXXXX
U2: UUUUUU

V3: XXXXXXX
U3: UUUUUU

V4: XXXXXXX
U4: UUUUUU
```

- **Grp** — the measurement group number currently being read
- **V1–V4** — decoded value for each of the 4 measurement slots
- **U1–U4** — unit string for each slot (e.g. `km/h`, `rpm`, `°C`)

**SELECT** toggles between slot pairs: slots 0/1 ↔ slots 2/3.

---

## Debug

Internal diagnostics. Only visible in `uno_debug` builds.

```
Con: 1        (software serial listening)
Ava: 0        (bytes available in RX buffer)
BC: 42        (KWP block counter)
KWP: Sensor   (current KWP mode)
Grp: 3        (measurement group being polled)
Adr: 0x17     (ECU address)
Baud:9600     (baud rate)
Att: 0        (connection attempt count)
RAM: 834      (estimated free RAM, bytes)
```

Use this screen to verify the connection parameters and monitor RAM during development.

---

## DTC (Fault Codes)

Read and clear Diagnostic Trouble Codes stored in the ECU.

### Main menu

```
DTC Menu

>Read
 Clear
 Show

DTCs: --
```

UP/DOWN moves the `>` cursor between the three actions. SELECT executes the highlighted action.

| Action | What it does |
|---|---|
| **Read** | Sends a Read Faults request to the ECU and stores up to 16 DTCs |
| **Clear** | Sends a Delete Faults request to the ECU |
| **Show** | Opens the DTC list sub-view (only useful after a Read) |

The `DTCs:` counter at the bottom shows `--` until a Read is performed, then shows the count returned by the ECU.

### Show sub-view (4 DTCs per page)

```
DTCs 1-4
#1  E:16825
    S:237
#2  E:17544
    S:64
#3  E:XXXXX
    S:XXX
#4  E:XXXXX
    S:XXX

U/D:pg  Sel:bk
```

- **E:** — raw error code (decimal)
- **S:** — status byte (decimal)
- UP/DOWN pages through groups of 4; SELECT returns to the main menu

### Decoding DTC codes

DTC codes from VAG KWP-1281 ECUs are **VAG-specific 5-digit codes**, not standard OBD-II P/U/B/C codes. To look up what a code means:

- **VCDS label files** — [Ross-Tech VCDS label file archive](https://www.ross-tech.com/vcds/label-files/) maps codes to plain-text descriptions for each ECU part number.
- **Ross-Tech Wiki** — [wiki.ross-tech.com](https://wiki.ross-tech.com) has fault code lists and troubleshooting guides organized by code.
- **OBD-Codes.com** — searching `VAG XXXXX` (e.g. `VAG 16825`) often returns a description and likely cause.
- Your ECU's label file (e.g. `1J0-920-xx0.LBL` for the instruments cluster) lists what each fault code means for that specific module.

The status byte indicates how the fault was recorded:

| Bit | Meaning |
|---|---|
| 0 | Fault is currently present |
| 4 | Fault is intermittent (sporadic) |
| 5 | Fault was present, now gone |

---

## Settings

```
Settings

>Exit
 KWP: Sens
 AutoRcn: Y
```

When connected to the **instruments cluster (0x17)** an additional item appears:

```
>Fuel: 33L
 MID=save
```

UP/DOWN moves the cursor. SELECT acts on the highlighted item.

| Item | SELECT action |
|---|---|
| **Exit** | Ends the ECU session cleanly and returns to the startup setup menu |
| **KWP:** | Cycles the KWP mode: `ACK` → `Grp` → `Sens` → `ACK` … |
| **AutoRcn:** | Toggles auto-reconnect Y/N |
| **Fuel:** | *(0x17 only)* Saves the current 0x17 fuel sensor reading to EEPROM as the fuel start level for the 0x01 trip computer. Press SELECT when the tank is known (e.g. just filled up). |

### Setting up range calculation (0x01 trip computer)

The 0x01 engine ECU computes fuel consumption from RPM and engine load but has no fuel level sensor. To enable the km-remaining display on 0x01 screen 4:

1. Fill up the tank.
2. Connect to the **instruments cluster** (`0x17`) — this has the real fuel level sensor.
3. Navigate to **Settings** (RIGHT from Cockpit).
4. Navigate DOWN to the `Fuel:` item — it shows the current sensor reading.
5. Press **SELECT** to save that reading to EEPROM.
6. Switch ECU to **0x01** (Settings → Exit, then change address).
7. Screen 4 (trip computer) will now show `---K` until the engine has been running a moment, then update the km-remaining estimate.

Repeat after every fill-up. The value persists across power cycles.

> **Calibration note**: If the L/100km readout seems consistently off, adjust `ENGINE01_FUEL_DENOM` in `Config.h`. Higher → lower reading; lower → higher reading.

### KWP modes

| Display | Mode | Behaviour |
|---|---|---|
| `ACK` | Acknowledge only | Sends keepalive blocks only — no sensor data is read. Useful to hold a session open without loading the ECU. |
| `Grp` | Read Group | Reads measurement groups sequentially (groups 1, 2, 3, …). This is the normal operating mode. |
| `Sens` | Read Sensors | Reads individual sensor channels. Slower than group mode but more granular. |

Changing the mode takes effect on the next KWP cycle without disconnecting.
