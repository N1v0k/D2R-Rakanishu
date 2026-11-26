









#include <stdatomic.h>
#include "game_addrs.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <TargetConditionals.h>
#include "dm.h"
#include "dm_widget.h"
#include "dm_cursor.h"
#include "dm_rift.h"
#include "dm_i18n.h"
#include "dm_input.h"
void dm_ui_send(const char *cat, const char *act, const char *arg);
uint64_t dm_guest_frame(void);

static _Atomic int g_dirty = 1, g_step;
static int g_pair = -1;
static _Atomic int g_act;
static int g_sel = -1, g_opened, g_open_f, g_seq;
static uint8_t *rb(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 14) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static uint64_t panel_named(const char *nm) { uint8_t *b = rb(); uint64_t root = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0; return root ? dm_ui_top(root, nm) : 0; }
static uint64_t panel(void) { return panel_named("MessageLogPanel"); }
static int panel_up(void) { uint64_t p = panel(); return p && *(uint8_t *)(uintptr_t)(p + 0x51); }
static uint64_t body(void) { uint64_t cp = panel(), bd = cp ? find(cp, "TouchRift", 1) : 0; return bd ? find(bd, "TouchRF_Body", 0) : 0; }




static void pause_set(int on) { static int mine; uint8_t *b = rb(); if (!b) return; volatile uint8_t *f = b + GA_PANEL_PAUSE_MENU;
    if (on && !mine && *(uint32_t *)(b + GA_GAME_TYPE) == 0 && !*f) { *f = 1; mine = 1; LOG("大秘境面板：开着，走原版单机暂停（界面状态 +9 置 1）"); }
    else if (!on && mine) { mine = 0; *f = 0; LOG("大秘境面板：已关，放开暂停"); } }






static void plate_hide(int on) { static int mine; uint8_t *b = rb(); if (!b) return; volatile int32_t *c = (volatile int32_t *)(b + GA_MODAL_PANEL_COUNT);
    if (on && !mine) { mine = 1; LOG("大秘境面板：模态面板计数 %d → %d，清目标槽（名牌不画）", *c, *c + 1); *c += 1; ((void (*)(uint32_t, uint64_t))(b + GA_TARGET_SLOT_CLEAR))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT), 0); }
    else if (!on && mine) { mine = 0; *c -= 1; LOG("大秘境面板：模态面板计数放回 %d", *c); } }
static void rect(uint64_t w, int x, int y) { if (!w) return; int32_t *r = (int32_t *)(w + 0x70); if (r[0] == x && r[1] == y) return; r[0] = x; r[1] = y; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); }
static void text(uint64_t w, const char *s) {
    uint8_t *b = rb(); if (!w || *(uint64_t *)w - (uint64_t)(uintptr_t)b != GA_VT_TEXTBOX) return;
    size_t n = strlen(s); if (*(uint64_t *)(w + 0x90) == n && !memcmp(*(const char **)(w + 0x88), s, n)) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(b + GA_TEXT_SET))(w + 0x88, s, n); ((void (*)(uint64_t))(b + GA_TEXT_LAYOUT))(w);
}




DM_WIDGET_RED_DEF
static void btn_grey(uint64_t w, int grey) {
    static struct { dm_wref r; int32_t f[4]; } sv[8]; static int n; if (!w || !DM_WIDGET_OK(rb(), w, DM_VT_BUTTON, "秘境面板")) return; int i = 0; while (i < n && dm_wref_get(&sv[i].r) != w) i++;
    if (i == n) { i = 0; while (i < n && dm_wref_get(&sv[i].r)) i++; if (i == 8) return; if (i == n) n++; dm_wref_set(&sv[i].r, w); memcpy(sv[i].f, (void *)(uintptr_t)(w + 0x6e8), 16); }
    int32_t *f = (int32_t *)(uintptr_t)(w + 0x6e8); int32_t want[4]; for (int k = 0; k < 4; k++) want[k] = grey ? sv[i].f[3] : sv[i].f[k];
    if (memcmp(f, want, 16)) { memcpy(f, want, 16); *(int32_t *)(uintptr_t)(w + 0x610) = want[0]; }
}
static void color(uint64_t w, int r, int g, int bl) { if (!w) return; int32_t *c = (int32_t *)(w + 0xbc); c[0] = r; c[1] = g; c[2] = bl; }
void dm_rift_ui_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (strcmp(cat, "PanelManager") || strcmp(act, "ClosePanel") || strncmp(arg, "TouchRF_", 8)) return;
    const char *k = arg + 8;
    if (!strcmp(k, "M1")) atomic_fetch_add(&g_step, -1); else if (!strcmp(k, "P1")) atomic_fetch_add(&g_step, 1);
    else if (!strcmp(k, "M10")) atomic_fetch_add(&g_step, -10); else if (!strcmp(k, "P10")) atomic_fetch_add(&g_step, 10);
    else if (!strcmp(k, "Start")) atomic_store(&g_act, 1); else if (!strcmp(k, "Cont")) atomic_store(&g_act, 2); else if (!strcmp(k, "Abandon") || !strcmp(k, "Resel")) atomic_store(&g_act, 3);
    atomic_store(&g_dirty, 1);
}


