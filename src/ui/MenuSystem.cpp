/**
 * @file MenuSystem.cpp
 * @brief Lightweight menu system implementation
 */

#include "MenuSystem.h"

//------------------------------------------------------------------------------
// MenuItem Implementation
//------------------------------------------------------------------------------

MenuItem::MenuItem(const __FlashStringHelper* label, MenuActionCallback action)
  : label_(label), action_(action)
{
}

void MenuItem::activate() const
{
  if (action_)
  {
    action_();
  }
}

//------------------------------------------------------------------------------
// MenuScreen Implementation
//------------------------------------------------------------------------------

MenuScreen::MenuScreen(const __FlashStringHelper* title, MenuDisplayCallback displayCallback)
  : title_(title), itemCount_(0), displayCallback_(displayCallback)
{
  for (uint8_t i = 0; i < MAX_ITEMS; i++)
  {
    items_[i] = nullptr;
  }
}

bool MenuScreen::addItem(MenuItem* item)
{
  if (itemCount_ >= MAX_ITEMS || !item)
  {
    return false;
  }
  
  items_[itemCount_++] = item;
  return true;
}

MenuItem* MenuScreen::getItem(uint8_t index) const
{
  if (index >= itemCount_)
  {
    return nullptr;
  }
  return items_[index];
}

void MenuScreen::display(LiquidCrystal& lcd, bool forceRedraw) const
{
  if (displayCallback_)
  {
    displayCallback_(lcd, forceRedraw);
  }
}

//------------------------------------------------------------------------------
// MenuSystem Implementation
//------------------------------------------------------------------------------

MenuSystem::MenuSystem(LiquidCrystal& lcd, uint8_t cols, uint8_t rows)
  : lcd_(lcd), cols_(cols), rows_(rows), screenCount_(0),
    currentScreenIndex_(0), cursorPosition_(0), scrollOffset_(0), needsRedraw_(true)
{
  for (uint8_t i = 0; i < MAX_SCREENS; i++)
  {
    screens_[i] = nullptr;
  }
}

uint8_t MenuSystem::addScreen(MenuScreen* screen)
{
  if (screenCount_ >= MAX_SCREENS || !screen)
  {
    return 255;
  }
  
  screens_[screenCount_] = screen;
  return screenCount_++;
}

void MenuSystem::showScreen(uint8_t screenIndex)
{
  if (screenIndex >= screenCount_)
  {
    return;
  }
  
  currentScreenIndex_ = screenIndex;
  cursorPosition_ = 0;
  scrollOffset_ = 0;
  needsRedraw_ = true;
}

void MenuSystem::navigateUp()
{
  MenuScreen* currentScreen = screens_[currentScreenIndex_];
  if (!currentScreen || currentScreen->hasCustomDisplay())
  {
    return; // No navigation in custom display screens
  }
  
  if (cursorPosition_ > 0)
  {
    cursorPosition_--;
    if (cursorPosition_ < scrollOffset_)
    {
      scrollOffset_ = cursorPosition_;
    }
    needsRedraw_ = true;
  }
}

void MenuSystem::navigateDown()
{
  MenuScreen* currentScreen = screens_[currentScreenIndex_];
  if (!currentScreen || currentScreen->hasCustomDisplay())
  {
    return;
  }
  
  uint8_t itemCount = currentScreen->getItemCount();
  if (cursorPosition_ < itemCount - 1)
  {
    cursorPosition_++;
    uint8_t maxVisible = rows_;
    if (cursorPosition_ >= scrollOffset_ + maxVisible)
    {
      scrollOffset_ = cursorPosition_ - maxVisible + 1;
    }
    needsRedraw_ = true;
  }
}

void MenuSystem::navigateLeft()
{
  // Navigate to previous screen (wrap around)
  if (currentScreenIndex_ > 0)
  {
    showScreen(currentScreenIndex_ - 1);
  }
  else
  {
    showScreen(screenCount_ - 1);
  }
}

void MenuSystem::navigateRight()
{
  // Navigate to next screen (wrap around)
  if (currentScreenIndex_ < screenCount_ - 1)
  {
    showScreen(currentScreenIndex_ + 1);
  }
  else
  {
    showScreen(0);
  }
}

void MenuSystem::select()
{
  MenuScreen* currentScreen = screens_[currentScreenIndex_];
  if (!currentScreen || currentScreen->hasCustomDisplay())
  {
    return;
  }
  
  MenuItem* item = currentScreen->getItem(cursorPosition_);
  if (item)
  {
    item->activate();
    needsRedraw_ = true;
  }
}

void MenuSystem::update(bool forceRedraw)
{
  if (needsRedraw_ || forceRedraw)
  {
    drawScreen(forceRedraw);
    needsRedraw_ = false;
  }
}

void MenuSystem::drawScreen(bool forceRedraw)
{
  MenuScreen* currentScreen = screens_[currentScreenIndex_];
  if (!currentScreen)
  {
    return;
  }
  
  if (forceRedraw)
  {
    lcd_.clear();
  }
  
  // Handle custom display screens
  if (currentScreen->hasCustomDisplay())
  {
    currentScreen->display(lcd_, forceRedraw);
    return;
  }
  
  // Draw menu items
  uint8_t itemCount = currentScreen->getItemCount();
  uint8_t visibleItems = rows_;
  
  for (uint8_t row = 0; row < visibleItems; row++)
  {
    uint8_t itemIndex = scrollOffset_ + row;
    
    if (itemIndex < itemCount)
    {
      MenuItem* item = currentScreen->getItem(itemIndex);
      bool selected = (itemIndex == cursorPosition_);
      drawMenuItem(row, item, selected);
    }
    else
    {
      clearLine(row);
    }
  }
}

void MenuSystem::drawMenuItem(uint8_t row, MenuItem* item, bool selected)
{
  lcd_.setCursor(0, row);
  
  // Draw selection indicator
  lcd_.print(selected ? F(">") : F(" "));
  
  // Draw item label
  if (item)
  {
    lcd_.print(item->getLabel());
  }
  
  // Clear rest of line
  uint8_t labelLen = 1; // Account for selection indicator
  if (item)
  {
    // Approximate label length (FlashStringHelper doesn't provide length easily)
    // We'll just pad with spaces
    for (uint8_t i = labelLen; i < cols_; i++)
    {
      lcd_.print(F(" "));
    }
  }
}

void MenuSystem::clearLine(uint8_t row)
{
  lcd_.setCursor(0, row);
  for (uint8_t i = 0; i < cols_; i++)
  {
    lcd_.print(F(" "));
  }
}
