// SPDX-License-Identifier: GPL-3.0-or-later
#include "OBDDisplay.h"
#include "../Config.h"
#include "../debug.h"

namespace obd
{

using namespace Display;
using namespace KWP;
using namespace Model;
using namespace Input;

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

    // LEFT/RIGHT navigate menus unless jump mode has captured them.
    bool inJumpMode = (menuState_.currentMenu() == Display::MenuId::Experimental &&
                       signals_.experimental.grpJumpActive);
    if (!inJumpMode && (btns & BTN_MASK_RIGHT))
    {
        menuState_.nextMenu();
        dtcShowActive_ = false; // leave DTC show mode when navigating away
        any = true;
    }
    else if (!inJumpMode && (btns & BTN_MASK_LEFT))
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
            case MenuId::Debug:
            {
                MenuId mid = menuState_.currentMenu();
                if (btns & BTN_MASK_UP)
                {
                    menuState_.nextScreen(mid);
                    // When landing on readiness page (screen 1, 0x01), trigger immediate poll
                    if (mid == MenuId::Cockpit && addrSelected_ == 0x01 &&
                        menuState_.screen(MenuId::Cockpit) == 1)
                        requestReadiness_ = true;
                    any = true;
                }
                else if (btns & BTN_MASK_DOWN)
                {
                    menuState_.prevScreen(mid);
                    if (mid == MenuId::Cockpit && addrSelected_ == 0x01 &&
                        menuState_.screen(MenuId::Cockpit) == 1)
                        requestReadiness_ = true;
                    any = true;
                }
                break;
            }
            case MenuId::Experimental:
            {
                auto& eg = signals_.experimental;
                if (eg.grpJumpActive)
                {
                    if (btns & BTN_MASK_UP)
                    {
                        uint8_t idx = eg.grpJumpCursor;
                        uint8_t* d = eg.grpJumpDigits;
                        // Hundreds capped at 2; tens/units capped at 5 when constrained by 255.
                        uint8_t maxDig = (idx == 0)                             ? 2u
                                         : (idx == 1 && d[0] == 2)              ? 5u
                                         : (idx == 2 && d[0] == 2 && d[1] == 5) ? 5u
                                                                                : 9u;
                        if (++d[idx] > maxDig)
                            d[idx] = 0;
                        // Clamp lower digits if entering the constrained zone.
                        if (idx == 0 && d[0] == 2)
                        {
                            if (d[1] > 5)
                                d[1] = 5;
                            if (d[1] == 5 && d[2] > 5)
                                d[2] = 5;
                        }
                        else if (idx == 1 && d[0] == 2 && d[1] == 5 && d[2] > 5)
                        {
                            d[2] = 5;
                        }
                        menuState_.markScreenChanged();
                        any = true;
                    }
                    else if (btns & BTN_MASK_DOWN)
                    {
                        uint8_t idx = eg.grpJumpCursor;
                        uint8_t* d = eg.grpJumpDigits;
                        uint8_t maxDig = (idx == 0)                             ? 2u
                                         : (idx == 1 && d[0] == 2)              ? 5u
                                         : (idx == 2 && d[0] == 2 && d[1] == 5) ? 5u
                                                                                : 9u;
                        d[idx] = (d[idx] == 0) ? maxDig : d[idx] - 1u;
                        menuState_.markScreenChanged();
                        any = true;
                    }
                    else if (btns & BTN_MASK_LEFT)
                    {
                        if (eg.grpJumpCursor > 0)
                            eg.grpJumpCursor--;
                        menuState_.markScreenChanged();
                        any = true;
                    }
                    else if (btns & BTN_MASK_RIGHT)
                    {
                        if (eg.grpJumpCursor < 2)
                            eg.grpJumpCursor++;
                        menuState_.markScreenChanged();
                        any = true;
                    }
                    else if (btns & BTN_MASK_MID)
                    {
                        // Digit constraints guarantee value fits in uint8 (1-255).
                        uint8_t target = (uint8_t)(eg.grpJumpDigits[0] * 100u +
                                                   eg.grpJumpDigits[1] * 10u + eg.grpJumpDigits[2]);
                        if (target < 1u)
                            target = 1u;
                        menuState_.setScreen(Display::MenuId::Experimental, target);
                        eg.grpJumpActive = false;
                        menuState_.markScreenChanged();
                        any = true;
                    }
                }
                else
                {
                    if (btns & BTN_MASK_UP)
                    {
                        menuState_.nextScreen(MenuId::Experimental);
                        any = true;
                    }
                    else if (btns & BTN_MASK_DOWN)
                    {
                        menuState_.prevScreen(MenuId::Experimental);
                        any = true;
                    }
                    else if (btns & BTN_MASK_MID)
                    {
                        uint8_t cur = eg.groupCurrent;
                        eg.grpJumpDigits[0] = cur / 100;
                        eg.grpJumpDigits[1] = (cur / 10) % 10;
                        eg.grpJumpDigits[2] = cur % 10;
                        eg.grpJumpCursor = 0;
                        eg.grpJumpActive = true;
                        menuState_.markScreenChanged();
                        any = true;
                    }
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
            {
                // 4 items on 0x17 (adds Fuel EEPROM), 3 items on other ECUs
                const uint8_t settingsItemCount = (addrSelected_ == 0x17) ? 4u : 3u;
                const bool onFuelItem = (settingsMenuCursor_ == 3 && addrSelected_ == 0x17);

                if (btns & BTN_MASK_UP)
                {
                    settingsMenuCursor_ =
                        (settingsMenuCursor_ + settingsItemCount - 1u) % settingsItemCount;
                    menuState_.markScreenChanged();
                    any = true;
                }
                else if (btns & BTN_MASK_DOWN)
                {
                    settingsMenuCursor_ = (settingsMenuCursor_ + 1u) % settingsItemCount;
                    menuState_.markScreenChanged();
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
                    else if (settingsMenuCursor_ == 2)
                    {
                        autoReconnect_ = !autoReconnect_;
                        if (!autoReconnect_)
                            reconnectAttempts_ = 0;
                        menuState_.markScreenChanged();
                        any = true;
                    }
                    else if (onFuelItem)
                    {
                        // Save current 0x17 fuel sensor reading to EEPROM
                        writeEepromFuel(signals_.instruments.fuelLevel);
                        dtcStatusUntil_ = millis() + 2000u;
                        dtcStatusType_ = 3; // fuel saved confirmation
                        menuState_.markScreenChanged();
                        any = true;
                    }
                }
                break;
            }
        }
    }

    if (!any)
    {
        return;
    }

    if (actions.requestReconnect)
    {
        kwp_.disconnect();
        connected_ = false;
        wasConnected_ = false;
        phase_ = Phase::WaitingForConnect;
        showWaitingScreen_();
        return;
    }

    if (actions.requestExit)
    {
        if (connected_)
            kwp_.exitSession();
        kwp_.disconnect();
        connected_ = false;
        wasConnected_ = false;
        // Clear autoSetup so the next runSetupFlow_() runs the full interactive
        // stages instead of returning early — user wants to reconfigure.
        autoSetup_ = false;
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
            signals_.experimental.grpJumpActive = false;
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
    if (actions.clearDtc)
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

} // namespace obd
