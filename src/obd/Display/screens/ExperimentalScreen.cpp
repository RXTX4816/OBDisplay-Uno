// SPDX-License-Identifier: GPL-3.0-or-later
#include "ExperimentalScreen.h"

namespace obd
{
namespace Display
{

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
        dm.print(0, 0, F("Grp:"));
        dm.print(5, 0, (int32_t)eg.groupCurrent);

        static const uint8_t vRows[4] = {2, 5, 8, 11};
        static const uint8_t kRows[4] = {3, 6, 9, 12};

        for (uint8_t i = 0; i < 4; ++i)
        {
            const uint8_t vr = vRows[i];
            const uint8_t kr = kRows[i];

            if (eg.k[i] == 16)
            {
                // k=16 is "Binary Bits" — show lower byte of the value as 8-bit binary.
                // Layout: "V1" at col 0-1, 8 bits at col 2-9 (exactly 10 cols).
                char vlabel[3] = {'V', (char)('1' + i), '\0'};
                dm.print(0, vr, vlabel);
                uint8_t val = (uint8_t)(eg.v[i] / 10);
                char bin[9];
                for (uint8_t b = 0; b < 8; ++b)
                    bin[b] = (val & (0x80 >> b)) ? '1' : '0';
                bin[8] = '\0';
                dm.print(2, vr, bin);
            }
            else if (eg.k[i] == 36)
            {
                // k=36 km: always a multiple of 10 — no decimal needed.
                // fmtScaled always appends ".x" so bypass it: format as plain integer.
                char vlabel[4] = {'V', (char)('1' + i), ':', '\0'};
                dm.print(0, vr, vlabel);
                char buf[8];
                ltoa((long)(eg.v[i] / 10), buf, 10);
                dm.print(3, vr, buf, 7);
            }
            else
            {
                char vlabel[4] = {'V', (char)('1' + i), ':', '\0'};
                dm.print(0, vr, vlabel);
                dm.print(3, vr, eg.v[i], 1, 7);
            }

            // k row: always decimal type code + unit string.
            dm.print(0, kr, F("k:"));
            dm.print(2, kr, (int32_t)eg.k[i]);
            dm.print(5, kr, eg.unit[i]);
        }

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
