




#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

static int checks, fails;
#define CHECK(c, msg, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: " msg " (line %d)\n", ##__VA_ARGS__, __LINE__); } } while (0)

static char g_log[4096];
#define SL_LOG(...) do { size_t _o = strlen(g_log); snprintf(g_log + _o, sizeof g_log - _o, __VA_ARGS__); _o = strlen(g_log); if (_o + 1 < sizeof g_log) { g_log[_o] = '\n'; g_log[_o + 1] = 0; } } while (0)

static uint8_t arena[1 << 18];
int dm_guest_heap_readable(uint64_t a, size_t n) { uint64_t b = (uint64_t)(uintptr_t)arena; return a >= b && n <= sizeof arena && a - b <= sizeof arena - n; }
static int sw_on = 1, session = 7;
int dm_custom_get(const char *key) { return !strcmp(key, "shrine_stacking") ? sw_on : 0; }
int dm_build_session(void) { return session; }

static int getter_calls;
static uint64_t getter_stub(uint64_t off, uint64_t w);
#define SL_GETTER(b, off, w) getter_stub((off), (w))
#include "../dm_shrine_layout.c"

static uint64_t addr(const void *p) { return (uint64_t)(uintptr_t)p; }
static size_t top;
static uint8_t *take(size_t n) { top = (top + 15) & ~(size_t)15; uint8_t *p = arena + top; top += n; if (top > sizeof arena) { fprintf(stderr, "arena full\n"); abort(); } return p; }


typedef struct { uint8_t *S, *pagevec, *page; uint32_t *packed; uint8_t *dense; size_t stride; int n, cap; } fstore_t;
static void st_init(fstore_t *s, size_t stride, int cap) {
    s->S = take(S_HEAD); s->pagevec = take(8); s->page = take(256 * 4); s->packed = (uint32_t *)take(4 * (size_t)cap); s->dense = take(stride * (size_t)cap);
    s->stride = stride; s->n = 0; s->cap = cap;
    memset(s->page, 0xff, 256 * 4);
    uint64_t pv = addr(s->page); memcpy(s->pagevec, &pv, 8);
}
static void st_sync(fstore_t *s) {
    uint64_t h[9] = { 0, addr(s->pagevec), addr(s->pagevec) + 8, 0, addr(s->packed), addr(s->packed) + 4 * (uint64_t)s->n, 0, addr(s->dense), addr(s->dense) + s->stride * (uint64_t)s->n };
    memcpy(s->S, h, sizeof h);
}
static uint8_t *st_add(fstore_t *s, uint32_t e) {
    int i = s->n++;
    uint32_t v = (uint32_t)i; memcpy(s->page + (e & 0xff) * 4, &v, 4);
    s->packed[i] = e;
    st_sync(s);
    return s->dense + s->stride * (size_t)i;
}
static uint8_t *st_get(fstore_t *s, uint32_t e) { for (int i = 0; i < s->n; i++) if (s->packed[i] == e) return s->dense + s->stride * (size_t)i; return NULL; }
static void st_del(fstore_t *s, uint32_t e) {
    int i = 0; while (i < s->n && s->packed[i] != e) i++;
    if (i == s->n) return;
    int last = s->n - 1;
    if (i != last) {
        s->packed[i] = s->packed[last]; memcpy(s->dense + s->stride * (size_t)i, s->dense + s->stride * (size_t)last, s->stride);
        uint32_t v = (uint32_t)i; memcpy(s->page + (s->packed[i] & 0xff) * 4, &v, 4);
    }
    memset(s->page + (e & 0xff) * 4, 0xff, 4);
    s->n--; st_sync(s);
}

static fstore_t O, L, T;
static uint8_t *unit, *merc;
enum { UID = 5, ROOT = 0x100010, MROOT = 0x100011 };
static float yaw, scale;

