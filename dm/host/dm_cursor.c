#include "dm_elysium.h"
#include "dm_cursor.h"
#include "dm_custom.h"
#include "dm.h"
#include "dm_uiscreen.h"
#include "dm_input.h"
#include "dm_hud.h"
#include "dm_actions.h"
#include "dm_cursor_item.h"
#include "dm_i18n.h"
#include "dm_portrait.h"
#include "dm_widget.h"
#include "dm_itemjob.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <TargetConditionals.h>
#include <stdatomic.h>
#include <math.h>

static module_t *main_module(void) {
    for (int i = 0; i < g_nmod; ++i)
        if (!strcmp(g_mod[i].name, "main")) return &g_mod[i];
    return NULL;
}

static _Atomic uint64_t g_a_wait;
int dm_lat_on(void) { static int on = -1; if (on < 0) on = getenv("DM_LAT_LOG") != NULL; return on; }
void dm_cursor_set(int x, int y) {
    module_t *m = main_module();
    if (!m) { fprintf(stderr, "dm_cursor_set：没有 main 模块\n"); return; }
    *(int32_t *)(m->base + 0x2d128f8) = x;
    *(int32_t *)(m->base + 0x2d128fc) = y;
}

static _Atomic uint64_t g_req;

void dm_cursor_set_screen(float x, float y, float screen_w, float screen_h) {
    if (screen_w <= 0 || screen_h <= 0) return;
    float nx = x / screen_w, ny = y / screen_h;
    nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny;
    atomic_store(&g_req, (1ull << 63) | ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f));
}

void dm_cursor_clear(void) { atomic_store(&g_req, 0); }









enum { DM_CURSOR_NEAR = 4, DM_CURSOR_JUMP = 10, DM_CURSOR_LAND = 6 };
enum { DM_CURSOR_SYNC_POLLS = 1 };
enum { AQ_MAX = 4 };
typedef struct { int x, y; uint64_t t; } aq_entry;
static aq_entry g_aq[AQ_MAX];
static int g_naq;
static _Atomic int g_naq_pub;
int dm_cursor_pending(void) { return atomic_load(&g_naq_pub); }

static void stick_toward(int dx, int dy, int distance, int32_t *lx, int32_t *ly) {
    double strength = distance <= 16 ? 1.0 : fmax(0.25, fmin(1.0, distance / 60.0));
    *lx = (int32_t)lround(dx / (double)distance * strength * DM_STICK_MAX);
    *ly = (int32_t)lround(-dy / (double)distance * strength * DM_STICK_MAX);
    if (*lx > DM_STICK_MAX) *lx = DM_STICK_MAX;
    if (*lx < -DM_STICK_MAX) *lx = -DM_STICK_MAX;
    if (*ly > DM_STICK_MAX) *ly = DM_STICK_MAX;
    if (*ly < -DM_STICK_MAX) *ly = -DM_STICK_MAX;
}







typedef struct { int valid, x, y, cell_h, r, rows; } drop_cell;
static drop_cell g_dc;
typedef struct { int active, pending, tries, id, x, y, cell_h, r, rows, stage, tick; uint64_t t, tick_t; } drop_retry;

static drop_retry g_rt;








static int item_name(const char *n);
static int panel_skip(const char *nm);
static int has_visible_leaf(uint64_t w, int dep);
uint64_t dm_guest_frame(void);







static _Atomic uint32_t g_ui_gen = 1; static _Atomic uint64_t g_ui_gen_fr;
static _Atomic uint64_t g_reload_at, g_reload_seen;
void dm_ui_gen_bump(int why) { uint32_t g = atomic_fetch_add(&g_ui_gen, 1) + 1; uint64_t fr = dm_guest_frame(); atomic_store(&g_ui_gen_fr, fr);
    if (why == DM_UIGEN_OPMODE || why == DM_UIGEN_PERFMODE || why == DM_UIGEN_LANG) atomic_store(&g_reload_at, fr + 1);
    else if (why == DM_UIGEN_RESIZE) atomic_store(&g_reload_seen, fr + 1);
    else if (atomic_exchange(&g_reload_at, 0)) atomic_store(&g_reload_seen, fr + 1);
    static unsigned n; if (n++ >= 200) return;
    switch (why) { case DM_UIGEN_OPMODE: LOG("界面代号 → %u（游戏取走运行模式通知，要重载布局）", g); break; case DM_UIGEN_PERFMODE: LOG("界面代号 → %u（游戏取走性能模式通知）", g); break; case DM_UIGEN_RESIZE: LOG("界面代号 → %u（游戏取走性能模式通知：同比例原地改出图尺寸，不重载布局）", g); break;
        case DM_UIGEN_LANG: LOG("界面代号 → %u（切语言）", g); break; case DM_UIGEN_ROOT: LOG("界面代号 → %u（界面根换了）", g); break; default: LOG("界面代号 → %u（有顶层面板被卸掉）", g); } }
static void ui_gen_scan(void) {
    static uint64_t fr0 = UINT64_MAX, prev[256], root0; static int np; uint64_t fr = dm_guest_frame(); if (fr == fr0) return; fr0 = fr;
    module_t *m = main_module(); uint64_t root = 0; if (m) memcpy(&root, m->base + 0x2f05458, 8);
    uint64_t ch = root ? *(uint64_t *)(root + 0x58) : 0, n = root ? *(uint64_t *)(root + 0x60) : 0; if (n > 256) n = 0;
    int gone = root != root0 && root0; for (int i = 0; i < np && !gone; i++) { int k = 0; while (k < (int)n && *(uint64_t *)(ch + 8 * k) != prev[i]) k++; gone = k == (int)n; }
    if (gone) dm_ui_gen_bump(root != root0 ? DM_UIGEN_ROOT : DM_UIGEN_PANEL_GONE);
    root0 = root; np = (int)n; for (int i = 0; i < np; i++) prev[i] = *(uint64_t *)(ch + 8 * i);
}
uint32_t dm_ui_gen(void) { ui_gen_scan(); return atomic_load(&g_ui_gen); }
static int ui_has(uint64_t w, uint64_t want, int dep);
static int ui_alive(uint64_t w) { module_t *m = main_module(); uint64_t root = 0; if (m) memcpy(&root, m->base + 0x2f05458, 8); return w && root && ui_has(root, w, 0); }




uint64_t dm_wref_get(dm_wref *r) { if (!r->w) return 0; uint32_t g = dm_ui_gen(); if (r->gen == g) return r->w; if (ui_alive(r->w)) { r->gen = g; return r->w; } return 0; }
int dm_ui_gen_revalidate(uint32_t *gen, uint64_t anchor) { uint32_t g = dm_ui_gen(); if (*gen == g) return 1; if (ui_alive(anchor)) { *gen = g; return 1; } return 0; }
int dm_ui_unstable(void) { int dm_padmode_busy(void); (void)dm_ui_gen(); return dm_padmode_busy() || dm_guest_frame() - atomic_load(&g_ui_gen_fr) < 2; }
int dm_ui_reloading(void) { (void)dm_ui_gen(); uint64_t f = dm_guest_frame() + 1, a = atomic_load(&g_reload_at), s = atomic_load(&g_reload_seen);
    return (a && (f < a || f - a < 240)) || (s && (f < s || f - s < 2)); }
static int free_walk(uint64_t w, int dep, uint64_t base) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    const char *nm = (const char *)*(uint64_t *)(w + 8);
    if (dep == 1 && nm && (!strncmp(nm, "HUD", 3) || panel_skip(nm))) return 0;
    if ((nm && (item_name(nm) || !strncmp(nm, "SkillButton", 11))) || *(uint64_t *)w == base + 0x20c7d68) return 1;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (free_walk(*(uint64_t *)(ch + 8 * i), dep + 1, base)) return 1;
    return 0;
}






static uint64_t modal_any(uint64_t root, int with_bind, const char **name) {
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60); if (!ch || n > 256) return 0;
    for (int i = (int)n - 1; i >= 0; i--) { uint64_t p = *(uint64_t *)(ch + 8 * i); if (!p || !*(uint8_t *)(p + 0x50)) continue;
        const char *nm = (const char *)*(uint64_t *)(p + 8); size_t l = nm ? strlen(nm) : 0;
        if (l < 5 || strcmp(nm + l - 5, "Modal") || (!with_bind && !strcmp(nm, "ButtonBindingModal")) || !has_visible_leaf(p, 1)) continue;
        if (name) *name = nm; return p; }
    return 0;
}
static uint64_t ui_find(uint64_t w, const char *name, int dep);
static uint64_t ui_panel(uint64_t root, const char *name);
static int free_cursor_page(module_t *m) {
    static uint64_t at = UINT64_MAX; static int yes; static const char *whyp = ""; uint64_t fr = dm_guest_frame(); if (fr == at) return yes;








    uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8); int was = yes; at = fr; yes = 0; const char *mn = "";
    if (root && free_walk(root, 0, (uint64_t)m->base)) { uint64_t dp;
        if (modal_any(root, 1, &mn)) yes = 0;
        else if ((dp = ui_find(root, "NpcDialogPanel", 0)) && *(uint8_t *)(dp + 0x50) && has_visible_leaf(dp, 1)) { yes = 0; mn = "NpcDialogPanel"; }


        else if ((dp = ui_find(root, "DropdownListContents", 0)) && *(uint8_t *)(dp + 0x50)) { yes = 0; mn = "DropdownListContents"; }
        else yes = 1; }
    if (yes != was || (!yes && mn != whyp && mn[0])) LOG("dm_cursor_apply：这一页%s自由光标%s%s%s", yes ? "有" : "没有", yes ? "" : "，光标请求不执行（不推摇杆）", mn[0] ? "；上面盖着 " : "", mn);
    whyp = mn;
    return yes;
}
static void cursor_apply(uint64_t *buttons, int32_t *lx, int32_t *ly);
static _Atomic uint64_t g_exact_xy = UINT64_MAX;
static _Atomic int g_sn_new;
static int sn_on; static _Atomic uint64_t g_snap;
static _Atomic int g_free_pub;


int dm_ui_mouse_hover_now(float nx, float ny) {
    module_t *m = main_module();
    uint64_t waiting = atomic_load(&g_a_wait);
    if (!m || dm_hud_held() || dm_cursor_pending() || (waiting && now_ns() - waiting < 300000000ull) || !free_cursor_page(m)) return 0;
    int w = *(int32_t *)(m->base + 0x211da38), h = *(int32_t *)(m->base + 0x211da3c);
    if (w <= 0 || h <= 0) return 0;
    int x, y;
    if (!(nx >= 0 && nx <= 1 && ny >= 0 && ny <= 1)) { x = w * 2 / 100; y = h / 2; }
    else { x = (int)(nx * w); y = (int)(ny * h); if (x >= w) x = w - 1; if (y >= h) y = h - 1; }
    { int dm_vault_ui_blocks(int, int); if ((dm_ui_panel_state() & DM_PS_VAULT) && dm_vault_ui_blocks(x, y)) return 0; }
    int ok = dm_ui_hover_now(x, y);
    if (getenv("DM_TEST_HOVER_LOG")) { dm_cursor_item_info ci = {0}; int found = dm_cursor_item(&ci); LOG("鼠标悬停刷新：(%d,%d) → %d 物品 %s id %u", x, y, ok, found ? ci.code : "-", ci.id); }
    return ok;
}
int dm_ui_free_cursor(void) { return atomic_load(&g_free_pub); }
void dm_cursor_apply(uint64_t *buttons, int32_t *lx, int32_t *ly) {
    cursor_apply(buttons, lx, ly);




    { module_t *cm = main_module(); static int cmp = -1; int want = cm ? free_cursor_page(cm) : 0; atomic_store(&g_free_pub, want);
      int dm_padmode_current(void), dm_itemcmp_pad_compare(void); int c = want && (!dm_padmode_current() || dm_itemcmp_pad_compare()); if (c != cmp) { cmp = c; dm_item_compare(c); } }
    if ((*buttons & DM_BTN_A) && atomic_load(&g_a_wait)) atomic_store(&g_a_wait, now_ns());
}
static dm_wref g_down; static _Atomic uint64_t g_origin_xy;










static int g_pick_beltable = 1;
static uint64_t ui_find(uint64_t w, const char *name, int deep);
static int belt_row_block(module_t *m, int x, int y) {
    if (g_pick_beltable) return 0; dm_hud_info hi; dm_hud_get(&hi); if (!hi.valid || !hi.holding) return 0;
    uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8); uint64_t pn = root ? ui_find(root, "PlayerInventoryExpansionLayout", 1) : 0, b = pn && *(uint8_t *)(pn + 0x50) ? ui_find(pn, "Belt", 1) : 0;
    if (!b || !*(uint8_t *)(b + 0x50)) return 0;
    uint64_t xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32), ch = *(uint64_t *)(pn + 0x58), n = *(uint64_t *)(pn + 0x60);


    for (uint64_t i = 0; ch && i < n && n <= 256; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i); const char *nm = c ? (const char *)*(uint64_t *)(c + 8) : NULL;
        if (c && nm && !strncmp(nm, "slot_", 5) && *(uint8_t *)(c + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)c + 0x48))(c, xy) & 1)) return 0; }
    return ((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)b + 0x48))(b, xy) & 1;
}
static void origin_note(module_t *m, int x, int y) {
    dm_hud_info hi; dm_hud_get(&hi); if (hi.valid && hi.holding) return;
    dm_cursor_item_info ci; uint64_t w = dm_wref_get(&g_down); atomic_store(&g_origin_xy, 0);
    if (!w || !dm_cursor_item(&ci)) return;
    g_pick_beltable = ci.beltable || ci.container == DM_ITEM_AT_BELT;
    int32_t vw = *(int32_t *)(m->base + 0x211da38), vh = *(int32_t *)(m->base + 0x211da3c);
    uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48); int x0 = x, x1 = x, y0 = y, y1 = y;
#define ORG_HIT(px, py) (((int (*)(uint64_t, uint64_t))fn)(w, (uint64_t)(uint32_t)(px) | ((uint64_t)(uint32_t)(py) << 32)) & 1)
    if (!ORG_HIT(x, y)) return;
    while (x0 > 1 && ORG_HIT(x0 - 2, y)) x0 -= 2; while (x1 < vw - 2 && ORG_HIT(x1 + 2, y)) x1 += 2;
    while (y0 > 1 && ORG_HIT(x, y0 - 2)) y0 -= 2; while (y1 < vh - 2 && ORG_HIT(x, y1 + 2)) y1 += 2;
    int ox = (x0 + x1) / 2, oy = (y0 + y1) / 2;
    if (ci.container == DM_ITEM_AT_BELT) { int cw = (x1 - x0) / 4; if (cw > 0) { int c = (x - x0) / cw, r = (y - y0) / cw; ox = x0 + c * cw + cw / 2; oy = y0 + r * cw + cw / 2; if (oy > y1) oy = (y0 + y1) / 2; } }
    else if (ci.container != DM_ITEM_AT_EQUIP && ci.gw > 0 && ci.gh > 0) { double cell = 112.5 * vh / 2160.0; int cols = (int)lround((x1 - x0) / cell), rows = (int)lround((y1 - y0) / cell);
        if (cols > 0 && rows > 0) { ox = x0 + (int)((ci.gx + ci.gw / 2.0) * (x1 - x0) / cols); oy = y0 + (int)((ci.gy + ci.gh / 2.0) * (y1 - y0) / rows); } }
    atomic_store(&g_origin_xy, (1ull << 63) | (uint64_t)(uint32_t)ox | ((uint64_t)(uint32_t)oy << 32));
}
int dm_ui_origin_point(float *nx, float *ny) {
    module_t *m = main_module(); if (!m) return 0; int32_t vw = *(int32_t *)(m->base + 0x211da38), vh = *(int32_t *)(m->base + 0x211da3c); uint64_t v = atomic_load(&g_origin_xy);
    if (!(v >> 63) || vw <= 0 || vh <= 0) return 0; *nx = (float)(((v & 0xffffffff) + 0.5) / vw); *ny = (float)((((v >> 32) & 0x7fffffff) + 0.5) / vh); return 1;
}














static int g_fb_used, g_fb_nudge;
static _Atomic int g_drop_loc;
static _Atomic uint64_t g_pick_xy;
static _Atomic uint64_t g_set_back_at;
#define DROP_RETRY_ARM() do { dm_hud_info hi; dm_hud_get(&hi); if (g_rt.active && g_rt.stage) {                                                                                                           } else if (hi.valid && hi.holding) { int grid = g_dc.valid && abs(tx - g_dc.x) <= 5 && abs(ty - g_dc.y) <= 5; \
    g_rt = (drop_retry){ .active = 1, .id = hi.holding, .x = tx, .y = ty, .cell_h = grid ? g_dc.cell_h : 0, .r = grid ? g_dc.r : 0, .rows = grid ? g_dc.rows : 1, .t = now_ns() }; g_dc.valid = 0; } else g_rt.active = 0; } while (0)























































static _Atomic int g_hand_idle; static uint64_t g_pick_t; static int g_pick_id;
static _Atomic int g_auto_sess, g_auto_hint; static int g_auto_id, g_hg_failed; static uint64_t g_auto_busy_t, g_a_edge_t;
void dm_ui_hand_idle(int idle) { atomic_store(&g_hand_idle, idle != 0); }
static int job_busy(void) { return dm_itemjob_owner() != DM_ITEMJOB_NONE; }
int dm_hand_hint_pending(void) { return atomic_load(&g_auto_hint) && atomic_load(&g_auto_sess); }
void dm_hand_hint_done(void) { atomic_store(&g_auto_hint, 0); }
static void auto_track(int h, int valid, int last, int busy, uint64_t now) {
    static int was_busy; int sess = atomic_load(&g_auto_sess);
    if (busy) { g_auto_busy_t = now; was_busy = 1; if (h) { atomic_store(&g_auto_sess, 1); g_auto_id = h; } return; }
    if (was_busy) { was_busy = 0; if (sess && h) LOG("手上物品守门：自动化做完、手上留着它的那件（id %d）→ 会话里不按返回、不丢", h); }
    if (h && h != last && !sess && g_auto_busy_t && now - g_auto_busy_t < 10000000000ull && now - g_a_edge_t > 3500000000ull && !g_naq) {
        atomic_store(&g_auto_sess, 1); g_auto_id = h; LOG("手上物品守门：任务结束 %.1f 秒后手上来了 id %d、之前没按 A → 当作自动化迟到的回包，会话里不按返回、不丢", (now - g_auto_busy_t) / 1e9, h); }
    if (sess && valid && !h) { atomic_store(&g_auto_sess, 0); atomic_store(&g_auto_hint, 0); g_auto_id = 0; LOG("手上物品守门：手上空了，自动化留下的会话结束"); }
}
static void hand_guard(module_t *m) {
    dm_hud_info hi; dm_hud_get(&hi); int h = hi.valid ? hi.holding : 0; uint64_t now = now_ns();
    static int last, off = -1; static uint64_t since; if (off < 0) { const char *e = getenv("DM_TEST_HAND_GUARD"); off = e && !strcmp(e, "0"); }
    int busy = job_busy(); auto_track(h, hi.valid, last, busy, now);
    if (h && h != last) g_pick_id = now - g_pick_t < 1500000000ull ? h : 0;
    last = h; if (!h) g_hg_failed = 0;
    int dm_padmode_current(void), sess = atomic_load(&g_auto_sess), left = sess && h == g_auto_id;
    if (off || !h || h == g_hg_failed || !atomic_load(&g_hand_idle) || (dm_padmode_current() && !left) || busy || g_rt.active || g_naq || dm_held_pending() || dm_ui_targeting() || (dm_ui_panel_state() & DM_PS_IMBUE)) { since = 0; return; }
    if (!since) { since = now; return; }
    if (now - since < 600000000ull) return;
    since = 0; g_hg_failed = h;
    int back = !sess && (atomic_load(&g_pick_xy) >> 63) && g_pick_id == h && m && free_cursor_page(m);
    g_rt = (drop_retry){ .active = 1, .id = h, .rows = 1, .stage = back ? 0 : 1, .t = now - 400000000ull }; atomic_store(&g_drop_loc, 0);
    LOG("手上物品守门：触摸已空闲 0.6 秒、手上还拿着（id %d）→ %s", h, back ? "放回拿起时的原格" : sess ? "自动化留下的：只收进背包，收不进就留在手上" : "收进背包");
}





static void recipe_handoff(void) {
    if (!job_busy() || (!g_rt.active && !g_naq)) return;
    LOG("dm_cursor_apply：配方书 / 一键合成在动物品，光标归它 → 松手收尾（%d，第 %d 步）与扣住的 A（%d 个）作废", g_rt.active, g_rt.stage, g_naq);
    g_rt.active = g_rt.pending = 0; g_naq = 0; atomic_store(&g_naq_pub, 0);
}










enum { STOW_STALL_MS = 3000, STOW_MAX_MS = 30000 };
static int stow_waiting(uint64_t now) {
    if (dm_held_stow_result() >= 0) return 0;
    int k = dm_held_stow_tick();
    if (!g_rt.tick_t || k != g_rt.tick) { g_rt.tick = k; g_rt.tick_t = now; }
    if (now - g_rt.tick_t < STOW_STALL_MS * 1000000ull && now - g_rt.t < STOW_MAX_MS * 1000000ull) return 1;
    LOG("【红】dm_cursor_apply：收进背包 %.1f 秒没出结果（%s）→ 不再等（手上 id %d）", (now - g_rt.t) / 1e9, now - g_rt.tick_t >= STOW_STALL_MS * 1000000ull ? "3 秒没再试" : "超过 30 秒", g_rt.id);
    return 0;
}
static void cursor_apply(uint64_t *buttons, int32_t *lx, int32_t *ly) {
    recipe_handoff();
    hand_guard(main_module());
    static uint64_t prev_a;
    uint64_t a_now = *buttons & DM_BTN_A, a_edge = a_now && !prev_a;
    prev_a = a_now; if (a_edge) g_a_edge_t = now_ns();
    static int y_wait; uint64_t y_now = *buttons & DM_BTN_Y; if (!y_now) y_wait = 0;
    uint64_t r = atomic_load(&g_req);








    if (g_rt.active && (dm_ui_panel_state() & DM_PS_IMBUE)) g_rt.active = 0;




    static int g_back_id; { dm_hud_info h0; dm_hud_get(&h0); if (!h0.holding || (g_back_id && h0.holding != g_back_id)) g_back_id = 0; }
    if (g_rt.active && !g_rt.pending && now_ns() - g_rt.t > 300000000ull && !(dm_held_pending() && now_ns() - g_rt.t < 2000000000ull)) {
        dm_hud_info hi; dm_hud_get(&hi);
        uint64_t pk = atomic_load(&g_pick_xy); int sess = atomic_load(&g_auto_sess);
        if (!hi.valid || !hi.holding) { g_rt.active = 0; if (g_fb_used) { g_fb_used = 0; g_fb_nudge = 1; } }
        else if (g_naq >= AQ_MAX) g_rt.active = 0;
        else if (g_rt.stage == 0 && hi.holding == g_rt.id && g_rt.tries < 2 && g_rt.rows > 1 && ({ int hw_ = 0, hh_ = 0; int dm_held_size(int *, int *); dm_held_size(&hw_, &hh_) && hh_ > 1; })) {
            int nr = g_rt.r + (2 * g_rt.r + 1 < g_rt.rows ? 1 : -1); g_rt.y += (nr - g_rt.r) * g_rt.cell_h; g_rt.r = nr; g_rt.tries++;
            LOG("dm_cursor_apply：放下后物品还在手上，往里挪一格到 (%d,%d) 再放（第 %d 次）", g_rt.x, g_rt.y, g_rt.tries);
            g_rt.pending = 1; g_aq[g_naq++] = (aq_entry){g_rt.x, g_rt.y, now_ns()}; }
        else if (g_rt.stage == 0 && hi.holding == g_rt.id && atomic_load(&g_drop_loc) > 0) {
            int loc = atomic_exchange(&g_drop_loc, 0); void dm_held_equip(int); dm_held_equip(loc); g_rt.t = now_ns(); g_fb_used = 1;
            LOG("dm_cursor_apply：放在装备槽上没放下（手上 id %d）→ 直接调原版放下，栏位 %d", hi.holding, loc); }
        else if (g_rt.stage == 0 && (pk >> 63) && !sess) { g_rt.stage = 1; g_rt.x = (int)(pk & 0x7fffffff); g_rt.y = (int)(pk >> 32 & 0x7fffffff);
            LOG("dm_cursor_apply：放下后手上还有物品（%s，id %d）→ 放到被拖物品腾出的原位 (%d,%d)", hi.holding == g_rt.id ? "没放下" : "换下来的那件", hi.holding, g_rt.x, g_rt.y);
            g_rt.pending = 1; g_aq[g_naq++] = (aq_entry){g_rt.x, g_rt.y, now_ns()}; }
        else if (g_rt.stage < 2) { g_rt.stage = 2; g_rt.t = now_ns(); g_rt.tick_t = 0; dm_held_stow();
            LOG("dm_cursor_apply：原位也放不下（手上 id %d）→ 收进背包（不关面板）", hi.holding); }
        else if (g_rt.stage == 2 && stow_waiting(now_ns())) {                                                                                                                                                                                                                                                                                                                                                                                              }





        else if (g_rt.stage >= 2 && sess) { int res = dm_held_stow_result(), why = dm_held_stow_why();
            if (g_rt.stage == 2 && res > 0 && now_ns() - g_rt.t < 5000000000ull) {                                                               }
            else { g_rt.active = 0; g_hg_failed = hi.holding; if (res == 0 && why == 4) atomic_store(&g_auto_hint, 1);
                LOG("dm_cursor_apply：自动化留下的那件（手上 id %d）收不进背包（结果 %d，原因 %d）→ 不按返回、不丢，留在手上%s", hi.holding, res, why, res == 0 && why == 4 ? "，提示「没有足够的空间」" : ""); } }
        else if (g_rt.stage == 2 && g_back_id == hi.holding) { g_rt.active = 0; static int lg; if (lg++ < 20) LOG("【红】dm_cursor_apply：手上这件（id %d）已按过一轮返回仍没放下 → 不再按返回（不关玩家的面板），等手上换了再说", hi.holding); }
        else if (g_rt.stage == 2) { g_rt.stage = 3; g_rt.t = now_ns(); g_back_id = hi.holding; atomic_store(&g_set_back_at, dm_guest_frame() + 1);
            LOG("dm_cursor_apply：背包也收不进（手上 id %d，结果 %d）→ 按返回，交给游戏自己处理", hi.holding, dm_held_stow_result()); }
        else if (g_rt.stage == 3) { g_rt.stage = 4; g_rt.t = now_ns(); atomic_store(&g_set_back_at, dm_guest_frame() + 1);
            LOG("dm_cursor_apply：按返回只关了面板、手上还拿着（id %d）→ 再按一次返回，由游戏放背包或丢在脚下", hi.holding); }
        else { g_rt.active = 0; LOG("【红】dm_cursor_apply：按两次返回后手上仍有物品（id %d），收尾放弃", hi.holding); }
    }
    if (!(r >> 63) && !g_rt.pending) { g_naq = 0; atomic_store(&g_naq_pub, 0); return; }
    module_t *m = main_module();
    static int warned, logged, last_ix = -1, last_iy = -1;
    if (!m) { if (!warned++) LOG("【红】dm_cursor_apply：没有 main 模块，跳过"); return; }
    if (!free_cursor_page(m)) { uint64_t mine = (DM_BTN_A | DM_BTN_Y) & ~dm_ui_auto_buttons();
        if (*buttons & mine) { static int lg; if (lg++ < 20) LOG("dm_cursor_apply：这一页没有自由光标，手势层带着光标请求送来的确认 / 丢弃键不放行（防止被模态框吃掉）"); *buttons &= ~mine; }
        g_naq = 0; g_rt.active = g_rt.pending = 0; atomic_store(&g_naq_pub, 0); return; }
    int32_t width = *(int32_t *)(m->base + 0x211da38);
    int32_t height = *(int32_t *)(m->base + 0x211da3c);
    if (width <= 0 || height <= 0) { if (!warned++) LOG("【红】dm_cursor_apply：原版视口 %dx%d 无效，跳过", width, height); return; }
    int ix = (int)(((r >> 16) & 0xffff) / 65535.0 * width), iy = (int)((r & 0xffff) / 65535.0 * height);
    if (!(r >> 63)) { ix = g_rt.x; iy = g_rt.y; }
    static int sn_tx, sn_ty, sn_x, sn_y;






    if (atomic_exchange(&g_sn_new, 0)) sn_on = 0;
    if (g_fb_nudge) { g_fb_nudge = 0; sn_on = 1; sn_tx = ix; sn_ty = iy; sn_x = width * 2 / 100; sn_y = height / 2; dm_ui_hover_now(sn_x, sn_y); LOG("dm_cursor_apply：兜底装上后光标送到停靠点 (%d,%d)，离开腰带", sn_x, sn_y); }
    if (sn_on && ix == sn_tx && iy == sn_ty) { ix = sn_x; iy = sn_y; } else sn_on = 0;
    if (ix >= width) ix = width - 1;
    if (iy >= height) iy = height - 1;
    int32_t cx = *(int32_t *)(m->base + 0x2d128f8);
    int32_t cy = *(int32_t *)(m->base + 0x2d128fc);
    int arrived_now = (int)hypot((double)(ix - cx), (double)(iy - cy)) <= DM_CURSOR_NEAR;
    if (a_edge && (g_naq || !arrived_now)) {
        if (g_naq < AQ_MAX) g_aq[g_naq++] = (aq_entry){ix, iy, now_ns()};
        else LOG("【红】dm_cursor_apply：扣住的 A 超过 %d 个，丢弃 (%d,%d)", AQ_MAX, ix, iy);
    }
    while (g_naq && now_ns() - g_aq[0].t > 3000000000ull) {


        LOG("【红】dm_cursor_apply：目标 (%d,%d) 3 秒未到位，放弃这次 A", g_aq[0].x, g_aq[0].y);
        memmove(g_aq, g_aq + 1, --g_naq * sizeof g_aq[0]); if (g_naq) g_aq[0].t = now_ns();
    }
    int tx = g_naq ? g_aq[0].x : ix, ty = g_naq ? g_aq[0].y : iy;
    static int an_x = -100000, an_y = -100000;
    int dx = tx - cx, dy = ty - cy, distance = (int)hypot((double)dx, (double)dy);


















    uint64_t aw = atomic_load(&g_a_wait); int waiting = aw && now_ns() - aw < 300000000ull; if (aw && !waiting) atomic_store(&g_a_wait, 0);
    if (waiting) { tx = cx; ty = cy; dx = dy = 0; distance = 0; }
    else if (belt_row_block(m, tx, ty)) { tx = cx; ty = cy; dx = dy = 0; distance = 0; }
    else if (distance > 0 && dm_ui_hover_now(tx, ty)) { DM_LAT("hover_now %d,%d", tx, ty); cx = tx; cy = ty; dx = dy = 0; distance = 0;




        dm_cursor_item_info ci; dm_hud_info hi; dm_hud_get(&hi);




        if (!g_naq && !(hi.valid && hi.holding) && !dm_cursor_item(&ci) && atomic_load(&g_exact_xy) != ((uint64_t)(uint32_t)tx | (uint64_t)(uint32_t)ty << 32)) {
            int st = height * 20 / 750, found = 0; static const int ox[8] = { 0, 0, -1, 1, -1, 1, -1, 1 }, oy[8] = { -1, 1, 0, 0, -1, -1, 1, 1 };
            for (int k = 0; k < 8 && !found; k++) { int px = tx + ox[k] * st, py = ty + oy[k] * st; if (px < 0 || py < 0 || px >= width || py >= height) continue;
                if (dm_ui_hover_now(px, py) && dm_cursor_item(&ci)) { found = 1; sn_on = 1; sn_tx = tx; sn_ty = ty; sn_x = px; sn_y = py; atomic_store(&g_snap, (uint64_t)(tx & 0xffff) | (uint64_t)(ty & 0xffff) << 16 | (uint64_t)(px & 0xffff) << 32 | (uint64_t)(py & 0xffff) << 48); cx = px; cy = py; DM_LAT("hover_snap %d,%d", px, py); } }
            if (!found) dm_ui_hover_now(tx, ty); } }
    static int sync_left;
    { static int lt; int tg = tx * 4096 + ty; if (tg != lt) { lt = tg; DM_LAT("target %d,%d d=%d", tx, ty, distance); } }
    if (distance > DM_CURSOR_JUMP && !sync_left) {
        sync_left = DM_CURSOR_SYNC_POLLS + 1; DM_LAT("jump");
        cx = tx - (int)lround(dx * (double)DM_CURSOR_LAND / distance); cy = ty - (int)lround(dy * (double)DM_CURSOR_LAND / distance);
        *(int32_t *)(m->base + 0x2d128f8) = cx; *(int32_t *)(m->base + 0x2d128fc) = cy;
        dx = tx - cx; dy = ty - cy; distance = (int)hypot((double)dx, (double)dy);
    }
    if (sync_left && --sync_left) {
        *lx = *ly = 0; *buttons &= ~DM_BTN_A; if (y_now) { *buttons &= ~DM_BTN_Y; y_wait = 1; }
        atomic_store(&g_naq_pub, g_naq + y_wait); return;
    }
    if (g_naq) {
        *buttons &= ~DM_BTN_A;
        if (waiting) { *lx = *ly = 0; }
        else if (distance <= DM_CURSOR_NEAR) { *buttons |= DM_BTN_A; *lx = *ly = 0;
            if (g_rt.pending) { g_rt.pending = 0; g_rt.t = now_ns(); }
            else { DROP_RETRY_ARM(); }
            DM_LAT("a_out %d,%d", cx, cy);
            origin_note(m, tx, ty);
            an_x = tx; an_y = ty; atomic_store(&g_a_wait, now_ns());
            memmove(g_aq, g_aq + 1, --g_naq * sizeof g_aq[0]); if (g_naq) g_aq[0].t = now_ns(); static int lg; if (lg++ < 60) LOG("dm_cursor_apply：到位 (%d,%d)，放出扣住的 A", cx, cy); }
        else stick_toward(dx, dy, distance, lx, ly);
    } else if (distance <= DM_CURSOR_NEAR) {
        *lx = *ly = 0;
        if (a_edge) { dm_hud_info ph; dm_hud_get(&ph); int empty = !(ph.valid && ph.holding); origin_note(m, tx, ty); if (empty) { atomic_store(&g_pick_xy, atomic_load(&g_origin_xy)); g_pick_t = now_ns(); } an_x = tx; an_y = ty; atomic_store(&g_a_wait, now_ns()); DROP_RETRY_ARM(); }
        else if (a_now && !waiting && (abs(tx - an_x) > DM_CURSOR_NEAR || abs(ty - an_y) > DM_CURSOR_NEAR)) *buttons &= ~DM_BTN_A;
        { static int was; int tgt = tx * 4096 + ty; if (tgt != was) { was = tgt; DM_LAT("arrive %d,%d", tx, ty); } if (a_edge) DM_LAT("a_pass %d,%d", cx, cy); if (y_now && !y_wait) { static uint64_t yl; if (now_ns() - yl > 500000000ull) DM_LAT("y_pass"); yl = now_ns(); } }
    } else {
        *buttons &= ~DM_BTN_A;
        stick_toward(dx, dy, distance, lx, ly);
    }
    if (y_now) { if (g_naq || distance > DM_CURSOR_NEAR) { *buttons &= ~DM_BTN_Y; y_wait = 1; } else y_wait = 0; }
    atomic_store(&g_naq_pub, g_naq + y_wait);
    if ((tx != last_ix || ty != last_iy) && logged < 40) { logged++; last_ix = tx; last_iy = ty; LOG("dm_cursor_apply：目标 (%d,%d)，原生 (%d,%d)，距 %d，扣住 A %d", tx, ty, cx, cy, distance, g_naq); }
}












