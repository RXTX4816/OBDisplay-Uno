#include "DisplayManager.h"
#include "screens/CockpitScreen.h"
#include "screens/ExperimentalScreen.h"
#include "screens/DebugScreen.h"
#include "screens/DTCScreen.h"
#include "screens/SettingsScreen.h"

namespace obd
{
namespace Display
{

DisplayManager::DisplayManager(::Display& display) : display_(display) {}

void DisplayManager::begin()
{
    display_.begin();
}

void DisplayManager::clear()
{
    display_.clear();
}

void DisplayManager::print(uint8_t x, uint8_t y, const __FlashStringHelper* s)
{
    display_.setCursor(x, y);
    display_.print(s);
}

void DisplayManager::print(uint8_t x, uint8_t y, const String& s)
{
    display_.setCursor(x, y);
    display_.print(s.c_str());
}

void DisplayManager::print(uint8_t x, uint8_t y, const String& s, uint8_t width)
{
    String tmp = s;
    while (tmp.length() < width)
        tmp += " ";
    print(x, y, tmp);
}

void DisplayManager::print(uint8_t x, uint8_t y, int32_t value)
{
    display_.setCursor(x, y);
    display_.print(value);
}

void DisplayManager::print(uint8_t x, uint8_t y, const char* s, uint8_t width)
{
    String tmp(s);
    while (tmp.length() < width)
        tmp += " ";
    print(x, y, tmp);
}

void DisplayManager::print(uint8_t x, uint8_t y, float value, uint8_t width)
{
    display_.setCursor(x, y);
    String s = String(value, 1);
    if (width > 0 && s.length() > width)
    {
        display_.print(s.c_str());
        return;
    }
    while (width > 0 && s.length() < width)
    {
        s += " ";
    }
    display_.print(s.c_str());
}

void DisplayManager::clearRegion(uint8_t x, uint8_t y, uint8_t width)
{
    display_.setCursor(x, y);
    for (uint8_t i = 0; i < width; ++i)
    {
        display_.print(" ");
    }
}

void DisplayManager::initMenu(const Input::MenuState& menuState, uint8_t addrSelected,
                              int kwpModeInt)
{
    (void)kwpModeInt;
    switch (menuState.currentMenu())
    {
        case MenuId::Cockpit:
            initMenuCockpit(menuState.cockpitScreen(), addrSelected);
            break;
        case MenuId::Experimental:
            initMenuExperimental();
            break;
        case MenuId::Debug:
            initMenuDebug();
            break;
        case MenuId::Dtc:
            initMenuDtc(menuState.dtcScreen());
            break;
        case MenuId::Settings:
            initMenuSettings(menuState.settingsScreen());
            break;
    }
}

void DisplayManager::render(const Input::MenuState& menuState, const Model::OBDSignals& signals,
                            const Model::DTCStore& dtcStore, uint8_t addrSelected, int kwpModeInt,
                            bool forceUpdate)
{
    switch (menuState.currentMenu())
    {
        case MenuId::Cockpit:
            displayMenuCockpit(menuState.cockpitScreen(), addrSelected, signals, forceUpdate);
            break;
        case MenuId::Experimental:
            displayMenuExperimental(menuState.experimentalScreen(), signals, forceUpdate);
            break;
        case MenuId::Debug:
            displayMenuDebug(menuState.debugScreen(), signals, kwpModeInt, forceUpdate);
            break;
        case MenuId::Dtc:
            displayMenuDtc(menuState.dtcScreen(), dtcStore, forceUpdate);
            break;
        case MenuId::Settings:
            displayMenuSettings(menuState.settingsScreen(), kwpModeInt, forceUpdate);
            break;
    }
}

// Screen implementations are in screens/XxxScreen.cpp for modularity.
// These private methods delegate to the free functions in those files.

void DisplayManager::initMenuCockpit(uint8_t screen, uint8_t addrSelected)
{
    initCockpitScreen(*this, screen, addrSelected);
}

void DisplayManager::initMenuExperimental()
{
    initExperimentalScreen(*this);
}

void DisplayManager::initMenuDebug()
{
    initDebugScreen(*this);
}

void DisplayManager::initMenuDtc(uint8_t screen)
{
    initDtcScreen(*this, screen);
}

void DisplayManager::initMenuSettings(uint8_t screen)
{
    initSettingsScreen(*this, screen);
}

void DisplayManager::displayMenuCockpit(uint8_t screen, uint8_t addrSelected,
                                        const Model::OBDSignals& signals, bool forceUpdate)
{
    renderCockpitScreen(*this, screen, addrSelected, signals, forceUpdate);
}

void DisplayManager::displayMenuExperimental(uint8_t screen, const Model::OBDSignals& signals,
                                             bool forceUpdate)
{
    renderExperimentalScreen(*this, screen, signals, forceUpdate);
}

void DisplayManager::displayMenuDebug(uint8_t screen, const Model::OBDSignals& signals,
                                      int kwpModeInt, bool forceUpdate)
{
    renderDebugScreen(*this, screen, signals, kwpModeInt, forceUpdate);
}

void DisplayManager::displayMenuDtc(uint8_t screen, const Model::DTCStore& dtcStore,
                                    bool forceUpdate)
{
    renderDtcScreen(*this, screen, dtcStore, forceUpdate);
}

void DisplayManager::displayMenuSettings(uint8_t screen, int kwpModeInt, bool forceUpdate)
{
    renderSettingsScreen(*this, screen, kwpModeInt, forceUpdate);
}

} // namespace Display
} // namespace obd
