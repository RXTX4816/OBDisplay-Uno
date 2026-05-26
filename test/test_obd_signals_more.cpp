// Combined Unity test runner for host-safe model components.

#include <unity.h>

#include "obd/Model/OBDSignals.h"
#include "obd/Model/DTCStore.h"

using namespace obd::Model;

// ---- OBDSignals tests ----

void test_compute_realistic_trip()
{
    OBDSignals signals;
    signals.reset();

    // Simulate a short trip
    signals.instruments.odometerStart = 1000;   // km
    signals.instruments.odometer = 1050;        // km
    signals.instruments.fuelLevelStart = 60;    // percent
    signals.instruments.fuelLevel = 55;         // percent

    const uint32_t startMs = 0;
    const uint32_t nowMs = 3600UL * 1000UL;     // 1 hour later

    signals.compute(nowMs, startMs);

    // 50 km in 1h -> 50 km/h, burned 5% fuel
    TEST_ASSERT_EQUAL_UINT32(3600, signals.computed.elapsedSecondsSinceStart);
    TEST_ASSERT_EQUAL_UINT16(50, signals.computed.elapsedKmSinceStart);
    TEST_ASSERT_EQUAL_UINT8(5, signals.computed.fuelBurnedSinceStart);

    // Basic sanity checks on derived metrics (exact formulas tested in base test)
    TEST_ASSERT_TRUE(signals.computed.fuelPer100km >= 0.0f);
    TEST_ASSERT_TRUE(signals.computed.fuelPerHour >= 0.0f);
}

void test_update_simulation_changes_values()
{
    OBDSignals signals;
    signals.reset();

    // Call updateSimulation a few times and ensure some values toggle/update
    const uint8_t iterations = 5;
    for (uint8_t i = 0; i < iterations; ++i) {
        signals.updateSimulation();
    }

    // After simulation steps, updated flags should be set and values non-zero
    TEST_ASSERT_TRUE(signals.instruments.vehicleSpeedUpdated);
    TEST_ASSERT_TRUE(signals.instruments.engineRpmUpdated);
    TEST_ASSERT_TRUE(signals.instruments.coolantTempUpdated);
    TEST_ASSERT_TRUE(signals.instruments.oilTempUpdated);
    TEST_ASSERT_TRUE(signals.instruments.oilLevelOkUpdated);
    TEST_ASSERT_TRUE(signals.instruments.fuelLevelUpdated);
}

// ---- DTCStore tests ----

void test_dtc_store_reset()
{
    DTCStore store;

    // After construction/reset, all entries should be initialized to 0xFFFF / 0xFF
    for (uint8_t i = 0; i < DTCStore::MaxCount; ++i) {
        TEST_ASSERT_EQUAL_HEX16(0xFFFF, store.errorAt(i));
        TEST_ASSERT_EQUAL_HEX8(0xFF, store.statusAt(i));
    }
}

void test_dtc_store_set_and_read_back()
{
    DTCStore store;
    store.reset();

    store.set(0, 0x0123, 0x01);
    store.set(1, 0xABCD, 0x80);

    TEST_ASSERT_EQUAL_HEX16(0x0123, store.errorAt(0));
    TEST_ASSERT_EQUAL_HEX8(0x01, store.statusAt(0));

    TEST_ASSERT_EQUAL_HEX16(0xABCD, store.errorAt(1));
    TEST_ASSERT_EQUAL_HEX8(0x80, store.statusAt(1));
}

void test_dtc_store_set_out_of_range_is_ignored()
{
    DTCStore store;
    store.reset();

    // Setting an out-of-range index should not crash and should not modify valid entries
    store.set(DTCStore::MaxCount, 0x0000, 0x00);

    for (uint8_t i = 0; i < DTCStore::MaxCount; ++i) {
        TEST_ASSERT_EQUAL_HEX16(0xFFFF, store.errorAt(i));
        TEST_ASSERT_EQUAL_HEX8(0xFF, store.statusAt(i));
    }
}

void test_dtc_store_overwrite_existing()
{
    DTCStore store;
    store.reset();

    // Set initial values
    store.set(0, 0x1111, 0x11);
    TEST_ASSERT_EQUAL_HEX16(0x1111, store.errorAt(0));
    TEST_ASSERT_EQUAL_HEX8(0x11, store.statusAt(0));

    // Overwrite with new values
    store.set(0, 0x2222, 0x22);
    TEST_ASSERT_EQUAL_HEX16(0x2222, store.errorAt(0));
    TEST_ASSERT_EQUAL_HEX8(0x22, store.statusAt(0));
}

