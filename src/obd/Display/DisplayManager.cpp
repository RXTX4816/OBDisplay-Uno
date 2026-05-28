// SPDX-License-Identifier: GPL-3.0-or-later
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

void DisplayManager::beginBatch()
{
    display_.beginBatch();
}

void DisplayManager::endBatch()
{
    display_.endBatch();
}

void DisplayManager::flush()
{
    display_.flush();
}

void DisplayManager::print(uint8_t x, uint8_t y, const __FlashStringHelper* s)
{
    display_.setCursor(x, y);
    display_.print(s);
}

void DisplayManager::print(uint8_t x, uint8_t y, const char* s)
{
    display_.setCursor(x, y);
    display_.print(s);
}

void DisplayManager::print(uint8_t x, uint8_t y, int32_t value)
{
    display_.setCursor(x, y);
    display_.print(value);
}

void DisplayManager::print(uint8_t x, uint8_t y, const char* s, uint8_t width)
{
    if (width > ::Display::COLS)
        width = ::Display::COLS;
    char buf[11]; // COLS (10) + null
    uint8_t i = 0;
    while (i < width && s[i] != '\0')
    {
        buf[i] = s[i];
        ++i;
    }
    while (i < width)
        buf[i++] = ' ';
    buf[i] = '\0';
    display_.setCursor(x, y);
    display_.print(buf);
}

// Format a float to one decimal place without pulling in dtostrf/dtoa
// (saves ~1KB flash). Values here are small (temps, voltages, fuel).
static void fmtFixed1(float value, char* out)
{
    bool neg = value < 0.0f;
    if (neg)
        value = -value;
    uint32_t scaled = (uint32_t)(value * 10.0f + 0.5f);
    char* p = out;
    if (neg)
        *p++ = '-';
    utoa((uint16_t)(scaled / 10), p, 10);
    while (*p != '\0')
        ++p;
    *p++ = '.';
    *p++ = (char)('0' + (scaled % 10));
    *p = '\0';
}

void DisplayManager::print(uint8_t x, uint8_t y, float value, uint8_t width)
{
    char tmp[12];
    fmtFixed1(value, tmp);
    if (width == 0)
    {
        display_.setCursor(x, y);
        display_.print(tmp);
        return;
    }
    print(x, y, tmp, width);
}

// cppcheck-suppress functionStatic
void DisplayManager::clearRegion(uint8_t x, uint8_t y, uint8_t width)
{
    // No-op in text-only rendering: entries are always cleared before render.
    (void)x;
    (void)y;
    (void)width;
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

void DisplayManager::initMenuDtc(uint8_t /*screen*/) {}      // handled by renderDtcMenu()
void DisplayManager::initMenuSettings(uint8_t /*screen*/) {} // handled by renderSettings()

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

// Debug, DTC, and Settings are now rendered via the public overrides below;
// the old private paths are kept as no-ops so render() compiles cleanly.
void DisplayManager::displayMenuDebug(uint8_t /*screen*/, const Model::OBDSignals& /*signals*/,
                                      int /*kwpModeInt*/, bool /*forceUpdate*/)
{
}

void DisplayManager::displayMenuDtc(uint8_t /*screen*/, const Model::DTCStore& /*dtcStore*/,
                                    bool /*forceUpdate*/)
{
}

void DisplayManager::displayMenuSettings(uint8_t /*screen*/, int /*kwpModeInt*/,
                                         bool /*forceUpdate*/)
{
}

// ── Public menu-specific render overrides ───────────────────────────────────

void DisplayManager::renderDtcMenu(uint8_t cursor, bool showActive, uint8_t showPage,
                                   int8_t dtcCount, const Model::DTCStore& dtcStore)
{
    renderDtcScreen(*this, cursor, showActive, showPage, dtcCount, dtcStore);
}

void DisplayManager::renderSettings(uint8_t cursor, int kwpModeInt)
{
    renderSettingsScreen(*this, cursor, kwpModeInt);
}

void DisplayManager::renderDebug(const DebugInfo& di, int kwpModeInt)
{
    renderDebugScreen(*this, di, kwpModeInt);
}

} // namespace Display
} // namespace obd
