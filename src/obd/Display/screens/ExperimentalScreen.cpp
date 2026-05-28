// SPDX-License-Identifier: GPL-3.0-or-later
#include "ExperimentalScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

// Portrait layout (10 cols x 16 rows):
//   Row 0: G:XX S:X    (combined group number and side)
//   Row 1: V1:XXXXXXX  (first channel value)
//   Row 2: U1:UUUUUU   (first channel unit)
//   Row 3: V2:XXXXXXX  (second channel value)
//   Row 4: U2:UUUUUU   (second channel unit)

void initExperimentalScreen(DisplayManager& /*dm*/)
{
    // No-op: all rendering is done in renderExperimentalScreen
}

void renderExperimentalScreen(DisplayManager& dm, uint8_t /*screen*/,
                              const Model::OBDSignals& signals, bool forceUpdate)
{
    using namespace Model;
    ExperimentalGroup& eg = const_cast<ExperimentalGroup&>(signals.experimental);

    char buf[11];

    // Row 0: G:XX S:X (group number + side)
    buf[0] = 'G';
    buf[1] = ':';
    ltoa((long)eg.groupCurrent, buf + 2, 10);
    buf[4] = ' ';
    buf[5] = 'S';
    buf[6] = ':';
    buf[7] = '0' + (eg.groupSide ? 1 : 0);
    buf[8] = '\0';
    dm.print(0, 0, buf);

    // Determine which two values to show (based on group side)
    uint8_t first = eg.groupSide ? 2 : 0;
    uint8_t second = eg.groupSide ? 3 : 1;

    // Row 1: V1:XXXXXXX (first value)
    dm.print(0, 1, F("V1:"));
    dm.print(3, 1, eg.v[first], 7);

    // Row 2: U1:UUUUUU (first unit)
    dm.print(0, 2, F("U1:"));
    dm.print(3, 2, eg.unit[first]);

    // Row 3: V2:XXXXXXX (second value)
    dm.print(0, 3, F("V2:"));
    dm.print(3, 3, eg.v[second], 7);

    // Row 4: U2:UUUUUU (second unit)
    dm.print(0, 4, F("U2:"));
    dm.print(3, 4, eg.unit[second]);

    (void)forceUpdate; // Satisfy unused parameter warning
}

} // namespace Display
} // namespace obd
