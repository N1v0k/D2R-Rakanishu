









#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../dm_gear_equip_core.h"

static int checks, fails; static const char *g_case;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s line %d: %s\n", g_case, __LINE__, #c); } } while (0)


enum { G_INV, G_P0, G_P1, G_P2, G_TAB, G_TAB2, G_N };
static const dm_ge_place GP[DM_GO_MAXGRID] = { { DM_GE_INV, 0 }, { DM_GE_PAGE, 0 }, { DM_GE_PAGE, 1 }, { DM_GE_PAGE, 2 }, { DM_GE_TAB, 0 }, { DM_GE_TAB, 2 } };
static dm_go_world W; static int8_t T[DM_GO_SLOTS];
static int g_left_weapon;
static void world(const char *name) {
    g_case = name; memset(&W, 0, sizeof W); W.ngrid = G_N; W.inv = G_INV; g_left_weapon = 0;
    for (int g = G_INV; g < G_N; g++) { int w = 0, h = 0; dm_ge_grid_size(GP[g].where, &w, &h); W.grid[g] = (dm_go_grid){ (uint8_t)w, (uint8_t)h, 1, { 0 } }; }
}
static void fill(int g, int x, int y, int w, int h) { for (int r = y; r < y + h; r++) W.grid[g].used[r] |= (uint16_t)(((1u << w) - 1u) << x); }
static int add(int w, int h, int hand, int slot, int g, int x, int y) {
    int i = W.nitem++; W.item[i] = (dm_go_item){ (uint8_t)w, (uint8_t)h, (uint8_t)hand, (int8_t)slot, { (int8_t)(slot ? -1 : g), (int8_t)(slot ? -1 : x), (int8_t)(slot ? -1 : y) } };
    if (!slot) fill(g, x, y, w, h);
    return i;
}
static void target_worn(void) { dm_gear_order_worn(&W, T); }
static int r_can(void *ctx, const int8_t *worn, int slot, int item) {
    (void)worn; const dm_go_world *w = ctx; const dm_go_item *it = &w->item[item];
    if (slot == DM_GO_LEFT && it->hand == DM_GO_H_ONE && !g_left_weapon) return 0;
    if (slot == DM_GO_RIGHT && it->hand == DM_GO_H_OFF) return 0;
    return 1;
}
static int r_stable(void *ctx, const int8_t *worn) { (void)ctx; (void)worn; return 1; }


typedef struct {
    dm_go_world w; int hand;
    int shown, flips, settled, commits, panel, foreign;

    int inflight, due; dm_go_move mv;
    int flip_due, flip_dir, save_due;
    int nosave, delay_max, lost_pct, late_pct, refuse_pct, close_at, foreign_at;
    int bad_flip_hand, bad_flip_unsaved, bad_hidden, bad_twice;
    int view, view_due, view_turns, bad_view;
    unsigned rnd;
} game;
static unsigned rnd(game *g) { g->rnd = g->rnd * 1103515245u + 12345u; return g->rnd >> 16 & 0x7fff; }
static int page_of_grid(int grid) { return grid >= G_P0 && grid <= G_P2 ? grid - G_P0 : -1; }


