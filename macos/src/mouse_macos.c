#include <string.h>

#include "fix.h"
#include "mouse.h"
#include "timer.h"
#include "platform.h"

#define MACOS_MOUSE_BUTTONS 3
#define MACOS_MOUSE_SENSITIVITY_MULTIPLIER 1

static int mouse_installed;
static int mouse_buttons;
static int mouse_x;
static int mouse_y;
static int mouse_min_x;
static int mouse_min_y;
static int mouse_max_x = 319;
static int mouse_max_y = 199;
static int pending_dx;
static int pending_dy;
static unsigned pending_downs[MACOS_MOUSE_BUTTONS];
static fix went_down[MACOS_MOUSE_BUTTONS];
static fix held_down[MACOS_MOUSE_BUTTONS];

static void update_mouse(void)
{
	unsigned new_downs[MACOS_MOUSE_BUTTONS];
	fix now;
	int buttons;
	int dx, dy;
	int button;

	if (!mouse_installed)
		return;
	macos_poll_mouse(&dx, &dy, &buttons, new_downs);
	now = timer_get_fixed_seconds();
	pending_dx += dx;
	pending_dy += dy;
	mouse_x += dx;
	mouse_y += dy;
	if (mouse_x < mouse_min_x) mouse_x = mouse_min_x;
	if (mouse_x > mouse_max_x) mouse_x = mouse_max_x;
	if (mouse_y < mouse_min_y) mouse_y = mouse_min_y;
	if (mouse_y > mouse_max_y) mouse_y = mouse_max_y;

	for (button = 0; button < MACOS_MOUSE_BUTTONS; ++button) {
		int mask = 1 << button;
		pending_downs[button] += new_downs[button];
		if ((buttons & mask) && !(mouse_buttons & mask))
			went_down[button] = now;
		else if (!(buttons & mask) && (mouse_buttons & mask))
			held_down[button] += now - went_down[button];
	}
	mouse_buttons = buttons;
}

int mouse_init(int enable_cyberman)
{
	fix now;
	int button;

	(void)enable_cyberman;
	mouse_installed = 1;
	memset(pending_downs, 0, sizeof(pending_downs));
	memset(held_down, 0, sizeof(held_down));
	pending_dx = pending_dy = 0;
	mouse_buttons = 0;
	now = timer_get_fixed_seconds();
	for (button = 0; button < MACOS_MOUSE_BUTTONS; ++button)
		went_down[button] = now;
	macos_set_relative_mouse(1);
	update_mouse();
	memset(pending_downs, 0, sizeof(pending_downs));
	pending_dx = pending_dy = 0;
	return MACOS_MOUSE_BUTTONS;
}

void mouse_close(void)
{
	if (mouse_installed)
		macos_set_relative_mouse(0);
	mouse_installed = 0;
}

void mouse_flush(void)
{
	fix now;
	int button;

	update_mouse();
	memset(pending_downs, 0, sizeof(pending_downs));
	memset(held_down, 0, sizeof(held_down));
	pending_dx = pending_dy = 0;
	now = timer_get_fixed_seconds();
	for (button = 0; button < MACOS_MOUSE_BUTTONS; ++button)
		went_down[button] = now;
}

int mouse_set_limits(int x1, int y1, int x2, int y2)
{
	mouse_min_x = x1;
	mouse_min_y = y1;
	mouse_max_x = x2;
	mouse_max_y = y2;
	return 0;
}

void mouse_get_pos(int *x, int *y)
{
	update_mouse();
	*x = mouse_x;
	*y = mouse_y;
}

void mouse_get_delta(int *dx, int *dy)
{
	update_mouse();
	*dx = pending_dx * MACOS_MOUSE_SENSITIVITY_MULTIPLIER;
	*dy = pending_dy * MACOS_MOUSE_SENSITIVITY_MULTIPLIER;
	pending_dx = pending_dy = 0;
}

int mouse_get_btns(void)
{
	update_mouse();
	return mouse_buttons;
}

void mouse_set_pos(int x, int y)
{
	mouse_x = x;
	mouse_y = y;
}

void mouse_get_cyberman_pos(int *x, int *y)
{
	*x = *y = 0;
}

fix mouse_button_down_time(int button)
{
	fix value;
	fix now;

	update_mouse();
	if (button < 0 || button >= MACOS_MOUSE_BUTTONS)
		return 0;
	if ((mouse_buttons & (1 << button)) == 0) {
		value = held_down[button];
		held_down[button] = 0;
		return value;
	}
	now = timer_get_fixed_seconds();
	value = now - went_down[button];
	went_down[button] = now;
	return value;
}

int mouse_button_down_count(int button)
{
	unsigned value;

	update_mouse();
	if (button < 0 || button >= MACOS_MOUSE_BUTTONS)
		return 0;
	value = pending_downs[button];
	pending_downs[button] = 0;
	return (int)value;
}

int mouse_button_state(int button)
{
	update_mouse();
	if (button < 0 || button >= MACOS_MOUSE_BUTTONS)
		return 0;
	return (mouse_buttons & (1 << button)) != 0;
}
