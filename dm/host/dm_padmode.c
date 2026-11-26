

































#include "hle.h"
#include "game_addrs.h"
#include "dm_hud.h"
#include "dm_cursor.h"
#include "dm_input.h"
#include "dm_padkeys.h"
#include "dm_i18n.h"
#include "dm_modetx.h"
#include "dm_mode_items.h"
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <CommonCrypto/CommonDigest.h>
#include <TargetConditionals.h>

static const struct { uint32_t off; const char *pad, *touch; } k_pfx[] = {
    { GA_STR_UI_CONSOLE_SWITCH, "Controller/Console/Switch/", "0touch_console_switch_lay/" },
    { GA_STR_UI_CONSOLE, "Controller/Console/", "Ztouch_console_lay/" },
    { GA_STR_UI_CTRL_ROOT, "Controller/", "Xtouch_lay/" },
    { GA_STR_UI_CTRL_BUTTON_GLYPH, "Controller/ControllerButtons/%s/%s", "Ztouch_controller_buttons_ui/%s_%s" },
    { GA_STR_UI_CTRL_CURSOR_IDLE, "Controller/Cursor/Idle", "Xtouch_cursor_lay/Idle" },
    { GA_STR_UI_CTRL_CURSOR_HOLD, "Controller/Cursor/Hold", "Xtouch_cursor_lay/Hold" },
    { GA_STR_UI_CTRL_CURSOR_HELD, "Controller/Cursor/Held", "Xtouch_cursor_lay/Held" },
};
enum { BOUNCE_FRAMES = 60 };
static int g_have;
static _Atomic int g_want = -1;
static int g_cur;
static dm_modetx g_tx;
static _Atomic int g_freeze;
static uint32_t g_sig;
static uint64_t g_tx_from;
static _Atomic uint32_t g_rebuilds;
uint32_t dm_padmode_rebuilds(void) { return atomic_load(&g_rebuilds); }






#define GLYPH_OFF GA_STR_UI_CTRL_BUTTON_GLYPH
static const char *const k_glyph[DM_GLYPH_N] = { NULL, "Ztouch_controller_buttons_ui/%sK%s", "Ztouch_controller_buttons_ui/%sX%s", "Ztouch_controller_buttons_ui/%sP%s" };
static const char *const k_glyph_mark[DM_GLYPH_N] = { NULL, "kbd_glyphs", "xbox_glyphs", "ps_glyphs" };
static int g_glyph_have[DM_GLYPH_N] = { 1 }, g_glyph, g_glyph_next;
static _Atomic int g_glyph_want;
static const char *const k_glyph_name[DM_GLYPH_N] = { "Switch", "keyboard", "Xbox", "PlayStation" };
#if TARGET_OS_OSX
static int g_app;
#endif
void dm_padmode_request_glyph(int g) { if (g < 0 || g >= DM_GLYPH_N || !g_glyph_have[g]) g = DM_GLYPH_PAD; atomic_store(&g_glyph_want, g); }
int dm_padmode_kbd(void) { return g_glyph == DM_GLYPH_KBD; }




