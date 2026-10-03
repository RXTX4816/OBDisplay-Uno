// Display pipeline on the simulated ATmega328P, part 1: the text table,
// number formatting and every ScreenVM opcode. AVR only: Display.cpp drives
// the TWI registers directly (the writes go to an empty simulated bus).

#include "../unity_runner.h"
#include "../display_helpers.h"

#include "obd/Display/ScreenVM.h"
#include "obd/Model/OBDSignals.h"

using namespace obd::Display;
using namespace obd::Model;

// ---------------------------------------------------------------------------
// Text table
// ---------------------------------------------------------------------------
void test_print_places_text_on_column_grid()
{
    beginFrame();
    dm.print(3, 5, "ABC");
    EXPECT_TEXT(small(3, 5), "ABC");
    endFrame();
}

void test_text_longer_than_ten_chars_is_cut()
{
    beginFrame();
    dm.print(0, 0, "0123456789ABC");
    EXPECT_TEXT(small(0, 0), "0123456789");
    endFrame();
}

void test_table_full_drops_extra_entries_without_overflow()
{
    beginFrame();
    for (uint8_t i = 0; i < 40; ++i)
        dm.print(0, (uint8_t)(i % 16), "X");
    TEST_ASSERT_EQUAL_UINT8(DTA::maxEntries(), DTA::count(display));
    endFrame();
}

void test_clear_empties_the_table()
{
    beginFrame();
    dm.print(0, 0, "A");
    display.clear();
    TEST_ASSERT_EQUAL_UINT8(0, DTA::count(display));
    endFrame();
}

void test_nothing_is_sent_inside_a_batch()
{
    beginFrame();
    dm.print(0, 0, "A");
    TEST_ASSERT_TRUE(DTA::dirty(display)); // still pending
    endFrame();                            // flushed exactly here
}

// ---------------------------------------------------------------------------
// Number formatting
// ---------------------------------------------------------------------------
void test_scaled_value_formatting()
{
    beginFrame();
    dm.print(0, 0, (int32_t)123, 1);
    dm.print(0, 1, (int32_t)-5, 1);
    dm.print(0, 2, (int32_t)0, 1);
    dm.print(0, 3, (int32_t)-1234, 1);
    dm.print(0, 4, (int32_t)70, 1, 6);  // padded to width
    dm.print(0, 5, (int32_t)1310710, 1); // above 16 bits
    EXPECT_TEXT(small(0, 0), "12.3");
    EXPECT_TEXT(small(0, 1), "-0.5");
    EXPECT_TEXT(small(0, 2), "0.0");
    EXPECT_TEXT(small(0, 3), "-123.4");
    EXPECT_TEXT(small(0, 4), "7.0   ");
    EXPECT_TEXT(small(0, 5), "131071.0");
    endFrame();
}

void test_width_is_clamped_to_screen_columns()
{
    beginFrame();
    dm.print(0, 0, "AB", 15);
    EXPECT_TEXT(small(0, 0), "AB        ");
    endFrame();
}

void test_big_number_helpers()
{
    beginFrame();
    dm.printBig(0, 0, (uint16_t)65535);
    dm.printBig(0, 16, (int16_t)-10, '%');
    dm.printBigWithLabel(0, 32, 90, " C");
    dm.printBigScaled10(0, 48, 55, 'T');
    dm.printBigScaled10(0, 64, -270, 'T');
    dm.printBigVoltage(0, 80, 117);
    EXPECT_TEXT(big(0, 0), "65535");
    EXPECT_TEXT(big(0, 16), "-10%");
    EXPECT_TEXT(big(0, 32), "90 C");
    EXPECT_TEXT(big(0, 48), "5.5T");
    EXPECT_TEXT(big(0, 64), "-27.0T");
    EXPECT_TEXT(big(0, 80), "11V");
    endFrame();
}

void test_scaled_big_out_of_range_shows_err()
{
    beginFrame();
    dm.printBigScaled10(0, 0, 9990, 'L');
    dm.printBigScaled10(0, 16, 9991, 'L');
    dm.printBigScaled10(0, 32, -32768, 'L');
    EXPECT_TEXT(big(0, 0), "999.0L");
    EXPECT_TEXT(big(0, 16), "ERR");
    EXPECT_TEXT(big(0, 32), "ERR");
    endFrame();
}

void test_label_is_cut_to_buffer()
{
    beginFrame();
    dm.printBigWithLabel(0, 0, 32767, "ABCDEFGHIJ");
    dm.printBigWithLabel(0, 16, -40, " C"); // signed: sub-zero temperatures
    EXPECT_TEXT(big(0, 0), "32767ABCDE"); // 10 chars kept
    EXPECT_TEXT(big(0, 16), "-40 C");
    endFrame();
}

