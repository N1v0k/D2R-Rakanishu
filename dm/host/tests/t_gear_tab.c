






#define _POSIX_C_SOURCE 200809L
#include "t_gear_stock.h"
#include "gear_brute.h"
#include "../dm_gear_tab.h"
#include "../dm_gear_live.h"
#include "../dm_sim_ui.h"
#include <stdatomic.h>
#include <time.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)
static double now_s(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static void nap(int ms) { struct timespec t = { ms / 1000, (ms % 1000) * 1000000L }; nanosleep(&t, NULL); }



static const dm_sim_data *g_d;
static atomic_int g_unpins, g_pins;
const dm_sim_data *dm_sim_pin(const char *set) { (void)set; atomic_fetch_add(&g_pins, 1); return NULL; }
void dm_sim_unpin(const dm_sim_data *d) { if (d == g_d) atomic_fetch_add(&g_unpins, 1); }

static atomic_int g_slow_log, g_in_log;
void dm_gear_tab_log(const char *line) {
    if (atomic_load(&g_slow_log)) { atomic_store(&g_in_log, 1); nap(300); atomic_store(&g_in_log, 0); }
    (void)line;
}

static int g_offered;
static dm_gear_snap *snap_of(const gstock *g, uint64_t rev, double hits) {
    dm_gear_snap *s = dm_gear_snap_new(g->p.ncand, g->p.nweapon);
    if (!s) abort();
    dm_gear_cand *cand = s->cand; dm_gear_weapon *weapon = s->weapon;
    s->p = g->p; s->p.cand = cand; s->p.weapon = weapon; s->p.d = g_d; s->tables = g_d; s->p.ready = 0; s->p.charms_fixed = 1;
    memcpy(cand, g->cand, sizeof *cand * (size_t)g->p.ncand); memcpy(weapon, g->weapon, sizeof *weapon * (size_t)g->p.nweapon);
    s->rev = rev; snprintf(s->set, sizeof s->set, "rotw"); s->goal.survival_hits = hits; s->goal.boss_share = 0.5;
    for (int k = 0; k < DM_GEAR_P_N; k++) snprintf(s->slot[k], sizeof s->slot[k], "place %d", k);
    static const char *const measure[DM_GEAR_M_N] = { "group", "single", "survival", "recovery" };
    for (int k = 0; k < DM_GEAR_M_N; k++) snprintf(s->measure[k], sizeof s->measure[k], "%s", measure[k]);
    snprintf(s->missed, sizeof s->missed, "missed %%d"); snprintf(s->reached, sizeof s->reached, "reached %%d");
    snprintf(s->below, sizeof s->below, "below %%s for %%s"); snprintf(s->below_same, sizeof s->below_same, "below %%s");
    for (int i = 0; i < g->p.ncand; i++) {
        snprintf(s->show[i].name, sizeof s->show[i].name, "item \"%d\"", i); s->show[i].where = g->cand[i].worn ? DM_GTAB_WORN : DM_GTAB_INV;
        s->show[i].quality = 7; s->show[i].colour = '4'; s->show[i].row = 100 + i; s->show[i].gfx = -1; s->show[i].id = 1000 + (uint32_t)i;
        if (!g->cand[i].worn && i % 2) s->show[i].where = DM_GTAB_STASH;
        s->show[i].page = s->show[i].where == DM_GTAB_STASH ? 3 : 0;
    }
    return s;
}

static void give(dm_gear_snap *s) { int tabled = s->tables != NULL; while (dm_gear_tab_offer(s)) nap(1); g_offered += tabled; }
static char g_text[65536];
static const char *text(void) { int n = dm_gear_tab_text(g_text, sizeof g_text); return n < 0 ? "" : g_text; }
static int has(const char *needle) { return strstr(text(), needle) != NULL; }
static int wait_for(const char *needle, double seconds) { double t0 = now_s(); while (now_s() - t0 < seconds) { if (has(needle)) return 1; nap(5); } return 0; }


static int rows_named(const char *t) {
    int rows = 0;
    for (const char *r = strstr(t, "{\"slot\":"); r; r = strstr(r + 1, "{\"slot\":")) {
        const char *end = strchr(r, '}'); if (!end) return -1;
        const char *w = strstr(r, "\"where\":"); if (!w || w > end || w[8] == '-') continue;
        const char *id = strstr(r, "\"id\":"), *x = strstr(r, "\"x\":"), *c = strstr(r, "\"c\":\"4\""), *pic = strstr(r, "\"row\":1");
        if (!id || id > end || (x && x < end) || !c || c > end || !pic || pic > end) return -1;
        if (w[8] == '2' && strncmp(w, "\"where\":2,\"page\":3,", 19)) return -1;

        const char *pl = strstr(r, "\"p\":"); int named = -1;
        if (!pl || pl > end || (sscanf(r, "{\"slot\":\"place %d\"", &named) == 1 && atoi(pl + 4) != named)) return -1;
        rows++;
    }
    return rows;
}

static void check_rings(void) {

    static const struct { int best[2], now[2], n, place[2], cand[2]; } cases[] = {
        { { 5, 7 }, { 5, 6 }, 1, { 1, 0 }, { 7, 0 } },
        { { 5, 7 }, { -1, -1 }, 2, { 0, 1 }, { 5, 7 } },
        { { 5, 7 }, { 6, -1 }, 2, { 0, 1 }, { 5, 7 } },
        { { -1, -1 }, { 5, 6 }, 2, { 0, 1 }, { -1, -1 } },
        { { 6, 5 }, { 5, 6 }, 0, { 0, 0 }, { 0, 0 } },
        { { -1, 5 }, { 5, -1 }, 0, { 0, 0 }, { 0, 0 } },
        { { 7, -1 }, { 5, 6 }, 2, { 0, 1 }, { 7, -1 } },
        { { 7, -1 }, { -1, 6 }, 1, { 1, 0 }, { 7, 0 } },
    };
    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        int place[2] = { 9, 9 }, cand[2] = { 9, 9 };
        int n = dm_gear_tab_ring_rows(cases[i].best, cases[i].now, place, cand), ok = n == cases[i].n;
        for (int j = 0; ok && j < n; j++) ok = place[j] == cases[i].place[j] && cand[j] == cases[i].cand[j];
        if (n == 2) ok &= place[0] != place[1];
        CHECK(ok, "ring rows case %zu: %d rows, %d:%d %d:%d", i, n, place[0], cand[0], place[1], cand[1]);
    }
}




