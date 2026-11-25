








#ifndef DM_GEAR_EQUIP_TAB_H
#define DM_GEAR_EQUIP_TAB_H
#include "dm_gear_order.h"
#include "dm_gear_tab.h"
#include <stddef.h>
#include <stdint.h>


enum { DM_GEW_KEEP, DM_GEW_EMPTY, DM_GEW_UNIT, DM_GEW_STOCK };
typedef struct { int8_t kind; uint32_t id; uint64_t vh; } dm_gew_slot;
typedef struct {
    dm_gew_slot slot[DM_GO_SLOTS];
    int nseq; int8_t seq[DM_GO_MAXCHANGE];
    int nunit; uint16_t unit_slots[DM_GO_MAXCHANGE];



    uint32_t ring_was[2];
} dm_gew_target;


int dm_gew_ring_swap(const dm_gew_target *t, uint32_t right, uint32_t left);

int dm_gew_slot_of(int place);




int dm_gew_target_of(const dm_gear_snap *s, const dm_gtab_row *rows, int n, int row, dm_gew_target *t);


int dm_gew_units_done(const dm_gew_target *t, const dm_go_plan *p, int changes);



void dm_gear_equip_tab_init(void);

void dm_gear_equip_tab_pump(uint8_t *b, uint64_t pl);



int dm_gear_equip_tab_tap(const char *key, int row);
#endif
