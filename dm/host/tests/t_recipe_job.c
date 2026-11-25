














#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "../dm.h"
#include "../dm_hud.h"
#include "../dm_recipes.h"
#include "../dm_itemjob.h"
static uint64_t t_now = 1000000000ull;
static int t_clock_gettime(clockid_t c, struct timespec *ts) { (void)c; ts->tv_sec = (time_t)(t_now / 1000000000ull); ts->tv_nsec = (long)(t_now % 1000000000ull); return 0; }
#define clock_gettime(c, ts) t_clock_gettime(c, ts)
static void t_log(const char *fmt, ...);
#undef LOG
#define LOG(...) t_log(__VA_ARGS__)
#define DM_RECIPE_TESTS 1
#include "../dm_recipes.c"


enum { MAXI = 24, PID = 77, SHARED_UNIT = 900, CUBE = 1, R01 = 0x20313072             , BOX = 0x20786f62              };
enum { OWN_PLAYER, OWN_SHARED, OWN_NONE };
typedef struct { _Alignas(8) uint8_t u[0x120]; _Alignas(8) uint8_t d[0xc0]; _Alignas(8) uint8_t p[0x20]; int used, owner, w, h; } fake_item;
static fake_item g_it[MAXI];
static _Alignas(8) uint8_t g_pl[0x200], g_inv[0x60], g_sh[0x200], g_shinv[0x60];
static uint32_t g_hand;
#define F_ID(i) (*(uint32_t *)((i)->u + 8))
#define F_MODE(i) (*(uint32_t *)((i)->u + 0xc))
#define F_CODE(i) (*(uint32_t *)((i)->u + 0x100))
#define F_PAGE(i) ((i)->d[0x55])
#define F_STACK(i) ((i)->d[0x9c])
#define F_X(i) (*(int32_t *)((i)->p + 0x10))
#define F_Y(i) (*(int32_t *)((i)->p + 0x14))
static uint64_t ptr(const void *p) { return (uint64_t)(uintptr_t)p; }
static fake_item *find(uint32_t id) { for (int i = 0; i < MAXI; i++) if (g_it[i].used && F_ID(&g_it[i]) == id) return &g_it[i]; return NULL; }
static fake_item *add_item(uint32_t id, uint32_t code, int page, int x, int y, int w, int h, int owner) {
    for (int i = 0; i < MAXI; i++) if (!g_it[i].used) { fake_item *f = &g_it[i]; memset(f, 0, sizeof *f);
        f->used = 1; f->owner = owner; f->w = w; f->h = h; *(uint32_t *)f->u = 4; F_ID(f) = id; F_MODE(f) = 0; F_CODE(f) = code;
        *(uint64_t *)(f->u + 0x10) = ptr(f->d); *(uint64_t *)(f->u + 0x38) = ptr(f->p); *(uint32_t *)f->d = 2; F_PAGE(f) = (uint8_t)page; F_X(f) = x; F_Y(f) = y; return f; }
    return NULL;
}
static void relink(void) {
    uint8_t *heads[2] = { g_inv, g_shinv };
    for (int o = 0; o < 2; o++) { uint64_t *link = (uint64_t *)(heads[o] + 0x10); *link = 0;
        for (int i = 0; i < MAXI; i++) if (g_it[i].used && g_it[i].owner == o) { *link = ptr(g_it[i].u); link = (uint64_t *)(g_it[i].d + 0xb0); *link = 0; } }
}
static void grid_size(int page, int *w, int *h) { *w = page == 3 ? 3 : 10; *h = page == 3 ? 4 : page == 4 ? 10 : 4; }
static int cell_free(int page, int x, int y, int w, int h) {
    int gw, gh; grid_size(page, &gw, &gh); if (x < 0 || y < 0 || x + w > gw || y + h > gh) return 0;
    for (int i = 0; i < MAXI; i++) { fake_item *f = &g_it[i];
        if (!f->used || f->owner != OWN_PLAYER || F_MODE(f) != 0 || F_PAGE(f) != page) continue;
        if (x < F_X(f) + f->w && F_X(f) < x + w && y < F_Y(f) + f->h && F_Y(f) < y + h) return 0; }
    return 1;
}
static int first_free(int page, int w, int h, int *x, int *y) {
    int gw, gh; grid_size(page, &gw, &gh);
    for (int yy = 0; yy < gh; yy++) for (int xx = 0; xx < gw; xx++) if (cell_free(page, xx, yy, w, h)) { *x = xx; *y = yy; return 1; }
    return 0;
}
static void put(fake_item *f, int owner, int page, int x, int y) { f->owner = owner; F_MODE(f) = 0; F_PAGE(f) = (uint8_t)page; F_X(f) = x; F_Y(f) = y; if (g_hand == F_ID(f)) g_hand = 0; }
static void to_hand(fake_item *f) { f->owner = OWN_PLAYER; F_MODE(f) = 4; g_hand = F_ID(f); }