void test_dtc_store_all_slots_fillable()
{
    DTCStore store;
    store.reset();

    // Fill all slots with unique values
    for (uint8_t i = 0; i < DTCStore::MaxCount; ++i) {
        uint16_t error = (uint16_t)(0x1000 + i);
        uint8_t status = (uint8_t)(0x10 + i);
        store.set(i, error, status);
    }

    // Verify all were set correctly
    for (uint8_t i = 0; i < DTCStore::MaxCount; ++i) {
        TEST_ASSERT_EQUAL_HEX16(0x1000 + i, store.errorAt(i));
        TEST_ASSERT_EQUAL_HEX8(0x10 + i, store.statusAt(i));
    }
}

void test_signals_reset_clears_all_values()
{
    OBDSignals signals;
    signals.reset();

    // After reset, all updated flags should be false
    TEST_ASSERT_FALSE(signals.instruments.vehicleSpeedUpdated);
    TEST_ASSERT_FALSE(signals.instruments.engineRpmUpdated);
    TEST_ASSERT_FALSE(signals.instruments.coolantTempUpdated);

    // Core values should be zero
    TEST_ASSERT_EQUAL_UINT16(0, signals.instruments.vehicleSpeed);
    TEST_ASSERT_EQUAL_UINT16(0, signals.instruments.engineRpm);
    TEST_ASSERT_EQUAL_INT8(0, signals.instruments.coolantTemp);
}

void test_signals_zero_time_elapsed()
{
    OBDSignals signals;
    signals.reset();

    signals.instruments.odometerStart = 100;
    signals.instruments.odometer = 150;
    signals.instruments.fuelLevelStart = 80;
    signals.instruments.fuelLevel = 75;

    // Call compute with zero elapsed time
    signals.compute(0, 0);

    TEST_ASSERT_EQUAL_UINT32(0, signals.computed.elapsedSecondsSinceStart);
    TEST_ASSERT_EQUAL_UINT16(50, signals.computed.elapsedKmSinceStart);
    TEST_ASSERT_EQUAL_UINT8(5, signals.computed.fuelBurnedSinceStart);
}

void test_signals_fuel_consumption_calculation()
{
    OBDSignals signals;
    signals.reset();

    // Set up a 100km trip over 2 hours using 10 liters
    signals.instruments.odometerStart = 0;
    signals.instruments.odometer = 100;
    signals.instruments.fuelLevelStart = 50;
    signals.instruments.fuelLevel = 40;

    const uint32_t twoHoursMs = 2 * 3600UL * 1000UL;
    signals.compute(twoHoursMs, 0);

    // Verify basic calculations
    TEST_ASSERT_EQUAL_UINT32(2 * 3600, signals.computed.elapsedSecondsSinceStart);
    TEST_ASSERT_EQUAL_UINT16(100, signals.computed.elapsedKmSinceStart);
    TEST_ASSERT_EQUAL_UINT8(10, signals.computed.fuelBurnedSinceStart);

    // Fuel consumption should be: 10L / 100km = 0.1L/km = 10L/100km
    // fuelPer100km should be approximately 10.0
    TEST_ASSERT_TRUE(signals.computed.fuelPer100km >= 9.0f && signals.computed.fuelPer100km <= 11.0f);

    // fuelPerHour should be approximately 5.0 (10L / 2h)
    TEST_ASSERT_TRUE(signals.computed.fuelPerHour >= 4.0f && signals.computed.fuelPerHour <= 6.0f);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    // OBDSignals tests
    RUN_TEST(test_signals_reset_clears_all_values);
    RUN_TEST(test_signals_zero_time_elapsed);
    RUN_TEST(test_compute_realistic_trip);
    RUN_TEST(test_signals_fuel_consumption_calculation);
    RUN_TEST(test_update_simulation_changes_values);

    // DTCStore tests
    RUN_TEST(test_dtc_store_reset);
    RUN_TEST(test_dtc_store_set_and_read_back);
    RUN_TEST(test_dtc_store_set_out_of_range_is_ignored);
    RUN_TEST(test_dtc_store_overwrite_existing);
    RUN_TEST(test_dtc_store_all_slots_fillable);

    return UNITY_END();
}
