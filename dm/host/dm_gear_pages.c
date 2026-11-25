












#include "dm_gear_pages.h"
#include "dm.h"
#include "dm_itembuild.h"
#include "hle.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int dm_item_write(uint8_t *b, uint64_t it, uint8_t *buf);
uint64_t dm_item_make_all(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out, int *bytes_out);
uint64_t dm_item_make_all_r(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, dm_ib_result *r);
int dm_stash_pages(uint64_t G, uint64_t player, uint64_t pg[8], uint32_t id[8]);
uint64_t dm_unit_inv(uint64_t u);
int dm_pages_slot_n(int page);
int dm_pages_slot_item(int page, int i, const uint8_t **bits, int *len, int *x, int *y);
int dm_pages_shown(void);
int dm_gear_pages_profile(uint8_t *b, uint64_t pl, uint64_t it, int x, int y, int nodes, dm_gpg_prof *out);
uint64_t dm_gear_pages_psig(uint8_t *b, uint64_t pl);
uint32_t dm_gear_pages_sig(uint64_t it);
int dm_gear_pages_tables(void);

#define GP_PER_TICK 16
#define GP_CHECK_TICKS 10
#define GP_VBYTES 1024
#define GP_KIND_UNIT 0x756e6974ull
#define GP_OVER_NS (4 * DM_GEAR_FRAME_NS)

static dm_gpg_core *g_core;
static pthread_t g_thread; static int g_have_thread;
static uint64_t g_player, g_G;
static int g_ready;
static uint64_t g_psig;
static uint32_t g_tab[DM_GPG_TABS];
static atomic_uint g_ask, g_answered;
static uint64_t g_frame, g_frame_ns;

static uint64_t clock_cb(void *ctx) { (void)ctx; return now_ns(); }
static uint64_t client_player(uint8_t *b) { return ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)); }
static uint64_t inv_first(uint64_t u) { uint64_t inv = u ? *(uint64_t *)(u + 0x90) : 0; return inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; }
static uint64_t inv_next(uint64_t it) { uint64_t d = *(uint64_t *)(it + 0x10); return d ? *(uint64_t *)(d + 0xb0) : 0; }
static void item_free(uint8_t *b, uint64_t G, uint64_t it) { ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, it); }
static int item_skip(uint64_t it) { uint64_t d = *(uint64_t *)(it + 0x10); uint32_t fl = d ? *(uint32_t *)(d + 0x18) : 0; return !(fl & 0x10) || (fl & 0x100); }
static void item_cell(uint64_t it, int *x, int *y) { uint64_t pp = *(uint64_t *)(it + 0x38); *x = pp ? *(int32_t *)(pp + 0x10) : 0; *y = pp ? *(int32_t *)(pp + 0x14) : 0; }
static const char *place_name(int place) { return place < DM_GPG_PAGES ? "personal" : "shared"; }
static int place_no(int place) { return (place < DM_GPG_PAGES ? place : place - DM_GPG_PAGES) + 1; }

uint64_t dm_gear_frame_spent(void) { return g_frame == dm_guest_frame() ? g_frame_ns : 0; }
void dm_gear_frame_spend(uint64_t ns) { uint64_t f = dm_guest_frame(); if (f != g_frame) { g_frame = f; g_frame_ns = 0; } g_frame_ns += ns; }
unsigned dm_gear_pages_ask(void) { return atomic_fetch_add(&g_ask, 1) + 1; }
unsigned dm_gear_pages_asked(void) { return atomic_load(&g_ask); }
unsigned dm_gear_pages_answered(void) { return atomic_load(&g_answered); }


static uint64_t vhash(const uint8_t *bits, int len) {
    static uint8_t k[GP_VBYTES];
    if (len <= 0 || len > GP_VBYTES) return dm_gpg_fnv(bits, len > 0 ? (size_t)len : 0, DM_GPG_FNV0);
    memcpy(k, bits, (size_t)len);
    if (len >= 7) { k[1] &= (uint8_t)~0x20; k[4] &= 0x07; k[5] = 0; k[6] &= (uint8_t)~0x1f; }
    return dm_gpg_fnv(k, (size_t)len, DM_GPG_FNV0);
}
typedef struct { uint8_t *b; uint64_t G, player, pl; int nodes, shown, nt; uint64_t pg[8]; uint32_t id[8]; } gp_ctx;


static int prof_bits(void *vx, const dm_gpg_in *in, uint64_t *hash, dm_gpg_prof *out) {
    gp_ctx *x = vx; int socks = 0, used = 0;
    *hash = vhash(in->src, in->len);
    uint64_t nu = dm_item_make_all(x->b, x->G, 0, in->src, in->len, &socks, &used);
    if (!nu) return -1;
    int rc = used != in->len ? -1 : item_skip(nu) ? 1 : dm_gear_pages_profile(x->b, x->pl, nu, in->x, in->y, x->nodes, out) ? -1 : 0;
    if (rc == 0) x->nodes = 0;
    item_free(x->b, x->G, nu);
    return rc;
}
static int hash_bits(void *vx, const dm_gpg_in *in, uint64_t *hash) { (void)vx; *hash = vhash(in->src, in->len); return 0; }

