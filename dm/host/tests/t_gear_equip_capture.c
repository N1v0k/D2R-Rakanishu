













#include "t_gear_stock.h"
#include "../dm_gear_equip.h"
#include "../dm_gear_equip_rules.h"
#include "../dm_gear_pages.h"
#include "../dm_itemjob.h"
#include "../dm_feats.h"
#include <sys/mman.h>

static int checks, fails; static const char *g_case;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s line %d: %s\n", g_case, __LINE__, #c); } } while (0)


int dm_pages_shown(void) { return 0; }
int dm_pages_slot_n(int page) { (void)page; return 0; }
int dm_pages_slot_item(int page, int i, const uint8_t **bits, int *len, int *x, int *y) { (void)page; (void)i; (void)bits; (void)len; (void)x; (void)y; return -1; }
unsigned dm_pages_flips(void) { return 0; }
int dm_pages_settled(uint64_t *commit) { *commit = 0; return 1; }
void dm_pages_job_flip(int dir) { (void)dir; }
uint64_t dm_gear_pages_bits_vhash(const uint8_t *bits, int len) { (void)bits; (void)len; return 0; }
int dm_gear_pages_bits_size(uint8_t *b, const uint8_t *bits, int len, int *w, int *h) { (void)b; (void)bits; (void)len; (void)w; (void)h; return -1; }
int dm_gear_pages_tab_ids(uint32_t id[DM_GPG_TABS]) { (void)id; return 0; }

int dm_gear_pages_unit_vhash(uint8_t *b, uint32_t id, uint64_t *hash) { (void)b; *hash = (uint64_t)id * 0x9e3779b97f4a7c15ull ^ 0x5bd1e995ull; return 0; }
static int g_owner;
int dm_itemjob_acquire(int kind) { if (g_owner) return 0; g_owner = kind; return 1; }
int dm_itemjob_release(int kind) { if (g_owner != kind) return 0; g_owner = 0; return 1; }
int dm_itemjob_owner(void) { return g_owner; }
unsigned dm_itemjob_ends(void) { return 0; }
void dm_gear_tab_jobs(unsigned ends) { (void)ends; }
static unsigned g_feats;
void dm_feats_event(unsigned events) { g_feats |= events; }
void dm_gear_equip_tab_pump(uint8_t *b, uint64_t pl) { (void)b; (void)pl; }
int dm_ui_stash_view(int *tab, uint32_t *unit) { *tab = 0; *unit = 0; return 0; }
int dm_ui_stash_toward(int tab, uint32_t unit) { (void)tab; (void)unit; return 1; }

#if defined(__x86_64__)



