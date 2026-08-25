/*
 * Graphics-first platform stubs.
 *
 * NETWORK is not defined. Sound hardware and playback are intentionally
 * no-ops in this first pass; gameplay callers retain their original flow.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "types.h"
#include "fix.h"
#include "vecmat.h"
#include "digi.h"
#include "joy.h"
#include "mouse.h"
#include "key.h"
#include "gr.h"
#include "dos.h"

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
void digi_play_sample_3d(int n, int a, int v, int d) { (void)n; (void)a; (void)v; (void)d; }
int digi_link_sound_to_object(int n, short o, int f, fix v) { (void)n; (void)o; (void)f; (void)v; return -1; }
int digi_link_sound_to_object2(int n, short o, int f, fix v, fix d) { (void)d; return digi_link_sound_to_object(n,o,f,v); }
int digi_link_sound_to_pos(int n, short s, short side, vms_vector *p, int f, fix v) { (void)n;(void)s;(void)side;(void)p;(void)f;(void)v;return -1; }
int digi_link_sound_to_pos2(int n, short s, short side, vms_vector *p, int f, fix v, fix d) { (void)d; return digi_link_sound_to_pos(n,s,side,p,f,v); }
void digi_play_midi_song(char *a,char *b,char *c,int d) {(void)a;(void)b;(void)c;(void)d;}
void digi_init_sounds(void) {}
void digi_sync_sounds(void) {}
void digi_kill_sound_linked_to_segment(int a,int b,int c) {(void)a;(void)b;(void)c;}
void digi_kill_sound_linked_to_object(int a) {(void)a;}
void digi_set_midi_volume(int a) {(void)a;}
void digi_set_digi_volume(int a) {(void)a;}
void digi_set_volume(int a,int b) {(void)a;(void)b;}
int digi_is_sound_playing(int a) {(void)a;return 0;}
void digi_pause_all(void) {}
void digi_resume_all(void) {}
void digi_stop_all(void) {}
int digi_set_max_channels(int n) { max_channels=n; return n; }
int digi_get_max_channels(void) { return max_channels; }

char joy_installed, joy_present;
int joy_init(void) { return 0; }
void joy_close(void) {}
void joy_flush(void) {}
void joy_set_timer_rate(int n) {(void)n;}
int joy_get_timer_rate(void) {return 0;}
void joy_get_pos(int *x,int *y) {*x=*y=0;}
int joy_get_btns(void) {return 0;}
int joy_get_button_up_cnt(int n) {(void)n;return 0;}
int joy_get_button_down_cnt(int n) {(void)n;return 0;}
fix joy_get_button_down_time(int n) {(void)n;return 0;}
ubyte joy_read_raw_buttons(void) {return 0;}
ubyte joystick_read_raw_axis(ubyte m,int *a) {(void)m;if(a)*a=0;return 0;}
ubyte joy_get_present_mask(void) {return 0;}
int joy_get_button_state(int n) {(void)n;return 0;}
int joy_get_scaled_reading(int r,int a) {(void)r;(void)a;return 0;}
void joy_set_cen(void) {}
void joy_set_cen_fake(int n) {(void)n;}
void joy_set_ul(void) {}
void joy_set_lr(void) {}
void joy_get_cal_vals(int *a,int *b,int *c) {memset(a,0,4*sizeof(*a));memset(b,0,4*sizeof(*b));memset(c,0,4*sizeof(*c));}
void joy_set_cal_vals(int *a,int *b,int *c) {(void)a;(void)b;(void)c;}
void joy_set_btn_values(int a,int b,fix c,int d,int e) {(void)a;(void)b;(void)c;(void)d;(void)e;}
void joy_set_slow_reading(int a) {(void)a;}

int minit(void) {return 0;}
void mclose(int n) {(void)n;}
void mopen(int n,int r,int c,int w,int h,char *t) {(void)n;(void)r;(void)c;(void)w;(void)h;(void)t;}
void mclear(int n) {(void)n;}
void _mprintf(int n,char *format,...) {(void)n;(void)format;}
void _mprintf_at(int n,int r,int c,char *format,...) {(void)n;(void)r;(void)c;(void)format;}
void mputc(int n,char c) {(void)n;(void)c;}
void mputc_at(int n,int r,int col,char c) {(void)n;(void)r;(void)col;(void)c;}
void msetcursor(int r,int c) {(void)r;(void)c;}
void mrefresh(short n) {(void)n;}

int getch(void) { return key_getch(); }
int kbhit(void) { return key_checkch(); }
int filelength(int fd) { struct stat st; return fstat(fd,&st) ? -1 : (int)st.st_size; }
char *descent_strrev(char *s) { size_t i=0,j=strlen(s); while(i<j){char c=s[i];s[i++]=s[--j];s[j]=c;} return s; }
char *descent_itoa(int v,char *b,int radix) { if(radix==16)sprintf(b,"%x",v);else sprintf(b,"%d",v);return b; }
void _makepath(char *path, const char *drive, const char *dir, const char *name, const char *ext)
{
	path[0] = '\0';
	if (drive) strcat(path, drive);
	if (dir) strcat(path, dir);
	if (name) strcat(path, name);
	if (ext) strcat(path, ext);
}
void _splitpath(const char *path, char *drive, char *dir, char *name, char *ext)
{
	const char *base = strrchr(path, '/');
	const char *dot;
	base = base ? base + 1 : path;
	dot = strrchr(base, '.');
	if (!dot) dot = path + strlen(path);
	if (drive) drive[0] = '\0';
	if (dir) {
		size_t n = (size_t)(base - path);
		memcpy(dir, path, n);
		dir[n] = '\0';
	}
	if (name) {
		size_t n = (size_t)(dot - base);
		memcpy(name, base, n);
		name[n] = '\0';
	}
	if (ext) strcpy(ext, *dot ? dot : "");
}

/* DOS/runtime compatibility used by otherwise portable original C. */
int stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
int strcmpi(const char *a, const char *b) { return strcasecmp(a, b); }
int strnicmp(const char *a, const char *b, size_t n) { return strncasecmp(a, b, n); }
char *strlwr(char *s) { char *p; for (p=s; *p; ++p) if (*p>='A' && *p<='Z') *p=(char)(*p-'A'+'a'); return s; }
char *strrev(char *s) { return descent_strrev(s); }
int stackavail(void) { return 8 * 1024 * 1024; }
int min(int a, int b) { return a < b ? a : b; }
int max(int a, int b) { return a > b ? a : b; }