static int game_code(const game *g, int slot) {
    if (g->hand < 0) return 0;
    int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(&g->w, worn);
    const dm_go_item *it = &g->w.item[g->hand];
    if (it->hand == DM_GO_H_BOTH && slot == DM_GO_RIGHT && worn[DM_GO_LEFT] >= 0) return 7;
    if (slot == DM_GO_LEFT && worn[DM_GO_RIGHT] >= 0 && g->w.item[worn[DM_GO_RIGHT]].hand == DM_GO_H_BOTH) return 2;
    return worn[slot] >= 0 ? 5 : 1;
}
static void game_frame(game *g, int frame) {
    if (g->inflight && frame >= g->due) {
        g->inflight = 0;
        int pg = page_of_grid(g->mv.cell.grid);
        if ((g->mv.op == DM_GO_PICK_CELL || g->mv.op == DM_GO_PUT_CELL) && pg >= 0 && pg != g->shown) g->bad_hidden++;
        else if (g->mv.op == DM_GO_PUT_SLOT && game_code(g, g->mv.slot) != (g->mv.swap >= 0 ? 5 : 1)) { }
        else dm_gear_order_apply(&g->w, &g->mv, &g->hand);
    }
    if (g->flip_due && frame >= g->flip_due) { g->flip_due = 0; g->shown += g->flip_dir; g->flips++; g->settled = 0; g->save_due = g->nosave ? 0 : frame + 4 + (int)(rnd(g) % 20); }
    if (g->save_due && frame >= g->save_due) { g->save_due = 0; g->commits++; g->settled = 1; }
    if (g->close_at && frame == g->close_at) g->panel = 0;
    if (g->foreign_at && frame == g->foreign_at && g->hand < 0 && !g->inflight) g->foreign = 1;
}
static int view_of(dm_ge_place p) { return p.where == DM_GE_TAB ? 100 + p.index : 0; }
static void game_act(game *g, const dm_ge_act *a, int frame) {
    if (a->kind == DM_GE_A_SEND) {
        int gr = a->move.op == DM_GO_PICK_CELL || a->move.op == DM_GO_PUT_CELL ? a->move.cell.grid : -1;
        if (gr >= 0 && GP[gr].where != DM_GE_INV && g->view != view_of(GP[gr])) g->bad_view++;
        if (g->inflight) g->bad_twice++;
        int r = (int)(rnd(g) % 100);
        g->inflight = 1; g->mv = a->move; g->due = frame + 1 + (int)(rnd(g) % (unsigned)(g->delay_max ? g->delay_max : 3));
        if (r < g->lost_pct) g->inflight = 0;
        else if (r < g->lost_pct + g->late_pct) g->due = frame + 190;
        else if (r < g->lost_pct + g->late_pct + g->refuse_pct) g->mv.op = 0;
    }
    if (a->kind == DM_GE_A_FLIP) {
        if (g->hand >= 0 || g->foreign) g->bad_flip_hand++;
        if (!g->settled || g->flip_due) { g->bad_flip_unsaved++; return; }
        int to = g->shown + a->dir; if (to < 0 || to > 4) return;
        g->flip_due = frame + 3 + (int)(rnd(g) % 4); g->flip_dir = a->dir;
    }
}
static void observe(const game *g, dm_ge_seen *s, int frame, const dm_ge_core *c) {
    memset(s, 0, sizeof *s);
    s->now_ms = (uint64_t)frame * 16; s->run_ok = 1; s->panel_open = g->panel; s->hand = g->foreign ? -2 : g->hand; s->shown = g->shown;
    s->flips = (unsigned)g->flips; s->settled = g->settled && !g->flip_due; s->commits = (uint64_t)g->commits; s->code = -1; s->cell_free = -1;
    for (int i = 0; i < g->w.nitem; i++) {
        dm_ge_loc l = { DM_GE_NOWHERE, -1, -1, -1 }; const dm_go_item *it = &g->w.item[i];
        if (g->hand == i) l.where = DM_GE_HAND;
        else if (it->slot) { l.where = DM_GE_BODY; l.index = it->slot; }
        else if (it->cell.grid >= 0) { l.where = GP[it->cell.grid].where; l.index = GP[it->cell.grid].index; l.x = it->cell.x; l.y = it->cell.y; }
        s->at[i] = l;
    }
    const dm_go_move *m = dm_ge_pending(c);
    if (m && m->op == DM_GO_PUT_SLOT && g->hand == m->item) s->code = game_code(g, m->slot);
    if (m && m->op == DM_GO_PUT_CELL) { const dm_go_item *it = &g->w.item[m->item]; const dm_go_grid *gr = &g->w.grid[m->cell.grid]; int ok = m->cell.x + it->w <= gr->w && m->cell.y + it->h <= gr->h;
        for (int r = m->cell.y; ok && r < m->cell.y + it->h; r++) ok = !(gr->used[r] & (((1u << it->w) - 1u) << m->cell.x));
        s->cell_free = ok; }
}

