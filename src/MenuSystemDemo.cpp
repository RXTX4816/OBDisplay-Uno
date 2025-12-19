/**
 * @file MenuSystemDemo.cpp
 * @brief Standalone demo showing MenuSystem usage
 * 
 * This file demonstrates how to use the MenuSystem class with your
 * 16x2 LCD shield. It's a complete working example you can test.
 * 
 * To use this demo:
 * 1. Comment out #include "Controller.h" in main.cpp
 * 2. Uncomment the code in this file
 * 3. In main.cpp, call demoSetup() and demoLoop() instead of controller methods
 */

#include <Arduino.h>
#include "LiquidCrystal.h"
#include "ui/MenuSystem.h"

// Button threshold macros (from your original code)
#define BUTTON_UP(x) (x > 50 && x < 150)
#define BUTTON_DOWN(x) (x > 150 && x < 350)
#define BUTTON_LEFT(x) (x > 350 && x < 550)
#define BUTTON_RIGHT(x) (x > 550 && x < 750)
#define BUTTON_SELECT(x) (x > 750 && x < 950)

// Global LCD instance (adjust pins if your shield uses different pins)
// Standard LCD Keypad Shield uses: RS=8, EN=9, D4=4, D5=5, D6=6, D7=7
static LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Menu system instance
static MenuSystem* menuSystem = nullptr;

// Demo screens
static MenuScreen* dashboardScreen = nullptr;
static MenuScreen* settingsScreen = nullptr;
static MenuScreen* aboutScreen = nullptr;

// Demo menu items
static MenuItem* brightnessItem = nullptr;
static MenuItem* contrastItem = nullptr;
static MenuItem* resetItem = nullptr;

// Demo data
static int engineRPM = 0;
static int vehicleSpeed = 0;
static int coolantTemp = 20;
static unsigned long lastUpdate = 0;

//------------------------------------------------------------------------------
// Menu Display Callbacks
//------------------------------------------------------------------------------

/**
 * Dashboard screen - shows simulated sensor data
 */
void displayDashboard(LiquidCrystal& lcd, bool forceRedraw)
{
  if (forceRedraw)
  {
    lcd.clear();
    // Draw static labels
    lcd.setCursor(4, 0);
    lcd.print(F("KMH"));
    lcd.setCursor(13, 0);
    lcd.print(F("RPM"));
    lcd.setCursor(4, 1);
    lcd.print(F("C"));
  }
  
  // Update dynamic values (simulate sensor changes)
  unsigned long now = millis();
  if (now - lastUpdate > 500)
  {
    lastUpdate = now;
    
    // Simulate changing values
    engineRPM = random(800, 3000);
    vehicleSpeed = random(0, 120);
    coolantTemp = random(80, 95);
  }
  
  // Display speed
  lcd.setCursor(0, 0);
  lcd.print(F("   ")); // Clear old value
  lcd.setCursor(0, 0);
  lcd.print(vehicleSpeed);
  
  // Display RPM
  lcd.setCursor(8, 0);
  lcd.print(F("    ")); // Clear old value
  lcd.setCursor(8, 0);
  lcd.print(engineRPM);
  
  // Display temperature
  lcd.setCursor(0, 1);
  lcd.print(F("   ")); // Clear old value
  lcd.setCursor(0, 1);
  lcd.print(coolantTemp);
}

/**
 * About screen - shows static information
 */
void displayAbout(LiquidCrystal& lcd, bool forceRedraw)
{
  if (forceRedraw)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("OBDisplay Uno"));
    lcd.setCursor(0, 1);
    lcd.print(F("MenuSystem Demo"));
  }
}

//------------------------------------------------------------------------------
// Menu Action Callbacks
//------------------------------------------------------------------------------

void onBrightness()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Brightness"));
  lcd.setCursor(0, 1);
  lcd.print(F("(Not impl.)"));
  delay(1000);
}

void onContrast()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Contrast"));
  lcd.setCursor(0, 1);
  lcd.print(F("(Not impl.)"));
  delay(1000);
}

void onReset()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Resetting..."));
  delay(1000);
  
  // Reset demo data
  engineRPM = 0;
  vehicleSpeed = 0;
  coolantTemp = 20;
}

//------------------------------------------------------------------------------
// Setup and Loop
//------------------------------------------------------------------------------

void demoSetup()
{
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("MenuSystem Demo"));
  lcd.setCursor(0, 1);
  lcd.print(F("Initializing..."));
  
  delay(1500);
  
  // Create menu system
  menuSystem = new MenuSystem(lcd, 16, 2);
  
  // Create dashboard screen (custom display with live data)
  dashboardScreen = new MenuScreen(F("Dashboard"), displayDashboard);
  menuSystem->addScreen(dashboardScreen);
  
  // Create settings menu (list of menu items)
  settingsScreen = new MenuScreen(F("Settings"), nullptr);
  brightnessItem = new MenuItem(F("Brightness"), onBrightness);
  contrastItem = new MenuItem(F("Contrast"), onContrast);
  resetItem = new MenuItem(F("Reset Data"), onReset);
  settingsScreen->addItem(brightnessItem);
  settingsScreen->addItem(contrastItem);
  settingsScreen->addItem(resetItem);
  menuSystem->addScreen(settingsScreen);
  
  // Create about screen (static display)
  aboutScreen = new MenuScreen(F("About"), displayAbout);
  menuSystem->addScreen(aboutScreen);
  
  // Show first screen
  menuSystem->showScreen(0);
}

void demoLoop()
{
  static unsigned long lastButtonPress = 0;
  const unsigned long debounceDelay = 200;
  
  unsigned long now = millis();
  if (now - lastButtonPress < debounceDelay)
  {
    // Debouncing - ignore button presses
    menuSystem->update();
    return;
  }
  
  // Read button value from analog pin A0
  int buttonValue = analogRead(0);
  
  // Handle button input
  if (BUTTON_UP(buttonValue))
  {
    menuSystem->navigateUp();
    lastButtonPress = now;
  }
  else if (BUTTON_DOWN(buttonValue))
  {
    menuSystem->navigateDown();
    lastButtonPress = now;
  }
  else if (BUTTON_LEFT(buttonValue))
  {
    menuSystem->navigateLeft();
    lastButtonPress = now;
  }
  else if (BUTTON_RIGHT(buttonValue))
  {
    menuSystem->navigateRight();
    lastButtonPress = now;
  }
  else if (BUTTON_SELECT(buttonValue))
  {
    menuSystem->select();
    lastButtonPress = now;
  }
  
  // Update menu display
  menuSystem->update();
}

//------------------------------------------------------------------------------
// How to use this demo:
//------------------------------------------------------------------------------
// 
// In main.cpp, replace:
//   #include "Controller.h"
//   Controller controller;
//   void setup() { controller.setup(); }
//   void loop() { controller.loop(); }
//
// With:
//   void demoSetup();
//   void demoLoop();
//   void setup() { demoSetup(); }
//   void loop() { demoLoop(); }
//
// Then rebuild and upload to your Arduino!
//
// Navigation:
//   - LEFT/RIGHT: Switch between screens (Dashboard, Settings, About)
//   - UP/DOWN: Navigate menu items (on Settings screen)
//   - SELECT: Activate selected menu item
//
