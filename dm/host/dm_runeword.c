


























#include "hle.h"
#include "game_addrs.h"
#include "dm_rwlv_core.h"
#include "dm_i18n.h"
#include "dm_actions.h"
#include "dm_cursor.h"
#include "dm_recipes.h"
#include "dm_itemjob.h"
#include "dm_input.h"
#include "dm_cursor_item.h"
#include "dm_textbox_core.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NROW 15
enum { PG_INV = 0, PG_CUBE = 3, PG_STASH = 4 };




#define PAGE(d) (*(uint8_t *)((d) + 0x55))

static uint8_t *rb(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t unit_item(uint8_t *b, uint32_t id) { return ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + GA_UNIT_FIND))((uint64_t)(uintptr_t)(b + GA_CLIENT_UNIT_TABLE + 4 * 0x400), id & 0x7f, id, 4); }
static uint64_t player(uint8_t *b) { return ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)); }
static uint64_t tables(uint8_t *b, uint64_t it) { uint8_t v = *(uint8_t *)(it + 0x1bd); return v >= 1 && v <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + v * 16) : 0; }
static int sockets(uint8_t *b, uint64_t it) {
    static int fake = -1; if (fake < 0) { const char *e = getenv("DM_TEST_RW_SOCKETS"); fake = e ? atoi(e) : 0; }
    return fake > 0 ? fake : ((int (*)(uint64_t, int, int))(b + GA_STAT_GET))(it, 0xc2, 0); }
static int filled(uint64_t it) { uint64_t c = *(uint64_t *)(it + 0x90); return c && *(uint64_t *)(c + 0x10); }
static int nfilled(uint64_t it) { uint64_t c = *(uint64_t *)(it + 0x90); int n = 0; for (uint64_t s = c ? *(uint64_t *)(c + 0x10) : 0; s && n < 8; n++) { uint64_t d = *(uint64_t *)(s + 0x10); s = d ? *(uint64_t *)(d + 0xb0) : 0; } return n; }
static int is_type(uint8_t *b, uint64_t it, int t) { return ((int (*)(uint64_t, int))(b + GA_ITEM_IS_TYPE))(it, t); }
static const char *loc_str(uint8_t *b, const char *key) { struct { const char *p; uint64_t n; } k = { key, strlen(key) }; const char *s = ((const char *(*)(void *))(b + GA_LOC_STRING_GET))(&k); return s && *s ? s : key; }






static void loc_tb(uint8_t *b, const char *key, char *out, size_t n) {
    dm_textbox_ensure_loaded();
    const char *s = loc_str(b, key); char *o = NULL; size_t on = 0;
    if (dm_textbox_rewrite(s, strlen(s), &o, &on) && o) { snprintf(out, n, "%s", o); free(o); } else snprintf(out, n, "%s", s);
    { const char *u = dm_untag(out); if (u != out) memmove(out, u, strlen(u) + 1); }
}
#define CC "\xee\x81\xbe"
static void send_pkt(uint8_t *b, const uint8_t *p, int n) { ((void (*)(const uint8_t *, int))(b + GA_CLIENT_SEND))(p, n); }
static int grid_xy(uint64_t it, int *x, int *y) { uint64_t pp = *(uint64_t *)(it + 0x38); if (!pp) return 0; *x = *(int32_t *)(pp + 0x10); *y = *(int32_t *)(pp + 0x14); return 1; }

















typedef struct { char *p; uint64_t len, cap; } gstr_t;
typedef struct { uint64_t d[5]; uint64_t item; } preview_t;
static void g_free(uint8_t *b, void *p) { if (!(*(volatile uint8_t *)(b + GA_GAME_ALLOC_READY) & 1)) return; uint64_t a = *(uint64_t *)(b + GA_GAME_ALLOC); ((void (*)(uint64_t, void *))(*(uint64_t *)(*(uint64_t *)a + 0x28)))(a, p); }
static void preview_drop(uint8_t *b, preview_t *h) {
    uint64_t *obj = (h->d[0] & 1) ? (uint64_t *)(h->d[0] & ~1ull) : (uint64_t *)h;
    if (h->item) { uint64_t it = h->item; ((void (*)(void *, uint64_t *))(*(uint64_t *)(*obj + 0x10)))(obj, &it); h->item = 0; }
    obj = (h->d[0] & 1) ? (uint64_t *)(h->d[0] & ~1ull) : (uint64_t *)h;
    if (obj) { ((void (*)(void *))(*(uint64_t *)(*obj)))(obj); if (h->d[0] & 1) g_free(b, obj); }
    h->d[0] = 1;
}
















static int base_grp(uint64_t t, int ty) {
    uint64_t ir = *(uint64_t *)(t + 0x15c0), in = *(uint64_t *)(t + 0x15c8), bits = *(uint64_t *)(t + 0x1388);
    int32_t ntype = *(int32_t *)(t + 0x1370), stride = *(int32_t *)(t + 0x1380);
    if (!ir || !bits || ty < 0 || ty >= ntype) return -1;
    for (uint64_t i = 0; i < in; i++) { const uint8_t *row = (const uint8_t *)(ir + i * 0x1c0); int16_t it = *(const int16_t *)(row + 0x12e);
        if (it < 0 || it >= ntype) continue;
        if ((*(const uint32_t *)(bits + 4 * ((uint64_t)stride * (uint64_t)it + (uint64_t)(ty >> 5))) >> (ty & 31)) & 1) return row[0x14e]; }
    return -1;
}
static preview_t rw_preview(uint8_t *b, uint64_t t, const uint8_t *e, int force) {
    int ty = 0;
    if (force >= 0 && force < 3 && base_grp(t, *(const int16_t *)(e + 0x4a)) != force) {
        for (int k = 1; k < 6 && !ty; k++) { int16_t c = *(const int16_t *)(e + 0x4a + 2 * k); if (c > 0 && base_grp(t, c) == force) ty = c; }
        if (!ty) LOG("【黄】构建指南预览：符文之语 %u 没有组 %d 的底材，照 itype1 的底材", (unsigned)*(const uint16_t *)(e + 0x46), force);
    }
    if (!ty) return ((preview_t (*)(int, int, int, int))(b + GA_PREVIEW_ITEM_MAKE))(2, *(const uint16_t *)(e + 0x46), 1, 1);
    uint64_t rc[0xe8 / 8]; memcpy(rc, e, 0xe8); int16_t v = (int16_t)ty; memcpy((uint8_t *)rc + 0x4a, &v, 2);
    return ((preview_t (*)(const void *))(b + GA_PREVIEW_RUNEWORD_MAKE))(rc);
}
static int rw_desc_g(uint8_t *b, int rec, int force, char *out, size_t n) {
    out[0] = 0; uint8_t ver = *(uint8_t *)(b + GA_CHAR_DATA_VERSION); if (ver < 1 || ver > 3) return 0;
    uint64_t t = *(uint64_t *)(b + GA_DATA_TABLES + ver * 16), pl = player(b); if (!t || !pl) return 0;
    uint64_t rr = *(uint64_t *)(t + 0x1880); uint32_t rn = *(uint32_t *)(t + 0x1888); if (!rr || rec < 0 || (uint32_t)rec >= rn) return 0;
    const uint8_t *e = (const uint8_t *)(rr + (uint64_t)rec * 0xe8);
    preview_t h = rw_preview(b, t, e, force);
    if (!h.item) { preview_drop(b, &h); return 0; }
    uint8_t props[16 * (7 + 18)]; uint64_t np = 0;
    for (int k = 0; k < 7 && *(int16_t *)(e + 0x74 + 16 * k) >= 0; k++) memcpy(props + 16 * np++, e + 0x74 + 16 * k, 16);
    uint64_t ir = *(uint64_t *)(t + 0x15c0), gt = *(uint64_t *)(t + 0x1678), gn = *(uint64_t *)(t + 0x1680); uint32_t in = *(uint32_t *)(t + 0x15c8);
    int grp = force >= 0 && force < 3 ? force : ((int (*)(uint64_t))(b + GA_ITEM_GEM_GROUP))(h.item);
    for (int k = 0; k < 6 && grp >= 0 && grp < 3 && ir && gt; k++) { int32_t cls = *(int32_t *)(e + 0x5c + 4 * k); if (cls < 0 || (uint32_t)cls >= in) continue;
        int32_t gi = *(int32_t *)(ir + (uint64_t)cls * 0x1c0 + 0xf8); if (gi < 0 || (uint64_t)gi >= gn) continue;
        memcpy(props + 16 * np, (const void *)(gt + (uint64_t)gi * 0xc0 + 0x30 + 0x30 * grp), 0x30); np += 3; }
    struct { uint64_t n; uint8_t has; } cnt = { np, 1 };
    gstr_t sres = ((gstr_t (*)(uint64_t, uint64_t, uint64_t, int, int, int, void *, void *))(b + GA_ITEM_DESCRIBE))(pl, pl, h.item, 0, 1, 0, props, &cnt);
    size_t k = sres.p && sres.len < n ? sres.len : (sres.p ? n - 1 : 0); if (k) memcpy(out, sres.p, k); out[k] = 0;
    if (sres.p && !(sres.cap >> 63)) g_free(b, sres.p);
    preview_drop(b, &h);
    return (int)k;
}

static int rw_desc(uint8_t *b, int rec, char *out, size_t n) { return rw_desc_g(b, rec, -1, out, n); }











static int item_desc(uint8_t *b, int kind, int row, char *out, size_t n) {
    out[0] = 0; uint8_t ver = *(uint8_t *)(b + GA_CHAR_DATA_VERSION); if (ver < 1 || ver > 3) return DM_ITEM_DESC_LATER;
    uint64_t t = *(uint64_t *)(b + GA_DATA_TABLES + ver * 16), pl = player(b); if (!t || !pl) return DM_ITEM_DESC_LATER;
    uint64_t rr = *(uint64_t *)(t + (kind ? 0x13e8 : 0x13c8)), rn = *(uint64_t *)(t + (kind ? 0x13f0 : 0x13d0)); if (!rr || row < 0 || (uint64_t)row >= rn) return 0;
    const uint8_t *rec = (const uint8_t *)(rr + (uint64_t)row * (kind ? 0x15c : 0x1cc));
    preview_t h = ((preview_t (*)(int, int, int, int))(b + GA_PREVIEW_ITEM_MAKE))(kind ? 1 : 0, row, 1, 1);
    if (!h.item) { preview_drop(b, &h); return 0; }
    uint8_t props[16 * 12]; uint64_t np = 0;
    for (int k = 0; k < (kind ? 12 : 9); k++) if (*(int32_t *)(rec + 0x98 + 16 * k) >= 0) memcpy(props + 16 * np++, rec + 0x98 + 16 * k, 16);
    struct { uint64_t n; uint8_t has; } cnt = { np, 1 };
    gstr_t sres = ((gstr_t (*)(uint64_t, uint64_t, uint64_t, int, int, int, void *, void *))(b + GA_ITEM_DESCRIBE))(pl, pl, h.item, 0, 1, 0, props, &cnt);
    size_t k = sres.p && sres.len < n ? sres.len : (sres.p ? n - 1 : 0); if (k) memcpy(out, sres.p, k); out[k] = 0;
    if (sres.p && !(sres.cap >> 63)) g_free(b, sres.p);
    preview_drop(b, &h);
    return (int)k;
}