// ---------------------------------------------------------------------------
// ScreenVM opcodes
// ---------------------------------------------------------------------------
// clang-format off
static const uint8_t PROGMEM kOpScript[] = {
    SO_LABEL,    0, 0, 3, 'A', 'B', 'C',
    SO_U8,       0, 1, FLD_COOLANT_T,
    SO_U16,      0, 2, FLD_ENG_RPM,
    SO_U32,      0, 3, FLD_ODOMETER,
    SO_I8,       0, 4, FLD_LAMBDA,
    SO_I16,      0, 5, FLD_STEER_ANGLE,
    SO_SCALED,   0, 6, FLD_VOLTAGE, 5,
    SO_STR,      0, 7, FLD_EXP_U0,
    SO_BOOL_YN,  0, 8, FLD_VEH_SPEED,
    SO_BOOL_YN,  2, 8, FLD_FUEL_LVL,
    SO_HEX_U8,   0, 9, FLD_EXP_K0,
    SO_BIN_U8,   0, 10, FLD_TEMP3,
    SO_CURSOR,   0, 11, 1, 2, 'O', 'N',
    SO_CURSOR,   0, 12, 2, 3, 'O', 'F', 'F',
    SO_MODE_STR, 0, 13, FLD_KWP_MODE,
    SO_END
};
// clang-format on

void test_every_screen_vm_opcode()
{
    OBDSignals s;
    s.instruments.coolantTemp = 90;
    s.instruments.engineRpm = 13005;
    s.instruments.odometer = 299999;
    s.engine.lambda = -12;
    s.engine.steeringAngle = -901;
    s.engine.voltage = 141;
    strcpy(s.experimental.unit[0], "km/h");
    s.instruments.vehicleSpeed = 1;
    s.instruments.fuelLevel = 0;
    s.experimental.k[0] = 0xAB;
    s.engine.tempUnknown3 = 0xB2;
    ScreenCtx ctx{&s, nullptr, 1, 2};

    beginFrame();
    runScript(kOpScript, ctx, dm);
    EXPECT_TEXT(small(0, 0), "ABC");
    EXPECT_TEXT(small(0, 1), "90");
    EXPECT_TEXT(small(0, 2), "13005");
    EXPECT_TEXT(small(0, 3), "299999");
    EXPECT_TEXT(small(0, 4), "-12");
    EXPECT_TEXT(small(0, 5), "-901");
    EXPECT_TEXT(small(0, 6), "14.1 ");
    EXPECT_TEXT(small(0, 7), "km/h");
    EXPECT_TEXT(small(0, 8), "Y");
    EXPECT_TEXT(small(2, 8), "N");
    EXPECT_TEXT(small(0, 9), "0xab");
    EXPECT_TEXT(small(0, 10), "10110010");
    EXPECT_TEXT(small(0, 11), ">ON");
    EXPECT_TEXT(small(0, 12), " OFF");
    EXPECT_TEXT(small(0, 13), "Grp");
    endFrame();
}

void test_mode_string_for_each_kwp_mode()
{
    static const uint8_t PROGMEM script[] = {SO_MODE_STR, 0, 0, FLD_KWP_MODE, SO_END};
    const char* expected[] = {"ACK", "Sensor", "Grp"};
    for (uint8_t mode = 0; mode < 3; ++mode)
    {
        ScreenCtx ctx{nullptr, nullptr, 0, mode};
        beginFrame();
        runScript(script, ctx, dm);
        TEST_ASSERT_EQUAL_STRING(expected[mode], small(0, 0));
        endFrame();
    }
}

void test_signal_fields_without_signals_print_zero()
{
    static const uint8_t PROGMEM script[] = {SO_U16, 0, 0, FLD_ENG_RPM, SO_STR, 0, 1, FLD_EXP_U0,
                                             SO_END};
    ScreenCtx ctx{nullptr, nullptr, 0, 0};
    beginFrame();
    runScript(script, ctx, dm);
    EXPECT_TEXT(small(0, 0), "0");
    EXPECT_TEXT(small(0, 1), "");
    endFrame();
}

void runTests()
{
    RUN_TEST(test_print_places_text_on_column_grid);
    RUN_TEST(test_text_longer_than_ten_chars_is_cut);
    RUN_TEST(test_table_full_drops_extra_entries_without_overflow);
    RUN_TEST(test_clear_empties_the_table);
    RUN_TEST(test_nothing_is_sent_inside_a_batch);
    RUN_TEST(test_scaled_value_formatting);
    RUN_TEST(test_width_is_clamped_to_screen_columns);
    RUN_TEST(test_big_number_helpers);
    RUN_TEST(test_scaled_big_out_of_range_shows_err);
    RUN_TEST(test_label_is_cut_to_buffer);
    RUN_TEST(test_every_screen_vm_opcode);
    RUN_TEST(test_mode_string_for_each_kwp_mode);
    RUN_TEST(test_signal_fields_without_signals_print_zero);
}

UNITY_SUITE_MAIN()