static void test_grid(void) {
    static dm_gear_inv_cells inv[DM_GEAR_GRID_W * DM_GEAR_GRID_H]; int n = 0;
    for (int y = 0; y < DM_GEAR_GRID_H; y++) for (int x = 0; x < DM_GEAR_GRID_W; x++) if (x != DM_GEAR_GRID_W - 1 || y != DM_GEAR_GRID_H - 1) inv[n++] = (dm_gear_inv_cells){ x, y, 1, 1, 0, 0 };
    inv[n++] = (dm_gear_inv_cells){ DM_GEAR_GRID_W - 1, DM_GEAR_GRID_H - 1, 1, 1, 1, 0 };
    static dm_gear_problem p; static dm_gear_cand stash[1]; memset(&p, 0, sizeof p); memset(stash, 0, sizeof stash);
    stash[0].kind = DM_GEAR_K_CHARM; stash[0].w = stash[0].h = 1; p.cand = stash; p.ncand = 1;
    uint8_t take[1] = { 1 };
    dm_gear_grid(p.grid, inv, n);
    int taken = 0; for (int y = 0; y < DM_GEAR_GRID_H; y++) for (int x = 0; x < DM_GEAR_GRID_W; x++) taken += p.grid[y][x];
    CHECK(taken == DM_GEAR_GRID_W * DM_GEAR_GRID_H, "a charm the search does not place leaves its cell free (%d cells taken)", taken);
    CHECK(!dm_gear_place(&p, take), "a stash charm fits into the cell of a charm the search does not place");
    inv[n - 1].placed = 1;
    dm_gear_grid(p.grid, inv, n);
    CHECK(!p.grid[DM_GEAR_GRID_H - 1][DM_GEAR_GRID_W - 1] && dm_gear_place(&p, take), "a charm candidate's cell is not free for the search");

    dm_gear_inv_cells big[2] = { { 0, 0, 2, 3, 0, 0 }, { 9, 3, 2, 2, 0, 0 } };
    dm_gear_grid(p.grid, big, 2);
    CHECK(p.grid[2][1] && !p.grid[3][1] && p.grid[3][9] && !p.grid[0][9], "an item's cells are not marked as its size gives them");
}




typedef struct { int two, ring, charm, amu_new, amu_old; } pkg_stock;
static void pkg_build(gstock *g, const dm_gear_tables *gt, int need, int ring_str, int charm_str, int amulet, pkg_stock *k) {
    gspec sp; memset(&sp, 0, sizeof sp);
    sp.profile = gs_profile("class4-level80.prof"); sp.act = 2; sp.seed = 51; sp.variants = 1;
    if (gs_build(g, g_d, gt, &sp)) { fprintf(stderr, "package stock not built\n"); exit(2); }

    k->amu_old = -1;
    for (int i = 0; i < g->p.ncand; i++) {
        dm_gear_cand *c = &g->cand[i];
        if (!c->worn || c->weapon >= 0 || c->kind == DM_GEAR_K_SHIELD) continue;
        for (int j = 0; j < c->item.own.n; j++) if (c->item.own.v[j].stat == 0 || c->item.own.v[j].stat == 2) c->item.own.v[j].value = 0;
        if (c->kind == DM_GEAR_K_AMULET) { k->amu_old = i; if (amulet) dm_gear_list_add(&c->item.own, 0, 0, 5); }
    }
    if (dm_gear_problem_ready(&g->p)) { fprintf(stderr, "package stock not ready\n"); exit(2); }
    int str = g->p.nowc.value[0];
    dm_gear_cand *t = gs_item(g, DM_GEAR_K_WEAPON2, 3, 0); t->item.req_str = str + need;
    dm_gear_list_add(&t->item.own, 7, 0, 400 * 256); dm_gear_list_add(&t->item.own, 39, 0, 15);
    dm_gear_cand *r = gs_new(g, DM_GEAR_K_RING); dm_gear_list_add(&r->item.own, 0, 0, ring_str); dm_gear_list_add(&r->item.own, 39, 0, 10);
    dm_gear_cand *c = gs_new(g, DM_GEAR_K_CHARM); c->item.req_lvl = 1; dm_gear_list_add(&c->item.own, 0, 0, charm_str);
    k->two = (int)(t - g->cand); k->ring = (int)(r - g->cand); k->charm = (int)(c - g->cand); k->amu_new = -1;
    if (amulet) { dm_gear_cand *a = gs_new(g, DM_GEAR_K_AMULET); dm_gear_list_add(&a->item.own, 7, 0, 150 * 256); dm_gear_list_add(&a->item.own, 41, 0, 20); k->amu_new = (int)(a - g->cand); }
    g->p.charms_fixed = 0;
    if (dm_gear_problem_ready(&g->p)) { fprintf(stderr, "package stock not ready\n"); exit(2); }
}

