


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



    int order;
    dm_gear_list least;
    int least_kick_min, least_kick_max;
} dm_gear_bound_in;




enum {
    DM_GEAR_HOW_CASTS = 1 << 0,
    DM_GEAR_HOW_FREE = 1 << 2,
    DM_GEAR_HOW_LONG = 1 << 3,
    DM_GEAR_HOW_NODES = 1 << 4,

    DM_GEAR_HOW_P_HANDS = 1 << 5,
    DM_GEAR_HOW_P_ORDERED = 1 << 6,
    DM_GEAR_HOW_P_GRANTS = 1 << 7,
    DM_GEAR_HOW_P_BUFF = 1 << 8,
    DM_GEAR_HOW_P_AURAS = 1 << 9,
    DM_GEAR_HOW_P_LEAST = 1 << 10,
    DM_GEAR_HOW_P_NONE = 1 << 1,

    DM_GEAR_HOW_A_KIND = 1 << 11,
    DM_GEAR_HOW_A_READY = 1 << 12,
    DM_GEAR_HOW_X_TERMS = 1 << 13,
};

typedef struct {
    double group, single, survival_s, hits, recovery;
    unsigned how;
} dm_gear_bound_out;

int dm_gear_bound_eval(const dm_gear_problem *p, const dm_gear_char *now, const dm_gear_bound_in *in, dm_gear_bound_out *o);

void dm_gear_bound_key(const dm_gear_measures *now, const dm_gear_bound_out *b, const dm_gear_goal *goal, double key[3]);




#define DM_GEAR_CASTS_MAXA 12
#define DM_GEAR_CASTS_MAXG 4
#define DM_GEAR_CASTS_MAXN 40
#define DM_GEAR_CASTS_NODES 4000
typedef struct {
    int n, ng;
    double d[DM_GEAR_CASTS_MAXA], f[DM_GEAR_CASTS_MAXA], c[DM_GEAR_CASTS_MAXA], credit[DM_GEAR_CASTS_MAXA], uses[DM_GEAR_CASTS_MAXA];
    unsigned below[DM_GEAR_CASTS_MAXG];
    double cap[DM_GEAR_CASTS_MAXG];
    double span, mana;
} dm_gear_casts;


int dm_gear_casts_reach(const dm_gear_casts *x, double target, long *nodes, double *best);

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