static int hash_unit(void *vx, const dm_gpg_in *in, uint64_t *hash) {
    gp_ctx *x = vx; static uint8_t buf[GP_VBYTES];
    int len = dm_item_write(x->b, (uint64_t)(uintptr_t)in->src, buf);
    if (!len) return -1;
    *hash = vhash(buf, len);
    return 0;
}
static int prof_unit(void *vx, const dm_gpg_in *in, uint64_t *hash, dm_gpg_prof *out) {
    gp_ctx *x = vx;
    if (hash_unit(vx, in, hash)) return -1;
    if (dm_gear_pages_profile(x->b, x->pl, (uint64_t)(uintptr_t)in->src, in->x, in->y, x->nodes, out)) return -1;
    x->nodes = 0;
    return 0;
}
static uint64_t unit_key(uint64_t it) {
    uint64_t v[2] = { GP_KIND_UNIT << 32 | *(uint32_t *)(it + 8), dm_gear_pages_sig(it) };
    return dm_gpg_fnv(v, sizeof v, DM_GPG_FNV0);
}

static int list_units(uint64_t owner, int grid_only, dm_gpg_in *in, int max, int *more) {
    int n = 0; *more = 0;
    uint64_t it = inv_first(owner);
    for (int guard = 0; it && guard < 4096; guard++, it = inv_next(it)) {
        uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if (*(uint32_t *)it != 4 || *(uint32_t *)(it + 0xc) != 0) continue;
        if (grid_only && *(uint8_t *)(d + 0x55) != 4) continue;
        if (n >= max) { ++*more; continue; }
        dm_gpg_in *e = &in[n++]; memset(e, 0, sizeof *e);
        item_cell(it, &e->x, &e->y); e->unit = *(uint32_t *)(it + 8); e->skip = item_skip(it); e->key = unit_key(it); e->src = (const void *)(uintptr_t)it;
    }
    return n;
}

static int list_slot(int page, dm_gpg_in *in, int max, int *total) {
    int n = dm_pages_slot_n(page), k = 0; *total = n;
    for (int i = 0; i < n && k < max; i++) {
        const uint8_t *bits; int len, x, y;
        if (dm_pages_slot_item(page, i, &bits, &len, &x, &y)) { *total = k; break; }
        dm_gpg_in *e = &in[k++]; memset(e, 0, sizeof *e);
        e->x = x; e->y = y; e->src = bits; e->len = len; e->key = dm_gpg_fnv(bits, (size_t)len, DM_GPG_FNV0);
    }
    return k;
}

static int shared_tabs(uint64_t G, uint64_t player, uint64_t pg[8], uint32_t id[8]) {
    int n = dm_stash_pages(G, player, pg, id);
    for (int i = 1; i < n; i++) for (int j = i; j > 0 && id[j] < id[j - 1]; j--) { uint32_t t = id[j]; id[j] = id[j - 1]; id[j - 1] = t; uint64_t u = pg[j]; pg[j] = pg[j - 1]; pg[j - 1] = u; }
    return n > DM_GPG_TABS ? DM_GPG_TABS : n;
}

static int gp_list(void *vx, int place, dm_gpg_in *in, int max, dm_gpg_profile_fn *fn, dm_gpg_hash_fn *hash) {
    gp_ctx *x = vx; int more = 0;
    if (place < DM_GPG_PAGES) {
        if (x->shown < 0) return DM_GPG_UNLISTED;
        if (place == x->shown) { *fn = prof_unit; *hash = hash_unit; int n = list_units(x->player, 1, in, max, &more); return n + more; }
        int total = 0, n = list_slot(place, in, max, &total);
        if (total < 0) return DM_GPG_UNLISTED;
        *fn = prof_bits; *hash = hash_bits; return total > n ? total : n;
    }
    int t = place - DM_GPG_PAGES;
    if (t >= x->nt) return DM_GPG_ABSENT;
    *fn = prof_unit; *hash = hash_unit;
    int n = list_units(x->pg[t], 0, in, max, &more);
    return n + more;
}

#ifndef DM_PLAYER_BUILD
static void qa_tick(uint8_t *b, uint64_t G, uint64_t player, uint64_t pl, uint32_t tq);
static void pin_tick(uint8_t *b, uint64_t G, uint64_t player);
static int qa_idle(void);
#endif

