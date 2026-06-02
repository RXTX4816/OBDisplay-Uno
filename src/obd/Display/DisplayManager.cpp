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

void DisplayManager::print(uint8_t x, uint8_t y, const __FlashStringHelper* s) const
{
    display_.setCursor(x, y);
    display_.print(s);
}

void DisplayManager::print(uint8_t x, uint8_t y, const char* s) const
{
    display_.setCursor(x, y);
    display_.print(s);
}

void DisplayManager::print(uint8_t x, uint8_t y, int32_t value) const
{
    display_.setCursor(x, y);
    display_.print(value);
}

void DisplayManager::print(uint8_t x, uint8_t y, const char* s, uint8_t width) const
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

// Format a ×10 fixed-point integer with one decimal place (e.g. 123 → "12.3").
// No float arithmetic — avoids the entire soft-float library on AVR.
static void fmtScaled(int32_t value, char* out)
{
    char* p = out;
    if (value < 0)
    {
        *p++ = '-';
        value = -value;
    }
    uint32_t u = (uint32_t)value;
    utoa((uint16_t)(u / 10), p, 10);
    while (*p != '\0')
        ++p;
    *p++ = '.';
    *p++ = (char)('0' + (u % 10));
    *p = '\0';
}

void DisplayManager::print(uint8_t x, uint8_t y, int32_t value, uint8_t /*decimals*/,
                           uint8_t width) const
{
    char tmp[12];
    fmtScaled(value, tmp);
    if (width == 0)
    {
        display_.setCursor(x, y);
        display_.print(tmp);
        return;
    }
    print(x, y, tmp, width);
}

void DisplayManager::printBig(uint8_t x_px, uint8_t y_px, const char* s) const
{
    display_.printBig(x_px, y_px, s);
}

void DisplayManager::printBig(uint8_t x_px, uint8_t y_px, uint16_t val) const
{
    char buf[12];
    utoa(val, buf, 10);
    display_.printBig(x_px, y_px, buf);
}

void DisplayManager::printBig(uint8_t x_px, uint8_t y_px, int16_t val, char suffix) const
{
    char buf[12];
    uint8_t i = 0;
    itoa(val, buf, 10);
    while (buf[i] != '\0')
        ++i;
    buf[i++] = suffix;
    buf[i] = '\0';
    display_.printBig(x_px, y_px, buf);
}

void DisplayManager::printBigWithLabel(uint8_t x_px, uint8_t y_px, uint16_t val,
                                       const char* label) const
{
    char buf[12];
    uint8_t i = 0;
    utoa(val, buf, 10);
    while (buf[i] != '\0')
        ++i;
    while (*label && i < 11)
        buf[i++] = *label++;
    buf[i] = '\0';
    display_.printBig(x_px, y_px, buf);
}

void DisplayManager::printBigScaled10(uint8_t x_px, uint8_t y_px, int16_t val, char suffix) const
{
    // Guard: anything outside ±999 can't sensibly display in 5 chars.
    if (val > 9990 || val < -9990)
    {
        display_.printBig(x_px, y_px, "ERR");
        return;
    }
    char buf[12];
    char* p = buf;
    // Use int32_t to safely negate without UB at INT16_MIN.
    int32_t v = val;
    if (v < 0)
    {
        *p++ = '-';
        v = -v;
    }
    utoa((uint16_t)((uint32_t)v / 10u), p, 10);
    while (*p != '\0')
        ++p;
    *p++ = '.';
    *p++ = (char)('0' + (uint8_t)((uint32_t)v % 10u));
    *p++ = suffix;
    *p = '\0';
    display_.printBig(x_px, y_px, buf);
}

void DisplayManager::printBigVoltage(uint8_t x_px, uint8_t y_px, uint16_t val) const
{
    // val is ×10 (e.g. 123 = 12.3 V) — show integer part + 'V'
    char buf[12];
    uint8_t i = 0;
    utoa((uint16_t)(val / 10u), buf, 10);
    while (buf[i] != '\0')
        ++i;
    buf[i++] = 'V';
    buf[i] = '\0';
    display_.printBig(x_px, y_px, buf);
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
            initMenuCockpit(menuState.screen(MenuId::Cockpit), addrSelected);
            break;
        case MenuId::Experimental:
            initMenuExperimental();
            break;
        case MenuId::Debug:
            initMenuDebug();
            break;
        case MenuId::Dtc:
            initMenuDtc(menuState.screen(MenuId::Dtc));
            break;
        case MenuId::Settings:
            initMenuSettings(menuState.screen(MenuId::Settings));
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
            displayMenuCockpit(menuState.screen(MenuId::Cockpit), addrSelected, signals,
                               forceUpdate);
            break;
        case MenuId::Experimental:
            displayMenuExperimental(menuState.screen(MenuId::Experimental), signals, forceUpdate);
            break;
        case MenuId::Debug:
            displayMenuDebug(menuState.screen(MenuId::Debug), signals, kwpModeInt, forceUpdate);
            break;
        case MenuId::Dtc:
            displayMenuDtc(menuState.screen(MenuId::Dtc), dtcStore, forceUpdate);
            break;
        case MenuId::Settings:
            displayMenuSettings(menuState.screen(MenuId::Settings), kwpModeInt, forceUpdate);
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

void DisplayManager::renderSettings(uint8_t cursor, int kwpModeInt, bool autoReconnect,
                                    const char (*ecuLines)[11], uint8_t ecuLineCount)
{
    renderSettingsScreen(*this, cursor, kwpModeInt, autoReconnect, ecuLines, ecuLineCount);
}

void DisplayManager::renderDebug(const DebugInfo& di, int kwpModeInt)
{
    renderDebugScreen(*this, di, kwpModeInt);
}

} // namespace Display
} // namespace obd
