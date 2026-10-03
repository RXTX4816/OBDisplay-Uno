// KWP-1281 measurement decoding: every k-type formula, unit strings, the
// experimental group arrays and the mapping into named signal fields.
// Runs on the host (exhaustive over all a,b) and on the simulated ATmega328P
// (edge-value grid), where int is 16 bits and the tables live in flash.

#include "../unity_runner.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "obd/KWP/KWPSensorDecode.h"
#include "obd/Model/OBDSignals.h"

using namespace obd;
using namespace obd::Model;

static const uint8_t kGroup = 7; // arbitrary group shown in the experimental view

// Decode one measurement into slot 0 of the experimental view and return v (×10).
static int32_t decode(uint8_t k, uint8_t a, uint8_t b)
{
    OBDSignals s;
    s.experimental.groupCurrent = kGroup;
    KWP::processKwpMeasurement(0x42, kGroup, 0, k, a, b, s);
    return s.experimental.v[0];
}

// ---------------------------------------------------------------------------
// Reference formulas (VW KWP-1281 measurement table), in plain floating point.
// Independent of the firmware's fixed-point tables: written from the formula
// definitions, not from the implementation.
// ---------------------------------------------------------------------------
static double ref(uint8_t k, double a, double b)
{
    switch (k)
    {
        case 1: return 0.2 * a * b;
        case 2: return 0.002 * a * b;
        case 3: return 0.002 * a * b;
        case 4: return 0.01 * a * fabs(b - 127);
        case 5: return 0.1 * a * (b - 100);
        case 6: return 0.001 * a * b;
        case 7: return 0.01 * a * b;
        case 8: return 0.1 * a * b;
        case 9: return 0.02 * a * (b - 127);
        case 10: return b;
        case 11: return 0.0001 * a * (b - 128) + 1;
        case 12: return 0.001 * a * b;
        case 13: return 0.001 * a * (b - 127);
        case 14: return 0.005 * a * b;
        case 15: return 0.01 * a * b;
        case 16: return 256 * a + b;
        case 17: return 256 * a + b;
        case 18: return 0.04 * a * b;
        case 19: return 0.01 * a * b;
        case 20: return (b - 128) * a / 128;
        case 21: return 0.001 * a * b;
        case 22: return 0.001 * a * b;
        case 23: return a * b / 256;
        case 24: return 0.001 * a * b;
        case 25: return 1.421 * b + a / 182.0;
        case 26: return b - a;
        case 27: return 0.01 * a * fabs(b - 128);
        case 28: return b - a;
        case 29: return (b < a) ? 1 : 2;
        case 30: return a * b / 12.0;
        case 31: return a * b / 2560;
        case 32: return (b > 128) ? b - 256 : b;
        case 33: return (a > 0) ? 100 * b / a : 100 * b;
        case 34: return 0.01 * a * (b - 128);
        case 35: return 0.01 * a * b;
        case 36: return a * 2560 + b * 10;
        case 37: return b;
        case 38: return 0.001 * a * (b - 128);
        case 39: return a * b / 256;
        case 40: return 0.1 * b + 25.5 * a - 400;
        case 41: return 255 * a + b;
        case 42: return 0.1 * b + 25.5 * a - 400;
        case 43: return 0.1 * b + 25.5 * a;
        case 44: return a * 60 + b;
        case 45: return 0.001 * a * b;
        case 46: return (a * b - 3200) * 0.0027;
        case 47: return (b - 128) * a;
        case 48: return 255 * a + b;
        case 49: return 0.025 * a * b;
        case 50: return (a > 0) ? (b - 128) / (0.01 * a) : 0;
        case 51: return (b - 128) * a / 255;
        case 52: return 0.02 * a * b - a;
        case 53: return 1.4222 * b + 0.006 * a - 182.04;
        case 54: return 256 * a + b;
        case 55: return a * b / 200;
        case 56: return 256 * a + b;
        case 57: return 256 * a + b + 65536;
        case 58: return (b > 128) ? 1.0225 * (256 - b) : 1.0225 * b;
        case 59: return (256 * a + b) / 32768;
        case 60: return (256 * a + b) * 0.01;
        case 61: return (a > 0) ? (b - 128) / a : b - 128;
        case 62: return 0.256 * a * b;
        case 64: return a + b;
        case 65: return 0.01 * a * (b - 127);
        case 66: return a * b / 511.12;
        case 67: return 640 * a + 2.5 * b;
        case 68: return (256 * a + b) / 7.365;
        case 69: return (256 * a + b) * 0.3254;
        case 70: return (256 * a + b) * 0.192;
        default: return 0; // 0, 63 and k >= 71: no numeric value
    }
}

