







#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "dm.h"
#include "dm_hud.h"
#include "dm_recipes.h"
#include "dm_itemjob.h"

#define MAX_ITEMS 1024
#define MAX_CACHE 16
#define PAGE_ITEMS 100
enum { PG_INV = 0, PG_CUBE = 3, PG_STASH = 4 };

int dm_rw_owners(uint8_t *b, uint64_t pl, uint64_t us[3]);
int dm_pages_cur(void); int dm_pages_count(void);
int dm_pouch_each(void (*cb)(uint32_t code, void *arg), void *arg);
int dm_vault_each(void (*cb)(int kind, int idx, void *arg), void *arg);
int dm_pages_char(char *out, size_t n);
uint64_t dm_diag_game(void); uint64_t dm_diag_player(void); uint32_t dm_diag_epoch(void);

typedef struct { uint64_t key; int n; dm_recipe_item it[PAGE_ITEMS]; } page_t;
static page_t g_cache[MAX_CACHE];
static dm_recipe_item g_out[MAX_ITEMS], g_work[MAX_ITEMS];
static dm_recipe_head g_head;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static dm_recipe_item g_pouch[MAX_ITEMS], g_vault[MAX_ITEMS];
static int g_npouch, g_nvault;
static _Atomic int g_req, g_next, g_result;
static uint32_t g_read_epoch;
static char g_char[64], g_read_who[64];
static int g_named;

int dm_recipes_request_stock(void) { int id = atomic_fetch_add(&g_next, 1) + 1; atomic_store(&g_req, id); return id; }

int dm_recipes_stock_copy(void *out, int bytes) {
    pthread_mutex_lock(&g_mu); int n = (int)g_head.n, need = (int)sizeof g_head + n * (int)sizeof(dm_recipe_item), got = 0;
    if (g_head.gen && need <= bytes) { memcpy(out, &g_head, sizeof g_head); memcpy((uint8_t *)out + sizeof g_head, g_out, (size_t)n * sizeof(dm_recipe_item)); got = need; }
    pthread_mutex_unlock(&g_mu); return got;
}

static int add(dm_recipe_item *list, int n, int max, const dm_recipe_item *e) {
    if (n < max) list[n] = *e;
    return n + 1;
}

static void read_item(uint8_t *b, uint64_t it, uint64_t d, dm_recipe_item *e) {
    memset(e, 0, sizeof *e);
    e->id = *(uint32_t *)(it + 8);
    { uint64_t pp = *(uint64_t *)(it + 0x38); if (pp) { e->x = (uint8_t)*(int32_t *)(pp + 0x10); e->y = (uint8_t)*(int32_t *)(pp + 0x14); } }
    e->code = ((uint32_t (*)(uint64_t))(b + 0x570260))(it);
    e->qty = 1; e->quality = (uint8_t)*(uint32_t *)d;
    int s = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(it, 0xc2, 0);
    e->sockets = (uint8_t)(s < 0 ? 0 : s > 255 ? 255 : s);
    uint64_t c = *(uint64_t *)(it + 0x90); int f = 0;
    for (uint64_t k = c ? *(uint64_t *)(c + 0x10) : 0; k && f < 8; f++) { uint64_t kd = *(uint64_t *)(k + 0x10); k = kd ? *(uint64_t *)(kd + 0xb0) : 0; }
    e->filled = (uint8_t)f;
    uint32_t fl = *(uint32_t *)(d + 0x18);
    e->flags = (fl & 0x00400000u ? DM_RECIPE_ETHEREAL : 0) | (fl & 0x04000000u ? DM_RECIPE_RUNEWORD : 0);
    e->level = (uint8_t)*(uint32_t *)(d + 0x38);
    e->row = (uint16_t)(*(uint32_t *)(d + 0x34) & 0xffff);
    uint8_t v = *(uint8_t *)(it + 0x1bd); uint64_t t = v >= 1 && v <= 3 ? *(uint64_t *)(b + 0x2d64a18 + (uint64_t)v * 16) : 0, ir = t ? *(uint64_t *)(t + 0x15c0) : 0;
    uint32_t cls = *(uint32_t *)(it + 4), in = t ? *(uint32_t *)(t + 0x15c8) : 0;
    if (ir && cls < in) { if (*(uint8_t *)(ir + (uint64_t)cls * 0x1c0 + 0x13a)) e->flags |= DM_RECIPE_NO_WORD; }
    else e->flags |= DM_RECIPE_UNKNOWN;
}

static page_t *cache_slot(uint64_t key) {
    page_t *freeslot = NULL;
    for (int i = 0; i < MAX_CACHE; i++) { if (g_cache[i].key == key) return &g_cache[i]; if (!g_cache[i].key && !freeslot) freeslot = &g_cache[i]; }
    return freeslot;
}

static int walk(uint8_t *b, uint64_t pl, dm_recipe_item *list, int max, uint64_t *shared_key) {
    uint64_t us[3]; dm_rw_owners(b, pl, us); int n = 0, pcur = dm_pages_cur();
    page_t *own = g_named ? cache_slot((uint64_t)pcur + 1) : NULL, *shared = NULL;
    if (us[2]) { *shared_key = 1ull << 63 | *(uint32_t *)(us[2] + 8); shared = g_named ? cache_slot(*shared_key) : NULL; } else *shared_key = 0;
    if (own) { own->key = (uint64_t)pcur + 1; own->n = 0; }
    if (shared) { shared->key = *shared_key; shared->n = 0; }
    for (int o = 0; o < 3; o++) {
        uint64_t c = us[o] ? *(uint64_t *)(us[o] + 0x90) : 0, it = c && *(uint32_t *)c == 0x01020304 ? *(uint64_t *)(c + 0x10) : 0;
        for (int guard = 0; it && guard < 4096; guard++) {
            uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            if (*(uint32_t *)it == 4 && *(uint32_t *)(it + 0xc) == 0) {
                int pg = *(uint8_t *)(d + 0x55), st = *(uint8_t *)(d + 0x9c); dm_recipe_item e; read_item(b, it, d, &e);
                if (st) { e.place = DM_RECIPE_STACKS; e.qty = (uint16_t)st; e.unit = *(uint32_t *)(us[o] + 8); n = add(list, n, max, &e); }
                else if (o) { e.place = DM_RECIPE_SHARED; e.unit = *(uint32_t *)(us[o] + 8); n = add(list, n, max, &e); if (shared && shared->n < PAGE_ITEMS) shared->it[shared->n++] = e; }
                else if (pg == PG_INV) { e.place = DM_RECIPE_INV; n = add(list, n, max, &e); }
                else if (pg == PG_CUBE) { e.place = DM_RECIPE_CUBE; n = add(list, n, max, &e); }
                else if (pg == PG_STASH) { e.place = DM_RECIPE_STASH; n = add(list, n, max, &e); if (own && own->n < PAGE_ITEMS) own->it[own->n++] = e; }
            }
            it = *(uint64_t *)(d + 0xb0);
        }
    }
    return n;
}

