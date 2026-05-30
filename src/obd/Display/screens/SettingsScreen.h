// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"

namespace obd
{
namespace Display
{

// Settings: single screen, up/down cursor selects Exit (0), KWP Mode (1), AutoRcn (2).
// ecuLines is an array of up to ecuLineCount null-terminated 10-char strings captured
// from the KWP connect blocks, displayed at the bottom of the screen.
void renderSettingsScreen(const DisplayManager& dm, uint8_t cursor, int kwpModeInt,
                          bool autoReconnect, const char (*ecuLines)[11], uint8_t ecuLineCount);

} // namespace Display
} // namespace obd
