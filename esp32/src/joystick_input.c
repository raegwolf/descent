#include "joystick_input.h"

#include <limits.h>
#include <string.h>

#define ADC_MAX_VALUE 4095
#define ADC_DEAD_ZONE 240
#define MENU_AXIS_THRESHOLD 12000
#define MENU_INITIAL_REPEAT_MS 350U
#define MENU_REPEAT_MS 140U
#define MENU_QUEUE_SIZE 8U
#define BUTTON_DEBOUNCE_MS 25U

typedef struct joystick_state {
    int offset[DESCENT_JOYSTICK_AXIS_COUNT];
    int filtered[DESCENT_JOYSTICK_AXIS_COUNT];
    int axis[DESCENT_JOYSTICK_AXIS_COUNT];
    uint8_t raw_buttons;
    uint8_t buttons;
    uint32_t button_changed_at[DESCENT_JOYSTICK_BUTTON_COUNT];
    uint8_t button_down_count[DESCENT_JOYSTICK_BUTTON_COUNT];
    int8_t menu_direction[2];
    uint32_t menu_repeat_at[2];
    uint8_t menu_queue[MENU_QUEUE_SIZE];
    uint8_t menu_head;
    uint8_t menu_tail;
} joystick_state;

static joystick_state input;

static int absolute_value(int value)
{
    return value < 0 ? -value : value;
}

static int normalize_axis(int raw, int offset)
{
    int delta = raw + offset;
    int available;
    int value;

    if (absolute_value(delta) <= ADC_DEAD_ZONE)
        return 0;

    if (delta > 0) {
        available = ADC_MAX_VALUE + offset - ADC_DEAD_ZONE;
        if (available < 1)
            return 32767;
        value = ((delta - ADC_DEAD_ZONE) * 32767) / available;
        return value > 32767 ? 32767 : value;
    }

    available = -offset - ADC_DEAD_ZONE;
    if (available < 1)
        return -32767;
    value = ((-delta - ADC_DEAD_ZONE) * 32767) / available;
    return value > 32767 ? -32767 : -value;
}

static void queue_menu_input(int event)
{
    uint8_t next = (uint8_t)((input.menu_tail + 1U) % MENU_QUEUE_SIZE);
    if (next == input.menu_head)
        return;
    input.menu_queue[input.menu_tail] = (uint8_t)event;
    input.menu_tail = next;
}

static int menu_direction_for_stick(unsigned int stick)
{
    int x = input.axis[stick == 0 ? DESCENT_JOYSTICK_LEFT_X
                                  : DESCENT_JOYSTICK_RIGHT_X];
    int y = input.axis[stick == 0 ? DESCENT_JOYSTICK_LEFT_Y
                                  : DESCENT_JOYSTICK_RIGHT_Y];

    if (absolute_value(x) < MENU_AXIS_THRESHOLD &&
        absolute_value(y) < MENU_AXIS_THRESHOLD)
        return DESCENT_MENU_INPUT_NONE;

    /* Pick the dominant axis so a slightly diagonal stick does not move two
     * menu items at once. */
    if (absolute_value(y) >= absolute_value(x))
        return y > 0 ? DESCENT_MENU_INPUT_UP : DESCENT_MENU_INPUT_DOWN;
    return x > 0 ? DESCENT_MENU_INPUT_RIGHT : DESCENT_MENU_INPUT_LEFT;
}

static int time_reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

static void update_menu_direction(unsigned int stick, uint32_t now_ms)
{
    int direction = menu_direction_for_stick(stick);

    if (direction != input.menu_direction[stick]) {
        input.menu_direction[stick] = (int8_t)direction;
        input.menu_repeat_at[stick] = now_ms + MENU_INITIAL_REPEAT_MS;
        if (direction != DESCENT_MENU_INPUT_NONE)
            queue_menu_input(direction);
    } else if (direction != DESCENT_MENU_INPUT_NONE &&
               time_reached(now_ms, input.menu_repeat_at[stick])) {
        queue_menu_input(direction);
        input.menu_repeat_at[stick] = now_ms + MENU_REPEAT_MS;
    }
}

