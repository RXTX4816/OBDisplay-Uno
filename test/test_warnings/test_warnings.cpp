// Warning thresholds: every warning bit at its exact trip point, its level
// (which picks the buzzer pattern), ECU gating and the new-warning latch.
// Thresholds come from Config.h, so the tests follow any retuning there.

#include "../unity_runner.h"

#include "Config.h"
#include "obd/Model/OBDSignals.h"

using namespace obd::Model;

static bool has(const OBDSignals& s, WarnBit b)
{
    return (s.warnings.bits & (1u << b)) != 0;
}

// computeWarnings() with one field freshly updated; all others stale.
static OBDSignals& fresh(OBDSignals& s)
{
    s.reset();
    return s;
}

static void coolant17(OBDSignals& s, uint8_t t)
{
    fresh(s).instruments.coolantTemp = t;
    s.instruments.coolantTempUpdated = true;
    s.computeWarnings(0x17);
}

static void coolant01(OBDSignals& s, uint8_t t)
{
    fresh(s).engine.tempUnknown2 = t;
    s.engine.tempUnknown2Updated = true;
    s.computeWarnings(0x01);
}

void test_coolant_hot_trips_above_threshold_on_both_ecus()
{
    OBDSignals s;
    coolant17(s, WARN_COOLANT_HIGH_C);
    TEST_ASSERT_FALSE(has(s, WARN_COOL_HOT));
    coolant17(s, WARN_COOLANT_HIGH_C + 1);
    TEST_ASSERT_TRUE(has(s, WARN_COOL_HOT));
    TEST_ASSERT_EQUAL_UINT8(3, s.warnings.maxLevel);

    coolant01(s, WARN_COOLANT_HIGH_C);
    TEST_ASSERT_FALSE(has(s, WARN_COOL_HOT));
    coolant01(s, WARN_COOLANT_HIGH_C + 1);
    TEST_ASSERT_TRUE(has(s, WARN_COOL_HOT));
    TEST_ASSERT_EQUAL_UINT8(3, s.warnings.maxLevel);
}

void test_cold_engine_levels_on_both_ecus()
{
    OBDSignals s;
    coolant17(s, WARN_COOLANT_WARM_C);
    TEST_ASSERT_EQUAL_HEX16(0, s.warnings.bits);
    coolant17(s, WARN_COOLANT_WARM_C - 1);
    TEST_ASSERT_TRUE(has(s, WARN_COLD_ENG));
    TEST_ASSERT_FALSE(has(s, WARN_VERY_COLD));
    TEST_ASSERT_EQUAL_UINT8(1, s.warnings.maxLevel);
    coolant17(s, WARN_COOLANT_COLD_C);
    TEST_ASSERT_FALSE(has(s, WARN_VERY_COLD));
    coolant17(s, WARN_COOLANT_COLD_C - 1);
    TEST_ASSERT_TRUE(has(s, WARN_VERY_COLD) && has(s, WARN_COLD_ENG));
    TEST_ASSERT_EQUAL_UINT8(2, s.warnings.maxLevel);

    coolant01(s, WARN_COOLANT_COLD_C - 1);
    TEST_ASSERT_TRUE(has(s, WARN_VERY_COLD) && has(s, WARN_COLD_ENG));
    TEST_ASSERT_FALSE(has(s, WARN_COOL_HOT));
}

void test_normal_operating_temperature_is_quiet()
{
    OBDSignals s;
    for (uint8_t t = WARN_COOLANT_WARM_C; t <= WARN_COOLANT_HIGH_C; ++t)
    {
        coolant17(s, t);
        TEST_ASSERT_EQUAL_HEX16(0, s.warnings.bits);
        coolant01(s, t);
        TEST_ASSERT_EQUAL_HEX16(0, s.warnings.bits);
    }
}

void test_low_voltage()
{
    OBDSignals s;
    fresh(s).engine.voltage = WARN_VOLTAGE_LOW_X10;
    s.engine.voltageUpdated = true;
    s.computeWarnings(0x01);
    TEST_ASSERT_FALSE(has(s, WARN_LOW_VOLT));
    s.engine.voltage = WARN_VOLTAGE_LOW_X10 - 1;
    s.computeWarnings(0x01);
    TEST_ASSERT_TRUE(has(s, WARN_LOW_VOLT));
    TEST_ASSERT_EQUAL_UINT8(2, s.warnings.maxLevel);
}

void test_high_engine_load()
{
    OBDSignals s;
    fresh(s).engine.engineLoad = WARN_ENGINE_LOAD_HIGH;
    s.engine.engineLoadUpdated = true;
    s.computeWarnings(0x01);
    TEST_ASSERT_FALSE(has(s, WARN_HIGH_LOAD));
    s.engine.engineLoad = WARN_ENGINE_LOAD_HIGH + 1;
    s.computeWarnings(0x01);
    TEST_ASSERT_TRUE(has(s, WARN_HIGH_LOAD));
    TEST_ASSERT_EQUAL_UINT8(1, s.warnings.maxLevel);
}

void test_oil_level()
{
    OBDSignals s;
    fresh(s).instruments.oilLevelOk = WARN_OIL_LVL_RAW;
    s.instruments.oilLevelOkUpdated = true;
    s.computeWarnings(0x17);
    TEST_ASSERT_FALSE(has(s, WARN_OIL_LVL));
    s.instruments.oilLevelOk = WARN_OIL_LVL_RAW - 1;
    s.computeWarnings(0x17);
    TEST_ASSERT_TRUE(has(s, WARN_OIL_LVL));
    TEST_ASSERT_EQUAL_UINT8(3, s.warnings.maxLevel);
}

