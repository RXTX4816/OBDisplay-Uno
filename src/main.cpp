/**
 * @file main.cpp
 * @brief Main entry point for OBDisplay Arduino application
 * 
 * This file contains the Arduino setup() and loop() functions that orchestrate
 * the application. The actual business logic is delegated to the Controller.
 */

#include <Arduino.h>
#include "Controller.h"

// Global controller instance
Controller controller;

/**
 * @brief Arduino setup function - runs once at startup
 */
void setup()
{
	controller.setup();
}

/**
 * @brief Arduino loop function - runs continuously
 */
void loop()
{
	controller.loop();
}