int dm_item_desc(uint8_t *b, int kind, int row, int grp, char *out, size_t n) {
    if (!b || !out || n < 2) return 0;
    int r;
    if (kind == 0 || kind == 1) r = item_desc(b, kind, row, out, n);
    else if (kind != 2) return 0;
    else {
        uint8_t ver = *(uint8_t *)(b + GA_CHAR_DATA_VERSION); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + ver * 16) : 0;
        if (!t || !player(b)) return DM_ITEM_DESC_LATER;
        uint64_t rr = *(uint64_t *)(t + 0x1880); uint32_t rn = *(uint32_t *)(t + 0x1888);
        if (!rr || row < 0 || (uint32_t)row >= rn || !*(uint8_t *)(rr + (uint64_t)row * 0xe8 + 0x44)) return 0;
        r = rw_desc_g(b, row, grp, out, n);
    }

    if (r > 0 && (size_t)r >= n - 1) { LOG("【红】构建指南预览：种类 %d 行 %d 的说明超过 %zu 字节，不用", kind, row, n - 1); out[0] = 0; return 0; }
    return r;
}

int dm_rw_text(uint8_t *b, int rec, char *out, size_t n) {
    uint8_t ver = *(uint8_t *)(b + GA_CHAR_DATA_VERSION); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + ver * 16) : 0, rr = t ? *(uint64_t *)(t + 0x1880) : 0; uint32_t rn = t ? *(uint32_t *)(t + 0x1888) : 0;
    if (!rr || rec < 0 || (uint32_t)rec >= rn || !*(uint8_t *)(rr + (uint64_t)rec * 0xe8 + 0x44)) return 0;
    return rw_desc(b, rec, out, n) ? 1 : -1;
}



typedef struct { int rec, nr, cls[6], lv, ok, miss, hint; char name[64]; } cand_t;
typedef struct { int inv, stash, shared, stack, cube; } have_t;




static uint32_t shared_unit_id(uint8_t *b); static uint64_t unit_any(uint8_t *b, uint32_t id, int type);


static int owners(uint8_t *b, uint64_t pl, uint64_t us[3]) { us[0] = pl; us[1] = ((uint64_t (*)(uint64_t))(b + GA_ASTASH_UNIT_GET))(pl); uint32_t sid = shared_unit_id(b); us[2] = sid ? unit_any(b, sid, 0) : 0;
    if (!us[2]) us[2] = ((uint64_t (*)(uint64_t))(b + GA_SHARED_STASH_UNIT_GET))(pl); if (us[2] == us[1] || us[2] == pl) us[2] = 0; return 3; }




static uint32_t shared_unit_id(uint8_t *b) {
    uint64_t m = *(uint64_t *)(b + GA_UI_STATE_TABLE), bk = m ? *(uint64_t *)(m + 8) : 0, n = m ? *(uint64_t *)m : 0; if (!bk || !n) return 0;
    const uint64_t key = 0x8d4d18a675312bd4ull; uint64_t node = *(uint64_t *)(bk + 8 * ((n - 1) & 0x1f864eb9001295cdull));
    for (int g = 0; node && g < 64; g++, node = *(uint64_t *)node) if (*(uint64_t *)(node + 8) == key) { uint64_t a1 = *(uint64_t *)(node + 0x10), a2 = a1 ? *(uint64_t *)(a1 + 0x10) : 0; return a2 ? *(uint32_t *)a2 : 0; }
    return 0;
}
static uint64_t unit_any(uint8_t *b, uint32_t id, int type) { return ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + GA_UNIT_FIND))((uint64_t)(uintptr_t)(b + GA_CLIENT_UNIT_TABLE + type * 0x400), id & 0x7f, id, (uint32_t)type); }
int dm_rw_owners(uint8_t *b, uint64_t pl, uint64_t us[3]) { return owners(b, pl, us); }
uint32_t dm_rw_shared_unit(uint8_t *b) { return shared_unit_id(b); }
static uint64_t first_item(uint64_t u) { uint64_t c = u ? *(uint64_t *)(u + 0x90) : 0; return c && *(uint32_t *)c == 0x01020304 ? *(uint64_t *)(c + 0x10) : 0; }
static struct { uint32_t base; int n; cand_t c[NROW]; int open; } g_list;
static int rune_lv(uint8_t *b, uint64_t t, int cls, int lvoff) { uint64_t ir = *(uint64_t *)(t + 0x15c0); uint32_t in = *(uint32_t *)(t + 0x15c8); return ir && cls >= 0 && (uint32_t)cls < in && lvoff >= 0 ? *(uint8_t *)(ir + (uint64_t)cls * 0x1c0 + lvoff) : 0; }
static void rune_code(uint64_t t, int cls, char out[4]) { uint64_t ir = *(uint64_t *)(t + 0x15c0); uint32_t in = *(uint32_t *)(t + 0x15c8); memset(out, 0, 4); if (ir && cls >= 0 && (uint32_t)cls < in) memcpy(out, (const void *)(ir + (uint64_t)cls * 0x1c0 + 0x80), 3); }
static int g_lvoff = -1;



static int base_ok(uint8_t *b, uint64_t it, uint64_t t) {
    uint64_t d = *(uint64_t *)(it + 0x10); if (!d || *(uint32_t *)it != 4) return 0;
    if (*(uint32_t *)d >= 4) return 0;
    uint64_t ir = *(uint64_t *)(t + 0x15c0); uint32_t cls = *(uint32_t *)(it + 4); if (!ir || cls >= *(uint32_t *)(t + 0x15c8) || *(uint8_t *)(ir + (uint64_t)cls * 0x1c0 + 0x13a)) return 0;
    return sockets(b, it) > 0 && !filled(it);
}
static void count_have(uint8_t *b, uint64_t pl, int cls, have_t *h) {
    memset(h, 0, sizeof *h); uint64_t us[3]; owners(b, pl, us);
    for (int o = 0; o < 3; o++) for (uint64_t it = first_item(us[o]); it; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if ((int)*(uint32_t *)(it + 4) == cls && *(uint32_t *)(it + 0xc) == 0) { int pg = PAGE(d), st = *(uint8_t *)(d + 0x9c);
            if (st) h->stack += st; else if (o) h->shared++; else if (pg == PG_INV) h->inv++; else if (pg == PG_CUBE) h->cube++; else h->stash++; }
        it = *(uint64_t *)(d + 0xb0); }
}
static int find_lvoff(uint64_t t) {
    uint64_t ir = *(uint64_t *)(t + 0x15c0); uint32_t in = *(uint32_t *)(t + 0x15c8); int o = ir ? dm_rw_find_lvoff((const uint8_t *)ir, in, 0x1c0) : -1;


    const char *e = getenv("DM_TEST_RW_LVDUMP"); FILE *f = e && o >= 0 ? fopen(e, "w") : NULL;
    if (f) { for (uint32_t i = 0; i < in; i++) { const uint8_t *r = (const uint8_t *)(ir + (uint64_t)i * 0x1c0); char c[5] = {0}; memcpy(c, r + 0x80, 4); for (int k = 0; k < 4; k++) if (c[k] == ' ') c[k] = 0; fprintf(f, "%s\t%u\n", c, r[o]); } fclose(f); LOG("符文之语：DM_TEST_RW_LVDUMP 写了 %u 行 → %s", in, e); }
    return o;
}
static int cand_cmp(const void *a, const void *b2) { const cand_t *x = a, *y = b2; if (x->ok != y->ok) return y->ok - x->ok; if (x->miss != y->miss) return x->miss - y->miss; return x->lv - y->lv; }


static int base_where(uint8_t *b, uint64_t pl, uint64_t it, int *page, int *x, int *y, int *slot);
static int build_list(uint8_t *b, uint64_t it) {
    uint64_t t = tables(b, it), pl = player(b); if (!t || !pl || !base_ok(b, it, t)) return 0;
    if (g_lvoff == -1) { g_lvoff = find_lvoff(t); if (g_lvoff < 0) LOG("【红】符文之语：物品表里没找到「需求等级」那一列（锚点物品对不上），清单不显示等级"); else LOG("符文之语：物品表「需求等级」偏移 %#x", g_lvoff); if (g_lvoff < 0) g_lvoff = -2; }
    int ns = sockets(b, it), stash_open = *(uint8_t *)(b + GA_STASH_OPEN) != 0, bp = 0, bx = 0, by = 0, bsl = 0, bw = base_where(b, pl, it, &bp, &bx, &by, &bsl);
    int can_put = bw && !(bw == 1 && bp == PG_STASH && !stash_open);
    uint64_t rr = *(uint64_t *)(t + 0x1880), rn = *(uint64_t *)(t + 0x1888); g_list.n = 0;


    int dm_diag_ladder(int *offline, int *ladder); int offline = 0, ladder = 0, known = dm_diag_ladder(&offline, &ladder);
    for (uint64_t r = 0; rr && r < rn && g_list.n < NROW; r++) { const uint8_t *e = (const uint8_t *)(rr + r * 0xe8);
        if (!e[0x44] || !dm_rw_allowed(e[0xe4], *(const int16_t *)(e + 0x40), *(const int16_t *)(e + 0x42), known, offline, ladder)) continue;
        int nr = 0, cls[6]; for (int k = 0; k < 6; k++) { int v = *(const int32_t *)(e + 0x5c + 4 * k); if (v >= 1) cls[nr++] = v; }
        if (nr != ns) continue;
        int bad = 0; for (int k = 0; k < 3 && !bad; k++) { int ty = *(const int16_t *)(e + 0x56 + 2 * k); if (ty && is_type(b, it, ty)) bad = 1; }
        int good = 0; for (int k = 0; k < 6 && !good && !bad; k++) { int ty = *(const int16_t *)(e + 0x4a + 2 * k); if (ty && is_type(b, it, ty)) good = 1; }
        if (bad || !good) continue;
        cand_t *c = &g_list.c[g_list.n++]; memset(c, 0, sizeof *c); c->rec = (int)r; c->nr = nr; memcpy(c->cls, cls, sizeof cls); c->ok = can_put; int runes_ok = 1;
        int base_req = rune_lv(b, t, *(int32_t *)(it + 4), g_lvoff), rreq[6];
        char key[64]; memcpy(key, e, 63); key[63] = 0; loc_tb(b, key, c->name, sizeof c->name);
        for (int k = 0; k < nr; k++) rreq[k] = rune_lv(b, t, cls[k], g_lvoff);
        c->lv = dm_rw_need_level(base_req, rreq, nr);
        for (int k = 0; k < nr; k++) {
            int need = 0; for (int j = 0; j < nr; j++) need += cls[j] == cls[k]; have_t h; count_have(b, pl, cls[k], &h);
            int usable = h.inv + (stash_open ? h.stash + h.shared + h.stack : 0), total = h.inv + h.stash + h.shared + h.stack + h.cube;
            if (usable < need) { c->ok = 0; runes_ok = 0; }
            if (total < need) { int dup = 0; for (int j = 0; j < k; j++) dup |= cls[j] == cls[k]; if (!dup) c->miss += need - total; } }
        c->hint = runes_ok && !can_put && !bw; }
    qsort(g_list.c, g_list.n, sizeof *g_list.c, cand_cmp);
    return g_list.n;
}



