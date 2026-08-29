#ifndef DESCENT_ESP32_BRIDGE_H
#define DESCENT_ESP32_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *esp32_alloc_psram(unsigned int size);
void esp32_free_psram(void *buffer);
void esp32_present_indexed(const unsigned char *pixels,
                             const unsigned char *palette,
                             int width, int height);
uint32_t esp32_milliseconds(void);
void esp32_delay_ms(unsigned int milliseconds);
int esp32_check_heap_integrity(void);
void esp32_poll_joysticks(void);

int inferno_init(int argc, char **argv);
void function_loop(void);
int inferno_done(void);
void StartNewGame(int level_num);
void timer_init(void);
void timer_close(void);
void key_init(void);
void key_close(void);
extern int Skip_briefing_screens;
int esp32_init_world_storage(void);
int esp32_init_ai_storage(void);
int esp32_init_object_storage(void);
int esp32_init_polygon_storage(void);
int esp32_init_automap_storage(void);
int esp32_init_morph_storage(void);
int esp32_init_piggy_storage(void);
int esp32_init_bitmap_storage(void);
int esp32_init_lighting_storage(void);
int esp32_init_robot_storage(void);
int esp32_init_effect_storage(void);

#ifdef __cplusplus
}
#endif

#endif
