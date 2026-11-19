
















#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dm.h"
#include "dm_widget.h"
#include "dm_cursor.h"
#include "dm_cursor_item.h"
#include "dm_input.h"
#include "dm_i18n.h"
#include "dm_actions.h"
int dm_padmode_current(void);
#include "dm_looks.h"

enum {
    UI_ROOT = 0x2f05458, NPC_ID = 0x2d5ebfc, NPC_TABLE = 0x2d607a0, FN_UNIT = 0x45f5f0, FN_TAB_SELECT = 0xad9b80, TAB_SEL = 0xdc4, TAB_AVAIL = 0xe88,
    CL_PLAYER_ID = 0x2d20e4c, FN_CL_UNIT = 0x45f9d0, FN_STAT = 0x5b9290, FN_IMAGE = 0xac1704, FN_TEXT_SET = 0x136e5c, FN_TEXT_LAYOUT = 0xadc434,
    LOOKS_TAB = 4, CHARSI = 154, CELL = 98, GX = 162, GY = 159, TILES = DM_LOOKS_TILES, MAXLOOKS = 256, OFF = 99999, DYE_TILES = 18, DYE_ROW = DM_LOOKS_DYE_ROW,
    SW_X = 12, SW_Y = 72,
    PADLEG_X = -50, PADLEG_END = 610, PADLEG_Y = 1508, PADLEG_KEY = 66, PADLEG_GAP = 26,

};
static const struct { uint8_t body; const char *slot; } k_slots[] = {
    { 1, "slot_head" }, { 3, "slot_torso" }, { 4, "slot_right_arm" }, { 5, "slot_left_arm" }, { 10, "slot_gloves" }, { 9, "slot_feet" }, { 8, "slot_belt" },
};

static uint8_t *lu_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t lu_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 12) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = lu_find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static int lu_ours(uint64_t w) {
    for (int k = 0; w && k < 16; k++, w = *(uint64_t *)(w + 0x30)) { const char *nm = *(const char **)(w + 8); if (nm && (!strcmp(nm, "TouchLooks") || !strcmp(nm, "TouchLooksSel"))) return 1; }
    return 0;
}
static int lu_guard(uint64_t w) {
    if (!w) return 0; if (lu_ours(w)) return 1;
    static int n; if (n++ < 10) { const char *nm = *(const char **)(w + 8); LOG("ASSUME looks.ui_own: refused to write the game's widget %s", nm ? nm : "?"); }
    return 0;
}
static void lu_show(uint64_t w, int on) { if (lu_guard(w)) { *(uint8_t *)(w + 0x50) = (uint8_t)on; *(uint8_t *)(w + 0x51) = (uint8_t)on; } }
static void lu_relayout(uint64_t w) { ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); }
static void lu_rect(uint64_t w, int x, int y, int wd, int ht) {
    if (!lu_guard(w)) return; int32_t *r = (int32_t *)(w + 0x70);
    if (r[0] == x && r[1] == y && (wd < 0 || r[2] == wd) && (ht < 0 || r[3] == ht)) return;
    r[0] = x; r[1] = y; if (wd >= 0) r[2] = wd; if (ht >= 0) r[3] = ht; lu_relayout(w);
}
static void lu_move(uint64_t w, int x, int y) { lu_rect(w, x, y, -1, -1); }
static void lu_text(uint8_t *b, uint64_t w, const char *s) {
    if (!lu_guard(w) || !dm_widget_is(b, w, DM_VT_TEXT)) return;
    size_t n = strlen(s); if (*(uint64_t *)(w + 0x90) == n && !memcmp(*(const char **)(w + 0x88), s, n)) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(uintptr_t)(b + FN_TEXT_SET))(w + 0x88, s, n); ((void (*)(uint64_t))(uintptr_t)(b + FN_TEXT_LAYOUT))(w);
}
static void lu_color(uint8_t *b, uint64_t w, uint32_t rgb) {
    if (!lu_guard(w) || !dm_widget_is(b, w, DM_VT_TEXT)) return; int32_t *c = (int32_t *)(w + 0xbc), r = rgb >> 16 & 0xff, g = rgb >> 8 & 0xff, bl = rgb & 0xff;
    if (c[0] == r && c[1] == g && c[2] == bl) return; c[0] = r; c[1] = g; c[2] = bl; ((void (*)(uint64_t))(uintptr_t)(b + FN_TEXT_LAYOUT))(w);
}

static void lu_picture(uint8_t *b, uint64_t im, const char *path, int bw, int bh, float alpha) {
    if (!lu_guard(im) || !dm_widget_is(b, im, DM_VT_IMAGE) || !path) return;
    static struct { dm_wref r; const char *path; } cache[TILES]; int ci = -1, fr = -1;
    for (int i = 0; i < TILES; i++) { uint64_t cw = dm_wref_get(&cache[i].r); if (cw == im) { ci = i; break; } if (fr < 0 && !cw) fr = i; }
    if (ci < 0 && fr >= 0) { ci = fr; dm_wref_set(&cache[ci].r, im); cache[ci].path = NULL; }
    if (ci >= 0 && (cache[ci].path != path || *(int32_t *)(im + 0x78) <= 0 || *(int32_t *)(im + 0x7c) <= 0)) {
        struct { char *p; uint64_t n, cap; char buf[0xa0]; } s; size_t n = strlen(path); if (n >= sizeof s.buf) return;
        memcpy(s.buf, path, n + 1); s.p = s.buf; s.n = n; s.cap = (sizeof s.buf - 1) | 1ull << 63;
        ((void (*)(uint64_t, void *))(uintptr_t)(b + FN_IMAGE))(im, &s); cache[ci].path = path; *(float *)(im + 0x80) = 1.0f;
    }
    int iw = *(int32_t *)(im + 0x78), ih = *(int32_t *)(im + 0x7c);
    if (iw > 0 && ih > 0) { float s = (float)bw / iw < (float)bh / ih ? (float)bw / iw : (float)bh / ih; if (s > 1.0f) s = 1.0f;
        *(float *)(im + 0x80) = s; lu_move(im, (int)((bw - iw * s) / 2), (int)((bh - ih * s) / 2)); }
    *(float *)(im + 0x94) = alpha;
}
static int lu_hit(uint64_t w, int x, int y) { return w && *(uint8_t *)(w + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x48))(w, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1); }