int dm_rw_bases_for(uint8_t *b, uint64_t it, char *out, size_t n) {
    if (n) out[0] = 0;
    uint64_t t = it ? tables(b, it) : 0; if (!t || !base_ok(b, it, t) || !n) return 0;
    int dm_diag_ladder(int *offline, int *ladder);
    int ns = sockets(b, it), offline = 0, ladder = 0, known = dm_diag_ladder(&offline, &ladder), count = 0; size_t at = 0;
    uint64_t rr = *(uint64_t *)(t + 0x1880), rn = *(uint64_t *)(t + 0x1888);
    for (uint64_t r = 0; rr && r < rn; r++) { const uint8_t *e = (const uint8_t *)(rr + r * 0xe8);
        if (!e[0x44] || !dm_rw_allowed(e[0xe4], *(const int16_t *)(e + 0x40), *(const int16_t *)(e + 0x42), known, offline, ladder)) continue;
        int nr = 0; for (int k = 0; k < 6; k++) if (*(const int32_t *)(e + 0x5c + 4 * k) >= 1) nr++;
        if (nr != ns) continue;
        int bad = 0, good = 0;
        for (int k = 0; k < 3 && !bad; k++) { int ty = *(const int16_t *)(e + 0x56 + 2 * k); if (ty && is_type(b, it, ty)) bad = 1; }
        for (int k = 0; k < 6 && !good && !bad; k++) { int ty = *(const int16_t *)(e + 0x4a + 2 * k); if (ty && is_type(b, it, ty)) good = 1; }
        if (bad || !good) continue;
        char key[64], name[64]; memcpy(key, e, 63); key[63] = 0; loc_tb(b, key, name, sizeof name);
        int w = snprintf(out + at, n - at, "%s%s", count ? ", " : "", name);
        if (w < 0 || (size_t)w >= n - at) { out[at] = 0; break; }
        at += (size_t)w; count++; }
    return count;
}



static uint64_t find_w(uint64_t w, const char *name, int dep) {
    if (!w || dep > 6) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 512) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find_w(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static void set_text(uint8_t *b, uint64_t w, const char *s) {
    if (!w || *(uint64_t *)w - (uint64_t)(uintptr_t)b != GA_VT_TEXTBOX) return;
    size_t n = strlen(s); if (*(uint64_t *)(w + 0x90) == n && !memcmp(*(const char **)(w + 0x88), s, n)) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(b + GA_TEXT_SET))(w + 0x88, s, n); ((void (*)(uint64_t))(b + GA_TEXT_LAYOUT))(w);
}
static void place(uint64_t w, int x, int y) { if (!w) return; int32_t *r = (int32_t *)(w + 0x70); if (r[0] == x && r[1] == y) return; r[0] = x; r[1] = y; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); }
#define OFF 99999






enum { RW_X = 262, RW_W = 1125, RW_Y0 = 60, RW_HEAD = 80, RW_L1 = 56, RW_LH = 48, RW_RGAP = 28, RW_PAD = 36, RW_GAP = 22, RW_NAV_H = 80, RW_PG_W = 230, RW_YMAX = 1470, RW_PER_PAGE = 8 };
static int g_inv_open;
static uint64_t inv_panel(uint8_t *b) { if (!g_inv_open) return 0; uint64_t root = *(uint64_t *)(b + GA_UI_ROOT); return root ? find_w(root, "PlayerInventoryExpansionLayout", 0) : 0; }




static struct { uint64_t panel, bg, title, prev, next, pg, sep[RW_PER_PAGE - 1], desc, descs, back, backtxt, dgo, sel, name[NROW], lv[NROW], rw[NROW][6], rr[NROW][6], go[NROW], hint[NROW];
    int page, pages, shown_pg, shown_pages, first, last, bot, det, psel, rowy[NROW], red[NROW][6], rx[NROW][6], ry[NROW][6], rh[NROW], pstart[NROW + 1]; uint8_t *b; uint32_t ui_gen; } W;
static int g_check_at;
static int text_w(uint64_t w) { int x = w ? *(int32_t *)(w + 0x78) : 0; return x > 0 && x < 2000 ? x : 0; }
static int text_h(uint64_t w) { return w ? *(int32_t *)(w + 0x7c) : 0; }




static int real_w(uint64_t w) { return w ? (int)(*(int32_t *)(w + 0x78) * *(float *)(w + 0x80)) : 0; }
static int real_h(uint64_t w) { return w ? (int)(*(int32_t *)(w + 0x7c) * *(float *)(w + 0x80)) : 0; }
static int btn_w(void) { return real_w(W.go[0]); }
static int btn_h(void) { return real_h(W.go[0]); }


static void ui_lookup(uint64_t p, typeof(W) *o) {
    char nm[40]; o->panel = p;
    o->bg = find_w(p, "TouchRwBg", 0); o->title = find_w(p, "TouchRwTitle", 0); o->prev = find_w(p, "TouchRwPgPrev", 0); o->next = find_w(p, "TouchRwPgNext", 0); o->pg = find_w(p, "TouchRwPage", 0);
    for (int k = 0; k < RW_PER_PAGE - 1; k++) { snprintf(nm, sizeof nm, "TouchRwSep%d", k); o->sep[k] = find_w(p, nm, 0); }
    o->desc = find_w(p, "TouchRwDesc", 0); o->descs = find_w(p, "TouchRwDescS", 0); o->back = find_w(p, "TouchRwBack", 0); o->backtxt = find_w(p, "TouchRwBackTxt", 0); o->dgo = find_w(p, "TouchRwDetGo", 0); o->sel = find_w(p, "TouchRwSel", 0);
    for (int i = 0; i < NROW; i++) { snprintf(nm, sizeof nm, "TouchRwName%d", i); o->name[i] = find_w(p, nm, 0); snprintf(nm, sizeof nm, "TouchRwLv%d", i); o->lv[i] = find_w(p, nm, 0);
        snprintf(nm, sizeof nm, "TouchRwGo%d", i); o->go[i] = find_w(p, nm, 0); snprintf(nm, sizeof nm, "TouchRwHint%d", i); o->hint[i] = find_w(p, nm, 0);
        for (int j = 0; j < 6; j++) { snprintf(nm, sizeof nm, "TouchRwR%d_%dw", i, j); o->rw[i][j] = find_w(p, nm, 0); snprintf(nm, sizeof nm, "TouchRwR%d_%dr", i, j); o->rr[i][j] = find_w(p, nm, 0); } }
}
static void ui_cache(uint8_t *b) {
    uint64_t p = inv_panel(b); memset(&W, 0, sizeof W); W.ui_gen = dm_ui_gen(); W.panel = p; if (!p) return;
    ui_lookup(p, &W); W.shown_pg = -1; W.det = -1;
}
static void ui_hide_all(void) {
    place(W.bg, OFF, OFF); place(W.title, OFF, OFF); place(W.prev, OFF, OFF); place(W.next, OFF, OFF); place(W.pg, OFF, OFF); for (int k = 0; k < RW_PER_PAGE - 1; k++) place(W.sep[k], OFF, OFF);
    place(W.desc, OFF, OFF); place(W.descs, OFF, OFF); place(W.back, OFF, OFF); place(W.backtxt, OFF, OFF); place(W.dgo, OFF, OFF); place(W.sel, OFF, OFF);
    for (int i = 0; i < NROW; i++) { place(W.name[i], OFF, OFF); place(W.lv[i], OFF, OFF); place(W.go[i], OFF, OFF); place(W.hint[i], OFF, OFF); for (int j = 0; j < 6; j++) { place(W.rw[i][j], OFF, OFF); place(W.rr[i][j], OFF, OFF); } }
}
static int det_page(void) { return W.det >= 0; }
static int box_h(void) { return W.bot - RW_Y0 + (!det_page() && W.pages > 1 ? RW_NAV_H : 0) + 10; }


static void set_page_text(void) {
    if (!W.b || !W.pg || (W.shown_pg == W.page && W.shown_pages == W.pages)) return;
    char f[64], s[96]; loc_tb(W.b, "SharedStashPage", f, sizeof f); int v[2] = { W.page + 1, W.pages }, k = 0; size_t o = 0;
    for (const char *q = f; *q && o + 12 < sizeof s; ) { if (q[0] == '%' && q[1] == 'u' && k < 2) { o += snprintf(s + o, sizeof s - o, "%d", v[k++]); q += 2; } else s[o++] = *q++; }
    s[o] = 0; set_text(W.b, W.pg, s); W.shown_pg = W.page; W.shown_pages = W.pages;
}






