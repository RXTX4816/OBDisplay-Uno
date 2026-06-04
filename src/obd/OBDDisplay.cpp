// SPDX-License-Identifier: GPL-3.0-or-later
#include "OBDDisplay.h"
#include "Buzzer.h"
#include "../Config.h"
#include "../debug.h"
#include "Display/screens/CockpitScreen.h"

// AVR linker-defined heap bounds; must be at global scope for freeRam_().
extern int __heap_start;
extern int* __brkval;

namespace obd
{

using namespace Display;
using namespace KWP;
using namespace Model;
using namespace Input;

OBDDisplay::OBDDisplay(uint8_t rxPin, uint8_t txPin, ::Display& display)
    : obdSerial_(rxPin, txPin), display_(display), kwp_(obdSerial_, txPin), signals_(), dtcStore_(),
      menuState_(), buttons_(BTN_PIN_UP, BTN_PIN_DOWN, BTN_PIN_LEFT, BTN_PIN_RIGHT, BTN_PIN_MID),
      autoSetup_(false), baudRate_(0), addrSelected_(0x00), kwpMode_(Mode::ReadSensors),
      kwpModeLast_(Mode::ReadSensors), kwpModeBeforeGroup_(Mode::ReadSensors), kwpGroup_(1),
      inGroupScreen_(false), connected_(false), connectTimeStart_(0), buttonTimeoutUntil_(0)
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
    buttonTimeoutUntil_ = 0;
    reconnectAfterMs_ = millis() + RECONNECT_DELAY_MS;
}

void OBDDisplay::startupAnimation_()
{
    display_.clear();
    display_.print(0, 6, F("OBDisplay"));
    display_.print(2, 8, F(APP_VERSION));

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

void OBDDisplay::updateKwp()
{
    // Phase: Setup — run interactive setup flow, then transition to WaitingForConnect.
    if (phase_ == Phase::Setup)
    {
        runSetupFlow_();
        phase_ = Phase::WaitingForConnect;
        showWaitingScreen_();
        connectTimeStart_ = millis();
        buttonTimeoutUntil_ = 0;
        reconnectAfterMs_ = millis() + RECONNECT_DELAY_MS;
        return;
    }

    // Phase: WaitingForConnect — wait for user input or auto-reconnect timer.
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
    }

    // Phase: Running — manage ECU connection and poll sensor data.
    bool wasConnected = connected_;
    bool nowConnected = ensureConnected_();
    bool justConnected = !wasConnected && nowConnected;

    if ((nowConnected || wasConnected) && phase_ == Phase::Running)
    {
        // Skip the redundant read on the first KWP call after connection:
        // ensureConnected_() already seeded a full group-read round.
        if (!justConnected)
        {
            pollEcu_();
            displayDirty_ = true;
        }
        else
        {
            displayDirty_ = true; // ensureConnected_() seeded fresh data
        }
    }
}

void OBDDisplay::updateCompute()
{
    if (phase_ != Phase::Running)
        return;
    computeValues_();
    displayDirty_ = true;
}

void OBDDisplay::updateDisplay()
{
    handleInput_();
    updateDisplay_();
}