static int npc_class(uint8_t *b) {
    uint32_t id = *(volatile uint32_t *)(b + NPC_ID); if (!id) return -1;
    uint64_t u = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(uintptr_t)(b + FN_UNIT))((uint64_t)(uintptr_t)(b + NPC_TABLE), id & 0x7f, id, 1);
    return u ? (int)*(const uint32_t *)(u + 4) : -1;
}
static uint64_t client_player(uint8_t *b) { return ((uint64_t (*)(uint32_t))(uintptr_t)(b + FN_CL_UNIT))(*(uint32_t *)(b + CL_PLAYER_ID)); }

static uint64_t worn_item(uint64_t pl, int body) {
    uint64_t inv = pl ? *(uint64_t *)(pl + 0x90) : 0, it = inv ? *(uint64_t *)(inv + 0x10) : 0;
    for (int k = 0; it && k < 512; k++) {
        uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        int g, w, h;
        if (*(uint32_t *)(it + 0xc) == 1 && *(uint8_t *)(d + 0x54) == body && dm_looks_page_item(it, &g, &w, &h)) return it;
        it = *(uint64_t *)(d + 0xb0);
    }
    return 0;
}


static int g_active, g_body, g_page, g_pages = 1, g_n, g_dirty = 1, g_can_apply;
static int g_trect[TILES][4], g_drect[DYE_TILES][4];
static dm_look g_list[MAXLOOKS], g_pick; static int g_picked;
static dm_dye_offer g_dlist[DYE_TILES]; static int g_dn, g_dpick = -1;
static uint32_t g_preview;

static void end_preview(void) { if (g_preview) dm_looks_request_preview(0, (dm_look){ 0, 0 }, -1); g_preview = 0; g_picked = 0; g_dpick = -1; g_dirty = 1; }
static int same(dm_look a, dm_look b) { return a.kind == b.kind && a.id == b.id; }
static int sel_index(void) { for (int i = 0; g_picked && i < g_n; i++) if (same(g_list[i], g_pick)) return i; return -1; }
static int dsel_index(void) { return g_dpick < 0 ? -1 : dm_dyes_offer_at(g_dlist, g_dn, g_dpick); }

static void preview_now(uint64_t it) {
    dm_look cur, shown; dm_looks_page_state(it, &cur, &shown);
    if (!g_picked && g_dpick < 0) { end_preview(); return; }
    uint32_t id = *(uint32_t *)(it + 8);
    g_preview = id; dm_looks_request_preview(id, g_picked ? g_pick : cur, g_dpick);
}

static int page_cap(int w, int h) { return dm_looks_page_cap(w, h, dm_looks_dyes_on()); }



static void tab_available(uint64_t tabs, int on) {
    uint8_t *a = (uint8_t *)(uintptr_t)(tabs + TAB_AVAIL + LOOKS_TAB);
    if (*a != (uint8_t)on) { *a = (uint8_t)on; lu_relayout(tabs); }
}

int dm_looks_ui_active(void) { return g_active; }

static void place_slot_frame(uint64_t root, int body) {
    uint64_t ip = dm_ui_top(root, "PlayerInventoryExpansionLayout"), fr = ip ? lu_find(ip, "TouchLooksSel", 1) : 0;
    if (!fr) return;
    const char *name = NULL; for (unsigned i = 0; i < sizeof k_slots / sizeof k_slots[0]; i++) if (k_slots[i].body == body) name = k_slots[i].slot;
    uint64_t sl = name ? lu_find(ip, name, 1) : 0;
    if (!g_active || !sl || *(uint64_t *)(sl + 0x30) != *(uint64_t *)(fr + 0x30)) { lu_move(fr, OFF, 0); return; }
    const int32_t *r = (const int32_t *)(sl + 0x70);
    lu_rect(fr, r[0] - 18, r[1] - 18, r[2] + 36, r[3] + 36);
}

