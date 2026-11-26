





































#include "dm_build.h"
#include "game_addrs.h"
#include "dm_widget.h"
#include "dm_input.h"
#include "dm_i18n.h"
#include "hle.h"
#include "dm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DM_WIDGET_RED_DEF

void dm_ui_move_ours(uint64_t w, int x);
int dm_ui_tree_tab(uint64_t tree);
int dm_padmode_current(void);
uint64_t dm_padmode_page_keys(uint64_t mask);

static const int COL_X[3] = { 451, 789, 1125 }, TAB_X[3] = { 274, 588, 898 };




enum { OFF = 99999, PAD = 5, TAB_DX = 262, CARD_X = 0, STAT_X = 735, OPEN_X = -430 };

static uint64_t find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 6) return 0; const char *nm = *(const char **)(uintptr_t)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(uintptr_t)(w + 0x58), n = *(uint64_t *)(uintptr_t)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find(*(uint64_t *)(uintptr_t)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}

static void set_text(uint8_t *b, uint64_t w, const char *s) {
    if (!w || !DM_WIDGET_OK(b, w, DM_VT_TEXT, "build guide card")) return;
    size_t n = strlen(s); if (*(uint64_t *)(uintptr_t)(w + 0x90) == n && (!n || !memcmp(*(const char **)(uintptr_t)(w + 0x88), s, n))) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(b + GA_TEXT_SET))(w + 0x88, s, n); ((void (*)(uint64_t))(b + GA_TEXT_LAYOUT))(w);
}

static int tab_of(int page) {
    static int map = -1; if (map < 0) { const char *e = getenv("DM_BUILD_TABMAP"); map = e ? atoi(e) : 0; }
    return page < 1 || page > 3 ? -1 : map == 1 ? page - 1 : 3 - page;
}

static uint8_t *main_base(void) { for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) return (uint8_t *)g_mod[i].base; return NULL; }









static const char *const SKILL_BREAK_DE[] = { "skillname129", "skillname135", "LevitateName", "DemonicMasteryName" };
enum { SKILL_BREAK_N = sizeof SKILL_BREAK_DE / sizeof *SKILL_BREAK_DE, SKILL_BREAK_MIN = 6 };

static size_t skill_break_suffix(uint8_t *b) {
    static int done; static size_t len;
    if (done) return len;
    const char *first = dm_str(b, SKILL_BREAK_DE[0]); size_t n0 = strlen(first); len = n0;
    for (int i = 1; i < SKILL_BREAK_N; i++) {
        const char *s = dm_str(b, SKILL_BREAK_DE[i]); size_t n = strlen(s), k = 0;
        while (k < len && k < n && first[n0 - 1 - k] == s[n - 1 - k]) k++;
        len = k;
    }
    while (len && (first[n0 - len] & 0xc0) == 0x80) len--;
    if (len < SKILL_BREAK_MIN) { LOG("ASSUME build.card_break: the long German skill names share only %zu bytes at their end, no break", len); len = 0; }
    done = 1;
    return len;
}

static const char *skill_name(uint8_t *b, const char *key, int de, char *buf, size_t size) {
    const char *name = dm_str(b, key);
    if (!de) return name;
    int listed = 0; for (int i = 0; i < SKILL_BREAK_N; i++) listed |= !strcmp(key, SKILL_BREAK_DE[i]);
    size_t n = strlen(name), k = listed ? skill_break_suffix(b) : 0;
    if (!k || n <= k || n + 3 > size) return name;
    snprintf(buf, size, "%.*s-\n%s", (int)(n - k), name, name + n - k);
    return buf;
}




static void card_texts(uint8_t *b, uint64_t card) {
    char part[DM_BUILD_CARD_PARTS][DM_BUILD_CARD_TEXT]; int to, at, st; dm_build_card_get(part, &to, &at, &st);
    int de = dm_lang_group() == DM_LG_DE; char line[160], name[DM_BUILD_CARD_TEXT];
    set_text(b, find(card, "TouchBuildCardTitle", 0), part[de ? 0 : 1]);
    set_text(b, find(card, "TouchBuildCardPhase", 0), part[de ? 2 : 3]);
    if (st == 2) snprintf(line, sizeof line, dm_str(b, to == 1 ? "dmBuildCardSave1" : "dmBuildCardSaveN"), to);
    else snprintf(line, sizeof line, "%s", dm_str(b, "dmBuildCardNext"));
    set_text(b, find(card, "TouchBuildCardLabel", 0), line);
    int skill = (st == 0 || st == 1 || st == 2) && part[4][0];
    set_text(b, find(card, "TouchBuildCardSkill", 0), skill ? skill_name(b, part[4], de, name, sizeof name) : st == 5 ? dm_str(b, "dmBuildCardDone") : st == 4 ? dm_str(b, "dmBuildCardBlocked") : dm_str(b, "dmBuildCardWait"));
    if (st == 0) snprintf(line, sizeof line, dm_str(b, "dmBuildCardTo"), to);
    else if ((st == 2 || st == 3) && at > 0) snprintf(line, sizeof line, dm_str(b, st == 2 ? "dmBuildCardFrom" : "dmBuildCardUntil"), at);
    else snprintf(line, sizeof line, "%s", st == 1 ? dm_str(b, "dmBuildCardLevelUp") : "");
    set_text(b, find(card, "TouchBuildCardTo", 0), line);
    set_text(b, find(card, "TouchBuildCardNote", 0), st == 2 ? part[de ? 5 : 6] : "");
}

