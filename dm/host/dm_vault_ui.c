

















#include <ctype.h>
#include "game_addrs.h"
#include <stdatomic.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dm.h"
#include "dm_widget.h"
#include "dm_cursor.h"
#include "dm_vault_tab.h"
#include "dm_cursor_item.h"
#include "dm_i18n.h"

enum { VCOLS = 3, VROWS = 4, VPER = VCOLS * VROWS, VSET_ROWS = 7, VSET_N = 6, VU_UNIQ_Y = 516, VX0 = 156, VW0 = 1372, VT_TIER_Y = 444 };
enum { ST_NONE, ST_SEEN, ST_STORED };
int dm_vault_has(int kind, int idx); int dm_vault_gen(void);
static _Atomic int g_dirty = 1, g_flip, g_sub_req = -1;
static int g_sub, g_part, g_page[VT_NPART + 1], g_tiers = 7;
static int g_pg_cur, g_pg_tot;
static int g_drop_id;
static char g_query[64]; static int g_page_q;
static int g_tip_on;
static int g_cell_row[VPER], g_set_row[VSET_ROWS][VSET_N];

static uint8_t *vu_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t vu_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 12) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = vu_find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}




static int vu_ours(uint64_t w) {
    for (int k = 0; w && k < 16; k++, w = *(uint64_t *)(w + 0x30)) { const char *nm = *(const char **)(w + 8); if (nm && (!strcmp(nm, "TouchVault") || !strcmp(nm, "TouchPgPage"))) return 1; }
    return 0;
}
static int vu_guard(uint64_t w, const char *what) {
    if (!w) return 0; if (vu_ours(w)) return 1;
    static int n; if (n++ < 20) { const char *nm = *(const char **)(w + 8); LOG("【红】编年史仓库：%s 想写原版控件「%s」，拦下（只许写 TouchVault 底下的）", what, nm ? nm : "?"); }
    return 0;
}
static void vu_show(uint64_t w, int on) { if (vu_guard(w, "改可见")) { *(uint8_t *)(w + 0x50) = (uint8_t)on; *(uint8_t *)(w + 0x51) = (uint8_t)on; } }
static void vu_move(uint64_t w, int x, int y) { if (!vu_guard(w, "挪位置")) return; int32_t *r = (int32_t *)(w + 0x70); if (r[0] == x && r[1] == y) return; r[0] = x; r[1] = y; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); }


static void vu_image(uint8_t *b, uint64_t w, const char *path) {
    if (!vu_guard(w, "换图")) return; struct { char *p; uint64_t n, cap; char buf[0xa0]; } s; size_t n = strlen(path); if (n >= sizeof s.buf) return;
    memcpy(s.buf, path, n + 1); s.p = s.buf; s.n = n; s.cap = (sizeof s.buf - 1) | 1ull << 63;
    ((void (*)(uint64_t, void *))(b + GA_IMAGE_SET_PATH))(w, &s);
}
static void vu_text(uint8_t *b, uint64_t w, const char *s) {
    if (!vu_guard(w, "改字") || *(uint64_t *)w - (uint64_t)(uintptr_t)b != GA_VT_TEXTBOX) return;
    size_t n = strlen(s); if (*(uint64_t *)(w + 0x90) == n && !memcmp(*(const char **)(w + 0x88), s, n)) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(b + GA_TEXT_SET))(w + 0x88, s, n); ((void (*)(uint64_t))(b + GA_TEXT_LAYOUT))(w);
}
static int vu_is(uint64_t w, uint64_t vt);
static void vu_frame(uint64_t w, int f) {
    if (!w || !vu_guard(w, "换帧") || !vu_is(w, DM_VT_IMAGE) || *(int32_t *)(w + 0x90) == f) return; *(int32_t *)(w + 0x90) = f;
}
static void vu_color(uint8_t *b, uint64_t w, int r, int g, int bl) {
    if (!vu_guard(w, "改字色") || *(uint64_t *)w - (uint64_t)(uintptr_t)b != GA_VT_TEXTBOX) return; int32_t *c = (int32_t *)(w + 0xbc); if (c[0] == r && c[1] == g && c[2] == bl) return;
    c[0] = r; c[1] = g; c[2] = bl; ((void (*)(uint64_t))(b + GA_TEXT_LAYOUT))(w);
}
static const char *vu_str(uint8_t *b, const char *key) { struct { const char *k; uint64_t n; } s = { key, strlen(key) }; const char *r = dm_untag(((const char *(*)(void *))(b + GA_LOC_STRING_GET))(&s)); return r && *r ? r : key; }






static const char *vu_name(uint8_t *b, const char *key) {
    static char out[160]; const char *s = vu_str(b, key), *nl = strrchr(s, '\n'); if (nl) s = nl + 1;
    for (;;) { if ((uint8_t)s[0] == 0xc3 && (uint8_t)s[1] == 0xbf && s[2] == 'c' && s[3]) s += 4; else if ((uint8_t)s[0] == 0xee && (uint8_t)s[1] == 0x81 && (uint8_t)s[2] == 0xbe && s[3]) s += 4; else break; }
    strlcpy(out, s, sizeof out); return out;
}




enum { VT_TOGGLE = DM_VT_TOGGLE, VT_TABBAR = DM_VT_TABBAR };
static int vu_is(uint64_t w, uint64_t vt) { uint8_t *b = vu_base(); if (!w || !b) return 0; if (dm_widget_is(b, w, vt)) return 1;
    static int n; if (n++ < 10) { const char *nm = *(const char **)(w + 8); LOG("【红】编年史仓库：控件「%s」不是预期的类（虚表 main+0x%llx，应为 0x%llx），不按偏移读写", nm ? nm : "?", (unsigned long long)(*(uint64_t *)w - (uint64_t)(uintptr_t)b), (unsigned long long)vt); }
    return 0; }