static int one_place(const game *g) {
    uint16_t occ[DM_GO_MAXGRID][DM_GO_GRIDW]; memset(occ, 0, sizeof occ); int8_t slot[DM_GO_SLOTS]; memset(slot, -1, sizeof slot);
    for (int i = 0; i < g->w.nitem; i++) { const dm_go_item *it = &g->w.item[i]; int places = (g->hand == i) + (it->slot > 0) + (it->cell.grid >= 0);
        if (places != 1) return 0;
        if (it->slot) { if (slot[it->slot] >= 0) return 0; slot[it->slot] = (int8_t)i; }
        if (it->cell.grid >= 0) for (int r = it->cell.y; r < it->cell.y + it->h; r++) { uint16_t m = (uint16_t)(((1u << it->w) - 1u) << it->cell.x); if (occ[it->cell.grid][r] & m) return 0; occ[it->cell.grid][r] |= m; } }
    return 1;
}
static int same_items(const dm_go_world *a, const dm_go_world *b) {
    for (int i = 0; i < a->nitem; i++) { const dm_go_item *x = &a->item[i], *y = &b->item[i];
        if (x->slot != y->slot || (!x->slot && (x->cell.grid != y->cell.grid || x->cell.x != y->cell.x || x->cell.y != y->cell.y))) return 0; }
    return 1;
}
static int wears_target(const dm_go_world *w) { int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(w, worn); for (int s = 1; s <= DM_GO_GLOVES; s++) if (worn[s] != T[s] && !(s >= DM_GO_RRING && s <= DM_GO_LRING)) return 0; return 1; }

typedef struct { int end, why, frames, changes; game g; } result;
static int quiet = 1;
static void log_line(void *ctx, const char *line) { (void)ctx; if (!quiet) printf("  [%s] %s\n", g_case, line); }
static result run_job(const dm_go_plan *p, const game *start, const dm_ge_fault *f) {
    static dm_ge_core c; result r; memset(&r, 0, sizeof r); r.g = *start;
    game *g = &r.g; c.log = log_line; c.log_ctx = NULL;
    if (dm_ge_begin(&c, &W, p, GP, g->shown, 0, f)) { r.end = dm_ge_end(&c, &r.why, &r.changes); r.frames = 0; return r; }
    int frame = 0;
    for (; frame < 20000; frame++) {
        game_frame(g, frame);
        if (!one_place(g)) { CHECK(!"an item in two places or none"); break; }
        dm_ge_place v;
        if (g->hand < 0 && dm_ge_view(&c, &v) && g->view != view_of(v)) {
            if (!g->view_due) { g->view_due = frame + 2 + (int)(rnd(g) % 4); g->view_turns++; }
            else if (frame >= g->view_due) { g->view = view_of(v); g->view_due = 0; }
            continue;
        }
        dm_ge_seen s; observe(g, &s, frame, &c);
        dm_ge_act a; dm_ge_step(&c, &s, &a);
        if (a.kind == DM_GE_A_END) break;
        game_act(g, &a, frame);
    }
    r.frames = frame; r.end = dm_ge_end(&c, &r.why, &r.changes);
    return r;
}
static game game_of(int shown) { game g; memset(&g, 0, sizeof g); g.w = W; g.hand = -1; g.shown = shown; g.settled = 1; g.panel = 1; g.rnd = 7; return g; }
static int plan(dm_go_plan *p) { static dm_go_rules R; R = (dm_go_rules){ r_can, r_stable, &W }; return dm_gear_order_all(&W, T, &R, p); }

