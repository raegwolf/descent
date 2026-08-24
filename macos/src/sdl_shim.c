#include "sdl_shim.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>

struct descent_sdl {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint32_t rgba[DESCENT_SCREEN_WIDTH * DESCENT_SCREEN_HEIGHT];
    int dumped_test_frame;
    unsigned presented_frames;
};

static void dump_test_frame(descent_sdl *platform)
{
    const char *path = getenv("DESCENT_FRAME_DUMP");
    FILE *output;
    int index;
    if (platform->dumped_test_frame || platform->presented_frames < 30 ||
        path == NULL || path[0] == '\0')
        return;
    output = fopen(path, "wb");
    if (output == NULL)
        return;
    fprintf(output, "P6\n%d %d\n255\n", DESCENT_SCREEN_WIDTH,
            DESCENT_SCREEN_HEIGHT);
    for (index = 0; index < DESCENT_SCREEN_WIDTH * DESCENT_SCREEN_HEIGHT; ++index) {
        uint32_t pixel = platform->rgba[index];
        fputc((int)((pixel >> 16) & 255), output);
        fputc((int)((pixel >> 8) & 255), output);
        fputc((int)(pixel & 255), output);
    }
    fclose(output);
    platform->dumped_test_frame = 1;
    if (getenv("DESCENT_SMOKE_EXIT") != NULL)
        exit(0);
}

descent_sdl *descent_sdl_create(const char *title)
{
    descent_sdl *platform;

    if (!SDL_SetAppMetadata("Descent", "0.1", "org.descent.port"))
        return NULL;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        return NULL;

    platform = (descent_sdl *)calloc(1, sizeof(*platform));
    if (platform == NULL)
        return NULL;

    platform->window = SDL_CreateWindow(title, 960, 600, SDL_WINDOW_RESIZABLE);
    if (platform->window == NULL)
        goto error;
    platform->renderer = SDL_CreateRenderer(platform->window, NULL);
    if (platform->renderer == NULL)
        goto error;
    if (!SDL_SetRenderLogicalPresentation(platform->renderer,
                                           DESCENT_SCREEN_WIDTH,
                                           DESCENT_SCREEN_HEIGHT,
                                           SDL_LOGICAL_PRESENTATION_LETTERBOX))
        goto error;
    platform->texture = SDL_CreateTexture(platform->renderer,
                                           SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           DESCENT_SCREEN_WIDTH,
                                           DESCENT_SCREEN_HEIGHT);
    if (platform->texture == NULL)
        goto error;
    if (!SDL_SetTextureScaleMode(platform->texture, SDL_SCALEMODE_NEAREST))
        goto error;
    return platform;

error:
    descent_sdl_destroy(platform);
    return NULL;
}

void descent_sdl_destroy(descent_sdl *platform)
{
    if (platform != NULL) {
        SDL_DestroyTexture(platform->texture);
        SDL_DestroyRenderer(platform->renderer);
        SDL_DestroyWindow(platform->window);
        free(platform);
    }
    SDL_Quit();
}

descent_input descent_sdl_poll_input(descent_sdl *platform)
{
    SDL_Event event;
    (void)platform;

    if (!SDL_PollEvent(&event))
        return DESCENT_INPUT_NONE;
    if (event.type == SDL_EVENT_QUIT)
        return DESCENT_INPUT_QUIT;
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat)
        return DESCENT_INPUT_NONE;

    switch (event.key.key) {
        case SDLK_UP:
            return DESCENT_INPUT_UP;
        case SDLK_DOWN:
            return DESCENT_INPUT_DOWN;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE:
            return DESCENT_INPUT_ACCEPT;
        case SDLK_Q:
            if ((event.key.mod & SDL_KMOD_GUI) != 0)
                return DESCENT_INPUT_QUIT;
            break;
        default:
            break;
    }
    return DESCENT_INPUT_NONE;
}