void dm_gear_pages_tick(uint64_t G, uint64_t player, uint8_t *b) {
    if (!g_have_thread) { g_thread = pthread_self(); g_have_thread = 1; }
    if (!g_core && !(g_core = dm_gpg_new())) return;
    if (player != g_player) { g_player = player; dm_gpg_reset(g_core); g_ready = 0; }
    if (!player || !b) return;
    g_G = G;
    uint32_t tq = *(uint32_t *)(G + 0x170);
    uint64_t pl = client_player(b);
    unsigned ask = atomic_load(&g_ask); dm_gpg_ask(g_core, ask);
#ifndef DM_PLAYER_BUILD
    pin_tick(b, G, player);
#endif
    static uint32_t last; static int going;
    int every = GP_CHECK_TICKS;
#ifndef DM_PLAYER_BUILD
    if (qa_idle()) every = 0;
#endif
    if (pl && dm_gear_pages_tables() && (going || (every && tq - last >= (uint32_t)every) || !g_ready || dm_gpg_answered(g_core) != ask)) {
        static dm_gpg_in in[DM_GPG_PLACE_ITEMS];


        static dm_gpg_budget bud = { GP_PER_TICK, DM_GEAR_FRAME_NS, clock_cb, NULL, 0, { 200000, 50000, 500000 }, 0, 0, 0 };
        bud.spent = dm_gear_frame_spent(); dm_gpg_budget_start(&bud);
        uint64_t psig = dm_gear_pages_psig(b, pl);
        static gp_ctx x; memset(&x, 0, sizeof x);
        x.b = b; x.G = G; x.player = player; x.pl = pl; x.nodes = 1; x.shown = dm_pages_shown(); x.nt = shared_tabs(G, player, x.pg, x.id);
        g_psig = psig; g_ready = 1;
        for (int t = 0; t < DM_GPG_TABS; t++) g_tab[t] = t < x.nt ? x.id[t] : 0;
        dm_gpg_pass_out o; dm_gpg_pass(g_core, psig, gp_list, &x, in, DM_GPG_PLACE_ITEMS, &bud, &o);
        going = !o.swept; if (o.swept) last = tq;
        atomic_store(&g_answered, dm_gpg_answered(g_core));
        uint64_t used = dm_gpg_budget_used(&bud), own = used - bud.spent;
        dm_gear_frame_spend(own);
        static int said_over, over, swept_once; static uint64_t worst;
        if (own > worst) worst = own;
        over += used > DM_GEAR_FRAME_NS;

        if (own > GP_OVER_NS && swept_once && said_over < 8) { said_over++;
            LOG("ASSUME bestgear.pages_budget: one server tick took %.2f ms on the stash page stock (%d places listed, %d profiles; frame limit %.1f ms)", own / 1e6, o.listed, bud.made, DM_GEAR_FRAME_NS / 1e6); }
        swept_once |= o.swept;
        for (int p = 0; p < DM_GPG_PLACES; p++) if (o.unreadable >> p & 1) { static int said; if (said++ < 8) LOG("ASSUME bestgear.pages: %s stash %s %d unreadable", place_name(p), p < DM_GPG_PAGES ? "page" : "tab", place_no(p)); }
        static uint64_t rev_logged; static int said;
        dm_gpg_state st; dm_gpg_state_get(g_core, &st);
        if (o.swept && st.rev != rev_logged && !st.missing && said < 8) {
            said++; rev_logged = st.rev;
            LOG("best gear pages: %d places read (%#x), %d candidates, %d left out; longest tick so far %.2f ms, %d ticks over the frame's %.1f ms", __builtin_popcount(st.present), st.present, st.items, st.skipped, worst / 1e6, over, DM_GEAR_FRAME_NS / 1e6);
        }
    }
#ifndef DM_PLAYER_BUILD
    qa_tick(b, G, player, pl, tq);
#endif
}

static void check_thread(void) {
    static int said;
    if (g_have_thread && !pthread_equal(g_thread, pthread_self()) && !said++) LOG("ASSUME bestgear.pages_thread: the stock is read on another thread than the server tick");
}
void dm_gear_pages_state(dm_gpg_state *st) {
    check_thread();
    if (!g_core) { memset(st, 0, sizeof *st); return; }
    dm_gpg_state_get(g_core, st);
}
int dm_gear_pages_get(dm_gpg_item *out, int max, dm_gpg_state *st) {
    dm_gear_pages_state(st);
    return g_core ? dm_gpg_items(g_core, out, max) : 0;
}
int dm_gear_pages_info(dm_gpg_state *st, uint64_t *psig, uint32_t tab[DM_GPG_TABS]) {
    dm_gear_pages_state(st);
    if (!g_core || !g_ready) return 0;
    *psig = g_psig; memcpy(tab, g_tab, sizeof g_tab);
    return 1;
}
int dm_gear_pages_unit_hash(uint32_t unit, uint64_t *hash) { check_thread(); return g_core && g_ready ? dm_gpg_unit_hash(g_core, unit, hash) : 0; }
uint32_t dm_gear_pages_row_name(uint32_t unit, uint64_t *vhash) { check_thread(); if (g_core && g_ready) return dm_gpg_row_name(g_core, unit, vhash); *vhash = 0; return unit; }

int dm_gear_pages_with(uint8_t *b, int place, int x, int y, uint64_t hash, void (*fn)(uint64_t it, void *arg), void *arg) {
    check_thread();
    if (!b || !g_player || !g_G || place < 0 || place >= DM_GPG_PLACES) return -1;
    static dm_gpg_in in[DM_GPG_PLACE_ITEMS]; static uint8_t buf[GP_VBYTES];
    int n = 0, more = 0, shown = dm_pages_shown(), slot = place < DM_GPG_PAGES && place != shown;
    if (place < DM_GPG_PAGES) { if (shown < 0) return -1; int total = 0; n = slot ? list_slot(place, in, DM_GPG_PLACE_ITEMS, &total) : list_units(g_player, 1, in, DM_GPG_PLACE_ITEMS, &more); }
    else { uint64_t pg[8]; uint32_t id[8]; int nt = shared_tabs(g_G, g_player, pg, id); if (place - DM_GPG_PAGES >= nt) return -1; n = list_units(pg[place - DM_GPG_PAGES], 0, in, DM_GPG_PLACE_ITEMS, &more); }
    for (int i = 0; i < n; i++) {
        if (in[i].x != x || in[i].y != y || in[i].skip) continue;
        if (!slot) {
            uint64_t it = (uint64_t)(uintptr_t)in[i].src; int len = dm_item_write(b, it, buf);
            if (!len || vhash(buf, len) != hash) return -1;
            fn(it, arg); return 0;
        }
        if (vhash(in[i].src, in[i].len) != hash) return -1;
        int socks = 0, used = 0; uint64_t nu = dm_item_make_all(b, g_G, 0, in[i].src, in[i].len, &socks, &used);
        if (!nu) return -1;
        if (used == in[i].len) fn(nu, arg);
        item_free(b, g_G, nu);
        return used == in[i].len ? 0 : -1;
    }
    return -1;
}


uint64_t dm_gear_pages_bits_vhash(const uint8_t *bits, int len) { return vhash(bits, len); }