static void det_layout(void);
static void ui_layout(void) {
    if (!W.panel) return;
    if (det_page()) { place(W.sel, OFF, OFF); det_layout(); return; }
    int x0 = RW_X + RW_PAD + 12, right = RW_X + RW_W - RW_PAD, top = RW_Y0 + RW_HEAD, n = g_list.n < NROW ? g_list.n : NROW, all = 0;
    for (int i = 0; i < n; i++) { cand_t *c = &g_list.c[i]; int lim = right - (c->ok ? btn_w() + RW_GAP : c->hint ? text_w(W.hint[i]) + RW_GAP : 0), x = x0, ly = RW_L1, lines = 1;
        for (int j = 0; j < c->nr; j++) { int w = text_w(W.red[i][j] ? W.rr[i][j] : W.rw[i][j]);
            if (x > x0 && x + w > lim) { x = x0; ly += RW_LH; lines++; }
            W.rx[i][j] = x; W.ry[i][j] = ly; x += w + RW_GAP; }
        int h = RW_L1 + lines * RW_LH; if (c->ok && h < btn_h()) h = btn_h(); W.rh[i] = h + RW_RGAP; all += W.rh[i]; }
    W.pages = 0; W.pstart[0] = 0;
    if (top + all <= RW_YMAX && n <= RW_PER_PAGE) { W.pages = 1; W.pstart[1] = n; }
    else for (int i = 0, y = top; i < n; i++) { if ((y + W.rh[i] > RW_YMAX - RW_NAV_H || i - W.pstart[W.pages] >= RW_PER_PAGE) && i > W.pstart[W.pages]) { W.pstart[++W.pages] = i; y = top; } y += W.rh[i]; if (i == n - 1) W.pstart[++W.pages] = n; }
    if (W.pages < 1) { W.pages = 1; W.pstart[1] = n; }
    if (W.page >= W.pages) W.page = W.pages - 1; if (W.page < 0) W.page = 0;
    W.first = W.pstart[W.page]; W.last = W.pstart[W.page + 1];
    int y = top; for (int i = W.first; i < W.last; i++) y += W.rh[i]; W.bot = y;
    place(W.bg, RW_X, RW_Y0); { int32_t *r = (int32_t *)(W.bg + 0x70); int h = box_h(); if (W.bg && r[3] != h) { r[3] = h; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)W.bg + 0x10))(W.bg); } }
    place(W.title, RW_X, RW_Y0 + 14);
    y = top;
    for (int i = 0; i < NROW; i++) {
        int vis = i >= W.first && i < W.last, ok = vis && g_list.c[i].ok;
        place(W.name[i], vis ? RW_X + RW_PAD : OFF, vis ? y : OFF);
        place(W.lv[i], vis ? RW_X + RW_PAD + text_w(W.name[i]) + 28 : OFF, vis ? y + 4 : OFF);
        place(W.go[i], ok ? right + 12 - btn_w() : OFF, ok ? y + (W.rh[i] - RW_RGAP - btn_h()) / 2 : OFF);
        { int hv = vis && g_list.c[i].hint; place(W.hint[i], hv ? right - text_w(W.hint[i]) : OFF, hv ? y + (W.rh[i] - RW_RGAP - text_h(W.hint[i])) / 2 : OFF); }
        for (int j = 0; j < 6; j++) { int on = vis && j < g_list.c[i].nr; uint64_t a = on && W.red[i][j] ? W.rr[i][j] : W.rw[i][j], o = a == W.rw[i][j] ? W.rr[i][j] : W.rw[i][j];
            place(o, OFF, OFF); place(a, on ? W.rx[i][j] : OFF, on ? y + W.ry[i][j] : OFF); }
        if (vis) { W.rowy[i] = y; y += W.rh[i]; } }


    { int dm_padmode_current(void); int i = W.psel, on = dm_padmode_current() && i >= W.first && i < W.last; if (W.sel) { int32_t *r = (int32_t *)(W.sel + 0x70); int w = RW_W - 2 * RW_PAD + 48, h = W.rh[i < NROW ? i : 0] - RW_RGAP + 28;
        place(W.sel, on ? RW_X + RW_PAD - 24 : OFF, on ? W.rowy[i] - 14 : OFF); if (on && (r[2] != w || r[3] != h)) { r[2] = w; r[3] = h; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)W.sel + 0x10))(W.sel); } } }


    for (int k = 0, yy = top; k < RW_PER_PAGE - 1; k++) { int i = W.first + k, on = i + 1 < W.last; if (on) yy += W.rh[i];
        place(W.sep[k], on ? RW_X + (RW_W - real_w(W.sep[k])) / 2 : OFF, on ? yy - RW_RGAP / 2 - real_h(W.sep[k]) / 2 : OFF); }


    int nav = W.pages > 1, cx = RW_X + RW_W / 2, ny = W.bot + (RW_NAV_H - 64) / 2;
    place(W.pg, nav ? cx - RW_PG_W / 2 : OFF, nav ? ny : OFF);
    place(W.prev, nav ? cx - RW_PG_W / 2 - 2 - real_w(W.prev) : OFF, nav ? ny : OFF);
    place(W.next, nav ? cx + RW_PG_W / 2 + 2 : OFF, nav ? ny : OFF);
    if (nav) set_page_text();
    det_layout();
}


static uint64_t det_desc_pick(int room) {
    return text_h(W.desc) > 0 && text_h(W.desc) <= room && text_w(W.desc) <= RW_W - 24 ? W.desc : W.descs; }
static void det_layout(void) {
    if (W.det < 0) { place(W.desc, OFF, OFF); place(W.descs, OFF, OFF); place(W.back, OFF, OFF); place(W.backtxt, OFF, OFF); place(W.dgo, OFF, OFF); return; }
    int ok = g_list.c[W.det].ok, bw = real_w(W.dgo), bh = real_h(W.dgo), room = RW_YMAX - (RW_Y0 + 100) - (ok ? bh + 24 : 0) - 14;
    uint64_t dt = det_desc_pick(room), du = dt == W.desc ? W.descs : W.desc; place(du, OFF, OFF);
    int dw = text_w(dt), dh = text_h(dt);
    for (int i = 0; i < NROW; i++) { place(W.name[i], OFF, OFF); place(W.lv[i], OFF, OFF); place(W.go[i], OFF, OFF); place(W.hint[i], OFF, OFF); for (int j = 0; j < 6; j++) { place(W.rw[i][j], OFF, OFF); place(W.rr[i][j], OFF, OFF); } }
    for (int k = 0; k < RW_PER_PAGE - 1; k++) place(W.sep[k], OFF, OFF);
    place(W.prev, OFF, OFF); place(W.next, OFF, OFF); place(W.pg, OFF, OFF); place(W.title, OFF, OFF);
    place(W.back, RW_X + RW_PAD, RW_Y0 + 12); place(W.backtxt, RW_X + RW_PAD + real_w(W.back) + 8, RW_Y0 + 12);
    int y = RW_Y0 + 12 + 64 + 24; place(dt, RW_X + (RW_W - dw) / 2, y); y += dh;
    if (ok) { y += 24; place(W.dgo, RW_X + (RW_W - bw) / 2, y); y += bh; } else place(W.dgo, OFF, OFF);
    W.bot = y + 14;
    place(W.bg, RW_X, RW_Y0); { int32_t *r = (int32_t *)(W.bg + 0x70); int h = box_h(); if (W.bg && r[3] != h) { r[3] = h; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)W.bg + 0x10))(W.bg); } }
}






static int det_check(int log);
static int ui_check(int log) {
    if (!W.panel) return -1;
    if (W.det >= 0) return det_check(log); int bx = RW_X, by = RW_Y0, bw = RW_W, bh = box_h(), n = 0, out = 0, ovl = 0; struct { int x, y, w, h; const char *nm; } r[NROW * 10 + 8 + RW_PER_PAGE];
    #define ADD(wd, ww, hh, dy) do { if ((wd) && ((int32_t *)((wd) + 0x70))[0] != OFF) { r[n].x = ((int32_t *)((wd) + 0x70))[0]; r[n].y = ((int32_t *)((wd) + 0x70))[1] + (dy); r[n].w = (ww); r[n].h = (hh); r[n].nm = *(const char **)((wd) + 8); n++; } } while (0)
    #define TXT(wd) ADD(wd, text_w(wd), text_h(wd) * 3 / 5, text_h(wd) / 9)
    ADD(W.title, RW_W, 56, 0); ADD(W.pg, RW_PG_W, 64, 0); ADD(W.prev, real_w(W.prev), real_h(W.prev), 0); ADD(W.next, real_w(W.next), real_h(W.next), 0);
    for (int k = 0; k < RW_PER_PAGE - 1; k++) ADD(W.sep[k], real_w(W.sep[k]), real_h(W.sep[k]), 0);
    if (log && W.sep[0]) { int32_t *q = (int32_t *)(W.sep[0] + 0x70); LOG("符文之语：分隔线控件矩形 %d,%d %dx%d 缩放 %.2f", q[0], q[1], q[2], q[3], *(float *)(W.sep[0] + 0x80)); }
    for (int i = 0; i < NROW; i++) { TXT(W.name[i]); TXT(W.lv[i]); ADD(W.go[i], btn_w(), btn_h(), 0); TXT(W.hint[i]);
        for (int j = 0; j < 6; j++) { TXT(W.rw[i][j]); TXT(W.rr[i][j]); } }
    #undef TXT
    #undef ADD
    const char *bad = "";
    for (int a = 0; a < n && log; a++) if (r[a].x < bx || r[a].y < by || r[a].x + r[a].w > bx + bw || r[a].y + r[a].h > by + bh) { LOG("符文之语：排版自检 首个出底板 %s (%d,%d %dx%d)", r[a].nm, r[a].x, r[a].y, r[a].w, r[a].h); break; }
    for (int a = 0; a < n; a++) { if (r[a].x < bx || r[a].y < by || r[a].x + r[a].w > bx + bw || r[a].y + r[a].h > by + bh) { out++; bad = r[a].nm; }
        for (int c = a + 1; c < n; c++) if (r[a].x < r[c].x + r[c].w && r[c].x < r[a].x + r[a].w && r[a].y < r[c].y + r[c].h && r[c].y < r[a].y + r[a].h) {
            if (!ovl++ && log) LOG("符文之语：排版自检 首个重叠 %s (%d,%d %dx%d) × %s (%d,%d %dx%d)", r[a].nm, r[a].x, r[a].y, r[a].w, r[a].h, r[c].nm, r[c].x, r[c].y, r[c].w, r[c].h); if (!*bad) bad = r[a].nm; } }
    if (log) LOG("%s符文之语：排版自检 第 %d/%d 页 %d 个控件，出底板 %d、重叠 %d%s%s（底板 %d,%d %dx%d，面板 %dx%d）", out || ovl ? "【红】" : "", W.page + 1, W.pages, n, out, ovl, *bad ? "，例：" : "", bad, bx, by, bw, bh,
                 ((int32_t *)(W.panel + 0x70))[2], ((int32_t *)(W.panel + 0x70))[3]);
    return out + ovl;
}


