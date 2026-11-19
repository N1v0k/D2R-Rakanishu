


#ifndef DM_GEAR_COMPOSE_H
#define DM_GEAR_COMPOSE_H
#include <stdint.h>

#define DM_GEAR_NSTAT 512
#define DM_GEAR_MAXV 320
#define DM_GEAR_GREENV 16
#define DM_GEAR_MAXITEMS 16
#define DM_GEAR_MAXOP 160
#define DM_GEAR_MAXSET 64
#define DM_GEAR_MAXSETITEM 256
#define DM_GEAR_MAXBONUS 1024

typedef struct { uint16_t stat, layer; int32_t value; } dm_gear_v;
typedef struct { int n, full; dm_gear_v v[DM_GEAR_MAXV]; } dm_gear_list;
void dm_gear_list_add(dm_gear_list *l, int stat, int layer, int value);
int dm_gear_list_get(const dm_gear_list *l, int stat, int layer);
int dm_gear_list_sum(const dm_gear_list *l, int stat);


typedef struct { int16_t owner, level, stat, layer, unmapped; int32_t value; } dm_gear_bonus;

typedef struct {
    int ok;
    int nop;
    struct { int16_t stat, op, param, level, ntg, tg[3]; } op[DM_GEAR_MAXOP];
    uint8_t shift[DM_GEAR_NSTAT];
    int nset, nitem, nsetb, nitemb;
    int16_t pieces[DM_GEAR_MAXSET];
    struct { int16_t set, func; } item[DM_GEAR_MAXSETITEM];
    dm_gear_bonus setb[DM_GEAR_MAXBONUS];
    dm_gear_bonus itemb[DM_GEAR_MAXBONUS];
} dm_gear_tables;

int dm_gear_tables_parse(dm_gear_tables *t, const char *text);

typedef struct { int n; dm_gear_v v[DM_GEAR_GREENV]; } dm_gear_green;
typedef struct {
    uint32_t id;
    int slot;
    int req_str, req_dex, req_lvl;
    int setitem;
    dm_gear_list own;

    int green_read;
    dm_gear_green green[5];

    int ngrant;
    struct { int16_t skill, level, kind, chance, charges; } grant[8];
    unsigned hands;
} dm_gear_item;

typedef struct {
    int cls, clvl;
    int lpv, mpm;


    dm_gear_list base;
} dm_gear_base;

enum {
    DM_GEAR_C_ORDER = 1,
    DM_GEAR_C_UNMAPPED = 2,
    DM_GEAR_C_FUNC1 = 4,
    DM_GEAR_C_PCT = 8,
    DM_GEAR_C_ITEMOP = 16,
    DM_GEAR_C_FULL = 32
};
typedef struct {
    unsigned flags;
    unsigned char active[DM_GEAR_MAXITEMS];
    int nworn;
    struct { int16_t set, pieces; } worn[DM_GEAR_MAXITEMS];
    int nx;
    int16_t xstat[16];
    dm_gear_list gear;
    dm_gear_list green;
    dm_gear_list raw;


    int value[DM_GEAR_NSTAT];
} dm_gear_char;


int dm_gear_compose(const dm_gear_tables *t, const dm_gear_base *b, const dm_gear_item *items, int n, const unsigned char *given, dm_gear_char *out);


void dm_gear_item_conversions(const dm_gear_tables *t, int clvl, const dm_gear_list *green, dm_gear_list *out);
#endif
