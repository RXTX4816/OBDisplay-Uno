/**
 * @file Controller.cpp
 * @brief Main application controller implementation
 */

#include "Controller.h"
#include "LiquidCrystal.h"

// TODO: Add menu display and action callbacks as we port from obdisplay.cpp.old

Controller::Controller()
  : obdDisplay(nullptr)
  , menuSystem(nullptr)
  , cockpitScreen(nullptr)
  , tripScreen(nullptr)
  , experimentalScreen(nullptr)
  , debugScreen(nullptr)
  , dtcScreen(nullptr)
  , settingsScreen(nullptr)
  , exitItem(nullptr)
  , readDtcItem(nullptr)
  , clearDtcItem(nullptr)
  , connectItem(nullptr)
  , lastButtonPress(0)
{
}

Controller::~Controller()
{
  delete obdDisplay;
  delete menuSystem;
  delete cockpitScreen;
  delete tripScreen;
  delete experimentalScreen;
  delete debugScreen;
  delete dtcScreen;
  delete settingsScreen;
  delete exitItem;
  delete readDtcItem;
  delete clearDtcItem;
  delete connectItem;
}

void Controller::setup()
{
  // Initialize LCD
  static LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

  // Initialize OBD display facade
  obdDisplay = new obd::OBDDisplay(3, 2, lcd); // rx, tx
  obdDisplay->begin();

  // TODO: Initialize menu system when ready
  // setupMenus();
}

void Controller::loop()
{
  if (obdDisplay != nullptr)
  {
    obdDisplay->update();
  }

  // TODO: Extend with menu system once ported
  // handleButtons();
  // updateConnection();
  // updateSensors();
  // if (menuSystem != nullptr) { menuSystem->update(); }
}

void Controller::setupMenus()
{
  // TODO: Initialize menu system
  // This will be implemented after LCD is set up
}

void Controller::handleButtons()
{
  // TODO: Implement button handling
}

void Controller::updateConnection()
{
  // TODO: Implement connection management
  // Check if we need to connect, maintain keep-alive, etc.
}

void Controller::updateSensors()
{
  // TODO: Implement sensor reading
  // Call read_sensors() when connected
}

// Menu callbacks will be implemented as we port logic from obdisplay.cpp.old
