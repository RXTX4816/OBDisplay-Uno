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

    // 100 km/h constant speed for 1 hour → 100 km trip, 5 L burned
    signals.instruments.vehicleSpeed = 100;
    signals.instruments.fuelLevelStart = 55;
    signals.instruments.fuelLevelSmoothX8 = (uint16_t)(50u * 8u); // 5 L burned

    // Prime prevComputeMs_ with the start time
    signals.compute(0, 0, 0x17, 0); // first call: initializes prevComputeMs_

    // One big step: 3600 s at 100 km/h
    const uint32_t startMs = 0;
    const uint32_t nowMs = 3600UL * 1000UL;
    signals.compute(nowMs, startMs, 0x17, 0);

    TEST_ASSERT_EQUAL_UINT32(3600, signals.computed.elapsedSecondsSinceStart);
    // 100 km/h × 3600 s = 100 km → tripDistance100 ≈ 10000
    TEST_ASSERT_TRUE(signals.computed.tripDistance100 >= 9990 &&
                     signals.computed.tripDistance100 <= 10010);
    TEST_ASSERT_EQUAL_UINT16(100, signals.computed.elapsedKmSinceStart);

    // fuelBurnedSinceStart: 5L burned (via smoothX8)
    TEST_ASSERT_EQUAL_UINT8(5, signals.computed.fuelBurnedSinceStart);
    // fuelPer100km ×10: 5L/100km → 50
    TEST_ASSERT_TRUE(signals.computed.fuelPer100km >= 45 &&
                     signals.computed.fuelPer100km <= 55);
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

    // Integrator state must be reset to sentinel / zero
    TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, signals.prevComputeMs_);
    TEST_ASSERT_EQUAL_UINT32(0, signals.tripDistAccum_);
    TEST_ASSERT_EQUAL_UINT32(0, signals.computed.tripDistance100);
}

void test_signals_zero_time_elapsed()
{
    OBDSignals signals;
    signals.reset();

    signals.instruments.fuelLevelStart = 80;
    signals.instruments.fuelLevel = 75;
    signals.instruments.fuelLevelSmoothX8 = (uint16_t)(75u * 8u); // 5 L burned

    // Call compute with zero elapsed time (both calls at t=0)
    signals.compute(0, 0, 0x17, 0); // initializes prevComputeMs_
    signals.compute(0, 0, 0x17, 0);

    TEST_ASSERT_EQUAL_UINT32(0, signals.computed.elapsedSecondsSinceStart);
    TEST_ASSERT_EQUAL_UINT32(0, signals.computed.tripDistance100);
    TEST_ASSERT_EQUAL_UINT16(0, signals.computed.elapsedKmSinceStart);
    TEST_ASSERT_EQUAL_UINT8(5, signals.computed.fuelBurnedSinceStart);
}

void test_signals_fuel_consumption_calculation()
{
    OBDSignals signals;
    signals.reset();

    // 100 km/h for 2 hours → 200 km; 10 L burned
    signals.instruments.vehicleSpeed = 100;
    signals.instruments.fuelLevelStart = 50;
    signals.instruments.fuelLevelSmoothX8 = (uint16_t)(40u * 8u); // 10 L burned

    const uint32_t twoHoursMs = 2UL * 3600UL * 1000UL;
    signals.compute(0, 0, 0x17, 0);                  // prime prevComputeMs_
    signals.compute(twoHoursMs, 0, 0x17, 0);

    TEST_ASSERT_EQUAL_UINT32(2 * 3600, signals.computed.elapsedSecondsSinceStart);
    // tripDistance100 ≈ 20000 (200 km)
    TEST_ASSERT_TRUE(signals.computed.tripDistance100 >= 19990 &&
                     signals.computed.tripDistance100 <= 20010);
    TEST_ASSERT_EQUAL_UINT16(200, signals.computed.elapsedKmSinceStart);
    TEST_ASSERT_EQUAL_UINT8(10, signals.computed.fuelBurnedSinceStart);

    // fuelPer100km ×10: 10L/200km = 5.0 L/100km → 50. Range [45, 55].
    TEST_ASSERT_TRUE(signals.computed.fuelPer100km >= 45 &&
                     signals.computed.fuelPer100km <= 55);

    // fuelPerHour ×10: 10L/7200s * 3600 * 10 = 5.0 L/h → 50. Range [45, 55].
    TEST_ASSERT_TRUE(signals.computed.fuelPerHour >= 45 &&
                     signals.computed.fuelPerHour <= 55);
}

