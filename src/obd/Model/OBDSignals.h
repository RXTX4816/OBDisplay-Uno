// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>

namespace obd
{
namespace Model
{

struct ExperimentalGroup
{
    uint8_t k[4] = {0, 0, 0, 0};
    int32_t v[4] = {1234, 1234, 1234, 1234}; // ×10 fixed-point (e.g. 1234 = 123.4)
    // Fixed-size unit strings to avoid dynamic String allocations; initialized to "N/A".
    static constexpr uint8_t UnitWidth = 8; // enough for typical short unit labels
    char unit[4][UnitWidth + 1] = {
        {'N', '/', 'A', '\0'}, {'N', '/', 'A', '\0'}, {'N', '/', 'A', '\0'}, {'N', '/', 'A', '\0'}};

    bool kUpdated = false;
    bool vUpdated = false;
    bool unitUpdated = false;

    uint8_t groupCurrent = 1; // mirrors old group_current
    bool groupSide = false;   // false: 0/1, true: 2/3
    bool groupSideUpdated = false;

    // Jump-to-group digit-entry UI state (managed by OBDDisplay input handler)
    bool grpJumpActive = false;
    uint8_t grpJumpDigits[3] = {0, 0, 0}; // hundreds, tens, units
    uint8_t grpJumpCursor = 0;            // 0=hundreds, 1=tens, 2=units

    void reset();
    void invertGroupSide();
};

struct InstrumentSignals
{
    uint16_t vehicleSpeed = 0;
    bool vehicleSpeedUpdated = false;

    uint16_t engineRpm = 0;
    bool engineRpmUpdated = false;

    uint16_t oilPressureMin = 0;
    bool oilPressureMinUpdated = false;
    uint8_t oilPressureLowCount = 0; // debounce counter for WARN_OIL_PRES

    uint32_t timeEcu = 0;
    bool timeEcuUpdated = false;

    uint32_t odometer = 0;
    bool odometerUpdated = false;
    uint32_t odometerStart = 0;

    uint8_t fuelLevel = 0;
    bool fuelLevelUpdated = false;
    uint8_t fuelLevelStart = 0;

    uint16_t fuelSensorResistance = 0;
    bool fuelSensorResistanceUpdated = false;

    uint8_t ambientTemp = 0;
    bool ambientTempUpdated = false;

    uint8_t coolantTemp = 0;
    bool coolantTempUpdated = false;

    uint8_t oilLevelOk = 0;
    bool oilLevelOkUpdated = false;

    uint8_t oilTemp = 0;
    bool oilTempUpdated = false;
};

struct EngineSignals
{
    uint8_t tempUnknown1 = 0;
    bool tempUnknown1Updated = false;

    int8_t lambda = 0;
    bool lambdaUpdated = false;

    uint8_t basicSettingBits = 0; // group 1 v4: 1=condition met per bit
    bool basicSettingBitsUpdated = false;

    bool exhaustGasRecirculationError = false;
    bool oxygenSensorHeatingError = false;
    bool oxygenSensorError = false;
    bool airConditioningError = false;
    bool secondaryAirInjectionError = false;
    bool evaporativeEmissionsError = false;
    bool catalystHeatingError = false;
    bool catalyticConverter = false;
    bool errorBitsUpdated = false;
    // 8 characters plus null terminator for error bits representation.
    char bitsAsString[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};

    uint16_t pressure = 0;
    bool pressureUpdated = false;

    int16_t tbAngle = 0; // ×10 fixed-point (e.g. 456 = 45.6°)
    bool tbAngleUpdated = false;

    int16_t steeringAngle = 0; // ×10 fixed-point
    bool steeringAngleUpdated = false;

    uint16_t voltage = 0; // ×10 fixed-point (e.g. 123 = 12.3 V)
    bool voltageUpdated = false;

    uint8_t tempUnknown2 = 0;
    bool tempUnknown2Updated = false;

    uint8_t tempUnknown3 = 0;
    bool tempUnknown3Updated = false;

    uint16_t engineLoad = 0;
    bool engineLoadUpdated = false;

    int8_t lambda2 = 0;
    bool lambda2Updated = false;
};

struct ComputedStats
{
    uint32_t elapsedSecondsSinceStart = 0;
    bool elapsedSecondsSinceStartUpdated = false;

    uint16_t elapsedKmSinceStart = 0;
    bool elapsedKmSinceStartUpdated = false;

    uint8_t fuelBurnedSinceStart = 0;
    bool fuelBurnedSinceStartUpdated = false;

    uint16_t fuelPer100km = 0; // ×10 fixed-point (e.g. 83 = 8.3 L/100km)
    bool fuelPer100kmUpdated = false;

    uint16_t fuelPerHour = 0; // ×10 fixed-point (e.g. 45 = 4.5 L/h)
    bool fuelPerHourUpdated = false;

    uint16_t kmRemaining = 0; // estimated range at current consumption rate (capped at 9999)
    bool kmRemainingUpdated = false;
};

// Warning bitmask bits are ordered HIGH→MED→LOW so a forward iteration renders by severity.
// bit 0–2: HIGH  bit 3–6: MED  bit 7–9: LOW
enum WarnBit : uint8_t
{
    WARN_OIL_PRES =
        0, // HIGH: oilPressureMin != 31 (normal=31; low=unknown) sustained 3 reads (0x17)
    WARN_OIL_HOT = 1,   // HIGH: oilTemp > 93°C (0x17)
    WARN_COOL_HOT = 2,  // HIGH: coolantTemp > 93°C (both)
    WARN_OIL_LVL = 3,   // MED:  oilLevelOk == 0 (0x17)
    WARN_LOW_VOLT = 4,  // MED:  engine.voltage < 120 (12.0 V) (0x01)
    WARN_FUEL_CRIT = 5, // MED:  fuelLevel < 4 L (0x17)
    WARN_VERY_COLD = 6, // MED:  coolantTemp < 40°C (both)
    WARN_HIGH_LOAD = 7, // LOW:  engineLoad > 90% (0x01)
    WARN_FUEL_LOW = 8,  // LOW:  fuelLevel < 8 L (0x17)
    WARN_COLD_ENG = 9,  // LOW:  coolantTemp < 75°C (both)
    WARN_COUNT = 10,
};

struct WarningState
{
    uint16_t bits = 0;    // bitmask; bit N = WarnBit N
    uint8_t maxLevel = 0; // 0=none 1=low 2=med 3=high
    bool hasNew = false;  // set when a new warning fires; consumed by OBDDisplay
};

struct OBDSignals
{
    InstrumentSignals instruments;
    EngineSignals engine;
    ExperimentalGroup experimental;
    ComputedStats computed;
    WarningState warnings;

    void reset();
    void compute(uint32_t nowMs, uint32_t connectTimeStart);
    void computeWarnings(uint8_t ecuAddr);
    void updateSimulation();
};

} // namespace Model
} // namespace obd