static uint8_t g_q[8][24]; static int g_qn[8], g_nq, g_qt[8], g_tick, g_lag;
static int g_picks, g_places, g_stow_calls, g_stow_req, g_room, g_cube_shown, g_next_id;
static int g_drop_places, g_drop_picks;
static int g_floor_pkts; static uint32_t g_busy_hand;
static void queue(const uint8_t *p, int n) { if (g_nq < 8 && n <= 24) { memcpy(g_q[g_nq], p, (size_t)n); g_qt[g_nq] = g_tick; g_qn[g_nq++] = n; } }
static void apply_packets(void) {
    int keep = 0; g_tick++;
    for (int k = 0; k < g_nq; k++) { const uint8_t *p = g_q[k];
        if (g_tick - g_qt[k] <= g_lag) { if (keep != k) { memcpy(g_q[keep], g_q[k], sizeof g_q[k]); g_qn[keep] = g_qn[k]; g_qt[keep] = g_qt[k]; } keep++; continue; } uint32_t v[4]; memcpy(v, p + 1, 16); fake_item *f = find(v[0]);
        if (g_drop_picks && (p[0] == 0x19 || p[0] == 0x46 || p[0] == 0xf1)) continue;
        if (p[0] == 0x19 && f && !g_hand && f->owner == OWN_PLAYER && F_MODE(f) == 0 && F_PAGE(f) == v[3] && F_X(f) == (int)v[1] && F_Y(f) == (int)v[2]) to_hand(f);
        else if (p[0] == 0x18 && f && g_hand == v[0] && !g_drop_places && cell_free((int)v[3], (int)v[1], (int)v[2], f->w, f->h)) put(f, OWN_PLAYER, (int)v[3], (int)v[1], (int)v[2]);
        else if (p[0] == 0x46 && f && !g_hand && f->owner == OWN_SHARED) to_hand(f);
        else if (p[0] == 0xf1 && f && !g_hand && F_STACK(f) > 0) {
            F_STACK(f)--; fake_item *n = add_item((uint32_t)g_next_id++, F_CODE(f), 0, 0, 0, 1, 1, OWN_PLAYER); if (n) to_hand(n); } }
    g_nq = keep;
}
static void apply_stow(void) {
    if (!g_stow_req) return; g_stow_req = 0; fake_item *f = g_hand ? find(g_hand) : NULL; int x, y;
    if (f && g_room && first_free(0, f->w, f->h, &x, &y)) put(f, OWN_PLAYER, 0, x, y);
}

