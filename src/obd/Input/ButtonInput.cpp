#include "ButtonInput.h"

namespace obd {
namespace Input {

ButtonInput::ButtonInput(uint8_t analogPin)
    : analogPin_(analogPin)
{
}

int ButtonInput::readRaw() const
{
    return analogRead(analogPin_);
}

bool ButtonInput::isSelectPressed() const
{
    int v = readRaw();
    return isSelect(v);
}

bool ButtonInput::update(MenuState &menuState, InputActions &actions)
{
    int v = readRaw();
    bool any = false;

    if (isRight(v)) {
        menuState.nextMenu();
        any = true;
    } else if (isLeft(v)) {
        menuState.prevMenu();
        any = true;
    } else {
        using Display::MenuId;
        switch (menuState.currentMenu()) {
        case MenuId::Cockpit:
            if (isUp(v)) { menuState.nextCockpitScreen(); any = true; }
            else if (isDown(v)) { menuState.prevCockpitScreen(); any = true; }
            break;
        case MenuId::Experimental:
            if (isUp(v)) {
                menuState.nextExperimentalScreen();
                any = true;
            }
            else if (isDown(v)) {
                menuState.prevExperimentalScreen();
                any = true;
            }
            else if (isSelect(v)) {
                actions.invertGroupSide = true;
                any = true;
            }
            break;
        case MenuId::Debug:
            if (isUp(v)) { menuState.nextDebugScreen(); any = true; }
            else if (isDown(v)) { menuState.prevDebugScreen(); any = true; }
            break;
        case MenuId::Dtc:
            if (isUp(v)) { menuState.nextDtcScreen(); any = true; }
            else if (isDown(v)) { menuState.prevDtcScreen(); any = true; }
            else if (isSelect(v)) {
                if (menuState.dtcScreen() == 0) {
                    actions.readDtc = true; any = true; }
                else if (menuState.dtcScreen() == 1) {
                    actions.clearDtc = true; any = true; }
            }
            break;
        case MenuId::Settings:
            if (isUp(v)) { menuState.nextSettingsScreen(); any = true; }
            else if (isDown(v)) { menuState.prevSettingsScreen(); any = true; }
            else if (isSelect(v)) {
                // Map settings actions to match old behaviour: screen 0 = Exit,
                // screen 1 = KWP mode cycling.
                if (menuState.settingsScreen() == 0) {
                    actions.requestExit = true;
                    any = true;
                } else if (menuState.settingsScreen() == 1) {
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
