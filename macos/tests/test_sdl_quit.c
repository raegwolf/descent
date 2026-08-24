#include "sdl_shim.h"

#include <SDL3/SDL.h>

int main(void)
{
    SDL_Event event;
    int pressed;
    int scancode;

    if (!SDL_Init(SDL_INIT_EVENTS))
        return 2;
    SDL_zero(event);
    event.type = SDL_EVENT_QUIT;
    if (!SDL_PushEvent(&event))
        return 3;

    (void)descent_sdl_poll_key(NULL, &scancode, &pressed);
    return 1;
}
