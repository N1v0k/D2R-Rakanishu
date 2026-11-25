


#ifndef DM_GEAR_SEARCH_H
#define DM_GEAR_SEARCH_H
#include "dm_gear_compose.h"
#include "dm_gear_core.h"
#include "dm_itemcmp_core.h"
#include <stdatomic.h>




enum { DM_GEAR_K_NONE, DM_GEAR_K_HEAD, DM_GEAR_K_AMULET, DM_GEAR_K_ARMOR, DM_GEAR_K_RING, DM_GEAR_K_BELT, DM_GEAR_K_BOOTS, DM_GEAR_K_GLOVES,
       DM_GEAR_K_CHARM, DM_GEAR_K_WEAPON1, DM_GEAR_K_WEAPON2, DM_GEAR_K_BOW, DM_GEAR_K_XBOW, DM_GEAR_K_SHIELD, DM_GEAR_K_ARROWS, DM_GEAR_K_BOLTS,
       DM_GEAR_K_N };

enum {
    DM_GEAR_U_GRANT = 1,
    DM_GEAR_U_CHARMREQ = 2,
    DM_GEAR_U_LEVEL = 4,
    DM_GEAR_U_FACTS = 8,
    DM_GEAR_U_SKILL = 16
};


typedef struct {
    dm_sim_weapon w;
    int mode2;
    int strb, dexb, ar_zero, flat;
    int mcrit;
    int16_t mth[DM_SIM_MAXSKILL], mdmg[DM_SIM_MAXSKILL];


    int has_one;
    dm_sim_weapon one;
} dm_gear_weapon;

typedef struct {
    dm_gear_item item;
    int kind;
    int w, h;
    int inv, x, y;
    int worn;
    int weapon;
    int block;
    int dmin, dmax;
    unsigned unrated;
    int cls_only;
} dm_gear_cand;


enum { DM_GEAR_P_MAIN, DM_GEAR_P_OFF, DM_GEAR_P_ARMOR, DM_GEAR_P_AMULET, DM_GEAR_P_RING1, DM_GEAR_P_RING2, DM_GEAR_P_HEAD, DM_GEAR_P_BELT,
       DM_GEAR_P_BOOTS, DM_GEAR_P_GLOVES, DM_GEAR_P_N };
#define DM_GEAR_GRID_W 10
#define DM_GEAR_GRID_H 4
typedef struct {
    const dm_sim_data *d;
    const dm_gear_tables *t;
    dm_gear_base base;

    dm_sim_char now;
    dm_ic_cb cb;
    uint8_t node[DM_SIM_MAXSKILL];
    uint8_t prog[DM_SIM_MAXSKILL];
    int16_t tab[DM_SIM_MAXSKILL];
    int16_t elem[DM_SIM_MAXSKILL];



    uint8_t moves[DM_SIM_MAXSKILL];
    int ncand;
    dm_gear_cand *cand;
    int nweapon;
    const dm_gear_weapon *weapon;
    int unarmed;
    uint8_t grid[DM_GEAR_GRID_H][DM_GEAR_GRID_W];


    int charms_fixed;

    int ready;
    int16_t sv[80];
    dm_gear_char nowc;
    int16_t off_now[DM_GEAR_P_N];
    int16_t hands_now[2];



    int set2;
    int16_t set2_now[DM_SIM_MAXGRANT];


    int splash_on;



    uint8_t dr[DM_GEAR_NSTAT];
    int dr_ok, idle;



    int ref_sv[DM_GEAR_NSTAT], ref_cb[DM_GEAR_NSTAT];


    uint8_t hand_mixed[DM_GEAR_NSTAT];
} dm_gear_problem;

int dm_gear_problem_ready(dm_gear_problem *p);


int dm_gear_one_hand(const dm_gear_problem *p, int c);


int dm_gear_pair_weapon(const dm_gear_problem *p, int c);


typedef struct {
    dm_ic_cb cb;
    dm_sim_weapon w;
    int ias, ds, mcs;
} dm_gear_hand;




int dm_gear_dual_stats(const dm_sim_data *d, const uint8_t *dr, const int *const val[2], int *stat_value);





int dm_gear_dual_hands(const dm_sim_data *d, dm_sim_char *c, const dm_gear_hand h[2], const int *const val[2]);






int dm_gear_hand_passive(const dm_sim_data *d, const dm_sim_char *now, dm_sim_char *x, dm_ic_cb *cb, const int *mth, const int *mdmg, int mcrit, const int *prog);