// Allowed |v - ref×10|. Fixed-point truncation costs up to one ×10 unit; a few
// types use documented approximations of their coefficient.
static double tolerance(uint8_t k, double ref10)
{
    double tol = 1.0;
    switch (k)
    {
        case 25: tol = 2.0; break;                   // two truncated terms
        case 30: tol = 1.0 + fabs(ref10) * 0.005; break; // c1 0.083 for 1/12
        case 53: tol = 3.0; break;                   // 1.4222 → 1.422, 182.04 → 182.0
        default: break;
    }
    // On AVR double is a 32-bit float: allow its rounding on large references.
    return tol + fabs(ref10) * 4e-6;
}

#ifdef ARDUINO
// Every third value (0, 3, ..., 255) plus the edges around 0, 127/128 and 255:
// ~8000 a,b pairs per type, about 15 s in the simulator.
static const uint8_t kEdges[] = {1, 2, 4, 126, 127, 128, 129, 130, 253, 254};
static const uint16_t kSweepLen = 86 + sizeof(kEdges);
static uint8_t sweepValue(uint16_t i)
{
    return (i < 86) ? (uint8_t)(i * 3) : kEdges[i - 86];
}
#else
static const uint16_t kSweepLen = 256; // host: every a,b combination
static uint8_t sweepValue(uint16_t i)
{
    return (uint8_t)i;
}
#endif

void test_every_formula_matches_reference()
{
    char msg[96];
    for (uint8_t k = 0; k < 71; ++k)
    {
        for (uint16_t ia = 0; ia < kSweepLen; ++ia)
        {
            uint8_t a = sweepValue(ia);
            for (uint16_t ib = 0; ib < kSweepLen; ++ib)
            {
                uint8_t b = sweepValue(ib);
                int32_t v = decode(k, a, b);
                double r = ref(k, a, b) * 10.0;
                if (fabs((double)v - r) > tolerance(k, r))
                {
                    snprintf(msg, sizeof(msg), "k=%u a=%u b=%u: got %ld, want %ld (x10)", k, a, b,
                             (long)v, (long)lround(r));
                    TEST_FAIL_MESSAGE(msg);
                }
            }
        }
    }
}

// Hand-computed values for formulas that were once scaled wrong, so a
// regression reads as a plain number instead of a reference-table diff.
void test_lambda_factor_k11()
{
    TEST_ASSERT_EQUAL_INT32(10, decode(11, 0, 0));    // 1.0 when a=0
    TEST_ASSERT_EQUAL_INT32(20, decode(11, 100, 228)); // 0.0001*100*100 + 1 = 2.0
    TEST_ASSERT_EQUAL_INT32(-22, decode(11, 255, 0));  // 1 - 3.264 = -2.26
}

void test_current_and_power_k40_k42_k43()
{
    // 0.1*b + 25.5*a - 400
    TEST_ASSERT_EQUAL_INT32(1200, decode(40, 20, 100)); // 10 + 510 - 400 = 120.0
    TEST_ASSERT_EQUAL_INT32(-3745, decode(42, 0, 255)); // 25.5 - 400 = -374.5
    // 0.1*b + 25.5*a
    TEST_ASSERT_EQUAL_INT32(255, decode(43, 0, 255));  // 25.5
    TEST_ASSERT_EQUAL_INT32(2560, decode(43, 10, 10)); // 1 + 255 = 256.0
}

void test_sixteen_bit_scaled_k68_k69_k70()
{
    TEST_ASSERT_EQUAL_INT32(347, decode(68, 1, 0)); // 256 / 7.365 = 34.76 deg/s
    TEST_ASSERT_EQUAL_INT32(833, decode(69, 1, 0)); // 256 * 0.3254 = 83.30 bar
    TEST_ASSERT_EQUAL_INT32(491, decode(70, 1, 0)); // 256 * 0.192 = 49.15 m/s2
    TEST_ASSERT_EQUAL_INT32(88981, decode(68, 255, 255)); // 65535 / 7.365 = 8898.2
}

