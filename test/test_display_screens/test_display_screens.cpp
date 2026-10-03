// Display pipeline on the simulated ATmega328P, part 2: every page of every
// menu rendered with normal and extreme values. Each page must stay within the
// 24-entry text table, keep all text on the 64x128 panel and survive a full
// flush(); the heaviest pages are also measured for stack use.
// AVR only: Display.cpp drives the TWI registers directly.

#include "../unity_runner.h"
#include "../display_helpers.h"

#include "obd/Display/screens/CockpitScreen.h"
#include "obd/Display/screens/DTCScreen.h"
#include "obd/Display/screens/DebugScreen.h"
#include "obd/Display/screens/ExperimentalScreen.h"
#include "obd/Display/screens/SettingsScreen.h"
#include "obd/Model/DTCStore.h"
#include "obd/Model/OBDSignals.h"

using namespace obd::Display;
using namespace obd::Model;

// ---------------------------------------------------------------------------
// Cockpit pages
// ---------------------------------------------------------------------------
static void typical(OBDSignals& s)
{
    s.instruments.vehicleSpeed = 87;
    s.instruments.engineRpm = 2450;
    s.instruments.oilTemp = 96;
    s.instruments.coolantTemp = 90;
    s.instruments.fuelLevelSmoothX8 = 33 * 8;
    s.instruments.oilLevelOk = 180;
    s.computed.fuelPer100km = 74;
    s.computed.fuelPerHour = 52;
    s.computed.kmRemaining = 446;
    s.computed.fuelBurnedSinceStart = 3;
    s.engine.tempUnknown2 = 90;
    s.engine.tempUnknown3 = 24;
    s.engine.engineLoad = 23;
    s.engine.tbAngle = 55;
    s.engine.voltage = 141;
    s.engine.lambda = -3;
    s.engine.lambda2 = 2;
    s.engine.pressure = 380;
}

// Largest values each field can carry from the decoder.
static void extreme(OBDSignals& s)
{
    s.instruments.vehicleSpeed = 255;
    s.instruments.engineRpm = 13005;
    s.instruments.oilTemp = 150;
    s.instruments.coolantTemp = 99;
    s.instruments.fuelLevelSmoothX8 = 255 * 8;
    s.instruments.oilLevelOk = 255;
    s.computed.fuelPer100km = 300;
    s.computed.fuelPerHour = 999;
    s.computed.kmRemaining = 9999;
    s.computed.fuelBurnedSinceStart = 255;
    s.engine.tempUnknown2 = 99;
    s.engine.tempUnknown3 = 255;
    s.engine.engineLoad = 100;
    s.engine.tbAngle = 900;
    s.engine.voltage = 160;
    s.engine.lambda = -100;
    s.engine.lambda2 = -100;
    s.engine.pressure = 2550;
}

// `what` is a flash string naming the case, e.g. PSTR("typical 0x01 page").
static void renderCockpit(uint8_t addr, uint8_t page, const OBDSignals& s, PGM_P what)
{
    beginFrame();
    renderCockpitScreen(dm, page, addr, s, true);
    assertLayout(what, page);
    endFrame();
}

void test_every_cockpit_page_fits_with_typical_values()
{
    OBDSignals s;
    typical(s);
    s.engine.errorBitsUpdated = true;
    s.engine.basicSettingBitsUpdated = true;
    for (uint8_t page = 0; page <= 6; ++page)
        renderCockpit(0x01, page, s, PSTR("typical 0x01 page"));
    for (uint8_t page = 0; page <= 3; ++page)
        renderCockpit(0x17, page, s, PSTR("typical 0x17 page"));
}

void test_every_cockpit_page_fits_with_extreme_values()
{
    OBDSignals s;
    extreme(s);
    s.engine.errorBitsUpdated = true;
    s.engine.basicSettingBitsUpdated = true;
    s.warnings.bits = (1u << WARN_COUNT) - 1; // every warning active
    s.warnings.maxLevel = 3;
    for (uint8_t page = 0; page <= 6; ++page)
        renderCockpit(0x01, page, s, PSTR("extreme 0x01 page"));
    for (uint8_t page = 0; page <= 3; ++page)
        renderCockpit(0x17, page, s, PSTR("extreme 0x17 page"));
}

void test_cockpit_17_main_dashboard_content()
{
    OBDSignals s;
    typical(s);
    beginFrame();
    renderCockpitScreen(dm, 0, 0x17, s, true);
    EXPECT_TEXT(big(0, 0), "87");
    EXPECT_TEXT(big(0, 16), "2450");
    EXPECT_TEXT(big(0, 39), "96 O");
    EXPECT_TEXT(big(0, 55), "90 C");
    EXPECT_TEXT(big(0, 78), "33 L");
    EXPECT_TEXT(big(0, 94), "446K");
    EXPECT_TEXT(big(0, 110), "7.4L");
    endFrame();
}

