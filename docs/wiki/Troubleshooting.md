# Troubleshooting

## Display is blank or unresponsive

- Check SDA/SCL wiring (A4/A5 on the Uno).
- Confirm the I2C address. The default is `0x3C`; some SH1107 modules ship with `0x3D`. Update `OLED_I2C_ADDR` in `src/display/Display.h` and reflash.
- Run the `i2c_scanner_sketch.ino` from the `docs/` folder to confirm the address.

## ECU does not respond / connection fails immediately

- Ignition must be **ON** (engine does not need to be running).
- Verify the baud rate. Most VAG K-Line ECUs from ~1998–2006 use **10400 baud**. Try each rate from the setup menu.
- Double-check the K-Line wiring polarity — FT232 TXD → Arduino pin 3, FT232 RXD → Arduino pin 2.
- Confirm PCB traces were cut after soldering to the FT232 (see [Hardware Setup](Hardware-Setup)).
- Use the debug build to read event codes from `dbg_monitor.py`:
  - `KWP_SYNC_FAIL` or `KWP_SYNC_MISMATCH` — K-Line signal not reaching the Arduino, or wrong polarity.
  - `KWP_TIMEOUT` with val = 0 — no bytes received at all; check wiring and power.
  - `KWP_TIMEOUT` with val > 0 — partial response; ECU address may be wrong.

## ECU connects then drops / keepalive failures

- `KWP_KEEPALIVE_TX` or `KWP_KEEPALIVE_RX` in the debug log indicates the ECU is timing out between blocks.
- The cooperative task scheduler is tuned to keep the OBD communication cycle well within typical ECU timeout windows. If this occurs only under load, check that the display update loop is not blocking.
- Power supply instability can cause intermittent disconnects — try a dedicated USB power bank.

## Stale or incorrect sensor values

- Confirm the ECU address and measurement group mapping for your specific ECU. The default mapping targets the instruments cluster (`0x17`) of a 1J platform VW with a Marelli 1.6 16V engine ECU (`0x01`). Other ECUs have different group contents.
- Use a VCDS or equivalent KWP tool to record raw group data and compare against what `KWPSensorDecode.cpp` expects.

## Flash too full

- Production build (`pio run -e uno`) fits at ~96% flash. The debug build (`uno_debug`) reaches 100% on the standard Uno.
- If you add features, monitor `pio run` output for flash/RAM percentages. RAM overflows cause silent crashes.
- Disable `OBD_EXPERIMENTAL_SCREENS` if not needed — it saves meaningful flash.

## Upload fails / `avrdude` errors

- Check the port name: `/dev/ttyUSB0` (Linux), `COM3` (Windows), `/dev/cu.usbmodem*` (macOS).
- On Linux, add your user to the `dialout` (or `uucp` on Arch) group and log out/in.
- The 115200 baud rate used by `pio run --target upload` must match the Arduino Uno bootloader. Do not change it.

## Button press not registering

- Buttons use `INPUT_PULLUP`; they must connect pin to **GND** (not 5V).
- The 50 ms debounce or 222 ms SELECT timeout may absorb very short presses. Hold buttons slightly longer if inputs are being dropped.
- Auto-repeat fires on UP/DOWN/LEFT/RIGHT after the repeat threshold — this is intentional for fast menu scrolling.

## ECU emulator connection issues

When using [OBDisplay-Emu](https://github.com/RXTX4816/OBDisplay-Emu) for bench testing, verify the baud rate and ECU address match what the emulator expects, and confirm K-Line wiring is correct.
