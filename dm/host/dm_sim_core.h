


#ifndef DM_SIM_CORE_H
#define DM_SIM_CORE_H
#include <stddef.h>
#include <stdint.h>

#define DM_SIM_FPS 25
#define DM_SIM_MAXSKILL 512

enum { DM_SK_CURSE_LIFESTEAL = 82, DM_SK_SMITE = 97, DM_SK_SHIELD_BUFF = 117,
       DM_SK_DRAGON_TALON = 255, DM_SK_DRAGON_CLAW = 260, DM_SK_DRAGON_TAIL = 270,
       DM_SK_ECHOING_STRIKE = 388, DM_SK_BLADE_WARP = 390 };
#define DM_SIM_MAXMISS 1024
#define DM_SIM_MAXBAR 64
#define DM_SIM_HORIZON (600 * DM_SIM_FPS)

enum { DM_EL_PHYS, DM_EL_FIRE, DM_EL_LTNG, DM_EL_COLD, DM_EL_POIS, DM_EL_MAG, DM_EL_N };


enum {
    DM_FAM_W1 = 1 << 0, DM_FAM_W2 = 1 << 1, DM_FAM_W3 = 1 << 2, DM_FAM_W4 = 1 << 3, DM_FAM_W5 = 1 << 4,
    DM_FAM_Z1 = 1 << 5, DM_FAM_Z2 = 1 << 6, DM_FAM_Z3 = 1 << 7, DM_FAM_DOT = 1 << 8, DM_FAM_L = 1 << 9,
    DM_FAM_P = 1 << 10, DM_FAM_F = 1 << 11, DM_FAM_AOFF = 1 << 12, DM_FAM_ADMG = 1 << 13, DM_FAM_ADEF = 1 << 14,
    DM_FAM_K = 1 << 15, DM_FAM_B = 1 << 16, DM_FAM_PA = 1 << 17, DM_FAM_U = 1 << 18
};
#define DM_FAM_WEAPON (DM_FAM_W1 | DM_FAM_W2 | DM_FAM_W3 | DM_FAM_W4 | DM_FAM_W5)
#define DM_FAM_SPELL (DM_FAM_Z1 | DM_FAM_Z2 | DM_FAM_Z3 | DM_FAM_L)
#define DM_FAM_MODIFIER (DM_FAM_AOFF | DM_FAM_ADEF | DM_FAM_K | DM_FAM_B | DM_FAM_PA)


enum {
    DM_CF_CALC1 = 0, DM_CF_TOHIT = 10, DM_CF_DMGSYM, DM_CF_EDMGSYM, DM_CF_ELENSYM, DM_CF_AURALEN, DM_CF_AURARANGE, DM_CF_PETMAX,
    DM_CF_PRG1, DM_CF_AURASTAT1 = DM_CF_PRG1 + 3, DM_CF_PASSIVE1 = DM_CF_AURASTAT1 + 6, DM_CF_SUMSK1 = DM_CF_PASSIVE1 + 14,
    DM_CF_DDAM1 = DM_CF_SUMSK1 + 5, DM_CF_DDAM2, DM_CF_PERDELAY, DM_CF_STRIKEPERIOD, DM_CF_BURNLEN, DM_CF_N
};
enum { DM_MF_DMG, DM_MF_RANGE, DM_MF_DMGSYM, DM_MF_EDMGSYM, DM_MF_SRV1, DM_MF_N };

enum { DM_ROLE_DMGPCT, DM_ROLE_FLAT, DM_ROLE_ELPCT, DM_ROLE_SIAS, DM_ROLE_HITS, DM_ROLE_ATTACKS, DM_ROLE_AFTER1, DM_ROLE_NMISS,
       DM_ROLE_JUMPS, DM_ROLE_PETLVL, DM_ROLE_PETHP, DM_ROLE_LL, DM_ROLE_DUR, DM_ROLE_CHARGES, DM_ROLE_FIREPCT, DM_ROLE_COLDPCT, DM_ROLE_LTNGPCT, DM_ROLE_N };