int dm_gear_pages_unit_vhash(uint8_t *b, uint32_t id, uint64_t *hash) {
    check_thread();
    if (!b || !g_player || !g_G || !id) return -1;
    uint64_t found = 0, inv = dm_unit_inv(g_player), cur = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0;
    if (cur && *(uint32_t *)cur == 4 && *(uint32_t *)(cur + 8) == id) found = cur;
    for (uint64_t it = inv_first(g_player); it && !found; it = inv_next(it)) if (*(uint32_t *)it == 4 && *(uint32_t *)(it + 8) == id) found = it;
    if (!found) { uint64_t pg[8]; uint32_t ids[8]; int nt = shared_tabs(g_G, g_player, pg, ids);
        for (int t = 0; t < nt && !found; t++) for (uint64_t it = inv_first(pg[t]); it && !found; it = inv_next(it)) if (*(uint32_t *)it == 4 && *(uint32_t *)(it + 8) == id) found = it; }
    if (!found) return -1;
    static uint8_t buf[GP_VBYTES]; int len = dm_item_write(b, found, buf);
    if (!len) return -1;
    *hash = vhash(buf, len);
    return 0;
}

int dm_gear_pages_slot_sig(int page, uint64_t *sig) {
    if (page == dm_pages_shown()) return -1;
    int n = dm_pages_slot_n(page); if (n < 0) return -1;
    uint64_t h = DM_GPG_FNV0;
    for (int i = 0; i < n; i++) { const uint8_t *bits; int len, x, y; if (dm_pages_slot_item(page, i, &bits, &len, &x, &y)) return -1; h = dm_gpg_fnv(bits, (size_t)len, h); }
    *sig = h;
    return 0;
}
int dm_gear_pages_slot_items(uint8_t *b, uint64_t G, int page, void (*fn)(uint64_t it, void *arg), void *arg) {
    check_thread();
    if (!b || !G || page == dm_pages_shown()) return -1;
    int n = dm_pages_slot_n(page); if (n < 0) return -1;
    for (int i = 0; i < n; i++) {
        const uint8_t *bits; int len, x, y, socks = 0, used = 0;
        if (dm_pages_slot_item(page, i, &bits, &len, &x, &y)) return -1;
        uint64_t nu = dm_item_make_all(b, G, 0, bits, len, &socks, &used);
        if (!nu) return -1;
        if (used == len) fn(nu, arg);
        item_free(b, G, nu);
        if (used != len) return -1;
    }
    return 0;
}


static void unit_size(uint8_t *b, uint64_t it, int *w, int *h) {
    *w = *h = 0; uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
    if (t && ci < *(uint32_t *)(t + 0x15c8)) { uint64_t rec = *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0; *w = *(uint8_t *)(rec + 0x11e); *h = *(uint8_t *)(rec + 0x11f); }
}
int dm_gear_pages_bits_size(uint8_t *b, const uint8_t *bits, int len, int *w, int *h) {
    check_thread();
    if (!b || !g_G) return -1;
    int socks = 0, used = 0; uint64_t nu = dm_item_make_all(b, g_G, 0, bits, len, &socks, &used);
    if (!nu) return -1;
    unit_size(b, nu, w, h);
    item_free(b, g_G, nu);
    return used == len && *w > 0 && *h > 0 ? 0 : -1;
}

int dm_gear_pages_tab_ids(uint32_t id[DM_GPG_TABS]) { if (!g_player || !g_G) return 0; uint64_t pg[8]; uint32_t ids[8]; int nt = shared_tabs(g_G, g_player, pg, ids); for (int t = 0; t < nt; t++) id[t] = ids[t]; return nt; }



static int u64cmp(const void *a, const void *b) { uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b; return x < y ? -1 : x > y; }
int dm_gear_pages_census(uint8_t *b, int *count, uint64_t *hash) {
    check_thread();
    if (!b || !g_player || !g_G || dm_pages_shown() < 0) return -1;
    static uint64_t h[4096]; static uint8_t buf[GP_VBYTES]; int n = 0;
    uint64_t owners[1 + 8]; int no = 0; owners[no++] = g_player;
    { uint64_t pg[8]; uint32_t ids[8]; int nt = shared_tabs(g_G, g_player, pg, ids); for (int t = 0; t < nt; t++) owners[no++] = pg[t]; }
    for (int o = 0; o < no; o++) {
        for (uint64_t it = inv_first(owners[o]); it && n < 4096; it = inv_next(it)) { if (*(uint32_t *)it != 4) continue; int len = dm_item_write(b, it, buf); if (!len) return -1; h[n++] = vhash(buf, len); }
        uint64_t inv = dm_unit_inv(owners[o]), cur = !o && inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0;
        if (cur && *(uint32_t *)cur == 4 && n < 4096) { int dup = 0; for (uint64_t it = inv_first(owners[o]); it; it = inv_next(it)) dup |= it == cur; if (!dup) { int len = dm_item_write(b, cur, buf); if (!len) return -1; h[n++] = vhash(buf, len); } }
    }
    for (int p = 0; p < DM_GPG_PAGES; p++) {
        if (p == dm_pages_shown()) continue;
        int m = dm_pages_slot_n(p); if (m < 0) return -1;
        for (int i = 0; i < m && n < 4096; i++) { const uint8_t *bits; int len, x, y; if (dm_pages_slot_item(p, i, &bits, &len, &x, &y)) return -1; h[n++] = vhash(bits, len); }
    }
    qsort(h, (size_t)n, sizeof h[0], u64cmp);
    *count = n; *hash = dm_gpg_fnv(h, (size_t)n * sizeof h[0], DM_GPG_FNV0);
    return 0;
}

#ifndef DM_PLAYER_BUILD
















