# Development

## Project structure

```
src/
├── main.cpp                      # Arduino entry point, hand-rolled cooperative task loop
├── Config.h                      # All user-tunable constants (pins, thresholds, intervals)
├── Controller.h/cpp              # Application coordinator; wires tasks to OBDDisplay
├── debug.h                       # DBG() / DBGV() macros (OBD_DEBUG guard)
├── display/
│   └── Display.h/cpp             # SH1107 OLED driver (no-framebuffer, on-demand I2C)
├── scheduler/
│   └── TaskConfig.h              # Task interval constants
├── serial/
│   └── NewSoftwareSerial.h/cpp   # Software serial for K-Line
└── obd/
    ├── OBDDisplay.h/cpp          # Main state machine (Setup → WaitingForConnect → Running)
    ├── OBDDisplay_input.cpp      # Button handling and menu action dispatch
    ├── OBDDisplay_setup.cpp      # Startup animation, setup flow, connect/reconnect logic
    ├── Buzzer.h/cpp              # Optional active buzzer: warning beep patterns
    ├── KWP/
    │   ├── KWP1281Session.h/cpp  # Protocol: 5-baud init, blocks, keepalive, DTC
    │   ├── KWPSensorDecode.h/cpp # 56-case measurement type decode + signal mapping
    │   └── KWPBlocks.h           # Protocol constants
    ├── Display/
    │   ├── DisplayManager.h/cpp  # Screen routing by MenuId
    │   ├── ScreenVM.h/cpp        # Bytecode VM for PROGMEM-driven screen layouts
    │   └── screens/              # One .h/.cpp pair per screen
    │       ├── CockpitScreen     # 0x17 (4 screens) and 0x01 (7 screens)
    │       ├── ExperimentalScreen
    │       ├── DebugScreen
    │       ├── DTCScreen
    │       ├── SettingsScreen
    │       └── ScreenHelpers.h
    ├── Model/
    │   ├── OBDSignals.h/cpp      # Signal structs, computed stats, warning state (10 warnings)
    │   └── DTCStore.h/cpp        # DTC code storage (up to 16 codes)
    └── Input/
        ├── ButtonInput.h/cpp     # Button polling with debounce and auto-repeat
        └── MenuState.h/cpp       # Menu/screen navigation state machine
```

## Build environments

| Environment | Command | Notes |
|---|---|---|
| `uno` | `pio run -e uno` | Production: smallest binary, all `DBG()` expanded to nothing |
| `uno_debug` | `pio run -e uno_debug` | Debug build: binary frame logging over USB serial |
| `native` | `pio test -e native` | Unit tests on the host (no Arduino required) |
| `uno_sim` | `pio test -e uno_sim` | The same unit tests on a simulated ATmega328P (simavr) |

The `uno` and `uno_debug` builds run `tools/check_no_float.py` after linking and fail if any AVR soft-float symbol ends up in the firmware (~600 bytes for a single float expression). Use fixed-point integer math instead.

## Build macros

These `#define` flags gate optional functionality. Add them to `build_flags` in `platformio.ini`.

| Macro | Environment | Effect |
|---|---|---|
| `OBD_DEBUG` | `uno_debug` | Enable binary serial debug logging |
| `OBD_EXPERIMENTAL_SCREENS` | `uno_debug` | Include ExperimentalScreen and DebugScreen content |

## Binary debug logging (`OBD_DEBUG`)

When built with `-D OBD_DEBUG`, the firmware emits compact 5-byte binary frames over the hardware serial port (USB, 115200 baud):

```
0xAA  <code>  <val_hi>  <val_lo>  0x55
```

Decode in real time with the included Python script:

```bash
pip install pyserial
python tools/dbg_monitor.py --port /dev/ttyUSB0
# Windows:
python tools/dbg_monitor.py --port COM3
```

### Event code table

