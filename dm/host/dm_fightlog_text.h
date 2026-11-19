
#pragma once
#include "dm_fightlog.h"


typedef const char *(*dm_fl_namer)(const dm_fl_group *g, int *boss, void *ctx);
uint32_t dm_fl_fight_key(const dm_fl_fight *f);



int dm_fl_simulable(const dm_fl_fight *f, int group, uint32_t build);

int dm_fl_text_list(dm_fl_state *s, int recording, uint32_t build, dm_fl_namer name, void *ctx, char *out, size_t cap);
int dm_fl_text_fight(const dm_fl_fight *f, char *out, size_t cap);



enum { DM_FL_QUERY_DPS = -2 };
enum { DM_FL_CHANGE_LEVEL = 1, DM_FL_CHANGE_POINTS = 2, DM_FL_CHANGE_ATTRS = 4, DM_FL_CHANGE_GEAR = 8, DM_FL_CHANGE_DIFFICULTY = 16 };
int dm_fl_ui_query(int fight, int group, uint32_t key, int sim, char *out, size_t cap);

uint32_t dm_fl_build_key(void);


typedef struct { uint32_t gear, points, attrs; uint16_t level; uint8_t difficulty, pad; } dm_fl_parts;
uint32_t dm_fl_state_key(int difficulty, dm_fl_parts *parts);