static int glyph_swap_ok(uint8_t *b) {
    int dm_ingame_now(void), dm_ui_panel_state(void), dm_ui_fe_drag_ok(void);
    if (!dm_ingame_now()) return dm_ui_fe_drag_ok();
    return !(*(volatile uint8_t *)(b + GA_PANEL_PAUSE_MENU) | *(volatile uint8_t *)(b + GA_PANEL_PAUSE_2)) && !dm_ui_panel_state();
}
static int pfx_write(uint8_t *b, int pad) {
    for (unsigned i = 0; i < sizeof k_pfx / sizeof *k_pfx; i++) { const char *p = (const char *)(b + k_pfx[i].off);
        if (k_pfx[i].off == GLYPH_OFF) { int g = 1; while (g < DM_GLYPH_N && strcmp(p, k_glyph[g])) g++; if (g < DM_GLYPH_N) continue; }
        if (strcmp(p, k_pfx[i].pad) && strcmp(p, k_pfx[i].touch)) { LOG("【红】手柄 / 触屏布局：main+%#x 不是预期的「%s」（是「%.32s」），不改", k_pfx[i].off, k_pfx[i].pad, p); return 0; } }
    for (unsigned i = 0; i < sizeof k_pfx / sizeof *k_pfx; i++) memcpy(b + k_pfx[i].off, pad ? k_pfx[i].pad : k_pfx[i].touch, strlen(k_pfx[i].pad));
    _Static_assert(sizeof "Ztouch_controller_buttons_ui/%sK%s" == sizeof "Controller/ControllerButtons/%s/%s", "换的键帽路径要与原版路径等长");
    if (pad && g_glyph_next) memcpy(b + GLYPH_OFF, k_glyph[g_glyph_next], strlen(k_glyph[g_glyph_next]));
    return 1;
}
enum { NSTR = 192 };
static struct { char *key, *val; size_t klen; char sha[9]; uint64_t node, orig, olen; } g_str[NSTR];
static int g_nstr;
static uint8_t *g_b;
static uint64_t rd8(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static void unesc(char *s) { char *o = s; for (; *s; s++) { if (*s == '\\' && s[1]) { s++; *o++ = *s == 'n' ? '\n' : *s == 't' ? '\t' : *s; } else *o++ = *s; } *o = 0; }
static void str_load(void) {
    char p[1100], line[1024]; FILE *f = dm_overlay_file("touch_strings.tsv", p, sizeof p) ? fopen(p, "r") : NULL; if (!f) { LOG("【红】手柄 / 触屏布局：覆盖层有 ctrl_touch 却没有 touch_strings.tsv，字符串不换"); return; }
    while (g_nstr < NSTR && fgets(line, sizeof line, f)) { line[strcspn(line, "\r\n")] = 0; char *a = strchr(line, '\t'), *c = a ? strchr(a + 1, '\t') : NULL; if (!c || c - a - 1 != 8) continue;
        *a = 0; *c = 0; unesc(c + 1); g_str[g_nstr].key = strdup(line); g_str[g_nstr].klen = strlen(line); memcpy(g_str[g_nstr].sha, a + 1, 8); g_str[g_nstr].val = strdup(c + 1); g_nstr++; }
    if (g_nstr == NSTR && fgets(line, sizeof line, f)) LOG("【红】手柄 / 触屏布局：touch_strings.tsv 超过 %d 条，后面的没读（调大 NSTR）", NSTR);
    fclose(f); LOG("手柄 / 触屏布局：触摸说法 %d 条（touch_strings.tsv）", g_nstr);
}
static uint64_t str_node(uint64_t map, const char *key, size_t n) {
    uint64_t cnt = rd8(map), bk = rd8(map + 8); if (!cnt || !bk || (cnt & (cnt - 1))) return 0;
    uint64_t h[4] = {0}; ((void (*)(uint64_t *, const char *, uint64_t, int))(g_b + GA_STRING_HASH))(h, key, n, 0);
    for (uint64_t nd = rd8(bk + 8 * (h[0] & (cnt - 1))); nd; nd = rd8(nd)) if (rd8(nd + 0x10) == n && !memcmp((const void *)(uintptr_t)rd8(nd + 8), key, n)) return nd;
    return 0;
}
static void str_apply(int pad) {
    static int warned; int nset = 0, nbad = 0;
    for (int i = 0; i < g_nstr; i++) { uint64_t nd = str_node((uint64_t)(uintptr_t)(g_b + GA_STRINGS_CUR_LANG), g_str[i].key, g_str[i].klen); if (!nd) { nbad++; continue; }
        uint64_t *v = (uint64_t *)(uintptr_t)(nd + 0x20);
        if (nd != g_str[i].node) {
            uint64_t ov = v[0], ol = v[1]; if (ov == (uint64_t)(uintptr_t)g_str[i].val) continue;
            unsigned char md[CC_SHA1_DIGEST_LENGTH]; char hx[9]; CC_SHA1((const void *)(uintptr_t)ov, (CC_LONG)ol, md); snprintf(hx, sizeof hx, "%02x%02x%02x%02x", md[0], md[1], md[2], md[3]);
            g_str[i].node = nd; g_str[i].orig = memcmp(hx, g_str[i].sha, 8) ? 0 : ov; g_str[i].olen = ol; if (!g_str[i].orig) { nbad++; continue; } }
        if (!g_str[i].orig) { nbad++; continue; }
        uint64_t want = pad ? g_str[i].orig : (uint64_t)(uintptr_t)g_str[i].val, wl = pad ? g_str[i].olen : strlen(g_str[i].val);
        if (v[0] != want) { v[1] = wl; v[0] = want; nset++; } }
    if (nset) LOG("手柄 / 触屏布局：串表 %d 条换成%s说法（%d 条没换：不是这一种语言 / 原文对不上 / 键不在）", nset, pad ? "手柄" : "触摸", nbad);
    else if (nbad == g_nstr && g_nstr && !warned) { warned = 1; LOG("手柄 / 触屏布局：串表里 %d 条都没换（语言不在表里或原文对不上），照原版说法", nbad); }
}










static void notice_input_obs(const uint64_t *regs) {
    if (!g_have || g_cur) return; uint64_t msg = regs[1]; if (!msg) return;
    if (*(uint64_t *)msg != 0x4ae067c6248b6042ull || *(uint64_t *)(msg + 8) != 0x1e439768f7dd4594ull) return;
    uint64_t pay = *(uint64_t *)(msg + 0x10); if (!pay || *(int32_t *)(pay + 0x10) != 9) return;
    static uint64_t m2[8], p2[8]; memcpy(m2, (void *)msg, sizeof m2); memcpy(p2, (void *)pay, sizeof p2); *(int32_t *)((uint8_t *)p2 + 0x10) = -1; m2[2] = (uint64_t)(uintptr_t)p2;
    ((uint64_t *)regs)[1] = (uint64_t)(uintptr_t)m2;
    static int n; if (n++ < 20) LOG("手柄 / 触屏布局：系统提示框在屏上时按了功能键 → 触屏模式不让提示框吃掉这一下");
}
void dm_padmode_boot(void) {
    if (gate_obs_present(OBS_NOTICE_INPUT)) gate_obs_set(OBS_NOTICE_INPUT, notice_input_obs);
    module_t *m = NULL; for (int i = 0; i < g_nmod; ++i) if (!strcmp(g_mod[i].name, "main")) m = &g_mod[i]; char mk[1100];
    if (!m || !getenv("DM_ROMFS_OVERLAY")) return;
    if (!dm_overlay_file("ctrl_touch", mk, sizeof mk)) { LOG("手柄 / 触屏布局：覆盖层没有 ctrl_touch 标记，照原版布局"); return; }
    int pad = getenv("DM_APP") != NULL;
#if TARGET_OS_OSX
    g_app = pad;
#endif
    g_glyph_have[DM_GLYPH_KBD] = pad && dm_overlay_file(k_glyph_mark[DM_GLYPH_KBD], mk, sizeof mk);
    g_glyph_have[DM_GLYPH_XBOX] = dm_overlay_file(k_glyph_mark[DM_GLYPH_XBOX], mk, sizeof mk);
    g_glyph = g_glyph_next = g_glyph_have[DM_GLYPH_KBD] ? DM_GLYPH_KBD : DM_GLYPH_PAD; atomic_store(&g_glyph_want, g_glyph);
    g_glyph_have[DM_GLYPH_PS] = dm_overlay_file(k_glyph_mark[DM_GLYPH_PS], mk, sizeof mk);
    LOG("手柄 / 触屏布局：键帽 键盘 %d、Xbox %d、PlayStation %d", g_glyph_have[DM_GLYPH_KBD], g_glyph_have[DM_GLYPH_XBOX], g_glyph_have[DM_GLYPH_PS]);
    if (pfx_write((uint8_t *)m->base, pad)) { g_have = 1; g_cur = pad; atomic_store(&g_want, pad); g_b = (uint8_t *)m->base; str_load(); LOG("手柄 / 触屏布局：开机用%s", pad ? "原版手柄版（Mac App）" : "触屏版（layouts/xtouch_lay 等三个目录）"); }
}
void dm_padmode_request(int pad) { pad = pad ? 1 : 0;
#if TARGET_OS_OSX
    if (g_app) pad = 1;
#endif
    atomic_store(&g_want, pad); if (g_have && pad != g_cur) atomic_store(&g_freeze, 1); }
int dm_padmode_current(void) { return g_cur; }
int dm_padmode_busy(void) { return atomic_load(&g_freeze); }
int dm_padmode_input_gate(uint64_t *buttons) { static dm_modegate g; return dm_modegate_apply(&g, atomic_load(&g_freeze), buttons); }
static void dm_mode_none(void) {}
#define X(name, side, enter, leave) void enter(void), leave(void);
DM_MODE_ITEMS(X)
#undef X
static const struct { const char *name; int side; void (*enter)(void), (*leave)(void); } k_items[] = {
#define X(name, side, enter, leave) { name, side, enter, leave },
    DM_MODE_ITEMS(X)
#undef X
};
static void items_run(int side, int leave) { for (unsigned i = 0; i < sizeof k_items / sizeof *k_items; i++) { if (k_items[i].side != 2 && k_items[i].side != side) continue; void (*f)(void) = leave ? k_items[i].leave : k_items[i].enter; if (f) f(); } }











static _Atomic uint64_t g_pg_mask, g_pg_ev, g_pg_frame, g_pg_held, g_pg_pulse; static _Atomic int g_pg_pulse_n;




uint64_t dm_padmode_page_keys(uint64_t mask) {
    uint64_t f = dm_guest_frame(); if (atomic_exchange(&g_pg_frame, f) == f) atomic_fetch_or(&g_pg_mask, mask); else atomic_store(&g_pg_mask, mask);
    return atomic_fetch_and(&g_pg_ev, ~mask) & mask;
}
uint64_t dm_padmode_page_held(void) { return atomic_load(&g_pg_held); }
void dm_padmode_page_pulse(uint64_t mask) { atomic_store(&g_pg_pulse, mask); atomic_store(&g_pg_pulse_n, 4); }
void dm_padmode_page_drop(uint64_t mask) { atomic_fetch_and(&g_pg_ev, ~mask); }
uint64_t dm_padmode_page_filter(uint64_t bt) {
    static dm_padkeys k; uint64_t pressed;
    uint64_t m = g_cur && dm_guest_frame() - atomic_load(&g_pg_frame) <= 3 ? atomic_load(&g_pg_mask) : 0;
    uint64_t pulse = 0; { int n = atomic_load(&g_pg_pulse_n); if (n > 0) { atomic_store(&g_pg_pulse_n, n - 1); pulse = atomic_load(&g_pg_pulse); } }
    uint64_t out = dm_padkeys_step(&k, bt, m, &pressed);
    if (m) { atomic_fetch_or(&g_pg_ev, pressed); atomic_store(&g_pg_held, bt & m); } else { atomic_store(&g_pg_ev, 0); atomic_store(&g_pg_held, 0); }
    return out | pulse;
}














enum { HINT_VAULT, HINT_IDENT, NHINT };
static struct { const char *key, *ours_key; _Atomic int on; uint64_t node, orig, olen; char ours[48]; } g_hint[NHINT] = {
    { "ControllerPromptItemMoveToPrivateStash", "dmPadHintDeposit" }, { "ControllerPromptItemPickUpFromInventory", "dmPadLegIdentify" } };
void dm_padmode_vault_hint(int on) { atomic_store(&g_hint[HINT_VAULT].on, on ? 3 : 0); }
void dm_padmode_ident_hint(int on) { atomic_store(&g_hint[HINT_IDENT].on, on ? 3 : 0); }












void dm_padmode_lang_reset(void) {
    int back = 0;
    for (int i = 0; i < g_nstr; i++) { if (g_str[i].node && g_str[i].orig) { uint64_t *v = (uint64_t *)(uintptr_t)(g_str[i].node + 0x20); if (v[0] == (uint64_t)(uintptr_t)g_str[i].val) { v[0] = g_str[i].orig; v[1] = g_str[i].olen; back++; } }
        g_str[i].node = g_str[i].orig = g_str[i].olen = 0; }
    for (int h = 0; h < NHINT; h++) { if (g_hint[h].node) { uint64_t *v = (uint64_t *)(uintptr_t)(g_hint[h].node + 0x20); if (v[0] == (uint64_t)(uintptr_t)g_hint[h].ours) { v[0] = g_hint[h].orig; v[1] = g_hint[h].olen; back++; } }
        g_hint[h].node = g_hint[h].orig = g_hint[h].olen = 0; }
    if (g_have) LOG("手柄 / 触屏布局：串表要重装（切语言）→ %d 条写回原文，记的节点清空，重装后重新认", back);
}
static void hint_pump(void) {
    for (int h = 0; h < NHINT; h++) { uint64_t *nodep = &g_hint[h].node; char *ours = g_hint[h].ours;



        { const char *w = dm_str(g_b, g_hint[h].ours_key); if (w && strcmp(ours, w)) { if (*nodep && ((uint64_t *)(uintptr_t)(*nodep + 0x20))[0] == (uint64_t)(uintptr_t)ours) ((uint64_t *)(uintptr_t)(*nodep + 0x20))[1] = strlen(w); strlcpy(ours, w, sizeof g_hint[h].ours); } }
        int on = g_cur && atomic_load(&g_hint[h].on) > 0; if (atomic_load(&g_hint[h].on) > 0) atomic_fetch_sub(&g_hint[h].on, 1);
        uint64_t nd = str_node((uint64_t)(uintptr_t)(g_b + GA_STRINGS_CUR_LANG), g_hint[h].key, strlen(g_hint[h].key)); if (!nd) continue;
        uint64_t *v = (uint64_t *)(uintptr_t)(nd + 0x20);
        if (nd != *nodep) { if (v[0] == (uint64_t)(uintptr_t)ours) continue; *nodep = nd; g_hint[h].orig = v[0]; g_hint[h].olen = v[1]; }
        uint64_t want = on ? (uint64_t)(uintptr_t)ours : g_hint[h].orig, wl = on ? strlen(ours) : g_hint[h].olen;
        if (v[0] != want) { v[1] = wl; v[0] = want; LOG("手柄提示：%s → %s", g_hint[h].key, on ? ours : "原文"); } }
}






static _Atomic int g_pause_open, g_diff_pend;
static void legend_clear(uint8_t *b);
void dm_padmode_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (!cat || !act || !arg || strcmp(cat, "PanelManager")) return;
    if (!strcmp(act, "ClosePanel") && !strcmp(arg, "BankExpansionLayout")) legend_clear(g_b);
    if (!strncmp(arg, "PauseLayout", 11)) { if (!strcmp(act, "OpenPanel")) { atomic_store(&g_pause_open, 1); atomic_store(&g_diff_pend, 0); } else if (!strcmp(act, "ClosePanel")) atomic_store(&g_pause_open, 0); return; }
    if (strcmp(act, "ClosePanel") || strcmp(arg, "TouchPadDiff")) return;
    dm_hud_info h; dm_hud_get(&h); int cur = atomic_load(&g_diff_pend); if (!cur) cur = h.players;
    if (!cur) { LOG("【红】手柄暂停菜单：游戏难度没读到当前档位，不动"); return; }
    int to = cur % 8 + 1; dm_hud_request_players(to); atomic_store(&g_diff_pend, to); LOG("手柄暂停菜单：游戏难度 %d → %d", cur, to);
}
static uint64_t pm_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 6) return 0; const char *nm = (const char *)rd8(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = rd8(w + 0x58), n = rd8(w + 0x60); for (uint64_t i = 0; ch && i < n && i < 256; i++) { uint64_t r = pm_find(rd8(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}























enum { LG_MAX = 6, LG_CAP = 8 };
static struct { int act; char text[48]; } g_lg[LG_MAX], g_lg_on[LG_MAX]; static int g_lg_n, g_lg_on_n; static uint64_t g_lg_frame;
static dm_wref g_lg_widget;
static struct {
    uint8_t *base;
    void (*make)(void *, int, int, void *);
    void (*add)(uint64_t, void *);
    void (*remove)(uint64_t, int);
    void (*append)(uint64_t, uint64_t);
} g_lg_api;
static void legend_bind(uint8_t *b) {
    if (g_lg_api.base == b) return;
    g_lg_api.base = b;
    g_lg_api.make = (void (*)(void *, int, int, void *))(b + GA_LEGEND_ENTRY_MAKE);
    g_lg_api.add = (void (*)(uint64_t, void *))(b + GA_LEGEND_ADD);
    g_lg_api.remove = (void (*)(uint64_t, int))(b + GA_LEGEND_REMOVE);
    g_lg_api.append = (void (*)(uint64_t, uint64_t))(b + GA_LEGEND_APPEND);
}
void dm_padmode_legend(int act, const char *text) {
    uint64_t f = dm_guest_frame(); if (f != g_lg_frame) { g_lg_frame = f; g_lg_n = 0; }
    if (g_lg_n < LG_MAX) { g_lg[g_lg_n].act = act; strlcpy(g_lg[g_lg_n].text, text, sizeof g_lg[0].text); g_lg_n++; }
}






static struct { const char *legend; int act; } g_lh[4]; static int g_lh_n; static uint64_t g_lh_frame;

static int g_ld[4], g_ld_n; static uint64_t g_ld_frame;
void dm_padmode_legend_drop(int act) { uint64_t f = dm_guest_frame(); if (f != g_ld_frame) { g_ld_frame = f; g_ld_n = 0; } if (g_ld_n < 4) g_ld[g_ld_n++] = act; }
void dm_padmode_legend_hide(const char *legend, int act) { uint64_t f = dm_guest_frame(); if (f != g_lh_frame) { g_lh_frame = f; g_lh_n = 0; } if (g_lh_n < 4) { g_lh[g_lh_n].legend = legend; g_lh[g_lh_n].act = act; g_lh_n++; } }
static uint64_t legend_obj(uint8_t *b, int *visible) {
    *visible = 0; if (!b) return 0;
    uint64_t root = rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)), ch = root ? rd8(root + 0x58) : 0, n = root ? rd8(root + 0x60) : 0;
    for (uint64_t i = 0; ch && i < n && i < 256; i++) { uint64_t w = rd8(ch + 8 * i); const char *nm = w ? (const char *)(uintptr_t)rd8(w + 8) : NULL;
        if (nm && !strcmp(nm, "ControllerOverlay")) { *visible = *(uint8_t *)(uintptr_t)(w + 0x50) != 0; return rd8(w + 0x440); } }
    return 0;
}
static void legend_free(uint8_t *b, uint64_t *ent) {
    if (!(ent[4] >> 63) && (*(volatile uint8_t *)(b + GA_GAME_ALLOC_READY) & 1)) { uint64_t al = rd8((uint64_t)(uintptr_t)(b + GA_GAME_ALLOC)); ((void (*)(uint64_t, uint64_t))rd8(rd8(al) + 0x28))(al, ent[2]); }
}
static uint64_t legend_find(uint8_t *b, uint64_t lg, int act) {
    uint64_t ent[8] = { 0 }; struct { const char *p; uint64_t n; } sv = { "", 0 }; g_lg_api.make(ent, act, 0, &sv); legend_free(b, ent);
    if ((int32_t)ent[1] != 1) return 0;
    uint64_t arr = rd8(lg + 0x88), n = rd8(lg + 0x90); for (uint64_t i = 0; arr && i < n && i < 32; i++) { uint64_t e = arr + 0x40 * i; if (*(int32_t *)(uintptr_t)(e + 8) == 1 && *(uint32_t *)(uintptr_t)e == (uint32_t)ent[0]) return e; }
    return 0;
}
static int legend_ours(uint64_t e, const char *text) { size_t n = strlen(text); return rd8(e + 0x18) == n && !memcmp((const void *)(uintptr_t)rd8(e + 0x10), text, n); }








static uint64_t legend_at(uint8_t *b, uint64_t w, uint64_t xy, int dep) {
    if (!w || dep > 8 || !*(uint8_t *)(uintptr_t)(w + 0x50) || !*(uint8_t *)(uintptr_t)(w + 0x51)) return 0;
    uint64_t vt = rd8(w) - (uint64_t)(uintptr_t)b; if (vt == GA_VT_LEGEND_BAR_GAME || vt == GA_VT_LEGEND_BAR_FRONT) return                                                                                                                                                                                                                            rd8(w + 0x90) && (((int (*)(uint64_t, uint64_t))rd8(rd8(w) + 0x48))(w, xy) & 1) ? w : 0;
    uint64_t ch = rd8(w + 0x58), n = rd8(w + 0x60); for (uint64_t i = n < 256 ? n : 0; ch && i-- > 0; ) { uint64_t r = legend_at(b, rd8(ch + 8 * i), xy, dep + 1); if (r) return r; }
    return 0;
}


static uint64_t legend_top(uint8_t *b, uint64_t w, int dep) {
    if (!w || dep > 8 || !*(uint8_t *)(uintptr_t)(w + 0x50) || !*(uint8_t *)(uintptr_t)(w + 0x51)) return 0;
    uint64_t vt = rd8(w) - (uint64_t)(uintptr_t)b; if (vt == GA_VT_LEGEND_BAR_GAME || vt == GA_VT_LEGEND_BAR_FRONT) return rd8(w + 0x88) && rd8(w + 0x90) && rd8(w + 0x90) <= 12 ? w : 0;
    uint64_t ch = rd8(w + 0x58), n = rd8(w + 0x60); for (uint64_t i = n < 256 ? n : 0; ch && i-- > 0; ) { uint64_t r = legend_top(b, rd8(ch + 8 * i), dep + 1); if (r) return r; }
    return 0;
}
int dm_padmode_legend_has(uint32_t glyph) {
    extern module_t g_mod[]; extern int g_nmod; uint8_t *b = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    uint64_t root = b ? rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)) : 0, lg = root ? legend_top(b, root, 0) : 0; if (!lg) return 0;
    { const char *nm = (const char *)(uintptr_t)rd8(lg + 8); if (glyph == 0xe005 && nm && !strcmp(nm, "MainMenuLegend")) return 0; }
    uint64_t arr = rd8(lg + 0x88), n = rd8(lg + 0x90); for (uint64_t i = 0; i < n; i++) if (*(int32_t *)(uintptr_t)(arr + 0x40 * i + 8) == 1 && *(uint32_t *)(uintptr_t)(arr + 0x40 * i) == glyph) return 1;
    return 0;
}
uint64_t dm_padmode_legend_click(float nx, float ny) {
    extern module_t g_mod[]; extern int g_nmod; uint8_t *b = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    if (!b || !g_cur) return 0; int W = *(int32_t *)(b + GA_UI_WIDTH), H = *(int32_t *)(b + GA_UI_HEIGHT); uint64_t root = rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)); if (!root || W <= 0 || H <= 0) return 0;
    int x = (int)(nx * W), y = (int)(ny * H); uint64_t lg = legend_at(b, root, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32), 0); if (!lg) return 0;
    uint64_t arr = rd8(lg + 0x88), n = rd8(lg + 0x90); if (!arr || n > 12) return 0;
    float em = H * 0.0281f, gap = 0.67f * em, wd[12], tot = 0;
    for (uint64_t i = 0; i < n; i++) { const uint8_t *t = (const uint8_t *)(uintptr_t)rd8(arr + 0x40 * i + 0x10); uint64_t tn = rd8(arr + 0x40 * i + 0x18); float tw = 0;
        for (uint64_t k = 0; t && k < tn && k < 96; k++) { if ((t[k] & 0xc0) == 0x80) continue; tw += t[k] < 0x80 ? (t[k] == ' ' ? 0.3f : 0.55f) : 1.0f; }
        wd[i] = (1.3f + tw) * em; tot += wd[i] + (i ? gap : 0); }
    float x0 = W / 2.0f - tot / 2; int hit = -1; for (uint64_t i = 0; i < n; i++) { if (x >= x0 - gap / 2 && x < x0 + wd[i] + gap / 2) hit = (int)i; x0 += wd[i] + gap; }
    if (hit < 0) return 0;
    uint64_t e = arr + 0x40 * (uint64_t)hit; uint32_t gl = *(uint32_t *)(uintptr_t)e; uint64_t btn = 0;
    static const struct { int act; uint64_t btn; } km[] = { { 0, DM_BTN_A }, { 1, DM_BTN_Y }, { 2, DM_BTN_X }, { 4, DM_BTN_RS }, { 7, DM_BTN_ZL }, { 8, DM_BTN_ZR }, { 10, DM_BTN_MINUS }, { 11, DM_BTN_LEFT }, { 14, DM_BTN_RIGHT }, { 12, DM_BTN_UP }, { 13, DM_BTN_DOWN }, { 19, DM_BTN_L }, { 20, DM_BTN_R }, { 23, DM_BTN_B } };
    if (*(int32_t *)(uintptr_t)(e + 8) == 1) for (unsigned k = 0; k < sizeof km / sizeof *km && !btn; k++) {
        uint64_t ent[8] = { 0 }; struct { const char *p; uint64_t n; } sv = { "", 0 }; ((void (*)(void *, int, int, void *))(b + GA_LEGEND_ENTRY_MAKE))(ent, km[k].act, 0, &sv); legend_free(b, ent);
        if ((int32_t)ent[1] == 1 && (uint32_t)ent[0] == gl) btn = km[k].btn; }
    if (!btn && gl == 0xe008) btn = DM_BTN_LS;
    if (!btn && gl == 0xe005) btn = DM_BTN_PLUS;
    LOG("手柄提示条：鼠标点在第 %d 条（共 %llu 条，键帽 %x）→ %s", hit + 1, (unsigned long long)n, gl, btn ? "按它那个键" : "认不出是哪个键，不按");
    return btn;
}