void test_air_mass_k25_includes_a_term()
{
    TEST_ASSERT_EQUAL_INT32(1421, decode(25, 0, 100)); // 142.1 g/s
    TEST_ASSERT_EQUAL_INT32(10, decode(25, 182, 0));   // a/182 = 1.0 g/s
}

void test_k46_keeps_decimal()
{
    TEST_ASSERT_EQUAL_INT32(183, decode(46, 100, 100)); // (10000-3200)*0.0027 = 18.36
}

void test_signed_and_offset_types()
{
    TEST_ASSERT_EQUAL_INT32(-1270, decode(32, 0, 129)); // 129 → -127
    TEST_ASSERT_EQUAL_INT32(1280, decode(32, 0, 128));  // 128 stays positive
    TEST_ASSERT_EQUAL_INT32(-500, decode(5, 10, 50));   // 0.1*10*(50-100) = -50 °C
    TEST_ASSERT_EQUAL_INT32(-100, decode(26, 20, 10));  // b - a
}

void test_division_by_zero_is_guarded()
{
    TEST_ASSERT_EQUAL_INT32(50000, decode(33, 0, 50)); // a=0 → 100*b = 5000.0
    TEST_ASSERT_EQUAL_INT32(0, decode(50, 0, 200));   // a=0 → 0
    TEST_ASSERT_EQUAL_INT32(720, decode(61, 0, 200)); // a=0 → b-128
}

void test_largest_values_do_not_overflow()
{
    // These exceed 16 bits: on AVR any int-width intermediate would wrap.
    TEST_ASSERT_EQUAL_INT32(1310710, decode(57, 255, 255)); // 65535 + 65536
    TEST_ASSERT_EQUAL_INT32(6553500, decode(36, 255, 255)); // 255*2560 + 2550 km
    TEST_ASSERT_EQUAL_INT32(130050, decode(1, 255, 255));   // 13005 rpm
}

void test_unknown_type_decodes_to_zero_with_no_unit()
{
    OBDSignals s;
    s.experimental.groupCurrent = kGroup;
    KWP::processKwpMeasurement(0x42, kGroup, 2, 200, 12, 34, s);
    TEST_ASSERT_EQUAL_UINT8(200, s.experimental.k[2]);
    TEST_ASSERT_EQUAL_INT32(0, s.experimental.v[2]);
    TEST_ASSERT_EQUAL_STRING("", s.experimental.unit[2]);
}

// ---------------------------------------------------------------------------
// Units
// ---------------------------------------------------------------------------
// Like the firmware, start from the "ERR" placeholder that
// KWP1281Session::readSensorsGroup writes before every group read.
static void decodeUnit(uint8_t k, uint8_t b, char* out)
{
    OBDSignals s;
    s.experimental.groupCurrent = kGroup;
    strcpy(s.experimental.unit[1], "ERR");
    KWP::processKwpMeasurement(0x42, kGroup, 1, k, 1, b, s);
    strcpy(out, s.experimental.unit[1]);
}

void test_unit_strings()
{
    struct
    {
        uint8_t k;
        const char* unit;
    } const cases[] = {
        {1, "rpm"},   {2, "%%"},   {4, "ATDC"},   {5, "\xB0" "C"}, {6, "V"},     {7, "km/h"},
        {8, " "},     {12, "Ohm"}, {14, "bar"},   {15, "ms"},      {18, "mbar"}, {19, "l"},
        {24, "A"},    {25, "g/s"}, {26, "C"},     {27, "\xB0"},    {30, "Dk/w"},
        {34, "kW"},   {35, "l/h"}, {36, "km"},    {39, "mg/h"},    {41, "Ah"},   {42, "Kw"},
        {44, "h:m"},  {52, "Nm"},  {54, "count"}, {55, "s"},       {56, "WSC"},  {60, "sec"},
        {62, "S"},    {65, "mm"},  {68, "deg/s"}, {69, "bar"},     {70, "m/s2"}, {0, ""},
        {16, ""},     {63, ""},
    };
    char unit[ExperimentalGroup::UnitWidth + 1];
    char msg[48];
    for (const auto& c : cases)
    {
        decodeUnit(c.k, 1, unit);
        snprintf(msg, sizeof(msg), "unit for k=%u", c.k);
        TEST_ASSERT_EQUAL_STRING_MESSAGE(c.unit, unit, msg);
    }
}

