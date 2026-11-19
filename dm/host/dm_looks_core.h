




#ifndef DM_LOOKS_CORE_H
#define DM_LOOKS_CORE_H
#include <stddef.h>
#include <stdint.h>

enum { DM_LOOK_ORIGINAL = 0, DM_LOOK_BASE = 1, DM_LOOK_UNIQUE = 2, DM_LOOK_SET = 3, DM_LOOK_INVISIBLE = 4 };
typedef struct { uint8_t kind; uint32_t id; } dm_look;


enum { DM_LG_NONE = 0, DM_LG_WEAPON, DM_LG_SHIELD, DM_LG_HELM, DM_LG_ARMOR, DM_LG_GLOVES, DM_LG_BOOTS, DM_LG_BELT, DM_LG_COUNT };

typedef struct {
    uint32_t code;
    uint8_t group, w, h;
    int8_t cls;
    uint32_t stance;
    uint32_t pic;
    char icon[48];
} dm_look_base;
typedef struct { dm_look_base *b; int n; } dm_look_table;
int dm_looks_table_parse(const char *text, dm_look_table *t);
void dm_looks_table_free(dm_look_table *t);
const dm_look_base *dm_looks_table_find(const dm_look_table *t, uint32_t code);


int dm_looks_fits(const dm_look_table *t, uint32_t target, uint32_t donor, int pclass);
int dm_looks_invisible_ok(const dm_look_table *t, uint32_t target);
int dm_looks_group(const dm_look_table *t, uint32_t code);

int dm_looks_price(int item_level, int kind);

typedef struct { uint32_t fp, code; uint8_t quality; int32_t row; } dm_look_item;
int dm_looks_item_eq(const dm_look_item *a, const dm_look_item *b);

uint64_t dm_looks_sig_add(uint64_t h, const dm_look_item *it, uint32_t unit, int mode, int identified);



enum { DM_LOOK_PLACES = 4 };
typedef struct { uint32_t code; uint8_t quality; int32_t row; dm_look look; } dm_look_place;
typedef struct { char name[24]; dm_look_place p[DM_LOOK_PLACES]; } dm_look_char;



enum { DM_DYE_COLOURS = 21 };
typedef struct { dm_look_item it; dm_look look; uint8_t dye; } dm_look_map;
typedef struct { dm_look_item it; dm_look look; uint8_t dye; uint64_t seq; } dm_look_pending;

typedef struct {
    dm_look *coll; int ncoll, capcoll;
    dm_look_map *map; int nmap, capmap;
    dm_look_pending *pend; int npend, cappend;
    dm_look_char *chars; int nchars, capchars;
    uint64_t rev;
} dm_looks_store;
void dm_looks_store_free(dm_looks_store *s);

int dm_looks_coll_add(dm_looks_store *s, dm_look l);
int dm_looks_coll_has(const dm_looks_store *s, dm_look l);

dm_look dm_looks_current(const dm_looks_store *s, const dm_look_item *it);
int dm_looks_current_dye(const dm_looks_store *s, const dm_look_item *it);


int dm_looks_choose(dm_looks_store *s, const dm_look_item *it, dm_look l, int dye, uint64_t seq);



int dm_looks_pay_choose(dm_looks_store *s, const dm_look_item *it, dm_look l, int dye, uint64_t seq, int (*pay)(void *ctx, int *dye), void *ctx);

int dm_looks_busy(const dm_looks_store *s, const dm_look_item *it);




int dm_looks_promotable(const dm_looks_store *s, uint64_t work_seq, dm_look_pending *out, int max);
int dm_looks_settle(dm_looks_store *s, const dm_look_pending *done, int n);
int dm_looks_char_set(dm_looks_store *s, const dm_look_char *c);
const dm_look_char *dm_looks_char_get(const dm_looks_store *s, const char *name);



int dm_looks_bits_item(const uint8_t *bits, int len, dm_look_item *it, int *identified);


dm_look dm_looks_unlocks(const dm_look_table *t, const dm_look_item *it, int identified, int own_look);


int dm_looks_offer(const dm_looks_store *s, const dm_look_table *t, uint32_t target, int pclass, uint32_t (*base_of)(dm_look), dm_look *out, int max);




typedef struct { uint32_t code; uint8_t colour, grade; char icon[48]; } dm_dye_gem;
typedef struct { dm_dye_gem *g; int n; } dm_dye_table;
int dm_dyes_parse(const char *text, dm_dye_table *t);
void dm_dyes_free(dm_dye_table *t);
const dm_dye_gem *dm_dyes_find(const dm_dye_table *t, uint32_t code);


enum { DM_DYE_FIGURE = 1, DM_DYE_PICTURE = 2 };
int dm_dye_where(int figure_group, int picture_group);


enum { DM_DYE_GAME = 0, DM_DYE_CHOSEN = 1, DM_DYE_LOOK_ROW = 2, DM_DYE_UNCOLOURED = 3 };
int dm_dye_source(int dye, int look_kind);


typedef struct { uint8_t dye; int gem; } dm_dye_offer;
int dm_dyes_offer(const dm_dye_table *t, const uint32_t *codes, int n, dm_dye_offer *out, int max);
int dm_dyes_offer_at(const dm_dye_offer *o, int n, int dye);



enum { DM_LOOKS_GRID = 10, DM_LOOKS_DYE_ROW = 8, DM_LOOKS_TILES = 50 };
int dm_looks_page_cap(int w, int h, int colour_rows);
int dm_looks_page_index(int cap, int page, int tile, int n);




typedef struct { uint64_t state; int shown; } dm_looks_apply_gate;
int dm_looks_apply_gate_step(dm_looks_apply_gate *g, uint64_t state, int shown, int pressed);




enum { DM_LOOK_U_CLASS = 4, DM_LOOK_U_DATA = 0x10, DM_LOOK_D_QUALITY = 0, DM_LOOK_D_FLAGS = 0x18, DM_LOOK_D_ROW = 0x34, DM_LOOK_FL_IDENT = 0x10 };
void dm_looks_dress(const uint8_t *u, const uint8_t *d, uint32_t dclass, uint8_t dquality, int32_t drow, uint8_t *fu, size_t nu, uint8_t *fd, size_t nd);



enum { DM_LOOKS_VERSION = 2 };
enum { DM_LOOKS_OK = 0, DM_LOOKS_CORRUPT = -1, DM_LOOKS_NEWER = -2, DM_LOOKS_NOMEM = -3 };
size_t dm_looks_encode(const dm_looks_store *s, uint8_t **out);
int dm_looks_decode(const uint8_t *d, size_t n, dm_looks_store *s);


int dm_looks_merge(dm_looks_store *dev, const dm_looks_store *in);
#endif