enum {
    DM_ST_STR, DM_ST_DEX, DM_ST_VIT, DM_ST_ENE, DM_ST_MAXHP, DM_ST_MAXMANA, DM_ST_HPREGEN, DM_ST_MANAREGEN, DM_ST_AC, DM_ST_AC_PCT,
    DM_ST_TOHIT, DM_ST_TOHIT_PCT, DM_ST_TOBLOCK, DM_ST_DMG_PCT, DM_ST_DS, DM_ST_CB, DM_ST_OW, DM_ST_CRIT, DM_ST_LL, DM_ST_ML, DM_ST_IAS,
    DM_ST_FCR, DM_ST_FHR, DM_ST_FBR, DM_ST_SIAS, DM_ST_RES_FIRE, DM_ST_RES_LTNG, DM_ST_RES_COLD, DM_ST_RES_POIS, DM_ST_RES_PHYS, DM_ST_RES_MAG,
    DM_ST_MAXRES_FIRE, DM_ST_MAXRES_LTNG, DM_ST_MAXRES_COLD, DM_ST_MAXRES_POIS, DM_ST_DR_FLAT, DM_ST_MDR_FLAT, DM_ST_MAST_FIRE, DM_ST_MAST_LTNG,
    DM_ST_MAST_COLD, DM_ST_MAST_POIS, DM_ST_MAST_MAG, DM_ST_PIERCE_FIRE, DM_ST_PIERCE_LTNG, DM_ST_PIERCE_COLD, DM_ST_PIERCE_POIS,
    DM_ST_PIERCE_MAG, DM_ST_PIERCE_PHYS, DM_ST_PREVENTHEAL, DM_ST_HP_PCT, DM_ST_N
};

typedef struct { int16_t rp, msb, freezediv, colddiv, lsd, msd, ubonus, cbonus, pvspe, mercboss, pevspet, pevsmerc, sfmin, cedmg; } dm_sim_diffc;
typedef struct { char code[4], token[3]; int16_t tohitf, blockf, mincast, manaregen, lifevit, manamag, hppot, mppot; } dm_sim_class;

enum { DM_POT_HP1 = 0, DM_POT_MP1 = 5, DM_POT_RVS = 10, DM_POT_RVL, DM_POT_N };
enum { DM_PK_HEAL, DM_PK_MANA, DM_PK_REJUV };

typedef struct { int16_t kind; int32_t life, mana, len; } dm_sim_potion;
typedef struct { char token[3], mode[3], wclass[4]; int16_t frames, speed, action; } dm_sim_anim;

typedef struct {
    int16_t id, cls;
    uint32_t fam;
    char name[40], anim[4], range[8], pettype[16], aurastate[32], passivestate[32], auratargetstate[32];
    int16_t hitfactor, frameburn, timed, burnmiss, burnfactor, unavailable;
    int16_t descdam, srvstfunc, srvdofunc, seqnum, aura, passive, summon, localdelay, globaldelay;
    int16_t mana, lvlmana, minmana, manashift, tohit, levtohit, hitshift, srcdam;
    int32_t mindam, maxdam, minlev[5], maxlev[5], emin, emax, eminlev[5], emaxlev[5], elen, elevlen[3];
    int8_t etype;
    int8_t area;
    int16_t kick, periodic, perdelay, alwayshit, useattackrate, requiresweapon, reqlevel, maxlvl, missile, descmissile;
    int8_t weapsel;
    int32_t param[20];
    int16_t sumskill[5], variants[4];
    int16_t astat[6], pstat[14];
    int8_t role[DM_ROLE_N];
    int8_t rollback_param;
    int32_t calc[DM_CF_N];
} dm_sim_skill;

typedef struct {
    int16_t typefilter, hittypes, otherpct;
    int16_t apply_mastery, srcmissdamage, missile_skill, srvdofunc, initsteps, accel, maxvel;
    int16_t id, vel, nexthit, nextdelay, srcdamage, hitshift, skill, sub[3], hitsub[4], explosion;
    int32_t mindam, maxdam, minlev[5], maxlev[5], emin, emax, eminlev[5], emaxlev[5], elen, elevlen[3], param[5];
    int8_t etype;
    int32_t calc[DM_MF_N];
} dm_sim_miss;

enum { DM_MF_BOSS = 1, DM_MF_PRIME = 2, DM_MF_MELEE = 4, DM_MF_RANGED = 8, DM_MF_UNDEAD = 16, DM_MF_DEMON = 32, DM_MF_NORATIO = 64,
       DM_MF_SHIELD = 128, DM_MF_KILLABLE = 256, DM_MF_ENABLED = 512, DM_MF_NOPMH = 1024, DM_MF_CRITTER = 2048, DM_MF_PET = 4096, DM_MF_UNDERESTIMATED = 8192 };
