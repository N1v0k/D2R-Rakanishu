

#pragma once


void front_run(void (*guest)(void));


void *front_metal_layer(void);




#include <stdint.h>
uint64_t front_game_res(int docked);








int front_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int out_cap);





void dm_host_preload_start(int argc, char **argv);







typedef struct { uint32_t id; float x, y; } dm_touch;
void dm_touch_set(const dm_touch *t, int n);







void front_save_failed(int err);
