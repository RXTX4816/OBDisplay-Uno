// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"

namespace obd
{
namespace Display
{

void initDebugScreen(DisplayManager& dm);
void renderDebugScreen(DisplayManager& dm, const DebugInfo& di, int kwpModeInt);

} // namespace Display
} // namespace obd