void test_range_shows_dashes_until_consumption_known()
{
    OBDSignals s;
    typical(s);
    s.computed.fuelPer100km = 0;
    beginFrame();
    renderCockpitScreen(dm, 0, 0x17, s, true);
    EXPECT_TEXT(big(0, 94), "---");
    endFrame();
}

void test_cockpit_01_main_dashboard_content()
{
    OBDSignals s;
    typical(s);
    beginFrame();
    renderCockpitScreen(dm, 0, 0x01, s, true);
    EXPECT_TEXT(big(0, 0), "87");
    EXPECT_TEXT(big(0, 16), "2450");
    EXPECT_TEXT(big(0, 32), "90 C");
    EXPECT_TEXT(big(0, 48), "23%");
    EXPECT_TEXT(big(0, 64), "5.5T");
    EXPECT_TEXT(big(0, 80), "14V");
    EXPECT_TEXT(big(0, 96), "-3%");
    EXPECT_TEXT(big(0, 112), "24I");
    endFrame();
}

void test_hot_coolant_shows_warn_marker()
{
    OBDSignals s;
    typical(s);
    s.instruments.coolantTemp = 100;
    beginFrame();
    renderCockpitScreen(dm, 0, 0x17, s, true);
    TEST_ASSERT_NOT_NULL(big(0, 55));
    EXPECT_TEXT(big(0, 55), "-WARN-");
    endFrame();
}

void test_readiness_and_basic_setting_pages()
{
    OBDSignals s;
    beginFrame();
    renderCockpitScreen(dm, 1, 0x01, s, true);
    EXPECT_TEXT(small(0, 1), "not read");
    endFrame();

    s.engine.errorBitsUpdated = true;
    s.engine.exhaustGasRecirculationError = 1; // bit 7 → first row
    beginFrame();
    renderCockpitScreen(dm, 1, 0x01, s, true);
    EXPECT_TEXT(small(0, 0), "EGR  ");
    EXPECT_TEXT(small(6, 0), "FAIL");
    EXPECT_TEXT(small(6, 7), "PASS");
    endFrame();

    s.engine.basicSettingBitsUpdated = true;
    s.engine.basicSettingBits = 0x01; // only bit 0 → last row
    beginFrame();
    renderCockpitScreen(dm, 2, 0x01, s, true);
    EXPECT_TEXT(small(9, 0), "N");
    EXPECT_TEXT(small(9, 7), "Y");
    endFrame();
}

void test_warning_summary_lists_every_active_warning()
{
    OBDSignals s;
    beginFrame();
    renderCockpitScreen(dm, 3, 0x17, s, true);
    EXPECT_TEXT(small(0, 1), "ALL OK");
    endFrame();

    s.warnings.bits = (1u << WARN_COOL_HOT) | (1u << WARN_FUEL_LOW);
    beginFrame();
    renderCockpitScreen(dm, 3, 0x17, s, true);
    EXPECT_TEXT(small(0, 1), "COOL");
    EXPECT_TEXT(small(5, 1), "HOT");
    EXPECT_TEXT(small(0, 2), "FUEL");
    EXPECT_TEXT(small(5, 2), "LOW");
    endFrame();
}

void test_warning_flash_for_every_warning_and_level()
{
    const char* sev[] = {"ALRT", "CAUT", "CRIT"};
    for (uint8_t level = 1; level <= 3; ++level)
    {
        for (uint8_t w = 0; w < WARN_COUNT; ++w)
        {
            WarningState ws;
            ws.bits = (uint16_t)(1u << w);
            ws.maxLevel = level;
            beginFrame();
            renderWarningFlash(dm, ws, 0);
            assertLayout(PSTR("warning flash"));
            TEST_ASSERT_EQUAL_STRING(sev[level - 1], big(8, 0));
            TEST_ASSERT_EQUAL_STRING(sev[level - 1], big(8, 112));
            endFrame();
        }
    }
}

void test_warning_flash_cycles_through_active_warnings()
{
    WarningState ws;
    ws.bits = (1u << WARN_OIL_PRES) | (1u << WARN_LOW_VOLT);
    ws.maxLevel = 3;
    beginFrame();
    renderWarningFlash(dm, ws, 0);
    EXPECT_TEXT(big(8, 64), "PRES"); // "OIL" / "PRES"
    endFrame();
    beginFrame();
    renderWarningFlash(dm, ws, 1);
    EXPECT_TEXT(big(8, 64), "VOLT"); // "LOW" / "VOLT"
    endFrame();
    beginFrame();
    renderWarningFlash(dm, ws, 2); // wraps back
    EXPECT_TEXT(big(8, 64), "PRES");
    endFrame();
}