// The oil pressure switch reads 31 when OK; anything else must persist for
// three compute cycles (slosh debounce) before the critical warning fires.
void test_oil_pressure_debounce_and_recovery()
{
    OBDSignals s;
    fresh(s).instruments.oilPressureMinUpdated = true;
    s.instruments.oilPressureMin = 0;
    s.computeWarnings(0x17);
    s.computeWarnings(0x17);
    TEST_ASSERT_FALSE(has(s, WARN_OIL_PRES));
    s.computeWarnings(0x17);
    TEST_ASSERT_TRUE(has(s, WARN_OIL_PRES));
    TEST_ASSERT_EQUAL_UINT8(3, s.warnings.maxLevel);

    s.instruments.oilPressureMin = 31; // recovers one step per good reading
    s.computeWarnings(0x17);
    TEST_ASSERT_FALSE(has(s, WARN_OIL_PRES));
}

void test_oil_pressure_single_glitch_is_ignored()
{
    OBDSignals s;
    fresh(s).instruments.oilPressureMinUpdated = true;
    for (uint8_t i = 0; i < 20; ++i)
    {
        s.instruments.oilPressureMin = (i % 3 == 0) ? 0 : 31;
        s.computeWarnings(0x17);
        TEST_ASSERT_FALSE(has(s, WARN_OIL_PRES));
    }
}

void test_stale_values_raise_nothing()
{
    OBDSignals s;
    fresh(s).instruments.coolantTemp = 200;
    s.instruments.oilTemp = 200;
    s.instruments.oilLevelOk = 0;
    s.engine.voltage = 0;
    s.engine.engineLoad = 100;
    s.engine.tempUnknown2 = 200;
    s.computeWarnings(0x17);
    TEST_ASSERT_EQUAL_HEX16(0, s.warnings.bits);
    s.computeWarnings(0x01);
    TEST_ASSERT_EQUAL_HEX16(0, s.warnings.bits);
}

void test_each_ecu_only_checks_its_own_fields()
{
    OBDSignals s;
    fresh(s).engine.voltage = 0;
    s.engine.voltageUpdated = true;
    s.computeWarnings(0x17); // instrument cluster: voltage is not its signal
    TEST_ASSERT_FALSE(has(s, WARN_LOW_VOLT));

    fresh(s).instruments.oilLevelOk = 0;
    s.instruments.oilLevelOkUpdated = true;
    s.computeWarnings(0x01);
    TEST_ASSERT_FALSE(has(s, WARN_OIL_LVL));
}

void test_max_level_is_the_highest_active()
{
    OBDSignals s;
    fresh(s).engine.engineLoad = 100; // level 1
    s.engine.engineLoadUpdated = true;
    s.engine.voltage = 100; // level 2
    s.engine.voltageUpdated = true;
    s.computeWarnings(0x01);
    TEST_ASSERT_EQUAL_UINT8(2, s.warnings.maxLevel);
    s.engine.tempUnknown2 = 120; // level 3
    s.engine.tempUnknown2Updated = true;
    s.computeWarnings(0x01);
    TEST_ASSERT_EQUAL_UINT8(3, s.warnings.maxLevel);
}

// The buzzer plays for hasNew/newLevel: only on a warning appearing, never
// again while it stays active, and not when it clears.
void test_new_warning_latch_drives_buzzer_once()
{
    OBDSignals s;
    fresh(s).engine.voltage = 100;
    s.engine.voltageUpdated = true;
    s.computeWarnings(0x01);
    TEST_ASSERT_TRUE(s.warnings.hasNew);
    TEST_ASSERT_EQUAL_UINT8(2, s.warnings.newLevel);

    s.warnings.hasNew = false; // consumed by OBDDisplay
    s.warnings.newLevel = 0;
    for (uint8_t i = 0; i < 10; ++i)
        s.computeWarnings(0x01);
    TEST_ASSERT_FALSE(s.warnings.hasNew);

    s.engine.voltage = 140; // clears
    s.computeWarnings(0x01);
    TEST_ASSERT_FALSE(s.warnings.hasNew);
    TEST_ASSERT_EQUAL_HEX16(0, s.warnings.bits);

    s.engine.voltage = 100; // returns → plays again
    s.computeWarnings(0x01);
    TEST_ASSERT_TRUE(s.warnings.hasNew);
}

void test_buzzer_patterns_match_levels()
{
    // Level 1 is screen-only; levels 2 and 3 beep, 3 longer than 2.
    TEST_ASSERT_EQUAL_UINT8(0, BUZZER_BEEP_COUNT[0]);
    TEST_ASSERT_TRUE(BUZZER_BEEP_COUNT[1] > 0);
    TEST_ASSERT_TRUE(BUZZER_BEEP_COUNT[2] >= BUZZER_BEEP_COUNT[1]);
    TEST_ASSERT_TRUE(BUZZER_BEEP_MS[2] >= BUZZER_BEEP_MS[1]);
}

void runTests()
{
    RUN_TEST(test_coolant_hot_trips_above_threshold_on_both_ecus);
    RUN_TEST(test_cold_engine_levels_on_both_ecus);
    RUN_TEST(test_normal_operating_temperature_is_quiet);
    RUN_TEST(test_low_voltage);
    RUN_TEST(test_high_engine_load);
    RUN_TEST(test_oil_level);
    RUN_TEST(test_oil_pressure_debounce_and_recovery);
    RUN_TEST(test_oil_pressure_single_glitch_is_ignored);
    RUN_TEST(test_stale_values_raise_nothing);
    RUN_TEST(test_each_ecu_only_checks_its_own_fields);
    RUN_TEST(test_max_level_is_the_highest_active);
    RUN_TEST(test_new_warning_latch_drives_buzzer_once);
    RUN_TEST(test_buzzer_patterns_match_levels);
}

UNITY_SUITE_MAIN()
