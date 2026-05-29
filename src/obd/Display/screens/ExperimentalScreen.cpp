// SPDX-License-Identifier: GPL-3.0-or-later
#include "ExperimentalScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

// Portrait layout (10 cols x 16 rows):
//   Row  0: Grp: XX
//   Row  1: (empty)
//   Row  2: V1: XXXXXXX
//   Row  3: U1: UUUUUU
//   Row  4: (empty)
//   Row  5: V2: XXXXXXX
//   Row  6: U2: UUUUUU
//   Row  7: (empty)
//   Row  8: V3: XXXXXXX
//   Row  9: U3: UUUUUU
//   Row 10: (empty)
//   Row 11: V4: XXXXXXX
//   Row 12: U4: UUUUUU

void initExperimentalScreen(DisplayManager& /*dm*/)
{
    // No-op: all rendering is done in renderExperimentalScreen
}

void renderExperimentalScreen(const DisplayManager& dm, uint8_t /*screen*/,
                              const Model::OBDSignals& signals, bool forceUpdate)
{
#ifdef OBD_EXPERIMENTAL_SCREENS
    const Model::ExperimentalGroup& eg = signals.experimental;

    char buf[11];

    // Row 0: Grp: XX
    dm.print(0, 0, F("Grp:"));
    ltoa((long)eg.groupCurrent, buf, 10);
    dm.print(5, 0, buf);

    // Slot 1 (rows 2-3) — v[n] is ×10 fixed-point
    dm.print(0, 2, F("V1:"));
    dm.print(3, 2, eg.v[0], 1, 7);
    dm.print(0, 3, F("U1:"));
    dm.print(3, 3, eg.unit[0]);

    // Slot 2 (rows 5-6)
    dm.print(0, 5, F("V2:"));
    dm.print(3, 5, eg.v[1], 1, 7);
    dm.print(0, 6, F("U2:"));
    dm.print(3, 6, eg.unit[1]);

    // Slot 3 (rows 8-9)
    dm.print(0, 8, F("V3:"));
    dm.print(3, 8, eg.v[2], 1, 7);
    dm.print(0, 9, F("U3:"));
    dm.print(3, 9, eg.unit[2]);

    // Slot 4 (rows 11-12)
    dm.print(0, 11, F("V4:"));
    dm.print(3, 11, eg.v[3], 1, 7);
    dm.print(0, 12, F("U4:"));
    dm.print(3, 12, eg.unit[3]);

    (void)forceUpdate;
#else
    (void)dm;
    (void)signals;
    (void)forceUpdate;
#endif
}

} // namespace Display
} // namespace obd
