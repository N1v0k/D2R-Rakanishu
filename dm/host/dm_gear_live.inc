





#include "dm_gear_compose.h"

static dm_gear_tables g_gt;
static int g_gt_state;
static pthread_once_t g_gt_once = PTHREAD_ONCE_INIT;
static void gear_tables_load(void) {
    g_gt_state = -1;
    char p[1200]; FILE *f = dm_overlay_file("gear.tsv", p, sizeof p) ? fopen(p, "rb") : NULL;
    static char text[256 * 1024]; size_t n = f ? fread(text, 1, sizeof text - 1, f) : 0;
    int whole = f && n < sizeof text - 1 && !ferror(f);
    if (f) fclose(f);
    text[n] = 0;
    if (whole && !dm_gear_tables_parse(&g_gt, text)) { g_gt_state = 1; LOG("gear: gear.tsv read, %d sets, %d set items", g_gt.nset, g_gt.nitem); }
    else LOG("【红】gear: gear.tsv %s, set items stay unrated", f ? "unreadable" : "missing");
}

static const dm_gear_tables *gear_tables(void) { pthread_once(&g_gt_once, gear_tables_load); return g_gt_state > 0 ? &g_gt : NULL; }
const dm_gear_tables *dm_gear_tables_shared(void) { return gear_tables(); }

#define GEAR_SET_STAT 71
static int gear_quality5(uint64_t it) { uint64_t d = it ? *(uint64_t *)(it + 0x10) : 0; return d && *(uint32_t *)d == 5; }

static int gear_keys(uint64_t u, uint32_t *keys, int n, int max) {
    uint64_t sl = u ? *(uint64_t *)(u + 0x88) : 0; if (!sl) return n;
    for (int part = 0; part < 2; part++) {
        if (part && *(int32_t *)(sl + 0x1c) >= 0) break;
        uint64_t a = sl + (part ? 0xe8 : 0x30), arr = *(uint64_t *)a, cnt = *(uint64_t *)(a + 8);
        if (cnt > 512) return -1;
        for (uint64_t i = 0; arr && i < cnt; i++) {
            uint32_t key = *(uint32_t *)(arr + 8 * i); int dup = 0;
            for (int j = 0; j < n && !dup; j++) dup = keys[j] == key;
            if (!dup) { if (n >= max) return -1; keys[n++] = key; }
        }
    }
    return n;
}


static int gear_list_values(uint64_t n, dm_gear_list *l, int sign, int skip) {
    uint64_t arr = *(uint64_t *)(n + 0x30), cnt = *(uint64_t *)(n + 0x38);
    if (cnt > 64 || (cnt && !arr)) return -1;
    for (uint64_t i = 0; i < cnt; i++) { uint32_t key = *(uint32_t *)(arr + 8 * i); if ((int)(key >> 16) != skip) dm_gear_list_add(l, key >> 16, key & 0xffff, sign * *(int32_t *)(arr + 8 * i + 4)); }
    return 0;
}
static int gear_green(uint64_t it, dm_gear_list *l) {
    uint64_t sl = *(uint64_t *)(it + 0x88); if (!sl || *(int32_t *)(sl + 0x1c) >= 0) return 0;
    uint64_t n = *(uint64_t *)(sl + 0xd0);
    for (int i = 0; n && i < 64; i++, n = *(uint64_t *)(n + 0x68)) {
        uint32_t f = *(uint32_t *)(n + 0x1c);
        if ((f & 0x40) && !(f & 0x2000) && *(int32_t *)(n + 0x20) && gear_list_values(n, l, 1, -1)) return -1;
    }
    return 0;
}

static int gear_set_lists(uint64_t pl, dm_gear_list *l) {
    uint64_t sl = *(uint64_t *)(pl + 0x88); if (!sl || *(int32_t *)(sl + 0x1c) >= 0) return -1;
    uint64_t n = *(uint64_t *)(sl + 0xd0);
    for (int i = 0; n && i < 256; i++, n = *(uint64_t *)(n + 0x68)) {
        if (!*(int32_t *)(n + 0x20)) continue;
        uint64_t arr = *(uint64_t *)(n + 0x30), cnt = *(uint64_t *)(n + 0x38); int set = 0;
        for (uint64_t j = 0; arr && j < cnt && j < 64; j++) set |= (*(uint32_t *)(arr + 8 * j) >> 16) == GEAR_SET_STAT;
        if (set && gear_list_values(n, l, 1, GEAR_SET_STAT)) return -1;
    }
    return 0;
}