static int pc_scancode(SDL_Scancode code)
{
    switch (code) {
        case SDL_SCANCODE_ESCAPE: return 0x01;
        case SDL_SCANCODE_1: return 0x02; case SDL_SCANCODE_2: return 0x03;
        case SDL_SCANCODE_3: return 0x04; case SDL_SCANCODE_4: return 0x05;
        case SDL_SCANCODE_5: return 0x06; case SDL_SCANCODE_6: return 0x07;
        case SDL_SCANCODE_7: return 0x08; case SDL_SCANCODE_8: return 0x09;
        case SDL_SCANCODE_9: return 0x0a; case SDL_SCANCODE_0: return 0x0b;
        case SDL_SCANCODE_MINUS: return 0x0c; case SDL_SCANCODE_EQUALS: return 0x0d;
        case SDL_SCANCODE_BACKSPACE: return 0x0e; case SDL_SCANCODE_TAB: return 0x0f;
        case SDL_SCANCODE_Q: return 0x10; case SDL_SCANCODE_W: return 0x11;
        case SDL_SCANCODE_E: return 0x12; case SDL_SCANCODE_R: return 0x13;
        case SDL_SCANCODE_T: return 0x14; case SDL_SCANCODE_Y: return 0x15;
        case SDL_SCANCODE_U: return 0x16; case SDL_SCANCODE_I: return 0x17;
        case SDL_SCANCODE_O: return 0x18; case SDL_SCANCODE_P: return 0x19;
        case SDL_SCANCODE_LEFTBRACKET: return 0x1a; case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
        case SDL_SCANCODE_RETURN: return 0x1c; case SDL_SCANCODE_LCTRL: return 0x1d;
        case SDL_SCANCODE_A: return 0x1e; case SDL_SCANCODE_S: return 0x1f;
        case SDL_SCANCODE_D: return 0x20; case SDL_SCANCODE_F: return 0x21;
        case SDL_SCANCODE_G: return 0x22; case SDL_SCANCODE_H: return 0x23;
        case SDL_SCANCODE_J: return 0x24; case SDL_SCANCODE_K: return 0x25;
        case SDL_SCANCODE_L: return 0x26; case SDL_SCANCODE_SEMICOLON: return 0x27;
        case SDL_SCANCODE_APOSTROPHE: return 0x28; case SDL_SCANCODE_GRAVE: return 0x29;
        case SDL_SCANCODE_LSHIFT: return 0x2a; case SDL_SCANCODE_BACKSLASH: return 0x2b;
        case SDL_SCANCODE_Z: return 0x2c; case SDL_SCANCODE_X: return 0x2d;
        case SDL_SCANCODE_C: return 0x2e; case SDL_SCANCODE_V: return 0x2f;
        case SDL_SCANCODE_B: return 0x30; case SDL_SCANCODE_N: return 0x31;
        case SDL_SCANCODE_M: return 0x32; case SDL_SCANCODE_COMMA: return 0x33;
        case SDL_SCANCODE_PERIOD: return 0x34; case SDL_SCANCODE_SLASH: return 0x35;
        case SDL_SCANCODE_RSHIFT: return 0x36; case SDL_SCANCODE_KP_MULTIPLY: return 0x37;
        case SDL_SCANCODE_LALT: return 0x38; case SDL_SCANCODE_SPACE: return 0x39;
        case SDL_SCANCODE_CAPSLOCK: return 0x3a;
        case SDL_SCANCODE_F1: return 0x3b; case SDL_SCANCODE_F2: return 0x3c;
        case SDL_SCANCODE_F3: return 0x3d; case SDL_SCANCODE_F4: return 0x3e;
        case SDL_SCANCODE_F5: return 0x3f; case SDL_SCANCODE_F6: return 0x40;
        case SDL_SCANCODE_F7: return 0x41; case SDL_SCANCODE_F8: return 0x42;
        case SDL_SCANCODE_F9: return 0x43; case SDL_SCANCODE_F10: return 0x44;
        case SDL_SCANCODE_F11: return 0x57; case SDL_SCANCODE_F12: return 0x58;
        case SDL_SCANCODE_KP_7: return 0x47; case SDL_SCANCODE_KP_8: return 0x48;
        case SDL_SCANCODE_KP_9: return 0x49; case SDL_SCANCODE_KP_MINUS: return 0x4a;
        case SDL_SCANCODE_KP_4: return 0x4b; case SDL_SCANCODE_KP_5: return 0x4c;
        case SDL_SCANCODE_KP_6: return 0x4d; case SDL_SCANCODE_KP_PLUS: return 0x4e;
        case SDL_SCANCODE_KP_1: return 0x4f; case SDL_SCANCODE_KP_2: return 0x50;
        case SDL_SCANCODE_KP_3: return 0x51; case SDL_SCANCODE_KP_0: return 0x52;
        case SDL_SCANCODE_KP_PERIOD: return 0x53;
        case SDL_SCANCODE_UP: return 0xc8; case SDL_SCANCODE_DOWN: return 0xd0;
        case SDL_SCANCODE_LEFT: return 0xcb; case SDL_SCANCODE_RIGHT: return 0xcd;
        case SDL_SCANCODE_INSERT: return 0xd2; case SDL_SCANCODE_DELETE: return 0xd3;
        case SDL_SCANCODE_HOME: return 0xc7; case SDL_SCANCODE_END: return 0xcf;
        case SDL_SCANCODE_PAGEUP: return 0xc9; case SDL_SCANCODE_PAGEDOWN: return 0xd1;
        case SDL_SCANCODE_RCTRL: return 0x9d; case SDL_SCANCODE_RALT: return 0xb8;
        case SDL_SCANCODE_KP_ENTER: return 0x9c; case SDL_SCANCODE_KP_DIVIDE: return 0xb5;
        default: return 0;
    }
}

