









#pragma once
#include <stddef.h>
#include "game_addrs.h"
#include <stdint.h>


enum { IC_S_STR, IC_S_ENE, IC_S_DEX, IC_S_VIT, IC_S_HP, IC_S_MANA, IC_S_FIRE, IC_S_MAXFIRE, IC_S_LIGHT, IC_S_MAXLIGHT, IC_S_COLD, IC_S_MAXCOLD, IC_S_POIS, IC_S_MAXPOIS,
       IC_S_LL, IC_S_ML, IC_S_GF, IC_S_MF, IC_S_IAS, IC_S_FRW, IC_S_FHR, IC_S_FBR, IC_S_FCR, IC_S_ALLSK, IC_S_CLSK, IC_S_HPPCT, IC_S_MANAPCT, IC_S_N };
extern const int dm_ic_stat_id[IC_S_N];

enum { IC_M_FCR, IC_M_FHR, IC_M_FBR, IC_M_IAS, IC_M_RES_FIRE, IC_M_RES_COLD, IC_M_RES_LIGHT, IC_M_RES_POIS, IC_M_LIFE, IC_M_MANA, IC_M_ALLSK, IC_M_CLSK,
       IC_M_STR, IC_M_DEX, IC_M_VIT, IC_M_ENE, IC_M_MF, IC_M_GF, IC_M_FRW, IC_M_LL, IC_M_ML, IC_M_N };
int dm_ic_metric_of_stat(int stat_id);

#define IC_M_HIDDEN (1u << IC_M_ALLSK | 1u << IC_M_CLSK | 1u << IC_M_STR | 1u << IC_M_DEX | 1u << IC_M_VIT | 1u << IC_M_ENE)

typedef struct {
    int tot[IC_S_N];
    int old_[IC_S_N];
    int new_[IC_S_N];
    int lpv, mpm;
    int penalty;
    unsigned blocked;
    int hypothetical;
    int composed;
} dm_ic_in;
typedef struct { int metric, before, after, cap; char text[200]; } dm_ic_line;


int dm_ic_compute(const dm_ic_in *in, const char *const *labels, const char *cap_word, const char *to_fmt, dm_ic_line *out, int max);


int dm_ic_join(char *o, size_t n, const char *fmt, const char *before, const char *after);

int dm_ic_shown(const int *tot, int metric, int penalty);
void dm_ic_after(const dm_ic_in *in, int *after);

int dm_ic_res_shown(int total, int maxres, int penalty);
int dm_ic_res_cap(int maxres);


int dm_ic_req_value(int table_req, int percent, int ethereal);




enum { IC_REQ_SAME, IC_REQ_GAIN, IC_REQ_LOSS, IC_REQ_AMBIG };
int dm_ic_req_outcome(int req, int have_now, int have_after, int own);



int dm_ic_req_kept(int req, int total_now, int displaced);




typedef struct { int id, base, key; } dm_ic_sk;
void dm_ic_skill_order(dm_ic_sk *s, int n);








enum { IC_K_MUST, IC_K_FIXED, IC_K_WHY, IC_K_VAL, IC_K_END };
int dm_ic_pack(const int *kind, int n, int cap, int lost, char *keep, int *more_at);





int dm_ic_hints(const int *kind, int n, char *drop);











int dm_ic_slot(int want, int l1, int l2, const int worn[16]);
int dm_ic_other_hand(int slot);
int dm_ic_displaced(int code, int slot, int occ_slot, int occ_other);






enum { IC_CHARM_NONE, IC_CHARM_IN, IC_CHARM_OUT };
int dm_ic_charm_move(int is_charm, int own, int mode, int page);









typedef struct { int loc1, loc2, merc, slot, code, other_code, other_worn, pair; } dm_ic_panel_in;
int dm_ic_panel_hide(const dm_ic_panel_in *in);



#define DM_IC_PANEL_FRAMES 30
typedef struct { uint64_t key, frame; int hide, valid; } dm_ic_panel_memo;
int dm_ic_panel_memo_get(const dm_ic_panel_memo *m, uint64_t key, uint64_t frame);
void dm_ic_panel_memo_set(dm_ic_panel_memo *m, uint64_t key, uint64_t frame, int hide);



void dm_ic_hands_after(int slot, int mask, uint64_t cand, uint64_t right, uint64_t left, uint64_t out[2]);
int dm_ic_hands_change(int slot, int mask, uint64_t cand, uint64_t right, uint64_t left);






























enum { IC_C_DEF, IC_C_BLK, IC_C_AR, IC_C_DMG, IC_C_N };
typedef struct {
    int str, dex, lvl;
    int thf, bf, expansion;
    int ac, acpct, acovr;
    int toblock, can_block;
    int tohit, tohitpct;
    int ar_mast, ar_skill, ar_zero;
    int armed, mode2;
    int mn[2], mx[2];
    int bmn, bmx, srcdam;
    int dpct, dmast;
    int strb, dexb;
    int edmin, edmax, flat;
    int el[11];
    int mast[4];
    int plen_ovr, plen, pcnt;
    int xpct, xflat;
    int sadd_mn, sadd_mx;
    int ar_prog;
} dm_ic_cb;



