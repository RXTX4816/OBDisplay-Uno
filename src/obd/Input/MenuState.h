// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>
#include "../Display/DisplayTypes.h"

namespace obd
{
namespace Input
{

class MenuState
{
  public:
    MenuState();

    Display::MenuId currentMenu() const { return currentMenu_; }

    uint8_t screen(Display::MenuId id) const { return screens_[static_cast<uint8_t>(id)].current; }
    void setScreen(Display::MenuId id, uint8_t v)
    {
        screens_[static_cast<uint8_t>(id)].current = v;
    }

    void nextMenu();
    void prevMenu();
    void nextScreen(Display::MenuId id);
    void prevScreen(Display::MenuId id);

    bool consumeMenuChanged();
    bool consumeScreenChanged();

    void markMenuChanged();
    void markScreenChanged();

  private:
    struct NavCtx
    {
        uint8_t current;
        uint8_t max;
    };

    Display::MenuId currentMenu_;
    NavCtx screens_[5]; // indexed by static_cast<uint8_t>(MenuId)
    bool menuChanged_;
    bool screenChanged_;
};

} // namespace Input
} // namespace obd
