


#ifndef DM_GEAR_BOUND_H
#define DM_GEAR_BOUND_H
#include "dm_gear_search.h"


enum {
    DM_GEAR_SC_IRR,
    DM_GEAR_SC_GOOD,
    DM_GEAR_SC_REQ,
    DM_GEAR_SC_SKILL,
    DM_GEAR_SC_UNKNOWN,
    DM_GEAR_SC_OUT
};

void dm_gear_stat_classes(const dm_gear_problem *p, uint8_t cls[DM_GEAR_NSTAT]);

typedef struct {
    int main, off;
    int kick_min, kick_max;


    dm_gear_list hi, lo;
    int varied;



    int tight;


    int splash_on;
    int hands;

} dm_gear_bound_in;

typedef struct {
    double group, single, survival_s, hits, recovery;
} dm_gear_bound_out;

int dm_gear_bound_eval(const dm_gear_problem *p, const dm_gear_char *now, const dm_gear_bound_in *in, dm_gear_bound_out *o);

void dm_gear_bound_key(const dm_gear_measures *now, const dm_gear_bound_out *b, const dm_gear_goal *goal, double key[3]);

#ifdef DM_GEAR_INSTR




typedef struct {
    int have;
    double single, group;
    double win, boss;



    double lp, pets, auras, item_auras, ow, share, boost;
    double pool, regen;
    double stock, pot_rate;
    double pot_all;
    int astate;
    int ns, crush;
    double field;
    struct { int id, lo, hi, area; double d, dg, p, f, af, cd, c, pet, aura, mix; } sk[DM_SIM_MAXBAR + 1];



    int kill;
    double lp_t, lp_mana, lp_frames, lp_val, credit;


    int gwin;
    double g_best, g_item, g_struck;
} dm_gear_instr_terms;
void dm_gear_instr_cap(int on);
int dm_gear_instr_get_terms(dm_gear_instr_terms *single, dm_gear_instr_terms *group);
#endif
#endif