void OBDDisplay::pollEcu_()
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
            if (addrSelected_ == 0x01)
            {
                // ── 0x01 group rotation ─────────────────────────────────────────
                // Fast (every cycle): groups 1 + 5  → RPM/lambda/coolant-proxy + speed/load
                // Medium (every 3rd): group 4       → voltage/coolant/intake temp
                // Slow (every 6th):   group 3       → pressure/tbAngle/steeringAngle
                // Diagnostic (staggered via %90):   misfire, lambda detail, throttle sensors
                // Readiness (every 60th): group 100
                static uint8_t gr01Ctr = 0;
                ++gr01Ctr;

#define POLL01(g)                                                                                  \
    do                                                                                             \
    {                                                                                              \
        if (!kwp_.readSensorsGroup((g), signals_))                                                 \
        {                                                                                          \
            kwp_.disconnect();                                                                     \
            connected_ = false;                                                                    \
            return;                                                                                \
        }                                                                                          \
    } while (0)

                POLL01(1);
                POLL01(5);

                if (gr01Ctr % 3 == 0)
                    POLL01(4);

                if (gr01Ctr % 6 == 0)
                    POLL01(3);

                // Readiness (group 100): once per ~60 fast cycles (~2 min)
                // or immediately if requested (user navigated to readiness page)
                {
                    static uint8_t g100Ctr = 0;
                    if (requestReadiness_ || ++g100Ctr >= 60)
                    {
                        g100Ctr = 0;
                        requestReadiness_ = false;
                        kwp_.readSensorsGroup(100, signals_);
                    }
                }
#undef POLL01
            }
            else
            {
                // All other ECUs: poll groups 1–3 every cycle (unchanged)
                for (uint8_t g = 1; g <= 3; ++g)
                {
                    if (!kwp_.readSensorsGroup(g, signals_))
                    {
                        kwp_.disconnect();
                        connected_ = false;
                        break;
                    }
                }
            }
            break;
    }
}

void OBDDisplay::computeValues_()
{
    signals_.compute(millis(), connectTimeStart_, addrSelected_, fuelStartL_);
    signals_.computeWarnings(addrSelected_);
    if (signals_.warnings.hasNew)
    {
        signals_.warnings.hasNew = false;
        warningFlashUntilMs_ = millis() + 3000u;
        warningFlashPhase_ = true;
        warningFlashPage_ = 0;
        warningFlashSnapshot_ = signals_.warnings;
        beepWarning(signals_.warnings.maxLevel);
    }
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

int16_t OBDDisplay::freeRam_()
{
    int v;
    return (int16_t)((int)&v - (::__brkval == 0 ? (int)&::__heap_start : (int)::__brkval));
}

void OBDDisplay::updateDisplay_()
{
    uint32_t now = millis();

    bool menuChanged = menuState_.consumeMenuChanged() || menuState_.consumeScreenChanged();
    bool warningActive = millis() < warningFlashUntilMs_;

    // Skip rendering if nothing changed and no warning flash is in progress.
    if (!displayDirty_ && !menuChanged && !warningActive)
        return;
    displayDirty_ = false;

    display_.beginBatch();

    if (phase_ == Phase::Running)
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
                                        autoReconnect_, kwp_.ecuLinesData(), kwp_.ecuLineCount(),
                                        addrSelected_, signals_.instruments.fuelLevel);
                if (dtcStatusType_ == 3)
                {
                    if (now < dtcStatusUntil_)
                    {
                        display_.print(0, 9, F("Fuel:Savd"));
                    }
                    else
                    {
                        dtcStatusUntil_ = 0;
                        dtcStatusType_ = 0;
                    }
                }
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
                di.freeRam = freeRam_();
                display_.renderDebug(di, static_cast<int>(kwpMode_));
                break;
            }

            default:
            {
                display_.initMenu(menuState_, addrSelected_, static_cast<int>(kwpMode_));
                display_.render(menuState_, signals_, dtcStore_, addrSelected_,
                                static_cast<int>(kwpMode_), true);
                break;
            }
        }

        // Warning flash overlay: applied after menu content, interrupts all menus.
        if (warningActive)
        {
            if (warningFlashPhase_)
            {
                display_.clear();
                renderWarningFlash(display_, warningFlashSnapshot_, warningFlashPage_);
                ++warningFlashPage_;
            }
            warningFlashPhase_ = !warningFlashPhase_;
        }
        else
        {
            warningFlashPhase_ = false;
        }
    }

    display_.endBatch();
}

void OBDDisplay::incrementExperimentalGroup_()
{
    auto& eg = signals_.experimental;
    if (eg.groupCurrent >= 255)
        eg.groupCurrent = 1;
    else
        eg.groupCurrent++;
    eg.kUpdated = true;
}

void OBDDisplay::decrementExperimentalGroup_()
{
    auto& eg = signals_.experimental;
    if (eg.groupCurrent <= 1)
        eg.groupCurrent = 255;
    else
        eg.groupCurrent--;
    eg.kUpdated = true;
}

} // namespace obd