static void put_world(uint8_t *x, float y, float s, float px, float pz) {
    float w[16] = { s * cosf(y), 0, -s * sinf(y), 0, 0, s, 0, 0, s * sinf(y), 0, s * cosf(y), 0, px, 0, pz, 1 };
    memcpy(x + XF_WORLD, w, sizeof w);
}
static void world(void) {
    memset(arena, 0, sizeof arena); top = 0;
    st_init(&O, OV_STRIDE, 64); st_init(&L, LINK_STRIDE, 64); st_init(&T, XF_STRIDE, 64);
    unit = take(0x100); merc = take(0x100);
    uint32_t id = UID; memcpy(unit + U_ID, &id, 4); memcpy(merc + U_ID, &id, 4);
    yaw = 0.7f; scale = 0.93f;
    put_world(st_add(&T, ROOT), yaw, scale, 100, 50);
    put_world(st_add(&T, MROOT), 0.2f, 1.0f, 103, 52);
    memset(&g, 0, sizeof g); g_log[0] = 0; sw_on = 1; session = 7;
}
static void set_root(float y, float s) { yaw = y; scale = s; put_world(st_get(&T, ROOT), y, s, 100, 50); }


static void sym(uint32_t e, uint32_t ov, uint32_t serial, uint8_t *who, uint32_t parent) {
    uint32_t c[3] = { ov, UID, serial }; memcpy(st_add(&O, e), c, sizeof c);
    uint64_t lk[2] = { 0x1234, addr(who) }; memcpy(st_add(&L, e), lk, sizeof lk);
    uint8_t *x = st_add(&T, e);
    float m[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 14, 0, 1 }; memcpy(x, m, sizeof m);
    memcpy(x + XF_PARENT, &parent, 4);
}
static void unsym(uint32_t e) { st_del(&O, e); st_del(&L, e); st_del(&T, e); }
static const float *loc(uint32_t e) { return (const float *)(st_get(&T, e) + XF_LOCAL_T); }

static sl_in_t in(void) { return (sl_in_t){ sw_on, session, addr(unit), UID, addr(O.S), addr(L.S), addr(T.S) }; }
static int run(void) { sl_in_t i = in(); return layout_run(&i); }


static void screen_off(uint32_t e, float *sx, float *sy) {
    const float *t = loc(e), *p = (const float *)(st_get(&T, ROOT) + XF_WORLD);
    float d[3] = { t[0], t[1] - 14, t[2] }, w[3];
    for (int a = 0; a < 3; a++) w[a] = d[0] * p[a] + d[1] * p[4 + a] + d[2] * p[8 + a];
    *sx = (w[0] - w[2]) * 0.70710678f; *sy = w[1];
    float across = (w[0] + w[2]) * 0.70710678f;
    CHECK(fabsf(across) < 1e-4f, "offset has a component along the view axis %f", (double)across);
}
static int near(float a, float b) { return fabsf(a - b) < 1e-4f; }

static void t_slots(void) {

    static const struct { int n; float step; } k[] = { { 1, 0 }, { 2, 4.2f }, { 3, 4.2f }, { 4, 4.2f }, { 5, 4.2f }, { 6, 4.2f }, { 7, 3.4f }, { 8, 3.4f }, { 9, 2.55f }, { 10, 2.55f } };
    for (int c = 0; c < 10; c++) for (int i = 0; i < k[c].n; i++) {
        float sx, sy; dm_shrine_layout_slot(i, k[c].n, &sx, &sy);
        int n = k[c].n;
        float wx = n == 1 || (i == n - 1 && !(i & 1)) ? 0 : i & 1 ? 1.6f : -1.6f, wy = n == 1 ? 0 : (float)(i / 2) * k[c].step;
        CHECK(near(sx, wx) && near(sy, wy), "slot %d of %d: %f %f, want %f %f", i, n, (double)sx, (double)sy, (double)wx, (double)wy);
    }
}