static void vu_toggle(uint64_t w, int on) {
    if (!vu_guard(w, "改勾选") || !vu_is(w, VT_TOGGLE)) return; static const int32_t fr[2][4] = { { 0, 1, 3, 2 }, { 4, 5, 6, -1 } }; int32_t f[4]; int k = *(int32_t *)(w + 0x6f0) == 3 || *(int32_t *)(w + 0x6e8) == 4;
    if (k) memcpy(f, fr[on != 0], sizeof f); else for (int i = 0; i < 4; i++) f[i] = on ? 1 : 0;
    *(uint8_t *)(w + 0xa88) = on != 0; memcpy((void *)(w + 0x6e8), f, sizeof f); *(int32_t *)(w + 0x610) = f[0];
}


static int vu_seen(uint8_t *b, int kind, int row) {
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)), pd = pl ? *(uint64_t *)(pl + 0x10) : 0; if (!pd) return 0;
    uint64_t t = pd + 0x220 + (kind == 1 ? 0x20 : 0), nb = *(uint64_t *)t, bk = *(uint64_t *)(t + 8); if (!bk || !nb || (nb & (nb - 1))) return 0;
    uint64_t h = (uint64_t)(uint16_t)row * 0xff51afd7ed558ccdull; h ^= h >> 33; h *= 0xc4ceb9fe1a85ec53ull; h ^= h >> 33;
    for (uint64_t n = *(uint64_t *)(bk + 8 * (h & (nb - 1))), k = 0; n && k < 4096; n = *(uint64_t *)n, k++) if (*(uint32_t *)(n + 8) == (uint32_t)row) return 1;
    return 0;
}


static int vu_state(uint8_t *b, const vt_row *r) { static int tn = -1; if (tn < 0) { const char *e = getenv("DM_TEST_VAULT_SETS"); tn = e ? atoi(e) : 0; }
    if (r->kind == 0 && r->set < tn) return ST_STORED;
    return dm_vault_has(r->kind, r->row) ? ST_STORED : vu_seen(b, r->kind, r->row) ? ST_SEEN : ST_NONE; }


static void vu_tile(uint8_t *b, uint64_t cell, const char *tile, const vt_row *r, int st) {
    char nm[24]; uint64_t t = vu_find(cell, tile, 0); snprintf(nm, sizeof nm, "%sBox", tile); uint64_t box = vu_find(cell, nm, 0); snprintf(nm, sizeof nm, "%sImg", tile); uint64_t im = vu_find(cell, nm, 0);
    if (!t || !box || !im) return;
    uint64_t cg = vu_find(cell, "CellGold", 0); snprintf(nm, sizeof nm, "%sGold", tile); vu_show(vu_find(cell, nm, 0), st == ST_STORED && !cg); vu_show(cg, st == ST_STORED);
    static struct { dm_wref r; const char *icon; } cache[VPER + VSET_ROWS * VSET_N]; int ci = -1, fr = -1;
    for (int i = 0; i < (int)(sizeof cache / sizeof cache[0]); i++) { uint64_t cw = dm_wref_get(&cache[i].r); if (cw == im) { ci = i; break; } if (fr < 0 && !cw) fr = i; }
    if (ci < 0 && fr >= 0) { ci = fr; dm_wref_set(&cache[ci].r, im); cache[ci].icon = NULL; }


    if (ci >= 0 && (cache[ci].icon != r->icon || *(int32_t *)(im + 0x78) <= 0 || *(int32_t *)(im + 0x7c) <= 0)) { cache[ci].icon = r->icon; vu_image(b, im, r->icon); *(int32_t *)(im + 0x70) = 0; *(int32_t *)(im + 0x74) = 0; *(float *)(im + 0x80) = 1.0f; }
    int bw = *(int32_t *)(box + 0x78), bh = *(int32_t *)(box + 0x7c), iw = *(int32_t *)(im + 0x78), ih = *(int32_t *)(im + 0x7c);
    if (iw > 0 && ih > 0 && bw > 0 && bh > 0) { float s = (float)bw / iw < (float)bh / ih ? (float)bw / iw : (float)bh / ih; if (s > 1.0f) s = 1.0f;
        *(float *)(im + 0x80) = s; vu_move(im, (int)((bw - iw * s) / 2), (int)((bh - ih * s) / 2)); }
    *(float *)(im + 0x94) = st == ST_STORED ? 1.0f : st == ST_SEEN ? 0.55f : 0.3f;
}
static void vu_uniques(uint8_t *b, uint64_t vt) {
    int list[128], n = 0, nst = 0, nseen = 0;
    for (int i = 0; i < VT_NROW && n < 128; i++) { const vt_row *r = &vt_rows[i]; if (r->kind != 1 || r->part != g_part || !(g_tiers >> r->tier & 1)) continue; list[n++] = i; int st = vu_state(b, r); nst += st == ST_STORED; nseen += st != ST_NONE; }
    int pages = n ? (n + VPER - 1) / VPER : 1; int *pg = &g_page[g_part]; if (*pg >= pages) *pg = pages - 1; if (*pg < 0) *pg = 0;
    char buf[160]; snprintf(buf, sizeof buf, dm_str(b, "dmVaultStats"), n, nst, nseen); vu_text(b, vu_find(vt, "TouchV_Stats", 0), buf);
    uint64_t u = vu_find(vt, "TouchV_Uniq", 0);
    for (int k = 0; k < VPER; k++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_U%d", k); uint64_t c = vu_find(u, nm, 0); if (!c) continue;
        int j = *pg * VPER + k; g_cell_row[k] = j < n ? list[j] : -1; if (j >= n) { vu_show(c, 0); continue; } vu_show(c, 1);
        const vt_row *r = &vt_rows[list[j]]; int st = vu_state(b, r); vu_tile(b, c, "Tile", r, st);
        uint64_t nw = vu_find(c, "Name", 0), dm = vu_find(c, "Sub", 0);
        if (st == ST_STORED) { vu_color(b, nw, 199, 179, 119); vu_color(b, dm, 232, 210, 150); } else if (st == ST_SEEN) { vu_color(b, nw, 170, 160, 130); vu_color(b, dm, 150, 190, 255); } else { vu_color(b, nw, 110, 108, 100); vu_color(b, dm, 120, 118, 110); }
        vu_text(b, nw, vu_name(b, r->key)); vu_text(b, dm, st == ST_STORED ? dm_str(b, "dmVaultStored") : st == ST_SEEN ? dm_str(b, "dmVaultSeen") : dm_str(b, "dmVaultNone"));
        vu_text(b, vu_find(c, "Dot", 0), st == ST_SEEN ? "●" : ""); }
    g_pg_cur = *pg + 1; g_pg_tot = pages;
}
static void vu_sets(uint8_t *b, uint64_t vt) {
    int pages = (VT_NSET + VSET_ROWS - 1) / VSET_ROWS, *pg = &g_page[VT_NPART]; if (*pg >= pages) *pg = pages - 1; if (*pg < 0) *pg = 0;
    int tot = 0, nst = 0; for (int i = 0; i < VT_NROW; i++) if (vt_rows[i].kind == 0) { tot++; nst += dm_vault_has(0, vt_rows[i].row); }
    char buf[160]; snprintf(buf, sizeof buf, dm_str(b, "dmVaultSetStats"), VT_NSET, nst, tot); vu_text(b, vu_find(vt, "TouchV_Stats", 0), buf);
    uint64_t sg = vu_find(vt, "TouchV_Sets", 0);
    for (int r = 0; r < VSET_ROWS; r++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_S%d", r); uint64_t row = vu_find(sg, nm, 0); if (!row) continue;
        int s = *pg * VSET_ROWS + r; for (int j = 0; j < VSET_N; j++) g_set_row[r][j] = -1; if (s >= VT_NSET) { vu_show(row, 0); continue; } vu_show(row, 1);
        int k = 0, have = 0;
        for (int i = 0; i < VT_NROW && k < VSET_N; i++) { const vt_row *e = &vt_rows[i]; if (e->kind != 0 || e->set != s) continue; int st = vu_state(b, e); have += st == ST_STORED;
            g_set_row[r][k] = i; char tn[16]; snprintf(tn, sizeof tn, "Tile%dDot", k); vu_text(b, vu_find(row, tn, 0), st == ST_SEEN ? "●" : ""); snprintf(tn, sizeof tn, "Tile%d", k); vu_show(vu_find(row, tn, 0), 1); snprintf(nm, sizeof nm, "Tile%dBox", k); vu_show(vu_find(row, nm, 0), 1); vu_tile(b, row, tn, e, st); k++; }
        int n = k; for (; k < VSET_N; k++) { static const char *const sfx[] = { "", "Gold", "Box", "Dot" }; for (int q = 0; q < 4; q++) { char tn[20]; snprintf(tn, sizeof tn, "Tile%d%s", k, sfx[q]); vu_show(vu_find(row, tn, 0), 0); } }
        vu_text(b, vu_find(row, "Name", 0), vu_name(b, vt_set_key[s]));
        uint64_t cw = vu_find(row, "Count", 0); if (have == n) vu_color(b, cw, 199, 179, 119); else vu_color(b, cw, 191, 191, 191);
        vu_show(vu_find(row, "BgGold", 0), have == n);
        snprintf(buf, sizeof buf, dm_str(b, have == n ? "dmVaultSetFull" : "dmVaultSetHave"), have, n); vu_text(b, cw, buf); }
    g_pg_cur = *pg + 1; g_pg_tot = pages;
}