int dm_padmode_hud_click(float nx, float ny, uint64_t *btn) {
    extern module_t g_mod[]; extern int g_nmod; uint8_t *b = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    if (!b || !g_cur) return 0; int W = *(int32_t *)(b + GA_UI_WIDTH), H = *(int32_t *)(b + GA_UI_HEIGHT); uint64_t root = rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)); if (!root || W <= 0 || H <= 0) return 0;
    if (*(volatile uint8_t *)(b + GA_PANEL_PAUSE_MENU) | *(volatile uint8_t *)(b + GA_PANEL_PAUSE_2)) return 0;
    uint64_t hud = pm_find(root, "HUDPanel", 5); if (!hud || !*(uint8_t *)(uintptr_t)(hud + 0x50) || !*(uint8_t *)(uintptr_t)(hud + 0x51)) return 0;
    uint64_t xy = (uint64_t)(uint32_t)(nx * W) | ((uint64_t)(uint32_t)(ny * H) << 32), ch = rd8(hud + 0x58), n = rd8(hud + 0x60);
    static const struct { const char *nm; uint64_t btn; } km[] = { { "SkillButton1", DM_BTN_A }, { "SkillButton2", DM_BTN_B }, { "SkillButton3", DM_BTN_Y }, { "SkillButton4", DM_BTN_X }, { "SkillButton5", DM_BTN_R }, { "SkillButton6", DM_BTN_ZR },
        { "LevelUpButton", DM_BTN_PLUS }, { "RunButton", DM_BTN_LS }, { "RunButtonPrompt", DM_BTN_LS }, { "HealthBall", 0 }, { "ManaBall", 0 }, { "ExpBar", 0 }, { "StaminaBar", 0 }, { "SkillSwapIndicator", 0 }, { "LTButtonPrompt", 0 } };
    static const uint64_t belt[4] = { DM_BTN_LEFT, DM_BTN_UP, DM_BTN_DOWN, DM_BTN_RIGHT };
    #define HUD_HIT(w, q) (((int (*)(uint64_t, uint64_t))rd8(rd8(w) + 0x48))((w), (q)) & 1)
    int px = (int)(nx * W), py = (int)(ny * H);
    for (int pass = 0; pass < 2; pass++)
        for (uint64_t i = n < 64 ? n : 0; ch && i-- > 0; ) { uint64_t w = rd8(ch + 8 * i); if (!w || !*(uint8_t *)(uintptr_t)(w + 0x50)) continue; const char *nm = (const char *)(uintptr_t)rd8(w + 8); if (!nm) continue;
            if (!pass && !strcmp(nm, "Belt") && HUD_HIT(w, xy)) {
                int l = px, r = px; while (l > 0 && HUD_HIT(w, (uint64_t)(uint32_t)(l - 2) | ((uint64_t)(uint32_t)py << 32))) l -= 2; while (r < W - 1 && HUD_HIT(w, (uint64_t)(uint32_t)(r + 2) | ((uint64_t)(uint32_t)py << 32))) r += 2;
                int k = r > l ? (px - l) * 4 / (r - l + 1) : 0; if (k < 0) k = 0; if (k > 3) k = 3; *btn = belt[k]; LOG("手柄 HUD：鼠标点在腰带第 %d 格（腰带 x %d–%d）→ 按十字键 %s", k + 1, l, r, k == 0 ? "LEFT" : k == 1 ? "UP" : k == 2 ? "DOWN" : "RIGHT"); return 1; }
            for (unsigned k = 0; k < sizeof km / sizeof *km; k++) if ((km[k].btn != 0) == !pass && !strcmp(nm, km[k].nm) && HUD_HIT(w, xy)) {
                *btn = km[k].btn; uint64_t kb = km[k].btn; const char *kn = kb == DM_BTN_A ? "A" : kb == DM_BTN_B ? "B" : kb == DM_BTN_Y ? "Y" : kb == DM_BTN_X ? "X" : kb == DM_BTN_R ? "R" : kb == DM_BTN_ZR ? "ZR" : kb == DM_BTN_PLUS ? "PLUS" : kb == DM_BTN_LS ? "LS" : "";
                if (kb) LOG("手柄 HUD：鼠标点在 %s → 按 %s", nm, kn); else LOG("手柄 HUD：鼠标点在 %s → 没有对应的键，这一下不处理", nm); return kb ? 1 : 2; } }
    #undef HUD_HIT
    return 0;
}
static void legend_forget(int k) {
    memmove(g_lg_on + k, g_lg_on + k + 1, sizeof g_lg_on[0] * (size_t)(--g_lg_on_n - k));
}
static int legend_remove_owned(uint8_t *b, uint64_t lg, int k) {
    uint64_t e = legend_find(b, lg, g_lg_on[k].act); int ours = e && legend_ours(e, g_lg_on[k].text), act = g_lg_on[k].act;
    legend_forget(k);
    if (ours) g_lg_api.remove(lg, act);
    return ours;
}
static void legend_clear(uint8_t *b) {
    int visible; uint64_t lg = legend_obj(b, &visible);
    if (b) legend_bind(b);


    if (lg && lg == dm_wref_get(&g_lg_widget)) while (g_lg_on_n) legend_remove_owned(b, lg, g_lg_on_n - 1);
    g_lg_on_n = g_lg_n = 0; g_lg_frame = 0; dm_wref_set(&g_lg_widget, lg);
}