int dm_ic_cb_eval(const dm_ic_cb *c, int *v);






unsigned dm_ic_cb_new_cap(const dm_ic_cb *now, const dm_ic_cb *aft);






#define DM_IC_CELL_CAP_NEW (1ull << 32)
#define DM_IC_CELL_SET_COMP (1ull << 33)

#define DM_IC_CELL_CHARM (1ull << 34)



int dm_ic_slot_group(int sl, int moved_is_weapon);
uint64_t dm_ic_cell_key(int cls, int metric, int sgroup, int charm, int hand_mode, int wtype, uint32_t sources, int cap_new, int set_comp);


int dm_ic_cb_line(int metric, int bmin, int bmax, int amin, int amax, const char *label, const char *cap_word, const char *mixed_word, const char *to_fmt, int hypo, char *out, size_t n);




int dm_ic_label(const char *s, int kind, char *out, size_t n);






int dm_ic_unproven_text(char *o, size_t n, const char *fmt, unsigned mask, int attack, const char *const *labels);


int dm_ic_insert(void *base, size_t size, int n, int cap, int at);




int dm_ic_head_stays(int removing, int above, int below, int marks);




unsigned dm_ic_cb_without(const dm_ic_cb *now, const int *game, int d_acpct, int d_tohit, int d_dpct, int *before, int *after);


void dm_ic_cb_minus(dm_ic_cb *c, const int *off);








typedef struct { const char *name; int before, after, key; } dm_ic_pkg_sk;
typedef struct {
    const int *before, *after; int penalty;
    const char *with;
    const dm_ic_pkg_sk *sk; int nsk;
    int cb; const int *cb_before, *cb_after;
    const char *splash;
} dm_ic_pkg_in;
typedef struct {
    const char *const *labels; const char *cap_word, *to_fmt, *mixed_word;
    const char *with_fmt, *skill_fmt, *skill_gone, *skill_more;
    const char *cb_label[IC_C_N];
} dm_ic_pkg_words;
enum { IC_PKG_NOTE, IC_PKG_VAL, IC_PKG_MORE };
typedef struct { int kind; char text[240]; } dm_ic_pkg_line;
int dm_ic_pkg_lines(const dm_ic_pkg_in *in, const dm_ic_pkg_words *w, dm_ic_pkg_line *out, int max);







#define IC_X_BLOCK IC_M_N
int dm_ic_effect(int metric, int before, int after, int *pct);


int dm_ic_fill(const char *fmt, int value, char *out, size_t n);




typedef struct { const char *fmt[IC_M_N + 1], *brief, *cap, *over; } dm_ic_fx_words;
typedef struct { char bracket[128], brief[128], rest[128]; } dm_ic_fx;
int dm_ic_fx_text(int metric, int before, int after, int cap, int over, int hypo, const dm_ic_fx_words *w, dm_ic_fx *out);


int dm_ic_res_group(const int v[4], const dm_ic_fx *const fx[4]);




typedef struct { const char *to, *keep, *keep_next; } dm_ic_frame_words;
int dm_ic_frames_text(int before, int after, int next, int hypo, const char *name, const dm_ic_frame_words *w, dm_ic_fx *out);


int dm_ic_line_says(int metric, int before, int after, int item_value);



int dm_ic_splash_hand(const int weapon[2], int slot, int active);







enum { IC_SPLASH_NONE, IC_SPLASH_OWN, IC_SPLASH_CHANGE };
int dm_ic_splash_line(int s0, int w0, int s1, int w1, int weapon, int change_ok, int hypo, int known, const char *label, const char *cone, const char *to_fmt, char *out, size_t n);


int dm_ic_block_on(int item_cmp, int combat_sim, int splash_on, int weapon);

uint64_t dm_ic_splash_key(uint64_t key, int on, int kind, int share, int width);




int dm_ic_ranged_unproven(const int ranged_after[2], int current_snapshot);



typedef struct { unsigned bad, sim; const char *why[IC_C_N], *sim_why[IC_C_N]; } dm_ic_veto;
void dm_ic_veto_add(dm_ic_veto *v, unsigned mask, const char *key, int display_only);



unsigned dm_ic_sim_swap_attack(const dm_ic_veto *v, unsigned live, const char **why);


int dm_ic_mastery_moves(int type_now, int type_after, int mastery);





int dm_ic_one_after_mastery(int cls, const int weapon_now[2], const int kept[2], const int type_now[2], const int mastery_now[2], int type_after, int mastery_after);



const char *dm_ic_unrated_key(int read_rc, int now_valid, unsigned now_bad, unsigned after_bad, const char *attack_why);
