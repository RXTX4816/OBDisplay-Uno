// SPDX-License-Identifier: GPL-3.0-or-later
#include "OBDSignals.h"
#include "../../Config.h"

#include <string.h>

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
    // memset instead of assigning default-constructed temporaries: on AVR
    // that emitted a second copy of each constructor (~340 bytes of flash).
    // Every default in these structs is zero except engine.bitsAsString;
    // test_signals_reset_restores_every_default catches a new non-zero one.
    memset(&instruments, 0, sizeof instruments);
    memset(&engine, 0, sizeof engine);
    memset(engine.bitsAsString, ' ', sizeof engine.bitsAsString - 1);
    experimental.reset();
    memset(&computed, 0, sizeof computed);
    memset(&warnings, 0, sizeof warnings);
    memset(&fuelFloor, 0, sizeof fuelFloor);
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
        // One 50 ms cycle covers well under 1 km: the step fits 16 bits.
        uint16_t step = (uint16_t)(tripDistAccum_ / 36000u);
        tripDistAccum_ %= 36000u;
        computed.tripDistance100 += step;
        // No overflow check: the tank is dry long before 655 km past the floor.
        if (fuelFloor.active())
            fuelFloor.dist100 += step;
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
            // 0 = no reading yet (seed poll at connect failed): take the first
            // one as is instead of climbing from 0 through the fuel floor.
            if (instruments.fuelLevelSmoothX8 == 0u)
                instruments.fuelLevelSmoothX8 = newX8;
            int16_t delta = (int16_t)newX8 - (int16_t)instruments.fuelLevelSmoothX8;
            int16_t step = (delta < 0) ? (delta >> 6) : (delta >> 5);
            if (step == 0 && delta != 0)
                step = (delta < 0) ? -1 : 1;
            instruments.fuelLevelSmoothX8 =
                (uint16_t)((int16_t)instruments.fuelLevelSmoothX8 + step);
        }

        // ── Settled level: smoothed level held through tank slosh ───────────
        // The smoothed level still dips by several litres on a corner. Follow it
        // only once it has stayed on one side of the settled level for
        // FUEL_SETTLE_DWELL_CYCLES in a row; switching sides restarts the count.
        // 0 = unset (after reset): take the smoothed level as is.
        {
            uint16_t smX8 = instruments.fuelLevelSmoothX8;
            uint16_t& settledX8 = instruments.fuelLevelSettledX8;
            bool above = smX8 > settledX8;
            if (settledX8 == 0u || smX8 == settledX8)
            {
                settledX8 = smX8;
                instruments.fuelSettleCount = 0;
            }
            else
            {
                if (above != instruments.fuelSettleAbove)
                {
                    instruments.fuelSettleAbove = above;
                    instruments.fuelSettleCount = 0;
                }
                if (++instruments.fuelSettleCount >= FUEL_SETTLE_DWELL_CYCLES)
                {
                    settledX8 = smX8;
                    instruments.fuelSettleCount = 0;
                }
            }
        }

        // ── Fuel floor: enter at FUEL_FLOOR_L, leave on refuel ──────────────
        // settled - 1 < (FUEL_FLOOR_L + 1) × 8 - 1 also skips 0 (not known yet).
        FuelFloor& ff = fuelFloor;
        uint16_t fuelX8 = instruments.fuelLevelSettledX8;
        {
            bool active = ff.active();
            if (active ? fuelX8 >= FUEL_FLOOR_EXIT_L * 8u
                       : (uint16_t)(fuelX8 - 1u) < (FUEL_FLOOR_L + 1u) * 8u - 1u)
            {
                ff.dist100 = active ? 0u : 1u;
                ff.per100 = 0u;
            }
        }

        // ── Fuel burned via settled level ───────────────────────────────────
        // burnedX8 in 1/8 L units; clamp to zero if the level somehow exceeds start.
        uint16_t startX8 = (uint16_t)instruments.fuelLevelStart * 8u;
        uint16_t burnedX8 = (instruments.fuelLevelSettledX8 < startX8)
                                ? (startX8 - instruments.fuelLevelSettledX8)
                                : 0u;
        computed.fuelBurnedSinceStart = (uint8_t)(burnedX8 >> 3u);
        computed.fuelBurnedSinceStartUpdated = true;

        // ── fuelPer100km ×10 ────────────────────────────────────────────────
        // Real data once >10 km driven with actual fuel drop; otherwise estimate from
        // RPM/speed. 10 km minimum: fuel sensor has 1 L precision — at ≤10 L/100km
        // you need 10 km before consuming a full sensor step, so shorter trips yield
        // nonsensical values (e.g. 1 L sensor noise over 1 km → 100+ L/100km).
        // Capped at 300 (30.0 L/100km) as a safety net for any edge-case transient.
        bool measured = false; // trip-based value, not the momentary RPM estimate
        if (burnedX8 > 0u && computed.tripDistance100 > 1000u)
        {
            uint16_t candidate = (uint16_t)((uint32_t)burnedX8 * 12500u / computed.tripDistance100);
            computed.fuelPer100km = (candidate > 300u) ? 300u : candidate;
            measured = true;
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
        // At the floor the sender no longer drops, so the live value would fall
        // with every km. Freeze the measured value (or the default), and count
        // the fuel down from FUEL_FLOOR_L by distance instead of the sender.
        if (ff.active())
        {
            if (ff.per100 == 0u)
                ff.per100 = measured ? computed.fuelPer100km : FUEL_FLOOR_DEFAULT_PER100;
            computed.fuelPer100km = ff.per100;
            // 12500 / per100 = km ×100 per 1/8 L; 16-bit only. per100 is
            // 1..300 here (resumeFuelFloor rejects more), so it never divides by 0.
            uint16_t usedX8 = ff.dist100 / (12500u / ff.per100);
            fuelX8 = (usedX8 < FUEL_FLOOR_L * 8u) ? (uint16_t)(FUEL_FLOOR_L * 8u - usedX8) : 0u;
            ff.leftX8 = (uint8_t)fuelX8;
        }
        computed.fuelPer100kmUpdated = true;

        // ── fuelPerHour ×10: burnedX8*4500/sec ───────────────────────────────
        computed.fuelPerHour =
            (computed.elapsedSecondsSinceStart > 0)
                ? (uint16_t)((uint32_t)burnedX8 * 4500u / computed.elapsedSecondsSinceStart)
                : 0u;
        computed.fuelPerHourUpdated = true;

        // ── kmRemaining: fuelX8*125/fuelPer100km ─────────────────────────────
        // fuelX8: the settled level, or the estimate below the floor.
        uint32_t kmR =
            (computed.fuelPer100km > 0u) ? ((uint32_t)fuelX8 * 125u / computed.fuelPer100km) : 0u;
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

void OBDSignals::resumeFuelFloor()
{
    // The odometer moves in 10 km steps, so one step can be a tick during the
    // last km before power-off. More than one step means the car was driven
    // with the display unplugged: count the odometer difference (unbiased,
    // ±10 km). 0 = not read; the low 16 bits make the difference wrap-safe.
    // 300 km is more than FUEL_FLOOR_L lasts; the clamp keeps it in 16 bits.
    // A refuel meanwhile ends the floor on the first compute anyway.
    FuelFloor& ff = fuelFloor;
    // A torn EEPROM write can leave any value; above the 30 L/100km cap the
    // estimate could divide by zero. 0 = take the default on the next compute.
    if (ff.per100 > 300u)
        ff.per100 = 0u;
    uint16_t odoNow = (uint16_t)instruments.odometer;
    uint16_t unseenKm = odoNow - ff.odo;
    if (ff.active() && odoNow != 0u && ff.odo != 0u && unseenKm > 10u)
        ff.dist100 += (unseenKm > 300u ? 300u : unseenKm) * 100u;
}

// Helper: set a warning bit and update maxLevel (and newLevel if the bit was not set before)
static inline void setWarn(WarningState& w, uint16_t prevBits, WarnBit bit, uint8_t level)
{
    uint16_t mask = (uint16_t)(1u << bit);
    w.bits |= mask;
    if (level > w.maxLevel)
        w.maxLevel = level;
    if (!(prevBits & mask) && level > w.newLevel)
        w.newLevel = level;
}

// Helper: dwell + hysteresis for a fuel threshold. flX8 is the smoothed level in
// 1/8 L units, thresholdL the trip point in litres. The level must stay below the
// threshold for WARN_FUEL_DWELL_CYCLES consecutive compute cycles before the
// warning latches; once latched it holds until the level rises a further
// WARN_FUEL_HYST_X8 above the threshold.
static bool fuelBelow(uint8_t& count, uint16_t flX8, uint8_t thresholdL)
{
    bool latched = count >= WARN_FUEL_DWELL_CYCLES;
    uint16_t thresholdX8 = (uint16_t)thresholdL * 8u + (latched ? WARN_FUEL_HYST_X8 : 0u);
    if (flX8 < thresholdX8)
    {
        if (!latched)
            count++;
    }
    else
    {
        count = 0;
    }
    return count >= WARN_FUEL_DWELL_CYCLES;
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
            setWarn(warnings, prevBits, WARN_OIL_PRES, 3);
        if (instruments.oilTempUpdated && instruments.oilTemp > WARN_OIL_TEMP_HIGH_C)
            setWarn(warnings, prevBits, WARN_OIL_HOT, 3);
        if (instruments.oilLevelOkUpdated && instruments.oilLevelOk < WARN_OIL_LVL_RAW)
            setWarn(warnings, prevBits, WARN_OIL_LVL, 3);
        // Coolant: gate once, cache value, run all three threshold checks
        if (instruments.coolantTempUpdated)
        {
            int16_t ct = instruments.coolantTemp; // signed: -5 °C is cold, not hot
            if (ct > WARN_COOLANT_HIGH_C)
                setWarn(warnings, prevBits, WARN_COOL_HOT, 3);
            if (ct < WARN_COOLANT_COLD_C)
                setWarn(warnings, prevBits, WARN_VERY_COLD, 2);
            if (ct < WARN_COOLANT_WARM_C)
                setWarn(warnings, prevBits, WARN_COLD_ENG, 1);
        }
        // Fuel: evaluated on the EMA-smoothed level with a dwell counter, so a
        // tank sloshing on a turn or a bumpy road cannot fire a warning (and the
        // buzzer) while there is still plenty of fuel in the tank.
        if (instruments.fuelLevelUpdated)
        {
            uint16_t flX8 = instruments.fuelLevelSmoothX8;
            if (fuelBelow(instruments.fuelCritCount, flX8, WARN_FUEL_CRIT_L))
                setWarn(warnings, prevBits, WARN_FUEL_CRIT, 2);
            if (fuelBelow(instruments.fuelLowCount, flX8, WARN_FUEL_LOW_L))
                setWarn(warnings, prevBits, WARN_FUEL_LOW, 1);
        }
    }
    else if (ecuAddr == 0x01)
    {
        if (engine.voltageUpdated && engine.voltage < WARN_VOLTAGE_LOW_X10)
            setWarn(warnings, prevBits, WARN_LOW_VOLT, 2);
        if (engine.engineLoadUpdated && engine.engineLoad > WARN_ENGINE_LOAD_HIGH)
            setWarn(warnings, prevBits, WARN_HIGH_LOAD, 1);
        // Coolant proxy from group 4: gate once, cache, run all threshold checks
        if (engine.tempUnknown2Updated)
        {
            int16_t t2 = engine.tempUnknown2;
            if (t2 > WARN_COOLANT_HIGH_C)
                setWarn(warnings, prevBits, WARN_COOL_HOT, 3);
            if (t2 < WARN_COOLANT_COLD_C)
                setWarn(warnings, prevBits, WARN_VERY_COLD, 2);
            if (t2 < WARN_COOLANT_WARM_C)
                setWarn(warnings, prevBits, WARN_COLD_ENG, 1);
        }
    }

    // hasNew fires only when new bits appear (not when warnings clear)
    if (warnings.bits != prevBits && (warnings.bits & ~prevBits) != 0)
        warnings.hasNew = true;
}

} // namespace Model
} // namespace obd