typedef struct { dm_recipe_item *list; int n, max; } sink_t;
static void pouch_item(uint32_t code, void *arg) { sink_t *s = arg; dm_recipe_item e; memset(&e, 0, sizeof e); e.code = code; e.qty = 1; e.place = DM_RECIPE_POUCH; e.quality = 2; s->n = add(s->list, s->n, s->max, &e); }
static void vault_item(int kind, int idx, void *arg) { sink_t *s = arg; dm_recipe_item e; memset(&e, 0, sizeof e); e.qty = 1; e.place = DM_RECIPE_VAULT; e.quality = kind ? 7 : 5; e.row = (uint16_t)idx; e.flags = DM_RECIPE_UNKNOWN; s->n = add(s->list, s->n, s->max, &e); }

#ifdef DM_RECIPE_TESTS


static int test_stock(dm_recipe_item *list, int max) {
    const char *f = getenv("DM_TEST_RECIPE_STOCK"); FILE *fp = f ? fopen(f, "r") : NULL; if (!fp) return -1;
    char ln[128], code[8]; int n = 0, q, p, ql, s, fi, fl, r;
    while (n < max && fgets(ln, sizeof ln, fp)) if (sscanf(ln, "%7s %d %d %d %d %d %d %d", code, &q, &p, &ql, &s, &fi, &fl, &r) == 8) {
        dm_recipe_item e; memset(&e, 0, sizeof e); char c4[4] = { ' ', ' ', ' ', ' ' }; memcpy(c4, code, strlen(code) > 4 ? 4 : strlen(code)); memcpy(&e.code, c4, 4);
        e.qty = (uint16_t)q; e.place = (uint8_t)p; e.quality = (uint8_t)ql; e.sockets = (uint8_t)s; e.filled = (uint8_t)fi; e.flags = (uint8_t)fl; e.row = (uint16_t)r; list[n++] = e; }
    fclose(fp); return n;
}
static int test_busy(void) { return getenv("DM_TEST_RECIPE_BUSY") != NULL; }
#else
static int test_stock(dm_recipe_item *list, int max) { (void)list; (void)max; return -1; }
static int test_busy(void) { return 0; }
#endif


static int read_side(void) {
    static dm_recipe_item t[MAX_ITEMS]; int stale = 0; sink_t s = { t, 0, MAX_ITEMS };
    if (!test_busy() && dm_pouch_each(pouch_item, &s) == 0) { g_npouch = s.n > MAX_ITEMS ? MAX_ITEMS : s.n; memcpy(g_pouch, t, (size_t)g_npouch * sizeof *t); } else stale |= 1;
    s.n = 0;
    if (!test_busy() && dm_vault_each(vault_item, &s) == 0) { g_nvault = s.n > MAX_ITEMS ? MAX_ITEMS : s.n; memcpy(g_vault, t, (size_t)g_nvault * sizeof *t); } else stale |= 2;
    return stale;
}



#define MAX_STEPS 24
#define WAIT_NS 2000000000ull
void dm_rw_send(uint8_t *b, const uint8_t *p, int n); uint64_t dm_rw_item(uint8_t *b, uint32_t id); uint64_t dm_rw_held(uint8_t *b, uint64_t pl); void dm_rw_take_stack(uint8_t *b, uint64_t it); int dm_rw_panel_open(uint8_t *b);
void dm_held_stow(void); int dm_held_stow_result(void);
int dm_ui_panel_shown(const char *name);
void dm_item_use(uint32_t item_id);
enum { J_NONE, J_PANEL, J_PICK, J_HAND, J_PLACED, J_OPEN, J_WATCH, J_BACK, J_BACK_WAIT, J_STOW };
#define SETTLE_NS 10000000000ull
static struct { int k, n, tries, reason, stack_before, sent; uint64_t until, settle, G; uint32_t cur, cube, pid, epoch; int area; char who[64]; dm_recipe_step s[MAX_STEPS]; } g_job;
static _Atomic int g_jst;
static dm_recipe_step g_new[MAX_STEPS]; static int g_nnew; static uint32_t g_newcube, g_newgen; static _Atomic int g_newjob, g_abort, g_reason;

static uint64_t ns_now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
static void job_set(int st) { g_job.until = ns_now() + WAIT_NS; atomic_store(&g_jst, st); }

int dm_recipes_busy(void) { return atomic_load(&g_newjob) != 0 || atomic_load(&g_jst) != 0; }
void dm_recipes_abort(void) { atomic_store(&g_abort, 1); }
int dm_recipes_insert_state(void) { return atomic_load(&g_result) | atomic_load(&g_reason) << 8; }
int dm_recipes_insert(const int *plan, int steps, uint32_t cube_id, uint32_t gen) {
    int free_ = 0;
    if (steps <= 0 || steps > MAX_STEPS || atomic_load(&g_jst) || atomic_load(&g_newjob)) return 0;
    if (!dm_itemjob_acquire(DM_ITEMJOB_RECIPE)) return 0;
    atomic_store(&g_abort, 0);
    if (!atomic_compare_exchange_strong(&g_newjob, &free_, 1)) { dm_itemjob_release(DM_ITEMJOB_RECIPE); return 0; }
    memcpy(g_new, plan, (size_t)steps * sizeof(dm_recipe_step)); g_nnew = steps; g_newcube = cube_id; g_newgen = gen;
    atomic_store(&g_reason, 0); atomic_store(&g_result, DM_RECIPE_RUNNING); atomic_store(&g_newjob, 2); return 1;
}
static void job_done(int result, int reason) { atomic_store(&g_reason, reason); atomic_store(&g_result, result); atomic_store(&g_jst, J_NONE); dm_itemjob_release(DM_ITEMJOB_RECIPE); }