typedef struct { char *p; uint64_t len, cap; } gstr_t;
typedef struct { uint64_t d[5]; uint64_t item; } preview_t;
static void vu_gfree(uint8_t *b, void *p) { if (!(*(volatile uint8_t *)(b + GA_GAME_ALLOC_READY) & 1)) return; uint64_t a = *(uint64_t *)(b + GA_GAME_ALLOC); ((void (*)(uint64_t, void *))(*(uint64_t *)(*(uint64_t *)a + 0x28)))(a, p); }
static void vu_preview_drop(uint8_t *b, preview_t *h) {
    uint64_t *obj = (h->d[0] & 1) ? (uint64_t *)(h->d[0] & ~1ull) : (uint64_t *)h;
    if (h->item) { uint64_t it = h->item; ((void (*)(void *, uint64_t *))(*(uint64_t *)(*obj + 0x10)))(obj, &it); h->item = 0; }
    obj = (h->d[0] & 1) ? (uint64_t *)(h->d[0] & ~1ull) : (uint64_t *)h;
    if (obj) { ((void (*)(void *))(*(uint64_t *)(*obj)))(obj); if (h->d[0] & 1) vu_gfree(b, obj); }
    h->d[0] = 1;
}
static int vu_desc(uint8_t *b, const vt_row *e, char *out, size_t n) {
    out[0] = 0; uint8_t ver = *(uint8_t *)(b + GA_CHAR_DATA_VERSION); if (ver < 1 || ver > 3) return 0;
    uint64_t t = *(uint64_t *)(b + GA_DATA_TABLES + ver * 16), pl = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)); if (!t || !pl) return 0;
    uint64_t rr = *(uint64_t *)(t + (e->kind ? 0x13e8 : 0x13c8)), rn = *(uint64_t *)(t + (e->kind ? 0x13f0 : 0x13d0)); if (!rr || e->row >= rn) return 0;
    const uint8_t *rec = (const uint8_t *)(rr + (uint64_t)e->row * (e->kind ? 0x15c : 0x1cc));
    preview_t h = ((preview_t (*)(int, int, int, int))(b + GA_PREVIEW_ITEM_MAKE))(e->kind ? 1 : 0, e->row, 1, 1);
    if (!h.item) { vu_preview_drop(b, &h); return 0; }
    uint8_t props[16 * 12]; uint64_t np = 0; for (int k = 0; k < (e->kind ? 12 : 9) && *(int32_t *)(rec + 0x98 + 16 * k) >= 0; k++) memcpy(props + 16 * np++, rec + 0x98 + 16 * k, 16);
    struct { uint64_t n; uint8_t has; } cnt = { np, 1 };
    gstr_t r = ((gstr_t (*)(uint64_t, uint64_t, uint64_t, int, int, int, void *, void *))(b + GA_ITEM_DESCRIBE))(pl, pl, h.item, 0, 1, 0, props, &cnt);
    size_t k = r.p && r.len < n ? r.len : (r.p ? n - 1 : 0); if (k) memcpy(out, r.p, k); out[k] = 0;
    if (r.p && !(r.cap >> 63)) vu_gfree(b, r.p);
    vu_preview_drop(b, &h);
    return (int)k;
}
static void vu_tip_place(uint64_t tb, uint64_t bg) {
    int w = *(int32_t *)(tb + 0x78), h = *(int32_t *)(tb + 0x7c); if (w <= 0 || h <= 0) return;
    int x = 156 + (1372 - w) / 2, y = 296 + (820 - h) / 2; if (y < 150) y = 150;
    int32_t *r = (int32_t *)(bg + 0x70); if (r[2] != w + 80 || r[3] != h + 60) { r[2] = w + 80; r[3] = h + 60; r[0] = -1; }
    vu_move(bg, x - 40, y - 30); vu_move(tb, x, y);
}
static void vu_tip_exact(uint8_t *b, uint64_t vt) {
    static char d[4096]; int dm_vault_take_desc(char *, size_t); if (!dm_vault_take_desc(d, sizeof d - 64)) return; size_t n = strlen(d);
    snprintf(d + n, sizeof d - n, "\n%s", dm_str(b, "dmVaultStoredTip")); vu_text(b, vu_find(vt, "TouchV_Tip", 0), d); LOG("编年史仓库：说明框换成存着那件自己的数（%zu 字节）", n);
}
static void vu_tip(uint8_t *b, uint64_t vt, const vt_row *e) {
    uint64_t tb = vu_find(vt, "TouchV_Tip", 0), bg = vu_find(vt, "TouchV_TipBg", 0); if (!tb || !bg) return;
    if (!e) { vu_move(tb, 99999, 0); vu_move(bg, 99999, 0); return; }
    static char d[4096]; int n = vu_desc(b, e, d, sizeof d - 160); int st = vu_state(b, e);
    if (st == ST_STORED) { void dm_vault_request_desc(int, int); dm_vault_request_desc(e->kind, e->row); }
    snprintf(d + n, sizeof d - n, "%s%s", n ? "\n" : "", st == ST_STORED ? dm_str(b, "dmVaultStoredTip") : st == ST_SEEN ? dm_str(b, "dmVaultSeen") : dm_str(b, "dmVaultNone"));
    vu_text(b, tb, d); vu_tip_place(tb, bg);
}
static int vu_hit(uint64_t w, int x, int y) { return w && *(uint8_t *)(w + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x48))(w, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1); }




