#ifndef DESCENT_MACOS_PLATFORM_H
#define DESCENT_MACOS_PLATFORM_H

extern int start_to_new_game;
extern int show_fps;

void macos_present_frame(void);
int macos_poll_key_event(int *scancode, int *pressed);
void macos_poll_mouse(int *dx, int *dy, int *buttons,
                      unsigned down_counts[3]);
void macos_set_relative_mouse(int enabled);
void macos_delay(unsigned milliseconds);

#endif
