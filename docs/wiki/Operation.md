# Operation

## Navigation

| Button | Action |
|---|---|
| LEFT / RIGHT | Previous / next menu |
| UP / DOWN | Previous / next screen within the current menu |
| SELECT | Context action (varies per menu — see below) |

Menu order: **Cockpit → Experimental → Debug → DTC → Settings**

## Cockpit screen

Pixel-doubled big font, portrait 64×128. All fields update at ~177 ms intervals. SELECT has no action.

### ADDR_INSTRUMENTS `0x17` — one screen

```
130        speed (km/h)
2200       RPM

99 O       oil temp (°C)  ← -WARN- at ≥ 100
99 C       coolant (°C)   ← -WARN- at ≥ 100

33 L       fuel level (L)
20AIR      ambient (°C)
```

### ADDR_ENGINE `0x01` — two screens (UP/DOWN)

**Screen 0:**

```
120        speed (km/h)
1200       RPM
99 O       oil temp       ← -WARN- at ≥ 100
99 C       coolant        ← -WARN- at ≥ 100
20%        engine load
5.5T       throttle body angle
12V        battery voltage
5%         lambda
```

**Screen 1** (error bits, small font):

```
TBa: xxxxx  STa: xxxxx
mb:  xxxx
bits:
xxxxxxxx
```

### Other addresses

For addresses without a dedicated layout (`0x03`, `0x08`, `0x19`, `0x46`) the cockpit screen shows the address and "no data". Use the Experimental screen to browse raw measurement groups for those ECUs.

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
