// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsScreen.h"

namespace obd
{
namespace Display
{

// Portrait layout (10 cols x 16 rows):
//
// Screen 0 (Exit ECU):
//   Row 0: Exit ECU:
//   Row 1: [SELECT]
//
// Screen 1 (KWP Mode):
//   Row 0: KWP Mode:
//   Row 1: < ACK >  or < GROUP > or < SENSOR >

void initSettingsScreen(DisplayManager& /*dm*/, uint8_t /*screen*/)
{
    // No-op: all rendering is done in renderSettingsScreen
}

void renderSettingsScreen(DisplayManager& dm, uint8_t screen, int kwpModeInt, bool /*forceUpdate*/)
{
    if (screen == 0)
    {
        // Exit ECU confirmation screen
        dm.print(0, 0, F("Exit ECU:"));
        dm.print(0, 1, F("[SELECT]"));
    }
    else if (screen == 1)
    {
        // KWP Mode selection screen
        dm.print(0, 0, F("KWP Mode:"));

        // Display current mode as centered text on row 1
        // kwpModeInt values: 0=ACK, 1=ReadGroup, 2=ReadSensors
        // Mode cycling: ACK -> GROUP -> SENSOR -> ACK
        switch (kwpModeInt)
        {
            case 0:
                dm.print(0, 1, F("< ACK >"));
                break;
            case 2:
                dm.print(0, 1, F("< GROUP >"));
                break;
            default: // case 1 or ReadSensors
                dm.print(0, 1, F("< SENSOR>"));
                break;
        }
    }
    else
    {
        // Unknown settings screen fallback
        char buf[8];
        dm.print(0, 0, F("Screen"));
        ltoa((long)screen, buf, 10);
        dm.print(7, 0, buf);
        dm.print(0, 1, F("no data"));
    }
}

} // namespace Display
} // namespace obd
