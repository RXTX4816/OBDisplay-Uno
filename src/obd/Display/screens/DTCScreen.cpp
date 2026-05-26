#include "DTCScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

// Portrait layout (10 cols x 16 rows):
//
// Screen 0 (DTC menu - Read):
//   Row 0: DTC menu:
//   Row 1: < Read >
//
// Screen 1 (DTC menu - Clear):
//   Row 0: DTC menu:
//   Row 1: < Clear >
//
// Screen >=2 (DTC data):
//   Row 0: #XX       (DTC index)
//   Row 1: E:XXXXX   (error code)
//   Row 2: S:XXX     (status)
//   Row 3: ---       (separator)
//   Row 4: #XX       (DTC index)
//   Row 5: E:XXXXX   (error code)
//   Row 6: S:XXX     (status)

void initDtcScreen(DisplayManager& /*dm*/, uint8_t /*screen*/)
{
    // No-op: all rendering is done in renderDtcScreen
}

void renderDtcScreen(DisplayManager& dm, uint8_t screen, const Model::DTCStore& dtcStore,
                     bool forceUpdate)
{
    if (screen == 0)
    {
        // DTC menu: Read option
        dm.print(0, 0, F("DTC menu:"));
        dm.print(0, 1, F("< Read >"));
    }
    else if (screen == 1)
    {
        // DTC menu: Clear option
        dm.print(0, 0, F("DTC menu:"));
        dm.print(0, 1, F("< Clear >"));
    }
    else
    {
        // DTC data display (screen >= 2)
        char buf[11];
        uint8_t dtcPointer = screen - 2;
        if (dtcPointer > 7)
            return;

        uint16_t e0 = dtcStore.errorAt(dtcPointer * 2);
        uint8_t s0 = dtcStore.statusAt(dtcPointer * 2);
        uint16_t e1 = dtcStore.errorAt(dtcPointer * 2 + 1);
        uint8_t s1 = dtcStore.statusAt(dtcPointer * 2 + 1);

        // First DTC entry (rows 0-2)
        // Row 0: #XX
        buf[0] = '#';
        ltoa((long)(dtcPointer * 2 + 1), buf + 1, 10);
        dm.print(0, 0, buf);

        // Row 1: E:XXXXX
        buf[0] = 'E';
        buf[1] = ':';
        ltoa((long)e0, buf + 2, 10);
        dm.print(0, 1, buf);

        // Row 2: S:XXX
        buf[0] = 'S';
        buf[1] = ':';
        ltoa((long)s0, buf + 2, 10);
        dm.print(0, 2, buf);

        // Row 3: separator (blank line, optional)

        // Second DTC entry (rows 4-6)
        // Row 4: #XX
        buf[0] = '#';
        ltoa((long)(dtcPointer * 2 + 2), buf + 1, 10);
        dm.print(0, 4, buf);

        // Row 5: E:XXXXX
        buf[0] = 'E';
        buf[1] = ':';
        ltoa((long)e1, buf + 2, 10);
        dm.print(0, 5, buf);

        // Row 6: S:XXX
        buf[0] = 'S';
        buf[1] = ':';
        ltoa((long)s1, buf + 2, 10);
        dm.print(0, 6, buf);

        (void)forceUpdate; // Satisfy unused parameter warning
    }
}

} // namespace Display
} // namespace obd
