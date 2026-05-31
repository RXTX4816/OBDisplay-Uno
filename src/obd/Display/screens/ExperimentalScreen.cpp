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
    SO_LABEL,  0,  3, 2, 'k',':',          SO_U8,     2,  3, FLD_EXP_K0,   SO_STR, 5, 3, FLD_EXP_U0,
    SO_LABEL,  0,  5, 3, 'V','2',':',      SO_SCALED, 3,  5, FLD_EXP_V1, 7,
    SO_LABEL,  0,  6, 2, 'k',':',          SO_U8,     2,  6, FLD_EXP_K1,   SO_STR, 5, 6, FLD_EXP_U1,
    SO_LABEL,  0,  8, 3, 'V','3',':',      SO_SCALED, 3,  8, FLD_EXP_V2, 7,
    SO_LABEL,  0,  9, 2, 'k',':',          SO_U8,     2,  9, FLD_EXP_K2,   SO_STR, 5, 9, FLD_EXP_U2,
    SO_LABEL,  0, 11, 3, 'V','4',':',      SO_SCALED, 3, 11, FLD_EXP_V3, 7,
    SO_LABEL,  0, 12, 2, 'k',':',          SO_U8,     2, 12, FLD_EXP_K3,   SO_STR, 5, 12, FLD_EXP_U3,
    SO_END
};
// clang-format on

void initExperimentalScreen(DisplayManager& /*dm*/) {}

void renderExperimentalScreen(const DisplayManager& dm, uint8_t /*screen*/,
                              const Model::OBDSignals& signals, bool forceUpdate)
{
#ifdef OBD_EXPERIMENTAL_SCREENS
    (void)forceUpdate;
    const auto& eg = signals.experimental;

    if (eg.grpJumpActive)
    {
        // Full-screen digit entry — replaces the normal group view while active.
        dm.print(0, 0, F("Jump:Grp  "));
        dm.print(0, 1, F("----------"));

        // Row 3: three digit slots, each 3 chars wide; active slot wrapped in [].
        // Layout: [slot0][slot1] [slot2]  (cols 0-2, 3-5, gap at 6, 7-9)
        const uint8_t c = eg.grpJumpCursor;
        const uint8_t* d = eg.grpJumpDigits;
        char buf[11];
        buf[0] = (c == 0) ? '[' : ' ';
        buf[1] = static_cast<char>('0' + d[0]);
        buf[2] = (c == 0) ? ']' : ' ';
        buf[3] = (c == 1) ? '[' : ' ';
        buf[4] = static_cast<char>('0' + d[1]);
        buf[5] = (c == 1) ? ']' : ' ';
        buf[6] = ' ';
        buf[7] = (c == 2) ? '[' : ' ';
        buf[8] = static_cast<char>('0' + d[2]);
        buf[9] = (c == 2) ? ']' : ' ';
        buf[10] = '\0';
        dm.print(0, 3, buf);
        dm.print(0, 4, F(" H  T   U ")); // labels aligned to digit cols 1, 4, 8

        // Row 6: target group preview.
        // Digit constraints guarantee value fits in uint8 (0-255); clamp 0 -> 1.
        uint16_t raw = (uint16_t)d[0] * 100u + (uint16_t)d[1] * 10u + d[2];
        uint8_t target = (raw == 0u) ? 1u : (uint8_t)raw;
        dm.print(0, 6, F("->Grp:"));
        dm.print(6, 6, (int32_t)target);
        if (raw == 0u)
            dm.print(7, 6, F("!"));

        dm.print(0, 9, F("----------"));
        dm.print(0, 10, F("U/D:digit "));
        dm.print(0, 11, F("L/R:move  "));
        dm.print(0, 12, F("SEL=ok    "));
    }
    else
    {
        // Normal group data view.
        ScreenCtx ctx{&signals, nullptr, 0, 0};
        runScript(kExpScript, ctx, dm);
        dm.print(0, 15, F("OK:Sel Grp"));
    }
#else
    (void)dm;
    (void)signals;
    (void)forceUpdate;
#endif
}

} // namespace Display
} // namespace obd
