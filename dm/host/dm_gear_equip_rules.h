



#ifndef DM_GEAR_EQUIP_RULES_H
#define DM_GEAR_EQUIP_RULES_H
#include "dm_gear_order.h"
#include "dm_gear_search.h"

typedef struct {
    const dm_gear_problem *p;
    int16_t cand[DM_GO_MAXITEM];
    int nitem;
    dm_gear_pick now;
} dm_ge_rules;
extern const int8_t dm_ge_place_of_slot[DM_GO_SLOTS];

int dm_ge_hand_of(const dm_gear_cand *c);

int dm_ge_rules_init(dm_ge_rules *r, const dm_gear_problem *p, const int16_t *cand, int nitem, dm_go_rules *out);
#endif
