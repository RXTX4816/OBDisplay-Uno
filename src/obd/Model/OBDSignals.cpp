// SPDX-License-Identifier: GPL-3.0-or-later
#include "OBDSignals.h"

namespace obd
{
namespace Model
{

void ExperimentalGroup::reset()
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        k[i] = 0;
        v[i] = 1234; // 123.4 ×10
        // Reset unit text to "N/A"
        unit[i][0] = 'N';
        unit[i][1] = '/';
        unit[i][2] = 'A';
        unit[i][3] = '\0';
        // Clear remaining characters in buffer
        for (uint8_t j = 4; j < UnitWidth + 1; ++j)
        {
            unit[i][j] = '\0';
        }
    }
    kUpdated = vUpdated = unitUpdated = false;
    groupSide = false;
    groupSideUpdated = false;
}

void ExperimentalGroup::invertGroupSide()
{
    groupSide = !groupSide;
    groupSideUpdated = true;
}

void OBDSignals::reset()
{
    instruments = InstrumentSignals{};
    engine = EngineSignals{};
    experimental.reset();
    computed = ComputedStats{};
    warnings = WarningState{};
}

void OBDSignals::compute(uint32_t nowMs, uint32_t connectTimeStart)
{
    computed.elapsedSecondsSinceStart = (nowMs - connectTimeStart) / 1000;
    computed.elapsedSecondsSinceStartUpdated = true;

    computed.elapsedKmSinceStart = (instruments.odometer - instruments.odometerStart);
    computed.elapsedKmSinceStartUpdated = true;

    computed.fuelBurnedSinceStart =
        abs((int)instruments.fuelLevelStart - (int)instruments.fuelLevel);
    computed.fuelBurnedSinceStartUpdated = true;

    // fuelPer100km ×10: burned*1000/km (e.g. 5L/60km → 83 = 8.3 L/100km)
    computed.fuelPer100km = (computed.elapsedKmSinceStart > 0)
                                ? (uint16_t)((uint32_t)computed.fuelBurnedSinceStart * 1000u /
                                             computed.elapsedKmSinceStart)
                                : 0u;
    computed.fuelPer100kmUpdated = true;

    // fuelPerHour ×10: burned*36000/sec (e.g. 5L/1800s → 100 = 10.0 L/h)
    computed.fuelPerHour = (computed.elapsedSecondsSinceStart > 0)
                               ? (uint16_t)((uint32_t)computed.fuelBurnedSinceStart * 36000u /
                                            computed.elapsedSecondsSinceStart)
                               : 0u;
    computed.fuelPerHourUpdated = true;

    // kmRemaining: estimated range at current L/100km rate; capped at 9999 for display
    uint32_t kmR = (computed.fuelPer100km > 0)
                       ? ((uint32_t)instruments.fuelLevel * 1000u / computed.fuelPer100km)
                       : 0u;
    computed.kmRemaining = (uint16_t)(kmR > 9999u ? 9999u : kmR);
    computed.kmRemainingUpdated = true;
}

// Helper: set a warning bit and update maxLevel
static inline void setWarn(WarningState& w, WarnBit bit, uint8_t level)
{
    w.bits |= (uint16_t)(1u << bit);
    if (level > w.maxLevel)
        w.maxLevel = level;
}