static void check_end(const result *r, const dm_go_plan *p) {
    const game *g = &r->g;
    CHECK(r->end != DM_GE_LOST && r->end != DM_GE_RUNNING);
    CHECK(g->bad_flip_hand == 0); CHECK(g->bad_flip_unsaved == 0); CHECK(g->bad_hidden == 0); CHECK(g->bad_twice == 0); CHECK(g->bad_view == 0);
    CHECK((uint64_t)r->frames * 16 <= DM_GE_ALL_MS + DM_GE_SETTLE_MS + 2000);
    if (r->end == DM_GE_DONE) CHECK(wears_target(&g->w) && g->hand < 0);
    if (r->end == DM_GE_RETURNED) CHECK(same_items(&g->w, &W) && g->hand < 0);
    if (r->end == DM_GE_IN_HAND) CHECK(g->hand >= 0);
    if (r->end == DM_GE_PARTIAL) CHECK(g->hand < 0 && r->changes < p->nchange);
}


typedef struct { const char *name; int shown; void (*make)(void); } kase;
static int A, B, C, D;
static void mk_amulet(void) {
    A = add(1, 1, 0, DM_GO_NECK, 0, 0, 0); B = add(1, 1, 0, 0, G_P1, 3, 4);
    add(2, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0); fill(G_P0, 0, 0, 10, 10); fill(G_INV, 0, 0, 10, 4);
    target_worn(); T[DM_GO_NECK] = (int8_t)B;
}
static void mk_two_hander(void) {
    A = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0); B = add(2, 2, DM_GO_H_OFF, DM_GO_LEFT, 0, 0, 0); C = add(2, 4, DM_GO_H_BOTH, 0, G_P1, 0, 0);
    fill(G_INV, 0, 0, 10, 4); fill(G_P0, 0, 0, 10, 10);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)C; T[DM_GO_LEFT] = -1;
}
static void mk_one_and_shield(void) {
    A = add(2, 4, DM_GO_H_BOTH, DM_GO_RIGHT, 0, 0, 0); B = add(1, 3, DM_GO_H_ONE, 0, G_P1, 5, 5); C = add(2, 2, DM_GO_H_OFF, 0, G_P2, 0, 0);
    fill(G_INV, 0, 0, 10, 4); fill(G_P0, 0, 0, 10, 10);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)B; T[DM_GO_LEFT] = (int8_t)C;
}
static void mk_all(void) {
    A = add(1, 1, 0, DM_GO_NECK, 0, 0, 0); B = add(1, 1, 0, 0, G_P1, 0, 0);
    C = add(1, 1, 0, DM_GO_RRING, 0, 0, 0); D = add(1, 1, 0, 0, G_INV, 9, 3);
    int h0 = add(2, 2, 0, DM_GO_HEAD, 0, 0, 0), h1 = add(2, 2, 0, 0, G_TAB, 4, 4);
    target_worn(); T[DM_GO_NECK] = (int8_t)B; T[DM_GO_RRING] = (int8_t)D; T[DM_GO_HEAD] = (int8_t)h1; (void)h0;
}
static void mk_ring_left(void) {
    A = add(1, 1, 0, DM_GO_RRING, 0, 0, 0); B = add(1, 1, 0, DM_GO_LRING, 0, 0, 0); C = add(1, 1, 0, 0, G_INV, 2, 1);
    target_worn(); T[DM_GO_LRING] = (int8_t)C;
}
static int E;
static void mk_wide_tabs(void) {
    A = add(1, 1, 0, DM_GO_RRING, 0, 0, 0); B = add(1, 1, 0, 0, G_TAB, 13, 0); C = add(2, 2, 0, DM_GO_GLOVES, 0, 0, 0); D = add(2, 2, 0, 0, G_TAB2, 12, 8);
    fill(G_INV, 0, 0, 10, 4); fill(G_TAB, 0, 0, 13, 10); fill(G_TAB, 13, 1, 1, 9); fill(G_TAB2, 0, 0, 12, 10); fill(G_TAB2, 12, 0, 2, 8);
    target_worn(); T[DM_GO_RRING] = (int8_t)B; T[DM_GO_GLOVES] = (int8_t)D;
}
static void mk_tab_and_page(void) {
    A = add(1, 1, 0, DM_GO_NECK, 0, 0, 0); B = add(1, 1, 0, 0, G_TAB, 10, 9); C = add(2, 1, 0, DM_GO_BELT, 0, 0, 0); E = add(2, 1, 0, 0, G_P2, 11, 4);
    fill(G_INV, 0, 0, 10, 4); fill(G_P0, 0, 0, DM_GE_STASH_W, 10);
    target_worn(); T[DM_GO_NECK] = (int8_t)B; T[DM_GO_BELT] = (int8_t)E;
}
static void mk_hands_swapped(void) {
    g_left_weapon = 1; A = add(1, 3, DM_GO_H_ONE, DM_GO_RIGHT, 0, 0, 0); B = add(1, 3, DM_GO_H_ONE, DM_GO_LEFT, 0, 0, 0);
    fill(G_INV, 0, 0, 10, 4); W.grid[G_INV].used[0] &= (uint16_t)~(1u << 9); W.grid[G_INV].used[1] &= (uint16_t)~(1u << 9); W.grid[G_INV].used[2] &= (uint16_t)~(1u << 9);
    target_worn(); T[DM_GO_RIGHT] = (int8_t)B; T[DM_GO_LEFT] = (int8_t)A;
}
static const kase K[] = { { "amulet from page 2", 0, mk_amulet }, { "two-hander from page 2", 0, mk_two_hander },
                          { "weapon and shield from pages 2 and 3", 0, mk_one_and_shield }, { "all: amulet, ring, helm", 0, mk_all },
                          { "left ring", 2, mk_ring_left }, { "ring and gloves from shared tabs 1 and 3, last columns", 0, mk_wide_tabs },
                          { "amulet from shared tab 1, belt from page 3", 0, mk_tab_and_page }, { "hands swapped", 0, mk_hands_swapped } };