uint64_t dm_guest_frame(void);






static _Atomic int g_ut_phase;
static _Atomic uint64_t g_ut_xy;














static _Atomic int g_cc_open;
static _Atomic int g_fe_name_open, g_fe_name_tap;
static _Atomic int g_fe_name_has;
static const char *const k_cc_class[8] = { "Amazon", "Assassin", "Necromancer", "Barbarian", "Paladin", "Sorceress", "Warlock", "Druid" };
static _Atomic int g_fe_opt_open, g_fe_confirm_open, g_fe_back;
static _Atomic int g_fe_opt_side;






#define BLISS_Y0 1290
#define BLISS_Y1 1470
static _Atomic int g_fe_bliss;
int dm_fe_bliss(void) { return atomic_load(&g_fe_bliss); }


static _Atomic int g_fe_del_open, g_del_q, g_del_hold;
int dm_restart_busy(void);
void dm_ui_touch(int phase, float nx, float ny) { if (phase != 3 && dm_restart_busy()) return;
    if (phase == 3) { atomic_store(&g_ut_phase, 0); atomic_store(&g_del_q, 0); atomic_store(&g_del_hold, 0); return; }
    if (phase != 1) DM_LAT("ui_touch %d", phase);
    nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny;
    atomic_store(&g_ut_xy, ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f));


    if (phase == 2 && atomic_load(&g_ut_phase) == 1 && atomic_load(&g_fe_opt_open) && !atomic_load(&g_fe_confirm_open) && (ny < 899 / 2160.0f || ny > 1259 / 2160.0f) && !(ny > BLISS_Y0 / 2160.0f && ny < BLISS_Y1 / 2160.0f)) { atomic_store(&g_ut_phase, 0); atomic_store(&g_fe_back, 1); return; }
    if (phase == 2 && atomic_load(&g_fe_opt_open) && !atomic_load(&g_fe_confirm_open)) atomic_store(&g_fe_opt_side, nx >= 0.5f);
    if (phase == 2 && (atomic_exchange(&g_del_hold, 0) | atomic_exchange(&g_del_q, 0))) { atomic_store(&g_ut_phase, 0); return; }
    if (phase == 0 && atomic_load(&g_fe_del_open)) atomic_store(&g_del_q, 1);
    if (phase == 2) { if (atomic_load(&g_ut_phase) == 1) atomic_store(&g_ut_phase, 2); }
    else atomic_store(&g_ut_phase, 1);
}
int dm_ui_touch_active(void) { return atomic_load(&g_ut_phase) != 0; }


int dm_ui_fe_drag_ok(void) { return !atomic_load(&g_fe_del_open) && !atomic_load(&g_fe_opt_open) && !atomic_load(&g_fe_name_open) && !atomic_load(&g_fe_confirm_open); }
static uint64_t ui_hash(const char *s) { uint64_t h = 0xcbf29ce484222325ull; for (; *s; s++) h = (h ^ (uint8_t)*s) * 0x100000001b3ull; return h; }
static void ui_send_mouse_arg(module_t *m, uint64_t root, uint64_t act, int x, int y, int arg) {
    static uint64_t payload[16], msg[8];
    memset(payload, 0, sizeof payload); payload[0] = (uint64_t)(uint32_t)x << 32; payload[1] = (uint32_t)y; payload[2] = (uint32_t)arg;
    memset(msg, 0, sizeof msg); msg[0] = ui_hash("InputMessage"); msg[1] = act; msg[2] = (uint64_t)payload;
    uint64_t vt = 0, fn = 0; memcpy(&vt, (void *)root, 8); memcpy(&fn, (void *)(vt + 0x28), 8);
    dm_cursor_set(x, y); (void)m;
    ((void (*)(uint64_t, void *))fn)(root, msg);
}
static void ui_send_mouse(module_t *m, uint64_t root, uint64_t act, int x, int y) { ui_send_mouse_arg(m, root, act, x, y, 0); }




void dm_ui_wheel_on(uint64_t w, int notches) {
    module_t *m = main_module(); uint64_t root = 0; if (!m || !w || !notches) return; memcpy(&root, m->base + 0x2f05458, 8); if (!root) return;
    int W = *(int32_t *)(m->base + 0x211da38), H = *(int32_t *)(m->base + 0x211da3c), x0 = W, x1 = -1, y0 = H, y1 = -1; if (W <= 0 || H <= 0) return;
    uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48);
    for (int y = 8; y < H; y += 16) for (int x = 8; x < W; x += 16) if (((int (*)(uint64_t, uint64_t))fn)(w, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1) { if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y; }
    if (x1 < 0) return; int cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
    ui_send_mouse(m, root, ui_hash("MouseMove"), cx, cy); ui_send_mouse_arg(m, root, ui_hash("MouseScroll"), cx, cy, notches * 120);
}




static _Atomic int g_tab_unhover;
static _Atomic int g_up_hold; static _Atomic uint64_t g_up_xy; static struct { int kind, arg; } g_up_q[32]; static _Atomic unsigned g_up_w, g_up_r;
void dm_ui_ptr(int kind, float nx, float ny, int arg) { if (dm_restart_busy()) return;
    nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny;
    if (kind <= DM_UP_MOVE) { atomic_store(&g_up_xy, ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f)); if (kind == DM_UP_HOVER) atomic_store(&g_up_hold, 1); return; }
    unsigned w = atomic_load(&g_up_w); if (w - atomic_load(&g_up_r) >= 32) return;
    g_up_q[w % 32].kind = kind; g_up_q[w % 32].arg = arg; atomic_store(&g_up_w, w + 1);
}








static _Atomic uint64_t g_iq_xy; static _Atomic unsigned g_iq_seq, g_iq_done; static _Atomic int g_iq_res;
static _Atomic int g_iq_drop;
static _Atomic int g_drop_last = -1;




static int hit_log_ok(const char *hit) {
    static char last[96]; static uint64_t t0; static int n; uint64_t now = now_ns();
    if (strcmp(last, hit) || now - t0 > 1000000000ull) { strlcpy(last, hit, sizeof last); t0 = now; n = 0; }
    return n++ < 4;
}
static _Atomic int g_iq_out;
#define UI_OUT(nx, ny) ((nx) < 0 || (nx) > 1 || (ny) < 0 || (ny) > 1)
unsigned dm_ui_item_query(float nx, float ny) {
    if (!atomic_load(&g_iq_drop)) { atomic_store(&g_drop_last, -1); atomic_store(&g_sn_new, 1); }
    DM_LAT("query");
    atomic_store(&g_iq_out, UI_OUT(nx, ny));
    nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny;
    atomic_store(&g_iq_xy, ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f));
    return atomic_fetch_add(&g_iq_seq, 1) + 1;
}
int dm_ui_item_result(unsigned seq) { return atomic_load(&g_iq_done) == seq ? atomic_load(&g_iq_res) : -1; }
static int item_name(const char *n) { return !strcmp(n, "grid") || !strcmp(n, "horadriccube_grid") || !strcmp(n, "Belt") || !strncmp(n, "slot_", 5) || !strncmp(n, "advancedstash_", 14) || !strcmp(n, "basicstash_container"); }
static uint64_t g_iw_base;
static const char *g_modal_name = "";
static dm_wref g_slot; static const char *g_slot_panel;
static int item_walk(uint64_t w, uint64_t xy, int dep, char *hit, size_t hn) {
    if (!w || dep > 14) return 0;
    uint64_t nm = *(uint64_t *)(w + 8), ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60);
    if (!*(uint8_t *)(w + 0x50)) return 0;
    if (dep == 1 && nm && !strncmp((const char *)nm, "HUD", 3)) return 0;


    if (nm && (item_name((const char *)nm) || (g_iw_base && (*(uint64_t *)w - g_iw_base == 0x20892f8 || *(uint64_t *)w - g_iw_base == 0x20898e8)))) {                                                                                                                                                                                                                                                                                                                                               uint64_t vt = *(uint64_t *)w, fn = *(uint64_t *)(vt + 0x48);
        if (((int (*)(uint64_t, uint64_t))fn)(w, xy) & 1) { strlcpy(hit, (const char *)nm, hn); dm_wref_set(&g_slot, w); return 1; } }
    if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (item_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1, hit, hn)) return 1;
    return 0;
}
static _Atomic uint64_t g_taps_base;


static void ui_dump(uint64_t w, int dep, const char *want, int on, module_t *m) {
    if (!w || dep > 14) return; uint64_t nm = *(uint64_t *)(w + 8), ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); const char *name = nm ? (const char *)nm : "";
    if (!strcmp(want, "@top")) { if (dep <= 2 && *(uint8_t *)(w + 0x50)) LOG("UIDUMP %*s%s n=%llu vt=main+%#llx", dep * 2, "", name, (unsigned long long)n, (unsigned long long)(*(uint64_t *)w - (uint64_t)m->base)); if (dep >= 2) return; }
    if (!on && !strcmp(name, want)) on = 1;
    if (on) { char b[700]; int k = 0; for (int o = 0x40; o < 0x1c0 && k < 640; o += 4) { int32_t v = *(int32_t *)(w + o); float f; memcpy(&f, &v, 4); if (v && o != 0x58 && o != 0x5c && o != 0x60) { if (v > -100000 && v < 100000) k += snprintf(b + k, sizeof b - k, " %x:%d", o, v); else if (f > -9000 && f < 9000 && (f > .01f || f < -.01f)) k += snprintf(b + k, sizeof b - k, " %x:%.1f", o, f); } }
        LOG("UIDUMP %*s%s vis=%d n=%llu vt=main+%#llx 消息处理=main+%#llx |%s", dep * 2, "", name, *(uint8_t *)(w + 0x50), (unsigned long long)n, (unsigned long long)(*(uint64_t *)w - (uint64_t)m->base), (unsigned long long)(*(uint64_t *)(*(uint64_t *)w + 0x28) - (uint64_t)m->base), b); }
    if (!ch || n > 256) return;
    for (uint64_t i = 0; i < n; i++) ui_dump(*(uint64_t *)(ch + 8 * i), dep + 1, want, on, m);
}














#define TAB_SEL(w) (*(int32_t *)((w) + 0xdc4))








static dm_wref g_tab_ref; static _Atomic int g_tab_have; static int g_tab_k; static char g_tab_name[48]; static _Atomic int g_tab_direct;
static int ui_has(uint64_t w, uint64_t want, int dep) {
    if (!w || dep > 14) return 0; if (w == want) return 1; uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (ui_has(*(uint64_t *)(ch + 8 * i), want, dep + 1)) return 1;
    return 0; }
static int tab_walk(uint64_t w, uint64_t xy, int dep, uint64_t vt_tab, int *delta, int *top) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    uint64_t nm = *(uint64_t *)(w + 8), ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60);
    if (!ch || n > 256) return 0;
    if (*(uint64_t *)w == vt_tab) {
        int hit = -1, act = -1, hx = 0, ax = 0, hk = -1, sel = TAB_SEL(w); struct { int x, k; } tabs[32]; int nt = 0;
        for (uint64_t i = 0; i < n; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i); if (!c || !*(uint8_t *)(c + 0x50)) continue; const char *cn = (const char *)*(uint64_t *)(c + 8);
            if (!cn || strncmp(cn, "Image", 5) || cn[5] < '0' || cn[5] > '9' || nt >= 32) continue; int x = *(int32_t *)(c + 0x70), kk = atoi(cn + 5); tabs[nt].x = x; tabs[nt++].k = kk;
            if (kk == sel) { act = 1; ax = x; }
            uint64_t cvt = *(uint64_t *)c, fn = *(uint64_t *)(cvt + 0x48); if (((int (*)(uint64_t, uint64_t))fn)(c, xy) & 1) { hit = 1; hx = x; hk = kk; } }
        if (hit > 0) { static int lg; if (lg++ < 200) LOG("界面页签：页签条「%s」可见 %d 个，当前选中第 %d 个%s，点中第 %d 个", nm ? (const char *)nm : "?", nt, sel, act > 0 ? "" : "（不在可见页签里，不切）", hk); }
        if (hit > 0 && act > 0) { int d = 0; for (int i = 0; i < nt; i++) { if (tabs[i].x > ax && tabs[i].x <= hx) d++; if (tabs[i].x < ax && tabs[i].x >= hx) d--; }
            *delta = d; *top = nm && !strcmp((const char *)nm, "UISwitcherTabs"); g_tab_k = hk; strlcpy(g_tab_name, nm ? (const char *)nm : "", sizeof g_tab_name); dm_wref_set(&g_tab_ref, w); atomic_store(&g_tab_have, 1); return 1; }
        return 0;
    }
    for (uint64_t i = 0; i < n; i++) if (tab_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1, vt_tab, delta, top)) return 1;
    return 0;
}






static const char *g_inside_name = "";
static _Atomic int g_cube_side_req, g_cube_side_close, g_cube_side_on;
void dm_cube_side_mode_leave(void) { atomic_store(&g_cube_side_close, 1); }


static uint64_t ui_find(uint64_t w, const char *name, int dep);
static _Atomic uint64_t g_bd_hit;
static dm_wref g_bd_hit_ref;
static int bd_skill_walk(uint64_t w, uint64_t xy, int dep) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    uint64_t nm = *(uint64_t *)(w + 8), ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60);
    if (nm && !strncmp((const char *)nm, "SkillButton", 11) && strncmp((const char *)nm, "SkillButtonC", 12)) { uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48); int h = ((int (*)(uint64_t, uint64_t))fn)(w, xy) & 1; if (h) dm_wref_set(&g_bd_hit_ref, w); return h; }
    if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (bd_skill_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1)) return 1;
    return 0;
}








static _Atomic uint32_t g_bd_hit_id;
uint32_t dm_ui_hit_id(void) { return atomic_load(&g_bd_hit_id); }
static int bd_stat_walk(uint64_t w, uint64_t xy, int dep) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    uint64_t nm = *(uint64_t *)(w + 8), ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); size_t L = nm ? strlen((const char *)nm) : 0;
    if (L > 11 && !strcmp((const char *)nm + L - 11, "PointButton")) { uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48); int h = ((int (*)(uint64_t, uint64_t))fn)(w, xy) & 1; if (h) dm_wref_set(&g_bd_hit_ref, w); return h; }
    if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (bd_stat_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1)) return 1;
    return 0;
}










static dm_wref g_bd_inst;
static uint32_t g_bd_serial;
static uint32_t bd_skill_serial(uint64_t w) {
    uint32_t g = dm_ui_gen(); if (w && w == g_bd_inst.w && g == g_bd_inst.gen && g_bd_serial) return g_bd_serial;
    if (w && w == g_bd_inst.w) { static int lg; if (lg++ < 20) LOG("技能图标：同一地址上是重建后的新控件（界面代号 %u → %u）→ 换新编号，这一下只选中", g_bd_inst.gen, g); }
    dm_wref_set(&g_bd_inst, w); g_bd_serial = (g_bd_serial + 1) & 0x7fffffffu; if (!g_bd_serial) g_bd_serial = 1; return g_bd_serial;
}





int dm_ui_skill_spend_ok(void) { int dm_padmode_busy(void); return !dm_padmode_busy() && !dm_ui_reloading() && (!g_bd_inst.w || g_bd_inst.gen == dm_ui_gen()); }
static void bd_note_skill(module_t *m, uint64_t root, int x, int y) {
    int32_t w = *(int32_t *)(m->base + 0x211da38), h = *(int32_t *)(m->base + 0x211da3c); if (w <= 0 || h <= 0) return;




    if (modal_any(root, 1, NULL)) { atomic_store(&g_bd_hit_id, 0); return; }
    uint64_t xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32), tree = ui_find(root, "SkillsTreePanel", 0), cs = ui_find(root, "CharacterStatsPanel", 0);
    dm_wref_set(&g_bd_hit_ref, 0); uint64_t kind = tree && bd_skill_walk(tree, xy, 0) ? (1ull << 63) : cs && bd_stat_walk(cs, xy, 0) ? (1ull << 63) | (1ull << 62) : 0; if (!kind) { atomic_store(&g_bd_hit_id, 0); return; }
    { const char *hn = dm_wref_get(&g_bd_hit_ref) ? (const char *)*(uint64_t *)(dm_wref_get(&g_bd_hit_ref) + 8) : "";
      uint32_t id = (kind >> 62) & 1 ? (hn[0] == 'S' ? 1 : hn[0] == 'D' ? 2 : hn[0] == 'V' ? 3 : hn[0] == 'E' ? 4 : 0) : bd_skill_serial(dm_wref_get(&g_bd_hit_ref)) | 0x80000000u; atomic_store(&g_bd_hit_id, id); }
    atomic_store(&g_bd_hit, kind | ((uint64_t)(x * 65535ll / w) << 16) | (uint64_t)(y * 65535ll / h));
}
int dm_ui_bind_skill_hit(float *nx, float *ny) { uint64_t v = atomic_load(&g_bd_hit); if (!(v >> 63)) return 0;                                                                                                 *nx = ((v >> 16) & 0xffff) / 65535.0f; *ny = (v & 0xffff) / 65535.0f; return (v >> 62) & 1 ? 2 : 1; }
static int inside_walk(uint64_t w, uint64_t xy, int dep, uint64_t vt_catch) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60), vt = *(uint64_t *)w;
    if (vt == vt_catch) return 0;
    { const char *nm = (const char *)*(uint64_t *)(w + 8);
      static const char *skip[] = { "PanelBordersPanel", "ControllerCursorBounds", "ShowItemsPanel", "MiniMenuPanel", "TooltipsPanel", "EmoteWheelPanel", "TransparentBG", "SpriteColoringHelper", "LanguageFontRemapper", "ShowItemsParameters", "ControllerCursorParameters", "MoviePlayer", "AutoMap", "TooltipTextBox" };
      if (nm) for (unsigned i = 0; i < sizeof skip / sizeof *skip; i++) if (!strcmp(nm, skip[i])) return 0; }
    { uint64_t rel = vt - (vt_catch - 0x20c1328);






      if (rel == 0x20b8650 && n && ch && n <= 256) { for (uint64_t i = 0; i < n; i++) if (inside_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1, vt_catch)) return 1;
          uint64_t fn = *(uint64_t *)(vt + 0x48); if (((int (*)(uint64_t, uint64_t))fn)(w, xy) & 1) { g_inside_name = *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "?"; return 1; } return 0; }
      if (!n || !ch || rel == 0x20b9580 || rel == 0x20bc048) { uint64_t fn = *(uint64_t *)(vt + 0x48); int h = ((int (*)(uint64_t, uint64_t))fn)(w, xy) & 1; if (h) { g_inside_name = *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "?"; return 1; } if (!n || !ch) return 0; } }
    if (n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (inside_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1, vt_catch)) return 1;
    return 0;
}
static const char *g_inside_owner = "";
static int free_walk(uint64_t w, int dep, uint64_t base);




static int in_item_panel_rect(module_t *m, uint64_t root, uint64_t xy) {
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60); if (!ch || n > 256) return 0;




    int32_t vh = *(int32_t *)(m->base + 0x211da3c); int x = (int)(uint32_t)xy; uint64_t yy = xy >> 32 << 32;
    for (uint64_t i = 0; i < n; i++) { uint64_t p = *(uint64_t *)(ch + 8 * i); if (!p || !*(uint8_t *)(p + 0x50)) continue;
        uint64_t fn = *(uint64_t *)(*(uint64_t *)p + 0x48); int dx = (int)(180 * *(float *)(p + 0x80) * vh / 2160.0), xl = x > dx ? x - dx : 0;
        if (((((int (*)(uint64_t, uint64_t))fn)(p, xy) | ((int (*)(uint64_t, uint64_t))fn)(p, yy | (uint32_t)xl) | ((int (*)(uint64_t, uint64_t))fn)(p, yy | (uint32_t)(x + dx))) & 1)
            && !(((int (*)(uint64_t, uint64_t))fn)(p, (uint64_t)3 | ((uint64_t)3 << 32)) & 1) && free_walk(p, 1, (uint64_t)m->base)) return 1; }
    return 0;
}
static int ui_inside_panel(module_t *m, uint64_t root, uint64_t xy) {
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60); if (!ch || n > 256) return 0;
    for (int i = (int)n - 1; i >= 0; i--) { uint64_t p = *(uint64_t *)(ch + 8 * i); if (!p || !*(uint8_t *)(p + 0x50)) continue; const char *nm = (const char *)*(uint64_t *)(p + 8);
        if (nm && !strncmp(nm, "HUD", 3)) continue;
        if (nm && !strcmp(nm, "NotificationPanel")) continue;
        if (nm && !strcmp(nm, "FocusManager")) continue;
        if (inside_walk(p, xy, 1, (uint64_t)m->base + 0x20c1328)) { g_inside_owner = nm ? nm : "?"; return 1; } }
    return 0;
}






static dm_wref g_click;
static const char *g_click_name = ""; static int g_click_exact;
static int click_walk(uint64_t w, uint64_t xy, int dep, uint64_t base) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    uint64_t vt = *(uint64_t *)w - base, ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60);








    if (vt == 0x20b7b00 || vt == 0x208a5f8 || vt == 0x20bc110 || vt == 0x20c7d68 || vt == 0x20c80f0 || vt == 0x20c6fb8 || vt == 0x20b84d0 || vt == 0x20b8cd0 || vt == 0x20b97f8 || vt == 0x20b91c0 || vt == 0x20b8718 || vt == 0x20baff8 || (vt == 0x20bad70 && g_click_exact)) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48); if (((int (*)(uint64_t, uint64_t))fn)(w, xy) & 1) { dm_wref_set(&g_click, w); g_click_name = *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "?"; return 1; } }
    if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (click_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1, base)) return 1;
    return 0;
}










static int panel_skip(const char *nm) { static const char *sk[] = { "AutoMap", "PanelBordersPanel", "ShowItemsPanel", "MiniMenuPanel", "TooltipsPanel", "EmoteWheelPanel", "FocusManager", "InGameMoviePlayerPanel",
        "NotificationPanel" };
    if (!nm) return 0; if (!strncmp(nm, "HUD", 3)) return 2; for (unsigned i = 0; i < sizeof sk / sizeof *sk; i++) if (!strcmp(nm, sk[i])) return 1; return 0; }




static int panel_clipped(uint64_t p, uint64_t xy) { uint64_t fn = *(uint64_t *)(*(uint64_t *)p + 0x48);
    return !(((int (*)(uint64_t, uint64_t))fn)(p, (uint64_t)3 | ((uint64_t)3 << 32)) & 1) && !(((int (*)(uint64_t, uint64_t))fn)(p, xy) & 1) && *(int32_t *)(p + 0x78) > 0 && *(int32_t *)(p + 0x7c) > 0; }












static int has_visible_leaf(uint64_t w, int dep) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50) || !*(uint8_t *)(w + 0x51)) return 0;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || !n) return dep > 1;
    if (n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (has_visible_leaf(*(uint64_t *)(ch + 8 * i), dep + 1)) return 1;
    return 0;
}
static int ui_top_cover(module_t *m, uint64_t root, int x, int y) {
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60), base = (uint64_t)m->base; if (!ch || n > 256) return -1;
#define SNAP_XY(px, py) ((uint64_t)(uint32_t)(px) | ((uint64_t)(uint32_t)(py) << 32))
    int top = -1;
    for (int i = (int)n - 1; i >= 0 && top < 0; i--) { uint64_t p = *(uint64_t *)(ch + 8 * i); if (!p || !*(uint8_t *)(p + 0x50) || panel_skip((const char *)*(uint64_t *)(p + 8))) continue;
        uint64_t pfn = *(uint64_t *)(*(uint64_t *)p + 0x48);
        if (panel_clipped(p, SNAP_XY(x, y))) continue;
        if ((((int (*)(uint64_t, uint64_t))pfn)(p, SNAP_XY(x, y)) & 1 && !(((int (*)(uint64_t, uint64_t))pfn)(p, SNAP_XY(3, 3)) & 1) && has_visible_leaf(p, 1)) || inside_walk(p, SNAP_XY(x, y), 1, base + 0x20c1328)) top = i; }
    return top;
}




static int slider_hit(uint64_t w, int x, int y) { uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48); return ((int (*)(uint64_t, uint64_t))fn)(w, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1; }



static void slider_collect(uint64_t w, uint64_t vt, int dep, uint64_t *out, uint64_t *clips, uint64_t clip, int *n) {
    if (!w || dep > 16 || *n >= 48 || !*(uint8_t *)(w + 0x50)) return;
    if (*(uint64_t *)w == vt) { clips[*n] = clip; out[(*n)++] = w; return; }
    const char *nm = (const char *)*(uint64_t *)(w + 8); if (nm && !strcmp(nm, "ScrollView")) clip = w;
    uint64_t ch = *(uint64_t *)(w + 0x58), k = *(uint64_t *)(w + 0x60); if (!ch || k > 256) return;
    for (uint64_t i = 0; i < k; i++) slider_collect(*(uint64_t *)(ch + 8 * i), vt, dep + 1, out, clips, clip, n);
}
static int slider_arrow(module_t *m, uint64_t root, int *x, int *y, int W, int H) {
    static uint64_t sl[48], cl[48]; int n = 0; slider_collect(root, (uint64_t)m->base + 0x20bad70, 0, sl, cl, 0, &n); if (!n) return 0;
    float u = H / 2160.0f; int vt = (int)(106 * u), reach = (int)(212 * u), st = (int)(12 * u) > 1 ? (int)(12 * u) : 1, off = (int)(44 * u);
    for (int d = 0; d <= vt; d += st) for (int sg = 0; sg < (d ? 2 : 1); sg++) { int yy = *y + (sg ? d : -d); if (yy < 0 || yy >= H) continue;
        for (int e = 0; e <= reach + st; e += st) for (int dir = -1; dir <= 1; dir += 2) { int xx = *x + dir * e; if (xx < 0 || xx >= W || (!e && dir > 0)) continue;
            for (int i = 0; i < n; i++) if (slider_hit(sl[i], xx, yy) && (!cl[i] || slider_hit(cl[i], xx, yy))) { uint64_t w = sl[i]; int a, b, l, r, top, bot;
                for (a = 0, b = xx; a < b;) { int mid = (a + b) / 2; if (slider_hit(w, mid, yy)) b = mid; else a = mid + 1; } l = a;
                for (a = xx, b = W - 1; a < b;) { int mid = (a + b + 1) / 2; if (slider_hit(w, mid, yy)) a = mid; else b = mid - 1; } r = a;
                for (a = 0, b = yy; a < b;) { int mid = (a + b) / 2; if (slider_hit(w, xx, mid)) b = mid; else a = mid + 1; } top = a;
                for (a = yy, b = H - 1; a < b;) { int mid = (a + b + 1) / 2; if (slider_hit(w, xx, mid)) a = mid; else b = mid - 1; } bot = a;
                int in = (int)(64 * u), side = dir;
                if (*x >= l && *x <= r) { if (*x - l <= in && *x - l <= r - *x) side = 1; else if (r - *x <= in) side = -1; else return 0; }
                int ax = side > 0 ? l - off : r + off, ay = (top + bot) / 2;
                LOG("界面吸附：(%d,%d) 在滑块 %s 的%s端（滑槽 x %d–%d，行 y=%d）→ 点箭头 (%d,%d)", *x, *y, *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "?", side > 0 ? "左" : "右", l, r, ay, ax, ay);


                dm_wref_set(&g_click, w); g_click_name = *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "?";
                *x = ax; *y = ay; return 1; } } }
    return 0;
}
static void slot_center(uint64_t w, int *x, int *y, int vw, int vh, int cells);
static int ui_snap(module_t *m, uint64_t root, int *x, int *y) {
    int32_t w = *(int32_t *)(m->base + 0x211da38), h = *(int32_t *)(m->base + 0x211da3c); if (w <= 0 || h <= 0) return 0;
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60), base = (uint64_t)m->base; if (!ch || n > 256) return 0;
    int top = ui_top_cover(m, root, *x, *y);
    if (getenv("DM_DBG_COVER")) { char b[900]; int k = 0; for (int i = (int)n - 1; i >= 0 && k < 820; i--) { uint64_t p = *(uint64_t *)(ch + 8 * i); if (!p || !*(uint8_t *)(p + 0x50)) continue; const char *nm = (const char *)*(uint64_t *)(p + 8);
            int rh = ((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)p + 0x48))(p, SNAP_XY(*x, *y)) & 1, r0 = ((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)p + 0x48))(p, SNAP_XY(3, 3)) & 1;
            int lf = inside_walk(p, SNAP_XY(*x, *y), 1, base + 0x20c1328); if (!rh && !lf) continue;
            k += snprintf(b + k, sizeof b - k, " [%d]%s:根%d角%d叶=%s", i, nm ? nm : "?", rh, r0, lf ? g_inside_name : "-"); } LOG("盖层探查 (%d,%d)：%s", *x, *y, b); }
    int lo = top < 0 ? 0 : top;
#define SNAP_AT(px, py, from, to) ({ int hit_ = 0; for (int i_ = (to); i_ >= (from) && !hit_; i_--) { uint64_t p_ = *(uint64_t *)(ch + 8 * i_); if (p_ && *(uint8_t *)(p_ + 0x50) && (top < 0 || panel_skip((const char *)*(uint64_t *)(p_ + 8)) != 1) && !panel_clipped(p_, SNAP_XY(px, py))) hit_ = click_walk(p_, SNAP_XY(px, py), 1, base); } hit_; })
    if (SNAP_AT(*x, *y, lo, (int)n - 1)) return 1;
    if (slider_arrow(m, root, x, y, w, h)) return 2;
    g_click_exact = 1; int on = SNAP_AT(*x, *y, lo, (int)n - 1); g_click_exact = 0; if (on) return 1;
    static const int dx[8] = { 0, 0, -1, 1, -1, 1, -1, 1 }, dy[8] = { -1, 1, 0, 0, -1, -1, 1, 1 };
    float R = 90.0f * h / 2160.0f;
    for (int ring = 1; ring <= 9; ring++) for (int d = 0; d < 8; d++) { float r = R * ring / 9 * (d < 4 ? 1.0f : 0.7071f);
        int px = *x + (int)(dx[d] * r), py = *y + (int)(dy[d] * r); if (px < 0 || py < 0 || px >= w || py >= h) continue;
        if (SNAP_AT(px, py, lo, (int)n - 1)) {
            uint64_t cw = dm_wref_get(&g_click); int qx = px, qy = py; slot_center(cw, &qx, &qy, w, h, 0);
            if (SNAP_AT(qx, qy, lo, (int)n - 1) && dm_wref_get(&g_click) == cw) { px = qx; py = qy; } else { SNAP_AT(px, py, lo, (int)n - 1); }
            *x = px; *y = py; return 2; } }
    return 0;
}
static uint64_t ui_find(uint64_t w, const char *name, int dep);




static int snap_guarded(uint64_t root, const char *nm) {
    static const char *const k[] = { "CancelButton", "DefaultButton", "convert", "button_repair_all", "button_repair", "button_refresh", "button_blank", "Exit", "TouchRestart" };


    if (!nm) return 0; for (unsigned i = 0; i < sizeof k / sizeof *k; i++) if (!strcmp(nm, k[i])) return 1;
    if (!strcmp(nm, "Confirm")) { uint64_t dm = ui_find(root, "DeleteCharacterModal", 0); return dm && *(uint8_t *)(dm + 0x50); }
    return 0;
}
static int snap_guard_near(uint64_t cw, const char *nm, int x, int y, int sx, int sy, int vw, int vh) {
    if (!cw || vh <= 0) return 0; uint64_t fn = *(uint64_t *)(*(uint64_t *)cw + 0x48); int x0 = sx, x1 = sx, y0 = sy, y1 = sy;
#define SG_HIT(px, py) (((int (*)(uint64_t, uint64_t))fn)(cw, (uint64_t)(uint32_t)(px) | ((uint64_t)(uint32_t)(py) << 32)) & 1)
    while (x0 > 0 && SG_HIT(x0 - 1, sy)) x0--; while (x1 < vw - 1 && SG_HIT(x1 + 1, sy)) x1++;
    while (y0 > 0 && SG_HIT(sx, y0 - 1)) y0--; while (y1 < vh - 1 && SG_HIT(sx, y1 + 1)) y1++;
#undef SG_HIT
    float pt = vh / 375.0f, wp = (x1 - x0 + 1) / pt, hp = (y1 - y0 + 1) / pt, ex = wp < 44 ? (44 - wp) / 2 * pt : 0, ey = hp < 44 ? (44 - hp) / 2 * pt : 0;
    int up = strcmp(nm, "CancelButton") && strcmp(nm, "DefaultButton");
    int ok = x >= x0 - ex && x <= x1 + ex && y >= y0 - (up ? ey : 0) && y <= y1 + ey;
    LOG("吸附排除：%s 本体 %.1f×%.1f 点，触点离本体 横 %.1f 纵 %.1f 点，外扩 横 %.1f 纵 %.1f 点 → %s", nm, wp, hp, (x < x0 ? x0 - x : x > x1 ? x - x1 : 0) / pt, (y < y0 ? y0 - y : y > y1 ? y - y1 : 0) / pt, ex / pt, ey / pt, ok ? "算点中" : "不点");
    return ok;
}
static _Atomic uint64_t g_speech_end, g_speech_back_at, g_close_down;














static _Atomic int g_qt_arm, g_qt_sel, g_qt_fin, g_qt_speech;


static _Atomic uint64_t g_qt_dn, g_qt_xy, g_qt_stop_at;






static int quest_log_tap(module_t *m, uint64_t root, int x, int y) {
    uint64_t qp = ui_find(root, "QuestLogPanelExpansion", 1); if (!qp || !*(uint8_t *)(qp + 0x50)) return 0;
    uint64_t cd = atomic_load(&g_close_down); if (cd && dm_guest_frame() - cd < 60) return 0;
    int t = ui_top_cover(m, root, x, y); if (t < 0) return 0;
    uint64_t ch = *(uint64_t *)(root + 0x58), tp = *(uint64_t *)(ch + 8 * t); if (!tp) return 0;
    const char *nm = *(uint64_t *)(tp + 8) ? (const char *)*(uint64_t *)(tp + 8) : "";
    if (tp != qp && !ui_has(tp, qp, 0) && strcmp(nm, "QuestLogSpeechPanel") && strcmp(nm, "ControllerOverlay")) return 0;
    int sx = x, sy = y; return ui_snap(m, root, &sx, &sy) ? 1 : 2;
}
static _Atomic uint64_t g_set_back_at;
static _Atomic int g_tab_delta, g_tab_top, g_tab_left; static _Atomic uint64_t g_tab_mask; static _Atomic uint64_t g_tab_then;






static int npc_menu_rows(uint64_t root, uint64_t pn, int x, int y, int vh, int *delta);






static _Atomic int g_menu_want = -1;