void test_unknown_ecu_cockpit()
{
    OBDSignals s;
    beginFrame();
    renderCockpitScreen(dm, 0, 0x56, s, true);
    EXPECT_TEXT(small(0, 0), "Addr 0x");
    EXPECT_TEXT(small(7, 0), "56");
    assertLayout(PSTR("unknown ecu"));
    endFrame();
}

// ---------------------------------------------------------------------------
// Other menus
// ---------------------------------------------------------------------------
void test_experimental_group_view()
{
    OBDSignals s;
    s.experimental.groupCurrent = 255;
    const uint8_t ks[4] = {1, 16, 36, 5};
    const int32_t vs[4] = {130050, 1780, 6553500, -999999};
    for (uint8_t i = 0; i < 4; ++i)
    {
        s.experimental.k[i] = ks[i];
        s.experimental.v[i] = vs[i];
        strcpy(s.experimental.unit[i], "count"); // longest unit
    }
    beginFrame();
    renderExperimentalScreen(dm, 0, s, true);
    assertLayout(PSTR("experimental"));
    EXPECT_TEXT(small(5, 0), "255");
    EXPECT_TEXT(small(3, 2), "13005.0");
    EXPECT_TEXT(small(2, 5), "10110010"); // k=16 binary
    EXPECT_TEXT(small(3, 8), "655350 ");  // k=36 km, no decimal
    endFrame();
}

void test_experimental_jump_view()
{
    OBDSignals s;
    s.experimental.grpJumpActive = true;
    s.experimental.grpJumpCursor = 2;
    s.experimental.grpJumpDigits[0] = 2;
    s.experimental.grpJumpDigits[1] = 5;
    s.experimental.grpJumpDigits[2] = 5;
    beginFrame();
    renderExperimentalScreen(dm, 0, s, true);
    assertLayout(PSTR("jump"));
    EXPECT_TEXT(small(0, 3), " 2  5  [5]");
    EXPECT_TEXT(small(6, 6), "255");
    endFrame();
}

void test_debug_screen()
{
    DebugInfo di{1, 255, 255, 255, 0x17, 10400, 255, -32768};
    beginFrame();
    renderDebugScreen(dm, di, 1);
    assertLayout(PSTR("debug"));
    EXPECT_TEXT(small(4, 5), "0x17");
    EXPECT_TEXT(small(5, 6), "10400");
    EXPECT_TEXT(small(4, 3), "Sensor");
    endFrame();
}

void test_dtc_menu_and_every_show_page()
{
    DTCStore store;
    store.reset();
    for (uint8_t i = 0; i < DTCStore::MaxCount; ++i)
        store.set(i, (uint16_t)(65534u - i), 255);

    beginFrame();
    renderDtcScreen(dm, 1, false, 0, 16, store);
    assertLayout(PSTR("dtc menu"));
    EXPECT_TEXT(small(0, 4), ">Clear");
    EXPECT_TEXT(small(5, 14), "16");
    endFrame();

    for (uint8_t page = 0; page < DTCStore::MaxCount / 4; ++page)
    {
        beginFrame();
        renderDtcScreen(dm, 0, true, page, 16, store);
        assertLayout(PSTR("dtc show"));
        endFrame();
    }
    beginFrame();
    renderDtcScreen(dm, 0, true, 3, 16, store);
    EXPECT_TEXT(small(0, 0), "DTCs 13-16");
    EXPECT_TEXT(small(0, 2), "#13E:65522");
    EXPECT_TEXT(small(0, 3), "   S:255");
    EXPECT_TEXT(small(0, 14), "UD:pg S:bk");
    endFrame();
}

void test_settings_with_fuel_item_and_ecu_lines()
{
    const char lines[4][11] = {"036906034A", "MARELLI 4L", "V   0001  ", "CODING 001"};
    beginFrame();
    renderSettingsScreen(dm, 3, 2, true, lines, 4, 0x17, 255);
    dm.print(0, 9, F("Fuel:Savd")); // status line OBDDisplay adds after saving
    assertLayout(PSTR("settings 0x17"));
    EXPECT_TEXT(small(0, 8), ">");
    EXPECT_TEXT(small(6, 8), "255");
    EXPECT_TEXT(small(0, 14), "CODING 001");
    endFrame();

    beginFrame();
    renderSettingsScreen(dm, 0, 0, false, lines, 4, 0x01, 0);
    assertLayout(PSTR("settings 0x01"));
    EXPECT_TEXT(small(0, 2), ">Exit");
    EXPECT_TEXT(small(9, 6), "N");
    EXPECT_TEXT(small(0, 8), "ECU ID:");
    endFrame();
}