static void pkg_best(const gstock *g, const pkg_stock *k, dm_gear_pick *best) {
    dm_gear_pick_now(&g->p, best);
    best->at[DM_GEAR_P_MAIN] = (int16_t)k->two; best->at[DM_GEAR_P_OFF] = -1; best->at[DM_GEAR_P_RING1] = (int16_t)k->ring; best->charm[k->charm] = 1;
    if (k->amu_new >= 0) best->at[DM_GEAR_P_AMULET] = (int16_t)k->amu_new;
}
static int row_of(const dm_gtab_row *rows, int n, int place, int cand) { for (int i = 0; i < n; i++) if (rows[i].place == place && rows[i].cand == cand) return i; return -1; }
static int charm_row(const dm_gtab_row *rows, int n, int cand) { for (int i = 0; i < n; i++) if (rows[i].place < 0 && rows[i].cand == cand) return i; return -1; }
static int units_n(const dm_gtab_units *s) { int c = 0; for (size_t k = 0; k < sizeof s->w / sizeof *s->w; k++) c += __builtin_popcountll(s->w[k]); return c; }
static void test_order(const dm_gear_tables *gt) {
    static gstock g; static dm_gear_pick now, best; pkg_stock k; static dm_gtab_row rows[DM_GTAB_ROWS];

    pkg_build(&g, gt, 6, 3, 3, 0, &k); dm_gear_pick_now(&g.p, &now); pkg_best(&g, &k, &best);
    int n = dm_gear_tab_rows(&g.p, &now, &best, rows, DM_GTAB_ROWS);
    int m0 = row_of(rows, n, DM_GEAR_P_MAIN, k.two), o0 = row_of(rows, n, DM_GEAR_P_OFF, -1);
    CHECK(m0 == 0 && o0 == 1 && rows[o0].unit == rows[m0].unit, "place order: the hands first, as one unit (%d %d)", m0, o0);
    CHECK(!dm_gear_tab_order(&g.p, &now, rows, n), "no order for ring + charm + two-hand weapon");
    int m = row_of(rows, n, DM_GEAR_P_MAIN, k.two), o = row_of(rows, n, DM_GEAR_P_OFF, -1), r = row_of(rows, n, DM_GEAR_P_RING1, k.ring), c = charm_row(rows, n, k.charm);
    CHECK(r >= 0 && c >= 0 && m > r && m > c && o == m + 1, "suppliers before the hands: ring %d charm %d main %d off %d", r, c, m, o);
    CHECK(r == 0 || c == 0, "a supplier is not the first row (ring %d, charm %d)", r, c);
    CHECK(m >= 0 && r >= 0 && c >= 0 && rows[r].first && rows[c].first && !rows[m].first, "first marks: ring %d charm %d hands %d", r >= 0 && rows[r].first, c >= 0 && rows[c].first, m >= 0 && rows[m].first);
    CHECK(m >= 0 && r >= 0 && c >= 0 && units_n(&rows[m].needs) == 2 && dm_gtab_has(&rows[m].needs, rows[r].unit) && dm_gtab_has(&rows[m].needs, rows[c].unit)
          && !memcmp(&rows[o].needs, &rows[m].needs, sizeof rows[m].needs), "the hands need %d units", m >= 0 ? units_n(&rows[m].needs) : -1);


    {
        static dm_gear_pick before, after; static dm_sim_char cb; static dm_gear_char comp; unsigned fl;
        CHECK(!dm_gear_tab_pkg_picks(&g.p, &now, rows, n, m, &before, &after), "no package picks for the hands");
        CHECK(after.at[DM_GEAR_P_MAIN] == k.two && after.at[DM_GEAR_P_OFF] == -1 && before.at[DM_GEAR_P_MAIN] == now.at[DM_GEAR_P_MAIN] && before.at[DM_GEAR_P_OFF] == now.at[DM_GEAR_P_OFF],
              "hands of the package: before %d/%d after %d/%d", before.at[DM_GEAR_P_MAIN], before.at[DM_GEAR_P_OFF], after.at[DM_GEAR_P_MAIN], after.at[DM_GEAR_P_OFF]);
        CHECK(before.at[DM_GEAR_P_RING1] == k.ring && after.at[DM_GEAR_P_RING1] == k.ring && before.charm[k.charm] && after.charm[k.charm], "the suppliers are not in both picks");
        CHECK(!dm_gear_leaf_char(&g.p, &before, &cb, &comp, &fl) && !dm_gear_leaf_char(&g.p, &after, &cb, &comp, &fl), "a package pick does not measure (%#x)", fl);
        static dm_gear_pick alone; alone = now; alone.at[DM_GEAR_P_MAIN] = (int16_t)k.two; alone.at[DM_GEAR_P_OFF] = -1;
        CHECK(dm_gear_leaf_char(&g.p, &alone, &cb, &comp, &fl) && (fl & DM_GEAR_L_REQ), "the weapon works without the suppliers (%#x)", fl);
        CHECK(!dm_gear_tab_pkg_picks(&g.p, &now, rows, n, r, &before, &after) && after.at[DM_GEAR_P_MAIN] == now.at[DM_GEAR_P_MAIN] && after.at[DM_GEAR_P_RING1] == k.ring
              && before.at[DM_GEAR_P_RING1] == now.at[DM_GEAR_P_RING1] && before.charm[k.charm], "the ring's package keeps the hands that need it");
        CHECK(dm_gear_tab_pkg_picks(&g.p, &now, rows, n, c, &before, &after) == -1, "a charm row has a package");
    }

    pkg_build(&g, gt, 6, 6, 3, 0, &k); dm_gear_pick_now(&g.p, &now); pkg_best(&g, &k, &best);
    n = dm_gear_tab_rows(&g.p, &now, &best, rows, DM_GTAB_ROWS);
    CHECK(!dm_gear_tab_order(&g.p, &now, rows, n), "no order for the ring alone");
    m = row_of(rows, n, DM_GEAR_P_MAIN, k.two); r = row_of(rows, n, DM_GEAR_P_RING1, k.ring); c = charm_row(rows, n, k.charm);
    CHECK(r == 0 && rows[r].first && !rows[c].first && units_n(&rows[m].needs) == 1 && dm_gtab_has(&rows[m].needs, rows[r].unit), "ring alone: ring %d first %d, charm first %d, needs %d units",
          r, rows[r].first, rows[c].first, units_n(&rows[m].needs));

    pkg_build(&g, gt, 6, 6, 5, 1, &k); dm_gear_pick_now(&g.p, &now); pkg_best(&g, &k, &best);
    n = dm_gear_tab_rows(&g.p, &now, &best, rows, DM_GTAB_ROWS);
    CHECK(!dm_gear_tab_order(&g.p, &now, rows, n), "no order with the amulet that lowers strength");
    m = row_of(rows, n, DM_GEAR_P_MAIN, k.two); r = row_of(rows, n, DM_GEAR_P_RING1, k.ring); int a = row_of(rows, n, DM_GEAR_P_AMULET, k.amu_new);
    CHECK(r >= 0 && m > r && a > m && rows[r].first, "amulet that lowers strength: ring %d, hands %d, amulet %d", r, m, a);
    {
        static dm_gear_pick st, t; static dm_gear_char comp; uint8_t act[DM_GEAR_P_N + 1]; int ok = 1;
        st = now;
        for (int i = 0; i < n; i++) {
            if (rows[i].place >= 0) st.at[rows[i].place] = (int16_t)rows[i].cand; else st.charm[rows[i].cand] = rows[i].charm == 1;
            if (i + 1 < n && rows[i + 1].unit == rows[i].unit) continue;
            t = st; dm_gear_pick_compose(&g.p, &t, &comp, act);
            for (int j = 0; j <= i; j++) if (rows[j].unit == rows[i].unit && rows[j].place >= 0 && rows[j].cand >= 0) ok &= act[rows[j].place];
        }
        CHECK(ok, "an item does not work when it is put on in the order shown");
    }

    pkg_build(&g, gt, 40, 3, 3, 0, &k); dm_gear_pick_now(&g.p, &now); pkg_best(&g, &k, &best);
    n = dm_gear_tab_rows(&g.p, &now, &best, rows, DM_GTAB_ROWS);
    CHECK(dm_gear_tab_order(&g.p, &now, rows, n) == -1 && rows[0].place == DM_GEAR_P_MAIN && !rows[0].first && !units_n(&rows[0].needs), "a result that breaks a requirement was ordered");
}