static void draw(uint8_t *b, uint64_t lk, uint64_t pl, uint64_t it) {
    char nm[24];
    int group = 0, w = 1, h = 1; dm_looks_page_item(it, &group, &w, &h);
    if (g_dirty) g_n = dm_looks_page_offer(it, (int)*(uint32_t *)(pl + 4), g_list, MAXLOOKS);
    if (g_picked && sel_index() < 0) { LOG("looks: the tapped look left the list, preview ended"); end_preview(); }
    dm_look cur, shown; dm_looks_page_state(it, &cur, &shown);
    int cdye = 0, sdye = 0; dm_looks_page_dye(it, &cdye, &sdye);
    int where = dm_looks_page_where(it, g_picked ? g_pick : cur), rows = dm_looks_dyes_on() ? DYE_ROW : 10;
    static int last_where = -1, last_cdye = -1;

    if (g_dirty || where != last_where || cdye != last_cdye) {
        g_dn = 0; g_dlist[g_dn++] = (dm_dye_offer){ 0, -1 };
        if (where) { g_dn += dm_looks_page_gems(g_dlist + 1, DYE_TILES - 2); int have = !cdye; for (int i = 1; i < g_dn; i++) if (g_dlist[i].dye == cdye) have = 1;
            if (!have && dm_looks_dye_gem(cdye) >= 0) g_dlist[g_dn++] = (dm_dye_offer){ (uint8_t)cdye, dm_looks_dye_gem(cdye) }; }
        last_where = where; last_cdye = cdye; g_dirty = 0;
    }
    if (g_dpick >= 0 && (!where || dsel_index() < 0)) {
        LOG("looks: the tapped colour left the colour row, its preview ended"); g_dpick = -1; preview_now(it);
    }
    int sel = sel_index(), dsel = dsel_index();
    int per = 10 / w, cap = page_cap(w, h); if (cap < 1) cap = 1;
    int pages = (g_n + cap - 1) / cap; if (pages < 1) pages = 1; if (g_page >= pages) g_page = pages - 1; if (g_page < 0) g_page = 0;
    g_pages = pages;
    int tw = w * CELL, th = h * CELL, sel_tile = -1, busy = dm_looks_page_busy(it);
    for (int t = 0; t < TILES; t++) {
        snprintf(nm, sizeof nm, "TouchL_T%d", t); uint64_t tile = lu_find(lk, nm, 1); int idx = dm_looks_page_index(cap, g_page, t, g_n);
        if (!tile) continue;
        g_trect[t][0] = OFF;
        if (idx < 0) { lu_move(tile, OFF, 0); continue; }
        dm_look l = g_list[idx];
        lu_rect(tile, GX + (t % per) * tw, GY + (t / per) * th, tw, th);
        g_trect[t][0] = GX + (t % per) * tw; g_trect[t][1] = GY + (t / per) * th; g_trect[t][2] = tw; g_trect[t][3] = th;
        lu_rect(lu_find(tile, "Hit", 0), 0, 0, tw, th);
        uint64_t hi = lu_find(tile, "Hi", 0); lu_rect(hi, 0, 0, tw, th); lu_show(hi, g_preview && idx == sel);
        uint64_t box = lu_find(tile, "Box", 0); lu_rect(box, 6, 6, tw - 12, th - 12);
        lu_picture(b, lu_find(tile, "Img", 1), dm_looks_icon(it, l), tw - 12, th - 12, l.kind == DM_LOOK_INVISIBLE || busy ? 0.3f : 1.0f);
        uint64_t lab = lu_find(tile, "Label", 0); lu_rect(lab, 4, th - 40, tw - 8, 36);
        lu_text(b, lab, l.kind == DM_LOOK_ORIGINAL ? dm_str(b, "dmLooksOriginal") : l.kind == DM_LOOK_INVISIBLE ? dm_str(b, "dmLooksInvisible") : "");
        uint64_t gold = lu_find(tile, "Gold", 0); lu_rect(gold, -18, -18, tw + 36, th + 36); lu_show(gold, same(l, cur));
        if (idx == sel) sel_tile = t;
    }

    uint64_t nb = lu_find(lk, "TouchL_NameBg", 0), nt = lu_find(lk, "TouchL_Name", 0); const char *name = NULL;
    if (sel_tile >= 0 && (g_list[sel].kind == DM_LOOK_BASE || g_list[sel].kind == DM_LOOK_UNIQUE || g_list[sel].kind == DM_LOOK_SET)) name = dm_looks_name(g_list[sel]);
    if (name) {
        int x = GX + (sel_tile % per) * tw + tw / 2 - 200, y = GY + (sel_tile / per) * th + th + 4;
        if (x < GX) x = GX; if (x > GX + 10 * CELL - 400) x = GX + 10 * CELL - 400; if (y > GY + rows * CELL - 60) y = GY + (sel_tile / per) * th - 64;
        lu_move(nb, x, y); lu_move(nt, x, y); lu_text(b, nt, name);
    } else { lu_move(nb, OFF, 0); lu_move(nt, OFF, 0); }
    int dsel_tile = -1, swatch[DM_DYE_COLOURS]; for (int c = 0; c < DM_DYE_COLOURS; c++) swatch[c] = -1;
    for (int t = 0; t < DYE_TILES; t++) {
        snprintf(nm, sizeof nm, "TouchL_D%d", t); uint64_t tile = lu_find(lk, nm, 1);
        g_drect[t][0] = OFF;
        if (!tile) continue;
        if (!where || t >= g_dn) { lu_move(tile, OFF, 0); continue; }
        lu_move(tile, GX + (1 + t % 9) * CELL, GY + (DYE_ROW + t / 9) * CELL);
        g_drect[t][0] = GX + (1 + t % 9) * CELL; g_drect[t][1] = GY + (DYE_ROW + t / 9) * CELL; g_drect[t][2] = g_drect[t][3] = CELL;
        lu_show(lu_find(tile, "Hi", 0), g_preview && t == dsel);
        int bh = t ? SW_Y - 8 : CELL - 12;
        lu_rect(lu_find(tile, "Box", 0), 6, t ? 4 : 6, CELL - 12, bh);
        lu_picture(b, lu_find(tile, "Img", 1), t ? dm_looks_gem_icon(g_dlist[t].gem) : dm_looks_icon(it, (dm_look){ DM_LOOK_ORIGINAL, 0 }), CELL - 12, bh, busy ? 0.3f : 1.0f);
        lu_text(b, lu_find(tile, "Label", 0), t ? "" : dm_str(b, "dmLooksOriginal"));
        lu_show(lu_find(tile, "Gold", 0), g_dlist[t].dye == cdye);
        if (t && g_dlist[t].dye >= 1 && g_dlist[t].dye <= DM_DYE_COLOURS) swatch[g_dlist[t].dye - 1] = t;
        if (t == dsel) dsel_tile = t;
    }
    for (int c = 0; c < DM_DYE_COLOURS; c++) {
        snprintf(nm, sizeof nm, "TouchL_S%d", c); int t = swatch[c];
        lu_move(lu_find(lk, nm, 0), t >= 0 ? GX + (1 + t % 9) * CELL + SW_X : OFF, GY + (DYE_ROW + t / 9) * CELL + SW_Y);
    }
    lu_move(lu_find(lk, "TouchL_DyeTitle", 0), where ? GX : OFF, GY + DYE_ROW * CELL + 8); lu_text(b, lu_find(lk, "TouchL_DyeTitle", 0), dm_str(b, "dmDyeTitle"));
    lu_move(lu_find(lk, "TouchL_DyeLine", 0), where ? GX : OFF, GY + DYE_ROW * CELL - 2);
    uint64_t db = lu_find(lk, "TouchL_DyeNameBg", 0), dt = lu_find(lk, "TouchL_DyeName", 0); const char *gname = dsel_tile > 0 ? dm_looks_gem_name(g_dlist[dsel_tile].gem) : NULL;
    if (gname && g_dlist[dsel_tile].dye != cdye) {
        char line[96]; snprintf(line, sizeof line, dm_str(b, "dmDyeUses"), gname);
        int x = GX + (1 + dsel_tile % 9) * CELL + CELL / 2 - 450; if (x < GX) x = GX; if (x > GX + 10 * CELL - 900) x = GX + 10 * CELL - 900;
        int y = GY + DYE_ROW * CELL - 66;
        lu_move(db, x, y); lu_move(dt, x, y); lu_text(b, dt, line);
    } else { lu_move(db, OFF, 0); lu_move(dt, OFF, 0); }

    uint64_t pb = lu_find(lk, "TouchL_PriceBg", 0), pt = lu_find(lk, "TouchL_Price", 0), ap = lu_find(lk, "TouchL_Apply", 0);
    int locked = dm_looks_locked();
    int look_change = sel >= 0 && !same(g_list[sel], cur), dye_change = dsel >= 0 && g_dlist[dsel].dye != cdye;
    int price = look_change ? dm_looks_page_price(it, g_list[sel]) : 0;
    int (*stat)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(b + FN_STAT);
    int ok = stat(pl, 14, 0) + stat(pl, 15, 0) >= price;
    if (price && !locked && !busy) {
        char s[24]; snprintf(s, sizeof s, "%d", price);
        lu_show(pb, 1); lu_move(pt, 547, 1254); lu_text(b, pt, s); lu_color(b, pt, ok ? 0x4fc238 : 0xfc4646);
    } else { lu_show(pb, 0); lu_move(pt, OFF, 0); }

    g_can_apply = (look_change || dye_change) && ok && !locked && !busy;
    lu_move(ap, g_can_apply && !dm_padmode_current() ? 470 : OFF, 1492);
    lu_text(b, lu_find(lk, "TouchL_Hint", 0), locked ? dm_str(b, "dmLooksLocked") : busy ? dm_str(b, "dmLooksSaving") : where && g_dn == 1 ? dm_str(b, "dmDyeNoGem")
        : where == DM_DYE_FIGURE ? dm_str(b, "dmDyeFigureOnly") : group >= 5 ? dm_str(b, "dmLooksHintIcon") : group == 4 ? dm_str(b, "dmLooksHintArmor") : "");
    char pg[16]; snprintf(pg, sizeof pg, "%d/%d", g_page + 1, pages);
    lu_text(b, lu_find(lk, "TouchL_Page", 0), pages > 1 ? pg : "");
    lu_move(lu_find(lk, "TouchL_PgL", 0), pages > 1 ? 900 : OFF, GY + 10 * CELL + 2); lu_move(lu_find(lk, "TouchL_PgLHit", 0), pages > 1 ? 880 : OFF, GY + 10 * CELL - 10);
    lu_move(lu_find(lk, "TouchL_PgR", 0), pages > 1 ? 1070 : OFF, GY + 10 * CELL + 2); lu_move(lu_find(lk, "TouchL_PgRHit", 0), pages > 1 ? 1050 : OFF, GY + 10 * CELL - 10);
    lu_text(b, lu_find(lk, "TouchL_Broken", 0), "");
    lu_move(lu_find(lk, "TouchL_Anew", 0), OFF, GY + 500);
}

