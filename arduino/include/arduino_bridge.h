#ifndef DESCENT_ARDUINO_BRIDGE_H
#define DESCENT_ARDUINO_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *arduino_alloc_psram(unsigned int size);
void arduino_free_psram(void *buffer);
void arduino_present_indexed(const unsigned char *pixels,
                             const unsigned char *palette,
                             int width, int height);
uint32_t arduino_milliseconds(void);
void arduino_delay_ms(unsigned int milliseconds);

int inferno_init(int argc, char **argv);
void function_loop(void);
int inferno_done(void);
void StartNewGame(int level_num);
void timer_init(void);
void timer_close(void);
void key_init(void);
void key_close(void);
extern int Skip_briefing_screens;
int arduino_init_world_storage(void);
int arduino_init_ai_storage(void);
int arduino_init_object_storage(void);
int arduino_init_polygon_storage(void);
int arduino_init_automap_storage(void);
int arduino_init_morph_storage(void);
int arduino_init_piggy_storage(void);
int arduino_init_bitmap_storage(void);
int arduino_init_lighting_storage(void);
int arduino_init_robot_storage(void);
int arduino_init_effect_storage(void);

#ifdef __cplusplus
}
#endif

#endif
