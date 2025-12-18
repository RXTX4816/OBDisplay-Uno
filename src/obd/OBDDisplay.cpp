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

    dtcStore_.reset();
}

void OBDDisplay::update()
{
    if (!ensureConnected_()) {
        return;
    }

    updateKwpOrSimulation_();
    computeValues_();
    handleInput_();
    updateDisplay_();
}

bool OBDDisplay::ensureConnected_()
{
    if (connected_) {
        return true;
    }

    if (!kwp_.connectToEcu(simulationModeActive_, autoSetup_, baudRate_, addrSelected_)) {
        kwp_.disconnect();
        connected_ = false;
        return false;
    }

    connected_ = true;
    connectTimeStart_ = millis();
    menuState_.markMenuChanged();
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
        kwp_.disconnect();
        connected_ = false;
        return;
    }

    if (actions.requestExit) {
        kwp_.exitSession();
        kwp_.disconnect();
        connected_ = false;
        return;
    }
    if (actions.invertGroupSide) {
        signals_.experimental.invertGroupSide();
        menuState_.markScreenChanged();
    }

    if (actions.readDtc) {
        int8_t dtcCount = kwp_.readDtcCodes(dtcStore_);
        (void)dtcCount; // DisplayManager will show success/error using updated store later
    }
    if (actions.clearDtc) {
        if (kwp_.deleteDtcCodes()) {
            dtcStore_.reset();
        }
    }
}

void OBDDisplay::updateDisplay_()
{
    uint32_t now = millis();
    if (menuState_.consumeMenuChanged() || menuState_.consumeScreenChanged()) {
    display_.clear();
    display_.initMenu(menuState_, addrSelected_, static_cast<int>(kwpMode_));
    display_.render(menuState_, signals_, dtcStore_, addrSelected_, static_cast<int>(kwpMode_), true);
    }

    if (now >= displayFrameTimestamp_) {
    display_.render(menuState_, signals_, dtcStore_, addrSelected_, static_cast<int>(kwpMode_), false);
        displayFrameTimestamp_ = now + DISPLAY_FRAME_LENGTH_MS;
    }
}

} // namespace obd