enum { ACT_NONE, ACT_STOW, ACT_STOW_SRC, ACT_GROUND, ACT_VANISH, ACT_CUBE, ACT_SHARED };
static int g_actor, g_actor_state, g_actor_page, g_hold_ticks; static uint32_t g_last_hand;
static void actor(void) {
    fake_item *f = g_hand ? find(g_hand) : NULL; int x, y;
    if (!g_actor || !f || atomic_load(&g_jst) != g_actor_state || g_hold_ticks < 40) return;
    switch (g_actor) {
    case ACT_STOW: if (first_free(0, f->w, f->h, &x, &y)) put(f, OWN_PLAYER, 0, x, y); break;
    case ACT_STOW_SRC: if (first_free(g_actor_page, f->w, f->h, &x, &y)) put(f, OWN_PLAYER, g_actor_page, x, y); break;
    case ACT_GROUND: f->owner = OWN_NONE; F_MODE(f) = 3; g_hand = 0; break;
    case ACT_VANISH: g_hand = 0; f->used = 0; break;
    case ACT_CUBE: if (cell_free(3, 2, 3, f->w, f->h)) put(f, OWN_PLAYER, 3, 2, 3); break;
    case ACT_SHARED: put(f, OWN_SHARED, 0, 2, 2); break;
    }
    g_actor = ACT_NONE;
}


static uint32_t t_item_code(uint64_t it) { return *(uint32_t *)(it + 0x100); }
_Atomic int g_dm_lang;
uint64_t dm_rw_item(uint8_t *b, uint32_t id) { (void)b; fake_item *f = find(id); return f ? ptr(f->u) : 0; }
uint64_t dm_rw_held(uint8_t *b, uint64_t pl) { (void)b; (void)pl; fake_item *f = g_hand ? find(g_hand) : NULL; return f ? ptr(f->u) : 0; }
void dm_rw_send(uint8_t *b, const uint8_t *p, int n) { (void)b; if (p[0] == 0x19 || p[0] == 0x46) g_picks++; if (p[0] == 0x18) g_places++; if (p[0] == 0x17) g_floor_pkts++; queue(p, n); }
void dm_rw_take_stack(uint8_t *b, uint64_t it) { (void)b; uint8_t p[5] = { 0xf1 }; uint32_t id = *(uint32_t *)(it + 8); memcpy(p + 1, &id, 4); g_picks++; queue(p, 5); }
int dm_rw_panel_open(uint8_t *b) { (void)b; return 1; }
int dm_rw_owners(uint8_t *b, uint64_t pl, uint64_t us[3]) { (void)b; us[0] = pl; us[1] = ptr(g_sh); us[2] = 0; return 2; }
int dm_rw_text(uint8_t *b, int rec, char *out, size_t n) { (void)b; (void)rec; (void)out; (void)n; return 0; }
int dm_item_desc(uint8_t *b, int kind, int row, int grp, char *out, size_t n) { (void)b; (void)kind; (void)row; (void)grp; (void)n; out[0] = 0; return 0; }
void dm_held_stow(void) { g_stow_calls++; g_stow_req = 1; }
int dm_ui_panel_shown(const char *name) { return g_cube_shown && !strcmp(name, "HoradricCubeLayout"); }
void dm_item_use(uint32_t item_id) { if (item_id == CUBE) g_cube_shown = 1; }
void dm_hud_get(dm_hud_info *out) { memset(out, 0, sizeof *out); out->valid = 1; out->area = 40; out->holding = (int)g_hand; }
uint64_t dm_diag_game(void) { return 0x1000; }
uint64_t dm_diag_player(void) { return 0x2000; }
uint32_t dm_diag_epoch(void) { return 7; }
int dm_pages_char(char *out, size_t n) { (void)out; (void)n; return -1; }
int dm_pages_cur(void) { return 0; }
int dm_pages_count(void) { return 1; }
int dm_unsocket_ready(void) { return 0; }
int dm_pouch_each(void (*cb)(uint32_t code, void *arg), void *arg) { (void)cb; (void)arg; return -1; }
int dm_vault_each(void (*cb)(int kind, int idx, void *arg), void *arg) { (void)cb; (void)arg; return -1; }


