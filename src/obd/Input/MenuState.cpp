// SPDX-License-Identifier: GPL-3.0-or-later
#include "MenuState.h"

namespace obd
{
namespace Input
{

MenuState::MenuState()
    : currentMenu_(Display::MenuId::Cockpit), menuChanged_(false), screenChanged_(false)
{
    screens_[0] = {0, 1};  // Cockpit: max 1
    screens_[1] = {0, 64}; // Experimental: max 64
    screens_[2] = {0, 4};  // Debug: max 4
    screens_[3] = {0, 0};  // Dtc: max 0
    screens_[4] = {0, 0};  // Settings: max 0
}

void MenuState::nextMenu()
{
    uint8_t val = static_cast<uint8_t>(currentMenu_);
    val = (val + 1) % 5;
    currentMenu_ = static_cast<Display::MenuId>(val);
    menuChanged_ = true;
}

void MenuState::prevMenu()
{
    uint8_t val = static_cast<uint8_t>(currentMenu_);
    if (val == 0)
        val = 4;
    else
        --val;
    currentMenu_ = static_cast<Display::MenuId>(val);
    menuChanged_ = true;
}

void MenuState::nextScreen(Display::MenuId id)
{
    NavCtx& c = screens_[static_cast<uint8_t>(id)];
    if (++c.current > c.max)
        c.current = 0;
    screenChanged_ = true;
}

void MenuState::prevScreen(Display::MenuId id)
{
    NavCtx& c = screens_[static_cast<uint8_t>(id)];
    if (c.current == 0)
        c.current = c.max;
    else
        --c.current;
    screenChanged_ = true;
}

bool MenuState::consumeMenuChanged()
{
    bool tmp = menuChanged_;
    menuChanged_ = false;
    return tmp;
}

bool MenuState::consumeScreenChanged()
{
    bool tmp = screenChanged_;
    screenChanged_ = false;
    return tmp;
}

void MenuState::markMenuChanged()
{
    menuChanged_ = true;
}

void MenuState::markScreenChanged()
{
    screenChanged_ = true;
}

} // namespace Input
} // namespace obd