void descent_joystick_calibrate(
    uint32_t now_ms,
    const uint16_t neutral_reading[DESCENT_JOYSTICK_AXIS_COUNT])
{
    unsigned int axis;
    memset(&input, 0, sizeof(input));
    for (axis = 0; axis < DESCENT_JOYSTICK_AXIS_COUNT; ++axis) {
        input.offset[axis] = -(int)neutral_reading[axis];
        input.filtered[axis] = neutral_reading[axis];
    }
    input.menu_repeat_at[0] = now_ms + MENU_INITIAL_REPEAT_MS;
    input.menu_repeat_at[1] = now_ms + MENU_INITIAL_REPEAT_MS;
}

void descent_joystick_update(
    uint32_t now_ms,
    const uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT],
    uint8_t pressed_buttons)
{
    unsigned int axis;
    unsigned int button;

    for (axis = 0; axis < DESCENT_JOYSTICK_AXIS_COUNT; ++axis) {
        input.filtered[axis] =
            (input.filtered[axis] * 3 + (int)raw[axis]) / 4;
        input.axis[axis] = normalize_axis(input.filtered[axis],
                                          input.offset[axis]);
    }

    /* J1 is mounted opposite J2 in the enclosure, so only J2 needs its raw Y
     * direction inverted to expose positive-up values. */
    input.axis[DESCENT_JOYSTICK_RIGHT_Y] =
        -input.axis[DESCENT_JOYSTICK_RIGHT_Y];

    for (button = 0; button < DESCENT_JOYSTICK_BUTTON_COUNT; ++button) {
        uint8_t mask = (uint8_t)(1U << button);
        int raw_pressed = (pressed_buttons & mask) != 0;
        int previous_raw_pressed = (input.raw_buttons & mask) != 0;
        int stable_pressed = (input.buttons & mask) != 0;

        if (raw_pressed != previous_raw_pressed) {
            if (raw_pressed)
                input.raw_buttons |= mask;
            else
                input.raw_buttons &= (uint8_t)~mask;
            input.button_changed_at[button] = now_ms;
        } else if (raw_pressed != stable_pressed &&
                   time_reached(now_ms,
                                input.button_changed_at[button] +
                                BUTTON_DEBOUNCE_MS)) {
            if (raw_pressed) {
                input.buttons |= mask;
                if (input.button_down_count[button] < UCHAR_MAX)
                    ++input.button_down_count[button];
                queue_menu_input(DESCENT_MENU_INPUT_SELECT);
            } else {
                input.buttons &= (uint8_t)~mask;
            }
        }
    }

    update_menu_direction(0, now_ms);
    update_menu_direction(1, now_ms);
}

void descent_joystick_flush(uint32_t now_ms)
{
    unsigned int button;
    input.menu_head = input.menu_tail = 0;
    for (button = 0; button < DESCENT_JOYSTICK_BUTTON_COUNT; ++button)
        input.button_down_count[button] = 0;
    input.menu_direction[0] = (int8_t)menu_direction_for_stick(0);
    input.menu_direction[1] = (int8_t)menu_direction_for_stick(1);
    input.menu_repeat_at[0] = now_ms + MENU_INITIAL_REPEAT_MS;
    input.menu_repeat_at[1] = now_ms + MENU_INITIAL_REPEAT_MS;
}

int descent_joystick_axis_value(unsigned int axis)
{
    if (axis >= DESCENT_JOYSTICK_AXIS_COUNT)
        return 0;
    return input.axis[axis];
}

int descent_joystick_axis_offset(unsigned int axis)
{
    if (axis >= DESCENT_JOYSTICK_AXIS_COUNT)
        return 0;
    return input.offset[axis];
}

int descent_joystick_button_state(unsigned int button)
{
    if (button >= DESCENT_JOYSTICK_BUTTON_COUNT)
        return 0;
    return (input.buttons & (1U << button)) != 0;
}

unsigned int descent_joystick_take_button_down_count(unsigned int button)
{
    unsigned int count;
    if (button >= DESCENT_JOYSTICK_BUTTON_COUNT)
        return 0;
    count = input.button_down_count[button];
    input.button_down_count[button] = 0;
    return count;
}

int descent_joystick_peek_menu_input(void)
{
    if (input.menu_head == input.menu_tail)
        return DESCENT_MENU_INPUT_NONE;
    return input.menu_queue[input.menu_head];
}

int descent_joystick_take_menu_input(void)
{
    int event = descent_joystick_peek_menu_input();
    if (event != DESCENT_MENU_INPUT_NONE)
        input.menu_head = (uint8_t)((input.menu_head + 1U) % MENU_QUEUE_SIZE);
    return event;
}