static int det_check(int log) {
    uint64_t box = W.bg; if (!box) return -1; int32_t *br = (int32_t *)(box + 0x70);
    int bx = br[0], by = br[1], bw = br[2], bh = box_h(), n = 0, out = 0, ovl = 0; struct { int x, y, w, h; const char *nm; } r[7];
    #define ADD(wd, ww, hh) do { if ((wd) && ((int32_t *)((wd) + 0x70))[0] != OFF) { r[n].x = ((int32_t *)((wd) + 0x70))[0]; r[n].y = ((int32_t *)((wd) + 0x70))[1]; r[n].w = (ww); r[n].h = (hh); r[n].nm = *(const char **)((wd) + 8); n++; } } while (0)
    ADD(W.desc, text_w(W.desc), text_h(W.desc)); ADD(W.descs, text_w(W.descs), text_h(W.descs)); ADD(W.dgo, real_w(W.dgo), real_h(W.dgo)); ADD(W.back, real_w(W.back), real_h(W.back)); ADD(W.backtxt, text_w(W.backtxt), text_h(W.backtxt));
    #undef ADD
    const char *bad = "";
    for (int a = 0; a < n; a++) { if (r[a].x < bx || r[a].y < by || r[a].x + r[a].w > bx + bw || r[a].y + r[a].h > by + bh) { out++; bad = r[a].nm; if (log) LOG("符文之语：详情自检 出框 %s (%d,%d %dx%d)", r[a].nm, r[a].x, r[a].y, r[a].w, r[a].h); }
        for (int c = a + 1; c < n; c++) if (r[a].x < r[c].x + r[c].w && r[c].x < r[a].x + r[a].w && r[a].y < r[c].y + r[c].h && r[c].y < r[a].y + r[a].h) { ovl++; if (!*bad) bad = r[a].nm; } }
    if (log) LOG("%s符文之语：详情自检「%s」%d 个控件，出框 %d、重叠 %d%s%s（框 %d,%d %dx%d，说明用 %s 号 %dx%d）", out || ovl ? "【红】" : "", g_list.c[W.det].name, n, out, ovl,
                 *bad ? "，例：" : "", bad, bx, by, bw, bh, ((int32_t *)(W.desc + 0x70))[0] != OFF ? "32" : "22", ((int32_t *)(W.desc + 0x70))[0] != OFF ? text_w(W.desc) : text_w(W.descs), ((int32_t *)(W.desc + 0x70))[0] != OFF ? text_h(W.desc) : text_h(W.descs));
    return out + ovl;
}




static void det_text(uint8_t *b, int i, char *out, size_t n) {
    static char d[4096]; out[0] = 0; if (!rw_desc(b, g_list.c[i].rec, d, sizeof d)) return;
    uint64_t it = unit_item(b, g_list.base); char bn[96] = "", lv[64] = "", so[64] = "", ins[300];
    if (it) { uint32_t code = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it); char ck[5] = {0}; memcpy(ck, &code, 4); for (int k = 3; k >= 0 && ck[k] == ' '; k--) ck[k] = 0; loc_tb(b, ck, bn, sizeof bn); }
    char f[64]; loc_tb(b, "ItemStats1p", f, sizeof f); { char *q = strstr(f, "%d"); if (q && g_list.c[i].lv) { *q = 0; snprintf(lv, sizeof lv, "%s%d%s", f, g_list.c[i].lv, q + 2); } }
    loc_tb(b, "Socketable", f, sizeof f); { char *q = strstr(f, "%i"); if (q && it) { *q = 0; snprintf(so, sizeof so, "%s%d%s", f, sockets(b, it), q + 2); } }
    snprintf(ins, sizeof ins, "\n" CC "0%s%s%s%s%s", so, *lv ? "\n" CC "0" : "", lv, *bn ? "\n" CC "0" : "", bn);
    char *rl = strstr(d, "\n" CC "4'"); size_t at = rl ? (size_t)(rl - d) : strlen(d);
    snprintf(out, n, "%.*s%s%s", (int)at, d, ins, d + at);
}
static void det_open(uint8_t *b, int i) {
    static char t[4600]; if (i < 0 || i >= g_list.n || i >= NROW) return;
    det_text(b, i, t, sizeof t); if (!t[0]) { LOG("【红】符文之语：「%s」的原版说明没生成出来，不开详情", g_list.c[i].name); return; }
    W.det = i; set_text(b, W.desc, t); set_text(b, W.descs, t); ui_layout(); g_check_at = 6;
    LOG("符文之语：详情「%s」（%zu 字节）", g_list.c[i].name, strlen(t));
}
static void ui_fill(uint8_t *b, uint64_t it) {
    uint64_t t = tables(b, it), pl = player(b); if (!W.panel || !t || !pl) return; char s[256];
    uint32_t code = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it); char ck[5] = {0}; memcpy(ck, &code, 4); for (int k = 3; k >= 0 && ck[k] == ' '; k--) ck[k] = 0;
    char bn[96]; loc_tb(b, ck, bn, sizeof bn); snprintf(s, sizeof s, dm_str(b, "dmRwTitle"), bn, sockets(b, it)); set_text(b, W.title, s);
    W.b = b;
    for (int i = 0; i < g_list.n && i < NROW; i++) { cand_t *c = &g_list.c[i];
        set_text(b, W.name[i], c->name); if (c->lv) snprintf(s, sizeof s, dm_str(b, "dmRwLevel"), c->lv); else s[0] = 0; set_text(b, W.lv[i], s);
        for (int j = 0; j < c->nr; j++) {
            char rc[4]; rune_code(t, c->cls[j], rc); int num = atoi(rc + 1);
            char rk[8]; snprintf(rk, sizeof rk, "%sL", rc);
            char rn[64]; loc_tb(b, rk, rn, sizeof rn); char *hs = strchr(rn, '#'); if (hs) *hs = 0; for (int k = (int)strlen(rn) - 1; k >= 0 && rn[k] == ' '; k--) rn[k] = 0;
            have_t h; count_have(b, pl, c->cls[j], &h); int seen = 0; for (int q = 0; q < j; q++) seen += c->cls[q] == c->cls[j];
            int total = h.inv + h.stash + h.shared + h.stack + h.cube; W.red[i][j] = total <= seen;
            snprintf(s, sizeof s, "#%d %s", num, rn); set_text(b, W.rw[i][j], s); set_text(b, W.rr[i][j], s); } }
}









static int w_stale(void) {
    if (!W.panel || W.ui_gen == dm_ui_gen()) return 0;
    uint32_t was = W.ui_gen; if (!dm_ui_gen_revalidate(&W.ui_gen, W.panel)) return 1;
    static typeof(W) now; memset(&now, 0, sizeof now); ui_lookup(W.panel, &now);
    if (!memcmp(&now, &W, offsetof(typeof(W), page))) return 0;
    W.ui_gen = was; { static int lg; if (lg++ < 20) LOG("符文之语：界面重建后物品栏面板还在原地址，清单子控件却换了 → 整组缓存作废"); }
    return 1;
}
static void ui_show(uint8_t *b, int on) { if (on) { ui_cache(b); ui_layout(); } else { if (!W.panel) ui_cache(b); if (!w_stale()) ui_hide_all(); memset(&W, 0, sizeof W); } }
static void list_close(uint8_t *b, const char *why) { if (!g_list.open) return; g_list.open = 0; ui_show(b, 0); LOG("符文之语：清单关闭（%s）", why); }









void dm_runeword_ui_rebuild(void) {
    int was = g_list.open; g_list.open = 0; uint8_t *b = W.b; memset(&W, 0, sizeof W); W.b = b; g_check_at = 0;
    if (was) LOG("符文之语：界面要重建（手柄 / 触屏切换）→ 清单当作已关，缓存的控件指针作废");
}













enum { SRC_GRID = 1, SRC_STACK, SRC_SHARED };
typedef struct { int kind, x, y, page; uint32_t id, unit; } src_t;
static struct { int st, rec, nr, k, wait, cls[6], bkind, bpage, bx, by, bslot, pend, stack_before; uint32_t base, rune; src_t src[6]; dm_itemjob_settle settle; } g_job;


static uint64_t held_item(uint8_t *b, uint64_t pl) { uint64_t inv = pl ? *(uint64_t *)(pl + 0x90) : 0, h = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0; (void)b; return h && *(uint32_t *)h == 4 ? h : 0; }


static int base_where(uint8_t *b, uint64_t pl, uint64_t it, int *page, int *x, int *y, int *slot) {
    uint64_t d = it ? *(uint64_t *)(it + 0x10) : 0; if (!d || !pl) return 0; int own = 0;
    for (uint64_t q = first_item(pl); q && !own; ) { uint64_t qd = *(uint64_t *)(q + 0x10); if (!qd) break; own = q == it; q = *(uint64_t *)(qd + 0xb0); }
    if (!own) return 0;
    uint32_t mode = *(uint32_t *)(it + 0xc);
    if (mode == 0) { int pg = PAGE(d); if (pg != PG_INV && pg != PG_CUBE && pg != PG_STASH) return 0; *page = pg; grid_xy(it, x, y); return 1; }
    if (mode == 1) { int sl = *(uint8_t *)(d + 0x54); if (sl < 1 || sl > 10) return 0; *slot = sl; return 2; }
    return 0;
}


static int pick_src(uint8_t *b, uint64_t pl, int cls, int stash_open, int k, src_t *out) {
    uint64_t us[3]; owners(b, pl, us);
    for (int pass = 0; pass < 4; pass++) for (int o = 0; o < 3; o++) for (uint64_t it = first_item(us[o]); it; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if ((int)*(uint32_t *)(it + 4) == cls && *(uint32_t *)(it + 0xc) == 0) {
            int pg = PAGE(d), st = *(uint8_t *)(d + 0x9c); uint32_t id = *(uint32_t *)(it + 8);
            int kind = st ? SRC_STACK : o ? SRC_SHARED : pg == PG_INV ? SRC_GRID : pg == PG_STASH ? SRC_GRID : 0, rank = st ? 2 : o ? 3 : pg == PG_INV ? 0 : 1;
            if (kind && rank == pass && (rank == 0 || stash_open)) {
                int used = 0; for (int j = 0; j < k; j++) used += g_job.src[j].id == id;
                if (used < (st ? st : 1)) { out->kind = kind; out->id = id; out->unit = o ? *(uint32_t *)(us[o] + 8) : 0; out->page = pg; grid_xy(it, &out->x, &out->y); return 1; } } }
        it = *(uint64_t *)(d + 0xb0); }
    return 0;
}




