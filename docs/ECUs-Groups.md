# ECU Group Reference

## Contents

- [0x01 Engine — Marelli 4LV](#0x01-engine--marelli-4lv-036-906-034am)
  - [Group 0 — Raw values](#group-0--raw-ecu-values)
  - [Basic Functions (Groups 1–6)](#basic-functions-groups-16)
  - [Ignition & Misfire (Groups 7–28)](#ignition--misfire-groups-728)
  - [Oxygen Sensor / Lambda (Groups 30–46)](#oxygen-sensor--lambda-groups-3046)
  - [Idle Speed & EPC Throttle (Groups 50–62)](#idle-speed--epc-throttle-groups-5062)
  - [Emissions (Groups 70–75)](#emissions-groups-7075)
  - [OBD / Readiness (Groups 99–100)](#obd--readiness-groups-99100)
  - [Communication (Groups 120–125)](#communication-groups-120125)
  - [Coding](#coding-ecu-0x01)
- [0x03 ABS](#0x03-abs)
- [0x08 HVAC — Climatronic](#0x08-hvac--climatronic-3b1-907-044)
- [0x17 Instruments — Cluster](#0x17-instruments--instrument-cluster-1j0-920-xx0)
  - [Adaptation Channels](#adaptation-channels-0x17-instruments)
  - [Coding](#coding-0x17-instruments)
- [0x19 CAN Gateway](#0x19-can-gateway-6n0-909-901)
- [0x46 Central Convenience](#0x46-central-convenience--comfort-system-j393-1j0-959-799)
  - [Adaptation Channels](#adaptation-channels-0x46-central-convenience)
  - [Coding](#coding-0x46-central-convenience)

---

Values marked **[verify]** were not directly captured from a real car with VCDS —
they come from label files for closely related ECU variants and may differ slightly
from the actual ECU in this car. Everything else was confirmed against a real VCDS
recording. Where label file data contradicts the VCDS capture, the capture wins.

---

## 0x01 Engine — Marelli 4LV (036 906 034AM)

Label file reference: `036-906-034-APE` (APE/AQQ/AUA/AUB/BBY/BBZ/BKY variant,
same 036-906-034 family). Groups 1–6, 10–11, 18, 20 confirmed from VCDS recording.

---

### Group 0 — Raw ECU values

VCDS shows 3-digit numbers for positions 1–8. Meaning unknown — possibly internal
calibration or coding bytes. Only Group 0 shows up in the VCDS "Measuring Blocks"
dropdown; higher groups must be entered manually.

| Position | Captured value |
|----------|---------------|
| 1 | 148 |
| 2 | 203 |
| 3 | 000 |
| 4 | 126 |
| 5 | 040 |
| 6 | 127 |
| 7 | 143 |
| 8 | 128 |

---

### Basic Functions (Groups 1–6)

**Group 1** — Basic engine parameters + basic setting status

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Engine Speed | 0 /min | |
| 2 | Coolant Temperature | 17.0°C | VCDS label showed "oil temp" but label file says coolant — label file is likely correct |
| 3 | Lambda Controller | 0.0% | spec: -15 to +15% |
| 4 | Basic Setting Requirements | `10110010` binary | Bits indicate whether conditions for basic settings are met (see below) |

Group 1 Value 4 bit meanings (1 = condition met):
- bit 7: coolant temperature above 80°C
- bit 6: engine speed below 2000 RPM
- bit 5: throttle valve closed
- bit 4: lambda regulation correct
- bit 3: idle state active
- bit 2: A/C compressor deactivated
- bit 1: catalytic converter above 300°C
- bit 0: no malfunction detected by self-diagnosis

**Group 2** — Load and injection

| # | Description | Captured value | Spec |
|---|-------------|---------------|------|
| 1 | Engine Speed | 0 /min | 790–900 (manual) |
| 2 | Engine Load | 0.0% | 10–20% |
| 3 | Injection Timing Correction | 0.0 ms | 2.0–4.0 ms |
| 4 | Intake Manifold Pressure | 1012.0 mbar | 240–420 mbar |

**Group 3** — Pressures and angles

| # | Description | Captured value | Spec |
|---|-------------|---------------|------|
| 1 | Engine Speed | 0 /min | |
| 2 | Absolute Pressure | 1016.0 mbar | |
| 3 | Throttle Body Angle | 5.5° | |
| 4 | Steering Angle | 0.0° | |

**Group 4** — Electrical and temperature

| # | Description | Captured value | Spec |
|---|-------------|---------------|------|
| 1 | Engine Speed | 0 /min | |
| 2 | Battery Voltage | 11.70 V | 12.0–15.0 V |
| 3 | Coolant Temperature | 17.0°C | 80–110°C |
| 4 | Intake Air Temperature | 14.0°C | -30 to +120°C |

**Group 5** — Speed and load status

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Engine Speed | 0 /min | |
| 2 | Engine Load | 0.0% | |
| 3 | Vehicle Speed | 0.0 km/h | |
| 4 | Load Status | Part Throttle | Idle / Part Throttle / WOT / Enrichment / Deceleration |

**Group 6** — Lambda and temperatures

| # | Description | Captured value | Spec |
|---|-------------|---------------|------|
| 1 | Engine Speed | 0 /min | |
| 2 | Engine Load | 0.0% | |
| 3 | Intake Air Temperature | 14.0°C | -40 to +120°C |
| 4 | Heights Correction Factor (Lambda) | -1.0% | -30 to +5% |

---

### Ignition & Misfire (Groups 7–28)

**Groups 7–9** — No values shown in VCDS.

**Group 10** — Ignition timing

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Engine Speed | 0 /min | |
| 2 | Engine Load | 0.0% | |
| 3 | Throttle Drive Angle Sensor 1 (G187) — EPC | 6.0% | **[verify]** |
| 4 | Ignition Timing Angle | 0.0° | **[verify]** |

**Group 11** — Temperatures

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Engine Speed | 0 /min |
| 2 | Temperature | 17.0°C |
| 3 | Temperature | 14.0°C |
| 4 | Steering Angle | 0.0° |

**Groups 12–13** — Empty, no values.

**Group 14** — Misfire detection **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 790–900 /min |
| 2 | Engine Load | 10–20% |
| 3 | Misfire counter (total) | 0 |
| 4 | Misfire recognition status | active / inactive |

**Group 15** — Misfire per cylinder (1–3) **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Misfire counter cylinder 1 | 0 |
| 2 | Misfire counter cylinder 2 | 0 |
| 3 | Misfire counter cylinder 3 | 0 |
| 4 | Misfire recognition status | active / inactive |

**Group 16** — Misfire cylinder 4 **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Misfire counter cylinder 4 | 0 |
| 2–3 | (empty) | |
| 4 | Misfire recognition status | active / inactive |

**Group 17** — Empty.

**Group 18** — Dual lambda

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Engine Speed | 0 /min |
| 2 | Engine Speed | 0 /min |
| 3 | Lambda | 0.0% |
| 4 | Lambda | 0.0% |

**Group 19** — Empty.

**Group 20** — Steering angles

| # | Description | Captured value |
|---|-------------|---------------|
| 1–4 | Steering Angle | 0.0° each |

**Group 21** — Empty.

**Group 22** — Ignition knock retardation cylinders 1–2 **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 790–5700 /min |
| 2 | Engine Load | 15–175% |
| 3 | Cylinder 1 ignition angle delay | 0.0–14.0° |
| 4 | Cylinder 2 ignition angle delay | 0.0–14.0° |

**Group 23** — Ignition knock retardation cylinders 3–4 **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 790–5700 /min |
| 2 | Engine Load | 15–175% |
| 3 | Cylinder 3 ignition angle delay | 0.0–14.0° |
| 4 | Cylinder 4 ignition angle delay | 0.0–14.0° |

**Group 28** — Knock sensor test **[verify]**

| # | Description | Captured value | Spec |
|---|-------------|---------------|------|
| 1 | Engine Speed | | 2000–4000 /min |
| 2 | Engine Load | | 50–70% |
| 3 | Coolant Temperature | | 60–110°C |
| 4 | Knock sensor test result | | Test ON / Test OFF / OK / not OK |

---

### Oxygen Sensor / Lambda (Groups 30–46)

**Group 30** — Oxygen sensor status **[verify]**

| # | Description | Spec / Notes |
|---|-------------|-------------|
| 1 | Bank 1 Sensor 1 status | Binary: `1xx`=heater on, `x1x`=sensor ready, `xx1`=lambda active. Spec: 111 |
| 2 | Bank 1 Sensor 2 status | Same layout. Spec: 110 |

**Group 32** — Lambda self-adaptation **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Additive self-adaptation | -10 to +10% |
| 2 | Multiplicative self-adaptation | -10 to +10% |

**Group 33** — Lambda control values **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Bank 1 lambda control | -10 to +10% |
| 2 | Bank 1 oxygen sensor voltage | 0.00–1.00 V |

**Group 34** — Oxygen sensor aging test Bank 1 **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 1500–3900 /min |
| 2 | Exhaust gas temperature before catalyst | |
| 3 | Bank 1 Sensor 1 dynamic factor | max 1.0 s |
| 4 | Result | Test ON / Test OFF / B1-S1 OK / not OK |

**Group 36** — Sensor 2 readiness after catalyst **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Bank 1 Sensor 2 voltage | 0.00–1.00 V |
| 2 | Lambda availability result | Test ON / Test OFF / B1-S2 OK / not OK |

**Group 37** — Sensor 2 diagnostic **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Load | 12–40% |
| 2 | Bank 1 Sensor 2 voltage | |
| 3 | (empty) | |
| 4 | Result | Test ON / Test OFF / OK / not OK |

**Group 41** — Oxygen sensor heater resistance **[verify]**

| # | Description |
|---|-------------|
| 1 | Resistance Bank 1 Sensor 1 |
| 2 | Heater condition |
| 3 | Resistance Bank 1 Sensor 2 |
| 4 | Heater condition |

**Group 46** — Catalytic converter efficiency test **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 1500–3900 /min |
| 2 | Catalyst temperature | min 300°C |
| 3 | Amplitude behavior | min 50% |
| 4 | Catalyst conversion result | Test ON / Test OFF / CatConvB1 OK / not OK |

---

### Idle Speed & EPC Throttle (Groups 50–62)

**Group 50** — Speed regulation **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 800 /min (manual) |
| 2 | Target engine speed | |
| 3 | A/C system condition | A/C-High / A/C-Low |
| 4 | A/C compressor status | Compr.ON / Compr.OFF |

**Group 54** — Throttle and pedal sensors **[verify]**

| # | Description |
|---|-------------|
| 1 | Engine Speed |
| 2 | Load status (Idle / Part Throttle / WOT / Enrichment / Deceleration) |
| 3 | Accelerator pedal position sensor 2 (G79) — 0–100% |
| 4 | Throttle drive angle sensor 1 (G187) — 0–100% |

**Group 55** — Idle regulator **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 800 /min |
| 2 | Idle regulator | -10 to 0% |
| 3 | Idle stabilization self-adaptation | A/C-High: -6 to +8% / A/C-Low: -3 to +8% |
| 4 | Load status bits | `--1xx`=A/C on, `--x1x`=gear selected, `--xx1`=compressor on |

**Group 56** — Idle torque regulation **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 800 /min |
| 2 | Target engine speed | |
| 3 | Idle regulator (Nm) | -10 to 0 Nm |
| 4 | Load status bits | same as group 55 |

**Group 60** — EPC throttle adaptation **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Throttle drive angle sensor 1 (G187) | 10–85% |
| 2 | Throttle drive angle sensor 2 (G188) | 85–10% (inverse) |
| 3 | Self-adaptation steps counter | 0–12 |
| 4 | Result | ADP runs / ADP OK / ADP ERROR |

**Group 61** — EPC system status **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 800–5700 /min |
| 2 | Battery voltage | 12.0–15.0 V |
| 3 | Throttle drive angle sensor 1 — 0–100% | |
| 4 | Load status bits | same as group 55 |

**Group 62** — All throttle/pedal sensors **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Throttle drive angle sensor 1 (G187) | 3–97% |
| 2 | Throttle drive angle sensor 2 (G188) | 97–3% (inverse) |
| 3 | Throttle position sensor (G79) | 0–100% |
| 4 | Accelerator pedal position sensor 2 (G185) | 0–100% |

---

### Emissions (Groups 70–75)

**Group 70** — Evaporative emissions **[verify]**

| # | Description |
|---|-------------|
| 1 | TVV (Tank Ventilation Valve) opening angle |
| 2 | Lambda controller diagnostic value (active during diagnosis) |
| 3 | Intake manifold pressure |
| 4 | Result: Test ON / Test OFF / TEV OK / TEV not OK |

**Group 74** — EGR valve adaptation **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Null-position voltage | 0.3–1.3 V |
| 2 | Max-position voltage | 2.9–4.6 V |
| 3 | Potentiometer voltage (actual) | 0.3–4.6 V |
| 4 | Adaptation status | ADP run / ADP OK / ADP ERROR |

**Group 75** — Exhaust gas recirculation test **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 1800–5600 /min |
| 2 | Intake manifold pressure | |
| 3 | Intake manifold pressure differential | min +50 mbar |
| 4 | Result | Test ON / Test OFF / Sys. OK / Sys. n.OK |

---

### OBD / Readiness (Groups 99–100)

> **Note:** Group 100 Value 1 is exactly what VCDS reads when opening the Readiness
> screen — it is a normal KWP1281 group reading (command 0x29). The emulator currently
> refuses groups above 23 and needs a dynamic override for group 100 added.

**Group 99** — OBD compatibility **[verify]**

| # | Description | Spec |
|---|-------------|------|
| 1 | Engine Speed | 800 /min |
| 2 | Coolant Temperature | 85–110°C |
| 3 | Oxygen sensor control percentage | -10 to +10% |
| 4 | Oxygen sensor control condition | ON / OFF |

**Group 100** — OBD readiness status **[verify]**

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Readiness bits | `10100101` (`0xA5`) | 1 = not complete / failed, 0 = complete / passed |
| 2 | Coolant Temperature | | |
| 3 | Time since engine start | | |
| 4 | OBD status flags | | see bit meanings below |

Group 100 Value 1 readiness bit meanings:
- bit 7: Exhaust Gas Recirculation (EGR)
- bit 6: O2 sensor heater
- bit 5: Oxygen sensors
- bit 4: A/C system
- bit 3: Secondary Air Injection
- bit 2: Activated charcoal / evaporative system
- bit 1: Catalytic converter heater
- bit 0: Catalytic converter

Group 100 Value 4 OBD status bit meanings:
- bit 7: MIL warning lamp on
- bit 6: complete distance recorded
- bit 5: at least one malfunction detected
- bit 1: heating cycle ended
- bit 0: heating cycle not possible

---

### Communication (Groups 120–125)

**Group 120** — Traction control (ASR/TCS) **[verify]**

| # | Description |
|---|-------------|
| 1 | Engine Speed |
| 2 | Target engine speed |
| 3 | Actual engine speed |
| 4 | ASR condition: ASR active / not active |

**Group 122** — Transmission torque **[verify]**

| # | Description |
|---|-------------|
| 1 | Engine Speed |
| 2 | Target engine speed |
| 3 | Actual engine speed |
| 4 | Status: Torque reduction / No torque reduction |

**Group 125** — CAN powertrain bus **[verify]**

| # | Description |
|---|-------------|
| 1 | Brake electronics status |
| 2 | Transmission status |
| 3 | Instrument cluster status |
| 4 | Airbag status |

---

### Coding (ECU 0x01)

| Digit | Meaning | Values |
|-------|---------|--------|
| 000x? | Gearbox type | 1 = Manual, 3 = Automatic |
| 000?x | Additional functions | +1 ABS, +2 Airbag, +4 Air Conditioning |

---

---

## 0x03 ABS

**Group 0** — Not available.

**Group 1** — Wheel speeds (all four wheels, follows simulated vehicle speed)

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Wheel speed FL | 0.0 km/h |
| 2 | Wheel speed FR | 0.0 km/h |
| 3 | Wheel speed RL | 0.0 km/h |
| 4 | Wheel speed RR | 0.0 km/h |

**Group 2** — Wheel speeds at high value

| # | Description | Captured value |
|---|-------------|---------------|
| 1–4 | Wheel speeds | 255.0 km/h each |

**Group 3** — Switch/sensor status

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Status | Not Oper. |
| 2 | Status | Not Oper. |
| 3 | Status | N/A |
| 4 | Status | N/A |

**Group 4** — Motion sensors

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Steering Angle | 0.00° |
| 2 | Lateral Acceleration | 0.31 m/s² |
| 3 | Turn Rate | -0.18°/s |
| 4 | N/A | |

**Group 5** — Brake pressures

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Brake pressure | -1.27 bar |
| 2 | Brake pressure | 0.42 bar |
| 3–4 | N/A | |

**Groups 6+** — Not available.

---

---

## 0x08 HVAC — Climatronic (3B1-907-044)

**Group 0** — Not available.

**Group 1** — General A/C values

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | A/C clutch switch-off condition | 9.0 | Range: 1–12 |
| 2 | Engine speed recognition | 0.0 | 0 = no, 1 = yes |
| 3 | Road speed | 0.0 km/h | |
| 4 | Standing time | 121.0 | Range: 0–240 min |

**Group 2** — Temperature regulator flap

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Measured value | 42.0 |
| 2 | Specified value | 42.0 |
| 3 | Position: air supply cooled | 219.0 |
| 4 | Position: air supply heated | 42.0 |

**Group 3** — Center air flap

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Measured value | 221.0 |
| 2 | Specified value | 221.0 |
| 3 | Position: air flow to panel | 221.0 |
| 4 | Position: air flow to footwell | 40.0 |

**Group 4** — Footwell / defroster flap

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Measured value | 223.0 |
| 2 | Specified value | 223.0 |
| 3 | Position: air flow to footwell | 223.0 |
| 4 | Position: air flow to defroster | 39.0 |

**Group 5** — Air flow flap (fresh air vs. recirculation)

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Measured value | 237.0 |
| 2 | Specified value | 234.0 |
| 3 | Position: fresh air | 234.0 |
| 4 | Position: recirculating | 30.0 |

**Group 6** — Temperatures and sun sensor

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Temperature displayed on panel | 0.0°C |
| 2 | Temperature air intake duct | 7.0°C |
| 3 | Outside air temperature | 0.0°C |
| 4 | Sun photo sensor | 0.0% (range: 0–120%) |

**Group 7** — Outlet temperatures

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Outlet temp. panel (G191) | 0.0 |
| 2 | Outlet temp. floor (G192) | 5.0°C |
| 3 | Panel temp. near LCD (G56) | 3.0°C |
| 4 | N/A | 0.0 |

**Group 8** — Voltages

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Specified voltage air blower | 0.00 V | |
| 2 | Measured voltage air blower | 0.28 V | |
| 3 | Measured voltage A/C clutch | 12.18 V | label file has a duplicate entry error for pos 3 |
| 4 | (no units) | 0.0 | |

**Groups 9+** — Not available.

---

---

## 0x17 Instruments — Instrument Cluster (1J0-920-xx0)

Label file covers part numbers 1J?-920-??0/??1/??2/??5, tested with 1J0 920 900 K,
1J0 920 825 C, 1J5 920 845 A.

**Group 0** — Not available.

**Group 1** — Speed / Engine speed / Oil pressure / Time

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Vehicle speed | 0.0 km/h | |
| 2 | Engine speed | 0 /min | |
| 3 | Oil pressure indicator | 2 (`<min`, 0.9 bar) | Meaning of exact value unclear — probably an index (0, 1, 2) |
| 4 | Time | 21:50 | |

**Group 2** — Distance / Fuel / Ambient temperature

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Odometer | 272727 km |
| 2 | Fuel level | 23.0 L |
| 3 | Fuel sender resistance | 93 Ohm |
| 4 | Ambient temperature (MFD only) | 0.0°C |

**Group 3** — Coolant and oil

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Coolant temperature | 12.0°C | |
| 2 | Oil level | OK / n.OK | Probably encoded as 0 or 1 |
| 3 | Oil temperature | 11.0°C | |
| 4 | N/A | | |

**Group 20** — Immobilizer ID Block 1 **[verify]**

| # | Description |
|---|-------------|
| 1 | Immobilizer digit 1 & 2 |
| 2 | Immobilizer digit 3 & 4 |
| 3 | Immobilizer digit 5 & 6 |
| 4 | Immobilizer digit 7 & 8 |

**Group 21** — Immobilizer ID Block 2 **[verify]**

| # | Description |
|---|-------------|
| 1 | Immobilizer digit 9 & 10 |
| 2 | Immobilizer digit 11 & 12 |
| 3 | Immobilizer digit 13 & 14 |
| 4 | N/A |

**Group 22** — Immobilizer status **[verify]**

| # | Description |
|---|-------------|
| 1 | Starting permitted (yes/no) |
| 2 | Engine (ECM) responds (yes/no) |
| 3 | Key condition OK (yes/no) |
| 4 | Number of keys coded |

**Group 23** — Immobilizer details **[verify]**

| # | Description | Notes |
|---|-------------|-------|
| 1 | Variable code authorized (yes/no) | |
| 2 | Key status — transponder | |
| 3 | Fixed code authorized (yes/no) | |
| 4 | Immobilizer status code | 4=delivery condition, 5=customer service locked, 6=adapted (normal), 7=key adaptation active |

**Group 50** — Miscellaneous **[verify]**

| # | Description |
|---|-------------|
| 1 | Odometer |
| 2 | Engine speed |
| 3 | Oil temperature |
| 4 | Coolant temperature |

**Group 125** — CAN powertrain bus status **[verify]**

| # | Description |
|---|-------------|
| 1 | Engine |
| 2 | Transmission |
| 3 | ABS |
| 4 | N/A |

**Group 126** — CAN powertrain bus status (cont.) **[verify]**

| # | Description |
|---|-------------|
| 1 | Steering sensor |
| 2 | Airbag |
| 3–4 | N/A |

**Groups 4–19, 24–49, 51–124, 127+** — Not available.

---

### Adaptation Channels (0x17 Instruments)

| Channel | Description | Notes |
|---------|-------------|-------|
| A2 | Service reminder | Write 0 to reset service reminder |
| A3 | Consumption display correction | Base: 100, range: 85–115, unit: 5% per step |
| A4 | Language for error messages / navigation (MFD only) | 1=German, 2=English, 3=French, 4=Italian, 5=Spanish, 6=Portuguese, 8=Czech |
| A9 | Distance (in 10 km steps) | Requires login 13861 first. Can only be changed if current value is under 100 km |
| A16 | Distance impulse identifier | Read only |
| A21 | Key count (immobilizer) | Old key count |
| A30 | Fuel gauge adaptation | Base: 100, range: 120–136, unit: 1 Ohm |
| A35 | Engine speed threshold | Base: 0, range: 0–1000, unit: 250 RPM per step |
| A40 | Distance from last inspection | Resolution: 1 = 100 km. Must be saved in order: 42 → 43 → 44 → 40 → 41 |
| A41 | Time from last inspection | Same save order as A40 |
| A42 | Minimum mileage value | Resolution: 1 = 1000 km. Same save order |
| A43 | Maximum mileage value | Resolution: 1 = 1000 km. Same save order |
| A44 | Maximum time interval | Same save order as A40 |

### Coding (0x17 Instruments)

| Digit | Meaning | Values |
|-------|---------|--------|
| xx?xx | Country version | 1=Europe, 2=USA, 3=Canada/Mexico/Latin America, 4=England, 5=Japan, 6=Saudi Arabia, 7=Australia |
| xxx?x | Service intervals | 0=mini without sensors, 1=flexible with oil level/temp sensor, 2=fixed with sensors, 3=without (USA/Canada) |
| xxxx? | Distance impulse number (K-value) | 1=4358 (1.4l/55kW AHW/AKQ manual), 2=3538 (all others), 3=4146 (1.6l/74-77kW or 1.9l/50kW-SDI manual), 4=3648 |
| ??xxx | Additional equipment | +01 brake pad warning, +02 seatbelt warning, +04 washer fluid warning, +16 radio/navigation |

---

---

## 0x19 CAN Gateway (6N0-909-901)

Note from label file: this controller is used in many cars but measuring blocks are
not present in every car. Coding also differs per car. Part number is always the
same without index.

**Groups 1–9** — No data shown (confirmed from label file — blocks exist but return nothing).

**Group 80** — Manufacturer identification **[verify]**
- Date of manufacture, manufacturer changing status, test stand number, running
  manufacturer number (e.g. BPA = Bosch factory Ansbach)

**Groups 81–124** — Not available.

**Groups 125–143** — CAN bus node status (1 = OK / 0 = not OK) **[verify]**

| Group | Node 1 | Node 2 | Node 3 | Node 4 |
|-------|--------|--------|--------|--------|
| 125 | Engine | Auto. Transmission | ABS Brake System | — |
| 126 | Steering Angle | Airbag | — | — |
| 127 | — | AWD | — | — |
| 128 | Battery Management | Electrical Ignition Lock 1 | Level control | — |
| 130 | Comfort CAN Bus Mode | Central Convenience | Driver Door | Passenger Door |
| 131 | Rear Right Door | Rear Left Door | Driver Seat Memory | Electrical Load Control |
| 132 | — | Steering Wheel Electronics | Air Conditioning | Tire Pressure Monitoring |
| 133 | Roof Electronics | Pax Seat Memory | Rear Seat Memory | Park Distance Control |
| 134 | — | Electrical Ignition Lock 1 | Wiper Control Module | — |
| 135 | — | Display Control Front 1 | Display Control Rear 1 | — |
| 136 | — | Central Locking | — | — |
| 140 | Optical Data Bus Mode | — | Navigation System | — |
| 141 | — | — | — | Telematics |
| 142 | Display Control Front 1 | — | — | — |
| 143 | Digital Sound Processor | — | — | — |

**Groups 144+** — Not available.

---

---

## 0x46 Central Convenience — Comfort System J393 (1J0-959-799)

Generation II (pre MY2002). Applies to: Seat Leon/Toledo 1M, Skoda Octavia 1U,
VW Bora/Golf 1J, VW New Beetle 1C/9C, VW Passat 3B, VW Polo Classic 9V.

**Group 0** — Not available.

**Group 1** — Global switches

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | Child safety switch (E39) | OFF | yes / no / not installed |
| 2 | Interior locking driver switch (E150) | Not Oper. | lock / unlock / not operated / implausible |
| 3 | Electric window hall signal (motor moving?) | Still | rotating / stop |
| 4 | N/A | — | |

**Group 2** — Driver's door window regulator buttons

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | WR button (E40) — driver side | Not Oper. | auto.open / auto.close / man.open / man.close / not operated / implausible |
| 2 | WR button (E81) — passenger side | Not Oper. | same |
| 3 | WR button (E53) — rear left | Not Oper. | same |
| 4 | WR button (E55) — rear right | Not Oper. | same |

**Group 3** — Driver's door central locking

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | CL key switch driver (E?) | Not Oper. | open / closed / not operated / implausible |
| 2 | Thermal protection / rotary latch | `01` binary | `0x`=door closed / `1x`=door open, `x0`=thermal protection active / `x1`=inactive |
| 3 | CL feedback driver side | Unlocked | locked / unlocked |
| 4 | Safe feedback driver side | Not safe | safe / not safe |

**Group 4** — Driver's door mirror switches

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | Mirror adjustment switch driver (E43) | Not Oper. | Pos X+ / Pos X- / Pos Y+ / Pos Y- / not operated |
| 2 | Mirror selection switch driver (E48) | Not Oper. | left / right / fold / not operated |
| 3 | Mirror folding switch (E263) | Not installed | released / engaged / not installed |
| 4 | N/A | — | |

**Group 5** — Passenger door window switch and mirror

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | WR button (E107) — passenger side | Not Oper. | auto.open / auto.close / man.open / man.close / not operated / implausible |
| 2 | Interior locking passenger switch (E198) — USA only | Not Oper. | lock / unlock / not operated / implausible |
| 3 | Mirror folding switch (E263) | Not installed | released / engaged / not installed |

**Group 6** — Passenger door central locking

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | CL key switch passenger (E?) | Not Oper. | open / closed / not operated / implausible |
| 2 | Thermal protection / rotary latch | `01` binary | same as group 3 pos 2 |
| 3 | CL feedback passenger side | Unlocked | locked / unlocked |
| 4 | Safe feedback passenger side | Not safe | safe / not safe |

**Group 7** — Rear right door

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | WR button (E54) — rear right | Not Oper. | auto.open / auto.close / man.open / man.close / not operated / implausible |
| 2 | Thermal protection / rotary latch | `01` binary | same as group 3 pos 2 |
| 3 | CL feedback rear right | Unlocked | locked / unlocked |
| 4 | Safe feedback rear right | Not safe | safe / not safe |

**Group 8** — Rear left door

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | WR button (E52) — rear left | Not Oper. | auto.open / auto.close / man.open / man.close / not operated / implausible |
| 2 | Thermal protection / rotary latch | `01` binary | same as group 3 pos 2 |
| 3 | CL feedback rear left | Unlocked | locked / unlocked |
| 4 | Safe feedback rear left | Not safe | safe / not safe |

**Group 9** — Signals

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | Instrument illumination signal | 0.0% | 0–100% |
| 2 | Speed signal (G22) | 0.0 km/h | steps of 2 km/h |
| 3 | Remote control button | `0000` binary | open / closed / RLR / panic (RLR = rear lid release) |
| 4 | Interior monitor sensor (G273) | Not installed | yes / no / not installed |

**Group 10** — Signals (cont.)

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | S-contact (terminal S) | Activated | operated / not operated |
| 2 | Mirror heating (E231) | OFF | on / off / not installed |
| 3 | Rear lid / tailgate key switch (E165) | Not Oper. | open / closed / not operated / implausible |
| 4 | Ignition (terminal 15) | ON | Terminal 15 on / Terminal 15 off |

**Group 11** — Contact switches

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | Hood contact switch (F120) | Closed | operated / not operated / not installed |
| 2 | Rear lid / tailgate contact switch (F124) | Closed | open / closed |
| 3 | Sliding/tilting sunroof released | Yes | yes / no |
| 4 | N/A | — | |

**Group 12** — CAN bus communication

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | CAN bus status | Bus OK | specification: 2-wire |
| 2 | Door control modules (front) | Driver / Passenger | |
| 3 | Door control modules (rear) | RL / RL+RR / RR | |
| 4 | Additional equipment | Memory | |

**Group 13** — Remote control status

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Permanent code known | no value (OK/n.OK) |
| 2 | Code within effective range | no value (OK/n.OK) |
| 3 | Algorithm | no value (OK/n.OK) |
| 4 | Key number (0 = not operated) | 0 |

**Group 14** — Supply voltage and rear lid

| # | Description | Captured value | Notes |
|---|-------------|---------------|-------|
| 1 | Supply voltage terminal 30 | 12.32 V | |
| 2 | Rear lid button / handle (E188/E234) | Not Oper. | not operated / remote switch / lid lever / implausible |
| 3 | Interior monitoring switch-off (E267) | Not installed | on / off / not installed |
| 4 | CL thermal protection bits | `11 11 1` | `1xxxx`=DS, `x1xxx`=PS, `xx1xx`=RL, `xxx1x`=RR, `xxxx1`=rear lid |

**Group 15** — Alarm sources (last 4 triggered)

| # | Description | Captured value |
|---|-------------|---------------|
| 1 | Last alarm source | 16 |
| 2 | 2nd last alarm source | 4 |
| 3 | 3rd last alarm source | 128 |
| 4 | 4th last alarm source | 128 |

Alarm source codes:
| Value | Source |
|-------|--------|
| 1 | Rear lid / tailgate contact switch |
| 2 | Rear left rotary latch switch |
| 4 | Rear right rotary latch switch |
| 8 | Front passenger rotary latch switch |
| 16 | Ignition |
| 17 | Immobilizer |
| 32 | Interior monitoring |
| 64 | Engine hood contact switch |
| 128 | Driver's rotary latch switch |
| 255 | No alarm |

**Group 16** — Auto locks and immobilizer

| # | Description | Captured value | Display range |
|---|-------------|---------------|--------------|
| 1 | Immobilizer key recognition | Not installed | yes / no / not installed |
| 2 | Automatic lock/unlock switch | Not Oper. | operated / not operated / implausible |
| 3 | Rear first detent (latch) | Closed | open / closed / not installed |

**Groups 17+** — Not available.

---

### Adaptation Channels (0x46 Central Convenience)

| Channel | Description | Values |
|---------|-------------|--------|
| A1 | Remote control key adaptation | Enter number of keys to adapt, then press button 1 or 2 on each remote for ≥1s. All keys must be adapted in one procedure within 15 seconds. |
| A3 | Auto-Lock | 0 = off, 1 = on. Locks doors automatically at 15 km/h (or 10 mph). |
| A4 | Auto-Unlock | 0 = off, 1 = on. Unlocks doors when ignition key is removed. |
| A5 | Interior Monitoring | 0 = off, 1 = on |
| A6 | Unlock Horn | 0 = off, 1 = on. Horn beep when unlocking via remote. |
| A7 | Lock Horn | 0 = off, 1 = on. Horn beep when locking via remote. |
| A8 | Unlock Blink | 0 = off, 1 = on. Blink when unlocking via remote. |
| A9 | Lock Blink | 0 = off, 1 = on. Blink when locking via remote. |
| A10 | Country setting for alarm horn | 1 = Rest of World, 2 = Germany, 3 = Great Britain |

### Coding (0x46 Central Convenience)

| Code | Description |
|------|-------------|
| 00256 | 2 power windows — selective unlocking |
| 00257 | 2 power windows — all doors unlock |
| 00258 | 2 power windows + memory seats — selective unlocking |
| 00259 | 2 power windows + memory seats — all doors unlock |
| 04096 | 4 power windows — selective unlocking |
| 04097 | 4 power windows — all doors unlock |
| 04098 | 4 power windows + memory seats — selective unlocking |
| 04099 | 4 power windows + memory seats — all doors unlock |
