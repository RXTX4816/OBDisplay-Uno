#pragma once

#include <Arduino.h>
#include "../../LiquidCrystal.h"
#include "../Model/OBDSignals.h"
#include "../Model/DTCStore.h"
#include "../Input/MenuState.h"
#include "DisplayTypes.h"

namespace obd {
namespace Display {

class DisplayManager {
public:
    explicit DisplayManager(LiquidCrystal &lcd);

    void begin(uint8_t cols, uint8_t rows);
    void clear();

    void initMenu(const Input::MenuState &menuState,
                  uint8_t addrSelected,
                  int kwpModeInt);

    void render(const Input::MenuState &menuState,
                const Model::OBDSignals &signals,
                const Model::DTCStore &dtcStore,
                uint8_t addrSelected,
                int kwpModeInt,
                bool forceUpdate);

    void print(uint8_t x, uint8_t y, const __FlashStringHelper *s);
    void print(uint8_t x, uint8_t y, const String &s);
    void print(uint8_t x, uint8_t y, const String &s, uint8_t width);
    void print(uint8_t x, uint8_t y, const char *s, uint8_t width);
    void print(uint8_t x, uint8_t y, int value);

    void print(uint8_t x, uint8_t y, float value, uint8_t width = 0);
    void clearRegion(uint8_t x, uint8_t y, uint8_t width);

private:
    LiquidCrystal &lcd_;

    void initMenuCockpit(uint8_t screen, uint8_t addrSelected);
    void initMenuExperimental();
    void initMenuDebug();
    void initMenuDtc(uint8_t screen);
    void initMenuSettings(uint8_t screen);

    void displayMenuCockpit(uint8_t screen, uint8_t addrSelected,
                            const Model::OBDSignals &signals,
                            bool forceUpdate);
    void displayMenuExperimental(uint8_t screen,
                                 const Model::OBDSignals &signals,
                                 bool forceUpdate);
    void displayMenuDebug(uint8_t screen,
                          const Model::OBDSignals &signals,
                          int kwpModeInt,
                          bool forceUpdate);
    void displayMenuDtc(uint8_t screen,
                        const Model::DTCStore &dtcStore,
                        bool forceUpdate);
    void displayMenuSettings(uint8_t screen,
                             int kwpModeInt,
                             bool forceUpdate);
};

} // namespace Display
} // namespace obd