static int same_run(uint64_t pl) {
    dm_hud_info h; dm_hud_get(&h); char who[64];
    if (!pl || !h.valid || h.dead || dm_diag_epoch() != g_job.epoch || dm_diag_game() != g_job.G || h.area != g_job.area || *(uint32_t *)(pl + 8) != g_job.pid) return 0;
    uint32_t m = *(uint32_t *)(pl + 0xc); if (m == 0 || m == 17) return 0;
    return dm_pages_char(who, sizeof who) != 0 || !strcmp(who, g_job.who);
}
static int near_stash(uint8_t *b) {
    uint64_t G = dm_diag_game(), sp = dm_diag_player();
    return G && sp ? ((int (*)(uint64_t, uint64_t, int, int))(b + 0x650d10))(G, sp, 0x10b, 0x32) & 1 : 0;
}
static int stack_left(uint64_t it) { uint64_t d = it ? *(uint64_t *)(it + 0x10) : 0; return d ? *(uint8_t *)(d + 0x9c) : 0; }
static int on_unit(uint8_t *b, uint64_t pl, uint32_t unit) { uint64_t us[3]; dm_rw_owners(b, pl, us); for (int i = 1; i < 3; i++) if (us[i] && *(uint32_t *)(us[i] + 8) == unit) return 1; return 0; }

static int step_ok(uint8_t *b, uint64_t pl, const dm_recipe_step *s, int want) {
    uint64_t it = dm_rw_item(b, s->id), d = it ? *(uint64_t *)(it + 0x10) : 0;
    if (!d || *(uint32_t *)it != 4 || *(uint32_t *)(it + 0xc) != 0) return DM_RECIPE_WHY_GONE;
    if (((uint32_t (*)(uint64_t))(b + 0x570260))(it) != (uint32_t)s->code || (int)*(uint32_t *)d != s->quality) return DM_RECIPE_WHY_GONE;
    { uint64_t pp = *(uint64_t *)(it + 0x38); if (!pp || *(int32_t *)(pp + 0x10) != s->x || *(int32_t *)(pp + 0x14) != s->y) return DM_RECIPE_WHY_GONE; }
    if ((s->kind == 0 || s->kind == 3) && *(uint8_t *)(d + 0x55) != s->page) return DM_RECIPE_WHY_GONE;
    if ((s->kind == 1 || s->kind == 2) && !on_unit(b, pl, s->unit)) return DM_RECIPE_WHY_GONE;
    if (s->kind == 1 ? *(uint8_t *)(d + 0x9c) < want : *(uint8_t *)(d + 0x9c) != 0) return DM_RECIPE_WHY_GONE;
    if ((s->kind == 1 || s->kind == 2 || s->page == PG_STASH) && s->kind != 3 && !near_stash(b)) return DM_RECIPE_WHY_FAR;
    return 0;
}

static int cube_ok(uint8_t *b) {
    uint64_t it = dm_rw_item(b, g_job.cube), d = it ? *(uint64_t *)(it + 0x10) : 0;
    return d && *(uint32_t *)it == 4 && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == PG_INV && ((uint32_t (*)(uint64_t))(b + 0x570260))(it) == 0x20786f62;
}
static void pick(uint8_t *b, const dm_recipe_step *s, uint64_t it) {
    if (s->kind == 1) dm_rw_take_stack(b, it);
    else if (s->kind == 2) { uint8_t p[13] = { 0x46 }; uint32_t v[3] = { (uint32_t)s->id, s->unit, (uint32_t)s->x | (uint32_t)s->y << 16 }; memcpy(p + 1, v, 12); dm_rw_send(b, p, 13); }
    else { uint8_t p[17] = { 0x19 }; uint32_t v[4] = { (uint32_t)s->id, (uint32_t)s->x, (uint32_t)s->y, (uint32_t)s->page }; memcpy(p + 1, v, 16); dm_rw_send(b, p, 17); }
}
static void place(uint8_t *b, uint32_t id, int page, int x, int y) { uint8_t p[17] = { 0x18 }; uint32_t v[4] = { id, (uint32_t)x, (uint32_t)y, (uint32_t)page }; memcpy(p + 1, v, 16); dm_rw_send(b, p, 17); }
static int placed(uint8_t *b, uint64_t pl, uint32_t id, int page, int x, int y) {
    uint64_t it = dm_rw_item(b, id), d = it ? *(uint64_t *)(it + 0x10) : 0, pp = it ? *(uint64_t *)(it + 0x38) : 0;
    return d && pp && !dm_rw_held(b, pl) && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == page && *(int32_t *)(pp + 0x10) == x && *(int32_t *)(pp + 0x14) == y;
}




static int at_home(uint64_t pl, const dm_recipe_step *s, uint32_t id, int *x, int *y) {
    uint64_t c = pl ? *(uint64_t *)(pl + 0x90) : 0, it = c && *(uint32_t *)c == 0x01020304 ? *(uint64_t *)(c + 0x10) : 0;
    for (int guard = 0; it && guard < 4096; guard++) {
        uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if (*(uint32_t *)(it + 8) == id) {
            int pg = *(uint8_t *)(d + 0x55); uint64_t pp = *(uint64_t *)(it + 0x38);
            if (*(uint32_t *)it != 4 || *(uint32_t *)(it + 0xc) != 0 || !(pg == PG_INV || (s->kind == 0 && pg == s->page))) return -1;
            *x = pp ? *(int32_t *)(pp + 0x10) : -1; *y = pp ? *(int32_t *)(pp + 0x14) : -1; return pg; }
        it = *(uint64_t *)(d + 0xb0);
    }
    return -1;
}


static int hand_is_ours(uint8_t *b, uint64_t h, const dm_recipe_step *s) {
    if (g_job.cur) return *(uint32_t *)(h + 8) == g_job.cur;
    if (s->kind != 1) return *(uint32_t *)(h + 8) == (uint32_t)s->id;
    return ((uint32_t (*)(uint64_t))(b + 0x570260))(h) == (uint32_t)s->code && stack_left(dm_rw_item(b, (uint32_t)s->id)) == g_job.stack_before - 1;
}


