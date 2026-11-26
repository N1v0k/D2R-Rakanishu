



















#include "dm.h"
#include "game_addrs.h"
#include "dm_custom.h"
#include "dm_build.h"
#include <math.h>
#include <string.h>
#include <time.h>

enum {
    WORLD = GA_ECS_WORLD, GET_LINK = GA_ECS_LINK_STORE_GET, GET_XFORM = GA_ECS_XFORM_STORE_GET, GET_OVERLAY = GA_ECS_OVERLAY_STORE_GET,
    S_PAGES = 0x08, S_PACKED = 0x20, S_DENSE = 0x38, S_HEAD = 0x48,
    OV_STRIDE = 0x0c, LINK_STRIDE = 0x10, XF_STRIDE = 0xd0,
    LINK_UNIT = 0x08, XF_LOCAL_T = 0x30, XF_WORLD = 0x40, XF_PARENT = 0xc0,
    U_ID = 0x08, U_STATLIST = 0x88, SL_STATE_BITS = 0xb30, SHRINE_STATE_WORD = 128 / 32,
    OVERLAY_FIRST = 59, OVERLAY_LAST = 68,
    MAX_WALK = 4096, MAX_PAGES = 4096, MAX_SYM = 32,
};
#define ID_NONE 0xfffffu
#define HALF_STEP 1.6f
#define ROW_STEP 4.2f
#define HEAD_ROOM 10.2f

#ifndef SL_LOG
#define SL_LOG LOG
#endif

int dm_guest_heap_readable(uint64_t a, size_t n);


static int ok_range(uint64_t a, uint64_t n) { return !n || (a && n <= ((size_t)-1) && dm_guest_heap_readable(a, (size_t)n)); }

typedef struct { uint64_t pages, npages, packed, npacked, dense, ndense; } store_t;
typedef enum { F_NONE, F_STORAGE, F_JOIN, F_PARENT, F_MATRIX } fail_t;
static const char *const k_fail[] = { "", "storage", "join", "parent", "matrix" };


static int64_t vec(const uint64_t be[2], uint64_t sz, uint64_t max) {
    if (be[1] < be[0] || (be[1] - be[0]) % sz) return -1;
    uint64_t n = (be[1] - be[0]) / sz;
    if (n > max || !ok_range(be[0], n * sz)) return -1;
    return (int64_t)n;
}

static fail_t store_resolve(uint64_t S, uint64_t stride, uint64_t max_dense, store_t *s) {
    uint64_t h[S_HEAD / 8];
    if (!S || !ok_range(S, S_HEAD)) return F_STORAGE;
    memcpy(h, (const void *)(uintptr_t)S, sizeof h);
    int64_t np = vec(&h[S_PAGES / 8], 8, MAX_PAGES), nk = vec(&h[S_PACKED / 8], 4, ID_NONE), nd = vec(&h[S_DENSE / 8], stride, max_dense);
    if (np < 0 || nk < 0 || nd < 0) return F_STORAGE;
    *s = (store_t){ h[S_PAGES / 8], (uint64_t)np, h[S_PACKED / 8], (uint64_t)nk, h[S_DENSE / 8], (uint64_t)nd };
    return F_NONE;
}

static uint32_t ld32(uint64_t a) { uint32_t v; memcpy(&v, (const void *)(uintptr_t)a, 4); return v; }


static int store_find(const store_t *s, uint32_t e, uint64_t *idx) {
    uint64_t pi = (e >> 8) & 0xfff, page;
    if ((e & ID_NONE) == ID_NONE || pi >= s->npages) return 0;
    memcpy(&page, (const void *)(uintptr_t)(s->pages + pi * 8), 8);
    if (!page) return 0;
    if (!ok_range(page + (uint64_t)(e & 0xff) * 4, 4)) return -1;
    uint32_t v = ld32(page + (uint64_t)(e & 0xff) * 4);
    if ((v & ID_NONE) == ID_NONE) return 0;
    uint64_t i = v & ID_NONE;
    if (i >= s->npacked || i >= s->ndense) return -1;
    if (ld32(s->packed + i * 4) != e) return -2;
    *idx = i;
    return 1;
}

static int finite_n(const float *f, int n) { for (int i = 0; i < n; i++) if (!isfinite(f[i])) return 0; return 1; }
static float dot3(const float *a, const float *b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }


