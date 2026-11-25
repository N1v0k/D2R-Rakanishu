





#ifndef DM_GEAR_ORDER_H
#define DM_GEAR_ORDER_H
#include <stdint.h>

#define DM_GO_SLOTS 13
#define DM_GO_MAXITEM 64
#define DM_GO_MAXGRID 12
#define DM_GO_GRIDW 16
#define DM_GO_MAXSTEP 64
#define DM_GO_MAXCHANGE 9

enum { DM_GO_HEAD = 1, DM_GO_NECK, DM_GO_TORSO, DM_GO_RIGHT, DM_GO_LEFT, DM_GO_RRING, DM_GO_LRING, DM_GO_BELT, DM_GO_FEET, DM_GO_GLOVES };

enum { DM_GO_H_NONE, DM_GO_H_ONE, DM_GO_H_BOTH, DM_GO_H_OFF };

typedef struct { int8_t grid, x, y; } dm_go_cell;
typedef struct {
    uint8_t w, h;
    uint8_t hand;
    int8_t slot;
    dm_go_cell cell;
} dm_go_item;
typedef struct {
    uint8_t w, h;
    uint8_t place;
    uint16_t used[DM_GO_GRIDW];
} dm_go_grid;
typedef struct {
    int ngrid; dm_go_grid grid[DM_GO_MAXGRID];
    int nitem; dm_go_item item[DM_GO_MAXITEM];
    int inv;
} dm_go_world;


typedef struct {

    int (*can_wear)(void *ctx, const int8_t *worn, int slot, int item);

    int (*stable)(void *ctx, const int8_t *worn);
    void *ctx;
} dm_go_rules;

enum { DM_GO_PICK_CELL = 1, DM_GO_PICK_SLOT, DM_GO_PUT_SLOT, DM_GO_PUT_CELL };
typedef struct {
    uint8_t op;
    int8_t item;
    int8_t slot;
    int8_t swap;
    dm_go_cell cell;
} dm_go_move;
typedef struct { dm_go_move go, back; int8_t change; } dm_go_step;
typedef struct {
    int nstep; dm_go_step step[DM_GO_MAXSTEP];
    int nchange; struct { int8_t first, n; uint16_t slots; } change[DM_GO_MAXCHANGE];
} dm_go_plan;

enum {
    DM_GO_OK = 0,
    DM_GO_E_INPUT = -1,
    DM_GO_E_ROOM = -2,
    DM_GO_E_RULE = -3,
    DM_GO_E_MOVE = -4
};


int dm_gear_order_all(const dm_go_world *w, const int8_t *target, const dm_go_rules *r, dm_go_plan *p);


int dm_gear_order_all_seq(const dm_go_world *w, const int8_t *target, const dm_go_rules *r, const int8_t *slots, int nslots, dm_go_plan *p);

int dm_gear_order_one(const dm_go_world *w, const int8_t *target, int slot, const dm_go_rules *r, dm_go_plan *p);

int dm_gear_order_apply(dm_go_world *w, const dm_go_move *m, int *hand);
void dm_gear_order_worn(const dm_go_world *w, int8_t *worn);
#endif
