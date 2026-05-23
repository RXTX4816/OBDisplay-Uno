#include "DTCScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

void initDtcScreen(DisplayManager& dm, uint8_t screen)
{
    switch (screen)
    {
        case 0:
            dm.print(0, 0, F("DTC menu addr "));
            dm.print(0, 1, F("<"));
            dm.print(5, 1, F("Read"));
            dm.print(15, 1, F(">"));
            break;
        case 1:
            dm.print(0, 0, F("DTC menu addr "));
            dm.print(0, 1, F("<"));
            dm.print(5, 1, F("Clear"));
            dm.print(15, 1, F(">"));
            break;
        default:
            dm.print(1, 0, F("/"));
            dm.print(10, 0, F("St:"));
            dm.print(0, 1, F("/8"));
            dm.print(10, 1, F("St:"));
            break;
    }
}

void renderDtcScreen(DisplayManager& dm, uint8_t screen, const Model::DTCStore& dtcStore,
                     bool forceUpdate)
{
    if (screen == 0 || screen == 1)
        return;

    uint8_t dtcPointer = screen - 2;
    if (dtcPointer > 7)
        return;

    bool upd = true;
    uint16_t e0 = dtcStore.errorAt(dtcPointer * 2);
    uint8_t s0 = dtcStore.statusAt(dtcPointer * 2);
    uint16_t e1 = dtcStore.errorAt(dtcPointer * 2 + 1);
    uint8_t s1 = dtcStore.statusAt(dtcPointer * 2 + 1);

    printField(dm, 0, 0, (uint8_t)(dtcPointer + 1), 1, upd, forceUpdate);
    upd = true;
    printFieldStr(dm, 3, 0, String(e0), 6, upd, forceUpdate);
    upd = true;
    printField(dm, 13, 0, (int32_t)s0, 3, upd, forceUpdate);
    upd = true;
    printFieldStr(dm, 3, 1, String(e1), 6, upd, forceUpdate);
    upd = true;
    printField(dm, 13, 1, (int32_t)s1, 3, upd, forceUpdate);
}

} // namespace Display
} // namespace obd
