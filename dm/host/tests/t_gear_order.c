





#include <stdio.h>
#include <string.h>
#include "../dm_gear_order.h"

static int checks, fails; static const char *g_case;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s line %d: %s\n", g_case, __LINE__, #c); } } while (0)



typedef struct { int base, req[DM_GO_MAXITEM], str[DM_GO_MAXITEM], left_weapon; const dm_go_world *w; } model;
static int str_with(const model *m, const int8_t *worn, int except) { int s = m->base; for (int k = 1; k < DM_GO_SLOTS; k++) if (worn[k] >= 0 && worn[k] != except) s += m->str[worn[k]]; return s; }
static int m_can(void *ctx, const int8_t *worn, int slot, int item) {
    const model *m = ctx; const dm_go_item *it = &m->w->item[item];
    if (slot == DM_GO_LEFT && it->hand == DM_GO_H_ONE && !m->left_weapon) return 0;
    if (slot == DM_GO_RIGHT && it->hand == DM_GO_H_OFF) return 0;
    return str_with(m, worn, item) >= m->req[item];
}
static int m_stable(void *ctx, const int8_t *worn) { const model *m = ctx; for (int k = 1; k < DM_GO_SLOTS; k++) if (worn[k] >= 0 && str_with(m, worn, worn[k]) < m->req[worn[k]]) return 0; return 1; }

static dm_go_world W; static model M; static dm_go_rules R = { m_can, m_stable, &M }; static int8_t T[DM_GO_SLOTS];
enum { INV, STASH, SHARED };
static void world(const char *name) {
    g_case = name; memset(&W, 0, sizeof W); memset(&M, 0, sizeof M); M.base = 100; M.w = &W;
    W.ngrid = 3; W.inv = INV;
    W.grid[INV] = (dm_go_grid){ 10, 4, 1, { 0 } }; W.grid[STASH] = (dm_go_grid){ 10, 10, 1, { 0 } }; W.grid[SHARED] = (dm_go_grid){ 10, 10, 1, { 0 } };
}
static void fill(int g, int x, int y, int w, int h) { for (int r = y; r < y + h; r++) W.grid[g].used[r] |= (uint16_t)(((1u << w) - 1u) << x); }
static int add(int w, int h, int hand, int slot, int g, int x, int y) {
    int i = W.nitem++; W.item[i] = (dm_go_item){ (uint8_t)w, (uint8_t)h, (uint8_t)hand, (int8_t)slot, { (int8_t)(slot ? -1 : g), (int8_t)(slot ? -1 : x), (int8_t)(slot ? -1 : y) } };
    if (!slot) fill(g, x, y, w, h);
    return i;
}
static void target_worn(void) { dm_gear_order_worn(&W, T); }






static int game_apply(dm_go_world *w, const dm_go_move *m, int *hand) {
    if (m->op == DM_GO_PUT_SLOT && m->item >= 0 && m->item < w->nitem && m->slot > 0 && m->slot < DM_GO_SLOTS) {
        int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(w, worn);
        if (!m_can(&M, worn, m->slot, m->item)) return -1;
        int both = w->item[m->item].hand == DM_GO_H_BOTH, got = worn[m->slot], r = worn[DM_GO_RIGHT];
        if (both && (m->slot == DM_GO_LEFT || (m->slot == DM_GO_RIGHT && worn[DM_GO_LEFT] >= 0))) return -1;
        if (m->slot == DM_GO_LEFT && r >= 0 && w->item[r].hand == DM_GO_H_BOTH) got = r;
        if (got != m->swap) return -1;
    }
    return dm_gear_order_apply(w, m, hand);
}