static int gear_profile(uint8_t *b, uint64_t pl, uint64_t it, int slot, const dm_gear_tables *t, dm_gear_item *g) {
    memset(g, 0, sizeof *g); g->id = *(uint32_t *)(it + 8); g->slot = slot; g->setitem = -1;
    uint64_t d = *(uint64_t *)(it + 0x10);
    if (gear_quality5(it)) { uint32_t row = *(uint32_t *)(d + 0x34); if (row >= (uint32_t)t->nitem) return -1; g->setitem = (int)row; }
    req_table(b, pl, it, &g->req_str, &g->req_dex); g->req_lvl = ((int (*)(uint64_t, uint64_t))(b + 0x571580))(it, pl);
    static uint32_t keys[512]; int n = gear_keys(it, keys, 0, 512), k = 0;
    KIDS(it, c, k) if (n >= 0) n = gear_keys(c, keys, n, 512);
    if (n < 0) return -1;
    static dm_gear_list green; green.n = green.full = 0;
    if (g->setitem >= 0 && gear_green(it, &green)) return -1;
    static dm_gear_list conv; conv.n = conv.full = 0; dm_gear_item_conversions(t, gstat(b, pl, 12, 0), &green, &conv);
    for (int i = 0; i < conv.n; i++) dm_gear_list_add(&green, conv.v[i].stat, conv.v[i].layer, conv.v[i].value);
    for (int i = 0; i < n; i++) { int st = keys[i] >> 16, ly = keys[i] & 0xffff; if (st == 6 || st == 8 || st == 10) continue;
        dm_gear_list_add(&g->own, st, ly, gstat(b, it, st, ly) - dm_gear_list_get(&green, st, ly)); }
    return g->own.full ? -1 : 0;
}


typedef struct {
    int ok;
    unsigned flags;
    int nworn, nafter;
    dm_gear_item worn[DM_GEAR_MAXITEMS], after[DM_GEAR_MAXITEMS];
    unsigned char given[DM_GEAR_MAXITEMS];
    dm_gear_base base;
    dm_gear_char now, aft;
    int mism_stat, mism_model, mism_game;
} gear_swap_out;
enum { GEAR_S_CAPTURE = 1u << 16,
       GEAR_S_KEEP = 1u << 17,
       GEAR_S_SKILL = 1u << 18 };