static int accounted(uint8_t *b, uint64_t pl, const dm_recipe_step *s) {
    int x, y;
    if (g_job.cur) return placed(b, pl, g_job.cur, s->to_page, s->to_x, s->to_y) || at_home(pl, s, g_job.cur, &x, &y) >= 0;
    if (!g_job.sent) return 1;
    uint64_t it = dm_rw_item(b, (uint32_t)s->id), pp = it ? *(uint64_t *)(it + 0x38) : 0;
    if (!it || *(uint32_t *)(it + 0xc) != 0) return 0;
    if (s->kind == 1) return stack_left(it) == g_job.stack_before;
    return pp && *(int32_t *)(pp + 0x10) == s->x && *(int32_t *)(pp + 0x14) == s->y;
}
static int settle_over(void) { return ns_now() > g_job.settle; }
static void job_lost(void) {
    LOG("【红】配方书：手上空了，那件既不在原处也不在背包里（id %u）", g_job.cur);
    LOG("ASSUME itemjob.settle: recipe item of step %d unaccounted for at the wind-down limit", g_job.k + 1);
    job_done(DM_RECIPE_HELD, DM_RECIPE_WHY_LOST);
}
static void job_held(int why) { LOG("【红】配方书：光标上的东西不碰，留在手上（放入魔盒停在第 %d 步）", g_job.k + 1); job_done(DM_RECIPE_HELD, why); }
static void job_fail(int why, const char *text) {
    LOG("【红】配方书：放入魔盒停在第 %d / %d 步：%s", g_job.k + 1, g_job.n, text);
    g_job.reason = why; g_job.tries = 0; g_job.settle = ns_now() + SETTLE_NS; job_set(J_WATCH);
}
static int job_test_fail(int point) {
#ifdef DM_RECIPE_TESTS
    const char *e = getenv("DM_TEST_RECIPE_FAIL"); FILE *f = e ? fopen(e, "r") : NULL; int k = 0; char pts[16] = ""; if (!f) return 0;
    int n = fscanf(f, "%d:%15s", &k, pts); fclose(f);
    return n == 2 && k == g_job.k + 1 && strchr(pts, '0' + point);
#else
    (void)point; return 0;
#endif
}
static void job_pump(uint8_t *b, uint64_t pl) {
    if (atomic_load(&g_newjob) == 2) {
        memset(&g_job, 0, sizeof g_job); memcpy(g_job.s, g_new, sizeof g_new); g_job.n = g_nnew; g_job.cube = g_newcube;
        dm_hud_info h; dm_hud_get(&h); g_job.G = dm_diag_game(); g_job.epoch = dm_diag_epoch(); g_job.area = h.area; g_job.pid = pl ? *(uint32_t *)(pl + 8) : 0; strlcpy(g_job.who, g_char, sizeof g_job.who);
        job_set(J_PANEL); atomic_store(&g_newjob, 0);
        if (atomic_exchange(&g_abort, 0)) { LOG("配方书：放入魔盒还没开始就被打断，什么都没动"); job_done(DM_RECIPE_FAILED, DM_RECIPE_WHY_ABORT); return; }

        int why = !pl ? DM_RECIPE_WHY_STALE : 0;
#ifdef DM_RECIPE_TESTS
        if (g_newgen)
#endif
        if (g_newgen != g_head.gen || g_read_epoch != g_job.epoch || strcmp(g_read_who, g_char)) why = DM_RECIPE_WHY_STALE;
        if (!why && !same_run(pl)) why = DM_RECIPE_WHY_STALE;
        if (!why && dm_rw_held(b, pl)) why = DM_RECIPE_WHY_HAND;
        for (int i = 0; i < g_job.n && !why; i++) { int want = 0;
            for (int k = 0; k < g_job.n; k++) want += g_job.s[k].kind == 1 && g_job.s[k].id == g_job.s[i].id;
            why = step_ok(b, pl, &g_job.s[i], want); }
        if (!why && g_job.cube && !cube_ok(b)) why = DM_RECIPE_WHY_GONE;
        if (why) { LOG("配方书：放入魔盒没开始，原因 %d，什么都没动", why); job_done(DM_RECIPE_FAILED, why); return; }
        if (!dm_rw_panel_open(b) && g_job.cube) dm_item_use(g_job.cube);
    }
    int st = atomic_load(&g_jst); if (!st) return;
    uint64_t h = pl ? dm_rw_held(b, pl) : 0; dm_recipe_step *s = &g_job.s[g_job.k]; int late = ns_now() > g_job.until;
    if (st < J_OPEN) {
        if (atomic_exchange(&g_abort, 0)) { job_fail(DM_RECIPE_WHY_ABORT, "被打断"); return; }
        if (!same_run(pl)) { job_fail(DM_RECIPE_WHY_RUN, "角色或这一局变了"); return; }
    }
    switch (st) {
    case J_PANEL: if (dm_rw_panel_open(b)) job_set(J_PICK); else if (late) { atomic_store(&g_jst, J_NONE); job_done(DM_RECIPE_FAILED, DM_RECIPE_WHY_PANEL); } return;
    case J_PICK: { while (g_job.k < g_job.n && g_job.s[g_job.k].kind == 3) g_job.k++;
        s = &g_job.s[g_job.k];
        if (g_job.k >= g_job.n) { LOG("配方书：放入魔盒完成，%d 步", g_job.n);
            job_set(J_OPEN); return; }
        if (h) { job_held(DM_RECIPE_WHY_HAND); return; }
        if (!dm_rw_panel_open(b)) { job_fail(DM_RECIPE_WHY_PANEL, "物品面板关了"); return; }
        int why = step_ok(b, pl, s, 1); if (why || job_test_fail(1)) { job_fail(why ? why : DM_RECIPE_WHY_GONE, "这一件变了"); return; }
        g_job.cur = 0; g_job.stack_before = stack_left(dm_rw_item(b, (uint32_t)s->id));
        pick(b, s, dm_rw_item(b, (uint32_t)s->id)); g_job.sent = 1; job_set(J_HAND); return; }
    case J_HAND: {
        if (!h) { if (late) job_fail(DM_RECIPE_WHY_LOST, "2 秒没到光标上"); return; }
        if (!hand_is_ours(b, h, s)) { job_held(DM_RECIPE_WHY_HAND); return; }
        g_job.cur = *(uint32_t *)(h + 8); g_job.sent = 0;
        if (job_test_fail(2)) { job_fail(DM_RECIPE_WHY_LOST, "测试：拿到手上后当作出错"); return; }
        place(b, g_job.cur, s->to_page, s->to_x, s->to_y); job_set(J_PLACED); return; }
    case J_PLACED:
        if (placed(b, pl, g_job.cur, s->to_page, s->to_x, s->to_y)) { g_job.k++; g_job.cur = 0; g_job.sent = 0; job_set(J_PICK); return; }
        if (late) job_fail(DM_RECIPE_WHY_LOST, "2 秒没放好"); return;
    case J_OPEN: {
        static uint64_t next; uint64_t now = ns_now();
        if (!g_job.cube || dm_ui_panel_shown("HoradricCubeLayout") || late) { if (g_job.cube && late) LOG("配方书：魔盒 2 秒没开"); job_done(DM_RECIPE_DONE, 0); return; }
        if (!same_run(pl) || !cube_ok(b)) { LOG("配方书：魔盒不对了，不开"); job_done(DM_RECIPE_DONE, 0); return; }
        if (now >= next) { dm_item_use(g_job.cube); next = now + 400000000ull; }
        return; }
    case J_WATCH:

        if (h && !hand_is_ours(b, h, s)) { job_held(DM_RECIPE_WHY_HAND); return; }
        if (h) { g_job.cur = *(uint32_t *)(h + 8); g_job.sent = 0; job_set(J_BACK); return; }
        if (!late) return;
        if (g_job.sent ? settle_over() && accounted(b, pl, s) : accounted(b, pl, s)) { if (g_job.sent) LOG("配方书：step %d: the pick was never answered, the item is still at its source", g_job.k + 1); job_done(DM_RECIPE_FAILED, g_job.reason); return; }
        if (settle_over()) job_lost(); return;
    case J_BACK: { int test = job_test_fail(3);
        if (!pl) { job_done(DM_RECIPE_FAILED, g_job.reason); return; }
        if (!h) { job_set(J_BACK_WAIT); return; }
        if (!hand_is_ours(b, h, s)) { job_held(DM_RECIPE_WHY_HAND); return; }
        if (!test) { if (s->kind == 0 || s->kind == 3) place(b, g_job.cur, s->page, s->x, s->y); else place(b, g_job.cur, 0, s->back_x, s->back_y); }
        job_set(J_BACK_WAIT); return; }
    case J_BACK_WAIT: { int back = s->kind == 0 || s->kind == 3, x = -1, y = -1, pg;
        if (h && !hand_is_ours(b, h, s)) { job_held(DM_RECIPE_WHY_HAND); return; }
        if (placed(b, pl, g_job.cur, back ? s->page : 0, back ? s->x : s->back_x, back ? s->y : s->back_y)) { LOG("配方书：手上那件已放回"); job_done(DM_RECIPE_FAILED, g_job.reason); return; }

        if (!h && (pg = at_home(pl, s, g_job.cur, &x, &y)) >= 0) { LOG("配方书：手上那件没回原格，在%s的 %d,%d（id %u）", pg == PG_INV ? "背包" : "来源页", x, y, g_job.cur); job_done(DM_RECIPE_FAILED, g_job.reason); return; }
        if (!h && placed(b, pl, g_job.cur, s->to_page, s->to_x, s->to_y)) { LOG("配方书：step %d: the item reached its cell in the cube after all (id %u)", g_job.k + 1, g_job.cur); job_done(DM_RECIPE_FAILED, g_job.reason); return; }
        if (!h) { if (settle_over()) job_lost(); return; }
        if (!late) return;
        if (++g_job.tries < 2) { job_set(J_BACK); return; }
        dm_held_stow(); job_set(J_STOW); return; }
    case J_STOW: {
        if (h && !hand_is_ours(b, h, s)) { job_held(DM_RECIPE_WHY_HAND); return; }
        int x = -1, y = -1, pg = h ? -1 : at_home(pl, s, g_job.cur, &x, &y);
        if (pg >= 0) { LOG("配方书：手上那件%s的 %d,%d（id %u）", pg == PG_INV ? "收进了背包" : "回到了来源页", x, y, g_job.cur); job_done(DM_RECIPE_FAILED, g_job.reason); return; }
        if (!h && placed(b, pl, g_job.cur, s->to_page, s->to_x, s->to_y)) { LOG("配方书：step %d: the item reached its cell in the cube after all (id %u)", g_job.k + 1, g_job.cur); job_done(DM_RECIPE_FAILED, g_job.reason); return; }
        if (!h) { if (settle_over()) job_lost(); return; }
        if (!late) return;
        LOG("【红】配方书：手上那件放不回去，留在手上（id %u）", g_job.cur); job_done(DM_RECIPE_HELD, DM_RECIPE_WHY_HELD); return; }
    }
}

