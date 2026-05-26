#include "DebugScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

// Portrait layout (10 cols x 16 rows):
//   Row 0: C:X
//   Row 1: A:XXX
//   Row 2: BC:XXX
//   Row 3: KWP:X
//   Row 4: FPS:XXX

void initDebugScreen(DisplayManager& /*dm*/)
{
    // No-op: all rendering is done in renderDebugScreen
}

void renderDebugScreen(DisplayManager& dm, uint8_t /*screen*/, const Model::OBDSignals& /*signals*/,
                       int kwpModeInt, bool /*forceUpdate*/)
{
    char buf[11];

    // Row 0: C:0 (placeholder)
    dm.print(0, 0, F("C:0"));

    // Row 1: A:0 (placeholder)
    dm.print(0, 1, F("A:0"));

    // Row 2: BC:0 (placeholder)
    dm.print(0, 2, F("BC:0"));

    // Row 3: KWP:X (KWP mode)
    buf[0] = 'K';
    buf[1] = 'W';
    buf[2] = 'P';
    buf[3] = ':';
    ltoa((long)kwpModeInt, buf + 4, 10);
    dm.print(0, 3, buf);

    // Row 4: FPS:X (frames per second, approx 1000/177 = 5-6)
    buf[0] = 'F';
    buf[1] = 'P';
    buf[2] = 'S';
    buf[3] = ':';
    ltoa((long)(1000 / 177), buf + 4, 10);
    dm.print(0, 4, buf);
}

} // namespace Display
} // namespace obd