static int gear_d(const gear_swap_out *o, int st, int ly) {
    return ly ? dm_gear_list_get(&o->aft.raw, st, ly) - dm_gear_list_get(&o->now.raw, st, ly) : o->aft.value[st] - o->now.value[st];
}
static void gear_swap(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int sl, gear_swap_out *o) {
    memset(o, 0, sizeof *o); o->ok = -1;
    const dm_gear_tables *t = gear_tables(); if (!t) return;
    int cls = (int)*(uint32_t *)(pl + 4), lpv, mpm, pen;
    if (!bench_coeffs(cls, &lpv, &mpm, &pen, b)) return;
    o->ok = 0;
    uint64_t wit[DM_GEAR_MAXITEMS]; int nw = 0;
    for (uint64_t q = first_item(pl); q; q = next_item(q)) { uint64_t qd = *(uint64_t *)(q + 0x10); int s = qd ? *(uint8_t *)(qd + 0x54) : 0;
        if (*(uint32_t *)(q + 0xc) != 1 || s < 1 || s > 10) continue;
        if (nw >= DM_GEAR_MAXITEMS - 1 || gear_profile(b, pl, q, s, t, &o->worn[nw])) { o->flags |= DM_GEAR_C_FULL; return; }
        o->given[nw] = (unsigned char)item_effective(b, pl, q); wit[nw++] = q; }
    o->nworn = nw;
    o->base.cls = cls; o->base.clvl = gstat(b, pl, 12, 0); o->base.lpv = lpv; o->base.mpm = mpm;
    static dm_gear_char worn; dm_gear_base empty = o->base; memset(&empty.base, 0, sizeof empty.base);
    if (dm_gear_compose(t, &empty, o->worn, nw, o->given, &worn)) return;

    static uint32_t keys[512]; int nk = gear_keys(pl, keys, 0, 512); if (nk < 0) { o->flags |= DM_GEAR_C_FULL; return; }
    for (int i = 0; i < nk; i++) { int st = keys[i] >> 16, ly = keys[i] & 0xffff; if (st == 6 || st == 8 || st == 10) continue;
        dm_gear_list_add(&o->base.base, st, ly, gstat(b, pl, st, ly) - (ly ? dm_gear_list_get(&worn.raw, st, ly) : worn.value[st])); }
    for (int i = 0; i < worn.raw.n; i++) { const dm_gear_v *v = &worn.raw.v[i]; int known = 0; for (int j = 0; j < nk && !known; j++) known = keys[j] == ((uint32_t)v->stat << 16 | v->layer);
        if (!known) dm_gear_list_add(&o->base.base, v->stat, v->layer, gstat(b, pl, v->stat, v->layer) - (v->layer ? v->value : worn.value[v->stat])); }
    if (o->base.base.full) { o->flags |= DM_GEAR_C_FULL; return; }
    dm_gear_compose(t, &o->base, o->worn, nw, o->given, &o->now);


    static dm_gear_list model, game; model.n = model.full = game.n = game.full = 0;
    for (int i = 0; i < worn.gear.n; i++) dm_gear_list_add(&model, worn.gear.v[i].stat, worn.gear.v[i].layer, worn.gear.v[i].value);
    for (int i = 0; i < nw; i++) if (o->given[i]) for (int k = 0; k < o->worn[i].own.n; k++) dm_gear_list_add(&model, o->worn[i].own.v[k].stat, o->worn[i].own.v[k].layer, -o->worn[i].own.v[k].value);
    int bad = gear_set_lists(pl, &game) < 0;
    for (int i = 0; i < nw && !bad; i++) if (o->given[i] && o->worn[i].setitem >= 0) bad = gear_green(wit[i], &game) < 0;
    for (int pass = 0; pass < 2 && !bad; pass++) { const dm_gear_list *x = pass ? &game : &model, *y = pass ? &model : &game;
        for (int i = 0; i < x->n; i++) { const dm_gear_v *v = &x->v[i]; int w = dm_gear_list_get(y, v->stat, v->layer);
            if (w != v->value && !(worn.flags & DM_GEAR_C_UNMAPPED)) { bad = 1; o->mism_stat = v->stat; o->mism_model = pass ? w : v->value; o->mism_game = pass ? v->value : w; break; } } }
    static dm_gear_char check; dm_gear_compose(t, &o->base, o->worn, nw, NULL, &check);
    if (memcmp(check.active, o->given, (size_t)nw)) { bad = 1; if (!o->mism_stat) o->mism_stat = -1; }
    if (bad) o->flags |= GEAR_S_CAPTURE;

    int na = 0, keep[DM_GEAR_MAXITEMS];
    for (int i = 0; i < nw; i++) { int out = 0; for (int k = 0; k < nold; k++) out |= wit[i] == olds[k]; if (out) continue; keep[na] = i; o->after[na++] = o->worn[i]; }
    if (cand) { if (gear_profile(b, pl, cand, sl, t, &o->after[na])) { o->flags |= DM_GEAR_C_FULL; return; } keep[na] = -1; na++; }
    o->nafter = na;
    if (dm_gear_compose(t, &o->base, o->after, na, NULL, &o->aft)) return;
    for (int i = 0; i < na; i++) if (keep[i] >= 0 && o->aft.active[i] != o->given[keep[i]]) o->flags |= GEAR_S_KEEP;

    static dm_gear_list bonus; bonus.n = bonus.full = 0;
    for (int side = 0; side < 2; side++) { const dm_gear_char *c = side ? &o->aft : &o->now; const dm_gear_item *it = side ? o->after : o->worn; int cnt = side ? na : nw, sg = side ? 1 : -1;
        for (int i = 0; i < c->gear.n; i++) dm_gear_list_add(&bonus, c->gear.v[i].stat, c->gear.v[i].layer, sg * c->gear.v[i].value);
        for (int i = 0; i < cnt; i++) if (c->active[i]) for (int k = 0; k < it[i].own.n; k++) dm_gear_list_add(&bonus, it[i].own.v[k].stat, it[i].own.v[k].layer, -sg * it[i].own.v[k].value); }
    for (int i = 0; i < bonus.n; i++) { int st = bonus.v[i].stat; if (bonus.v[i].value && (st == 83 || st == 97 || st == 107 || st == 126 || st == 127 || st == 151 || st == 188 || st == 204)) o->flags |= GEAR_S_SKILL; }
    o->flags |= o->aft.flags | (o->now.flags & ~DM_GEAR_C_ORDER);
    o->ok = !(o->flags & (DM_GEAR_C_ORDER | DM_GEAR_C_UNMAPPED | DM_GEAR_C_FUNC1 | DM_GEAR_C_PCT | DM_GEAR_C_ITEMOP | DM_GEAR_C_FULL | GEAR_S_CAPTURE | GEAR_S_KEEP | GEAR_S_SKILL));
    if (o->flags & GEAR_S_CAPTURE) {
        static int said; if (said++ < 8) LOG("ASSUME bestgear.compose: worn set composed differs from capture (%s %d: model %d, game %d)",
                                             o->mism_stat < 0 ? "requirement" : "stat", o->mism_stat, o->mism_model, o->mism_game); }
}
static void gear_deltas(const gear_swap_out *o, int *d) { for (int st = 0; st < DM_GEAR_NSTAT; st++) d[st] = o->aft.value[st] - o->now.value[st]; }