#ifdef DM_RECIPE_TESTS


static void test_move(uint8_t *b, uint64_t pl) {
    static int tick; if (++tick % 30 || dm_recipes_busy()) return;
    const char *f = getenv("DM_TEST_RECIPE_MOVE"); FILE *fp = f ? fopen(f, "r") : NULL; if (!fp) return;
    char code[8]; int from, to, x, y, ok = fscanf(fp, "%7s %d %d %d %d", code, &from, &to, &x, &y) == 5; fclose(fp); unlink(f); if (!ok) return;
    if (!strcmp(code, "dump")) { static dm_recipe_item l[256]; uint64_t k; int n = walk(b, pl, l, 256, &k); uint64_t G = dm_diag_game(), sp = dm_diag_player();
        { dm_hud_info h; dm_hud_get(&h); uint64_t ob = 0; int sx = 0, sy = 0; for (int k = 0; k < 128 && !ob; k++) for (uint64_t u = *(uint64_t *)(b + 0x2d603a0 + 2 * 0x400 + 8 * (uint64_t)k); u; u = *(uint64_t *)(u + 0x158)) if (*(uint32_t *)(u + 4) == 267) { ob = u; uint64_t pp = *(uint64_t *)(u + 0x38); if (pp) { sx = *(int32_t *)(pp + 0x10); sy = *(int32_t *)(pp + 0x14); } break; }
          LOG("测试：位置 %d %d 箱 %d %d", h.x, h.y, sx, sy); }
        LOG("测试：人在箱旁 %d，箱子开着 %d，手上 %d，物品面板 %d", G && sp ? ((int (*)(uint64_t, uint64_t, int, int))(b + 0x650d10))(G, sp, 0x10b, 0x32) & 1 : -1, *(uint8_t *)(b + 0x2d5ff60) != 0, dm_rw_held(b, pl) != 0, dm_rw_panel_open(b));
        for (int i = 0; i < n && i < 256; i++) LOG("测试：%.4s 地方 %d 格 %d,%d id %u 品质 %d 件 %d", (char *)&l[i].code, l[i].place, l[i].x, l[i].y, l[i].id, l[i].quality, l[i].qty);
        return; }
    uint64_t us[3]; dm_rw_owners(b, pl, us); uint64_t c = *(uint64_t *)(pl + 0x90), it = c ? *(uint64_t *)(c + 0x10) : 0;
    char c4[4] = { ' ', ' ', ' ', ' ' }; memcpy(c4, code, strlen(code) > 4 ? 4 : strlen(code)); uint32_t want; memcpy(&want, c4, 4);
    uint64_t G = dm_diag_game(), sp = dm_diag_player(); int near = G && sp ? ((int (*)(uint64_t, uint64_t, int, int))(b + 0x650d10))(G, sp, 0x10b, 0x32) & 1 : -1;
    for (int g = 0; it && g < 4096; g++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == from && ((uint32_t (*)(uint64_t))(b + 0x570260))(it) == want) {
            uint64_t pp = *(uint64_t *)(it + 0x38); dm_recipe_step s = { *(uint32_t *)(it + 8), 0, from, pp ? *(int32_t *)(pp + 0x10) : 0, pp ? *(int32_t *)(pp + 0x14) : 0, 0, to, x, y, (int)want, (int)*(uint32_t *)d, 0, 0 };
            LOG("测试 DM_TEST_RECIPE_MOVE：%s 页 %d → 页 %d (%d,%d)，人在箱旁 %d，箱子开着 %d，物品面板 %d", code, from, to, x, y, near, *(uint8_t *)(b + 0x2d5ff60) != 0, dm_rw_panel_open(b));
            dm_recipes_insert((const int *)&s, 1, 0, 0); return; }
        it = *(uint64_t *)(d + 0xb0); }
    LOG("测试 DM_TEST_RECIPE_MOVE：没有页 %d 的 %s", from, code);
}
#endif











