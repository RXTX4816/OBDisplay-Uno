#include "ExperimentalScreen.h"
#include "ScreenHelpers.h"

namespace obd
{
namespace Display
{

void initExperimentalScreen(DisplayManager& dm)
{
    dm.print(0, 0, F("G:"));
    dm.print(0, 1, F("S:"));
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
    printField(dm, 2, 1, sideVal, 2, sideUpdated, true);
    eg.groupSideUpdated = false;

    uint8_t first = eg.groupSide ? 2 : 0;
    uint8_t second = eg.groupSide ? 3 : 1;

    printFieldFloat(dm, 4, 0, eg.v[first], 7, eg.vUpdated, true);
    printFieldFloat(dm, 4, 1, eg.v[second], 7, eg.vUpdated, true);
    eg.vUpdated = false;

    printFieldStr(dm, 11, 0, eg.unit[first], 7, eg.unitUpdated, true);
    printFieldStr(dm, 11, 1, eg.unit[second], 7, eg.unitUpdated, true);
    eg.unitUpdated = false;
}

} // namespace Display
} // namespace obd
