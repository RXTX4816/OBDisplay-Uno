# Getting Started

## Option A: Flash pre-built firmware (no toolchain required)

1. Download `OBDisplay-Uno-<version>.hex` from the [Releases](https://github.com/RXTX4816/OBDisplay-Uno/releases) page.
2. Flash with `avrdude` (included with the Arduino IDE, or install separately):

```bash
avrdude -c arduino -p atmega328p -P /dev/ttyUSB0 -b 115200 \
  -U flash:w:OBDisplay-Uno-<version>.hex:i
```

Replace `/dev/ttyUSB0` with your port (`COM3` on Windows, `/dev/cu.usbmodem*` on macOS).

The release also ships `OBDisplay-Uno-<version>.elf` for symbol-level debugging (`avr-gdb`, `avr-nm`). You cannot flash it directly.

## Option B: Build from source

### Install PlatformIO

```bash
pip install platformio
# Arch Linux:
sudo pacman -S platformio-core
```

Or install the [PlatformIO extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) for VS Code.

### Clone and build

```bash
git clone https://github.com/RXTX4816/OBDisplay-Uno.git
cd OBDisplay-Uno

# Build only
pio run -e uno

# Build and upload
pio run -e uno --target upload

# Run host-side unit tests (no Arduino required)
pio test -e native
```

### Build environments

| Environment | Command | Use |
|---|---|---|
| `uno` | `pio run -e uno` | Production — smallest binary, no serial output |
| `uno_debug` | `pio run -e uno_debug` | Debug build — binary frame logging over USB serial |
| `native` | `pio test -e native` | Host-side model unit tests |

## First use

Power on normally → interactive setup:

0. **Preset** — LEFT/RIGHT to cycle `Manual/off` / `0x01 9600` (engine) / `0x17 10400` (instruments), SELECT to confirm. A preset fills in baud and address and jumps straight to auto-reconnect; `Manual/off` continues with the baud screen.
1. **Baud rate** — LEFT/RIGHT to cycle (1200 / 2400 / 4800 / 9600 / 10400), SELECT to confirm
2. **ECU address** — LEFT/RIGHT to cycle supported addresses, SELECT to confirm
3. **Auto-reconnect** — LEFT = off (manual), RIGHT = on (auto)

UP goes back one screen.

### Boot autoconnect (saved in EEPROM)

On the **Preset** screen, press **DOWN** to save the shown preset as boot autoconnect. The line under `DN:boot` shows what is currently saved.

| Saved value (EEPROM byte 2) | Preset | Boot behaviour |
|---|---|---|
| `0` | `Manual/off` | Interactive setup (default) |
| `1` | `0x01 9600` | Skips setup, connects to the engine ECU at 9600 baud |
| `2` | `0x17 10400` | Skips setup, connects to the instruments cluster at 10400 baud |

The setting persists across power cycles and takes priority on every boot. To change or disable it, **hold SELECT during power-on**: the saved preset is ignored for this boot and the setup opens. Then select another preset and press DOWN, or press DOWN on `Manual/off` to turn it off. The setup is also reachable via **Settings → Exit** while connected.

Once connected the Cockpit screen appears and data updates live.

## ECU emulator (bench testing)

[OBDisplay-Emu](https://github.com/RXTX4816/OBDisplay-Emu) turns an Arduino Mega into a KWP-1281 ECU emulator for bench testing without a car.
