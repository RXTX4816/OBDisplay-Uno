/**
 * @file MenuExample.h
 * @brief Example of how to use MenuSystem with your OBD display
 * 
 * This shows how to refactor your existing menu code to use the
 * lightweight MenuSystem class.
 */

#pragma once

#include "ui/MenuSystem.h"

// Example: How to create menu screens for your application

//------------------------------------------------------------------------------
// Example 1: Simple Menu Items (like Settings, Exit)
//------------------------------------------------------------------------------

// Define menu item callbacks
void onExitECU()
{
  // Your kwp_exit() and disconnect() logic here
  // kwp_exit();
  // disconnect();
}

void onReadDTC()
{
  // Your read_DTC_codes() logic here
  // int8_t codes = read_DTC_codes();
}

void onClearDTC()
{
  // Your delete_DTC_codes() logic here
  // delete_DTC_codes();
}

// Create menu items (use F() macro to store strings in PROGMEM)
MenuItem exitItem(F("Exit ECU"), onExitECU);
MenuItem readDtcItem(F("Read DTC"), onReadDTC);
MenuItem clearDtcItem(F("Clear DTC"), onClearDTC);

//------------------------------------------------------------------------------
// Example 2: Custom Display Screen (like Cockpit with Live Data)
//------------------------------------------------------------------------------

// Custom display callback for cockpit screen
void displayCockpit(LiquidCrystal& lcd, bool forceRedraw)
{
  // Access your global sensor variables
  // extern uint16_t vehicle_speed;
  // extern uint16_t engine_rpm;
  // extern uint8_t coolant_temp;
  
  if (forceRedraw)
  {
    lcd.clear();
    // Display static labels
    lcd.setCursor(4, 0);
    lcd.print(F("KMH"));
    lcd.setCursor(13, 0);
    lcd.print(F("RPM"));
    lcd.setCursor(3, 1);
    lcd.print(F("C"));
  }
  
  // Update dynamic values (only if changed - add your logic)
  // lcd.setCursor(0, 0);
  // lcd.print(vehicle_speed);
  
  // lcd.setCursor(8, 0);
  // lcd.print(engine_rpm);
  
  // lcd.setCursor(0, 1);
  // lcd.print(coolant_temp);
}

// Custom display for trip computer
void displayTripComputer(LiquidCrystal& lcd, bool forceRedraw)
{
  // extern uint32_t odometer;
  // extern float fuel_per_100km;
  
  if (forceRedraw)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Trip:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Fuel:"));
  }
  
  // Update values
  // lcd.setCursor(6, 0);
  // lcd.print(odometer);
  // lcd.print(F("km"));
  
  // lcd.setCursor(6, 1);
  // lcd.print(fuel_per_100km, 1);
  // lcd.print(F("L/100"));
}

//------------------------------------------------------------------------------
// Example 3: How to Set Up Menus in Controller
//------------------------------------------------------------------------------

/*
// In your Controller class:

class Controller
{
private:
  LiquidCrystal lcd;
  MenuSystem* menuSystem;
  
  // Define screens
  MenuScreen cockpitScreen;
  MenuScreen tripScreen;
  MenuScreen dtcScreen;
  MenuScreen settingsScreen;
  
  void setupMenus()
  {
    // Initialize menu system
    menuSystem = new MenuSystem(lcd, 16, 2);
    
    // Create cockpit screen (custom display)
    cockpitScreen = MenuScreen(F("Cockpit"), displayCockpit);
    menuSystem->addScreen(&cockpitScreen);
    
    // Create trip computer screen (custom display)
    tripScreen = MenuScreen(F("Trip Comp"), displayTripComputer);
    menuSystem->addScreen(&tripScreen);
    
    // Create DTC menu screen (menu items)
    dtcScreen = MenuScreen(F("Diagnostics"), nullptr);
    dtcScreen.addItem(&readDtcItem);
    dtcScreen.addItem(&clearDtcItem);
    menuSystem->addScreen(&dtcScreen);
    
    // Create settings screen (menu items)
    settingsScreen = MenuScreen(F("Settings"), nullptr);
    settingsScreen.addItem(&exitItem);
    menuSystem->addScreen(&settingsScreen);
    
    // Show first screen
    menuSystem->showScreen(0);
  }
  
public:
  void loop()
  {
    // Handle button input
    int buttonValue = analogRead(0);
    
    if (BUTTON_UP(buttonValue))
    {
      menuSystem->navigateUp();
      delay(200); // Debounce
    }
    else if (BUTTON_DOWN(buttonValue))
    {
      menuSystem->navigateDown();
      delay(200);
    }
    else if (BUTTON_LEFT(buttonValue))
    {
      menuSystem->navigateLeft();
      delay(200);
    }
    else if (BUTTON_RIGHT(buttonValue))
    {
      menuSystem->navigateRight();
      delay(200);
    }
    else if (BUTTON_SELECT(buttonValue))
    {
      menuSystem->select();
      delay(200);
    }
    
    // Update display
    menuSystem->update();
  }
};
*/

//------------------------------------------------------------------------------
// Benefits of This Approach:
//------------------------------------------------------------------------------

// 1. No library dependency (just ~500 bytes of your own code)
// 2. Clean separation of concerns
// 3. Easy to add new screens/items
// 4. Custom display callbacks for live data screens
// 5. PROGMEM support built-in (saves RAM)
// 6. Function pointers instead of giant switch statements

//------------------------------------------------------------------------------
// Migration Strategy from Your Old Code:
//------------------------------------------------------------------------------

// Your old code:
//   void display_menu_cockpit() { /* 50 lines */ }
//   void init_menu_cockpit() { /* 20 lines */ }
//   switch(menu) {
//     case 0: init_menu_cockpit(); break;
//     ...
//   }
//
// New code:
//   void displayCockpit(LiquidCrystal& lcd, bool forceRedraw) { /* clean logic */ }
//   menuSystem->addScreen(new MenuScreen(F("Cockpit"), displayCockpit));
//   menuSystem->update(); // Automatic!
