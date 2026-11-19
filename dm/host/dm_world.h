

#pragma once
unsigned dm_world_tap(float nx, float ny);
int dm_world_tap_result(unsigned seq);
int dm_world_has_target(void);
void dm_world_pump(void);
void dm_world_watch(int type, uint32_t cls);
int dm_world_watch_pos(float *nx, float *ny);


enum { DM_MAP_GOAL_MAX = 24, DM_MG_WP = 255, DM_MG_LVL = 256 };
typedef struct { int n, px, py; float sx, sy; int x[DM_MAP_GOAL_MAX], y[DM_MAP_GOAL_MAX]; unsigned short art[DM_MAP_GOAL_MAX]; } dm_map_goal_view;
int dm_map_goals(dm_map_goal_view *v);
