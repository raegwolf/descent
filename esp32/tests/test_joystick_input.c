#include <assert.h>
#include <stdio.h>

#include "joystick_input.h"

static void settle(uint32_t *now, uint16_t lx, uint16_t ly,
                   uint16_t rx, uint16_t ry, uint8_t buttons)
{
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT] = {lx, ly, rx, ry};
    int sample;
    for (sample = 0; sample < 12; ++sample) {
        *now += 10;
        descent_joystick_update(*now, raw, buttons);
    }
}

static void reset_centered(uint32_t *now)
{
    uint16_t center[DESCENT_JOYSTICK_AXIS_COUNT] = {2048, 2048, 2048, 2048};
    descent_joystick_calibrate(*now, center);
}

static void test_asymmetric_startup_offsets(void)
{
    uint32_t now = 0;
    uint16_t neutral[DESCENT_JOYSTICK_AXIS_COUNT] = {1700, 2250, 1900, 2400};
    descent_joystick_calibrate(now, neutral);

    assert(descent_joystick_axis_offset(DESCENT_JOYSTICK_LEFT_X) == -1700);
    assert(descent_joystick_axis_offset(DESCENT_JOYSTICK_LEFT_Y) == -2250);
    assert(descent_joystick_axis_offset(DESCENT_JOYSTICK_RIGHT_X) == -1900);
    assert(descent_joystick_axis_offset(DESCENT_JOYSTICK_RIGHT_Y) == -2400);
    settle(&now, 1700, 2250, 1900, 2400, 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_X) == 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_Y) == 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_RIGHT_X) == 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_RIGHT_Y) == 0);
}

static void test_axes_and_dead_zone(void)
{
    uint32_t now = 0;
    reset_centered(&now);
    settle(&now, 2100, 2000, 2080, 2020, 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_X) == 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_Y) == 0);

    settle(&now, 4095, 0, 0, 4095, 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_X) < -30000);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_Y) < -30000);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_RIGHT_X) < -30000);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_RIGHT_Y) > 30000);
}

static void test_buttons_are_edge_counted(void)
{
    uint32_t now = 0;
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT] = {2048, 2048, 2048, 2048};
    reset_centered(&now);

    /* Button state is immediate and independent of the analogue axes. */
    now += 5;
    descent_joystick_update(now, raw, 1);
    assert(descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 1);
    assert(descent_joystick_take_any_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 1);
    assert(descent_joystick_take_any_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 0);

    now += 5;
    descent_joystick_update(now, raw, 0);
    assert(!descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
    now += 5;
    descent_joystick_update(now, raw, 1);
    assert(descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 1);
    assert(descent_joystick_take_any_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 1);
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 0);
    settle(&now, 2048, 2048, 2048, 2048, 0);
    assert(!descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
}

static void test_menu_direction_select_and_repeat(void)
{
    uint32_t now = 0;
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT] = {2048, 4095, 2048, 2048};
    reset_centered(&now);
    settle(&now, 2048, 4095, 2048, 2048, 0);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_UP);
    while (descent_joystick_take_menu_input() != DESCENT_MENU_INPUT_NONE) {}

    now += 400;
    descent_joystick_update(now, raw, 0);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_UP);

    settle(&now, 2048, 2048, 2048, 2048, 0);
    descent_joystick_flush(now);

    settle(&now, 2048, 2048, 4095, 2048, 0);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_RIGHT);
    settle(&now, 2048, 2048, 2048, 2048, 0);
    descent_joystick_flush(now);

    settle(&now, 2048, 2048, 2048, 2048, 2);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_SELECT);
}

static void test_buttons_with_deflected_axes(void)
{
    uint32_t now = 0;
    uint16_t j1_deflected[DESCENT_JOYSTICK_AXIS_COUNT] = {
        4095, 0, 2048, 2048
    };
    uint16_t j2_deflected[DESCENT_JOYSTICK_AXIS_COUNT] = {
        2048, 2048, 4095, 0
    };
    reset_centered(&now);

    now += 10;
    descent_joystick_update(now, j1_deflected, 1);
    assert(descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 1);
    assert(descent_joystick_take_any_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 1);

    reset_centered(&now);
    now += 10;
    descent_joystick_update(now, j2_deflected, 2);
    assert(descent_joystick_button_state(DESCENT_JOYSTICK_RIGHT_BUTTON));
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_RIGHT_BUTTON) == 1);
    assert(descent_joystick_take_any_button_down_count(
               DESCENT_JOYSTICK_RIGHT_BUTTON) == 1);
}

static void test_flush_clears_both_button_consumers(void)
{
    uint32_t now = 0;
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT] = {2048, 2048, 2048, 2048};
    reset_centered(&now);

    descent_joystick_update(++now, raw, 3);
    descent_joystick_flush(now);
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 0);
    assert(descent_joystick_take_any_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 0);
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_RIGHT_BUTTON) == 0);
    assert(descent_joystick_take_any_button_down_count(
               DESCENT_JOYSTICK_RIGHT_BUTTON) == 0);
}

static void test_left_long_press_emits_one_escape(void)
{
    uint32_t now = 100;
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT] = {2048, 2048, 2048, 2048};
    reset_centered(&now);

    descent_joystick_update(now, raw, 1);
    assert(descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_SELECT);

    now += 899;
    descent_joystick_update(now, raw, 1);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_NONE);

    now += 1;
    descent_joystick_update(now, raw, 1);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_ESCAPE);

    now += 1000;
    descent_joystick_update(now, raw, 1);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_NONE);

    now += 10;
    descent_joystick_update(now, raw, 0);
    now += 10;
    descent_joystick_update(now, raw, 1);
    assert(descent_joystick_take_menu_input() == DESCENT_MENU_INPUT_SELECT);
}

int main(void)
{
    test_asymmetric_startup_offsets();
    test_axes_and_dead_zone();
    test_buttons_are_edge_counted();
    test_menu_direction_select_and_repeat();
    test_buttons_with_deflected_axes();
    test_flush_clears_both_button_consumers();
    test_left_long_press_emits_one_escape();
    puts("joystick input tests passed");
    return 0;
}
