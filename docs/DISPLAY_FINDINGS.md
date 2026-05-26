# SH1107 64x128 OLED Display — Key Findings

## Hardware
- **Panel**: 64 columns × 128 rows (1.3-inch OLED)
- **Controller**: SH1107 (sold as SSD1312 by some vendors — functionally equivalent for our purposes)
- **Interface**: I2C at address 0x3C
- **Native Orientation**: Portrait (64 wide × 128 tall)

## Critical Discovery: Column Addressing

**The display requires column HIGH nibble = 0x12, not 0x10!**

```c
writeCmd((uint8_t)(0xB0 | page));  // Page address (0xB0-0xBF for pages 0-15)
writeCmd(0x00);                     // Column low nibble = 0x00
writeCmd(0x12);                     // Column high nibble = 0x12 ← CRITICAL!
```

Without 0x12, the right half of the display (columns 32-63) becomes inaccessible or shows garbage.

## Initialization Sequence

Proven working init (from U8g2 SH1107 driver, adapted):

```c
const uint8_t PROGMEM kInit[] = {
    0xAE,       // display off
    0xDC, 0x00, // display start line = 0 (SH1107 command)
    0x81, 0x2F, // contrast
    0x20,       // memory addressing mode: page
    0xA0,       // segment remap (flip to 0xA1 if text is left-right mirrored)
    0xC0,       // COM scan dir (flip to 0xC8 if text is upside down)
    0xA8, 0x7F, // multiplex ratio = 128
    0xD3, 0x00, // display offset = 0
    0xD5, 0x51, // clock divide / osc frequency
    0xD9, 0x22, // pre-charge period
    0xDB, 0x35, // VCOMH deselect level
    0xDA, 0x12, // COM pins config
    0xA4,       // output follows RAM
    0xA6,       // normal (non-inverted)
    0xAF,       // display on
};
```

**Important**: Send each command in its own I2C transaction with delays:
```c
for (uint8_t i = 0; i < sizeof(kInit); ++i)
{
    writeCmd(pgm_read_byte(&kInit[i]));
    delay(5);  // Per-command delay for stability
}
delay(200);  // Wait for display to stabilize
```

## I2C Communication

- **Speed**: 100 kHz (slower is more stable than 400 kHz)
- **Command transactions**: Single command per beginTransmission/endTransmission
- **Data transactions**: Can batch multiple data bytes (0x40 control byte + data)

```c
void writeCmd(uint8_t c)
{
    Wire.beginTransmission(0x3C);
    Wire.write(0x00);  // control byte: command
    Wire.write(c);
    Wire.endTransmission();
}

void sendData(const uint8_t* data, uint8_t len)
{
    Wire.beginTransmission(0x3C);
    Wire.write(0x40);  // control byte: data stream
    for (uint8_t i = 0; i < len; ++i)
        Wire.write(data[i]);
    Wire.endTransmission();
}
```

## Buffer Layout (Portrait Mode)

- **Size**: 1024 bytes (64 columns × 16 pages)
- **Organization**: Column-major, `buf[col + page*64]`
- **Addressing**: 
  - Pages 0-15 correspond to rows 0-127 (8 rows per page)
  - Columns 0-63 correspond to columns 0-63

## Display Flushing

For each page (0-15):
```c
writeCmd(0xB0 | page);  // Set page address
writeCmd(0x00);         // Column low = 0x00
writeCmd(0x12);         // Column high = 0x12 ← THE KEY!
// Then send 64 bytes of data (one byte per column)
```

## What Doesn't Work (Yet)

- **Landscape mode (90° rotation)**: Requires per-pixel software rotation in the drawing code. Not yet implemented.
- **Hardware rotation commands**: 0xA0/0xA1 and 0xC0/0xC8 only flip/mirror, they don't rotate 90°.

## Memory Constraints

