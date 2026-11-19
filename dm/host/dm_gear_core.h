


#ifndef DM_GEAR_CORE_H
#define DM_GEAR_CORE_H
#include "dm_sim_core.h"

#define DM_GEAR_PACK_EXTRA 4
#define DM_GEAR_PACK_UNITS (DM_GEAR_PACK_EXTRA + 1)
#define DM_GEAR_AREA_HITS 3
#define DM_GEAR_MANA_FRAMES (10 * DM_SIM_FPS)
#define DM_GEAR_SINGLE_FRAMES (30 * DM_SIM_FPS)
#define DM_GEAR_STEP 0.5
#define DM_GEAR_HITS_PER 10

enum { DM_GEAR_M_GROUP, DM_GEAR_M_SINGLE, DM_GEAR_M_SURVIVAL, DM_GEAR_M_RECOVERY, DM_GEAR_M_N };
typedef struct {





    double group;


    double single;
    double survival_s;
    double hits;
    double recovery;
    int skill;
} dm_gear_measures;



int dm_gear_measure(const dm_sim_data *d, const dm_sim_char *c, int splash_on, dm_gear_measures *m);



int dm_gear_measure_survival(const dm_sim_data *d, const dm_sim_char *c, dm_gear_measures *m);
int dm_gear_measure_damage(const dm_sim_data *d, const dm_sim_char *c, int splash_on, dm_gear_measures *m);

int dm_gear_boss_life(const dm_sim_data *d, int diff, int act);

typedef struct {
    double survival_hits;
    double boss_share;
} dm_gear_goal;
enum { DM_GEAR_GOAL_NONE, DM_GEAR_GOAL_MISSED, DM_GEAR_GOAL_REACHED };
typedef struct {
    double change[DM_GEAR_M_N];
    double hits;
    int damage_base;
    double damage;
    double survival;



    int dots_damage, dots_survival;
    int goal;


    double key[3];
} dm_gear_score_out;

double dm_gear_hits_key(double hits, double goal);

double dm_gear_damage_key(const dm_gear_measures *now, const dm_gear_measures *after, const dm_gear_goal *goal);

double dm_gear_steps(double v);
void dm_gear_score(const dm_gear_measures *now, const dm_gear_measures *after, const dm_gear_goal *goal, dm_gear_score_out *o);


#define DM_GEAR_DOTS_DAMAGE(code) ((code) & 7)
#define DM_GEAR_DOTS_SURVIVAL(code) (((code) >> 3) & 7)
static inline int dm_gear_dots_code(const dm_gear_score_out *o) { return (o->damage_base ? o->dots_damage + 4 : 0) | (o->dots_survival + 4) << 3; }


typedef struct { int ok, skill, cast, before, after, next; } dm_gear_frames;


int dm_gear_recover_frames(const dm_sim_data *d, const char *token, const char *wclass, int block, int holy, int v);



enum { DM_GEAR_R_DEFENSE, DM_GEAR_R_LIFE, DM_GEAR_R_MANA, DM_GEAR_R_REGEN, DM_GEAR_R_LEECH, DM_GEAR_R_RESIST, DM_GEAR_R_CAST, DM_GEAR_R_ATTACK_SPEED,
       DM_GEAR_R_HIT_RECOVERY, DM_GEAR_R_SKILLS, DM_GEAR_R_DAMAGE, DM_GEAR_R_N };
typedef struct {
    int group[DM_GEAR_M_N];
    double share[DM_GEAR_M_N];
} dm_gear_reasons_out;

int dm_gear_reasons(const dm_sim_data *d, const dm_sim_char *now, const dm_sim_char *after, const dm_gear_measures *mnow, const dm_gear_measures *mafter,
                    int splash_on, dm_gear_reasons_out *o);

enum { DM_GEAR_LINES = 12, DM_GEAR_LINEB = 240 };
typedef struct {
    dm_gear_measures now, after;
    dm_gear_score_out score;
    dm_gear_reasons_out reasons;
    int dir[DM_GEAR_M_N];
    int act;
    double goal_hits;


    int immune[DM_EL_N];
    dm_sim_grant_miss grant;



    dm_gear_frames main, recover, block;
} dm_gear_tip;


enum { DM_GEAR_RATE_GRANTS = 1 };
int dm_gear_tip_rate(const dm_sim_data *d, const dm_sim_char *now, const dm_sim_char *after, const dm_gear_goal *goal, int splash_on, dm_gear_tip *t);


typedef struct {
    const char *measure[DM_GEAR_M_N];
    const char *group[DM_GEAR_R_N];
    const char *more, *less;
    const char *missed, *reached;
    const char *below, *below_same;
} dm_gear_words;





int dm_gear_tip_text(const dm_gear_tip *t, const dm_gear_words *w, int comma, char line[][DM_GEAR_LINEB], int max);
#endif
