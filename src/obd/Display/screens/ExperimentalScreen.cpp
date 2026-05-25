#include "ExperimentalScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

// Landscape layout (21 cols x 8 rows):
//   Row 0: G:XX S:X
//   Row 1: VVVVVVV UUUUUU
//   Row 2: VVVVVVV UUUUUU

void initExperimentalScreen(DisplayManager& dm)
{
    dm.print(0, 0, F("G:"));
    dm.print(5, 0, F("S:"));
}

void renderExperimentalScreen(DisplayManager& dm, uint8_t /*screen*/,
                              const Model::OBDSignals& signals, bool forceUpdate)
{
    using namespace Model;
    ExperimentalGroup& eg = const_cast<ExperimentalGroup&>(signals.experimental);

    bool groupUpdated = true;
    printField(dm, 2, 0, eg.groupCurrent, 2, groupUpdated, forceUpdate);

    bool sideUpdated = eg.groupSideUpdated || forceUpdate;
    uint8_t sideVal = eg.groupSide ? 1 : 0;
    printField(dm, 7, 0, sideVal, 1, sideUpdated, true);
    eg.groupSideUpdated = false;

    uint8_t first = eg.groupSide ? 2 : 0;
    uint8_t second = eg.groupSide ? 3 : 1;

    printFieldFloat(dm, 0, 1, eg.v[first],  7, eg.vUpdated, true);
    printFieldFloat(dm, 0, 2, eg.v[second], 7, eg.vUpdated, true);
    eg.vUpdated = false;

    printFieldStr(dm, 8, 1, eg.unit[first],  6, eg.unitUpdated, true);
    printFieldStr(dm, 8, 2, eg.unit[second], 6, eg.unitUpdated, true);
    eg.unitUpdated = false;
}

} // namespace Display
} // namespace obd