static int g_menu_dismiss;
static int npc_menu_sel(uint64_t pn, uint64_t ct, int *cur) {
    module_t *m = main_module(); uint64_t ls = pn ? *(uint64_t *)(pn + 0x248) : 0; if (!m || !ls || *(uint64_t *)ls != (uint64_t)m->base + 0x20b8df0 || *(uint64_t *)(ls + 0xd0) != ct) return 0;
    int32_t c = *(int32_t *)(ls + 0x140); uint64_t n = *(uint64_t *)(ct + 0x60); *cur = c >= 0 && (uint64_t)c < n ? c : -1; return 1;
}
static int npc_menu(uint64_t root, int x, int y, int vh, int *delta) {
    atomic_store(&g_menu_want, -1);
    uint64_t pn = ui_find(root, "NpcDialogPanel", 0); if (!pn || !*(uint8_t *)(pn + 0x50)) return 0;
    int shop = dm_shop_open(), shown = has_visible_leaf(pn, 1);
    if (shop && !shown) return 0;
    g_menu_dismiss = 0;
    if (npc_menu_rows(root, pn, x, y, vh, delta)) { if (shop && g_menu_dismiss) *delta = 0; return 1; }




    if (shop) { *delta = 0; g_menu_dismiss = 1; return 1; }
    return 0;
}
static int npc_menu_rows(uint64_t root, uint64_t pn, int x, int y, int vh, int *delta) {
    (void)root;
    uint64_t ae = ui_find(pn, "ActiveElements", 0), ct = ae ? ui_find(ae, "Container", 0) : 0; if (!ct) return 0;
    uint64_t ch = *(uint64_t *)(ct + 0x58), n = *(uint64_t *)(ct + 0x60); if (!ch || !n || n > 32) return 0;
    int hit = -1, cur = -1; uint32_t col[32];
    for (uint64_t i = 0; i < n; i++) { uint64_t it = *(uint64_t *)(ch + 8 * i), tb = it && *(uint64_t *)(it + 0x60) ? *(uint64_t *)*(uint64_t *)(it + 0x58) : 0; col[i] = tb ? *(uint32_t *)(tb + 0xbc) : 0; }
    for (uint64_t i = 0; i < n; i++) { int same = 0; for (uint64_t j = 0; j < n; j++) same += col[j] == col[i]; if (same == 1 && n >= 3) cur = (int)i; }








    { int blue = -1, nb = 0; for (uint64_t i = 0; i < n; i++) { uint64_t it = *(uint64_t *)(ch + 8 * i), tb = it && *(uint64_t *)(it + 0x60) ? *(uint64_t *)*(uint64_t *)(it + 0x58) : 0;
          if (tb && *(uint32_t *)(tb + 0xc4) > *(uint32_t *)(tb + 0xbc)) { blue = (int)i; nb++; } }
      if (nb == 1) cur = blue; }




    int col_cur = cur, sel = -1, have = npc_menu_sel(pn, ct, &sel); if (have) cur = sel;


    static const int xoff[7] = { 0, 70, -70, 140, -140, 210, -210 };


    int used = 0;
    for (int q = 0; q < 7 && hit < 0; q++) for (int k = 0; k < 21 && hit < 0; k++) { int dy = (k + 1) / 2 * 6 * (k & 1 ? -1 : 1), yy = y + dy * vh / 2160, xx = x + xoff[q] * vh / 2160; if (xx < 0 || yy < 0) continue; uint64_t xy = (uint64_t)(uint32_t)xx | ((uint64_t)(uint32_t)yy << 32); used = dy;
        for (uint64_t i = 0; i < n && hit < 0; i++) { uint64_t it = *(uint64_t *)(ch + 8 * i); if (it && *(uint8_t *)(it + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)it + 0x48))(it, xy) & 1)) hit = (int)i; } }




    if (n == 1) { *delta = 0; g_menu_dismiss = 1; return 1; }
    if (hit < 0 || (cur < 0 && !have)) return 0;
    atomic_store(&g_menu_want, hit);
    { uint64_t hw = *(uint64_t *)(ch + 8 * hit); const char *hnm = hw && *(uint64_t *)(hw + 8) ? (const char *)*(uint64_t *)(hw + 8) : "?";
    LOG("界面对话菜单：%llu 行，点中第 %d 行（%s）%s，选中行 %d（%s%s）", (unsigned long long)n, hit, hnm, used ? "（触点在行外，就近）" : "", cur, have ? "菜单自己记的" : "【红】列表结构对不上，按文字颜色认的", have && col_cur != cur ? "；按颜色认会是另一行" : ""); }
    *delta = hit - cur; return 1;
}







static uint64_t modal_top(module_t *m, uint64_t root, const char **name) { (void)m; return modal_any(root, 0, name); }
static int ui_classify0(module_t *m, uint64_t root, int x, int y, char *hit, size_t hn);
static int swaptab_hit(uint64_t root, int x, int y); int dm_swapskill_wset(void); uint32_t dm_swapskill_itemsig(void); static _Atomic int g_swaptab_tap; static void fe_send(const char *cat, const char *act, const char *arg);






static int g_cls_out;


int dm_runeword_long_press(const dm_cursor_item_info *ci);
int dm_runeword_list_open(void);
void dm_runeword_list_close(void);
void dm_runeword_button(const char *name);




static int rw_hit(uint64_t w, uint64_t xy, int dep, const char **btn) {
    if (!w || dep > 5 || !*(uint8_t *)(w + 0x50)) return 0; const char *nm = *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "";
    int r = 0; if (!strncmp(nm, "TouchRw", 7) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x48))(w, xy) & 1)) { *btn = nm; r = 1; }
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return r;
    for (uint64_t i = 0; i < n; i++) if (rw_hit(*(uint64_t *)(ch + 8 * i), xy, dep + 1, btn)) r = 1;
    return r;
}


static void ui_show_ours(uint64_t w, int on) {
    const char *nm = w ? (const char *)*(uint64_t *)(w + 8) : NULL; if (!w) return;
    if (!nm || strncmp(nm, "Touch", 5)) { static int lg; if (lg++ < 8) LOG("【红】界面：要改可见的控件「%s」不是我们的（Touch 开头），不写", nm ? nm : "?"); return; }
    if (*(uint8_t *)(w + 0x50) != (uint8_t)on || *(uint8_t *)(w + 0x51) != (uint8_t)on) { *(uint8_t *)(w + 0x50) = (uint8_t)on; *(uint8_t *)(w + 0x51) = (uint8_t)on; }
}
static void ui_move_ours(uint64_t w, int x) {
    const char *nm = w ? (const char *)*(uint64_t *)(w + 8) : NULL; if (!w) return;
    if (!nm || strncmp(nm, "Touch", 5)) { static int lg; if (lg++ < 8) LOG("【红】界面：要挪的控件「%s」不是我们的，不写", nm ? nm : "?"); return; }
    int32_t *r = (int32_t *)(w + 0x70); if (r[0] != x) { r[0] = x; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); }
}
void dm_ui_move_ours(uint64_t w, int x) { ui_move_ours(w, x); }
#define PG_REACH 112
static int pg_personal(uint64_t root) {
    uint64_t bk = ui_panel(root, "BankExpansionLayout"), tabs = bk && *(uint8_t *)(bk + 0x50) ? ui_find(bk, "BankTabs", 0) : 0; return tabs && TAB_SEL(tabs) == 0;
}
static int pg_near(uint64_t root, int x, int y, int vh) {
    uint64_t bk = ui_panel(root, "BankExpansionLayout"); if (!bk || vh <= 0) return 0;
    static const int dx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, dy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
    for (int r = 16; r <= PG_REACH; r += 16) for (int k = 0; k < 2; k++) { uint64_t t = ui_find(bk, k ? "TouchStashRight" : "TouchStashLeft", 0); if (!t || !*(uint8_t *)(t + 0x50)) continue;
        uint64_t fn = *(uint64_t *)(*(uint64_t *)t + 0x48);
        for (int d = 0; d < 8; d++) { int xx = x + dx[d] * r * vh / 2160, yy = y + dy[d] * r * vh / 2160; if (xx < 0 || yy < 0) continue;
            if (((int (*)(uint64_t, uint64_t))fn)(t, (uint64_t)(uint32_t)xx | ((uint64_t)(uint32_t)yy << 32)) & 1) return k ? 1 : -1; } }
    return 0;
}
static int ui_classify(module_t *m, uint64_t root, int x, int y, char *hit, size_t hn) {
    if (g_cls_out) { g_cls_out = 0; const char *on = "";
        if (modal_top(m, root, &on)) { snprintf(hit, hn, "模态框外 %s", on); g_modal_name = on; return 9; }
        g_inside_owner = ""; atomic_store(&g_bd_hit, 0); atomic_store(&g_close_down, 0); snprintf(hit, hn, "（界面矩形外）"); return 3; }




    { int dm_looks_ui_over(int, int); if (dm_looks_ui_over(x, y)) { g_inside_owner = ""; atomic_store(&g_bd_hit, 0); snprintf(hit, hn, "appearance tab"); return 0; } }
    if (dm_ui_panel_state() & DM_PS_VAULT) { int dm_vault_ui_over(int, int); if (dm_vault_ui_over(x, y)) { g_inside_owner = ""; atomic_store(&g_bd_hit, 0); snprintf(hit, hn, "编年史页"); return 0; } }




    { uint64_t st = ui_panel(root, "SettingsPanel"), xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32);
      uint64_t cp = ui_panel(root, "VideoOptionsPanel");



      int on_sl = 0; if (cp && *(uint8_t *)(cp + 0x50)) { int ax = x, ay = y; g_click_exact = 1; on_sl = click_walk(cp, xy, 1, (uint64_t)m->base); g_click_exact = 0;
          if (!on_sl) on_sl = slider_arrow(m, cp, &ax, &ay, *(int32_t *)(m->base + 0x211da38), *(int32_t *)(m->base + 0x211da3c)); }
      if (st && *(uint8_t *)(st + 0x50) && !click_walk(st, xy, 1, (uint64_t)m->base) && !on_sl && !(cp && *(uint8_t *)(cp + 0x50) && click_walk(cp, xy, 1, (uint64_t)m->base))) {
          static const char *const pl[] = { "PauseLayoutGarden", "PauseLayout" };
          for (int i = 0; i < 2; i++) { uint64_t pw = ui_panel(root, pl[i]); if (pw && *(uint8_t *)(pw + 0x50) && click_walk(pw, xy, 1, (uint64_t)m->base)) { snprintf(hit, hn, "设置页盖着的暂停菜单按钮（不点，可滚）"); return 11; }                                                                                                                                                                                                                                                                                                                                                                                                                     } } }
    const char *mn = ""; uint64_t mp = modal_top(m, root, &mn);
    if (mp) { uint64_t xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32), fn = *(uint64_t *)(*(uint64_t *)mp + 0x48);




        int in = ((((int (*)(uint64_t, uint64_t))fn)(mp, xy) & 1) && !(((int (*)(uint64_t, uint64_t))fn)(mp, (uint64_t)3 | ((uint64_t)3 << 32)) & 1))
              || inside_walk(mp, xy, 1, (uint64_t)m->base + 0x20c1328) || click_walk(mp, xy, 1, (uint64_t)m->base);
        if (!in) { snprintf(hit, hn, "模态框外 %s", mn); g_modal_name = mn; return 9; } }




    { uint64_t ip = ui_find(root, "ImbueItemsPanel", 0), tc = ip && *(uint8_t *)(ip + 0x50) ? ui_find(ip, "TouchClose", 1) : 0;
      if (tc && *(uint8_t *)(tc + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)tc + 0x48))(tc, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1)) {
          g_inside_owner = ""; atomic_store(&g_bd_hit, 0); atomic_store(&g_close_down, 0); snprintf(hit, hn, "单件任务槽面板的关闭钮（按面板外算）"); return 3; } }
    if (dm_runeword_list_open()) {




        const char *bn = NULL; int rw = rw_hit(ui_find(root, "PlayerInventoryExpansionLayout", 0), (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32), 1, &bn);
        if (rw && bn) { dm_runeword_button(bn); snprintf(hit, hn, "符文之语清单 %s（宿主处理，不发鼠标点击）", bn); return 6; }
        if (!rw) { dm_runeword_list_close(); snprintf(hit, hn, "符文之语清单外 → 关清单"); return 6; } }
    if (swaptab_hit(root, x, y)) { atomic_store(&g_swaptab_tap, 1); snprintf(hit, hn, "技能树 I / II 页签 → 换武器"); return 6; }
    { uint64_t bk = ui_panel(root, "BankExpansionLayout"), tabs = bk && *(uint8_t *)(bk + 0x50) ? ui_find(bk, "BankTabs", 0) : 0;
      if (tabs && TAB_SEL(tabs) == 0) for (int k = 0; k < 2; k++) { uint64_t t = ui_find(bk, k ? "TouchStashRight" : "TouchStashLeft", 0);
          if (t && *(uint8_t *)(t + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)t + 0x48))(t, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1)) {
              void dm_pages_request_flip(int); dm_pages_request_flip(k ? 1 : -1); snprintf(hit, hn, "个人仓库%s一页", k ? "下" : "上"); return 6; } } }
    return ui_classify0(m, root, x, y, hit, hn);
}
static int ui_classify0(module_t *m, uint64_t root, int x, int y, char *hit, size_t hn) {
    uint64_t xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32); atomic_store(&g_bd_hit, 0); g_inside_owner = "";
    uint64_t rch = *(uint64_t *)(root + 0x58), rn = *(uint64_t *)(root + 0x60); if (!rch || rn > 256) return 3;
    g_iw_base = (uint64_t)m->base;
    int lo = ui_top_cover(m, root, x, y); if (lo < 0) lo = 0;


    if (dm_shop_open()) { int nd = 0; if (npc_menu(root, x, y, *(int32_t *)(m->base + 0x211da3c), &nd)) {
        if (g_menu_dismiss) { atomic_store(&g_tab_top, 4); snprintf(hit, hn, "商人页结果提示（按返回收）"); return 2; }
        atomic_store(&g_tab_delta, nd); atomic_store(&g_tab_top, 3); snprintf(hit, hn, "对话菜单 %+d 行", nd); return 2; } }
    for (int i = (int)rn - 1; i >= lo; i--) { uint64_t pw = *(uint64_t *)(rch + 8 * i); if (item_walk(pw, xy, 1, hit, hn)) { g_slot_panel = pw ? (const char *)*(uint64_t *)(pw + 8) : NULL; return 1; } }
    { int nd = 0; if (npc_menu(root, x, y, *(int32_t *)(m->base + 0x211da3c), &nd)) { atomic_store(&g_tab_delta, nd); atomic_store(&g_tab_top, 3); snprintf(hit, hn, "对话菜单 %+d 行", nd); return 2; } }
    int d = 0, top = 0, th = 0; { int32_t vh = *(int32_t *)(m->base + 0x211da3c); static const int off[5] = { 0, 45, -45, 90, -90 };




        int on_ctl = -1;
        for (int i = 0; i < 5 && !th; i++) { if (i == 1) { int qx = x, qy = y; if (on_ctl < 0) on_ctl = ui_snap(m, root, &qx, &qy) == 1; if (on_ctl) break; }
            int yy = y + off[i] * vh / 2160; if (yy < 0 || yy >= vh) continue; int tl = ui_top_cover(m, root, x, yy); if (tl < 0) tl = 0;
            for (int k = (int)rn - 1; k >= tl && !th; k--) th = tab_walk(*(uint64_t *)(rch + 8 * k), (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)yy << 32), 1, (uint64_t)m->base + 0x20ba3a8, &d, &top); } }
    if (th) { atomic_store(&g_tab_delta, d); atomic_store(&g_tab_top, top); snprintf(hit, hn, "页签 %+d%s", d, top ? "（顶部）" : ""); return 2; }




    { int dm_padmode_current(void); uint64_t sp = dm_padmode_current() ? ui_find(root, "SkillsTreePanel", 0) : 0;
      if (sp && *(uint8_t *)(sp + 0x50) && *(uint8_t *)(sp + 0x51)) { int tgt = -1, cur = 3; char nm[16];
          for (int k = 0; k < 4; k++) { snprintf(nm, sizeof nm, "TextTab%d", k); uint64_t tw = ui_find(sp, nm, 0);
              if (tw && *(uint8_t *)(tw + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)tw + 0x48))(tw, xy) & 1)) tgt = k;
              snprintf(nm, sizeof nm, "Tab%d", k); uint64_t tb = k < 3 ? ui_find(sp, nm, 0) : 0, bg = tb ? ui_find(tb, "Background", 0) : 0; if (bg && *(uint8_t *)(bg + 0x50)) cur = k; }
          if (tgt >= 0) { dm_wref_set(&g_tab_ref, 0); atomic_store(&g_tab_have, 0); atomic_store(&g_tab_delta, tgt - cur); atomic_store(&g_tab_top, 0); snprintf(hit, hn, "技能树分页 %+d", tgt - cur); return 2; } } }
    { int sx = x, sy = y, sn = ui_snap(m, root, &sx, &sy); if (sn == 6) { snprintf(hit, hn, "盖:%s 不点", g_inside_name); return 6; }
      if (sn == 1) { if (snap_guarded(root, g_click_name)) snap_guard_near(dm_wref_get(&g_click), g_click_name, x, y, sx, sy, *(int32_t *)(m->base + 0x211da38), *(int32_t *)(m->base + 0x211da3c));
          snprintf(hit, hn, " 可点控件 %s", g_click_name); bd_note_skill(m, root, sx, sy); return 0; }




      if (sn && (!strcmp(g_click_name, "TouchStashLeft") || !strcmp(g_click_name, "TouchStashRight")) && pg_personal(root)) {
          void dm_pages_request_flip(int); int dir = g_click_name[10] == 'R' ? 1 : -1; dm_pages_request_flip(dir); snprintf(hit, hn, "个人仓库%s一页（点偏，吸附）", dir > 0 ? "下" : "上"); return 6; }
      if (sn && snap_guarded(root, g_click_name) && !snap_guard_near(dm_wref_get(&g_click), g_click_name, x, y, sx, sy, *(int32_t *)(m->base + 0x211da38), *(int32_t *)(m->base + 0x211da3c))) { snprintf(hit, hn, "吸附到 %s：后果重的按钮只认本体（不足 44 点的外扩到 44 点见方），不点", g_click_name); return 6; }
      if (sn) { snprintf(hit, hn, " 吸附 %s (%d,%d)", g_click_name, sx, sy); bd_note_skill(m, root, sx, sy); return 0; }




      { int dir = pg_personal(root) ? pg_near(root, x, y, *(int32_t *)(m->base + 0x211da3c)) : 0;
        if (dir) { void dm_pages_request_flip(int); dm_pages_request_flip(dir); snprintf(hit, hn, "个人仓库%s一页（点偏，外圈）", dir > 0 ? "下" : "上"); return 6; } } }
    if (ui_inside_panel(m, root, xy)) { snprintf(hit, hn, " %s@%s", g_inside_name, g_inside_owner); atomic_store(&g_close_down, !strcmp(g_inside_name, "TouchClose") ? dm_guest_frame() : 0);
        if ((!strcmp(g_inside_name, "TouchClose") || !strcmp(g_inside_name, "close")) && atomic_load(&g_cube_side_on)) {
            static uint64_t lf; uint64_t f = dm_guest_frame(); if (f - lf > 30) { lf = f; atomic_store(&g_cube_side_close, 1); } snprintf(hit, hn, "并排的魔盒：关闭钮只关魔盒"); return 6; }
        return 0; }
    return 3;
}
void dm_ui_tab_go(void) {
    DM_LAT("tab_go");
    int d = atomic_load(&g_tab_delta), top = atomic_load(&g_tab_top);
    if (top == 4) { atomic_store(&g_set_back_at, dm_guest_frame() + 2); LOG("界面对话菜单：商人页结果提示 → 按返回键收掉"); return; }
    if (top == 3) { atomic_store(&g_tab_then, DM_BTN_A); atomic_store(&g_tab_mask, d > 0 ? DM_BTN_DOWN : DM_BTN_UP); atomic_store(&g_tab_left, d > 0 ? d : -d); LOG("界面对话菜单：方向键 %+d 行后按 A", d); return; }
    if (!d) return;






    atomic_store(&g_tab_unhover, 1);
    if (atomic_load(&g_tab_have)) { atomic_store(&g_tab_left, 0); atomic_store(&g_tab_direct, 1); LOG("界面页签：切 %+d 页 → 直接选中第 %d 个（%s）", d, g_tab_k, g_tab_name); return; }
    atomic_store(&g_tab_mask, d > 0 ? (top ? DM_BTN_R : DM_BTN_ZR) : (top ? DM_BTN_L : DM_BTN_ZL)); atomic_store(&g_tab_left, d > 0 ? d : -d);
    LOG("界面页签：切 %+d 页（%s）", d, top ? "L / R" : "ZL / ZR");
}


static int imbue_locked(module_t *m, uint64_t root) {
    uint64_t ip = m && root ? ui_find(root, "ImbueItemsPanel", 0) : 0; if (!ip || !*(uint8_t *)(ip + 0x50) || !has_visible_leaf(ip, 1)) return 0;
    dm_hud_info hi; dm_hud_get(&hi); if (!hi.valid || !hi.holding) return 0;
    return !(dm_wref_get(&g_slot) && *(uint64_t *)dm_wref_get(&g_slot) == (uint64_t)m->base + 0x20898e8);
}
int dm_ui_item_at_now(float nx, float ny) {
    module_t *m = main_module(); if (!m) return 0; uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8);
    int32_t w = *(int32_t *)(m->base + 0x211da38), h = *(int32_t *)(m->base + 0x211da3c); if (!root || w <= 0 || h <= 0) return 0;
    { int dm_vault_ui_tip_gate(int, int); if ((dm_ui_panel_state() & DM_PS_VAULT) && dm_vault_ui_tip_gate((int)(nx * w), (int)(ny * h))) return 6; }
    { int dm_pouch_tip_gate(int, int); if (dm_pouch_tip_gate((int)(nx * w), (int)(ny * h))) return 6; }
    char hit[40]; g_cls_out = UI_OUT(nx, ny); int r = ui_classify(m, root, (int)(nx * w), (int)(ny * h), hit, sizeof hit);
    if (r == 9) return 6;
    if (r == 1 && imbue_locked(m, root)) return 10;
    return r == 1 && g_slot_panel && !strcmp(g_slot_panel, "VendorPanelLayout") && dm_shop_open() ? 7 : r;
}








static _Atomic uint64_t g_drop_xy;
static void slot_center(uint64_t w, int *x, int *y, int vw, int vh, int cells) {
    uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48); int x0 = *x, x1 = *x, y0 = *y, y1 = *y;
#define SLOT_HIT(px, py) (((int (*)(uint64_t, uint64_t))fn)(w, (uint64_t)(uint32_t)(px) | ((uint64_t)(uint32_t)(py) << 32)) & 1)
    while (x0 > 2 && SLOT_HIT(x0 - 3, *y)) x0 -= 3; while (x1 < vw - 3 && SLOT_HIT(x1 + 3, *y)) x1 += 3;
    while (y0 > 2 && SLOT_HIT(*x, y0 - 3)) y0 -= 3; while (y1 < vh - 3 && SLOT_HIT(*x, y1 + 3)) y1 += 3;


    double cell = 112.5 * vh / 2160.0; int cols = (int)lround((x1 - x0) / cell), rows = (int)lround((y1 - y0) / cell);
    if (!cells) cols = rows = 1;




    if (cols > 1) { if (*x < x0 + 3) *x = x0 + 3; if (*x > x1 - 3) *x = x1 - 3; } else *x = (x0 + x1) / 2;
    if (rows > 1) { int r = (int)((*y - y0) / cell); if (r >= rows) r = rows - 1; if (r < 0) r = 0; if (*y < y0 + 3) *y = y0 + 3; if (*y > y1 - 3) *y = y1 - 3;
        g_dc = (drop_cell){ 1, *x, *y, (y1 - y0) / rows, r, rows }; } else { *y = (y0 + y1) / 2; g_dc.valid = 0; }
}
static int vt_hit_walk(uint64_t w, uint64_t xy, int dep, uint64_t vt) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    if (*(uint64_t *)w == vt) return ((int (*)(uint64_t, uint64_t))*(uint64_t *)(vt + 0x48))(w, xy) & 1;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) if (vt_hit_walk(*(uint64_t *)(ch + 8 * i), xy, dep + 1, vt)) return 1;
    return 0;
}


static _Atomic int g_drop_bag;
int dm_ui_drop_bag(void) { return atomic_load(&g_drop_bag); }
static int g_press_stack;
static _Atomic int g_vault_drop;
int dm_ui_vault_drop(void) { return atomic_load(&g_vault_drop); }




int dm_ui_bag_point(int x, int y, int *fx100, int *fy100, int *cols, int *rows) {
    module_t *m = main_module(); if (!m) return 0; uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8);
    int32_t vw = *(int32_t *)(m->base + 0x211da38), vh = *(int32_t *)(m->base + 0x211da3c); uint64_t pn = root ? ui_panel(root, "PlayerInventoryExpansionLayout") : 0, g = pn ? ui_find(pn, "grid", 0) : 0;
    if (!g || !*(uint8_t *)(g + 0x50) || vw <= 0 || vh <= 0 || x < 0 || y < 0 || x >= vw || y >= vh) return 0;
    uint64_t fn = *(uint64_t *)(*(uint64_t *)g + 0x48);
#define BAG_HIT(px, py) (((int (*)(uint64_t, uint64_t))fn)(g, (uint64_t)(uint32_t)(px) | ((uint64_t)(uint32_t)(py) << 32)) & 1)
    if (!BAG_HIT(x, y)) return 0;
    int a, b, x0, x1, y0, y1;
    for (a = 0, b = x; a < b;) { int mid = (a + b) / 2; if (BAG_HIT(mid, y)) b = mid; else a = mid + 1; } x0 = a;
    for (a = x, b = vw - 1; a < b;) { int mid = (a + b + 1) / 2; if (BAG_HIT(mid, y)) a = mid; else b = mid - 1; } x1 = a;
    for (a = 0, b = y; a < b;) { int mid = (a + b) / 2; if (BAG_HIT(x, mid)) b = mid; else a = mid + 1; } y0 = a;
    for (a = y, b = vh - 1; a < b;) { int mid = (a + b + 1) / 2; if (BAG_HIT(x, mid)) a = mid; else b = mid - 1; } y1 = a;
#undef BAG_HIT
    double cell = 112.5 * vh / 2160.0; *cols = (int)lround((x1 - x0 + 1) / cell); *rows = (int)lround((y1 - y0 + 1) / cell);
    *fx100 = (int)lround((x - x0) * 100.0 / cell); *fy100 = (int)lround((y - y0) * 100.0 / cell); return *cols > 0 && *rows > 0;
}




static int merc_lock_hit(const char *hit) {
    if (!dm_wref_get(&g_slot) || !hit || strncmp(hit, "slot_", 5) || *(uint32_t *)(dm_wref_get(&g_slot) + 0x5c8) != 1) return 0; int l_ = *(int32_t *)(dm_wref_get(&g_slot) + 0x5d8); int dm_rift_merc_unlocked(void); void dm_rift_merc_locked_hint(void);
    if ((l_ == 2 || (l_ >= 6 && l_ <= 10)) && !dm_rift_merc_unlocked()) { dm_rift_merc_locked_hint(); return 1; }
    return 0;
}
static int ui_drop_kind(module_t *m, uint64_t root, int x, int yl, int yf) {


    (void)yf; atomic_store(&g_drop_bag, 0); int32_t vw = *(int32_t *)(m->base + 0x211da38), vh = *(int32_t *)(m->base + 0x211da3c);
    char hit[40]; int px = x, py = yl;
    int c = ui_classify(m, root, x, yl, hit, sizeof hit);
    if (c == 9) return 0;




    if (dm_ui_panel_state() & DM_PS_VAULT) { int dm_vault_ui_over(int, int); uint32_t dm_vault_ui_held(void); uint32_t id = dm_vault_ui_held(); int v = id && dm_vault_ui_over(x, yl);
        atomic_store(&g_vault_drop, v ? (int)id : 0); if (v) return 0; } else atomic_store(&g_vault_drop, 0);






    { uint64_t ip = ui_find(root, "ImbueItemsPanel", 0);
      if (ip && *(uint8_t *)(ip + 0x50)) {


          static const int ox[5] = { 0, -70, 70, 0, 0 }, oy[5] = { 0, 0, 0, -70, 70 };
          for (int k = 0; k < 5 && vh > 0; k++) { int xx = x + ox[k] * vh / 2160, yy = yl + oy[k] * vh / 2160; if (xx < 0 || yy < 0) continue;
              if (vt_hit_walk(ip, (uint64_t)(uint32_t)xx | ((uint64_t)(uint32_t)yy << 32), 0, (uint64_t)m->base + 0x20898e8)) return 8; }


          uint64_t bg = ui_find(ip, "Background", 0);
          if (bg && *(uint8_t *)(bg + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)bg + 0x48))(bg, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)yl << 32)) & 1)) return 0; } }








    if (c != 1 && dm_shop_open()) { uint64_t vp = ui_find(root, "VendorPanelLayout", 0), xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)yl << 32);
        if (vp && *(uint8_t *)(vp + 0x50)) { static const char *part[3] = { "background", "background_repair", "tabs" };
            for (int k = 0; k < 3; k++) { uint64_t w = ui_find(vp, part[k], 0); if (w && *(uint8_t *)(w + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x48))(w, xy) & 1)) return 7; } } }
    if (c != 1) return c == 3 || (c == 0 && g_inside_owner[0] && !in_item_panel_rect(m, root, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)yl << 32))) ? 3 : 0;
    if (belt_row_block(m, x, yl)) return 0;
    if (g_slot_panel && !strcmp(g_slot_panel, "VendorPanelLayout") && dm_shop_open()) return 7;
    if (merc_lock_hit(hit)) { atomic_store(&g_drop_loc, 0); return 0; }
    atomic_store(&g_drop_loc, dm_wref_get(&g_slot) && !strncmp(hit, "slot_", 5) ? (*(int32_t *)(dm_wref_get(&g_slot) + 0x5d8) | (*(uint32_t *)(dm_wref_get(&g_slot) + 0x5c8) == 1 ? 0x100 : 0)) : 0);
    if (dm_wref_get(&g_slot)) slot_center(dm_wref_get(&g_slot), &px, &py, vw, vh, strncmp(hit, "slot_", 5) != 0);
    atomic_store(&g_drop_xy, (uint64_t)(uint32_t)px | ((uint64_t)(uint32_t)py << 32));
    atomic_store(&g_drop_bag, !strcmp(hit, "grid") && g_slot_panel && !strcmp(g_slot_panel, "PlayerInventoryExpansionLayout"));
    return 5;
}






int dm_ui_drop_last(void) { return atomic_load(&g_drop_last); }
int dm_ui_drop_point(float *nx, float *ny) {
    module_t *m = main_module(); if (!m) return 0; int32_t vw = *(int32_t *)(m->base + 0x211da38), vh = *(int32_t *)(m->base + 0x211da3c); if (vw <= 0 || vh <= 0) return 0;
    uint64_t v = atomic_load(&g_drop_xy); *nx = (float)((v & 0xffffffff) + 0.5) / vw; *ny = (float)((v >> 32) + 0.5) / vh; return 1;
}
static _Atomic uint32_t g_iq_y2;
unsigned dm_ui_drop_query(float nx, float ny_lifted, float ny_finger) {
    ny_finger = ny_finger < 0 ? 0 : ny_finger > 1 ? 1 : ny_finger; atomic_store(&g_iq_y2, (uint32_t)(ny_finger * 65535 + .5f)); atomic_store(&g_iq_drop, 1);
    return dm_ui_item_query(nx, ny_lifted);
}
int dm_ui_drop_at_now(float nx, float ny_lifted, float ny_finger) {
    module_t *m = main_module(); if (!m) return 1; uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8);
    int32_t w = *(int32_t *)(m->base + 0x211da38), h = *(int32_t *)(m->base + 0x211da3c); if (!root || w <= 0 || h <= 0) return 1;
    g_cls_out = UI_OUT(nx, ny_lifted); return ui_drop_kind(m, root, (int)(nx * w), (int)(ny_lifted * h), (int)(ny_finger * h));
}
static _Atomic uint64_t g_buy_at;
void dm_ui_shop_buy_at(float nx, float ny) {
    nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny;
    atomic_store(&g_buy_at, (1ull << 63) | ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f));
}


void dm_ui_stack_take_at(float nx, float ny) {
    nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny;
    atomic_store(&g_buy_at, (3ull << 62) | ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f));
}


void dm_ui_stack_cube_at(float nx, float ny) {
    nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny;
    atomic_store(&g_buy_at, (7ull << 61) | ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f));
}
static void ui_buy_pump(int width, int height) {
    uint64_t v = atomic_exchange(&g_buy_at, 0); if (!(v >> 63)) return;
    int x = (int)(((v >> 16) & 0xffff) / 65535.0 * width), y = (int)((v & 0xffff) / 65535.0 * height);


    int take = v >> 62 & 1; dm_cursor_item_info ci;
    if (!dm_ui_hover_now(x, y)) { LOG("【红】界面拖%s：此刻不是手柄光标模式，光标放不回 (%d,%d) → 不%s", take ? "取" : "买", x, y, take ? "取" : "买"); return; }
    if (!dm_cursor_item(&ci) || ci.container != (take ? DM_ITEM_AT_STASH : DM_ITEM_AT_SHOP)) { LOG("【红】界面拖%s：光标放回 (%d,%d) 后选中的不是%s → 不%s", take ? "取" : "买", x, y, take ? "储藏箱里的一叠" : "货架上的货", take ? "取" : "买"); return; }
    if (v >> 61 & 1) { dm_ui_key_pulse(DM_BTN_X, 6); LOG("界面拖放：一叠松在魔盒格上 → 光标回那一叠（%d,%d），按 X", x, y); }
    else if (take) { void dm_stash_take(int); dm_stash_take(1); } else dm_shop_buy();
}




static _Atomic int g_tap_req, g_targeting_pub;
void dm_ui_item_tap(void) { atomic_store(&g_tap_req, 1); void dm_tipx_tap(void); dm_tipx_tap(); }
int dm_ui_targeting(void) { return atomic_load(&g_targeting_pub); }
static _Atomic int g_tap_box;
int dm_ui_tap_box(void) { return atomic_load(&g_tap_box); }






static _Atomic uint32_t g_cur_stack_id;




static _Atomic uint32_t g_hover_unid;
uint32_t dm_ui_hover_unid(void) { return atomic_load(&g_hover_unid); }
static _Atomic uint32_t g_ident_req;
void dm_ui_identify_hovered(void) { atomic_store(&g_ident_req, atomic_load(&g_hover_unid)); }
uint32_t dm_ui_cur_stack_id(void) { return atomic_load(&g_cur_stack_id); }




