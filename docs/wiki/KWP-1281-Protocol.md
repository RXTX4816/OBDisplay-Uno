# KWP-1281 Protocol

## Overview

KWP-1281 (Keyword Protocol 1281) is the diagnostic protocol used by VAG (Volkswagen/Audi Group) vehicles from roughly 1995–2006. It runs over a single-wire K-Line at 5V logic, and is distinct from the later KWP-2000 (ISO 14230) and CAN-based protocols used on newer vehicles.

This project implements KWP-1281 using a Software Serial port on pins 2/3 and a modified KKL OBD-to-USB cable as the level shifter.

## Connection sequence

1. **5-baud initialization** — a GPIO pulse on the TX pin asserts the K-Line LOW for ~200 ms then HIGH, transmitting the ECU address at 5 baud (7N1).
2. **Sync bytes** — the ECU responds with `0x55` (sync), `0x01`, `0x8A` (keyword bytes). The firmware verifies these.
3. **Device blocks** — the ECU sends its identification blocks; the firmware ACKs each one.
4. **Session open** — data exchange begins.

## Block structure

A KWP-1281 block consists of:

```
[length] [counter] [title] [data...] [end 0x03]
```

The firmware increments a block counter on every block sent and verifies the ECU's counter matches. A complement byte handshake confirms each received byte during certain phases.

## Supported ECU addresses

| Address | Module | Cockpit display |
|---|---|---|
| `0x01` | Engine ECU | Dedicated big-font layout |
| `0x03` | ABS Brakes | Raw group values |
| `0x08` | Auto HVAC | Raw group values |
| `0x17` | Instruments / dashboard | Dedicated big-font layout |
| `0x19` | CAN Gateway | Raw group values |
| `0x46` | Central Convenience | Raw group values |

Addresses without a dedicated cockpit layout show the raw group values from the Experimental screen. Do not connect to `0x15` (airbag) — on some ECUs clearing DTCs there can deploy the airbag if an electrical fault is present.

## Supported baud rates

1200, 2400, 4800, 9600, 10400 baud. Most VAG K-Line ECUs from this era use **10400 baud**. The auto-setup shortcut defaults to 10400.

## Measurement groups

Each ECU address exposes numbered groups. Each group returns 4 measurement values. The meaning of each value depends on your specific ECU.

### ADDR_INSTRUMENTS `0x17` (label: `1J0-920-xx0.LBL`)

| Group | Values |
|---|---|
| 1 | Speed (km/h), engine RPM, min oil pressure, ECU time |
| 2 | Odometer (km), fuel level (L), fuel sender (Ω), ambient temp (°C) |
| 3 | Coolant temp (°C), oil level OK, oil temp (°C) |

### ADDR_ENGINE `0x01` (example ECU: `036906034AM` 1.6 16V Marelli)

| Group | Values |
|---|---|
| 1 | RPM, temperature, lambda %, error bits |
| 3 | RPM, pressure (mbar), throttle angle, steering angle |
| 4 | RPM, voltage (V), temp (°C), temp (°C) |
| 6 | RPM, engine load %, temp (°C), lambda2 % |

If your ECU uses different groups or value mappings, record them with VCDS or another KWP tool and update `src/obd/KWP/KWPSensorDecode.cpp`.

## Measurement type decode table

The firmware includes a 56-case decode table that converts raw KWP-1281 measurement bytes into engineering values (the full VW/Audi type table). The relevant source file is `src/obd/KWP/KWPSensorDecode.cpp`.

## DTC codes

DTCs are read using the `0x07` (Read Faults) block title and cleared with `0x05` (Delete Faults). The firmware stores up to 16 DTCs in `Model::DTCStore`.

## Protocol reference

- [blafusel.de KW1281 reference](https://www.blafusel.de/obd/obd2_kw1281.html)
- [mkirbst's lupo-gti-tripcomputer-kw1281](https://github.com/mkirbst/lupo-gti-tripcomputer-kw1281)