static void pad_legend(uint8_t *b, uint64_t lk, int a, int x, int y);
static void draw_broken(uint8_t *b, uint64_t lk) {
    char nm[24];
    for (int t = 0; t < TILES; t++) { snprintf(nm, sizeof nm, "TouchL_T%d", t); lu_move(lu_find(lk, nm, 1), OFF, 0); }
    for (int t = 0; t < DYE_TILES; t++) { snprintf(nm, sizeof nm, "TouchL_D%d", t); lu_move(lu_find(lk, nm, 1), OFF, 0); }
    for (int c = 0; c < DM_DYE_COLOURS; c++) { snprintf(nm, sizeof nm, "TouchL_S%d", c); lu_move(lu_find(lk, nm, 0), OFF, 0); }
    for (int t = 0; t < TILES; t++) g_trect[t][0] = OFF;
    for (int t = 0; t < DYE_TILES; t++) g_drect[t][0] = OFF;
    g_can_apply = 0;
    lu_move(lu_find(lk, "TouchL_Focus", 0), OFF, 0);
    pad_legend(b, lk, 0, 0, 0);
    static const char *const off[] = { "TouchL_NameBg", "TouchL_Name", "TouchL_Price", "TouchL_Apply", "TouchL_PgL", "TouchL_PgR", "TouchL_PgLHit", "TouchL_PgRHit",
        "TouchL_DyeTitle", "TouchL_DyeLine", "TouchL_DyeNameBg", "TouchL_DyeName" };
    for (unsigned i = 0; i < sizeof off / sizeof off[0]; i++) lu_move(lu_find(lk, off[i], 0), OFF, 0);
    lu_show(lu_find(lk, "TouchL_PriceBg", 0), 0);
    lu_text(b, lu_find(lk, "TouchL_Hint", 0), ""); lu_text(b, lu_find(lk, "TouchL_Page", 0), "");
    lu_text(b, lu_find(lk, "TouchL_Broken", 0), dm_str(b, "dmLooksBroken"));
    lu_move(lu_find(lk, "TouchL_Anew", 0), 474, GY + 500);
}