static void test_package_text(const dm_gear_tables *gt) {
    static gstock g; static dm_gear_pick now; pkg_stock k;
    pkg_build(&g, gt, 6, 3, 3, 0, &k); dm_gear_pick_now(&g.p, &now);
    dm_gear_snap *s = snap_of(&g, 0x20, 15); s->p.charms_fixed = 0;
    snprintf(s->show[k.ring].name, sizeof s->show[k.ring].name, "Ring of Strength"); snprintf(s->show[k.charm].name, sizeof s->show[k.charm].name, "Large Charm of Strength");
    give(s);
    CHECK(wait_for("\"rev\":\"0000000000000020\",\"status\":\"done\"", 300), "the package snapshot: %s", text());
    dm_gear_result r; CHECK(dm_gear_tab_best(&r) && r.best.at[DM_GEAR_P_MAIN] == k.two && r.best.charm[k.charm], "the search does not take the weapon and the charm: %s", text());
    const char *t = text(), *ring = strstr(t, "\"name\":\"Ring of Strength\""), *charm = strstr(t, "\"name\":\"Large Charm of Strength\"");
    char two[64]; snprintf(two, sizeof two, "\"name\":\"item \\\"%d\\\"\"", k.two);
    const char *hands = strstr(t, two);
    CHECK(ring && charm && hands && ring < hands && charm < hands, "suppliers after the hands in the text: %s", t);
    CHECK(ring && strstr(t, "\"first\":1") && strstr(t, "\"first\":1") < hands, "no first mark before the hands: %s", t);
    {
        const char *open = hands; while (open > t && *open != '{') open--; const char *end = strchr(hands, '}');
        const char *f = strstr(open, "\"first\""); CHECK(!f || f > end, "the hands row carries the first mark: %s", t);
    }
    dm_gtab_pkg pk; memset(&pk, 0, sizeof pk);
    uint32_t id = s->show[k.two].id;
    CHECK(dm_gear_tab_tip_pkg(id, DM_GEAR_P_MAIN, &pk) == 0, "a package before the row's tooltip was asked for");
    dm_gear_tab_tip_ask(id, DM_GEAR_P_MAIN); text();
    CHECK(dm_gear_tab_tip_pkg(id, DM_GEAR_P_MAIN, &pk) == 1 && pk.ok, "no package for the hands row");
    CHECK(dm_gear_tab_tip_pkg(id, DM_GEAR_P_OFF, &pk) == 0, "a package for another place");
    dm_gear_tab_tip_pkg(id, DM_GEAR_P_MAIN, &pk);
    CHECK(!strcmp(pk.with, "Ring of Strength + Large Charm of Strength") || !strcmp(pk.with, "Large Charm of Strength + Ring of Strength"), "with: \"%s\"", pk.with);

    CHECK(pk.before[IC_S_STR] == g.p.now.stat_value[0] + 6 && pk.after[IC_S_STR] == pk.before[IC_S_STR], "strength before %d after %d (now %d)", pk.before[IC_S_STR],
          pk.after[IC_S_STR], g.p.now.stat_value[0]);
    CHECK(pk.cb && pk.cb_after[IC_C_DMG] != pk.cb_before[IC_C_DMG], "combat values: known %d, damage %d -> %d", pk.cb, pk.cb_before[IC_C_DMG], pk.cb_after[IC_C_DMG]);

    {
        static dm_gtab_row rows[DM_GTAB_ROWS]; static dm_gear_pick before, after; static dm_sim_char cb, ca; static dm_gear_char gb, ga; unsigned fl; dm_gear_tip tip;
        int n = dm_gear_tab_rows(&g.p, &now, &r.best, rows, DM_GTAB_ROWS); dm_gear_tab_order(&g.p, &now, rows, n);
        dm_gear_tab_pkg_picks(&g.p, &now, rows, n, row_of(rows, n, DM_GEAR_P_MAIN, k.two), &before, &after);
        dm_gear_leaf_char(&g.p, &before, &cb, &gb, &fl); dm_gear_leaf_char(&g.p, &after, &ca, &ga, &fl);
        CHECK(pk.after[IC_S_FIRE] - pk.before[IC_S_FIRE] == ga.value[39] - gb.value[39] && ga.value[39] != gb.value[39], "fire resistance %d -> %d against the picks' %d -> %d",
              pk.before[IC_S_FIRE], pk.after[IC_S_FIRE], gb.value[39], ga.value[39]);
        CHECK(!dm_gear_tip_rate(g_d, &cb, &ca, &s->goal, 0, &tip) && pk.rated == DM_GEAR_TIP_DONE && !memcmp(&tip.score, &pk.tip.score, sizeof tip.score),
              "the package's rating (%d) differs from the search's measure of its picks", pk.rated);
        CHECK(tip.score.change[DM_GEAR_M_SURVIVAL] != 0 || tip.score.change[DM_GEAR_M_SINGLE] != 0, "the weapon changes no measure within the result");
    }

    int rp = r.best.at[DM_GEAR_P_RING2] == k.ring && now.at[DM_GEAR_P_RING2] >= 0 ? DM_GEAR_P_RING2 : DM_GEAR_P_RING1;
    {
        static dm_gtab_row rows[DM_GTAB_ROWS]; int n = dm_gear_tab_rows(&g.p, &now, &r.best, rows, DM_GTAB_ROWS);
        for (int i = 0; i < n; i++) if (rows[i].cand == k.ring && rows[i].place >= 0) rp = rows[i].place;
    }
    dm_gear_tab_tip_ask(s->show[k.ring].id, rp); text();
    CHECK(dm_gear_tab_tip_pkg(s->show[k.ring].id, rp, &pk) == 1 && pk.ok && !pk.with[0], "the ring's package: ok %d with \"%s\"", pk.ok, pk.with);
    dm_gear_tab_tip_ask(0, -1);
    printf("gear tab: package rows in put on order, first marks, package values and rating\n");
}