static _Atomic int g_stack_left = -1, g_stack_want;
int dm_ui_stack_left(void) { return atomic_load(&g_stack_left); }
static void ui_target_pump(module_t *m, uint64_t fr) {
    if (atomic_load(&g_stack_want)) { dm_cursor_item_info si; if (dm_cursor_item(&si) && si.container == DM_ITEM_AT_STASH && si.stack > 0) { atomic_store(&g_stack_left, si.stack); atomic_store(&g_stack_want, 0); } }
    int tg = dm_item_targeting() != 0; atomic_store(&g_targeting_pub, tg);
    static uint64_t check_at; int tap = atomic_exchange(&g_tap_req, 0);
    if (tap) { dm_cursor_item_info bi; atomic_store(&g_tap_box, dm_cursor_item(&bi) && !strcmp(bi.code, "box")); }
    if (tap) { int dm_looks_ui_item_tap(void); dm_looks_ui_item_tap(); }
    { dm_cursor_item_info ui; int c = dm_cursor_item(&ui) ? ui.container : 0;
      atomic_store(&g_hover_unid, c && ui.unid && (c == DM_ITEM_AT_INVENTORY || c == DM_ITEM_AT_STASH || c == DM_ITEM_AT_CUBE || c == DM_ITEM_AT_EQUIP) ? ui.id : 0);









      { int dm_padmode_current(void); uint64_t dm_padmode_page_keys(uint64_t), dm_padmode_page_held(void); void dm_padmode_page_pulse(uint64_t), dm_padmode_ident_hint(int);
        static uint64_t t0; static int fired; uint32_t hu = atomic_load(&g_hover_unid); dm_hud_info hh; dm_hud_get(&hh);
        if (dm_padmode_current() && hu && !(hh.valid && hh.holding)) { uint64_t ev = dm_padmode_page_keys(DM_BTN_A), now = now_ns(); int held = (dm_padmode_page_held() & DM_BTN_A) != 0; dm_padmode_ident_hint(1);
            if (ev & DM_BTN_A) { t0 = now; fired = 0; }
            if (held && t0 && !fired && now - t0 >= 500000000ull) { fired = 1; atomic_store(&g_ident_req, hu); LOG("免费鉴定：手柄按住 A 0.5 秒，光标下未鉴定物品 id %u → 鉴定", hu); }
            if (!held && t0) { if (!fired) { dm_padmode_page_pulse(DM_BTN_A); LOG("免费鉴定：手柄短按 A → 还给原版（捡起）"); } t0 = 0; } }
        else t0 = 0; }
      uint32_t rq = atomic_exchange(&g_ident_req, 0); if (rq) { void dm_item_identify_free(uint32_t); dm_item_identify_free(rq); } }
    if (dm_ui_panel_state() & DM_PS_BANK) { dm_cursor_item_info ci; atomic_store(&g_cur_stack_id, dm_cursor_item(&ci) && ci.container == DM_ITEM_AT_INVENTORY ? ci.id : 0); } else atomic_store(&g_cur_stack_id, 0);
    if (tap && !tg && dm_vendor_repair_mode()) { dm_cursor_item_info ri; if (dm_cursor_item(&ri)) { dm_item_repair(ri.id); LOG("界面修理：修理态下单击 %s（id %u）→ 修理", ri.code, ri.id); } }
    if (!tg) { check_at = 0; return; }
    if (!free_cursor_page(m)) { dm_item_target_cancel(); LOG("界面鉴定：面板关了，退出鉴定态"); return; }
    if (check_at && fr >= check_at) { check_at = 0; dm_item_target_cancel(); LOG("界面鉴定：点的那件鉴不了（状态没变），退出鉴定态"); return; }
    if (!tap) return;
    dm_cursor_item_info ci;
    if (dm_cursor_item(&ci)) { dm_item_identify(ci.id); check_at = fr + 24; LOG("界面鉴定：单击 %s（id %u）→ 鉴定", ci.code, ci.id); }
    else { dm_item_target_cancel(); LOG("界面鉴定：点在空格上，退出鉴定态"); }
}
static _Atomic int g_use_req;
void dm_ui_use_item(void) { atomic_store(&g_use_req, 1); }


static int finger_item(dm_cursor_item_info *ci, const char *who) {
    int has = dm_cursor_item(ci);




    if (has && sn_on) { uint64_t g = atomic_load(&g_snap); int tx = g & 0xffff, ty = g >> 16 & 0xffff, px = g >> 32 & 0xffff, py = g >> 48 & 0xffff; dm_cursor_item_info c2;
        int near = dm_ui_hover_now(tx + (px - tx) / 2, ty + (py - ty) / 2) && dm_cursor_item(&c2) && c2.id == ci->id; dm_ui_hover_now(px, py);
        if (!near) has = 0; LOG("界面%s：光标是吸附到 %s 上的（请求 %d,%d → 落点 %d,%d）；四分之一格内%s", who, ci->code, tx, ty, px, py, near ? "够得着 → 按物品算" : "够不着 → 按空格算"); }
    return has;
}
static int belt_widget(uint64_t w) { return w && g_iw_base && *(uint64_t *)(*(uint64_t *)w + 0x28) == g_iw_base + 0x3911d8; }
static void ui_use_pump(void) {
    if (!atomic_exchange(&g_use_req, 0)) return;
    dm_cursor_item_info ci;


    if (belt_widget(dm_wref_get(&g_slot)) && !finger_item(&ci, "双击")) { dm_belt_fill(); LOG("界面双击：腰带空格 → 一键补满"); return; }
    if (!dm_cursor_item(&ci)) { LOG("界面双击：光标下没有物品，不动"); return; }
    if ((dm_ui_panel_state() & DM_PS_VAULT) && ci.container == DM_ITEM_AT_INVENTORY) { void dm_vault_request_deposit(uint32_t); dm_vault_request_deposit(ci.id); LOG("界面双击：编年史页开着，背包的 %s（id %u）→ 存进编年史仓库", ci.code, ci.id); return; }
    if (ci.container == DM_ITEM_AT_SHOP) { dm_shop_buy(); LOG("界面双击：货架上的 %s → 买", ci.code); }
    else if (ci.container == DM_ITEM_AT_EQUIP) { dm_item_unequip(ci.id); LOG("界面双击：身上的 %s（id %u）→ 卸到背包", ci.code, ci.id); }
    else if (ci.equippable) { dm_item_equip(ci.id); LOG("界面双击：%s（id %u）能装备 → 装上", ci.code, ci.id); }
    else if (!strcmp(ci.code, "box") && (dm_ui_panel_state() & DM_PS_BANK)) { atomic_store(&g_cube_side_req, 1); LOG("界面双击：储藏箱开着，魔盒（id %u）→ 并排打开（不走原版使用：那条路会先关储藏箱）", ci.id); }
    else { dm_item_use(ci.id); LOG("界面双击：%s（id %u）→ 使用", ci.code, ci.id); }
    DM_LAT("use");
}
static _Atomic int g_long_req;
void dm_ui_long_press(void) { atomic_store(&g_long_req, 1); }
static void ui_long_pump(void) {
    if (!atomic_exchange(&g_long_req, 0)) return;


    if (belt_widget(dm_wref_get(&g_slot))) { LOG("界面长按：腰带 → 不动（补满改成空格双击）"); return; }
    dm_cursor_item_info ci; int has = finger_item(&ci, "长按");
    if (!has) {


        const char *nm = dm_wref_get(&g_slot) ? (const char *)*(uint64_t *)(dm_wref_get(&g_slot) + 8) : NULL, *pn = g_slot_panel; char chain[160] = ""; int page = -1, grid = nm && (!strcmp(nm, "grid") || !strcmp(nm, "horadriccube_grid") || !strncmp(nm, "advancedstash_", 14) || !strcmp(nm, "basicstash_container"));
        if (grid && !(pn && !strcmp(pn, "VendorPanelLayout"))) { page = 0;
            for (uint64_t w = dm_wref_get(&g_slot); w && strlen(chain) < 120; w = *(uint64_t *)(w + 0x30)) { const char *n = *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "?"; strlcat(chain, n, sizeof chain); strlcat(chain, "<", sizeof chain);
                if (page == 0 && (strcasestr(n, "stash") || strcasestr(n, "bank")) && strcmp(n, "BankExpansionLayout") && strcmp(n, "BankOriginalLayout")) page = 4;
                if (page == 0 && strcasestr(n, "cube")) page = 3; } }
        if (page >= 0) { void dm_inv_sort(int); dm_inv_sort(page); }
        LOG("界面长按：空格（控件链 %s，面板 %s）→ %s", chain[0] ? chain : (nm ? nm : "?"), pn ? pn : "?", page < 0 ? "不动" : page == 0 ? "整理背包" : page == 3 ? "整理方块" : "整理储藏箱");
        return; }
    if (ci.container == DM_ITEM_AT_STASH && ci.stack > 0) { void dm_stash_take(int); dm_stash_take(3); LOG("界面长按：高级仓库里的一叠 %s（%d 件）→ 取 3 件到背包", ci.code, ci.stack); return; }
    if (!strcmp(ci.code, "box") && ci.container == DM_ITEM_AT_INVENTORY && (dm_ui_panel_state() & DM_PS_BANK)) { atomic_store(&g_cube_side_req, 1); LOG("界面长按：储藏箱开着，魔盒（id %u）→ 并排打开", ci.id); return; }
    if (ci.container == DM_ITEM_AT_INVENTORY && ci.belt_room) { dm_belt_put(ci.id); LOG("界面长按：背包里的 %s（id %u）→ 腰带", ci.code, ci.id); }
    else if (dm_runeword_long_press(&ci)) { g_fb_nudge = 1; LOG("界面长按：%s → 符文之语清单（光标送停靠点，不挂说明框）", ci.code); }
    else LOG("界面长按：%s（id %u，位置 %d，可进腰带 %d，腰带有位 %d）不动", ci.code, ci.id, ci.container, ci.beltable, ci.belt_room);
}
static _Atomic uint64_t g_shelf_box, g_shelf_pt, g_shelf_vp; static _Atomic unsigned g_shelf_seq;




int dm_ui_item_rect(unsigned seq, float r[4]) {
    if (atomic_load(&g_shelf_seq) != seq) return 0;
    uint64_t b = atomic_load(&g_shelf_box), p = atomic_load(&g_shelf_pt), v = atomic_load(&g_shelf_vp);
    int x0 = b & 0xffff, y0 = (b >> 16) & 0xffff, x1 = (b >> 32) & 0xffff, y1 = (b >> 48) & 0xffff, px = (int)(uint32_t)p, py = (int)(p >> 32), vw = (int)(uint32_t)v, vh = (int)(v >> 32);
    if (vw <= 0 || vh <= 0 || x1 <= x0 || y1 <= y0) return 0;
    dm_shop_info si; dm_shop_get(&si); if (!si.open || !si.sel_valid || si.sel_gw <= 0 || si.sel_gh <= 0) return 0;
    double cell = 112.5 * vh / 2160.0; int cols = (int)lround((x1 - x0) / cell), rows = (int)lround((y1 - y0) / cell); if (cols < 1 || rows < 1) return 0;
    double cw = (double)(x1 - x0) / cols, chh = (double)(y1 - y0) / rows;
    double rx0 = x0 + si.sel_gx * cw, ry0 = y0 + si.sel_gy * chh, rx1 = rx0 + si.sel_gw * cw, ry1 = ry0 + si.sel_gh * chh;
    if (px < rx0 - 2 || px > rx1 + 2 || py < ry0 - 2 || py > ry1 + 2) return 0;
    r[0] = (float)(rx0 / vw); r[1] = (float)(ry0 / vh); r[2] = (float)(rx1 / vw); r[3] = (float)(ry1 / vh); return 1;
}


int dm_ui_shelf_grid(float g[4]) {
    uint64_t b = atomic_load(&g_shelf_box), v = atomic_load(&g_shelf_vp);
    int x0 = b & 0xffff, y0 = (b >> 16) & 0xffff, x1 = (b >> 32) & 0xffff, y1 = (b >> 48) & 0xffff, vw = (int)(uint32_t)v, vh = (int)(v >> 32);
    if (vw <= 0 || vh <= 0 || x1 <= x0 || y1 <= y0) return 0;
    double cell = 112.5 * vh / 2160.0; int cols = (int)lround((x1 - x0) / cell), rows = (int)lround((y1 - y0) / cell); if (cols < 1 || rows < 1) return 0;
    g[0] = (float)x0 / vw; g[1] = (float)y0 / vh; g[2] = (float)(x1 - x0) / cols / vw; g[3] = (float)(y1 - y0) / rows / vh; return 1;
}
static void ui_item_pump(uint64_t root, int width, int height) {
    if (dm_lat_on()) { static unsigned ls; unsigned ds = atomic_load(&g_iq_done); float rr[4]; if (ds != ls && dm_ui_item_rect(ds, rr)) { ls = ds; DM_LAT("shelf_rect %d,%d-%d,%d", (int)(rr[0] * width), (int)(rr[1] * height), (int)(rr[2] * width), (int)(rr[3] * height)); } }
    unsigned seq = atomic_load(&g_iq_seq); if (seq == atomic_load(&g_iq_done)) return;
    uint64_t v = atomic_load(&g_iq_xy); int x = (int)(((v >> 16) & 0xffff) / 65535.0 * width), y = (int)((v & 0xffff) / 65535.0 * height);
    char hit[40] = ""; int r; g_cls_out = atomic_load(&g_iq_out);
    if (atomic_exchange(&g_iq_drop, 0)) { r = ui_drop_kind(main_module(), root, x, y, (int)(atomic_load(&g_iq_y2) / 65535.0 * height)); 
        if (r == 5 && g_press_stack && dm_wref_get(&g_slot) && *(uint64_t *)(dm_wref_get(&g_slot) + 8) && !strcmp((const char *)*(uint64_t *)(dm_wref_get(&g_slot) + 8), "horadriccube_grid")) r = 11;
        snprintf(hit, sizeof hit, "拖放落点类 %d", r); atomic_store(&g_iq_res, r); atomic_store(&g_drop_last, r); atomic_store(&g_iq_done, seq);
        if (hit_log_ok(hit)) LOG("界面命中：(%d,%d) → %s", x, y, hit); return; }
    { int dm_pouch_tip_gate(int, int); if (dm_pouch_tip_gate(x, y)) { r = 6; atomic_store(&g_iq_res, r); atomic_store(&g_iq_done, seq); return; } }
    { int dm_vault_ui_tip_gate(int, int); if ((dm_ui_panel_state() & DM_PS_VAULT) && dm_vault_ui_tip_gate(x, y)) { r = 6; atomic_store(&g_iq_res, r); atomic_store(&g_iq_done, seq); return; } }
    r = ui_classify(main_module(), root, x, y, hit, sizeof hit);
    if (r == 9) {
        if (!strcmp(g_modal_name, "DropGoldModal")) { atomic_store(&g_set_back_at, dm_guest_frame() + 2); LOG("界面命中：(%d,%d) → 模态框外，按返回取消 %s", x, y, g_modal_name); }
        else LOG("界面命中：(%d,%d) → 模态框外（%s），这一下不处理", x, y, g_modal_name);
        r = 6; atomic_store(&g_iq_res, r); atomic_store(&g_iq_done, seq); return; }
    if (r != 1 && r != 7 && dm_item_targeting()) { dm_item_target_cancel(); LOG("界面鉴定：点在物品区之外，退出鉴定态"); }
    if (r == 1) dm_wref_set(&g_down, dm_wref_get(&g_slot));
    { dm_hud_info hh; dm_hud_get(&hh); if (r == 0 && hh.valid && hh.holding && g_inside_owner[0] && !in_item_panel_rect(main_module(), root, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32))) r = 3; }
    if (r == 1 && dm_wref_get(&g_slot) && g_iw_base && *(uint64_t *)dm_wref_get(&g_slot) - g_iw_base == 0x20892f8) { r = 8; atomic_store(&g_exact_xy, (uint64_t)(uint32_t)x | (uint64_t)(uint32_t)y << 32); atomic_store(&g_stack_left, -1); atomic_store(&g_stack_want, 1); }
    g_press_stack = r == 8;
    if (r == 1 && imbue_locked(main_module(), root)) r = 10;
    if (r == 1 && g_slot_panel && !strcmp(g_slot_panel, "VendorPanelLayout") && dm_shop_open()) r = 7;
    if ((r == 7 || r == 8) && dm_wref_get(&g_slot)) {
        uint64_t fn = *(uint64_t *)(*(uint64_t *)dm_wref_get(&g_slot) + 0x48); int x0 = x, x1 = x, y0 = y, y1 = y;
#define SHELF_HIT(px, py) (((int (*)(uint64_t, uint64_t))fn)(dm_wref_get(&g_slot), (uint64_t)(uint32_t)(px) | ((uint64_t)(uint32_t)(py) << 32)) & 1)
        while (x0 > 1 && SHELF_HIT(x0 - 2, y)) x0 -= 2; while (x1 < width - 2 && SHELF_HIT(x1 + 2, y)) x1 += 2;
        while (y0 > 1 && SHELF_HIT(x, y0 - 2)) y0 -= 2; while (y1 < height - 2 && SHELF_HIT(x, y1 + 2)) y1 += 2;
        atomic_store(&g_shelf_box, (uint64_t)(uint16_t)x0 | ((uint64_t)(uint16_t)y0 << 16) | ((uint64_t)(uint16_t)x1 << 32) | ((uint64_t)(uint16_t)y1 << 48));
        atomic_store(&g_shelf_pt, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32) ); atomic_store(&g_shelf_vp, (uint64_t)(uint32_t)width | ((uint64_t)(uint32_t)height << 32)); atomic_store(&g_shelf_seq, seq); }




    if (r == 1 && dm_wref_get(&g_slot) && !strncmp(hit, "slot_", 5) && *(uint32_t *)(dm_wref_get(&g_slot) + 0x5c8) == 1) { dm_hud_info hh; dm_hud_get(&hh); int dm_rift_merc_slot_empty(int);
        if (!(hh.valid && hh.holding) && (merc_lock_hit(hit) || dm_rift_merc_slot_empty(*(int32_t *)(dm_wref_get(&g_slot) + 0x5d8)))) { r = 6; snprintf(hit + strlen(hit), sizeof hit - strlen(hit), "（佣兵空格）"); } }
    atomic_store(&g_iq_res, r); atomic_store(&g_iq_done, seq);
    if (r == 1) merc_lock_hit(hit);
    if (hit_log_ok(hit)) { LOG("界面命中：(%d,%d) → %s%s", x, y, r == 1 ? "物品区 " : r == 10 ? "物品区（单件槽占用，不动）" : r == 8 ? "成叠格 " : r == 7 ? "货架 " : r == 2 ? "" : r == 3 ? "面板外" : r == 6 ? "" : "面板内控件", hit); }
}
static void ui_ptr_pump(module_t *m, uint64_t root, int width, int height) {
    unsigned r = atomic_load(&g_up_r); int pend = r != atomic_load(&g_up_w), unhover = atomic_exchange(&g_tab_unhover, 0);
    if (unhover) { atomic_store(&g_up_xy, atomic_load(&g_iq_xy)); atomic_store(&g_up_hold, 0); }
    if (!atomic_load(&g_up_hold) && !pend && !unhover) return;
    static uint64_t hs[10]; if (!hs[0]) { static const char *nm[10] = { "MouseMove", "", "", "LeftMouseButtonDown", "LeftMouseButtonUp", "RightMouseButtonDown", "RightMouseButtonUp", "MouseScroll", "KeyDown", "KeyUp" }; for (int i = 0; i < 10; i++) hs[i] = ui_hash(nm[i]); }
    uint64_t v = atomic_load(&g_up_xy); int x = (int)(((v >> 16) & 0xffff) / 65535.0 * width), y = (int)((v & 0xffff) / 65535.0 * height); if (x >= width) x = width - 1; if (y >= height) y = height - 1;
    int dm_vault_ui_blocks(int, int), vb = (dm_ui_panel_state() & DM_PS_VAULT) && dm_vault_ui_blocks(x, y);
    if (!vb) ui_send_mouse(m, root, hs[0], x, y);
    if (!pend) return;
    int k = g_up_q[r % 32].kind, a = g_up_q[r % 32].arg; atomic_store(&g_up_r, r + 1);
    if (k == DM_UP_END) atomic_store(&g_up_hold, 0); else if (k >= DM_UP_LDOWN && k <= DM_UP_KEYUP && !vb) { ui_send_mouse_arg(m, root, hs[k], x, y, a); LOG("界面指针：事件 %d 参数 %d @(%d,%d)", k, a, x, y); }
}
















#define UI_SCROLL_STEP(h) ((h) * 32 / 900 > 4 ? (h) * 32 / 900 : 4)
static _Atomic int g_dg_kind; static _Atomic uint64_t g_dg_o, g_dg_c; static _Atomic unsigned g_dg_seq; static _Atomic uint32_t g_dg_tol;
static uint64_t dg_pack(float nx, float ny) { nx = nx < 0 ? 0 : nx > 1 ? 1 : nx; ny = ny < 0 ? 0 : ny > 1 ? 1 : ny; return ((uint64_t)(uint32_t)(nx * 65535 + .5f) << 16) | (uint32_t)(ny * 65535 + .5f); }
void dm_ui_drag_begin(int kind, float ox, float oy, float tol_ny) {
    uint64_t o = dg_pack(ox, oy); atomic_store(&g_dg_o, o); atomic_store(&g_dg_c, o); atomic_store(&g_dg_tol, (uint32_t)((tol_ny < 0 ? 0 : tol_ny > 0.5f ? 0.5f : tol_ny) * 65535));
    atomic_store(&g_dg_kind, kind == 2 ? 2 : 1); atomic_fetch_add(&g_dg_seq, 1);
}
void dm_ui_drag_move(float x, float y) { if (atomic_load(&g_dg_kind)) atomic_store(&g_dg_c, dg_pack(x, y)); }
void dm_ui_drag_end(void) { atomic_store(&g_dg_kind, 0); }
static _Atomic uint32_t g_dg_lift;
void dm_ui_drag_lift(float ny) { atomic_store(&g_dg_lift, (uint32_t)((ny < 0 ? 0 : ny > 0.5f ? 0.5f : ny) * 65535)); }
float dm_ui_drag_lift_get(void) { return atomic_load(&g_dg_lift) / 65535.0f; }
static int dg_hit(uint64_t w, int x, int y) { uint64_t fn = *(uint64_t *)(*(uint64_t *)w + 0x48); return ((int (*)(uint64_t, uint64_t))fn)(w, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1; }
static void dg_sliders(uint64_t w, uint64_t vt, int dep, uint64_t *out, uint64_t *clips, uint64_t clip, int *n) {
    if (!w || dep > 16 || *n >= 48 || !*(uint8_t *)(w + 0x50)) return;
    if (*(uint64_t *)w == vt) { clips[*n] = clip; out[(*n)++] = w; return; }
    const char *nm = (const char *)*(uint64_t *)(w + 8); if (nm && !strcmp(nm, "ScrollView")) clip = w;
    uint64_t ch = *(uint64_t *)(w + 0x58), k = *(uint64_t *)(w + 0x60); if (!ch || k > 256) return;
    for (uint64_t i = 0; i < k; i++) dg_sliders(*(uint64_t *)(ch + 8 * i), vt, dep + 1, out, clips, clip, n);
}
static void ui_drag_pump(module_t *m, uint64_t root, int W, int H) {
    static unsigned seen; static int kind, sent, s_ok, s_y, s_l, s_r, s_last, s_down; static uint64_t h_mv, h_dn, h_up, h_sc;
    if (!h_mv) { h_mv = ui_hash("MouseMove"); h_dn = ui_hash("LeftMouseButtonDown"); h_up = ui_hash("LeftMouseButtonUp"); h_sc = ui_hash("MouseScroll"); }
    unsigned seq = atomic_load(&g_dg_seq); int live = atomic_load(&g_dg_kind);
    uint64_t o = atomic_load(&g_dg_o), c = atomic_load(&g_dg_c);
    int ox = (int)(((o >> 16) & 0xffff) / 65535.0 * (W - 1)), oy = (int)((o & 0xffff) / 65535.0 * (H - 1)), cx = (int)(((c >> 16) & 0xffff) / 65535.0 * (W - 1)), cy = (int)((c & 0xffff) / 65535.0 * (H - 1));
    if (kind == 2 && s_down) { ui_send_mouse(m, root, h_mv, s_last, s_y); ui_send_mouse(m, root, h_up, s_last, s_y); s_down = 0; return; }
    if (seq != seen) {
        seen = seq; kind = live; sent = 0; s_ok = 0; s_down = 0; s_last = -100000;
        { int dm_vault_ui_drag(int, int, int), dm_vault_ui_over(int, int); if ((dm_ui_panel_state() & DM_PS_VAULT) && dm_vault_ui_drag(0, ox, oy)) { kind = 3; return; }
          if ((dm_ui_panel_state() & DM_PS_VAULT) && dm_vault_ui_over(ox, oy)) { kind = 0; return; } }
        if (kind == 2) {
            static uint64_t sl[48], cl[48]; int n = 0; dg_sliders(root, (uint64_t)m->base + 0x20bad70, 0, sl, cl, 0, &n);
            int tol = (int)(atomic_load(&g_dg_tol) / 65535.0 * H), q = H / 180 > 1 ? H / 180 : 1; uint64_t w = 0; int hy = oy, hx = ox;
            for (int d = 0; d <= tol && !w; d += q) for (int sg = 0; sg < (d ? 2 : 1) && !w; sg++) { int y = oy + (sg ? d : -d); if (y < 0 || y >= H) continue;
                for (int e = 0; e < 5 && !w; e++) { int x = ox + (e == 1 ? -tol / 2 : e == 2 ? tol / 2 : e == 3 ? -tol : e == 4 ? tol : 0); if (x < 0 || x >= W) continue;
                    for (int i = 0; i < n; i++) if (dg_hit(sl[i], x, y) && (!cl[i] || dg_hit(cl[i], x, y))) { w = sl[i]; hy = y; hx = x; break; } } }
            if (w) {
                int a, b, top, bot;
                for (a = 0, b = hx; a < b;) { int mid = (a + b) / 2; if (dg_hit(w, mid, hy)) b = mid; else a = mid + 1; } s_l = a;
                for (a = hx, b = W - 1; a < b;) { int mid = (a + b + 1) / 2; if (dg_hit(w, mid, hy)) a = mid; else b = mid - 1; } s_r = a;
                for (a = 0, b = hy; a < b;) { int mid = (a + b) / 2; if (dg_hit(w, hx, mid)) b = mid; else a = mid + 1; } top = a;
                for (a = hy, b = H - 1; a < b;) { int mid = (a + b + 1) / 2; if (dg_hit(w, hx, mid)) a = mid; else b = mid - 1; } bot = a;
                int hh = bot - top + 1; s_y = (top + bot) / 2; s_ok = s_r - s_l > 16;
                LOG("界面拖动：起手 (%d,%d) 命中滑块 %s，行 y=%d 高 %d，滑槽 x %d–%d%s", ox, oy, *(uint64_t *)(w + 8) ? (const char *)*(uint64_t *)(w + 8) : "?", s_y, hh, s_l, s_r, s_ok ? "" : "（太窄，不做）");
            } else LOG("界面拖动：横向拖的起手 (%d,%d) 不在滑块上（可见滑块 %d 个，上下放宽 %d 像素），不发任何输入", ox, oy, n, tol);
        } else if (kind == 1) { static int once; if (!once++) LOG("界面拖动：首例纵向拖 → 在起手位置 (%d,%d) 发滚轮，每 %d 像素一格", ox, oy, UI_SCROLL_STEP(H)); }
    }
    if (!kind) return;
    if (kind == 3) { int dm_vault_ui_drag(int, int, int); if (live) dm_vault_ui_drag(1, cx, cy); else { dm_vault_ui_drag(2, cx, cy); kind = 0; } return; }
    if (kind == 1) {
        if (!live) { kind = 0; return; }
        int want = (cy - oy) / UI_SCROLL_STEP(H), d = want - sent; if (!d) return;
        if (d > 3) d = 3; if (d < -3) d = -3;
        ui_send_mouse(m, root, h_mv, ox, oy); ui_send_mouse_arg(m, root, h_sc, ox, oy, d * 120); sent += d; return;
    }
    if (!s_ok) { if (!live) kind = 0; return; }
    int x = cx < s_l ? s_l : cx > s_r ? s_r : cx;
    if (x != s_last && (x - s_last >= 2 || s_last - x >= 2 || !live || x == s_l || x == s_r)) { ui_send_mouse(m, root, h_mv, x, s_y); ui_send_mouse(m, root, h_dn, x, s_y); s_last = x; s_down = 1; return; }
    if (!live) kind = 0;
}
static void ui_auto_key(uint64_t mask);
static uint64_t ui_find(uint64_t w, const char *name, int dep);

























static _Atomic uint64_t g_bd_key; static _Atomic int g_bd_state;
static _Atomic int g_bd_stay;
void dm_ui_bind_begin(uint64_t key_mask) { if (!key_mask) return; atomic_store(&g_bd_key, key_mask); atomic_store(&g_bd_stay, 0); atomic_store(&g_bd_state, 1); }
int dm_ui_bind_active(void) { int s = atomic_load(&g_bd_state); return s == 0 ? 0 : (s == 1 || s == 5) ? 1 : s == 2 ? 2 : 3; }
void dm_ui_bind_cancel(void) { atomic_store(&g_bd_state, 0); }


int dm_ui_bind_slot(void) { if (!atomic_load(&g_bd_state)) return 0; uint64_t k = atomic_load(&g_bd_key); int s = (k & DM_BTN_B) ? 1 : (k & DM_BTN_X) ? 2 : (k & DM_BTN_Y) ? 3 : (k & DM_BTN_R) ? 4 : 0; return s ? s + ((k & DM_BTN_ZL) ? 4 : 0) : 0; }


uint64_t dm_ui_auto_release(void) { int s = atomic_load(&g_bd_state); return s == 1 || s == 5 || s == 4 ? DM_BTN_ZL : 0; }
static int bd_open(uint64_t root, const char *name) { uint64_t w = ui_find(root, name, 0); return w && *(uint8_t *)(w + 0x50); }










static uint64_t bd_child(uint64_t w, const char *name) {
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i); const char *cn = c ? (const char *)*(uint64_t *)(c + 8) : 0; if (cn && !strcmp(cn, name)) return c; }
    return 0;
}
static int bd_tab_learned(uint64_t tab) {
    uint64_t box = bd_child(tab, "SkillButtonContainer"); if (!box) return 0;
    uint64_t ch = *(uint64_t *)(box + 0x58), n = *(uint64_t *)(box + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i), cnt = c ? bd_child(c, "Counter") : 0; uint8_t t = cnt ? *(uint8_t *)(cnt + 0xa0) : 0; if (t >= '1' && t <= '9') return 1; }
    return 0;
}
static int bd_tab_cur(uint64_t tree) { char nm[16]; int32_t col[4]; int have[4] = {0}, n = 0;
    for (int k = 0; k < 4; k++) { snprintf(nm, sizeof nm, "TextTab%d", k); uint64_t t = bd_child(tree, nm); if (t) { have[k] = 1; col[k] = *(int32_t *)(t + 0xbc); n++; if (col[k] == 240) return k; } }


    for (int k = 0; k < 4 && n >= 3; k++) { if (!have[k]) continue; int same = 0; for (int j = 0; j < 4; j++) same += have[j] && col[j] == col[k]; if (same == 1) return k; }
    return -1; }
int dm_ui_tree_tab(uint64_t tree) { return bd_tab_cur(tree); }




static _Atomic int g_swaptab_open;
int dm_ui_swaptab_take(void) { return atomic_exchange(&g_swaptab_tap, 0); }
int dm_ui_skilltree_open(void) { return atomic_load(&g_swaptab_open); }
static uint64_t swaptab_w(uint64_t root, int k) { static const char *nm[4] = { "TouchSwapTab0", "TouchSwapTab1", "TouchSwapI", "TouchSwapII" }; uint64_t tw = ui_panel(root, "SkillsTreePanel"); return tw ? ui_find(tw, nm[k], 0) : 0; }
static void swaptab_pump(uint64_t root) {
    uint64_t tw = ui_panel(root, "SkillsTreePanel"); int open = tw && *(uint8_t *)(tw + 0x50), show = open && bd_tab_cur(tw) == 3, w = dm_swapskill_wset();
    atomic_store(&g_swaptab_open, open);




    static uint32_t lastsig; static int had; uint32_t sig = dm_swapskill_itemsig(); if (show && had && sig != lastsig) { module_t *sm = main_module(); uint64_t us = ui_find(root, "UISwitcher", 0);
        if (sm && us && *(uint8_t *)(us + 0x50) && *(uint64_t *)(*(uint64_t *)us + 0x28) == (uint64_t)sm->base + 0x404fd8) { int cur = *(int32_t *)(us + 0x1b8);
            ((void (*)(uint64_t, int, int))(sm->base + 0x405898))(us, cur - 1, 0); ((void (*)(uint64_t, int, int))(sm->base + 0x405898))(us, cur, 1);
            fe_send("SkillsTreePanelMessage", "ActivateTab", "3"); LOG("副手组技能：武器组的物品技能变了 → 技能树重开到综合技能页"); } }
    lastsig = sig; had = show; (void)w;
    for (int k = 0; k < 4 && tw; k++) { uint64_t x = swaptab_w(root, k); if (!x) continue; int v = show && (k >= 2 || (w < 0 ? k == 0 : k == w)); ui_show_ours(x, v); }
}
static int swaptab_hit(uint64_t root, int x, int y) {
    for (int k = 0; k < 2; k++) { uint64_t t = swaptab_w(root, k); if (t && *(uint8_t *)(t + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)t + 0x48))(t, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1)) return 1; }
    return 0;
}
static void bd_tab_step(int d) { atomic_store(&g_tab_delta, d); atomic_store(&g_tab_top, 0); dm_wref_set(&g_tab_ref, 0); atomic_store(&g_tab_have, 0); dm_ui_tab_go(); }










static int bd_cell_row(const char *nm) { return !nm ? 0 : !strncmp(nm, "AltSkillButton", 14) ? 2 : !strncmp(nm, "SkillButton", 11) ? 1 : 0; }
static int bd_modal_walk(uint64_t w, uint64_t xy, int dep) {
    if (!w || dep > 8 || !*(uint8_t *)(w + 0x50)) return 0;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i); if (!c || !*(uint8_t *)(c + 0x50)) continue; int row = bd_cell_row((const char *)*(uint64_t *)(c + 8));
        if (!row) { int r = bd_modal_walk(c, xy, dep + 1); if (r) return r; continue; }
        if (!(((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)c + 0x48))(c, xy) & 1)) continue;
        int col = 1, cx = *(int32_t *)(c + 0x70);
        for (uint64_t k = 0; k < n; k++) { uint64_t o = *(uint64_t *)(ch + 8 * k); if (o && o != c && bd_cell_row((const char *)*(uint64_t *)(o + 8)) == row && *(int32_t *)(o + 0x70) < cx) col++; }
        return col >= 1 && col <= 5 ? col + (row == 2 ? 5 : 0) : 0; }
    return 0;
}


static int ui_bind_modal_tap(uint64_t root, int x, int y) {
    if (atomic_load(&g_bd_state)) return 0;
    uint64_t md = ui_find(root, "ButtonBindingModal", 0); if (!md || !*(uint8_t *)(md + 0x50)) return 0;
    int slot = bd_modal_walk(md, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32), 0);




    if (!slot) { module_t *m = main_module(); int vh = m ? *(int32_t *)(m->base + 0x211da3c) : 0; static const int oy[8] = { -32, 32, -45, 45, 0, 0, -32, 32 }, ox[8] = { 0, 0, 0, 0, -45, 45, -45, 45 };
        for (int k = 0; k < 8 && !slot && vh > 0; k++) { int xx = x + ox[k] * vh / 2160, yy = y + oy[k] * vh / 2160; if (xx < 0 || yy < 0) continue; slot = bd_modal_walk(md, (uint64_t)(uint32_t)xx | ((uint64_t)(uint32_t)yy << 32), 0); } }
    if (!slot) {
        uint64_t bg = ui_find(md, "Background", 0); uint64_t xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32);
        if (bg && !(((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)bg + 0x48))(bg, xy) & 1)) {
            atomic_store(&g_bd_key, DM_BTN_MINUS); atomic_store(&g_bd_stay, 1); atomic_store(&g_bd_state, 3); LOG("技能绑定：点在绑定框外 → 取消（MINUS）"); return 1; }
        return bg ? 1 : 0; }
    static const uint64_t k[5] = { DM_BTN_A, DM_BTN_B, DM_BTN_Y, DM_BTN_X, DM_BTN_R };
    atomic_store(&g_bd_key, k[(slot - 1) % 5] | (slot > 5 ? DM_BTN_ZL : 0)); atomic_store(&g_bd_stay, 1); atomic_store(&g_bd_state, 3);
    LOG("技能绑定：绑定框里点了第 %d 排第 %d 列", slot > 5 ? 2 : 1, (slot - 1) % 5 + 1); return 1;
}