#ifndef DM_PLAYER_BUILD
static void test_pump(uint8_t *b, uint64_t root, uint64_t tabs);
#endif


uint64_t dm_padmode_page_keys(uint64_t mask); void dm_padmode_page_drop(uint64_t mask); void dm_padmode_legend_drop(int act);
int dm_looks_ui_tap(int x, int y); int dm_looks_ui_item_tap(void); void dm_looks_ui_on_uimsg(const char *cat, const char *act, const char *arg);

static int lu_reach(uint64_t w, int x, int y, int dx, int dy) { int lo = 0, hi = 8192; while (hi - lo > 1) { int m = (lo + hi) / 2; if (lu_hit(w, x + dx * m, y + dy * m)) lo = m; else hi = m; } return lo; }
static int tile_rect(int k, int i, int *r) { const int *s = k == 1 ? g_trect[i] : g_drect[i]; if (s[0] >= OFF) return 0; memcpy(r, s, 4 * sizeof *r); return 1; }


static int pad_step(int fx, int fy, int dx, int dy, int *kind, int *idx) {
    long best = -1; int r[4];
    for (int k = 1; k <= 2; k++) for (int i = 0, n = k == 1 ? TILES : DYE_TILES; i < n; i++) {
        if (!tile_rect(k, i, r)) continue;
        int ax = r[0] + r[2] / 2 - fx, ay = r[1] + r[3] / 2 - fy, along = dx ? ax * dx : ay * dy, across = dx ? abs(ay) : abs(ax);
        if (along <= CELL / 4) continue;
        long score = along + 2L * across;
        if (best < 0 || score < best) { best = score; *kind = k; *idx = i; }
    }
    return best >= 0;
}
static int looks_slot(uint32_t id) {
    uint8_t *b = lu_base(); uint64_t pl = b ? client_player(b) : 0;
    for (unsigned i = 0; i < sizeof k_slots / sizeof k_slots[0]; i++) { uint64_t it = worn_item(pl, k_slots[i].body); if (it && *(uint32_t *)(it + 8) == id) return k_slots[i].body; }
    return 0;
}

static struct { int ok, x0, y0; float px; } g_cal;
static int calibrate(uint8_t *b, uint64_t back) {
    static unsigned tries; if (g_cal.ok || tries++ % 30) return g_cal.ok;
    int W = *(int32_t *)(b + 0x211da38), H = *(int32_t *)(b + 0x211da3c);
    for (int y = H / 4; !g_cal.ok && y < H * 3 / 4; y += 16) for (int x = 0; x < W; x += 16) if (lu_hit(back, x, y)) {
        int x0 = x - lu_reach(back, x, y, -1, 0), x1 = x + lu_reach(back, x, y, 1, 0), y0 = y - lu_reach(back, x, y, 0, -1);
        if (x1 > x0) { g_cal.ok = 1; g_cal.x0 = x0; g_cal.y0 = y0; g_cal.px = (float)(x1 - x0) / (10 * CELL); LOG("looks: controller grid at %d,%d, %d wide", x0, y0, x1 - x0); }
        break;
    }
    return g_cal.ok;
}
static int to_px(int lx) { return g_cal.x0 + (int)((lx - GX) * g_cal.px); }
static int to_py(int ly) { return g_cal.y0 + (int)((ly - GY) * g_cal.px); }

static int focus_point(uint8_t *b, uint64_t vp, uint64_t back, int *fx, int *fy) {
    uint64_t root = *(uint64_t *)(b + UI_ROOT), fm = root ? *(uint64_t *)(root + 0xd0) : 0, f = fm ? *(uint64_t *)(fm + 0x190) : 0;
    int inside = 0; uint64_t w = f; for (int d = 0; w && d < 16 && !inside; d++, w = *(uint64_t *)(w + 0x30)) inside = w == vp;
    static uint64_t last_f; static int last_c, last_r;
    if (inside && dm_widget_is_slot(b, f)) {
        int c = *(int32_t *)(f + 0x544), r = *(int32_t *)(f + 0x548);
        if (f != last_f || c != last_c || r != last_r) { last_f = f; last_c = c; last_r = r;
            LOG("looks: controller focus %s cell %d,%d", *(const char **)(f + 8) ? *(const char **)(f + 8) : "?", c, r); }
        if (c >= 0 && c < 10 && r >= 0 && r < 10) { *fx = GX + c * CELL + CELL / 2; *fy = GY + r * CELL + CELL / 2; return 1; }
    }
    int cx = *(int32_t *)(b + 0x2d128f8), cy = *(int32_t *)(b + 0x2d128fc);
    if (!lu_hit(back, cx, cy)) return 0;
    *fx = GX + (int)((cx - g_cal.x0) / g_cal.px); *fy = GY + (int)((cy - g_cal.y0) / g_cal.px);
    return 1;
}
static int tile_at(int fx, int fy, int *kind, int *idx) {
    int r[4];
    for (int k = 1; k <= 2; k++) for (int i = 0, n = k == 1 ? TILES : DYE_TILES; i < n; i++)
        if (tile_rect(k, i, r) && fx >= r[0] && fx < r[0] + r[2] && fy >= r[1] && fy < r[1] + r[3]) { *kind = k; *idx = i; return 1; }
    return 0;
}