static inline void dm_gear_dual_skill(int ab0, int ab1, int dm0, int dm1, int prog, int *ar_bonus, int *delta) {
    *ar_bonus = (ab0 + ab1) / 2 + prog; *delta = (dm0 + dm1) / 2;
}


#define DM_GEAR_MAXCAND 1024
typedef struct {
    int16_t at[DM_GEAR_P_N];
    uint8_t charm[DM_GEAR_MAXCAND];
} dm_gear_pick;


int dm_gear_pick_kept(const dm_gear_pick *now, const dm_gear_pick *pk);


enum {
    DM_GEAR_L_COMPOSE = 1,
    DM_GEAR_L_COMBAT = 2,
    DM_GEAR_L_PASSIVE = 4,
    DM_GEAR_L_SKILL = 8,
    DM_GEAR_L_MEASURE = 16,
    DM_GEAR_L_PLACE = 32,
    DM_GEAR_L_INPUT = 64,
    DM_GEAR_L_REQ = 128,

    DM_GEAR_L_AMMO = 256,
    DM_GEAR_L_DUAL = 512

};

int dm_gear_leaf_char(const dm_gear_problem *p, const dm_gear_pick *pk, dm_sim_char *out, dm_gear_char *comp, unsigned *flags);


int dm_gear_leaf_char_cb(const dm_gear_problem *p, const dm_gear_pick *pk, dm_sim_char *out, dm_gear_char *comp, unsigned *flags, int *cbv);


int dm_gear_pick_compose(const dm_gear_problem *p, const dm_gear_pick *pk, dm_gear_char *comp, uint8_t active[DM_GEAR_P_N + 1]);



void dm_gear_char_from(const dm_gear_problem *p, const dm_gear_char *now, const dm_gear_char *comp, const dm_gear_char *body, int main, int off,
                       int kick_min, int kick_max, dm_sim_char *out, unsigned *flags);

int dm_gear_leaf_measure(const dm_gear_problem *p, const dm_gear_pick *pk, dm_gear_measures *m, unsigned *flags);

void dm_gear_pick_now(const dm_gear_problem *p, dm_gear_pick *pk);

int dm_gear_key_cmp(const double a[3], const double b[3]);

typedef struct dm_gear_result dm_gear_result;

typedef struct {
    atomic_long runs;
    atomic_long fresh;
    atomic_long bounds;
    atomic_long pruned;
    atomic_long leaves;
    atomic_long estimate;
    atomic_int place;
    atomic_int choice;
    atomic_int found;
    atomic_int share;

    void (*better)(const dm_gear_result *res, void *ctx);
    void *ctx;
} dm_gear_progress;

enum { DM_GEAR_S_DONE = 0, DM_GEAR_S_ABORTED = 1, DM_GEAR_S_NOREF = -1, DM_GEAR_S_INPUT = -2 };
struct dm_gear_result {
    int status;
    dm_gear_pick best;
    dm_gear_measures now, after;
    dm_gear_score_out score;




    int unrated, unrated_worn, filtered, charm_groups;
    long leaves_unrated, tripwire;


    long leaves_partial;
    double raw_combinations;
    int seeded;
};

typedef struct dm_gear_search dm_gear_search;

dm_gear_search *dm_gear_search_new(const dm_gear_problem *p);
void dm_gear_search_free(dm_gear_search *s);


int dm_gear_search_run(dm_gear_search *s, const dm_gear_goal *goal, const atomic_int *stop, dm_gear_progress *prog, dm_gear_result *res);





#define DM_GEAR_MAXTHREADS 6
void dm_gear_search_wide(dm_gear_search *s, int threads, const atomic_int *wide, void (*pace)(int wide));




void dm_gear_search_seed(dm_gear_search *s, const dm_gear_pick *pk);
#ifndef DM_PLAYER_BUILD


void dm_gear_search_test_run_us(dm_gear_search *s, long run_us, long max_runs);



#define DM_GEAR_WALK_HOW 16
typedef struct { long leaves, bounds, halves, above, dual; long how[DM_GEAR_WALK_HOW]; int failed; } dm_gear_walk;
int dm_gear_search_test_walk(dm_gear_search *s, dm_gear_walk *w);
#endif

#define DM_GEAR_MODEL 6



uint64_t dm_gear_problem_hash(const dm_gear_problem *p, const dm_gear_goal *goal);


uint64_t dm_gear_cand_hash(const dm_gear_problem *p, int i);

size_t dm_gear_search_bytes(const dm_gear_search *s);



int dm_gear_place(const dm_gear_problem *p, const uint8_t *take);
#endif
