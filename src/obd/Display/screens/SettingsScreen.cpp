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
            dm.print(0, 1, F("[SELECT]"));
            break;
        case 1:
            dm.print(0, 0, F("KWP Mode:"));
            dm.print(0, 1, F("<"));
            dm.print(20, 1, F(">"));
            break;
        default:
            dm.print(0, 0, F("Screen"));
            dm.print(7, 0, String(screen));
            dm.print(0, 1, F("no data"));
            break;
    }
}

void renderSettingsScreen(DisplayManager& dm, uint8_t screen, int kwpModeInt, bool /*forceUpdate*/)
{
    if (screen != 1)
        return;

    dm.clearRegion(2, 1, 18);
    switch (kwpModeInt)
    {
        case 0:
            dm.print(2, 1, F("ACK"));
            break;
        case 2:
            dm.print(2, 1, F("GROUP"));
            break;
        default:
            dm.print(2, 1, F("SENSOR"));
            break;
    }
}

} // namespace Display
} // namespace obd
