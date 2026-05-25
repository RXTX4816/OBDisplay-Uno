#pragma once

#include <Arduino.h>

// SH1107 64x128 OLED, I2C address 0x3C.
//
// The panel is portrait-native (64 wide x 128 tall). We drive it in LANDSCAPE
// (128 wide x 64 tall) by holding a 1KB framebuffer and rotating every pixel
// 90 degrees in setPixel(). This is the only way to get upright landscape text
// on this controller: SH1107 segment-remap/COM-scan can only mirror, not rotate.
//
// Drawing model: text on a 21-col x 8-row grid (6x8 cells, 5x7 glyphs).
// Flushing: outside a batch, every draw call auto-sends the buffer over I2C.
// Inside beginBatch()/endBatch() nothing is sent until the outermost endBatch(),
// so a full frame (many fields) becomes a single I2C transfer. See Display.cpp.
class Display
{
  public:
    Display() = default;

    void begin();
    void clear();
    void flush();

    void beginBatch();
    void endBatch();

    void setCursor(uint8_t col, uint8_t row);

    void print(const char* s);
    void print(const __FlashStringHelper* s);
    void print(int32_t n);

    // Convenience overloads (set cursor + print) kept for API compatibility.
    void print(uint8_t col, uint8_t row, const __FlashStringHelper* s)
    {
        setCursor(col, row);
        print(s);
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

    static constexpr uint8_t WIDTH = 128; // landscape pixels
    static constexpr uint8_t HEIGHT = 64;
    static constexpr uint8_t COLS = 21; // 6px text cells across 128
    static constexpr uint8_t ROWS = 8;  // 8px rows down 64

  private:
    void drawChar(uint8_t col, uint8_t row, char c);
    void setPixel(uint8_t lx, uint8_t ly, bool on);
    void markDirty()
    {
        dirty_ = true;
        if (batchDepth_ == 0)
            flush();
    }

    uint8_t buf_[1024] = {}; // native layout: 64 columns x 16 pages
    uint8_t cursorCol_ = 0;
    uint8_t cursorRow_ = 0;
    uint8_t batchDepth_ = 0;
    bool dirty_ = false;
};