void svc_legend_append(uint64_t *regs) {
    uint8_t *b = g_b;
    if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) { b = (uint8_t *)g_mod[i].base; break; }
    if (!b) die("提示条追加门找不到 main");
    legend_bind(b);
    static int announced; if (!announced) { announced = 1; LOG("手柄提示条：追加容量保护已启用（最多 %u 条）", LG_CAP); }
    uint64_t lg = regs[19];


    int evicted = 0;
    while (rd8(lg + 0x90) >= LG_CAP && lg == dm_wref_get(&g_lg_widget) && g_lg_on_n) evicted += legend_remove_owned(b, lg, g_lg_on_n - 1);
    if (evicted) { static unsigned logged; if (logged++ < 8) LOG("手柄提示条：撤 %d 条自定义提示，为原版新增操作留位", evicted); }
    if (rd8(lg + 0x90) >= LG_CAP) {
        static unsigned logged; if (logged++ < 8) LOG("手柄提示条：已满 %u 条，暂缓新增提示", LG_CAP);
        return;
    }
    g_lg_api.append(regs[0], regs[1]);
}
static void legend_pump(uint8_t *b, uint64_t frame) {
    static int miss, tot; static uint64_t logged; int test = getenv("DM_TEST_LEGEND") != NULL;



    if (test) { static int n = -1; static struct { int a; char t[24]; } tv[LG_MAX]; if (n < 0) { n = 0; const char *e = getenv("DM_TEST_LEGEND"); for (const char *q = e; q && *q && n < LG_MAX; ) { int a; char t[24]; if (sscanf(q, "%d:%23[^,]", &a, t) != 2) break; tv[n].a = a; strlcpy(tv[n].t, t, sizeof tv[n].t); n++; q = strchr(q, ','); if (q) q++; } }
      static long off = -1; if (off < 0) { const char *e = getenv("DM_TEST_LEGEND_OFF"); off = e ? atol(e) : 0; }
      if (!off || frame < (uint64_t)off) for (int i = 0; i < n; i++) dm_padmode_legend(tv[i].a, tv[i].t); }
    legend_bind(b);
    if (g_cur && frame - g_lh_frame <= 2) for (int i = 0; i < g_lh_n; i++) { uint64_t w = pm_find(rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)), g_lh[i].legend, 0);
        if (w && legend_find(b, w, g_lh[i].act)) { g_lg_api.remove(w, g_lh[i].act); static int lgd; if (lgd++ < 8) LOG("手柄提示条：%s 上原版的动作 %d 那一条拿掉（这一页把这个键收掉了）", g_lh[i].legend, g_lh[i].act); } }
    int visible; uint64_t lg = legend_obj(b, &visible);
    if (lg != dm_wref_get(&g_lg_widget)) { g_lg_on_n = 0; dm_wref_set(&g_lg_widget, lg); }
    if (!lg) return;
    if (!g_cur || !visible || dm_padmode_busy()) { legend_clear(b); return; }
    if (frame - g_ld_frame <= 2) for (int i = 0; i < g_ld_n; i++) { uint64_t e = legend_find(b, lg, g_ld[i]); int mine = 0;
        for (int k = 0; k < g_lg_on_n; k++) mine |= g_lg_on[k].act == g_ld[i] && e && legend_ours(e, g_lg_on[k].text);
        if (e && !mine) g_lg_api.remove(lg, g_ld[i]); }
    int live = frame - g_lg_frame <= 2 ? g_lg_n : 0;
    for (int k = g_lg_on_n - 1; k >= 0; k--) { int keep = 0; for (int i = 0; i < live; i++) keep |= g_lg[i].act == g_lg_on[k].act;
        if (!keep) legend_remove_owned(b, lg, k); }
    for (int i = 0; i < live; i++) { tot++; uint64_t e = legend_find(b, lg, g_lg[i].act);


        if (!e && rd8(lg + 0x90) >= LG_CAP) continue;
        if (!(e && legend_ours(e, g_lg[i].text))) { miss++;
            uint64_t ent[8] = { 0 }; struct { const char *p; uint64_t n; } sv = { g_lg[i].text, strlen(g_lg[i].text) };
            g_lg_api.make(ent, g_lg[i].act, 0, &sv); g_lg_api.add(lg, ent); legend_free(b, ent); }
        int k = 0; while (k < g_lg_on_n && g_lg_on[k].act != g_lg[i].act) k++;
        if (k < LG_MAX) { g_lg_on[k] = g_lg[i]; if (k == g_lg_on_n) g_lg_on_n++; }
    }
    if ((test || getenv("DM_TEST_LEGEND_LOG")) && frame - logged >= 120) { logged = frame; uint64_t arr = rd8(lg + 0x88), n = rd8(lg + 0x90); char o[300]; size_t L = 0; o[0] = 0;
        for (uint64_t i = 0; arr && i < n && i < 10; i++) L += (size_t)snprintf(o + L, L < sizeof o ? sizeof o - L : 0, " %x", *(uint32_t *)(uintptr_t)(arr + 0x40 * i));
        LOG("手柄提示条：帧 %llu，近 120 帧声明 %d 次、补加 %d 次；提示条现有 %llu 条，字形%s", (unsigned long long)frame, tot, miss, (unsigned long long)n, o); tot = miss = 0; }
}
static void diff_text(uint8_t *b, uint64_t frame) {
    if (!g_cur || !atomic_load(&g_pause_open) || frame % 6) return;
    uint64_t w = pm_find(rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)), "TouchPadDiffTxt", 0); if (!w || rd8(w) - (uint64_t)(uintptr_t)b != GA_VT_TEXTBOX) return;
    dm_hud_info h; dm_hud_get(&h); int n = atomic_load(&g_diff_pend); if (!n) n = h.players; if (!n) return;
    char t[64]; snprintf(t, sizeof t, dm_str(b, "dmPadDiffLine"), n); size_t L = strlen(t);
    if (rd8(w + 0x90) == L && !memcmp((const void *)(uintptr_t)rd8(w + 0x88), t, L)) return;
    ((void (*)(uint64_t, const char *, uint64_t))(b + GA_TEXT_SET))(w + 0x88, t, L); ((void (*)(uint64_t))(b + GA_TEXT_LAYOUT))(w);
}