static _Atomic int g_job_done;
int dm_runeword_done_count(void) { return atomic_load(&g_job_done); }
static void job_readback(uint8_t *b, const char *how) {
    uint64_t it = b ? unit_item(b, g_job.base) : 0, d = it ? *(uint64_t *)(it + 0x10) : 0; char rs[64] = ""; size_t o = 0; int n = 0;
    uint64_t c = it ? *(uint64_t *)(it + 0x90) : 0;
    for (uint64_t x = c ? *(uint64_t *)(c + 0x10) : 0; x && n < 8; n++) { uint64_t xd = *(uint64_t *)(x + 0x10); uint32_t cd = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(x);
        o += snprintf(rs + o, sizeof rs - o, "%s%.3s", n ? " " : "", (const char *)&cd); if (o >= sizeof rs) o = sizeof rs - 1; x = xd ? *(uint64_t *)(xd + 0xb0) : 0; }
    uint32_t code = it ? ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it) : 0;
    LOG("符文之语：成型读回（%s）底材 id %u「%.3s」%d 孔、孔里 %d 颗（%s）标志 %#x", how, g_job.base, (const char *)&code, it ? sockets(b, it) : -1, n, rs, d ? *(uint32_t *)(d + 0x18) : 0);
    atomic_fetch_add(&g_job_done, 1);
}


static void put_back(uint8_t *b, int again) {
    src_t *sr = &g_job.src[g_job.k];
    if (sr->kind == SRC_GRID && !again) { uint8_t p[17] = {0x18}; uint32_t v[4] = { g_job.rune, (uint32_t)sr->x, (uint32_t)sr->y, (uint32_t)sr->page }; memcpy(p + 1, v, 16); send_pkt(b, p, 17); LOG("符文之语：手上的符文（id %u）放回原格 %d,%d 页 %d", g_job.rune, sr->x, sr->y, sr->page); }
    else { dm_held_stow(); LOG("符文之语：手上的符文（id %u）收进背包（来源是%s）", g_job.rune, again ? "second try" : sr->kind == SRC_STACK ? "符文页成叠" : "共享仓库"); }
}
static void job_end(uint8_t *b, const char *how) { g_job.st = 0; dm_itemjob_release(DM_ITEMJOB_RUNEWORD); job_readback(b, how); }

static void job_settle(uint8_t *b) {
    uint64_t pl = b ? player(b) : 0;
    if (!pl || g_job.k >= g_job.nr) { LOG("runeword: wind-down ends (%s), nothing of the job in flight", pl ? "every rune socketed" : "no player"); job_end(b, "中途停"); return; }
    src_t *sr = &g_job.src[g_job.k]; uint64_t h = held_item(b, pl), r; dm_itemjob_seen v = { 0 };
    if (h) { uint32_t id = *(uint32_t *)(h + 8);
        int ours = (g_job.rune && id == g_job.rune) || (g_job.pend && (sr->kind == SRC_STACK ? (int)*(uint32_t *)(h + 4) == g_job.cls[g_job.k] : id == sr->id));
        v.hand = ours ? DM_ITEMJOB_HAND_OWN : DM_ITEMJOB_HAND_OTHER;
        if (ours) { g_job.rune = id; g_job.pend = 0; } }
    v.tracked = g_job.rune != 0; v.pending = g_job.pend;
    if (g_job.rune && !h && (r = unit_item(b, g_job.rune))) { uint32_t m = *(uint32_t *)(r + 0xc); v.placed = m == 0 || m == 6; }
    if (g_job.pend) { r = unit_item(b, sr->id); uint64_t d = r ? *(uint64_t *)(r + 0x10) : 0;
        v.at_source = r && *(uint32_t *)(r + 0xc) == 0 && (sr->kind != SRC_STACK || (d && *(uint8_t *)(d + 0x9c) == g_job.stack_before)); }
    switch (dm_itemjob_settle_step(&g_job.settle, &v)) {
    case DM_ITEMJOB_WAIT: return;
    case DM_ITEMJOB_PUT_BACK: put_back(b, g_job.settle.backs > 1); return;
    case DM_ITEMJOB_SETTLED: LOG("runeword: wind-down done, rune %u %s", g_job.rune, !v.tracked && v.pending ? "never reached the hand, still at its source" : v.tracked ? "accounted for" : "nothing in flight"); job_end(b, "中途停"); return;
    case DM_ITEMJOB_IN_HAND: LOG("【红】runeword: the rune (id %u) could not be put back in 10 s, it stays in hand for the hand guard", g_job.rune); job_end(b, "中途停"); return;
    case DM_ITEMJOB_OTHER: LOG("【红】runeword: another item is in hand during the wind-down, left there"); job_end(b, "中途停"); return;
    default: LOG("【红】runeword: rune %u of step %d is neither in hand nor placed after 10 s", g_job.rune, g_job.k + 1);
        LOG("ASSUME itemjob.settle: runeword rune of step %d unaccounted for at the wind-down limit", g_job.k + 1); job_end(b, "中途停"); return;
    }
}


static void job_stop(const char *why, int red) {
    uint8_t *b = rb(); uint64_t r = b && g_job.rune ? unit_item(b, g_job.rune) : 0, bs = b ? unit_item(b, g_job.base) : 0;
    LOG("%s符文之语：一键合成停（%s；已镶 %d / %d；符文 id %u %s 模式 %d；底材孔里 %d 颗）", red ? "【红】" : "", why, g_job.k, g_job.nr, g_job.rune, r ? "在" : "不在", r ? (int)*(uint32_t *)(r + 0xc) : -1, bs ? nfilled(bs) : -1);
    g_job.st = 5; dm_itemjob_settle_begin(&g_job.settle, 600, 120); job_settle(b); }