#define NK ((int)(sizeof K / sizeof K[0]))


static void last_place(const dm_go_plan *p, int *page, int *view) {
    *page = *view = -1;
    for (int k = 0; k < p->nstep; k++) { const dm_go_move *m = &p->step[k].go; int g = m->op == DM_GO_PICK_CELL || m->op == DM_GO_PUT_CELL ? m->cell.grid : -1;
        if (g < 0 || GP[g].where == DM_GE_INV) continue;
        *view = view_of(GP[g]); if (GP[g].where == DM_GE_PAGE) *page = GP[g].index; }
}
static void t_cases(void) {
    for (int k = 0; k < NK; k++) {
        world(K[k].name); K[k].make();
        dm_go_plan p; int e = plan(&p); CHECK(e == DM_GO_OK); if (e) continue;
        CHECK(dm_ge_check(&W, &p, GP) == 0);
        game g = game_of(K[k].shown); result r = run_job(&p, &g, NULL);
        check_end(&r, &p);
        CHECK(r.end == DM_GE_DONE && r.why == DM_GE_WHY_NONE);
        int page, view; last_place(&p, &page, &view);
        CHECK(r.g.shown == (page >= 0 ? page : K[k].shown)); CHECK(r.g.view == (view >= 0 ? view : 0));
        if (k == 0) { CHECK(r.g.w.item[A].cell.grid == G_P1 && r.g.w.item[A].cell.x == 3 && r.g.w.item[A].cell.y == 4); CHECK(r.g.flips == 1); }
        if (k == 1) { CHECK(r.g.w.item[B].cell.grid == G_P1); CHECK(r.g.w.item[A].cell.grid == G_P1); CHECK(p.step[0].go.op == DM_GO_PICK_SLOT && p.step[0].go.item == B); }
        if (k == 2) CHECK(r.g.flips == 2);
        if (k == 3) CHECK(r.changes == 3);
        if (k == 5) { CHECK(r.g.w.item[A].cell.grid == G_TAB && r.g.w.item[A].cell.x == 13 && r.g.w.item[A].cell.y == 0);
                      CHECK(r.g.w.item[C].cell.grid == G_TAB2 && r.g.w.item[C].cell.x == 12 && r.g.w.item[C].cell.y == 8); CHECK(r.g.view_turns == 2); CHECK(r.g.flips == 0); }
        if (k == 6) { CHECK(r.g.w.item[A].cell.grid == G_TAB && r.g.w.item[A].cell.x == 10); CHECK(r.g.w.item[C].cell.grid == G_P2 && r.g.w.item[C].cell.x == 11);
                      CHECK(r.g.view_turns == (view == 0 ? 2 : 1)); CHECK(r.g.flips == 2); }
        if (k == 3) CHECK(r.g.view_turns == 2); else if (k < 5) CHECK(r.g.view_turns == 0);
        if (k == 7) { int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(&r.g.w, worn); CHECK(worn[DM_GO_RIGHT] == B && worn[DM_GO_LEFT] == A); CHECK(r.g.flips == 0 && r.g.view_turns == 0); }
    }
}



