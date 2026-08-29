#ifndef DESCENT_ESP32_JOYSTICK_INPUT_H
#define DESCENT_ESP32_JOYSTICK_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum descent_joystick_axis {
    DESCENT_JOYSTICK_LEFT_X = 0,
    DESCENT_JOYSTICK_LEFT_Y,
    DESCENT_JOYSTICK_RIGHT_X,
    DESCENT_JOYSTICK_RIGHT_Y,
    DESCENT_JOYSTICK_AXIS_COUNT
};

enum descent_joystick_button {
    DESCENT_JOYSTICK_LEFT_BUTTON = 0,
    DESCENT_JOYSTICK_RIGHT_BUTTON,
    DESCENT_JOYSTICK_BUTTON_COUNT
};

enum descent_menu_input {
    DESCENT_MENU_INPUT_NONE = 0,
    DESCENT_MENU_INPUT_UP,
    DESCENT_MENU_INPUT_DOWN,
    DESCENT_MENU_INPUT_LEFT,
    DESCENT_MENU_INPUT_RIGHT,
    DESCENT_MENU_INPUT_SELECT
};

/* Call reset while both sticks are released and centered. */
void descent_joystick_reset(uint32_t now_ms,
                            const uint16_t center[DESCENT_JOYSTICK_AXIS_COUNT]);
void descent_joystick_update(
    uint32_t now_ms,
    const uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT],
    uint8_t pressed_buttons);
void descent_joystick_flush(uint32_t now_ms);

/* Axes are normalized to -32767..32767. X is positive to the right and Y is
 * positive upward, independent of the KY-023 potentiometer direction. */
int descent_joystick_axis_value(unsigned int axis);
int descent_joystick_button_state(unsigned int button);
unsigned int descent_joystick_take_button_down_count(unsigned int button);

int descent_joystick_peek_menu_input(void);
int descent_joystick_take_menu_input(void);

#ifdef __cplusplus
}
#endif

#endif