static float parent_scale2(const float *w) {
    if (!finite_n(w, 16)) return 0;
    float l0 = dot3(w, w), l1 = dot3(w + 4, w + 4), l2 = dot3(w + 8, w + 8);
    if (l0 < 0.01f || l0 > 100.0f || fabsf(l1 - l0) > 0.01f * l0 || fabsf(l2 - l0) > 0.01f * l0) return 0;
    if (fabsf(dot3(w, w + 4)) > 1e-3f * l0 || fabsf(dot3(w, w + 8)) > 1e-3f * l0 || fabsf(dot3(w + 4, w + 8)) > 1e-3f * l0) return 0;
    return (l0 + l1 + l2) / 3.0f;
}


static int local_shape(const float *m) {
    static const float id[12] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 };
    if (!finite_n(m, 16)) return 0;
    for (int i = 0; i < 12; i++) if ((i & 3) != 3 && fabsf(m[i] - id[i]) > 1e-3f) return 0;
    return 1;
}
static int base_shape(const float *t) { return finite_n(t, 3) && fabsf(t[0]) <= 1e-3f && fabsf(t[2]) <= 1e-3f && fabsf(t[1]) <= 64.0f; }


static void dm_shrine_layout_slot(int i, int n, float *sx, float *sy) {
    *sx = *sy = 0;
    if (n < 2 || i < 0 || i >= n) return;
    int rows = (n + 1) / 2, row = i / 2;
    float step = rows < 2 ? 0 : fminf(ROW_STEP, HEAD_ROOM / (float)(rows - 1));
    *sx = i == n - 1 && !(i & 1) ? 0 : i & 1 ? HALF_STEP : -HALF_STEP;
    *sy = (float)row * step;
}



static void dm_shrine_layout_target(const float base[3], const float *pw, float l2, float sx, float sy, float out[3]) {
    const float c = 0.70710678f, d[3] = { sx * c, sy, -sx * c };
    for (int a = 0; a < 3; a++) out[a] = base[a] + dot3(d, pw + 4 * a) / l2;
}

typedef struct { uint32_t e, serial, ov, seq; float base[3]; int displaced; float sx, sy; } entry_t;
typedef struct {
    int on;
    int session;
    uint64_t unit;
    uint32_t unit_id;
    uint64_t ov, link, xf;
} sl_in_t;

static struct {
    int valid, session; uint64_t unit, ov, link, xf; uint32_t unit_id;
    entry_t ent[MAX_SYM]; int n; uint32_t next_seq;
    int displaced, latched;
#ifdef DM_EFFECT_TESTS
    int wrote, n_sym, n_walk;
#endif
} g;

static void forget(void) { g.valid = 0; g.n = 0; g.displaced = 0; g.next_seq = 0; }

static void fail(fail_t f, const char *msg) {
    if (!g.latched) SL_LOG("ASSUME shrine_layout.%s: %s; layout off for this run", k_fail[f], msg);
    g.latched = 1;
}

#ifdef DM_EFFECT_TESTS
#define QA_SKIP(why, e) do { static unsigned n_; if (n_++ < 5) SL_LOG("shrine_layout skip: %s for entity %x", why, (unsigned)(e)); } while (0)
#else
#define QA_SKIP(why, e) do { } while (0)
#endif
typedef struct { uint32_t e, ov, serial, seq; uint64_t xa; float local[16], pw[16], l2, tgt[3], sx, sy; int k; } sym_t;

#ifdef DM_EFFECT_TESTS
static void qa_frame(const sym_t *s, int n, const entry_t *ent);
#endif


