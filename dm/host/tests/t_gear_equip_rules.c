






#include "t_gear_stock.h"
#include "../dm_gear_equip_rules.h"

static int checks, fails;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static gstock G; static dm_go_world W; static int16_t CAND[DM_GO_MAXITEM]; static dm_ge_rules RR; static dm_go_rules R;

static void world_of(const int *extra, int nextra) {
    memset(&W, 0, sizeof W); W.ngrid = 1; W.inv = 0; W.grid[0] = (dm_go_grid){ 10, 4, 1, { 0 } };
    dm_gear_pick now; dm_gear_pick_now(&G.p, &now);
    for (int s = 1; s <= DM_GO_GLOVES; s++) { int c = now.at[dm_ge_place_of_slot[s]]; if (c < 0) continue;
        int i = W.nitem++; CAND[i] = (int16_t)c; W.item[i] = (dm_go_item){ 1, 1, (uint8_t)dm_ge_hand_of(&G.p, c), (int8_t)s, { -1, -1, -1 } }; }
    for (int k = 0; k < nextra; k++) { int i = W.nitem++; CAND[i] = (int16_t)extra[k];
        W.item[i] = (dm_go_item){ 1, 1, (uint8_t)dm_ge_hand_of(&G.p, extra[k]), 0, { 0, (int8_t)k, 0 } }; W.grid[0].used[0] |= (uint16_t)(1u << k); }
    CHECK(dm_ge_rules_init(&RR, &G.p, CAND, W.nitem, &R) == 0);
}
static int item_of(int cand) { for (int i = 0; i < W.nitem; i++) if (CAND[i] == cand) return i; return -1; }

static int hands_to(int right, int left) {
    int8_t t[DM_GO_SLOTS]; dm_gear_order_worn(&W, t); t[DM_GO_RIGHT] = (int8_t)right; t[DM_GO_LEFT] = (int8_t)left;
    static dm_go_plan p; int e = dm_gear_order_all(&W, t, &R, &p);
    if (e) return e;

    dm_go_world w = W; int hand = -1;
    for (int k = 0; k < p.nstep; k++) if (dm_gear_order_apply(&w, &p.step[k].go, &hand)) return 99;
    int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(&w, worn);
    return hand == -1 && worn[DM_GO_RIGHT] == right && worn[DM_GO_LEFT] == left ? DM_GO_OK : 98;
}


static int step_of(const dm_go_plan *p, int item, int slot) { for (int k = 0; k < p->nstep; k++) if (p->step[k].go.op == DM_GO_PUT_SLOT && p->step[k].go.item == item && p->step[k].go.slot == slot) return k; return -1; }
static dm_gear_tables GT; static const dm_sim_data *D;
static int sword_world(const char *prof, int dual, int worn_sword, int alone, int *sw, int *w1, int *sh, int *ax) {
    gspec s; memset(&s, 0, sizeof s); s.profile = prof; s.act = 1; s.seed = 15; s.variants = 1; s.count[DM_GEAR_K_RING] = 1; s.dual = dual; s.worn_sword = worn_sword;
    if (gs_build(&G, D, &GT, &s)) return -1;
    *sw = *sh = -1;
    for (int q = 0; q < G.p.ncand; q++) {
        if (G.cand[q].worn && G.cand[q].kind == DM_GEAR_K_WEAPON2) *sw = q;
        if (G.cand[q].worn && G.cand[q].kind == DM_GEAR_K_SHIELD) { if (alone) G.cand[q].worn = 0; else *sh = q; }
    }
    if (*sw < 0) *sw = (int)(gs_sword(&G, 1, 0) - G.cand);
    if (*sh < 0) *sh = (int)(gs_item(&G, DM_GEAR_K_SHIELD, 1, 0) - G.cand);
    *w1 = (int)(gs_item(&G, DM_GEAR_K_WEAPON1, 1, 0) - G.cand);
    *ax = (int)(gs_item(&G, DM_GEAR_K_WEAPON2, 1, 0) - G.cand);
    int c4[4] = { *sw, *w1, *sh, *ax }, extra[4], n = 0;
    for (int k = 0; k < 4; k++) { G.cand[c4[k]].item.req_str = 0; if (!G.cand[c4[k]].worn) extra[n++] = c4[k]; }
    if (gs_capture(&G)) return -1;
    world_of(extra, n);
    return 0;
}

