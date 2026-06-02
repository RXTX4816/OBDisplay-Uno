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

Power on with **SELECT held** → auto-setup: skips the menus and connects immediately at baud 10400, address `0x17` (instruments).

Power on normally → interactive setup:

1. **Baud rate** — LEFT/RIGHT to cycle (1200 / 2400 / 4800 / 9600 / 10400), SELECT to confirm
2. **ECU address** — LEFT/RIGHT to cycle supported addresses, SELECT to confirm
3. **Auto-reconnect** — LEFT = off (manual), RIGHT = on (auto)

Once connected the Cockpit screen appears and data updates live.

## ECU emulator (bench testing)

[OBDisplay-Emu](https://github.com/RXTX4816/OBDisplay-Emu) turns an Arduino Mega into a KWP-1281 ECU emulator for bench testing without a car.