#define TEXT_RECS 256
#define TEXT_MAX 4096
#define TEXT_PER_FRAME 3
extern _Atomic int g_dm_lang;
int dm_rw_text(uint8_t *b, int rec, char *out, size_t n);
static char *g_text[TEXT_RECS]; static pthread_mutex_t g_tmu = PTHREAD_MUTEX_INITIALIZER;
static char g_tdir[900]; static _Atomic int g_twant, g_tprio; static int g_tnext, g_tlang = -1, g_tdone, g_tcount; static uint32_t g_tepoch;
void dm_recipes_text_start(const char *dir) { pthread_mutex_lock(&g_tmu); strlcpy(g_tdir, dir, sizeof g_tdir); pthread_mutex_unlock(&g_tmu); atomic_store(&g_twant, 1); }
int dm_recipes_text(int rec, char *out, int bytes) {
    int n = 0, have = 0; if (rec < 0 || rec >= TEXT_RECS || bytes <= 0) return 0;
    pthread_mutex_lock(&g_tmu);
    if (g_text[rec] && g_tlang == atomic_load(&g_dm_lang)) { have = 1; n = (int)strlen(g_text[rec]); if (n >= bytes) n = bytes - 1; memcpy(out, g_text[rec], (size_t)n); }
    pthread_mutex_unlock(&g_tmu);
    if (!have) atomic_store(&g_tprio, rec + 1);
    return n;
}
static void text_path(char *p, size_t n, const char *ext) { pthread_mutex_lock(&g_tmu); snprintf(p, n, "%s/recipe_text_%d_d3_3.3.94012.tsv%s", g_tdir, g_tlang, ext); pthread_mutex_unlock(&g_tmu); }
static void text_put(int rec, const char *s) { char *c = strdup(s); pthread_mutex_lock(&g_tmu); free(g_text[rec]); g_text[rec] = c; pthread_mutex_unlock(&g_tmu); }
static void text_clear(void) { pthread_mutex_lock(&g_tmu); for (int i = 0; i < TEXT_RECS; i++) { free(g_text[i]); g_text[i] = NULL; } pthread_mutex_unlock(&g_tmu); g_tnext = 0; g_tcount = 0; g_tdone = 0; }
#define TEXT_PRE "#recipe text 3.3.94012 "
#define TEXT_HEAD TEXT_PRE "%d\n"

static int text_want(const char *ln) {
    size_t k = strlen(TEXT_PRE); if (strncmp(ln, TEXT_PRE, k) || ln[k] < '0' || ln[k] > '9') return 0;
    char *e; long v = strtol(ln + k, &e, 10); return *e == '\n' && !e[1] && v > 0 && v <= TEXT_RECS ? (int)v : 0;
}

static int text_load(void) {
    char p[1000]; text_path(p, sizeof p, ""); FILE *f = fopen(p, "r"); if (!f) return 0;
    static char ln[TEXT_MAX + 32]; static uint8_t seen[TEXT_RECS]; int n = 0, want = fgets(ln, sizeof ln, f) ? text_want(ln) : 0, bad = !want;
    memset(seen, 0, sizeof seen);
    while (!bad && fgets(ln, sizeof ln, f)) {
        char *t, *e = strchr(ln, '\n'); long rec = strtol(ln, &t, 10);
        if (t == ln || *t != '\t' || !e || rec < 0 || rec >= TEXT_RECS || seen[rec] || !t[1] || t[1] == '\n') { bad = 1; break; }
        seen[rec] = 1; t++; *e = 0; for (char *q = t; *q; q++) if (*q == 1) *q = '\n'; text_put((int)rec, t); n++; }
    bad |= ferror(f) != 0;
    fclose(f); if (bad || n != want) { LOG("配方书：说明文字文件不完整（%d / %d 条），重做", n, want); text_clear(); return 0; }
    return n;
}

