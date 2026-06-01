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
    prevComputeMs_ = 0xFFFFFFFFu;
    tripDistAccum_ = 0;
}

void OBDSignals::compute(uint32_t nowMs, uint32_t connectTimeStart)
{
    computed.elapsedSecondsSinceStart = (nowMs - connectTimeStart) / 1000;
    computed.elapsedSecondsSinceStartUpdated = true;

    // ── Speed-integrated trip distance ──────────────────────────────────────
    // Guard first call: sentinel 0xFFFFFFFF means prevComputeMs_ is uninitialized.
    if (prevComputeMs_ == 0xFFFFFFFFu)
    {
        prevComputeMs_ = nowMs;
    }
    else
    {
        uint32_t deltaMs = nowMs - prevComputeMs_;
        prevComputeMs_ = nowMs;
        // tripDistAccum_ in units of km/h × ms; 36000 km/h·ms = 0.01 km
        tripDistAccum_ += (uint32_t)instruments.vehicleSpeed * deltaMs;
        computed.tripDistance100 += tripDistAccum_ / 36000u;
        tripDistAccum_ %= 36000u;
    }
    computed.elapsedKmSinceStart = (uint16_t)(computed.tripDistance100 / 100u);
    computed.elapsedKmSinceStartUpdated = true;

    // ── EMA-smoothed fuel level (α=1/32, τ≈1.6 s at 50 ms) — 16-bit only ───
    // smooth += (newX8 - smooth) >> 5  (arithmetic shift handles both directions)
    if (instruments.fuelLevelUpdated)
    {
        uint16_t newX8 = (uint16_t)instruments.fuelLevel << 3u;
        int16_t delta = (int16_t)newX8 - (int16_t)instruments.fuelLevelSmoothX8;
        instruments.fuelLevelSmoothX8 =
            (uint16_t)((int16_t)instruments.fuelLevelSmoothX8 + (delta >> 5));
    }

    // ── Fuel burned via smoothed level ──────────────────────────────────────
    // burnedX8 in 1/8 L units; clamp to zero if smooth somehow exceeds start.
    uint16_t startX8 = (uint16_t)instruments.fuelLevelStart * 8u;
    uint16_t burnedX8 =
        (instruments.fuelLevelSmoothX8 < startX8) ? (startX8 - instruments.fuelLevelSmoothX8) : 0u;
    computed.fuelBurnedSinceStart = (uint8_t)(burnedX8 >> 3u);
    computed.fuelBurnedSinceStartUpdated = true;

    // ── fuelPer100km ×10 ────────────────────────────────────────────────────
    // Real data once >1 km driven with actual fuel drop; otherwise estimate
    // from RPM/speed: L/100km×10 ≈ (RPM/2)*5/speed — all 16-bit safe on AVR.
    // Calibrated for ~1.4 L petrol: 2700 RPM at 100 km/h → ~6.7 L/100km.
    if (burnedX8 > 0u && computed.tripDistance100 > 100u)
    {
        computed.fuelPer100km = (uint16_t)((uint32_t)burnedX8 * 12500u / computed.tripDistance100);
    }
    else if (instruments.vehicleSpeed > 17u)
    {
        // At speed>17 result fits uint16_t and stays below 999 (no overflow, no cap needed)
        computed.fuelPer100km =
            (uint16_t)instruments.engineRpm / 2u * 5u / instruments.vehicleSpeed;
    }
    else
    {
        computed.fuelPer100km = 0u;
    }
    computed.fuelPer100kmUpdated = true;

    // ── fuelPerHour ×10: burnedX8*4500/sec ──────────────────────────────────
    // Derivation: (burnedX8/8 L) / secs * 3600 * 10 = burnedX8 * 4500 / secs
    computed.fuelPerHour =
        (computed.elapsedSecondsSinceStart > 0)
            ? (uint16_t)((uint32_t)burnedX8 * 4500u / computed.elapsedSecondsSinceStart)
            : 0u;
    computed.fuelPerHourUpdated = true;

    // ── kmRemaining: fuelLevelSmoothX8*125/fuelPer100km ─────────────────────
    // Derivation: (smooth/8 L) / (fuelPer100km/10 L/100km) * 100
    //           = smooth * 100 * 10 / (8 * fuelPer100km)
    //           = smooth * 125 / fuelPer100km
    uint32_t kmR = (computed.fuelPer100km > 0)
                       ? ((uint32_t)instruments.fuelLevelSmoothX8 * 125u / computed.fuelPer100km)
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
        // Oil pressure switch: normal value = 31. Low-pressure encoded value unknown.
        // Debounce 3 compute cycles to filter oil-slosh switch chatter.
        if (instruments.oilPressureMinUpdated)
        {
            if (instruments.oilPressureMin != 31)
            {
                if (instruments.oilPressureLowCount < 3)
                    instruments.oilPressureLowCount++;
            }
            else
            {
                if (instruments.oilPressureLowCount > 0)
                    instruments.oilPressureLowCount--;
            }
        }
        if (instruments.oilPressureLowCount >= 3)
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
