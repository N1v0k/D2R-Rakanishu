



#ifndef DM_GEAR_TAB_H
#define DM_GEAR_TAB_H
#include "dm_gear_search.h"
#include "dm_splash_rule.h"
#include <stdint.h>


enum { DM_GTAB_WORN, DM_GTAB_INV, DM_GTAB_STASH, DM_GTAB_SHARED };

enum { DM_GTAB_OK, DM_GTAB_NOCHAR, DM_GTAB_SET2, DM_GTAB_NOSIM, DM_GTAB_NOREF };
#define DM_GTAB_NAME 128
typedef struct {
    char name[DM_GTAB_NAME];

    char colour;
    char code[5];
    int quality;
    int where, page;
    int row;
    int gfx;
    uint32_t id;

    uint64_t vhash;
} dm_gtab_show;

#define DM_GTAB_STOCK_ID 0x40000000u
typedef struct dm_gear_snap dm_gear_snap;
struct dm_gear_snap {
    uint64_t rev;
    int state;
    char set[8];
    const dm_sim_data *tables;
    dm_gear_problem p;
    dm_gear_goal goal;
    dm_gear_cand *cand;
    dm_gear_weapon *weapon;
    dm_gtab_show *show;
    char slot[DM_GEAR_P_N][48];
    char measure[DM_GEAR_M_N][64], missed[96], reached[96], below[512], below_same[512];
    int comma;
    int stash_pages;
    int shared_tabs;
    int skipped;
    unsigned jobs;

    int refs;
    uint64_t fp;
    dm_gear_pick now;



    dm_gear_pick idle;
    int idle_done;
    dm_gear_snap *next;
};
dm_gear_snap *dm_gear_snap_new(int ncand, int nweapon);
void dm_gear_snap_free(dm_gear_snap *s);



void dm_gear_tab_charms(int search);
int dm_gear_tab_charms_wanted(void);


void dm_gear_tab_want(int on);
int dm_gear_tab_wanted(void);


int dm_gear_tab_offer(dm_gear_snap *s);


void dm_gear_tab_jobs(unsigned ends);

int dm_gear_tab_text(char *out, size_t cap);


dm_gear_snap *dm_gear_tab_hold(void);
void dm_gear_tab_unhold(dm_gear_snap *s);



int dm_gear_tab_ring_rows(const int best[2], const int now[2], int place[2], int cand[2]);

void dm_gear_tab_log(const char *line);






void dm_gear_tab_tip_ask(uint32_t id, int place);
uint32_t dm_gear_tab_tip_wanted(int *place);
int dm_gear_tab_tip_put(uint32_t id, int place, const char *text, const char *block, int dots);

int dm_gear_tab_tip_hash(uint32_t id, uint64_t *vhash);


typedef struct { int x, y, w, h, charm, placed; } dm_gear_inv_cells;


void dm_gear_grid(uint8_t grid[DM_GEAR_GRID_H][DM_GEAR_GRID_W], const dm_gear_inv_cells *it, int n);






#define DM_GTAB_ROWS (DM_GEAR_P_N + 2 * DM_GEAR_GRID_W * DM_GEAR_GRID_H)

typedef struct { uint64_t w[(DM_GTAB_ROWS + 63) / 64]; } dm_gtab_units;
static inline int dm_gtab_has(const dm_gtab_units *s, int u) { return (int)(s->w[u >> 6] >> (u & 63) & 1); }
typedef struct { int place, cand, charm, unit, first; dm_gtab_units needs; } dm_gtab_row;


int dm_gear_tab_rows(const dm_gear_problem *p, const dm_gear_pick *now, const dm_gear_pick *best, dm_gtab_row *rows, int max);


#define DM_GTAB_HANDS DM_SIM_BAR_HANDS
int dm_gear_tab_hands(const dm_gear_problem *p, const dm_gear_pick *pk, int out[][2], int max);




int dm_gear_tab_order(const dm_gear_problem *p, const dm_gear_pick *now, dm_gtab_row *rows, int n);


int dm_gear_tab_pkg_picks(const dm_gear_problem *p, const dm_gear_pick *now, const dm_gtab_row *rows, int n, int i, dm_gear_pick *before, dm_gear_pick *after);






typedef struct dm_gtab_pkg {
    uint32_t id; int place; unsigned gen;
    int ok, rated;
    dm_gear_tip tip;
    int before[IC_S_N], after[IC_S_N];
    int cb, cb_before[IC_C_N + 1], cb_after[IC_C_N + 1];
    int16_t lvl_before[DM_SIM_MAXSKILL], lvl_after[DM_SIM_MAXSKILL];
    char with[2 * DM_GTAB_NAME + 8];
    dm_splash_area splash_before, splash_after;
} dm_gtab_pkg;

int dm_gear_tab_tip_pkg(uint32_t id, int place, dm_gtab_pkg *out);



typedef int (*dm_gear_tab_extra_fn)(const dm_gear_snap *s, uint64_t key, const dm_gtab_row *rows, int n, const dm_gear_measures *now,
                                    const dm_gear_measures *after, char *out, size_t cap);
void dm_gear_tab_extra(dm_gear_tab_extra_fn fn);

long dm_gear_tab_runs(void);
long dm_gear_tab_starts(void);
int dm_gear_tab_best(dm_gear_result *out);
#endif
