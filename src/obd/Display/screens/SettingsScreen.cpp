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
    SO_END
};
// clang-format on

void renderSettingsScreen(const DisplayManager& dm, uint8_t cursor, int kwpModeInt)
{
    ScreenCtx ctx{nullptr, nullptr, cursor, (uint8_t)kwpModeInt};
    runScript(kSettingsScript, ctx, dm);
}

} // namespace Display
} // namespace obd