static char g_log[96][400]; static int g_nlog;
static void t_log(const char *fmt, ...) {
    char line[400]; va_list a; va_start(a, fmt); vsnprintf(line, sizeof line, fmt, a); va_end(a);
    if (g_nlog < 96) memcpy(g_log[g_nlog++], line, sizeof line);
    if (getenv("T_VERBOSE")) printf("    | %s\n", line);
}
static int logged(const char *needle) { for (int i = 0; i < g_nlog; i++) if (strstr(g_log[i], needle)) return 1; return 0; }
static int reds(void) { int n = 0; for (int i = 0; i < g_nlog; i++) n += strstr(g_log[i], "【红】") != NULL; return n; }
static int checks, fails; static const char *g_case;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s line %d: %s\n", g_case, __LINE__, #c); } } while (0)

static uint8_t *B;
static char g_failfile[256];
static void fail_spec(const char *spec) {
    if (!spec) { unsetenv("DM_TEST_RECIPE_FAIL"); return; }
    FILE *f = fopen(g_failfile, "w"); if (!f) { perror(g_failfile); exit(2); } fprintf(f, "%s\n", spec); fclose(f); setenv("DM_TEST_RECIPE_FAIL", g_failfile, 1);
}

static void world(const char *name) {
    g_case = name; memset(g_it, 0, sizeof g_it); g_hand = 0; g_nq = 0; g_lag = 0; g_drop_picks = 0; g_picks = g_places = g_stow_calls = g_stow_req = g_cube_shown = g_drop_places = g_floor_pkts = 0; g_busy_hand = 0; g_room = 1; g_next_id = 500;
    g_actor = ACT_NONE; g_actor_state = J_BACK_WAIT; g_actor_page = 0; g_hold_ticks = 0; g_last_hand = 0; g_nlog = 0; fail_spec(NULL);
    memset(g_pl, 0, sizeof g_pl); memset(g_inv, 0, sizeof g_inv); memset(g_sh, 0, sizeof g_sh); memset(g_shinv, 0, sizeof g_shinv);
    *(uint32_t *)(g_pl + 8) = PID; *(uint32_t *)(g_pl + 0xc) = 1; *(uint64_t *)(g_pl + 0x90) = ptr(g_inv); *(uint32_t *)g_inv = 0x01020304;
    *(uint32_t *)(g_sh + 8) = SHARED_UNIT; *(uint64_t *)(g_sh + 0x90) = ptr(g_shinv); *(uint32_t *)g_shinv = 0x01020304;
    add_item(CUBE, BOX, 0, 0, 0, 2, 3, OWN_PLAYER);
    add_item(11, R01, 0, 8, 0, 1, 1, OWN_PLAYER); add_item(12, R01, 0, 7, 3, 1, 1, OWN_PLAYER); add_item(13, R01, 0, 7, 2, 1, 1, OWN_PLAYER);
    relink();
}
static int run(int max) {
    for (int t = 0; t < max; t++) {
        apply_packets(); apply_stow(); actor(); relink();
        g_hold_ticks = g_hand && g_hand == g_last_hand ? g_hold_ticks + 1 : 0; g_last_hand = g_hand;
        t_now += 16000000ull; job_pump(B, ptr(g_pl));
        if (!dm_recipes_busy()) return t + 1;
        if (g_hand) g_busy_hand = g_hand;
    }
    return -1;
}
static int result(void) { return dm_recipes_insert_state() & 255; }
static int reason(void) { return dm_recipes_insert_state() >> 8; }
static int at(uint32_t id, int page, int x, int y) { fake_item *f = find(id); return f && f->owner == OWN_PLAYER && F_MODE(f) == 0 && F_PAGE(f) == page && F_X(f) == x && F_Y(f) == y; }
static int on_page(uint32_t id, int page) { fake_item *f = find(id); return f && f->owner == OWN_PLAYER && F_MODE(f) == 0 && F_PAGE(f) == page; }

