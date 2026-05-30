// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsScreen.h"
#include "../ScreenVM.h"

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
                          bool autoReconnect, const char (*ecuLines)[11], uint8_t ecuLineCount)
{
    ScreenCtx ctx{nullptr, nullptr, cursor, (uint8_t)kwpModeInt};
    runScript(kSettingsScript, ctx, dm);
    dm.print(9, 6, autoReconnect ? F("Y") : F("N"));

    if (ecuLineCount > 0)
    {
        dm.print(0, 8, F("ECU ID:"));
        for (uint8_t i = 0; i < ecuLineCount && i < 6; ++i)
            dm.print(0, (uint8_t)(9 + i), ecuLines[i]);
    }
}

} // namespace Display
} // namespace obd
