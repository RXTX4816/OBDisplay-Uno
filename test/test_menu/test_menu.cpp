// MenuState: menu and screen navigation with wrap-around, change flags.

#include "../unity_runner.h"

#include "obd/Input/MenuState.h"

using obd::Display::MenuId;
using obd::Input::MenuState;

void test_starts_on_first_cockpit_screen()
{
    MenuState m;
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MenuId::Cockpit, (uint8_t)m.currentMenu());
    for (uint8_t id = 0; id < 5; ++id)
        TEST_ASSERT_EQUAL_UINT8(0, m.screen((MenuId)id));
    TEST_ASSERT_FALSE(m.consumeMenuChanged());
    TEST_ASSERT_FALSE(m.consumeScreenChanged());
}

void test_next_menu_cycles_through_all_five_and_wraps()
{
    MenuState m;
    const MenuId order[] = {MenuId::Experimental, MenuId::Debug, MenuId::Dtc, MenuId::Settings,
                            MenuId::Cockpit};
    for (MenuId expected : order)
    {
        m.nextMenu();
        TEST_ASSERT_EQUAL_UINT8((uint8_t)expected, (uint8_t)m.currentMenu());
    }
}

void test_prev_menu_wraps_from_first_to_last()
{
    MenuState m;
    m.prevMenu();
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MenuId::Settings, (uint8_t)m.currentMenu());
    m.prevMenu();
    TEST_ASSERT_EQUAL_UINT8((uint8_t)MenuId::Dtc, (uint8_t)m.currentMenu());
}

void test_menu_change_flag_is_consumed_once()
{
    MenuState m;
    m.nextMenu();
    TEST_ASSERT_TRUE(m.consumeMenuChanged());
    TEST_ASSERT_FALSE(m.consumeMenuChanged());
    m.markMenuChanged();
    TEST_ASSERT_TRUE(m.consumeMenuChanged());
}

void test_cockpit_screens_wrap_at_max()
{
    MenuState m;
    m.setCockpitMax(6);
    for (uint8_t i = 1; i <= 6; ++i)
    {
        m.nextScreen(MenuId::Cockpit);
        TEST_ASSERT_EQUAL_UINT8(i, m.screen(MenuId::Cockpit));
    }
    m.nextScreen(MenuId::Cockpit);
    TEST_ASSERT_EQUAL_UINT8(0, m.screen(MenuId::Cockpit));
    m.prevScreen(MenuId::Cockpit);
    TEST_ASSERT_EQUAL_UINT8(6, m.screen(MenuId::Cockpit));
}

// The group view covers the full uint8 range: 255 → 0 must wrap without
// the increment overflowing into a stuck value.
void test_experimental_group_wraps_over_full_byte_range()
{
    MenuState m;
    m.setScreen(MenuId::Experimental, 254);
    m.nextScreen(MenuId::Experimental);
    TEST_ASSERT_EQUAL_UINT8(255, m.screen(MenuId::Experimental));
    m.nextScreen(MenuId::Experimental);
    TEST_ASSERT_EQUAL_UINT8(0, m.screen(MenuId::Experimental));
    m.prevScreen(MenuId::Experimental);
    TEST_ASSERT_EQUAL_UINT8(255, m.screen(MenuId::Experimental));
}

void test_single_screen_menus_stay_on_zero()
{
    MenuState m;
    m.nextScreen(MenuId::Dtc);
    TEST_ASSERT_EQUAL_UINT8(0, m.screen(MenuId::Dtc));
    m.prevScreen(MenuId::Settings);
    TEST_ASSERT_EQUAL_UINT8(0, m.screen(MenuId::Settings));
}

void test_screens_are_tracked_per_menu()
{
    MenuState m;
    m.nextScreen(MenuId::Debug);
    m.nextScreen(MenuId::Debug);
    TEST_ASSERT_EQUAL_UINT8(2, m.screen(MenuId::Debug));
    TEST_ASSERT_EQUAL_UINT8(0, m.screen(MenuId::Cockpit));
    TEST_ASSERT_TRUE(m.consumeScreenChanged());
    TEST_ASSERT_FALSE(m.consumeScreenChanged());
}

void runTests()
{
    RUN_TEST(test_starts_on_first_cockpit_screen);
    RUN_TEST(test_next_menu_cycles_through_all_five_and_wraps);
    RUN_TEST(test_prev_menu_wraps_from_first_to_last);
    RUN_TEST(test_menu_change_flag_is_consumed_once);
    RUN_TEST(test_cockpit_screens_wrap_at_max);
    RUN_TEST(test_experimental_group_wraps_over_full_byte_range);
    RUN_TEST(test_single_screen_menus_stay_on_zero);
    RUN_TEST(test_screens_are_tracked_per_menu);
}

UNITY_SUITE_MAIN()