#define STEP(id, x, y, cx) { id, 0, 0, x, y, 0, 3, cx, 0, R01, 2, 0, 0 }
static const dm_recipe_step INV3[3] = { STEP(11, 8, 0, 0), STEP(12, 7, 3, 1), STEP(13, 7, 2, 2) };
static int insert(const dm_recipe_step *s, int n) { return dm_recipes_insert((const int *)s, n, CUBE, 0); }

static void start_in_hand(const dm_recipe_step *s, int n, int stack_before) {
    memset(&g_job, 0, sizeof g_job); memcpy(g_job.s, s, (size_t)n * sizeof *s); g_job.n = n; g_job.cube = CUBE;
    g_job.G = dm_diag_game(); g_job.epoch = dm_diag_epoch(); g_job.area = 40; g_job.pid = PID; g_job.stack_before = stack_before;
    atomic_store(&g_reason, 0); atomic_store(&g_result, DM_RECIPE_RUNNING); job_set(J_HAND);
}


static void t_normal(void) {
    world("normal"); CHECK(insert(INV3, 3) == 1); CHECK(run(2000) > 0);
    CHECK(result() == DM_RECIPE_DONE && reason() == 0);
    CHECK(at(11, 3, 0, 0) && at(12, 3, 1, 0) && at(13, 3, 2, 0) && !g_hand);
    CHECK(reds() == 0 && g_stow_calls == 0 && g_picks == 3 && g_places == 3 && logged("放入魔盒完成"));
}
static void t_check_only(void) {
    world("kind3 check only"); fake_item *f = find(13); put(f, OWN_PLAYER, 3, 0, 0); relink();
    dm_recipe_step s[3] = { { 13, 3, 3, 0, 0, 0, 3, 0, 0, R01, 2, 0, 0 }, STEP(11, 8, 0, 1), STEP(12, 7, 3, 2) };
    CHECK(insert(s, 3) == 1); CHECK(run(2000) > 0);
    CHECK(result() == DM_RECIPE_DONE && at(13, 3, 0, 0) && at(11, 3, 1, 0) && at(12, 3, 2, 0));
    CHECK(g_picks == 2 && g_places == 2 && reds() == 0);
}
static void t_e7_11(void) {
    world("E7 1:1"); fail_spec("1:1"); CHECK(insert(INV3, 3) == 1); CHECK(run(2000) > 0);
    CHECK(result() == DM_RECIPE_FAILED && reason() == DM_RECIPE_WHY_GONE);
    CHECK(at(11, 0, 8, 0) && at(12, 0, 7, 3) && at(13, 0, 7, 2) && !g_hand && g_picks == 0 && g_places == 0);
    CHECK(reds() == 1 && logged("停在第 1 / 3 步"));
}
static void t_e7_22(void) {
    world("E7 2:2"); fail_spec("2:2"); CHECK(insert(INV3, 3) == 1); CHECK(run(2000) > 0);
    CHECK(result() == DM_RECIPE_FAILED && reason() == DM_RECIPE_WHY_LOST);
    CHECK(at(11, 3, 0, 0) && at(12, 0, 7, 3) && at(13, 0, 7, 2) && !g_hand);
    CHECK(logged("手上那件已放回") && !logged("手上空了") && reds() == 1 && g_stow_calls == 0);
}
static void t_e7_223_own(void) {
    world("E7 2:23 own stow"); fail_spec("2:23"); CHECK(insert(INV3, 3) == 1); int t = run(2000); CHECK(t > 0);
    CHECK(result() == DM_RECIPE_FAILED && reason() == DM_RECIPE_WHY_LOST);
    CHECK(at(11, 3, 0, 0) && on_page(12, 0) && !at(12, 0, 7, 3) && at(13, 0, 7, 2) && !g_hand);
    CHECK(g_stow_calls == 1 && logged("手上那件收进了背包") && !logged("手上空了") && reds() == 1);
    CHECK(t > 2 * 125);
    CHECK(g_places == 1);
}
static void t_e7_223_guard(void) {
    world("E7 2:23 stowed by someone else"); fail_spec("2:23"); g_actor = ACT_STOW; CHECK(insert(INV3, 3) == 1); CHECK(run(2000) > 0);
    CHECK(result() == DM_RECIPE_FAILED && reason() == DM_RECIPE_WHY_LOST);
    CHECK(at(11, 3, 0, 0) && on_page(12, 0) && !g_hand);
    CHECK(logged("没回原格，在背包") && !logged("手上空了") && reds() == 1 && g_stow_calls == 0);
}
static void t_e7b_full(void) {
    world("E7b no room"); fail_spec("2:23"); g_room = 0; CHECK(insert(INV3, 3) == 1); CHECK(run(2000) > 0);
    CHECK(result() == DM_RECIPE_HELD && reason() == DM_RECIPE_WHY_HELD);
    CHECK(g_hand == 12 && at(11, 3, 0, 0) && at(13, 0, 7, 2) && g_stow_calls == 1);
    CHECK(logged("放不回去，留在手上") && !logged("手上空了"));


    CHECK(!dm_recipes_busy() && g_busy_hand == 12 && g_hand == 12 && g_floor_pkts == 0);
}
static void t_loss(const char *name, int act, int state, int want_reason) {
    world(name); fail_spec("2:23"); g_actor = act; g_actor_state = state; if (state == J_STOW) g_room = 0;
    CHECK(insert(INV3, 3) == 1); CHECK(run(2000) > 0);
    CHECK(result() == DM_RECIPE_HELD && reason() == want_reason && !g_hand);
    CHECK(logged("手上空了，那件既不在原处也不在背包里") && reds() == 2 && !logged("收进了背包") && !logged("没回原格"));
}



