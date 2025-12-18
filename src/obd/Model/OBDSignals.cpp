#include "OBDSignals.h"

namespace obd {
namespace Model {

void ExperimentalGroup::reset()
{
    for (uint8_t i = 0; i < 4; ++i) {
        k[i] = 0;
        v[i] = 123.4f;
        // Reset unit text to "N/A"
        unit[i][0] = 'N';
        unit[i][1] = '/';
        unit[i][2] = 'A';
        unit[i][3] = '\0';
        // Clear remaining characters in buffer
        for (uint8_t j = 4; j < UnitWidth + 1; ++j) {
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
}

void OBDSignals::compute(uint32_t nowMs, uint32_t connectTimeStart)
{
    computed.elapsedSecondsSinceStart = (nowMs - connectTimeStart) / 1000;
    computed.elapsedSecondsSinceStartUpdated = true;

    computed.elapsedKmSinceStart = (instruments.odometer - instruments.odometerStart);
    computed.elapsedKmSinceStartUpdated = true;

    computed.fuelBurnedSinceStart = abs((int)instruments.fuelLevelStart - (int)instruments.fuelLevel);
    computed.fuelBurnedSinceStartUpdated = true;

    if (computed.elapsedKmSinceStart > 0) {
        computed.fuelPer100km = (100.0f / computed.elapsedKmSinceStart) * computed.fuelBurnedSinceStart;
    } else {
        computed.fuelPer100km = 0.0f;
    }
    computed.fuelPer100kmUpdated = true;

    if (computed.elapsedSecondsSinceStart > 0) {
        computed.fuelPerHour = (3600.0f / computed.elapsedSecondsSinceStart) * computed.fuelBurnedSinceStart;
    } else {
        computed.fuelPerHour = 0.0f;
    }
    computed.fuelPerHourUpdated = true;
}

void OBDSignals::updateSimulation()
{
    InstrumentSignals &i = instruments;

    // Helper functions similar to simulate_values_helper in old code
    struct SimHelper {
        static void simulateUint8(uint8_t &val, uint8_t amount, bool &up,
                                  bool &updated, uint8_t maxVal, uint8_t minVal = 0)
        {
            if (up) val += amount; else val -= amount;
            updated = true;
            if (up && val >= maxVal) up = false;
            else if (!up && val <= minVal) up = true;
        }

        static void simulateUint16(uint16_t &val, uint8_t amount, bool &up,
                                   bool &updated, uint16_t maxVal, uint16_t minVal = 0)
        {
            if (up) val += amount; else val -= amount;
            updated = true;
            if (up && val >= maxVal) up = false;
            else if (!up && val <= minVal) up = true;
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
