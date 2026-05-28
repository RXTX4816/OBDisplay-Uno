// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"
#include "../../Model/DTCStore.h"

namespace obd
{
namespace Display
{

void initDtcScreen(DisplayManager& dm, uint8_t screen);
void renderDtcScreen(DisplayManager& dm, uint8_t screen, const Model::DTCStore& dtcStore,
                     bool forceUpdate);

} // namespace Display
} // namespace obd
