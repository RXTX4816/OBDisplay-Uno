#include "DisplayManager.h"

namespace obd {
namespace Display {

DisplayManager::DisplayManager(LiquidCrystal &lcd)
    : lcd_(lcd)
{
}

void DisplayManager::begin(uint8_t cols, uint8_t rows)
{
    lcd_.begin(cols, rows);
}

void DisplayManager::clear()
{
    lcd_.clear();
}

void DisplayManager::print(uint8_t x, uint8_t y, const __FlashStringHelper *s)
{
    lcd_.setCursor(x, y);
    lcd_.print(s);
}

void DisplayManager::print(uint8_t x, uint8_t y, const String &s)
{
    lcd_.setCursor(x, y);
    lcd_.print(s);
}

void DisplayManager::print(uint8_t x, uint8_t y, const String &s, uint8_t width)
{
    String tmp = s;
    while (tmp.length() < width) tmp += " ";
    print(x, y, tmp);
}

void DisplayManager::print(uint8_t x, uint8_t y, int value)
{
    lcd_.setCursor(x, y);
    lcd_.print(value);
}

void DisplayManager::print(uint8_t x, uint8_t y, const char *s, uint8_t width)
{
    String tmp(s);
    while (tmp.length() < width) tmp += " ";
    print(x, y, tmp);
}

void DisplayManager::print(uint8_t x, uint8_t y, float value, uint8_t width)
{
    lcd_.setCursor(x, y);
    // Always print with 1 decimal like original lcd_print(float,...)
    String s = String(value, 1);
    if (width > 0 && s.length() > width) {
        // If it does not fit, just print without padding (same behavior as original)
        lcd_.print(s);
        return;
    }
    while (width > 0 && s.length() < width) {
        s += " ";
    }
    lcd_.print(s);
}

void DisplayManager::clearRegion(uint8_t x, uint8_t y, uint8_t width)
{
    lcd_.setCursor(x, y);
    for (uint8_t i = 0; i < width; ++i) {
        lcd_.print(" ");
    }
}

template <typename T>
static void printCockpitNumeric(DisplayManager &dm,
                                uint8_t x,
                                uint8_t y,
                                T value,
                                uint8_t width,
                                bool &updated,
                                bool forceUpdate)
{
    if (!(updated || forceUpdate)) return;
    dm.clearRegion(x, y, width);
    // width check like original: only print if it fits
    String s = String(value);
    if (s.length() <= width) {
        dm.print(x, y, s);
    }
    updated = false;
}

static void printCockpitFloat(DisplayManager &dm,
                              uint8_t x,
                              uint8_t y,
                              float value,
                              uint8_t width,
                              bool &updated,
                              bool forceUpdate)
{
    if (!(updated || forceUpdate)) return;
    dm.clearRegion(x, y, width);
    String s = String(value, 1);
    if (s.length() <= width) {
        dm.print(x, y, s);
    }
    updated = false;
}

static void printCockpitString(DisplayManager &dm,
                               uint8_t x,
                               uint8_t y,
                               const String &text,
                               uint8_t width,
                               bool &updated,
                               bool forceUpdate)
{
    if (!(updated || forceUpdate)) return;
    dm.clearRegion(x, y, width);
    dm.print(x, y, text);
    updated = false;
}

void DisplayManager::initMenu(const Input::MenuState &menuState,
                              uint8_t addrSelected,
                              int kwpModeInt)
{
    (void)kwpModeInt;
    switch (menuState.currentMenu()) {
    case MenuId::Cockpit:
        initMenuCockpit(menuState.cockpitScreen(), addrSelected);
        break;
    case MenuId::Experimental:
        initMenuExperimental();
        break;
    case MenuId::Debug:
        initMenuDebug();
        break;
    case MenuId::Dtc:
        initMenuDtc(menuState.dtcScreen());
        break;
    case MenuId::Settings:
        initMenuSettings(menuState.settingsScreen());
        break;
    }
}

void DisplayManager::render(const Input::MenuState &menuState,
                            const Model::OBDSignals &signals,
                            const Model::DTCStore &dtcStore,
                            uint8_t addrSelected,
                            int kwpModeInt,
                            bool forceUpdate)
{
    switch (menuState.currentMenu()) {
    case MenuId::Cockpit:
        displayMenuCockpit(menuState.cockpitScreen(), addrSelected,
                           signals, forceUpdate);
        break;
    case MenuId::Experimental:
        displayMenuExperimental(menuState.experimentalScreen(),
                                signals, forceUpdate);
        break;
    case MenuId::Debug:
        displayMenuDebug(menuState.debugScreen(),
                         signals, kwpModeInt, forceUpdate);
        break;
    case MenuId::Dtc:
        displayMenuDtc(menuState.dtcScreen(), dtcStore, forceUpdate);
        break;
    case MenuId::Settings:
        displayMenuSettings(menuState.settingsScreen(), kwpModeInt,
                            forceUpdate);
        break;
    }
}

// NOTE: The bodies of initMenu* and displayMenu* should be ported directly
// from obdisplay.cpp.old, preserving behavior, updating to use fields
// from Model::OBDSignals / Model::DTCStore instead of globals.

void DisplayManager::initMenuCockpit(uint8_t screen, uint8_t addrSelected)
{
    switch (addrSelected) {
    case 0x01: // ADDR_ENGINE
        switch (screen) {
        case 0:
            print(15, 0, F("V"));
            print(13, 1, F("TBa"));
            break;
        case 1:
            print(10, 0, F("load"));
            print(13, 1, F("STa"));
            break;
        case 2:
            print(12, 0, F("bits"));
            print(10, 1, F("lambda"));
            break;
        case 3:
            print(6, 0, F("kmh"));
            print(8, 1, F("mbar"));
            break;
        case 4:
            print(6, 0, F("C temp"));
            print(6, 1, F("C temp"));
            break;
        default:
            print(0, 0, F("Screen"));
            print(7, 0, String(screen));
            print(0, 1, F("not supported!"));
            break;
        }
        break;
    case 0x17: // ADDR_INSTRUMENTS
        switch (screen) {
        case 0:
            print(4, 0, F("KMH"));
            print(13, 0, F("RPM"));
            print(3, 1, F("C"));
            print(8, 1, F("C"));
            print(13, 1, F("L"));
            break;
        case 1:
            print(2, 0, F("OL"));
            print(7, 0, F("OP"));
            print(13, 0, F("AT"));
            print(6, 1, F("KM"));
            print(13, 1, F("FSR"));
            break;
        case 2:
            print(6, 0, F("TIME"));
            print(7, 1, F("L/100km"));
            break;
        case 3:
            print(9, 0, F("secs"));
            print(6, 1, F("km"));
            break;
        case 4:
            print(6, 0, F("km burned"));
            print(7, 1, F("L/h"));
            break;
        default:
            print(0, 0, F("Screen"));
            print(7, 0, String(screen));
            print(0, 1, F("not supported!"));
            break;
        }
        break;
    default:
        print(0, 0, F("Addr"));
        print(6, 0, String(addrSelected, HEX));
        print(0, 1, F("not supported!"));
        break;
    }
}

void DisplayManager::initMenuExperimental()
{
    print(0, 0, F("G:"));
    print(0, 1, F("S:"));
}

void DisplayManager::initMenuDebug()
{
    // Status bar
    print(0, 0, F("C:"));
    print(4, 0, F("A:"));
    print(9, 0, F("BC:"));
    print(0, 1, F("KWP:"));
    print(7, 1, F("FPS:"));
}

void DisplayManager::initMenuDtc(uint8_t screen)
{
    switch (screen) {
    case 0:
        print(0, 0, F("DTC menu addr "));
        print(0, 1, F("<"));
        print(5, 1, F("Read"));
        print(15, 1, F(">"));
        break;
    case 1:
        print(0, 0, F("DTC menu addr "));
        print(0, 1, F("<"));
        print(5, 1, F("Clear"));
        print(15, 1, F(">"));
        break;
    default:
        // 2-9 share the same static labels
        print(1, 0, F("/"));
        print(10, 0, F("St:"));
        print(0, 1, F("/8"));
        print(10, 1, F("St:"));
        break;
    }
}

void DisplayManager::initMenuSettings(uint8_t screen)
{
    switch (screen) {
    case 0:
        print(0, 0, F("Exit ECU:"));
        print(0, 1, F("< Press select >"));
        break;
    case 1:
        print(0, 0, F("KWP Mode:"));
        print(0, 1, F("<"));
        print(15, 1, F(">"));
        break;
    default:
        print(0, 0, F("Screen"));
        print(7, 0, String(screen));
        print(0, 1, F("not supported!"));
        break;
    }
}

void DisplayManager::displayMenuCockpit(uint8_t screen, uint8_t addrSelected,
                                        const Model::OBDSignals &signals,
                                        bool forceUpdate)
{
    using namespace Model;

    switch (addrSelected) {
    case 0x01: { // ADDR_ENGINE
        const EngineSignals &e = signals.engine;
        const InstrumentSignals &i = signals.instruments;
        switch (screen) {
        case 0:
            printCockpitFloat(*this, 0, 0, e.voltage, 7,
                              const_cast<bool &>(e.voltageUpdated), forceUpdate);
            printCockpitFloat(*this, 0, 1, e.tbAngle, 7,
                              const_cast<bool &>(e.tbAngleUpdated), forceUpdate);
            break;
        case 1:
            printCockpitNumeric(*this, 0, 0, e.engineLoad, 7,
                                const_cast<bool &>(e.engineLoadUpdated), forceUpdate);
            printCockpitFloat(*this, 0, 1, e.steeringAngle, 7,
                              const_cast<bool &>(e.steeringAngleUpdated), forceUpdate);
            break;
        case 2: {
            EngineSignals &em = const_cast<EngineSignals &>(e);
            if (em.errorBitsUpdated || forceUpdate) {
                em.bitsAsString[0] = em.exhaustGasRecirculationError ? '1' : '0';
                em.bitsAsString[1] = em.oxygenSensorHeatingError ? '1' : '0';
                em.bitsAsString[2] = em.oxygenSensorError ? '1' : '0';
                em.bitsAsString[3] = em.airConditioningError ? '1' : '0';
                em.bitsAsString[4] = em.secondaryAirInjectionError ? '1' : '0';
                em.bitsAsString[5] = em.evaporativeEmissionsError ? '1' : '0';
                em.bitsAsString[6] = em.catalystHeatingError ? '1' : '0';
                em.bitsAsString[7] = em.catalyticConverter ? '1' : '0';
                em.bitsAsString[8] = '\0';
            }
            printCockpitString(*this, 0, 0, em.bitsAsString, 7,
                               const_cast<bool &>(em.errorBitsUpdated), forceUpdate);
            printCockpitNumeric(*this, 0, 1, e.lambda2, 7,
                                const_cast<bool &>(e.lambda2Updated), forceUpdate);
            break;
        }
        case 3:
            printCockpitNumeric(*this, 0, 0, i.vehicleSpeed, 7,
                                const_cast<bool &>(i.vehicleSpeedUpdated), forceUpdate);
            printCockpitNumeric(*this, 0, 1, e.pressure, 7,
                                const_cast<bool &>(e.pressureUpdated), forceUpdate);
            break;
        case 4:
            printCockpitNumeric(*this, 0, 0, e.tempUnknown2, 4,
                                const_cast<bool &>(e.tempUnknown2Updated), forceUpdate);
            printCockpitNumeric(*this, 0, 1, e.tempUnknown3, 4,
                                const_cast<bool &>(e.tempUnknown3Updated), forceUpdate);
            break;
        default:
            print(0, 0, F("Screen"));
            print(7, 0, String(screen));
            print(0, 1, F("not supported!"));
            break;
        }
        break;
    }
    case 0x17: { // ADDR_INSTRUMENTS
        const InstrumentSignals &i = signals.instruments;
        const ComputedStats &c = signals.computed;
        switch (screen) {
        case 0:
            printCockpitNumeric(*this, 0, 0, i.vehicleSpeed, 3,
                                const_cast<bool &>(i.vehicleSpeedUpdated), forceUpdate);
            printCockpitNumeric(*this, 8, 0, i.engineRpm, 4,
                                const_cast<bool &>(i.engineRpmUpdated), forceUpdate);
            printCockpitNumeric(*this, 0, 1, i.coolantTemp, 3,
                                const_cast<bool &>(i.coolantTempUpdated), forceUpdate);
            printCockpitNumeric(*this, 5, 1, i.oilTemp, 3,
                                const_cast<bool &>(i.oilTempUpdated), forceUpdate);
            printCockpitNumeric(*this, 10, 1, i.fuelLevel, 2,
                                const_cast<bool &>(i.fuelLevelUpdated), forceUpdate);
            break;
        case 1:
            printCockpitNumeric(*this, 0, 0, i.oilLevelOk, 1,
                                const_cast<bool &>(i.oilLevelOkUpdated), forceUpdate);
            printCockpitNumeric(*this, 5, 0, i.oilPressureMin, 1,
                                const_cast<bool &>(i.oilPressureMinUpdated), forceUpdate);
            printCockpitNumeric(*this, 10, 0, i.ambientTemp, 2,
                                const_cast<bool &>(i.ambientTempUpdated), forceUpdate);
            printCockpitNumeric(*this, 0, 1, i.odometer, 6,
                                const_cast<bool &>(i.odometerUpdated), forceUpdate);
            printCockpitNumeric(*this, 9, 1, i.fuelSensorResistance, 3,
                                const_cast<bool &>(i.fuelSensorResistanceUpdated), forceUpdate);
            break;
        case 2:
            printCockpitNumeric(*this, 0, 0, i.timeEcu, 5,
                                const_cast<bool &>(i.timeEcuUpdated), forceUpdate);
            printCockpitFloat(*this, 0, 1, c.fuelPer100km, 6,
                              const_cast<bool &>(c.fuelPer100kmUpdated), forceUpdate);
            break;
        case 3:
            printCockpitNumeric(*this, 0, 0, c.elapsedSecondsSinceStart, 8,
                                const_cast<bool &>(c.elapsedSecondsSinceStartUpdated), forceUpdate);
            printCockpitNumeric(*this, 0, 1, c.elapsedKmSinceStart, 5,
                                const_cast<bool &>(c.elapsedKmSinceStartUpdated), forceUpdate);
            break;
        case 4:
            printCockpitNumeric(*this, 0, 0, c.fuelBurnedSinceStart, 5,
                                const_cast<bool &>(c.fuelBurnedSinceStartUpdated), forceUpdate);
            printCockpitFloat(*this, 0, 1, c.fuelPerHour, 6,
                              const_cast<bool &>(c.fuelPerHourUpdated), forceUpdate);
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void DisplayManager::displayMenuExperimental(uint8_t /*screen*/,
                                             const Model::OBDSignals &signals,
                                             bool forceUpdate)
{
    using namespace Model;
    ExperimentalGroup &eg = const_cast<ExperimentalGroup &>(signals.experimental);

    // G: <groupCurrent>
    bool groupUpdated = true; // always print when forced
    printCockpitNumeric(*this, 2, 0, eg.groupCurrent, 2,
                        groupUpdated, forceUpdate);

    // S: <group_side>
    bool sideUpdated = eg.groupSideUpdated || forceUpdate;
    uint8_t sideVal = eg.groupSide ? 1 : 0;
    printCockpitNumeric(*this, 2, 1, sideVal, 2,
                        sideUpdated, true);
    eg.groupSideUpdated = false;

    uint8_t first = eg.groupSide ? 2 : 0;
    uint8_t second = eg.groupSide ? 3 : 1;

    printCockpitFloat(*this, 4, 0, eg.v[first], 7,
                      eg.vUpdated, true);
    printCockpitFloat(*this, 4, 1, eg.v[second], 7,
                      eg.vUpdated, true);
    eg.vUpdated = false;

    printCockpitString(*this, 11, 0, eg.unit[first], 7,
                       eg.unitUpdated, true);
    printCockpitString(*this, 11, 1, eg.unit[second], 7,
                       eg.unitUpdated, true);
    eg.unitUpdated = false;
}

void DisplayManager::displayMenuDebug(uint8_t /*screen*/,
                                      const Model::OBDSignals & /*signals*/,
                                      int /*kwpModeInt*/,
                                      bool /*forceUpdate*/)
{
    // Status bar: C:connected A:available
    print(0, 0, F("C:"));
    print(4, 0, F("A:"));
    // NOTE: actual connection / available counters are driven elsewhere;
    // here we only mirror static labels and placeholders.
}

void DisplayManager::displayMenuDtc(uint8_t screen,
                                    const Model::DTCStore &dtcStore,
                                    bool forceUpdate)
{
    if (screen == 0 || screen == 1) {
        return;
    }

    uint8_t dtcPointer = screen - 2;
    if (dtcPointer > 7) return;

    bool updatedDummy = true; // DTCStore does not track updated flags; always force
    uint16_t e0 = dtcStore.errorAt(dtcPointer * 2);
    uint8_t s0 = dtcStore.statusAt(dtcPointer * 2);
    uint16_t e1 = dtcStore.errorAt(dtcPointer * 2 + 1);
    uint8_t s1 = dtcStore.statusAt(dtcPointer * 2 + 1);

    printCockpitNumeric(*this, 0, 0, (uint8_t)(dtcPointer + 1), 1,
                        updatedDummy, forceUpdate);
    updatedDummy = true;
    printCockpitString(*this, 3, 0, String(e0), 6,
                       updatedDummy, forceUpdate);
    updatedDummy = true;
    printCockpitNumeric(*this, 13, 0, s0, 3,
                        updatedDummy, forceUpdate);

    updatedDummy = true;
    printCockpitString(*this, 3, 1, String(e1), 6,
                       updatedDummy, forceUpdate);
    updatedDummy = true;
    printCockpitNumeric(*this, 13, 1, s1, 3,
                        updatedDummy, forceUpdate);
}

void DisplayManager::displayMenuSettings(uint8_t /*screen*/,
                                         int kwpModeInt,
                                         bool forceUpdate)
{
    // Screen 1 shows current KWP mode text; others have static content
    // set in initMenuSettings.
    // We only support updating the mode string here.
    (void)forceUpdate;
    // Mode integer mapping from original: 0=ACK,1=READSENSORS,2=READGROUP
    const uint8_t screenForMode = 1;
    // Caller ensures we are on correct screen; here we just render text.
    clearRegion(4, 1, 7);
    lcd_.setCursor(4, 1);
    switch (kwpModeInt) {
    case 0:
        lcd_.print(F("ACK"));
        break;
    case 2:
        lcd_.print(F("GROUP"));
        break;
    case 1:
    default:
        lcd_.print(F("SENSOR"));
        break;
    }
}

} // namespace Display
} // namespace obd
