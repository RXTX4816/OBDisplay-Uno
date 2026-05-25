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
            dm.print(0,  0, F("DTC menu:"));
            dm.print(0,  1, F("<"));
            dm.print(9,  1, F("Read"));
            dm.print(20, 1, F(">"));
            break;
        case 1:
            dm.print(0,  0, F("DTC menu:"));
            dm.print(0,  1, F("<"));
            dm.print(8,  1, F("Clear"));
            dm.print(20, 1, F(">"));
            break;
        default:
            // Row format: #XX E:XXXXX S:XXX
            dm.print(0,  0, F("#"));
            dm.print(3,  0, F("E:"));
            dm.print(11, 0, F("S:"));
            dm.print(0,  1, F("#"));
            dm.print(3,  1, F("E:"));
            dm.print(11, 1, F("S:"));
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

    // Row layout: #(1) index(2) E:(2) code(5) S:(2) status(3) = cols 0,1-2,3-4,5-9,11-12,13-15
    printField(dm, 1, 0, (uint8_t)(dtcPointer * 2 + 1), 2, upd, forceUpdate);
    upd = true;
    printFieldStr(dm, 5, 0, String(e0), 5, upd, forceUpdate);
    upd = true;
    printField(dm, 13, 0, (int32_t)s0, 3, upd, forceUpdate);
    upd = true;
    printField(dm, 1, 1, (uint8_t)(dtcPointer * 2 + 2), 2, upd, forceUpdate);
    upd = true;
    printFieldStr(dm, 5, 1, String(e1), 5, upd, forceUpdate);
    upd = true;
    printField(dm, 13, 1, (int32_t)s1, 3, upd, forceUpdate);
}

} // namespace Display
} // namespace obd