static void pad_legend(uint8_t *b, uint64_t lk, int a, int x, int y) {
    static const struct { const char *img, *txt, *key; int act; } k[3] = {
        { "TouchL_PKA", "TouchL_PTA", "dmPadLegChoose", 0 }, { "TouchL_PKX", "TouchL_PTX", "dmLooksApply", 2 }, { "TouchL_PKY", "TouchL_PTY", "dmPadLegNext", 1 } };
    int on[3] = { a, x, y }, px = PADLEG_X;
    for (int i = 0; i < 3; i++) {
        uint64_t im = lu_find(lk, k[i].img, 0), tx = lu_find(lk, k[i].txt, 0);
        if (on[i] && im && tx) { dm_padmode_legend_drop(k[i].act); lu_text(b, tx, dm_str(b, k[i].key)); }
        int w = tx ? *(int32_t *)(tx + 0x78) : 0;
        if (!on[i] || !im || !tx || px + PADLEG_KEY + w > PADLEG_END) { lu_move(im, OFF, 0); lu_move(tx, OFF, 0); continue; }
        lu_move(im, px, PADLEG_Y - 12); lu_move(tx, px + PADLEG_KEY, PADLEG_Y + 4);
        px += PADLEG_KEY + w + PADLEG_GAP;
    }
}
static void pad_pump(uint8_t *b, uint64_t vp, uint64_t lk) {
    uint64_t back = lu_find(lk, "TouchL_Back", 0), fr = lu_find(lk, "TouchL_Focus", 0);
    int fx, fy, fk = 0, fi = -1, r[4];
    int on = calibrate(b, back) && focus_point(b, vp, back, &fx, &fy);
    if (on) tile_at(fx, fy, &fk, &fi);
    if (fk && tile_rect(fk, fi, r)) lu_rect(fr, r[0] - 18, r[1] - 18, r[2] + 36, r[3] + 36); else lu_move(fr, OFF, 0);
    dm_cursor_item_info ci; int worn = !on && dm_cursor_item(&ci) && ci.container == DM_ITEM_AT_EQUIP && looks_slot(ci.id);
    int apply = g_can_apply, paged = on && g_pages > 1;
    uint64_t dirs = DM_BTN_LEFT | DM_BTN_RIGHT | DM_BTN_UP | DM_BTN_DOWN;
    uint64_t mask = (on ? dirs | DM_BTN_A : 0) | (worn ? DM_BTN_A : 0) | (apply ? DM_BTN_X : 0) | (paged ? DM_BTN_Y : 0);
    { static uint32_t last; uint32_t id = worn ? ci.id : 0; if (id != last) { last = id; if (id) LOG("looks: controller on worn item %u", id); } }
    if (!apply) dm_padmode_page_drop(DM_BTN_X);
    if (!mask) { pad_legend(b, lk, 0, 0, 0); return; }
    uint64_t ev = dm_padmode_page_keys(mask);

    static dm_looks_apply_gate gate;
    uint64_t choice = (uint64_t)g_pick.id | (uint64_t)g_pick.kind << 32 | (uint64_t)(g_picked != 0) << 40 | (uint64_t)(uint8_t)(g_dpick + 1) << 48 | (uint64_t)(uint8_t)g_body << 56;
    int pay = dm_looks_apply_gate_step(&gate, choice, apply, (ev & DM_BTN_X) != 0);
    if (pay < 0) dm_padmode_page_drop(DM_BTN_X);
    else if (pay) dm_looks_ui_on_uimsg("PanelManager", "ClosePanel", "TouchLooksApply");
    pad_legend(b, lk, fk || worn, apply, paged);
    if ((ev & DM_BTN_A) && fk && tile_rect(fk, fi, r)) dm_looks_ui_tap(to_px(r[0] + r[2] / 2), to_py(r[1] + r[3] / 2));
    else if ((ev & DM_BTN_A) && worn) dm_looks_ui_item_tap();
    if (ev & DM_BTN_Y) { g_page = (g_page + 1) % g_pages; end_preview(); LOG("looks: controller page %d", g_page + 1); }
    uint64_t d = ev & dirs; if (!d) return;
    int dx = d & DM_BTN_LEFT ? -1 : d & DM_BTN_RIGHT ? 1 : 0, dy = dx ? 0 : d & DM_BTN_UP ? -1 : 1, k, i;

    if (fk && tile_rect(fk, fi, r)) { fx = r[0] + r[2] / 2; fy = r[1] + r[3] / 2; }
    if (pad_step(fx, fy, dx, dy, &k, &i) && tile_rect(k, i, r)) {
        dm_ui_hover_now(to_px(r[0] + r[2] / 2), to_py(r[1] + r[3] / 2));
        LOG("looks: controller on %s tile %d", k == 1 ? "look" : "colour", i);
    } else { void dm_padmode_page_pulse(uint64_t); dm_padmode_page_pulse(d); LOG("looks: controller hands %#llx to the game at %d,%d", (unsigned long long)d, fx, fy); }
}