static void t_layout(void) {
    world();
    sym(0x200001, 59, 1, unit, ROOT);
    CHECK(run() == 0 && near(loc(0x200001)[0], 0) && near(loc(0x200001)[1], 14), "one symbol stays where the game put it");
    sym(0x200002, 60, 2, unit, ROOT);
    CHECK(run() == 2, "two symbols written");
    float ax, ay, bx, by; screen_off(0x200001, &ax, &ay); screen_off(0x200002, &bx, &by);
    CHECK(near(ax, -1.6f) && near(bx, 1.6f) && near(ay, 0) && near(by, 0), "pair side by side: %f %f / %f %f", (double)ax, (double)ay, (double)bx, (double)by);
    CHECK(run() == 0, "nothing written once in place");
    sym(0x200003, 65, 3, unit, ROOT); sym(0x200004, 63, 4, unit, ROOT);
    run();
    float x[4], y[4]; uint32_t e[4] = { 0x200001, 0x200002, 0x200004, 0x200003 };
    for (int i = 0; i < 4; i++) screen_off(e[i], &x[i], &y[i]);
    CHECK(near(x[0], -1.6f) && near(x[1], 1.6f) && near(x[2], -1.6f) && near(x[3], 1.6f) && near(y[0], 0) && near(y[1], 0) && near(y[2], 4.2f) && near(y[3], 4.2f), "2 x 2 in overlay id order");
    unsym(0x200001);
    run();
    screen_off(0x200002, &x[0], &y[0]); screen_off(0x200004, &x[1], &y[1]); screen_off(0x200003, &x[2], &y[2]);
    CHECK(near(x[0], -1.6f) && near(x[1], 1.6f) && near(x[2], 0) && near(y[2], 4.2f) && near(y[0], 0), "re-flow after expiry");
    CHECK(g.n == 3, "expired entry retired");
    sw_on = 0;
    run();
    for (int i = 0; i < 3; i++) { uint32_t s = (uint32_t[]){ 0x200002, 0x200004, 0x200003 }[i]; CHECK(near(loc(s)[0], 0) && near(loc(s)[1], 14) && near(loc(s)[2], 0), "switch off restores the creation spot"); }
    CHECK(!g.displaced && run() == 0, "nothing displaced after the restore");
    sw_on = 1;
    CHECK(run() == 3 && g.n == 3, "off -> on lays out again from the kept baselines");

    unsym(0x200003); unsym(0x200004); run();
    CHECK(near(loc(0x200002)[0], 0) && near(loc(0x200002)[1], 14) && !g.displaced, "the last symbol goes back to the default spot");
    unsym(0x200002);
    CHECK(run() == 0 && g.n == 0, "no symbol: nothing written, nothing kept");
}



static void t_arrival(void) {
    world();
    float x, y;
    sym(0x200002, 60, 2, unit, ROOT); run();
    sym(0x200001, 59, 1, unit, ROOT); run();
    screen_off(0x200002, &x, &y); CHECK(near(x, -1.6f) && near(y, 0), "first arrival keeps the left slot: %f %f", (double)x, (double)y);
    screen_off(0x200001, &x, &y); CHECK(near(x, 1.6f) && near(y, 0), "later arrival takes the next slot: %f %f", (double)x, (double)y);
    sym(0x200003, 61, 3, unit, ROOT); run();
    screen_off(0x200003, &x, &y); CHECK(near(x, 0) && near(y, 4.2f), "third on top, centred");
    unsym(0x200002); run();
    screen_off(0x200001, &x, &y); CHECK(near(x, -1.6f) && near(y, 0), "second moves to the first slot: %f %f", (double)x, (double)y);
    screen_off(0x200003, &x, &y); CHECK(near(x, 1.6f) && near(y, 0), "third moves down beside it: %f %f", (double)x, (double)y);
    unsym(0x200001); run();
    sym(0x200005, 59, 5, unit, ROOT); run();
    screen_off(0x200003, &x, &y); CHECK(near(x, -1.6f) && near(y, 0), "the earlier one keeps the first slot");
    screen_off(0x200005, &x, &y); CHECK(near(x, 1.6f) && near(y, 0), "a re-taken shrine arrives last: %f %f", (double)x, (double)y);
    dm_shrine_layout_pump(0, 0);
    CHECK(g.next_seq == 0, "arrival counter reset with the baselines");
}

