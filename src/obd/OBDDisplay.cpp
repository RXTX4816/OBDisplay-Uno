#include "OBDDisplay.h"

namespace obd {

using namespace Display;
using namespace KWP;
using namespace Model;
using namespace Input;

static constexpr uint16_t ECU_TIMEOUT_MS = 1300;
static constexpr uint16_t DISPLAY_FRAME_LENGTH_MS = 177;
static constexpr uint16_t BUTTON_TIMEOUT_MS = 222;

OBDDisplay::OBDDisplay(uint8_t rxPin, uint8_t txPin, LiquidCrystal &lcd)
    : obdSerial_(rxPin, txPin, false)
    , display_(lcd)
    , kwp_(obdSerial_)
    , signals_()
    , dtcStore_()
    , menuState_()
    , buttons_(A0) // analog pin for buttons, same as old code
    , simulationModeActive_(false)
    , autoSetup_(false)
    , baudRate_(0)
    , addrSelected_(0x00)
    , kwpMode_(Mode::ReadSensors)
    , kwpModeLast_(Mode::ReadSensors)
    , kwpGroup_(1)
    , connected_(false)
    , connectTimeStart_(0)
    , displayFrameTimestamp_(0)
    , buttonTimeoutUntil_(0)
{
}

void OBDDisplay::begin()
{
    // Serial debug is handled elsewhere if needed
    display_.begin(16, 2);

    // Configure serial session initial defaults (kept same as old globals)
    baudRate_ = 0;
    addrSelected_ = 0x00;
    kwp_.setConfig(baudRate_, addrSelected_);

    startupAnimation_();

    // Perform interactive setup like original connect() before first connect.
    runSetupFlow_();

    // After setup, wait for explicit user confirmation to start the actual ECU connect.
    phase_ = Phase::WaitingForConnect;
    display_.clear();
    display_.print(0, 0, F("->   ENTER   <-"));
    display_.print(0, 1, F("Press SELECT"));

    connectTimeStart_ = millis();
    displayFrameTimestamp_ = millis();
    buttonTimeoutUntil_ = 0;
}

void OBDDisplay::startupAnimation_()
{
    display_.clear();
    display_.print(0, 0, F("O B D"));
    display_.print(1, 1, F("D I S P L A Y"));

    uint32_t start = millis();
    while (millis() - start < 777) {
        if (buttons_.isSelectPressed()) {
            autoSetup_ = true;
            break;
        }
    }

    // Mirror old AUTO_SETUP defaults when user holds SELECT during splash.
    if (autoSetup_) {
        static constexpr uint8_t AUTO_SETUP_ADDRESS = 0x17;   // ADDR_INSTRUMENTS
        static constexpr uint16_t AUTO_SETUP_BAUD_RATE = 10400;
        addrSelected_ = AUTO_SETUP_ADDRESS;
        baudRate_ = AUTO_SETUP_BAUD_RATE;
        kwp_.setConfig(baudRate_, addrSelected_);
    }

    dtcStore_.reset();
}

void OBDDisplay::runSetupFlow_()
{
    // Mirror the old connect() setup phase: choose SIM/ECU, baud, and address.

    // For retries, pre-fill SIM/ECU from previous state.
    int8_t userSimMode = -1; // 0 = ECU, 1 = SIM
    if (connectionAttempts_ > 0) {
        userSimMode = simulationModeActive_ ? 1 : 0;
    }

    // Always clear any previous signal / DTC state so we don't
    // carry over simulation values into a real ECU session.
    signals_.reset();
    dtcStore_.reset();

    if (!autoSetup_) {
        // 1) Connect mode: ECU vs SIM
        display_.clear();
        display_.print(0, 0, F("Connect mode"));
        display_.print(0, 1, F("<- ECU"));
        display_.print(9, 1, F("SIM ->"));

        while (userSimMode == -1) {
            int v = analogRead(A0);
            if (v < 60) {
                // RIGHT = SIM
                userSimMode = 1;
            } else if (v >= 400 && v < 600) {
                // LEFT = ECU
                userSimMode = 0;
            }
        }

        simulationModeActive_ = (userSimMode == 1);

        // 2) Baud rate selection
        uint16_t supportedBaudRates[5] = {1200, 2400, 4800, 9600, 10400};
        uint8_t baudPtr = 3; // default 9600
        uint16_t userBaud = supportedBaudRates[baudPtr];

        display_.clear();
        display_.print(0, 0, F("<--   Baud:  -->"));
        display_.print(2, 1, String("-> ") + String(userBaud), 10);

        bool pressedEnter = false;
        while (!pressedEnter) {
            int v = analogRead(A0);
            if (v < 60) {
                // RIGHT
                baudPtr = (baudPtr >= 4) ? 0 : static_cast<uint8_t>(baudPtr + 1);
                userBaud = supportedBaudRates[baudPtr];
                display_.print(2, 1, String("-> ") + String(userBaud), 10);
                delay(333);
            } else if (v >= 400 && v < 600) {
                // LEFT
                baudPtr = (baudPtr == 0) ? 4 : static_cast<uint8_t>(baudPtr - 1);
                userBaud = supportedBaudRates[baudPtr];
                display_.print(2, 1, String("-> ") + String(userBaud), 10);
                delay(333);
            } else if (v >= 600 && v < 800) {
                // SELECT = enter
                pressedEnter = true;
            }
            delay(10);
        }

        baudRate_ = userBaud;
        delay(555);

        // 3) ECU address selection: 0x01 or 0x17
        int8_t userAddr = -1; // 0 -> 0x01, 1 -> 0x17
        display_.clear();
        display_.print(0, 0, F("ECU address:"));
        display_.print(0, 1, F("<-- 01"));
        display_.print(9, 1, F("17 -->"));

        while (userAddr == -1) {
            int v = analogRead(A0);
            if (v < 60) {
                // RIGHT
                userAddr = 1;
            } else if (v >= 400 && v < 600) {
                // LEFT
                userAddr = 0;
            }
        }

        addrSelected_ = (userAddr == 0) ? 0x01 : 0x17;
    } else {
        // Auto-setup path already populated simulationModeActive_, baudRate_, addrSelected_
        // in startupAnimation_(). Nothing extra needed here.
    }

    kwp_.setConfig(baudRate_, addrSelected_);
}

void OBDDisplay::update()
{
    // Phase-based behaviour to mirror original UX.
    if (phase_ == Phase::Setup) {
        // Allow re-running the interactive setup flow after a manual
        // exit. This mirrors the original behaviour where the user
        // could change mode/baud/address again.
        runSetupFlow_();

        // After setup, go back to the explicit press-to-connect
        // prompt.
        phase_ = Phase::WaitingForConnect;
        display_.clear();
        display_.print(0, 0, F("->   ENTER   <-"));
        display_.print(0, 1, F("Press SELECT"));

        connectTimeStart_ = millis();
        displayFrameTimestamp_ = millis();
        buttonTimeoutUntil_ = 0;
        return;
    }

    if (phase_ == Phase::WaitingForConnect) {
        // Block connection attempts until user presses SELECT. Respect
        // the button timeout so that a SELECT used to exit does not
        // immediately auto-connect.
        if (millis() < buttonTimeoutUntil_ || !buttons_.isSelectPressed()) {
            // Keep showing the "Press SELECT" screen; no ECU comms yet.
            return;
        }

        // Transition to running phase and force cockpit to re-init so
        // labels are drawn immediately after leaving the PRESS SELECT
        // screen.
        phase_ = Phase::Running;
        menuState_ = Input::MenuState();
        menuState_.markMenuChanged();

        // In simulation mode, there is no real ECU to connect to; treat as
        // immediately "connected" and skip ensureConnected_().
        if (simulationModeActive_) {
            connected_ = true;
        }
    }

    // Always keep UI responsive, even when not connected to an ECU.
    bool wasConnected = connected_;
    bool nowConnected = ensureConnected_();

    // Only talk to ECU or run simulation when we have (or had) a connection.
    // If ensureConnected_() failed in ECU mode, it already showed an
    // error and returned to PRESS SELECT; in that case we must not run
    // the tripcomputer loop.
    if ((nowConnected || wasConnected || simulationModeActive_) && phase_ == Phase::Running) {
        updateKwpOrSimulation_();
        computeValues_();
    }

    handleInput_();
    updateDisplay_();
}

bool OBDDisplay::ensureConnected_()
{
    // In simulation mode, we never talk to a real ECU; treat as always connected.
    if (simulationModeActive_) {
        return true;
    }

    if (connected_) {
        return true;
    }

    // If we have no valid configuration yet, don't block the UI; behave like
    // the original sketch where menus were shown before any connection.
    if (baudRate_ == 0 || addrSelected_ == 0x00) {
        return false;
    }

    if (!kwp_.connectToEcu(simulationModeActive_, autoSetup_, baudRate_, addrSelected_)) {
        kwp_.disconnect();
        connected_ = false;

        // In ECU mode, a failed connect should behave like the old obd_connect():
        // show an error and do not start the tripcomputer loop.
        if (!simulationModeActive_) {
            display_.clear();
            display_.print(0, 0, F("ECU connect ERR"));
            display_.print(0, 1, F("Retrying..."));

            // After a short timeout, go back to the explicit
            // press-to-connect prompt and reset state so we do
            // not fall through into the tripcomputer.
            delay(ECU_TIMEOUT_MS);
            phase_ = Phase::WaitingForConnect;
            connected_ = false;
            menuState_ = Input::MenuState();
            display_.clear();
            display_.print(0, 0, F("->   ENTER   <-"));
            display_.print(0, 1, F("Press SELECT"));
        }

        return false;
    }

    connected_ = true;
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
    if (!simulationModeActive_) {
        switch (kwpMode_) {
        case Mode::Ack:
            if (!kwp_.keepAlive()) {
                kwp_.disconnect();
                connected_ = false;
            }
            break;
        case Mode::ReadGroup:
            if (!kwp_.readSensorsGroup(kwpGroup_, signals_)) {
                kwp_.disconnect();
                connected_ = false;
            }
            break;
        case Mode::ReadSensors:
        default:
            for (uint8_t g = 1; g <= 3; ++g) {
                if (!kwp_.readSensorsGroup(g, signals_)) {
                    kwp_.disconnect();
                    connected_ = false;
                    break;
                }
            }
            break;
        }
    } else {
        signals_.updateSimulation();
        delay(222);
    }
}

void OBDDisplay::computeValues_()
{
    signals_.compute(millis(), connectTimeStart_);
}

void OBDDisplay::handleInput_()
{
    uint32_t now = millis();
    if (now < buttonTimeoutUntil_) {
        return;
    }

    InputActions actions{};
    if (!buttons_.update(menuState_, actions)) {
        return;
    }

    buttonTimeoutUntil_ = now + BUTTON_TIMEOUT_MS;

    if (actions.requestReconnect) {
        // Only meaningful in real ECU mode; in SIM it just
        // resets counters but keeps us running.
        if (!simulationModeActive_) {
            kwp_.disconnect();
            connected_ = false;
            phase_ = Phase::WaitingForConnect;
            display_.clear();
            display_.print(0, 0, F("->   ENTER   <-"));
            display_.print(0, 1, F("Press SELECT"));
        }
        return;
    }

    if (actions.requestExit) {
        // Settings screen 0: Exit ECU. Match old behaviour:
        // send KWP end block, disconnect, and go back to
        // "press to connect" if we are in ECU mode.
        if (connected_ && !simulationModeActive_) {
            kwp_.exitSession();
        }
        kwp_.disconnect();
        connected_ = false;

    // After exit, go back into the setup phase so the user can
    // change SIM/ECU, baud and address again before returning to
    // the PRESS SELECT prompt.
    phase_ = Phase::Setup;
    // Also debounce the SELECT used to exit so it doesn't
    // immediately trigger actions inside the setup flow.
    buttonTimeoutUntil_ = millis() + BUTTON_TIMEOUT_MS;
        return;
    }
    if (actions.toggleKwpMode) {
        // Cycle through KWP modes: ACK -> READGROUP -> READSENSORS -> ACK ...
        switch (kwpMode_) {
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
    if (actions.invertGroupSide) {
        signals_.experimental.invertGroupSide();
        menuState_.markScreenChanged();
    }

    // Mirror old experimental group_current behaviour: keep groupCurrent in
    // sync with experimentalScreen index (1..64) and mark as updated so
    // displayMenuExperimental() repaints the group index.
    if (menuState_.currentMenu() == Display::MenuId::Experimental) {
        if (menuState_.experimentalScreen() == 0) {
            menuState_.setExperimentalScreen(1);
        }
        signals_.experimental.groupCurrent = menuState_.experimentalScreen();
        signals_.experimental.kUpdated = true;
    }

    if (actions.readDtc) {
        if (simulationModeActive_) {
            // In SIM mode, we mimic the old random-test helper and
            // fill the DTC store with synthetic values so the DTC
            // menu shows something changing.
            for (uint8_t i = 0; i < 16; ++i) {
                uint16_t code = (uint16_t)(i * 1000u);
                uint8_t status = (uint8_t)(i * 10u);
                dtcStore_.set(i, code, status);
            }
        } else {
            int8_t dtcCount = kwp_.readDtcCodes(dtcStore_);
            if (dtcCount < 0) {
                // Communication error while reading DTCs: show error,
                // disconnect and go back to press-to-connect.
                display_.clear();
                display_.print(0, 0, F("DTC read error"));
                display_.print(0, 1, F("Disconnecting..."));
                delay(1222);
                kwp_.disconnect();
                connected_ = false;
                phase_ = Phase::WaitingForConnect;
                display_.clear();
                display_.print(0, 0, F("->   ENTER   <-"));
                display_.print(0, 1, F("Press SELECT"));
            } else {
                // Success: briefly show success on second line like old code.
                display_.print(3, 1, F("<Success>"));
                delay(500);
            }
        }
    }
    if (actions.clearDtc) {
        if (simulationModeActive_) {
            // In SIM mode, just clear stored codes and do not touch ECU.
            dtcStore_.reset();
        } else {
            if (!kwp_.deleteDtcCodes()) {
                // Not supported or communication problem: show message
                // but stay in current session (like old sketch).
                display_.clear();
                display_.print(0, 0, F("DTC delete"));
                display_.print(0, 1, F("Not supported"));
                delay(1222);
            } else {
                dtcStore_.reset();
                display_.print(3, 1, F("<Success>"));
                delay(500);
            }
        }
    }
}

void OBDDisplay::updateDisplay_()
{
    uint32_t now = millis();

    // If menu or screen changed, re-init and force a full render once
    if (menuState_.consumeMenuChanged() || menuState_.consumeScreenChanged()) {
        display_.clear();
        display_.initMenu(menuState_, addrSelected_, static_cast<int>(kwpMode_));
        display_.render(menuState_, signals_, dtcStore_, addrSelected_,
                        static_cast<int>(kwpMode_), true);
    }

    // Periodic refresh like DISPLAY_FRAME_LENGTH in old sketch
    if (now >= displayFrameTimestamp_) {
        display_.render(menuState_, signals_, dtcStore_, addrSelected_,
                        static_cast<int>(kwpMode_), false);
        displayFrameTimestamp_ = now + DISPLAY_FRAME_LENGTH_MS;
    }
}

void OBDDisplay::incrementExperimentalGroup_()
{
    auto &eg = signals_.experimental;
    const uint8_t groupMax = 64;
    if (eg.groupCurrent >= groupMax) {
        eg.groupCurrent = 1;
    } else {
        eg.groupCurrent++;
    }
    eg.kUpdated = true;
}

void OBDDisplay::decrementExperimentalGroup_()
{
    auto &eg = signals_.experimental;
    const uint8_t groupMax = 64;
    if (eg.groupCurrent <= 1) {
        eg.groupCurrent = groupMax;
    } else {
        eg.groupCurrent--;
    }
    eg.kUpdated = true;
}

} // namespace obd
