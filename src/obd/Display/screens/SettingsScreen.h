// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"

namespace obd
{
namespace Display
{

// Settings: single screen, up/down cursor selects Exit (0), KWP Mode (1), AutoRcn (2).
void renderSettingsScreen(const DisplayManager& dm, uint8_t cursor, int kwpModeInt,
                          bool autoReconnect);

} // namespace Display
} // namespace obd
