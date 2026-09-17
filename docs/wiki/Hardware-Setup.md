# Hardware Setup

## Components

| Component | Details |
|---|---|
| Microcontroller | Arduino Uno (ATmega328P, 16 MHz) |
| Display | SH1107 64×128 OLED (GME64128-02), I2C address `0x3C` |
| Buttons | 5-way navigation switch (UP/DOWN/LEFT/RIGHT/SELECT) |
| K-Line interface | Modified KKL OBD-to-USB cable with FT232 MCU |
| Buzzer (optional) | Active buzzer, e.g. SFM-27 (3–24 V DC, ≤ 20 mA) |

## Pinout

```
Arduino Uno         Function
────────────────────────────────
GND                 Ground (K-Line and display)
5V                  Power (OLED and FT232)
A4 (SDA)            OLED I2C data
A5 (SCL)            OLED I2C clock
Pin 2 (SoftTX)      K-Line TX → FT232 RXD (Software Serial)
Pin 3 (SoftRX)      K-Line RX ← FT232 TXD (Software Serial)
Pin 4               UP button (active LOW, INPUT_PULLUP)
Pin 5               DOWN button (active LOW, INPUT_PULLUP)
Pin 6               LEFT button (active LOW, INPUT_PULLUP)
Pin 7               RIGHT button (active LOW, INPUT_PULLUP)
Pin 8               SELECT button (active LOW, INPUT_PULLUP)
Pin 10              Buzzer + (optional, HIGH = sound)
```

Hardware UART (pins 0/1) is reserved for USB serial monitoring and programming. The K-Line interface uses Software Serial on pins 2/3 to avoid conflicts.

## OLED (SH1107)

```
SH1107 module     Arduino Uno
──────────────────────────────
VCC           →   5V
GND           →   GND
SDA           →   A4
SCL           →   A5
```

- I2C address: `0x3C` (default). If unresponsive, try `0x3D` and update `OLED_I2C_ADDR` in `src/display/Display.h`.
- Clock: 100 kHz (stable for 5V operation)
- Resolution: 64×128 px, landscape → 10 columns × 16 rows text grid (6×8 px characters)

## Button wiring

All five buttons wire between their Arduino pin and GND. Internal pull-ups are enabled (`INPUT_PULLUP`), so unpressed = HIGH, pressed = LOW.

- 50 ms debounce for directional buttons
- 222 ms timeout on SELECT to prevent accidental re-triggers

## Buzzer (optional)

An **active** buzzer (one with its own oscillator, which sounds on plain DC) beeps when a new medium or critical warning appears. It is driven directly from pin 10; no resistor or transistor is needed as long as it draws ≤ 20 mA at 5 V (e.g. SFM-27).

```
Buzzer            Arduino Uno
──────────────────────────────
Red (+)       →   Pin 10
Black (−)     →   GND
```

- If all GND header pins are taken, the ICSP header has one: the 2×3 block, pin 6. Pin 1 has the square solder pad on the underside; GND is the diagonally opposite corner.
- **Passive** buzzers (only click on DC, e.g. 12085 42 Ω) are not supported by the firmware and would need a series resistor (≥ 220 Ω) to protect the pin.
- A short chirp at power-on confirms the wiring.
- Beep patterns and the pin are set in `src/Config.h` (`BUZZER_PIN`, `BUZZER_BEEP_COUNT`, `BUZZER_BEEP_MS`, `BUZZER_GAP_MS`). Comment out `BUZZER_PIN` to build without a buzzer. See [Screen Reference](Screen-Reference#warnings) for which warning plays which pattern.

## K-Line cable modification

The K-Line interface uses a modified **KKL OBD-to-USB cable** (Autodia K409 or compatible). The FT232R/FT232RQ USB-to-serial chip is repurposed as a level shifter.

**Steps:**

1. Open the KKL cable connector and locate the FT232R(Q) chip.
2. Identify TXD and RXD pins from the FT232 datasheet.
3. Solder connections to Arduino Software Serial pins:
   - FT232 **TXD** → Arduino **pin 3** (software serial RX)
   - FT232 **RXD** → Arduino **pin 2** (software serial TX)
4. **Cut both PCB traces** immediately after the solder points to isolate the FT232 from the OBD lines.
5. Power the FT232 board from Arduino **5V** and **GND**.
6. Connect the original K-Line wire and GND from the OBD-9 connector to the FT232 board.

See `assets/` in the repository for photos of the modification.

## Power

| Rail | Draw |
|---|---|
| FT232 board | ~50 mA |
| Arduino Uno | ~30 mA |
| SH1107 OLED | ~20 mA |
| Buzzer (optional, while beeping) | ≤ 20 mA |
| **Total** | **~150 mA** |

When connected to a car, use a separate power source (USB power bank or 12V buck converter) to avoid ground loop issues through the OBD port.