static int bd_top_tab(uint64_t root) {
    uint64_t tabs = ui_find(root, "UISwitcherTabs", 0); if (!tabs || !*(uint8_t *)(tabs + 0x50)) return -1;
    return TAB_SEL(tabs);
}










enum { MV_BIND, MV_LIFT, MV_WIDE, MV_SAFE, MV_CUBE, MV_TROVE, MV_N };
static const char *const mv_name[MV_N] = { "技能绑定框挪屏外", "真全屏贴顶上提", "储藏箱加宽", "安全区内移", "魔盒并排右移", "珍宝匣页签" };
typedef struct { dm_wref r; int own, f, relayout; int32_t orig, val; } mvent;
static mvent g_mv[96]; static int g_nmv, g_mv_want[MV_N];
static int mv_allowed(int own, uint64_t w) {
    const char *nm = (const char *)*(uint64_t *)(w + 8), *pn = *(uint64_t *)(w + 0x30) ? (const char *)*(uint64_t *)(*(uint64_t *)(w + 0x30) + 8) : NULL; if (!nm) return 0;
    switch (own) {
    case MV_BIND: return !strcmp(nm, "ButtonBindingModal");
    case MV_LIFT: return *(float *)(w + 0x4c) == 0 && *(int32_t *)(w + 0x84) <= -100;
    case MV_WIDE: { static const char *const k[] = { "WideBackground", "Text0", "Text1", "TabsEndcapLeft", "TabsEndcapRight", "ControllerCursorBounds", "BankExpansionLayout", "PlayerInventoryExpansionLayout" };
        for (unsigned i = 0; i < sizeof k / sizeof *k; i++) if (!strcmp(nm, k[i])) return 1; return 0; }
    case MV_CUBE: return !strcmp(nm, "HoradricCubeLayout") || !strcmp(nm, "PlayerInventoryExpansionLayout") || !strcmp(nm, "Text0") || !strcmp(nm, "Text1");
    case MV_TROVE: return pn && !strcmp(pn, "BankTabs");
    case MV_SAFE: return !strcmp(nm, "AutoMap") || !strcmp(nm, "HUDWarnings") || (pn && !strcmp(pn, "HireablesPanel"));
    }
    return 0;
}
static int mv_count(int own);
static void mv_prune(void) {
    module_t *m = main_module(); uint64_t root = 0; if (!m) return; memcpy(&root, m->base + 0x2f05458, 8); if (!root) return; int n0 = g_nmv;
    for (int i = g_nmv - 1; i >= 0; i--) if (!ui_has(root, g_mv[i].r.w, 0)) g_mv[i] = g_mv[--g_nmv];
    if (g_nmv != n0) { static int lg; if (lg++ < 20) LOG("位置守卫：%d 条的控件已不在界面树上（界面重建过），忘掉（剩 %d）", n0 - g_nmv, g_nmv); }
}
static int32_t g_mv_base; static int g_mv_base_on;
static void mv_offset(int own, uint64_t w, int f, int32_t delta, int relayout) {


    int dm_padmode_busy(void); if (delta && dm_padmode_busy()) return;
    if (!w) return;




    const char *name = (const char *)*(uint64_t *)(w + 8);
    if (name && !strcmp(name, "PlayerInventoryExpansionLayout")) relayout = 0;
    int32_t *v = (int32_t *)(w + 0x70 + 4 * f); int i = 0; while (i < g_nmv && !(g_mv[i].r.w == w && g_mv[i].f == f)) i++;
    if (i == g_nmv) { if (!delta) return; if (!mv_allowed(own, w)) { static int lg; if (lg++ < 8) LOG("【红】位置守卫：%s 要挪的控件「%s」不在名单里，不写", mv_name[own], (const char *)*(uint64_t *)(w + 8)); return; }
        mv_prune();
        if (g_nmv >= 96) { static int lg; if (lg++ < 8) LOG("【红】位置守卫：%s 条目满了（96 条都还挂在界面树上），「%s」不写", mv_name[own], (const char *)*(uint64_t *)(w + 8)); return; }
        if (!mv_count(own)) { static int lg[MV_N]; if (lg[own]++ < 40) LOG("位置守卫：%s 开始挪（%s）", mv_name[own], (const char *)*(uint64_t *)(w + 8)); }
        i = g_nmv;
        g_mv[g_nmv] = (mvent){ { 0, 0 }, own, f, relayout, *v, *v }; dm_wref_set(&g_mv[g_nmv++].r, w); }
    mvent *e = &g_mv[i];




    if (!delta && !g_mv_base_on && *v != e->val) return;
    if (g_mv_base_on) e->orig = g_mv_base; else if (*v != e->val) e->orig = *v;
    if (!delta) { if (*v == e->val && *v != e->orig) { *v = e->orig; if (e->relayout) ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); } g_mv[i] = g_mv[--g_nmv]; return; }
    int32_t want = e->orig + delta; if (*v != want) { *v = want; if (relayout) ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); } e->val = want;
}
static void mv_restore(uint64_t root, int own) {
    int n0 = mv_count(own); if (!n0) return;
    for (int i = g_nmv - 1; i >= 0; i--) if (g_mv[i].own == own) { if (root && ui_has(root, g_mv[i].r.w, 0)) mv_offset(own, g_mv[i].r.w, g_mv[i].f, 0, g_mv[i].relayout); else g_mv[i] = g_mv[--g_nmv]; }
    int n1 = mv_count(own); if (n1 == n0) return;
    static int lg[MV_N]; if (lg[own]++ < 40) LOG("位置守卫：%s 条件没了，%d 处全部写回（剩 %d）", mv_name[own], n0, n1);
}
static int mv_count(int own) { int n = 0; for (int i = 0; i < g_nmv; i++) n += g_mv[i].own == own; return n; }
static uint64_t g_mv_audit_at; static const char *g_mv_audit_path = "";
static void mv_audit_arm(const char *path) {
    static char both[96]; if (g_mv_audit_at && strcmp(g_mv_audit_path, path)) { snprintf(both, sizeof both, "%s → %s", g_mv_audit_path, path); g_mv_audit_path = both; } else g_mv_audit_path = path;
    g_mv_audit_at = dm_guest_frame() + 12; }
void dm_ui_lifecycle(int background) { mv_audit_arm(background ? "切后台" : "回前台"); }
static void mv_audit_pump(uint64_t root, uint64_t fr) {
    if (!g_mv_audit_at || fr < g_mv_audit_at) return; g_mv_audit_at = 0; int bad = 0; char line[256]; int k = 0;

    for (int o = 0; o < MV_N; o++) { int live = 0; for (int i = 0; i < g_nmv; i++) live += g_mv[i].own == o && ui_has(root, g_mv[i].r.w, 0) && *(int32_t *)(g_mv[i].r.w + 0x70 + 4 * g_mv[i].f) == g_mv[i].val;
        if (!g_mv_want[o] && live) { bad++; k += snprintf(line + k, sizeof line - k, "%s 还剩 %d 处没写回；", mv_name[o], live); }
        else if (k < 200) k += snprintf(line + k, sizeof line - k, "%s %s；", mv_name[o], g_mv_want[o] ? "条件成立、挪着" : "已全部写回"); }
    static const char *lp; static uint64_t lfr; if (!bad && lp == g_mv_audit_path && fr - lfr < 600) return;
    lp = g_mv_audit_path; lfr = fr; LOG("%s位置守卫自检（%s）：%s", bad ? "【红】" : "", g_mv_audit_path, line);
}
static void ui_bind_pump(uint64_t root, uint64_t fr) {
    static int last, tries, zl, page; static uint64_t t0, next, pressed;
    int st = atomic_load(&g_bd_state);






    { uint64_t md = ui_panel(root, "ButtonBindingModal"); int away = md && (st == 2 || st == 3) && !atomic_load(&g_bd_stay); g_mv_want[MV_BIND] = away;
      if (md && away) mv_offset(MV_BIND, md, 0, 30000, 0); else mv_restore(root, MV_BIND); }
    if (!st) { last = 0; return; }
    if (st != last) { last = st; t0 = fr; next = fr; tries = 0; zl = 0; pressed = 0; page = -1; }
    if (fr < next) return; next = fr + (st == 2 || st == 5 ? 6 : 1);
    int tree = bd_open(root, "SkillsTreePanel"), modal = bd_open(root, "ButtonBindingModal");
    uint64_t key = atomic_load(&g_bd_key);
    switch (st) {
    case 1: {


        if (tree) { LOG("技能绑定：技能树已开（开页用了 %llu 帧、按键 %d 次），等玩家点一个技能（目标键 %#llx）", (unsigned long long)(fr - t0), tries, (unsigned long long)key); atomic_store(&g_bd_state, 5); break; }
        if (tries >= 9) { LOG("【红】技能绑定：按了 %d 次键技能树还没开，放弃", tries); atomic_store(&g_bd_state, 0); break; }
        if (fr - t0 < ((key & DM_BTN_ZL) ? 12 : 2)) break;
        int dm = bd_open(root, "UISwitcher"), cur = dm ? bd_top_tab(root) : -1;
        if (tries) { uint64_t dt = fr - pressed; int moved = dm && cur >= 0 && cur != page;
            if (!((moved && dt >= 7) || dt >= 24)) break; }
        page = cur; pressed = fr; dm_ui_key_pulse(dm ? DM_BTN_R : DM_BTN_PLUS, 4); tries++; break; }
    case 5: {
        uint64_t tw = ui_panel(root, "SkillsTreePanel"); int cur = tw ? bd_tab_cur(tw) : -1; char nm[16];
        if (!tree || cur < 0 || tries > 6) { atomic_store(&g_bd_state, 2); break; }
        if (atomic_load(&g_tab_left)) break;
        next = fr + 12;
        if (!tries) { tries = 1; if (cur) { bd_tab_step(-cur); break; } }
        snprintf(nm, sizeof nm, "Tab%d", cur); uint64_t tb = bd_child(tw, nm);
        if (cur == 3 || (tb && bd_tab_learned(tb))) { LOG("技能绑定：停在第 %d 页（第一个有已学技能的分类页）", cur); atomic_store(&g_bd_state, 2); break; }
        tries++; bd_tab_step(1); break; }
    case 2:
        if (modal) { atomic_store(&g_bd_state, 3); break; }
        if (!tree && fr - t0 > 30) { LOG("技能绑定：玩家关了技能树，取消"); atomic_store(&g_bd_state, 0); }
        break;
    case 3:
        if (!modal) { if (tries) { int stay = atomic_load(&g_bd_stay); LOG("技能绑定：%s %#llx，%s", key == DM_BTN_MINUS ? "已取消，键" : "已绑到键", (unsigned long long)key, stay ? "留在技能树" : "关功能页"); atomic_store(&g_bd_state, stay ? 0 : 4); } else if (fr - t0 > 120) atomic_store(&g_bd_state, 0); break; }
        if (fr - t0 < 12) break;
        if (pressed && fr - pressed < 30) break;
        if (tries >= 3) { LOG("【红】技能绑定：在绑定框里按了 3 次目标键 %#llx 框都没关，放弃", (unsigned long long)key); atomic_store(&g_bd_state, 0); break; }


        if ((key & DM_BTN_ZL) && !zl) { zl = 1; dm_ui_key_pulse(DM_BTN_ZL, 30); next = fr + 10; break; }
        zl = 0; dm_ui_key_pulse(key, 6); tries++; pressed = fr; break;
    case 4:
        if (!tree) { atomic_store(&g_bd_state, 0); break; }
        if (pressed && fr - pressed < 30) break;
        if (tries >= 3) { atomic_store(&g_bd_state, 0); break; }
        if (fr - t0 < ((key & DM_BTN_ZL) ? 10 : 3)) break;
        dm_ui_key_pulse(DM_BTN_B, 4); tries++; pressed = fr; break;
    }
}

















static _Atomic int g_map_req = -1, g_map_full_now, g_map_hold;
static int g_map_want = -1, g_map_cfg_seen = -1;
void dm_ui_map_full(int on) { atomic_store(&g_map_req, on != 0); }
void dm_ui_map_hold(int on) { atomic_store(&g_map_hold, on != 0); }
int dm_ui_map_is_full(void) { return atomic_load(&g_map_full_now); }










static void ui_map_center(module_t *m, uint64_t root, int mini) {
    static int ours; int32_t *sc = (int32_t *)(m->base + 0x2d5e914); int want = 0;
    int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); uint64_t am = !mini && dm_uiscreen_ingame() && dm_screen_real(&rw, &rh) && h > 0 && h < rh ? ui_panel(root, "AutoMap") : 0;
    if (am && *(uint64_t *)am == (uint64_t)m->base + 0x20c58a0) { float q = ceilf(((float (*)(uint64_t))(m->base + 0x390b38))(am) * *(float *)(am + 0x15a8) * 8) / 8; if (q < 0.125f) q = 0.125f;
        float cell = q * *(float *)(m->base + 0x2d5e91c); if (cell > 0.1f) want = (int)lroundf((y - (rh - h) / 2.0f) / cell); }
    if (*sc == want || (*sc != ours && *sc != 0)) { if (*sc == want) ours = want; return; }
    { static int lg; if (lg++ < 8) LOG("全屏地图中心：纵向卷动量 %d → %d 格", *sc, want); } *sc = want; ours = want;
}








static _Atomic int g_map_movie;
static void ui_map_pump(module_t *m, uint64_t root) {








    if (!dm_uiscreen_ingame()) {
        int pending = atomic_exchange(&g_map_req, -1);
        atomic_store(&g_map_full_now, 0);
        if (pending >= 0) { static int n; if (n++ < 4) LOG("地图：局外的切换请求已丢掉（局外不调原版地图切换）"); }
        return;
    }
    int cfg = *(volatile int32_t *)(m->base + 0x2d479f4), mini = *(volatile int32_t *)(m->base + 0x2d5c748);


    { static long at = -1; static int tv; uint64_t fr = dm_guest_frame(); if (at < 0) { const char *e = getenv("DM_TEST_AUTOMAP"); at = e ? atol(e) : 0; const char *c = e ? strchr(e, ':') : NULL; tv = c ? atoi(c + 1) : 0; }
      if (at > 0 && fr >= (uint64_t)at) { at = 0; *(volatile int32_t *)(m->base + 0x2d479f4) = tv; ((void (*)(int))(m->base + 0x40f780))(tv); cfg = tv; mini = *(volatile int32_t *)(m->base + 0x2d5c748); LOG("测试 DM_TEST_AUTOMAP：帧 %llu 把「透视地图尺寸」改成 %d", (unsigned long long)fr, tv); } }
    int dm_padmode_current(void); dm_hud_info hi; dm_hud_get(&hi);
    int r = atomic_exchange(&g_map_req, -1);
    if (dm_padmode_current()) {
        if (r >= 0) {
            if (cfg != 1 && cfg != 2) { static int n; if (n++ < 4) LOG("地图切换：玩家的「透视地图尺寸」是 %d（不是微缩档），不切", cfg); }
            else if ((mini == 0) != r) { ((void (*)(int))(m->base + 0x40f780))(r ? 0 : cfg); mini = *(volatile int32_t *)(m->base + 0x2d5c748); LOG("地图切换：→ %s（配置值 %d 不动，不落盘）", r ? "全屏" : cfg == 1 ? "微缩放左" : "微缩放右", cfg); }
        }
        atomic_store(&g_map_full_now, mini == 0 && (cfg == 1 || cfg == 2)); return;
    }
    if (cfg != g_map_cfg_seen) {
        g_map_want = cfg == 0; { static int n; if (n++ < 8) LOG("地图：「透视地图尺寸」读到 %d → 触屏下%s", cfg, cfg == 0 ? "缺省全屏地图（点右上角那一块回小地图）" : "缺省小地图"); } g_map_cfg_seen = cfg; }
    if (r >= 0 && g_map_want != r) { g_map_want = r; LOG("地图：玩家点了 → 记住%s", r ? "全屏地图" : "小地图"); }
    int small = cfg == 1 ? 1 : 2, hold = atomic_load(&g_map_hold), want = g_map_want == 1 && !hold;
    if (hi.valid && (mini == 0) != want) {
        ((void (*)(int))(m->base + 0x40f780))(want ? 0 : small); mini = *(volatile int32_t *)(m->base + 0x2d5c748);
        static int n; if (n++ < 40) LOG("地图：→ %s（%s；配置值 %d 不动）", want ? "全屏" : small == 1 ? "微缩放左" : "微缩放右", hold ? "面板开着先让开" : g_map_want ? "玩家要全屏" : "玩家要小地图", cfg); }
    atomic_store(&g_map_full_now, mini == 0);
    { void dm_ui_key_pulse_alone(uint64_t mask, int frames); uint64_t am = ui_panel(root, "AutoMap"); static uint64_t sent;
      if (am && *(uint64_t *)am == (uint64_t)m->base + 0x20c58a0 && !*(uint8_t *)(am + 0x50) && *(volatile int32_t *)(m->base + 0x2d485b4) && hi.valid && dm_uiscreen_ingame() && !atomic_load(&g_map_movie)) {
          uint64_t fr = dm_guest_frame(); if (!sent || fr >= sent + 120) { sent = fr; dm_ui_key_pulse_alone(DM_BTN_MINUS, 4); LOG("地图：局内小地图关着（过场关的）、选项「进局自动开启地图」开着 → 替玩家按一下地图键（−）"); } } }
}






static _Atomic uint64_t g_mm_rect; static _Atomic int g_mm_have;
int dm_ui_minimap_rect(float r[4]) { if (!atomic_load(&g_mm_have)) return 0; uint64_t v = atomic_load(&g_mm_rect); for (int i = 0; i < 4; i++) r[i] = (float)(int16_t)(uint16_t)(v >> (16 * (3 - i))) / 8192.0f; return 1; }




























static _Atomic int g_load_up = -1;
int dm_ui_load_tracked(void) { return atomic_load(&g_load_up); }
static uint64_t g_lift_next;




















#define NOTE_PANEL_Y -310








static _Atomic uint64_t g_tip_fr; static int32_t g_tip_box[4];
static dm_wref g_nd; static int32_t g_nd_orig, g_nd_mine; static int g_nd_on = -1, g_nd_hold;
static int g_nd_ychk; static int32_t g_nd_ywant; static unsigned g_nd_ywr, g_nd_ybad;
static int note_dock_snap(uint64_t w, int32_t from, int32_t to) {
    int32_t *ry = (int32_t *)(w + 0x74); if (!*(uint8_t *)(w + 0x198) || *ry != from || from == to) return 0;
    *ry = to; g_nd_ychk = 1; g_nd_ywant = to; g_nd_ywr++; return 1;
}
static void note_dock_restore(uint64_t root, const char *why) {
    uint64_t w = root ? ui_panel(root, "NotificationPanel") : 0; if (!w || w != dm_wref_get(&g_nd)) return;
    int32_t *t = (int32_t *)(w + 0x19c);
    if (*t == g_nd_mine && g_nd_mine != g_nd_orig) {
        *t = g_nd_orig; int still = note_dock_snap(w, g_nd_mine, g_nd_orig);
        static int lg; if (lg++ < 60) LOG("提示框停靠：%s → 写回原值 y %d%s", why, g_nd_orig, still ? "（框已停稳，矩形 y 一并写回）" : ""); }
    g_nd_mine = *t; g_nd_on = 0;
}
static void ui_note_dock_pump(uint64_t root) {
    static const char *const big[] = { "PlayerInventoryExpansionLayout", "SkillsTreePanel", "CharacterStatsPanel", "QuestLogPanelExpansion", "BankExpansionLayout", "HoradricCubeLayout",
        "WaypointsPanelExpansion", "VendorPanelLayout", "HirelingInventoryPanel", "ImbueItemsPanel", "ChroniclePanel", "MessageLogPanel", "PauseLayout", "PauseLayoutGarden",
        "SettingsPanel", "SoundOptionsPanel", "VideoOptionsPanel", "GameplayOptionsPanel" };
    static int py = 1;
    if (py > 0) { const char *e = getenv("DM_TEST_NOTE_PANEL_Y"); py = e ? atoi(e) : NOTE_PANEL_Y; }




    int ing = dm_uiscreen_ingame(); if (g_nd_hold == 1 && !ing) g_nd_hold = 2; else if (g_nd_hold == 2 && ing) g_nd_hold = 0;
    uint64_t w = ui_panel(root, "NotificationPanel");
    if (!w) { dm_wref_set(&g_nd, 0); return; }
    int32_t *t = (int32_t *)(w + 0x19c), h = *(int32_t *)(w + 0x7c);
    if (g_nd_ychk) { g_nd_ychk = 0; int32_t y = *(int32_t *)(w + 0x74); static int lg;
        if (w != dm_wref_get(&g_nd)) { if (lg++ < 60) LOG("提示框停靠：写回矩形 y 后提示框对象已换（出局 / 重建），不核"); }
        else if (y != g_nd_ywant) { g_nd_ybad++; LOG("【红】提示框停靠：矩形 y 写成 %d 后下一帧被改成 %d（动画期写入，第 %u 次）", g_nd_ywant, y, g_nd_ybad); }
        else if (lg++ < 60) LOG("提示框停靠：停稳的框下一帧在 y %d（%s；矩形 y 写入第 %u 次，动画期 %u 次）", y, y == g_nd_orig ? "原位" : "面板态位", g_nd_ywr, g_nd_ybad); }
    if (w != dm_wref_get(&g_nd) || *t != g_nd_mine) { dm_wref_set(&g_nd, w); g_nd_orig = *t; g_nd_mine = *t; }
    int panel = 0; uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60);
    for (uint64_t j = 0; ing && !g_nd_hold && j < n && !panel; j++) { uint64_t c = *(uint64_t *)(ch + 8 * j); const char *nm = c ? (const char *)*(uint64_t *)(c + 8) : NULL;
        if (!nm || !*(uint8_t *)(c + 0x50)) continue;
        for (unsigned i = 0; i < sizeof big / sizeof *big && !panel; i++) panel = !strcmp(nm, big[i]) && has_visible_leaf(c, 1); }
    int32_t pv = h > 0 ? (int32_t)lroundf(py * h / 244.0f) : g_nd_orig;




    { module_t *mm = main_module(); int UW = mm ? *(int32_t *)(mm->base + 0x211da38) : 0, UH = mm ? *(int32_t *)(mm->base + 0x211da3c) : 0; uint64_t fr = dm_guest_frame(), tf = atomic_load(&g_tip_fr);
      static int moved; int32_t rw = *(int32_t *)(w + 0x78);
      if (panel && UH > 0 && h > 0 && rw > 0 && tf && fr - tf <= 2) {
          int cy = UH + (int)lroundf((float)pv * UH / 2160.0f), hh = (int)lroundf((float)h * UH / 4320.0f), hw = (int)lroundf((float)rw * UH / 4320.0f), gap = UH / 90;
          const int32_t *b = g_tip_box; int hit = UW / 2 - hw < b[2] && UW / 2 + hw > b[0] && cy - hh < b[3] + gap && cy + hh > b[1] - gap;
          if (hit) { int up = b[1] - gap - hh, dn = b[3] + gap + hh, ok_up = up - hh >= 0, ok_dn = dn + hh <= UH;
              int to = ok_up && ok_dn ? (abs(up - cy) <= abs(dn - cy) ? up : dn) : ok_up ? up : ok_dn ? dn : cy;
              if (to != cy) { pv = (int32_t)lroundf((float)(to - UH) * 2160.0f / UH); if (!moved) { moved = 1; LOG("提示框停靠：说明框占 (%d,%d)–(%d,%d) 压到提示框（中心 y %d）→ 挪到说明框%s，中心 y %d", b[0], b[1], b[2], b[3], cy, to < cy ? "上方" : "下方", to); } } }
          else moved = 0; }
      else if (moved) { moved = 0; LOG("提示框停靠：说明框没了 → 回面板态位"); } }
    if (!(panel && pv > g_nd_orig)) { if (g_nd_on) note_dock_restore(root, ing ? "面板关了" : "不在局内"); }
    else if (*t != pv) { int32_t was = *t; *t = pv; g_nd_mine = pv;
        if (note_dock_snap(w, was, pv)) { static int lg; if (lg++ < 60) LOG("提示框停靠：目标换成 %d 时框已停稳在 y %d，矩形 y 一并写成新目标", pv, was); } }
    if (panel != g_nd_on) { g_nd_on = panel; static int lg; if (lg++ < 40) LOG("提示框停靠：%s，目标 y %d（布局原值 %d，矩形高 %d）", panel ? "面板态" : "无面板", *t, g_nd_orig, h); }
}
static void ui_lift_pump(uint64_t root, uint64_t fr) {



    int dm_padmode_busy(void); if (dm_ui_unstable() && (fr || dm_padmode_busy())) return;
    if (fr < g_lift_next) return; g_lift_next = fr + 6;
    int x, y, w, h; dm_ui_rect(&x, &y, &w, &h); int lift = h > 0 && dm_uiscreen_ingame() ? (int)((long)y * 2160 / h) : 0;
    g_mv_want[MV_LIFT] = lift != 0; if (!lift) { mv_restore(root, MV_LIFT); return; }
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60);
    for (uint64_t k = 0; ch && k < n && k < 256; k++) { uint64_t p = *(uint64_t *)(ch + 8 * k); if (!p) continue; const char *nm = (const char *)*(uint64_t *)(p + 8);
        float ay = *(float *)(p + 0x4c); int layer = *(int32_t *)(p + 0x84); if (!(ay == 0 && layer <= -100)) continue;
        int32_t y0 = *(int32_t *)(p + 0x74); mv_offset(MV_LIFT, p, 1, -lift, 0);
        if (*(int32_t *)(p + 0x74) != y0) { static int lg; if (lg++ < 16) LOG("贴顶上提：%s（层 %d）y %d → %d", nm ? nm : "?", layer, y0, *(int32_t *)(p + 0x74)); } }
}






static void ui_trove_tabs_pump(module_t *m, uint64_t root, uint64_t fr) {
    static uint64_t next; if (fr < next) return; next = fr + 2;
    uint64_t bk = ui_panel(root, "BankExpansionLayout"), tabs = bk && *(uint8_t *)(bk + 0x50) ? ui_find(bk, "BankTabs", 0) : 0;




    { uint64_t gm = tabs ? ui_find(bk, "advancedstash_gems", 0) : 0; if (!gm || !ui_find(gm, "r01", 0)) tabs = 0; }
    g_mv_want[MV_TROVE] = tabs != 0; if (!tabs || *(uint64_t *)tabs != (uint64_t)m->base + 0x20ba3a8) { mv_restore(root, MV_TROVE); return; }
    uint64_t ch = *(uint64_t *)(tabs + 0x58), n = *(uint64_t *)(tabs + 0x60); int x0 = 0, x1 = 0, have = 0;
    for (uint64_t i = 0; ch && i < n && i < 64; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i); const char *cn = c ? (const char *)*(uint64_t *)(c + 8) : NULL; if (!cn) continue;
        if (!strcmp(cn, "Image0")) { x0 = *(int32_t *)(c + 0x70); have |= 1; } if (!strcmp(cn, "Image1")) { x1 = *(int32_t *)(c + 0x70); have |= 2; } }
    { static int lg; if (!lg++) { char b[700]; int k = 0; for (uint64_t i = 0; ch && i < n && i < 64 && k < 640; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i); const char *cn = c ? (const char *)*(uint64_t *)(c + 8) : NULL; if (cn) k += snprintf(b + k, sizeof b - k, " %s(%d,%d)", cn, *(int32_t *)(c + 0x70), *(int32_t *)(c + 0x74)); }
          LOG("珍宝匣页签：页签条子控件 %llu 个：%s", (unsigned long long)n, b); } }
    if (have != 3 || x1 <= x0) return; int tw = x1 - x0;
    for (uint64_t i = 0; ch && i < n && i < 64; i++) { uint64_t c = *(uint64_t *)(ch + 8 * i); const char *cn = c ? (const char *)*(uint64_t *)(c + 8) : NULL; if (!cn) continue;
        size_t l = strlen(cn); if (!l || cn[l - 1] < '0' || cn[l - 1] > '9' || (l > 1 && cn[l - 2] >= '0' && cn[l - 2] <= '9')) continue; int k = cn[l - 1] - '0';
        if (k == 3 || k == 4) mv_offset(MV_TROVE, c, 1, 6000, 0); else if (k == 5) mv_offset(MV_TROVE, c, 0, -2 * tw, 0); }
    static int last = -1; int sel = TAB_SEL(tabs);


    for (int k = 0; k < 2; k++) { uint64_t t = ui_find(bk, k ? "TouchStashRight" : "TouchStashLeft", 0); static int32_t ox[2] = { INT32_MIN, INT32_MIN }; if (!t) continue;
        int32_t x = *(int32_t *)(t + 0x70); if (x < 50000) ox[k] = x; if (ox[k] != INT32_MIN) ui_move_ours(t, sel >= 2 ? 99999 : ox[k]); }
    if (sel == 3 || sel == 4) { int to = last >= 5 ? 2 : 5; ((void (*)(uint64_t, int, int))(m->base + 0xad9b80))(tabs, to, 0); LOG("珍宝匣页签：落在第 %d 个（空页，上一个是第 %d 个）→ 改选第 %d 个，现为 %d", sel, last, to, TAB_SEL(tabs)); last = TAB_SEL(tabs); }
    else last = sel;
}










static void ui_npcdlg_pump(uint64_t root) {
    static dm_wref pw; static int ox, oy; int rx, ry, rw, rh, W = 0, H = 0; dm_ui_rect(&rx, &ry, &rw, &rh);
    if (!dm_uiscreen_ingame() || !dm_screen_real(&W, &H) || rh <= 0 || rh >= H) { dm_wref_set(&pw, 0); return; }
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60), p = 0;
    for (uint64_t k = 0; ch && k < n && k < 256 && !p; k++) { uint64_t c = *(uint64_t *)(ch + 8 * k); const char *nm = c ? (const char *)*(uint64_t *)(c + 8) : 0; if (nm && !strcmp(nm, "NpcDialogPanel")) p = c; }
    if (!p || *(uint8_t *)(p + 0x238)) { dm_wref_set(&pw, 0); return; }
    int32_t *f = (int32_t *)(p + 0x220), off = *(int32_t *)(p + 0x228); if ((p == dm_wref_get(&pw) && f[0] == ox && f[1] == oy) || (!f[0] && !f[1])) return;
    double lx = f[0], ly = f[1] - off; int nx = (int)lround(400 + (lx - 400) * H / rh), ny = (int)lround((ly * H / 600.0 - ry) * 600.0 / rh) + off;
    LOG("NPC 对话菜单：经典坐标 (%d,%d) → (%d,%d)（应有位置 真实画面 y %.0f）", f[0], f[1], nx, ny, ly * H / 600.0); f[0] = ox = nx; f[1] = oy = ny; dm_wref_set(&pw, p); *(uint8_t *)(p + 0x239) = 1;
}










static _Atomic uint64_t g_wtap;
static _Atomic int g_mercpg;
void dm_ui_world_tap(float ux, float uy, float wx, float wy) { if (dm_restart_busy()) return;
    uint64_t q[4]; float f[4] = { ux, uy, wx, wy };
    for (int i = 0; i < 2; i++) { float c = f[i] < -3.99f ? -3.99f : f[i] > 3.99f ? 3.99f : f[i]; q[i] = (uint64_t)(uint16_t)(int16_t)lroundf(c * 8192); }
    for (int i = 2; i < 4; i++) { float c = f[i] < 0 ? 0 : f[i] > 1 ? 1 : f[i]; q[i] = (uint64_t)(c * 32767 + .5f); }
    atomic_store(&g_wtap, 1ull << 63 | q[0] << 46 | q[1] << 30 | q[2] << 15 | q[3]);
}
static void ui_wtap_pump(module_t *m, uint64_t root, int width, int height) {
    uint64_t v = atomic_exchange(&g_wtap, 0); if (!(v >> 63)) return;
    int x = (int)lround((int16_t)(v >> 46 & 0xffff) / 8192.0 * width), y = (int)lround((int16_t)(v >> 30 & 0xffff) / 8192.0 * height);
    uint64_t p = ui_find(root, "HireablesPanel", 0), hd = p && *(uint8_t *)(p + 0x50) ? *(uint64_t *)(p + 0x1c8) : 0;
    if (hd && *(int32_t *)(m->base + 0x2d58cb0) != 2 && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)hd + 0x48))(hd, (uint64_t)(uint32_t)x | (uint64_t)(uint32_t)y << 32) & 1)) {
        atomic_store(&g_mercpg, 1); LOG("界面命中：(%d,%d) → 佣兵头像，开佣兵页", x, y); return; }




    { uint64_t sp = ui_find(root, "ScrollSpeechPanel", 0); static const char *part[2] = { "DialogBorder", "ScrollingText" };
      for (int k = 0; sp && *(uint8_t *)(sp + 0x50) && k < 2; k++) { uint64_t c = ui_find(sp, part[k], 0);
          if (c && *(uint8_t *)(c + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)c + 0x48))(c, (uint64_t)(uint32_t)x | (uint64_t)(uint32_t)y << 32) & 1)) {
              float ux = (v >> 45 & 0x7fff) / 32767.0f, uy = (v >> 30 & 0x7fff) / 32767.0f; dm_ui_touch(0, ux, uy); dm_ui_touch(2, ux, uy);
              LOG("界面命中：(%d,%d) → 讲话文字框 %s@ScrollSpeechPanel（单独开着），送界面点击关它", x, y, part[k]); return; } } }
    unsigned dm_world_tap(float, float); dm_world_tap((v >> 15 & 0x7fff) / 32767.0f, (v & 0x7fff) / 32767.0f);
}


static void ui_mercpg_pump(module_t *m, uint64_t root, uint64_t fr) {
    static int on, tries, page; static uint64_t pressed;
    if (atomic_exchange(&g_mercpg, 0) && !on) { on = 1; tries = 0; page = -1; pressed = 0; }
    if (!on) return;
    if (*(uint8_t *)(m->base + 0x2d5ff66)) { LOG("佣兵页：已开（按键 %d 次）", tries); on = 0; return; }
    if (tries >= 9) { LOG("【红】佣兵页：按了 %d 次键还没翻到，放弃", tries); on = 0; return; }
    int dm = bd_open(root, "UISwitcher"), cur = dm ? bd_top_tab(root) : -1;
    if (tries) { uint64_t dt = fr - pressed; int moved = dm && cur >= 0 && cur != page; if (!((moved && dt >= 7) || dt >= 24)) return; }
    page = cur; pressed = fr; dm_ui_key_pulse(dm ? DM_BTN_R : DM_BTN_PLUS, 4); tries++;
}






