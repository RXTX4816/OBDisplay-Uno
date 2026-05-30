// SPDX-License-Identifier: GPL-3.0-or-later
#include "OBDDisplay.h"
#include "../debug.h"

// AVR linker-defined heap bounds; must be at global scope for freeRam_().
extern int __heap_start;
extern int* __brkval;

namespace obd
{

using namespace Display;
using namespace KWP;
using namespace Model;
using namespace Input;

static constexpr uint16_t ECU_TIMEOUT_MS = 1300;
static constexpr uint16_t DISPLAY_FRAME_LENGTH_MS = 177;
static constexpr uint16_t BUTTON_TIMEOUT_MS = 222;
static constexpr uint16_t RECONNECT_DELAY_MS = 5000;
static constexpr uint16_t BUTTON_REPEAT_INITIAL_MS = 400; // delay before first auto-repeat
static constexpr uint16_t BUTTON_REPEAT_PERIOD_MS = 120;  // interval between repeats

// 5-way navigation switch pins (active LOW, INPUT_PULLUP)
static constexpr uint8_t BTN_PIN_UP = 4;
static constexpr uint8_t BTN_PIN_DOWN = 5;
static constexpr uint8_t BTN_PIN_LEFT = 6;
static constexpr uint8_t BTN_PIN_RIGHT = 7;
static constexpr uint8_t BTN_PIN_MID = 8;

// Bitmask values for pendingBtns_ latch
static constexpr uint8_t BTN_MASK_RIGHT = 0x01;
static constexpr uint8_t BTN_MASK_LEFT = 0x02;
static constexpr uint8_t BTN_MASK_UP = 0x04;
static constexpr uint8_t BTN_MASK_DOWN = 0x08;
static constexpr uint8_t BTN_MASK_MID = 0x10;

// ECU address list (PROGMEM to save RAM)
static const uint8_t kEcuAddrs[] PROGMEM = {0x01, 0x03, 0x08, 0x17, 0x19, 0x46};
static const char kEcuN0[] PROGMEM = "Engine";
static const char kEcuN1[] PROGMEM = "ABS Brakes";
static const char kEcuN2[] PROGMEM = "Auto HVAC";
static const char kEcuN3[] PROGMEM = "Instrument";
static const char kEcuN4[] PROGMEM = "CAN Gatway";
static const char kEcuN5[] PROGMEM = "Cntr Conv.";
static PGM_P const kEcuNames[] PROGMEM = {kEcuN0, kEcuN1, kEcuN2, kEcuN3, kEcuN4, kEcuN5};
static constexpr uint8_t kEcuCount = 6;

static uint8_t ecuAddrAt(uint8_t i)
{
    return pgm_read_byte(&kEcuAddrs[i]);
}
static void ecuNameAt(uint8_t i, char* buf)
{
    strncpy_P(buf, (PGM_P)pgm_read_word(&kEcuNames[i]), 10);
    buf[10] = '\0';
}

OBDDisplay::OBDDisplay(uint8_t rxPin, uint8_t txPin, ::Display& display)
    : obdSerial_(rxPin, txPin), display_(display), kwp_(obdSerial_, txPin), signals_(), dtcStore_(),
      menuState_(), buttons_(BTN_PIN_UP, BTN_PIN_DOWN, BTN_PIN_LEFT, BTN_PIN_RIGHT, BTN_PIN_MID),
      simulationModeActive_(false), autoSetup_(false), baudRate_(0), addrSelected_(0x00),
      kwpMode_(Mode::ReadSensors), kwpModeLast_(Mode::ReadSensors),
      kwpModeBeforeGroup_(Mode::ReadSensors), kwpGroup_(1), inGroupScreen_(false),
      connected_(false), connectTimeStart_(0), displayFrameTimestamp_(0), buttonTimeoutUntil_(0)
{
}

void OBDDisplay::begin()
{
    // Serial debug is handled elsewhere if needed
    display_.begin();

    // Configure serial session initial defaults (kept same as old globals)
    baudRate_ = 0;
    addrSelected_ = 0x00;
    kwp_.setConfig(baudRate_, addrSelected_);

    startupAnimation_();

    // Perform interactive setup like original connect() before first connect.
    runSetupFlow_();

    // After setup, wait for explicit user confirmation to start the actual ECU connect.
    phase_ = Phase::WaitingForConnect;
    showWaitingScreen_();

    connectTimeStart_ = millis();
    displayFrameTimestamp_ = millis();
    buttonTimeoutUntil_ = 0;
    reconnectAfterMs_ = millis() + RECONNECT_DELAY_MS;
}

void OBDDisplay::startupAnimation_()
{
    display_.clear();
    display_.print(0, 6, F("O B D"));
    display_.print(2, 8, F("DISPLAY"));

    uint32_t start = millis();
    while (millis() - start < 777)
    {
        if (buttons_.isSelectPressed())
        {
            autoSetup_ = true;
            break;
        }
    }

    // Mirror old AUTO_SETUP defaults when user holds SELECT during splash.
    if (autoSetup_)
    {
        static constexpr uint8_t AUTO_SETUP_ADDRESS = 0x17; // ADDR_INSTRUMENTS
        static constexpr uint16_t AUTO_SETUP_BAUD_RATE = 10400;
        addrSelected_ = AUTO_SETUP_ADDRESS;
        baudRate_ = AUTO_SETUP_BAUD_RATE;
        kwp_.setConfig(baudRate_, addrSelected_);
    }

    dtcStore_.reset();
}

void OBDDisplay::showWaitingScreen_()
{
    display_.clear();
    display_.print(0, 0, simulationModeActive_ ? F("Mode: SIM") : F("Mode: ECU"));
    display_.print(0, 1, F("Baud:"));
    display_.print(5, 1, (int32_t)baudRate_);
    {
        char addrBuf[11] = "Addr: 0x";
        addrBuf[8] = "0123456789ABCDEF"[(addrSelected_ >> 4) & 0xF];
        addrBuf[9] = "0123456789ABCDEF"[addrSelected_ & 0xF];
        addrBuf[10] = '\0';
        display_.print(0, 2, addrBuf);
    }
    display_.print(0, 3, autoReconnect_ ? F("AutoRcn: Y") : F("AutoRcn: N"));
    display_.print(0, 4, F("--------"));
    if (autoReconnect_)
    {
        display_.print(0, 6, F("AUTO"));
        display_.print(0, 8, F("CONNECT"));
    }
    else
    {
        display_.print(0, 6, F("< ENTER >"));
        display_.print(0, 8, F("< SELECT>"));
    }
    display_.flush();
}

void OBDDisplay::runSetupFlow_(uint8_t startStage)
{
    reconnectAttempts_ = 0;
    signals_.reset();
    dtcStore_.reset();

    if (autoSetup_)
    {
        kwp_.setConfig(baudRate_, addrSelected_);
        return;
    }

    static const uint16_t supportedBaudRates[5] = {1200, 2400, 4800, 9600, 10400};

    // Track baud index across back-navigation; seed from current baudRate_ if valid.
    uint8_t baudPtr = 3; // default 9600
    for (uint8_t i = 0; i < 5; ++i)
        if (supportedBaudRates[i] == baudRate_)
        {
            baudPtr = i;
            break;
        }

    uint8_t stage = startStage;
    while (stage <= 3)
    {
        switch (stage)
        {
            case 0: // Mode: ECU or SIM
            {
                display_.beginBatch();
                display_.clear();
                display_.print(0, 5, F("Mode:"));
                display_.print(0, 7, F("< ECU"));
                display_.print(0, 8, F("  SIM >"));
                display_.endBatch();

                for (;;)
                {
                    if (digitalRead(BTN_PIN_RIGHT) == LOW)
                    {
                        simulationModeActive_ = true;
                        delay(333);
                        break;
                    }
                    if (digitalRead(BTN_PIN_LEFT) == LOW)
                    {
                        simulationModeActive_ = false;
                        delay(333);
                        break;
                    }
                }
                stage = 1;
                break;
            }
            case 1: // Baud rate
            {
                char baudStr[8];
                auto drawBaud = [&]()
                {
                    display_.beginBatch();
                    display_.clear();
                    display_.print(0, 0, simulationModeActive_ ? F("Mode: SIM") : F("Mode: ECU"));
                    display_.print(0, 2, F("Baud:"));
                    ltoa((long)supportedBaudRates[baudPtr], baudStr, 10);
                    display_.print(0, 3, F("< Sel >"));
                    display_.print(0, 4, baudStr, 8);
                    display_.print(0, 15, F("UP:back"));
                    display_.endBatch();
                };
                drawBaud();

                bool done = false;
                while (!done)
                {
                    if (digitalRead(BTN_PIN_UP) == LOW)
                    {
                        delay(333);
                        stage = 0;
                        done = true;
                    }
                    else if (digitalRead(BTN_PIN_RIGHT) == LOW)
                    {
                        baudPtr = (baudPtr >= 4) ? 0 : static_cast<uint8_t>(baudPtr + 1);
                        drawBaud();
                        delay(333);
                    }
                    else if (digitalRead(BTN_PIN_LEFT) == LOW)
                    {
                        baudPtr = (baudPtr == 0) ? 4 : static_cast<uint8_t>(baudPtr - 1);
                        drawBaud();
                        delay(333);
                    }
                    else if (digitalRead(BTN_PIN_MID) == LOW)
                    {
                        baudRate_ = supportedBaudRates[baudPtr];
                        delay(333);
                        stage = 2;
                        done = true;
                    }
                    delay(10);
                }
                break;
            }
            case 2: // ECU address
            {
                uint8_t prevAddr = addrSelected_;

                // Seed addrPtr from the current selection.
                uint8_t addrPtr = 0;
                for (uint8_t i = 0; i < kEcuCount; ++i)
                    if (ecuAddrAt(i) == addrSelected_)
                    {
                        addrPtr = i;
                        break;
                    }

                char nameBuf[11];
                char hexBuf[5]; // "0x01"
                auto drawAddr = [&]()
                {
                    ecuNameAt(addrPtr, nameBuf);
                    uint8_t a = ecuAddrAt(addrPtr);
                    hexBuf[0] = '0';
                    hexBuf[1] = 'x';
                    hexBuf[2] = "0123456789ABCDEF"[(a >> 4) & 0xF];
                    hexBuf[3] = "0123456789ABCDEF"[a & 0xF];
                    hexBuf[4] = '\0';
                    display_.beginBatch();
                    display_.clear();
                    display_.print(0, 0, simulationModeActive_ ? F("Mode: SIM") : F("Mode: ECU"));
                    display_.print(0, 1, F("Baud:"));
                    display_.print(5, 1, (int32_t)baudRate_);
                    display_.print(0, 3, F("ECU Addr:"));
                    display_.print(0, 4, F("< Sel >"));
                    display_.print(0, 5, hexBuf);
                    display_.print(0, 6, nameBuf);
                    display_.print(0, 15, F("UP:back"));
                    display_.endBatch();
                };
                drawAddr();

                bool done = false;
                while (!done)
                {
                    if (digitalRead(BTN_PIN_UP) == LOW)
                    {
                        delay(333);
                        stage = 1;
                        done = true;
                    }
                    else if (digitalRead(BTN_PIN_RIGHT) == LOW)
                    {
                        addrPtr =
                            (addrPtr >= kEcuCount - 1) ? 0 : static_cast<uint8_t>(addrPtr + 1);
                        drawAddr();
                        delay(333);
                    }
                    else if (digitalRead(BTN_PIN_LEFT) == LOW)
                    {
                        addrPtr = (addrPtr == 0) ? static_cast<uint8_t>(kEcuCount - 1)
                                                 : static_cast<uint8_t>(addrPtr - 1);
                        drawAddr();
                        delay(333);
                    }
                    else if (digitalRead(BTN_PIN_MID) == LOW)
                    {
                        addrSelected_ = ecuAddrAt(addrPtr);
                        delay(333);
                        stage = 3;
                        done = true;
                    }
                    delay(10);
                }
                if (addrSelected_ != prevAddr)
                    reconnectAttempts_ = 0;
                break;
            }
            case 3: // Auto-reconnect
            {
                display_.beginBatch();
                display_.clear();
                display_.print(0, 0, simulationModeActive_ ? F("Mode: SIM") : F("Mode: ECU"));
                display_.print(0, 1, F("Baud:"));
                display_.print(5, 1, (int32_t)baudRate_);
                {
                    char addrBuf[11];
                    addrBuf[0] = 'A';
                    addrBuf[1] = 'd';
                    addrBuf[2] = 'd';
                    addrBuf[3] = 'r';
                    addrBuf[4] = ':';
                    addrBuf[5] = ' ';
                    addrBuf[6] = '0';
                    addrBuf[7] = 'x';
                    addrBuf[8] = "0123456789ABCDEF"[(addrSelected_ >> 4) & 0xF];
                    addrBuf[9] = "0123456789ABCDEF"[addrSelected_ & 0xF];
                    addrBuf[10] = '\0';
                    display_.print(0, 2, addrBuf);
                }
                display_.print(0, 4, F("AutoRcn:"));
                display_.print(0, 5, F("< N"));
                display_.print(0, 6, F("  Y >"));
                display_.print(0, 15, F("UP:back"));
                display_.endBatch();

                for (;;)
                {
                    if (digitalRead(BTN_PIN_UP) == LOW)
                    {
                        delay(333);
                        stage = 2;
                        break;
                    }
                    if (digitalRead(BTN_PIN_RIGHT) == LOW)
                    {
                        autoReconnect_ = true;
                        delay(333);
                        stage = 4;
                        break;
                    }
                    if (digitalRead(BTN_PIN_LEFT) == LOW)
                    {
                        autoReconnect_ = false;
                        delay(333);
                        stage = 4;
                        break;
                    }
                }
                break;
            }
            default:
                stage = 4; // exit loop
                break;
        }
    }

    kwp_.setConfig(baudRate_, addrSelected_);
}

void OBDDisplay::update()
{
    // Phase-based behaviour to mirror original UX.
    if (phase_ == Phase::Setup)
    {
        // Allow re-running the interactive setup flow after a manual
        // exit. This mirrors the original behaviour where the user
        // could change mode/baud/address again.
        runSetupFlow_();

        // After setup, go back to the explicit press-to-connect
        // prompt.
        phase_ = Phase::WaitingForConnect;
        showWaitingScreen_();

        connectTimeStart_ = millis();
        displayFrameTimestamp_ = millis();
        buttonTimeoutUntil_ = 0;
        reconnectAfterMs_ = millis() + RECONNECT_DELAY_MS;
        return;
    }

    if (phase_ == Phase::WaitingForConnect)
    {
        uint32_t now = millis();

        // UP while autoReconnect is off: re-enter setup starting at AutoRCN stage so
        // the user can step back through their settings.
        if (!autoReconnect_ && now >= buttonTimeoutUntil_ && digitalRead(BTN_PIN_UP) == LOW)
        {
            runSetupFlow_(3);
            showWaitingScreen_();
            buttonTimeoutUntil_ = millis() + BUTTON_TIMEOUT_MS;
            reconnectAfterMs_ = millis() + RECONNECT_DELAY_MS;
            return;
        }

        // LEFT or RIGHT toggles auto-reconnect (debounced via buttonTimeoutUntil_).
        if (now >= buttonTimeoutUntil_ &&
            (digitalRead(BTN_PIN_LEFT) == LOW || digitalRead(BTN_PIN_RIGHT) == LOW))
        {
            autoReconnect_ = !autoReconnect_;
            if (!autoReconnect_)
                reconnectAttempts_ = 0;
            buttonTimeoutUntil_ = now + BUTTON_TIMEOUT_MS;
            showWaitingScreen_();
            return;
        }

        bool shouldConnect = false;

        if (autoReconnect_ && now >= reconnectAfterMs_)
        {
            reconnectAttempts_++;
            if (reconnectAttempts_ > 5)
            {
                autoReconnect_ = false;
                reconnectAttempts_ = 0;
                showWaitingScreen_();
                return;
            }
            shouldConnect = true;
        }
        else if (now >= buttonTimeoutUntil_ && buttons_.isSelectPressed())
        {
            reconnectAttempts_ = 0;
            shouldConnect = true;
        }

        if (!shouldConnect)
            return;

        phase_ = Phase::Running;
        menuState_ = Input::MenuState();
        menuState_.markMenuChanged();

        if (simulationModeActive_)
            connected_ = true;
    }

    // Always keep UI responsive, even when not connected to an ECU.
    bool wasConnected = connected_;
    bool nowConnected = ensureConnected_();

    // Only talk to ECU or run simulation when we have (or had) a connection.
    // If ensureConnected_() failed in ECU mode, it already showed an
    // error and returned to PRESS SELECT; in that case we must not run
    // the tripcomputer loop.
    if ((nowConnected || wasConnected || simulationModeActive_) && phase_ == Phase::Running)
    {
        updateKwpOrSimulation_();
        computeValues_();
    }

    handleInput_();
    updateDisplay_();
}

bool OBDDisplay::ensureConnected_()
{
    // In simulation mode, we never talk to a real ECU; treat as always connected.
    if (simulationModeActive_)
    {
        return true;
    }

    if (connected_)
    {
        return true;
    }

    // If connection was lost after being established, skip retry and go to press-select screen
    if (wasConnected_)
    {
        display_.clear();
        display_.print(0, 6, F("Conn. ERR"));
        display_.print(0, 8, F("Lost"));
        display_.flush();
        delay(1000);

        reconnectAfterMs_ = millis() + RECONNECT_DELAY_MS;
        phase_ = Phase::WaitingForConnect;
        showWaitingScreen_();
        buttonTimeoutUntil_ = 0;
        lastConnectionFailed_ = false;
        wasConnected_ = false;
        return false;
    }

    // If we have no valid configuration yet, don't block the UI; behave like
    // the original sketch where menus were shown before any connection.
    if (baudRate_ == 0 || addrSelected_ == 0x00)
    {
        lastConnectionFailed_ = false;
        return false;
    }

    // Skip retry attempt after initial failed connection (go to press-select screen with same
    // settings)
    if (lastConnectionFailed_)
    {
        lastConnectionFailed_ = false;
        reconnectAfterMs_ = millis() + RECONNECT_DELAY_MS;
        phase_ = Phase::WaitingForConnect;
        showWaitingScreen_();
        buttonTimeoutUntil_ = 0;
        return false;
    }

    if (!kwp_.connectToEcu(simulationModeActive_, autoSetup_, baudRate_, addrSelected_))
    {
        kwp_.disconnect();
        connected_ = false;

        // In ECU mode, a failed connect should behave like the old obd_connect():
        // show an error and do not start the tripcomputer loop.
        if (!simulationModeActive_)
        {
            display_.clear();
            display_.print(0, 0, F("Conn. ERR"));
            display_.print(0, 1, F("Retry..."));

            const __FlashStringHelper* reason = nullptr;
            switch (kwp_.lastConnectError())
            {
                case DBG_KWP_TIMEOUT:
                    reason = F("Timeout");
                    break;
                case DBG_KWP_COMPLEMENT:
                    reason = F("Compl.err");
                    break;
                case DBG_KWP_SYNC_MISMATCH:
                    reason = F("Bad sync");
                    break;
                case DBG_KWP_SYNC_FAIL:
                    reason = F("No sync");
                    break;
                case DBG_KWP_BLOCKS_FAIL:
                    reason = F("Blk err");
                    break;
            }
            if (reason)
                display_.print(0, 2, reason);
            display_.flush();

            delay(3000);

            // Set flag to skip immediate retry attempt on next frame
            lastConnectionFailed_ = true;
        }

        return false;
    }

    connected_ = true;
    wasConnected_ = true;
    connectTimeStart_ = millis();
    // After a successful connect, always start in the cockpit menu (tripcomputer)
    // like the original sketch did.
    menuState_ = Input::MenuState(); // reset to defaults (Cockpit, screen 0)
    menuState_.markMenuChanged();

    // Seed one round of data so the very first cockpit frame drawn
    // after connect is fully populated without waiting for a manual
    // screen change.
    updateKwpOrSimulation_();
    computeValues_();
    return true;
}

void OBDDisplay::updateKwpOrSimulation_()
{
    if (!simulationModeActive_)
    {
        switch (kwpMode_)
        {
            case Mode::Ack:
                if (!kwp_.keepAlive())
                {
                    kwp_.disconnect();
                    connected_ = false;
                }
                break;
            case Mode::ReadGroup:
                if (!kwp_.readSensorsGroup(kwpGroup_, signals_))
                {
                    kwp_.disconnect();
                    connected_ = false;
                }
                break;
            case Mode::ReadSensors:
            default:
                for (uint8_t g = 1; g <= 3; ++g)
                {
                    if (!kwp_.readSensorsGroup(g, signals_))
                    {
                        kwp_.disconnect();
                        connected_ = false;
                        break;
                    }
                }
                break;
        }
    }
    else
    {
        signals_.updateSimulation();
        delay(222);
    }
}

void OBDDisplay::computeValues_()
{
    signals_.compute(millis(), connectTimeStart_);
}

void OBDDisplay::pollButtons()
{
    uint8_t current = 0;
    if (digitalRead(BTN_PIN_RIGHT) == LOW)
        current |= BTN_MASK_RIGHT;
    if (digitalRead(BTN_PIN_LEFT) == LOW)
        current |= BTN_MASK_LEFT;
    if (digitalRead(BTN_PIN_UP) == LOW)
        current |= BTN_MASK_UP;
    if (digitalRead(BTN_PIN_DOWN) == LOW)
        current |= BTN_MASK_DOWN;
    if (digitalRead(BTN_PIN_MID) == LOW)
        current |= BTN_MASK_MID;

    // Latch only rising edges (0→1 transitions) so a held button never
    // re-fires after the debounce timeout expires.
    pendingBtns_ |= (current & ~lastBtns_);
    lastBtns_ = current;
}

void OBDDisplay::handleInput_()
{
    // Always latch the current button state so brief presses occurring
    // during the slow KWP read phase are captured here too.
    pollButtons();

    uint32_t now = millis();
    uint8_t btns = 0;

    if (pendingBtns_ != 0 && now >= buttonTimeoutUntil_)
    {
        // Fresh press: consume latch and arm auto-repeat for directional buttons.
        btns = pendingBtns_;
        pendingBtns_ = 0;
        buttonTimeoutUntil_ = now + BUTTON_TIMEOUT_MS;

        // MID (SELECT) never auto-repeats — only directional buttons do.
        repeatBtns_ = btns & (BTN_MASK_LEFT | BTN_MASK_RIGHT | BTN_MASK_UP | BTN_MASK_DOWN);
        if (repeatBtns_ != 0)
            repeatFireAt_ = now + BUTTON_REPEAT_INITIAL_MS;
    }
    else if (repeatBtns_ != 0)
    {
        if ((repeatBtns_ & lastBtns_) == 0)
        {
            // Button released — cancel auto-repeat.
            repeatBtns_ = 0;
        }
        else if (now >= repeatFireAt_)
        {
            // Auto-repeat fires; only for buttons still physically held.
            btns = repeatBtns_ & lastBtns_;
            repeatFireAt_ = now + BUTTON_REPEAT_PERIOD_MS;
        }
    }

    if (btns == 0)
        return;

    // Navigation + action dispatch (mirrors ButtonInput::update logic,
    // but reads from the latched bitmask rather than live digitalRead).
    InputActions actions{};
    bool any = false;

    if (btns & BTN_MASK_RIGHT)
    {
        menuState_.nextMenu();
        dtcShowActive_ = false; // leave DTC show mode when navigating away
        any = true;
    }
    else if (btns & BTN_MASK_LEFT)
    {
        menuState_.prevMenu();
        dtcShowActive_ = false;
        any = true;
    }
    else
    {
        using Display::MenuId;
        switch (menuState_.currentMenu())
        {
            case MenuId::Cockpit:
            case MenuId::Experimental:
            case MenuId::Debug:
            {
                MenuId mid = menuState_.currentMenu();
                if (btns & BTN_MASK_UP)
                {
                    menuState_.nextScreen(mid);
                    any = true;
                }
                else if (btns & BTN_MASK_DOWN)
                {
                    menuState_.prevScreen(mid);
                    any = true;
                }
                break;
            }
            case MenuId::Dtc:
                if (dtcShowActive_)
                {
                    // DTC show sub-view: up/down page, MID = back
                    uint8_t pageMax = dtcCount_ > 0 ? static_cast<uint8_t>((dtcCount_ - 1) / 4) : 0;
                    if (btns & BTN_MASK_UP)
                    {
                        if (dtcShowPage_ < pageMax)
                            dtcShowPage_++;
                        any = true;
                    }
                    else if (btns & BTN_MASK_DOWN)
                    {
                        if (dtcShowPage_ > 0)
                            dtcShowPage_--;
                        any = true;
                    }
                    else if (btns & BTN_MASK_MID)
                    {
                        dtcShowActive_ = false;
                        any = true;
                    }
                }
                else
                {
                    // DTC main menu: up/down moves cursor, MID executes
                    if (btns & BTN_MASK_UP)
                    {
                        dtcMenuCursor_ = (dtcMenuCursor_ + 2) % 3;
                        any = true;
                    }
                    else if (btns & BTN_MASK_DOWN)
                    {
                        dtcMenuCursor_ = (dtcMenuCursor_ + 1) % 3;
                        any = true;
                    }
                    else if (btns & BTN_MASK_MID)
                    {
                        if (dtcMenuCursor_ == 0)
                        {
                            actions.readDtc = true;
                            any = true;
                        }
                        else if (dtcMenuCursor_ == 1)
                        {
                            actions.clearDtc = true;
                            any = true;
                        }
                        else
                        {
                            dtcShowActive_ = true;
                            dtcShowPage_ = 0;
                            any = true;
                        }
                    }
                }
                break;
            case MenuId::Settings:
                if (btns & BTN_MASK_UP)
                {
                    settingsMenuCursor_ = (settingsMenuCursor_ + 2) % 3;
                    any = true;
                }
                else if (btns & BTN_MASK_DOWN)
                {
                    settingsMenuCursor_ = (settingsMenuCursor_ + 1) % 3;
                    any = true;
                }
                else if (btns & BTN_MASK_MID)
                {
                    if (settingsMenuCursor_ == 0)
                    {
                        actions.requestExit = true;
                        any = true;
                    }
                    else if (settingsMenuCursor_ == 1)
                    {
                        actions.toggleKwpMode = true;
                        any = true;
                    }
                    else
                    {
                        autoReconnect_ = !autoReconnect_;
                        if (!autoReconnect_)
                            reconnectAttempts_ = 0;
                        menuState_.markScreenChanged();
                        any = true;
                    }
                }
                break;
        }
    }

    if (!any)
    {
        return;
    }

    if (actions.requestReconnect)
    {
        if (!simulationModeActive_)
        {
            kwp_.disconnect();
            connected_ = false;
            wasConnected_ = false;
            phase_ = Phase::WaitingForConnect;
            showWaitingScreen_();
        }
        return;
    }

    if (actions.requestExit)
    {
        if (connected_ && !simulationModeActive_)
        {
            kwp_.exitSession();
        }
        kwp_.disconnect();
        connected_ = false;
        wasConnected_ = false;
        phase_ = Phase::Setup;
        buttonTimeoutUntil_ = millis() + BUTTON_TIMEOUT_MS;
        return;
    }

    if (actions.toggleKwpMode)
    {
        switch (kwpMode_)
        {
            case Mode::Ack:
                kwpMode_ = Mode::ReadGroup;
                break;
            case Mode::ReadGroup:
                kwpMode_ = Mode::ReadSensors;
                break;
            case Mode::ReadSensors:
            default:
                kwpMode_ = Mode::Ack;
                break;
        }
        menuState_.markScreenChanged();
    }
    // Auto-switch to ReadGroup when entering the Experimental (Group) screen,
    // and restore the previous mode when leaving.
    {
        bool nowInGroup = (menuState_.currentMenu() == Display::MenuId::Experimental);
        if (nowInGroup && !inGroupScreen_)
        {
            kwpModeBeforeGroup_ = kwpMode_;
            kwpMode_ = Mode::ReadGroup;
            menuState_.markScreenChanged();
        }
        else if (!nowInGroup && inGroupScreen_)
        {
            kwpMode_ = kwpModeBeforeGroup_;
            menuState_.markScreenChanged();
        }
        inGroupScreen_ = nowInGroup;
    }

    // Keep groupCurrent and kwpGroup_ in sync with the experimental screen index.
    if (menuState_.currentMenu() == Display::MenuId::Experimental)
    {
        if (menuState_.screen(Display::MenuId::Experimental) == 0)
        {
            menuState_.setScreen(Display::MenuId::Experimental, 1);
        }
        signals_.experimental.groupCurrent = menuState_.screen(Display::MenuId::Experimental);
        kwpGroup_ = signals_.experimental.groupCurrent; // ReadGroup mode uses this
        signals_.experimental.kUpdated = true;
    }

    if (actions.readDtc)
    {
        if (simulationModeActive_)
        {
            for (uint8_t i = 0; i < 16; ++i)
            {
                uint16_t code = (uint16_t)(i * 1000u);
                uint8_t status = (uint8_t)(i * 10u);
                dtcStore_.set(i, code, status);
            }
            dtcCount_ = 16;
        }
        else
        {
            int8_t cnt = kwp_.readDtcCodes(dtcStore_);
            if (cnt < 0)
            {
                display_.clear();
                display_.print(0, 6, F("DTC error"));
                display_.print(0, 8, F("Disconn."));
                delay(1222);
                kwp_.disconnect();
                connected_ = false;
                phase_ = Phase::WaitingForConnect;
                showWaitingScreen_();
            }
            else
            {
                dtcCount_ = cnt;
                dtcStatusType_ = 1;
                dtcStatusValue_ = cnt;
                dtcStatusUntil_ = millis() + 1500;
                menuState_.markScreenChanged();
            }
        }
    }
    if (actions.clearDtc)
    {
        if (simulationModeActive_)
        {
            dtcStore_.reset();
            dtcCount_ = 0;
        }
        else
        {
            if (!kwp_.deleteDtcCodes())
            {
                display_.clear();
                display_.print(0, 6, F("DTC delete"));
                display_.print(0, 8, F("No supp."));
                delay(1222);
            }
            else
            {
                dtcStore_.reset();
                dtcCount_ = 0;
                dtcStatusType_ = 2;
                dtcStatusUntil_ = millis() + 1500;
                menuState_.markScreenChanged();
            }
        }
    }
}

int16_t OBDDisplay::freeRam_()
{
    int v;
    return (int16_t)((int)&v - (::__brkval == 0 ? (int)&::__heap_start : (int)::__brkval));
}

void OBDDisplay::updateDisplay_()
{
    uint32_t now = millis();

    display_.beginBatch();

    if (phase_ == Phase::Running)
    {
        bool menuChanged = menuState_.consumeMenuChanged() || menuState_.consumeScreenChanged();
        bool timeToUpdate = now >= displayFrameTimestamp_;

        if (menuChanged || timeToUpdate)
        {
            display_.clear();

            using Display::MenuId;
            switch (menuState_.currentMenu())
            {
                case MenuId::Dtc:
                    display_.renderDtcMenu(dtcMenuCursor_, dtcShowActive_, dtcShowPage_, dtcCount_,
                                           dtcStore_);
                    if (dtcStatusUntil_ > 0 && !dtcShowActive_)
                    {
                        if (now < dtcStatusUntil_)
                        {
                            if (dtcStatusType_ == 1)
                            {
                                display_.print(0, 10, F("Read OK"));
                                display_.print(0, 11, F("DTCs:"));
                                display_.print(5, 11, (int32_t)dtcStatusValue_);
                            }
                            else if (dtcStatusType_ == 2)
                            {
                                display_.print(0, 10, F("Clear OK"));
                            }
                        }
                        else
                        {
                            dtcStatusUntil_ = 0;
                            dtcStatusType_ = 0;
                        }
                    }
                    break;

                case MenuId::Settings:
                    display_.renderSettings(settingsMenuCursor_, static_cast<int>(kwpMode_),
                                            autoReconnect_, kwp_.ecuLinesData(),
                                            kwp_.ecuLineCount());
                    break;

                case MenuId::Debug:
                {
                    Display::DebugInfo di;
                    di.serialCon = obdSerial_.isListening() ? 1u : 0u;
                    di.serialAva = (uint8_t)min((int)obdSerial_.available(), 255);
                    di.blockCtr = kwp_.getBlockCounter();
                    di.attempts = reconnectAttempts_;
                    di.addr = addrSelected_;
                    di.baud = baudRate_;
                    di.group = kwpGroup_;
                    di.sim = simulationModeActive_;
                    di.freeRam = freeRam_();
                    display_.renderDebug(di, static_cast<int>(kwpMode_));
                    break;
                }

                default:
                    display_.initMenu(menuState_, addrSelected_, static_cast<int>(kwpMode_));
                    display_.render(menuState_, signals_, dtcStore_, addrSelected_,
                                    static_cast<int>(kwpMode_), true);
                    break;
            }

            if (timeToUpdate)
                displayFrameTimestamp_ = now + DISPLAY_FRAME_LENGTH_MS;
        }
    }

    display_.endBatch();
}

void OBDDisplay::incrementExperimentalGroup_()
{
    auto& eg = signals_.experimental;
    const uint8_t groupMax = 64;
    if (eg.groupCurrent >= groupMax)
    {
        eg.groupCurrent = 1;
    }
    else
    {
        eg.groupCurrent++;
    }
    eg.kUpdated = true;
}

void OBDDisplay::decrementExperimentalGroup_()
{
    auto& eg = signals_.experimental;
    const uint8_t groupMax = 64;
    if (eg.groupCurrent <= 1)
    {
        eg.groupCurrent = groupMax;
    }
    else
    {
        eg.groupCurrent--;
    }
    eg.kUpdated = true;
}

} // namespace obd
