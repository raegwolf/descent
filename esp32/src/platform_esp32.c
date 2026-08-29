/* ESP32-S3 platform services for the original Descent engine. */
#if !defined(DESCENT_MENU_ONLY) || !DESCENT_MENU_ONLY
#define DESCENT_ENGINE_BUILD 1
#include "esp32_compat.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "esp32_bridge.h"
#include "types.h"
#include "fix.h"
#include "vecmat.h"
#include "digi.h"
#include "joy.h"
#include "mouse.h"
#include "key.h"
#include "gr.h"
#include "songs.h"
#include "kconfig.h"
#include "dos.h"
#include "joystick_input.h"

int start_to_new_game = 1;

/* Time */
static uint32_t timer_epoch;
void timer_init(void) { timer_epoch = esp32_milliseconds(); }
void timer_close(void) {}
void timer_set_rate(int count_val) { (void)count_val; }
void timer_set_function(void *function) { (void)function; }
fix timer_get_fixed_seconds(void)
{
	uint32_t elapsed = esp32_milliseconds() - timer_epoch;
	return (fix)(((uint64_t)elapsed * (uint64_t)F1_0) / 1000U);
}
fix timer_get_fixed_secondsX(void) { return timer_get_fixed_seconds(); }
fix timer_get_approx_seconds(void) { return timer_get_fixed_seconds(); }

/* Menus consume the two physical joysticks as virtual arrow/Enter keys. */
char keyd_buffer_type = 0;
char keyd_repeat = 0;
char keyd_editor_mode = 0;
volatile unsigned char keyd_last_pressed;
volatile unsigned char keyd_last_released;
volatile unsigned char keyd_pressed[256];
volatile int keyd_time_when_last_pressed;
extern ubyte kc_use_external_control;
extern fix FrameTime;

static int menu_input_to_key(int input)
{
	switch (input) {
	case DESCENT_MENU_INPUT_UP: return KEY_UP;
	case DESCENT_MENU_INPUT_DOWN: return KEY_DOWN;
	case DESCENT_MENU_INPUT_LEFT: return KEY_LEFT;
	case DESCENT_MENU_INPUT_RIGHT: return KEY_RIGHT;
	case DESCENT_MENU_INPUT_SELECT: return KEY_ENTER;
	case DESCENT_MENU_INPUT_ESCAPE: return KEY_ESC;
	default: return 0;
	}
}

void key_init(void)
{
	memset((void *)keyd_pressed, 0, sizeof(keyd_pressed));
	esp32_poll_joysticks();
	descent_joystick_flush(esp32_milliseconds());
	/* Reuse the engine's existing external-controller callback point. */
	kc_use_external_control = 1;
}
void key_close(void) {}
void key_debug(void) {}
void key_flush(void)
{
	memset((void *)keyd_pressed, 0, sizeof(keyd_pressed));
	esp32_poll_joysticks();
	descent_joystick_flush(esp32_milliseconds());
}
int key_checkch(void)
{
	esp32_poll_joysticks();
	return descent_joystick_peek_menu_input() != DESCENT_MENU_INPUT_NONE;
}
int key_inkey(void)
{
	esp32_poll_joysticks();
	return menu_input_to_key(descent_joystick_take_menu_input());
}
int key_inkey_time(fix *time)
{
	int key = key_inkey();
	if (time) *time = timer_get_fixed_seconds();
	return key;
}
int key_peekkey(void)
{
	esp32_poll_joysticks();
	return menu_input_to_key(descent_joystick_peek_menu_input());
}
int key_getch(void)
{
	int key;
	for (;;) {
		key = key_inkey();
		if (key) return key;
		gr_sync_display();
		esp32_delay_ms(10);
	}
}
unsigned int key_get_shift_status(void) { return 0; }
fix key_down_time(int scancode) { (void)scancode; return 0; }
unsigned int key_down_count(int scancode) { (void)scancode; return 0; }
unsigned int key_up_count(int scancode) { (void)scancode; return 0; }
char key_to_ascii(int keycode) { (void)keycode; return 0; }

