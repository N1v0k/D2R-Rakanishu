






#ifndef DM_GEAR_PAGES_H
#define DM_GEAR_PAGES_H
#include "dm_gear_tab.h"
#include <stddef.h>
#include <stdint.h>

enum { DM_GPG_PERSONAL, DM_GPG_SHARED };
#define DM_GPG_PAGES 5
#define DM_GPG_TABS 8
#define DM_GPG_PLACES (DM_GPG_PAGES + DM_GPG_TABS)
#define DM_GPG_GRID_W 14
#define DM_GPG_GRID_H 10
#define DM_GPG_PLACE_ITEMS (DM_GPG_GRID_W * DM_GPG_GRID_H)


typedef struct {
    int kind;
    int hasw;
    dm_gear_cand c;
    dm_gear_weapon w;
    dm_gtab_show show;
} dm_gpg_prof;



typedef struct {
    int where, index;
    int x, y;
    uint64_t hash;
    uint32_t unit;
    dm_gpg_prof p;
} dm_gpg_item;

typedef struct {
    uint64_t rev;
    unsigned present;
    unsigned missing;
    unsigned unreadable;
    int items;
    int skipped;
    int place_items[DM_GPG_PLACES], place_skipped[DM_GPG_PLACES];
} dm_gpg_state;



void dm_gear_pages_tick(uint64_t G, uint64_t player, uint8_t *b);

void dm_gear_pages_state(dm_gpg_state *st);

int dm_gear_pages_get(dm_gpg_item *out, int max, dm_gpg_state *st);



int dm_gear_pages_info(dm_gpg_state *st, uint64_t *psig, uint32_t tab[DM_GPG_TABS]);


int dm_gear_pages_unit_hash(uint32_t unit, uint64_t *hash);

uint32_t dm_gear_pages_row_name(uint32_t unit, uint64_t *vhash);



unsigned dm_gear_pages_ask(void);
unsigned dm_gear_pages_answered(void);
unsigned dm_gear_pages_asked(void);

uint64_t dm_gear_frame_spent(void);
void dm_gear_frame_spend(uint64_t ns);
#define DM_GEAR_FRAME_NS 1000000ull



int dm_gear_pages_with(uint8_t *b, int place, int x, int y, uint64_t hash, void (*fn)(uint64_t it, void *arg), void *arg);




uint64_t dm_gear_pages_bits_vhash(const uint8_t *bits, int len);
int dm_gear_pages_unit_vhash(uint8_t *b, uint32_t id, uint64_t *hash);
int dm_gear_pages_bits_size(uint8_t *b, const uint8_t *bits, int len, int *w, int *h);
int dm_gear_pages_tab_ids(uint32_t id[DM_GPG_TABS]);
int dm_gear_pages_census(uint8_t *b, int *count, uint64_t *hash);



int dm_gear_pages_slot_sig(int page, uint64_t *sig);
int dm_gear_pages_slot_items(uint8_t *b, uint64_t G, int page, void (*fn)(uint64_t it, void *arg), void *arg);



typedef struct {
    int x, y;
    uint64_t key;
    uint32_t unit;
    int skip;
    const void *src;
    int len;
} dm_gpg_in;

typedef int (*dm_gpg_profile_fn)(void *ctx, const dm_gpg_in *in, uint64_t *hash, dm_gpg_prof *out);

typedef int (*dm_gpg_hash_fn)(void *ctx, const dm_gpg_in *in, uint64_t *hash);



enum { DM_GPG_W_LIST, DM_GPG_W_HASH, DM_GPG_W_PROFILE, DM_GPG_W_N };
typedef struct {
    int max;
    uint64_t limit_ns;
    uint64_t (*clock)(void *ctx);
    void *ctx;
    uint64_t spent;
    uint64_t est[DM_GPG_W_N];

    int made;
    unsigned begun;
    uint64_t t0;
} dm_gpg_budget;
typedef struct dm_gpg_core dm_gpg_core;
dm_gpg_core *dm_gpg_new(void);
void dm_gpg_free(dm_gpg_core *c);
void dm_gpg_reset(dm_gpg_core *c);
void dm_gpg_budget_start(dm_gpg_budget *b);
uint64_t dm_gpg_budget_used(const dm_gpg_budget *b);



int dm_gpg_place(dm_gpg_core *c, int place, uint64_t psig, const dm_gpg_in *in, int n, dm_gpg_profile_fn fn, dm_gpg_hash_fn hash, void *ctx,
                 dm_gpg_budget *bud);
void dm_gpg_absent(dm_gpg_core *c, int place);

void dm_gpg_missing(dm_gpg_core *c, int place);


#define DM_GPG_ABSENT (-1)
#define DM_GPG_UNLISTED (-2)
typedef int (*dm_gpg_list_fn)(void *ctx, int place, dm_gpg_in *in, int max, dm_gpg_profile_fn *fn, dm_gpg_hash_fn *hash);
typedef struct {
    int swept;
    unsigned unreadable;
    int listed;
} dm_gpg_pass_out;
void dm_gpg_pass(dm_gpg_core *c, uint64_t psig, dm_gpg_list_fn list, void *ctx, dm_gpg_in *in, int max, dm_gpg_budget *bud, dm_gpg_pass_out *out);

void dm_gpg_ask(dm_gpg_core *c, unsigned n);
unsigned dm_gpg_answered(const dm_gpg_core *c);
void dm_gpg_state_get(const dm_gpg_core *c, dm_gpg_state *st);
int dm_gpg_items(const dm_gpg_core *c, dm_gpg_item *out, int max);




#define DM_GPG_SEL_MAX 1024
typedef struct { int n, pages, shared, skipped, busy; int idx[DM_GPG_SEL_MAX], page[DM_GPG_SEL_MAX]; } dm_gpg_sel;
void dm_gpg_select(const dm_gpg_state *st, const dm_gpg_item *items, int n, int pcur, const int tabno[DM_GPG_TABS], unsigned tabs_read, dm_gpg_sel *o);


typedef struct { int where, page, x, y; uint64_t hash; } dm_gpg_id;


uint64_t dm_gpg_ids_hash(dm_gpg_id *ids, int n, uint64_t h);

int dm_gpg_unit_hash(dm_gpg_core *c, uint32_t unit, uint64_t *hash);

uint32_t dm_gpg_stock_id(int place, int x, int y);



uint32_t dm_gpg_row_name(dm_gpg_core *c, uint32_t unit, uint64_t *vhash);

uint64_t dm_gpg_content(const dm_gpg_in *in, int n);
uint64_t dm_gpg_fnv(const void *data, size_t n, uint64_t h);
#define DM_GPG_FNV0 1469598103934665603ull
#endif
