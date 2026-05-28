// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsScreen.h"

namespace obd
{
namespace Display
{

// Settings layout (10 cols × 16 rows):
//   Row 0:  Settings
//   Row 2:  [>] Exit
//   Row 4:  [ ] KWP: Sensor   (shows current mode)

void renderSettingsScreen(DisplayManager& dm, uint8_t cursor, int kwpModeInt)
{
    dm.print(0, 0, F("Settings"));

    dm.print(0, 2, cursor == 0 ? F(">Exit") : F(" Exit"));

    // KWP mode row — show the current mode name inline
    dm.print(0, 4, cursor == 1 ? F(">KWP:") : F(" KWP:"));
    switch (kwpModeInt)
    {
        case 0:
            dm.print(5, 4, F("ACK"));
            break;
        case 2:
            dm.print(5, 4, F("Grp"));
            break;
        default:
            dm.print(5, 4, F("Sens"));
            break;
    }
}

} // namespace Display
} // namespace obd
