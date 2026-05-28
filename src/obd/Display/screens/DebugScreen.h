// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"
#include "../../Model/OBDSignals.h"

namespace obd
{
namespace Display
{

void initDebugScreen(DisplayManager& dm);
void renderDebugScreen(DisplayManager& dm, uint8_t screen, const Model::OBDSignals& signals,
                       int kwpModeInt, bool forceUpdate);

} // namespace Display
} // namespace obd