static uint32_t code_of(uint8_t *b, uint64_t it) { return ((uint32_t (*)(uint64_t))(b + 0x570260))(it); }
static void code_str(uint32_t c, char out[5]) { memcpy(out, &c, 4); out[4] = 0; for (int i = 3; i >= 0 && (out[i] == ' ' || !out[i]); i--) out[i] = 0; }
static int count_units(uint64_t table) {
    int n = 0;
    for (int k = 0; k < 128; k++) for (uint64_t u = *(uint64_t *)(table + 8 * (uint64_t)k), g = 0; u && g < 100000; u = *(uint64_t *)(u + 0x158), g++) n += *(uint32_t *)u == 4;
    return n;
}
int dm_guest_heap_free_bytes(unsigned long long *free_bytes);
static int prof_same(const dm_gpg_prof *a, const dm_gpg_prof *b) {
    dm_gear_cand ca = a->c, cb = b->c; ca.item.id = cb.item.id = 0;
    return a->kind == b->kind && a->hasw == b->hasw && !memcmp(&ca, &cb, sizeof ca) && (!a->hasw || !memcmp(&a->w, &b->w, sizeof a->w)) &&
           a->show.quality == b->show.quality && !strcmp(a->show.code, b->show.code) && !strcmp(a->show.name, b->show.name);
}
#define QA_ITEMS 1024
static dm_gpg_item g_qa_all[QA_ITEMS]; static uint64_t g_qa_rev; static int g_qa_n;
static void qa_items(void) { dm_gpg_state st; dm_gpg_state_get(g_core, &st); if (st.rev != g_qa_rev || !g_qa_rev) { g_qa_rev = st.rev; g_qa_n = dm_gpg_items(g_core, g_qa_all, QA_ITEMS); } }
static const dm_gpg_item *stock_item(int place, int x, int y) {
    qa_items(); const dm_gpg_item *all = g_qa_all; int n = g_qa_n;
    int where = place < DM_GPG_PAGES ? DM_GPG_PERSONAL : DM_GPG_SHARED, index = place < DM_GPG_PAGES ? place : place - DM_GPG_PAGES;
    for (int i = 0; i < n; i++) if (all[i].where == where && all[i].index == index && all[i].x == x && all[i].y == y) return &all[i];
    return NULL;
}
static void qa_log_state(uint8_t *b) {
    (void)b;
    qa_items(); const dm_gpg_item *all = g_qa_all; int n = g_qa_n; dm_gpg_state st; dm_gpg_state_get(g_core, &st);
    LOG("[gear-pages] state rev %llu present %#x missing %#x unreadable %#x candidates %d skipped %d", (unsigned long long)st.rev, st.present, st.missing, st.unreadable, st.items, st.skipped);
    for (int i = 0; i < n; i++) { const dm_gpg_item *e = &all[i];
        LOG("[gear-pages] item %s %d cell %d,%d code %s kind %d quality %d hash %016llx unit %u unrated %#x weapon %d name \"%s\"", e->where == DM_GPG_PERSONAL ? "personal" : "shared", e->index + 1,
            e->x, e->y, e->p.show.code, e->p.kind, e->p.show.quality, (unsigned long long)e->hash, e->unit, e->p.c.unrated, e->p.hasw, e->p.show.name); }
}
#define QA_DELTAS 16384
static long long g_qa_delta[QA_DELTAS], g_qa_ctrl[QA_DELTAS];
typedef struct { int ndelta; uint64_t last_ns; int done, built, same, differ, rewrite_same, rewrite_flag, rewrite_differ, failed, socketed; uint64_t ns, ns_max; int srv0, cli0; unsigned long long heap0; } probe_t;
static int qa_idle(void) { static int v = -1; if (v < 0) { const char *e = getenv("DM_TEST_GEAR_PAGES_IDLE"); v = e && !strcmp(e, "1"); } return v; }
static void probe_place(uint8_t *b, uint64_t G, uint64_t player, uint64_t pl, int place, int shown, probe_t *P, int first) {
    static dm_gpg_in in[DM_GPG_PLACE_ITEMS]; static uint8_t buf[GP_VBYTES], again[GP_VBYTES]; static dm_gpg_prof prof;
    uint64_t pg[8]; uint32_t id[8]; int nt = shared_tabs(G, player, pg, id), more = 0, total = 0, n, slot = place < DM_GPG_PAGES && place != shown;
    if (place < DM_GPG_PAGES) n = slot ? list_slot(place, in, DM_GPG_PLACE_ITEMS, &total) : list_units(player, 1, in, DM_GPG_PLACE_ITEMS, &more);
    else if (place - DM_GPG_PAGES < nt) n = list_units(pg[place - DM_GPG_PAGES], 0, in, DM_GPG_PLACE_ITEMS, &more);
    else return;
    for (int i = 0; i < n; i++) {
        if (in[i].skip) continue;
        const uint8_t *bits = in[i].src; int len = in[i].len;
        if (!slot) { len = dm_item_write(b, (uint64_t)(uintptr_t)in[i].src, buf); bits = buf; if (!len) { P->failed++; continue; } }

        unsigned long long c0 = 0, c1 = 0, h0 = 0, h1 = 0; dm_guest_heap_free_bytes(&c0);
        for (uint64_t w0 = now_ns(), span = P->last_ns ? P->last_ns : 50000; now_ns() - w0 < span;) { }
        dm_guest_heap_free_bytes(&c1); if (P->ndelta < QA_DELTAS) g_qa_ctrl[P->ndelta] = (long long)c1 - (long long)c0;
        dm_guest_heap_free_bytes(&h0);
        uint64_t t0 = now_ns(); dm_ib_result ir;
        uint64_t nu = dm_item_make_all_r(b, G, 0, bits, len, &ir); int socks = ir.socked, used = ir.bytes;
        if (!nu || used != len) { P->failed++; if (nu) item_free(b, G, nu); continue; }
        P->socketed += socks > 0;
        int rc = item_skip(nu) ? 1 : dm_gear_pages_profile(b, pl, nu, in[i].x, in[i].y, 1, &prof);
        if (first) { int l2 = dm_item_write(b, nu, again), flag = 0;


            if (l2 == len && len > 1 && memcmp(again, bits, (size_t)len)) {
                static uint8_t allow[GP_VBYTES]; memset(allow, 0, (size_t)len); allow[1] = 1;
                for (int k = 0; k < ir.socked && k < DM_IB_AT; k++) if (ir.at[k] + 1 < len) allow[ir.at[k] + 1] = 1;
                flag = 1;
                for (int k = 0; k < len && flag; k++) if (again[k] != bits[k]) flag = allow[k] && again[k] == (uint8_t)(bits[k] & ~0x20);
            }
            if (l2 == len && !memcmp(again, bits, (size_t)len)) P->rewrite_same++;
            else if (flag) P->rewrite_flag++;
            else { P->rewrite_differ++; char cs[5]; code_str(code_of(b, nu), cs); int at = -1; for (int k = 0; k < len && k < l2 && at < 0; k++) if (again[k] != bits[k]) at = k;
                LOG("[gear-pages-probe] %s %d cell %d,%d %s written again: %d bytes against %d, first difference at byte %d", place_name(place), place_no(place), in[i].x, in[i].y, cs, l2, len, at); } }
        item_free(b, G, nu);
        uint64_t ns = now_ns() - t0; P->last_ns = ns;
        dm_guest_heap_free_bytes(&h1); if (P->ndelta < QA_DELTAS) g_qa_delta[P->ndelta++] = (long long)h1 - (long long)h0; P->ns += ns; if (ns > P->ns_max) P->ns_max = ns; P->built++;
        const dm_gpg_item *s = rc ? NULL : stock_item(place, in[i].x, in[i].y);
        if ((rc == 1 || (rc == 0 && prof.kind == DM_GEAR_K_NONE)) && !s) P->same++;
        else if (s && prof_same(&prof, &s->p)) P->same++;
        else P->differ++;
    }
}
static void probe_tick(uint8_t *b, uint64_t G, uint64_t player, uint64_t pl, uint32_t tq) {
    static int rounds = -1, ran; static uint32_t ready_at;
    if (rounds < 0) { const char *e = getenv("DM_TEST_GEAR_PAGES_PROBE"); rounds = e ? atoi(e) : 0; }
    if (rounds <= 0 || ran || !pl) return;
    dm_gpg_state st; dm_gpg_state_get(g_core, &st);
    if (st.missing || !st.present) { ready_at = 0; return; }
    if (!ready_at) { ready_at = tq; return; }
    if (tq - ready_at < 25) return;
    ran = 1;


    static probe_t P; int shown = dm_pages_shown();
    for (int place = 0; place < DM_GPG_PLACES; place++) if (st.present & (1u << place)) probe_place(b, G, player, pl, place, shown, &P, 1);
    memset(&P, 0, sizeof P);
    P.srv0 = count_units(G + 0x2e30); P.cli0 = count_units((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400)); dm_guest_heap_free_bytes(&P.heap0);
    LOG("[gear-pages-probe] start: %d rounds over present %#x, server item units %d, client item units %d, main heap free %llu", rounds, st.present, P.srv0, P.cli0, P.heap0);
    uint64_t t0 = now_ns();
    for (; P.done < rounds; P.done++)
        for (int place = 0; place < DM_GPG_PLACES; place++) if (st.present & (1u << place)) probe_place(b, G, player, pl, place, shown, &P, P.done == 0);
    uint64_t total = now_ns() - t0;

    int nd = P.ndelta, zero = 0, below = 0, czero = 0, cbelow = 0; long long med = 0, cmed = 0;
    for (int i = 1; i < nd; i++) for (int j = i; j > 0 && g_qa_delta[j] < g_qa_delta[j - 1]; j--) { long long t = g_qa_delta[j]; g_qa_delta[j] = g_qa_delta[j - 1]; g_qa_delta[j - 1] = t; }
    for (int i = 1; i < nd; i++) for (int j = i; j > 0 && g_qa_ctrl[j] < g_qa_ctrl[j - 1]; j--) { long long t = g_qa_ctrl[j]; g_qa_ctrl[j] = g_qa_ctrl[j - 1]; g_qa_ctrl[j - 1] = t; }
    for (int i = 0; i < nd; i++) { zero += !g_qa_delta[i]; below += g_qa_delta[i] < 0; czero += !g_qa_ctrl[i]; cbelow += g_qa_ctrl[i] < 0; }
    if (nd) { med = g_qa_delta[nd / 2]; cmed = g_qa_ctrl[nd / 2]; }
    unsigned long long heap1 = 0; dm_guest_heap_free_bytes(&heap1);
    int srv1 = count_units(G + 0x2e30), cli1 = count_units((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400));
    LOG("[gear-pages-probe] done: rounds %d built %d failed %d profiles same %d differ %d rewritten bitstreams same %d new-item flag only %d differ %d; with socket fillers %d; server item units %d -> %d, client item units %d -> %d, main heap free %llu -> %llu, per build change median %lld, unchanged %d, lower %d of %d, without a build median %lld, unchanged %d, lower %d; per build %.3f ms mean %.3f ms max, %.0f ms in all",
        P.done, P.built, P.failed, P.same, P.differ, P.rewrite_same, P.rewrite_flag, P.rewrite_differ, P.socketed, P.srv0, srv1, P.cli0, cli1, P.heap0, heap1, med, zero, below, nd, cmed, czero, cbelow, P.built ? P.ns / 1e6 / P.built : 0, P.ns_max / 1e6, total / 1e6);
}

