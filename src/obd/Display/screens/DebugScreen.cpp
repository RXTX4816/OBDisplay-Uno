// SPDX-License-Identifier: GPL-3.0-or-later
#include "DebugScreen.h"
#include "../ScreenVM.h"

namespace obd
{
namespace Display
{

// clang-format off
static const uint8_t PROGMEM kDebugScript[] = {
    SO_LABEL,  0, 0, 4, 'C','o','n',':',   SO_U8,      4, 0, FLD_DBG_CON,
    SO_LABEL,  0, 1, 4, 'A','v','a',':',   SO_U8,      4, 1, FLD_DBG_AVA,
    SO_LABEL,  0, 2, 3, 'B','C', ':',      SO_U8,      3, 2, FLD_DBG_BC,
    SO_LABEL,  0, 3, 4, 'K','W','P',':',   SO_MODE_STR, 4, 3, FLD_KWP_MODE,
    SO_LABEL,  0, 4, 4, 'G','r','p',':',   SO_U8,      4, 4, FLD_DBG_GRP,
    SO_LABEL,  0, 5, 4, 'A','d','r',':',   SO_HEX_U8,  4, 5, FLD_DBG_ADDR,
    SO_LABEL,  0, 6, 5, 'B','a','u','d',':', SO_U16,   5, 6, FLD_DBG_BAUD,
    SO_LABEL,  0, 7, 4, 'A','t','t',':',   SO_U8,      4, 7, FLD_DBG_ATT,
    SO_LABEL,  0, 8, 4, 'R','A','M',':',   SO_U16,     4, 8, FLD_DBG_RAM,
    SO_END
};
// clang-format on

void initDebugScreen(DisplayManager& /*dm*/) {}

void renderDebugScreen(const DisplayManager& dm, const DebugInfo& di, int kwpModeInt)
{
#ifdef OBD_EXPERIMENTAL_SCREENS
    ScreenCtx ctx{nullptr, &di, 0, (uint8_t)kwpModeInt};
    runScript(kDebugScript, ctx, dm);
#else
    (void)dm;
    (void)di;
    (void)kwpModeInt;
#endif
}

} // namespace Display
} // namespace obd
