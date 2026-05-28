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

bool ButtonInput::update(MenuState& menuState, InputActions& actions)
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
                if (readUp())
                {
                    menuState.nextCockpitScreen();
                    any = true;
                }
                else if (readDown())
                {
                    menuState.prevCockpitScreen();
                    any = true;
                }
                break;
            case MenuId::Experimental:
                if (readUp())
                {
                    menuState.nextExperimentalScreen();
                    any = true;
                }
                else if (readDown())
                {
                    menuState.prevExperimentalScreen();
                    any = true;
                }
                else if (readMid())
                {
                    actions.invertGroupSide = true;
                    any = true;
                }
                break;
            case MenuId::Debug:
                if (readUp())
                {
                    menuState.nextDebugScreen();
                    any = true;
                }
                else if (readDown())
                {
                    menuState.prevDebugScreen();
                    any = true;
                }
                break;
            case MenuId::Dtc:
                if (readUp())
                {
                    menuState.nextDtcScreen();
                    any = true;
                }
                else if (readDown())
                {
                    menuState.prevDtcScreen();
                    any = true;
                }
                else if (readMid())
                {
                    if (menuState.dtcScreen() == 0)
                    {
                        actions.readDtc = true;
                        any = true;
                    }
                    else if (menuState.dtcScreen() == 1)
                    {
                        actions.clearDtc = true;
                        any = true;
                    }
                }
                break;
            case MenuId::Settings:
                if (readUp())
                {
                    menuState.nextSettingsScreen();
                    any = true;
                }
                else if (readDown())
                {
                    menuState.prevSettingsScreen();
                    any = true;
                }
                else if (readMid())
                {
                    if (menuState.settingsScreen() == 0)
                    {
                        actions.requestExit = true;
                        any = true;
                    }
                    else if (menuState.settingsScreen() == 1)
                    {
                        actions.toggleKwpMode = true;
                        any = true;
                    }
                }
                break;
        }
    }

    return any;
}

} // namespace Input
} // namespace obd
