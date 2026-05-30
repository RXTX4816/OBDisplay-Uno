// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>
#include "../../display/Display.h"
#include "../Model/OBDSignals.h"
#include "../Model/DTCStore.h"
#include "../Input/MenuState.h"
#include "DisplayTypes.h"

namespace obd
{
namespace Display
{

// Low-level debug data populated by OBDDisplay and passed to renderDebugScreen.
struct DebugInfo
{
    uint8_t serialCon; // NewSoftwareSerial::isListening()
    uint8_t serialAva; // NewSoftwareSerial::available()
    uint8_t blockCtr;  // KWP block counter
    uint8_t attempts;  // connection attempts (capped at 255)
    uint8_t addr;      // ECU address selected
    uint16_t baud;     // baud rate selected
    uint8_t group;     // current KWP group
    bool sim;          // simulation mode active
    int16_t freeRam;   // estimated free RAM (bytes)
};

class DisplayManager
{
  public:
    explicit DisplayManager(::Display& display);

    void begin();
    void clear();

    // Batch a full frame into a single I2C transfer (see Display::beginBatch).
    void beginBatch();
    void endBatch();
    void flush();

    void initMenu(const Input::MenuState& menuState, uint8_t addrSelected, int kwpModeInt);

    void render(const Input::MenuState& menuState, const Model::OBDSignals& signals,
                const Model::DTCStore& dtcStore, uint8_t addrSelected, int kwpModeInt,
                bool forceUpdate);

    // Menu-specific render overrides that bypass the generic render() path.
    void renderDtcMenu(uint8_t cursor, bool showActive, uint8_t showPage, int8_t dtcCount,
                       const Model::DTCStore& dtcStore);
    void renderSettings(uint8_t cursor, int kwpModeInt);
    void renderDebug(const DebugInfo& di, int kwpModeInt);

    void print(uint8_t x, uint8_t y, const __FlashStringHelper* s) const;
    void print(uint8_t x, uint8_t y, const char* s) const;
    void print(uint8_t x, uint8_t y, const char* s, uint8_t width) const;
    void print(uint8_t x, uint8_t y, int32_t value) const;
    // Print a ×10 fixed-point value with one decimal place (e.g. 123 → "12.3").
    void print(uint8_t x, uint8_t y, int32_t value, uint8_t decimals, uint8_t width = 0) const;
    void clearRegion(uint8_t x, uint8_t y, uint8_t width);

  private:
    ::Display& display_;

    void initMenuCockpit(uint8_t screen, uint8_t addrSelected);
    void initMenuExperimental();
    void initMenuDebug();
    static void initMenuDtc(uint8_t screen);
    static void initMenuSettings(uint8_t screen);

    void displayMenuCockpit(uint8_t screen, uint8_t addrSelected, const Model::OBDSignals& signals,
                            bool forceUpdate);
    void displayMenuExperimental(uint8_t screen, const Model::OBDSignals& signals,
                                 bool forceUpdate);
    static void displayMenuDebug(uint8_t screen, const Model::OBDSignals& signals, int kwpModeInt,
                                 bool forceUpdate);
    static void displayMenuDtc(uint8_t screen, const Model::DTCStore& dtcStore, bool forceUpdate);
    static void displayMenuSettings(uint8_t screen, int kwpModeInt, bool forceUpdate);
};

} // namespace Display
} // namespace obd
