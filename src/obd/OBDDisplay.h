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

// EEPROM fuel level helpers — implemented in OBDDisplay_setup.cpp
uint8_t readEepromFuel();
void writeEepromFuel(uint8_t liters);

class OBDDisplay
{
  public:
    OBDDisplay(uint8_t rxPin, uint8_t txPin, ::Display& display);

    void begin();

    // Called from the scheduler tasks (see Controller.cpp):
    void updateKwp();     // every loop iteration (INTERVAL_KWP_MS = 0)
    void updateCompute(); // every INTERVAL_COMPUTE_MS
    void updateDisplay(); // every INTERVAL_DISPLAY_MS

    // Called from the scheduler input task to latch pressed buttons at a
    // high frequency, independently of the display update cycle.
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

    bool autoSetup_;
    uint16_t baudRate_;
    uint8_t addrSelected_;
    KWP::Mode kwpMode_;
    KWP::Mode kwpModeLast_;
    KWP::Mode kwpModeBeforeGroup_;
    uint8_t kwpGroup_;
    bool inGroupScreen_;

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
    uint8_t settingsMenuCursor_ = 0; // 0=Exit, 1=KWP Mode, 2=AutoRcn, 3=Fuel (0x17 only)

    // 0x01 fuel tracking
    uint8_t fuelStartL_ = 0; // fuel level at session start (from EEPROM, L)

    // Readiness immediate-poll request: set from input handler when user
    // navigates to the readiness page; cleared after the next group-100 poll.
    bool requestReadiness_ = false;

    bool autoReconnect_ = true;
    uint8_t reconnectAttempts_ = 0;
    uint32_t reconnectAfterMs_ = 0;

    bool connected_;
    bool wasConnected_ = false;
    bool lastConnectionFailed_ = false;
    uint16_t connectionAttempts_ = 0;
    uint32_t connectTimeStart_;
    uint32_t buttonTimeoutUntil_;

    bool displayDirty_ = false; // set by updateKwp/Compute; cleared after each display frame

    // Warning flash overlay: replaces cockpit view for ~3 s when a new warning fires.
    // warningFlashSnapshot_ latches the state at trigger time so the flash keeps showing
    // the correct warning even if the live signal clears before the 3 s expire.
    uint32_t warningFlashUntilMs_ = 0;
    bool warningFlashPhase_ = false;
    uint8_t warningFlashPage_ = 0;
    Model::WarningState warningFlashSnapshot_;

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
    void runSetupFlow_(uint8_t startStage = 0);
    void showWaitingScreen_();
    void drawSetupHeader_(bool showBaud, bool showAddr, bool showBack);
    static int16_t freeRam_();
    void resetState_();
    bool ensureConnected_();
    void pollEcu_();
    void computeValues_();
    void handleInput_();
    void updateDisplay_();

    void incrementExperimentalGroup_();
    void decrementExperimentalGroup_();
};

} // namespace obd
