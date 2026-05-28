# Development

## Project structure

```
src/
├── main.cpp                      # Arduino entry point, TaskScheduler setup
├── Controller.h/cpp              # Application coordinator
├── debug.h                       # DBG() / DBGV() macros (OBD_DEBUG guard)
├── display/
│   └── Display.h                 # SH1107 OLED wrapper (SSD1306Ascii, I2C)
├── scheduler/
│   └── TaskConfig.h              # Task intervals
├── serial/
│   └── NewSoftwareSerial.h/cpp   # Software serial for K-Line
└── obd/
    ├── OBDDisplay.h/cpp          # Main state machine (Setup → WaitingForConnect → Running)
    ├── KWP/
    │   ├── KWP1281Session.h/cpp  # Protocol: 5-baud init, blocks, keepalive, DTC
    │   ├── KWPSensorDecode.h/cpp # 56-case measurement type decode + signal mapping
    │   └── KWPBlocks.h           # Protocol constants
    ├── Display/
    │   ├── DisplayManager.h/cpp  # Screen routing by MenuId
    │   └── screens/              # One .h/.cpp pair per screen
    │       ├── CockpitScreen
    │       ├── ExperimentalScreen
    │       ├── DebugScreen
    │       ├── DTCScreen
    │       └── SettingsScreen
    ├── Model/
    │   ├── OBDSignals.h/cpp      # Signal structs, simulation mode, computed stats
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
| `native` | `pio test -e native` | Host-side model unit tests (no Arduino required) |

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
- **Page-by-page rendering** — 128 px tall = 16 pages; rendered individually during flush
- **Batch I2C** — all writes grouped into 16-byte transfers (~16 transactions per full-screen update)
- **Conditional refresh** — re-renders only on menu state change OR on the 177 ms timer

## Task scheduler

`TaskScheduler` runs cooperative tasks to prevent ECU keepalive timeouts. Button polling runs at a higher frequency than the main `update()` loop; pressed states are latched in `pendingBtns_` and consumed by `handleInput_()` on the next cycle.

## CI/CD

Every push to `main` runs three steps:

1. **Lint** — `clang-format` style check + `cppcheck` static analysis
2. **Build** — `pio run -e uno`, flash and RAM usage reported
3. **Test** — `pio test -e native` (model layer unit tests)

Releases are created via the **Semantic Release** workflow (Actions → Semantic Release → Run workflow). It reads conventional commits since the last tag and bumps the version (`feat:` → minor, `fix:` → patch, `BREAKING CHANGE:` → major), creates a tag, and triggers the **Release** workflow which uploads `firmware.hex` and `firmware.elf`.

Wiki pages in `docs/wiki/` are automatically synced to the GitHub Wiki on each push to `main`.

## Unit tests

Tests live in `test/` and target the `native` environment (Linux x86, no Arduino dependency):

- `test_dtc_store.cpp` — DTCStore read/clear/overflow
- `test_obd_signals.cpp` / `test_obd_signals_more.cpp` — sensor decode and signal computation

Run with:

```bash
pio test -e native
```

Or locally with the CI script:

```bash
bash run-ci-local.sh
```
