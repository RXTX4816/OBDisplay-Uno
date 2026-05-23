#include "DebugScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

void initDebugScreen(DisplayManager& dm)
{
    dm.print(0, 0, F("C:"));
    dm.print(4, 0, F("A:"));
    dm.print(9, 0, F("BC:"));
    dm.print(0, 1, F("KWP:"));
    dm.print(7, 1, F("FPS:"));
}

void renderDebugScreen(DisplayManager& dm, uint8_t /*screen*/, const Model::OBDSignals& signals,
                       int kwpModeInt, bool /*forceUpdate*/)
{
    (void)signals;

    bool upd = true;
    printField(dm, 2, 0, (int32_t)0, 1, upd, true);
    upd = true;
    printField(dm, 6, 0, (int32_t)0, 3, upd, true);
    upd = true;
    printField(dm, 13, 0, (int32_t)0, 3, upd, true);
    upd = true;
    printField(dm, 5, 1, (int32_t)kwpModeInt, 1, upd, true);
    upd = true;
    printField(dm, 12, 1, (int32_t)(1000 / 177), 3, upd, true);
}

} // namespace Display
} // namespace obd