typedef struct {
    int32_t s1[3];
    int32_t hpmin, hpmax, ac, a1min, a1max, a1th, a2min, a2max, a2th, res[6], block, drain, coldeffect, aidel, elpct[3], elmin[3], elmax[3], eldur[3];
} dm_sim_mond;
typedef struct {
    char id[24], namekey[40];
    int32_t flags, lvl[3], crit, regen, grpmin, grpmax, skilldamage;
    int16_t skills[8];
    char elmode[3][3];
    int8_t eltype[3];
    int16_t a1[3], a2[3];
    dm_sim_mond d[3];
} dm_sim_mon;
typedef struct { int16_t id, act, mlvl[3], rotw; char namekey[40]; } dm_sim_area;
typedef struct { int16_t area, list, n; int16_t mon[25]; } dm_sim_spawn;
typedef struct { int16_t idx, mon, mods[3], grpmin, grpmax, area, area_guess, act; char namekey[40]; } dm_sim_su;
enum { DM_BK_ACTBOSS, DM_BK_QUEST, DM_BK_UBER, DM_BK_ANCIENT, DM_BK_CLONE };
typedef struct { int16_t mon, act, kind, area; } dm_sim_boss;
typedef struct {
    int16_t diff, act, n, mlvl; int32_t hp, ac, ar, dmin, dmax, elavg; int16_t elshare, elpct, res[6], immune[6], ranged, undead, demon, block, cycle, regen, drain;
    int16_t lmin, lmax;
    int32_t bhp, bac, bar, bdmin, bdmax, bel;
} dm_sim_std;
typedef struct { char name[32]; int16_t id, sunderfull, sunderreduce; } dm_sim_state;

typedef struct dm_sim_data {
    int ok;
    char sha[65], set_name[8];
    dm_sim_diffc diff[3];
    dm_sim_class cls[8]; int ncls;
    dm_sim_potion potion[DM_POT_N]; unsigned potions_loaded;
    dm_sim_anim *anim; int nanim;
    char stat_name[512][40];
    int16_t stat[DM_ST_N];
    dm_sim_state *state; int nstate;
    dm_sim_skill *skill[DM_SIM_MAXSKILL];
    dm_sim_skill *skills; int nskill;
    dm_sim_miss *miss[DM_SIM_MAXMISS];
    dm_sim_miss *misses; int nmiss;
    int32_t monlvl[111][12];
    int16_t umod[8];
    int32_t uconst[64]; int nuconst;
    dm_sim_mon *mon; int nmon;
    dm_sim_area *area; int narea;
    dm_sim_spawn *spawn; int nspawn;
    dm_sim_su *su; int nsu;
    dm_sim_boss *boss; int nboss;
    dm_sim_std std[3][6]; int nstd;
    int32_t *code; int ncode, capcode;
    unsigned generation;
    char err[160];
} dm_sim_data;


enum { DM_OP_END, DM_OP_NUM, DM_OP_VAR, DM_OP_XVAR, DM_OP_STAT, DM_OP_BIN, DM_OP_NEG, DM_OP_NOT, DM_OP_TERN, DM_OP_LN, DM_OP_DM, DM_OP_BAD };
enum { DM_V_LVL, DM_V_BLVL, DM_V_ULVL, DM_V_EDMN, DM_V_EDMX, DM_V_EDLN, DM_V_EDNS, DM_V_EDXS, DM_V_ENMA, DM_V_EXMA, DM_V_ENMS, DM_V_EXMS,
       DM_V_EDMA, DM_V_TOHT, DM_V_MANA, DM_V_LEN, DM_V_RNG, DM_V_PETS, DM_V_MAEL, DM_V_MADM, DM_V_MATH, DM_V_MACR, DM_V_PNMA, DM_V_PXMA,
       DM_V_PNMS, DM_V_PXMS, DM_V_SKPT, DM_V_USMC, DM_V_MPS, DM_V_FNUM, DM_V_CLC1, DM_V_AST1 = DM_V_CLC1 + 10, DM_V_PST1 = DM_V_AST1 + 6,
       DM_V_N = DM_V_PST1 + 14 };
typedef struct dm_sim_calc_env {

    int64_t (*var)(const struct dm_sim_calc_env *e, int skill, int var, int *bad);
    int64_t (*stat)(const struct dm_sim_calc_env *e, int stat_id);
    int skill, depth;
    const void *user;
} dm_sim_calc_env;
int dm_sim_calc_compile(dm_sim_data *d, const char *postfix);
int64_t dm_sim_calc_eval(const dm_sim_data *d, int prog, const dm_sim_calc_env *e, int *bad);
int dm_sim_calc_var_id(const char *name);


