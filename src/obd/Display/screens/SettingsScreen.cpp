#include "SettingsScreen.h"

namespace obd
{
namespace Display
{

void initSettingsScreen(DisplayManager& dm, uint8_t screen)
{
    switch (screen)
    {
        case 0:
            dm.print(0, 0, F("Exit ECU:"));
            dm.print(0, 1, F("< Press select >"));
            break;
        case 1:
            dm.print(0, 0, F("KWP Mode:"));
            dm.print(0, 1, F("<"));
            dm.print(15, 1, F(">"));
            break;
        default:
            dm.print(0, 0, F("Screen"));
            dm.print(7, 0, String(screen));
            dm.print(0, 1, F("not supported!"));
            break;
    }
}

void renderSettingsScreen(DisplayManager& dm, uint8_t screen, int kwpModeInt, bool /*forceUpdate*/)
{
    if (screen != 1)
        return;

    dm.clearRegion(4, 1, 7);
    dm.print(4, 1, F("")); // position cursor for the switch below
    // Re-use display_ directly by going through the public print interface
    switch (kwpModeInt)
    {
        case 0:
            dm.print(4, 1, F("ACK"));
            break;
        case 2:
            dm.print(4, 1, F("GROUP"));
            break;
        default:
            dm.print(4, 1, F("SENSOR"));
            break;
    }
}

} // namespace Display
} // namespace obd
