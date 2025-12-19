#pragma once

#include <Arduino.h>
#include "MenuState.h"
#include "../Display/DisplayTypes.h"
#include "../KWP/KWP1281Session.h"

namespace obd {
namespace Input {

struct InputActions {
    bool requestReconnect = false;
    bool requestExit = false;

    bool readDtc = false;
    bool clearDtc = false;

    bool invertGroupSide = false;

    bool toggleKwpMode = false;

    // optional changes for KWP mode/group can be requested via
    // toggleKwpMode and are applied in OBDDisplay.
};

class ButtonInput {
public:
    explicit ButtonInput(uint8_t analogPin);

    // Returns true if any action occurred
    bool update(MenuState &menuState, InputActions &actions);

    bool isSelectPressed() const;

private:
    uint8_t analogPin_;

    int readRaw() const;

    bool isRight(int v) const { return v < 60; }
    bool isUp(int v) const { return v >= 60 && v < 200; }
    bool isDown(int v) const { return v >= 200 && v < 400; }
    bool isLeft(int v) const { return v >= 400 && v < 600; }
    bool isSelect(int v) const { return v >= 600 && v < 800; }
};

} // namespace Input
} // namespace obd
