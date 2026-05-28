// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"
#include "../../Model/DTCStore.h"

namespace obd
{
namespace Display
{

// DTC menu: single screen with up/down cursor selecting Read / Clear / Show.
// Show sub-view: 4 DTCs per page with page navigation.
void renderDtcScreen(DisplayManager& dm, uint8_t cursor, bool showActive, uint8_t showPage,
                     int8_t dtcCount, const Model::DTCStore& dtcStore);

} // namespace Display
} // namespace obd
