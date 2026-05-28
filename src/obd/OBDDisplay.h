// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>
#include "../display/Display.h"
#include "../serial/NewSoftwareSerial.h"
#include "Display/DisplayManager.h"
#include "KWP/KWP1281Session.h"
#include "Model/OBDSignals.h"
#include "Model/DTCStore.h"
#include "Input/MenuState.h"
#include "Input/ButtonInput.h"

namespace obd
{

class OBDDisplay
{
  public:
    OBDDisplay(uint8_t rxPin, uint8_t txPin, ::Display& display);

    void begin();
    void update();

    // Called from the scheduler input task to latch pressed buttons at a
    // high frequency, independently of the main update() cycle.
    void pollButtons();

    bool isConnected() const { return connected_; }

  private:
    NewSoftwareSerial obdSerial_;
    Display::DisplayManager display_;
    KWP::KWP1281Session kwp_;
    Model::OBDSignals signals_;
    Model::DTCStore dtcStore_;
    Input::MenuState menuState_;
    Input::ButtonInput buttons_;

    bool simulationModeActive_;
    bool autoSetup_;
    uint16_t baudRate_;
    uint8_t addrSelected_;
    KWP::Mode kwpMode_;
    KWP::Mode kwpModeLast_;
    uint8_t kwpGroup_;

    // DTC menu state
    uint8_t dtcMenuCursor_ = 0; // 0=Read, 1=Clear, 2=Show
    bool dtcShowActive_ = false;
    uint8_t dtcShowPage_ = 0;
    int8_t dtcCount_ = -1; // -1=never read, 0..16=count after last read

    // Non-blocking DTC status overlay (shown on DTC main menu after read/clear)
    uint32_t dtcStatusUntil_ = 0; // millis() expiry; 0 = no overlay
    uint8_t dtcStatusType_ = 0;   // 1=read ok, 2=clear ok
    int8_t dtcStatusValue_ = -1;  // dtcCount for type 1

    // Settings menu state
    uint8_t settingsMenuCursor_ = 0; // 0=Exit, 1=KWP Mode

    bool connected_;
    bool wasConnected_ = false;
    bool lastConnectionFailed_ = false;
    uint16_t connectionAttempts_ = 0;
    uint32_t connectTimeStart_;
    uint32_t displayFrameTimestamp_;
    uint32_t buttonTimeoutUntil_;

    // Latched button state — bits set by pollButtons(), consumed by handleInput_().
    // This decouples detection (high-frequency) from action (per update() cycle).
    uint8_t pendingBtns_ = 0;
    uint8_t lastBtns_ = 0;      // previous physical state for rising-edge detection
    uint8_t repeatBtns_ = 0;    // directional buttons in auto-repeat state
    uint32_t repeatFireAt_ = 0; // millis() when next auto-repeat fires

    enum class Phase : uint8_t
    {
        Setup,
        WaitingForConnect,
        Running
    } phase_ = Phase::Setup;

    void startupAnimation_();
    void runSetupFlow_();
    void showWaitingScreen_();
    static int16_t freeRam_();
    void resetState_();
    bool ensureConnected_();
    void updateKwpOrSimulation_();
    void computeValues_();
    void handleInput_();
    void updateDisplay_();

    void incrementExperimentalGroup_();
    void decrementExperimentalGroup_();
};

} // namespace obd