static int run(const dm_go_plan *p, dm_go_world *out) {
    dm_go_world w = W; int hand = -1;
    for (int c = 0; c < p->nchange; c++) {
        for (int k = p->change[c].first; k < p->change[c].first + p->change[c].n; k++) if (game_apply(&w, &p->step[k].go, &hand)) return 0;
        if (hand != -1) return 0;
    }
    if (out) *out = w;
    return 1;
}
static int same_world(const dm_go_world *a, const dm_go_world *b) {
    for (int i = 0; i < a->nitem; i++) { const dm_go_item *x = &a->item[i], *y = &b->item[i];
        if (x->slot != y->slot || (!x->slot && (x->cell.grid != y->cell.grid || x->cell.x != y->cell.x || x->cell.y != y->cell.y))) return 0; }
    for (int g = 0; g < a->ngrid; g++) if (memcmp(a->grid[g].used, b->grid[g].used, sizeof a->grid[g].used)) return 0;
    return 1;
}

static int undo_all(const dm_go_plan *p) {
    for (int c = 0; c < p->nchange; c++) for (int k = p->change[c].first; k < p->change[c].first + p->change[c].n; k++) {
        dm_go_world w = W, before; int hand = -1;
        for (int j = 0; j < p->change[c].first; j++) if (game_apply(&w, &p->step[j].go, &hand)) return 0;
        before = w;
        for (int j = p->change[c].first; j <= k; j++) if (game_apply(&w, &p->step[j].go, &hand)) return 0;
        for (int j = k; j >= p->change[c].first; j--) if (game_apply(&w, &p->step[j].back, &hand)) return 0;
        if (hand != -1 || !same_world(&w, &before)) return 0;
    }
    return 1;
}
static int worn_now(const dm_go_world *w, int slot) { int8_t m[DM_GO_SLOTS]; dm_gear_order_worn(w, m); return m[slot]; }
static int at(const dm_go_world *w, int i, int g, int x, int y) { const dm_go_item *it = &w->item[i]; return !it->slot && it->cell.grid == g && it->cell.x == x && it->cell.y == y; }

