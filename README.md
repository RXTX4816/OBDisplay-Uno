# OBDisplay-Uno

KWP-1281 K-Line trip computer for Arduino Uno with SSD1312 OLED display.

Reads live sensor data and fault codes from VAG vehicles (Golf Mk4, Bora, Jetta, ~1998–2006) that use the K-Line OBD interface and the KWP-1281 protocol.

## Features

- Full KWP-1281 implementation: 5-baud GPIO init, block send/receive, ACK, keepalive, exit
- Supported baud rates: 1200, 2400, 4800, 9600, 10400
- Supported ECU addresses: `0x01` (engine) and `0x17` (instruments/dashboard)
- Three KWP modes: ACK (keepalive only), group read, full sensor read
- 56-case sensor decode table (full VW/Audi KWP-1281 measurement type table)
- Read and clear DTC fault codes
- SSD1312 128×64 OLED display — full dashboard on a single 8-row screen, no paging needed
- Simulation mode for testing without a car
- Cooperative task scheduler (TaskScheduler) to prevent ECU timeouts
- Auto-setup shortcut: hold SELECT during splash to skip the setup menu

## Hardware

| Component | Details |
|---|---|
| Microcontroller | Arduino Uno (ATmega328P) |
| Display | SSD1312 128×64 OLED, I2C (SDA=A4, SCL=A5), I2C address 0x3C |
| Buttons | Standard LCD shield analog keypad on A0 |
| K-Line interface | Autodia K409 or similar KKL OBD-to-USB cable, wired to pins 2 (RX) and 3 (TX) |

### OLED wiring

```
SSD1312 module    Arduino Uno
──────────────────────────────
VCC           →   3.3V or 5V (check your module label)
GND           →   GND
SDA           →   A4
SCL           →   A5
```

I2C address is `0x3C` by default. If the display is unresponsive try `0x3D` (update `OLED_I2C_ADDR` in [src/display/Display.h](src/display/Display.h)).

### K-Line cable wiring

Open the KKL OBD cable, locate the FT232R(Q) MCU, and identify its RXD and TXD pins from the datasheet. Solder a wire from each pin to the Arduino:

- FT232 TXD → Arduino pin 2 (RX)
- FT232 RXD → Arduino pin 3 (TX)

Cut both traces on the KKL PCB after the solder point so the USB chip no longer drives the lines. Power the FT232 board from the Arduino's 5V and GND. You'll need a separate power source for the Arduino itself when the car is running (a USB power bank works).

