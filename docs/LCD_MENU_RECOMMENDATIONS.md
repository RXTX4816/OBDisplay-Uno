# LCD Library & Menu System Recommendations

## Your Current Setup

### LiquidCrystal Library Analysis
You're using a **custom/bundled version** of LiquidCrystal in `src/LiquidCrystal.cpp`. This is based on the standard Arduino LiquidCrystal library.

**✅ Pros:**
- Simple, lightweight, well-understood
- Direct control over LCD
- Works with parallel interface (4-bit or 8-bit mode)
- No external dependencies

**❌ Cons:**
- No built-in menu system
- No button debouncing
- No abstractions for common UI patterns
- You have to manually manage cursor positioning and screen updates
- Code becomes messy quickly with multiple screens/menus

**Verdict:** It's **adequate but basic**. Fine for simple displays, but for a trip computer with menus, you're reinventing the wheel.

---

## Recommended Upgrade Path

### Option 1: **LcdMenu by forntoh** (⭐ RECOMMENDED)
```ini
lib_deps = forntoh/LcdMenu@^5.12.1
```

**Why This is Best for Your Project:**
- ✅ **213 GitHub stars**, actively maintained (last update June 2025!)
- ✅ **Works with your existing LiquidCrystal setup** (or I2C variants)
- ✅ Built-in support for **button navigation** (up/down/back/enter)
- ✅ **Hierarchical menus** - perfect for trip computer sections
- ✅ **24 examples** including simple button navigation
- ✅ Modern C++ API with callbacks
- ✅ Good documentation: https://lcdmenu.forntoh.dev
- ✅ Lightweight: 46KB (compiles to much less)

**Features:**
- Menu items with callbacks
- Editable numeric/string items
- Navigation history (back button support)
- Screen transitions
- Value editing modes
- Works with 16x2, 20x4, etc.

**Example Structure:**
```cpp
#include <LcdMenu.h>

LcdMenu menu(lcd);  // Pass your existing lcd object

// Define menu items
MenuItem speedItem("Speed", []() { 
  return String(vehicle_speed) + " km/h"; 
});

MenuItem rpmItem("RPM", []() { 
  return String(engine_rpm); 
});

// Add to menu
menu.add(&speedItem);
menu.add(&rpmItem);

// In loop()
menu.process(buttonPressed);  // Handles navigation
```

---

### Option 2: **ArduinoMenu by neu-rah**
```ini
lib_deps = neu-rah/ArduinoMenu library@^4.21.5
```

**When to Use This Instead:**
- ✅ **951 GitHub stars** - very mature
- ✅ More powerful, supports **many input methods** (encoder, keypad, serial)
- ✅ **51 examples** - comprehensive
- ✅ Advanced features: submenus, value ranges, custom widgets

**Cons:**
- ⚠️ More complex API (steeper learning curve)
- ⚠️ Larger footprint (912KB library, though compiles smaller)
- ⚠️ Might be overkill for 16x2 display

---

### Option 3: **LiquidMenu by vase7u**
```ini
lib_deps = vase7u/LiquidMenu@^1.6.0
```

**Characteristics:**
- Simple wrapper around LiquidCrystal
- Good for static menus
- Less actively maintained (last update 2021)
- Simpler than ArduinoMenu but less features than LcdMenu

---

## Button Shield Considerations

Since you mentioned you have a **button shield**, likely one of these:

### Common Arduino LCD Shields:
1. **DFRobot LCD Keypad Shield** - 5 buttons on analog pin A0
2. **SainSmart LCD Keypad Shield** - Similar design
3. **Generic 1602 LCD Shield** - Buttons on A0

Your current code already handles this (see `BUTTON_*` macros in old obdisplay.cpp):
```cpp
#define BUTTON_RIGHT(in) (in < 60)
#define BUTTON_UP(in) (in >= 60 && in < 200)
#define BUTTON_DOWN(in) (in >= 200 && in < 400)
#define BUTTON_LEFT(in) (in >= 400 && in < 600)
#define BUTTON_SELECT(in) (in >= 600 && in < 800)
```

**LcdMenu works perfectly with this setup!** You can use their button adapter or keep your existing button reading logic.

---

## Migration Strategy

### Phase 1: Keep Your Current Display Code
Your existing display code works. Don't rush to replace everything.

### Phase 2: Evaluate LcdMenu
1. Install the library
2. Create a **parallel menu system** for one screen (e.g., cockpit view)
3. Test navigation with your button shield
4. Compare code complexity

### Phase 3: Gradual Migration
- Port one menu at a time
- Keep your measurement reading code as-is
- Only change the UI layer

---

## Code Size Reality Check ⚠️

**IMPORTANT:** With only 32KB flash on Arduino Uno, you need to be very careful!

Your current empty Controller uses only **518 bytes**. Once you add back all the OBD protocol code, LCD handling, menu logic, and measurements from `obdisplay.cpp.old`, you'll likely use:
- **18-25KB** for your application logic
- This leaves only **7-14KB** for libraries

### Library Size Reality:
| Library | Library Size | Compiled Impact | Verdict |
|---------|--------------|-----------------|---------|
| LcdMenu | 46KB source | ~4-6KB compiled | ⚠️ Tight but possible |
| ArduinoMenu | 163KB source | ~8-12KB compiled | ❌ Too large |
| Manual menus | 0KB | Your code only | ✅ Most efficient |

**Conclusion:** With 32KB, menu libraries might push you over the limit once you add your full application back!

---

## My Revised Recommendation for 32KB Arduino Uno

**Given your memory constraints:**

### ✅ **Keep Manual Menu System, But Refactor It**

Your current approach is actually the most memory-efficient. Instead of adding a library, let's improve what you have:

1. **Create lightweight menu abstractions** in your Controller:
   ```cpp
   // Minimal menu system - no library overhead
   struct MenuItem {
       const char* label;
       void (*onUpdate)();      // Update sensor values
       void (*onDisplay)();     // Display the screen
   };
   
   class MenuSystem {
       MenuItem* items[10];
       uint8_t currentIndex = 0;
       // ... simple navigation logic
   };
   ```

2. **Use function pointers** instead of switch statements (cleaner, same size)

3. **Share common LCD routines** to reduce duplication

4. **Use PROGMEM** for strings to save RAM:
   ```cpp
   const char str_speed[] PROGMEM = "Speed: ";
   const char str_rpm[] PROGMEM = "RPM: ";
   ```

### Alternative: Simpler Libraries

If you still want library help, consider these **minimal** alternatives:

1. **Just use better LiquidCrystal**: Switch to I2C version
   ```ini
   lib_deps = marcoschwartz/LiquidCrystal_I2C@^1.1.4
   ```
   - Saves GPIO pins (only uses 2 pins via I2C)
   - Same memory footprint as regular LiquidCrystal
   - Still manual menus, but cleaner wiring

2. **Write your own minimal menu helper** (~500 bytes):
   - Basic structure for menu items
   - Navigation logic
   - No fancy features
   - Tailored to your exact needs

### Optimization Tips

To maximize your 32KB:
- ✅ Use `F()` macro for string literals: `lcd.print(F("Speed"));`
- ✅ Use `PROGMEM` for constant arrays and strings
- ✅ Share common display functions (don't duplicate)
- ✅ Use `-Os` optimization flag (already in platformio.ini)
- ✅ Enable LTO (Link Time Optimization) - commented in your platformio.ini
- ✅ Remove debug code in production builds

Would you like me to:
1. **Refactor your existing menu system** with better C++ patterns (no libraries)?
2. **Create a minimal custom MenuSystem class** tailored to your needs?
3. **Optimize the platformio.ini** for maximum space savings?
