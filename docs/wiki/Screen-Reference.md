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

## Cockpit

Live sensor data displayed in a pixel-doubled big font (12 px/char, 16 px/row). No SELECT action — data updates automatically every ~177 ms.

### Instruments cluster (address `0x17`)

One screen, six values stacked vertically:

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
| 4 | Fuel level | `NN L` | |
| 5 | Ambient temperature | `NNAIR` | |

### Engine ECU (address `0x01`)

**Screen 0** — eight values stacked vertically (big font):

```
120
1200
99 O
99 C
20%
5.5T
12V
5%
```

| Row | Field | Format | Notes |
|---|---|---|---|
| 0 | Vehicle speed | `NNN` km/h | |
| 1 | Engine RPM | `NNNN` | |
| 2 | Oil temperature | `NN O` | Shows `-WARN-` at ≥ 100 °C |
| 3 | Coolant temperature | `NN C` | Shows `-WARN-` at ≥ 100 °C |
| 4 | Engine load | `NN%` | |
| 5 | Throttle body angle | `N.NT` | ×10 fixed-point |
| 6 | Battery voltage | `NNV` | |
| 7 | Lambda | `NN%` | |

**Screen 1** — error bits (small font):

```
TBa: xxxxx  STa: xxxxx
mb:  xxxx
bits:
xxxxxxxx
```

| Field | Meaning |
|---|---|
| `TBa` | Throttle body angle |
| `STa` | Steering angle |
| `mb` | Manifold pressure (mbar) |
| `bits` | Engine error flags (8 bits: EGR / O2 heat / O2 / AC / SAI / EVAP / cat heat / cat) |

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
Sim: N        (simulation mode active?)
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
```

UP/DOWN moves the cursor. SELECT acts on the highlighted item.

| Item | SELECT action |
|---|---|
| **Exit** | Ends the ECU session cleanly and returns to the startup setup menu |
| **KWP:** | Cycles the KWP mode: `ACK` → `Grp` → `Sens` → `ACK` … |

### KWP modes

| Display | Mode | Behaviour |
|---|---|---|
| `ACK` | Acknowledge only | Sends keepalive blocks only — no sensor data is read. Useful to hold a session open without loading the ECU. |
| `Grp` | Read Group | Reads measurement groups sequentially (groups 1, 2, 3, …). This is the normal operating mode. |
| `Sens` | Read Sensors | Reads individual sensor channels. Slower than group mode but more granular. |

Changing the mode takes effect on the next KWP cycle without disconnecting.
