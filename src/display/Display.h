#pragma once

#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

// SSD1306/SSD1312 128x64 OLED over I2C (SDA=A4, SCL=A5).
// SSD1306Ascii — no framebuffer, ~2KB flash overhead.
// System5x7 font: 20 cols × 8 rows visible (6px wide chars).
// I2C address: 0x3C (solder bridge for 0x3D if needed).
static constexpr uint8_t OLED_I2C_ADDR = 0x3C;

class Display
{
  public:
    Display() = default;

    void begin()
    {
        Wire.begin();
        oled_.begin(&Adafruit128x64, OLED_I2C_ADDR);
        oled_.setFont(System5x7);
        oled_.clear();
    }

    void clear() { oled_.clear(); }

    void setCursor(uint8_t col, uint8_t row) { oled_.setCursor(col * 6, row); }

    void print(const char* s) { oled_.print(s); }
    void print(const __FlashStringHelper* s) { oled_.print(s); }
    void print(int32_t n) { oled_.print(n); }

  private:
    SSD1306AsciiWire oled_;
};