static fix scaled_axis_time(int axis, fix frame_time)
{
	return (fix)(((int64_t)descent_joystick_axis_value(axis) * frame_time) /
	             32767);
}

static ubyte add_count_saturated(ubyte current, unsigned int added)
{
	unsigned int total = current + added;
	return (ubyte)(total > 255U ? 255U : total);
}

void kconfig_read_external_controls(void)
{
	unsigned int count;
	esp32_poll_joysticks();
	Controls.forward_thrust_time +=
		scaled_axis_time(DESCENT_JOYSTICK_LEFT_Y, FrameTime);
	Controls.sideways_thrust_time +=
		scaled_axis_time(DESCENT_JOYSTICK_LEFT_X, FrameTime);
	/* Descent's positive pitch angle points the nose downward. */
	Controls.pitch_time -=
		scaled_axis_time(DESCENT_JOYSTICK_RIGHT_Y, FrameTime) / 2;
	Controls.heading_time +=
		scaled_axis_time(DESCENT_JOYSTICK_RIGHT_X, FrameTime);

	Controls.fire_primary_state |= descent_joystick_button_state(
		DESCENT_JOYSTICK_LEFT_BUTTON);
	count = descent_joystick_take_button_down_count(
		DESCENT_JOYSTICK_LEFT_BUTTON);
	Controls.fire_primary_down_count = add_count_saturated(
		Controls.fire_primary_down_count, count);
	Controls.fire_secondary_state |= descent_joystick_button_state(
		DESCENT_JOYSTICK_RIGHT_BUTTON);
	count = descent_joystick_take_button_down_count(
		DESCENT_JOYSTICK_RIGHT_BUTTON);
	Controls.fire_secondary_down_count = add_count_saturated(
		Controls.fire_secondary_down_count, count);
}

/* Sound and music are deliberately absent. */
int digi_driver_board, digi_driver_port, digi_driver_irq, digi_driver_dma;
int digi_midi_type, digi_midi_port, digi_lomem;
int digi_timer_rate = 9943;
static int max_channels = 8;
int digi_get_settings(void) { return 0; }
int digi_init(void) { return 0; }
void digi_reset(void) {}
void digi_close(void) {}
void digi_play_sample(int n, fix v) { (void)n; (void)v; }
void digi_play_sample_once(int n, fix v) { (void)n; (void)v; }
void digi_play_sample_3d(int n, int a, int v, int d) { (void)n;(void)a;(void)v;(void)d; }
int digi_link_sound_to_object(int n, short o, int f, fix v) { (void)n;(void)o;(void)f;(void)v; return -1; }
int digi_link_sound_to_object2(int n, short o, int f, fix v, fix d) { (void)d; return digi_link_sound_to_object(n,o,f,v); }
int digi_link_sound_to_pos(int n, short s, short side, vms_vector *p, int f, fix v) { (void)n;(void)s;(void)side;(void)p;(void)f;(void)v; return -1; }
int digi_link_sound_to_pos2(int n, short s, short side, vms_vector *p, int f, fix v, fix d) { (void)d; return digi_link_sound_to_pos(n,s,side,p,f,v); }
void digi_play_midi_song(char *a, char *b, char *c, int d) { (void)a;(void)b;(void)c;(void)d; }
void digi_init_sounds(void) {}
void digi_sync_sounds(void) {}
void digi_kill_sound_linked_to_segment(int a,int b,int c) { (void)a;(void)b;(void)c; }
void digi_kill_sound_linked_to_object(int a) { (void)a; }
void digi_set_midi_volume(int a) { (void)a; }
void digi_set_digi_volume(int a) { (void)a; }
void digi_set_volume(int a,int b) { (void)a;(void)b; }
int digi_is_sound_playing(int a) { (void)a; return 0; }
void digi_pause_all(void) {}
void digi_resume_all(void) {}
void digi_stop_all(void) {}
int digi_set_max_channels(int n) { max_channels = n; return n; }
int digi_get_max_channels(void) { return max_channels; }