void test_warm_cold_unit_follows_b()
{
    char unit[ExperimentalGroup::UnitWidth + 1];
    decodeUnit(10, 1, unit);
    TEST_ASSERT_EQUAL_STRING("WARM", unit);
    decodeUnit(10, 0, unit);
    TEST_ASSERT_EQUAL_STRING("COLD", unit);
}

void test_every_unit_fits_and_is_terminated()
{
    for (uint16_t k = 0; k < 256; ++k)
    {
        OBDSignals s;
        s.experimental.groupCurrent = kGroup;
        memset(s.experimental.unit[0], 'X', sizeof(s.experimental.unit[0]));
        KWP::processKwpMeasurement(0x42, kGroup, 0, (uint8_t)k, 1, 1, s);
        TEST_ASSERT_EQUAL_CHAR('\0', s.experimental.unit[0][ExperimentalGroup::UnitWidth]);
        // The group view prints the unit after "k:NN", leaving 5 of 10 columns.
        TEST_ASSERT_TRUE(strlen(s.experimental.unit[0]) <= 5);
    }
}

// The firmware resets every slot to "ERR" before decoding a group
// (KWP1281Session::readSensorsGroup); a new unit then always replaces the old
// one, even when both start with the same letter.
void test_unit_replaced_after_group_reset()
{
    OBDSignals s;
    s.experimental.groupCurrent = kGroup;
    KWP::processKwpMeasurement(0x42, kGroup, 0, 18, 1, 1, s); // mbar
    TEST_ASSERT_EQUAL_STRING("mbar", s.experimental.unit[0]);
    strcpy(s.experimental.unit[0], "ERR");
    KWP::processKwpMeasurement(0x42, kGroup, 0, 15, 1, 1, s); // ms
    TEST_ASSERT_EQUAL_STRING("ms", s.experimental.unit[0]);
}

// ---------------------------------------------------------------------------
// Experimental view bookkeeping
// ---------------------------------------------------------------------------
void test_other_group_leaves_experimental_view_untouched()
{
    OBDSignals s;
    s.experimental.groupCurrent = 5;
    KWP::processKwpMeasurement(0x17, 1, 0, 7, 100, 88, s); // group 1 while viewing 5
    TEST_ASSERT_EQUAL_UINT8(0, s.experimental.k[0]);
    TEST_ASSERT_EQUAL_INT32(1234, s.experimental.v[0]);
    TEST_ASSERT_FALSE(s.experimental.vUpdated);
    // ...but the named signal is still updated for the cockpit
    TEST_ASSERT_EQUAL_UINT16(88, s.instruments.vehicleSpeed);
    TEST_ASSERT_TRUE(s.instruments.vehicleSpeedUpdated);
}

void test_updated_flags_only_on_change()
{
    OBDSignals s;
    s.experimental.groupCurrent = kGroup;
    KWP::processKwpMeasurement(0x42, kGroup, 3, 7, 100, 50, s);
    TEST_ASSERT_TRUE(s.experimental.kUpdated);
    TEST_ASSERT_TRUE(s.experimental.vUpdated);
    TEST_ASSERT_EQUAL_INT32(500, s.experimental.v[3]);

    s.experimental.kUpdated = s.experimental.vUpdated = false;
    KWP::processKwpMeasurement(0x42, kGroup, 3, 7, 100, 50, s);
    TEST_ASSERT_FALSE(s.experimental.kUpdated);
    TEST_ASSERT_FALSE(s.experimental.vUpdated);

    KWP::processKwpMeasurement(0x42, kGroup, 3, 7, 100, 51, s);
    TEST_ASSERT_FALSE(s.experimental.kUpdated);
    TEST_ASSERT_TRUE(s.experimental.vUpdated);
}

