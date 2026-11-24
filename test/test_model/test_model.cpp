// Model tests: OBDSignals computations, warnings and DTCStore.
// Run on the host and on the simulated ATmega328P (16-bit int, avr-libc).

#include "../unity_runner.h"

#include "obd/Model/OBDSignals.h"
#include "obd/Model/DTCStore.h"
#include "Config.h"

#include <string.h>

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

// reset() clears with memset (far smaller than assigning default-constructed
// temporaries on AVR). A non-zero default added to one of these structs must
// show up here. Static: zero-initialised padding, so memcmp is exact on host.
static OBDSignals freshSignals;

void test_signals_reset_restores_every_default()
{
    static OBDSignals s;
    memset(&s.instruments, 0x5A, sizeof s.instruments);
    memset(&s.engine, 0x5A, sizeof s.engine);
    memset(&s.computed, 0x5A, sizeof s.computed);
    memset(&s.warnings, 0x5A, sizeof s.warnings);
    memset(&s.fuelFloor, 0x5A, sizeof s.fuelFloor);
    s.reset();
    TEST_ASSERT_EQUAL_MEMORY(&freshSignals.instruments, &s.instruments, sizeof s.instruments);
    TEST_ASSERT_EQUAL_MEMORY(&freshSignals.engine, &s.engine, sizeof s.engine);
    TEST_ASSERT_EQUAL_MEMORY(&freshSignals.computed, &s.computed, sizeof s.computed);
    TEST_ASSERT_EQUAL_MEMORY(&freshSignals.warnings, &s.warnings, sizeof s.warnings);
    TEST_ASSERT_EQUAL_MEMORY(&freshSignals.fuelFloor, &s.fuelFloor, sizeof s.fuelFloor);
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

// Range and consumption must not follow tank slosh (#64). Speed 0 keeps the
// trip fixed, so only the fuel level can move the results.
static uint32_t fuelClockMs;

static void driveFuelTrip(OBDSignals& s)
{
    fuelClockMs = 3600UL * 1000UL;
    s.reset();
    s.instruments.vehicleSpeed = 100;
    s.instruments.fuelLevelStart = 40;
    s.instruments.fuelLevelSmoothX8 = 30u * 8u; // 10 L burned
    s.compute(0, 0, 0x17, 0);
    s.compute(fuelClockMs, 0, 0x17, 0); // 100 km
    s.instruments.vehicleSpeed = 0;
}

static void holdCompute(OBDSignals& s, uint16_t cycles)
{
    for (uint16_t i = 0; i < cycles; ++i)
    {
        fuelClockMs += 50;
        s.compute(fuelClockMs, 0, 0x17, 0);
    }
}

void test_range_and_consumption_ignore_fuel_slosh()
{
    OBDSignals signals;
    driveFuelTrip(signals);
    const uint16_t range = signals.computed.kmRemaining;
    const uint16_t per100 = signals.computed.fuelPer100km;
    TEST_ASSERT_EQUAL_UINT8(10, signals.computed.fuelBurnedSinceStart);
    TEST_ASSERT_EQUAL_UINT16(100, per100); // 10 L / 100 km
    TEST_ASSERT_EQUAL_UINT16(300, range);  // 30 L at 10 L/100km

    // A corner drags the smoothed level 8 L down for 5 s: nothing moves.
    signals.instruments.fuelLevelSmoothX8 = 22u * 8u;
    holdCompute(signals, 100);
    TEST_ASSERT_EQUAL_UINT16(range, signals.computed.kmRemaining);
    TEST_ASSERT_EQUAL_UINT16(per100, signals.computed.fuelPer100km);
    TEST_ASSERT_EQUAL_UINT8(10, signals.computed.fuelBurnedSinceStart);

    // Back on the settled level for one sample restarts the dwell.
    signals.instruments.fuelLevelSmoothX8 = 30u * 8u;
    holdCompute(signals, 1);
    signals.instruments.fuelLevelSmoothX8 = 22u * 8u;
    holdCompute(signals, FUEL_SETTLE_DWELL_CYCLES - 1);
    TEST_ASSERT_EQUAL_UINT16(range, signals.computed.kmRemaining);
    TEST_ASSERT_EQUAL_UINT16(per100, signals.computed.fuelPer100km);
}

void test_range_follows_sustained_fuel_drop()
{
    OBDSignals signals;
    driveFuelTrip(signals);

    // 5 L really used: held below for the whole dwell, then adopted.
    signals.instruments.fuelLevelSmoothX8 = 25u * 8u;
    holdCompute(signals, FUEL_SETTLE_DWELL_CYCLES - 1);
    TEST_ASSERT_EQUAL_UINT8(10, signals.computed.fuelBurnedSinceStart);
    holdCompute(signals, 1);
    TEST_ASSERT_EQUAL_UINT8(15, signals.computed.fuelBurnedSinceStart);
    TEST_ASSERT_EQUAL_UINT16(150, signals.computed.fuelPer100km); // 15 L / 100 km
    TEST_ASSERT_EQUAL_UINT16(166, signals.computed.kmRemaining);  // 25 L at 15 L/100km
}

void test_settled_fuel_restarts_dwell_when_side_changes()
{
    OBDSignals signals;
    driveFuelTrip(signals);

    // Below for most of the dwell, then above: neither side has lasted long enough.
    signals.instruments.fuelLevelSmoothX8 = 28u * 8u;
    holdCompute(signals, FUEL_SETTLE_DWELL_CYCLES - 1);
    signals.instruments.fuelLevelSmoothX8 = 32u * 8u;
    holdCompute(signals, FUEL_SETTLE_DWELL_CYCLES - 1);
    TEST_ASSERT_EQUAL_UINT16(30u * 8u, signals.instruments.fuelLevelSettledX8);

    // Refuel stays up: adopted after one more cycle.
    holdCompute(signals, 1);
    TEST_ASSERT_EQUAL_UINT16(32u * 8u, signals.instruments.fuelLevelSettledX8);
}

// ---- Fuel floor (#83) ----
// The Golf 4 sender stops at 3 L. 100 km at 9 L/100km brings a 13 L tank to
// the floor; after that, distance must count the estimate down, not the sender.
static void driveToFuelFloor(OBDSignals& s)
{
    fuelClockMs = 3600UL * 1000UL;
    s.reset();
    s.instruments.vehicleSpeed = 100;
    s.instruments.fuelLevelStart = 13;
    s.instruments.fuelLevelSmoothX8 = 4u * 8u;
    s.compute(0, 0, 0x17, 0);
    s.compute(fuelClockMs, 0, 0x17, 0); // 100 km, settled at 4 L
    s.instruments.vehicleSpeed = 0;
    s.instruments.fuelLevelSmoothX8 = FUEL_FLOOR_L * 8u; // reading drops to 3 L
    holdCompute(s, FUEL_SETTLE_DWELL_CYCLES);            // settled follows: floor reached
}

static void driveKm(OBDSignals& s, uint16_t km)
{
    s.instruments.vehicleSpeed = 100;
    fuelClockMs += (uint32_t)km * 36000UL; // 100 km/h: 36 s per km
    s.compute(fuelClockMs, 0, 0x17, 0);
    s.instruments.vehicleSpeed = 0;
}

void test_fuel_floor_counts_down_with_distance()
{
    OBDSignals signals;
    driveToFuelFloor(signals);
    TEST_ASSERT_TRUE(signals.fuelFloor.active());
    TEST_ASSERT_EQUAL_UINT8(24, signals.fuelFloor.leftX8); // assumed 3.0 L
    TEST_ASSERT_EQUAL_UINT16(100, signals.computed.fuelPer100km); // 10 L / 100 km at the floor
    TEST_ASSERT_EQUAL_UINT16(30, signals.computed.kmRemaining);

    // The sender stays at 3 L; 10 km at 10 L/100km use 1.0 L.
    driveKm(signals, 10);
    TEST_ASSERT_EQUAL_UINT8(16, signals.fuelFloor.leftX8); // 2.0 L
    TEST_ASSERT_EQUAL_UINT16(100, signals.computed.fuelPer100km); // frozen, not 80/110 km
    TEST_ASSERT_EQUAL_UINT16(20, signals.computed.kmRemaining);

    // Past the assumed 3.0 L the estimate stays at zero.
    driveKm(signals, 40);
    TEST_ASSERT_EQUAL_UINT8(0, signals.fuelFloor.leftX8);
    TEST_ASSERT_EQUAL_UINT16(0, signals.computed.kmRemaining);
}

void test_fuel_floor_ends_on_refuel_only()
{
    OBDSignals signals;
    driveToFuelFloor(signals);
    driveKm(signals, 10);

    // Sender back to 4 L (float lifted, not a refuel): estimate carries on.
    signals.instruments.fuelLevelSmoothX8 = 4u * 8u;
    holdCompute(signals, FUEL_SETTLE_DWELL_CYCLES);
    TEST_ASSERT_TRUE(signals.fuelFloor.active());
    TEST_ASSERT_EQUAL_UINT8(16, signals.fuelFloor.leftX8);

    // 20 L in the tank for the settle time: back to the sender.
    signals.instruments.fuelLevelSmoothX8 = 20u * 8u;
    holdCompute(signals, FUEL_SETTLE_DWELL_CYCLES);
    TEST_ASSERT_FALSE(signals.fuelFloor.active());
    TEST_ASSERT_EQUAL_UINT16(0, signals.fuelFloor.dist100);
    driveKm(signals, 1);
    TEST_ASSERT_EQUAL_UINT16(0, signals.fuelFloor.dist100);
}

void test_fuel_floor_without_measured_consumption_uses_default()
{
    // Power-on with the tank already at the floor: no trip-measured L/100km.
    OBDSignals signals;
    fuelClockMs = 0;
    signals.reset();
    signals.instruments.fuelLevelStart = FUEL_FLOOR_L;
    signals.instruments.fuelLevelSmoothX8 = FUEL_FLOOR_L * 8u;
    holdCompute(signals, 2);
    TEST_ASSERT_TRUE(signals.fuelFloor.active());
    TEST_ASSERT_EQUAL_UINT16(FUEL_FLOOR_DEFAULT_PER100, signals.computed.fuelPer100km);
    TEST_ASSERT_EQUAL_UINT16(37, signals.computed.kmRemaining); // 3.0 L at 8.0 L/100km

    // Pulling away at 3000 rpm, 18 km/h: the RPM estimate (38 L/100km) must
    // not be frozen for the rest of the tank.
    signals.instruments.engineRpm = 3000;
    signals.instruments.vehicleSpeed = 18;
    holdCompute(signals, 20);
    TEST_ASSERT_EQUAL_UINT16(FUEL_FLOOR_DEFAULT_PER100, signals.computed.fuelPer100km);

    // 10 km at 8.0 L/100km: 0.8 L used.
    signals.instruments.vehicleSpeed = 0;
    signals.fuelFloor.dist100 = 1001;
    holdCompute(signals, 1);
    TEST_ASSERT_EQUAL_UINT8(18, signals.fuelFloor.leftX8); // 2.25 L (1/8 L steps)
}

void test_fuel_level_seeds_from_first_reading_after_failed_poll()
{
    // The seed poll at connect failed: smoothed starts at 0. The first real
    // reading must be taken as is, not climbed to from 0 (which would pass
    // through the floor and start the estimate on a full tank).
    OBDSignals signals;
    fuelClockMs = 0;
    signals.reset();
    signals.instruments.fuelLevel = 30;
    signals.instruments.fuelLevelUpdated = true;
    holdCompute(signals, 1);
    TEST_ASSERT_EQUAL_UINT16(30u * 8u, signals.instruments.fuelLevelSmoothX8);
    TEST_ASSERT_FALSE(signals.fuelFloor.active());
}

static void resumeAt(OBDSignals& s, uint32_t odoNow, uint16_t odoAtSave, uint16_t per100 = 100)
{
    fuelClockMs = 0;
    s.reset();
    s.instruments.odometer = odoNow;
    s.instruments.fuelLevelStart = FUEL_FLOOR_L;
    s.instruments.fuelLevelSmoothX8 = FUEL_FLOOR_L * 8u;
    s.fuelFloor.dist100 = 500; // as loaded from EEPROM: 5 km since the floor
    s.fuelFloor.per100 = per100; // default: 10 L/100km
    s.fuelFloor.odo = odoAtSave;
    s.resumeFuelFloor();
    holdCompute(s, 1);
}

void test_fuel_floor_resumes_after_power_off()
{
    OBDSignals signals;
    resumeAt(signals, 123460, (uint16_t)123460u);
    TEST_ASSERT_TRUE(signals.fuelFloor.active());
    TEST_ASSERT_EQUAL_UINT16(500, signals.fuelFloor.dist100);
    TEST_ASSERT_EQUAL_UINT8(20, signals.fuelFloor.leftX8); // 2.5 L
    TEST_ASSERT_EQUAL_UINT16(25, signals.computed.kmRemaining);

    // One odometer step can be a tick during the last km before power-off.
    resumeAt(signals, 123470, (uint16_t)123460u);
    TEST_ASSERT_EQUAL_UINT16(500, signals.fuelFloor.dist100);
}

void test_fuel_floor_counts_drives_without_display()
{
    // 20 km more on the odometer than on the last save: driven unplugged.
    OBDSignals signals;
    resumeAt(signals, 123480, (uint16_t)123460u);
    TEST_ASSERT_EQUAL_UINT16(2500, signals.fuelFloor.dist100);
    TEST_ASSERT_EQUAL_UINT8(4, signals.fuelFloor.leftX8); // 0.5 L

    // The saved odometer is the low 16 bits: still right across the wrap.
    resumeAt(signals, 65546, (uint16_t)65526u);
    TEST_ASSERT_EQUAL_UINT16(2500, signals.fuelFloor.dist100);

    // Odometer not read yet, now or at the save: nothing to compare.
    resumeAt(signals, 0, (uint16_t)123460u);
    TEST_ASSERT_EQUAL_UINT16(500, signals.fuelFloor.dist100);
    resumeAt(signals, 123460, 0);
    TEST_ASSERT_EQUAL_UINT16(500, signals.fuelFloor.dist100);
}

void test_fuel_floor_rejects_garbage_consumption_from_eeprom()
{
    // A torn write can leave any per100; above 12500 the estimate would divide
    // by zero. Anything above the 30 L/100km cap falls back to the default.
    OBDSignals signals;
    resumeAt(signals, 123460, (uint16_t)123460u, 40000);
    TEST_ASSERT_EQUAL_UINT16(FUEL_FLOOR_DEFAULT_PER100, signals.computed.fuelPer100km);
    TEST_ASSERT_EQUAL_UINT8(21, signals.fuelFloor.leftX8); // 3.0 − 5 km × 8.0 / 100 = 2.6 L
}

// ---- Warning tests ----

static bool oilHot(const OBDSignals& s)
{
    return (s.warnings.bits & (1u << WARN_OIL_HOT)) != 0;
}

static void setOilTemp(OBDSignals& s, int16_t t)
{
    s.instruments.oilTemp = t;
    s.instruments.oilTempUpdated = true;
    s.computeWarnings(0x17);
}

void test_oil_hot_not_raised_up_to_110()
{
    OBDSignals signals;
    signals.reset();

    for (uint8_t t = 90; t <= 110; ++t)
    {
        setOilTemp(signals, t);
        TEST_ASSERT_FALSE(oilHot(signals));
    }
    TEST_ASSERT_FALSE(signals.warnings.hasNew);
}

void test_oil_hot_raised_when_threshold_value_is_skipped()
{
    OBDSignals signals;
    signals.reset();

    setOilTemp(signals, 109);
    TEST_ASSERT_FALSE(oilHot(signals));

    // Jump straight past 111 — must still warn and trigger the flash
    setOilTemp(signals, 115);
    TEST_ASSERT_TRUE(oilHot(signals));
    TEST_ASSERT_TRUE(signals.warnings.hasNew);
    TEST_ASSERT_EQUAL_UINT8(3, signals.warnings.maxLevel);
}

void test_oil_hot_stays_until_below_111()
{
    OBDSignals signals;
    signals.reset();

    setOilTemp(signals, 111);
    TEST_ASSERT_TRUE(oilHot(signals));
    signals.warnings.hasNew = false;

    // Stays active (without re-flashing) while at or above 111
    setOilTemp(signals, 118);
    TEST_ASSERT_TRUE(oilHot(signals));
    setOilTemp(signals, 111);
    TEST_ASSERT_TRUE(oilHot(signals));
    TEST_ASSERT_FALSE(signals.warnings.hasNew);

    // Clears once under 111
    setOilTemp(signals, 110);
    TEST_ASSERT_FALSE(oilHot(signals));
}

static void setOilLevel(OBDSignals &s, uint8_t raw)
{
    s.instruments.oilLevelOk = raw;
    s.instruments.oilLevelOkUpdated = true;
    s.computeWarnings(0x17);
}

void test_oil_level_below_45_percent_is_critical()
{
    OBDSignals signals;
    signals.reset();

    setOilLevel(signals, 115);
    TEST_ASSERT_EQUAL_UINT16(0, signals.warnings.bits);

    setOilLevel(signals, 114);
    TEST_ASSERT_TRUE(signals.warnings.bits & (1u << WARN_OIL_LVL));
    TEST_ASSERT_EQUAL_UINT8(3, signals.warnings.newLevel);
}

static void holdFuel(OBDSignals& s, uint16_t cycles)
{
    for (uint16_t i = 0; i < cycles; ++i)
        s.computeWarnings(0x17);
}

static bool fuelCrit(OBDSignals& s)
{
    return (s.warnings.bits & (1u << WARN_FUEL_CRIT)) != 0;
}

void test_fuel_slosh_does_not_trigger_critical_warning()
{
    OBDSignals signals;
    signals.reset();

    // ~10 L in the tank; a corner drops the sender reading for a few seconds.
    signals.instruments.fuelLevel = 10;
    signals.instruments.fuelLevelUpdated = true;
    signals.instruments.fuelLevelSmoothX8 = 10u * 8u;

    // Sender slosh pulls the smoothed level below the critical threshold, but
    // only for 5 s (100 cycles) — shorter than the dwell, so nothing fires.
    signals.instruments.fuelLevelSmoothX8 = 3u * 8u;
    holdFuel(signals, 100);
    TEST_ASSERT_FALSE(fuelCrit(signals));
    TEST_ASSERT_FALSE(signals.warnings.hasNew);

    // Level recovers — the dwell counter restarts from zero.
    signals.instruments.fuelLevelSmoothX8 = 10u * 8u;
    holdFuel(signals, 1);
    signals.instruments.fuelLevelSmoothX8 = 3u * 8u;
    holdFuel(signals, 100);
    TEST_ASSERT_FALSE(fuelCrit(signals));
}

void test_fuel_critical_fires_when_sustained_and_holds_with_hysteresis()
{
    OBDSignals signals;
    signals.reset();

    signals.instruments.fuelLevel = 3;
    signals.instruments.fuelLevelUpdated = true;
    signals.instruments.fuelLevelSmoothX8 = 3u * 8u;

    holdFuel(signals, WARN_FUEL_DWELL_CYCLES - 1);
    TEST_ASSERT_FALSE(fuelCrit(signals));

    holdFuel(signals, 1);
    TEST_ASSERT_TRUE(fuelCrit(signals));
    TEST_ASSERT_TRUE(signals.warnings.hasNew);
    TEST_ASSERT_EQUAL_UINT8(2, signals.warnings.newLevel);
    signals.warnings.hasNew = false;
    signals.warnings.newLevel = 0;

    // Sloshing back just over the threshold keeps it latched — no re-beep.
    signals.instruments.fuelLevelSmoothX8 = (uint16_t)(WARN_FUEL_CRIT_L * 8u) + 4u;
    holdFuel(signals, 5);
    TEST_ASSERT_TRUE(fuelCrit(signals));
    TEST_ASSERT_FALSE(signals.warnings.hasNew);

    // Refuelling past threshold + hysteresis clears it.
    signals.instruments.fuelLevelSmoothX8 = 20u * 8u;
    holdFuel(signals, 1);
    TEST_ASSERT_FALSE(fuelCrit(signals));
}

void test_new_level_reflects_only_newly_fired_warnings()
{
    OBDSignals signals;
    signals.reset();

    setOilTemp(signals, 115); // OIL HOT, level 3
    TEST_ASSERT_EQUAL_UINT8(3, signals.warnings.newLevel);
    signals.warnings.hasNew = false;
    signals.warnings.newLevel = 0;

    // Fuel low (level 1) fires while OIL HOT is still active
    signals.instruments.fuelLevel = 7;
    signals.instruments.fuelLevelUpdated = true;
    signals.instruments.fuelLevelSmoothX8 = 7u * 8u;
    holdFuel(signals, WARN_FUEL_DWELL_CYCLES - 1);
    signals.warnings.newLevel = 0;
    signals.warnings.hasNew = false;
    setOilTemp(signals, 115);
    TEST_ASSERT_TRUE(signals.warnings.hasNew);
    TEST_ASSERT_EQUAL_UINT8(3, signals.warnings.maxLevel);
    TEST_ASSERT_EQUAL_UINT8(1, signals.warnings.newLevel);
}

void runTests()
{
    // OBDSignals tests
    RUN_TEST(test_signals_reset_clears_all_values);
    RUN_TEST(test_signals_reset_restores_every_default);
    RUN_TEST(test_signals_zero_time_elapsed);
    RUN_TEST(test_compute_realistic_trip);
    RUN_TEST(test_signals_fuel_consumption_calculation);
    RUN_TEST(test_speed_integration_accumulates_distance);
    RUN_TEST(test_fuel_ema_smooths_spike);
    RUN_TEST(test_fuelper100km_uses_smooth_distance);
    RUN_TEST(test_range_and_consumption_ignore_fuel_slosh);
    RUN_TEST(test_range_follows_sustained_fuel_drop);
    RUN_TEST(test_settled_fuel_restarts_dwell_when_side_changes);
    RUN_TEST(test_fuel_floor_counts_down_with_distance);
    RUN_TEST(test_fuel_floor_ends_on_refuel_only);
    RUN_TEST(test_fuel_floor_without_measured_consumption_uses_default);
    RUN_TEST(test_fuel_level_seeds_from_first_reading_after_failed_poll);
    RUN_TEST(test_fuel_floor_resumes_after_power_off);
    RUN_TEST(test_fuel_floor_counts_drives_without_display);
    RUN_TEST(test_fuel_floor_rejects_garbage_consumption_from_eeprom);

    // Warning tests
    RUN_TEST(test_oil_hot_not_raised_up_to_110);
    RUN_TEST(test_oil_hot_raised_when_threshold_value_is_skipped);
    RUN_TEST(test_oil_hot_stays_until_below_111);
    RUN_TEST(test_oil_level_below_45_percent_is_critical);
    RUN_TEST(test_fuel_slosh_does_not_trigger_critical_warning);
    RUN_TEST(test_fuel_critical_fires_when_sustained_and_holds_with_hysteresis);
    RUN_TEST(test_new_level_reflects_only_newly_fired_warnings);

    // DTCStore tests
    RUN_TEST(test_dtc_store_reset);
    RUN_TEST(test_dtc_store_set_and_read_back);
    RUN_TEST(test_dtc_store_set_out_of_range_is_ignored);
    RUN_TEST(test_dtc_store_overwrite_existing);
    RUN_TEST(test_dtc_store_all_slots_fillable);
}

UNITY_SUITE_MAIN()
