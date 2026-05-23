#pragma once

#include <Arduino.h>
#include "MenuState.h"
#include "../Display/DisplayTypes.h"
#include "../KWP/KWP1281Session.h"

namespace obd
{
namespace Input
{

struct InputActions
{
    bool requestReconnect = false;
    bool requestExit = false;

    bool readDtc = false;
    bool clearDtc = false;

    bool invertGroupSide = false;

    bool toggleKwpMode = false;

    // optional changes for KWP mode/group can be requested via
    // toggleKwpMode and are applied in OBDDisplay.
};

class ButtonInput
{
  public:
    explicit ButtonInput(uint8_t analogPin);

    // Returns true if any action occurred
    bool update(MenuState& menuState, InputActions& actions);

    bool isSelectPressed() const;

  private:
    uint8_t analogPin_;

    int readRaw() const;

    static bool isRight(int v) { return v < 60; }
    static bool isUp(int v) { return v >= 60 && v < 200; }
    static bool isDown(int v) { return v >= 200 && v < 400; }
    static bool isLeft(int v) { return v >= 400 && v < 600; }
    static bool isSelect(int v) { return v >= 600 && v < 800; }
};

} // namespace Input
} // namespace obd
