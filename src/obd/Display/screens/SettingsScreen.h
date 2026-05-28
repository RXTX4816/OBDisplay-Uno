// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "../DisplayManager.h"

namespace obd
{
namespace Display
{

void initSettingsScreen(DisplayManager& dm, uint8_t screen);
void renderSettingsScreen(DisplayManager& dm, uint8_t screen, int kwpModeInt, bool forceUpdate);

} // namespace Display
} // namespace obd