static void t_turned_parent(void) {
    static const float sc[] = { 0.5f, 0.93f, 2.0f };
    for (int f = 0; f < 8; f++) for (int s = 0; s < 3; s++) {
        world();
        set_root((float)f * 0.785398f + 0.1f, sc[s]);
        sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT); sym(0x200003, 61, 3, unit, ROOT);
        run();
        float x, y;
        screen_off(0x200001, &x, &y); CHECK(near(x, -1.6f) && near(y, 0), "facing %d scale %d: left %f %f", f, s, (double)x, (double)y);
        screen_off(0x200003, &x, &y); CHECK(near(x, 0) && near(y, 4.2f), "facing %d scale %d: top %f %f", f, s, (double)x, (double)y);
        set_root((float)f * 0.785398f + 0.9f, sc[s]);
        run();
        screen_off(0x200002, &x, &y); CHECK(near(x, 1.6f) && near(y, 0), "facing %d scale %d after a turn: %f %f", f, s, (double)x, (double)y);
    }
}

static void t_lifetime(void) {

    world();
    sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT);
    run();
    static uint8_t snap[sizeof arena];
    memcpy(snap, arena, sizeof arena);
    forget();
    CHECK(!g.valid && g.n == 0 && !g.displaced && !memcmp(snap, arena, sizeof arena), "cache dropped, memory untouched");

    world();
    sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT);
    run();
    unsym(0x200001); unsym(0x200002); session = 8;
    sym(0x200001, 59, 9, unit, ROOT); sym(0x200002, 60, 10, unit, ROOT);
    run();
    CHECK(g.session == 8 && g.n == 2 && !g.latched, "new session: fresh baselines");

    world();
    sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT);
    run();
    unsym(0x200001); sym(0x200001, 59, 3, unit, ROOT);
    run();
    CHECK(!g.latched && g.n == 2, "re-created entity accepted");
    for (int k = 0; k < g.n; k++) if (g.ent[k].e == 0x200001) CHECK(g.ent[k].serial == 3 && near(g.ent[k].base[0], 0) && near(g.ent[k].base[1], 14), "baseline of the new creation");

    world();
    sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT); sym(0x200003, 61, 3, unit, ROOT);
    run();
    unsym(0x200003);
    uint8_t tail[XF_STRIDE]; memcpy(tail, T.dense + T.stride * (size_t)T.n, XF_STRIDE);
    run();
    CHECK(!memcmp(tail, T.dense + T.stride * (size_t)T.n, XF_STRIDE), "retired entity not written");

    sw_on = 0; unsym(0x200002);
    memcpy(tail, T.dense + T.stride * (size_t)T.n, XF_STRIDE);
    run();
    CHECK(near(loc(0x200001)[0], 0) && !memcmp(tail, T.dense + T.stride * (size_t)T.n, XF_STRIDE) && !g.displaced, "restore through resolved entities only");

    world();
    sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT);
    run();
    CHECK(run() == 0 && run() == 0, "refresh: layout unchanged");

    unsym(0x200001); unsym(0x200002);
    memcpy(snap, arena, sizeof arena);
    CHECK(run() == 0 && !memcmp(snap, arena, sizeof arena) && g.n == 0, "curse removal: nothing written");
    sym(0x200005, 59, 5, unit, ROOT);
    CHECK(run() == 0 && near(loc(0x200005)[0], 0) && near(loc(0x200005)[1], 14), "a shrine taken afterwards sits on the default spot");
}

static void t_units(void) {

    world();
    sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT);
    sym(0x200003, 59, 3, merc, MROOT); sym(0x200004, 61, 4, merc, MROOT);
    uint32_t c[3] = { 62, UID + 1, 5 }; memcpy(st_add(&O, 0x200006), c, sizeof c);
    uint64_t lk[2] = { 0, addr(merc) }; memcpy(st_add(&L, 0x200006), lk, sizeof lk);
    float m[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 14, 0, 1 }; uint8_t *x = st_add(&T, 0x200006); memcpy(x, m, sizeof m); uint32_t p = MROOT; memcpy(x + XF_PARENT, &p, 4);
    CHECK(run() == 2, "only the own two symbols written");
    CHECK(near(loc(0x200003)[0], 0) && near(loc(0x200004)[0], 0) && near(loc(0x200006)[0], 0) && near(loc(0x200003)[1], 14), "other units untouched");
}