static void t_ownership(void) {
    world("ownership while the job runs"); CHECK(dm_itemjob_owner() == DM_ITEMJOB_NONE); CHECK(insert(INV3, 3) == 1);
    int held = 1;
    for (int t = 0; t < 2000 && dm_recipes_busy(); t++) {
        held &= dm_itemjob_owner() == DM_ITEMJOB_RECIPE;
        apply_packets(); apply_stow(); actor(); relink(); t_now += 16000000ull; job_pump(B, ptr(g_pl)); }
    CHECK(held && !dm_recipes_busy() && result() == DM_RECIPE_DONE && dm_itemjob_owner() == DM_ITEMJOB_NONE);
    world("ownership held until the failed job's item is back"); fail_spec("2:2"); CHECK(insert(INV3, 3) == 1);
    held = 1;
    for (int t = 0; t < 2000 && dm_recipes_busy(); t++) {
        held &= dm_itemjob_owner() == DM_ITEMJOB_RECIPE;
        apply_packets(); apply_stow(); actor(); relink(); t_now += 16000000ull; job_pump(B, ptr(g_pl)); }
    CHECK(held && result() == DM_RECIPE_FAILED && at(12, 0, 7, 3) && dm_itemjob_owner() == DM_ITEMJOB_NONE);
    world("another item job owns the cursor"); CHECK(dm_itemjob_acquire(DM_ITEMJOB_EQUIP));
    CHECK(insert(INV3, 3) == 0 && !dm_recipes_busy() && run(50) == 1 && g_picks == 0 && at(11, 0, 8, 0) && dm_itemjob_owner() == DM_ITEMJOB_EQUIP);
    CHECK(dm_itemjob_release(DM_ITEMJOB_EQUIP) && insert(INV3, 3) == 1 && run(2000) > 0 && result() == DM_RECIPE_DONE && dm_itemjob_owner() == DM_ITEMJOB_NONE);
}



