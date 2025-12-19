/**
 * @file MenuSystem.h
 * @brief Lightweight menu system for 16x2 LCD displays
 * 
 * A minimal menu framework (~500 bytes) designed for memory-constrained
 * Arduino projects. Provides hierarchical menu navigation with callbacks.
 */

#pragma once

#include <Arduino.h>
#include <LiquidCrystal.h>

// Forward declarations
class MenuItem;
class MenuScreen;

/**
 * @brief Function pointer types for menu callbacks
 */
typedef void (*MenuActionCallback)();
typedef void (*MenuDisplayCallback)(LiquidCrystal& lcd, bool forceRedraw);

/**
 * @brief Single menu item that can be selected and activated
 */
class MenuItem
{
public:
  /**
   * @brief Construct a menu item
   * @param label Menu item label (stored in PROGMEM recommended)
   * @param action Callback when item is selected
   */
  MenuItem(const __FlashStringHelper* label, MenuActionCallback action = nullptr);
  
  const __FlashStringHelper* getLabel() const { return label_; }
  void activate() const;
  
private:
  const __FlashStringHelper* label_;
  MenuActionCallback action_;
};

/**
 * @brief A screen/page containing multiple menu items or custom display
 */
class MenuScreen
{
public:
  /**
   * @brief Construct a menu screen
   * @param title Screen title (stored in PROGMEM recommended)
   * @param displayCallback Custom display function (optional)
   */
  MenuScreen(const __FlashStringHelper* title, MenuDisplayCallback displayCallback = nullptr);
  
  /**
   * @brief Add a menu item to this screen
   * @param item Pointer to menu item (must remain valid)
   * @return true if added, false if screen is full
   */
  bool addItem(MenuItem* item);
  
  /**
   * @brief Get the title of this screen
   */
  const __FlashStringHelper* getTitle() const { return title_; }
  
  /**
   * @brief Get number of items in this screen
   */
  uint8_t getItemCount() const { return itemCount_; }
  
  /**
   * @brief Get item at specific index
   */
  MenuItem* getItem(uint8_t index) const;
  
  /**
   * @brief Check if this screen has custom display callback
   */
  bool hasCustomDisplay() const { return displayCallback_ != nullptr; }
  
  /**
   * @brief Execute custom display callback
   */
  void display(LiquidCrystal& lcd, bool forceRedraw) const;
  
private:
  static constexpr uint8_t MAX_ITEMS = 8;
  
  const __FlashStringHelper* title_;
  MenuItem* items_[MAX_ITEMS];
  uint8_t itemCount_;
  MenuDisplayCallback displayCallback_;
};

/**
 * @brief Main menu system controller
 * 
 * Manages navigation between screens, handles button input,
 * and renders menus on LCD display.
 */
class MenuSystem
{
public:
  /**
   * @brief Construct menu system
   * @param lcd Reference to LiquidCrystal display
   * @param cols Number of LCD columns (typically 16 or 20)
   * @param rows Number of LCD rows (typically 2 or 4)
   */
  MenuSystem(LiquidCrystal& lcd, uint8_t cols = 16, uint8_t rows = 2);
  
  /**
   * @brief Add a screen to the menu system
   * @param screen Pointer to screen (must remain valid)
   * @return Index of added screen, or 255 if failed
   */
  uint8_t addScreen(MenuScreen* screen);
  
  /**
   * @brief Navigate to a specific screen by index
   * @param screenIndex Index of screen to show
   */
  void showScreen(uint8_t screenIndex);
  
  /**
   * @brief Navigate up in current menu
   */
  void navigateUp();
  
  /**
   * @brief Navigate down in current menu
   */
  void navigateDown();
  
  /**
   * @brief Navigate left (back/previous screen)
   */
  void navigateLeft();
  
  /**
   * @brief Navigate right (next screen)
   */
  void navigateRight();
  
  /**
   * @brief Select/activate current menu item
   */
  void select();
  
  /**
   * @brief Update display (call in loop)
   * @param forceRedraw Force complete redraw
   */
  void update(bool forceRedraw = false);
  
  /**
   * @brief Get current screen index
   */
  uint8_t getCurrentScreen() const { return currentScreenIndex_; }
  
  /**
   * @brief Get current cursor position
   */
  uint8_t getCursorPosition() const { return cursorPosition_; }
  
  /**
   * @brief Mark display as needing redraw
   */
  void setNeedsRedraw() { needsRedraw_ = true; }
  
private:
  static constexpr uint8_t MAX_SCREENS = 10;
  
  void drawScreen(bool forceRedraw);
  void drawMenuItem(uint8_t row, MenuItem* item, bool selected);
  void clearLine(uint8_t row);
  
  LiquidCrystal& lcd_;
  uint8_t cols_;
  uint8_t rows_;
  
  MenuScreen* screens_[MAX_SCREENS];
  uint8_t screenCount_;
  uint8_t currentScreenIndex_;
  uint8_t cursorPosition_;
  uint8_t scrollOffset_;
  bool needsRedraw_;
};
