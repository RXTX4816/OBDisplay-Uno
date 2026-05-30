// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>

// SH1107 64x128 OLED, I2C address 0x3C.
//
// Portrait-native (64 wide × 128 tall). Text-only rendering: no framebuffer.
// 10 cols × 16 rows grid (6px wide chars, 8px tall). Store text entries and
// render on-the-fly during flush (page-by-page) to save ~920 bytes of SRAM.
//
// Drawing model: accumulate text entries in a small table (20 max), then render
// all at once during flush. Each entry stores position, text string.
// Flushing: outside a batch, every draw call auto-sends all entries to I2C.
// Inside beginBatch()/endBatch() nothing is sent until endBatch(), allowing a
// full frame to become a single I2C transfer.
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

    // 2× pixel-doubled text at arbitrary pixel coordinates.
    void printBig(uint8_t x_px, uint8_t y_px, const char* s);
    void printBig(uint8_t x_px, uint8_t y_px, int32_t n);

    static constexpr uint8_t WIDTH = 64; // portrait pixels
    static constexpr uint8_t HEIGHT = 128;
    static constexpr uint8_t COLS = 10; // 64 / 6 ≈ 10 text columns
    static constexpr uint8_t ROWS = 16; // 128 / 8 = 16 text rows

  private:
    static constexpr uint8_t kMaxEntries = 20;
    static constexpr uint8_t kTextLen = 11; // 10 chars + null terminator

    struct TextEntry
    {
        uint8_t x = 0;            // pixel column (0-63)
        uint8_t line = 0;         // scale=1: row (0-15); scale=2: raw pixel y (0-127)
        uint8_t scale = 1;        // 1 = normal 5x7; 2 = 2x pixel-doubled 10x14
        char text[kTextLen] = {}; // text content, always copied here
    };

    void drawCharToPage(uint8_t x, uint8_t y, char c, uint8_t page, uint8_t* pageBuf);
    void drawChar2xToPage(uint8_t x, uint8_t y, char c, uint8_t page, uint8_t* pageBuf);
    void addTextEntry(uint8_t x, uint8_t line, const char* text, uint8_t scale = 1);
    void markDirty()
    {
        dirty_ = true;
        if (batchDepth_ == 0)
            flush();
    }

    TextEntry entries_[kMaxEntries];
    uint8_t entryCount_ = 0;
    uint8_t cursorCol_ = 0;
    uint8_t cursorRow_ = 0;
    uint8_t batchDepth_ = 0;
    bool dirty_ = false;
};