static int run_owned(int max, int *held) {
    *held = 1;
    for (int t = 0; t < max; t++) {
        apply_packets(); apply_stow(); actor(); relink();
        g_hold_ticks = g_hand && g_hand == g_last_hand ? g_hold_ticks + 1 : 0; g_last_hand = g_hand;
        t_now += 16000000ull; job_pump(B, ptr(g_pl));
        if (!dm_recipes_busy()) return t + 1;
        *held &= dm_itemjob_owner() == DM_ITEMJOB_RECIPE;
    }
    return -1;
}
static void t_late(void) {
    int held, t;
    world("late pick answer after the watch window: it comes to the hand and goes back"); g_lag = 300;
    CHECK(insert(INV3, 3) == 1); t = run_owned(3000, &held);
    CHECK(t > 300 && held && result() == DM_RECIPE_FAILED && reason() == DM_RECIPE_WHY_LOST && dm_itemjob_owner() == DM_ITEMJOB_NONE);
    CHECK(on_page(11, 0) && at(12, 0, 7, 3) && at(13, 0, 7, 2) && !g_hand && !logged("手上空了") && g_floor_pkts == 0);
    world("a pick never answered: the job waits the wind-down limit, the item stays at its source"); g_drop_picks = 1;
    CHECK(insert(INV3, 3) == 1); t = run_owned(3000, &held);
    CHECK(t > 600 && held && result() == DM_RECIPE_FAILED && at(11, 0, 8, 0) && !g_hand && logged("never answered") && !logged("手上空了"));
    CHECK(dm_itemjob_owner() == DM_ITEMJOB_NONE);
    world("late place answer: the item reaches its cube cell after the put-back was sent"); g_lag = 140;
    fail_spec(NULL); CHECK(insert(INV3, 1) == 1); t = run_owned(3000, &held);
    CHECK(t > 0 && held && (at(11, 3, 0, 0) || at(11, 0, 8, 0)) && !g_hand && !logged("手上空了") && dm_itemjob_owner() == DM_ITEMJOB_NONE);
    world("the item vanishes for good: lost only at the wind-down limit, with the ASSUME line"); fail_spec("2:23"); g_actor = ACT_VANISH;
    CHECK(insert(INV3, 3) == 1); t = run_owned(3000, &held);
    CHECK(held && result() == DM_RECIPE_HELD && reason() == DM_RECIPE_WHY_LOST && logged("ASSUME itemjob.settle") && t > 625);
}