static void pages_text(uint8_t *b, uint64_t tb, int page, int total) {
    struct { const char *s; uint64_t n; } key = { "SharedStashPage", 15 }; uint64_t fmt = ((uint64_t (*)(void *))(b + 0x7f5630))(&key);
    char buf[256]; memset(buf, 0, sizeof buf); ((void (*)(void *, uint64_t, int64_t, int64_t))(b + 0x399130))(buf, fmt, page, total);
    ((void (*)(uint64_t, const char *, uint64_t))(b + 0x136e5c))(tb + 0x88, buf, buf[0] ? strlen(buf) : 0); ((void (*)(uint64_t))(b + 0xadc434))(tb);
}
static int g_bank_tab_keep = -1; static uint64_t g_bank_tab_until;
static int ui_touch_or_pad_ready(void) { int dm_padmode_busy(void); return !dm_padmode_busy(); }
static void ui_pages_pump(module_t *m, uint64_t root) {
    static int lt = -1, lp = -1; uint64_t bk = ui_panel(root, "BankExpansionLayout");
    if (!bk || !*(uint8_t *)(bk + 0x50)) { lt = lp = -1; return; }
    uint64_t tabs = ui_find(bk, "BankTabs", 0), pg = ui_find(bk, "TouchPgPage", 0), tb = pg ? ui_find(pg, "TouchPgPage_Text", 0) : 0;




    if (g_bank_tab_keep >= 0 && tabs && ui_touch_or_pad_ready()) { int keep = g_bank_tab_keep; g_bank_tab_keep = -1;
        int was = TAB_SEL(tabs); if (dm_guest_frame() <= g_bank_tab_until && was != keep) { ((void (*)(uint64_t, int, int))(m->base + 0xad9b80))(tabs, keep, 0); LOG("储藏箱：切换布局后页签从第 %d 个选回第 %d 个（现为 %d）", was, keep, TAB_SEL(tabs)); } }
    if (!tabs || !pg || !tb) return;
    int tab = TAB_SEL(tabs), cur; int dm_pages_cur(void), dm_pages_count(void); cur = dm_pages_cur(); uint8_t *b = (uint8_t *)m->base;
    ui_move_ours(pg, tab == 0 ? 0 : 99999);
    if (tab == 0 && (lt != 0 || lp != cur)) { pages_text(b, tb, cur + 1, dm_pages_count()); lp = cur; LOG("个人仓库：页码写成第 %d/%d 页", cur + 1, dm_pages_count()); }
    if (tab != 0) lp = -1;
    lt = tab;






    { int dm_padmode_current(void); void dm_padmode_legend(int, const char *), dm_pages_request_flip(int); uint64_t dm_padmode_page_keys(uint64_t);
      if (tab == 0 && dm_padmode_current()) { uint64_t ev = dm_padmode_page_keys(DM_BTN_L | DM_BTN_R);
          dm_padmode_legend(19, dm_str(b, "dmPadLegPrev")); dm_padmode_legend(20, dm_str(b, "dmPadLegNext"));
          if (ev & DM_BTN_L) { dm_pages_request_flip(-1); LOG("个人仓库：手柄 L → 上一页"); } else if (ev & DM_BTN_R) { dm_pages_request_flip(1); LOG("个人仓库：手柄 R → 下一页"); } } }
}








static void ui_widestash_pump(uint64_t root) {
    uint64_t bank = ui_panel(root, "BankExpansionLayout"); int on = bank && has_visible_leaf(bank, 1); static int was = -1; g_mv_want[MV_WIDE] = on;
    if (!on && !was && !mv_count(MV_WIDE)) return;
    uint64_t ov = ui_panel(root, "ControllerOverlay");


    if (!ov || !ui_find(ov, "WideBackground", 0)) g_mv_want[MV_WIDE] = on = 0;
    if (on != was) { LOG("储藏箱加宽：%s", on ? "开（宽外框、物品栏右移）" : "关（还原）"); was = on; }


    if (!on) { mv_restore(root, MV_WIDE); return; }
    uint64_t tb = ui_find(ov, "Tabs", 0);


    static const struct { const char *nm; int in, dx, dw; } t[] = { { "WideBackground", 1, -196 - 99999, 0 }, { "Text0", 2, -196, 392 }, { "Text1", 2, 196, 0 },
        { "TabsEndcapLeft", 1, -196, 0 }, { "TabsEndcapRight", 1, 196, 0 }, { "ControllerCursorBounds", 1, -196, 392 }, { "BankExpansionLayout", 0, -196, 392 }, { "PlayerInventoryExpansionLayout", 0, 196, 0 } };
    for (int i = 0; i < 8; i++) { uint64_t w = t[i].in == 2 ? (tb ? ui_find(tb, t[i].nm, 0) : 0) : t[i].in ? ui_find(ov, t[i].nm, 0) : ui_panel(root, t[i].nm); if (!w) continue;
        float k = t[i].in ? 1.0f : *(float *)(w + 0x80);
        mv_offset(MV_WIDE, w, 0, (int)lroundf(t[i].dx * k), 1); mv_offset(MV_WIDE, w, 2, (int)lroundf(t[i].dw * k), 1); }
}








static _Atomic int g_safe_r;
static _Atomic int g_safe_l;
void dm_ui_safe_left(int units) { atomic_store(&g_safe_l, units < 0 ? 0 : units > 800 ? 800 : units); }
void dm_ui_safe_right(int units) { atomic_store(&g_safe_r, units < 0 ? 0 : units > 800 ? 800 : units); }
static void ui_safe_pump(module_t *m, uint64_t root, uint64_t fr) {
    static uint64_t next; if (fr < next) return; next = fr + 6; int u = atomic_load(&g_safe_r);
    int ul0 = atomic_load(&g_safe_l); g_mv_want[MV_SAFE] = dm_uiscreen_ingame() && (u || ul0);
    if (!g_mv_want[MV_SAFE]) { mv_restore(root, MV_SAFE); return; }
    uint64_t w = ui_panel(root, "AutoMap");
    if (w && *(uint64_t *)w == (uint64_t)m->base + 0x20c58a0) {
        int cfg = *(volatile int32_t *)(m->base + 0x2d479f4), mini = *(volatile int32_t *)(m->base + 0x2d5c748); int32_t *rx = (int32_t *)(w + 0x70), ww = *(int32_t *)(w + 0x78);
        int on = cfg == 2 && mini && ww > 0 && ww < 4000; int32_t x0 = *rx;








        g_mv_base = -ww; g_mv_base_on = on;
        mv_offset(MV_SAFE, w, 0, on ? -u : 0, 0); g_mv_base_on = 0; if (*rx != x0) LOG("安全区：小地图矩形 x %d → %d", x0, *rx);
    }






    { uint64_t p = ui_panel(root, "HireablesPanel"); int ul = atomic_load(&g_safe_l);
      uint64_t ch = p ? *(uint64_t *)(p + 0x58) : 0, n = p ? *(uint64_t *)(p + 0x60) : 0;
      for (uint64_t k = 0; ch && k < n && k < 40; k++) { uint64_t c = *(uint64_t *)(ch + 8 * k); if (!c) continue; int32_t x0 = *(int32_t *)(c + 0x70);
          mv_offset(MV_SAFE, c, 0, ul, 1); if (*(int32_t *)(c + 0x70) != x0) { static int lg; if (lg++ < 6) LOG("安全区：头像 %d x %d → %d", (int)k, x0, *(int32_t *)(c + 0x70)); } } }
    uint64_t h = ui_panel(root, "HUDWarnings");
    if (h) { int32_t x0 = *(int32_t *)(h + 0x70); mv_offset(MV_SAFE, h, 0, -u, 0); if (*(int32_t *)(h + 0x70) != x0) LOG("安全区：信息栏 x %d → %d", x0, *(int32_t *)(h + 0x70)); }

}
















static _Atomic uint64_t g_portrait;
static int portrait_read(module_t *m, uint64_t root, int fake_row) {
    const uint8_t *b = m->base; uint64_t p = ui_panel(root, "HireablesPanel");
    if (!p || !dm_widget_is(b, p, 0x2089568)) return DM_PT_UNKNOWN;
    if (!*(uint8_t *)(p + 0x50) || *(volatile int32_t *)(m->base + 0x2d58cb0) == 2) return DM_PT_EMPTY;
    float pax = *(float *)(p + 0x48), pay = *(float *)(p + 0x4c), ps = *(float *)(p + 0x80); int32_t py = *(int32_t *)(p + 0x74);
    if (pax != 0 || pay != 0 || !dm_pt_finite_in(ps, 0.1f, 10)) return DM_PT_UNKNOWN;
    uint64_t ch = *(uint64_t *)(p + 0x58), n = *(uint64_t *)(p + 0x60); if (n && (!ch || n > 40)) return DM_PT_UNKNOWN;
    float lowest = -INFINITY; int seen = 0;
    for (uint64_t k = 0; k < n; k++) {
        uint64_t c = *(uint64_t *)(ch + 8 * k); if (!c || !(*(uint8_t *)(c + 0x50) || (fake_row && k == 0))) continue;
        if (!dm_widget_is(b, c, 0x20b73a0) || *(float *)(c + 0x48) != 0 || *(float *)(c + 0x4c) != 0) return DM_PT_UNKNOWN;
        dm_pt_icon ic = { *(int32_t *)(c + 0x74), *(int32_t *)(c + 0x7c), *(float *)(c + 0x80), 0, 0, 0, 0, 0 };
        uint64_t cc = *(uint64_t *)(c + 0x58), cn = *(uint64_t *)(c + 0x60); if (cn && (!cc || cn > 8)) return DM_PT_UNKNOWN;
        for (uint64_t j = 0; j < cn; j++) { uint64_t t = *(uint64_t *)(cc + 8 * j); const char *tn = t ? *(const char **)(t + 8) : NULL;
            if (!tn || strcmp(tn, "Name") || !*(uint8_t *)(t + 0x50)) continue;
            if (!dm_widget_is(b, t, DM_VT_TEXT)) return DM_PT_UNKNOWN;
            uint64_t len = *(uint64_t *)(t + 0x90); if (!len) continue; if (len > 256) return DM_PT_UNKNOWN;
            ic.name = 1; ic.name_y = *(int32_t *)(t + 0x74); ic.name_anchor = *(float *)(t + 0x4c); ic.name_scale = *(float *)(t + 0x80); ic.name_pt = *(int32_t *)(t + 0xb8); }
        float bt = dm_pt_icon_bottom(py, ps, &ic); if (!isfinite(bt)) return DM_PT_UNKNOWN;
        if (bt > lowest) lowest = bt; seen++;
    }
    if (!seen) return DM_PT_EMPTY;
    int ux, uy, uw, uh, W = 0, H = 0; dm_ui_rect(&ux, &uy, &uw, &uh); if (!dm_screen_real(&W, &H)) return DM_PT_UNKNOWN;
    return dm_pt_code(lowest, uy, uh, H);
}
static void ui_portrait_pump(module_t *m, uint64_t root, uint64_t fr) {
    static int t_kind = -1, t_at, t_len; static uint64_t in0;
    if (t_kind < 0) { const char *e = getenv("DM_TEST_PORTRAIT"); char k[12] = ""; t_kind = 0;
        if (e && sscanf(e, "%d:%d:%11s", &t_at, &t_len, k) == 3) { t_kind = !strcmp(k, "row") ? 1 : !strcmp(k, "empty") ? 2 : !strcmp(k, "unknown") ? 3 : 0; if (t_kind) LOG("DM_TEST_PORTRAIT：进局后第 %d 帧起 %d 帧 %s", t_at, t_len, k); } }
    int code = DM_PT_UNKNOWN, fake = 0;
    if (m && root && dm_uiscreen_ingame()) {
        if (!in0) in0 = fr;
        if (t_kind && fr >= in0 + (uint64_t)t_at && fr < in0 + (uint64_t)t_at + (uint64_t)t_len) fake = t_kind;
        if (dm_ui_unstable() && !fake) return;
        code = fake == 2 ? DM_PT_EMPTY : fake == 3 ? DM_PT_UNKNOWN : portrait_read(m, root, fake == 1);
    } else in0 = 0;
    static int last = -3, lg; if (code != last && lg < 200) { lg++; LOG("头像栏下沿：%d（-2 读不到，-1 空，其余 = 画面高的 1/100000）", code); }
    last = code;
    atomic_store(&g_portrait, dm_pt_pack(now_ns() / 1000000, code));
}
int dm_ui_portrait_bottom(void) { return dm_pt_unpack(atomic_load(&g_portrait), now_ns() / 1000000); }










static pthread_mutex_t g_lw_lock = PTHREAD_MUTEX_INITIALIZER; static char g_lw[48];
static void ui_level_word_pump(module_t *m, uint64_t fr) {
    static uint64_t next; if (fr < next) return; next = fr + 30;
    char w[48] = "";
    if (m && dm_uiscreen_ingame()) {
        struct { const char *k; uint64_t n; } key = { "strchrlvl", 9 }; const char *s = ((const char *(*)(void *))(m->base + 0x7f5630))(&key);
        if (s) { s = dm_untag(s); size_t n = strnlen(s, sizeof w); int ok = n > 0 && n < sizeof w && strcmp(s, "strchrlvl");
            for (size_t i = 0; ok && i < n; i++) ok = (uint8_t)s[i] >= 0x20 && (uint8_t)s[i] != 0x7f && !((uint8_t)s[i] == 0xc3 && (uint8_t)s[i + 1] == 0xbf);
            if (ok) memcpy(w, s, n + 1); }
    }
    pthread_mutex_lock(&g_lw_lock); int changed = strcmp(g_lw, w) != 0; if (changed) memcpy(g_lw, w, sizeof w); pthread_mutex_unlock(&g_lw_lock);
    if (changed) LOG("底栏等级字：「%s」", w);
}
int dm_ui_level_word(char *out, int cap) {
    if (!out || cap <= 0) return 0;
    pthread_mutex_lock(&g_lw_lock); int n = (int)strnlen(g_lw, sizeof g_lw); if (n >= cap) n = 0; memcpy(out, g_lw, (size_t)n); pthread_mutex_unlock(&g_lw_lock);
    out[n < cap ? n : 0] = 0; return n;
}






static _Atomic uint64_t g_mapgeom;
static void ui_map_geom_pump(module_t *m, uint64_t root) {
    uint64_t am = ui_panel(root, "AutoMap"); int mode = 0; float cell = 0;
    if (am && *(uint64_t *)am == (uint64_t)m->base + 0x20c58a0 && *(uint8_t *)(am + 0x50)) {
        mode = *(volatile int32_t *)(m->base + 0x2d5c748) == 0 ? 2 : 1;
        float q = ceilf(((float (*)(uint64_t))(m->base + 0x390b38))(am) * *(float *)(am + (mode == 2 ? 0x15a8 : 0x15ac)) * 8) / 8; if (q < 0.125f) q = 0.125f;
        cell = q * *(float *)(m->base + 0x2d5e918); }
    uint32_t cb; memcpy(&cb, &cell, 4); atomic_store(&g_mapgeom, (uint64_t)mode << 32 | cb);
}
int dm_ui_map_geom(float *cell) { uint64_t v = atomic_load(&g_mapgeom); uint32_t cb = (uint32_t)v; memcpy(cell, &cb, 4); return (int)(v >> 32); }






static void ui_boss_pct_pump(module_t *m, uint64_t root) {
    uint64_t panel = ui_panel(root, "HUDMonsterHealth"); if (!panel || !*(uint8_t *)(panel + 0x50)) return;
    uint64_t hp = ui_find(panel, "Health", 1), tx = hp ? ui_find(hp, "TouchBossPct", 1) : 0;
    if (!tx || *(uint64_t *)hp != (uint64_t)m->base + 0x2089650 || *(uint64_t *)tx != (uint64_t)m->base + 0x20ba4c8) return;
    float v = *(float *)(hp + 0x154); if (!(v >= 0 && v <= 1)) return;
    char t[8]; int n = snprintf(t, sizeof t, "%d%%", (int)lroundf(v * 100));
    if (*(uint64_t *)(tx + 0x90) == (uint64_t)n && !memcmp(*(const char **)(tx + 0x88), t, (size_t)n)) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(m->base + 0x136e5c))(tx + 0x88, t, (uint64_t)n); ((void (*)(uint64_t))(m->base + 0xadc434))(tx);
}




typedef struct { uint64_t object; int valid, area, ingame, load, cfg, mini, visible, x, y, w, h, ux, uy, uw, uh, rw, rh; } map_diag_t;
static map_diag_t g_map_diag; static int g_map_diag_have;
static int ui_map_diag(module_t *m, uint64_t root) {
    map_diag_t d; memset(&d, 0, sizeof d); dm_hud_info hi; dm_hud_get(&hi); d.valid = hi.valid; d.area = hi.area;
    d.ingame = dm_uiscreen_ingame(); d.load = atomic_load(&g_load_up);
    d.cfg = *(int32_t *)(m->base + 0x2d479f4); d.mini = *(int32_t *)(m->base + 0x2d5c748);
    dm_ui_rect(&d.ux, &d.uy, &d.uw, &d.uh); dm_screen_real(&d.rw, &d.rh);
    uint64_t am = ui_panel(root, "AutoMap");
    if (am && *(uint64_t *)am == (uint64_t)m->base + 0x20c58a0) { d.object = am; d.visible = *(uint8_t *)(am + 0x50);
        d.x = *(int32_t *)(am + 0x70); d.y = *(int32_t *)(am + 0x74); d.w = *(int32_t *)(am + 0x78); d.h = *(int32_t *)(am + 0x7c); }
    if (g_map_diag_have && !memcmp(&d, &g_map_diag, sizeof d)) return 0;
    int rebuilt = !g_map_diag_have || d.object != g_map_diag.object; memcpy(&g_map_diag, &d, sizeof d); g_map_diag_have = 1;
    LOG("小地图诊断：区域 %d 玩家有效 %d 局内 %d 读取 %d 对象 %s 重建 %d 可见 %d 配置 %d 微缩 %d 矩形 %d,%d,%d,%d 界面 %d,%d,%d,%d 真实 %dx%d", d.area, d.valid, d.ingame, d.load, d.object ? "有" : "无", rebuilt, d.visible, d.cfg, d.mini, d.x, d.y, d.w, d.h, d.ux, d.uy, d.uw, d.uh, d.rw, d.rh);
    return 1;
}
static void ui_minimap_pump(module_t *m, uint64_t root, int W, int H, uint64_t fr) {
    static uint64_t next; if (fr < next) return; next = fr + 30;
    ui_map_diag(m, root);
    uint64_t w = ui_panel(root, "AutoMap"); if (!w || *(uint64_t *)w != (uint64_t)m->base + 0x20c58a0) return;
    int32_t x = *(int32_t *)(w + 0x70), y = *(int32_t *)(w + 0x74), ww = *(int32_t *)(w + 0x78), hh = *(int32_t *)(w + 0x7c); float sc = *(float *)(w + 0x80);
    if (ww <= 0 || hh <= 0 || ww > 4000 || hh > 2160 || !(sc > 0.1f && sc < 10)) return;
    double u = H / 2160.0, x0 = (x < 0 ? W + x * u : x * u), y0 = y * u, x1 = x0 + ww * sc * u, y1 = y0 + hh * sc * u;
    if (x0 < 0) x0 = 0; if (x1 > W) x1 = W; if (y1 > H) y1 = H; if (x1 - x0 < 8 || y1 - y0 < 8) return;


    #define MM_Q(val) ((uint64_t)(uint16_t)(int16_t)lround((val) * 8192.0))
    uint64_t v = MM_Q(x0 / W) << 48 | MM_Q(y0 / H) << 32 | MM_Q(x1 / W) << 16 | MM_Q(y1 / H);
    #undef MM_Q
    int chg = !atomic_load(&g_mm_have) || atomic_load(&g_mm_rect) != v; atomic_store(&g_mm_rect, v); atomic_store(&g_mm_have, 1);
    if (chg) { float r[4]; dm_ui_minimap_rect(r); LOG("小地图矩形：控件 (%d,%d,%d,%d) 缩放 %.2f → 画面 %dx%d 里 x %.0f–%.0f、y %.0f–%.0f；前端读到 x %.0f–%.0f、y %.0f–%.0f", x, y, ww, hh, sc, W, H, x0, x1, y0, y1, r[0] * W, r[2] * W, r[1] * H, r[3] * H); }
}






static char g_wn[2][64]; static _Atomic int g_wn_on; static _Atomic uint64_t g_wn_pos;
void dm_ui_watch(const char *spec) {
    atomic_store(&g_wn_on, 0); atomic_store(&g_wn_pos, 0); if (!spec) return;
    const char *at = strchr(spec, '@'); snprintf(g_wn[0], sizeof g_wn[0], "%.*s", at ? (int)(at - spec) : 63, spec); snprintf(g_wn[1], sizeof g_wn[1], "%s", at ? at + 1 : "");
    atomic_store(&g_wn_on, 1);
}
int dm_ui_watch_pos(float *nx, float *ny) { uint64_t v = atomic_load(&g_wn_pos); if (!v) return 0; *nx = ((v >> 16) & 0xffff) / 65535.0f; *ny = (v & 0xffff) / 65535.0f; return 1; }
static uint64_t vis_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    const char *nm = (const char *)*(uint64_t *)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (int i = (int)n - 1; i >= 0; i--) { uint64_t r = vis_find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}


static int ui_widget_center(uint64_t t, int w, int h, float *nx, float *ny) {
    int (*hit)(uint64_t, uint64_t) = (int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)t + 0x48);
    enum { GX = 128, GY = 72 }; static uint8_t on[GY][GX]; double sx = 0, sy = 0; int n = 0;
    for (int j = 0; j < GY; j++) for (int i = 0; i < GX; i++) { int x = (int)((i + 0.5) * w / GX), y = (int)((j + 0.5) * h / GY); on[j][i] = hit(t, SNAP_XY(x, y)) & 1; if (on[j][i]) { sx += x; sy += y; n++; } }
    if (!n) return 0;
    sx /= n; sy /= n; double best = 1e18; int bx = 0, by = 0;
    for (int j = 0; j < GY; j++) for (int i = 0; i < GX; i++) if (on[j][i]) { double x = (i + 0.5) * w / GX, y = (j + 0.5) * h / GY, d = (x - sx) * (x - sx) + (y - sy) * (y - sy); if (d < best) { best = d; bx = (int)x; by = (int)y; } }
    *nx = (float)((bx + 0.5) / w); *ny = (float)((by + 0.5) / h); return 1;
}
static void ui_watch_pump(void) {
    if (!atomic_load(&g_wn_on)) return;
    module_t *m = main_module(); if (!m) return;
    int32_t w = *(int32_t *)(m->base + 0x211da38), h = *(int32_t *)(m->base + 0x211da3c); uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8);
    if (w <= 0 || h <= 0 || !root) return;
    uint64_t p = g_wn[1][0] ? vis_find(root, g_wn[1], 0) : root, t = p ? vis_find(p, g_wn[0], 0) : 0;
    if (!t) { atomic_store(&g_wn_pos, 0); return; }
    float nx, ny; if (!ui_widget_center(t, w, h, &nx, &ny)) { atomic_store(&g_wn_pos, 0); return; }
    uint64_t v = ((uint64_t)(uint32_t)(nx * 65535) << 16) | (uint32_t)(ny * 65535);
    atomic_store(&g_wn_pos, v ? v : 1);
}
















int dm_ui_panel_resident(const char *name) {
    static const char *k[] = { "EmoteWheelPanel", "HUDLevelNames", "HUDMessagePanel", "HUDMonsterHealth", "HUDPanel", "HUDWarnings", "HireablesPanel",
        "InGameMoviePlayerPanel", "MiniMenuPanel", "PaladinAuraPanel", "PanelBordersPanel", "ShowItemsPanel", "TooltipsPanel",
        "SaveIndicatorPanel", "NotificationPanel", "PartyPanelMessage", "HelpButtonPanel", "ControllerOverlay", "UISwitcher", "AutoMap",






        "CandidateListPanel" };
    if (!name) return 0; for (unsigned i = 0; i < sizeof k / sizeof *k; i++) if (!strcmp(name, k[i])) return 1; return 0;
}






enum { HUD_STACK_MAX = 64 };
static struct { char name[64]; int open; } g_hud_stack[HUD_STACK_MAX];
__attribute__((weak)) void dm_platform_menu_ready(void) {}
static void hud_stack_update(const char *act, const char *arg) {






    if(!strcmp(act,"UnloadPanel") && !strcmp(arg,"LoadScreenPanel")) {
        for(int i=0;i<HUD_STACK_MAX;i++) {
            if(g_hud_stack[i].open>0 && strcmp(g_hud_stack[i].name,"LoadScreenPanel") && dm_ui_panel_shown(g_hud_stack[i].name)) fprintf(stderr,"[hud] 面板栈 读取画面撤下：%s 屏上还在，照记\n",g_hud_stack[i].name);
            else memset(&g_hud_stack[i],0,sizeof g_hud_stack[i]);
        }
    }
    else if(!strcmp(act,"UnloadAll")) memset(g_hud_stack,0,sizeof g_hud_stack);
    else {






        int tog=!strcmp(act,"TogglePanel");
        int open=!strcmp(act,"OpenPanel")||!strcmp(act,"OpenExclusivePanel"), close=!strcmp(act,"ClosePanel")||!strcmp(act,"UnloadPanel");
        if(!open && !close && !tog) return;
        if(dm_ui_panel_resident(arg)) return;
        int slot=-1; for(int i=0;i<HUD_STACK_MAX;i++) if(!strcmp(g_hud_stack[i].name,arg)) { slot=i; break; }
        if(slot<0) for(int i=0;i<HUD_STACK_MAX;i++) if(!g_hud_stack[i].name[0]) { slot=i; strlcpy(g_hud_stack[i].name,arg,sizeof g_hud_stack[i].name); break; }
        if(slot<0) return;
        if(tog) { open=g_hud_stack[slot].open<=0; close=!open; }




        if(close && g_hud_stack[slot].open<=0) return;








        if(open && g_hud_stack[slot].open>0) { fprintf(stderr,"[hud] 面板栈 %s %s：已经开着，不重复计\n",act,arg); return; }
        g_hud_stack[slot].open=open?1:0; if(open && !strcmp(arg,"MainMenuButtonRibbon")) dm_platform_menu_ready();
        fprintf(stderr,"[hud] 面板栈 %s %s → %d\n",act,arg,g_hud_stack[slot].open);
    }
}





void (*dm_qa_panels_hook)(int n, const char *const *open_names);
void (*dm_qa_tooltip_hook)(const char *text, uint32_t hovered);
int dm_hud_stack_msg(const char *act, const char *arg) {
    hud_stack_update(act, arg);
    if (dm_qa_panels_hook) { const char *nm[HUD_STACK_MAX]; int k = 0; for (int i = 0; i < HUD_STACK_MAX; i++) if (g_hud_stack[i].open > 0) nm[k++] = g_hud_stack[i].name; dm_qa_panels_hook(k, nm); }




    int n=0, dlg=0; for(int i=0;i<HUD_STACK_MAX;i++) { if(!strcmp(g_hud_stack[i].name,"NpcDialogPanel")||!strcmp(g_hud_stack[i].name,"ScrollSpeechPanel")) dlg|=g_hud_stack[i].open>0; else n+=g_hud_stack[i].open; }
    if(!n && dlg) n=-1;
    return n;
}
void dm_hud_stack_dump(void) { for (int i = 0; i < HUD_STACK_MAX; i++) if (g_hud_stack[i].open) fprintf(stderr, "%s×%d ", g_hud_stack[i].name, g_hud_stack[i].open); }
static struct { char name[48]; int open; } g_pm[48];
static void ui_panel_note(const char *act, const char *arg) {
    if (!strcmp(act, "UnloadAll")) { memset(g_pm, 0, sizeof g_pm); return; }
    if (!strcmp(act, "UnloadPanel") && arg && !strcmp(arg, "LoadScreenPanel")) {
        for (int i = 0; i < 48; i++) if (!g_pm[i].open || !strcmp(g_pm[i].name, "LoadScreenPanel") || !dm_ui_panel_shown(g_pm[i].name)) memset(&g_pm[i], 0, sizeof g_pm[i]);
        return; }


    int tg = !strcmp(act, "TogglePanel"), op = !strcmp(act, "OpenPanel") || !strcmp(act, "OpenExclusivePanel"), cl = !strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel");
    if ((!op && !cl && !tg) || !arg || !arg[0] || panel_skip(arg) || dm_ui_panel_resident(arg)) return;
    int k = -1; for (int i = 0; i < 48; i++) if (!strcmp(g_pm[i].name, arg)) { k = i; break; }
    if (tg) { op = k < 0 || !g_pm[k].open; cl = !op; }
    if (k < 0 && op) for (int i = 0; i < 48; i++) if (!g_pm[i].name[0]) { k = i; strlcpy(g_pm[i].name, arg, sizeof g_pm[i].name); break; }
    if (k < 0) return;
    g_pm[k].open = op;
}








static void ui_panel_heal_open(uint64_t ch, uint64_t n) {
    void uimsg_emit(const char *, const char *, const char *);
    static struct { char name[48]; int miss; } seen[8]; int keep[8] = {0};
    for (uint64_t j = 0; j < n; j++) { uint64_t p = *(uint64_t *)(ch + 8 * j); const char *nm = p ? (const char *)*(uint64_t *)(p + 8) : NULL;
        if (!nm || !nm[0] || strlen(nm) >= 48 || panel_skip(nm) || dm_ui_panel_resident(nm) || !*(uint8_t *)(p + 0x50) || !has_visible_leaf(p, 1)) continue;
        int on = 0; for (int i = 0; i < 48; i++) if (g_pm[i].open && !strcmp(g_pm[i].name, nm)) { on = 1; break; }
        if (on) continue;
        int k = -1; for (int i = 0; i < 8; i++) if (!strcmp(seen[i].name, nm)) { k = i; break; }
        if (k < 0) for (int i = 0; i < 8; i++) if (!seen[i].name[0]) { k = i; strlcpy(seen[i].name, nm, sizeof seen[i].name); seen[i].miss = 0; break; }
        if (k < 0) continue;
        keep[k] = 1;
        if (++seen[k].miss < 3) continue;
        LOG("【红】界面安全阀：%s 在屏上开着 1.5 秒，界面消息上却没记它开着 → 补发 OpenPanel（有一条打开没记上或被清掉了，去查这段消息序列）", nm);
        ui_panel_note("OpenPanel", nm); uimsg_emit("PanelManager", "OpenPanel", nm); keep[k] = 0; }
    for (int i = 0; i < 8; i++) if (!keep[i]) seen[i].name[0] = 0;
}










static char g_chr_child[8][48]; static _Atomic int g_chr_closed;
static void ui_chronicle_children(void) {
    if (!atomic_exchange(&g_chr_closed, 0)) return; void uimsg_emit(const char *, const char *, const char *);
    for (int c = 0; c < 8; c++) { if (!g_chr_child[c][0]) continue;
        for (int i = 0; i < 48; i++) if (g_pm[i].open && !strcmp(g_pm[i].name, g_chr_child[c])) { g_pm[i].open = 0; uimsg_emit("PanelManager", "ClosePanel", g_chr_child[c]); LOG("界面：编年史关了 → 替子页 %s 补发 ClosePanel", g_chr_child[c]); } }
}
static void ui_panel_heal(uint64_t root, uint64_t fr) {
    ui_chronicle_children();
    static uint64_t next; static int miss; if (fr < next) return; next = fr + 30;
    void uimsg_emit(const char *, const char *, const char *); dm_hud_info hi; dm_hud_get(&hi); if (!hi.valid) { miss = 0; return; }
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60); if (!ch || n > 256) return;
    int stale = 0, alive = 0;
    for (int i = 0; i < 48; i++) { if (!g_pm[i].name[0] || !g_pm[i].open) continue;
        int shown = 0;
        for (uint64_t j = 0; j < n; j++) { uint64_t p = *(uint64_t *)(ch + 8 * j); const char *nm = p ? (const char *)*(uint64_t *)(p + 8) : NULL;
            if (nm && !strcmp(nm, g_pm[i].name)) { shown = *(uint8_t *)(p + 0x50) && has_visible_leaf(p, 1); break; } }
        if (shown) alive = 1; else stale++; }
    ui_panel_heal_open(ch, n);
    if (alive || !stale) { miss = 0; return; }
    if (++miss < 3) return;
    miss = 0;
    for (int i = 0; i < 48; i++) { if (!g_pm[i].name[0] || !g_pm[i].open) continue;
        char nm[48]; strlcpy(nm, g_pm[i].name, sizeof nm);
        LOG("【红】界面安全阀：%s 在界面消息上还开着，但屏上已经没有任何面板 1.5 秒 → 补发 ClosePanel（有一条没见过的只开不关，去查这段消息序列）", nm);
        uimsg_emit("PanelManager", "ClosePanel", nm); g_pm[i].open = 0; }
}




static _Atomic int g_pstate;
static _Atomic int g_dead_reopen; void dm_dead_modal_mode_enter(void) { atomic_store(&g_dead_reopen, 1); }
int dm_ui_panel_state(void) { return atomic_load(&g_pstate); }
static int root_panel_shown(uint64_t root, const char *name) {
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t j = 0; j < n; j++) { uint64_t p = *(uint64_t *)(ch + 8 * j); const char *nm = p ? (const char *)*(uint64_t *)(p + 8) : NULL;
        if (nm && !strcmp(nm, name)) return *(uint8_t *)(p + 0x50) && has_visible_leaf(p, 1); }
    return 0;
}
int dm_ui_panel_shown(const char *name) {
    module_t *m = main_module(); uint64_t root = 0; if (m) memcpy(&root, m->base + 0x2f05458, 8);
    return root && name && name[0] && root_panel_shown(root, name);
}






static uint64_t ui_panel(uint64_t root, const char *name) {
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t j = 0; j < n; j++) { uint64_t p = *(uint64_t *)(ch + 8 * j); const char *nm = p ? (const char *)*(uint64_t *)(p + 8) : NULL; if (nm && !strcmp(nm, name)) return p; }
    return 0;
}


