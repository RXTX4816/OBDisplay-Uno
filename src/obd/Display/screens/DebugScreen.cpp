#include "DebugScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

// Landscape layout (21 cols x 8 rows):
//   Row 0: C:X  A:XXX  BC:XXX
//   Row 1: KWP:X  FPS:XXX

void initDebugScreen(DisplayManager& dm)
{
    dm.print(0,  0, F("C:"));
    dm.print(5,  0, F("A:"));
    dm.print(11, 0, F("BC:"));
    dm.print(0,  1, F("KWP:"));
    dm.print(7,  1, F("FPS:"));
}

void renderDebugScreen(DisplayManager& dm, uint8_t /*screen*/, const Model::OBDSignals& signals,
                       int kwpModeInt, bool /*forceUpdate*/)
{
    (void)signals;

    bool upd = true;
    printField(dm, 2,  0, (int32_t)0,             1, upd, true);
    upd = true;
    printField(dm, 7,  0, (int32_t)0,             3, upd, true);
    upd = true;
    printField(dm, 14, 0, (int32_t)0,             3, upd, true);
    upd = true;
    printField(dm, 4,  1, (int32_t)kwpModeInt,    1, upd, true);
    upd = true;
    printField(dm, 11, 1, (int32_t)(1000 / 177),  3, upd, true);
}

} // namespace Display
} // namespace obd