static uint64_t getter_stub(uint64_t off, uint64_t w) { (void)w; getter_calls++; return off == GET_OVERLAY ? addr(O.S) : off == GET_LINK ? addr(L.S) : addr(T.S); }


static void pump_arrival(uint32_t first_ov, uint32_t second_ov) {
    world();
    uint8_t *sl = take(0xc00); uint64_t a = addr(sl); memcpy(unit + U_STATLIST, &a, 8);
    uint32_t bits = 1; memcpy(sl + SL_STATE_BITS + SHRINE_STATE_WORD * 4, &bits, 4);
    sym(0x200001, first_ov, 1, unit, ROOT);
    for (int f = 0; f < 3; f++) dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    bits = 3; memcpy(sl + SL_STATE_BITS + SHRINE_STATE_WORD * 4, &bits, 4);
    sym(0x200002, second_ov, 2, unit, ROOT);
    dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    float x, y;
    screen_off(0x200001, &x, &y); CHECK(near(x, -1.6f) && near(y, 0), "overlay %u taken first keeps the first slot: %f", first_ov, (double)x);
    screen_off(0x200002, &x, &y); CHECK(near(x, 1.6f) && near(y, 0), "overlay %u taken later goes next: %f", second_ov, (double)x);
}


static void t_pump(void) {
    world();
    uint8_t *sl = take(0xc00); uint64_t a = addr(sl); memcpy(unit + U_STATLIST, &a, 8);
    uint32_t bits = 0; memcpy(sl + SL_STATE_BITS + SHRINE_STATE_WORD * 4, &bits, 4);
    getter_calls = 0;
    dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    CHECK(getter_calls == 0 && !g.valid, "no shrine bit, nothing displaced: no storage touched");
    bits = 1; memcpy(sl + SL_STATE_BITS + SHRINE_STATE_WORD * 4, &bits, 4);
    sym(0x200001, 59, 1, unit, ROOT);
    static uint8_t one[sizeof arena];
    memcpy(one, arena, sizeof arena);
    dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    CHECK(getter_calls == 3 && g.n == 1 && !memcmp(one, arena, sizeof arena), "one shrine: arrival recorded, nothing written");
    bits = 3; memcpy(sl + SL_STATE_BITS + SHRINE_STATE_WORD * 4, &bits, 4);
    sym(0x200002, 60, 2, unit, ROOT);
    dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    float x, y; screen_off(0x200002, &x, &y);
    CHECK(g.valid && near(x, 1.6f) && near(y, 0), "two shrine bits: laid out");
    sw_on = 0;
    dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    CHECK(near(loc(0x200002)[0], 0) && !g.displaced, "switch off restores");
    getter_calls = 0;
    dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    CHECK(getter_calls == 0, "switch off, nothing displaced: storages untouched");
    sw_on = 1;
    dm_shrine_layout_pump((uint8_t *)16, addr(unit));
    CHECK(g.displaced, "laid out again");
    static uint8_t snap[sizeof arena];
    memcpy(snap, arena, sizeof arena);
    dm_shrine_layout_pump(0, 0);
    CHECK(!g.valid && !g.n && !g.displaced && !memcmp(snap, arena, sizeof arena), "no player: dropped, memory untouched");
}


