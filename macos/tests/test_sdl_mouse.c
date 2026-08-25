#undef NDEBUG
#include "sdl_shim.h"

#include <SDL3/SDL.h>

#include <assert.h>

int main(void)
{
    descent_sdl *platform;
    SDL_Event event;
    unsigned downs[3];
    int buttons;
    int pressed;
    int scancode;
    int dx, dy;

    platform = descent_sdl_create("input test");
    assert(platform != NULL);
    assert(SDL_GetHintBoolean(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, false));
    assert(SDL_strcmp(SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE), "10") == 0);

    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = SDL_SCANCODE_W;
    assert(SDL_PushEvent(&event));

    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.xrel = 12;
    event.motion.yrel = -7;
    assert(SDL_PushEvent(&event));

    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.button = SDL_BUTTON_LEFT;
    assert(SDL_PushEvent(&event));

    descent_sdl_poll_mouse(platform, &dx, &dy, &buttons, downs);
    assert(dx == 12);
    assert(dy == -7);
    assert(buttons == 1);
    assert(downs[0] == 1);

    /* Mouse polling must not consume keyboard input. */
    assert(descent_sdl_poll_key(platform, &scancode, &pressed));
    assert(scancode == 0x11);
    assert(pressed == 1);

    /* SDL3 reports floating-point relative motion.  Fractions from separate
     * events must accumulate and any remainder must survive polling. */
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.xrel = 0.375f;
    event.motion.yrel = -0.375f;
    assert(SDL_PushEvent(&event));
    assert(SDL_PushEvent(&event));
    assert(SDL_PushEvent(&event));
    assert(SDL_PushEvent(&event));
    descent_sdl_poll_mouse(platform, &dx, &dy, &buttons, downs);
    assert(dx == 1);
    assert(dy == -1);

    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION;
    event.motion.xrel = 0.5f;
    event.motion.yrel = -0.5f;
    assert(SDL_PushEvent(&event));
    descent_sdl_poll_mouse(platform, &dx, &dy, &buttons, downs);
    assert(dx == 1);
    assert(dy == -1);

    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.button = SDL_BUTTON_LEFT;
    assert(SDL_PushEvent(&event));
    assert(!descent_sdl_poll_key(platform, &scancode, &pressed));
    descent_sdl_poll_mouse(platform, &dx, &dy, &buttons, downs);
    assert(buttons == 0);

    descent_sdl_destroy(platform);
    return 0;
}