| Code | Name | Description |
|---|---|---|
| `0x01` | `KWP_CONNECT` | Connecting; baud = val × 100 |
| `0x02` | `KWP_5BAUD_START` | 5-baud init started |
| `0x03` | `KWP_5BAUD_DONE` | 5-baud init done |
| `0x04` | `KWP_SYNC_WAIT` | Waiting for ECU sync bytes |
| `0x05` | `KWP_SYNC_FAIL` | Sync bytes receive failed |
| `0x06` | `KWP_SYNC_MISMATCH` | Sync bytes mismatch; val = first byte received |
| `0x07` | `KWP_SYNC_OK` | Sync OK; val = first byte (expect `0x55`) |
| `0x08` | `KWP_BLOCKS_READ` | Reading device data blocks |
| `0x09` | `KWP_BLOCKS_FAIL` | Device data read failed |
| `0x0A` | `KWP_TIMEOUT` | `receiveBlock_` timeout; val = bytes received so far |
| `0x0B` | `KWP_COMPLEMENT` | Complement mismatch; val = byte index |
| `0x0C` | `KWP_KEEPALIVE_TX` | Keep-alive send ACK failed |
| `0x0D` | `KWP_KEEPALIVE_RX` | Keep-alive receive ACK failed |
| `0x10` | `DISP_INIT` | `Display::begin()` starting |
| `0x11` | `DISP_WIRE_OK` | I2C initialized at 100 kHz |
| `0x12` | `DISP_OFF` | Sending display OFF command |
| `0x13` | `DISP_SEQ` | Sending init sequence |
| `0x14` | `DISP_INIT_DONE` | Init complete |
| `0x15` | `DISP_CLEAR` | Clearing display |
| `0x16` | `DISP_READY` | Display cleared and ready |
| `0x20` | `CTRL_STEP` | Startup step; val = step number (1–3) |

In production builds all `DBG()` / `DBGV()` macros expand to nothing — zero flash cost.

## Display rendering

The SH1107 driver uses a **text-only, on-demand rendering strategy** to stay within the 2 KB RAM constraint.

- **No framebuffer** — renders directly over I2C, saving ~920 bytes
- **Entry buffer** — up to 20 text entries (position + string) queued per frame
- **Page-by-page rendering** — 128 px tall = 16 pages; rendered individually during flush. Small-font glyphs are written as whole column bytes and the 2× font as pre-doubled columns, so a flush costs little CPU time
- **Batch I2C** — all writes grouped into 16-byte transfers (~16 transactions per full-screen update)
- **Conditional refresh** — re-renders only on menu state change OR on the 177 ms timer

## Task scheduler

`TaskScheduler` runs cooperative tasks to prevent ECU keepalive timeouts. Button polling runs at a higher frequency than the main `update()` loop; pressed states are latched in `pendingBtns_` and consumed by `handleInput_()` on the next cycle.

## CI/CD

Every push and pull request runs three jobs:

1. **Lint** — `clang-format` style check + `cppcheck` static analysis
2. **Build** — `pio run -e uno -e uno_debug`, flash and RAM usage reported. A debug image that no longer fits in flash fails CI.
3. **Test** — `pio test -e native`, then `pio test -e uno_sim`

### Releases

After every merge to `main` whose CI passes, the **Semantic Release** workflow starts by itself and waits for approval in the `release` environment (a required reviewer approves it on the run page). It tags the exact commit CI tested, bumping the version from the conventional commits since the last tag (`feat:` → minor, `fix:` → patch, `BREAKING CHANGE:` → major). If there are no new commits it ends without a release. It can still be started by hand (Actions → Semantic Release → Run workflow), which waits for the same approval.

The tag triggers the **Release** workflow, which builds `uno` and `uno_debug` and attaches:

- `OBDisplay-Uno-<version>.hex` and `.elf`
- `OBDisplay-Uno-<version>-memory.json` — RAM/flash usage of both builds (from `tools/memory_report.py`), also shown as a *Memory usage* table in the release notes

It also force-pushes four shields.io badge files to the orphan `badges` branch; the RAM/flash badges in the README read from there and update on every release. That branch is never merged.

Wiki pages in `docs/wiki/` are automatically synced to the GitHub Wiki on each push to `main`.

## Unit tests

Tests live in `test/`, one folder per suite:

| Suite | Covers |
|---|---|
| `test_model` | Signal computation, fuel smoothing, range and consumption, DTC store |
| `test_warnings` | Warning thresholds, fuel dwell and hysteresis, sub-zero temperatures |
| `test_kwp_decode` | KWP-1281 measurement decoding for both ECUs |
| `test_menu` | Menu and screen navigation |
| `test_display_render` | Font renderers and page buffers |
| `test_display_screens` | Cockpit and other screen layouts |

Every suite runs in two places:

```bash
pio test -e native    # host: fast, easy to debug
pio test -e uno_sim   # simulated Uno: 16-bit int, avr-libc, real PROGMEM
```

The host has 32-bit `int`, treats `PROGMEM` as plain memory and does floats in hardware, so bugs like fixed-point overflow or a flash table read without `pgm_read_*` only show up on `uno_sim`. It builds each suite with the production compiler flags and runs it under simavr (installed by PlatformIO as `tool-simavr`). A run is capped at 180 s, so a hang or stack overflow fails the suite instead of stalling CI. `test/unity_runner.h` provides the shared entry point (`main()` on the host, `setup()` on AVR).

Or locally with the CI script:

```bash
bash run-ci-local.sh
```
