#pragma once

#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

// SH1107 64x128 OLED (GME64128-02) - 64 COM lines, 128 SEG lines
// Initialized for landscape use: 128 pixels wide x 64 pixels tall
// I2C address: 0x3C
static constexpr uint8_t OLED_I2C_ADDR = 0x3C;

// Custom SH1107 landscape initialization
static const uint8_t sh1107_init[] PROGMEM = {
    0xAE,        // display off
    0xD5, 0x80,  // clock: divide=1, osc=8
    0xA8, 0x3F,  // multiplex: 64 (for 64 COM lines)
    0xD3, 0x00,  // display offset: 0
    0x40,        // start line: 0
    0xAD, 0x8B,  // charge pump: on
    0xA1,        // segment remap: flipped
    0xC8,        // COM scan: reversed
    0xDA, 0x12,  // COM pins: alternative
    0x81, 0x80,  // contrast: 128
    0xD9, 0x1F,  // pre-charge period
    0xDB, 0x40,  // VCOM deselect
    0xA4,        // display resume (use GDDRAM)
    0xA6,        // normal display (not inverted)
    0xAF,        // display on
};

static const DevType sh1107_landscape PROGMEM = {
    sh1107_init,
    sizeof(sh1107_init),
    128,  // width (for landscape display)
    64,   // height (for landscape display)
    0,    // col_offset
};

class Display
{
  public:
    Display() = default;

    void begin()
    {
        Wire.begin();
        oled_.begin(&sh1107_landscape, OLED_I2C_ADDR);
        oled_.setFont(System5x7);
        oled_.clear();
    }

    void clear() { oled_.clear(); }

    void setCursor(uint8_t col, uint8_t row) { oled_.setCursor(col * 6, row); }

    void print(const char* s) { oled_.print(s); }
    void print(const __FlashStringHelper* s) { oled_.print(s); }
    void print(int32_t n) { oled_.print(n); }

    // Overloads for OBDDisplay compatibility
    void print(uint8_t col, uint8_t row, const __FlashStringHelper* s)
    {
        setCursor(col, row);
        print(s);
    }
    void print(uint8_t col, uint8_t row, const String& s)
    {
        setCursor(col, row);
        print(s.c_str());
    }
    void print(uint8_t col, uint8_t row, const char* s)
    {
        setCursor(col, row);
        print(s);
    }
    void print(uint8_t col, uint8_t row, int32_t n)
    {
        setCursor(col, row);
        print(n);
    }

  private:
    SSD1306AsciiWire oled_;
};
