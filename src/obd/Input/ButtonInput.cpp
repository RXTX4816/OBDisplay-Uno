// SPDX-License-Identifier: GPL-3.0-or-later
#include "ButtonInput.h"

namespace obd
{
namespace Input
{

ButtonInput::ButtonInput(uint8_t pinUp, uint8_t pinDown, uint8_t pinLeft, uint8_t pinRight,
                         uint8_t pinMid)
    : pinUp_(pinUp), pinDown_(pinDown), pinLeft_(pinLeft), pinRight_(pinRight), pinMid_(pinMid)
{
    pinMode(pinUp_, INPUT_PULLUP);
    pinMode(pinDown_, INPUT_PULLUP);
    pinMode(pinLeft_, INPUT_PULLUP);
    pinMode(pinRight_, INPUT_PULLUP);
    pinMode(pinMid_, INPUT_PULLUP);
}

bool ButtonInput::isSelectPressed() const
{
    return readMid();
}

bool ButtonInput::update(MenuState& menuState, const InputActions& actions)
{
    bool any = false;

    if (readRight())
    {
        menuState.nextMenu();
        any = true;
    }
    else if (readLeft())
    {
        menuState.prevMenu();
        any = true;
    }
    else
    {
        using Display::MenuId;
        switch (menuState.currentMenu())
        {
            case MenuId::Cockpit:
            case MenuId::Experimental:
            case MenuId::Debug:
            {
                MenuId mid = menuState.currentMenu();
                if (readUp())
                {
                    menuState.nextScreen(mid);
                    any = true;
                }
                else if (readDown())
                {
                    menuState.prevScreen(mid);
                    any = true;
                }
                break;
            }
            case MenuId::Dtc:
            case MenuId::Settings:
                // DTC and Settings navigation is handled entirely by
                // OBDDisplay::handleInput_() using cursor-based state.
                break;
        }
    }

    return any;
}

} // namespace Input
} // namespace obd
