#ifndef DESCENT_MACOS_PLATFORM_H
#define DESCENT_MACOS_PLATFORM_H

extern int start_to_new_game;
extern int show_fps;

void macos_present_frame(void);
int macos_poll_key_event(int *scancode, int *pressed);
void macos_delay(unsigned milliseconds);

#endif