int descent_sdl_poll_key(descent_sdl *platform, int *scan, int *pressed)
{
    SDL_Event event;
    (void)platform;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            *scan = 0x01;
            *pressed = 1;
            return 1;
        }
        if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
            int mapped = pc_scancode(event.key.scancode);
            if (mapped != 0) {
                *scan = mapped;
                *pressed = event.type == SDL_EVENT_KEY_DOWN;
                return 1;
            }
        }
    }
    return 0;
}

int descent_sdl_present(descent_sdl *platform,
                        const uint8_t *indexed_pixels,
                        const uint8_t palette[256][3])
{
    int pixel_count = DESCENT_SCREEN_WIDTH * DESCENT_SCREEN_HEIGHT;
    int index;

    for (index = 0; index < pixel_count; ++index) {
        const uint8_t *rgb = palette[indexed_pixels[index]];
        platform->rgba[index] = UINT32_C(0xff000000) |
                                ((uint32_t)rgb[0] << 16) |
                                ((uint32_t)rgb[1] << 8) |
                                rgb[2];
    }

    platform->presented_frames++;
    dump_test_frame(platform);

    if (!SDL_UpdateTexture(platform->texture, NULL, platform->rgba,
                           DESCENT_SCREEN_WIDTH * (int)sizeof(uint32_t)))
        return 0;
    if (!SDL_SetRenderDrawColor(platform->renderer, 0, 0, 0, 255))
        return 0;
    if (!SDL_RenderClear(platform->renderer))
        return 0;
    if (!SDL_RenderTexture(platform->renderer, platform->texture, NULL, NULL))
        return 0;
    if (!SDL_RenderPresent(platform->renderer))
        return 0;
    return 1;
}

int descent_sdl_present_6bit(descent_sdl *platform,
                             const uint8_t *indexed_pixels,
                             const uint8_t palette[256 * 3])
{
    uint8_t expanded[256][3];
    int color;
    int component;

    for (color = 0; color < 256; ++color) {
        for (component = 0; component < 3; ++component) {
            uint8_t value = palette[color * 3 + component] & 63;
            expanded[color][component] = (uint8_t)((value << 2) | (value >> 4));
        }
    }
    return descent_sdl_present(platform, indexed_pixels, expanded);
}

uint64_t descent_sdl_ticks(void)
{
    return SDL_GetTicks();
}

void descent_sdl_delay(uint32_t milliseconds)
{
    SDL_Delay(milliseconds);
}

const char *descent_sdl_error(void)
{
    return SDL_GetError();
}