enum { CI_CELL, CI_WEAPON, CI_GEAR, CI_SWORD, CI_N };
static const uint8_t CI_W[CI_N] = { 1, 1, 2, 2 }, CI_H[CI_N] = { 1, 3, 2, 4 };
typedef struct { uint8_t raw[0x200], data[0x100], pos[0x20]; } fitem;
#define NFI 96
static fitem g_it[NFI]; static int g_nit;
static uint8_t g_player[0x100], g_inv[0x50], g_table[0x1600], g_rec[CI_N * 0x1c0];
static uint8_t *g_base;
static uint64_t P(const void *p) { return (uint64_t)(uintptr_t)p; }
static uint32_t id_of(const fitem *f) { return *(const uint32_t *)(f->raw + 8); }
static uint32_t mode_of(const fitem *f) { return *(const uint32_t *)(f->raw + 0xc); }
static int ci_of(const fitem *f) { return (int)*(const uint32_t *)(f->raw + 4); }
static fitem *by_id(uint32_t id) { for (int i = 0; i < g_nit; i++) if (id_of(&g_it[i]) == id) return &g_it[i]; return NULL; }
static fitem *by_ptr(uint64_t p) { for (int i = 0; i < g_nit; i++) if (P(&g_it[i]) == p) return &g_it[i]; return NULL; }
static fitem *held(void) { return by_ptr(*(uint64_t *)(g_inv + 0x40)); }
static void set_held(fitem *f) { *(uint64_t *)(g_inv + 0x40) = f ? P(f) : 0; if (f) *(uint32_t *)(f->raw + 0xc) = 4; }
static void to_body(fitem *f, int slot) { *(uint32_t *)(f->raw + 0xc) = 1; f->data[0x54] = (uint8_t)slot; }
static void to_cell(fitem *f, int x, int y) { *(uint32_t *)(f->raw + 0xc) = 0; f->data[0x55] = 0; *(int32_t *)(f->pos + 0x10) = x; *(int32_t *)(f->pos + 0x14) = y; }
static fitem *worn_in(int slot) { for (int i = 0; i < g_nit; i++) if (mode_of(&g_it[i]) == 1 && g_it[i].data[0x54] == slot) return &g_it[i]; return NULL; }
static fitem *add_item(uint32_t id, int ci) {
    fitem *f = &g_it[g_nit++]; memset(f, 0, sizeof *f);
    *(uint32_t *)f->raw = 4; *(uint32_t *)(f->raw + 4) = (uint32_t)ci; *(uint32_t *)(f->raw + 8) = id;
    *(uint64_t *)(f->raw + 0x10) = P(f->data); *(uint64_t *)(f->raw + 0x38) = P(f->pos); f->raw[0x1bd] = 1;
    if (g_nit > 1) *(uint64_t *)(g_it[g_nit - 2].data + 0xb0) = P(f->raw); else *(uint64_t *)(g_inv + 0x10) = P(f->raw);
    return f;
}

static int inv_cells(uint16_t used[DM_GE_INV_H]) {
    memset(used, 0, DM_GE_INV_H * sizeof *used);
    for (int i = 0; i < g_nit; i++) { const fitem *f = &g_it[i]; if (mode_of(f) != 0) continue;
        int x = *(const int32_t *)(f->pos + 0x10), y = *(const int32_t *)(f->pos + 0x14), w = CI_W[ci_of(f)], h = CI_H[ci_of(f)];
        if (x < 0 || y < 0 || x + w > DM_GE_INV_W || y + h > DM_GE_INV_H) return -1;
        for (int r = y; r < y + h; r++) { uint16_t m = (uint16_t)(((1u << w) - 1u) << x); if (used[r] & m) return -1; used[r] |= m; } }
    return 0;
}

static int g_sends, g_bad;
static uint32_t f_code(uint64_t it) { fitem *f = by_ptr(it); return f ? 0x20202000u | (uint32_t)ci_of(f) : 0; }


static void f_body_put(uint64_t it, int slot, int a, int b, int c) {
    (void)a; (void)b; (void)c; g_sends++; fitem *h = held(), *w = worn_in(slot);
    if (!it) { if (h || !w) { g_bad++; return; } set_held(w); return; }
    if (!h || P(h) != it) { g_bad++; return; }
    to_body(h, slot); set_held(w);
}

static void f_send(const uint8_t *p, int n) {
    g_sends++; uint32_t v[4]; if (n != 17) { g_bad++; return; } memcpy(v, p + 1, sizeof v);
    fitem *f = by_id(v[0]); if (!f || v[3] != 0) { g_bad++; return; }
    if (p[0] == 0x19) {
        if (held() || mode_of(f) != 0 || *(int32_t *)(f->pos + 0x10) != (int32_t)v[1] || *(int32_t *)(f->pos + 0x14) != (int32_t)v[2]) { g_bad++; return; }
        set_held(f); return;
    }
    if (p[0] == 0x18) {
        if (held() != f) { g_bad++; return; }
        to_cell(f, (int)v[1], (int)v[2]); uint16_t used[DM_GE_INV_H];
        if (inv_cells(used)) { set_held(f); g_bad++; return; }
        set_held(NULL); return;
    }
    g_bad++;
}