void dm_looks_ui_pump(void) {
    uint8_t *b = lu_base(); uint64_t root = b ? *(uint64_t *)(b + UI_ROOT) : 0;
    if (!root) return;
    uint64_t vp = dm_ui_top(root, "VendorPanelLayout"); if (vp && !*(uint8_t *)(vp + 0x50)) vp = 0;
    uint64_t tabs = vp ? lu_find(vp, "tabs", 1) : 0, lk = vp ? lu_find(vp, "TouchLooks", 1) : 0;
    if (tabs && !dm_widget_is(b, tabs, DM_VT_TABBAR)) tabs = 0;
#ifndef DM_PLAYER_BUILD
    test_pump(b, root, tabs);
#endif
    int avail = tabs && lk && npc_class(b) == CHARSI && dm_looks_unlocked() && dm_looks_state() != DM_LOOKS_STATE_OFF;
    if (tabs) tab_available(tabs, avail);
    int sel = tabs ? *(int32_t *)(tabs + TAB_SEL) : -1;
    if (tabs && !avail && sel == LOOKS_TAB) ((void (*)(uint64_t, int, int))(uintptr_t)(b + FN_TAB_SELECT))(tabs, 0, 0);
    int active = avail && sel == LOOKS_TAB;
    if (active != g_active) { LOG("looks: the appearance tab is %s", active ? "open" : "closed"); end_preview(); g_page = 0; g_body = 0; g_cal.ok = 0; }
    g_active = active;
    if (!active) { if (lk) lu_move(lk, OFF, 0); place_slot_frame(root, 0); return; }
    lu_move(lk, 0, 0);
    uint64_t pl = client_player(b), it = 0;
    if (g_body) it = worn_item(pl, g_body);
    for (unsigned i = 0; !it && i < sizeof k_slots / sizeof k_slots[0]; i++) if ((it = worn_item(pl, k_slots[i].body))) { g_body = k_slots[i].body; g_dirty = 1; end_preview(); }
    if (!it) g_body = 0;
    place_slot_frame(root, g_body);
    if (dm_looks_state() != DM_LOOKS_STATE_OK) { draw_broken(b, lk); return; }
    static uint64_t frames; if (++frames % 30 == 0) g_dirty = 1;
    if (it) { draw(b, lk, pl, it); if (dm_padmode_current()) pad_pump(b, vp, lk); else { lu_move(lu_find(lk, "TouchL_Focus", 0), OFF, 0); pad_legend(b, lk, 0, 0, 0); } }
    else { draw_broken(b, lk); lu_text(b, lu_find(lk, "TouchL_Broken", 0), ""); lu_move(lu_find(lk, "TouchL_Anew", 0), OFF, 0); }
}

static void tile_tapped(int t) {
    uint8_t *b = lu_base(); uint64_t pl = b ? client_player(b) : 0, it = g_body ? worn_item(pl, g_body) : 0; int group, w, h;
    if (!it || !dm_looks_page_item(it, &group, &w, &h)) return;
    int idx = dm_looks_page_index(page_cap(w, h), g_page, t, g_n); if (idx < 0) return;
    dm_look cur, shown; dm_looks_page_state(it, &cur, &shown);
    uint32_t id = *(uint32_t *)(it + 8);
    if (dm_looks_page_busy(it)) return;
    if (same(g_list[idx], cur)) { g_picked = 0; preview_now(it); LOG("looks: the worn look tapped"); return; }
    g_pick = g_list[idx]; g_picked = 1; preview_now(it);
    LOG("looks: preview of look kind %d id %u on unit %u", g_list[idx].kind, g_list[idx].id, id);
}
static void dye_tapped(int t) {
    uint8_t *b = lu_base(); uint64_t pl = b ? client_player(b) : 0, it = g_body ? worn_item(pl, g_body) : 0;
    if (!it || t >= g_dn || dm_looks_page_busy(it)) return;
    int cdye = 0, sdye = 0; dm_looks_page_dye(it, &cdye, &sdye);
    g_dpick = g_dlist[t].dye == cdye ? -1 : g_dlist[t].dye; preview_now(it);
    LOG("looks: preview of colour %d on unit %u", g_dlist[t].dye - 1, *(uint32_t *)(it + 8));
}


int dm_looks_ui_tap(int x, int y) {
    uint8_t *b = lu_base(); uint64_t root = b && g_active ? *(uint64_t *)(b + UI_ROOT) : 0, vp = root ? dm_ui_top(root, "VendorPanelLayout") : 0, lk = vp ? lu_find(vp, "TouchLooks", 1) : 0;
    if (!lk) return 0;
    if (lu_hit(lu_find(lk, "TouchL_PgLHit", 0), x, y)) { g_page--; end_preview(); return 1; }
    if (lu_hit(lu_find(lk, "TouchL_PgRHit", 0), x, y)) { g_page++; end_preview(); return 1; }
    char nm[24];
    for (int t = 0; t < TILES; t++) { snprintf(nm, sizeof nm, "TouchL_T%d", t); uint64_t tile = lu_find(lk, nm, 1);
        if (tile && *(int32_t *)(tile + 0x70) < OFF && lu_hit(lu_find(tile, "Hit", 0), x, y)) { tile_tapped(t); return 1; } }
    for (int t = 0; t < DYE_TILES; t++) { snprintf(nm, sizeof nm, "TouchL_D%d", t); uint64_t tile = lu_find(lk, nm, 1);
        if (tile && *(int32_t *)(tile + 0x70) < OFF && lu_hit(lu_find(tile, "Hit", 0), x, y)) { dye_tapped(t); return 1; } }
    return lu_hit(lu_find(lk, "TouchL_Back", 0), x, y);
}
int dm_looks_ui_over(int x, int y) {
    uint8_t *b = lu_base(); uint64_t root = b && g_active ? *(uint64_t *)(b + UI_ROOT) : 0, vp = root ? dm_ui_top(root, "VendorPanelLayout") : 0, lk = vp ? lu_find(vp, "TouchLooks", 1) : 0;
    return lk && lu_hit(lu_find(lk, "TouchL_Back", 0), x, y);
}