int dm_rw_panel_open(uint8_t *b);
static int item_panel_open(uint8_t *b) { return inv_panel(b) || *(uint8_t *)(b + GA_STASH_OPEN); }
int dm_rw_panel_open(uint8_t *b) { return item_panel_open(b); }
void dm_rw_send(uint8_t *b, const uint8_t *p, int n) { send_pkt(b, p, n); }
uint64_t dm_rw_item(uint8_t *b, uint32_t id) { return unit_item(b, id); }
uint64_t dm_rw_held(uint8_t *b, uint64_t pl) { return held_item(b, pl); }
void dm_rw_take_stack(uint8_t *b, uint64_t it) { static uint64_t fake[0x620 / 8]; fake[GF_STACK_CELL_ITEM / 8] = it; ((void (*)(void *))(b + GA_STACK_TAKE_ONE))(fake); }
static void job_start(uint8_t *b, int row) {
    if (g_job.st || row < 0 || row >= g_list.n || !g_list.c[row].ok) return;
    uint64_t it = unit_item(b, g_list.base), pl = player(b), t = it ? tables(b, it) : 0; int hw, hh;


    if (!it || !pl || !t || !base_ok(b, it, t) || dm_held_size(&hw, &hh) || !item_panel_open(b)) { list_close(b, "预检没过"); LOG("符文之语：预检没过（底材 %s、光标%s），不动", it ? "在" : "不在", dm_held_size(&hw, &hh) ? "上有东西" : "空"); return; }
    cand_t c = g_list.c[row]; build_list(b, it); int still = 0; for (int i = 0; i < g_list.n; i++) if (g_list.c[i].rec == c.rec && g_list.c[i].ok) still = 1;
    if (!still) { list_close(b, "预检：符文已不齐"); return; }
    memset(&g_job, 0, sizeof g_job); g_job.rec = c.rec; g_job.nr = c.nr; memcpy(g_job.cls, c.cls, sizeof c.cls); g_job.base = g_list.base;
    g_job.bkind = base_where(b, pl, it, &g_job.bpage, &g_job.bx, &g_job.by, &g_job.bslot);
    int stash_open = *(uint8_t *)(b + GA_STASH_OPEN) != 0;
    if (!g_job.bkind || (g_job.bkind == 1 && g_job.bpage == PG_STASH && !stash_open)) { list_close(b, "预检：底材位置不能镶"); LOG("符文之语：预检没过（底材位置不能镶），不动"); return; }
    for (int k = 0; k < c.nr; k++) if (!pick_src(b, pl, c.cls[k], stash_open, k, &g_job.src[k])) { list_close(b, "预检：符文来源不齐"); LOG("符文之语：预检没过（第 %d 颗没有可取的来源），不动", k + 1); memset(&g_job, 0, sizeof g_job); return; }
    if (!dm_itemjob_acquire(DM_ITEMJOB_RUNEWORD)) { int o = dm_itemjob_owner(); memset(&g_job, 0, sizeof g_job); list_close(b, "pre-check: another item job runs"); LOG("runeword: item job %d owns the cursor, the craft does not start, nothing moves", o); return; }
    g_job.st = 2; list_close(b, "开始合成");
    LOG("符文之语：一键合成「%s」开始（底材 id %u %s，%d 颗）", c.name, g_job.base, g_job.bkind == 2 ? "身上" : g_job.bpage == PG_STASH ? "储藏箱" : g_job.bpage == PG_CUBE ? "魔盒" : "背包", c.nr);
}
static void job_pump(uint8_t *b) {
    if (!g_job.st) return;
    if (g_job.st == 5) { job_settle(b); return; }
    uint64_t pl = player(b), base = unit_item(b, g_job.base); if (!pl || !base) { job_stop("底材或玩家不见了", 1); return; }


#ifndef DM_PLAYER_BUILD
    static int fail_at = -2; if (fail_at == -2) { const char *e = getenv("DM_TEST_RW_FAIL"); fail_at = e ? atoi(e) : 0; }
#endif
    switch (g_job.st) {
    case 2: {
        if (g_job.k >= g_job.nr) { uint64_t d = *(uint64_t *)(base + 0x10); uint32_t fl = d ? *(uint32_t *)(d + 0x18) : 0;
            if (fl & 0x04000000) LOG("符文之语：一键合成成型（底材 id %u，%d 颗，标志 %#x）", g_job.base, g_job.nr, fl); else LOG("【红】符文之语：%d 颗都镶进去了但没成型（标志 %#x）", g_job.nr, fl);
            job_end(b, fl & 0x04000000 ? "成型" : "没成型"); return; }
        if (held_item(b, pl)) { job_stop("光标上有别的东西", 1); return; }
        if (!item_panel_open(b)) { job_stop("物品面板关了", 1); return; }
#ifndef DM_PLAYER_BUILD
        if (fail_at == g_job.k + 1) { job_stop("测试开关 DM_TEST_RW_FAIL：当作来源没了", 0); return; }
#endif
        src_t *sr = &g_job.src[g_job.k]; uint64_t r = unit_item(b, sr->id);
        if (!r || *(uint32_t *)(r + 0xc) != 0) { job_stop("这颗符文的来源不见了", 1); return; }
        { uint64_t rd = *(uint64_t *)(r + 0x10); g_job.pend = 1; g_job.stack_before = rd ? *(uint8_t *)(rd + 0x9c) : 0; }
        if (sr->kind == SRC_GRID) { uint8_t p[17] = {0x19}; uint32_t v[4] = { sr->id, (uint32_t)sr->x, (uint32_t)sr->y, (uint32_t)sr->page }; memcpy(p + 1, v, 16); send_pkt(b, p, 17); }
        else if (sr->kind == SRC_SHARED) { uint8_t p[13] = {0x46}; uint32_t v[3] = { sr->id, sr->unit, (uint32_t)sr->x | (uint32_t)sr->y << 16 }; memcpy(p + 1, v, 12); send_pkt(b, p, 13); }
        else { static uint64_t fake[0x620 / 8]; fake[GF_STACK_CELL_ITEM / 8] = r; ((void (*)(void *))(b + GA_STACK_TAKE_ONE))(fake); }
        g_job.rune = 0; g_job.st = 3; g_job.wait = 120;
        LOG("符文之语：第 %d 颗：从%s拿起（id %u%s）", g_job.k + 1, sr->kind == SRC_GRID ? (sr->page == PG_STASH ? "储藏箱格" : "背包格") : sr->kind == SRC_STACK ? "符文页成叠" : "共享仓库格", sr->id, sr->kind == SRC_STACK ? "，取 1 件" : ""); return; }
    case 3: {
        uint64_t h = held_item(b, pl); if (!h || (int)*(uint32_t *)(h + 4) != g_job.cls[g_job.k]) { if (--g_job.wait <= 0) job_stop("符文 2 秒没到光标上", 1); return; }
        g_job.rune = *(uint32_t *)(h + 8); g_job.pend = 0;
        uint8_t p[21] = {0x28}; uint32_t v[4]; uint32_t w5;
        if (g_job.bkind == 2) { v[0] = g_job.rune; v[1] = g_job.base; v[2] = (uint32_t)g_job.bslot; v[3] = 0xff; w5 = (uint32_t)g_job.bslot; }
        else { v[0] = g_job.rune; v[1] = g_job.base; v[2] = 0; v[3] = (uint32_t)g_job.bpage; w5 = (uint32_t)(uint16_t)g_job.bx | (uint32_t)(uint16_t)g_job.by << 16; }
        memcpy(p + 1, v, 16); memcpy(p + 17, &w5, 4); send_pkt(b, p, 21);
        g_job.st = 4; g_job.wait = 120;
        if (g_job.bkind == 2) LOG("符文之语：第 %d 颗：镶进身上装备位 %d 的底材（id %u）", g_job.k + 1, g_job.bslot, g_job.base);
        else LOG("符文之语：第 %d 颗：镶进底材（id %u，页 %d，格 %d,%d）", g_job.k + 1, g_job.base, g_job.bpage, g_job.bx, g_job.by); return; }
    case 4: {
        uint64_t r = unit_item(b, g_job.rune);
        if (r && *(uint32_t *)(r + 0xc) == 6 && nfilled(base) >= g_job.k + 1) { g_job.k++; g_job.rune = 0; g_job.st = 2; return; }
        if (--g_job.wait <= 0) job_stop("符文 2 秒没进孔", 1); return; }
    }
}



int dm_runeword_long_press(const dm_cursor_item_info *ci) {
    uint8_t *b = rb(); if (!b || !ci || g_job.st) return 0;
    if (ci->container != DM_ITEM_AT_INVENTORY && ci->container != DM_ITEM_AT_STASH && ci->container != DM_ITEM_AT_EQUIP) return 0;
    uint64_t it = unit_item(b, ci->id); if (!it || *(uint32_t *)(it + 0xc) > 1) return 0;
    if (!build_list(b, it)) return 0;
    g_list.base = ci->id; g_list.open = 1; ui_show(b, 1); ui_fill(b, it); ui_layout(); g_check_at = 6;
    LOG("符文之语：长按 %s（id %u）→ 清单 %d 行（首行「%s」需要等级 %d %s）", ci->code, ci->id, g_list.n, g_list.c[0].name, g_list.c[0].lv, g_list.c[0].ok ? "可做" : "不可做");
    {
        int dm_diag_ladder(int *offline, int *ladder); int off = 0, lad = 0, kn = dm_diag_ladder(&off, &lad); char all[NROW * (sizeof g_list.c[0].name + 3) + 1]; size_t o = 0; all[0] = 0;
        for (int i = 0; i < g_list.n && o < sizeof all; i++) { int w = snprintf(all + o, sizeof all - o, "%s%s", i ? " / " : "", g_list.c[i].name); if (w < 0) break; o += (size_t)w; }
        LOG("符文之语：清单全部 %d 行：%s（局：读到 %d、离线 %d、天梯 %d）", g_list.n, all, kn, off, lad); }
    if (getenv("DM_TEST_RW_DESC") && !strcmp(getenv("DM_TEST_RW_DESC"), "all")) {
        uint8_t ver = *(uint8_t *)(b + GA_CHAR_DATA_VERSION); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + ver * 16) : 0, rr = t ? *(uint64_t *)(t + 0x1880) : 0; uint32_t rn = t ? *(uint32_t *)(t + 0x1888) : 0;
        int mx = 0, mxr = -1, cnt = 0, fail = 0; static char d[4096];
        for (uint32_t r = 0; rr && r < rn; r++) { if (!*(uint8_t *)(rr + (uint64_t)r * 0xe8 + 0x44)) continue; cnt++;
            if (!rw_desc(b, (int)r, d, sizeof d)) { fail++; continue; } int nl = 1; for (char *q = d; *q; q++) nl += *q == '\n';
            if (nl > mx) { mx = nl; mxr = (int)r; } }
        LOG("符文之语：全表说明 %d 条（生成失败 %d），最多 %d 行（记录 %d）", cnt, fail, mx, mxr);
        if (mxr >= 0 && rw_desc(b, mxr, d, sizeof d)) { for (char *q = d; *q; q++) if (*q == '\n') *q = '|'; LOG("符文之语：最长那条：%s", d); } }
    else if (getenv("DM_TEST_RW_DESC")) for (int i = 0; i < g_list.n; i++) {
        static char d[4096]; int k = rw_desc(b, g_list.c[i].rec, d, sizeof d); for (char *q = d; *q; q++) if (*q == '\n') *q = '|';
        LOG("符文之语：说明「%s」%d 字节：%s", g_list.c[i].name, k, d); }
    return 1;
}
void dm_runeword_list_close(void) { uint8_t *b = rb(); if (b) list_close(b, "点了清单外"); }
int dm_recipes_busy(void);
int dm_runeword_busy(void) { return g_job.st != 0; }
int dm_runeword_list_open(void) { return g_list.open || g_job.st || dm_recipes_busy(); }




void dm_runeword_button(const char *name) {
    uint8_t *b = rb(); if (!b || !name || !g_list.open) return;
    if (W.det >= 0) {
        if (!strcmp(name, "TouchRwDetGo")) { job_start(b, W.det); return; }
        if (!strncmp(name, "TouchRwBack", 11)) { LOG("符文之语：收起详情「%s」（点了 %s）", g_list.c[W.det].name, name); W.det = -1; ui_layout(); g_check_at = 6; }
        return; }
    if (!strncmp(name, "TouchRwName", 11) || !strncmp(name, "TouchRwLv", 9)) { det_open(b, atoi(name + (name[7] == 'N' ? 11 : 9))); return; }
    if (!strncmp(name, "TouchRwGo", 9)) { job_start(b, atoi(name + 9)); return; }
    if (!strncmp(name, "TouchRwPg", 9)) {
        int d = !strcmp(name + 9, "Next") ? 1 : -1, to = W.page + d;
        if (to >= 0 && to < W.pages) { W.page = to; ui_layout(); g_check_at = 6; }
        LOG("符文之语：翻页 %s → 第 %d/%d 页（第 %d–%d 个）", name + 9, W.page + 1, W.pages, W.first + 1, W.last); }
}