See [assets/](assets/) for photos of the cable modification. Also see [mkirbst's project](https://github.com/mkirbst/lupo-gti-tripcomputer-kw1281) for additional wiring photos.

## Software setup

Install PlatformIO:
```
pip install platformio
# or on Arch Linux:
sudo pacman -S platformio-core
```

### Build and flash

```bash
# Build only
pio run

# Build and upload
pio run --target upload

# Run host-side model tests (no Arduino required)
pio test -e native
```

Or use the PlatformIO extension in VS Code — the Build and Upload buttons in the status bar do the right thing.

## Operation

### Startup

Power on with SELECT held → **auto-setup** skips the menus and connects with baud 10400, address 0x17.

Power on normally → interactive setup:

1. **Connect mode** — LEFT = ECU (real car), RIGHT = SIM (simulation without car)
2. **Baud rate** — LEFT/RIGHT to cycle, SELECT to confirm
3. **ECU address** — LEFT = 0x01 (engine), RIGHT = 0x17 (instruments)
4. **Press SELECT** to start the connection

### Navigation

| Button | Action |
|---|---|
| LEFT / RIGHT | Previous / next menu (Cockpit → Experimental → Debug → DTC → Settings) |
| UP / DOWN | Previous / next screen within the current menu |
| SELECT | Context action (see below) |

**Context actions by menu:**

- **Cockpit** — no SELECT action (data is live)
- **Experimental** — SELECT toggles between measurement slots 0/1 and 2/3
- **DTC screen 0** — SELECT reads fault codes from ECU
- **DTC screen 1** — SELECT clears fault codes on ECU
- **Settings screen 0** — SELECT exits the ECU session and returns to setup
- **Settings screen 1** — SELECT cycles KWP mode (ACK / GROUP / SENSOR)

### Cockpit screen layout (ADDR_INSTRUMENTS 0x17)

The full 128×64 OLED shows all data at once — no screen paging needed:

```
SPD:xxx  RPM:xxxx
CLT:xxx  OIL:xxx
OLV:x OPR:x AMB:xx
ODO:xxxxxxxxxx
FUL:xx  FSR:xxxxx
TIME:xxxxxxxxx
L/100:xxxxx L/h:xxxx
km:xxxxx  L:xxxxx
```

- `SPD` km/h, `RPM` engine speed, `CLT` coolant °C, `OIL` oil temp °C
- `OLV` oil level OK (0/1), `OPR` min oil pressure, `AMB` ambient temp °C
- `ODO` odometer km, `FUL` fuel level L, `FSR` fuel sender resistance Ω
- `TIME` ECU uptime seconds
- `L/100` fuel consumption per 100 km (since connect), `L/h` fuel per hour
- `km` trip distance since connect, `L` fuel burned since connect

### Cockpit screen layout (ADDR_ENGINE 0x01)

Two screens, UP/DOWN to switch:

```
Screen 0:           Screen 1:
RPM:xxxx  V:xxxxx   TBa:xxxxx STa:xxxxx
T1:xx T2:xx T3:xx   mbar:xxxx bits:xxxxxxxx
LAM:xxx LAM2:xxx
LOAD:xxx
```

## ECU label files

Each ECU address exposes measurement groups with 4 values each. What those values mean depends on your ECU. Known mapping for the 1.6 16V MARELLI ECU `036906034AM`:

```
ADDR_ENGINE = 0x01
  Group 1: RPM, temperature, lambda %, error bits
  Group 3: RPM, pressure mbar, throttle angle, steering angle
  Group 4: RPM, voltage V, temp °C, temp °C
  Group 6: RPM, engine load %, temp °C, lambda2 %

ADDR_INSTRUMENTS = 0x17  (label file: 1J0-920-xx0.LBL)
  Group 1: speed km/h, engine RPM, min oil pressure, ECU time
  Group 2: odometer km, fuel level L, fuel sender Ω, ambient temp °C
  Group 3: coolant temp °C, oil level OK, oil temp °C
```

If your ECU differs, use VCDS or any KWP tool to record your measurement groups, then update the signal mapping in [src/obd/KWP/KWPSensorDecode.cpp](src/obd/KWP/KWPSensorDecode.cpp).

## Project structure

```
src/
├── main.cpp                      # Arduino entry, TaskScheduler setup
├── Controller.h/cpp              # App coordinator
├── display/
│   └── Display.h                 # SSD1312 OLED wrapper (SSD1306Ascii, I2C)
├── scheduler/
│   └── TaskConfig.h              # Task intervals
├── serial/
│   └── NewSoftwareSerial.h/cpp   # Software serial for K-Line
└── obd/
    ├── OBDDisplay.h/cpp          # Main state machine (setup → connect → run)
    ├── KWP/
    │   ├── KWP1281Session.h/cpp  # Protocol: 5-baud init, blocks, keepalive
    │   ├── KWPSensorDecode.h/cpp # 56-case measurement type decode + signal mapping
    │   └── KWPBlocks.h           # Protocol constants
    ├── Display/
    │   ├── DisplayManager.h/cpp  # Screen routing
    │   └── screens/              # One file per screen
    │       ├── CockpitScreen
    │       ├── ExperimentalScreen
    │       ├── DebugScreen
    │       ├── DTCScreen
    │       └── SettingsScreen
    ├── Model/
    │   ├── OBDSignals.h/cpp      # Signal structs, simulation, computed stats
    │   └── DTCStore.h/cpp        # DTC code storage
    └── Input/
        ├── ButtonInput.h/cpp     # Analog keypad reader
        └── MenuState.h/cpp       # Menu/screen navigation state
```

## CI / Releases

Every push to `main` runs:

1. **Lint** — `clang-format` style check + `cppcheck` static analysis
2. **Build** — `pio run -e uno`, memory usage reported
3. **Test** — `pio test -e native` (model layer unit tests)
4. **Release** — semantic-release creates a GitHub release with the compiled `firmware.hex` attached (conventional commits: `feat:` → minor, `fix:` → patch, `BREAKING CHANGE:` → major)

## ECU emulator

If you don't have a car available, [OBDisplay-Emu](https://github.com/RXTX4816/OBDisplay-Emu) turns an Arduino Mega into a KWP-1281 ECU for bench testing.

## Caution

**Use at your own risk.** Wrong wiring can damage the ECU or create a fire hazard. This software only reads measurement groups and sends keepalive blocks — it does not write to or reprogram any ECU. The OBD port was designed for diagnostics only; increased workload on the ECU during driving is possible.

Do not use address `0x15` (airbag) to clear DTCs — on some affected ECUs this can deploy the airbag if an electrical fault is present in the airbag circuit.

Turn ignition ON (engine does not need to be running) before connecting.

## Credits

- [Blafusel KW1281 protocol reference](https://www.blafusel.de/obd/obd2_kw1281.html)
- [mkirbst's lupo-gti-tripcomputer-kw1281](https://github.com/mkirbst/lupo-gti-tripcomputer-kw1281)