int dm_looks_ui_item_tap(void) {
    dm_cursor_item_info ci; uint8_t *b = lu_base();
    if (!g_active || !b || !dm_cursor_item(&ci) || ci.container != DM_ITEM_AT_EQUIP) return 0;
    uint64_t pl = client_player(b);
    for (unsigned i = 0; i < sizeof k_slots / sizeof k_slots[0]; i++) { uint64_t it = worn_item(pl, k_slots[i].body);
        if (it && *(uint32_t *)(it + 8) == ci.id) { if (g_body != k_slots[i].body) { end_preview(); g_body = k_slots[i].body; g_page = 0; g_dirty = 1; LOG("looks: slot %d chosen", g_body); } return 1; } }
    return 0;
}

int dm_looks_ui_reset(void) { if (!g_active) return 0; end_preview(); LOG("looks: preview reset"); return 1; }

void dm_looks_ui_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (strcmp(cat, "PanelManager") || strcmp(act, "ClosePanel") || !g_active) return;
    if (!strcmp(arg, "TouchLooksAnew")) { LOG("looks: start anew %s", dm_looks_start_anew() ? "failed" : "done"); g_dirty = 1; return; }

    if (strcmp(arg, "TouchLooksApply") || (!g_picked && g_dpick < 0) || (g_picked && sel_index() < 0) || (g_dpick >= 0 && dsel_index() < 0)) return;
    uint8_t *b = lu_base(); uint64_t it = b && g_body ? worn_item(client_player(b), g_body) : 0;
    if (!it) return;
    dm_look cur, shown; dm_looks_page_state(it, &cur, &shown);
    dm_look l = g_picked ? g_pick : cur; int dye = g_dpick;
    dm_looks_request_apply(*(uint32_t *)(it + 8), l, dye);
    LOG("looks: apply look kind %d id %u colour %d to unit %u", l.kind, l.id, dye - 1, *(uint32_t *)(it + 8));
    end_preview();
}

#ifndef DM_PLAYER_BUILD




static void test_pump(uint8_t *b, uint64_t root, uint64_t tabs) {
    static int shop = -1, tab, pick = -1, apply, done_menu, done_tab, done_pick, wait, dye = -1, dye_apply, done_dye;
    if (shop < 0) { shop = getenv("DM_TEST_LOOKS_SHOP") != NULL; tab = getenv("DM_TEST_LOOKS_TAB") != NULL;
        const char *p = getenv("DM_TEST_LOOKS_PICK"); if (p) { pick = atoi(p); apply = strstr(p, ":apply") != NULL; }
        const char *d = getenv("DM_TEST_LOOKS_DYE"); if (d) { dye = atoi(d); dye_apply = strstr(d, ":apply") != NULL; } }
    if (shop && !done_menu) {
        uint64_t pn = dm_ui_top(root, "NpcDialogPanel"), ls = pn && *(uint8_t *)(pn + 0x50) ? *(uint64_t *)(pn + 0x248) : 0;
        if (ls && *(uint64_t *)ls == (uint64_t)(uintptr_t)b + 0x20b8df0) {
            uint64_t ct = *(uint64_t *)(ls + 0xd0); uint64_t n = ct ? *(uint64_t *)(ct + 0x60) : 0;
            if (n >= 2) { *(int32_t *)(ls + 0x140) = 1; dm_ui_key_pulse(DM_BTN_A, 6); done_menu = 1; LOG("[looks] test: NPC %d menu row 1 of %llu", npc_class(b), (unsigned long long)n); }
        }
    }
    if (!tabs) return;
    if (tab && !done_tab && wait++ >= 30 && npc_class(b) == CHARSI && dm_looks_unlocked()) {
        ((void (*)(uint64_t, int, int))(uintptr_t)(b + FN_TAB_SELECT))(tabs, LOOKS_TAB, 0); done_tab = 1; LOG("[looks] test: appearance tab selected"); wait = 0;
    }
    if (pick >= 0 && done_tab && !done_pick && g_active && g_n && wait++ >= 150) {
        done_pick = 1; tile_tapped(pick);
        if (apply) dm_looks_ui_on_uimsg("PanelManager", "ClosePanel", "TouchLooksApply");
    }
    if (dye >= 0 && done_tab && !done_dye && g_active && g_dn > dye && wait++ >= 150) { done_dye = 1; dye_tapped(dye); wait = 0; }
    if (dye_apply && done_dye == 1 && g_active && wait++ >= 150) { done_dye = 2; dm_looks_ui_on_uimsg("PanelManager", "ClosePanel", "TouchLooksApply"); }
    static int last = -2; int sel = *(int32_t *)(tabs + TAB_SEL);
    if (sel != last) { last = sel; LOG("[looks] vendor %d tab %d, unlocked %d", npc_class(b), sel, dm_looks_unlocked()); }
}
#endif