static _Atomic uint64_t g_test_cur;
void dm_padmode_test_cursor(float nx, float ny) { nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny; atomic_store(&g_test_cur, 1ull << 63 | (uint64_t)(uint32_t)(nx * 65535 + .5f) << 16 | (uint32_t)(ny * 65535 + .5f)); }
void dm_padmode_step(uint8_t *b, uint64_t frame) {
#ifndef DM_PLAYER_BUILD
    { static long at = -1; if (at < 0) { const char *e = getenv("DM_TEST_PADMODE_LANG_RESET"); at = e ? atol(e) : 0; }
      if (at > 0 && frame >= (uint64_t)at) { at = 0; dm_padmode_lang_reset(); } }
#endif
    { uint64_t tc = atomic_exchange(&g_test_cur, 0); if (tc >> 63) { int dm_ui_hover_now(int, int); int W = *(int32_t *)(b + GA_UI_WIDTH), H = *(int32_t *)(b + GA_UI_HEIGHT), x = (int)(((tc >> 16) & 0xffff) / 65535.0 * W), y = (int)((tc & 0xffff) / 65535.0 * H);
        int ok = dm_ui_hover_now(x, y); LOG("测试 padcur：手柄光标放到 (%d,%d) → %d", x, y, ok); } }
    diff_text(b, frame);
    legend_pump(b, frame);
    if (g_have) hint_pump();
    void dm_oe_ui_opmode(int m); int dm_opmode(void);



#ifndef DM_PLAYER_BUILD
    { static int n = -1, k; static struct { uint64_t f; int m; } ev[8]; static uint64_t chk = 300;
      if (n < 0) { n = 0; const char *e = getenv("DM_TEST_PADMODE"); for (const char *q = e; q && *q && n < 8; ) { unsigned long long f; int m; if (sscanf(q, "%llu:%d", &f, &m) != 2) break; ev[n].f = f; ev[n].m = m; n++; q = strchr(q, ','); if (q) q++; } }
      if (k < n && frame >= ev[k].f) { dm_padmode_request(ev[k].m); chk = frame + BOUNCE_FRAMES + 5; k++; }
      if (g_have && chk && frame >= chk && (n || getenv("DM_TEST_PADMODE_READ"))) { chk = 0; char o[512]; size_t L = 0; o[0] = 0;
          for (int i = 0; i < g_nstr && i < 3; i++) { struct { const char *s; uint64_t n; } q = { g_str[i].key, g_str[i].klen }; const char *r = ((const char *(*)(void *))(b + GA_LOC_STRING_GET))(&q);
              L += (size_t)snprintf(o + L, L < sizeof o ? sizeof o - L : 0, "%s%s=「%s」", i ? "、" : "", g_str[i].key, r ? r : "(空)"); }
          LOG("DM_TEST_PADMODE：帧 %llu 模式 %s，原版取串 %s", (unsigned long long)frame, g_cur ? "手柄" : "触屏", o); } }
#endif
#if TARGET_OS_OSX


    if (g_glyph_have[DM_GLYPH_KBD] && frame % 10 == 0) { int kb = g_glyph == DM_GLYPH_KBD; uint64_t root = rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)), a = root ? pm_find(root, "TouchBlissKey", 0) : 0, k = a ? pm_find(root, "TouchBlissKeyK", 0) : 0;
        for (int i = 0; i < 2 && a && k; i++) { uint64_t w = i ? k : a; int32_t *r = (int32_t *)(uintptr_t)(w + 0x70), x = (i ? kb : !kb) ? -1665 : 99999; if (r[0] != x) { r[0] = x; ((void (*)(uint64_t))rd8(rd8(w) + 0x10))(w); } } }