void test_speed_integration_accumulates_distance()
{
    OBDSignals signals;
    signals.reset();

    signals.instruments.vehicleSpeed = 100; // km/h
    signals.instruments.fuelLevelSmoothX8 =
        (uint16_t)signals.instruments.fuelLevel * 8u;

    // Prime, then drive 1 hour
    signals.compute(0, 0, 0x17, 0);
    signals.compute(3600UL * 1000UL, 0, 0x17, 0);

    // 100 km/h × 3600 s = 100 km = tripDistance100 10000
    TEST_ASSERT_TRUE(signals.computed.tripDistance100 >= 9990 &&
                     signals.computed.tripDistance100 <= 10010);
    TEST_ASSERT_EQUAL_UINT16(100, signals.computed.elapsedKmSinceStart);
}

void test_fuel_ema_smooths_spike()
{
    OBDSignals signals;
    signals.reset();

    signals.instruments.vehicleSpeed = 0;
    signals.instruments.fuelLevelStart = 40;
    signals.instruments.fuelLevelSmoothX8 = (uint16_t)(40u * 8u); // = 320

    // Inject a +10 L slosh spike
    signals.instruments.fuelLevel = 50;
    signals.instruments.fuelLevelUpdated = true;

    signals.compute(0, 0, 0x17, 0);   // prime
    signals.compute(50, 0, 0x17, 0);  // one 50 ms step

    // After one EMA step: smooth = (320*31 + 400) >> 5 = (9920 + 400) / 32 = 10320/32 = 322
    // i.e. moved only ~2/320 = 0.6% toward the spike
    TEST_ASSERT_TRUE(signals.instruments.fuelLevelSmoothX8 >= 320 &&
                     signals.instruments.fuelLevelSmoothX8 <= 325);
}

void test_fuelper100km_uses_smooth_distance()
{
    OBDSignals signals;
    signals.reset();

    // Directly seed tripDistance100 by driving 60 km before calling compute
    signals.instruments.vehicleSpeed = 100; // km/h
    signals.instruments.fuelLevelStart = 50;
    signals.instruments.fuelLevelSmoothX8 = (uint16_t)(45u * 8u); // 5 L burned

    signals.compute(0, 0, 0x17, 0); // prime
    // 100 km/h × 2160 s = 60 km
    signals.compute(2160UL * 1000UL, 0, 0x17, 0);

    // tripDistance100 ≈ 6000; burnedX8 = 5*8 = 40
    // fuelPer100km = 40 * 12500 / 6000 = 500000/6000 ≈ 83 → 8.3 L/100km
    TEST_ASSERT_TRUE(signals.computed.fuelPer100km >= 75 &&
                     signals.computed.fuelPer100km <= 91);
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
    RUN_TEST(test_speed_integration_accumulates_distance);
    RUN_TEST(test_fuel_ema_smooths_spike);
    RUN_TEST(test_fuelper100km_uses_smooth_distance);

    // DTCStore tests
    RUN_TEST(test_dtc_store_reset);
    RUN_TEST(test_dtc_store_set_and_read_back);
    RUN_TEST(test_dtc_store_set_out_of_range_is_ignored);
    RUN_TEST(test_dtc_store_overwrite_existing);
    RUN_TEST(test_dtc_store_all_slots_fillable);

    return UNITY_END();
}
