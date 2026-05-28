// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>

namespace obd
{
namespace Display
{

enum class MenuId : uint8_t
{
    Cockpit = 0,
    Experimental = 1,
    Debug = 2,
    Dtc = 3,
    Settings = 4
};

} // namespace Display
} // namespace obd