/* DOS interrupt, selector, and VGA hardware have no macOS equivalent. */
void _enable(void) {}
void _disable(void) {}
int inp(int port) { (void)port; return 0; }
int outp(int port, int value) { (void)port; return value; }
int dpmi_init(int verbose) { (void)verbose; return 0; }
int dpmi_modify_selector_base(ushort selector, void *address) { (void)selector; (void)address; return 0; }
void gr_modex_setstart(short x, short y, int wait) { (void)x; (void)y; (void)wait; }
void gr_vesa_setstart(short x, short y) { (void)x; (void)y; }
int gr_vesa_checkmode(int mode) { (void)mode; return 11; }

/* Unsupported first-pass VR hardware is reported as absent. */
int Victor_headset_installed = 0;
int iglasses_headset_installed = 0;
void victor_read_headset_filtered(fix *yaw, fix *pitch, fix *roll) { *yaw=*pitch=*roll=0; }
int iglasses_read_headset(fix *yaw, fix *pitch, fix *roll) { *yaw=*pitch=*roll=0; return 0; }
void vfx_close_graphics(void) {}
void vfx_init_graphics(void) {}
void victor_init_graphics(void) {}
void vfx_set_palette_sub(ubyte *palette) { gr_palette_load(palette); }

/* External DOS controller interrupt support is intentionally unavailable. */
void kconfig_read_external_controls(void) {}
