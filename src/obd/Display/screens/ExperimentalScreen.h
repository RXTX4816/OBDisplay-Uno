#pragma once

#include "../DisplayManager.h"
#include "../../Model/OBDSignals.h"

namespace obd
{
namespace Display
{

void initExperimentalScreen(DisplayManager& dm);
void renderExperimentalScreen(DisplayManager& dm, uint8_t screen, const Model::OBDSignals& signals,
                              bool forceUpdate);

} // namespace Display
} // namespace obd
