








#ifndef DM_GEAR_EQUIP_H
#define DM_GEAR_EQUIP_H
#include "dm_gear_equip_core.h"
#include "dm_gear_tab.h"
#include "dm_gear_equip_tab.h"
#include <stdint.h>


typedef struct {
    dm_go_world w;
    dm_ge_place gp[DM_GO_MAXGRID];
    uint64_t vh[DM_GO_MAXITEM];
    uint32_t code[DM_GO_MAXITEM];
    uint32_t unit[DM_GO_MAXITEM];
    int8_t target[DM_GO_SLOTS];
    int shown;
    uint64_t hash;
} dm_gear_equip_cap;



int dm_gear_equip_plan(const dm_gear_snap *s, dm_gear_equip_cap *cap, dm_go_plan *plan);

int dm_gear_equip_plan_seq(const dm_gear_snap *s, dm_gear_equip_cap *cap, const int8_t *slots, int nslots, dm_go_plan *plan);


int dm_gear_equip_start(uint8_t *b, uint64_t pl, const dm_gear_equip_cap *cap, const dm_go_plan *plan);


int dm_gear_equip_capture(uint8_t *b, uint64_t pl, const dm_ge_loc *src, dm_gear_equip_cap *cap);


int dm_gear_equip_capture_want(uint8_t *b, uint64_t pl, const dm_gew_target *t, dm_gear_equip_cap *cap);
void dm_gear_equip_builds(int n);
const char *dm_gear_equip_fail(void);
void dm_gear_equip_fresh(void);
int dm_gear_equip_ready(uint8_t *b, uint64_t pl);
int dm_gear_equip_open(uint8_t *b);

int dm_gear_equip_state(int *why, int *done, int *changes);

void dm_gear_equip_pump(uint8_t *b, uint64_t pl);


void dm_gear_equip_worthy(int worthy);
#endif
