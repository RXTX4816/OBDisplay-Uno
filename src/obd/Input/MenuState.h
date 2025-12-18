#pragma once

#include <Arduino.h>
#include "../Display/DisplayTypes.h"

namespace obd {
namespace Input {

class MenuState {
public:
    MenuState();

    Display::MenuId currentMenu() const { return currentMenu_; }
    uint8_t cockpitScreen() const { return cockpitScreen_; }
    uint8_t experimentalScreen() const { return experimentalScreen_; }
    uint8_t debugScreen() const { return debugScreen_; }
    uint8_t dtcScreen() const { return dtcScreen_; }
    uint8_t settingsScreen() const { return settingsScreen_; }

    void nextMenu();
    void prevMenu();

    void nextCockpitScreen();
    void prevCockpitScreen();
    void nextExperimentalScreen();
    void prevExperimentalScreen();
    void nextDebugScreen();
    void prevDebugScreen();
    void nextDtcScreen();
    void prevDtcScreen();
    void nextSettingsScreen();
    void prevSettingsScreen();

    bool consumeMenuChanged();
    bool consumeScreenChanged();

    void markMenuChanged();
    void markScreenChanged();

private:
    Display::MenuId currentMenu_;

    uint8_t cockpitScreen_;
    uint8_t cockpitScreenMax_;

    uint8_t experimentalScreen_;
    uint8_t experimentalScreenMax_;

    uint8_t debugScreen_;
    uint8_t debugScreenMax_;

    uint8_t dtcScreen_;
    uint8_t dtcScreenMax_;

    uint8_t settingsScreen_;
    uint8_t settingsScreenMax_;

    bool menuChanged_;
    bool screenChanged_;
};

} // namespace Input
} // namespace obd