static void t_faults(void) {
    int runs = 0, last = fails;
    for (int k = 0; k < NK; k++) {
        world(K[k].name); K[k].make();
        dm_go_plan p; if (plan(&p)) continue;
        for (int step = 1; step <= p.nstep; step++) for (int point = 1; point <= 6; point++) {
            if (point == 5) continue;
            dm_ge_fault f = { step, 0, point }; game g = game_of(K[k].shown); result r = run_job(&p, &g, &f); runs++;
            check_end(&r, &p);
            int ch = p.step[step - 1].change, before = ch, last_of = step == p.change[ch].first + p.change[ch].n;
            dm_go_world w = W; int hand = -1; for (int q = 0; q < step; q++) dm_gear_order_apply(&w, &p.step[q].go, &hand);


            if (point == 6 && hand >= 0) CHECK(r.end == DM_GE_IN_HAND || (r.end == DM_GE_RETURNED && ch == 0));
            else if (point == 6) {
                if (last_of) { CHECK(r.end == (ch + 1 == p.nchange ? DM_GE_DONE : DM_GE_PARTIAL)); CHECK(r.changes == ch + 1); }
                else { CHECK(r.end == DM_GE_PARTIAL); CHECK(r.changes == before); }
            } else {
                CHECK(r.end == (before ? DM_GE_PARTIAL : DM_GE_RETURNED) || (point == 4 && p.step[step - 1].go.op != DM_GO_PUT_SLOT && r.end == DM_GE_DONE));
                if (r.end != DM_GE_DONE) CHECK(r.changes == before);
            }
            if (r.g.hand < 0 && r.end != DM_GE_DONE) CHECK(r.g.shown == K[k].shown);
            if (fails != last) { printf("  ^ %s step %d (op %d) point %d: end %s why %s changes %d shown %d\n", K[k].name, step, p.step[step - 1].go.op, point, dm_ge_end_name(r.end), dm_ge_why_name(r.why), r.changes, r.g.shown); last = fails; }
        }
        for (int flip = 1; flip <= 4; flip++) {
            dm_ge_fault f = { 0, flip, 5 }; game g = game_of(K[k].shown); result r = run_job(&p, &g, &f); runs++;
            check_end(&r, &p);
        }
    }
    printf("t_gear_equip: %d fault runs\n", runs);
}

