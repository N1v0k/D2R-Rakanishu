


#ifndef DM_GEAR_LIVE_H
#define DM_GEAR_LIVE_H
#include "dm_gear_core.h"
#include <stdint.h>

#define DM_GEAR_DEFAULT_HITS 15
#define DM_GEAR_DEFAULT_BOSS 0.5

enum { DM_GEAR_TIP_UNKNOWN = -1, DM_GEAR_TIP_PENDING = 0, DM_GEAR_TIP_DONE = 1, DM_GEAR_TIP_FAILED = 2, DM_GEAR_TIP_GRANTS = 3 };


int dm_gear_tip_get(uint64_t key, dm_gear_tip *out, uint64_t *asked);


int dm_gear_tip_put(uint64_t key, const char *set, const dm_sim_char *now, const dm_sim_char *after, const dm_gear_goal *goal, int splash_on, uint64_t frame);

double dm_gear_tip_ms(void);



void dm_gear_goal_set(int session, int survival_hits, int boss_percent);
void dm_gear_goal_get(int session, dm_gear_goal *g);






enum { DM_GEAR_DOTS_BOX = 10 };
void dm_gear_dots_set(uint64_t frame, int dots, const float box[DM_GEAR_DOTS_BOX]);
int dm_gear_dots_get(uint64_t now, float box[DM_GEAR_DOTS_BOX]);
#endif