void OBDSignals::computeWarnings(uint8_t ecuAddr)
{
    uint16_t prevBits = warnings.bits;
    warnings.bits = 0;
    warnings.maxLevel = 0;

    if (ecuAddr == 0x17)
    {
        // Oil pressure: value 2 = "<min, 0.9 bar" confirmed; encoding uncertain, verify on car
        if (instruments.oilPressureMinUpdated && instruments.oilPressureMin >= 2)
            setWarn(warnings, WARN_OIL_PRES, 3);
        if (instruments.oilTempUpdated && instruments.oilTemp > 93)
            setWarn(warnings, WARN_OIL_HOT, 3);
        if (instruments.oilLevelOkUpdated && instruments.oilLevelOk == 0)
            setWarn(warnings, WARN_OIL_LVL, 2);
        // Coolant: gate once, cache value, run all three threshold checks
        if (instruments.coolantTempUpdated)
        {
            uint8_t ct = instruments.coolantTemp;
            if (ct > 93)
                setWarn(warnings, WARN_COOL_HOT, 3);
            if (ct < 40)
                setWarn(warnings, WARN_VERY_COLD, 2);
            if (ct < 75)
                setWarn(warnings, WARN_COLD_ENG, 1);
        }
        // Fuel: gate once, cache value, run both threshold checks
        if (instruments.fuelLevelUpdated)
        {
            uint8_t fl = instruments.fuelLevel;
            if (fl < 4)
                setWarn(warnings, WARN_FUEL_CRIT, 2);
            if (fl < 8)
                setWarn(warnings, WARN_FUEL_LOW, 1);
        }
    }
    else if (ecuAddr == 0x01)
    {
        if (engine.voltageUpdated && engine.voltage < 120)
            setWarn(warnings, WARN_LOW_VOLT, 2);
        if (engine.engineLoadUpdated && engine.engineLoad > 90)
            setWarn(warnings, WARN_HIGH_LOAD, 1);
        // Coolant proxy: gate once, cache, run all three threshold checks
        if (engine.tempUnknown2Updated)
        {
            uint8_t t2 = engine.tempUnknown2;
            if (t2 > 93)
                setWarn(warnings, WARN_COOL_HOT, 3);
            if (t2 < 40)
                setWarn(warnings, WARN_VERY_COLD, 2);
            if (t2 < 75)
                setWarn(warnings, WARN_COLD_ENG, 1);
        }
    }

    // hasNew fires only when new bits appear (not when warnings clear)
    if (warnings.bits != prevBits && (warnings.bits & ~prevBits) != 0)
        warnings.hasNew = true;
}

void OBDSignals::updateSimulation()
{
    InstrumentSignals& i = instruments;

    // Helper functions similar to simulate_values_helper in old code
    struct SimHelper
    {
        static void simulateUint8(uint8_t& val, uint8_t amount, bool& up, bool& updated,
                                  uint8_t maxVal, uint8_t minVal = 0)
        {
            if (up)
                val += amount;
            else
                val -= amount;
            updated = true;
            if (up && val >= maxVal)
                up = false;
            else if (!up && val <= minVal)
                up = true;
        }

        static void simulateUint16(uint16_t& val, uint8_t amount, bool& up, bool& updated,
                                   uint16_t maxVal, uint16_t minVal = 0)
        {
            if (up)
                val += amount;
            else
                val -= amount;
            updated = true;
            if (up && val >= maxVal)
                up = false;
            else if (!up && val <= minVal)
                up = true;
        }
    };

    static bool speedUp = true;
    static bool rpmUp = true;
    static bool coolantUp = true;
    static bool oilTempUp = true;
    static bool oilLevelUp = true;
    static bool fuelLevelUp = true;

    SimHelper::simulateUint16(i.vehicleSpeed, 1, speedUp, i.vehicleSpeedUpdated, (uint16_t)200);
    SimHelper::simulateUint16(i.engineRpm, 87, rpmUp, i.engineRpmUpdated, (uint16_t)7100);
    SimHelper::simulateUint8(i.coolantTemp, 1, coolantUp, i.coolantTempUpdated, (uint8_t)160);
    SimHelper::simulateUint8(i.oilTemp, 1, oilTempUp, i.oilTempUpdated, (uint8_t)160);
    SimHelper::simulateUint8(i.oilLevelOk, 1, oilLevelUp, i.oilLevelOkUpdated, (uint8_t)8);
    SimHelper::simulateUint8(i.fuelLevel, 1, fuelLevelUp, i.fuelLevelUpdated, (uint8_t)57);
}

} // namespace Model
} // namespace obd