int dm_sim_load(dm_sim_data *d, const char *text, size_t n, const char *set);
void dm_sim_free(dm_sim_data *d);
int dm_sim_skill_by_name(const dm_sim_data *d, const char *name);
int dm_sim_fam_parse(const char *s, uint32_t *out);


enum { DM_WK_NONE, DM_WK_MELEE, DM_WK_THROW, DM_WK_BOW, DM_WK_XBOW, DM_WK_CLAW };
typedef struct {
    int min, max;
    int elmin[DM_EL_N], elmax[DM_EL_N];
    int strbonus, dexbonus;
    int wsm, kind, twohand, ias;
    int edmin, edmax, postflat;
    int start_frame, start_known;
    char wclass[4];


    char stype[8];
    int srange;



    int ar_base_d, ar_pct_d, ed_d, ds_d, cs_d;


    int cb_d, ow_d, ll_d, ml_d, pmh_d, demon_d, undead_d;
} dm_sim_weapon;








enum { DM_GR_AURA, DM_GR_BUFF, DM_GR_CHARGE, DM_GR_ATTACK, DM_GR_HIT, DM_GR_STRUCK, DM_GR_N };
#define DM_SIM_MAXGRANT 24
typedef struct { int16_t skill, lvl, held, kind, chance, charges, checked, set2; int32_t game[6]; } dm_sim_grant;

typedef struct {
    int cls, clvl, diff, players, hardcore;
    int ordered, potions;
    int act;
    int belt[DM_POT_N];
    unsigned capture_bad;
    int str, dex, vit, ene;
    double life, mana;
    int ar, def, block;
    int res[DM_EL_N], maxres[DM_EL_N];
    int dr_flat, mdr_flat, absorb_pct[DM_EL_N], absorb_flat[DM_EL_N];
    int ias, fcr, fhr, fbr, sias;
    int ed;
    int ar_pct;
    int ar_base, ar_base_known;
    int ar_bonus[DM_SIM_MAXSKILL];
    int damage_mastery_delta[DM_SIM_MAXSKILL];
    int stats_known;
    int roll_inputs_bad;
    int endpoint_bad;
    uint8_t endpoint_state_bad[DM_SIM_MAXSKILL];
    int ds, cs, cb, ow, ll, ml;
    int mastery[DM_EL_N], pierce[DM_EL_N], sunder[DM_EL_N];
    int liferegen, manaregen_pct, prevent_heal;
    int dmg_demon, dmg_undead;
    int nhands; dm_sim_weapon hand[2];
    int shield, shield_min, shield_max, kick_min, kick_max;
    int form;
    int16_t lvl[DM_SIM_MAXSKILL];
    int16_t blvl[DM_SIM_MAXSKILL];
    int bar[DM_SIM_MAXBAR], nbar;
    int active_aura;
    int16_t active_lvl[DM_SIM_MAXSKILL];
    int active_buff[8], nactive_buff;
    int dodge, mastery_cs, mercenary, charges, poison_frames, missile_frame;
    int stat_value[512];
    int bone_pool, cyclone_pool, ward_pool, es_ratio;
    int es_pct;
    int ngrant; dm_sim_grant grant[DM_SIM_MAXGRANT];
    double struck;
} dm_sim_char;

enum { DM_TK_NORMAL, DM_TK_CHAMPION, DM_TK_UNIQUE, DM_TK_MINION, DM_TK_SUPERUNIQUE, DM_TK_ACTBOSS, DM_TK_QUEST, DM_TK_UBER, DM_TK_ANCIENT,
       DM_TK_CLONE, DM_TK_STD };
typedef struct {
    char name[40];
    int kind, mon, diff, mlvl, act, area;
    int extra_targets;
    int hp, ac, ar, dmin, dmax, cycle, crit, block, drain, regen, flags;
    int res[DM_EL_N];
    int elpct[3], elmin[3], elmax[3], eltype[3];
    int immune_share[DM_EL_N];
    int ranged_share, undead_share, demon_share, area_estimated;
} dm_sim_target;

typedef struct {
    int res[DM_EL_N];
    int immune_res[DM_EL_N], sunder_res[DM_EL_N], res_rules;
    int lifetap;
    int def_pct, dmg_pct, ar_pct;
    int skill_res_state[DM_EL_N];
} dm_sim_tmod;