static int f_place_check(uint64_t pl, int slot, uint64_t it, int a, int b) { (void)pl; (void)it; (void)a; (void)b; return worn_in(slot) ? 5 : 1; }
static void tramp(uint64_t off, void *fn) { uint8_t *p = g_base + off; p[0] = 0x48; p[1] = 0xb8; memcpy(p + 2, &fn, 8); p[10] = 0xff; p[11] = 0xe0; }
static int base_init(void) {
    g_base = mmap(NULL, 0x2d70000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (g_base == MAP_FAILED) return -1;
    tramp(0x570260, (void *)f_code); tramp(0x3bb24c, (void *)f_body_put); tramp(0x2ceab0, (void *)f_send); tramp(0x565950, (void *)f_place_check);
    *(uint64_t *)(g_base + 0x2d64a18 + 16) = P(g_table); *(uint64_t *)(g_table + 0x15c0) = P(g_rec); *(uint32_t *)(g_table + 0x15c8) = CI_N;
    for (int c = 0; c < CI_N; c++) { g_rec[c * 0x1c0 + 0x11e] = CI_W[c]; g_rec[c * 0x1c0 + 0x11f] = CI_H[c]; }
    g_base[0x2d5ff60] = 1;
    *(uint64_t *)(g_player + 0x90) = P(g_inv); *(uint32_t *)g_inv = 0x01020304;
    return 0;
}


static gstock G; static dm_gear_snap S; static dm_gtab_show SHOW[GS_MAXCAND]; static dm_gtab_row R[4]; static int NR;
static int g_main, g_off;
static dm_gear_tables GT; static const dm_sim_data *D;
static int snap_init(void) {
    if (!D) D = gs_tables(&GT);
    const dm_sim_data *d = D;
    char *prof = gs_profile("class4-level80.prof");
    gspec s; memset(&s, 0, sizeof s); s.profile = prof; s.act = 1; s.seed = 3; s.variants = 1; s.dual = 1;
    int e = gs_build(&G, d, &GT, &s); free(prof); if (e) return -1;
    memset(&S, 0, sizeof S); S.state = DM_GTAB_OK; S.p = G.p; S.show = SHOW; dm_gear_pick_now(&S.p, &S.now);
    g_main = S.now.at[DM_GEAR_P_MAIN]; g_off = S.now.at[DM_GEAR_P_OFF];
    if (!S.p.ready || g_main < 0 || g_off < 0 || G.cand[g_main].kind != DM_GEAR_K_WEAPON1 || G.cand[g_off].kind != DM_GEAR_K_WEAPON1) return -1;
    for (int c = 0; c < S.p.ncand; c++) { SHOW[c].where = G.cand[c].worn ? DM_GTAB_WORN : DM_GTAB_STASH; SHOW[c].id = G.cand[c].item.id; }

    memset(R, 0, sizeof R); NR = 0;
    R[NR].place = DM_GEAR_P_MAIN; R[NR].cand = g_off; NR++;
    R[NR].place = DM_GEAR_P_OFF; R[NR].cand = g_main; NR++;
    return 0;
}


static void game_with(int sword, int fillers, const uint16_t hole[DM_GE_INV_H]) {
    g_nit = 0; memset(g_inv + 0x10, 0, 0x40); g_owner = 0; g_sends = g_bad = 0;
    for (int slot = 1; slot <= DM_GO_GLOVES; slot++) { int c = S.now.at[dm_ge_place_of_slot[slot]]; if (c < 0) continue;
        to_body(add_item(G.cand[c].item.id, G.cand[c].kind == DM_GEAR_K_WEAPON2 ? CI_SWORD : G.cand[c].weapon >= 0 ? CI_WEAPON : CI_GEAR), slot); }
    uint16_t taken[DM_GE_INV_H]; memcpy(taken, hole, sizeof taken);
    if (sword >= 0) { to_cell(add_item(G.cand[sword].item.id, CI_SWORD), 0, 0); for (int y = 0; y < CI_H[CI_SWORD]; y++) taken[y] |= (uint16_t)((1u << CI_W[CI_SWORD]) - 1u); }
    for (int y = 0, n = 0; y < DM_GE_INV_H; y++) for (int x = 0; x < DM_GE_INV_W && n < fillers; x++) {
        if (taken[y] >> x & 1) continue; to_cell(add_item((uint32_t)(5000 + n), CI_CELL), x, y); n++; }
}
static void game(int fillers, const uint16_t hole[DM_GE_INV_H]) { game_with(-1, fillers, hole); }

static void check_items(void) {
    int slots[DM_GO_SLOTS] = { 0 };
    for (int i = 0; i < g_nit; i++) { uint32_t m = mode_of(&g_it[i]); CHECK(m == 0 || m == 1); if (m == 1) slots[g_it[i].data[0x54]]++; }
    for (int s = 0; s < DM_GO_SLOTS; s++) CHECK(slots[s] <= 1);
    uint16_t used[DM_GE_INV_H]; CHECK(inv_cells(used) == 0);
    CHECK(held() == NULL);
}


static int plan_swap(dm_gear_equip_cap *cap, dm_go_plan *plan) {
    dm_gew_target t;
    CHECK(dm_gew_target_of(&S, R, NR, 0, &t) == 0);
    CHECK(t.nunit == 1 && t.slot[DM_GO_RIGHT].kind == DM_GEW_UNIT && t.slot[DM_GO_RIGHT].id == G.cand[g_off].item.id && t.slot[DM_GO_LEFT].id == G.cand[g_main].item.id);
    int e = dm_gear_equip_capture_want(g_base, P(g_player), &t, cap);
    CHECK(e == 0); if (e) { printf("  capture: %s\n", dm_gear_equip_fail()); return 1; }
    CHECK(cap->unit[cap->target[DM_GO_RIGHT]] == G.cand[g_off].item.id && cap->unit[cap->target[DM_GO_LEFT]] == G.cand[g_main].item.id);
    int r = dm_gear_equip_plan_seq(&S, cap, t.seq, t.nseq, plan);
    if (!r && dm_ge_check(&cap->w, plan, cap->gp)) r = DM_GO_E_ROOM;
    return r;
}

static void t_no_room(const char *name, int fillers, const uint16_t hole[DM_GE_INV_H]) {
    g_case = name; game(fillers, hole);
    uint32_t right = id_of(worn_in(DM_GO_RIGHT)), left = id_of(worn_in(DM_GO_LEFT));
    static dm_gear_equip_cap cap; static dm_go_plan plan;
    CHECK(plan_swap(&cap, &plan) == DM_GO_E_ROOM);

    CHECK(g_owner == 0);
    CHECK(g_sends == 0 && id_of(worn_in(DM_GO_RIGHT)) == right && id_of(worn_in(DM_GO_LEFT)) == left);
    check_items();
}
static void t_swap(void) {
    g_case = "room for one weapon: the hands trade places";
    uint16_t hole[DM_GE_INV_H] = { 1u << 9, 1u << 9, 1u << 9, 0 };
    game(37, hole);
    uint32_t right = id_of(worn_in(DM_GO_RIGHT)), left = id_of(worn_in(DM_GO_LEFT)); uint16_t before[DM_GE_INV_H]; inv_cells(before);
    int n0 = g_nit;
    static dm_gear_equip_cap cap; static dm_go_plan plan;
    int e = plan_swap(&cap, &plan); CHECK(e == DM_GO_OK); if (e) return;
    CHECK(plan.nchange == 1 && plan.change[0].slots == (1u << DM_GO_RIGHT | 1u << DM_GO_LEFT));
    g_feats = 0; dm_gear_equip_worthy(1);
    CHECK(dm_gear_equip_start(g_base, P(g_player), &cap, &plan) == 0);
    int frames = 0, st;
    while ((st = dm_gear_equip_state(NULL, NULL, NULL)) == DM_GE_RUNNING && frames < 5000) { dm_gear_equip_pump(g_base, P(g_player)); frames++; }
    int why = 0, done = 0, changes = 0; st = dm_gear_equip_state(&why, &done, &changes);
    CHECK(st == DM_GE_DONE); CHECK(why == DM_GE_WHY_NONE); CHECK(done == 1);
    CHECK(g_feats == DM_FT_E_EQUIP);
    CHECK(g_bad == 0 && g_sends == plan.nstep);
    CHECK(id_of(worn_in(DM_GO_RIGHT)) == left && id_of(worn_in(DM_GO_LEFT)) == right);
    CHECK(g_nit == n0); check_items();
    uint16_t after[DM_GE_INV_H]; inv_cells(after); CHECK(!memcmp(before, after, sizeof before));
    CHECK(g_owner == 0);
    printf("t_gear_equip_capture: the swap in %d moves over %d frames\n", plan.nstep, frames);
}


static int g_sword;

static int sword_init(int dual) {
    if (!D) D = gs_tables(&GT);
    char *prof = gs_profile("class4-level80.prof");
    gspec s; memset(&s, 0, sizeof s); s.profile = prof; s.act = 1; s.seed = 5; s.variants = 1; s.dual = dual;
    int e = gs_build(&G, D, &GT, &s); free(prof); if (e) return -1;
    dm_gear_cand *x = gs_sword(&G, 1, 0); x->item.req_str = 0; g_sword = (int)(x - G.cand);
    if (gs_capture(&G)) return -1;
    memset(&S, 0, sizeof S); S.state = DM_GTAB_OK; S.p = G.p; S.show = SHOW; dm_gear_pick_now(&S.p, &S.now);
    g_main = S.now.at[DM_GEAR_P_MAIN]; g_off = S.now.at[DM_GEAR_P_OFF];
    if (!S.p.ready || g_main < 0 || g_off < 0 || G.cand[g_off].kind != (dual ? DM_GEAR_K_WEAPON1 : DM_GEAR_K_SHIELD)) return -1;
    for (int c = 0; c < S.p.ncand; c++) { SHOW[c].where = G.cand[c].worn ? DM_GTAB_WORN : c == g_sword ? DM_GTAB_INV : DM_GTAB_STASH; SHOW[c].id = G.cand[c].item.id; }
    return 0;
}

static int plan_hands(int main, int off, dm_gear_equip_cap *cap, dm_go_plan *plan) {
    memset(R, 0, sizeof R); NR = 0;
    if (main != g_main) { R[NR].place = DM_GEAR_P_MAIN; R[NR].cand = main; NR++; }
    if (off != g_off) { R[NR].place = DM_GEAR_P_OFF; R[NR].cand = off; NR++; }
    dm_gew_target t;
    CHECK(dm_gew_target_of(&S, R, NR, 0, &t) == 0);
    int e = dm_gear_equip_capture_want(g_base, P(g_player), &t, cap);
    CHECK(e == 0); if (e) { printf("  capture: %s\n", dm_gear_equip_fail()); return 1; }
    int r = dm_gear_equip_plan_seq(&S, cap, t.seq, t.nseq, plan);
    if (!r && dm_ge_check(&cap->w, plan, cap->gp)) r = DM_GO_E_ROOM;
    return r;
}
static uint32_t id_or0(int slot) { fitem *f = worn_in(slot); return f ? id_of(f) : 0; }

static void run_job(const dm_gear_equip_cap *cap, const dm_go_plan *plan, int main, int off) {
    int n0 = g_nit; g_feats = 0;
    CHECK(dm_gear_equip_start(g_base, P(g_player), cap, plan) == 0);
    int frames = 0;
    while (dm_gear_equip_state(NULL, NULL, NULL) == DM_GE_RUNNING && frames < 5000) { dm_gear_equip_pump(g_base, P(g_player)); frames++; }
    int why = 0, done = 0, changes = 0, st = dm_gear_equip_state(&why, &done, &changes);
    CHECK(st == DM_GE_DONE); CHECK(why == DM_GE_WHY_NONE); CHECK(done == 1);
    CHECK(g_bad == 0 && g_sends == plan->nstep);
    CHECK(id_or0(DM_GO_RIGHT) == (main >= 0 ? G.cand[main].item.id : 0) && id_or0(DM_GO_LEFT) == (off >= 0 ? G.cand[off].item.id : 0));
    CHECK(g_nit == n0); check_items(); CHECK(g_owner == 0);
    CHECK(g_feats == 0);
}
static void t_sword(void) {
    static dm_gear_equip_cap cap; static dm_go_plan plan; uint16_t none[DM_GE_INV_H] = { 0 };
    int full = DM_GE_INV_W * DM_GE_INV_H - CI_W[CI_SWORD] * CI_H[CI_SWORD];
    g_case = "sword beside the shield, full inventory";
    if (sword_init(0)) { CHECK(0); return; }
    game_with(g_sword, full, none);
    int e = plan_hands(g_sword, g_off, &cap, &plan); CHECK(e == DM_GO_OK);
    if (!e) run_job(&cap, &plan, g_sword, g_off);
    g_case = "sword alone, full inventory: no room for the shield";
    game_with(g_sword, full, none);
    uint32_t right = id_or0(DM_GO_RIGHT), left = id_or0(DM_GO_LEFT);
    CHECK(plan_hands(g_sword, -1, &cap, &plan) == DM_GO_E_ROOM);
    CHECK(g_owner == 0 && g_sends == 0 && id_or0(DM_GO_RIGHT) == right && id_or0(DM_GO_LEFT) == left); check_items();
    g_case = "sword alone, room for the shield";
    uint16_t spot[DM_GE_INV_H] = { 3u << 8, 3u << 8, 0, 0 };
    game_with(g_sword, full - 4, spot);
    e = plan_hands(g_sword, -1, &cap, &plan); CHECK(e == DM_GO_OK);
    if (!e) run_job(&cap, &plan, g_sword, -1);
    g_case = "sword as the second weapon, full inventory";
    if (sword_init(1)) { CHECK(0); return; }
    game_with(g_sword, full, none);
    e = plan_hands(g_main, g_sword, &cap, &plan); CHECK(e == DM_GO_OK);
    if (!e) run_job(&cap, &plan, g_main, g_sword);
    printf("t_gear_equip_capture: the sword cases done\n");
}



static int g_plain;
static int claw_init(void) {
    if (!D) D = gs_tables(&GT);
    char *prof = gs_profile("class6-level80.prof");
    gspec s; memset(&s, 0, sizeof s); s.profile = prof; s.act = 1; s.seed = 7; s.variants = 1; s.dual = 1; s.claws = 2; s.count[DM_GEAR_K_WEAPON1] = 1;
    int e = gs_build(&G, D, &GT, &s); free(prof); if (e) return -1;
    g_plain = -1;
    for (int c = 0; c < G.p.ncand; c++) if (G.cand[c].kind == DM_GEAR_K_WEAPON1 && !G.cand[c].worn) { g_plain = c; G.cand[c].item.req_str = 0; }
    if (g_plain < 0 || gs_capture(&G)) return -1;
    memset(&S, 0, sizeof S); S.state = DM_GTAB_OK; S.p = G.p; S.show = SHOW; dm_gear_pick_now(&S.p, &S.now);
    g_main = S.now.at[DM_GEAR_P_MAIN]; g_off = S.now.at[DM_GEAR_P_OFF];
    if (!S.p.ready || g_main < 0 || g_off < 0 || !dm_gear_pair_weapon(&S.p, g_main) || !dm_gear_pair_weapon(&S.p, g_off) || dm_gear_pair_weapon(&S.p, g_plain)) return -1;
    for (int c = 0; c < S.p.ncand; c++) { SHOW[c].where = G.cand[c].worn ? DM_GTAB_WORN : c == g_plain ? DM_GTAB_INV : DM_GTAB_STASH; SHOW[c].id = G.cand[c].item.id; }
    return 0;
}

static int pairs_hold(const dm_gear_equip_cap *cap, const dm_go_plan *plan) {
    dm_go_world w = cap->w; int hand = -1;
    for (int k = 0; k <= plan->nstep; k++) {
        int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(&w, worn);
        int c[2] = { -1, -1 };
        for (int h = 0; h < 2; h++) { int i = worn[h ? DM_GO_LEFT : DM_GO_RIGHT]; if (i < 0) continue;
            for (int q = 0; q < S.p.ncand; q++) if (G.cand[q].item.id == cap->unit[i]) c[h] = q; }
        if (c[0] >= 0 && c[1] >= 0 && G.cand[c[0]].weapon >= 0 && G.cand[c[1]].weapon >= 0 && (!dm_gear_pair_weapon(&S.p, c[0]) || !dm_gear_pair_weapon(&S.p, c[1]))) return 0;
        if (k < plan->nstep && dm_gear_order_apply(&w, &plan->step[k].go, &hand)) return 0;
    }
    return 1;
}
static void t_claws(void) {
    static dm_gear_equip_cap cap; static dm_go_plan plan; uint16_t none[DM_GE_INV_H] = { 0 }, spot[DM_GE_INV_H] = { 1u << 9, 1u << 9, 1u << 9, 0 };
    int full = DM_GE_INV_W * DM_GE_INV_H - CI_W[CI_SWORD] * CI_H[CI_SWORD];
    g_case = "claws to a plain weapon, full inventory: no room for the left claw while the plain weapon lies in its cells";
    if (claw_init()) { CHECK(0); return; }
    game_with(g_plain, full, none);
    uint32_t right = id_or0(DM_GO_RIGHT), left = id_or0(DM_GO_LEFT);
    CHECK(plan_hands(g_plain, -1, &cap, &plan) == DM_GO_E_ROOM);
    CHECK(g_owner == 0 && g_sends == 0 && id_or0(DM_GO_RIGHT) == right && id_or0(DM_GO_LEFT) == left); check_items();
    g_case = "claws to a plain weapon, one spot for a claw";
    game_with(g_plain, full - 3, spot);
    int e = plan_hands(g_plain, -1, &cap, &plan); CHECK(e == DM_GO_OK);
    if (!e) { CHECK(pairs_hold(&cap, &plan)); run_job(&cap, &plan, g_plain, -1); }
    g_case = "claws trade places, one spot for a claw";
    game_with(g_plain, full - 3, spot);
    e = plan_hands(g_off, g_main, &cap, &plan); CHECK(e == DM_GO_OK);
    if (!e) { CHECK(pairs_hold(&cap, &plan)); run_job(&cap, &plan, g_off, g_main); }
    g_case = "a plain weapon beside a claw: no plan";
    game_with(g_plain, full, none);
    CHECK(plan_hands(g_main, g_plain, &cap, &plan) != DM_GO_OK); CHECK(g_sends == 0); check_items();
    printf("t_gear_equip_capture: the claw cases done\n");
}

int main(void) {
    if (base_init() || snap_init()) { printf("FAIL: no fake game or no stock with two weapons\n"); return 1; }
    uint16_t none[DM_GE_INV_H] = { 0 };
    t_no_room("a completely full inventory: no plan", DM_GE_INV_W * DM_GE_INV_H, none);
    uint16_t one[DM_GE_INV_H] = { 0, 0, 0, 1u << 9 };
    t_no_room("one free cell: no room for a weapon, no plan", DM_GE_INV_W * DM_GE_INV_H - 1, one);
    t_swap();
    t_sword();
    t_claws();
    printf("t_gear_equip_capture: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
#else
int main(void) { printf("t_gear_equip_capture: the fake game needs x86-64, skipped\n"); return 0; }
#endif