#endif

#ifndef DM_PLAYER_BUILD
    { static int n = -1, k; static struct { uint64_t f; int m; } ev[8];
      if (n < 0) { n = 0; const char *e = getenv("DM_TEST_GLYPH"); for (const char *q = e; q && *q && n < 8; ) { unsigned long long f; int m; if (sscanf(q, "%llu:%d", &f, &m) != 2) break; ev[n].f = f; ev[n].m = m; n++; q = strchr(q, ','); if (q) q++; } }
      if (k < n && frame >= ev[k].f) { LOG("DM_TEST_GLYPH：帧 %llu 请求键帽样式 %d", (unsigned long long)frame, ev[k].m); dm_padmode_request_glyph(ev[k].m); k++; } }
#endif
    if (g_have && frame % 30 == 0) str_apply(g_cur);
    if (!g_have) return;
    { uint32_t dm_oe_ui_opmode_reads(void); static uint64_t fp0; static uint32_t nfp; uint64_t root = rd8((uint64_t)(uintptr_t)(b + GA_UI_ROOT)), ch = root ? rd8(root + 0x58) : 0, n = root ? rd8(root + 0x60) : 0, fp = n * 0x9e3779b97f4a7c15ull;
      for (uint64_t i = 0; ch && i < n && i < 256; i++) fp = (fp << 7 | fp >> 57) ^ rd8(ch + 8 * i);
      if (fp != fp0) { fp0 = fp; nfp++; } g_sig = dm_oe_ui_opmode_reads() + nfp; }
    int w = atomic_load(&g_want), wx, cx;
    { int tp = w >= 0 ? w : g_cur, tk = tp ? atomic_load(&g_glyph_want) : 0; if (tk != g_glyph && tp == g_cur && !glyph_swap_ok(b)) tk = g_glyph;
      wx = tp | tk << 1; cx = g_cur | g_glyph << 1; if (wx == cx && w < 0) wx = -1; }
    int act = dm_modetx_step(&g_tx, frame, wx, cx, g_sig);
    if (act == DM_TXA_BEGIN) { g_glyph_next = wx >> 1; w = wx & 1; }
    if (act == DM_TXA_NONE) { if (g_tx.st == DM_TX_IDLE && atomic_load(&g_freeze)) { atomic_store(&g_freeze, 0); if (atomic_load(&g_want) != w) atomic_store(&g_freeze, 1); } return; }
    if (act == DM_TXA_BEGIN) {
        legend_clear(b);
        if (!pfx_write(b, w)) { g_tx.st = DM_TX_IDLE; atomic_store(&g_want, -1); atomic_store(&g_freeze, 0); return; }
        atomic_store(&g_freeze, 1); g_tx_from = frame;
        items_run(g_cur, 1);
        g_cur = w; str_apply(w); dm_oe_ui_opmode(!dm_opmode());
        if (g_glyph != g_glyph_next) LOG("手柄 / 触屏布局：键帽 %s → %s", k_glyph_name[g_glyph], k_glyph_name[g_glyph_next]);
        g_glyph = g_glyph_next;
        LOG("手柄 / 触屏布局：切到%s版——开始（输入冻结，让游戏重载布局）", w ? "原版手柄" : "触屏"); return; }
    if (g_tx.timeouts) LOG("【红】手柄 / 触屏布局：%d 帧内没回读到界面重建（游戏没来读运行模式、界面根下的面板也没换），按超时往下走", DM_MODETX_TIMEOUT);
    if (act == DM_TXA_REPORT_BACK) { dm_oe_ui_opmode(-1); return; }
    atomic_fetch_add(&g_rebuilds, 1); items_run(g_cur, 0);
    if (atomic_load(&g_want) == g_cur) atomic_store(&g_want, -1);
    atomic_store(&g_freeze, 0);
    if (atomic_load(&g_want) >= 0 && atomic_load(&g_want) != g_cur) atomic_store(&g_freeze, 1);
    LOG("手柄 / 触屏布局：切到%s版——完成（%llu 帧，两次重建里 %d 次靠超时）", g_cur ? "原版手柄" : "触屏", (unsigned long long)(frame - g_tx_from), g_tx.timeouts);
    if (g_cur) {
        int dm_swapskill_wset(void); int bank = dm_swapskill_wset() ? 16 : 0; char o[240]; size_t L = 0; o[0] = 0;
        static const struct { uint32_t lg; const char *nm; } face[6] = { { 0x2000, "A" }, { 0x1000, "B" }, { 0x8000, "X" }, { 0x4000, "Y" }, { 0x200, "R" }, { 0x800, "ZR" } };
        for (int f = 0; f < 6; f++) for (int i = 0; i < 6; i++) { uint32_t lg, sl[2], id[2] = { 0xffffffffu, 0xffffffffu }; memcpy(&lg, b + GA_PAD_BUTTON_SLOTS + 12 * i, 4); memcpy(sl, b + GA_PAD_BUTTON_SLOTS + 12 * i + 4, 8); if (lg != face[f].lg) continue;
            for (int k = 0; k < 2; k++) if (sl[k] < 12) { memcpy(&id[k], b + GA_SKILL_SLOT_BINDS + (uint64_t)(bank + sl[k]) * 28, 4); if (id[k] == 0xffffffffu && bank) memcpy(&id[k], b + GA_SKILL_SLOT_BINDS + (uint64_t)sl[k] * 28, 4); }
            L += (size_t)snprintf(o + L, L < sizeof o ? sizeof o - L : 0, " %s=%d/%d", face[f].nm, (int)id[0], (int)id[1]); }
        LOG("手柄 / 触屏布局：技能格（技能号，/ 后是按着 ZL 的第二组，-1 = 空；当前武器组 %d）%s", dm_swapskill_wset(), o); }
}
