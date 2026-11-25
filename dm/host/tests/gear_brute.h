


#ifndef GEAR_BRUTE_H
#define GEAR_BRUTE_H
#include "../dm_gear_search.h"

#define BRUTE_MAXGOAL 16
typedef struct { int n; int16_t a[DM_GEAR_MAXCAND * 4], b[DM_GEAR_MAXCAND * 4]; } brute_choices;
typedef struct {
    brute_choices place[8];
    int nfree; int16_t freec[DM_GEAR_MAXCAND];
    uint8_t keep[DM_GEAR_MAXCAND];
} brute_plan;
void brute_make_plan(const dm_gear_problem *p, brute_plan *pl);
int brute_fit(const dm_gear_problem *p, const uint8_t *take);

typedef void (*brute_fn)(const dm_gear_pick *pk, const int *choice, int depth, void *ctx);
void brute_walk(const dm_gear_problem *p, const brute_plan *pl, brute_fn fn, void *ctx);

typedef struct {
    double key[BRUTE_MAXGOAL][3];
    int kept[BRUTE_MAXGOAL];
    dm_gear_pick best[BRUTE_MAXGOAL];
    long combos, placed, rated;
    dm_gear_measures now;
} brute_out;

int gear_brute(const dm_gear_problem *p, const dm_gear_goal *goals, int ngoals, brute_out *o);
#endif