// ---------------------------------------------------------------------------
// Mapping into named signals (what the cockpit pages and warnings read)
// ---------------------------------------------------------------------------
static void feed(OBDSignals& s, uint8_t ecu, uint8_t group, int idx, uint8_t k, uint8_t a,
                 uint8_t b)
{
    KWP::processKwpMeasurement(ecu, group, idx, k, a, b, s);
}

void test_instruments_0x17_mapping()
{
    OBDSignals s;
    s.experimental.groupCurrent = 0; // no experimental view: mapping only
    feed(s, 0x17, 1, 0, 7, 100, 88);  // 0.01*a*b km/h
    feed(s, 0x17, 1, 1, 1, 200, 20);  // 0.2*a*b rpm
    feed(s, 0x17, 1, 2, 37, 0, 31);   // raw oil pressure switch
    feed(s, 0x17, 1, 3, 44, 2, 30);   // h:m → minutes
    feed(s, 0x17, 2, 0, 36, 12, 34);  // km
    feed(s, 0x17, 2, 1, 19, 100, 45); // 0.01*a*b l
    feed(s, 0x17, 2, 2, 12, 100, 90); // 0.001*a*b Ohm
    feed(s, 0x17, 2, 3, 5, 10, 118);  // 0.1*a*(b-100) °C
    feed(s, 0x17, 3, 0, 5, 10, 190);
    feed(s, 0x17, 3, 1, 37, 0, 200);
    feed(s, 0x17, 3, 2, 5, 10, 195);

    TEST_ASSERT_EQUAL_UINT16(88, s.instruments.vehicleSpeed);
    TEST_ASSERT_EQUAL_UINT16(800, s.instruments.engineRpm);
    TEST_ASSERT_EQUAL_UINT16(31, s.instruments.oilPressureMin);
    TEST_ASSERT_EQUAL_UINT32(150, s.instruments.timeEcu);
    TEST_ASSERT_EQUAL_UINT32(31060, s.instruments.odometer);
    TEST_ASSERT_EQUAL_UINT8(45, s.instruments.fuelLevel);
    TEST_ASSERT_EQUAL_UINT16(9, s.instruments.fuelSensorResistance);
    TEST_ASSERT_EQUAL_UINT8(18, s.instruments.ambientTemp);
    TEST_ASSERT_EQUAL_UINT8(90, s.instruments.coolantTemp);
    TEST_ASSERT_EQUAL_UINT8(200, s.instruments.oilLevelOk);
    TEST_ASSERT_EQUAL_UINT8(95, s.instruments.oilTemp);
    TEST_ASSERT_TRUE(s.instruments.vehicleSpeedUpdated && s.instruments.engineRpmUpdated &&
                     s.instruments.odometerUpdated && s.instruments.fuelLevelUpdated &&
                     s.instruments.coolantTempUpdated && s.instruments.oilTempUpdated);
}

void test_engine_0x01_mapping()
{
    OBDSignals s;
    s.experimental.groupCurrent = 0;
    feed(s, 0x01, 1, 0, 1, 40, 100);   // 800 rpm
    feed(s, 0x01, 1, 1, 5, 10, 185);   // 85 °C
    feed(s, 0x01, 1, 2, 20, 128, 118); // (b-128)*a/128 = -10 %
    feed(s, 0x01, 1, 3, 16, 0, 0xB2);  // basic-setting bits
    feed(s, 0x01, 3, 1, 18, 253, 100); // 0.04*a*b = 1012 mbar
    feed(s, 0x01, 3, 2, 3, 11, 250);   // 0.002*a*b = 5.5°, kept ×10
    feed(s, 0x01, 3, 3, 9, 50, 100);   // 0.02*a*(b-127) = -27.0°, kept ×10
    feed(s, 0x01, 4, 1, 6, 117, 100);  // 11.7 V, kept ×10
    feed(s, 0x01, 4, 2, 5, 10, 190);   // 90 °C
    feed(s, 0x01, 4, 3, 5, 10, 114);   // 14 °C
    feed(s, 0x01, 6, 3, 20, 128, 127); // -1 %

    TEST_ASSERT_EQUAL_UINT16(800, s.instruments.engineRpm);
    TEST_ASSERT_EQUAL_UINT8(85, s.engine.tempUnknown1);
    TEST_ASSERT_EQUAL_INT8(-10, s.engine.lambda);
    TEST_ASSERT_EQUAL_HEX8(0xB2, s.engine.basicSettingBits);
    TEST_ASSERT_EQUAL_UINT16(1012, s.engine.pressure);
    TEST_ASSERT_EQUAL_INT16(55, s.engine.tbAngle);
    TEST_ASSERT_EQUAL_INT16(-270, s.engine.steeringAngle);
    TEST_ASSERT_EQUAL_UINT16(117, s.engine.voltage);
    TEST_ASSERT_EQUAL_UINT8(90, s.engine.tempUnknown2);
    TEST_ASSERT_EQUAL_UINT8(14, s.engine.tempUnknown3);
    TEST_ASSERT_EQUAL_INT8(-1, s.engine.lambda2);
}