static void item_cells(uint8_t *b, uint64_t it, int *w, int *h) {
    *w = *h = 1; uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
    if (t && ci < *(uint32_t *)(t + 0x15c8)) { uint64_t rec = *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0; *w = *(uint8_t *)(rec + 0x11e); *h = *(uint8_t *)(rec + 0x11f); }
}
static void put_tick(uint8_t *b, uint64_t G, uint64_t player, uint32_t tq) {
    static int parsed, n; static struct { uint32_t at; char code[5]; int shared, move, index, done, x, y; } q[8]; static const char *file;
    if (!parsed) { parsed = 1; const char *e = getenv("DM_TEST_STASH_PUT"); if (!e) return; char buf[256]; strlcpy(buf, e, sizeof buf);
        if (e[0] == '@') file = e + 1;
        else for (char *sv, *t = strtok_r(buf, ",", &sv); t && n < 8; t = strtok_r(NULL, ",", &sv)) {
            unsigned at; char code[5] = "", kind = 0; int idx = 0;
            int cx = 0, cy = 0, f = sscanf(t, "%u:%4[^:]:%c%d@%d,%d", &at, code, &kind, &idx, &cx, &cy);
            if ((f == 4 || f == 6) && (kind == 'p' || kind == 's' || kind == 'S') && idx >= 1 && cx >= 0 && cx < DM_GPG_GRID_W && cy >= 0 && cy < DM_GPG_GRID_H) {
                q[n].at = at; memcpy(q[n].code, code, 5); q[n].shared = kind != 'p'; q[n].move = kind == 'S'; q[n].index = idx - 1; q[n].x = cx; q[n].y = cy; n++; }
            else LOG("[stash-put] bad entry %s", t); } }
    if (file && n < 8) { FILE *fp = fopen(file, "r");
        if (fp) { char ln[64];
            while (n < 8 && fgets(ln, sizeof ln, fp)) { char code[5] = "", kind = 0; int idx = 0, dir = 0, cx = 0, cy = 0, f;
                if (sscanf(ln, "flip:%d", &dir) == 1 && (dir == 1 || dir == -1)) { void dm_pages_request_flip(int); dm_pages_request_flip(dir); LOG("[stash-put] page turn %+d asked", dir); continue; }
                if (((f = sscanf(ln, "%4[^:]:%c%d@%d,%d", code, &kind, &idx, &cx, &cy)) == 3 || f == 5) && (kind == 'p' || kind == 's' || kind == 'S') && idx >= 1 &&
                    cx >= 0 && cx < DM_GPG_GRID_W && cy >= 0 && cy < DM_GPG_GRID_H) {
                    q[n].at = 0; memcpy(q[n].code, code, 5); q[n].shared = kind != 'p'; q[n].move = kind == 'S'; q[n].index = idx - 1; q[n].done = 0; q[n].x = cx; q[n].y = cy; n++; }
                else LOG("[stash-put] bad line %s", ln); }
            fclose(fp); remove(file); LOG("[stash-put] %d entries read from the file", n); } }
    int dm_ser_state(void), dm_request_save(void), dm_pages_test_put(int page, const uint8_t *bits, int len, int x, int y), dm_itemjob_owner(void);
    if (dm_itemjob_owner()) return;
    for (int k = 0; k < n; k++) {
        if (q[k].done || tq < q[k].at || dm_ser_state()) continue;
        q[k].done = 1;
        uint64_t it = 0;
        for (uint64_t c = inv_first(player); c && !it; c = inv_next(c)) { uint64_t d = *(uint64_t *)(c + 0x10); char cs[5]; code_str(code_of(b, c), cs);
            if (*(uint32_t *)c == 4 && *(uint32_t *)(c + 0xc) == 0 && d && *(uint8_t *)(d + 0x55) == 0 && !item_skip(c) && !strcmp(cs, q[k].code)) it = c; }
        if (!it) { LOG("[stash-put] no identified %s in the inventory", q[k].code); continue; }
        { int w, h; item_cells(b, it, &w, &h);
          if (q[k].x + w > DM_GPG_GRID_W || q[k].y + h > DM_GPG_GRID_H) { LOG("[stash-put] %s (%d x %d) does not fit at cell %d,%d", q[k].code, w, h, q[k].x, q[k].y); continue; } }
        static uint8_t buf[GP_VBYTES]; int len = dm_item_write(b, it, buf);
        if (!len) { LOG("[stash-put] %s: no bitstream", q[k].code); continue; }
        int placed = 0;
        if (!q[k].shared) {
            if (q[k].index == dm_pages_shown() || dm_pages_slot_n(q[k].index) != 0) { LOG("[stash-put] personal page %d is shown or not empty", q[k].index + 1); continue; }
            placed = dm_pages_test_put(q[k].index, buf, len, q[k].x, q[k].y) == 0;
        } else {
            uint64_t pg[8]; uint32_t id[8]; int nt = shared_tabs(G, player, pg, id);
            if (q[k].index >= nt || inv_first(pg[q[k].index])) { LOG("[stash-put] shared tab %d missing or not empty (%d tabs)", q[k].index + 1, nt); continue; }
            if (q[k].move) {
                uint64_t d = *(uint64_t *)(it + 0x10), ip = *(uint64_t *)(it + 0x38); int x, y, pg0 = *(uint8_t *)(d + 0x55); item_cell(it, &x, &y);
                if (!ip) { LOG("[stash-put] %s has no cell", q[k].code); continue; }
                ((void (*)(uint64_t))(b + 0x55e430))(it);
                ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, it, 0x20, 4, 0, pg0, y, x);
                *(uint8_t *)(d + 0x55) = 4; *(int32_t *)(ip + 0x10) = q[k].x; *(int32_t *)(ip + 0x14) = q[k].y; uint64_t one = it, ox = 0;
                ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(b + 0x6f1940))(G, pg[q[k].index], &one, &ox);
                if (*(uint64_t *)(d + 0xa0)) { LOG("[stash-put] tick %u %s moved from inventory %d,%d to shared tab %d cell %d,%d; save requested -> %d", tq, q[k].code, x, y, q[k].index + 1, q[k].x, q[k].y, dm_request_save()); continue; }
                *(uint8_t *)(d + 0x55) = (uint8_t)pg0; *(int32_t *)(ip + 0x10) = x; *(int32_t *)(ip + 0x14) = y; one = it;
                ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(b + 0x6f1940))(G, player, &one, &ox);
                ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, uint64_t))(b + 0x642f70))(G, player, it, 1, 2, 0, pg0, 0, (uint64_t)x | (uint64_t)y << 16);
                LOG("[stash-put] %s could not be moved into shared tab %d, put back into the inventory (%s)", q[k].code, q[k].index + 1, *(uint64_t *)(d + 0xa0) ? "done" : "failed");
                continue;
            }
            int socks = 0, used = 0; uint64_t nu = dm_item_make_all(b, G, 0, buf, len, &socks, &used), nd = nu ? *(uint64_t *)(nu + 0x10) : 0, ip = nu ? *(uint64_t *)(nu + 0x38) : 0;
            if (nd && ip && used == len) { *(uint8_t *)(nd + 0x55) = 4; *(int32_t *)(ip + 0x10) = q[k].x; *(int32_t *)(ip + 0x14) = q[k].y; uint64_t one = nu, ox = 0;
                ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(b + 0x6f1940))(G, pg[q[k].index], &one, &ox); placed = *(uint64_t *)(nd + 0xa0) != 0; }
            if (!placed && nu) item_free(b, G, nu);
        }
        if (!placed) { LOG("[stash-put] %s could not be put into %s %d, it stays in the inventory", q[k].code, q[k].shared ? "shared tab" : "personal page", q[k].index + 1); continue; }
        uint64_t d = *(uint64_t *)(it + 0x10); int x, y; item_cell(it, &x, &y);
        ((void (*)(uint64_t))(b + 0x55e430))(it);
        ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, it, 0x20, 4, 0, *(uint8_t *)(d + 0x55), y, x);
        item_free(b, G, it);
        LOG("[stash-put] tick %u %s from inventory %d,%d to %s %d cell %d,%d (%d bytes); save requested -> %d", tq, q[k].code, x, y, q[k].shared ? "shared tab" : "personal page", q[k].index + 1, q[k].x, q[k].y, len, dm_request_save());
    }
}
static void pin_tick(uint8_t *b, uint64_t G, uint64_t player) {
    static int parsed, n, left; static struct { char code[5]; int index, x, y, done; } q[4];
    if (!parsed) { parsed = 1; const char *e = getenv("DM_TEST_STASH_PIN"); if (!e) return; char buf[128]; strlcpy(buf, e, sizeof buf);
        for (char *sv, *t = strtok_r(buf, ";", &sv); t && n < 4; t = strtok_r(NULL, ";", &sv)) {
            char code[5] = ""; int idx = 0, cx = 0, cy = 0;
            if (sscanf(t, "%4[^:]:s%d@%d,%d", code, &idx, &cx, &cy) == 4 && idx >= 1) { memcpy(q[n].code, code, 5); q[n].index = idx - 1; q[n].x = cx; q[n].y = cy; n++; }
            else LOG("[stash-pin] bad entry %s", t); }
        left = n; }
    if (!left) return;
    uint64_t pg[8]; uint32_t id[8]; int nt = shared_tabs(G, player, pg, id);
    for (int k = 0; k < n; k++) {
        if (q[k].done || q[k].index >= nt) continue;
        uint64_t it = 0; int x = 0, y = 0;
        for (uint64_t c = inv_first(pg[q[k].index]); c && !it; c = inv_next(c)) { char cs[5]; code_str(code_of(b, c), cs); item_cell(c, &x, &y);
            if (*(uint32_t *)c == 4 && x == q[k].x && y == q[k].y && !strcmp(cs, q[k].code)) it = c; }
        if (!it) continue;
        q[k].done = 1; left--;
        dm_test_give_stats_server(b, it, q[k].code);
        LOG("[stash-pin] tick %u %s at shared tab %d cell %d,%d pinned", *(uint32_t *)(G + 0x170), q[k].code, q[k].index + 1, q[k].x, q[k].y);
    }
}
static void qa_tick(uint8_t *b, uint64_t G, uint64_t player, uint64_t pl, uint32_t tq) {
    static int log = -1; static uint64_t logged;
    if (log < 0) { const char *e = getenv("DM_TEST_GEAR_PAGES"); log = e && !strcmp(e, "1"); }
    put_tick(b, G, player, tq);
    if (log) { dm_gpg_state st; dm_gpg_state_get(g_core, &st); if (st.rev != logged && !st.missing) { logged = st.rev; qa_log_state(b); } }
    probe_tick(b, G, player, pl, tq);
}
#endif
