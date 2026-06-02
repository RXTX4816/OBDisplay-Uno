// SPDX-License-Identifier: GPL-3.0-or-later
#include "Display.h"
#include "../debug.h"
#include <avr/io.h>

namespace
{
constexpr uint8_t OLED_I2C_ADDR = 0x3C;

// Minimal blocking TWI driver — master TX only, single fixed address.
// Replaces Arduino Wire (~1.3 KB) for write-only OLED communication.
static void twiInit()
{
    PORTC |= (1 << PC4) | (1 << PC5); // pull-ups on SDA/SCL
    TWSR = 0;                         // prescaler = 1
    TWBR = 72;                        // 100 kHz @ 16 MHz: (16e6/100e3 - 16) / 2
}

static void twiWait()
{
    while (!(TWCR & (1 << TWINT)))
        ;
}

static void twiStart(uint8_t addr)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    twiWait();
    TWDR = (uint8_t)(addr << 1); // write direction
    TWCR = (1 << TWINT) | (1 << TWEN);
    twiWait();
}

static void twiWrite(uint8_t b)
{
    TWDR = b;
    TWCR = (1 << TWINT) | (1 << TWEN);
    twiWait();
}

static void twiStop()
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
    while (TWCR & (1 << TWSTO))
        ;
}

// SH1107 initialization sequence (from U8g2 driver, proven working).
// Each command is sent individually with delays for stability.
const uint8_t PROGMEM kInit[] = {
    0xAE,       // display off
    0xDC, 0x00, // display start line = 0 (SH1107 specific)
    0x81, 0x2F, // contrast = 0x2F (reasonable brightness)
    0x20,       // memory addressing mode: page
    0xA1,       // segment remap
    0xC8,       // COM output scan direction
    0xA8, 0x7F, // multiplex ratio = 128 (full height)
    0xD3, 0x00, // display offset = 0
    0xD5, 0x51, // clock divide ratio / oscillator frequency
    0xD9, 0x22, // pre-charge period
    0xDB, 0x35, // VCOMH deselect level
    0xDA, 0x12, // COM pins configuration
    0xA4,       // output follows RAM (not forced all-on)
    0xA6,       // normal display (non-inverted)
    0xAF,       // display on
};

// 5x7 font: one byte per column, 5 columns per character, 1-pixel gap.
// ASCII 0x20 (space) through 0x7F.
const uint8_t PROGMEM kFont[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // 0x20 space
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x14, 0x08, 0x3E, 0x08, 0x14, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x09, 0x01, // F
    0x3E, 0x41, 0x49, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x3F, 0x40, 0x38, 0x40, 0x3F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x07, 0x08, 0x70, 0x08, 0x07, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x7F, 0x41, 0x41, 0x00, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // backslash
    0x00, 0x41, 0x41, 0x7F, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x0C, 0x52, 0x52, 0x52, 0x3E, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x7F, 0x10, 0x28, 0x44, 0x00, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x04, 0x08, 0x10, 0x08, // ~
    0x00, 0x00, 0x00, 0x00, 0x00, // 0x7F
};

void writeCmd(uint8_t c)
{
    twiStart(OLED_I2C_ADDR);
    twiWrite(0x00); // control byte: command
    twiWrite(c);
    twiStop();
}

void sendInit()
{
    for (uint8_t i = 0; i < sizeof(kInit); ++i)
    {
        writeCmd(pgm_read_byte(&kInit[i]));
        delay(5); // Per-command delay for stability
    }
    delay(200); // Post-init stabilization
}

} // namespace

// cppcheck-suppress functionStatic
void Display::begin()
{
    DBG(DBG_DISP_INIT);
    twiInit();
    DBG(DBG_DISP_WIRE_OK);

    DBG(DBG_DISP_OFF);
    writeCmd(0xAE);
    delay(100);

    DBG(DBG_DISP_SEQ);
    sendInit();
    DBG(DBG_DISP_INIT_DONE);

    delay(500); // Extra stabilization time

    DBG(DBG_DISP_CLEAR);
    for (uint8_t page = 0; page < 16; ++page)
    {
        writeCmd((uint8_t)(0xB0 | page));
        writeCmd(0x00);
        writeCmd(0x12); // CRITICAL: column high nibble = 0x12

        twiStart(OLED_I2C_ADDR);
        twiWrite(0x40); // control byte: data stream
        for (uint8_t col = 0; col < 64; ++col)
            twiWrite(0x00);
        twiStop();
    }
    DBG(DBG_DISP_READY);
}