static void t_amulet_from_stash(void) {
    world("amulet from the stash, the old one into its cell");
    int old = add(1, 1, 0, DM_GO_NECK, 0, 0, 0), nw = add(1, 1, 0, 0, STASH, 4, 6);
    target_worn(); T[DM_GO_NECK] = (int8_t)nw;
    dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 1 && p.nstep == 3 && p.change[0].slots == 1u << DM_GO_NECK);
    CHECK(p.step[0].go.op == DM_GO_PICK_CELL && p.step[1].go.op == DM_GO_PUT_SLOT && p.step[1].go.swap == old && p.step[2].go.op == DM_GO_PUT_CELL);
    CHECK(run(&p, &end) && worn_now(&end, DM_GO_NECK) == nw && at(&end, old, STASH, 4, 6) && undo_all(&p));
    CHECK(p.step[1].back.op == DM_GO_PUT_SLOT && p.step[1].back.item == old && p.step[1].back.swap == nw);
}
static void t_old_does_not_fit(void) {
    world("a bigger old item: another stash cell, then the inventory, then no room");
    int old = add(2, 3, 0, DM_GO_TORSO, 0, 0, 0), nw = add(2, 2, 0, 0, STASH, 0, 0);
    fill(STASH, 0, 2, 10, 8);
    target_worn(); T[DM_GO_TORSO] = (int8_t)nw; dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && run(&p, &end) && at(&end, old, INV, 0, 0) && undo_all(&p));
    for (int r = 2; r < 4; r++) W.grid[STASH].used[r] &= (uint16_t)~(3u << 6);
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && run(&p, &end) && at(&end, old, STASH, 6, 0) && undo_all(&p));
    fill(STASH, 6, 0, 2, 4); fill(INV, 0, 0, 10, 4);
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_ROOM && p.nstep == 0);
    CHECK(dm_gear_order_one(&W, T, DM_GO_TORSO, &R, &p) == DM_GO_E_ROOM);
}
static void t_two_hander(void) {
    world("two-hander for weapon and shield: the off-hand first");
    int wpn = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0), sh = add(2, 2, DM_GO_H_OFF, DM_GO_LEFT, 0, 0, 0), two = add(2, 4, DM_GO_H_BOTH, 0, STASH, 0, 0);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)two; T[DM_GO_LEFT] = -1; dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 1 && p.change[0].slots == (1u << DM_GO_RIGHT | 1u << DM_GO_LEFT));
    CHECK(p.step[0].go.op == DM_GO_PICK_SLOT && p.step[0].go.item == sh && p.step[1].go.op == DM_GO_PUT_CELL && p.step[1].go.cell.grid == STASH);
    CHECK(p.step[2].go.op == DM_GO_PICK_CELL && p.step[2].go.item == two && p.step[3].go.op == DM_GO_PUT_SLOT && p.step[3].go.swap == wpn);
    CHECK(run(&p, &end) && worn_now(&end, DM_GO_RIGHT) == two && worn_now(&end, DM_GO_LEFT) == -1 && at(&end, wpn, STASH, 0, 0) && undo_all(&p));

    { dm_go_world w = W; int hand = -1, ok = 1;
      for (int k = 0; k < p.nstep; k++) { dm_gear_order_apply(&w, &p.step[k].go, &hand); int r = worn_now(&w, DM_GO_RIGHT), l = worn_now(&w, DM_GO_LEFT);
          if (r >= 0 && l >= 0 && (W.item[r].hand == DM_GO_H_BOTH || W.item[l].hand == DM_GO_H_BOTH)) ok = 0; }
      CHECK(ok); }
    world("weapon and shield for a two-hander: the weapon first");
    two = add(2, 4, DM_GO_H_BOTH, DM_GO_RIGHT, 0, 0, 0); wpn = add(1, 3, DM_GO_H_ONE, 0, INV, 0, 0); sh = add(2, 2, DM_GO_H_OFF, 0, INV, 1, 0);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)wpn; T[DM_GO_LEFT] = (int8_t)sh;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.step[0].go.item == wpn && p.step[1].go.swap == two);
    CHECK(run(&p, &end) && worn_now(&end, DM_GO_RIGHT) == wpn && worn_now(&end, DM_GO_LEFT) == sh && !end.item[two].slot && undo_all(&p));
    world("a shield for the two-hander, the right hand left empty: the two-hander off first");
    two = add(2, 4, DM_GO_H_BOTH, DM_GO_RIGHT, 0, 0, 0); sh = add(2, 2, DM_GO_H_OFF, 0, INV, 4, 0);
    { dm_go_world w = W; int hand = -1; dm_go_move pk = { DM_GO_PICK_CELL, (int8_t)sh, 0, -1, { INV, 4, 0 } }, put = { DM_GO_PUT_SLOT, (int8_t)sh, DM_GO_LEFT, -1, { -1, -1, -1 } };
      CHECK(game_apply(&w, &pk, &hand) == 0 && game_apply(&w, &put, &hand) == -1 && hand == sh); }
    target_worn(); T[DM_GO_RIGHT] = -1; T[DM_GO_LEFT] = (int8_t)sh;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nstep == 4 && p.step[0].go.op == DM_GO_PICK_SLOT && p.step[0].go.item == two);
    CHECK(p.step[3].go.op == DM_GO_PUT_SLOT && p.step[3].go.item == sh && p.step[3].go.swap == -1);
    CHECK(run(&p, &end) && worn_now(&end, DM_GO_LEFT) == sh && worn_now(&end, DM_GO_RIGHT) == -1 && !end.item[two].slot && undo_all(&p));
    world("a shield with the two-hander kept: refused");
    two = add(2, 4, DM_GO_H_BOTH, DM_GO_RIGHT, 0, 0, 0); sh = add(2, 2, DM_GO_H_OFF, 0, INV, 0, 0);
    target_worn(); T[DM_GO_LEFT] = (int8_t)sh;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_RULE && dm_gear_order_one(&W, T, DM_GO_LEFT, &R, &p) == DM_GO_E_RULE);
}
static void t_weapon_pair(void) {
    world("weapon pair: the second weapon replaces the shield");
    int w1 = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0), sh = add(2, 2, DM_GO_H_OFF, DM_GO_LEFT, 0, 0, 0), w2 = add(1, 3, DM_GO_H_ONE, 0, SHARED, 5, 5);
    target_worn(); T[DM_GO_LEFT] = (int8_t)w2; dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_RULE);
    M.left_weapon = 1;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nstep == 3 && run(&p, &end) && worn_now(&end, DM_GO_LEFT) == w2 && worn_now(&end, DM_GO_RIGHT) == w1);
    CHECK(at(&end, sh, SHARED, 5, 5) && undo_all(&p));
    fill(SHARED, 6, 5, 1, 1);
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && run(&p, &end) && at(&end, sh, SHARED, 0, 0) && undo_all(&p));
    world("a two-hander for a weapon pair: the second weapon after the first");
    M.left_weapon = 1; int two = add(2, 4, DM_GO_H_BOTH, DM_GO_RIGHT, 0, 0, 0); w1 = add(1, 3, DM_GO_H_ONE, 0, INV, 8, 1); w2 = add(1, 3, DM_GO_H_ONE, 0, INV, 7, 1);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)w1; T[DM_GO_LEFT] = (int8_t)w2;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.step[0].go.item == w1 && p.step[1].go.swap == two && run(&p, &end) && undo_all(&p));
    CHECK(worn_now(&end, DM_GO_RIGHT) == w1 && worn_now(&end, DM_GO_LEFT) == w2 && !end.item[two].slot);
    world("a weapon pair for a two-hander: the second weapon off first");
    M.left_weapon = 1; w1 = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0); w2 = add(1, 3, DM_GO_H_ONE, DM_GO_LEFT, 0, 0, 0); two = add(2, 4, DM_GO_H_BOTH, 0, STASH, 2, 2);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)two; T[DM_GO_LEFT] = -1;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.step[0].go.op == DM_GO_PICK_SLOT && p.step[0].go.item == w2 && p.step[1].go.cell.grid == STASH);
    CHECK(run(&p, &end) && worn_now(&end, DM_GO_RIGHT) == two && worn_now(&end, DM_GO_LEFT) == -1 && at(&end, w1, STASH, 2, 2) && undo_all(&p));
    world("weapon pair with the hands swapped");
    M.left_weapon = 1; w1 = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0); w2 = add(1, 3, DM_GO_H_ONE, DM_GO_LEFT, 0, 0, 0);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)w2; T[DM_GO_LEFT] = (int8_t)w1;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && run(&p, &end) && worn_now(&end, DM_GO_RIGHT) == w2 && worn_now(&end, DM_GO_LEFT) == w1 && undo_all(&p));
    world("weapon pair back to weapon and shield");
    M.left_weapon = 1; w1 = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0); w2 = add(1, 3, DM_GO_H_ONE, DM_GO_LEFT, 0, 0, 0); sh = add(2, 2, DM_GO_H_OFF, 0, STASH, 3, 3);
    target_worn(); T[DM_GO_LEFT] = (int8_t)sh;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && run(&p, &end) && worn_now(&end, DM_GO_LEFT) == sh && at(&end, w2, STASH, 3, 3) && undo_all(&p));
}
static void t_rings(void) {
    world("ring pair: a worn ring named for the other side stays where it is");
    int a = add(1, 1, 0, DM_GO_RRING, 0, 0, 0), b = add(1, 1, 0, DM_GO_LRING, 0, 0, 0), c = add(1, 1, 0, 0, STASH, 9, 9);
    target_worn(); T[DM_GO_RRING] = (int8_t)b; T[DM_GO_LRING] = (int8_t)c; dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 1 && p.change[0].slots == 1u << DM_GO_RRING && p.nstep == 3);
    CHECK(run(&p, &end) && worn_now(&end, DM_GO_RRING) == c && worn_now(&end, DM_GO_LRING) == b && at(&end, a, STASH, 9, 9) && undo_all(&p));
    target_worn(); T[DM_GO_RRING] = (int8_t)b; T[DM_GO_LRING] = (int8_t)a;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 0 && p.nstep == 0);
    world("two new rings: two changes, each old ring into its new ring's cell");
    a = add(1, 1, 0, DM_GO_RRING, 0, 0, 0); b = add(1, 1, 0, DM_GO_LRING, 0, 0, 0); c = add(1, 1, 0, 0, STASH, 1, 1); int d = add(1, 1, 0, 0, INV, 2, 2);
    target_worn(); T[DM_GO_RRING] = (int8_t)c; T[DM_GO_LRING] = (int8_t)d;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 2 && run(&p, &end) && at(&end, a, STASH, 1, 1) && at(&end, b, INV, 2, 2) && undo_all(&p));
}
static void t_requirements(void) {
    world("requirement chain: the supplier goes first whatever its slot");
    M.base = 50; int helm = add(2, 2, 0, 0, STASH, 0, 0), glove = add(2, 2, 0, 0, STASH, 2, 0);
    M.req[helm] = 60; M.str[glove] = 20;
    target_worn(); T[DM_GO_HEAD] = (int8_t)helm; T[DM_GO_GLOVES] = (int8_t)glove; dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 2 && p.change[0].slots == 1u << DM_GO_GLOVES && p.change[1].slots == 1u << DM_GO_HEAD);
    CHECK(run(&p, &end) && undo_all(&p));
    CHECK(dm_gear_order_one(&W, T, DM_GO_HEAD, &R, &p) == DM_GO_E_RULE);
    CHECK(dm_gear_order_one(&W, T, DM_GO_GLOVES, &R, &p) == DM_GO_OK && p.nchange == 1);
    world("a removed supplier goes last");
    M.base = 50; int belt = add(2, 1, 0, DM_GO_BELT, 0, 0, 0), boots = add(2, 2, 0, DM_GO_FEET, 0, 0, 0), belt2 = add(2, 1, 0, 0, STASH, 0, 0), boots2 = add(2, 2, 0, 0, STASH, 4, 0);
    M.str[belt] = 20; M.req[boots] = 65; M.req[boots2] = 40;
    target_worn(); T[DM_GO_BELT] = (int8_t)belt2; T[DM_GO_FEET] = (int8_t)boots2;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 2 && p.change[0].slots == 1u << DM_GO_FEET && p.change[1].slots == 1u << DM_GO_BELT);
    CHECK(run(&p, &end) && undo_all(&p));
    CHECK(dm_gear_order_one(&W, T, DM_GO_BELT, &R, &p) == DM_GO_E_RULE);
    world("no safe order: each needs the other's strength");
    M.base = 50; int x = add(2, 2, 0, 0, STASH, 0, 0), y = add(2, 2, 0, 0, STASH, 2, 0); M.req[x] = M.req[y] = 60; M.str[x] = M.str[y] = 20;
    target_worn(); T[DM_GO_HEAD] = (int8_t)x; T[DM_GO_TORSO] = (int8_t)y;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_RULE && p.nchange == 0 && p.nstep == 0);
}
static void t_way_back(void) {
    world("the way back must meet the requirements: the old shield needs the old weapon's strength");
    M.base = 50; int w0 = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0), s0 = add(2, 2, DM_GO_H_OFF, DM_GO_LEFT, 0, 0, 0);
    int w1 = add(1, 3, DM_GO_H_ONE, 0, STASH, 0, 0), s1 = add(2, 2, DM_GO_H_OFF, 0, STASH, 2, 0);
    M.str[w0] = 30; M.req[s0] = 80; M.req[w1] = 50; M.req[s1] = 50;
    target_worn(); T[DM_GO_RIGHT] = (int8_t)w1; T[DM_GO_LEFT] = (int8_t)s1; dm_go_plan p; dm_go_world end;
    { dm_go_world w = W; int hand = -1;
      dm_go_move mv[6] = { { DM_GO_PICK_CELL, (int8_t)w1, 0, -1, { STASH, 0, 0 } }, { DM_GO_PUT_SLOT, (int8_t)w1, DM_GO_RIGHT, (int8_t)w0, { -1, -1, -1 } },
                           { DM_GO_PUT_CELL, (int8_t)w0, 0, -1, { STASH, 0, 0 } }, { DM_GO_PICK_CELL, (int8_t)s1, 0, -1, { STASH, 2, 0 } },
                           { DM_GO_PUT_SLOT, (int8_t)s1, DM_GO_LEFT, (int8_t)s0, { -1, -1, -1 } }, { DM_GO_PUT_SLOT, (int8_t)s0, DM_GO_LEFT, (int8_t)s1, { -1, -1, -1 } } };
      int ok = 1; for (int k = 0; k < 5; k++) ok &= game_apply(&w, &mv[k], &hand) == 0;
      CHECK(ok && hand == s0 && game_apply(&w, &mv[5], &hand) == -1); }
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 1 && p.step[0].go.item == s1 && p.step[1].go.slot == DM_GO_LEFT);
    CHECK(run(&p, &end) && worn_now(&end, DM_GO_RIGHT) == w1 && worn_now(&end, DM_GO_LEFT) == s1 && undo_all(&p));
    CHECK(dm_gear_order_one(&W, T, DM_GO_LEFT, &R, &p) == DM_GO_OK && p.step[0].go.item == s1 && undo_all(&p));
    world("no way back in any order: refused before anything moves");
    M.base = 50; w0 = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0); s0 = add(2, 2, DM_GO_H_OFF, DM_GO_LEFT, 0, 0, 0);
    w1 = add(1, 3, DM_GO_H_ONE, 0, STASH, 0, 0); s1 = add(2, 2, DM_GO_H_OFF, 0, STASH, 2, 0);
    M.str[w0] = 30; M.str[s0] = 30; M.req[w0] = 80; M.req[s0] = 80;
    target_worn(); T[DM_GO_RIGHT] = (int8_t)w1; T[DM_GO_LEFT] = (int8_t)s1;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_RULE && p.nstep == 0);
}
static void t_room_search(void) {
    world("the first free spot would block a later item: the other cell is taken");

    fill(STASH, 0, 0, 10, 10); fill(SHARED, 0, 0, 10, 10);
    fill(INV, 2, 3, 2, 1); fill(INV, 4, 2, 2, 2); fill(INV, 6, 0, 4, 4);
    M.base = 50; int w0 = add(2, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0), s0 = add(2, 4, DM_GO_H_OFF, DM_GO_LEFT, 0, 0, 0);
    int w1 = add(1, 3, DM_GO_H_ONE, 0, INV, 0, 0), s1 = add(2, 2, DM_GO_H_OFF, 0, INV, 4, 0);
    M.str[w1] = 20; M.req[s1] = 60;
    target_worn(); T[DM_GO_RIGHT] = (int8_t)w1; T[DM_GO_LEFT] = (int8_t)s1; dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.step[0].go.item == w1);
    CHECK(run(&p, &end) && at(&end, w0, INV, 2, 0) && at(&end, s0, INV, 0, 0) && undo_all(&p));
    CHECK(worn_now(&end, DM_GO_RIGHT) == w1 && worn_now(&end, DM_GO_LEFT) == s1);
    W.grid[INV].used[3] |= 1u << 1;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_ROOM && p.nstep == 0);
    world("cells across changes: the old helm leaves the only 2x3 spot to the old armour");
    fill(STASH, 0, 0, 10, 10); fill(SHARED, 0, 0, 10, 10); fill(INV, 0, 0, 10, 4);
    for (int r = 0; r < 3; r++) W.grid[INV].used[r] &= (uint16_t)~(3u << 0);
    W.grid[INV].used[0] &= (uint16_t)~(3u << 4); W.grid[INV].used[1] &= (uint16_t)~(3u << 4);
    M.base = 50; int h0 = add(2, 2, 0, DM_GO_HEAD, 0, 0, 0), a0 = add(2, 3, 0, DM_GO_TORSO, 0, 0, 0);
    int h1 = add(1, 1, 0, 0, INV, 8, 3), a1 = add(1, 1, 0, 0, INV, 9, 3);
    M.str[h1] = 20; M.req[a1] = 60;
    target_worn(); T[DM_GO_HEAD] = (int8_t)h1; T[DM_GO_TORSO] = (int8_t)a1;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 2 && p.change[0].slots == 1u << DM_GO_HEAD && run(&p, &end) && undo_all(&p));
    CHECK(at(&end, h0, INV, 4, 0) && at(&end, a0, INV, 0, 0) && worn_now(&end, DM_GO_HEAD) == h1 && worn_now(&end, DM_GO_TORSO) == a1);
}
static void t_refused(void) {
    dm_go_plan p;
    world("weapon set II is never changed");
    int s2 = add(1, 3, DM_GO_H_ONE, 11, 0, 0, 0), w = add(1, 3, DM_GO_H_ONE, 0, INV, 0, 0); (void)s2;
    target_worn(); T[11] = (int8_t)w; CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_INPUT);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)s2; CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_MOVE);
    world("a worn item to another slot");
    int ring = add(1, 1, 0, DM_GO_RRING, 0, 0, 0); int amu = add(1, 1, 0, 0, INV, 0, 0); (void)amu;
    target_worn(); T[DM_GO_NECK] = (int8_t)ring; T[DM_GO_RRING] = -1; CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_MOVE);
    world("an item in hand, an overlap, a cell not marked used, one item twice");
    int a = add(1, 1, 0, 0, INV, 0, 0); target_worn();
    W.item[a].cell.grid = -1; CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_INPUT); W.item[a].cell.grid = INV;
    int b = add(1, 1, 0, 0, INV, 0, 0); (void)b; CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_INPUT); W.nitem--;
    W.grid[INV].used[0] = 0; CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_INPUT); fill(INV, 0, 0, 1, 1);
    target_worn(); T[DM_GO_HEAD] = T[DM_GO_TORSO] = (int8_t)a; CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_E_INPUT);
    target_worn(); CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 0);
    world("apply refuses a move that does not fit");
    a = add(1, 1, 0, 0, INV, 0, 0); b = add(1, 1, 0, 0, INV, 1, 0); int hand = -1;
    dm_go_move m = { DM_GO_PICK_CELL, (int8_t)a, 0, -1, { INV, 1, 0 } }; CHECK(dm_gear_order_apply(&W, &m, &hand) == -1 && hand == -1);
    m.cell.x = 0; CHECK(dm_gear_order_apply(&W, &m, &hand) == 0 && hand == a);
    m.op = DM_GO_PUT_CELL; m.cell.x = 1; CHECK(dm_gear_order_apply(&W, &m, &hand) == -1 && hand == a);
    m.op = DM_GO_PUT_SLOT; m.slot = DM_GO_NECK; m.swap = (int8_t)b; CHECK(dm_gear_order_apply(&W, &m, &hand) == -1);
}
static void t_all(void) {
    world("a full set from inventory, stash and shared tab, undone from every step");
    M.base = 40; M.left_weapon = 1;
    int helm0 = add(2, 2, 0, DM_GO_HEAD, 0, 0, 0), amu0 = add(1, 1, 0, DM_GO_NECK, 0, 0, 0), w0 = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0), s0 = add(2, 2, DM_GO_H_OFF, DM_GO_LEFT, 0, 0, 0);
    int r0 = add(1, 1, 0, DM_GO_RRING, 0, 0, 0), gl0 = add(2, 2, 0, DM_GO_GLOVES, 0, 0, 0);
    int helm = add(2, 2, 0, 0, STASH, 0, 0), amu = add(1, 1, 0, 0, INV, 9, 3), two = add(2, 4, DM_GO_H_BOTH, 0, SHARED, 0, 0), r1 = add(1, 1, 0, 0, SHARED, 9, 9), r2 = add(1, 1, 0, 0, STASH, 9, 9), gl = add(2, 2, 0, 0, INV, 0, 0);
    M.str[gl] = 30; M.req[two] = 65; M.str[gl0] = 0; (void)helm0; (void)amu0; (void)w0; (void)s0; (void)r0;
    target_worn(); T[DM_GO_HEAD] = (int8_t)helm; T[DM_GO_NECK] = (int8_t)amu; T[DM_GO_RIGHT] = (int8_t)two; T[DM_GO_LEFT] = -1; T[DM_GO_RRING] = (int8_t)r1; T[DM_GO_LRING] = (int8_t)r2; T[DM_GO_GLOVES] = (int8_t)gl;
    dm_go_plan p; dm_go_world end;
    CHECK(dm_gear_order_all(&W, T, &R, &p) == DM_GO_OK && p.nchange == 6 && run(&p, &end) && undo_all(&p));
    int gl_at = -1, hands_at = -1; for (int c = 0; c < p.nchange; c++) { if (p.change[c].slots == 1u << DM_GO_GLOVES) gl_at = c; if (p.change[c].slots >> DM_GO_RIGHT & 1) hands_at = c; }
    CHECK(gl_at >= 0 && hands_at > gl_at);
    for (int s = 1; s <= DM_GO_GLOVES; s++) CHECK(worn_now(&end, s) == T[s]);
    for (int c = 0; c < p.nchange; c++) for (int k = p.change[c].first; k < p.change[c].first + p.change[c].n; k++) CHECK(p.step[k].change == c);
}

