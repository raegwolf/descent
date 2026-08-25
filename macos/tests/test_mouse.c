#undef NDEBUG
#include "fix.h"
#include "mouse.h"

#include <assert.h>
#include <string.h>

static fix fake_time;
static int fake_dx;
static int fake_dy;
static int fake_buttons;
static int relative_mouse;
static unsigned fake_downs[3];

fix timer_get_fixed_seconds(void)
{
	return fake_time;
}

void macos_set_relative_mouse(int enabled)
{
	relative_mouse = enabled;
}

void macos_poll_mouse(int *dx, int *dy, int *buttons, unsigned down_counts[3])
{
	*dx = fake_dx;
	*dy = fake_dy;
	*buttons = fake_buttons;
	memcpy(down_counts, fake_downs, sizeof(fake_downs));
	fake_dx = fake_dy = 0;
	memset(fake_downs, 0, sizeof(fake_downs));
}

int main(void)
{
	int dx, dy;

	assert(mouse_init(0) == 3);
	assert(relative_mouse == 1);

	fake_time = 100;
	fake_dx = 5;
	fake_dy = -3;
	fake_buttons = MOUSE_LBTN;
	fake_downs[MB_LEFT] = 1;
	mouse_get_delta(&dx, &dy);
	assert(dx == 5);
	assert(dy == -3);
	assert(mouse_get_btns() == MOUSE_LBTN);
	assert(mouse_button_down_count(MB_LEFT) == 1);
	assert(mouse_button_state(MB_LEFT) == 1);

	fake_time = 200;
	assert(mouse_button_down_time(MB_LEFT) == 100);
	fake_time = 250;
	fake_buttons = 0;
	assert(mouse_button_state(MB_LEFT) == 0);
	assert(mouse_button_down_time(MB_LEFT) == 50);

	mouse_close();
	assert(relative_mouse == 0);
	return 0;
}
