












#ifndef DM_GEAR_EQUIP_CORE_H
#define DM_GEAR_EQUIP_CORE_H
#include "dm_gear_order.h"
#include <stdint.h>

enum { DM_GE_NOWHERE, DM_GE_BODY, DM_GE_INV, DM_GE_PAGE, DM_GE_TAB, DM_GE_HAND };


typedef struct { int8_t where, index, x, y; } dm_ge_loc;
typedef struct { int8_t where, index; } dm_ge_place;



#define DM_GE_INV_W 10
#define DM_GE_INV_H 4
#define DM_GE_STASH_W 14
#define DM_GE_STASH_H 10
int dm_ge_grid_size(int where, int *w, int *h);


typedef struct {
    uint64_t now_ms;
    int run_ok;
    int panel_open;
    int hand;
    int shown;
    unsigned flips;
    int settled;
    uint64_t commits;
    int code;
    int cell_free;
    dm_ge_loc at[DM_GO_MAXITEM];
} dm_ge_seen;

enum { DM_GE_A_WAIT, DM_GE_A_SEND, DM_GE_A_FLIP, DM_GE_A_END };
typedef struct { int kind; dm_go_move move; int dir; } dm_ge_act;
enum { DM_GE_RUNNING, DM_GE_DONE, DM_GE_RETURNED, DM_GE_PARTIAL, DM_GE_IN_HAND, DM_GE_LOST };
enum { DM_GE_WHY_NONE, DM_GE_WHY_CHANGED, DM_GE_WHY_CODE, DM_GE_WHY_LATE, DM_GE_WHY_PANEL, DM_GE_WHY_PAGE, DM_GE_WHY_SAVE, DM_GE_WHY_HAND,
       DM_GE_WHY_RUN, DM_GE_WHY_FAULT, DM_GE_WHY_TIME, DM_GE_WHY_BACK, DM_GE_WHY_N };



typedef struct { int step, flip, point; } dm_ge_fault;


#define DM_GE_CONFIRM_MS 2000
#define DM_GE_GRACE_MS 1000
#define DM_GE_FLIP_MS 15000
#define DM_GE_SETTLE_MS 10000
#define DM_GE_QUIET_MS 1500
#define DM_GE_GO_MS 90000
#define DM_GE_ALL_MS 120000
#define DM_GE_TRIES 2

typedef struct {
    dm_go_world w0, wc, w;
    dm_go_plan p;
    dm_ge_place gp[DM_GO_MAXGRID];
    int hand;
    int home;
    int mode, sub, j, lo, ch;
    int flip_to; unsigned flips0; uint64_t commit0;
    uint64_t t0, t_sub, ignore_until, settle_until, quiet_until;
    int tries, nflip, back_mute, end, why, done_changes;
    dm_ge_fault fault;
    void (*log)(void *ctx, const char *line);
    void *log_ctx;
} dm_ge_core;



int dm_ge_check(const dm_go_world *w, const dm_go_plan *p, const dm_ge_place *gp);

int dm_ge_begin(dm_ge_core *c, const dm_go_world *w, const dm_go_plan *p, const dm_ge_place *gp, int home, uint64_t now_ms, const dm_ge_fault *f);

void dm_ge_step(dm_ge_core *c, const dm_ge_seen *s, dm_ge_act *a);

const dm_go_move *dm_ge_pending(const dm_ge_core *c);



int dm_ge_view(const dm_ge_core *c, dm_ge_place *out);

dm_ge_loc dm_ge_expect(const dm_ge_core *c, int i, int after);

int dm_ge_end(const dm_ge_core *c, int *why, int *changes);
const char *dm_ge_end_name(int end);
const char *dm_ge_why_name(int why);
#endif