void Display::clear()
{
    entryCount_ = 0;
    cursorCol_ = 0;
    cursorRow_ = 0;
    markDirty();
}

void Display::beginBatch()
{
    ++batchDepth_;
}

void Display::endBatch()
{
    if (batchDepth_ > 0 && --batchDepth_ == 0)
        flush();
}

void Display::setCursor(uint8_t col, uint8_t row)
{
    cursorCol_ = col;
    cursorRow_ = row;
}

void Display::addTextEntry(uint8_t x, uint8_t line, const char* text, uint8_t scale)
{
    if (entryCount_ < kMaxEntries)
    {
        TextEntry& e = entries_[entryCount_++];
        e.x = x;
        e.line = line;
        e.scale = scale;
        strncpy(e.text, text, kTextLen - 1);
        e.text[kTextLen - 1] = '\0';
    }
}

void Display::print(const char* s)
{
    if (s == nullptr || entryCount_ >= kMaxEntries)
        return;

    // Convert column to pixel x
    uint8_t px = (uint8_t)(cursorCol_ * 6);
    addTextEntry(px, cursorRow_, s);

    // Advance cursor by length of string
    cursorCol_ += (uint8_t)strlen(s);

    markDirty();
}

void Display::print(const __FlashStringHelper* s)
{
    if (s == nullptr || entryCount_ >= kMaxEntries)
        return;

    // Copy from PROGMEM into local buffer first
    const char* p = reinterpret_cast<const char*>(s);
    char buf[kTextLen];
    uint8_t i = 0;
    char c;
    while (i < kTextLen - 1 && (c = pgm_read_byte(p++)) != '\0')
        buf[i++] = c;
    buf[i] = '\0';

    // Convert column to pixel x
    uint8_t px = (uint8_t)(cursorCol_ * 6);
    addTextEntry(px, cursorRow_, buf);

    // Advance cursor by length of string
    cursorCol_ += i;

    markDirty();
}

void Display::print(int32_t n)
{
    if (entryCount_ >= kMaxEntries)
        return;

    char buf[12];
    ltoa(n, buf, 10);
    print(buf);
}

void Display::printBig(uint8_t x_px, uint8_t y_px, const char* s)
{
    if (s == nullptr || entryCount_ >= kMaxEntries)
        return;
    addTextEntry(x_px, y_px, s, 2);
    markDirty();
}

void Display::printBig(uint8_t x_px, uint8_t y_px, int32_t n)
{
    if (entryCount_ >= kMaxEntries)
        return;
    char buf[12];
    ltoa(n, buf, 10);
    printBig(x_px, y_px, buf);
}

void Display::drawBar(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (entryCount_ >= kMaxEntries)
        return;
    TextEntry& e = entries_[entryCount_++];
    e.x = x;
    e.line = y;
    e.scale = 3;
    e.text[0] = (char)w;
    e.text[1] = (char)h;
    markDirty();
}

void Display::drawBarClear(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (entryCount_ >= kMaxEntries)
        return;
    TextEntry& e = entries_[entryCount_++];
    e.x = x;
    e.line = y;
    e.scale = 4;
    e.text[0] = (char)w;
    e.text[1] = (char)h;
    markDirty();
}

// cppcheck-suppress functionStatic
void Display::drawChar2xToPage(uint8_t x, uint8_t y, char c, uint8_t page, uint8_t* pageBuf)
{
    if (x >= 64 || y >= 128)
        return;

    uint8_t glyph = ((uint8_t)c < 0x20 || (uint8_t)c > 0x7F) ? 0 : (uint8_t)(c - 0x20);
    const uint8_t* g = &kFont[(uint16_t)glyph * 5];

    // 5 columns × 2 = 10 pixel-wide glyph (+ 2px gap = 12px per char)
    for (uint8_t col = 0; col < 5; ++col)
    {
        uint8_t bits = pgm_read_byte(g + col);
        uint8_t sx = x + (uint8_t)(col * 2);

        for (uint8_t row = 0; row < 7; ++row)
        {
            if (!(bits & (1u << row)))
                continue;

            // Draw a 2×2 block for this set pixel
            for (uint8_t dy = 0; dy < 2; ++dy)
            {
                uint8_t py = y + (uint8_t)(row * 2) + dy;
                if ((py >> 3) != page)
                    continue;
                uint8_t bit = py & 7u;
                for (uint8_t dx = 0; dx < 2; ++dx)
                {
                    uint8_t px = sx + dx;
                    if (px < 64)
                        pageBuf[px] |= (uint8_t)(1u << bit);
                }
            }
        }
    }
}