// ---------------------------------------------------------------------------
// Renderer robustness and stack use
// ---------------------------------------------------------------------------
// Text and bars running off every edge: flush() must clip, never write past
// its page buffer (the #57 freeze).
void test_flush_clips_everything_running_off_the_panel()
{
    beginFrame();
    for (uint8_t col = 0; col < 12; ++col)
        dm.print(col, (uint8_t)(col + 4), "WWWWWWWWWW");
    display.printBig(60, 0, "WWWW");
    display.printBig(63, 120, "W");
    display.drawBar(60, 120, 20, 20);
    display.drawBarClear(0, 0, 255, 255);
    display.printBig(0, 64, "\x7f\x01\xff");
    endFrame();
    TEST_PASS();
}

// Same declarations as OBDDisplay.cpp (LTO requires them to match).
extern int __heap_start;
extern int* __brkval;
static const uint8_t kPaint = 0xA5;

// Fill the unused RAM between heap and stack with a pattern; return the stack
// pointer at the time of painting.
static __attribute__((noinline)) uint8_t* paintStack()
{
    uint8_t* top = (uint8_t*)SP;
    uint8_t* p = __brkval ? (uint8_t*)__brkval : (uint8_t*)&__heap_start;
    while (p < top - 8)
        *p++ = kPaint;
    return top;
}

// Deepest stack use below `top` since paintStack().
static __attribute__((noinline)) uint16_t stackUsedBelow(uint8_t* top)
{
    uint8_t* p = __brkval ? (uint8_t*)__brkval : (uint8_t*)&__heap_start;
    while (p < top && *p == kPaint)
        ++p;
    return (uint16_t)(top - p);
}

// The firmware has 2048 - 997 = 1051 bytes for stack (see `pio run -e uno`).
// Rendering and flushing the heaviest pages must leave at least half of it for
// the callers above (loop, OBDDisplay, Controller) and interrupts.
void test_render_and_flush_stack_depth()
{
    OBDSignals s;
    extreme(s);
    s.engine.errorBitsUpdated = true;
    s.warnings.bits = (1u << WARN_COUNT) - 1;
    s.warnings.maxLevel = 3;

    uint8_t* top = paintStack();
    for (uint8_t page = 0; page <= 6; ++page)
    {
        beginFrame();
        renderCockpitScreen(dm, page, 0x01, s, true);
        endFrame();
    }
    beginFrame();
    renderExperimentalScreen(dm, 0, s, true);
    endFrame();
    beginFrame();
    renderWarningFlash(dm, s.warnings, 3);
    endFrame();
    uint16_t used = stackUsedBelow(top);

    char msg[64];
    snprintf(msg, sizeof(msg), "render+flush used %u bytes of stack", used);
    TEST_MESSAGE(msg);
    TEST_ASSERT_TRUE_MESSAGE(used < 1051u / 2u, msg);
}

void runTests()
{
    RUN_TEST(test_every_cockpit_page_fits_with_typical_values);
    RUN_TEST(test_every_cockpit_page_fits_with_extreme_values);
    RUN_TEST(test_cockpit_17_main_dashboard_content);
    RUN_TEST(test_range_shows_dashes_until_consumption_known);
    RUN_TEST(test_cockpit_01_main_dashboard_content);
    RUN_TEST(test_hot_coolant_shows_warn_marker);
    RUN_TEST(test_readiness_and_basic_setting_pages);
    RUN_TEST(test_warning_summary_lists_every_active_warning);
    RUN_TEST(test_warning_flash_for_every_warning_and_level);
    RUN_TEST(test_warning_flash_cycles_through_active_warnings);
    RUN_TEST(test_unknown_ecu_cockpit);
    RUN_TEST(test_experimental_group_view);
    RUN_TEST(test_experimental_jump_view);
    RUN_TEST(test_debug_screen);
    RUN_TEST(test_dtc_menu_and_every_show_page);
    RUN_TEST(test_settings_with_fuel_item_and_ecu_lines);
    RUN_TEST(test_flush_clips_everything_running_off_the_panel);
    RUN_TEST(test_render_and_flush_stack_depth);
}

UNITY_SUITE_MAIN()