static void many_build(gstock *g, const dm_gear_tables *gt, int *two, int *supplier, int *first_charm) {
    gspec sp; memset(&sp, 0, sizeof sp);
    sp.profile = gs_profile("class4-level80.prof"); sp.act = 2; sp.seed = 61; sp.variants = 1;
    if (gs_build(g, g_d, gt, &sp)) { fprintf(stderr, "charm stock not built\n"); exit(2); }
    *first_charm = g->p.ncand;
    for (int i = 0; i < DM_GEAR_GRID_W * DM_GEAR_GRID_H; i++) {
        dm_gear_cand *c = gs_new(g, DM_GEAR_K_CHARM); c->item.req_lvl = 1; c->inv = 1; c->x = i % DM_GEAR_GRID_W; c->y = i / DM_GEAR_GRID_W;
        dm_gear_list_add(&c->item.own, 79, 0, 5);
    }
    for (int i = 0; i < DM_GEAR_GRID_W * DM_GEAR_GRID_H; i++) {
        dm_gear_cand *c = gs_new(g, DM_GEAR_K_CHARM); c->item.req_lvl = 1;
        dm_gear_list_add(&c->item.own, 7, 0, 20 * 256);
    }
    *supplier = g->p.ncand - 1;
    dm_gear_list_add(&g->cand[*supplier].item.own, 0, 0, 30);
    if (dm_gear_problem_ready(&g->p)) { fprintf(stderr, "charm stock not ready\n"); exit(2); }
    dm_gear_cand *t = gs_item(g, DM_GEAR_K_WEAPON2, 3, 0); t->item.req_str = g->p.nowc.value[0] + 20;
    *two = (int)(t - g->cand);
    g->p.charms_fixed = 0;
    if (dm_gear_problem_ready(&g->p)) { fprintf(stderr, "charm stock not ready\n"); exit(2); }
}
static void test_many_charms(const dm_gear_tables *gt) {
    static gstock g; static dm_gear_pick now, best; static dm_gtab_row rows[DM_GTAB_ROWS];
    int two, supplier, c0, cells = DM_GEAR_GRID_W * DM_GEAR_GRID_H;
    many_build(&g, gt, &two, &supplier, &c0);

    dm_gear_pick_now(&g.p, &now); best = now;
    best.at[DM_GEAR_P_MAIN] = (int16_t)two; best.at[DM_GEAR_P_OFF] = -1;
    for (int i = 0; i < 2 * cells; i++) best.charm[c0 + i] = i >= cells;
    int hands = (now.at[DM_GEAR_P_OFF] >= 0) + 1;
    int n = dm_gear_tab_rows(&g.p, &now, &best, rows, DM_GTAB_ROWS);
    CHECK(n == hands + 2 * cells && n <= DM_GTAB_ROWS, "%d rows for %d hand rows and %d charms", n, hands, 2 * cells);
    int missing = 0;
    for (int i = 0; i < 2 * cells; i++) { int c = charm_row(rows, n, c0 + i); missing += c < 0 || rows[c].charm != (i >= cells ? 1 : 2); }
    CHECK(!missing, "%d charms without their row", missing);
    int s = charm_row(rows, n, supplier);
    CHECK(s >= 64 && rows[s].unit == s, "the strength charm's unit %d is not past 64", s >= 0 ? rows[s].unit : -1);
    int su = s >= 0 ? rows[s].unit : 0;
    CHECK(!dm_gear_tab_order(&g.p, &now, rows, n), "no order for the charm heavy result");
    int m = row_of(rows, n, DM_GEAR_P_MAIN, two); s = charm_row(rows, n, supplier);
    CHECK(m >= 0 && s >= 0 && s < m && rows[s].first && units_n(&rows[m].needs) == 1 && dm_gtab_has(&rows[m].needs, su),
          "the weapon after its strength charm: charm row %d (first %d), weapon row %d needs %d units", s, s >= 0 ? rows[s].first : -1, m, m >= 0 ? units_n(&rows[m].needs) : -1);
    int firsts = 0; for (int i = 0; i < n; i++) firsts += rows[i].first;
    CHECK(firsts == 1, "%d rows marked first", firsts);
    {
        static dm_gear_pick before, after;
        CHECK(m >= 0 && !dm_gear_tab_pkg_picks(&g.p, &now, rows, n, m, &before, &after) && before.charm[supplier] && after.charm[supplier] && after.at[DM_GEAR_P_MAIN] == two
              && before.at[DM_GEAR_P_MAIN] == now.at[DM_GEAR_P_MAIN], "the weapon's package without its strength charm");
    }

    dm_gear_snap *sn = snap_of(&g, 0x30, 15); sn->p.charms_fixed = 0; give(sn);
    CHECK(wait_for("\"rev\":\"0000000000000030\",\"status\":\"done\"", 300), "the charm heavy snapshot: %s", text());
    dm_gear_result r; int changed = 0, named = 0;
    CHECK(dm_gear_tab_best(&r), "no result for the charm heavy stock");
    const char *t = text();
    for (int i = 0; i < g.p.ncand; i++) {
        if (g.cand[i].kind != DM_GEAR_K_CHARM || !r.best.charm[i] == !now.charm[i]) continue;

        char want[96], marked[112];
        snprintf(want, sizeof want, "{\"slot\":\"\",\"charm\":%d,\"name\":\"item \\\"%d\\\"\"", r.best.charm[i] ? 1 : 2, i);
        snprintf(marked, sizeof marked, "{\"slot\":\"\",\"charm\":%d,\"first\":1,\"name\":\"item \\\"%d\\\"\"", r.best.charm[i] ? 1 : 2, i);
        changed++; named += strstr(t, want) || strstr(t, marked);
    }
    CHECK(changed > 48 && named == changed, "%d of %d changed charms have their row in the text", named, changed);
    printf("gear tab: %d rows ordered for a charm heavy result, %d charm rows in the text\n", n, named);
}