static int gear_involved(uint64_t cand, const uint64_t *olds, int nold) {
    int r = gear_quality5(cand); for (int k = 0; k < nold; k++) r |= gear_quality5(olds[k]); return r;
}


static gear_swap_out g_bench_gear;
static int g_bench_gear_on;

static int bench_gear_predict(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int sl, uint32_t cid) {
    g_bench_gear_on = 0;
    if (!gear_involved(cand, olds, nold)) return 0;
    gear_swap_out *o = &g_bench_gear; gear_swap(b, pl, cand, olds, nold, sl, o);
    static char sets[200]; int k = 0; sets[0] = 0;
    for (int i = 0; i < o->now.nworn && k < 150; i++) k += snprintf(sets + k, sizeof sets - k, " set %d: %d", o->now.worn[i].set, o->now.worn[i].pieces);
    k += snprintf(sets + k, sizeof sets - k, " ->");
    for (int i = 0; i < o->aft.nworn && k < 190; i++) k += snprintf(sets + k, sizeof sets - k, " set %d: %d", o->aft.worn[i].set, o->aft.worn[i].pieces);
    LOG("[cmp] 验算台 套装 id %u：%s（标记 %#x）%s", cid, o->ok > 0 ? "合成" : "不合成", o->flags, sets);
    if (o->ok <= 0) return 0;
    g_bench_gear_on = 1; return 1;
}
static int bench_gear_delta(int st, int ly) { return gear_d(&g_bench_gear, st, ly); }
static void bench_gear_deltas(int *d) { gear_deltas(&g_bench_gear, d); }

static int bench_gear_check(uint8_t *b, uint64_t pl, uint32_t cid) {
    if (!g_bench_gear_on) return -1;
    const gear_swap_out *o = &g_bench_gear; static uint32_t keys[512]; int nk = gear_keys(pl, keys, 0, 512), ok = 0, bad = 0;
    for (int i = 0; i < o->aft.raw.n && nk >= 0 && nk < 512; i++) { uint32_t key = (uint32_t)o->aft.raw.v[i].stat << 16 | o->aft.raw.v[i].layer; int dup = 0; for (int j = 0; j < nk && !dup; j++) dup = keys[j] == key; if (!dup) keys[nk++] = key; }
    static char line[1200]; int q = 0;

    for (int i = 0; i < nk; i++) { int st = keys[i] >> 16, ly = keys[i] & 0xffff; if (st == 6 || st == 8 || st == 10 || st == GEAR_SET_STAT) continue;
        int pv = ly ? dm_gear_list_get(&o->aft.raw, st, ly) : o->aft.value[st], gv = gstat(b, pl, st, ly), was = ly ? dm_gear_list_get(&o->now.raw, st, ly) : o->now.value[st];
        if (pv == gv) { ok++; if (pv != was && q < (int)sizeof line - 40) q += snprintf(line + q, sizeof line - q, " %d/%d %d->%d exact;", st, ly, was, gv); continue; }
        bad++; if (q < (int)sizeof line - 48) q += snprintf(line + q, sizeof line - q, " %d/%d %d->%d pred %d MISMATCH;", st, ly, was, gv, pv); }
    LOG("[cmp] 验算台 套装合成 id %u：%s（准 %d、不准 %d）%s", cid, bad ? "【红】有不准的" : "全准", ok, bad, q ? line : "");
    g_bench_gear_on = 0; return bad;
}