static void t_stash(const char *name, const char *spec, int act, int room, int want_result, const char *want_log) {
    world(name); fake_item *f = add_item(80, R01, 4, 3, 2, 1, 1, OWN_PLAYER); to_hand(f); relink();
    dm_recipe_step s[1] = { { 80, 0, 4, 3, 2, 0, 3, 0, 0, R01, 2, 0, 0 } };
    fail_spec(spec); g_actor = act; g_actor_page = 4; g_room = room; start_in_hand(s, 1, 0); CHECK(run(2000) > 0);
    CHECK(result() == want_result && logged(want_log));
    if (want_result == DM_RECIPE_FAILED) CHECK(reason() == DM_RECIPE_WHY_LOST && !g_hand && (on_page(80, 4) || on_page(80, 0)) && !logged("手上空了"));
    else CHECK(reason() == DM_RECIPE_WHY_HELD && g_hand == 80 && g_busy_hand == 80 && !dm_recipes_busy());
    CHECK(g_floor_pkts == 0);
}
static void t_stack(void) {
    for (int v = 0; v < 2; v++) {
        world(v ? "stack 1:23 stowed by someone else" : "stack 1:2 put back on the reserved cell");
        fake_item *st = add_item(50, R01, 0, 0, 0, 1, 1, OWN_SHARED); F_STACK(st) = 4; fake_item *n = add_item(60, R01, 0, 0, 0, 1, 1, OWN_PLAYER); to_hand(n); relink();
        dm_recipe_step s[1] = { { 50, 1, 0, 0, 0, SHARED_UNIT, 3, 0, 0, R01, 2, 9, 3 } };
        fail_spec(v ? "1:23" : "1:2"); g_actor = v ? ACT_STOW : ACT_NONE; start_in_hand(s, 1, 5); CHECK(run(2000) > 0);
        CHECK(result() == DM_RECIPE_FAILED && reason() == DM_RECIPE_WHY_LOST && g_job.cur == 60 && !g_hand);
        CHECK(v ? (on_page(60, 0) && logged("没回原格，在背包") && logged("id 60")) : (at(60, 0, 9, 3) && logged("手上那件已放回")));
        CHECK(F_STACK(find(50)) == 4 && find(50)->owner == OWN_SHARED && !logged("手上空了"));
    }
}
static void t_shared(void) {
    for (int v = 0; v < 2; v++) {
        world(v ? "shared 1:23 onto the shared chain" : "shared 1:2 put back on the reserved cell");
        fake_item *f = add_item(70, R01, 0, 0, 0, 1, 1, OWN_PLAYER); to_hand(f); relink();
        dm_recipe_step s[1] = { { 70, 2, 0, 4, 4, SHARED_UNIT, 3, 0, 0, R01, 2, 9, 3 } };
        fail_spec(v ? "1:23" : "1:2"); g_actor = v ? ACT_SHARED : ACT_NONE; start_in_hand(s, 1, 0); CHECK(run(2000) > 0);
        if (!v) CHECK(result() == DM_RECIPE_FAILED && reason() == DM_RECIPE_WHY_LOST && at(70, 0, 9, 3) && logged("手上那件已放回"));
        else CHECK(result() == DM_RECIPE_HELD && reason() == DM_RECIPE_WHY_LOST && logged("手上空了，那件既不在原处也不在背包里"));
    }
}

int main(void) {
    B = (uint8_t *)((uintptr_t)&t_item_code - 0x570260u);
    const char *td = getenv("TMPDIR"); snprintf(g_failfile, sizeof g_failfile, "%s/t_recipe_job_fail_%d.txt", td && *td ? td : "/tmp", (int)getpid());
    t_normal();
    t_check_only();
    t_e7_11();
    t_e7_22();
    t_e7_223_own();
    t_e7_223_guard();
    t_e7b_full();
    t_loss("loss: dropped on the ground while waiting for the put-back", ACT_GROUND, J_BACK_WAIT, DM_RECIPE_WHY_LOST);
    t_loss("loss: gone while waiting for the put-back", ACT_VANISH, J_BACK_WAIT, DM_RECIPE_WHY_LOST);
    t_loss("loss: into the cube, not the inventory", ACT_CUBE, J_BACK_WAIT, DM_RECIPE_WHY_LOST);
    t_loss("loss: onto the shared stash's chain", ACT_SHARED, J_BACK_WAIT, DM_RECIPE_WHY_LOST);
    t_loss("loss: dropped on the ground while the stow waits", ACT_GROUND, J_STOW, DM_RECIPE_WHY_LOST);
    t_stash("stash 1:2 put back on its cell", "1:2", ACT_NONE, 1, DM_RECIPE_FAILED, "手上那件已放回");
    t_stash("stash 1:23 stowed into the stash elsewhere", "1:23", ACT_STOW_SRC, 1, DM_RECIPE_FAILED, "在来源页");
    t_stash("stash 1:23 own stow into the inventory", "1:23", ACT_NONE, 1, DM_RECIPE_FAILED, "手上那件收进了背包");
    t_stash("stash 1:23 inventory full (E7b)", "1:23", ACT_NONE, 0, DM_RECIPE_HELD, "放不回去，留在手上");
    t_stack();
    t_shared();
    t_ownership();
    t_late();
    unlink(g_failfile);
    printf("t_recipe_job: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
