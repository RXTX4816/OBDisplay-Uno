// SPDX-License-Identifier: GPL-3.0-or-later
#include "OBDDisplay.h"
#include "../Config.h"
#include "../debug.h"

#include <avr/pgmspace.h>
#include <stdlib.h> // ltoa
#include <EEPROM.h>

namespace obd
{

// EEPROM layout (first 3 bytes):
//   Byte 0: magic 0xA5 (indicates initialized)
//   Byte 1: fuel level in L when last set from 0x17 (0 = never set)
//   Byte 2: boot autoconnect preset (0 = off, 1 = 0x01 @ 9600, 2 = 0x17 @ 10400)
static constexpr uint8_t kEepromMagic = 0xA5;
static constexpr uint8_t kEepromAddrMagic = 0;
static constexpr uint8_t kEepromAddrFuel = 1;
static constexpr uint8_t kEepromAddrAutoConnect = 2;

static constexpr uint8_t kPresetCount = 3;

uint8_t readEepromFuel()
{
    if (EEPROM.read(kEepromAddrMagic) != kEepromMagic)
        return 0;
    return EEPROM.read(kEepromAddrFuel);
}

void writeEepromFuel(uint8_t liters)
{
    EEPROM.update(kEepromAddrMagic, kEepromMagic);
    EEPROM.update(kEepromAddrFuel, liters);
}

// Independent of the magic byte: an erased cell (0xFF) reads as 0 = off.
uint8_t readEepromAutoConnect()
{
    uint8_t preset = EEPROM.read(kEepromAddrAutoConnect);
    return preset < kPresetCount ? preset : 0;
}

using namespace Display;
using namespace KWP;
using namespace Model;
using namespace Input;

// ── ECU address + name table (PROGMEM) ───────────────────────────────────────
static const uint8_t kEcuAddrs[] PROGMEM = {0x01, 0x03, 0x08, 0x17, 0x19, 0x46};
static const char kEcuN0[] PROGMEM = "Engine";
static const char kEcuN1[] PROGMEM = "ABS Brakes";
static const char kEcuN2[] PROGMEM = "Auto HVAC";
static const char kEcuN3[] PROGMEM = "Instrument";
static const char kEcuN4[] PROGMEM = "CAN Gatway";
static const char kEcuN5[] PROGMEM = "Cntr Conv.";
static PGM_P const kEcuNames[] PROGMEM = {kEcuN0, kEcuN1, kEcuN2, kEcuN3, kEcuN4, kEcuN5};
static constexpr uint8_t kEcuCount = 6;

static const uint16_t kBaudRates[] = {1200, 2400, 4800, 9600, 10400};
static constexpr uint8_t kBaudCount = 5;

// Connection presets; the index is the value stored at kEepromAddrAutoConnect.
// Stored as indices into kBaudRates / kEcuAddrs (index 0 = manual setup).
static const uint8_t kPresetBaudIdx[kPresetCount] PROGMEM = {3, 3, 4}; // -, 9600, 10400
static const uint8_t kPresetEcuIdx[kPresetCount] PROGMEM = {0, 0, 3};  // -, 0x01, 0x17
static const char kPresetN0[] PROGMEM = "Manual/off";
static const char kPresetN1[] PROGMEM = "0x01 9600";
static const char kPresetN2[] PROGMEM = "0x17 10400";
static PGM_P const kPresetNames[kPresetCount] PROGMEM = {kPresetN0, kPresetN1, kPresetN2};

static const char kHexDigits[] PROGMEM = "0123456789ABCDEF";

static uint8_t ecuAddrAt(uint8_t i)
{
    return pgm_read_byte(&kEcuAddrs[i]);
}
static void ecuNameAt(uint8_t i, char* buf)
{
    strncpy_P(buf, (PGM_P)pgm_read_word(&kEcuNames[i]), 10);
    buf[10] = '\0';
}

// ── KWP connect-error string table (PROGMEM) ─────────────────────────────────
static const char kErrTimeout[] PROGMEM = "Timeout";
static const char kErrComplement[] PROGMEM = "Compl.err";
static const char kErrSyncMismatch[] PROGMEM = "Bad sync";
static const char kErrSyncFail[] PROGMEM = "No sync";
static const char kErrBlocksFail[] PROGMEM = "Blk err";
static const uint8_t kErrCodes[] PROGMEM = {DBG_KWP_TIMEOUT, DBG_KWP_COMPLEMENT,
                                            DBG_KWP_SYNC_MISMATCH, DBG_KWP_SYNC_FAIL,
                                            DBG_KWP_BLOCKS_FAIL};
static PGM_P const kErrStrs[] PROGMEM = {kErrTimeout, kErrComplement, kErrSyncMismatch,
                                         kErrSyncFail, kErrBlocksFail};
static constexpr uint8_t kErrCount = 5;

// ─────────────────────────────────────────────────────────────────────────────

void OBDDisplay::drawSetupHeader_(bool showBaud, bool showAddr, bool showBack)
{
    if (showBaud)
    {
        display_.print(0, 0, F("Baud:"));
        display_.print(5, 0, (int32_t)baudRate_);
    }
    if (showAddr)
    {
        char addrBuf[11] = "Addr: 0x";
        addrBuf[8] = (char)pgm_read_byte(&kHexDigits[(addrSelected_ >> 4) & 0xF]);
        addrBuf[9] = (char)pgm_read_byte(&kHexDigits[addrSelected_ & 0xF]);
        addrBuf[10] = '\0';
        display_.print(0, 1, addrBuf);
    }
    if (showBack)
        display_.print(0, 15, F("UP:back"));
}

void OBDDisplay::showWaitingScreen_()
{
    display_.clear();
    drawSetupHeader_(true, true, false);
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

void OBDDisplay::applyPreset_(uint8_t preset)
{
    baudRate_ = kBaudRates[pgm_read_byte(&kPresetBaudIdx[preset])];
    addrSelected_ = ecuAddrAt(pgm_read_byte(&kPresetEcuIdx[preset]));
}

// Setup stages: 0 = preset, 1 = baud rate, 2 = ECU address, 3 = auto-reconnect.
// Every stage shares one loop: draw, debounce, wait for a button, act.
// LEFT/RIGHT cycle, SELECT confirms, UP goes back one stage.
void OBDDisplay::runSetupFlow_(uint8_t stage)
{
    reconnectAttempts_ = 0;
    signals_.reset();
    dtcStore_.reset();

    if (autoSetup_)
    {
        kwp_.setConfig(baudRate_, addrSelected_);
        return;
    }

    // Seed the cursors from the current selection (defaults: 9600, first ECU).
    uint8_t preset = 0;
    uint8_t baudPtr = 3;
    for (uint8_t i = 0; i < kBaudCount; ++i)
        if (kBaudRates[i] == baudRate_)
            baudPtr = i;
    uint8_t addrPtr = 0;
    for (uint8_t i = 0; i < kEcuCount; ++i)
        if (ecuAddrAt(i) == addrSelected_)
            addrPtr = i;

    // Wait for any button still held from the previous screen (SELECT on the
    // splash or Settings→Exit, UP on the waiting screen) to be released.
    while (readButtons_() != 0)
        delay(10);

    while (stage <= 3)
    {
        uint8_t* ptr = &preset;
        uint8_t count = kPresetCount;

        display_.beginBatch();
        display_.clear();
        switch (stage)
        {
            case 0:
                display_.print(0, 0, F("Preset:"));
                display_.print(0, 1, F("< Sel >"));
                display_.print(
                    0, 2,
                    reinterpret_cast<__FlashStringHelper*>(pgm_read_word(&kPresetNames[preset])));
                display_.print(0, 4, F("DN:boot"));
                display_.print(0, 5,
                               reinterpret_cast<__FlashStringHelper*>(
                                   pgm_read_word(&kPresetNames[readEepromAutoConnect()])));
                break;
            case 1:
                ptr = &baudPtr;
                count = kBaudCount;
                drawSetupHeader_(false, false, true);
                display_.print(0, 2, F("Baud:"));
                display_.print(0, 3, F("< Sel >"));
                display_.print(0, 4, (int32_t)kBaudRates[baudPtr]);
                break;
            case 2:
            {
                ptr = &addrPtr;
                count = kEcuCount;
                uint8_t a = ecuAddrAt(addrPtr);
                char buf[11];
                drawSetupHeader_(true, false, true);
                display_.print(0, 3, F("ECU Addr:"));
                display_.print(0, 4, F("< Sel >"));
                buf[0] = '0';
                buf[1] = 'x';
                buf[2] = (char)pgm_read_byte(&kHexDigits[(a >> 4) & 0xF]);
                buf[3] = (char)pgm_read_byte(&kHexDigits[a & 0xF]);
                buf[4] = '\0';
                display_.print(0, 5, buf);
                ecuNameAt(addrPtr, buf);
                display_.print(0, 6, buf);
                break;
            }
            default:
                drawSetupHeader_(true, true, true);
                display_.print(0, 4, F("AutoRcn:"));
                display_.print(0, 5, F("< N"));
                display_.print(0, 6, F("  Y >"));
                break;
        }
        display_.endBatch();

        delay(333);
        uint8_t btns;
        while ((btns = readButtons_()) == 0)
            delay(10);

        if (btns & BTN_MASK_UP)
        {
            if (stage > 0)
                --stage;
        }
        else if (stage == 3)
        {
            // Auto-reconnect: LEFT = off, RIGHT = on; either one finishes setup.
            if (btns & (BTN_MASK_LEFT | BTN_MASK_RIGHT))
            {
                autoReconnect_ = (btns & BTN_MASK_RIGHT) != 0;
                ++stage;
            }
        }
        else if (btns & BTN_MASK_RIGHT)
        {
            *ptr = (uint8_t)((*ptr + 1u) % count);
        }
        else if (btns & BTN_MASK_LEFT)
        {
            *ptr = (uint8_t)((*ptr + count - 1u) % count);
        }
        else if (btns & BTN_MASK_DOWN)
        {
            // Preset screen: persist as boot autoconnect ("Manual/off" stores 0).
            if (stage == 0)
                EEPROM.update(kEepromAddrAutoConnect, preset);
        }
        else if (btns & BTN_MASK_MID)
        {
            if (stage == 0 && preset != 0)
            {
                // A preset fills in baud + address and skips to auto-reconnect.
                baudPtr = pgm_read_byte(&kPresetBaudIdx[preset]);
                addrPtr = pgm_read_byte(&kPresetEcuIdx[preset]);
                stage = 2;
            }
            baudRate_ = kBaudRates[baudPtr];
            if (stage >= 2)
                addrSelected_ = ecuAddrAt(addrPtr);
            ++stage;
        }
    }
    delay(333);

    kwp_.setConfig(baudRate_, addrSelected_);
}

bool OBDDisplay::ensureConnected_()
{
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

    if (!kwp_.connectToEcu(autoSetup_, baudRate_, addrSelected_))
    {
        kwp_.disconnect();
        connected_ = false;

        display_.clear();
        display_.print(0, 0, F("Conn. ERR"));
        display_.print(0, 1, F("Retry..."));

        uint8_t err = kwp_.lastConnectError();
        for (uint8_t i = 0; i < kErrCount; ++i)
        {
            if (pgm_read_byte(&kErrCodes[i]) == err)
            {
                display_.print(0, 2,
                               reinterpret_cast<__FlashStringHelper*>(pgm_read_word(&kErrStrs[i])));
                break;
            }
        }
        display_.flush();

        delay(3000);

        // Set flag to skip immediate retry attempt on next frame
        lastConnectionFailed_ = true;

        return false;
    }

    connected_ = true;
    wasConnected_ = true;
    connectTimeStart_ = millis();
    // After a successful connect, always start in the cockpit menu (tripcomputer)
    // like the original sketch did.
    menuState_ = Input::MenuState(); // reset to defaults (Cockpit, screen 0)
    menuState_.markMenuChanged();
    menuState_.setCockpitMax(addrSelected_ == 0x01 ? 6u : addrSelected_ == 0x17 ? 3u : 0u);

    // Seed fuel start from EEPROM for 0x01 trip computer
    if (addrSelected_ == 0x01)
        fuelStartL_ = readEepromFuel();

    // Seed one round of data so the very first cockpit frame drawn
    // after connect is fully populated without waiting for a manual
    // screen change.
    pollEcu_();
    signals_.instruments.odometerStart = signals_.instruments.odometer;
    signals_.instruments.fuelLevelStart = signals_.instruments.fuelLevel;
    signals_.instruments.fuelLevelSmoothX8 = (uint16_t)signals_.instruments.fuelLevel * 8u;
    computeValues_();
    return true;
}

} // namespace obd