enum { DM_SF_UNAVAIL = 1, DM_SF_ESTIMATE = 2, DM_SF_SEQ = 4, DM_SF_IMMUNE = 8, DM_SF_NOTARGET = 16, DM_SF_MODIFIER = 32, DM_SF_UTILITY = 64,
       DM_SF_NOWEAPON = 128, DM_SF_PET = 256, DM_SF_CORPSE = 512, DM_SF_PCTLIFE = 1024, DM_SF_NOMANA = 2048, DM_SF_TIMED = 4096,
       DM_SF_MINION = 8192,
       DM_SF_LENGTH = 16384 };
typedef struct {
    int known, low, high, resistance, element;
    double chance, critical;
} dm_sim_roll;
typedef struct {
    int skill, flags;
    dm_sim_roll roll;
    double hit[DM_EL_N];
    double raw[DM_EL_N];
    int dot_frames, travel, action_frame;
    int burst_attacks;
    double ow_rate, ow_chance;
    int burn_frames, burn_element;
    double burn;
    double survival_delta;
    double dot;
    double cb_frac;
    double sf_frac, sf_floor;
    double cth, hits;
    int frames, cooldown, mana_x256;
    int dur;
    double pet_element[DM_EL_N], dps_element[DM_EL_N];
    double pet_poison_uptime;
    double pet_dps;
    int pets;
    double dps;
    double ow_dps;



    double splash;
    double splash_pois;
    double mana_seconds;
    double ttk;
    int charges;


    double ll, ml;
    int pmh;
} dm_sim_skill_out;

typedef struct {
    double dps_element[DM_EL_N];
    int nseq, seq[DM_SIM_MAXBAR];
    double dps, ttk, mana_short;
    double ow_dps;
    int primary, cooldown_skill, curse, aura, flags;
    double phys_dps, mana_spent, mana_end, mana_rate, mana_seconds;
    double ll_dps, ml_dps;
} dm_sim_rot_out;

typedef struct {
    double cth, p_land, dmg_hit, max_hit, in_dps, sustain, hits_to_death, ttd;
    int underestimated;


    double spare;
} dm_sim_surv_out;

enum { DM_LIGHT_UNKNOWN=-1, DM_LIGHT_GREEN, DM_LIGHT_ORANGE, DM_LIGHT_RED };
typedef struct { int light, reason; double r, m; } dm_sim_light_out;
enum { DM_LR_NONE, DM_LR_IMMUNE, DM_LR_TTK, DM_LR_SAFETY, DM_LR_ONESHOT };

int dm_sim_frames_attack(int fpd, int speed, int ias, int sias, int wsm, int sequence);
int dm_sim_frames_rollback(int length, int action, int rate, int rollback, int hits);
int dm_sim_frames_cast(int fpd, int speed, int fcr, int sequence);


int dm_sim_frames_recover(int fpd, int speed, int v, int base);
int dm_sim_cth(int ar, int def, int alvl, int dlvl);
int dm_sim_eff_res(int base, int skill_red, int plain_red, int sunder, int state_full, int state_reduce);
int dm_sim_level_value(int base, const int32_t lev[5], int lvl);
int dm_sim_len_value(int base, const int32_t lev[3], int lvl);
int dm_sim_mana_cost(const dm_sim_skill *s, int lvl);
void dm_sim_char_init(dm_sim_char *c, int cls);
int dm_sim_target_mon(const dm_sim_data *d, int mon, int diff, int mlvl, int kind, int players, dm_sim_target *t);
int dm_sim_target_spawn(const dm_sim_data *d, int spawn, int index, int diff, int kind, dm_sim_target *t);

int dm_sim_target_std(const dm_sim_data *d, int diff, int act, dm_sim_target *t);

int dm_sim_std_level(const dm_sim_data *d, int diff, int act, int clvl);

int dm_sim_target_std_at(const dm_sim_data *d, int diff, int act, int clvl, dm_sim_target *t);


int dm_sim_target_act(const dm_sim_data *d, const dm_sim_char *c, dm_sim_target *t);
int dm_sim_area_act(const dm_sim_data *d, int area);
void dm_sim_tmod_init(dm_sim_tmod *m);
int dm_sim_skill_eval(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, int skill, dm_sim_skill_out *o);






int dm_splash_skill_ok(const dm_sim_skill *s);
int dm_splash_char_ok(const dm_sim_char *c, const dm_sim_skill *s);



double dm_splash_hand_reach(const dm_sim_char *c, const dm_sim_weapon *w, int extra);


void dm_sim_dual_poison(dm_sim_char *c, const int frames[2]);