static void merc_lock_pump(void) {
    static const struct { const char *n; } k[] = { { "TouchMercLock_neck" }, { "TouchMercLock_gloves" }, { "TouchMercLock_right_hand" }, { "TouchMercLock_belt" }, { "TouchMercLock_left_hand" }, { "TouchMercLock_feet" } };
    uint64_t p = panel_named("HirelingInventoryPanel"); if (!p || !*(uint8_t *)(uintptr_t)(p + 0x51)) return; int lock = !dm_rift_merc_unlocked();
    for (unsigned i = 0; i < sizeof k / sizeof *k; i++) {
        char sn[32]; snprintf(sn, sizeof sn, "slot_%s", k[i].n + 14); uint64_t w = find(p, k[i].n, 0), sl = find(p, sn, 0);
        if (w && sl) rect(w, lock ? ((int32_t *)(uintptr_t)(sl + 0x70))[0] : 99999, ((int32_t *)(uintptr_t)(w + 0x70))[1]); }




    if (lock) { static dm_wref last; uint8_t *b = rb(); uint64_t pm = b ? *(uint64_t *)(b + GA_UI_ROOT) : 0, fm = pm ? *(uint64_t *)(uintptr_t)(pm + 0xd0) : 0, cur = 0;
        for (int c = 0; fm && c < 2 && !cur; c++) { uint64_t w = *(uint64_t *)(uintptr_t)(fm + (c ? 0x178 : 0x190)); const char *nm = w ? *(const char **)(uintptr_t)(w + 8) : NULL;
            if (!nm || strncmp(nm, "slot_", 5) || !dm_widget_is_slot(b, w) || *(uint32_t *)(uintptr_t)(w + 0x5c8) != 1) continue;
            for (unsigned i = 0; i < sizeof k / sizeof *k; i++) if (!strcmp(nm + 5, k[i].n + 14)) cur = w; }
        if (cur && cur != dm_wref_get(&last)) dm_rift_merc_locked_hint(); dm_wref_set(&last, cur); }




    { static const struct { const char *n; uint32_t loc; } sl[] = { { "slot_neck", 2 }, { "slot_right_hand", 6 }, { "slot_left_hand", 7 }, { "slot_belt", 8 }, { "slot_feet", 9 }, { "slot_gloves", 10 } };
      for (unsigned i = 0; i < sizeof sl / sizeof *sl; i++) { uint64_t w = find(p, sl[i].n, 0); if (!w || *(uint64_t *)w - (uint64_t)(uintptr_t)rb() != GA_VT_EQUIP_SLOT || *(uint32_t *)(uintptr_t)(w + 0x5d8) == sl[i].loc) continue;
          *(uint32_t *)(uintptr_t)(w + 0x5d8) = sl[i].loc; *(uint64_t *)(uintptr_t)(w + 0x5b8) = 0x6200000062ull; LOG("佣兵装备位：面板里的 %s 补上部位号 %u", sl[i].n, sl[i].loc); } }
}
void dm_rift_ui_pump(void) {
    merc_lock_pump();
    uint8_t *b = rb(); if (!b) return; uint64_t fr = dm_guest_frame();
    dm_rift_menu_t m; int want = dm_rift_menu(&m), up = panel_up();
    if (!want) { if (g_opened) { g_opened = 0; g_sel = -1; } }
    else if (!g_opened && getenv("DM_RIFT_TEST_KB")) { g_opened = 3; g_open_f = (int)fr; dm_ui_send("PanelManager", "OpenPanel", "ChroniclePanel"); return; }
    else if (g_opened == 3) { uint64_t cp = panel_named("ChroniclePanel"); if (cp && (int)fr - g_open_f > 6) { g_opened = 2;
        dm_ui_send("ChroniclePanelMessage", "ToggleChildPanel", "TouchKillBoard"); dm_ui_send("PanelManager", "ClosePanel", "TouchKB_Act5");
        uint64_t tb = find(cp, "ChronicleTabs", 2); if (tb) *(int32_t *)(tb + 0xdc4) = 3; } return; }






    else if (!g_opened || (g_opened == 1 && !up && dm_rift_menu_seq() != g_seq)) { g_opened = 1; g_seq = dm_rift_menu_seq(); g_sel = -1; dm_ui_send("PanelManager", "OpenPanel", "MessageLogPanel"); LOG("大秘境面板：点了门，开面板（%s）", m.menu == 3 ? "离开这一场的确认" : m.menu == 2 ? "继续 / 放弃" : "选层"); return; }
    if (g_opened == 1 && up) g_opened = 2;
    if (want && g_opened == 2 && !up) { LOG("大秘境面板：玩家关了面板"); dm_rift_menu_close(); g_opened = 0; g_sel = -1; }
    pause_set(up && !dm_rift_menu_pending()); plate_hide(up);
    uint64_t bw = body(); if (!bw || !up) return;
    if (!want) { m.menu = 1; dm_rift_menu_peek(&m); }
    { static const char *au = (const char *)1; static int at; if (au == (const char *)1) au = getenv("DM_RIFT_MENU_AUTO");
      if (au && want) { if (!at) at = (int)fr; if ((int)fr - at > 90) { at = 0; static int lg; if (lg++ < 4) LOG("秘境选层页：测试开关替玩家点 %s（页 %d）", au, m.menu);
          if (m.menu == 3) { static int nth; int cancel = strstr(au, "cancel") && !nth++;
              if (cancel) dm_ui_send("PanelManager", "ClosePanel", "MessageLogPanel"); else if (strstr(au, "next")) atomic_store(&g_act, 2); }
          else if (m.menu == 1 && atoi(au) > 0) { g_sel = atoi(au); if (g_sel > m.unlock) g_sel = m.unlock; atomic_store(&g_act, 1); } else if (m.menu == 2 && (strstr(au, "cont") || strstr(au, "abandon"))) atomic_store(&g_act, strstr(au, "abandon") ? 3 : 2); } } else at = 0; }








    { int dm_padmode_current(void); uint64_t dm_padmode_page_keys(uint64_t); void dm_padmode_legend(int, const char *);
      { int kx = dm_padmode_current() ? 52 : 99999, px = kx;
#if TARGET_OS_OSX


        { int dm_padmode_kbd(void); int qx = 99999; if (dm_padmode_kbd()) { qx = kx; px = 99999; }
          rect(find(bw, "TouchRF_KeyAK", 0), qx, 1181); rect(find(bw, "TouchRF_KeyA2K", 0), qx, 1181); rect(find(bw, "TouchRF_KeyXK", 0), m.menu == 3 ? 99999 : qx, 844); }
#endif
        rect(find(bw, "TouchRF_KeyA", 0), px, 1181); rect(find(bw, "TouchRF_KeyA2", 0), px, 1181); rect(find(bw, "TouchRF_KeyX", 0), m.menu == 3 ? 99999 : px, 844); }
      { uint64_t an = find(panel(), "TouchRF_Anchor", 1); int pad = dm_padmode_current(); rect(find(an, "TouchRF_CloseX", 1), pad ? 99999 : 1766, 64); rect(find(an, "TouchClose", 1), pad ? 99999 : 1705, -11); rect(find(an, "TouchRF_CloseBtn", 1), pad ? 1868 : 99999, 8); }
      if (dm_padmode_current()) { int ch = m.menu >= 2, cf = m.menu == 3;
          uint64_t ev = dm_padmode_page_keys(DM_BTN_A | DM_BTN_X | DM_BTN_LEFT | DM_BTN_RIGHT | DM_BTN_UP | DM_BTN_DOWN);
          dm_padmode_legend(0, dm_str(b, ch ? "dmRiftBtnCont" : "dmRiftBtnStart")); if (ch && !cf) dm_padmode_legend(2, dm_str(b, m.done ? "dmRiftBtnResel" : "dmRiftBtnAbandon"));
          if (!ch) { int st = (ev & DM_BTN_LEFT ? -1 : 0) + (ev & DM_BTN_RIGHT ? 1 : 0) + (ev & DM_BTN_DOWN ? -10 : 0) + (ev & DM_BTN_UP ? 10 : 0); if (st) atomic_fetch_add(&g_step, st); if (ev & DM_BTN_A) atomic_store(&g_act, 1); }
          else if (ev & DM_BTN_A) atomic_store(&g_act, 2);
          else if (cf) { }
          else if (m.done) { if (ev & DM_BTN_X) atomic_store(&g_act, 3); }
          else { static uint64_t t0; uint64_t now_ns(void), dm_padmode_page_held(void), now = now_ns();
              if (!(dm_padmode_page_held() & DM_BTN_X)) t0 = 0; else if (!t0) t0 = now; else if (t0 != 1 && now - t0 >= 800000000ull) { t0 = 1; atomic_store(&g_act, 3); LOG("大秘境面板：手柄按住 X 0.8 秒 → 放弃"); } }
          if (ev) { atomic_store(&g_dirty, 1); LOG("大秘境面板：手柄键 %#llx（%s页）", (unsigned long long)ev, cf ? "确认" : ch ? "继续 / 放弃" : "选层"); } } }
    int a = atomic_exchange(&g_act, 0);
    if (g_sel < 0) g_sel = m.def < 1 ? 1 : m.def > m.unlock ? m.unlock : m.def;
    { int st = atomic_exchange(&g_step, 0); if (st) { g_sel += st; if (g_sel < 1) g_sel = 1; if (g_sel > m.unlock) g_sel = m.unlock; } }
    if (a == 1 && m.menu == 1) { dm_rift_menu_start(g_sel); pause_set(0); dm_ui_send("PanelManager", "ClosePanel", "MessageLogPanel"); return; }
    if (a == 2 && m.menu == 2) { dm_rift_menu_continue(); pause_set(0); dm_ui_send("PanelManager", "ClosePanel", "MessageLogPanel"); return; }
    if (a == 2 && m.menu == 3) { LOG("大秘境面板：确认页点了继续"); dm_rift_menu_next(); pause_set(0); dm_ui_send("PanelManager", "ClosePanel", "MessageLogPanel"); return; }
    if (a == 3 && m.menu == 2) { dm_rift_menu_abandon(); pause_set(0); g_sel = -1; return; }
    static int frame; if (!atomic_exchange(&g_dirty, 0) && ++frame % 15) return;
    rect(find(bw, "TouchRF_Sel", 0), m.menu >= 2 ? 99999 : 0, 0); rect(find(bw, "TouchRF_Cho", 0), m.menu >= 2 ? 0 : 99999, 0);
    char t[128];






    int en = !dm_lang_zh();
    { int dx = en ? 16 : 0; static const char *kv[] = { "Range", "Best", "Str", "Lvl", "Time" };
      for (int i = 0; i < 5; i++) { int y = 414 + 64 * i; snprintf(t, sizeof t, "TouchRF_%s", kv[i]); rect(find(bw, t, 0), 230 + dx, y);
          snprintf(t, sizeof t, "TouchRF_%sL", kv[i]); rect(find(bw, t, 0), en ? 99999 : 0, y); snprintf(t, sizeof t, "TouchRF_%sLE", kv[i]); rect(find(bw, t, 0), en ? 0 : 99999, y + (en ? 4 : 0)); }
      for (int i = 0; i < 4; i++) { int y = 842 + 64 * i; for (int c = 0; c < 2; c++) { snprintf(t, sizeof t, "TouchRF_Gam%d%c", i, "US"[c]); rect(find(bw, t, 0), 230 + dx - (c == 0 && i == g_pair ? 210 : 0), y); }
          snprintf(t, sizeof t, "TouchRF_Gam%dL", i); rect(find(bw, t, 0), en ? 99999 : 0, y); snprintf(t, sizeof t, "TouchRF_Gam%dLE", i); rect(find(bw, t, 0), en ? 0 : 99999, y + (en ? 4 : 0)); } }
    rect(find(bw, "TouchRF_MsN", 0), en ? 99999 : 1030, -24); rect(find(bw, "TouchRF_MsNE", 0), en ? 930 : 99999, -24);
    rect(find(bw, "TouchRF_AbT", 0), en ? 99999 : 0, 746); rect(find(bw, "TouchRF_AbTE", 0), en ? 0 : 99999, 746); rect(find(bw, "TouchRF_ContT", 0), en ? 99999 : 0, 1083); rect(find(bw, "TouchRF_ContTE", 0), en ? 0 : 99999, 1083);
    if (m.menu == 3) {




        int retry = m.next <= m.tier;
        snprintf(t, sizeof t, dm_str(rb(), "dmRiftTierN"), m.next); text(find(bw, "TouchRF_ChTier", 0), t);
        text(find(bw, "TouchRF_ChT", 0), dm_str(rb(), retry ? "dmRiftTitleRetry" : "dmRiftTitleNext")); text(find(bw, "TouchRF_ChNote", 0), "");
        text(find(bw, "TouchRF_ChLeftL", 0), dm_str(rb(), "dmRiftLeaveWarn1")); text(find(bw, "TouchRF_ChLeft", 0), ""); text(find(bw, "TouchRF_ChProgL", 0), dm_str(rb(), "dmRiftLeaveWarn2")); text(find(bw, "TouchRF_ChProg", 0), ""); text(find(bw, "TouchRF_ChNoteL", 0), "");
        rect(find(bw, "TouchRF_Abandon", 0), 99999, 818); rect(find(bw, "TouchRF_Resel", 0), 99999, 818);
        text(find(bw, en ? "TouchRF_AbTE" : "TouchRF_AbT", 0), dm_str(rb(), "dmRiftCapStay"));
        snprintf(t, sizeof t, dm_str(rb(), "dmRiftCapGo"), m.next); text(find(bw, en ? "TouchRF_ContTE" : "TouchRF_ContT", 0), t);
    } else if (m.menu == 2) {
        snprintf(t, sizeof t, dm_str(rb(), "dmRiftTierN"), m.tier); text(find(bw, "TouchRF_ChTier", 0), t);




        { int dm_padmode_current(void); text(find(bw, en ? "TouchRF_AbTE" : "TouchRF_AbT", 0), dm_str(rb(), m.done ? "dmRiftCapResel" : dm_padmode_current() ? "dmRiftCapVoidPad" : "dmRiftCapVoid")); }






        rect(find(bw, "TouchRF_Abandon", 0), m.done ? 99999 : 0, 818); rect(find(bw, "TouchRF_Resel", 0), m.done ? 0 : 99999, 818);
        text(find(bw, en ? "TouchRF_ContTE" : "TouchRF_ContT", 0), dm_str(rb(), m.done ? "dmRiftCapGuard" : m.overtime ? "dmRiftCapContOver" : "dmRiftCapCont"));
        text(find(bw, "TouchRF_ChNote", 0), ""); text(find(bw, "TouchRF_ChT", 0), dm_str(rb(), m.done ? (m.inc ? "dmRiftTitleDone" : "dmRiftTitleLate") : m.overtime ? "dmRiftTitleLate" : "dmRiftTitleRun"));
        if (m.done) { text(find(bw, "TouchRF_ChLeftL", 0), dm_str(rb(), "dmRiftGuardDown")); text(find(bw, "TouchRF_ChLeft", 0), ""); text(find(bw, "TouchRF_ChProgL", 0), dm_str(rb(), m.inc ? "dmRiftNextOpen" : "dmRiftRetryOpen")); text(find(bw, "TouchRF_ChProg", 0), ""); text(find(bw, "TouchRF_ChNoteL", 0), dm_str(rb(), m.inc ? "dmRiftSaved" : "dmRiftNotCounted")); }
        else { int s = m.secs_left; text(find(bw, "TouchRF_ChLeftL", 0), dm_str(rb(), "dmRiftTimeLeft")); snprintf(t, sizeof t, "%d:%02d", s / 60, s % 60); text(find(bw, "TouchRF_ChLeft", 0), t);
            text(find(bw, "TouchRF_ChProgL", 0), dm_str(rb(), "dmRiftProgress")); snprintf(t, sizeof t, "%d%%", (int)(m.progress * 100)); text(find(bw, "TouchRF_ChProg", 0), t); text(find(bw, "TouchRF_ChNoteL", 0), ""); }
    } else {
        snprintf(t, sizeof t, "%d", g_sel); text(find(bw, "TouchRF_Num", 0), t);
        btn_grey(find(bw, "TouchRF_M1", 0), g_sel <= 1); btn_grey(find(bw, "TouchRF_M10", 0), g_sel <= 1); btn_grey(find(bw, "TouchRF_P1", 0), g_sel >= m.unlock); btn_grey(find(bw, "TouchRF_P10", 0), g_sel >= m.unlock);
        snprintf(t, sizeof t, "1 – %d", m.unlock); text(find(bw, "TouchRF_Range", 0), t);
        snprintf(t, sizeof t, "%d", m.best); text(find(bw, "TouchRF_Best", 0), t);



        int lv = dm_rift_tier_level(g_sel, m.clvl), hx = dm_rift_tier_hp_x10(g_sel, m.clvl);
        if (hx < 100) snprintf(t, sizeof t, "×%d.%d", hx / 10, hx % 10); else snprintf(t, sizeof t, "×%d", (hx + 5) / 10); text(find(bw, "TouchRF_Str", 0), t);
        if (lv) snprintf(t, sizeof t, "%d", lv); else snprintf(t, sizeof t, "%s", dm_str(rb(), "dmRiftLvlBase")); text(find(bw, "TouchRF_Lvl", 0), t);
        text(find(bw, "TouchRF_Time", 0), "15:00");
        uint32_t gu, gs, nu, ns; dm_rift_gamble_vals(m.best, &gu, &gs); dm_rift_gamble_vals(g_sel, &nu, &ns);


        char ln[4][3][40]; memset(ln, 0, sizeof ln); int r = 0;
        if (gu) { snprintf(ln[r][0], 40, "%s", dm_str(rb(), "dmRiftGamUni")); snprintf(ln[r++][1], 40, "%u.%u%%", gu / 1000, gu / 100 % 10); snprintf(ln[r][0], 40, "%s", dm_str(rb(), "dmRiftGamSet")); snprintf(ln[r++][2], 40, "%u.%u%%", gs / 1000, gs / 100 % 10); }



        int pair = -1;
        if ((!gu || g_sel > m.best) && en) { snprintf(ln[r++][0], 40, "%s", dm_str(rb(), "dmRiftGamAfterEn")); pair = r; snprintf(ln[r][1], 40, "%u.%u%%", nu / 1000, nu / 100 % 10); snprintf(ln[r++][2], 40, "%u.%u%%", ns / 1000, ns / 100 % 10); }
        else if (!gu || g_sel > m.best) { snprintf(ln[r][0], 40, "%s", dm_str(rb(), "dmRiftGamUniAfter")); snprintf(ln[r++][1], 40, "%u.%u%%", nu / 1000, nu / 100 % 10); snprintf(ln[r][0], 40, "%s", dm_str(rb(), "dmRiftGamSetAfter")); snprintf(ln[r++][2], 40, "%u.%u%%", ns / 1000, ns / 100 % 10); }
        g_pair = pair;
        for (int k = 0; k < 4; k++) for (int c = 0; c < 3; c++) { char nm[24]; snprintf(nm, sizeof nm, "TouchRF_Gam%d%s", k, c == 0 ? (en ? "LE" : "L") : c == 1 ? "U" : "S"); text(find(bw, nm, 0), ln[k][c]); }
    }
    int nxt = -1; for (int i = 0; i < 10; i++) if (!(m.ms & (1u << i))) { nxt = i; break; }
    uint64_t list = find(bw, "TouchRF_List", 4);
    for (int i = 0; i < 10 && list; i++) { char nm[20]; snprintf(nm, sizeof nm, "TouchRF_Row%d", i); uint64_t w = find(list, nm, 0); if (!w) continue;
        int got = (m.ms >> i) & 1; uint64_t st = find(w, "State", 0);
        text(st, got ? dm_str(rb(), "dmRiftMsGot") : i == nxt ? dm_str(rb(), "dmRiftMsNext") : dm_str(rb(), "dmRiftMsNo"));
        if (got) color(st, 0, 255, 0); else if (i == nxt) color(st, 255, 215, 0); else color(st, 160, 160, 160); }
}