static void t_nosave(void) {
    world("no save after a turn"); mk_amulet();
    dm_go_plan p; CHECK(plan(&p) == DM_GO_OK);
    game g = game_of(0); g.nosave = 1; result r = run_job(&p, &g, NULL);
    check_end(&r, &p); CHECK(r.end == DM_GE_RETURNED); CHECK(r.why == DM_GE_WHY_SAVE);
    CHECK((uint64_t)r.frames * 16 < DM_GE_ALL_MS);
}

static void t_two_pages(void) {
    world("a run over two pages");
    int a = add(1, 1, 0, DM_GO_NECK, 0, 0, 0), b = add(1, 1, 0, 0, G_P1, 0, 0);
    dm_go_plan p; memset(&p, 0, sizeof p);
    p.nstep = 3; p.nchange = 1; p.change[0].first = 0; p.change[0].n = 3; p.change[0].slots = 1u << DM_GO_NECK;
    p.step[0].go = (dm_go_move){ DM_GO_PICK_CELL, (int8_t)b, 0, -1, { G_P1, 0, 0 } }; p.step[0].back = (dm_go_move){ DM_GO_PUT_CELL, (int8_t)b, 0, -1, { G_P1, 0, 0 } };
    p.step[1].go = (dm_go_move){ DM_GO_PUT_SLOT, (int8_t)b, DM_GO_NECK, (int8_t)a, { -1, -1, -1 } }; p.step[1].back = (dm_go_move){ DM_GO_PUT_SLOT, (int8_t)a, DM_GO_NECK, (int8_t)b, { -1, -1, -1 } };
    p.step[2].go = (dm_go_move){ DM_GO_PUT_CELL, (int8_t)a, 0, -1, { G_P2, 5, 5 } }; p.step[2].back = (dm_go_move){ DM_GO_PICK_CELL, (int8_t)a, 0, -1, { G_P2, 5, 5 } };
    CHECK(dm_ge_check(&W, &p, GP) == -1);
    static dm_ge_core c; memset(&c, 0, sizeof c);
    CHECK(dm_ge_begin(&c, &W, &p, GP, 0, 0, NULL) == -1); CHECK(dm_ge_end(&c, NULL, NULL) == DM_GE_RETURNED);
}

static void t_two_places(void) {
    for (int other = 0; other < 2; other++) {
        world(other ? "a run over a shared tab and a page" : "a run over two shared tabs");
        int a = add(1, 1, 0, DM_GO_NECK, 0, 0, 0), b = add(1, 1, 0, 0, G_TAB, 0, 0), to = other ? G_P0 : G_TAB2;
        dm_go_plan p; memset(&p, 0, sizeof p);
        p.nstep = 3; p.nchange = 1; p.change[0].first = 0; p.change[0].n = 3; p.change[0].slots = 1u << DM_GO_NECK;
        p.step[0].go = (dm_go_move){ DM_GO_PICK_CELL, (int8_t)b, 0, -1, { G_TAB, 0, 0 } }; p.step[0].back = (dm_go_move){ DM_GO_PUT_CELL, (int8_t)b, 0, -1, { G_TAB, 0, 0 } };
        p.step[1].go = (dm_go_move){ DM_GO_PUT_SLOT, (int8_t)b, DM_GO_NECK, (int8_t)a, { -1, -1, -1 } }; p.step[1].back = (dm_go_move){ DM_GO_PUT_SLOT, (int8_t)a, DM_GO_NECK, (int8_t)b, { -1, -1, -1 } };
        p.step[2].go = (dm_go_move){ DM_GO_PUT_CELL, (int8_t)a, 0, -1, { (int8_t)to, 5, 5 } }; p.step[2].back = (dm_go_move){ DM_GO_PICK_CELL, (int8_t)a, 0, -1, { (int8_t)to, 5, 5 } };
        CHECK(dm_ge_check(&W, &p, GP) == -1);
        p.step[2].go.cell.grid = p.step[2].back.cell.grid = G_TAB;
        CHECK(dm_ge_check(&W, &p, GP) == 0);
    }
}


