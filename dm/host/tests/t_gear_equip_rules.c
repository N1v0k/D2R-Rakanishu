




#include "t_gear_stock.h"
#include "../dm_gear_equip_rules.h"

static int checks, fails;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static gstock G; static dm_go_world W; static int16_t CAND[DM_GO_MAXITEM]; static dm_ge_rules RR; static dm_go_rules R;

static void world_of(const int *extra, int nextra) {
    memset(&W, 0, sizeof W); W.ngrid = 1; W.inv = 0; W.grid[0] = (dm_go_grid){ 10, 4, 1, { 0 } };
    dm_gear_pick now; dm_gear_pick_now(&G.p, &now);
    for (int s = 1; s <= DM_GO_GLOVES; s++) { int c = now.at[dm_ge_place_of_slot[s]]; if (c < 0) continue;
        int i = W.nitem++; CAND[i] = (int16_t)c; W.item[i] = (dm_go_item){ 1, 1, (uint8_t)dm_ge_hand_of(&G.p.cand[c]), (int8_t)s, { -1, -1, -1 } }; }
    for (int k = 0; k < nextra; k++) { int i = W.nitem++; CAND[i] = (int16_t)extra[k];
        W.item[i] = (dm_go_item){ 1, 1, (uint8_t)dm_ge_hand_of(&G.p.cand[extra[k]]), 0, { 0, (int8_t)k, 0 } }; W.grid[0].used[0] |= (uint16_t)(1u << k); }
    CHECK(dm_ge_rules_init(&RR, &G.p, CAND, W.nitem, &R) == 0);
}
static int item_of(int cand) { for (int i = 0; i < W.nitem; i++) if (CAND[i] == cand) return i; return -1; }
static int step_of(const dm_go_plan *p, int item, int slot) { for (int k = 0; k < p->nstep; k++) if (p->step[k].go.op == DM_GO_PUT_SLOT && p->step[k].go.item == item && p->step[k].go.slot == slot) return k; return -1; }

int main(void) {
    static dm_gear_tables gt; const dm_sim_data *d = gs_tables(&gt);
    char *prof = gs_profile("class4-level80.prof");
    gspec s; memset(&s, 0, sizeof s); s.profile = prof; s.act = 1; s.seed = 14; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (gs_build(&G, d, &gt, &s)) { printf("FAIL: no stock\n"); return 1; }
    int str = dm_gear_list_get(&G.p.base.base, 0, 0);
    dm_gear_cand *a = gs_item(&G, DM_GEAR_K_ARMOR, 1, 0); a->item.req_str = str + 12; dm_gear_list_add(&a->item.own, 0, 0, 20);
    dm_gear_cand *r = gs_item(&G, DM_GEAR_K_RING, 1, 0); r->item.own.n = 0; dm_gear_list_add(&r->item.own, 0, 0, 12);
    dm_gear_cand *w = gs_item(&G, DM_GEAR_K_WEAPON1, 1.4, 0); w->item.req_str = str + 25;
    dm_gear_cand *sh = gs_item(&G, DM_GEAR_K_SHIELD, 1, 0); sh->item.req_str = 0;
    dm_gear_cand *w2 = gs_item(&G, DM_GEAR_K_WEAPON2, 1, 0); w2->item.req_str = 0;
    int ca = (int)(a - G.p.cand), cr = (int)(r - G.p.cand), cw = (int)(w - G.p.cand), cs = (int)(sh - G.p.cand), c2 = (int)(w2 - G.p.cand);
    CHECK(dm_gear_problem_ready(&G.p) == 0);
    int extra[] = { ca, cr, cw, cs, c2 };
    world_of(extra, 5);
    int ia = item_of(ca), ir = item_of(cr), iw = item_of(cw), is = item_of(cs), i2 = item_of(c2);
    int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(&W, worn);

    CHECK(!R.can_wear(R.ctx, worn, DM_GO_TORSO, ia));
    int8_t with_ring[DM_GO_SLOTS]; memcpy(with_ring, worn, sizeof with_ring); with_ring[DM_GO_RRING] = (int8_t)ir;
    CHECK(R.can_wear(R.ctx, with_ring, DM_GO_TORSO, ia));
    CHECK(!R.can_wear(R.ctx, with_ring, DM_GO_RIGHT, iw));
    int8_t both[DM_GO_SLOTS]; memcpy(both, with_ring, sizeof both); both[DM_GO_TORSO] = (int8_t)ia;
    CHECK(R.can_wear(R.ctx, both, DM_GO_RIGHT, iw));

    int8_t bad[DM_GO_SLOTS]; memcpy(bad, worn, sizeof bad); bad[DM_GO_TORSO] = (int8_t)ia; CHECK(!R.stable(R.ctx, bad)); CHECK(R.stable(R.ctx, worn));

    CHECK(!R.can_wear(R.ctx, worn, DM_GO_RIGHT, is)); CHECK(R.can_wear(R.ctx, worn, DM_GO_LEFT, is));
    CHECK(!R.can_wear(R.ctx, worn, DM_GO_LEFT, i2)); CHECK(R.can_wear(R.ctx, worn, DM_GO_RIGHT, i2));
    CHECK(!R.can_wear(R.ctx, worn, DM_GO_NECK, ir)); CHECK(!R.can_wear(R.ctx, worn, DM_GO_HEAD, ia));
    CHECK(R.can_wear(R.ctx, both, DM_GO_LEFT, iw) == (G.p.now.cls == 4 || G.p.now.cls == 6));
    { int16_t none[DM_GO_MAXITEM]; memcpy(none, CAND, sizeof none); none[ir] = -1; dm_ge_rules r2; dm_go_rules o2; CHECK(dm_ge_rules_init(&r2, &G.p, none, W.nitem, &o2) == 0); CHECK(!o2.can_wear(o2.ctx, worn, DM_GO_RRING, ir)); }

    int8_t t[DM_GO_SLOTS]; dm_gear_order_worn(&W, t); t[DM_GO_RRING] = (int8_t)ir; t[DM_GO_TORSO] = (int8_t)ia; t[DM_GO_RIGHT] = (int8_t)iw;
    W.grid[0].used[0] = 0x1f; static dm_go_plan p;
    int e = dm_gear_order_all(&W, t, &R, &p); CHECK(e == DM_GO_OK);
    if (!e) { int kr = step_of(&p, ir, DM_GO_RRING), ka = step_of(&p, ia, DM_GO_TORSO), kw = step_of(&p, iw, DM_GO_RIGHT); CHECK(kr >= 0 && ka > kr && kw > ka); }

    dm_gear_order_worn(&W, t); t[DM_GO_TORSO] = (int8_t)ia; CHECK(dm_gear_order_all(&W, t, &R, &p) == DM_GO_E_RULE);
    free(prof);
    printf("t_gear_equip_rules: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