int dm_sim_skills_prepared(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, int n, const int *ids, dm_sim_skill_out *out);
int dm_sim_rotation(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, dm_sim_rot_out *o);
int dm_sim_damage_taken(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, double out[DM_EL_N]);
int dm_sim_survive(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, const dm_sim_rot_out *rot, dm_sim_surv_out *o);






#define DM_SIM_TRACE_BUCKETS 256
#define DM_SIM_TRACE_POTIONS 16
enum { DM_TS_ENEMY, DM_TS_LIFE, DM_TS_MANA, DM_TS_N };
enum { DM_END_NONE, DM_END_KILL, DM_END_DEATH };
typedef struct {
    float lo[DM_TS_N], hi[DM_TS_N], last[DM_TS_N];
    uint64_t acts;
    uint8_t shortage, used;
} dm_sim_trace_bucket;
typedef struct {
    int width, n;
    int end, end_kind;
    int nskill, skill[DM_SIM_MAXBAR + 1];
    int best;
    int npotion;
    struct { int frame, kind; float life, mana; } potion[DM_SIM_TRACE_POTIONS];
    dm_sim_trace_bucket b[DM_SIM_TRACE_BUCKETS];

    int open, last_frame, last_short; float last_value[DM_TS_N];
} dm_sim_trace;
int dm_sim_fight(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, dm_sim_rot_out *rot, dm_sim_surv_out *surv, dm_sim_trace *trace);


typedef struct { double life; int frames, killed; } dm_sim_dealt_out;
int dm_sim_dealt(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, int frames, dm_sim_dealt_out *o);
int dm_sim_light(const dm_sim_target *t, double ttk, const dm_sim_surv_out *s, int all_immune, dm_sim_light_out *o);


int dm_sim_grant_dps(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, double *aura, double *struck);

int dm_sim_grants_modelled(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t);



typedef struct { int skill, level, stat, model, game; } dm_sim_grant_miss;
int dm_sim_grants_ok(const dm_sim_data *d, const dm_sim_char *c, dm_sim_grant_miss *miss);







enum { DM_SIM_NO_NODE = -32768 };
typedef struct { int tohit_pct, armor_pct, ar_mastery, damage_mastery; } dm_sim_passive_shift;
int dm_sim_passive_levels(const dm_sim_data *d, const dm_sim_char *now, dm_sim_char *after, const int *mastery_th, const int *mastery_dmg,
                          int mastery_crit, dm_sim_passive_shift *shift);



int dm_sim_swap_skills_ok(const dm_sim_data *d, const dm_sim_char *now, const dm_sim_char *after, const unsigned char *has_node,
                          const unsigned char *known, int passive_unknown);
double dm_sim_skill_value(const dm_sim_data *d, const dm_sim_char *c, int skill, int lvl, int which, int *bad);
int64_t dm_sim_skill_raw(const dm_sim_data *d, const dm_sim_char *c, int skill, int lvl, int which, int *bad);
int dm_sim_skill_endpoints(const dm_sim_data *d,const dm_sim_char *c,int skill,int out[2]);
enum { DM_SV_EMIN, DM_SV_EMAX, DM_SV_PMIN, DM_SV_PMAX, DM_SV_ELEN };
int dm_sim_eval_text(const char *profile, char *out, size_t cap);
int dm_sim_target_su(const dm_sim_data *d, int idx, int diff, dm_sim_target *t);
int dm_sim_profile_parse(const dm_sim_data *d, const char *text, dm_sim_char *c, char *err, size_t errcap);
int dm_sim_report(const dm_sim_data *d, const dm_sim_char *c, int bosses, char *out, size_t cap);

#ifdef DM_GEAR_INSTR



typedef struct { int frame; double removed, bound; } dm_gear_instr_cp;


void dm_sim_instr_set_leaf(uint64_t h1, uint64_t h2);

void dm_sim_instr_dealt_last(int *kill, int *frame, long *iters);
void dm_sim_instr_dealt_cps(const dm_gear_instr_cp **cps, int *n);

void dm_sim_instr_fights(long *runs, long *iters, long *frames);


void dm_sim_instr_inputs(long *fresh, long *dup_walk, long *dup_tail, long *tail_overflow);
void dm_sim_instr_tail_new(void);
void dm_sim_instr_p6_reset(void);


void dm_gear_instr_times(double *target, double *survival, double *group, double *dealt);
void dm_gear_instr_times_reset(void);
#endif
#endif