void test_engine_load_and_speed_groups_5_and_6()
{
    OBDSignals s;
    s.experimental.groupCurrent = 0;
    feed(s, 0x01, 5, 1, 2, 100, 125); // 0.002*a*b = 25.0 %
    feed(s, 0x01, 5, 2, 7, 100, 50);  // 50 km/h
    TEST_ASSERT_EQUAL_UINT16(25, s.engine.engineLoad);
    TEST_ASSERT_EQUAL_UINT16(50, s.instruments.vehicleSpeed);
    feed(s, 0x01, 6, 1, 2, 100, 200); // 40.0 %
    TEST_ASSERT_EQUAL_UINT16(40, s.engine.engineLoad);
}

void test_readiness_bits_group_100()
{
    OBDSignals s;
    s.experimental.groupCurrent = 0;
    feed(s, 0x01, 100, 0, 16, 0, 0xA5); // 1010 0101
    TEST_ASSERT_TRUE(s.engine.errorBitsUpdated);
    TEST_ASSERT_EQUAL(1, s.engine.exhaustGasRecirculationError);
    TEST_ASSERT_EQUAL(0, s.engine.oxygenSensorHeatingError);
    TEST_ASSERT_EQUAL(1, s.engine.oxygenSensorError);
    TEST_ASSERT_EQUAL(0, s.engine.airConditioningError);
    TEST_ASSERT_EQUAL(0, s.engine.secondaryAirInjectionError);
    TEST_ASSERT_EQUAL(1, s.engine.evaporativeEmissionsError);
    TEST_ASSERT_EQUAL(0, s.engine.catalystHeatingError);
    TEST_ASSERT_EQUAL(1, s.engine.catalyticConverter);
}

void test_unmapped_ecu_touches_no_named_signal()
{
    OBDSignals s;
    s.experimental.groupCurrent = 0;
    feed(s, 0x42, 1, 0, 7, 100, 88);
    TEST_ASSERT_FALSE(s.instruments.vehicleSpeedUpdated);
    TEST_ASSERT_EQUAL_UINT16(0, s.instruments.vehicleSpeed);
}

void runTests()
{
    RUN_TEST(test_every_formula_matches_reference);
    RUN_TEST(test_lambda_factor_k11);
    RUN_TEST(test_current_and_power_k40_k42_k43);
    RUN_TEST(test_sixteen_bit_scaled_k68_k69_k70);
    RUN_TEST(test_air_mass_k25_includes_a_term);
    RUN_TEST(test_k46_keeps_decimal);
    RUN_TEST(test_signed_and_offset_types);
    RUN_TEST(test_division_by_zero_is_guarded);
    RUN_TEST(test_largest_values_do_not_overflow);
    RUN_TEST(test_unknown_type_decodes_to_zero_with_no_unit);
    RUN_TEST(test_unit_strings);
    RUN_TEST(test_warm_cold_unit_follows_b);
    RUN_TEST(test_every_unit_fits_and_is_terminated);
    RUN_TEST(test_unit_replaced_after_group_reset);
    RUN_TEST(test_other_group_leaves_experimental_view_untouched);
    RUN_TEST(test_updated_flags_only_on_change);
    RUN_TEST(test_instruments_0x17_mapping);
    RUN_TEST(test_engine_0x01_mapping);
    RUN_TEST(test_engine_load_and_speed_groups_5_and_6);
    RUN_TEST(test_readiness_bits_group_100);
    RUN_TEST(test_unmapped_ecu_touches_no_named_signal);
}

UNITY_SUITE_MAIN()
