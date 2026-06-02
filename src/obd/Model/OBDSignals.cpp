// SPDX-License-Identifier: GPL-3.0-or-later
#include "OBDSignals.h"
#include "../../Config.h"

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
    fuelBurnAccum01_ = 0;
    fuelBurnedX8_01_ = 0;
}

void OBDSignals::compute(uint32_t nowMs, uint32_t connectTimeStart, uint8_t ecuAddr,
                         uint8_t fuelStartL)
{
    computed.elapsedSecondsSinceStart = (nowMs - connectTimeStart) / 1000;
    computed.elapsedSecondsSinceStartUpdated = true;

    // ── Speed-integrated trip distance ──────────────────────────────────────
    // Guard first call: sentinel 0xFFFFFFFF means prevComputeMs_ is uninitialized.
    uint32_t deltaMs = 0;
    if (prevComputeMs_ == 0xFFFFFFFFu)
    {
        prevComputeMs_ = nowMs;
    }
    else
    {
        deltaMs = nowMs - prevComputeMs_;
        prevComputeMs_ = nowMs;
        // tripDistAccum_ in units of km/h × ms; 36000 km/h·ms = 0.01 km
        tripDistAccum_ += (uint32_t)instruments.vehicleSpeed * deltaMs;
        computed.tripDistance100 += tripDistAccum_ / 36000u;
        tripDistAccum_ %= 36000u;
    }
    computed.elapsedKmSinceStart = (uint16_t)(computed.tripDistance100 / 100u);
    computed.elapsedKmSinceStartUpdated = true;

    if (ecuAddr == 0x17)
    {
        // ── EMA-smoothed fuel level — 16-bit only ───────────────────────────
        // Asymmetric: downward α=1/64 (slosh resistance), upward α=1/32 (fast recovery).
        // C++ arithmetic right-shift truncates positive values toward zero, so small
        // positive deltas can yield step=0 and the EMA never recovers upward — add a
        // ±1 floor for non-zero deltas.
        if (instruments.fuelLevelUpdated)
        {
            uint16_t newX8 = (uint16_t)instruments.fuelLevel << 3u;
            int16_t delta = (int16_t)newX8 - (int16_t)instruments.fuelLevelSmoothX8;
            int16_t step = (delta < 0) ? (delta >> 6) : (delta >> 5);
            if (step == 0 && delta != 0)
                step = (delta < 0) ? -1 : 1;
            instruments.fuelLevelSmoothX8 =
                (uint16_t)((int16_t)instruments.fuelLevelSmoothX8 + step);
        }

        // ── Fuel burned via smoothed level ──────────────────────────────────
        // burnedX8 in 1/8 L units; clamp to zero if smooth somehow exceeds start.
        uint16_t startX8 = (uint16_t)instruments.fuelLevelStart * 8u;
        uint16_t burnedX8 = (instruments.fuelLevelSmoothX8 < startX8)
                                ? (startX8 - instruments.fuelLevelSmoothX8)
                                : 0u;
        computed.fuelBurnedSinceStart = (uint8_t)(burnedX8 >> 3u);
        computed.fuelBurnedSinceStartUpdated = true;

        // ── fuelPer100km ×10 ────────────────────────────────────────────────
        // Real data once >10 km driven with actual fuel drop; otherwise estimate from
        // RPM/speed. 10 km minimum: fuel sensor has 1 L precision — at ≤10 L/100km
        // you need 10 km before consuming a full sensor step, so shorter trips yield
        // nonsensical values (e.g. 1 L sensor noise over 1 km → 100+ L/100km).
        // Capped at 300 (30.0 L/100km) as a safety net for any edge-case transient.
        if (burnedX8 > 0u && computed.tripDistance100 > 1000u)
        {
            uint16_t candidate =
                (uint16_t)((uint32_t)burnedX8 * 12500u / computed.tripDistance100);
            computed.fuelPer100km = (candidate > 300u) ? 300u : candidate;
        }
        else if (instruments.vehicleSpeed > 17u)
        {
            computed.fuelPer100km =
                (uint16_t)instruments.engineRpm / 10u * 23u / instruments.vehicleSpeed;
        }
        else
        {
            computed.fuelPer100km = 0u;
        }
        computed.fuelPer100kmUpdated = true;

        // ── fuelPerHour ×10: burnedX8*4500/sec ───────────────────────────────
        computed.fuelPerHour =
            (computed.elapsedSecondsSinceStart > 0)
                ? (uint16_t)((uint32_t)burnedX8 * 4500u / computed.elapsedSecondsSinceStart)
                : 0u;
        computed.fuelPerHourUpdated = true;

        // ── kmRemaining: fuelLevelSmoothX8*125/fuelPer100km ──────────────────
        uint32_t kmR =
            (computed.fuelPer100km > 0u)
                ? ((uint32_t)instruments.fuelLevelSmoothX8 * 125u / computed.fuelPer100km)
                : 0u;
        computed.kmRemaining = (uint16_t)(kmR > 9999u ? 9999u : kmR);
        computed.kmRemainingUpdated = true;
    }
    else if (ecuAddr == 0x01)
    {
        // ── Fuel flow from RPM × load (ENGINE01_FUEL_DENOM calibration) ─────
        // fuelPerHour ×10 = RPM × engineLoad / DENOM
        uint16_t fph = 0;
        if (engine.engineLoad > 0 && instruments.engineRpm > 0)
        {
            uint32_t raw =
                (uint32_t)instruments.engineRpm * engine.engineLoad / ENGINE01_FUEL_DENOM;
            fph = (raw > 0xFFFFu) ? 0xFFFFu : (uint16_t)raw;
        }
        computed.fuelPerHour = fph;
        computed.fuelPerHourUpdated = true;

        // ── Fuel burn integration ×8 ─────────────────────────────────────────
        // 1 unit of fuelBurnedX8_01_ = 0.125 L
        // fuelBurnAccum01_ accumulates fph * deltaMs; threshold = 4,500,000 per unit
        // Derivation: 1 L/hr×10 × 1 ms × 8 / (10 × 3,600,000 ms/hr) = 1/4,500,000
        fuelBurnAccum01_ += (uint32_t)fph * deltaMs;
        while (fuelBurnAccum01_ >= 4500000UL)
        {
            fuelBurnedX8_01_++;
            fuelBurnAccum01_ -= 4500000UL;
        }
        computed.fuelBurnedSinceStart = (uint8_t)(fuelBurnedX8_01_ >> 3u);
        computed.fuelBurnedSinceStartUpdated = true;

        // ── fuelPer100km ×10 = fuelPerHour × 100 / speed ────────────────────
        if (instruments.vehicleSpeed > 5u && fph > 0u)
        {
            uint32_t f100 = (uint32_t)fph * 100u / instruments.vehicleSpeed;
            computed.fuelPer100km = (f100 > 0xFFFFu) ? 0xFFFFu : (uint16_t)f100;
        }
        else
        {
            computed.fuelPer100km = 0u;
        }
        computed.fuelPer100kmUpdated = true;

        // ── kmRemaining from EEPROM fuel start ──────────────────────────────
        // fuelStartL set from 0x17 EEPROM at connect time; 0 = not set
        if (fuelStartL > 0u && computed.fuelPer100km > 0u)
        {
            uint8_t burned = computed.fuelBurnedSinceStart;
            uint8_t remaining = (burned < fuelStartL) ? (fuelStartL - burned) : 0u;
            uint32_t kmRem = (uint32_t)remaining * 1000u / computed.fuelPer100km;
            computed.kmRemaining = (uint16_t)(kmRem > 9999u ? 9999u : kmRem);
        }
        else
        {
            computed.kmRemaining = 0u;
        }
        computed.kmRemainingUpdated = true;
    }
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
        if (instruments.oilTempUpdated && instruments.oilTemp > WARN_OIL_TEMP_HIGH_C)
            setWarn(warnings, WARN_OIL_HOT, 3);
        if (instruments.oilLevelOkUpdated)
        {
            uint8_t ol = instruments.oilLevelOk;
            if (ol < WARN_OIL_LVL_CRIT_RAW)
                setWarn(warnings, WARN_OIL_LVL, 3);
            if (ol < WARN_OIL_LVL_LOW_RAW)
                setWarn(warnings, WARN_OIL_LVL_LOW, 1);
        }
        // Coolant: gate once, cache value, run all three threshold checks
        if (instruments.coolantTempUpdated)
        {
            uint8_t ct = instruments.coolantTemp;
            if (ct > WARN_COOLANT_HIGH_C)
                setWarn(warnings, WARN_COOL_HOT, 3);
            if (ct < WARN_COOLANT_COLD_C)
                setWarn(warnings, WARN_VERY_COLD, 2);
            if (ct < WARN_COOLANT_WARM_C)
                setWarn(warnings, WARN_COLD_ENG, 1);
        }
        // Fuel: gate once, cache value, run both threshold checks
        if (instruments.fuelLevelUpdated)
        {
            uint8_t fl = instruments.fuelLevel;
            if (fl < WARN_FUEL_CRIT_L)
                setWarn(warnings, WARN_FUEL_CRIT, 2);
            if (fl < WARN_FUEL_LOW_L)
                setWarn(warnings, WARN_FUEL_LOW, 1);
        }
    }
    else if (ecuAddr == 0x01)
    {
        if (engine.voltageUpdated && engine.voltage < WARN_VOLTAGE_LOW_X10)
            setWarn(warnings, WARN_LOW_VOLT, 2);
        if (engine.engineLoadUpdated && engine.engineLoad > WARN_ENGINE_LOAD_HIGH)
            setWarn(warnings, WARN_HIGH_LOAD, 1);
        // Coolant proxy from group 4: gate once, cache, run all threshold checks
        if (engine.tempUnknown2Updated)
        {
            uint8_t t2 = engine.tempUnknown2;
            if (t2 > WARN_COOLANT_HIGH_C)
                setWarn(warnings, WARN_COOL_HOT, 3);
            if (t2 < WARN_COOLANT_COLD_C)
                setWarn(warnings, WARN_VERY_COLD, 2);
            if (t2 < WARN_COOLANT_WARM_C)
                setWarn(warnings, WARN_COLD_ENG, 1);
        }
    }

    // hasNew fires only when new bits appear (not when warnings clear)
    if (warnings.bits != prevBits && (warnings.bits & ~prevBits) != 0)
        warnings.hasNew = true;
}

} // namespace Model
} // namespace obd
