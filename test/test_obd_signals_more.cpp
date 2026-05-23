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

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

    // OBDSignals
    RUN_TEST(test_compute_realistic_trip);
    RUN_TEST(test_update_simulation_changes_values);

    // DTCStore
    RUN_TEST(test_dtc_store_reset);
    RUN_TEST(test_dtc_store_set_and_read_back);
    RUN_TEST(test_dtc_store_set_out_of_range_is_ignored);

    return UNITY_END();
}
