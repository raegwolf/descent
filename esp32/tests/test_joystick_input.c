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
    descent_joystick_reset(*now, center);
}

static void test_axes_and_dead_zone(void)
{
    uint32_t now = 0;
    reset_centered(&now);
    settle(&now, 2100, 2000, 2080, 2020, 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_X) == 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_Y) == 0);

    settle(&now, 4095, 0, 0, 4095, 0);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_X) > 30000);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_LEFT_Y) > 30000);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_RIGHT_X) < -30000);
    assert(descent_joystick_axis_value(DESCENT_JOYSTICK_RIGHT_Y) < -30000);
}

static void test_buttons_are_edge_counted(void)
{
    uint32_t now = 0;
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT] = {2048, 2048, 2048, 2048};
    reset_centered(&now);

    /* A short press/release bounce must not become a stable fire event. */
    now += 5;
    descent_joystick_update(now, raw, 1);
    now += 5;
    descent_joystick_update(now, raw, 0);
    assert(!descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 0);

    settle(&now, 2048, 2048, 2048, 2048, 1);
    assert(descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 1);
    assert(descent_joystick_take_button_down_count(
               DESCENT_JOYSTICK_LEFT_BUTTON) == 0);
    settle(&now, 2048, 2048, 2048, 2048, 0);
    assert(!descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON));
}

static void test_menu_direction_select_and_repeat(void)
{
    uint32_t now = 0;
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT] = {2048, 0, 2048, 2048};
    reset_centered(&now);
    settle(&now, 2048, 0, 2048, 2048, 0);
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

int main(void)
{
    test_axes_and_dead_zone();
    test_buttons_are_edge_counted();
    test_menu_direction_select_and_repeat();
    puts("joystick input tests passed");
    return 0;
}
