// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"

namespace obd
{
namespace Display
{

// Settings: single screen, up/down cursor selects Exit (0) or KWP Mode (1).
void renderSettingsScreen(DisplayManager& dm, uint8_t cursor, int kwpModeInt);

} // namespace Display
} // namespace obd
