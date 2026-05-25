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
    ButtonInput(uint8_t pinUp, uint8_t pinDown, uint8_t pinLeft, uint8_t pinRight, uint8_t pinMid);

    // Returns true if any action occurred
    bool update(MenuState& menuState, InputActions& actions);

    bool isSelectPressed() const;

  private:
    uint8_t pinUp_, pinDown_, pinLeft_, pinRight_, pinMid_;

    bool readUp() const    { return digitalRead(pinUp_) == LOW; }
    bool readDown() const  { return digitalRead(pinDown_) == LOW; }
    bool readLeft() const  { return digitalRead(pinLeft_) == LOW; }
    bool readRight() const { return digitalRead(pinRight_) == LOW; }
    bool readMid() const   { return digitalRead(pinMid_) == LOW; }
};

} // namespace Input
} // namespace obd
