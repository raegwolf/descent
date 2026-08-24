#ifndef DESCENT_MACOS_SDL_SHIM_H
#define DESCENT_MACOS_SDL_SHIM_H

#include <stdint.h>

enum {
    DESCENT_SCREEN_WIDTH = 320,
    DESCENT_SCREEN_HEIGHT = 200
};

typedef struct descent_sdl descent_sdl;

typedef enum descent_input {
    DESCENT_INPUT_NONE,
    DESCENT_INPUT_UP,
    DESCENT_INPUT_DOWN,
    DESCENT_INPUT_ACCEPT,
    DESCENT_INPUT_QUIT
} descent_input;

descent_sdl *descent_sdl_create(const char *title);
void descent_sdl_destroy(descent_sdl *platform);
descent_input descent_sdl_poll_input(descent_sdl *platform);
int descent_sdl_poll_key(descent_sdl *platform, int *pc_scancode, int *pressed);
int descent_sdl_present(descent_sdl *platform,
                        const uint8_t *indexed_pixels,
                        const uint8_t palette[256][3]);
int descent_sdl_present_6bit(descent_sdl *platform,
                             const uint8_t *indexed_pixels,
                             const uint8_t palette[256 * 3]);
uint64_t descent_sdl_ticks(void);
void descent_sdl_delay(uint32_t milliseconds);
const char *descent_sdl_error(void);

#endif