static void broken(const char *what, const char *name, void (*breakit)(void)) {
    static uint8_t snap[sizeof arena];
    world();
    sym(0x200001, 59, 1, unit, ROOT); sym(0x200002, 60, 2, unit, ROOT); sym(0x200003, 61, 3, unit, ROOT);
    breakit();
    memcpy(snap, arena, sizeof arena);
    int w = run();
    char want[64]; snprintf(want, sizeof want, "ASSUME shrine_layout.%s:", name);
    CHECK(w == 0 && !memcmp(snap, arena, sizeof arena), "%s: memory changed", what);
    CHECK(strstr(g_log, want) != NULL, "%s: no '%s' in log [%s]", what, want, g_log);
    CHECK(g.latched && run() == 0 && !memcmp(snap, arena, sizeof arena), "%s: not latched off", what);
}
static void b_trunc(void) { uint64_t e; memcpy(&e, T.S + S_PACKED + 8, 8); e -= 2; memcpy(T.S + S_PACKED + 8, &e, 8); }
static void b_misalign(void) { uint64_t e; memcpy(&e, O.S + S_DENSE + 8, 8); e -= 4; memcpy(O.S + S_DENSE + 8, &e, 8); }
static void b_reversed(void) { uint64_t b; memcpy(&b, L.S + S_PAGES, 8); b += 16; memcpy(L.S + S_PAGES, &b, 8); }
static void b_unmapped_vec(void) { uint64_t h[2] = { 0x1000, 0x1000 + 8 * 4 }; memcpy(T.S + S_PACKED, h, 16); }
static void b_unmapped_page(void) { uint64_t p = 0x7000; memcpy(L.pagevec, &p, 8); }
static void b_dense_range(void) { uint32_t v = 60; memcpy(T.page + (0x200002 & 0xff) * 4, &v, 4); }
static void b_stale_sym(void) { T.packed[2] = 0x300001; }
static void b_stale_link(void) { L.packed[1] = 0x300002; }
static void b_stale_parent(void) { T.packed[0] = 0x200010; }
static void b_no_parent(void) { uint32_t p = 0x100020; memcpy(st_get(&T, 0x200002) + XF_PARENT, &p, 4); }
static void b_nan_local(void) { float v = NAN; memcpy(st_get(&T, 0x200003) + 4, &v, 4); }
static void b_inf_parent(void) { float v = INFINITY; memcpy(st_get(&T, ROOT) + XF_WORLD + 4 * 13, &v, 4); }
static void b_zero_scale(void) { put_world(st_get(&T, ROOT), 0.3f, 0.0f, 1, 1); }
static void b_shear(void) { float v = 0.2f; memcpy(st_get(&T, ROOT) + XF_WORLD + 4, &v, 4); }
static void b_rows(void) { float v = 0.5f; memcpy(st_get(&T, 0x200002) + 4 * 2, &v, 4); }
static void b_base_x(void) { float v = 0.5f; memcpy(st_get(&T, 0x200003) + XF_LOCAL_T, &v, 4); }
static void b_null_store(void) { memset(O.S, 0, S_HEAD); uint64_t h[2] = { 0, 8 }; memcpy(O.S + S_PACKED, h, 16); }

int main(void) {
    t_slots();
    t_layout();
    t_arrival();
    t_turned_parent();
    t_lifetime();
    t_units();
    t_pump();
    pump_arrival(60, 59);
    pump_arrival(59, 60);
    broken("truncated packed vector", "storage", b_trunc);
    broken("misaligned dense vector", "storage", b_misalign);
    broken("reversed page vector", "storage", b_reversed);
    broken("vector outside mapped memory", "storage", b_unmapped_vec);
    broken("page pointer outside mapped memory", "storage", b_unmapped_page);
    broken("dense index out of range", "storage", b_dense_range);
    broken("stale generation in the symbol join", "join", b_stale_sym);
    broken("stale generation in the unit link join", "join", b_stale_link);
    broken("stale generation in the parent join", "join", b_stale_parent);
    broken("parent missing", "parent", b_no_parent);
    broken("NaN in the local matrix", "matrix", b_nan_local);
    broken("Inf in the parent matrix", "parent", b_inf_parent);
    broken("zero-scale parent", "parent", b_zero_scale);
    broken("non-orthogonal parent", "parent", b_shear);
    broken("non-identity local rows", "matrix", b_rows);
    broken("new symbol with baseline x != 0", "matrix", b_base_x);
    broken("null vector with a size", "storage", b_null_store);
    printf("t_shrine_layout: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