static void t_grid(void) {
    int w = 0, h = 0;
    CHECK(dm_ge_grid_size(DM_GE_TAB, &w, &h) == 0 && w == 14 && h == 10);
    CHECK(dm_ge_grid_size(DM_GE_PAGE, &w, &h) == 0 && w == 14 && h == 10);
    CHECK(dm_ge_grid_size(DM_GE_INV, &w, &h) == 0 && w == 10 && h == 4);
    CHECK(dm_ge_grid_size(DM_GE_BODY, &w, &h) == -1);
    world("a ring in the last column"); mk_wide_tabs();
    dm_go_plan p; CHECK(plan(&p) == DM_GO_OK);
    W.grid[G_TAB].w = W.grid[G_TAB2].w = 10; CHECK(plan(&p) == DM_GO_E_INPUT);
}


static void t_fuzz(void) {
    int ends[DM_GE_LOST + 1] = { 0 }, runs = 0;
    for (unsigned seed = 1; seed <= 5000; seed++) {
        int k = (int)(seed % NK); world(K[k].name); K[k].make();
        dm_go_plan p; if (plan(&p)) continue;
        game g = game_of(K[k].shown); g.rnd = seed * 2654435761u;
        g.delay_max = 1 + (int)(rnd(&g) % 6); g.lost_pct = (int)(rnd(&g) % 8); g.late_pct = (int)(rnd(&g) % 6); g.refuse_pct = (int)(rnd(&g) % 6);
        if (rnd(&g) % 10 == 0) g.close_at = 5 + (int)(rnd(&g) % 300);
        if (rnd(&g) % 10 == 0) g.foreign_at = 5 + (int)(rnd(&g) % 300);
        if (rnd(&g) % 20 == 0) g.nosave = 1;
        dm_ge_fault f = { 0, 0, 0 }; if (rnd(&g) % 4 == 0) { f.step = 1 + (int)(rnd(&g) % (unsigned)p.nstep); f.point = 1 + (int)(rnd(&g) % 6); if (f.point == 5) f.flip = 1 + (int)(rnd(&g) % 3); }
        result r = run_job(&p, &g, &f); runs++;
        check_end(&r, &p); ends[r.end >= 0 && r.end <= DM_GE_LOST ? r.end : 0]++;
    }
    printf("t_gear_equip: fuzz %d runs: done %d returned %d partial %d in hand %d lost %d\n", runs, ends[DM_GE_DONE], ends[DM_GE_RETURNED], ends[DM_GE_PARTIAL], ends[DM_GE_IN_HAND], ends[DM_GE_LOST]);
    CHECK(ends[DM_GE_LOST] == 0);
}


static void t_view(void) {
    world("the view of each run"); mk_tab_and_page();
    dm_go_plan p; CHECK(plan(&p) == DM_GO_OK);
    static dm_ge_core c; memset(&c, 0, sizeof c); CHECK(dm_ge_begin(&c, &W, &p, GP, 0, 0, NULL) == 0);
    dm_ge_place v = { -1, -1 }; CHECK(dm_ge_view(&c, &v) == 1);
    int first = p.step[0].go.cell.grid; CHECK(first == G_TAB || first == G_P2);
    CHECK(v.where == GP[first].where && v.index == GP[first].index);

    world("no view for the inventory"); mk_ring_left();
    CHECK(plan(&p) == DM_GO_OK); memset(&c, 0, sizeof c); CHECK(dm_ge_begin(&c, &W, &p, GP, 2, 0, NULL) == 0);
    CHECK(dm_ge_view(&c, &v) == 0);
}
int main(int argc, char **argv) {
    quiet = !(argc > 1 && !strcmp(argv[1], "-v"));
    t_grid(); t_view(); t_cases(); t_faults(); t_nosave(); t_two_pages(); t_two_places(); t_fuzz();
    printf("t_gear_equip: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
