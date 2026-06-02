// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"

namespace obd
{
namespace Display
{

// Settings: single screen. Cursor selects:
//   0=Exit  1=KWP Mode  2=AutoRcn  3=Fuel (0x17 only, addrSelected must be passed)
// ecuLines is an array of up to ecuLineCount null-terminated 10-char strings captured
// from the KWP connect blocks, displayed at the bottom of the screen.
// addrSelected / fuelL: used to render the Fuel EEPROM item when on 0x17.
void renderSettingsScreen(const DisplayManager& dm, uint8_t cursor, int kwpModeInt,
                          bool autoReconnect, const char (*ecuLines)[11], uint8_t ecuLineCount,
                          uint8_t addrSelected = 0, uint8_t fuelL = 0);

} // namespace Display
} // namespace obd
