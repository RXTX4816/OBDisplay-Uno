// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsScreen.h"
#include "../ScreenVM.h"
#include <stdlib.h> // ltoa

namespace obd
{
namespace Display
{

// clang-format off
static const uint8_t PROGMEM kSettingsScript[] = {
    SO_LABEL,    0, 0, 8, 'S','e','t','t','i','n','g','s',
    SO_CURSOR,   0, 2, 0, 4, 'E','x','i','t',
    SO_CURSOR,   0, 4, 1, 4, 'K','W','P',':',
    SO_MODE_STR, 5, 4, FLD_KWP_MODE,
    SO_CURSOR,   0, 6, 2, 8, 'A','u','t','o','R','c','n',':',
    SO_END
};
// clang-format on

void renderSettingsScreen(const DisplayManager& dm, uint8_t cursor, int kwpModeInt,
                          bool autoReconnect, const char (*ecuLines)[11], uint8_t ecuLineCount,
                          uint8_t addrSelected, uint8_t fuelL)
{
    ScreenCtx ctx{nullptr, nullptr, cursor, (uint8_t)kwpModeInt};
    runScript(kSettingsScript, ctx, dm);
    dm.print(9, 6, autoReconnect ? F("Y") : F("N"));

    // Fuel EEPROM item — only shown when connected to 0x17 (Instruments cluster).
    // fuelL = current 0x17 fuel sensor reading. MID saves it to EEPROM.
    if (addrSelected == 0x17)
    {
        dm.print(0, 8, cursor == 3 ? F(">") : F(" "));
        dm.print(1, 8, F("Fuel:"));
        char buf[5];
        ltoa(fuelL, buf, 10);
        dm.print(6, 8, buf);
        dm.print(9, 8, F("L"));
        dm.print(1, 9, F("MID=save"));
    }

    if (ecuLineCount > 0)
    {
        uint8_t startRow = (addrSelected == 0x17) ? 10u : 8u;
        dm.print(0, startRow, F("ECU ID:"));
        for (uint8_t i = 0; i < ecuLineCount && i < 4; ++i)
            dm.print(0, (uint8_t)(startRow + 1u + i), ecuLines[i]);
    }
}

} // namespace Display
} // namespace obd