static void t_all_seq(void) {
    world("all changes in the order the rows list them");
    M.base = 40;
    int amu0 = add(1, 1, 0, DM_GO_NECK, 0, 0, 0), r0 = add(1, 1, 0, DM_GO_RRING, 0, 0, 0), b0 = add(2, 2, 0, DM_GO_FEET, 0, 0, 0);
    int amu = add(1, 1, 0, 0, INV, 9, 3), r1 = add(1, 1, 0, 0, STASH, 0, 0), b1 = add(2, 2, 0, 0, INV, 0, 0), gl = add(2, 2, 0, 0, STASH, 4, 4);
    M.str[gl] = 30; M.req[r1] = 60; (void)amu0; (void)r0; (void)b0;
    target_worn(); T[DM_GO_NECK] = (int8_t)amu; T[DM_GO_RRING] = (int8_t)r1; T[DM_GO_FEET] = (int8_t)b1; T[DM_GO_GLOVES] = (int8_t)gl;
    dm_go_plan p; dm_go_world end;
    const int8_t want[] = { DM_GO_GLOVES, DM_GO_FEET, DM_GO_RRING, DM_GO_NECK };
    CHECK(dm_gear_order_all_seq(&W, T, &R, want, 4, &p) == DM_GO_OK && p.nchange == 4 && run(&p, &end) && undo_all(&p));
    for (int c = 0; c < p.nchange && c < 4; c++) CHECK(p.change[c].slots == 1u << want[c]);

    const int8_t bad[] = { DM_GO_RRING, DM_GO_NECK, DM_GO_GLOVES, DM_GO_FEET };
    CHECK(dm_gear_order_all_seq(&W, T, &R, bad, 4, &p) == DM_GO_OK && p.nchange == 4 && run(&p, &end) && undo_all(&p));
    int ring_at = -1, gl_at = -1, neck_at = -1, feet_at = -1;
    for (int c = 0; c < p.nchange; c++) {
        unsigned m = p.change[c].slots;
        ring_at = m == 1u << DM_GO_RRING ? c : ring_at; gl_at = m == 1u << DM_GO_GLOVES ? c : gl_at;
        neck_at = m == 1u << DM_GO_NECK ? c : neck_at; feet_at = m == 1u << DM_GO_FEET ? c : feet_at;
    }
    CHECK(gl_at >= 0 && ring_at > gl_at && neck_at >= 0 && feet_at >= 0);
    CHECK(dm_gear_order_all_seq(&W, T, &R, NULL, 0, &p) == DM_GO_OK && p.change[0].slots == 1u << DM_GO_NECK);
}

int main(void) {
    t_amulet_from_stash();
    t_old_does_not_fit();
    t_two_hander();
    t_weapon_pair();
    t_rings();
    t_requirements();
    t_way_back();
    t_room_search();
    t_refused();
    t_all();
    t_all_seq();
    printf("t_gear_order: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