int main(void) {
    test_grid();
    static dm_gear_tables gt; g_d = gs_tables(&gt);
    char *prof = gs_profile("class4-level80.prof");
    static gstock g;
    gspec spec; memset(&spec, 0, sizeof spec);
    spec.profile = prof; spec.act = 2; spec.seed = 31; spec.variants = 1;
    spec.count[DM_GEAR_K_HEAD] = 2; spec.count[DM_GEAR_K_RING] = 3; spec.count[DM_GEAR_K_AMULET] = 2; spec.count[DM_GEAR_K_WEAPON1] = 2;
    spec.count[DM_GEAR_K_SHIELD] = 1; spec.charms_inv = 2; spec.charms_stash = 2;
    if (gs_build(&g, g_d, &gt, &spec)) { fprintf(stderr, "stock not built\n"); return 2; }
    g.p.charms_fixed = 1; dm_gear_problem_ready(&g.p);
    dm_gear_goal goal = { 15, 0.5 };
    brute_out *b = malloc(sizeof *b); if (!b || gear_brute(&g.p, &goal, 1, b)) { fprintf(stderr, "no enumerator result\n"); return 2; }
    check_rings();


    give(snap_of(&g, 11, 15));
    nap(50);
    CHECK(dm_gear_tab_runs() == 0, "a search ran with the tab closed (%ld runs)", dm_gear_tab_runs());
    dm_gear_tab_want(1);
    double t0 = now_s();
    CHECK(wait_for("\"status\":\"done\"", 120), "the search did not finish: %s", text());
    double took = now_s() - t0;
    CHECK(has("\"rev\":\"000000000000000b\"") && has("\"state\":\"ok\"") && has("\"charms\":0"), "text: %s", text());
    CHECK(atomic_load(&g_pins) == 0, "the tab pinned tables itself (%d times) instead of searching on the snapshot's", atomic_load(&g_pins));

    {
        dm_gear_search *x = dm_gear_search_new(&g.p); dm_gear_result r; dm_gear_search_run(x, &goal, NULL, NULL, &r); dm_gear_search_free(x);
        CHECK(!dm_gear_key_cmp(r.score.key, b->key[0]), "search (%g %g %g) against enumerator (%g %g %g)", r.score.key[0], r.score.key[1], r.score.key[2],
              b->key[0][0], b->key[0][1], b->key[0][2]);
        dm_gear_result shown; CHECK(dm_gear_tab_best(&shown) && !dm_gear_key_cmp(shown.score.key, b->key[0]), "the tab shows another result than the optimum");
        int changes = 0; dm_gear_pick now; dm_gear_pick_now(&g.p, &now);
        for (int k = 0; k < DM_GEAR_P_N; k++) changes += r.best.at[k] != now.at[k];
        CHECK(changes ? has("\"rows\":[{\"slot\":\"place ") && has("\\\"") : has("\"rows\":[]"), "rows for %d changes: %s", changes, text());
        CHECK(!changes || has("\"totals\":[{"), "no totals for %d changes: %s", changes, text());
        const double v[3][2] = { { r.now.group, r.after.group }, { r.now.single, r.after.single }, { r.now.survival_s, r.after.survival_s } };
        static const char *const word[3] = { "group", "single", "survival" };
        for (int k = 0; changes && k < 3; k++) {
            double pct = v[k][0] > 0 ? 100 * (v[k][1] - v[k][0]) / v[k][0] : 0; char want[64], p[16];
            snprintf(p, sizeof p, "%+.1f", pct);
            if (!strcmp(p + 1, "0.0") || (k < 2 && !r.score.damage_base)) continue;
            snprintf(want, sizeof want, "{\"c\":\"%c\",\"t\":\"%s %s %%\"}", pct > 0 ? '3' : '1', word[k], p);
            CHECK(has(want), "totals lack %s: %s", want, text());
        }
        CHECK(rows_named(text()) >= 0, "a row lacks the item's id, picture or colour, or names a cell: %s", text());
    }
    long done_runs = dm_gear_tab_runs();

    give(snap_of(&g, 11, 15));
    nap(50);
    CHECK(has("\"status\":\"done\"") && dm_gear_tab_runs() == done_runs, "an equal revision restarted the search: %s", text());


    {
        dm_gear_result r0, r1; dm_gear_tab_best(&r0);
        give(snap_of(&g, 21, 15));
        CHECK(wait_for("\"rev\":\"0000000000000015\",\"status\":\"done\"", 5) && dm_gear_tab_runs() == done_runs, "the same problem was searched again (%ld runs): %s",
              dm_gear_tab_runs() - done_runs, text());
        CHECK(dm_gear_tab_best(&r1) && !memcmp(&r1.best, &r0.best, sizeof r0.best), "the kept result is not the one found before");
        dm_gear_snap *c = snap_of(&g, 22, 15); int j = -1;
        for (int i = 0; i < g.p.ncand && j < 0; i++) {
            int in = 0; for (int k = 0; k < DM_GEAR_P_N; k++) in |= r0.best.at[k] == i;
            if (!g.cand[i].worn && g.cand[i].kind != DM_GEAR_K_CHARM && !in) j = i;
        }
        CHECK(j >= 0, "no item to change");
        if (j >= 0) c->cand[j].item.req_lvl = 999;
        give(c);
        CHECK(wait_for("\"rev\":\"0000000000000016\",\"status\":\"done\"", 120) && dm_gear_tab_runs() > done_runs, "a changed stock was not searched: %s", text());
        long changed = dm_gear_tab_runs();
        give(snap_of(&g, 23, 15));
        CHECK(wait_for("\"rev\":\"0000000000000017\",\"status\":\"done\"", 5) && dm_gear_tab_runs() == changed, "the stock back was searched again: %s", text());
        CHECK(dm_gear_tab_best(&r1) && !memcmp(&r1.best, &r0.best, sizeof r0.best), "the kept result is not the one found before the change");
    }

    {
        CHECK(!has("\"tip\":"), "a tooltip without asking: %s", text());
        int place = -2;
        dm_gear_tab_tip_ask(1003, DM_GEAR_P_RING2);
        CHECK(dm_gear_tab_tip_wanted(&place) == 1003 && place == DM_GEAR_P_RING2, "the game thread does not see the id and place asked for (%d)", place);
        CHECK(!has("\"tip\":"), "a tooltip before the game thread built it: %s", text());
        CHECK(!dm_gear_tab_tip_put(1003, DM_GEAR_P_RING2, "line \"b\"\nname", "3better", 2), "an idle tooltip mutex refused the text");
        char want[160]; snprintf(want, sizeof want, "\"tip\":{\"id\":1003,\"p\":%d,\"dots\":2,\"t\":\"line \\\"b\\\"\\u000aname\",\"b\":\"3better\"}", DM_GEAR_P_RING2);
        CHECK(has(want), "the tooltip is not in the text: %s", text());

        dm_gear_tab_tip_ask(1003, DM_GEAR_P_RING1);
        CHECK(!has("\"tip\":"), "the tooltip of another place is shown: %s", text());
        dm_gear_tab_tip_ask(1004, DM_GEAR_P_RING2);
        CHECK(!has("\"tip\":"), "the tooltip of another item is shown: %s", text());
        dm_gear_tab_tip_ask(1005, -1);
        CHECK(dm_gear_tab_tip_wanted(&place) == 1005 && place == -1, "a charm row's tooltip has a place (%d)", place);
        dm_gear_tab_tip_ask(0, -1);
        CHECK(!has("\"tip\":") && dm_gear_tab_tip_wanted(&place) == 0, "a tooltip after the app let go of it: %s", text());
    }

    give(snap_of(&g, 12, 1e9));
    CHECK(wait_for("\"rev\":\"000000000000000c\",\"status\":\"searching\"", 5), "the new revision is not searched: %s", text());

    dm_gear_result before; memset(&before, 0, sizeof before);
    {
        dm_gear_pick now; dm_gear_pick_now(&g.p, &now); double t1 = now_s(); int better = 0;
        while (!better && now_s() - t1 < 30) { if (dm_gear_tab_best(&before)) for (int k = 0; k < DM_GEAR_P_N; k++) better |= before.best.at[k] != now.at[k]; if (!better) nap(2); }
        CHECK(better, "no better result while searching: %s", text());
    }
    dm_gear_tab_want(0);
    CHECK(wait_for("\"status\":\"stopped\"", 2), "closing did not stop the search: %s", text());
    long stopped = dm_gear_tab_runs();
    dm_gear_tab_best(&before);
    nap(300);
    CHECK(dm_gear_tab_runs() == stopped, "runs after the tab closed: %ld -> %ld", stopped, dm_gear_tab_runs());

    {
        long starts = dm_gear_tab_starts(); dm_gear_tab_want(1);
        double t1 = now_s(); while (dm_gear_tab_starts() == starts && now_s() - t1 < 5) nap(0);
        dm_gear_tab_want(0);
        CHECK(wait_for("\"status\":\"stopped\"", 2) || has("\"status\":\"done\""), "closing again did not stop the search: %s", text());
        dm_gear_result r; CHECK(dm_gear_tab_best(&r) && dm_gear_key_cmp(r.score.key, before.score.key) >= 0, "a short reopening fell back to a worse result");
        dm_gear_tab_best(&before);
    }

    dm_gear_tab_want(1);
    {
        double t1 = now_s(); int worse = 0, missing = 0;
        while (now_s() - t1 < 0.5) { dm_gear_result r; if (!dm_gear_tab_best(&r)) missing++; else if (dm_gear_key_cmp(r.score.key, before.score.key) < 0) worse++; nap(1); }
        CHECK(!worse && !missing, "after reopening the shown result fell back %d times and was missing %d times", worse, missing);
    }
    CHECK(wait_for("\"rev\":\"000000000000000c\",\"status\":\"done\"", 300), "the stopped search did not finish after reopening: %s", text());

    {
        atomic_store(&g_slow_log, 1);
        give(snap_of(&g, 13, 15));
        double t1 = now_s(); while (!atomic_load(&g_in_log) && now_s() - t1 < 120) nap(1);
        CHECK(atomic_load(&g_in_log), "the search did not end");
        double t2 = now_s(); int busy = 0;
        dm_gear_snap *s = snap_of(&g, 14, 15);
        for (int i = 0; i < 100; i++) { busy += !dm_gear_tab_wanted(); }
        int refused = dm_gear_tab_offer(s); if (!refused) g_offered++;
        text();
        double waited = now_s() - t2;
        CHECK(atomic_load(&g_in_log) && waited < 0.05 && !busy && !refused, "the game thread waited %.0f ms on the search thread's log line (refused %d)", waited * 1e3, refused);
        if (refused) dm_gear_snap_free(s);
        atomic_store(&g_slow_log, 0);
        CHECK(wait_for("\"rev\":\"000000000000000e\",\"status\":\"done\"", 120), "the snapshot offered during the log line was not searched: %s", text());
    }

    {
        dm_gear_snap *s = snap_of(&g, 15, 15); s->cand[0].kind = DM_GEAR_K_NONE; give(s);
        CHECK(wait_for("\"rev\":\"000000000000000f\",\"status\":\"failed\",\"state\":\"ok\"", 5), "a search that cannot start is not shown as failed: %s", text());
        long starts = dm_gear_tab_starts();
        nap(50);
        CHECK(dm_gear_tab_starts() == starts, "a failed search was tried again while the tab stayed open");
        dm_gear_tab_want(0); dm_gear_tab_want(1);
        double t1 = now_s(); while (dm_gear_tab_starts() == starts && now_s() - t1 < 5) nap(1);
        CHECK(dm_gear_tab_starts() == starts + 1 && wait_for("\"status\":\"failed\"", 5), "reopening did not try the failed search again: %ld starts", dm_gear_tab_starts() - starts);
    }

    {
        dm_gear_snap *s = snap_of(&g, 16, 15); s->p.charms_fixed = 0; give(s);
        CHECK(wait_for("\"rev\":\"0000000000000010\",\"status\":\"done\"", 300) && has("\"charms\":1"), "the searched charms' snapshot: %s", text());
        dm_gear_result r; dm_gear_pick now; dm_gear_pick_now(&g.p, &now); int charms = 0;
        CHECK(dm_gear_tab_best(&r), "no result with the charms searched");
        for (int i = 0; i < g.p.ncand; i++) {
            if (g.cand[i].kind != DM_GEAR_K_CHARM || !r.best.charm[i] == !now.charm[i]) continue;
            char want[96]; snprintf(want, sizeof want, "{\"slot\":\"\",\"charm\":%d,\"name\":\"item \\\"%d\\\"\"", r.best.charm[i] ? 1 : 2, i); charms++;
            CHECK(has(want), "no row %s: %s", want, text());
        }
        CHECK(!has("\"charm\":") || charms, "charm rows without a charm change: %s", text());
        printf("gear tab: %d charm rows with the charms searched\n", charms);
    }

    {
        dm_gear_snap *s = snap_of(&g, 17, 15); s->p.charms_fixed = 0; int idle = -1;
        for (int i = 0; i < g.p.ncand && idle < 0; i++) if (g.cand[i].kind == DM_GEAR_K_CHARM && g.cand[i].inv && !g.cand[i].unrated) idle = i;
        CHECK(idle >= 0, "no inventory charm in the stock");
        if (idle >= 0) { dm_gear_list *own = &s->cand[idle].item.own; own->n = 0; own->full = 0; dm_gear_list_add(own, 79, 0, 50); }
        give(s);
        CHECK(wait_for("\"rev\":\"0000000000000011\",\"status\":\"done\"", 300), "the idle charm's snapshot: %s", text());
        char drop[96]; snprintf(drop, sizeof drop, "\"charm\":2,\"name\":\"item \\\"%d\\\"\"", idle);
        CHECK(idle < 0 || !has(drop), "a row drops the charm that changes nothing: %s", text());
    }


    {
        static gstock g2;
        gspec s3; memset(&s3, 0, sizeof s3);
        s3.profile = "class=1\nlevel=80\ndiff=2\nlife=1200\nmana=50000\nstr=60\ndex=60\nvit=80\nene=200\nar=500\ndef=800\nfcr=63\n"
                     "weapon=1,2,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\nskill=59,20,20\nbar=59\nskill=45,20,20\nbar=45\n";
        s3.act = 2; s3.seed = 41; s3.variants = 1; s3.count[DM_GEAR_K_RING] = 1;
        CHECK(!gs_build(&g2, g_d, &gt, &s3), "the idle penalty stock is not built");
        dm_gear_cand *armor = gs_item(&g2, DM_GEAR_K_ARMOR, 1, 0); armor->item.own.n = 0; armor->item.req_str = 0;
        dm_gear_list_add(&armor->item.own, 31, 0, 3000); dm_gear_list_add(&armor->item.own, 7, 0, 300 * 256);
        dm_gear_cand *charm = gs_new(&g2, DM_GEAR_K_CHARM); charm->item.req_lvl = 1; charm->inv = 1; charm->x = 0; charm->y = 0;
        dm_gear_list_add(&charm->item.own, 0, 0, -10);
        int ia = (int)(armor - g2.cand), ic = (int)(charm - g2.cand);
        g2.p.charms_fixed = 0;

        dm_gear_pick now, without, swap; unsigned flags; static dm_sim_char ch; static dm_gear_char comp;
        CHECK(!dm_gear_problem_ready(&g2.p), "the idle penalty stock is not ready");
        dm_gear_pick_now(&g2.p, &now); without = now; without.charm[ic] = 0; swap = without; swap.at[DM_GEAR_P_ARMOR] = (int16_t)ia;
        dm_gear_leaf_char(&g2.p, &swap, &ch, &comp, &flags);
        armor->item.req_str = (int)comp.value[0] - 5;
        CHECK(!dm_gear_problem_ready(&g2.p), "the idle penalty stock is not ready");
        dm_gear_measures a, b;
        int idle_now = !dm_gear_leaf_measure(&g2.p, &now, &a, &flags) && !dm_gear_leaf_measure(&g2.p, &without, &b, &flags) && !memcmp(&a, &b, sizeof a);
        CHECK(idle_now, "the strength penalty charm is not idle with the gear worn now (%g/%g %g/%g %g/%g)", a.group, b.group, a.single, b.single, a.hits, b.hits);
        swap.charm[ic] = 1;
        CHECK(dm_gear_leaf_char(&g2.p, &swap, &ch, NULL, &flags) && (flags & DM_GEAR_L_REQ), "the armor still works with the charm (%#x)", flags);
        dm_gear_snap *s = snap_of(&g2, 18, 15); s->p.charms_fixed = 0; give(s);
        CHECK(wait_for("\"rev\":\"0000000000000012\",\"status\":\"done\"", 300), "the idle penalty snapshot: %s", text());
        dm_gear_result r; dm_gear_measures m;
        CHECK(dm_gear_tab_best(&r) && r.best.at[DM_GEAR_P_ARMOR] == ia, "the result does not take the better armor: %s", text());
        CHECK(!r.best.charm[ic], "the charm that takes the armor's strength went back into the result");
        CHECK(!dm_gear_leaf_measure(&g2.p, &r.best, &m, &flags) && !memcmp(&m, &r.after, sizeof m), "the pick shown does not measure the values shown (%#x)", flags);
    }
    test_order(&gt);
    test_package_text(&gt);
    test_many_charms(&gt);

    dm_gear_snap *s2 = dm_gear_snap_new(0, 1); s2->rev = 2; s2->state = DM_GTAB_SET2; give(s2);
    CHECK(wait_for("\"state\":\"set2\"", 5) && !has("\"rows\""), "set II state: %s", text());
    dm_gear_tab_want(0);

    {
        double t1 = now_s(); while (atomic_load(&g_unpins) < g_offered && now_s() - t1 < 5) nap(5);
        nap(20);
        CHECK(atomic_load(&g_unpins) == g_offered, "unpinned %d times for %d snapshots with tables", atomic_load(&g_unpins), g_offered);
    }
    printf("gear tab: first search %.2f s, restart, stop on close, reopen without falling back, failure and retry, no wait on the log, ring places, "
           "item names and tooltip, searched charms, snapshot tables; %s\n", took, failures ? "FAILED" : "passed");
    free(b);
    return failures != 0;
}
