#include "MenuState.h"

namespace obd {
namespace Input {

MenuState::MenuState()
    : currentMenu_(Display::MenuId::Cockpit)
    , cockpitScreen_(0)
    , cockpitScreenMax_(4)
    , experimentalScreen_(0)
    , experimentalScreenMax_(64)
    , debugScreen_(0)
    , debugScreenMax_(4)
    , dtcScreen_(0)
    , dtcScreenMax_(9)
    , settingsScreen_(0)
    , settingsScreenMax_(10)
    , menuChanged_(false)
    , screenChanged_(false)
{
}

void MenuState::nextMenu()
{
    uint8_t val = static_cast<uint8_t>(currentMenu_);
    val = (val + 1) % 5;
    currentMenu_ = static_cast<Display::MenuId>(val);
    menuChanged_ = true;
}

void MenuState::prevMenu()
{
    uint8_t val = static_cast<uint8_t>(currentMenu_);
    if (val == 0) val = 4; else --val;
    currentMenu_ = static_cast<Display::MenuId>(val);
    menuChanged_ = true;
}

void MenuState::nextCockpitScreen() {
    if (++cockpitScreen_ > cockpitScreenMax_) cockpitScreen_ = 0;
    screenChanged_ = true;
}

void MenuState::prevCockpitScreen() {
    if (cockpitScreen_ == 0) cockpitScreen_ = cockpitScreenMax_; else --cockpitScreen_;
    screenChanged_ = true;
}

void MenuState::nextExperimentalScreen() {
    if (++experimentalScreen_ > experimentalScreenMax_) experimentalScreen_ = 0;
    screenChanged_ = true;
}

void MenuState::prevExperimentalScreen() {
    if (experimentalScreen_ == 0) experimentalScreen_ = experimentalScreenMax_; else --experimentalScreen_;
    screenChanged_ = true;
}

void MenuState::nextDebugScreen() {
    if (++debugScreen_ > debugScreenMax_) debugScreen_ = 0;
    screenChanged_ = true;
}

void MenuState::prevDebugScreen() {
    if (debugScreen_ == 0) debugScreen_ = debugScreenMax_; else --debugScreen_;
    screenChanged_ = true;
}

void MenuState::nextDtcScreen() {
    if (++dtcScreen_ > dtcScreenMax_) dtcScreen_ = 0;
    screenChanged_ = true;
}

void MenuState::prevDtcScreen() {
    if (dtcScreen_ == 0) dtcScreen_ = dtcScreenMax_; else --dtcScreen_;
    screenChanged_ = true;
}

void MenuState::nextSettingsScreen() {
    if (++settingsScreen_ > settingsScreenMax_) settingsScreen_ = 0;
    screenChanged_ = true;
}

void MenuState::prevSettingsScreen() {
    if (settingsScreen_ == 0) settingsScreen_ = settingsScreenMax_; else --settingsScreen_;
    screenChanged_ = true;
}

bool MenuState::consumeMenuChanged()
{
    bool tmp = menuChanged_;
    menuChanged_ = false;
    return tmp;
}

bool MenuState::consumeScreenChanged()
{
    bool tmp = screenChanged_;
    screenChanged_ = false;
    return tmp;
}

void MenuState::markMenuChanged()
{
    menuChanged_ = true;
}

void MenuState::markScreenChanged()
{
    screenChanged_ = true;
}

} // namespace Input
} // namespace obd