static void tree_pump(uint64_t root, uint8_t *b, int hints) {
    uint64_t tw = root ? dm_ui_top(root, "SkillsTreePanel") : 0; int open = tw && *(uint8_t *)(uintptr_t)(tw + 0x50);
    static int was_open, last_state = -2;
    if (!open && !was_open) return;
    int session, page, row, col, show, guide; dm_build_target_get(&session, &page, &row, &col, &show, &guide);
    int live = open && hints && session && session == dm_build_session();
    int cur = open ? dm_ui_tree_tab(tw) : -1, want = live && show && row >= 1 && row <= 6 && col >= 1 && col <= 3 ? tab_of(page) : -1;
    char nm[24]; int placed = -1;
    for (int k = 0; k < 6 && tw; k++) {
        snprintf(nm, sizeof nm, "TouchBuildRow%d", k); uint64_t w = find(tw, nm, 0); if (!w) continue;
        int on = want >= 0 && cur == want && k == row - 1;
        dm_ui_move_ours(w, on ? COL_X[col - 1] - PAD : OFF); if (on) placed = k;
    }
    uint64_t tab = tw ? find(tw, "TouchBuildTab", 0) : 0;
    if (tab) dm_ui_move_ours(tab, want >= 0 && want <= 2 && cur != want ? TAB_X[want] + TAB_DX : OFF);
    int card_s, to, at, st; char part[DM_BUILD_CARD_PARTS][DM_BUILD_CARD_TEXT]; card_s = dm_build_card_get(part, &to, &at, &st);
    int card_on = live && guide && card_s == session;
    uint64_t card = tw ? find(tw, "TouchBuildCard", 0) : 0;
    if (card) { if (card_on && b) card_texts(b, card); dm_ui_move_ours(card, card_on ? CARD_X : OFF); }
    else { uint64_t btn = tw ? find(tw, "TouchBuildOpen", 0) : 0; if (btn) dm_ui_move_ours(btn, live && guide ? OPEN_X : OFF); }
    if (card_on && dm_padmode_current() && (dm_padmode_page_keys(DM_BTN_RS) & DM_BTN_RS)) { dm_build_open_request(); LOG("构建指南：手柄右摇杆 → 打开指南"); }
    int state = !open ? -1 : (placed >= 0 ? placed * 10 + cur : 100 + (cur & 0xff)) + (card_on ? 1000 : 0) + (hints ? 0 : 2000);
    if (state != last_state) { LOG("构建指南：技能树%s，当前页签 %d，目标 页 %d 行 %d 列 %d（页签 %d）%s%s%s", open ? "开着" : "关了", cur, page, row, col, want,
                                   placed >= 0 ? "→ 金框已放" : want >= 0 ? "→ 目标在别的页签（徽记）" : "→ 不画", card_on ? "，卡片在" : "", hints ? "" : "，游戏内提示关着"); last_state = state; }
    was_open = open;
}

static void stats_pump(uint64_t root, int hints) {
    uint64_t sw = root ? dm_ui_top(root, "CharacterStatsPanel") : 0; int open = sw && *(uint8_t *)(uintptr_t)(sw + 0x50);
    static int was_open, last_state = -2;
    if (!open && !was_open) return;
    int session, stat, show; dm_build_stat_get(&session, &stat, &show);
    int on = open && hints && show && stat >= 1 && stat <= 4 && session && session == dm_build_session() && dm_build_free_stats() > 0;
    char nm[24];
    for (int k = 0; k < 4 && sw; k++) { snprintf(nm, sizeof nm, "TouchBuildStat%d", k); uint64_t w = find(sw, nm, 0); if (w) dm_ui_move_ours(w, on && k == stat - 1 ? STAT_X : OFF); }
    int state = !open ? -1 : on ? stat : 0;
    if (state != last_state) { LOG("构建指南：属性页%s，目标属性 %d%s", open ? "开着" : "关了", stat, on ? " → 金框已放" : " → 不画"); last_state = state; }
    was_open = open;
}

void dm_build_ui_pump(uint64_t root) {
    int hints = dm_build_hints();
    tree_pump(root, main_base(), hints);
    stats_pump(root, hints);
}