static _Atomic uint64_t g_sort_btn[2];
enum { SORT_INV_DX = 816, SORT_INV_DY = 1805, SORT_CUBE_DX = 749, SORT_CUBE_DY = 1774, SORT_BANK_DX = 969, SORT_BANK_DY = 1774 };
static uint64_t sort_place(uint64_t panel, int page, int dx, int dy) {
    int ux, uy, uw, uh, W = 0, H = 0; dm_ui_rect(&ux, &uy, &uw, &uh); dm_screen_real(&W, &H); if (!panel || uh <= 0 || W <= 0 || H <= 0) return 0;
    int32_t x = *(int32_t *)(panel + 0x70), y = *(int32_t *)(panel + 0x74); float ax = *(float *)(panel + 0x48), ay = *(float *)(panel + 0x4c); double u = uh / 2160.0, px = ux + ax * uw + (x + dx) * u, py = uy + ay * uh + (y + dy) * u;
    if (px < 0 || px > W || py < 0 || py > H) return 0;
    return (uint64_t)(page + 1) << 32 | (uint64_t)lround(px / W * 65535) << 16 | (uint64_t)lround(py / H * 65535);
}
int dm_ui_sort_buttons(float *out) {
    int n = 0; if (dm_itemjob_owner()) return 0;
    for (int i = 0; i < 2; i++) { uint64_t v = atomic_load(&g_sort_btn[i]); if (!v) continue; out[n * 3] = (float)((v >> 32) - 1); out[n * 3 + 1] = (float)(v >> 16 & 0xffff) / 65535; out[n * 3 + 2] = (float)(v & 0xffff) / 65535; n++; }
    return n;
}
static void ui_panel_state_pump(uint64_t root) {
    void uimsg_emit(const char *, const char *, const char *);
    int st = (root_panel_shown(root, "BankExpansionLayout") ? DM_PS_BANK : 0) | (dm_shop_open() ? DM_PS_VENDOR : 0)
           | (root_panel_shown(root, "YouDiedModal") ? DM_PS_DEAD : 0) | (root_panel_shown(root, "ImbueItemsPanel") ? DM_PS_IMBUE : 0) | (root_panel_shown(root, "HoradricCubeLayout") ? DM_PS_CUBE : 0);
    if (st & DM_PS_BANK) { uint64_t bk = ui_panel(root, "BankExpansionLayout"), tabs = bk ? ui_find(bk, "BankTabs", 0) : 0; if (tabs && TAB_SEL(tabs) == 5) st |= DM_PS_VAULT; }




    if (atomic_exchange(&g_dead_reopen, 0) && (st & DM_PS_DEAD)) { fe_send("PanelManager", "ClosePanel", "YouDiedModal"); fe_send("PanelManager", "OpenPanel", "YouDiedModal"); LOG("手柄 / 触屏布局：死亡弹窗开着切了模式 → 让原版按当前布局重开一次"); }
    {








        uint64_t cube = ui_panel(root, "HoradricCubeLayout"), inv = ui_panel(root, "PlayerInventoryExpansionLayout"); int both = (st & DM_PS_BANK) && (st & DM_PS_CUBE) && cube && inv; static int folded;
        g_mv_want[MV_CUBE] = both; if (!both) folded = 0;
        if (both && atomic_exchange(&g_cube_side_close, 0) && !folded) { folded = 1; LOG("魔盒并排：收起魔盒面板，回到储藏箱 + 物品栏"); }
        if (both && atomic_exchange(&g_cube_side_req, 0) && folded) { folded = 0; LOG("魔盒并排：魔盒面板重新并排"); }
        if (both) { int i_ = 0; while (i_ < g_nmv && !(g_mv[i_].r.w == cube && g_mv[i_].f == 0)) i_++;
            int32_t cx0 = i_ < g_nmv && *(int32_t *)(cube + 0x70) == g_mv[i_].val ? g_mv[i_].orig : *(int32_t *)(cube + 0x70);
            mv_offset(MV_CUBE, cube, 0, folded ? 0 : *(int32_t *)(inv + 0x70) - 155 - cx0, 1); mv_offset(MV_CUBE, cube, 1, folded ? 6000 : 0, 1); mv_offset(MV_CUBE, inv, 1, folded ? 0 : 6000, 1); }
        else if (mv_count(MV_CUBE)) mv_restore(root, MV_CUBE);


        { uint64_t ov = ui_panel(root, "ControllerOverlay"), t0 = ov ? ui_find(ov, "Text0", 0) : 0, t1 = ov ? ui_find(ov, "Text1", 0) : 0, tt = ov ? ui_find(ov, "TouchSideTitle", 0) : 0;
          if (both) { mv_offset(MV_CUBE, t0, 1, 6000, 1); mv_offset(MV_CUBE, t1, 1, folded ? 0 : 6000, 1); }
          if (tt) { ui_move_ours(tt, both ? -196 : 99999); ui_show_ours(ui_find(tt, "TouchSideTitle_R", 0), both && !folded); } }
        atomic_store(&g_cube_side_on, both && !folded);
        if (!both) atomic_store(&g_cube_side_close, 0);
        if (atomic_exchange(&g_cube_side_req, 0) && (st & DM_PS_BANK) && !(st & DM_PS_CUBE)) { fe_send("PanelManager", "OpenPanel", "HoradricCubeLayout"); LOG("魔盒并排：储藏箱开着 → 魔盒面板并排打开"); } }
    { int beside = atomic_load(&g_cube_side_on);
      uint64_t cube = st & DM_PS_CUBE ? ui_panel(root, "HoradricCubeLayout") : 0, right = root_panel_shown(root, "PlayerInventoryExpansionLayout") && !beside ? sort_place(ui_panel(root, "PlayerInventoryExpansionLayout"), 0, SORT_INV_DX, SORT_INV_DY) : 0, left = 0;
      if (beside) right = sort_place(cube, 3, SORT_CUBE_DX, SORT_CUBE_DY);

      if (st & DM_PS_BANK) left = sort_place(ui_panel(root, "BankExpansionLayout"), !(st & DM_PS_VAULT) && pg_personal(root) ? 4 : 5, SORT_BANK_DX, SORT_BANK_DY);
      else if (cube) left = sort_place(cube, 3, SORT_CUBE_DX, SORT_CUBE_DY);
      atomic_store(&g_sort_btn[0], right); atomic_store(&g_sort_btn[1], left); }
    if (atomic_exchange(&g_pstate, st) != st) { LOG("界面状态：面板状态位 → %#x（1 储藏箱 / 2 商人页 / 4 死亡弹窗）", st); uimsg_emit("DM", "PanelState", ""); }








    { static const char *fz[] = {"PlayerInventoryExpansionLayout", "CharacterStatsPanel", "AdvancedStatsPanel", "SkillsTreePanel", "QuestLogPanelExpansion", "HirelingInventoryPanel", "VendorPanelLayout", "HireMenuPanel", "BankExpansionLayout", "BankOriginalLayout", "ImbueItemsPanel", "HoradricCubeLayout", "ChroniclePanel"};
      int want = 0, which = -1; for (unsigned i = 0; i < sizeof fz / sizeof fz[0] && !want; i++) { uint64_t w = ui_panel(root, fz[i]); if (w && *(uint8_t *)(w + 0x50) && has_visible_leaf(w, 1)) { want = 1; which = (int)i; } }
      if (want && ((st & DM_PS_DEAD) || root_panel_shown(root, "LoadScreenPanel"))) want = 0;
      extern _Atomic int g_dm_frz_want; if (atomic_exchange(&g_dm_frz_want, want) != want && getenv("DM_PANEL_FREEZE")) LOG("界面状态：算冻结的面板 %s%s", want ? "在屏上：" : "都不在屏上", want ? fz[which] : "");
      void dm_freeze_frame(int want); dm_freeze_frame(want); }
}



































static int ui_touch_mode(void);






static void tip_note(const uint64_t *regs) {
    module_t *m = main_module(); if (!m) return; int H = *(int32_t *)(m->base + 0x211da3c); const char *txt = (const char *)(uintptr_t)regs[1]; if (H <= 0 || !txt) return;
    int n = 1; for (const char *c = txt; *c && n < 64; c++) n += *c == '\n';
    int tx = (int32_t)regs[2], ty = (int32_t)(regs[2] >> 32), tw = (int32_t)regs[3], th = (int32_t)(regs[3] >> 32), bh = (int)lroundf((0.038f * n + 0.03f) * H), hw = H / 4, cx = tx + tw / 2;
    int below = ty + th + bh <= H, y0 = below ? ty + th : ty - bh;
    g_tip_box[0] = cx - hw; g_tip_box[1] = y0; g_tip_box[2] = cx + hw; g_tip_box[3] = y0 + bh; atomic_store(&g_tip_fr, dm_guest_frame());
}
static void tip_dock(const uint64_t *regs);



void dm_ui_tooltip_obs(const uint64_t *regs) { uint64_t *r = (uint64_t *)(uintptr_t)regs; tip_dock(regs); { void dm_itemcmp_spike_obs(uint64_t *), dm_tipx_obs(uint64_t *); dm_itemcmp_spike_obs(r); dm_tipx_obs(r); } if (dm_qa_tooltip_hook && regs[1]) { dm_cursor_item_info h; dm_qa_tooltip_hook((const char *)(uintptr_t)regs[1], dm_cursor_item(&h) ? h.id : 0); }
    tip_note(regs); }
static void tip_dock(const uint64_t *regs) {
    static int force = -2; if (force == -2) { const char *e = getenv("DM_TEST_TIPDOCK"); force = e ? atoi(e) : -1; }
    if (force == 0 || (force < 0 && !ui_touch_mode())) return;
    module_t *m = main_module(); uint64_t root = 0; if (!m) return; memcpy(&root, m->base + 0x2f05458, 8); if (!root) return;
    if (atomic_load(&g_pstate) & (DM_PS_BANK | DM_PS_VENDOR | DM_PS_IMBUE | DM_PS_CUBE | DM_PS_VAULT | DM_PS_DEAD)) return;
    if (!ui_panel(root, "PlayerInventoryExpansionLayout") || ui_panel(root, "HirelingInventoryPanel")) return;
    dm_cursor_item_info ci; if (!dm_cursor_item(&ci)) return;
    int W = *(int32_t *)(m->base + 0x211da38), H = *(int32_t *)(m->base + 0x211da3c); if (W <= 0 || H <= 0) return;
    int tx = (int32_t)regs[2], tw = (int32_t)regs[3], off = (int)lroundf(0.617f * H); if (W / 2 - off < H / 10) return;
    int left = tx + tw / 2 >= W / 2, cx = left ? W / 2 - off : W / 2 + off, y = (int)(0.117f * H);
    uint64_t *r = (uint64_t *)(uintptr_t)regs;
    r[2] = (uint64_t)(uint32_t)cx | (uint64_t)(uint32_t)y << 32; r[3] = 0;
    static int lg; static int ll = -1; if (ll != left && lg < 40) { ll = left; lg++; LOG("说明框停靠：物品 %s 在面板%s半 → 说明框停到%s侧空板（中线 x %d，顶 y %d；画面 %dx%d）", ci.code, left ? "右" : "左", left ? "左" : "右", cx, y, W, H); }
}
static int ui_touch_mode(void) { int dm_padmode_current(void), dm_padmode_busy(void); return !dm_padmode_current() && !dm_padmode_busy(); }




#if TARGET_OS_OSX
#define UI_MOUSE_ITEM_PUMPS(pm, proot, pw, phh) do { int dm_touch_host_is_live(void), dm_padmode_busy(void); if (!tm && dm_touch_host_is_live() && !dm_padmode_busy()) { ui_long_pump(); ui_use_pump(); ui_buy_pump(pw, phh); ui_drag_pump(pm, proot, pw, phh); } } while (0)
#else
#define UI_MOUSE_ITEM_PUMPS(pm, proot, pw, phh) do { } while (0)
#endif
void dm_ui_mouse_pump(void) {
    static uint64_t last_frame = UINT64_MAX; uint64_t fr = dm_guest_frame();
    if (fr == last_frame) return;
    (void)dm_ui_gen();
    ui_watch_pump();
    { module_t *pm0 = main_module(); uint64_t pr = 0; if (pm0) memcpy(&pr, pm0->base + 0x2f05458, 8); if (pr) { ui_panel_heal(pr, fr); ui_panel_state_pump(pr); } }




    { static int st; static uint64_t at; uint64_t sb = atomic_load(&g_set_back_at); module_t *sm = main_module(); uint64_t hroot = 0; if (sm) memcpy(&hroot, sm->base + 0x2f05458, 8);
      if (sb && fr >= sb) { atomic_store(&g_set_back_at, 0); ui_auto_key(DM_BTN_B); }
      { uint64_t qb = atomic_load(&g_speech_back_at); if (qb && fr >= qb) { atomic_store(&g_speech_back_at, 0); uint64_t qp = hroot ? ui_find(hroot, "QuestLogPanelExpansion", 1) : 0; if (qp && *(uint8_t *)(qp + 0x50)) ui_auto_key(DM_BTN_B); } }
      if ((atomic_load(&g_tab_left) > 0 || atomic_load(&g_tab_direct)) && atomic_load(&g_tab_top) == 1) { uint64_t sp = hroot ? ui_find(hroot, "SettingsPanel", 1) : 0; int open = sp && *(uint8_t *)(sp + 0x50);
          if (st == 0 && open) { ui_auto_key(DM_BTN_B); st = 1; at = fr + 36; LOG("界面页签：设置页开着，先按返回键退出"); return; }
          if (st == 1) { if (open && fr < at) return; st = 0; if (open) { atomic_store(&g_tab_left, 0); atomic_store(&g_tab_direct, 0); LOG("界面页签：设置页没关（在等确认框），这次不切页签"); return; } } }
      else if (!atomic_load(&g_tab_left) && !atomic_load(&g_tab_direct)) st = 0;
      if (atomic_exchange(&g_tab_direct, 0) && sm && hroot) {
          uint64_t gw = dm_wref_get(&g_tab_ref), tw = gw && ui_has(hroot, gw, 0) ? gw : 0, us = ui_find(hroot, "UISwitcher", 0); int d = atomic_load(&g_tab_delta);
          if (!strcmp(g_tab_name, "UISwitcherTabs") && us && *(uint8_t *)(us + 0x50) && *(uint64_t *)(*(uint64_t *)us + 0x28) == (uint64_t)sm->base + 0x404fd8) {




              int cur = *(int32_t *)(us + 0x1b8), n = *(int32_t *)(us + 0x188), to = g_tab_k; if (to < 0) to = 0; if (n > 0 && to >= n) to = n - 1;
              ((void (*)(uint64_t, int, int))(sm->base + 0x405898))(us, to, d > 0); LOG("界面页签：功能页第 %d → 第 %d 页（直接切，现为 %d）", cur, to, *(int32_t *)(us + 0x1b8)); }
          else if (tw && *(uint64_t *)tw == (uint64_t)sm->base + 0x20ba3a8 && *(uint8_t *)(tw + 0x50)) { int was = TAB_SEL(tw);
              ((void (*)(uint64_t, int, int))(sm->base + 0xad9b80))(tw, g_tab_k, 1); LOG("界面页签：「%s」第 %d → 第 %d 个（直接选中，现为 %d）", g_tab_name, was, g_tab_k, TAB_SEL(tw)); }
          else LOG("【红】界面页签：页签条「%s」已不在 / 不可见，这次不切", g_tab_name);
          dm_wref_set(&g_tab_ref, 0); atomic_store(&g_tab_have, 0); } }
    { static uint64_t tnext; if (fr >= tnext) { if (atomic_load(&g_tab_left) > 0) { atomic_fetch_sub(&g_tab_left, 1); ui_auto_key(atomic_load(&g_tab_mask)); tnext = fr + 8; }
        else { uint64_t th = atomic_exchange(&g_tab_then, 0);


            if (th == DM_BTN_A && atomic_load(&g_tab_top) == 3) { static int fix; int want = atomic_load(&g_menu_want), sel = -1; module_t *mm = main_module(); uint64_t rt = 0; if (mm) memcpy(&rt, mm->base + 0x2f05458, 8);
                uint64_t pn = rt ? ui_find(rt, "NpcDialogPanel", 0) : 0, ae = pn ? ui_find(pn, "ActiveElements", 0) : 0, ct = ae ? ui_find(ae, "Container", 0) : 0;
                if (want >= 0 && ct && npc_menu_sel(pn, ct, &sel) && sel != want) {
                    if (fix < 2) { fix++; int d = want - sel; atomic_store(&g_tab_then, DM_BTN_A); atomic_store(&g_tab_mask, d > 0 ? DM_BTN_DOWN : DM_BTN_UP); atomic_store(&g_tab_left, d > 0 ? d : -d); LOG("界面对话菜单：按确认前选中行是 %d、要的是 %d，补挪 %+d 行", sel, want, d); th = 0; }
                    else { LOG("【红】界面对话菜单：补挪两轮后选中行仍是 %d（要 %d），这一下不按确认", sel, want); th = 0; fix = 0; } }
                else fix = 0; }
            if (th) { ui_auto_key(th); tnext = fr + 8; } } } }
    { module_t *pm = main_module(); uint64_t proot = 0; if (pm) { memcpy(&proot, pm->base + 0x2f05458, 8); int32_t pw = *(int32_t *)(pm->base + 0x211da38), phh = *(int32_t *)(pm->base + 0x211da3c);
        static uint64_t pf = UINT64_MAX; if (proot && pw > 0 && phh > 0 && pf != fr) { pf = fr; { static int dd; const char *dn; if (!dd && (dn = getenv("DM_UI_DUMP")) && atomic_load(&g_taps_base) && fr >= atomic_load(&g_taps_base) + (getenv("DM_UI_DUMP_AT") ? atoi(getenv("DM_UI_DUMP_AT")) : 45)) { dd = 1; ui_dump(proot, 0, dn, 0, pm); } } int tm = ui_touch_mode(); if (tm) ui_wtap_pump(pm, proot, pw, phh); if (tm) ui_long_pump(); ui_target_pump(pm, fr); if (tm) ui_use_pump(); if (tm) ui_buy_pump(pw, phh); ui_item_pump(proot, pw, phh); ui_ptr_pump(pm, proot, pw, phh); if (tm) ui_drag_pump(pm, proot, pw, phh); UI_MOUSE_ITEM_PUMPS(pm, proot, pw, phh); { int lu = atomic_load(&g_load_up); dm_uiscreen_set_ingame(ui_panel(proot, "HUDPanel") && !(lu < 0 ? ui_find(proot, "LoadScreenPanel", 0) != 0 : lu)); } ui_safe_pump(pm, proot, fr); ui_lift_pump(proot, fr); ui_note_dock_pump(proot); ui_trove_tabs_pump(pm, proot, fr); ui_minimap_pump(pm, proot, pw, phh, fr); ui_map_pump(pm, proot); ui_map_geom_pump(pm, proot); ui_boss_pct_pump(pm, proot); ui_npcdlg_pump(proot); ui_map_center(pm, proot, *(volatile int32_t *)(pm->base + 0x2d5c748)); if (tm) ui_bind_pump(proot, fr); if (tm) swaptab_pump(proot); { void dm_build_ui_pump(uint64_t); dm_build_ui_pump(proot); } if (tm) ui_mercpg_pump(pm, proot, fr); ui_widestash_pump(proot); mv_audit_pump(proot, fr); ui_pages_pump(pm, proot); } } }
    if (atomic_load(&g_fe_name_open) && atomic_load(&g_ut_phase) == 2) {
        module_t *nm = main_module(); uint64_t nroot = 0; if (nm) memcpy(&nroot, nm->base + 0x2f05458, 8); uint64_t pn = nroot ? ui_find(nroot, "CharacterCreateNamePanel", 0) : 0;
        if (pn && nm) { int32_t w = *(int32_t *)(nm->base + 0x211da38), h = *(int32_t *)(nm->base + 0x211da3c); uint64_t v = atomic_load(&g_ut_xy);
            uint64_t xy = (uint64_t)(uint32_t)(int)(((v >> 16) & 0xffff) / 65535.0 * w) | ((uint64_t)(uint32_t)(int)((v & 0xffff) / 65535.0 * h) << 32);
            static const char *box[] = { "InputBackplate", "InputBackground", "InputText" };
            for (int i = 0; i < 3; i++) { uint64_t b = ui_find(pn, box[i], 0); if (b && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)b + 0x48))(b, xy) & 1)) { atomic_store(&g_ut_phase, 0); atomic_store(&g_fe_name_tap, 1); break; } } } }
    if (atomic_load(&g_del_q) && !atomic_load(&g_del_hold)) { module_t *dm = main_module(); uint64_t droot = 0; if (dm) memcpy(&droot, dm->base + 0x2f05458, 8);
        uint64_t modal = droot ? ui_find(droot, "DeleteCharacterModal", 0) : 0, yes = modal ? ui_find(modal, "Confirm", 0) : 0;
        if (yes && dm) { int32_t w = *(int32_t *)(dm->base + 0x211da38), h = *(int32_t *)(dm->base + 0x211da3c); uint64_t v = atomic_load(&g_ut_xy);
            uint64_t xy = (uint64_t)(uint32_t)(int)(((v >> 16) & 0xffff) / 65535.0 * w) | ((uint64_t)(uint32_t)(int)((v & 0xffff) / 65535.0 * h) << 32);
            uint64_t fn = *(uint64_t *)(*(uint64_t *)yes + 0x48);
            if (((int (*)(uint64_t, uint64_t))fn)(yes, xy) & 1) { atomic_store(&g_del_hold, 1); atomic_store(&g_ut_phase, 0); LOG("前端路径：删除角色弹窗按住「是」→ 按住 A 到松手"); }
            else atomic_store(&g_del_q, 0); }
        else atomic_store(&g_del_q, 0); }


    { uint64_t qs = atomic_load(&g_qt_stop_at);
      if (qs && fr >= qs && !atomic_load(&g_ut_phase)) { atomic_store(&g_qt_stop_at, 0);
          if (atomic_load(&g_qt_speech)) { atomic_store(&g_ut_xy, atomic_load(&g_qt_xy)); atomic_store(&g_ut_phase, 2); } } }


    { module_t *hm = main_module(); uint64_t hr = 0; if (hm) memcpy(&hr, hm->base + 0x2f05458, 8); ui_portrait_pump(hm, hr, fr); ui_level_word_pump(hm, fr); }
    int ph = atomic_load(&g_ut_phase); if (!ph) return;
    last_frame = fr;
    module_t *m = main_module(); if (!m) return;
    int32_t width = *(int32_t *)(m->base + 0x211da38), height = *(int32_t *)(m->base + 0x211da3c);
    uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8);
    if (width <= 0 || height <= 0 || !root) return;
    uint64_t r = atomic_load(&g_ut_xy);
    int x = (int)(((r >> 16) & 0xffff) / 65535.0 * width), y = (int)((r & 0xffff) / 65535.0 * height); if (x >= width) x = width - 1; if (y >= height) y = height - 1;
    if (atomic_load(&g_fe_opt_open)) { uint64_t bw = ui_find(root, "TouchBliss", 0);
        if (bw) { int on = *(int32_t *)(bw + 0x6e8) == 4; if (atomic_exchange(&g_fe_bliss, on) != on) { LOG("前端路径：极乐世界 %s", on ? "勾上" : "取消"); dm_elysium_arm(on); }
            int32_t vw = *(int32_t *)(m->base + 0x211da38), vh = *(int32_t *)(m->base + 0x211da3c);
            if (ph && vh > 0 && !atomic_load(&g_fe_confirm_open) && y > BLISS_Y0 * vh / 2160 && y < BLISS_Y1 * vh / 2160) { x = vw / 2 + (-1551 + 35) * vh / 2160; y = (1330 + 33) * vh / 2160; } } }
    static uint64_t h_mv, h_dn, h_up; if (!h_mv) { h_mv = ui_hash("MouseMove"); h_dn = ui_hash("LeftMouseButtonDown"); h_up = ui_hash("LeftMouseButtonUp"); }
    static int logged;
    { int dm_vault_ui_blocks(int, int); if (!((dm_ui_panel_state() & DM_PS_VAULT) && dm_vault_ui_blocks(x, y))) ui_send_mouse(m, root, h_mv, x, y); }




    static uint64_t snapped;
    if (ph == 2 && r != snapped && (dm_ui_panel_state() & DM_PS_VAULT) && ({ int dm_vault_ui_blocks(int, int); dm_vault_ui_blocks(x, y); })) snapped = r;
    if (ph == 2 && r != snapped) { int sx = x, sy = y, sn = ui_snap(m, root, &sx, &sy);
        if (sn == 6) { atomic_store(&g_ut_phase, 0); static int l6; if (l6++ < 40) LOG("界面鼠标：(%d,%d) 压着被上层面板盖住的按钮，不点", x, y); return; }
        if (sn == 2) { static int lg; if (lg++ < 40) LOG("界面鼠标：(%d,%d) 就近吸附到 (%d,%d)", x, y, sx, sy);
            r = ((uint64_t)(uint32_t)((sx + 0.5) / width * 65535) << 16) | (uint32_t)((sy + 0.5) / height * 65535); atomic_store(&g_ut_xy, r); x = sx; y = sy; }
        snapped = r; }
    static int hov; static uint64_t hov_xy; if (r != hov_xy) { hov_xy = r; hov = 0; }
    if (ph == 1) hov++;
    if (ph == 2 && hov < 2) { hov++; return; }
    if (ph == 2 && atomic_load(&g_cc_open) && !atomic_load(&g_fe_opt_open) && !atomic_load(&g_fe_name_open) && !atomic_load(&g_fe_confirm_open)) {
        uint64_t pn = ui_find(root, "CharacterCreatePanel", 0), xy = (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32), ws[8]; int hit = -1;
        for (int i = 0; pn && i < 8; i++) { ws[i] = ui_find(pn, k_cc_class[i], 0); if (ws[i] && hit < 0 && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)ws[i] + 0x48))(ws[i], xy) & 1)) hit = i; }
        for (int i = 0; pn && hit >= 0 && i < 8; i++) if (i != hit && ws[i] && *(int32_t *)(ws[i] + 0x13f4) == 1) {
            ((void (*)(uint64_t, int))(m->base + 0xc2cf24))(ws[i], 3); static int lg; if (lg++ < 12) LOG("前端路径：职业选择页改选 %s → 让出列中的 %s 归队", k_cc_class[hit], k_cc_class[i]); }
        if (pn && hit >= 0 && ws[hit] && *(int32_t *)(ws[hit] + 0x13f4) == 3) ((void (*)(uint64_t, int))(m->base + 0xc2cf24))(ws[hit], 0);
    }


    if (ph == 2) { int qt = quest_log_tap(m, root, x, y); atomic_store(&g_qt_arm, qt == 1);
        if (qt == 1) { atomic_store(&g_qt_sel, 0); atomic_store(&g_qt_fin, 0); atomic_store(&g_qt_dn, r); }
        else if (qt == 2) { uint64_t q = atomic_load(&g_qt_xy);
            if (atomic_load(&g_qt_speech) && q && q != r) { atomic_store(&g_ut_xy, q); LOG("任务日志：对白播着时点了空处 (%d,%d) → 改点正在播的任务（停对白、保留目标）", x, y); return; }
            int e = 2; atomic_compare_exchange_strong(&g_ut_phase, &e, 0); LOG("任务日志：点了空处 (%d,%d) → 不发给游戏（原版会取消选中、目标清空）", x, y); return; } }
    if (ph == 2 && ui_bind_modal_tap(root, x, y)) { int e = 2; atomic_compare_exchange_strong(&g_ut_phase, &e, 0); return; }
    if (ph == 2) { int dm_looks_ui_tap(int, int); if (dm_looks_ui_tap(x, y)) { int e = 2; atomic_compare_exchange_strong(&g_ut_phase, &e, 0); return; } }
    if (ph == 2 && (dm_ui_panel_state() & DM_PS_VAULT)) { int dm_vault_ui_tap(int, int); if (dm_vault_ui_tap(x, y)) { int e = 2; atomic_compare_exchange_strong(&g_ut_phase, &e, 0); return; } }
    if (ph == 2) {
        int px = x, py = y; static uint64_t st0; dm_hud_info hh; dm_hud_get(&hh);
        if (hh.valid && hh.holding && ui_snap(m, root, &px, &py) && !strcmp(g_click_name, "convert")) {
            if (!st0) { st0 = now_ns(); dm_held_stow(); LOG("合成钮：手上还拿着物品（id %d），原版会丢掉这次合成 → 先收进背包再按", hh.holding); return; }
            int res = dm_held_stow_result();
            if (res != 0 && now_ns() - st0 < 1600000000ull) return;






            LOG("合成钮：手上物品收不进背包（id %d，结果 %d）→ 不按合成（按了原版会静默丢掉），交收尾链：按返回关面板、由游戏放背包或丢在脚下", hh.holding, res);
            g_rt = (drop_retry){ .active = 1, .id = hh.holding, .rows = 1, .stage = 2, .t = now_ns() - 400000000ull }; atomic_store(&g_drop_loc, 0);
            st0 = 0; int e = 2; atomic_compare_exchange_strong(&g_ut_phase, &e, 0); return; }
        else if (st0) LOG("合成钮：手上已空，按下");
        else if (!(hh.valid && hh.holding) && ui_snap(m, root, &px, &py) && !strcmp(g_click_name, "convert")) LOG("合成钮：按下（HUD 读数 %s，手上 %d）——之后应有一行「合成读数：服务端收到合成请求」，没有就是请求没进游戏服务端", hh.valid ? "有效" : "无效", hh.holding);
        st0 = 0; }
    if (ph == 2) { ui_send_mouse(m, root, h_dn, x, y); int e = 2; atomic_compare_exchange_strong(&g_ut_phase, &e, 3); }
    else if (ph == 3) { ui_send_mouse(m, root, h_up, x, y); int e = 3; atomic_compare_exchange_strong(&g_ut_phase, &e, 0);
        DM_LAT("click %d,%d", x, y);
        if (logged < 400) { logged++; LOG("界面鼠标：点击 (%d,%d)", x, y); }




        { uint64_t sp = ui_find(root, "SettingsPanel", 1); char nm[40]; int px = x, py = y;
          if (sp && *(uint8_t *)(sp + 0x50) && ui_snap(m, root, &px, &py) == 1 && (strlcpy(nm, g_click_name, sizeof nm), !strcmp(nm, "AcceptButton") || !strcmp(nm, "CancelButton") || !strcmp(nm, "TouchClose"))) {
              atomic_store(&g_set_back_at, dm_guest_frame() + 12); LOG("界面鼠标：设置页点了 %s → 随后按返回键退出设置页", nm);


              dm_custom_on_uimsg("OptionsPanelMessage", nm[0] == 'A' ? "AcceptChanges" : "CancelChanges", ""); } }




        { uint64_t se = atomic_load(&g_speech_end), f = dm_guest_frame();
          if (se && f - se < 30 && atomic_load(&g_close_down) && f - atomic_load(&g_close_down) < 60) {                                                                                                                                                                                                                                                                                                                               atomic_store(&g_speech_back_at, f + 12); LOG("界面鼠标：任务语音播着时点了 X → 随后按返回键关面板"); } } }
}















#include <dirent.h>
#include <sys/stat.h>
static _Atomic int g_fe_ribbon, g_fe_list, g_fe_name_key; static _Atomic int g_fe_skip_tut, g_fe_menu, g_fe_mode_modal, g_fe_options; static _Atomic int g_fe_name, g_fe_name_done; static _Atomic uint64_t g_fe_key_until, g_fe_key_mask;
static int has_character(const char *dir, int depth) {
    DIR *d = opendir(dir); if (!d) return 0; struct dirent *e; int found = 0;
    while (!found && (e = readdir(d))) { if (e->d_name[0] == '.') continue; char p[1024]; snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
        size_t l = strlen(e->d_name); if (l > 4 && !strcmp(e->d_name + l - 4, ".d2s")) found = 1;
        else if (depth < 3) { struct stat st; if (!stat(p, &st) && S_ISDIR(st.st_mode)) found = has_character(p, depth + 1); } }
    closedir(d); return found;
}
static _Atomic uint64_t g_taps_base;
uint64_t dm_ui_taps_base(void) { return atomic_load(&g_taps_base); }
static void rs_on_uimsg(const char *cat, const char *act, const char *arg);








static char g_sel_name[17]; static uint64_t g_sel_fr;
static const char *sel_cur(uint8_t *b, const char *want, int64_t *at) {
    int mode = *(int32_t *)(b + 0x2d7badc) != 1; uint8_t *lv = b + (mode ? 0x2d710f0 : 0x2d71108), *slot = b + (mode ? 0x2d7b950 : 0x2d7b960);
    uint64_t arr = *(uint64_t *)lv, n = *(uint64_t *)(lv + 8), cur = *(uint64_t *)slot; const char *nm = NULL; if (at) *at = -1;
    for (uint64_t i = 0; arr && i < n && i < 256; i++) { uint64_t e = *(uint64_t *)(arr + 8 * i); if (!e) continue;
        if (i == cur && slot[8]) nm = (const char *)(e + 0x10); if (want && at && *at < 0 && !strncmp((const char *)(e + 0x10), want, 16)) *at = (int64_t)i; }
    return nm;
}
static void sel_keep(const char *cat, const char *act) {
    module_t *m = main_module(); if (!m) return; uint8_t *b = (uint8_t *)m->base; uint64_t fr = dm_guest_frame();
    if (!strcmp(cat, "CharacterSelect") && !strcmp(act, "SelectCharacter")) { const char *nm = sel_cur(b, NULL, NULL); g_sel_name[0] = 0; if (nm) { strlcpy(g_sel_name, nm, sizeof g_sel_name); g_sel_fr = fr; } return; }
    if (!g_sel_name[0] || strcmp(act, "UpdateCharacterInfo")) return;
    if (fr - g_sel_fr > 120) { g_sel_name[0] = 0; return; }
    int64_t at; const char *nm = sel_cur(b, g_sel_name, &at); char want[17]; strlcpy(want, g_sel_name, sizeof want); g_sel_name[0] = 0;
    if (nm && !strncmp(nm, want, 16)) return;
    if (at < 0) { LOG("【红】选角：确认的角色「%s」不在重排后的列表里，当前是「%s」，不动", want, nm ? nm : ""); return; }
    ((void (*)(int))(b + 0x7fe32c))((int)at); fe_send("MainMenuHD", "UpdateCharacterInfo", "");
    LOG("选角：确认的是「%s」，原版重排后当前成了「%s」（被选者没排到第一行）→ 已按名字设回第 %lld 行", want, nm ? nm : "", (long long)at);
}
void dm_ui_on_uimsg(const char *cat, const char *act, const char *arg) {
    sel_keep(cat, act);


    if (!strcmp(cat, "PanelManager") && !strcmp(act, "ClosePanel") && arg && strncmp(arg, "Touch", 5)) mv_audit_arm("关面板");
    else if (!strcmp(act, "SelectTab") || !strcmp(act, "ActivateTab")) mv_audit_arm("切页签");
    else if (!strcmp(cat, "FrontEndNavigation") && !strcmp(act, "ToFrontEnd")) mv_audit_arm("退出到前端");
    if (!strcmp(cat, "PanelManager") && arg && !strcmp(arg, "LoadScreenPanel") && (!strcmp(act, "OpenPanel") || !strcmp(act, "UnloadPanel"))) { int up = act[0] == 'O'; atomic_store(&g_load_up, up);
        module_t *pm = main_module(); uint64_t root = 0; if (pm) memcpy(&root, pm->base + 0x2f05458, 8);
        if (root) { dm_uiscreen_set_ingame(!up && ui_find(root, "HUDPanel", 0)); g_lift_next = 0; ui_lift_pump(root, 0); } }
    if (!strcmp(cat, "PausePanelMessage") && !strcmp(act, "ExitGame")) { void dm_resume_exit_game(void); dm_resume_exit_game(); }
    { int ex = (!strcmp(cat, "PausePanelMessage") && !strcmp(act, "ExitGame")) || (!strcmp(cat, "FrontEndNavigation") && !strcmp(act, "ToFrontEnd")),
          ld = !strcmp(cat, "PanelManager") && arg && !strcmp(arg, "LoadScreenPanel") && !strcmp(act, "OpenPanel");
      if (ex || ld) { module_t *pm = main_module(); uint64_t root = 0; if (pm) memcpy(&root, pm->base + 0x2f05458, 8); if (ex) g_nd_hold = 1;
          note_dock_restore(root, ex ? (act[0] == 'E' ? "保存并退出" : "回到前端") : "读取画面（切场景）"); } }
    if (!strcmp(cat, "ProgressionMessage") && !strcmp(act, "PlayVideo")) atomic_store(&g_map_movie, 1);
    else if ((!strcmp(cat, "Cinematics") && !strcmp(act, "MovieComplete")) || (!strcmp(cat, "FrontEndNavigation") && !strcmp(act, "ToFrontEnd"))) atomic_store(&g_map_movie, 0);
    if (!strcmp(cat, "QuestLogPanelMessage") && !strcmp(act, "DialogueFinished")) atomic_store(&g_speech_end, dm_guest_frame());


    if (!strcmp(cat, "QuestLogPanelMessage")) {
        if (!strcmp(act, "SelectQuest") && atomic_load(&g_qt_arm)) atomic_store(&g_qt_sel, 1);
        else if (!strcmp(act, "DialogueFinished") && atomic_load(&g_qt_arm)) atomic_store(&g_qt_fin, 1);
        else if (!strcmp(act, "PlayDialogue") && atomic_exchange(&g_qt_arm, 0)) { atomic_store(&g_qt_xy, atomic_load(&g_qt_dn));
            if (atomic_load(&g_qt_sel) && !atomic_load(&g_qt_fin)) { atomic_store(&g_qt_stop_at, dm_guest_frame() + 2); LOG("任务日志：点了新任务、对白开播 → 替玩家再点一下停对白，直接显示当前目标"); } } }
    if (!strcmp(cat, "PanelManager") && arg && !strcmp(arg, "QuestLogSpeechPanel")) {
        if (!strcmp(act, "OpenPanel")) atomic_store(&g_qt_speech, 1);
        else if (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) atomic_store(&g_qt_speech, 0); }
    if (!strcmp(cat, "PanelManager") && arg && !strcmp(arg, "QuestLogPanelExpansion") && (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel"))) { atomic_store(&g_qt_xy, 0); atomic_store(&g_qt_stop_at, 0); atomic_store(&g_qt_arm, 0); }
    if (!strcmp(cat, "PanelManager")) ui_panel_note(act, arg ? arg : "");
    if (!strcmp(cat, "ChroniclePanelMessage") && !strcmp(act, "ToggleChildPanel") && arg && arg[0]) { int c = 0; while (c < 8 && g_chr_child[c][0] && strcmp(g_chr_child[c], arg)) c++; if (c < 8 && !g_chr_child[c][0]) strlcpy(g_chr_child[c], arg, sizeof g_chr_child[c]); }
    if (!strcmp(cat, "PanelManager") && arg && !strcmp(arg, "ChroniclePanel") && (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel"))) atomic_store(&g_chr_closed, 1);
    dm_custom_on_uimsg(cat, act, arg ? arg : "");
    if (act && !strcmp(act, "ControllerActionEnd")) atomic_store(&g_a_wait, 0);
    if (strcmp(cat, "InputMessage") && strcmp(cat, "MonsterHealth") && strcmp(cat, "PartyPanelMessage")) DM_LAT("ui %s %s %s", cat, act, arg ? arg : "");
    { static const char *at; static int got; if (!got) { got = 1; at = getenv("DM_UI_TAPS_AT"); }
      if (at && !atomic_load(&g_taps_base)) { char k[160]; snprintf(k, sizeof k, "%s:%s", act, arg); if (strstr(k, at)) atomic_store(&g_taps_base, dm_guest_frame() | 1); } }
    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "CharacterCreateNamePanel")) { int op = !strcmp(act, "OpenPanel"); if (op || !strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) { atomic_store(&g_fe_name_open, op); if (op) atomic_store(&g_fe_name_has, 0); if (op) atomic_store(&g_fe_name, 1); } }
    if (!strcmp(cat, "PanelManager") && (!strcmp(arg, "CharacterOptionsPanel") || !strcmp(arg, "ConfirmationModal"))) { int op = !strcmp(act, "OpenPanel"); if (op || !strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) atomic_store(arg[1] == 'h' ? &g_fe_opt_open : &g_fe_confirm_open, op); }
    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "DeleteCharacterModal")) { if (!strcmp(act, "OpenPanel")) atomic_store(&g_fe_del_open, 1); else if (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) { atomic_store(&g_fe_del_open, 0); atomic_store(&g_del_hold, 0); } }
    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "CharacterCreatePanel")) { if (!strcmp(act, "OpenExclusivePanel") || !strcmp(act, "OpenPanel")) atomic_store(&g_cc_open, 1); else if (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) atomic_store(&g_cc_open, 0); }
    if (!strcmp(cat, "PanelManager") && !strcmp(act, "UnloadAll")) atomic_store(&g_cc_open, 0);


    if (!strcmp(cat, "PanelManager") && !strcmp(act, "ClosePanel") && !strncmp(arg, "TouchKey", 8)) { if (!strcmp(arg + 8, "PLUS")) atomic_store(&g_fe_name_key, 1); else if (!strcmp(arg + 8, "B")) atomic_store(&g_fe_name_key, 2); }
    if (!strcmp(cat, "ConsoleCharacterCreate") && !strcmp(act, "ToggleCharacterOptions") && !strcmp(arg, "1")) atomic_store(&g_fe_options, 1);
    rs_on_uimsg(cat, act, arg);


    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "MainMenuButtonRibbon") && !strcmp(act, "OpenPanel")) atomic_store(&g_fe_ribbon, 1);
    if ((!strcmp(cat, "PanelManager") && (!strcmp(act, "UnloadAll") || (!strcmp(arg, "MainMenuPanel") && !strcmp(act, "UnloadPanel")))) || (!strcmp(cat, "FrontEndNavigation") && (!strcmp(act, "ToGame") || !strcmp(act, "ToCharacterCreate")))) atomic_store(&g_fe_ribbon, 0);
    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "CharacterSelectPanel") && !strcmp(act, "OpenPanel") && atomic_exchange(&g_fe_bliss, 0)) { LOG("前端路径：回到选角页，极乐世界勾选清掉"); dm_elysium_arm(0); }
    if (!strcmp(cat, "PanelManager") && !strcmp(arg, "CharacterSelectPanel")) { if (!strcmp(act, "OpenPanel")) atomic_store(&g_fe_list, 1); else if (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) atomic_store(&g_fe_list, 0); }
    if (strcmp(cat, "PanelManager") || strcmp(act, "OpenPanel")) return;
    if (!strcmp(arg, "TutorialTooltip")) atomic_store(&g_fe_skip_tut, 1);
    else if (!strcmp(arg, "MainMenuButtonRibbon")) atomic_store(&g_fe_menu, 1);
    else if (!strcmp(arg, "OnlinePlayModeModal")) atomic_store(&g_fe_mode_modal, 1);
}
static uint64_t ui_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 6) return 0; uint64_t nm = *(uint64_t *)(w + 8), ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60);
    if (nm && !strcmp((const char *)nm, name)) return w;
    if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = ui_find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static uint64_t fe_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000; }