enum { PAD_HOLD = 15 };
static _Atomic uint64_t g_pad_ev;
uint64_t dm_runeword_pad_filter(uint64_t bt) {
    int dm_padmode_current(void); static uint64_t prev, eat; const uint64_t M = DM_BTN_UP | DM_BTN_DOWN | DM_BTN_LEFT | DM_BTN_RIGHT | DM_BTN_A | DM_BTN_B | DM_BTN_L | DM_BTN_R;
    uint64_t down = bt & ~prev; prev = bt;
    if (dm_recipes_busy()) { eat |= bt; return 0; }
    if (dm_padmode_current() && (g_list.open || g_job.st)) { atomic_fetch_or(&g_pad_ev, down & M); eat |= bt & M; return bt & ~M; }
    eat &= bt; return bt & ~eat;
}
static void pad_sel(int to) {
    if (to < 0 || to >= g_list.n || to >= NROW) return; W.psel = to;
    for (int p = 0; p < W.pages; p++) if (to >= W.pstart[p] && to < W.pstart[p + 1]) W.page = p;
    ui_layout(); g_check_at = 6;
}
static void pad_pump(uint8_t *b) {
    int dm_padmode_current(void); if (!dm_padmode_current()) { atomic_store(&g_pad_ev, 0); return; }
    static uint64_t t0; static int fired, parked, px, py;




    if (!g_list.open && !g_job.st && parked) { parked = 0; dm_ui_hover_now(px, py); }
    if (!g_list.open && !g_job.st) {
        uint64_t root = *(uint64_t *)(b + GA_UI_ROOT), fm = root ? *(uint64_t *)(root + 0xd0) : 0; int held = 0;


        int dm_itemcmp_pad_held(void); int ph = dm_itemcmp_pad_held();
        for (int i = 0; fm && i < 2 && !held; i++) { uint64_t w = *(uint64_t *)(fm + (i ? 0x178 : 0x190)); held = w && *(uint64_t *)w - (uint64_t)(uintptr_t)b == GA_VT_ITEM_GRID && (ph >= 0 ? ph : *(uint8_t *)(w + 0x632)); }
        uint64_t f = dm_guest_frame(); if (!held || !g_inv_open) { t0 = 0; fired = 0; }
        else if (!fired) { if (!t0) t0 = f; else if (f - t0 >= PAD_HOLD) { fired = 1; dm_cursor_item_info ci; if (dm_cursor_item(&ci)) { if (dm_runeword_long_press(&ci)) { W.psel = W.first; ui_layout(); LOG("符文之语：手柄按住对比键 %d 帧 → 打开清单", PAD_HOLD);
                px = *(int32_t *)(b + GA_PAD_CURSOR_X); py = *(int32_t *)(b + GA_PAD_CURSOR_Y); parked = 1; dm_ui_hover_now(*(int32_t *)(b + GA_UI_WIDTH) * 2 / 100, *(int32_t *)(b + GA_UI_HEIGHT) / 2); } } } }
        atomic_store(&g_pad_ev, 0); return; }
    uint64_t ev = atomic_exchange(&g_pad_ev, 0); if (!ev || g_job.st) return;
    if (W.det >= 0) {
        if (ev & DM_BTN_A) { if (g_list.c[W.det].ok) dm_runeword_button("TouchRwDetGo"); }
        else if (ev & DM_BTN_B) dm_runeword_button("TouchRwBack");
        return; }
    if (ev & DM_BTN_B) { list_close(b, "手柄 B"); return; }
    if (ev & DM_BTN_A) { det_open(b, W.psel); LOG("符文之语：手柄 A → 详情「%s」", g_list.c[W.psel].name); return; }
    if (ev & DM_BTN_DOWN) pad_sel(W.psel + 1); else if (ev & DM_BTN_UP) pad_sel(W.psel - 1);
    else if (ev & (DM_BTN_L | DM_BTN_R)) { int to = W.page + ((ev & DM_BTN_R) ? 1 : -1); if (to >= 0 && to < W.pages) pad_sel(W.pstart[to]); }
}
void dm_runeword_on_uimsg(const char *cat, const char *act, const char *arg) {
    uint8_t *b = rb(); if (!b || strcmp(cat, "PanelManager")) return;
    if (!strcmp(act, "ClosePanel") && (!strncmp(arg, "TouchRwGo", 9) || !strncmp(arg, "TouchRwPg", 9) || !strncmp(arg, "TouchRwBack", 11) || !strcmp(arg, "TouchRwDetGo"))) { dm_runeword_button(arg); return; }
    if (!strcmp(arg, "PlayerInventoryExpansionLayout")) { if (!strcmp(act, "OpenPanel") || !strcmp(act, "OpenExclusivePanel")) g_inv_open = 1; else if (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) g_inv_open = 0; }
    if (!strcmp(act, "ClosePanel") && (!strcmp(arg, "PlayerInventoryExpansionLayout") || !strcmp(arg, "BankExpansionLayout") || !strcmp(arg, "UISwitcher"))) list_close(b, "面板关了");
}
void dm_runeword_pump(void) {
    uint8_t *b = rb(); if (!b) return;
    if (g_list.open && dm_ui_unstable()) return;
    if (w_stale()) { int was = g_list.open; g_list.open = 0; memset(&W, 0, sizeof W); W.b = b; g_check_at = 0; if (was) LOG("符文之语：界面重建过（界面代号变了）→ 清单当作已关，缓存的控件作废"); }
    if (g_list.open) { uint64_t it = unit_item(b, g_list.base); if (!inv_panel(b) || !it || *(uint32_t *)(it + 0xc) > 1) list_close(b, "底材挪走了或面板没了"); }
    if (g_list.open) { ui_layout(); if (g_check_at > 0 && --g_check_at == 0) ui_check(1); }
    job_pump(b);
    pad_pump(b);




    static long at = -2, fr; static char code[32], where[16]; if (at == -2) { const char *e = getenv("DM_TEST_RW_OPEN"); at = -1; if (e) { at = atol(e); const char *c = strchr(e, ':'); if (c) strlcpy(code, c + 1, sizeof code); char *w = strchr(code, '@'); if (w) { strlcpy(where, w + 1, sizeof where); *w = 0; } } }


    static long go = -2; if (go == -2) { const char *e = getenv("DM_TEST_RW_GO"); go = e ? atol(e) : -1; }
    fr++; static double t0; struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); double now = ts.tv_sec + ts.tv_nsec / 1e9; if (!t0) t0 = now; double sec = now - t0;
    static long open_fr; if (g_inv_open && !open_fr) open_fr = fr; if (!g_inv_open) open_fr = 0;
    if (at > 0 && sec >= at && open_fr && fr - open_fr > 30) { at = -1; uint64_t pl = player(b), us[3] = {0}; int done = 0; if (pl) owners(b, pl, us);
        for (int o = 0; o < 3 && !done; o++) for (uint64_t it = first_item(us[o]); it && !done; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            int pg = PAGE(d), here = !where[0] ? !o : !strcmp(where, "inv") ? !o && pg == PG_INV : !strcmp(where, "stash") ? !o && pg == PG_STASH : !strcmp(where, "shared") ? o > 0 : !strcmp(where, "equip") ? !o && *(uint32_t *)(it + 0xc) == 1 : 0;
            uint32_t cd = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it); if (here && *(uint32_t *)(it + 0xc) <= 1 && !memcmp(&cd, code, strlen(code) < 4 ? strlen(code) : 4)) {
                dm_cursor_item_info ci; memset(&ci, 0, sizeof ci); ci.id = *(uint32_t *)(it + 8); memcpy(ci.code, code, 3); ci.container = *(uint32_t *)(it + 0xc) == 1 ? DM_ITEM_AT_EQUIP : o || pg == PG_STASH ? DM_ITEM_AT_STASH : DM_ITEM_AT_INVENTORY;
                int ok = dm_runeword_long_press(&ci); LOG("测试 DM_TEST_RW_OPEN：第 %.1f 秒对 %s（id %u，单位 %d 页 %d，孔 %d）长按 → %s", sec, code, ci.id, o, pg, sockets(b, it), ok ? "开了清单" : "不是匹配底材"); done = ok; }
            it = *(uint64_t *)(d + 0xb0); }
        if (!done) { LOG("测试 DM_TEST_RW_OPEN：第 %.1f 秒没有能开清单的 %s%s%s；同码物品：", sec, code, where[0] ? "@" : "", where);
            { uint32_t sid = shared_unit_id(b); LOG("测试 DM_TEST_RW_OPEN：共享仓库单位 id %u；另两个单位 id %u / %u", sid, us[1] ? *(uint32_t *)(us[1] + 8) : 0, us[2] ? *(uint32_t *)(us[2] + 8) : 0);
              for (int ty = 0; ty < 6 && sid; ty++) { uint64_t u = unit_any(b, sid, ty); if (!u) continue; int ni = 0; for (uint64_t q = first_item(u); q && ni < 200; ni++) { uint64_t qd = *(uint64_t *)(q + 0x10); q = qd ? *(uint64_t *)(qd + 0xb0) : 0; }
                  LOG("测试 DM_TEST_RW_OPEN：  按类型 %d 找到单位 %p，物品 %d 件", ty, (void *)u, ni); } }
            for (int o = 0; o < 3; o++) for (uint64_t it = first_item(us[o]); it; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t cd = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it);
                if (!memcmp(&cd, code, strlen(code) < 4 ? strlen(code) : 4)) LOG("测试 DM_TEST_RW_OPEN：  id %u 单位 %d 页 %u 模式 %u 孔 %d", *(uint32_t *)(it + 8), o, PAGE(d), *(uint32_t *)(it + 0xc), sockets(b, it));
                it = *(uint64_t *)(d + 0xb0); } } }


#ifndef DM_PLAYER_BUILD
    static double ls = -2; if (ls == -2) { const char *e = getenv("DM_TEST_RW_LIST"); ls = e ? atof(e) : -1; }
    if (ls > 0 && sec >= ls) { ls = -1; uint64_t pl = player(b), us[3]; int n = 0; if (pl) { owners(b, pl, us);
        for (int o = 0; o < 3; o++) for (uint64_t it = first_item(us[o]); it; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            if (*(uint32_t *)(d + 0x18) & 0x04000000) { uint32_t cd = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it); LOG("测试 DM_TEST_RW_LIST：符文之语物品 id %u「%.4s」品质 %u 单位 %d 页 %u 模式 %u 孔里 %d 颗 标志 %#x", *(uint32_t *)(it + 8), (const char *)&cd, *(uint32_t *)d, o, PAGE(d), *(uint32_t *)(it + 0xc), nfilled(it), *(uint32_t *)(d + 0x18)); n++;
                struct { uint64_t n; uint8_t has; } c0 = { 0, 0 };
                gstr_t tip = ((gstr_t (*)(uint64_t, uint64_t, uint64_t, int, int, int, void *, void *))(b + GA_ITEM_DESCRIBE))(pl, pl, it, 0, 1, 0, NULL, &c0);
                if (tip.p) { static char tb[4096]; size_t k = tip.len < sizeof tb - 1 ? tip.len : sizeof tb - 1; memcpy(tb, tip.p, k); tb[k] = 0; for (char *q = tb; *q; q++) if (*q == '\n') *q = '|';
                    LOG("测试 DM_TEST_RW_LIST：原版说明 %s", tb); if (!(tip.cap >> 63)) g_free(b, tip.p); } }
            it = *(uint64_t *)(d + 0xb0); } }
        LOG("测试 DM_TEST_RW_LIST：第 %.1f 秒共 %d 件符文之语物品", sec, n); }
#endif
    if (go > 0 && sec >= go) { go = -1; LOG("测试 DM_TEST_RW_GO：第 %.1f 秒点第一行「一键合成」", sec); job_start(b, 0); }
}
