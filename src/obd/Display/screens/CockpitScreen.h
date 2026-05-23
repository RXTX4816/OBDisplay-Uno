#pragma once

#include "../DisplayManager.h"
#include "../../Model/OBDSignals.h"

namespace obd
{
namespace Display
{

void initCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected);
void renderCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected,
                         const Model::OBDSignals& signals, bool forceUpdate);

} // namespace Display
} // namespace obd