static char g_fe_name_text[64]; static _Atomic uint64_t g_fe_name_ms;
_Atomic int g_fe_name_no_accept;
int dm_ui_fe_name_wants_input(void) { return atomic_load(&g_fe_name_open) && !atomic_load(&g_fe_name_has); }
void dm_ui_text_entered(const char *text) { if (text && *text && atomic_load(&g_fe_name_open)) atomic_store(&g_fe_name_has, 1); if (atomic_exchange(&g_fe_name_no_accept, 0)) return; if (text && *text && atomic_load(&g_fe_name_open)) { strlcpy(g_fe_name_text, text, sizeof g_fe_name_text); atomic_store(&g_fe_name_done, 1); } }


static _Atomic uint64_t g_pulse_mask, g_pulse_until;
void dm_ui_key_pulse(uint64_t mask, int frames) { atomic_store(&g_pulse_mask, mask); atomic_store(&g_pulse_until, dm_guest_frame() + (uint64_t)frames); }








static _Atomic uint64_t g_alone_mask, g_alone_until;
void dm_ui_key_pulse_alone(uint64_t mask, int frames) { atomic_store(&g_alone_until, (uint64_t)frames); atomic_store(&g_alone_mask, mask); }
uint64_t dm_ui_alone_take(uint64_t others) {
    static uint64_t cur, until; uint64_t fr = dm_guest_frame();
    if (cur) { if (fr < until) return cur; cur = 0; }
    uint64_t m = atomic_load(&g_alone_mask); if (!m || others || !dm_uiscreen_ingame()) return 0;
    { int dm_padmode_busy(void); if (dm_padmode_busy()) return 0; }
    for (int i = 0; i < 48; i++) if (g_pm[i].open) return 0;
    atomic_store(&g_alone_mask, 0); cur = m; until = fr + atomic_load(&g_alone_until);
    LOG("前端：进局常态，替玩家点一次「显示物品」键（掉落物名字常显；这 %d 帧只交这一个键 %#llx）", (int)atomic_load(&g_alone_until), (unsigned long long)m);
    return cur;
}
uint64_t dm_ui_auto_buttons(void) { return (dm_guest_frame() < atomic_load(&g_fe_key_until) ? atomic_load(&g_fe_key_mask) : 0) | (atomic_load(&g_del_hold) ? DM_BTN_A : 0)
                                           | (dm_guest_frame() < atomic_load(&g_pulse_until) ? atomic_load(&g_pulse_mask) : 0); }
static void fe_key(uint64_t mask) { atomic_store(&g_fe_key_mask, mask); atomic_store(&g_fe_key_until, dm_guest_frame() + 3); }
static void ui_auto_key(uint64_t mask) { fe_key(mask); }
static void fe_send(const char *cat, const char *act, const char *arg) {
    module_t *m = main_module(); uint64_t root = 0; if (!m) return; memcpy(&root, m->base + 0x2f05458, 8); if (!root) return;
    static char payload[64]; static uint64_t msg[8]; memset(payload, 0, sizeof payload); strlcpy(payload, arg, sizeof payload);
    memset(msg, 0, sizeof msg); msg[0] = ui_hash(cat); msg[1] = ui_hash(act); msg[2] = (uint64_t)payload; msg[3] = 1; msg[4] = 1;
    uint64_t vt = 0, fn = 0; memcpy(&vt, (void *)root, 8); memcpy(&fn, (void *)(vt + 0x28), 8);
    LOG("前端路径：发 %s:%s %s", cat, act, arg);
    ((void (*)(uint64_t, void *))fn)(root, msg);
}














enum { RS_IDLE, RS_EXIT, RS_FRONT, RS_LOAD, RS_SHOW };
static _Atomic int g_rs; static int g_rs_req, g_rs_diff, g_rs_verify, g_rs_resent; static char g_rs_name[64]; static uint64_t g_rs_t0, g_rs_f0, g_rs_sf;
int dm_restart_busy(void) { return atomic_load(&g_rs) != RS_IDLE; }
void (*g_dm_restart_cover_hook)(int on);
static void rs_set(int st) { if (st == RS_LOAD) g_rs_verify = 1; int was = atomic_exchange(&g_rs, st), cv = st == RS_IDLE ? 0 : st == RS_SHOW ? 2 : 1, cw = was == RS_IDLE ? 0 : was == RS_SHOW ? 2 : 1; if (cv != cw && g_dm_restart_cover_hook) g_dm_restart_cover_hook(cv); }
static void rs_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (!strcmp(cat, "PanelManager") && !strcmp(act, "ClosePanel") && !strcmp(arg, "TouchRestart")) g_rs_req = 1;
    int st = atomic_load(&g_rs); if (st == RS_IDLE) return;
    if (st == RS_EXIT && !strcmp(cat, "FrontEndNavigation") && !strcmp(act, "ToFrontEnd")) { rs_set(RS_FRONT); LOG("一键重开：已回到前端（%llu ms）", (unsigned long long)(fe_ms() - g_rs_t0)); }
    if ((st == RS_LOAD || st == RS_FRONT) && !strcmp(cat, "PanelManager") && !strcmp(arg, "LoadScreenPanel") && !strcmp(act, "OpenPanel")) { rs_set(RS_SHOW); LOG("一键重开：「%s」难度 %d 进局读取（从点按钮起 %llu ms）", g_rs_name, g_rs_diff, (unsigned long long)(fe_ms() - g_rs_t0)); }
    if (st == RS_SHOW && !strcmp(cat, "PanelManager") && !strcmp(arg, "LoadScreenPanel") && (!strcmp(act, "UnloadPanel") || !strcmp(act, "ClosePanel"))) { rs_set(RS_IDLE); LOG("一键重开：已进局（从点按钮起 %llu ms）", (unsigned long long)(fe_ms() - g_rs_t0)); }
}
static void rs_abort(const char *why) { rs_set(RS_IDLE); LOG("【红】一键重开：%s → 放弃，停在前端（从点按钮起 %llu ms）", why, (unsigned long long)(fe_ms() - g_rs_t0)); }
void dm_ui_send(const char *cat, const char *act, const char *arg) { fe_send(cat, act, arg); }
static void rs_load(uint64_t fr, const char *how) { char a[4]; snprintf(a, sizeof a, "%d", g_rs_diff); fe_send("CharacterSelect", "LoadCharacter", a); g_rs_sf = fr; rs_set(RS_LOAD); LOG("一键重开：%s → 原版载入「%s」，难度 %d（从点按钮起 %llu ms）", how, g_rs_name, g_rs_diff, (unsigned long long)(fe_ms() - g_rs_t0)); }




static _Atomic int g_rs_ext; static uint64_t g_rs_ext_f;
void dm_restart_request(void) { atomic_store(&g_rs_ext, 1); }
static void rs_pump(void) {
    uint64_t fr = dm_guest_frame();
    { int e = atomic_load(&g_rs_ext); if (e == 1 && atomic_load(&g_rs) == RS_IDLE) { atomic_store(&g_rs_ext, 2); g_rs_ext_f = fr; fe_send("PanelManager", "OpenPanel", getenv("DM_RS_PAUSE_PANEL") ? getenv("DM_RS_PAUSE_PANEL") : "PauseLayout"); LOG("一键重开：别的模块请求 → 先开选项页"); }
      else if (e == 2 && fr - g_rs_ext_f > 30) { atomic_store(&g_rs_ext, 0); g_rs_req = 1; } }
    if (g_rs_req) { g_rs_req = 0; if (atomic_load(&g_rs) != RS_IDLE) return;
        int dm_diag_difficulty(void); module_t *m = main_module(); uint8_t *b = m ? (uint8_t *)m->base : NULL; int d = dm_diag_difficulty();
        uint64_t pl = b ? ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)) : 0, pd = pl ? *(uint64_t *)(pl + 0x10) : 0;
        if (d < 0 || !pd || !*(const char *)pd) { LOG("【红】一键重开：取不到当前角色 / 难度（难度 %d），不动", d); return; }
        strlcpy(g_rs_name, (const char *)pd, sizeof g_rs_name); g_rs_diff = d; g_rs_t0 = fe_ms(); g_rs_f0 = fr; g_rs_resent = 0; rs_set(RS_EXIT);
        LOG("一键重开：「%s」难度 %d → 原版保存并退出", g_rs_name, d); fe_send("PausePanelMessage", "ExitGame", ""); return; }
    int st = atomic_load(&g_rs);
    if (st == RS_IDLE && g_rs_verify) {
        module_t *m = main_module(); uint8_t *b = m ? (uint8_t *)m->base : NULL; uint64_t pl = b ? ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)) : 0, pd = pl ? *(uint64_t *)(pl + 0x10) : 0;
        if (pd && *(const char *)pd) { int same = !strcmp((const char *)pd, g_rs_name); g_rs_verify = 0; LOG("%s一键重开：进局后核对，当前角色「%s」，刚退出的是「%s」→ %s", same ? "" : "【红】", (const char *)pd, g_rs_name, same ? "相符" : "不符"); }
        else if (fr - g_rs_f0 > 1800) { g_rs_verify = 0; LOG("【红】一键重开：1800 帧没读到局内角色，核对不了"); } }
    if (st == RS_IDLE) return;
    if (st == RS_SHOW) { if (fr - g_rs_f0 > 3600) { rs_set(RS_IDLE); LOG("【红】一键重开：读取画面 3600 帧没撤，放开触点"); } return; }
    if (fr - g_rs_f0 > 900) { rs_abort(st == RS_EXIT ? "900 帧没回到前端" : st == RS_FRONT ? "900 帧没发出载入" : "900 帧没出读取画面"); return; }
    if (st == RS_FRONT) { static uint64_t hf; const char *h = getenv("DM_TEST_RS_HOLD");
        if (h) { if (!hf) hf = fr; if (fr - hf < (uint64_t)atoi(h)) return; hf = 0; }










        { module_t *m = main_module(); uint8_t *b = (uint8_t *)m->base; int mode = *(int32_t *)(b + 0x2d7badc) != 1;
          uint8_t *lv = b + (mode ? 0x2d710f0 : 0x2d71108), *slot = b + (mode ? 0x2d7b950 : 0x2d7b960); uint64_t arr = *(uint64_t *)lv, n = *(uint64_t *)(lv + 8); int64_t idx = -1;
          for (uint64_t i = 0; arr && i < n && i < 256 && idx < 0; i++) { uint64_t e = *(uint64_t *)(arr + 8 * i); if (e && !strncmp((const char *)(e + 0x10), g_rs_name, 16)) idx = (int64_t)i; }
          if (idx < 0) { rs_abort("原版角色列表里没有刚退出的角色"); LOG("一键重开：列表 %llu 项，要的是「%s」", (unsigned long long)n, g_rs_name); return; }
          if (getenv("DM_TEST_RS_WRONGSEL") && n > 1) { *(uint64_t *)slot = (uint64_t)(idx + 1) % n; slot[8] = 1; LOG("DM_TEST_RS_WRONGSEL：先把选中改成第 %llu 项（模拟回前端后第一行不是刚退出的角色）", (unsigned long long)((idx + 1) % n)); }
          uint64_t was = *(uint64_t *)slot; if (was != (uint64_t)idx || !slot[8]) { *(uint64_t *)slot = (uint64_t)idx; slot[8] = 1; }
          uint64_t now = *(uint64_t *)slot, e = now < n ? *(uint64_t *)(arr + 8 * now) : 0;
          if (!slot[8] || now != (uint64_t)idx || !e || strncmp((const char *)(e + 0x10), g_rs_name, 16)) { rs_abort("选中刚退出的角色没成功"); LOG("一键重开：选中槽 %llu（有效 %d），要的是第 %lld 项「%s」", (unsigned long long)now, slot[8], (long long)idx, g_rs_name); return; }
          LOG("一键重开：原版列表第 %lld 项是「%s」，选中槽原为 %llu → %s", (long long)idx, g_rs_name, (unsigned long long)was, was == (uint64_t)idx ? "本来就是它" : "已改选它"); }
        rs_load(fr, "已回到前端"); return; }
    if (st == RS_LOAD && !g_rs_resent && fr - g_rs_sf > 90) { g_rs_resent = 1; rs_load(fr, "载入发出 90 帧没出读取画面，补发"); }
}


static char g_sendq[96]; static _Atomic int g_sendq_on;
void dm_ui_send_queue(const char *spec) { strlcpy(g_sendq, spec, sizeof g_sendq); atomic_store(&g_sendq_on, 1); }
void dm_ui_show_ours(uint64_t w, int on) { ui_show_ours(w, on); }
void dm_ui_send_now(const char *cat, const char *act, const char *arg) { fe_send(cat, act, arg); }
void dm_ui_frontend_pump(void) {
    if (atomic_exchange(&g_sendq_on, 0)) { char b[96], *c1, *c2; strlcpy(b, g_sendq, sizeof b); c1 = strchr(b, ':'); if (c1) { *c1++ = 0; c2 = strchr(c1, ':'); if (c2) *c2++ = 0; fe_send(b, c1, c2 ? c2 : ""); } }


    { static struct { uint64_t fr; char c[48], a[48], g[48]; int done; } ds[8]; static int nd = -1; if (nd < 0) { nd = 0; const char *e = getenv("DM_DBG_FE_SEND"); char buf[512]; if (e) { strlcpy(buf, e, sizeof buf);
          for (char *t = strtok(buf, ";"); t && nd < 8; t = strtok(NULL, ";")) { unsigned long long f; ds[nd].g[0] = 0; if (sscanf(t, "%llu:%47[^:]:%47[^:]:%47s", &f, ds[nd].c, ds[nd].a, ds[nd].g) >= 3) { ds[nd].fr = f; nd++; } } } }
      uint64_t b = atomic_load(&g_taps_base), f0 = dm_guest_frame(); if (nd > 0 && b) for (int i = 0; i < nd; i++) if (!ds[i].done && f0 >= b + ds[i].fr) { ds[i].done = 1; if (ds[i].c[0] == '!') { void uimsg_emit(const char *, const char *, const char *); LOG("前端路径：只给订阅者补发 %s:%s %s（不进游戏）", ds[i].c + 1, ds[i].a, ds[i].g); uimsg_emit(ds[i].c + 1, ds[i].a, ds[i].g); } else fe_send(ds[i].c, ds[i].a, ds[i].g); } }
    rs_pump(); if (atomic_load(&g_rs) != RS_IDLE) return;
    static uint64_t next; uint64_t fr = dm_guest_frame(); if (fr < next) return;
    if (atomic_load(&g_cc_open)) {
        module_t *m = main_module(); uint64_t root = 0; if (m) memcpy(&root, m->base + 0x2f05458, 8);
        uint64_t pn = root ? ui_find(root, "CharacterCreatePanel", 0) : 0, bk = pn ? ui_find(pn, "TouchBack", 0) : 0;
        if (bk) { const char *sv = getenv("DM_SAVE"); uint8_t want = !sv || has_character(sv, 0);
            ui_show_ours(bk, want); }
    }
    { int nk = atomic_exchange(&g_fe_name_key, 0); if (nk) { fe_key(nk == 1 ? DM_BTN_PLUS : DM_BTN_B); next = fr + 8; LOG("前端路径：起名页点了「%s」→ %s", nk == 1 ? "创建" : "返回", nk == 1 ? "＋" : "B"); return; } }
    if (atomic_exchange(&g_fe_name_tap, 0)) { fe_key(DM_BTN_A); next = fr + 8; LOG("前端路径：起名页点了输入框 → 重新弹键盘（A）"); return; }







    { int dm_padmode_current(void); uint64_t dm_padmode_page_keys(uint64_t); static int ck; module_t *mm = main_module(); uint64_t root = 0; if (mm) memcpy(&root, mm->base + 0x2f05458, 8);








      static int prep; static uint64_t prep_at; int padpage = dm_padmode_current() && root && atomic_load(&g_fe_opt_open);
      static int after_modal; if (padpage && atomic_load(&g_fe_confirm_open)) after_modal = 1; if (!padpage) after_modal = 0;
      if (!padpage || atomic_load(&g_fe_confirm_open)) prep = 0;
      else {
          if (prep == 0) { prep = 1; prep_at = fr + (after_modal ? 2 : 20); after_modal = 0; }
          else if (prep == 1 && fr >= prep_at) { fe_key(DM_BTN_DOWN); prep = 2; prep_at = fr + 8; LOG("前端路径：手柄模式页 → 焦点下移到「标准 / 专家」那一行（十字键下）"); }
          else if (prep == 2 && fr >= prep_at) prep = 3; }
      if (padpage) { uint64_t bw = ui_find(root, "TouchBliss", 0);
          if (bw) { int on = *(int32_t *)(bw + 0x6e8) == 4; if (atomic_exchange(&g_fe_bliss, on) != on) { LOG("前端路径：极乐世界 %s", on ? "勾上" : "取消"); dm_elysium_arm(on); } } }
      if (padpage && !atomic_load(&g_fe_confirm_open)) {






          { void dm_padmode_legend_hide(const char *, int); dm_padmode_legend_hide("CharacterOptionsLegend", 2); }
          if ((dm_padmode_page_keys(DM_BTN_Y | DM_BTN_X | (prep == 3 ? DM_BTN_UP | DM_BTN_DOWN : DM_BTN_LEFT | DM_BTN_RIGHT | DM_BTN_A)) & DM_BTN_Y) && !ck) { ck = 1; LOG("前端路径：手柄模式页按 Y → 翻「极乐世界」的勾"); }
          if (ck) { int32_t vw = *(int32_t *)(mm->base + 0x211da38), vh = *(int32_t *)(mm->base + 0x211da3c); int x = vw / 2 + (-1551 + 35) * vh / 2160, y = (1330 + 33) * vh / 2160;
              ui_send_mouse(mm, root, ui_hash("MouseMove"), x, y);
              if (ck == 2) ui_send_mouse(mm, root, ui_hash("LeftMouseButtonDown"), x, y); else if (ck == 3) ui_send_mouse(mm, root, ui_hash("LeftMouseButtonUp"), x, y);
              if (++ck > 3) ck = 0; return; } }
      else ck = 0; }


    static int fa = -1; if (fa < 0) { const char *e = getenv("DM_FE_AUTO"); fa = e ? atoi(e) != 0 : !getenv("DM_KEYS"); }
    if (!fa) return;








    { int dm_padmode_current(void);
      if (dm_padmode_current()) {
          atomic_store(&g_fe_skip_tut, 0); atomic_store(&g_fe_back, 0); atomic_store(&g_fe_options, 0); atomic_store(&g_fe_name, 0); atomic_store(&g_fe_name_done, 0);
          if (atomic_exchange(&g_fe_mode_modal, 0)) { atomic_store(&g_fe_menu, 0); fe_send("FrontEndNavigation", "ToCharacterCreate", "1"); next = fr + 8; return; }
          if (atomic_exchange(&g_fe_menu, 0)) { const char *sv = getenv("DM_SAVE"); if (sv && !has_character(sv, 0)) { fe_send("FrontEndNavigation", "ToCharacterCreate", "1"); next = fr + 8; }
              else { static int lg; if (lg++ < 6) LOG("前端路径：手柄模式，有存档 → 停在原版主菜单，不替玩家开选角页"); } }
          return; } }
    if (atomic_exchange(&g_fe_skip_tut, 0)) { fe_key(DM_BTN_B); next = fr + 8; LOG("前端路径：教学提示框 → 自动跳过（B）"); return; }
    if (atomic_exchange(&g_fe_back, 0)) { fe_key(DM_BTN_B); next = fr + 8; LOG("前端路径：模式页点了牌子以外 → 返回（B）"); return; }
















    { static int st, tries, ck; static uint64_t at; module_t *mm = main_module(); uint64_t root = 0; if (mm) memcpy(&root, mm->base + 0x2f05458, 8);
      if (atomic_exchange(&g_fe_options, 0) && !st) { st = 1; tries = 0; ck = 0; at = fr + 4; }
      if (st && fr >= at) { uint64_t pn = root ? ui_find(root, "CharacterOptionsPanel", 0) : 0, hw = pn ? *(uint64_t *)(pn + 0x1e0) : 0;
          if (!pn || !atomic_load(&g_fe_opt_open) || atomic_load(&g_fe_confirm_open)) st = 0;
          else if (ck) {
              int32_t vw = *(int32_t *)(mm->base + 0x211da38), vh = *(int32_t *)(mm->base + 0x211da3c); int x = vw / 2 + 908 * vh / 2160, y = 1079 * vh / 2160;
              ui_send_mouse(mm, root, ui_hash("MouseMove"), x, y);
              if (ck == 2) ui_send_mouse(mm, root, ui_hash("LeftMouseButtonDown"), x, y); else if (ck == 3) ui_send_mouse(mm, root, ui_hash("LeftMouseButtonUp"), x, y);
              if (++ck > 3) { ck = 0; at = fr + 4; } }
          else { int want = atomic_load(&g_fe_opt_side), cur = *(uint8_t *)(pn + 0x211) && hw && *(uint8_t *)(hw + 0xa88);
              if (cur == want) { st = 0; fe_key(DM_BTN_A); next = fr + 8; LOG("前端路径：模式页点了%s牌（原版状态一致）→ 接受（A）", want ? "专家" : "标准"); return; }
              if (tries++ < 2) { ck = 1; LOG("前端路径：模式页点了%s牌，原版状态还是%s → 替玩家点一下专家牌把勾翻过来（第 %d 次）", want ? "专家" : "标准", cur ? "专家" : "标准", tries); }
              else { st = 0; LOG("【红】前端路径：模式页点了%s牌，翻两次后原版状态仍是%s，不替玩家接受", want ? "专家" : "标准", cur ? "专家" : "标准"); } } }
      if (st) return; }


    if (atomic_exchange(&g_fe_name, 0)) { fe_key(DM_BTN_A); next = fr + 8; LOG("前端路径：起名页 → 弹键盘（A）"); return; }






    { static int seen; if (!atomic_load(&g_fe_name_done)) seen = 0; else if (!seen) { seen = 1; atomic_store(&g_fe_name_ms, fe_ms()); } }
    if (atomic_load(&g_fe_name_done)) {
        module_t *m = main_module(); uint64_t root = 0; if (m) memcpy(&root, m->base + 0x2f05458, 8);
        uint64_t pn = root ? ui_find(root, "CharacterCreateNamePanel", 0) : 0, it = pn ? ui_find(pn, "InputText", 0) : 0; size_t wl = strlen(g_fe_name_text);
        int same = 0; if (it && *(uint64_t *)it - (uint64_t)(uintptr_t)m->base == 0x20b8718) { const char *t = *(const char **)(it + 0x558); uint64_t n = *(uint64_t *)(it + 0x560); same = t && n == wl && !memcmp(t, g_fe_name_text, wl); }
        if (same) { atomic_store(&g_fe_name_done, 0); fe_key(DM_BTN_PLUS); next = fr + 8; LOG("前端路径：名字已进名字框（看到键盘交回后 %llu ms）→ 接受（PLUS）", (unsigned long long)(fe_ms() - atomic_load(&g_fe_name_ms))); }
        else if (fe_ms() - atomic_load(&g_fe_name_ms) > 2000) { atomic_store(&g_fe_name_done, 0); LOG("【红】前端路径：看到键盘交回 2 秒后名字框仍不是「%s」（%s），不自动接受，留给玩家手动确认", g_fe_name_text, it ? "内容不符" : "没找到名字框"); }
        return;
    }
    if (atomic_load(&g_fe_mode_modal)) { atomic_store(&g_fe_mode_modal, 0); atomic_store(&g_fe_menu, 0); fe_send("FrontEndNavigation", "ToCharacterCreate", "1"); next = fr + 8; return; }
    if (atomic_load(&g_fe_menu)) { atomic_store(&g_fe_menu, 0); const char *sv = getenv("DM_SAVE");
        if (sv && !has_character(sv, 0)) fe_send("FrontEndNavigation", "ToCharacterCreate", "1");
        else { fe_key(DM_BTN_X); LOG("前端路径：有存档 → 打开选角页（X）"); }
        next = fr + 8; return; }




    int ribbon = atomic_load(&g_fe_ribbon), list_vis = 0; if (ribbon) { module_t *m = main_module(); uint64_t root = 0; if (m) memcpy(&root, m->base + 0x2f05458, 8); uint64_t w = root ? ui_find(root, "CharacterSelectPanel", 0) : 0; list_vis = w && *(uint8_t *)(w + 0x50); }
    static uint64_t bare; if (ribbon && !list_vis && !atomic_load(&g_fe_confirm_open)) { if (!bare) bare = fr; else if (fr - bare >= 20) { bare = 0; const char *sv = getenv("DM_SAVE"); if (!sv || has_character(sv, 0)) { fe_key(DM_BTN_X); next = fr + 30; LOG("前端路径：选角列表被收起 → 重新打开（X）"); }
            else { atomic_store(&g_fe_ribbon, 0); fe_send("FrontEndNavigation", "ToCharacterCreate", "1"); next = fr + 30; LOG("前端路径：已没有角色（删掉了最后一个）→ 直达职业选择"); } } } else bare = 0;
}






void dm_ui_auto_mode_leave(void) {
    atomic_store(&g_fe_skip_tut, 0); atomic_store(&g_fe_back, 0); atomic_store(&g_fe_options, 0); atomic_store(&g_fe_name, 0); atomic_store(&g_fe_name_done, 0); atomic_store(&g_fe_name_key, 0); atomic_store(&g_fe_name_tap, 0);
    atomic_store(&g_fe_key_until, 0); atomic_store(&g_fe_key_mask, 0); atomic_store(&g_pulse_until, 0);
    atomic_store(&g_set_back_at, 0); atomic_store(&g_speech_back_at, 0); atomic_store(&g_tab_left, 0); atomic_store(&g_tab_direct, 0); atomic_store(&g_tab_then, 0); atomic_store(&g_del_q, 0); atomic_store(&g_del_hold, 0);
    atomic_store(&g_qt_arm, 0); atomic_store(&g_qt_speech, 0); atomic_store(&g_qt_xy, 0); atomic_store(&g_qt_stop_at, 0);
}


void dm_ui_cache_mode_leave(void) {
    module_t *m = main_module(); uint64_t root = 0; if (m) memcpy(&root, m->base + 0x2f05458, 8);
    { uint64_t bk = root ? ui_panel(root, "BankExpansionLayout") : 0, tabs = bk && *(uint8_t *)(bk + 0x50) ? ui_find(bk, "BankTabs", 0) : 0;
      g_bank_tab_keep = tabs ? TAB_SEL(tabs) : -1; g_bank_tab_until = dm_guest_frame() + 120; }
    for (int o = 0; o < MV_N; o++) mv_restore(root, o);
    g_nmv = 0; dm_wref_set(&g_down, 0); dm_wref_set(&g_slot, 0); dm_wref_set(&g_bd_hit_ref, 0); dm_wref_set(&g_click, 0); dm_wref_set(&g_tab_ref, 0); atomic_store(&g_tab_have, 0); atomic_store(&g_ut_phase, 0);


    atomic_store(&g_wtap, 0); atomic_store(&g_buy_at, 0); atomic_store(&g_tap_req, 0); atomic_store(&g_use_req, 0); atomic_store(&g_long_req, 0); atomic_store(&g_ident_req, 0);
    atomic_store(&g_dg_kind, 0); atomic_store(&g_bd_state, 0); atomic_store(&g_swaptab_tap, 0); atomic_store(&g_mercpg, 0);
    atomic_store(&g_up_hold, 0); atomic_store(&g_up_r, atomic_load(&g_up_w));
}







uint32_t dm_rw_shared_unit(uint8_t *b);
static int stash_page_no(uint8_t *b, uint32_t unit) {
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)), inv = pl ? *(uint64_t *)(pl + 0x90) : 0; int n = 0;
    if (!unit || !inv || *(uint32_t *)inv != 0x01020304) return 0;
    for (uint64_t en = *(uint64_t *)(inv + 0x68); en && n < 16; en = *(uint64_t *)(en + 0x10)) {
        if (!*(uint8_t *)(en + 9) || *(uint8_t *)(en + 0xa)) continue;
        n++; if (*(uint32_t *)en == unit) return n;
    }
    return 0;
}

static int stash_widgets(module_t *m, uint64_t *bk, uint64_t *tabs) {
    uint64_t root = 0; memcpy(&root, m->base + 0x2f05458, 8);
    *bk = root ? ui_panel(root, "BankExpansionLayout") : 0; *tabs = *bk && *(uint8_t *)(*bk + 0x50) ? ui_find(*bk, "BankTabs", 0) : 0;
    if (!*tabs || !*(uint8_t *)(*tabs + 0x50) || *(uint64_t *)*tabs != (uint64_t)m->base + 0x20ba3a8) return 0;
    if (*(uint64_t *)(*(uint64_t *)*bk + 0x28) != (uint64_t)m->base + 0x399858) {
        static int said; if (!said++) LOG("stash view: the stash panel's message handler is not the one the page turn belongs to (%#llx)", (unsigned long long)(*(uint64_t *)(*(uint64_t *)*bk + 0x28) - (uint64_t)m->base));
        return 0;
    }
    return 1;
}
int dm_ui_stash_view(int *tab, uint32_t *unit) {
    module_t *m = main_module(); uint64_t bk, tabs; *tab = -1; *unit = 0;
    if (!m || dm_ui_unstable() || !stash_widgets(m, &bk, &tabs)) return -1;
    *tab = TAB_SEL(tabs); if (*tab == 1) *unit = dm_rw_shared_unit((uint8_t *)m->base);
    return 0;
}
int dm_ui_stash_toward(int tab, uint32_t unit) {
    module_t *m = main_module(); uint64_t bk, tabs;
    if (!m || tab < 0 || (tab == 1) != (unit != 0) || dm_ui_unstable() || !stash_widgets(m, &bk, &tabs)) return -1;
    uint8_t *b = (uint8_t *)m->base; int sel = TAB_SEL(tabs);
    if (sel != tab) { ((void (*)(uint64_t, int, int))(b + 0xad9b80))(tabs, tab, 0); LOG("stash view: tab %d -> %d (now %d)", sel, tab, TAB_SEL(tabs)); return TAB_SEL(tabs) == tab ? 0 : -1; }
    if (tab != 1) return 1;
    uint32_t cur = dm_rw_shared_unit(b); if (cur == unit) return 1;
    int at = stash_page_no(b, cur), to = stash_page_no(b, unit); if (!at || !to) return -1;
    ((void (*)(uint64_t, int, int))(b + 0x39b9c0))(bk, to > at, 0);
    LOG("stash view: shared page %d -> %d (unit %u -> %u, now %u)", at, to, cur, unit, dm_rw_shared_unit(b));
    return 0;
}
