#pragma once

#include <Arduino.h>
#include "../LiquidCrystal.h"
#include "../NewSoftwareSerial.h"
#include "Display/DisplayManager.h"
#include "KWP/KWP1281Session.h"
#include "Model/OBDSignals.h"
#include "Model/DTCStore.h"
#include "Input/MenuState.h"
#include "Input/ButtonInput.h"

namespace obd {

class OBDDisplay {
public:
    OBDDisplay(uint8_t rxPin, uint8_t txPin, LiquidCrystal &lcd);

    void begin();   // to be called from Controller::setup()
    void update();  // to be called from Controller::loop()

    bool isConnected() const { return connected_; }

private:
    // Hardware & subsystems
    NewSoftwareSerial obdSerial_;
    Display::DisplayManager display_;
    KWP::KWP1281Session kwp_;
    Model::OBDSignals signals_;
    Model::DTCStore dtcStore_;
    Input::MenuState menuState_;
    Input::ButtonInput buttons_;

    // Config / state migrated from obdisplay.cpp.old
    bool simulationModeActive_;
    bool autoSetup_;
    uint16_t baudRate_;
    uint8_t addrSelected_;
    KWP::Mode kwpMode_;
    KWP::Mode kwpModeLast_;
    uint8_t kwpGroup_;

    bool connected_;
    uint32_t connectTimeStart_;
    uint32_t displayFrameTimestamp_;
    uint32_t buttonTimeoutUntil_;

    // Helper methods mirroring old loop()/setup() structure
    void startupAnimation_();
    void resetState_();
    bool ensureConnected_();
    void updateKwpOrSimulation_();
    void computeValues_();
    void handleInput_();
    void updateDisplay_();
};

} // namespace obd