int dm_vault_ui_over(int x, int y);
int dm_vault_ui_blocks(int x, int y);
static int vu_cell_at(uint64_t vt, int x, int y, int *cell) {
    int row = -1; *cell = -1;
    if (g_sub == 0 || g_query[0]) { uint64_t u = vu_find(vt, "TouchV_Uniq", 0); for (int k = 0; k < VPER && row < 0; k++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_U%d", k); uint64_t c = vu_find(u, nm, 0); if (vu_hit(c ? vu_find(c, "Bg", 0) : 0, x, y) && g_cell_row[k] >= 0) { row = g_cell_row[k]; *cell = k; } } }
    else { uint64_t sg = vu_find(vt, "TouchV_Sets", 0); for (int r = 0; r < VSET_ROWS && row < 0; r++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_S%d", r); uint64_t rw = vu_find(sg, nm, 0);
        for (int j = 0; rw && j < VSET_N && row < 0; j++) { char tn[16]; snprintf(tn, sizeof tn, "Tile%d", j); if (vu_hit(vu_find(rw, tn, 0), x, y) && g_set_row[r][j] >= 0) { row = g_set_row[r][j]; *cell = 100 + r * VSET_N + j; } } } }
    return row;
}




int dm_vault_ui_blocks(int x, int y) {
    if (!dm_vault_ui_over(x, y)) return 0;
    uint8_t *b = vu_base(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, bk = root ? dm_ui_top(root, "BankExpansionLayout") : 0, vt = bk ? vu_find(bk, "TouchVault", 1) : 0; if (!vt) return 0;
    static const char *const ours[] = { "TouchV_T0", "TouchV_T1", "TouchV_T2", "TouchV_SearchBg" };
    for (unsigned i = 0; i < sizeof ours / sizeof ours[0]; i++) if (vu_hit(vu_find(vt, ours[i], 1), x, y)) return 0;
    return 1;
}
int dm_vault_ui_tap(int x, int y) {
    uint8_t *b = vu_base(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, bk = root ? dm_ui_top(root, "BankExpansionLayout") : 0, vt = bk ? vu_find(bk, "TouchVault", 1) : 0; if (!vt) return 0;
    { uint64_t dl = vu_find(root, "DropdownListContents", 1); if (dl && *(uint8_t *)(dl + 0x50)) return 0; }


    { int dm_vault_ui_pager_hit(int, int); void dm_vault_ui_flip(int); int d = dm_vault_ui_pager_hit(x, y);
      if (d) { if (g_tip_on) { vu_tip(b, vt, NULL); g_tip_on = 0; } dm_vault_ui_flip(d); return 1; } }
    for (int s = 0; s < 2; s++) {
        uint64_t tab = vu_find(vt, s ? "TouchV_SubS" : "TouchV_SubU", 1);
        if (vu_hit(vu_find(tab, "Hit", 0), x, y)) {
            if (g_tip_on) { vu_tip(b, vt, NULL); g_tip_on = 0; }
            atomic_store(&g_sub_req, s); atomic_store(&g_dirty, 1); return 1;
        }
    }
    if (g_sub == 0) for (int p = 0; p < VT_NPART; p++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_PT%d", p); uint64_t t = vu_find(vt, nm, 1);
        if (t && vu_hit(vu_find(t, "Hit", 0), x, y)) { if (g_tip_on) { vu_tip(b, vt, NULL); g_tip_on = 0; } if (p != g_part) { g_part = p; atomic_store(&g_dirty, 1); LOG("编年史仓库：点部位页签 %s", vt_part_name[p]); } return 1; } }
    int cell = -1, row = vu_cell_at(vt, x, y, &cell);
    if (row < 0) { if (g_tip_on) { vu_tip(b, vt, NULL); g_tip_on = 0; }


        return dm_vault_ui_blocks(x, y); }
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); double now = ts.tv_sec + ts.tv_nsec / 1e9; static double last; static int lcell = -1;
    const vt_row *e = &vt_rows[row]; int dbl = cell == lcell && now - last < 0.40;
    if (dbl) { last = 0; lcell = -1;
        if (dm_vault_has(e->kind, e->row)) { void dm_vault_request_withdraw(int, int); dm_vault_request_withdraw(e->kind, e->row); LOG("编年史仓库：双击 %s（%s %d）→ 取出", e->key, e->kind ? "暗金" : "套装件", e->row); }
        else LOG("编年史仓库：双击 %s —— 这一格没存，不动", e->key); }
    else { last = now; lcell = cell; LOG("编年史仓库：点了 %s（%s）", e->key, dm_vault_has(e->kind, e->row) ? "存着" : "没存"); }
    if (dbl || g_tip_on == cell + 1) { vu_tip(b, vt, NULL); g_tip_on = 0; } else { vu_tip(b, vt, e); g_tip_on = cell + 1; }
    return 1;
}
uint32_t dm_vault_ui_held(void) {
    uint8_t *b = vu_base(); uint64_t pl = b ? ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)) : 0, inv = pl ? *(uint64_t *)(pl + 0x90) : 0, cur = inv ? *(uint64_t *)(inv + 0x40) : 0;
    return cur && *(uint32_t *)cur == 4 ? *(uint32_t *)(cur + 8) : 0;
}


int dm_vault_ui_over(int x, int y) {
    uint8_t *b = vu_base(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, bk = root ? dm_ui_top(root, "BankExpansionLayout") : 0, vt = bk ? vu_find(bk, "TouchVault", 1) : 0;
    if (!vt || *(int32_t *)(vt + 0x70) != 0) return 0;
    uint64_t dl = vu_find(root, "DropdownListContents", 1); if (dl && *(uint8_t *)(dl + 0x50)) return 0;
    return vu_hit(vu_find(vt, "TouchV_Back", 0), x, y) || vu_hit(vu_find(vt, "TouchV_Cover", 0), x, y);
}




int dm_vault_ui_tip_gate(int x, int y) {
    if (!g_tip_on) return 0;
    uint8_t *b = vu_base(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, bk = root ? dm_ui_top(root, "BankExpansionLayout") : 0, vt = bk ? vu_find(bk, "TouchVault", 1) : 0;
    if (!vt || *(int32_t *)(vt + 0x70) != 0) return 0;
    { uint64_t dl = vu_find(root, "DropdownListContents", 1); if (dl && *(uint8_t *)(dl + 0x50)) return 0; }
    int cell = -1; if (vu_cell_at(vt, x, y, &cell) >= 0 && cell + 1 == g_tip_on) return 0;
    if (vu_hit(vu_find(vt, "TouchV_TipBg", 0), x, y)) return 1;
    vu_tip(b, vt, NULL); g_tip_on = 0; LOG("编年史仓库：点了说明框外 → 收起说明框，这一下不做别的"); return 1;
}




static void vu_fold(const char *s, char *o, size_t cap) {
    static const char lat[64] = "aaaaaaaceeeeiiiidnooooo*ouuuuyts" "aaaaaaaceeeeiiiidnooooo/ouuuuyty";
    size_t k = 0; for (const unsigned char *p = (const unsigned char *)s; *p && k + 1 < cap; p++) {
        if (*p == 0xC3 && p[1] >= 0x80 && p[1] <= 0xBF) { o[k++] = lat[p[1] - 0x80]; p++; }
        else o[k++] = (char)(*p < 0x80 ? tolower(*p) : *p); }
    o[k] = 0; }
static int vu_has(const char *name, const char *q) {
    if (!name || !q) return 0; if (!*q) return 1;
    char fn[512], fq[256]; vu_fold(name, fn, sizeof fn); vu_fold(q, fq, sizeof fq);
    return strstr(fn, fq) != NULL;
}


static void vu_search(uint8_t *b, uint64_t vt) {
    static int list[VT_NROW]; int n = 0, nu = 0, ns = 0;
    for (int st = ST_STORED; st >= ST_NONE; st--) for (int i = 0; i < VT_NROW; i++) { const vt_row *r = &vt_rows[i]; if (vu_state(b, r) != st || !vu_has(vu_name(b, r->key), g_query)) continue; list[n++] = i; if (r->kind) nu++; else ns++; }
    int pages = n ? (n + VPER - 1) / VPER : 1; if (g_page_q >= pages) g_page_q = pages - 1; if (g_page_q < 0) g_page_q = 0;
    char buf[160], f[160]; snprintf(buf, sizeof buf, dm_strn(b, "dmVaultSearch", n, f, sizeof f), g_query, n, nu, ns); vu_text(b, vu_find(vt, "TouchV_Stats", 0), buf);
    uint64_t u = vu_find(vt, "TouchV_Uniq", 0);
    for (int k = 0; k < VPER; k++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_U%d", k); uint64_t c = vu_find(u, nm, 0); if (!c) continue;
        int j = g_page_q * VPER + k; g_cell_row[k] = j < n ? list[j] : -1; if (j >= n) { vu_show(c, 0); continue; } vu_show(c, 1);
        const vt_row *r = &vt_rows[list[j]]; int st = vu_state(b, r); vu_tile(b, c, "Tile", r, st); uint64_t nw = vu_find(c, "Name", 0), dm = vu_find(c, "Sub", 0);
        if (r->kind == 0) vu_color(b, nw, st == ST_NONE ? 60 : 0, st == ST_NONE ? 120 : st == ST_SEEN ? 190 : 252, st == ST_NONE ? 60 : 0);
        else if (st == ST_STORED) vu_color(b, nw, 199, 179, 119); else if (st == ST_SEEN) vu_color(b, nw, 170, 160, 130); else vu_color(b, nw, 110, 108, 100);
        if (st == ST_STORED) vu_color(b, dm, 232, 210, 150); else if (st == ST_SEEN) vu_color(b, dm, 150, 190, 255); else vu_color(b, dm, 120, 118, 110);
        vu_text(b, nw, vu_name(b, r->key)); vu_text(b, dm, st == ST_STORED ? dm_str(b, "dmVaultStored") : st == ST_SEEN ? dm_str(b, "dmVaultSeen") : dm_str(b, "dmVaultNone")); vu_text(b, vu_find(c, "Dot", 0), st == ST_SEEN ? "●" : ""); }
    g_pg_cur = g_page_q + 1; g_pg_tot = pages;
}








static struct { int on, row, bx0, by0; float ax, ay; } g_dg;
static void vu_drag_place(uint64_t vt, int x, int y) {
    uint64_t im = vu_find(vt, "TouchV_Drag", 0); if (!im || g_dg.ax <= 0 || g_dg.ay <= 0) return;
    int ux = 142 + (int)((x - g_dg.bx0) / g_dg.ax), uy = 136 + (int)((y - g_dg.by0) / g_dg.ay), w = *(int32_t *)(im + 0x78), h = *(int32_t *)(im + 0x7c);
    vu_move(im, ux - (w > 0 ? w / 2 : 49), uy - (h > 0 ? h / 2 : 49));
}
int dm_vault_ui_drag(int phase, int x, int y) {
    uint8_t *b = vu_base(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, bk = root ? dm_ui_top(root, "BankExpansionLayout") : 0, vt = bk ? vu_find(bk, "TouchVault", 1) : 0; if (!vt) { g_dg.on = 0; return 0; }
    float dm_ui_drag_lift_get(void); int vh = *(int32_t *)(b + GA_UI_HEIGHT), lift = (int)(vh * dm_ui_drag_lift_get());
    if (phase == 0) { int cell; int row = vu_cell_at(vt, x, y, &cell); if (row < 0 || !dm_vault_has(vt_rows[row].kind, vt_rows[row].row)) return 0;
        uint64_t bg = vu_find(vt, "TouchV_Back", 0); if (!bg) return 0; int a, c, x0, x1, y0, y1, vw = *(int32_t *)(b + GA_UI_WIDTH);
        for (a = 0, c = x; a < c;) { int mid = (a + c) / 2; if (vu_hit(bg, mid, y)) c = mid; else a = mid + 1; } x0 = a;
        for (a = x, c = vw - 1; a < c;) { int mid = (a + c + 1) / 2; if (vu_hit(bg, mid, y)) a = mid; else c = mid - 1; } x1 = a;
        for (a = 0, c = y; a < c;) { int mid = (a + c) / 2; if (vu_hit(bg, x, mid)) c = mid; else a = mid + 1; } y0 = a;
        for (a = y, c = vh - 1; a < c;) { int mid = (a + c + 1) / 2; if (vu_hit(bg, x, mid)) a = mid; else c = mid - 1; } y1 = a;
        g_dg = (typeof(g_dg)){ 1, row, x0, y0, (x1 - x0 + 1) / 1400.0f, (y1 - y0 + 1) / 1000.0f };
        if (g_tip_on) { vu_tip(b, vt, NULL); g_tip_on = 0; }
        uint64_t im = vu_find(vt, "TouchV_Drag", 0); if (im) { vu_image(b, im, vt_rows[row].icon); *(float *)(im + 0x80) = 1.0f; *(float *)(im + 0x94) = 1.0f; }
        if (cell >= 0 && cell < VPER) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_U%d", cell); uint64_t c2 = vu_find(vt, nm, 1), ti = c2 ? vu_find(c2, "TileImg", 0) : 0; if (ti) *(float *)(ti + 0x94) = 0.2f; }
        vu_drag_place(vt, x, y - lift); LOG("编年史仓库：从格子拖 %s", vt_rows[row].key); return 1; }
    if (!g_dg.on) return 0;
    if (phase == 1) { vu_drag_place(vt, x, y - lift); return 1; }
    g_dg.on = 0; vu_move(vu_find(vt, "TouchV_Drag", 0), 99999, 0); atomic_store(&g_dirty, 1);
    const vt_row *e = &vt_rows[g_dg.row]; int fx, fy, cols, rows, dm_ui_bag_point(int, int, int *, int *, int *, int *);
    if (dm_ui_bag_point(x, y - lift, &fx, &fy, &cols, &rows)) { void dm_vault_request_withdraw_at(int, int, int, int, int, int); dm_vault_request_withdraw_at(e->kind, e->row, fx, fy, cols, rows);
        LOG("编年史仓库：%s 拖到背包格 (%.2f, %.2f)（背包 %d×%d）→ 取出", e->key, fx / 100.0, fy / 100.0, cols, rows); }
    else { ((void (*)(int, uint64_t, int, int, int))(b + GA_SOUND_PLAY))(3, 0, 0, 0, 0); LOG("编年史仓库：%s 没拖到背包格上 → 不取（原版错误音）", e->key); }
    return 1;
}




int dm_vault_ui_pager_hit(int x, int y) {
    uint8_t *b = vu_base(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, bk = root ? dm_ui_top(root, "BankExpansionLayout") : 0, vt = bk ? vu_find(bk, "TouchVault", 1) : 0;
    if (!vt || *(int32_t *)(vt + 0x70) != 0) return 0;
    return vu_hit(vu_find(vt, "TouchV_PgLHit", 0), x, y) ? -1 : vu_hit(vu_find(vt, "TouchV_PgRHit", 0), x, y) ? 1 : 0;
}
void dm_vault_ui_flip(int dir) { atomic_store(&g_flip, dir < 0 ? -1 : 1); }
void dm_vault_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (!strcmp(cat, "DropdownListWidgetMessage") || !strcmp(cat, "TouchVaultMessage")) { atomic_store(&g_dirty, 1); return; }
    if (strcmp(cat, "PanelManager") || strcmp(act, "ClosePanel") || strncmp(arg, "TouchV_", 7)) return;
    if (!strcmp(arg + 7, "SubU")) atomic_store(&g_sub_req, 0); else if (!strcmp(arg + 7, "SubS")) atomic_store(&g_sub_req, 1);
    atomic_store(&g_dirty, 1);
}
















static int g_vt_fresh = 1;
static void vu_tiers_pump(uint64_t vt) {
    int dm_padmode_busy(void); int relay = dm_padmode_busy() || dm_ui_reloading();


    static dm_wref lastvt; uint64_t was_at = lastvt.w; int regen = was_at && lastvt.gen != dm_ui_gen(), changed = vt != dm_wref_get(&lastvt);
    int tm = 0; for (int i = 0; i < 3; i++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_T%d", i); uint64_t c = vu_find(vt, nm, 1); if (c && vu_is(c, VT_TOGGLE) && *(uint8_t *)(c + 0xa88)) tm |= 1 << i; }
    if (relay || g_vt_fresh || changed || regen) {
        if (!relay) { static int lg; if (lg++ < 60) LOG("编年史仓库：页%s（%s）→ 品质勾选按筛选 %d 重摆（此刻勾的是 %d）", changed ? "控件换了" : regen ? "界面重建过" : "重新出现", vt == was_at ? "同一地址" : "新地址", g_tiers, tm); }
        dm_wref_set(&lastvt, vt); for (int i = 0; i < 3; i++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_T%d", i); vu_toggle(vu_find(vt, nm, 1), g_tiers >> i & 1); }
        if (!relay || !g_vt_fresh) atomic_store(&g_dirty, 1);
        tm = g_tiers; g_vt_fresh = relay; }
    if (!tm) tm = 7;
    if (tm != g_tiers) { g_tiers = tm; atomic_store(&g_dirty, 1); }
}
void dm_vault_ui_pump(void) {
    uint8_t *b = vu_base(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, bk = root ? dm_ui_top(root, "BankExpansionLayout") : 0;
    static int was; if (!bk || !*(uint8_t *)(bk + 0x50)) { was = 0; g_vt_fresh = 1; return; }
    uint64_t tabs = vu_find(bk, "BankTabs", 0), vt = vu_find(bk, "TouchVault", 1); if (!tabs || !vt || !vu_is(tabs, VT_TABBAR)) { was = 0; g_vt_fresh = 1; return; }
    int on = *(int32_t *)(tabs + 0xdc4) == 5;
    vu_move(vt, on ? 0 : 99999, 0);
    if (!on) { if (g_tip_on) { vu_tip(b, vt, NULL); g_tip_on = 0; } was = 0; g_vt_fresh = 1; return; }

    if (!was) { was = 1; atomic_store(&g_dirty, 1); LOG("编年史仓库：进第六页（%s，部位 %d）", g_sub ? "套装" : "暗金", g_part); }






    { int dm_padmode_current(void); uint64_t dm_padmode_page_keys(uint64_t); void dm_ui_touch(int, float, float); unsigned long long dm_guest_frame(void);
      static unsigned long long up_at; static float ux, uy; unsigned long long fr = dm_guest_frame();
      if (up_at && fr >= up_at) { up_at = 0; dm_ui_touch(2, ux, uy); }
      if (dm_padmode_current()) { int cx = *(int32_t *)(b + GA_PAD_CURSOR_X), cy = *(int32_t *)(b + GA_PAD_CURSOR_Y), w = *(int32_t *)(b + GA_UI_WIDTH), h = *(int32_t *)(b + GA_UI_HEIGHT), over = w > 0 && h > 0 && dm_vault_ui_over(cx, cy);










          enum { Y_HOLD = 20 }; uint64_t dm_padmode_page_held(void); void dm_padmode_page_pulse(uint64_t), dm_padmode_vault_hint(int); dm_padmode_vault_hint(1);
          uint64_t ev = dm_padmode_page_keys(0x40 | 0x80 | 0x8 | (over ? 0x1 : 0));
          { static unsigned long long y0; static int fired; int held = (dm_padmode_page_held() & 0x8) != 0;
            if (ev & 0x8) { y0 = fr; fired = 0; }
            if (held && y0 && !fired && fr - y0 >= Y_HOLD) { fired = 1; dm_cursor_item_info ci;
                if (dm_cursor_item(&ci) && ci.container == DM_ITEM_AT_INVENTORY) { void dm_vault_request_deposit(uint32_t); dm_vault_request_deposit(ci.id); LOG("编年史仓库：手柄按住 Y，背包的 %s（id %u）→ 存进编年史仓库", ci.code, ci.id); }
                else LOG("编年史仓库：手柄按住 Y，光标下不是背包物品，不动"); }
            if (!held && y0) { if (!fired) { dm_padmode_page_pulse(0x8); LOG("编年史仓库：手柄短按 Y → 还给原版（丢弃）"); } y0 = 0; } }
          { void dm_padmode_legend(int, const char *);
            if (over) dm_padmode_legend(0, dm_str(b, "dmPadLegView")); dm_padmode_legend(19, dm_str(b, "dmPadLegPrev")); dm_padmode_legend(20, dm_str(b, "dmPadLegNext")); }
          if (ev & 0x40) dm_vault_ui_flip(-1); else if (ev & 0x80) dm_vault_ui_flip(1);
          if ((ev & 0x1) && over && !up_at) { ux = (float)cx / (float)w; uy = (float)cy / (float)h; dm_ui_touch(0, ux, uy); up_at = fr + 2; LOG("编年史仓库：手柄 A → 在光标处 (%d,%d) 点一下", cx, cy); } } }
    int sr = atomic_exchange(&g_sub_req, -1); if (sr >= 0 && sr != g_sub) { g_sub = sr; atomic_store(&g_dirty, 1); }
    int fl = atomic_exchange(&g_flip, 0); if (fl && g_query[0]) { g_page_q += fl; atomic_store(&g_dirty, 1); fl = 0; }
    if (fl) { g_page[g_sub ? VT_NPART : g_part] += fl; atomic_store(&g_dirty, 1); LOG("编年史仓库：%s翻%s一页", g_sub ? "套装" : vt_part_name[g_part], fl > 0 ? "下" : "上"); }
    { static int pinit; const char *e; if (!pinit && (e = getenv("DM_TEST_VAULT_PART"))) { int p = atoi(e); if (p >= 0 && p < VT_NPART) { g_part = p; atomic_store(&g_dirty, 1); LOG("编年史仓库：测试开关先选部位 %d", p); } } pinit = 1; }
    vu_tiers_pump(vt);
    { int dm_ui_vault_drop(void); static uint32_t held; static int wait;
      uint32_t h = dm_vault_ui_held(); if (held && !h && (uint32_t)dm_ui_vault_drop() == held) { wait = 8; g_drop_id = (int)held; } held = h;
      if (wait && !--wait) { void dm_vault_request_deposit(uint32_t); dm_vault_request_deposit((uint32_t)g_drop_id); LOG("编年史仓库：拖进来的物品 id %u → 存入", (unsigned)g_drop_id); } }
    { int dm_vault_take_full_sound(void); if (dm_vault_take_full_sound()) ((void (*)(int, uint64_t, int, int, int))(b + GA_SOUND_PLAY))(3, 0, 0, 0, 0); }








    { uint64_t in = vu_find(vt, "TouchV_Search", 1); if (in && *(uint64_t *)in - (uint64_t)(uintptr_t)b != GA_VT_INPUT_TEXT) in = 0;
      const char *q = in ? *(const char **)(in + 0x558) : NULL; uint64_t qn = in ? *(uint64_t *)(in + 0x560) : 0; char nq[sizeof g_query] = "";
      if (q && qn && qn < sizeof nq) { memcpy(nq, q, qn); nq[qn] = 0; } if (strcmp(nq, g_query)) { strlcpy(g_query, nq, sizeof g_query); g_page_q = 0; atomic_store(&g_dirty, 1); LOG("编年史仓库：搜索「%s」", g_query); } }
    static int gen = -1; int g = dm_vault_gen(); if (g != gen) { gen = g; atomic_store(&g_dirty, 1); }
    if (g_tip_on) { vu_tip_exact(b, vt); vu_tip_place(vu_find(vt, "TouchV_Tip", 0), vu_find(vt, "TouchV_TipBg", 0)); }
    static int fr; int dirty = atomic_exchange(&g_dirty, 0); if (!dirty && ++fr % 30) return;
    vu_frame(vu_find(vu_find(vt, "TouchV_SubU", 1), "Img", 0), g_sub == 0);
    vu_frame(vu_find(vu_find(vt, "TouchV_SubS", 1), "Img", 0), g_sub == 1);
    vu_show(vu_find(vt, "TouchV_Parts", 0), g_sub == 0); vu_show(vu_find(vt, "TouchV_Tiers", 0), g_sub == 0 && !g_query[0]);
    {




        static const int zx[3] = { 0, 200, 400 }, ex[3] = { 0, 240, 430 }; int zh = dm_lang_zh(); const int *xs = zh ? zx : ex;
        vu_move(vu_find(vt, "TouchV_Tiers", 0), VX0 + VW0 - (zh ? 590 : 650), VT_TIER_Y);
        for (int i = 0; i < 3; i++) { char nm[24]; snprintf(nm, sizeof nm, "TouchV_T%d", i); vu_move(vu_find(vt, nm, 1), xs[i], 0); snprintf(nm, sizeof nm, "TouchV_T%d_Text", i); vu_move(vu_find(vt, nm, 1), xs[i] + 75, -5); } }
    for (int p = 0; p < VT_NPART; p++) { char nm[16]; snprintf(nm, sizeof nm, "TouchV_PT%d", p); uint64_t t = vu_find(vt, nm, 1); if (!t) continue;
        vu_frame(vu_find(t, "Img", 0), p == g_part); vu_show(vu_find(t, "TextOn", 0), p == g_part); vu_show(vu_find(t, "Text", 0), p != g_part); }
    int grid = g_sub == 0 || g_query[0]; vu_move(vu_find(vt, "TouchV_Uniq", 0), grid ? 156 : 99999, VU_UNIQ_Y); vu_move(vu_find(vt, "TouchV_Sets", 0), grid ? 99999 : 156, 296);
    if (dirty && g_tip_on) { vu_tip(b, vt, NULL); g_tip_on = 0; }
    if (g_query[0]) vu_search(b, vt); else if (g_sub == 0) vu_uniques(b, vt); else vu_sets(b, vt);
    { struct { const char *s; uint64_t n; } key = { "dmVaultPage", 11 };                                                                                                                                                                                                                                                                                                                                                        uint64_t fmt = ((uint64_t (*)(void *))(b + GA_LOC_STRING_GET))(&key); char pt[256]; memset(pt, 0, sizeof pt);
      ((void (*)(void *, uint64_t, int64_t, int64_t))(b + GA_TEXT_FORMAT))(pt, fmt, g_pg_cur, g_pg_tot); vu_text(b, vu_find(vt, "TouchV_Page", 0), pt);
      vu_frame(vu_find(vt, "TouchV_PgL", 0), g_pg_cur > 1 ? 0 : 3); vu_frame(vu_find(vt, "TouchV_PgR", 0), g_pg_cur < g_pg_tot ? 0 : 3); }
}




const char *dm_vault_item_icon(int kind, int row) {
    for (int i = 0; i < VT_NROW; i++) if (vt_rows[i].kind == kind && vt_rows[i].row == row) return vt_rows[i].icon;
    return NULL;
}
const char *dm_vault_item_name(uint8_t *b, int kind, int row) {
    for (int i = 0; b && i < VT_NROW; i++) if (vt_rows[i].kind == kind && vt_rows[i].row == row) return vu_name(b, vt_rows[i].key);
    return NULL;
}
