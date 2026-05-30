// SPDX-License-Identifier: GPL-3.0-or-later
#include "ExperimentalScreen.h"
#include "../ScreenVM.h"

namespace obd
{
namespace Display
{

// clang-format off
static const uint8_t PROGMEM kExpScript[] = {
    SO_LABEL,  0,  0, 4, 'G','r','p',':',  SO_U8,     5,  0, FLD_EXP_GRP,
    SO_LABEL,  0,  2, 3, 'V','1',':',      SO_SCALED, 3,  2, FLD_EXP_V0, 7,
    SO_LABEL,  0,  3, 3, 'U','1',':',      SO_STR,    3,  3, FLD_EXP_U0,
    SO_LABEL,  0,  5, 3, 'V','2',':',      SO_SCALED, 3,  5, FLD_EXP_V1, 7,
    SO_LABEL,  0,  6, 3, 'U','2',':',      SO_STR,    3,  6, FLD_EXP_U1,
    SO_LABEL,  0,  8, 3, 'V','3',':',      SO_SCALED, 3,  8, FLD_EXP_V2, 7,
    SO_LABEL,  0,  9, 3, 'U','3',':',      SO_STR,    3,  9, FLD_EXP_U2,
    SO_LABEL,  0, 11, 3, 'V','4',':',      SO_SCALED, 3, 11, FLD_EXP_V3, 7,
    SO_LABEL,  0, 12, 3, 'U','4',':',      SO_STR,    3, 12, FLD_EXP_U3,
    SO_END
};
// clang-format on

void initExperimentalScreen(DisplayManager& /*dm*/) {}

void renderExperimentalScreen(const DisplayManager& dm, uint8_t /*screen*/,
                              const Model::OBDSignals& signals, bool forceUpdate)
{
#ifdef OBD_EXPERIMENTAL_SCREENS
    (void)forceUpdate;
    ScreenCtx ctx{&signals, nullptr, 0, 0};
    runScript(kExpScript, ctx, dm);
#else
    (void)dm;
    (void)signals;
    (void)forceUpdate;
#endif
}

} // namespace Display
} // namespace obd