song_info Songs[MAX_SONGS];
void songs_play_song(int songnum, int repeat) { (void)songnum; (void)repeat; }
void songs_play_level_song(int levelnum) { (void)levelnum; }

/* The ESP32 axes feed the portable control accumulator above instead of the
 * DOS calibration path. Bridge the two switches into the legacy button API as
 * well: death and score prompts use it for their "any button" checks. */
char joy_installed, joy_present;
int joy_init(void) { return 0; }
void joy_close(void) {}
void joy_flush(void)
{
	esp32_poll_joysticks();
	descent_joystick_flush(esp32_milliseconds());
}
void joy_set_timer_rate(int n) { (void)n; }
int joy_get_timer_rate(void) { return 0; }
void joy_get_pos(int *x,int *y) { *x = *y = 0; }
int joy_get_btns(void)
{
	int buttons = 0;
	esp32_poll_joysticks();
	if (descent_joystick_button_state(DESCENT_JOYSTICK_LEFT_BUTTON))
		buttons |= 1 << DESCENT_JOYSTICK_LEFT_BUTTON;
	if (descent_joystick_button_state(DESCENT_JOYSTICK_RIGHT_BUTTON))
		buttons |= 1 << DESCENT_JOYSTICK_RIGHT_BUTTON;
	return buttons;
}
int joy_get_button_up_cnt(int n) { (void)n; return 0; }
int joy_get_button_down_cnt(int n)
{
	esp32_poll_joysticks();
	return (int)descent_joystick_take_any_button_down_count((unsigned int)n);
}
fix joy_get_button_down_time(int n) { (void)n; return 0; }
ubyte joy_read_raw_buttons(void) { return (ubyte)joy_get_btns(); }
ubyte joystick_read_raw_axis(ubyte m,int *a) { (void)m; if (a) *a=0; return 0; }
ubyte joy_get_present_mask(void) { return 0; }
int joy_get_button_state(int n)
{
	esp32_poll_joysticks();
	return descent_joystick_button_state((unsigned int)n);
}
int joy_get_scaled_reading(int r,int a) { (void)r;(void)a; return 0; }
void joy_set_cen(void) {}
void joy_set_cen_fake(int n) { (void)n; }
void joy_set_ul(void) {}
void joy_set_lr(void) {}
void joy_get_cal_vals(int *a,int *b,int *c) { memset(a,0,4*sizeof(*a));memset(b,0,4*sizeof(*b));memset(c,0,4*sizeof(*c)); }
void joy_set_cal_vals(int *a,int *b,int *c) { (void)a;(void)b;(void)c; }
void joy_set_btn_values(int a,int b,fix c,int d,int e) { (void)a;(void)b;(void)c;(void)d;(void)e; }
void joy_set_slow_reading(int a) { (void)a; }

int mouse_init(int n) { (void)n; return 0; }
void mouse_close(void) {}
void mouse_flush(void) {}
int mouse_set_limits(int a,int b,int c,int d) { (void)a;(void)b;(void)c;(void)d; return 0; }
void mouse_get_pos(int *x,int *y) { *x = *y = 0; }
void mouse_get_delta(int *x,int *y) { *x = *y = 0; }
int mouse_get_btns(void) { return 0; }
void mouse_set_pos(int x,int y) { (void)x;(void)y; }
void mouse_get_cyberman_pos(int *x,int *y) { *x = *y = 0; }
fix mouse_button_down_time(int n) { (void)n; return 0; }
int mouse_button_down_count(int n) { (void)n; return 0; }
int mouse_button_state(int n) { (void)n; return 0; }