static int layout_run(const sl_in_t *in) {
    sym_t s[MAX_SYM]; int n = 0;
    store_t O, L, T;
    fail_t f;
    if (g.latched) return 0;
    if (g.valid && (g.session != in->session || g.unit != in->unit || g.unit_id != in->unit_id || g.ov != in->ov || g.link != in->link || g.xf != in->xf)) forget();
    if (!g.valid) { g.valid = 1; g.session = in->session; g.unit = in->unit; g.unit_id = in->unit_id; g.ov = in->ov; g.link = in->link; g.xf = in->xf; }

    if ((f = store_resolve(in->ov, OV_STRIDE, ID_NONE, &O)) || (f = store_resolve(in->link, LINK_STRIDE, ID_NONE, &L))
        || (f = store_resolve(in->xf, XF_STRIDE, ID_NONE, &T))) { fail(f, "storage vectors out of shape"); return 0; }
    if (O.npacked > MAX_WALK || O.ndense < O.npacked) { fail(F_STORAGE, "overlay storage count out of range"); return 0; }
#ifdef DM_EFFECT_TESTS
    g.n_walk = (int)O.npacked;
#endif
    for (uint64_t i = 0; i < O.npacked; i++) {
        uint32_t c[3], e = ld32(O.packed + i * 4);
        memcpy(c, (const void *)(uintptr_t)(O.dense + i * OV_STRIDE), sizeof c);
        if (c[0] < OVERLAY_FIRST || c[0] > OVERLAY_LAST || c[1] != in->unit_id) continue;
        uint64_t li, ti, pi; uint64_t up;
        int r = store_find(&L, e, &li);
        if (r == -2) { fail(F_JOIN, "unit link join with a stale entity id"); return 0; }
        if (r < 0) { fail(F_STORAGE, "unit link index out of range"); return 0; }
        if (!r) { QA_SKIP("unit link absent", e); return 0; }
        memcpy(&up, (const void *)(uintptr_t)(L.dense + li * LINK_STRIDE + LINK_UNIT), 8);
        if (up != in->unit) continue;
        if (n == MAX_SYM) { fail(F_STORAGE, "more symbol entities than shrine states"); return 0; }
        r = store_find(&T, e, &ti);
        if (r == -2) { fail(F_JOIN, "transform join with a stale entity id"); return 0; }
        if (r < 0) { fail(F_STORAGE, "transform index out of range"); return 0; }
        if (!r) { QA_SKIP("transform absent", e); return 0; }
        sym_t *y = &s[n];
        y->e = e; y->ov = c[0]; y->serial = c[2]; y->xa = T.dense + ti * XF_STRIDE;
        memcpy(y->local, (const void *)(uintptr_t)y->xa, sizeof y->local);
        uint32_t par = ld32(y->xa + XF_PARENT);
        r = store_find(&T, par, &pi);
        if (r == -2) { fail(F_JOIN, "parent join with a stale entity id"); return 0; }
        if (r < 0) { fail(F_STORAGE, "parent transform index out of range"); return 0; }
        if (!r) { fail(F_PARENT, "symbol without a parent transform"); return 0; }
        memcpy(y->pw, (const void *)(uintptr_t)(T.dense + pi * XF_STRIDE + XF_WORLD), sizeof y->pw);
        if (!(y->l2 = parent_scale2(y->pw))) { fail(F_PARENT, "parent rows not orthogonal with one non-zero scale"); return 0; }
        if (!local_shape(y->local)) { fail(F_MATRIX, "symbol local matrix not finite or not the creation shape"); return 0; }
        n++;
    }

    int keep[MAX_SYM] = { 0 }, ranks = 0, rank_of[MAX_SYM];
    for (int i = 0; i < n; i++) {
        int k = 0;
        while (k < g.n && !(g.ent[k].e == s[i].e && g.ent[k].serial == s[i].serial && g.ent[k].ov == s[i].ov)) k++;
        if (k == g.n) {
            if (!base_shape(s[i].local + 12)) { fail(F_MATRIX, "new symbol away from its creation spot"); return 0; }
            k = -1;
        }
        s[i].k = k;
        if (k >= 0) keep[k] = 1;
        int seen = 0;
        for (int j = 0; j < i; j++) seen |= s[j].ov == s[i].ov;
        ranks += !seen;
    }

    uint32_t fresh = 0;
    for (int v = OVERLAY_FIRST; v <= OVERLAY_LAST; v++) {
        int any = 0;
        for (int i = 0; i < n; i++) if (s[i].k < 0 && (int)s[i].ov == v) { s[i].seq = g.next_seq + fresh; any = 1; }
        fresh += any;
    }
    for (int i = 0; i < n; i++) if (s[i].k >= 0) s[i].seq = g.ent[s[i].k].seq;

    uint32_t first[OVERLAY_LAST - OVERLAY_FIRST + 1];
    for (int v = 0; v <= OVERLAY_LAST - OVERLAY_FIRST; v++) first[v] = UINT32_MAX;
    for (int i = 0; i < n; i++) if (s[i].seq < first[s[i].ov - OVERLAY_FIRST]) first[s[i].ov - OVERLAY_FIRST] = s[i].seq;
    for (int i = 0; i < n; i++) {
        int r = 0, me = (int)s[i].ov - OVERLAY_FIRST;
        for (int v = 0; v <= OVERLAY_LAST - OVERLAY_FIRST; v++) r += first[v] != UINT32_MAX && (first[v] < first[me] || (first[v] == first[me] && v < me));
        rank_of[i] = r;
    }
    for (int i = 0; i < n; i++) {
        const float *base = s[i].k >= 0 ? g.ent[s[i].k].base : s[i].local + 12;
        float sx = 0, sy = 0;
        if (in->on) dm_shrine_layout_slot(rank_of[i], ranks, &sx, &sy);
        s[i].sx = sx; s[i].sy = sy;
        dm_shrine_layout_target(base, s[i].pw, s[i].l2, sx, sy, s[i].tgt);
        float dx = s[i].tgt[0] - base[0], dy = s[i].tgt[1] - base[1], dz = s[i].tgt[2] - base[2];
        if (!finite_n(s[i].tgt, 3) || dx * dx + dy * dy + dz * dz > 32.0f * 32.0f) { fail(F_MATRIX, "layout target not finite or too far"); return 0; }
    }
#ifdef DM_EFFECT_TESTS
    qa_frame(s, n, g.ent);
#endif

    entry_t next[MAX_SYM]; int m = 0;
    for (int k = 0; k < g.n; k++) if (keep[k]) { next[m] = g.ent[k]; for (int i = 0; i < n; i++) if (s[i].k == k) s[i].k = m; m++; }
    for (int i = 0; i < n; i++) if (s[i].k < 0) { next[m] = (entry_t){ s[i].e, s[i].serial, s[i].ov, s[i].seq, { s[i].local[12], s[i].local[13], s[i].local[14] }, 0, 0, 0 }; s[i].k = m++; }
    memcpy(g.ent, next, (size_t)m * sizeof *next); g.n = m; g.next_seq += fresh;
    int wrote = 0;
    g.displaced = 0;
    for (int i = 0; i < n; i++) {
        entry_t *en = &g.ent[s[i].k];
        const float *cur = s[i].local + 12;
        int moved = 0, away = 0;
        for (int a = 0; a < 3; a++) { moved |= fabsf(s[i].tgt[a] - cur[a]) > 1e-4f; away |= fabsf(s[i].tgt[a] - en->base[a]) > 1e-4f; }
        if (moved) { memcpy((void *)(uintptr_t)(s[i].xa + XF_LOCAL_T), s[i].tgt, sizeof s[i].tgt); wrote++; }
        en->displaced = away;
        en->sx = s[i].sx; en->sy = s[i].sy;
        g.displaced |= en->displaced;
    }
#ifdef DM_EFFECT_TESTS
    g.wrote = wrote; g.n_sym = ranks;
#endif
    return wrote;
}