// cppcheck-suppress functionStatic
void Display::drawCharToPage(uint8_t x, uint8_t y, char c, uint8_t page, uint8_t* pageBuf)
{
    if (x >= 64 || y >= 128)
        return;

    uint8_t glyph = ((uint8_t)c < 0x20 || (uint8_t)c > 0x7F) ? 0 : (uint8_t)(c - 0x20);
    const uint8_t* g = &kFont[(uint16_t)glyph * 5];

    // Draw 5 columns + 1 gap = 6 pixels wide per character
    for (uint8_t cx = 0; cx < 6; ++cx)
    {
        uint8_t bits = (cx < 5) ? pgm_read_byte(g + cx) : 0x00;

        for (uint8_t by = 0; by < 8; ++by)
        {
            bool pixel = (by < 7) && (bits & (1u << by));
            uint8_t py = y + by;

            // Check if this pixel is in the current page
            if ((py >> 3) == page)
            {
                uint8_t bit = py & 7;
                uint8_t idx = x + cx;

                if (pixel)
                    pageBuf[idx] |= (1u << bit);
                else
                    pageBuf[idx] &= ~(1u << bit);
            }
        }
    }
}

void Display::flush()
{
    if (!dirty_)
        return;

    // Render page-by-page
    for (uint8_t page = 0; page < 16; ++page)
    {
        uint8_t pageBuf[64] = {0};

        // Render all entries into this page
        for (uint8_t t = 0; t < entryCount_; ++t)
        {
            const TextEntry& e = entries_[t];
            uint8_t x = e.x;
            const char* s = e.text;

            if (e.scale == 2)
            {
                uint8_t y = e.line; // raw pixel y for big text
                while (*s && x < 64)
                {
                    drawChar2xToPage(x, y, *s++, page, pageBuf);
                    x += 12; // 10px glyph + 2px gap
                }
            }
            else if (e.scale == 3 || e.scale == 4)
            {
                // scale=3: fill rect (set pixels); scale=4: clear rect (clear pixels).
                // e.line = top y, e.text[0] = width, e.text[1] = height.
                uint8_t by = e.line;
                uint8_t bw = (uint8_t)e.text[0];
                uint8_t bh = (uint8_t)e.text[1];
                uint8_t pageStart = (uint8_t)(page << 3);
                uint16_t byEnd = (uint16_t)by + bh;
                if (by >= pageStart + 8u || byEnd <= pageStart)
                    continue;
                uint8_t yLo = by > pageStart ? by : pageStart;
                uint8_t yHi = (byEnd < pageStart + 8u) ? (uint8_t)byEnd : (uint8_t)(pageStart + 8u);
                uint8_t mask = 0;
                for (uint8_t iy = yLo; iy < yHi; ++iy)
                    mask |= (uint8_t)(1u << (iy & 7u));
                uint8_t xEnd = x + bw;
                if (xEnd > 64u)
                    xEnd = 64u;
                if (e.scale == 3)
                    for (uint8_t ix = x; ix < xEnd; ++ix)
                        pageBuf[ix] |= mask;
                else
                    for (uint8_t ix = x; ix < xEnd; ++ix)
                        pageBuf[ix] &= (uint8_t)~mask;
            }
            else
            {
                uint8_t y = (uint8_t)(e.line * 8); // row to pixel y
                while (*s && x < 64)
                {
                    drawCharToPage(x, y, *s++, page, pageBuf);
                    x += 6;
                }
            }
        }

        // Send this page over I2C
        writeCmd((uint8_t)(0xB0 | page)); // page address
        writeCmd(0x00);                   // column low = 0
        writeCmd(0x12);                   // column high = 0x12 ← CRITICAL!

        uint8_t col = 0;
        while (col < 64)
        {
            twiStart(OLED_I2C_ADDR);
            twiWrite(0x40); // data stream control byte
            uint8_t n = 0;
            while (col < 64 && n < 16)
            {
                twiWrite(pageBuf[col]);
                ++col;
                ++n;
            }
            twiStop();
        }
    }

    dirty_ = false;
}
