#ifndef DESCENT_MACOS_PLATFORM_H
#define DESCENT_MACOS_PLATFORM_H

/* Set to zero to restore the original title, briefing, and main-menu startup. */
extern int start_to_new_game;

void macos_present_frame(void);
int macos_poll_key_event(int *scancode, int *pressed);
void macos_delay(unsigned milliseconds);

#endif