#ifdef DM_EFFECT_TESTS




static int qa_on(void) { static int on = -1; if (on < 0) on = getenv("DM_TEST_SHRINE_LAYOUT") != NULL; return on; }
static struct { unsigned checked, over, turning, logged, cost_n, raced, since_turn, settled_over; float max, r0[3]; uint64_t cost_sum, cost_max, t0; uint32_t sig; int said; uint8_t head[3][S_HEAD]; } q;
static uint64_t qa_ns(void) { struct timespec t; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t);
    return (uint64_t)t.tv_sec * 1000000000u + (uint64_t)t.tv_nsec; }

static void qa_frame(const sym_t *s, int n, const entry_t *ent) {
    if (!qa_on() || !n) return;
    const float c = 0.70710678f;
    float worst = 0;
    int turned = fabsf(s[0].pw[0] - q.r0[0]) + fabsf(s[0].pw[1] - q.r0[1]) + fabsf(s[0].pw[2] - q.r0[2]) > 1e-3f;
    memcpy(q.r0, s[0].pw, sizeof q.r0);
    for (int i = 0; i < n; i++) {
        if (s[i].k < 0) continue;
        const entry_t *en = &ent[s[i].k];
        float w[3], d[3] = { en->sx * c, en->sy, -en->sx * c };
        memcpy(w, (const void *)(uintptr_t)(s[i].xa + XF_WORLD + 0x30), sizeof w);
        float e2 = 0;
        for (int a = 0; a < 3; a++) {
            float want = s[i].pw[12 + a] + d[a] + en->base[0] * s[i].pw[a] + en->base[1] * s[i].pw[4 + a] + en->base[2] * s[i].pw[8 + a];
            e2 += (w[a] - want) * (w[a] - want);
        }
        if (e2 > worst) worst = e2;
    }
    worst = sqrtf(worst);
    q.checked++; q.turning += turned;
    q.since_turn = turned ? 0 : q.since_turn + 1;
    if (worst > 0.05f && q.since_turn > 3) q.settled_over++;
    if (worst > q.max) q.max = worst;
    if (worst > 0.05f) { q.over++; if (q.logged++ < 40) SL_LOG("shrine_layout phase: frame %u deviation %.3f (%d symbols, turning %d)", q.checked, (double)worst, n, turned); }
    if (q.checked % 120 == 0) SL_LOG("shrine_layout phase: %u frames checked, %u over 0.05, max %.4f, %u while turning, %u over when settled", q.checked, q.over, (double)q.max, q.turning, q.settled_over);
}