static void text_save(void) {
    char p[1000], q[1010]; text_path(p, sizeof p, ""); text_path(q, sizeof q, ".part"); FILE *f = fopen(q, "w"); if (!f) return; static char ln[TEXT_MAX];
    int ok = fprintf(f, TEXT_HEAD, g_tcount) > 0;
    for (int i = 0; i < TEXT_RECS && ok; i++) { pthread_mutex_lock(&g_tmu); if (g_text[i]) strlcpy(ln, g_text[i], sizeof ln); else ln[0] = 0; pthread_mutex_unlock(&g_tmu); if (!ln[0]) continue; for (char *c = ln; *c; c++) if (*c == '\n') *c = 1; ok = fprintf(f, "%d\t%s\n", i, ln) > 0; }
    ok = fflush(f) == 0 && ok; ok = fsync(fileno(f)) == 0 && ok; ok = fclose(f) == 0 && ok;
    if (ok) ok = rename(q, p) == 0;
    if (!ok) { unlink(q); LOG("【红】配方书：说明文字写不下去"); }
}
static void text_pump(uint8_t *b, uint64_t pl) {
    uint32_t epoch = dm_diag_epoch();
    if ((g_tnext || g_tcount) && !g_tdone && epoch != g_tepoch) { text_clear(); atomic_store(&g_twant, 0); LOG("配方书：这一局结束了，说明文字重做"); }
    if (!pl || !atomic_load(&g_twant) || dm_recipes_busy()) return;
    if (*(uint8_t *)(pl + 0x1bd) != 3) { atomic_store(&g_twant, 0); return; }
    int lang = atomic_load(&g_dm_lang); g_tepoch = epoch;
    if (lang != g_tlang) { text_clear(); pthread_mutex_lock(&g_tmu); g_tlang = lang; pthread_mutex_unlock(&g_tmu);
        int n = text_load(); if (n) { g_tdone = 1; LOG("配方书：说明文字读入 %d 条", n); atomic_store(&g_twant, 0); return; } }
    if (g_tdone) { atomic_store(&g_twant, 0); return; }
    static char d[TEXT_MAX]; int failed = 0, k = 0, pr = atomic_exchange(&g_tprio, 0) - 1;
    if (pr >= 0 && pr < TEXT_RECS && !g_text[pr]) { int r = dm_rw_text(b, pr, d, sizeof d); if (r > 0) { text_put(pr, d); g_tcount++; k++; } else if (r < 0) failed = 1; }
    for (; !failed && k < TEXT_PER_FRAME && g_tnext < TEXT_RECS; g_tnext++) { if (g_text[g_tnext]) continue;
        int r = dm_rw_text(b, g_tnext, d, sizeof d); if (r > 0) { text_put(g_tnext, d); g_tcount++; k++; } else if (r < 0) failed = 1; }
    if (failed) { LOG("【红】配方书：说明文字有一条做不出来，这次不存"); text_clear(); atomic_store(&g_twant, 0); return; }
    if (g_tnext >= TEXT_RECS) { text_save(); g_tdone = 1; atomic_store(&g_twant, 0); LOG("配方书：说明文字做完，%d 条", g_tcount); }
}














#define ITEM_SLOTS 64
#define ITEM_MAX 4096
enum { ITEM_EMPTY, ITEM_TEXT, ITEM_NONE };
typedef struct { int key, state; uint32_t used; char *text; } item_slot;
static item_slot g_items[ITEM_SLOTS];
static pthread_mutex_t g_imu = PTHREAD_MUTEX_INITIALIZER;
static _Atomic int g_ireq;
static uint32_t g_igen = 1, g_iuse;
static int g_ilang = -1; static uint32_t g_iepoch;

static int item_key(int kind, int row, int grp) { return kind < 0 || kind > 2 || row < 0 || row >= (1 << 20) || grp < -1 || grp > 2 ? -1 : kind << 24 | (grp + 1) << 20 | row; }
static void item_clear_locked(void) { for (int i = 0; i < ITEM_SLOTS; i++) { free(g_items[i].text); g_items[i] = (item_slot){ 0, ITEM_EMPTY, 0, NULL }; } g_igen++; }

static void item_sync_locked(int lang, uint32_t epoch) {
    if (lang != g_ilang || epoch != g_iepoch) { item_clear_locked(); g_ilang = lang; g_iepoch = epoch; }
}
int dm_item_text(int kind, int row, int grp, char *out, int bytes) {
    int key = item_key(kind, row, grp), n = 0, found = 0; if (key < 0 || !out || bytes < 2) return DM_ITEM_TEXT_NONE;
    pthread_mutex_lock(&g_imu);
    item_sync_locked(atomic_load(&g_dm_lang), dm_diag_epoch());
    for (int i = 0; i < ITEM_SLOTS; i++) { item_slot *s = &g_items[i]; if (s->state == ITEM_EMPTY || s->key != key) continue;
        found = 1; s->used = ++g_iuse;
        if (s->state == ITEM_NONE) n = DM_ITEM_TEXT_NONE; else { n = (int)strlen(s->text); if (n >= bytes) n = bytes - 1; memcpy(out, s->text, (size_t)n); out[n] = 0; }
        break; }
    pthread_mutex_unlock(&g_imu);
    if (!found) atomic_store(&g_ireq, key + 1);
    return n;
}
static void item_put_locked(int key, int state, const char *text) {
    item_slot *s = NULL;
    for (int i = 0; i < ITEM_SLOTS && !s; i++) if (g_items[i].state != ITEM_EMPTY && g_items[i].key == key) s = &g_items[i];
    for (int i = 0; i < ITEM_SLOTS && !s; i++) if (g_items[i].state == ITEM_EMPTY) s = &g_items[i];
    if (!s) { s = &g_items[0]; for (int i = 1; i < ITEM_SLOTS; i++) if (g_items[i].used < s->used) s = &g_items[i]; }
    free(s->text); s->text = state == ITEM_TEXT ? strdup(text) : NULL;
    s->key = key; s->state = state == ITEM_TEXT && !s->text ? ITEM_EMPTY : state; s->used = ++g_iuse;
}
#ifdef DM_RECIPE_TESTS
static void item_test(uint8_t *b, uint64_t pl);
#endif
static void item_pump(uint8_t *b, uint64_t pl) {
    int lang = atomic_load(&g_dm_lang);
    pthread_mutex_lock(&g_imu); item_sync_locked(lang, dm_diag_epoch()); uint32_t gen = g_igen; pthread_mutex_unlock(&g_imu);
#ifdef DM_RECIPE_TESTS
    if (pl) item_test(b, pl);
#endif
    int want = atomic_load(&g_ireq);
    if (!want || !pl || dm_recipes_busy() || *(uint8_t *)(pl + 0x1bd) != 3) return;
    if (!atomic_compare_exchange_strong(&g_ireq, &want, 0)) return;
    int key = want - 1; static char d[ITEM_MAX];
    int r = dm_item_desc(b, key >> 24, key & 0xfffff, ((key >> 20) & 15) - 1, d, sizeof d);
    if (r == DM_ITEM_DESC_LATER) { int none = 0; atomic_compare_exchange_strong(&g_ireq, &none, want); return; }
    pthread_mutex_lock(&g_imu);
    if (gen == g_igen && lang == g_ilang) item_put_locked(key, r > 0 ? ITEM_TEXT : ITEM_NONE, d);
    else { int none = 0; atomic_compare_exchange_strong(&g_ireq, &none, want); }
    pthread_mutex_unlock(&g_imu);
}
#ifdef DM_RECIPE_TESTS



