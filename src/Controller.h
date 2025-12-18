/**
 * @file Controller.h
 * @brief Main application controller interface
 * 
 * This class will eventually orchestrate all application logic including:
 * - Hardware initialization
 * - OBD communication
 * - UI management
 * - Button handling
 * - Display updates
 */

#pragma once

#include <Arduino.h>
#include "ui/MenuSystem.h"
#include "obd/OBDDisplay.h"

/**
 * @class Controller
 * @brief Main application controller
 * 
 * Manages the application state machine and coordinates between different
 * subsystems (OBD protocol, display, menu system, etc.)
 */
class Controller
{
public:
  /**
   * @brief Constructor
   */
  Controller();

  /**
   * @brief Destructor
   */
  ~Controller();

  /**
   * @brief Initialize the controller and all subsystems
   * 
   * Called once during Arduino setup(). Initializes:
   * - Serial communication (if DEBUG enabled)
   * - LCD display
   * - GPIO pins
   * - Initial state
   */
  void setup();

  /**
   * @brief Main application loop
   * 
   * Called repeatedly by Arduino loop(). Handles:
   * - Connection management
   * - Sensor reading
   * - Button input
   * - Display updates
   */
  void loop();

private:
  // OBD display facade
  obd::OBDDisplay* obdDisplay;

  // Menu system
  MenuSystem* menuSystem;
  
  // Menu screens
  MenuScreen* cockpitScreen;
  MenuScreen* tripScreen;
  MenuScreen* experimentalScreen;
  MenuScreen* debugScreen;
  MenuScreen* dtcScreen;
  MenuScreen* settingsScreen;
  
  // Menu items
  MenuItem* exitItem;
  MenuItem* readDtcItem;
  MenuItem* clearDtcItem;
  MenuItem* connectItem;
  
  // Button handling
  unsigned long lastButtonPress;
  static constexpr unsigned long BUTTON_DEBOUNCE_MS = 200;
  
  // Private methods
  void setupMenus();
  void handleButtons();
  void updateConnection();
  void updateSensors();
  
  // Prevent copying
  Controller(const Controller&) = delete;
  Controller& operator=(const Controller&) = delete;
};