static void qa_before(const sl_in_t *in) {
    if (!qa_on()) return;
    if (!q.said) {
        q.said = 1;
        SL_LOG("shrine_layout storages: overlay %s, link %s, transform %s", ok_range(in->ov, S_HEAD) ? "guest heap" : "outside the guest heap",
               ok_range(in->link, S_HEAD) ? "guest heap" : "outside the guest heap", ok_range(in->xf, S_HEAD) ? "guest heap" : "outside the guest heap");
    }
    const uint64_t st[3] = { in->ov, in->link, in->xf };
    for (int i = 0; i < 3; i++) if (ok_range(st[i], S_HEAD)) memcpy(q.head[i], (const void *)(uintptr_t)st[i], S_HEAD);
    q.t0 = qa_ns();
}

static void qa_after(const sl_in_t *in) {
    if (!qa_on()) return;
    uint64_t dt = qa_ns() - q.t0;
    const uint64_t st[3] = { in->ov, in->link, in->xf };
    for (int i = 0; i < 3; i++) if (ok_range(st[i], S_HEAD) && memcmp(q.head[i], (const void *)(uintptr_t)st[i], S_HEAD) && q.raced++ < 10)
        SL_LOG("shrine_layout phase: storage %d changed during the pass", i);
    q.cost_sum += dt; if (dt > q.cost_max) q.cost_max = dt;
    if (++q.cost_n == 2000) {
        SL_LOG("shrine_layout cost: 2000 passes mean %.4f ms max %.4f ms thread time (overlay entities %d)", (double)q.cost_sum / 2000e6, (double)q.cost_max / 1e6, g.n_walk);
        q.cost_n = 0; q.cost_sum = q.cost_max = 0;
    }
    uint32_t sig = (uint32_t)g.n_sym << 16 | (uint32_t)in->on << 8 | (uint32_t)g.latched;
    if (sig != q.sig) { q.sig = sig; SL_LOG("shrine_layout: %d symbols laid out (switch %d, wrote %d)", g.n_sym, in->on, g.wrote); }
}
#endif

#ifndef SL_GETTER
#define SL_GETTER(b, off, w) ((uint64_t (*)(uint64_t))(uintptr_t)((b) + (off)))(w)
#endif


void dm_shrine_layout_pump(uint8_t *b, uint64_t pl) {
    if (!b || !pl) { forget(); return; }
    if (g.latched || !ok_range(pl + U_ID, 4)) return;
    int on = dm_custom_get("shrine_stacking") > 0;
    uint64_t sl = 0; uint32_t bits = 0x3ff;
    if (ok_range(pl + U_STATLIST, 8) && (memcpy(&sl, (const void *)(uintptr_t)(pl + U_STATLIST), 8), sl)
        && ok_range(sl + SL_STATE_BITS + SHRINE_STATE_WORD * 4, 4)) bits = ld32(sl + SL_STATE_BITS + SHRINE_STATE_WORD * 4) & 0x3ff;

    if ((!on || !bits) && !g.displaced) return;
    uint64_t W = (uint64_t)(uintptr_t)(b + WORLD);
    sl_in_t in = { on, dm_build_session(), pl, ld32(pl + U_ID), SL_GETTER(b, GET_OVERLAY, W), SL_GETTER(b, GET_LINK, W), SL_GETTER(b, GET_XFORM, W) };
#ifdef DM_EFFECT_TESTS
    qa_before(&in);
#endif
    layout_run(&in);
#ifdef DM_EFFECT_TESTS
    qa_after(&in);
#endif
}