/* Debug console and DOS hardware compatibility. */
int minit(void) { return 0; }
void mclose(int n) { (void)n; }
void mopen(int n,int r,int c,int w,int h,char *t) { (void)n;(void)r;(void)c;(void)w;(void)h;(void)t; }
void mclear(int n) { (void)n; }
void _mprintf(int n,char *format,...)
{
	va_list arguments;
	(void)n;
	va_start(arguments, format);
	vprintf(format, arguments);
	va_end(arguments);
}
void _mprintf_at(int n,int r,int c,char *format,...) { (void)n;(void)r;(void)c;(void)format; }
void mputc(int n,char c) { (void)n;(void)c; }
void mputc_at(int n,int r,int col,char c) { (void)n;(void)r;(void)col;(void)c; }
void msetcursor(int r,int c) { (void)r;(void)c; }
void mrefresh(short n) { (void)n; }
int getch(void) { return key_getch(); }
int kbhit(void) { return 0; }
int filelength(int fd) { struct stat st; return fstat(fd,&st) ? -1 : (int)st.st_size; }

char *descent_strrev(char *s)
{
	size_t i=0,j=strlen(s);
	while (i<j) { char c=s[i]; s[i++]=s[--j]; s[j]=c; }
	return s;
}
char *descent_itoa(int v,char *b,int radix)
{
	if (radix==16) sprintf(b,"%x",v); else sprintf(b,"%d",v);
	return b;
}
void _makepath(char *path,const char *drive,const char *dir,const char *name,const char *ext)
{
	path[0]='\0'; if(drive)strcat(path,drive);if(dir)strcat(path,dir);if(name)strcat(path,name);if(ext)strcat(path,ext);
}
void _splitpath(const char *path,char *drive,char *dir,char *name,char *ext)
{
	const char *base=strrchr(path,'/'); const char *dot; base=base?base+1:path;
	dot=strrchr(base,'.'); if(!dot)dot=path+strlen(path); if(drive)drive[0]='\0';
	if(dir){size_t n=(size_t)(base-path);memcpy(dir,path,n);dir[n]='\0';}
	if(name){size_t n=(size_t)(dot-base);memcpy(name,base,n);name[n]='\0';}
	if(ext)strcpy(ext,*dot?dot:"");
}
int stricmp(const char *a,const char *b) { return strcasecmp(a,b); }
int strcmpi(const char *a,const char *b) { return strcasecmp(a,b); }
int strnicmp(const char *a,const char *b,size_t n) { return strncasecmp(a,b,n); }
char *strlwr(char *s) { char *p;for(p=s;*p;++p)if(*p>='A'&&*p<='Z')*p=(char)(*p-'A'+'a');return s; }
char *strrev(char *s) { return descent_strrev(s); }
int stackavail(void) { return 32768; }
int min(int a,int b) { return a<b?a:b; }
int max(int a,int b) { return a>b?a:b; }
void _enable(void) {}
void _disable(void) {}
int inp(int port) { (void)port; return 0; }
int outp(int port,int value) { (void)port; return value; }
int dpmi_init(int verbose) { (void)verbose; return 0; }
int dpmi_modify_selector_base(ushort selector,void *address) { (void)selector;(void)address;return 0; }
void gr_modex_setstart(short x,short y,int wait) { (void)x;(void)y;(void)wait; }
void gr_vesa_setstart(short x,short y) { (void)x;(void)y; }
int gr_vesa_checkmode(int mode) { (void)mode; return 11; }

int Victor_headset_installed;
int iglasses_headset_installed;
void victor_read_headset_filtered(fix *yaw,fix *pitch,fix *roll) { *yaw=*pitch=*roll=0; }
int iglasses_read_headset(fix *yaw,fix *pitch,fix *roll) { *yaw=*pitch=*roll=0;return 0; }
void vfx_close_graphics(void) {}
void vfx_init_graphics(void) {}
void victor_init_graphics(void) {}
void vfx_set_palette_sub(ubyte *palette) { gr_palette_load(palette); }

/* Filename enumeration is unused without menus, demos, or custom missions. */
int _dos_findfirst(const char *pattern,int attributes,struct find_t *result)
{ (void)pattern;(void)attributes;(void)result;return 1; }
int _dos_findnext(struct find_t *result) { (void)result;return 1; }

static unsigned int random_seed;
int psrand(void) { return ((random_seed=random_seed*0x41c64e6dU+0x3039U)>>16)&0x7fff; }
void pssrand(unsigned int seed) { random_seed=seed; }
#endif /* !DESCENT_MENU_ONLY */