static void item_test(uint8_t *b, uint64_t pl) {
    static int loaded, n, at, cur = -1; static int list[2048][4];
    if (!loaded) { loaded = 1; const char *f = getenv("DM_TEST_ITEM_TEXT"); FILE *fp = f ? fopen(f, "r") : NULL; char ln[64];
        while (fp && n < 2048 && fgets(ln, sizeof ln, fp)) { int k, a, z = -1, g = -1; int got = sscanf(ln, "%d %d-%d %d", &k, &a, &z, &g);
            if (got < 2) continue; if (got == 2 || z < a) { z = a; if (sscanf(ln, "%d %d %d", &k, &a, &g) < 3) g = -1; }
            list[n][0] = k; list[n][1] = a; list[n][2] = z; list[n][3] = g; n++; }
        if (fp) { fclose(fp); LOG("物品说明测试：%d 行", n); } }
    if (at >= n || dm_recipes_busy() || *(uint8_t *)(pl + 0x1bd) != 3) return;
    if (cur < list[at][1]) cur = list[at][1];
    static char d[ITEM_MAX]; int r = dm_item_desc(b, list[at][0], cur, list[at][3], d, sizeof d);
    if (r == DM_ITEM_DESC_LATER) return;
    for (char *q = d; *q; q++) if (*q == '\n') *q = '|';
    LOG("物品说明测试：kind %d row %d grp %d lang %d -> %d: %s", list[at][0], cur, list[at][3], atomic_load(&g_dm_lang), r, r > 0 ? d : "-");
    if (++cur > list[at][2]) { at++; cur = -1; if (at >= n) LOG("物品说明测试：完 | item text test: done"); }
}
#endif

void dm_recipes_pump(uint8_t *b, uint64_t pl) {
    job_pump(b, pl);
    text_pump(b, pl);
    item_pump(b, pl);
#ifdef DM_RECIPE_TESTS
    if (pl) test_move(b, pl);
#endif
    char who[sizeof g_char];
    g_named = !test_busy() && dm_pages_char(who, sizeof who) == 0 && who[0];
    if (g_named && strcmp(who, g_char)) { strlcpy(g_char, who, sizeof g_char); memset(g_cache, 0, sizeof g_cache); g_npouch = g_nvault = 0; }
    if (!pl) return;
    static int tick; uint64_t shared_key = 0;
    int want = atomic_exchange(&g_req, 0), stale = 0;
    if (!want) { if (++tick % 30 || !*(uint8_t *)(b + 0x2d5ff60)) return; walk(b, pl, g_work, MAX_ITEMS, &shared_key); return; }
    int n = walk(b, pl, g_work, MAX_ITEMS, &shared_key), pcur = dm_pages_cur(), seen = 0;
    int test = test_stock(g_work, MAX_ITEMS);
    if (test >= 0) n = test;
    for (int i = 0; i < MAX_CACHE && test < 0 && g_named; i++) { page_t *p = &g_cache[i];
        if (!p->key || p->key == (uint64_t)pcur + 1 || p->key == shared_key) { if (p->key && p->key <= 32) seen |= 1 << (p->key - 1); continue; }
        if (p->key <= 32) seen |= 1 << (p->key - 1);
        for (int k = 0; k < p->n; k++) { dm_recipe_item e = p->it[k]; e.place = DM_RECIPE_OTHER_PAGE; n = add(g_work, n, MAX_ITEMS, &e); } }
    sink_t s = { g_work, n, MAX_ITEMS };
    if (test < 0 || test_busy()) {
        stale = read_side() | (g_named ? 0 : 4);
        for (int i = 0; i < g_npouch; i++) s.n = add(g_work, s.n, MAX_ITEMS, &g_pouch[i]);
        for (int i = 0; i < g_nvault; i++) s.n = add(g_work, s.n, MAX_ITEMS, &g_vault[i]);
    }
    uint64_t G = dm_diag_game(), sp = dm_diag_player();
    pthread_mutex_lock(&g_mu);
    strlcpy(g_read_who, g_char, sizeof g_read_who); g_read_epoch = dm_diag_epoch();
    g_head.near = G && sp ? ((int (*)(uint64_t, uint64_t, int, int))(b + 0x650d10))(G, sp, 0x10b, 0x32) & 1 : 0;
    g_head.stash_open = *(uint8_t *)(b + 0x2d5ff60) != 0; g_head.insert = (uint8_t)atomic_load(&g_result);
    { int dm_unsocket_ready(void); g_head.flags = dm_unsocket_ready() ? DM_RECIPE_UNSOCKET_READY : 0; }
    g_head.n = (uint32_t)(s.n > MAX_ITEMS ? MAX_ITEMS : s.n); memcpy(g_out, g_work, g_head.n * sizeof(dm_recipe_item));
    g_head.truncated = s.n > MAX_ITEMS; g_head.data_version = *(uint8_t *)(pl + 0x1bd);
    g_head.difficulty = G ? *(uint8_t *)(uintptr_t)(G + 0x104) : 0xff; g_head.ladder_free = G ? *(uint8_t *)(uintptr_t)(G + 0x101) != 0 : 0;
    g_head.page = (uint8_t)pcur; g_head.pages = (uint8_t)dm_pages_count(); g_head.seen = (uint8_t)seen; g_head.stale = (uint8_t)stale; g_head.gen = (uint32_t)want;
    pthread_mutex_unlock(&g_mu);
    LOG("配方书：库存 %d 件（截断 %d，数据版本 %d，难度 %d，G+0x101 非 0：%d，旧读数 %d）", s.n, g_head.truncated, g_head.data_version, g_head.difficulty, g_head.ladder_free, stale);
}
