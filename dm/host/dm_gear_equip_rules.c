
#include "dm_gear_equip_rules.h"
#include <string.h>

const int8_t dm_ge_place_of_slot[DM_GO_SLOTS] = { -1, DM_GEAR_P_HEAD, DM_GEAR_P_AMULET, DM_GEAR_P_ARMOR, DM_GEAR_P_MAIN, DM_GEAR_P_OFF, DM_GEAR_P_RING1,
                                                  DM_GEAR_P_RING2, DM_GEAR_P_BELT, DM_GEAR_P_BOOTS, DM_GEAR_P_GLOVES, -1, -1 };

int dm_ge_hand_of(const dm_gear_cand *c) {
    switch (c->kind) {
    case DM_GEAR_K_WEAPON1: case DM_GEAR_K_BOW: case DM_GEAR_K_XBOW: return DM_GO_H_ONE;
    case DM_GEAR_K_WEAPON2: return DM_GO_H_BOTH;
    case DM_GEAR_K_SHIELD: case DM_GEAR_K_ARROWS: case DM_GEAR_K_BOLTS: return DM_GO_H_OFF;
    default: return DM_GO_H_NONE;
    }
}

static int pick_of(const dm_ge_rules *r, const int8_t *worn, dm_gear_pick *pk) {
    memcpy(pk->charm, r->now.charm, sizeof pk->charm);
    for (int k = 0; k < DM_GEAR_P_N; k++) pk->at[k] = -1;
    for (int s = 1; s <= DM_GO_GLOVES; s++) {
        if (worn[s] < 0) continue;
        int c = worn[s] < r->nitem ? r->cand[worn[s]] : -1;
        if (c < 0) return -1;
        pk->at[dm_ge_place_of_slot[s]] = (int16_t)c;
    }
    return 0;
}
static int can_wear(void *ctx, const int8_t *worn, int slot, int item) {
    const dm_ge_rules *r = ctx; const dm_gear_problem *p = r->p;
    if (item < 0 || item >= r->nitem || slot < 1 || slot > DM_GO_GLOVES || r->cand[item] < 0) return 0;
    const dm_gear_cand *c = &p->cand[r->cand[item]];
    int place = dm_ge_place_of_slot[slot], hand = dm_ge_hand_of(c);
    if ((place == DM_GEAR_P_MAIN || place == DM_GEAR_P_OFF) != (hand != DM_GO_H_NONE)) return 0;
    if (place == DM_GEAR_P_MAIN && hand == DM_GO_H_OFF) return 0;
    if (place == DM_GEAR_P_OFF && (hand == DM_GO_H_BOTH || (hand == DM_GO_H_ONE && c->kind != DM_GEAR_K_WEAPON1))) return 0;
    if (place == DM_GEAR_P_OFF && hand == DM_GO_H_ONE && p->now.cls != 4 && p->now.cls != 6) return 0;
    if (place != DM_GEAR_P_MAIN && place != DM_GEAR_P_OFF) {
        static const int8_t want[DM_GEAR_P_N] = { -1, -1, DM_GEAR_K_ARMOR, DM_GEAR_K_AMULET, DM_GEAR_K_RING, DM_GEAR_K_RING, DM_GEAR_K_HEAD, DM_GEAR_K_BELT,
                                                 DM_GEAR_K_BOOTS, DM_GEAR_K_GLOVES };
        if (c->kind != want[place]) return 0;
    }
    if (c->cls_only && c->cls_only - 1 != p->now.cls) return 0;
    static _Thread_local dm_gear_pick pk; static _Thread_local dm_gear_char comp; uint8_t active[DM_GEAR_P_N + 1];
    if (pick_of(r, worn, &pk) || dm_gear_pick_compose(p, &pk, &comp, active)) return 0;
    return comp.value[0] >= c->item.req_str && comp.value[2] >= c->item.req_dex && p->base.clvl >= c->item.req_lvl;
}

static int stable(void *ctx, const int8_t *worn) {
    const dm_ge_rules *r = ctx; const dm_gear_problem *p = r->p;
    static _Thread_local dm_gear_pick pk; static _Thread_local dm_gear_char comp; uint8_t active[DM_GEAR_P_N + 1];
    if (pick_of(r, worn, &pk) || dm_gear_pick_compose(p, &pk, &comp, active)) return 0;
    for (int k = 0; k < DM_GEAR_P_N; k++) if (pk.at[k] >= 0 && !active[k] && p->off_now[k] != pk.at[k]) return 0;
    return 1;
}
int dm_ge_rules_init(dm_ge_rules *r, const dm_gear_problem *p, const int16_t *cand, int nitem, dm_go_rules *out) {
    if (!r || !p || !p->ready || !cand || nitem < 0 || nitem > DM_GO_MAXITEM || !out) return -1;
    memset(r, 0, sizeof *r); r->p = p; r->nitem = nitem;
    for (int i = 0; i < nitem; i++) { if (cand[i] >= p->ncand) return -1; r->cand[i] = cand[i]; }
    dm_gear_pick_now(p, &r->now);
    out->can_wear = can_wear; out->stable = stable; out->ctx = r;
    return 0;
}