int main(void) {
    D = gs_tables(&GT); const dm_sim_data *d = D;
    char *prof = gs_profile("class4-level80.prof");
    gspec s; memset(&s, 0, sizeof s); s.profile = prof; s.act = 1; s.seed = 14; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
    if (gs_build(&G, d, &GT, &s)) { printf("FAIL: no stock\n"); return 1; }
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
    CHECK(R.can_wear(R.ctx, both, DM_GO_LEFT, iw));
    { int16_t none[DM_GO_MAXITEM]; memcpy(none, CAND, sizeof none); none[ir] = -1; dm_ge_rules r2; dm_go_rules o2; CHECK(dm_ge_rules_init(&r2, &G.p, none, W.nitem, &o2) == 0); CHECK(!o2.can_wear(o2.ctx, worn, DM_GO_RRING, ir)); }

    int8_t t[DM_GO_SLOTS]; dm_gear_order_worn(&W, t); t[DM_GO_RRING] = (int8_t)ir; t[DM_GO_TORSO] = (int8_t)ia; t[DM_GO_RIGHT] = (int8_t)iw;
    W.grid[0].used[0] = 0x1f; static dm_go_plan p;
    int e = dm_gear_order_all(&W, t, &R, &p); CHECK(e == DM_GO_OK);
    if (!e) { int kr = step_of(&p, ir, DM_GO_RRING), ka = step_of(&p, ia, DM_GO_TORSO), kw = step_of(&p, iw, DM_GO_RIGHT); CHECK(kr >= 0 && ka > kr && kw > ka); }

    dm_gear_order_worn(&W, t); t[DM_GO_TORSO] = (int8_t)ia; CHECK(dm_gear_order_all(&W, t, &R, &p) == DM_GO_E_RULE);


    int sw, w1, shd, ax;
    if (!sword_world(prof, 0, 0, 0, &sw, &w1, &shd, &ax)) {
        CHECK(dm_ge_hand_of(&G.p, sw) == DM_GO_H_ONE && dm_ge_hand_of(&G.p, ax) == DM_GO_H_BOTH);
        int isw = item_of(sw), ish = item_of(shd), iax = item_of(ax);
        int8_t now[DM_GO_SLOTS]; dm_gear_order_worn(&W, now); int iw = now[DM_GO_RIGHT];
        CHECK(now[DM_GO_LEFT] == ish && iw >= 0);
        CHECK(R.can_wear(R.ctx, now, DM_GO_LEFT, isw)); CHECK(!R.can_wear(R.ctx, now, DM_GO_LEFT, iax));
        CHECK(hands_to(isw, ish) == DM_GO_OK);
        CHECK(hands_to(isw, -1) == DM_GO_OK);
        CHECK(hands_to(iw, isw) == DM_GO_OK);
        CHECK(hands_to(iax, ish) == DM_GO_E_RULE);
        CHECK(hands_to(iw, iax) == DM_GO_E_RULE);
        CHECK(hands_to(iax, -1) == DM_GO_OK);
        G.p.now.cls = 0;
        for (int i = 0; i < W.nitem; i++) W.item[i].hand = (uint8_t)dm_ge_hand_of(&G.p, CAND[i]);
        CHECK(dm_ge_rules_init(&RR, &G.p, CAND, W.nitem, &R) == 0);
        CHECK(dm_ge_hand_of(&G.p, sw) == DM_GO_H_BOTH);
        CHECK(hands_to(isw, ish) == DM_GO_E_RULE); CHECK(hands_to(iw, isw) == DM_GO_E_RULE); CHECK(hands_to(isw, -1) == DM_GO_OK);
    } else CHECK(0);

    if (!sword_world(prof, 0, 1, 0, &sw, &w1, &shd, &ax)) {
        int isw = item_of(sw), ish = item_of(shd), iw = item_of(w1);
        CHECK(G.cand[sw].worn && G.cand[shd].worn);
        CHECK(hands_to(isw, -1) == DM_GO_OK); CHECK(hands_to(iw, ish) == DM_GO_OK); CHECK(hands_to(iw, isw) == DM_GO_OK);
    } else CHECK(0);

    if (!sword_world(prof, 0, 1, 1, &sw, &w1, &shd, &ax)) {
        int isw = item_of(sw), ish = item_of(shd), iw = item_of(w1), iax = item_of(ax);
        CHECK(G.cand[sw].worn && !G.cand[shd].worn);
        CHECK(hands_to(isw, ish) == DM_GO_OK); CHECK(hands_to(isw, iw) == DM_GO_OK); CHECK(hands_to(iax, -1) == DM_GO_OK); CHECK(hands_to(isw, iax) == DM_GO_E_RULE);
    } else CHECK(0);

    if (!sword_world(prof, 1, 1, 0, &sw, &w1, &shd, &ax)) {
        int8_t now[DM_GO_SLOTS]; dm_gear_order_worn(&W, now);
        int isw = item_of(sw), ish = item_of(shd), left = now[DM_GO_LEFT];
        CHECK(now[DM_GO_RIGHT] == isw && left >= 0);
        CHECK(hands_to(left, isw) == DM_GO_OK); CHECK(hands_to(isw, ish) == DM_GO_OK); CHECK(hands_to(isw, -1) == DM_GO_OK);
    } else CHECK(0);


    {
        char *prof6 = gs_profile("class6-level80.prof");
        gspec c6; memset(&c6, 0, sizeof c6); c6.profile = prof6; c6.act = 1; c6.seed = 16; c6.variants = 1; c6.count[DM_GEAR_K_RING] = 1; c6.dual = 1;
        c6.claws = 2; c6.count[DM_GEAR_K_WEAPON1] = 1;
        if (!gs_build(&G, d, &GT, &c6)) {
            int plain = -1, nc = 0;
            for (int q = 0; q < G.p.ncand; q++) if (G.cand[q].kind == DM_GEAR_K_WEAPON1) {
                if (G.weapon[G.cand[q].weapon].w.kind != DM_WK_CLAW) plain = q; else nc += G.cand[q].worn; }
            int shc = (int)(gs_item(&G, DM_GEAR_K_SHIELD, 1, 0) - G.cand);
            G.cand[shc].item.req_str = 0; if (plain >= 0) G.cand[plain].item.req_str = 0;
            int ex[2] = { plain, shc };
            if (plain >= 0 && nc == 2 && !gs_capture(&G)) {
                world_of(ex, 2);
                int8_t now[DM_GO_SLOTS]; dm_gear_order_worn(&W, now);
                int ir = now[DM_GO_RIGHT], il = now[DM_GO_LEFT], ip = item_of(plain), ish = item_of(shc);
                CHECK(ir >= 0 && il >= 0 && ip >= 0 && ish >= 0);
                int8_t right_only[DM_GO_SLOTS]; memcpy(right_only, now, sizeof right_only); right_only[DM_GO_LEFT] = -1;
                int8_t left_only[DM_GO_SLOTS]; memcpy(left_only, now, sizeof left_only); left_only[DM_GO_RIGHT] = -1;
                CHECK(R.can_wear(R.ctx, right_only, DM_GO_LEFT, il)); CHECK(!R.can_wear(R.ctx, right_only, DM_GO_LEFT, ip));
                CHECK(!R.can_wear(R.ctx, left_only, DM_GO_RIGHT, ip)); CHECK(R.can_wear(R.ctx, left_only, DM_GO_RIGHT, ir));
                CHECK(R.can_wear(R.ctx, right_only, DM_GO_LEFT, ish));
                CHECK(hands_to(il, ir) == DM_GO_OK); CHECK(hands_to(ir, ish) == DM_GO_OK); CHECK(hands_to(ip, ish) == DM_GO_OK); CHECK(hands_to(ip, -1) == DM_GO_OK);
                CHECK(hands_to(ip, il) == DM_GO_E_RULE); CHECK(hands_to(ir, ip) == DM_GO_E_RULE);
            } else CHECK(0);
        } else CHECK(0);
        free(prof6);
    }
    free(prof);
    printf("t_gear_equip_rules: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
