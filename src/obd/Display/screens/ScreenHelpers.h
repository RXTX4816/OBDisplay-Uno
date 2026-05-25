#pragma once

// Shared formatting helpers used by all screen render functions.
// These operate on DisplayManager (for clearRegion/print) and take
// an updated flag + forceUpdate to skip unchanged fields.

#include <stdlib.h>
#include <string.h>
#include "../DisplayManager.h"

namespace obd
{
namespace Display
{

template <typename T>
static inline void printField(DisplayManager& dm, uint8_t x, uint8_t y, T value, uint8_t width,
                              bool& updated, bool forceUpdate)
{
    if (!(updated || forceUpdate))
        return;
    dm.clearRegion(x, y, width);
    char buf[12];
    ltoa((long)value, buf, 10);
    if (strlen(buf) <= width)
        dm.print(x, y, buf);
    updated = false;
}

static inline void printFieldFloat(DisplayManager& dm, uint8_t x, uint8_t y, float value,
                                   uint8_t width, bool& updated, bool forceUpdate)
{
    if (!(updated || forceUpdate))
        return;
    dm.clearRegion(x, y, width);
    dm.print(x, y, value, width);
    updated = false;
}

static inline void printFieldStr(DisplayManager& dm, uint8_t x, uint8_t y, const char* text,
                                 uint8_t width, bool& updated, bool forceUpdate)
{
    if (!(updated || forceUpdate))
        return;
    dm.clearRegion(x, y, width);
    dm.print(x, y, text);
    updated = false;
}

} // namespace Display
} // namespace obd
