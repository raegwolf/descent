#include <string.h>

#include "fix.h"
#include "key.h"
#include "timer.h"
#include "platform.h"

#define KEY_QUEUE_SIZE 32

char keyd_buffer_type = 1;
char keyd_repeat = 1;
char keyd_editor_mode = 0;
volatile unsigned char keyd_last_pressed;
volatile unsigned char keyd_last_released;
volatile unsigned char keyd_pressed[256];
volatile int keyd_time_when_last_pressed;

static unsigned short queue[KEY_QUEUE_SIZE];
static fix queue_time[KEY_QUEUE_SIZE];
static unsigned head, tail;
static fix went_down[256], held_down[256];
static unsigned downs[256], ups[256];

static unsigned shifted_code(unsigned scan)
{
	unsigned code = scan;
	if (keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) code |= KEY_SHIFTED;
	if (keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT]) code |= KEY_ALTED;
	if (keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) code |= KEY_CTRLED;
	return code;
}

static void handle_event(int scan, int pressed)
{
	fix now;
	unsigned next;
	if (scan < 0 || scan > 255) return;
	now = timer_get_fixed_seconds();
	if (pressed) {
		if (!keyd_pressed[scan]) {
			keyd_pressed[scan] = 1;
			went_down[scan] = now;
			downs[scan]++;
			keyd_last_pressed = (ubyte)scan;
			keyd_time_when_last_pressed = now;
		} else if (!keyd_repeat) return;
		next = (tail + 1) % KEY_QUEUE_SIZE;
		if (next != head) {
			queue[tail] = (unsigned short)shifted_code((unsigned)scan);
			queue_time[tail] = now;
			tail = next;
		}
	} else if (keyd_pressed[scan]) {
		keyd_pressed[scan] = 0;
		held_down[scan] += now - went_down[scan];
		ups[scan]++;
		keyd_last_released = (ubyte)scan;
	}
}

static void pump(void)
{
	int scan, pressed;
	macos_present_frame();
	while (macos_poll_key_event(&scan, &pressed)) handle_event(scan, pressed);
}

void key_init(void) { key_flush(); }
void key_close(void) { key_flush(); }
void key_debug(void) {}

void key_flush(void)
{
	memset((void *)keyd_pressed, 0, sizeof(keyd_pressed));
	memset(went_down, 0, sizeof(went_down));
	memset(held_down, 0, sizeof(held_down));
	memset(downs, 0, sizeof(downs));
	memset(ups, 0, sizeof(ups));
	head = tail = 0;
}

int key_checkch(void) { pump(); return head != tail; }
int key_inkey(void)
{
	int value;
	pump();
	if (head == tail) return 0;
	value = queue[head];
	head = (head + 1) % KEY_QUEUE_SIZE;
	return value;
}
int key_inkey_time(fix *time)
{
	int value;
	pump();
	if (head == tail) return 0;
	value = queue[head];
	*time = queue_time[head];
	head = (head + 1) % KEY_QUEUE_SIZE;
	return value;
}
int key_peekkey(void) { pump(); return head == tail ? 0 : queue[head]; }
int key_getch(void)
{
	int key;
	while ((key = key_inkey()) == 0) macos_delay(1);
	return key;
}

unsigned int key_get_shift_status(void) { pump(); return shifted_code(0); }

fix key_down_time(int scan)
{
	fix now, value;
	pump();
	if (scan < 0 || scan > 255) return 0;
	if (!keyd_pressed[scan]) { value = held_down[scan]; held_down[scan] = 0; return value; }
	now = timer_get_fixed_seconds();
	value = now - went_down[scan];
	went_down[scan] = now;
	return value;
}

unsigned int key_down_count(int scan) { unsigned n; pump(); n = downs[scan & 255]; downs[scan & 255] = 0; return n; }
unsigned int key_up_count(int scan) { unsigned n; pump(); n = ups[scan & 255]; ups[scan & 255] = 0; return n; }

char key_to_ascii(int keycode)
{
	static const char normal[] = "\0\0331234567890-=\b\tqwertyuiop[]\n\0asdfghjkl;'`\0\\zxcvbnm,./\0*\0 ";
	int scan = keycode & 0xff;
	char c = scan < (int)sizeof(normal) ? normal[scan] : 0;
	if ((keycode & KEY_SHIFTED) && c >= 'a' && c <= 'z') c -= 'a' - 'A';
	return c;
}