- **Arduino Uno SRAM**: 2KB total
- **Framebuffer**: 1KB (64×16 pages)
- **Remaining for app**: ~287 bytes at 86% usage
- **Impact**: Large OBD applications cause stack overflow during initialization. Display-only test sketches work fine.

## Pixel Mapping (Portrait Mode) — WORKING ✅

```c
void setPixel(uint8_t x, uint8_t y, bool on)
{
    if (x >= 64 || y >= 128)
        return;
    uint8_t page = y / 8;           // Page 0-15 (each page = 8 rows)
    uint8_t bit = y % 8;            // Bit position within page
    uint16_t idx = x + page * 64;   // Buffer index (MUST be uint16_t!)
    if (on)
        buf[idx] |= (1u << bit);
    else
        buf[idx] &= ~(1u << bit);
}
```

**Critical**: `idx` **must be `uint16_t`, NOT `uint8_t`**. Using `uint8_t` causes integer overflow at page 4, wrapping pages 4-15 back to pages 0-3.

## Text Rendering

- **Font**: 5x7 glyphs, 6 pixels wide (with 1-pixel gap)
- **Grid**: 10 columns × 16 rows (64÷6 ≈ 10, 128÷8 = 16)
- **Recommended y-offsets**: Start at y≥64 for first line, then y+=15 for each new line

## Text-Only Rendering (No Framebuffer) — ✅ IMPLEMENTED

For memory-constrained systems, removed the 1024-byte global framebuffer. Instead:
- Store text entries in a small array (position, string, scale)
- Render on-the-fly during display flush (page-by-page)
- Only 64-byte temporary buffer per page needed

**RAM Savings**: 72.6% → 27.5% (freed ~920 bytes)

```c
struct TextEntry {
    uint8_t x, line;
    const char* text;
    uint8_t scale;
};

// Usage: addText(2, 0, "Hello", 1);  // x=2, line=0, scale=1
// During flush: render text directly to I2C, no global buffer
```

## Mixed Font Scales — ✅ WORKING

- Scale 1: 5×7 glyphs, 6px wide, 8px tall
- Scale 2: 10×14 glyphs, 12px wide, 16px tall
- **Important**: Line numbers must account for scale
  - Scale 1: y = line × 8
  - Scale 2: y = line × 16
  - Mixing scales requires careful layout to avoid overlap

## Compile-Time Text Validation — ✅ IMPLEMENTED

Macro catches text overflows at compile time, not runtime:

```c
#define ADD_TEXT_SAFE(x, line, s, scale) \
    do { \
        static_assert((sizeof(s) - 1) * (6 * (scale)) + (x) <= 64, \
                      "Text too long for display width"); \
        addText((x), (line), (s), (scale)); \
    } while(0)

// Use: ADD_TEXT_SAFE(2, 0, "Text", 1);
// Compiler error if text exceeds 64px width
```

## Status: ✅ FULLY WORKING

**Portrait Mode: COMPLETE**
- ✅ Full 64×128 display accessible
- ✅ Text rendering across entire screen (no framebuffer)
- ✅ Column addressing: 0x12 for column high nibble
- ✅ I2C stable at 100kHz with per-command delays
- ✅ All 16 pages (0-15) addressable
- ✅ Mixed font scales (1x and 2x)
- ✅ Compile-time text length validation
- ✅ RAM usage: 27.5% (563 bytes, down from 72.6%)

**Known Issues Fixed**
- ❌ ~~Integer overflow on idx (was uint8_t, now uint16_t)~~ ✅ FIXED
- ❌ ~~Pages 4-15 inaccessible~~ ✅ FIXED
- ❌ ~~1024-byte framebuffer eating RAM~~ ✅ FIXED (text-only rendering)
- ❌ ~~Text overlapping due to mixed scales~~ ✅ FIXED (proper line layout)

**Next Steps**
- Integrate text-only display driver into full OBDDisplay app
- Test KWP/KLine serial communication priority
- Monitor actual SRAM usage under full load
