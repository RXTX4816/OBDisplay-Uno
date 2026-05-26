# OBDisplay-Uno

KWP-1281 K-Line trip computer for Arduino Uno with SH1107 OLED display (64×128, landscape mode work in progress).

Reads live sensor data and fault codes from VAG vehicles (Golf Mk4, Bora, Jetta, ~1998–2006) that use the K-Line OBD interface and the KWP-1281 protocol. 

## Features

- Full KWP-1281 implementation: 5-baud GPIO init, block send/receive, ACK, keepalive, exit
- Supported baud rates: 1200, 2400, 4800, 9600, 10400
- Supported ECU addresses: `0x01` (engine) and `0x17` (instruments/dashboard)
- Three KWP modes: ACK (keepalive only), group read, full sensor read
- 56-case sensor decode table (full VW/Audi KWP-1281 measurement type table)
- Read and clear DTC fault codes
- SH1107 64×128 OLED display (GME64128-02) — text-only rendering with batch I2C transfers (optimized for responsiveness)
- Simulation mode for testing without a car
- Cooperative task scheduler (TaskScheduler) to prevent ECU timeouts
- Auto-setup shortcut: hold SELECT during splash to skip the setup menu

## Hardware

| Component | Details |
|---|---|
| Microcontroller | Arduino Uno (ATmega328P, 16 MHz) |
| Display | SH1107 64×128 OLED (GME64128-02), I2C address 0x3C |
| Buttons | 5-way navigation switch (UP/DOWN/LEFT/RIGHT/SELECT) on pins 4–8 |
| K-Line interface | Modified KKL OBD-to-USB cable with FT232 MCU, wired to pins 2 (RX) and 3 (TX) |

### Complete pinout

```
Arduino Uno         Function
────────────────────────────────
GND                 Ground (K-Line and display)
5V                  Power (OLED and FT232)
A4 (SDA)            OLED I2C data
A5 (SCL)            OLED I2C clock
Pin 2 (SoftTX)      K-Line TX → to FT232 RXD (Software Serial)
Pin 3 (SoftRX)      K-Line RX ← from FT232 TXD (Software Serial)
Pin 4 (UP)          UP button (active LOW)
Pin 5 (DOWN)        DOWN button (active LOW)
Pin 6 (LEFT)        LEFT button (active LOW)
Pin 7 (RIGHT)       RIGHT button (active LOW)
Pin 8 (SELECT)      SELECT button (active LOW)
```

**Button Configuration:**
- All 5-way buttons use `INPUT_PULLUP` (active LOW)
- 50ms debounce via button timeout mechanism
- SELECT button timeout prevents accidental re-triggers (222 ms)

### OLED wiring (SH1107 I2C)

```
SH1107 module     Arduino Uno
──────────────────────────────
VCC           →   5V
GND           →   GND
SDA           →   A4 (pin 27)
SCL           →   A5 (pin 28)
```

**I2C Configuration:**
- I2C address: `0x3C` (default for SH1107)
- Clock speed: 100 kHz
- If unresponsive, try `0x3D` (update `OLED_I2C_ADDR` in [src/display/Display.h](src/display/Display.h))

**Display Specifications:**
- Resolution: 64×128 pixels (landscape orientation)
- 10 columns × 16 rows text grid (6px wide × 8px tall characters)
- Text-only rendering with on-demand page updates (no framebuffer)
- Batch I2C transfers for reduced latency and MCU overhead

### K-Line cable wiring (KKL OBD-to-USB)

The K-Line interface uses a modified KKL OBD-to-USB cable (Autodia K409 or compatible). The FT232R MCU is repurposed as a level shifter for the K-Line protocol.

**Cable modification steps:**

1. Open the KKL cable connector and locate the FT232R(Q) USB MCU
2. Identify TXD and RXD pins from the FT232 datasheet
3. Solder connections to Arduino (Software Serial — **NOT** hardware UART pins 0/1):
   - FT232 TXD → Arduino pin 3 (software serial RX)
   - FT232 RXD → Arduino pin 2 (software serial TX)
4. **Cut both traces** on the KKL PCB immediately after the solder points to isolate the USB chip from the OBD lines
5. Power the FT232 board from Arduino 5V and GND
6. Connect the original K-Line and GND lines from the OBD-9 connector to the FT232 board

**Power supply:**
- Arduino + display: USB power or external 5V
- When connected to car: **separate power source recommended** (e.g., USB power bank or car 12V buck converter) to avoid ground loop issues
- The FT232 board draws ~50 mA, Arduino + OLED ~150 mA total

**Why Software Serial?**
- Hardware UART (pins 0/1) reserved for USB serial monitor and programming
- Software serial (NewSoftwareSerial) on pins 2/3 avoids conflicts with debug output

**Protocol notes:**
- K-Line operates at 5 baud initially (7 bits, no parity, 1 stop bit)
- 5-baud initialization: GPIO pulse (~200 ms LOW, then HIGH) on pin 3 before standard serial communication
- Standard baud rates (1200–10400) handled after 5-baud init
- See [assets/](assets/) for photos of cable modification and [mkirbst's project](https://github.com/mkirbst/lupo-gti-tripcomputer-kw1281) for detailed wiring examples


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

Displays full vehicle status on a single 64×128 screen (10 columns × 16 rows). All data updates simultaneously — no paging or scrolling needed.

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

**Data fields:**
- `SPD` vehicle speed (km/h)
- `RPM` engine speed (rev/min)
- `CLT` coolant temperature (°C)
- `OIL` oil temperature (°C)
- `OLV` oil level OK (1=OK, 0=low)
- `OPR` minimum oil pressure (bar)
- `AMB` ambient temperature (°C)
- `ODO` odometer reading (km)
- `FUL` fuel level (L)
- `FSR` fuel sender resistance (Ω)
- `TIME` ECU uptime (seconds)
- `L/100` consumption rate (L/100km) since connection
- `L/h` hourly fuel consumption (L/h)
- `km` trip distance (km) since connection
- `L` fuel burned (L) since connection

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
│   └── Display.h                 # SH1107 OLED wrapper (SSD1306Ascii, I2C)
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

## Display rendering optimization

The SH1107 driver uses a **text-only, on-demand rendering strategy** optimized for the 2 KB RAM limit:

### Memory layout
- **No framebuffer** — renders directly to I2C on-the-fly (saves ~920 bytes)
- **Entry buffer** — stores up to 20 text entries (position, string) per frame
- **Page-by-page rendering** — 128 pixels tall = 16 pages, rendered individually during flush
- **Batch I2C** — all writes to display are grouped into single transfers (reduces MCU overhead)

### Update strategy
- Display only re-renders when **menu state changes** (user navigation) OR on a **fixed 177 ms timer**
- Debug output **never triggers a refresh** — no serial spam in the display loop
- Failed ECU connections properly transition back to setup screens (not stale data)

### I2C communication
```
Port: I2C (Wire library)
Speed: 100 kHz (stable for 5V operation)
Chunking: 16 bytes per transmission packet
Total per frame: ~16 transactions (one per page)
Typical latency: <50 ms for full screen update
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
