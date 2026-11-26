























#include "dm_build.h"
#include "game_addrs.h"
#include "dm.h"
#include "hle.h"
#include "dm_actions.h"
#include "dm_cursor_item.h"
#include "dm_cursor.h"
#include "dm_custom.h"
#include "dm_input.h"
#include "dm_i18n.h"
#include "dm_itemcmp_core.h"
#include "dm_effects.h"
#include "dm_elysium.h"
#include "dm_tipx.h"
#include "dm_gear_tab.h"
#include "dm_splash_rule.h"
#include <stdarg.h>
#include <stdatomic.h>
#include <time.h>

static uint8_t *mb(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = g_mod[i].base; return b; }
#ifndef DM_PLAYER_BUILD
static int env_int(const char *k, int def) { const char *e = getenv(k); return e ? atoi(e) : def; }
#else


#define env_int(k, def) (def)
#endif
static int gstat(uint8_t *b, uint64_t u, int st, int layer) { return u ? ((int (*)(uint64_t, int, int))(b + GA_STAT_GET))(u, st, layer) : 0; }
static uint64_t player(uint8_t *b) { return ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(b + GA_LOCAL_PLAYER_SLOT)); }
static uint64_t item_by_id(uint8_t *b, uint32_t id) { return ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + GA_UNIT_FIND))((uint64_t)(uintptr_t)(b + GA_CLIENT_UNIT_TABLE + 4 * 0x400), id & 0x7f, id, 4); }
static uint64_t first_item(uint64_t u) { uint64_t inv = u ? *(uint64_t *)(u + 0x90) : 0; return inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; }
static uint64_t next_item(uint64_t it) { uint64_t d = *(uint64_t *)(it + 0x10); return d ? *(uint64_t *)(d + 0xb0) : 0; }
static uint32_t item_code(uint8_t *b, uint64_t it) { return ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it); }
static uint64_t held_item(uint64_t pl) { uint64_t inv = pl ? *(uint64_t *)(pl + 0x90) : 0; return inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0; }


static const struct { int st; const char *nm; } k_ms[] = {
    {0, "str"}, {1, "ene"}, {2, "dex"}, {3, "vit"}, {7, "maxhp"}, {9, "maxmana"}, {11, "maxstam"}, {39, "fireres"}, {40, "maxfire"}, {41, "lightres"}, {42, "maxlight"},
    {43, "coldres"}, {44, "maxcold"}, {45, "poisres"}, {46, "maxpois"}, {60, "ll"}, {62, "ml"}, {76, "hp%"}, {77, "mana%"}, {79, "gf"}, {80, "mf"}, {93, "ias"}, {96, "frw"},
    {99, "fhr"}, {102, "fbr"}, {105, "fcr"}, {127, "allskill"}, {83, "classskill"} };
#define NMS (int)(sizeof k_ms / sizeof *k_ms)
static int ms_layer(uint64_t pl, int st) { return st == 83 && pl ? (int)*(uint32_t *)(pl + 4) : 0; }

static void esc(char *o, size_t on, const char *s, size_t n) {
    size_t k = 0; for (size_t i = 0; s && i < n && k + 4 < on; i++) { unsigned char c = (unsigned char)s[i];
        if (c == '\n') o[k++] = '|'; else if (c == 0xee && i + 2 < n && (unsigned char)s[i + 1] == 0x81 && (unsigned char)s[i + 2] == 0xbe) { o[k++] = '^'; i += 2; } else if (c >= 0x20 || c >= 0x80) o[k++] = (char)c; else o[k++] = '.'; }
    o[k] = 0;
}
static void dump_list(uint8_t *b, const char *who, uint64_t u) {
    (void)b; uint64_t sl = u ? *(uint64_t *)(u + 0x88) : 0; if (!sl) { LOG("[cmp] 表 %s：没有属性表", who); return; }
    int ext = *(int32_t *)(sl + 0x1c) < 0; static char o[1600]; int k = 0;
    for (int part = 0; part < 1 + ext; part++) { uint64_t a = sl + (part ? 0xe8 : 0x30), arr = *(uint64_t *)a, cnt = *(uint64_t *)(a + 8);
        k += snprintf(o + k, sizeof o - k, " %s[%llu]:", part ? "full" : "base", (unsigned long long)cnt);
        for (uint64_t i = 0; arr && i < cnt && i < 96 && k < (int)sizeof o - 24; i++) { uint32_t key = *(uint32_t *)(arr + 8 * i); k += snprintf(o + k, sizeof o - k, " %u/%u=%d", key >> 16, key & 0xffff, *(int32_t *)(arr + 8 * i + 4)); } }
    LOG("[cmp] 表 %s flags %#x%s", who, *(uint32_t *)(sl + 0x1c), o);
}
static void dump_getter(uint8_t *b, const char *who, uint64_t u, uint64_t pl) {
    static char o[900]; int k = 0; for (int i = 0; i < NMS && k < (int)sizeof o - 24; i++) { int v = gstat(b, u, k_ms[i].st, ms_layer(pl, k_ms[i].st)); if (v) k += snprintf(o + k, sizeof o - k, " %s=%d", k_ms[i].nm, v); }
    LOG("[cmp] 取值 %s:%s", who, k ? o : " (全 0)");
}
static void dump_item(uint8_t *b, const char *who, uint64_t it, uint64_t pl) {
    if (!it) { LOG("[cmp] %s：没有", who); return; } uint64_t d = *(uint64_t *)(it + 0x10); uint32_t code = item_code(b, it);
    LOG("[cmp] %s：id %u「%.3s」品质 %u 标志 %#x 模式 %u 栏位 %u 页 %u 文件号 %u", who, *(uint32_t *)(it + 8), (const char *)&code, d ? *(uint32_t *)d : 0, d ? *(uint32_t *)(d + 0x18) : 0,
        *(uint32_t *)(it + 0xc), d ? *(uint8_t *)(d + 0x54) : 0, d ? *(uint8_t *)(d + 0x55) : 0, d ? *(uint32_t *)(d + 0x34) : 0);
    dump_getter(b, who, it, pl); dump_list(b, who, it);
    if (pl) {
        int so = 0, dx = 0, lv = 0, ok = ((int (*)(uint64_t, uint64_t, int, int *, int *, int *, int))(b + GA_ITEM_REQS_MET))(it, pl, 1, &so, &dx, &lv, 0), rl = ((int (*)(uint64_t, uint64_t))(b + GA_ITEM_REQ_LEVEL))(it, pl);
        uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4); int rs = -1, rd = -1;
        if (t && ci < *(uint32_t *)(t + 0x15c8)) { uint64_t rec = *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0; rs = *(uint16_t *)(rec + 0x11a); rd = *(uint16_t *)(rec + 0x11c); }
        int pct = gstat(b, it, 91, 0), pct2 = ((int (*)(uint64_t, uint64_t, int, int))(b + GA_SKILL_MASTERY))(pl, it, 0, 6);
        LOG("[cmp]   需求 %s：游戏判定 %d（力量够 %d 敏捷够 %d 等级够 %d）等级需求 %d 表力量 %d 表敏捷 %d 百分比 %d+%d 玩家 力量 %d 敏捷 %d 等级 %d", who, ok, so, dx, lv, rl, rs, rd, pct, pct2, gstat(b, pl, 0, 0), gstat(b, pl, 2, 0), gstat(b, pl, 12, 0)); }
    { uint64_t c = first_item(it); for (int n = 0; c && n < 6; n++, c = next_item(c)) { uint32_t cc = item_code(b, c); char w[48]; snprintf(w, sizeof w, "%s socket %.3s", who, (const char *)&cc); dump_getter(b, w, c, pl); } }
}


static int g_spike = -1;
static char g_mark[16384];
static uint64_t g_panel;
void dm_itemcmp_spike_obs(uint64_t *regs) {
    if (g_spike < 0) { g_spike = env_int("DM_TEST_ITEMCMP_SPIKE", 0); if (g_spike > 0) { int dm_freeze_attach_probe(void); LOG("[cmp] 探针：渲染入口门（观测门 13）%s", dm_freeze_attach_probe() ? "已挂" : "【红】产物里没有，看不到提交之后的记录"); } }
    if (g_spike <= 0) return;
    static int depth; if (depth) return; depth = 1;
    uint8_t *b = mb(); if (!b) return;
    static uint64_t fr_last, sig_last[8]; static int nfr; uint64_t fr = dm_guest_frame(); if (fr != fr_last) { fr_last = fr; nfr = 0; } int idx = nfr++;
    uint64_t lr = regs[101], lro = lr - (uint64_t)(uintptr_t)b; if (lro > 0x2000000) lro = 0;
    const char *t = (const char *)(uintptr_t)regs[1], *t2 = (const char *)(uintptr_t)regs[5]; size_t tn = t ? strlen(t) : 0, t2n = t2 ? strlen(t2) : 0;
    g_panel = regs[0];
    uint64_t w = 0, vt = 0, it = 0; int item_prod = lro == GA_AT_ITEM_TIP_SUBMIT_RET || lro == GA_AT_ITEM_TIP_SUBMIT2_RET || lro == GA_AT_EQUIP_TIP_SUBMIT_RET;
    if (item_prod) { w = regs[19]; vt = w ? *(uint64_t *)w - (uint64_t)(uintptr_t)b : 0; if (vt == GA_VT_ITEM_GRID || vt == GA_VT_EQUIP_SLOT) it = ((uint64_t (*)(uint64_t, void *))(b + GA_ITEM_CELL_ITEM))(w, NULL); if (it && *(uint32_t *)it != 4) it = 0; }
    uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < tn; i++) h = (h ^ (uint8_t)t[i]) * 1099511628211ull; h ^= lro * 31 + (it ? *(uint32_t *)(it + 8) : 0);
    if (idx < 8 && sig_last[idx] != h) { sig_last[idx] = h;
        dm_cursor_item_info ci; int have = dm_cursor_item(&ci); static char e1[700], e2[200]; esc(e1, sizeof e1, t, tn < 600 ? tn : 600); esc(e2, sizeof e2, t2, t2n < 150 ? t2n : 150);
        LOG("[cmp] 提交 帧 %llu 第 %d 条 调用者 %#llx 面板 %#llx x2 %#llx x3 %#llx 样式 %#llx 字 %zu 字2 %zu 控件 %#llx 虚表 %#llx 物品 %u 模式 %u 631=%d 632=%d 光标物品 %u",
            (unsigned long long)fr, idx, (unsigned long long)lro, (unsigned long long)regs[0], (unsigned long long)regs[2], (unsigned long long)regs[3], (unsigned long long)regs[4], tn, t2n,
            (unsigned long long)w, (unsigned long long)vt, it ? *(uint32_t *)(it + 8) : 0, it ? *(uint32_t *)(it + 0xc) : 0, w && vt ? *(uint8_t *)(w + 0x631) : -1, w && vt ? *(uint8_t *)(w + 0x632) : -1, have ? ci.id : 0);
        LOG("[cmp]   字「%s」 字2「%s」", e1, e2);
        if (it) { uint64_t pl = player(b); dump_item(b, "cand", it, pl); int sl = 0; int ok = ((int (*)(uint64_t, uint64_t, int *, int, int))(b + GA_EQUIP_SLOT_PICK))(pl, it, &sl, 1, 0);
            int may = ok ? ((int (*)(uint64_t, int, uint64_t, int, int))(b + GA_ITEM_PLACE_CHECK))(pl, sl, it, 1, 0) : -1; LOG("[cmp]   选栏位 %d → %d，能放 %d", ok, sl, may);
            for (uint64_t q = first_item(pl); q; q = next_item(q)) { uint64_t d = *(uint64_t *)(q + 0x10); if (*(uint32_t *)(q + 0xc) == 1 && d && (!ok || *(uint8_t *)(d + 0x54) == sl)) dump_item(b, "worn", q, pl); }
            static int once; if (!once++) { dump_getter(b, "player", pl, pl); dump_list(b, "player", pl); } } }
    if (g_spike == 3 && item_prod && it) {
        static int done; static uint8_t rec[0x2e8]; static char tx[8192]; if (!done) { done = 1;
            module_t *mm = NULL; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) mm = &g_mod[i]; (void)mm;
            uint64_t root = *(uint64_t *)(b + GA_UI_ROOT); float sc = root ? ((float (*)(uint64_t))(b + GA_UI_SCALE_GET))(root) : 0; int H = *(int32_t *)(b + GA_UI_HEIGHT), W = *(int32_t *)(b + GA_UI_WIDTH), pad = *(int32_t *)(regs[0] + 0xa40);
            LOG("[cmp] 量：屏 %dx%d 面板边距 %d 比例 %.3f → 上限高 %d；最小字号 %d", W, H, pad, sc, H - 2 * (int)(sc * pad), *(int32_t *)(regs[0] + 0xa50));
            static const char L1[] = "\xee\x81\xbe" "2Zauberrate +5 (15 % auf 20 %)";
            static const int nl[] = { 1, 2, 4, 8, 16, 24, 32, 40 }, nc[] = { 10, 20, 40, 80 };
            for (int v = 0; v < 12; v++) { int k = 0; if (v < 8) { for (int i = 0; i < nl[v]; i++) k += snprintf(tx + k, sizeof tx - k, "%s%s", i ? "\n" : "", L1); }
                else { for (int i = 0; i < nc[v - 8]; i++) tx[k++] = "Leben Mana Feuer "[i % 17]; tx[k] = 0; }
                memset(rec, 0, sizeof rec); ((void (*)(void *, const char *, uint64_t, uint64_t, uint64_t, const char *))(b + GA_TOOLTIP_RECORD_BUILD))(rec, tx, regs[2], regs[3], regs[4], "");
                const int32_t *q = (const int32_t *)(rec + 0x60); uint64_t sp = *(uint64_t *)rec;
                LOG("[cmp] 量 %s %d：框 %d %d %d %d %d %d %d %d 字号 %d %d；记录的字 %#llx（我们的 %p，%s）长 %llu 内容%s", v < 8 ? "行" : "字", v < 8 ? nl[v] : nc[v - 8], q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7], *(int32_t *)(rec + 0x80), *(int32_t *)(rec + 0x84),
                    (unsigned long long)sp, (void *)tx, sp == (uint64_t)(uintptr_t)tx ? "同一块！" : "另一块", (unsigned long long)*(uint64_t *)(rec + 8), sp && sp != (uint64_t)(uintptr_t)tx && !memcmp((const void *)(uintptr_t)sp, tx, strlen(tx)) ? "相同（是拷贝）" : "不同"); } } }
    if (g_spike == 2 && item_prod && it && tn + 64 < sizeof g_mark) {
        int k = snprintf(g_mark, sizeof g_mark, "\xee\x81\xbe" "4SPIKE %llu\n", (unsigned long long)fr); memcpy(g_mark + k, t, tn + 1); regs[1] = (uint64_t)(uintptr_t)g_mark; }
    depth = 0;
}


















enum { CBK_DEF31 = 1, CBK_DEFPCT = 2, CBK_TOHIT = 4, CBK_TOHITP = 8, CBK_BLOCK = 16, CBK_STR = 32, CBK_DEX = 64, CBK_WDMG = 128, CBK_FDMG = 256, CBK_WED = 512,
       CBK_ED = 1024, CBK_ELEM = 2048, CBK_W111 = 4096, CBK_SHIELD = 8192, CBK_WCHG = 16384, CBK_MODE = 32768, CBK_SET = 65536 };
static const unsigned k_cb_mask[IC_C_N] = { CBK_DEF31 | CBK_DEFPCT | CBK_DEX, CBK_BLOCK | CBK_DEX | CBK_SHIELD, CBK_TOHIT | CBK_TOHITP | CBK_DEX | CBK_WCHG,
                                            CBK_WDMG | CBK_FDMG | CBK_WED | CBK_ED | CBK_ELEM | CBK_W111 | CBK_STR | CBK_DEX | CBK_WCHG | CBK_MODE };










static const uint64_t k_proven[] = {
    0xe11a0001u,
    0xe2a00001u,
    0xe4a00001u,
    0xe8002000u,
    0xea000010u,
    0xec000040u,
    0xf09c4004u,
    0xf09e4004u,
    0xf11a4004u,
    0xf4a00040u,
    0xf6a00004u,
    0xf89c4880u,
    0xf89e4880u,
    0xf91ac880u,
    0xfaa00800u,
    0xfea00100u,
    0xfea00120u,




    0x0a000010u,
    0x2a000010u,
    0x4a000010u,
    0x6a000010u,
    0x8a000010u,
    0xaa000010u,
    0xca000010u,
    0x8c000040u,
};
static int cb_proven(uint64_t cell) { for (size_t i = 0; i < sizeof k_proven / sizeof *k_proven; i++) if (k_proven[i] && k_proven[i] == cell) return 1; return 0; }
static int lstat_(uint8_t *b, uint64_t u, int st) { uint64_t l = u ? *(uint64_t *)(u + 0x88) : 0; return l ? ((int (*)(uint8_t, uint64_t, int, int))(b + GA_STATLIST_GET))(*(uint8_t *)(u + 0x1bd), l, st, 0) : 0; }
static uint64_t item_rec(uint8_t *b, uint64_t it) {
    if (!it || *(uint32_t *)it != 4) return 0; uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
    return t && ci < *(uint32_t *)(t + 0x15c8) ? *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0 : 0; }
static int is_type(uint8_t *b, uint64_t it, int ty) { return it && ((int (*)(uint64_t, int))(b + GA_ITEM_IS_TYPE))(it, ty) != 0; }
static int hand_off(uint64_t it) { uint64_t d = it && *(uint32_t *)it == 4 ? *(uint64_t *)(it + 0x10) : 0; return d && (*(uint8_t *)(d + 0x19) & 0x41); }
static int wield(uint8_t *b, uint64_t pl, uint64_t it) { return ((int (*)(uint64_t, uint64_t, int))(b + GA_ITEM_WIELD_MODE))(pl, it, 0); }
static int hand_mode(uint8_t *b, uint64_t pl, uint64_t r4, uint64_t r5) {
    if (r4 && r5) { int a = wield(b, pl, r5), c = wield(b, pl, r4); return a == 3 || c == 3 ? 1 : a == 2 || c == 2 ? 2 : 1; }
    uint64_t x = r5 ? r5 : r4; if (!x) return 0; int r = wield(b, pl, x); return (r | 1) == 3 ? 2 : 1; }
static int can_block(uint8_t *b, uint64_t r4, uint64_t r5) { return (r4 && !hand_off(r4) && is_type(b, r4, 51)) || (r5 && !hand_off(r5) && is_type(b, r5, 51)); }
static uint64_t hands_weapon(uint8_t *b, uint64_t r4, uint64_t r5, int *dual) { int w4 = is_type(b, r4, 45), w5 = is_type(b, r5, 45); *dual = w4 && w5; return w5 ? r5 : w4 ? r4 : 0; }
static uint64_t skill_node(uint64_t pl, int id) { uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0; for (int k = 0; nd && k < 1024; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (r && *(int16_t *)r == id && *(int32_t *)(nd + 0x44) == -1) return nd; } return 0; }
static const char *raw_str(uint8_t *b, const char *key) { struct { const char *k; uint64_t n; } s = { key, strlen(key) }; const char *r = ((const char *(*)(void *))(b + GA_LOC_STRING_GET))(&s); return r && *r && strcmp(r, key) ? dm_untag(r) : NULL; }
typedef struct { int nowv[IC_C_N + 1], aftv[IC_C_N + 1], gamev[IC_C_N + 3]; dm_ic_veto v; unsigned unproven, affected, live; uint64_t cell[IC_C_N]; unsigned cat; dm_ic_cb now, aft; int r0[IC_C_N]; } cb_res;
static int cb_classic(uint8_t *b) { uint64_t gi = *(uint64_t *)(b + GA_GAME_INFO); return gi && *(uint8_t *)(gi + 0x5c) == 1; }




static int skill_calc(uint8_t *b, uint64_t pl, uint64_t nd, int lvl, int which) {
    uint64_t rec = nd ? *(uint64_t *)nd : 0; uint8_t ver = *(uint8_t *)(pl + 0x1bd); if (!rec || ver < 1 || ver > 3) return 0;
    uint64_t t = *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16); int16_t sd = *(int16_t *)(rec + 0x23c); if (!t || sd < 0 || (uint64_t)sd >= *(uint64_t *)(t + 0x11d0)) return 0;
    uint32_t off = *(uint32_t *)(*(uint64_t *)(t + 0x11c8) + (uint64_t)sd * 0x12c + 0x14 + 4 * which); uint64_t size = *(uint64_t *)(t + 0x158); if (size <= off) return 0;
    struct { uint64_t fn, u[7]; int32_t lvl; uint8_t z[4]; int32_t sk, lvl2; } ctx = { (uint64_t)(uintptr_t)(b + GA_SKILL_CALC_CTX_FN), { pl, pl, pl, pl, pl, pl, pl }, lvl, { 0 }, *(int16_t *)rec, lvl };
    return ((int (*)(int, uint64_t, int, uint64_t, uint64_t, int, void *))(b + GA_CALC_EVAL))(ver, *(uint64_t *)(t + 0x150) + off, (int)(size - off), (uint64_t)(uintptr_t)(b + GA_SKILL_CALC_TERM_CB), (uint64_t)(uintptr_t)(b + GA_SKILL_CALC_FN_TABLE), 9, &ctx);
}

static void skill_adds(uint8_t *b, uint64_t pl, uint64_t nd, int lvl, int *amn, int *amx) {
    int ver = *(uint8_t *)(pl + 0x1bd), sk = *(int16_t *)*(uint64_t *)nd; int (*f)(int, uint64_t, int, int, int);
    f = (int (*)(int, uint64_t, int, int, int))(b + GA_SKILL_PHYS_MIN); *amn = f(ver, pl, sk, lvl, 0) >> 8; f = (int (*)(int, uint64_t, int, int, int))(b + GA_SKILL_ELEM_MIN); *amn += f(ver, pl, sk, lvl, 1) >> 8;
    f = (int (*)(int, uint64_t, int, int, int))(b + GA_SKILL_PHYS_MAX); *amx = f(ver, pl, sk, lvl, 0) >> 8; f = (int (*)(int, uint64_t, int, int, int))(b + GA_SKILL_ELEM_MAX); *amx += f(ver, pl, sk, lvl, 1) >> 8;
}
static void cb_game(uint8_t *b, uint64_t pl, uint64_t nd, int lvl, uint64_t w, int *v) {
    v[IC_C_DEF] = ((int (*)(uint64_t))(b + GA_CHAR_DEFENSE))(pl);
    v[IC_C_BLK] = ((int (*)(uint64_t, int))(b + GA_CHAR_BLOCK))(pl, !cb_classic(b));
    int ar = 0, col = 0; if (nd) ((void (*)(uint64_t, int *, int *, int, uint64_t, uint64_t))(b + GA_CHAR_ATTACK_RATING))(pl, &ar, &col, lvl, nd, w); v[IC_C_AR] = ar;
    int mn = 0, mx = 0; col = 0;
    v[IC_C_N + 1] = v[IC_C_N + 2] = 0;


    if (nd) { int x1 = skill_calc(b, pl, nd, lvl, 0), x2 = skill_calc(b, pl, nd, lvl, 1), a1, a2;
              ((void (*)(uint64_t, int *, int *, int *, int, int, uint64_t, int, uint64_t))(b + GA_CHAR_DAMAGE_BASE))(pl, &mn, &mx, &col, x1, x2, nd, 128, w);
              v[IC_C_N + 1] = mn; v[IC_C_N + 2] = mx;
              ((void (*)(uint64_t, int *, int *, int *, uint64_t))(b + GA_CHAR_DAMAGE_ADD))(pl, &mn, &mx, &col, nd);
              skill_adds(b, pl, nd, lvl, &a1, &a2); mn += a1; mx += a2; if (!(mn < mx)) mx = mn + 1; }
    v[IC_C_DMG] = mn; v[IC_C_N] = mx;
}
static const int k_cb_g[] = { 0, 2, 12, 17, 18, 19, 20, 21, 22, 23, 24, 25, 31, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 101, 182, 185, 186, 325, 326, 329, 330, 331, 332 };
#define CB_NG (int)(sizeof k_cb_g / sizeof *k_cb_g)
static void cb_fill(uint8_t *b, uint64_t pl, const int *g, const int *lv, int cls, int canblk, uint64_t w, int mode, uint64_t nd, int alvl, dm_ic_cb *c) {
    #define GV(n) ({ int r_ = 0; for (int i_ = 0; i_ < CB_NG; i_++) if (k_cb_g[i_] == (n)) r_ = g[i_]; r_; })
    memset(c, 0, sizeof *c); c->str = GV(0); c->dex = GV(2); c->lvl = GV(12);
    uint8_t ver = *(uint8_t *)(pl + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0;
    if (t && (uint64_t)cls < *(uint64_t *)(t + 0x1280)) { uint64_t r = *(uint64_t *)(t + 0x1278) + (uint64_t)cls * 0xd4; c->thf = *(int32_t *)(r + 0x38); c->bf = *(uint8_t *)(r + 0x45); }
    c->expansion = !cb_classic(b);
    c->ac = GV(31); c->acpct = lv[0]; c->acovr = GV(182); c->toblock = GV(20); c->can_block = canblk; c->tohit = GV(19); c->tohitpct = lv[1];
    c->ar_mast = ((int (*)(uint64_t, uint64_t, uint64_t, int))(b + GA_SKILL_MASTERY))(pl, w, nd, 0); c->ar_skill = ((int (*)(uint64_t, int, int))(b + GA_SKILL_TOHIT))(pl, 0, alvl);
    uint64_t wr = item_rec(b, w); c->ar_zero = wr && *(uint16_t *)(wr + 0x12e) == 38;
    c->armed = w != 0; c->mode2 = mode == 2; c->mn[0] = GV(21); c->mx[0] = GV(22); c->mn[1] = GV(23); c->mx[1] = GV(24); c->bmn = GV(185); c->bmx = GV(186);
    c->srcdam = *(uint8_t *)(*(uint64_t *)nd + 0x24d); c->dpct = GV(25); c->dmast = ((int (*)(uint64_t, uint64_t, uint64_t, int))(b + GA_SKILL_MASTERY))(pl, w, nd, 1);
    if (wr) { c->strb = *(int16_t *)(wr + 0x116); c->dexb = *(int16_t *)(wr + 0x118); }
    c->edmin = GV(18); c->edmax = GV(17); c->flat = lstat_(b, w, 111);
    for (int i = 0; i < 11; i++) c->el[i] = GV(48 + i);
    for (int i = 0; i < 4; i++) c->mast[i] = GV(329 + i);
    c->plen_ovr = GV(101); c->plen = GV(59); c->pcnt = GV(326);
    c->xpct = skill_calc(b, pl, nd, alvl, 0); c->xflat = skill_calc(b, pl, nd, alvl, 1); skill_adds(b, pl, nd, alvl, &c->sadd_mn, &c->sadd_mx);
    if (*(uint8_t *)(*(uint64_t *)nd + 0x24) & 8) c->ar_prog = GV(325);
    #undef GV
}
static void cb_why(cb_res *R, unsigned m, const char *k) { dm_ic_veto_add(&R->v, m, k, 0); }
static unsigned cb_dep(int st);






static void cb_gather_scoped(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int sl, int omask, uint64_t r4n, uint64_t r5n, int dsk, int force, cb_res *R, int current_snapshot, const int *gdelta, int charm) {
    memset(R, 0, sizeof *R); int cls = (int)*(uint32_t *)(pl + 4); uint64_t inv = *(uint64_t *)(pl + 0x90), nd = skill_node(pl, 0);
    uint64_t hands_after[2]; dm_ic_hands_after(sl, omask, cand, r4n, r5n, hands_after); uint64_t r4a = hands_after[0], r5a = hands_after[1];
    int dn, da; uint64_t wn = hands_weapon(b, r4n, r5n, &dn), wa = hands_weapon(b, r4a, r5a, &da), wg = inv ? ((uint64_t (*)(uint64_t))(b + GA_INV_GET_WEAPON))(inv) : 0;
    int mn_ = hand_mode(b, pl, r4n, r5n), ma = hand_mode(b, pl, r4a, r5a), mg = inv ? ((int (*)(uint64_t, uint64_t, int))(b + GA_INV_HAND_MODE))(pl, inv, 0) : -1;
    int cbn = can_block(b, r4n, r5n), cba = can_block(b, r4a, r5a);

    unsigned cat = 0;
    for (int k = 0; k <= nold; k++) { uint64_t x = k ? olds[k - 1] : cand; if (!x) continue; int wpn = is_type(b, x, 45);
        if (gstat(b, x, 31, 0)) cat |= CBK_DEF31; if (gstat(b, x, 16, 0) || gstat(b, x, 171, 0) || gstat(b, x, 182, 0) || lstat_(b, x, 16) || lstat_(b, x, 171)) cat |= CBK_DEFPCT;
        if (gstat(b, x, 19, 0)) cat |= CBK_TOHIT; if (gstat(b, x, 119, 0) || lstat_(b, x, 119)) cat |= CBK_TOHITP; if (gstat(b, x, 20, 0)) cat |= CBK_BLOCK;
        if (gstat(b, x, 0, 0)) cat |= CBK_STR; if (gstat(b, x, 2, 0)) cat |= CBK_DEX;
        int dm = gstat(b, x, 21, 0) || gstat(b, x, 22, 0) || gstat(b, x, 23, 0) || gstat(b, x, 24, 0); if (dm) cat |= wpn ? CBK_WDMG : CBK_FDMG; if (gstat(b, x, 185, 0) || gstat(b, x, 186, 0)) cat |= CBK_FDMG;
        if (gstat(b, x, 17, 0) || gstat(b, x, 18, 0)) cat |= CBK_WED; if (gstat(b, x, 25, 0)) cat |= CBK_ED; if (gstat(b, x, 111, 0) || lstat_(b, x, 111)) cat |= CBK_W111;
        for (int s = 48; s <= 58; s++) if (gstat(b, x, s, 0)) cat |= CBK_ELEM;
        if (gstat(b, x, 59, 0) || gstat(b, x, 101, 0) || gstat(b, x, 326, 0)) cat |= CBK_ELEM; }
    if (cbn != cba) cat |= CBK_SHIELD; if (wn != wa) cat |= CBK_WCHG; if (mn_ != ma) cat |= CBK_MODE;
    R->cat = cat; for (int m = 0; m < IC_C_N; m++) if (cat & k_cb_mask[m]) R->affected |= 1u << m;
    if (!R->affected && !force) return;

    int ctx = charm ? 0 : dm_ic_slot_group(sl, is_type(b, cand ? cand : olds[0], 45));
    uint64_t war = item_rec(b, wa); int wt = !wa ? 127 : war ? *(int16_t *)(war + 0x12e) : -1;
    for (int m = 0; m < IC_C_N; m++)
        R->cell[m] = dm_ic_cell_key(cls, m, ctx, charm, ma, wt, cat & k_cb_mask[m], 0, 0);
    const unsigned ARD = 1u << IC_C_AR | 1u << IC_C_DMG, ALL = (1u << IC_C_N) - 1;
    if (!nd) { cb_why(R, ALL, "dmCmpLive"); return; }
    if (dn || da) cb_why(R, ARD, "dmCmpDualWield");
    if (wn != wg || mn_ != mg) { R->live |= 1; cb_why(R, ARD, "dmCmpLive"); }


    int ranged[2] = { 0, 0 };
    for (int k = 0; k < 4; k++) { uint64_t x = k == 0 ? r4n : k == 1 ? r5n : k == 2 ? r4a : r5a; if (!x) continue;
        if (hand_off(x)) { R->live |= 2; cb_why(R, ALL, "dmCmpLive"); }
        if (k >= 2) ranged[k - 2] = is_type(b, x, 47) || is_type(b, x, 48); }
    if (dm_ic_ranged_unproven(ranged, current_snapshot)) { R->live |= 16; cb_why(R, ARD, "dmCmpUnproven"); }




    if (((int (*)(uint64_t))(b + GA_CHAR_DMG_ALT_PATH))(pl) & 1) { R->live |= 4; dm_ic_veto_add(&R->v, 1u << IC_C_DMG, "dmCmpUnproven", 1); }
    if (*(uint8_t *)(*(uint64_t *)nd + 0x24d) != 128) cb_why(R, 1u << IC_C_DMG, "dmCmpUnproven");


    if (*(uint8_t *)(*(uint64_t *)nd + 0x24) & 8) R->live |= 8;
    int g[CB_NG], ga[CB_NG], lv[2], la[2]; unsigned setm = 0;
    for (int i = 0; i < CB_NG; i++) { int s = k_cb_g[i]; g[i] = gstat(b, pl, s, 0); ga[i] = g[i] + gstat(b, cand, s, 0); for (int k = 0; k < nold; k++) ga[i] -= gstat(b, olds[k], s, 0);
        if (gdelta) { if (ga[i] != g[i] + gdelta[s]) setm |= cb_dep(s); ga[i] = g[i] + gdelta[s]; } }
    lv[0] = lstat_(b, pl, 171) + lstat_(b, pl, 16); lv[1] = lstat_(b, pl, 119);
    la[0] = lv[0] + lstat_(b, cand, 171) + lstat_(b, cand, 16); la[1] = lv[1] + lstat_(b, cand, 119);
    for (int k = 0; k < nold; k++) { la[0] -= lstat_(b, olds[k], 171) + lstat_(b, olds[k], 16); la[1] -= lstat_(b, olds[k], 119); }
    if (gdelta) { int a0 = lv[0] + gdelta[171] + gdelta[16], a1 = lv[1] + gdelta[119]; if (a0 != la[0]) setm |= cb_dep(171); if (a1 != la[1]) setm |= cb_dep(119); la[0] = a0; la[1] = a1; }
    if (!cbn && !cba) setm &= ~(1u << IC_C_BLK);
    if (setm) R->cat |= CBK_SET;
    for (int m = 0; m < IC_C_N; m++) if (setm >> m & 1) { R->affected |= 1u << m; if (R->cell[m]) R->cell[m] = dm_ic_cell_key(cls, m, ctx, charm, ma, wt, cat & k_cb_mask[m], 0, 1); }
    int alvl = *(int32_t *)(nd + 0x38) + ((int (*)(uint64_t, uint64_t, int))(b + GA_SKILL_BONUS_LEVEL))(pl, nd, 0);
    cb_fill(b, pl, g, lv, cls, cbn, wn, mn_, nd, alvl, &R->now); cb_fill(b, pl, ga, la, cls, cba, wa, ma, nd, alvl + dsk, &R->aft);
    cb_game(b, pl, nd, alvl, wg, R->gamev);




    if (wn != wa && (R->now.ar_mast || R->now.dmast || R->aft.ar_mast || R->aft.dmast)) cb_why(R, ARD, "dmCmpPassive");
    if (!dm_ic_cb_eval(&R->now, R->nowv) || !dm_ic_cb_eval(&R->aft, R->aftv)) cb_why(R, ALL, "dmCmpUnproven");
    for (int m = 0; m < IC_C_N; m++) { int same = R->nowv[m] == R->gamev[m] && (m != IC_C_DMG || R->nowv[IC_C_N] == R->gamev[IC_C_N]); R->r0[m] = same; if (!same) cb_why(R, 1u << m, "dmCmpLive"); }
    if (R->cell[IC_C_BLK] && (dm_ic_cb_new_cap(&R->now, &R->aft) >> IC_C_BLK & 1)) R->cell[IC_C_BLK] = dm_ic_cell_key(cls, IC_C_BLK, ctx, charm, ma, wt, cat & k_cb_mask[IC_C_BLK], 1, (int)(setm >> IC_C_BLK & 1));
    for (int m = 0; m < IC_C_N; m++) if (!cb_proven(R->cell[m])) R->unproven |= 1u << m;
}
static void cb_gather(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int sl, int omask, uint64_t r4n, uint64_t r5n, int dsk, int force, cb_res *R) {
    cb_gather_scoped(b,pl,cand,olds,nold,sl,omask,r4n,r5n,dsk,force,R,0,NULL,0);
}
static void cb_snapshot(uint8_t *b,uint64_t pl,uint64_t r4,uint64_t r5,cb_res *R) {

    cb_gather_scoped(b,pl,0,(uint64_t[2]){0,0},0,0,0,r4,r5,0,1,R,1,NULL,0);
}

static unsigned cb_dep(int st) {
    const unsigned D_ = 1u << IC_C_DEF, B_ = 1u << IC_C_BLK, A_ = 1u << IC_C_AR, M_ = 1u << IC_C_DMG;
    switch (st) { case 2: return D_ | B_ | A_ | M_; case 0: return M_; case 12: case 20: return B_; case 31: case 16: case 171: case 182: return D_; case 19: case 119: case 325: return A_;
    case 17: case 18: case 21: case 22: case 23: case 24: case 25: case 59: case 101: case 111: case 185: case 186: case 326: return M_;
    default: return (st >= 48 && st <= 58) || (st >= 329 && st <= 332) ? M_ : 0; }
}
static int state101(uint64_t pl) { uint64_t l = *(uint64_t *)(pl + 0x88); return l && *(int32_t *)(l + 0x1c) < 0 && (*(uint8_t *)(l + 0xb3c) & 0x20); }

static void cb_game_now(uint8_t *b, uint64_t pl, int *v) {
    uint64_t inv = *(uint64_t *)(pl + 0x90), nd = skill_node(pl, 0); int lvl = nd ? *(int32_t *)(nd + 0x38) + ((int (*)(uint64_t, uint64_t, int))(b + GA_SKILL_BONUS_LEVEL))(pl, nd, 0) : 0;
    cb_game(b, pl, nd, lvl, inv ? ((uint64_t (*)(uint64_t))(b + GA_INV_GET_WEAPON))(inv) : 0, v); }


void dm_itemcmp_sheet(uint8_t *b, uint64_t pl, int *def, int *block, int *ar) {
    int v[IC_C_N + 3] = { 0 }; cb_game_now(b, pl, v);
    *def = v[IC_C_DEF]; *block = v[IC_C_BLK]; *ar = skill_node(pl, 0) ? v[IC_C_AR] : -1; }






unsigned dm_itemcmp_shrine_values(uint8_t *b, uint64_t pl, int d171, int d19, int d25, int *out) {
    memset(out, 0, 8 * sizeof *out);
    if (!b || !pl) return 0;
    uint64_t r4 = 0, r5 = 0, olds[2] = { 0, 0 };
    for (uint64_t q = first_item(pl); q; q = next_item(q)) { uint64_t d = *(uint64_t *)(q + 0x10); if (d && *(uint32_t *)(q + 0xc) == 1) { int sl = *(uint8_t *)(d + 0x54); if (sl == 4) r4 = q; else if (sl == 5) r5 = q; } }
    static cb_res R; cb_gather(b, pl, 0, olds, 0, 0, 0, r4, r5, 0, 1, &R);
    int bf[IC_C_N + 1], af[IC_C_N + 1];
    unsigned ok = dm_ic_cb_without(&R.now, R.gamev, d171, d19, d25, bf, af) & ~R.v.bad;
    out[0] = bf[IC_C_DEF]; out[1] = af[IC_C_DEF]; out[2] = bf[IC_C_AR]; out[3] = af[IC_C_AR]; out[4] = bf[IC_C_DMG]; out[5] = bf[IC_C_N]; out[6] = af[IC_C_DMG]; out[7] = af[IC_C_N];
    return ok;
}

static int bench_coeffs(int cls, int *lpv, int *mpm, int *pen, uint8_t *b);

























enum { B_OFF, B_IDENT, B_DRESS, B_PICK, B_SENT, B_CONF, B_RSTOW, B_RHELD, B_ROLD, B_ROLDW, B_RUNC, B_VERIFY, B_SWAP, B_DRESS2, B_REQW, B_FAILREST, B_HOLD, B_DONE };
static struct { int st, ncand, ci, stable, nexact, nmis, nskip, nrest; uint32_t cand[24], old, cid, wait_id; int slot, dressed; uint64_t t0, hash0, hash; int worn0[16]; int tot0[NMS], pred[NMS]; int vit_d, ene_d; int nsk, skid[64], sklv[64], skpred[64]; dm_ic_in in; int in_ok, ndisp_ok, ndisp_bad;
                uint32_t olds[2]; int oslot[2], nold, place, ri, unc_tried, set2, swap_to, swap_hands[2], nhand;
                int reqm, base0, base_ok, req_want, nrq, rq_closed, rq_slot[8], rq_stat[8], rq_oc[8], vs0[16], vd0[16], nreq_ok, nreq_bad; uint32_t adj_for;
                cb_res cb; int ncb_ok, ncb_bad, ncb_none; } V;
typedef struct { uint64_t it; int slot, stat, req, oc; } req_flip;
static void req_table(uint8_t *b, uint64_t pl, uint64_t it, int *rs, int *rd);
static int req_scan(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *wit, int omask, int sl, int tstr, int tdex, int dstr, int ddex, req_flip *out, int max, int *closed);






static _Atomic int g_req_base = -1;
static int base_stat(uint64_t u, int st) {
    uint64_t sl = u ? *(uint64_t *)(u + 0x88) : 0, arr = sl ? *(uint64_t *)(sl + 0x30) : 0, cnt = sl ? *(uint64_t *)(sl + 0x38) : 0;
    for (uint64_t i = 0; arr && i < cnt && i < 512; i++) if (*(uint32_t *)(arr + 8 * i) == (uint32_t)st << 16) return *(int32_t *)(arr + 8 * i + 4);
    return 0;
}
static int place_code(uint8_t *b, uint64_t pl, int sl, uint64_t it) { return ((int (*)(uint64_t, int, uint64_t, int, int))(b + GA_ITEM_PLACE_CHECK))(pl, sl, it, 1, 0); }
static int two_handed(uint8_t *b, uint64_t it) { return it ? ((int (*)(uint64_t))(b + GA_ITEM_IS_2HANDED))(it) != 0 : 0; }
static void worn_map(uint8_t *b, uint64_t pl, int *m) { (void)b; memset(m, 0, 16 * sizeof *m); for (uint64_t q = first_item(pl); q; q = next_item(q)) { uint64_t d = *(uint64_t *)(q + 0x10); if (d && *(uint32_t *)(q + 0xc) == 1 && *(uint8_t *)(d + 0x54) < 16) m[*(uint8_t *)(d + 0x54)] = (int)*(uint32_t *)(q + 8); } }
static uint64_t totals(uint8_t *b, uint64_t pl, int *t) { uint64_t h = 1469598103934665603ull; for (int i = 0; i < NMS; i++) { t[i] = gstat(b, pl, k_ms[i].st, ms_layer(pl, k_ms[i].st)); h = (h ^ (uint32_t)t[i]) * 1099511628211ull; } return h; }

static int skill_levels(uint8_t *b, uint64_t pl, int *id, int *lv, int max) {
    int n = 0; uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0;
    for (int k = 0; nd && k < 1024 && n < max; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (!r || *(int32_t *)(nd + 0x44) != -1 || *(int32_t *)(nd + 0x38) <= 0) continue;
        id[n] = *(int16_t *)r; lv[n] = *(int32_t *)(nd + 0x38) + ((int (*)(uint64_t, uint64_t, int))(b + GA_SKILL_BONUS_LEVEL))(pl, nd, 0); n++; }
    return n;
}
static void skill_stats(uint8_t *b, const char *who, uint64_t it, uint64_t pl, const int *id, int n) {
    if (!it) return; static char o[600]; int cls = (int)*(uint32_t *)(pl + 4), k = snprintf(o, sizeof o, " 127=%d 83=%d 188=%d/%d/%d", gstat(b, it, 127, 0), gstat(b, it, 83, cls), gstat(b, it, 188, cls * 8), gstat(b, it, 188, cls * 8 + 1), gstat(b, it, 188, cls * 8 + 2));
    for (int i = 0; i < n && k < (int)sizeof o - 40; i++) { int a = gstat(b, it, 107, id[i]), c = gstat(b, it, 97, id[i]); if (a || c) k += snprintf(o + k, sizeof o - k, " skill %d: 107=%d 97=%d", id[i], a, c); }
    LOG("[cmp]   技能加成 %s:%s", who, o);
}
static int pick_slot(uint8_t *b, uint64_t pl, uint64_t it, int *sl) { *sl = 0; return ((int (*)(uint64_t, uint64_t, int *, int, int))(b + GA_EQUIP_SLOT_PICK))(pl, it, sl, 1, 0) && *sl > 0 && *sl < 16; }
static int may_place(uint8_t *b, uint64_t pl, int sl, uint64_t it) { return ((int (*)(uint64_t, int, uint64_t, int, int))(b + GA_ITEM_PLACE_CHECK))(pl, sl, it, 1, 0) != 0; }
static int inv_item(uint64_t it) { uint64_t d = *(uint64_t *)(it + 0x10); return d && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0; }
static int merc_open(uint8_t *b) { uint64_t root = *(uint64_t *)(b + GA_UI_ROOT); (void)root; int dm_ui_panel_shown(const char *); return dm_ui_panel_shown("HirelingInventoryPanel"); }
#define BENCH_FAIL(log_stmt) do { log_stmt; V.st = B_DONE; if (V.reqm && V.base_ok) { atomic_store(&g_req_base, V.base0); V.st = B_FAILREST; V.t0 = now_ns(); LOG("[cmp] 验算台：停台，基础力量设回 %d", V.base0); } } while (0)
static void collect(uint8_t *b, uint64_t pl, int hands_only) {
    V.ncand = V.ci = 0;
    for (uint64_t q = first_item(pl); q && V.ncand < 24; q = next_item(q)) { int sl; if (inv_item(q) && pick_slot(b, pl, q, &sl) && may_place(b, pl, sl, q) && (!hands_only || sl == 4 || sl == 5)) V.cand[V.ncand++] = *(uint32_t *)(q + 8); }
    LOG("[cmp] 验算台：背包里能穿的 %d 件（武器 %s 组）；难度 %u 职业 %u", V.ncand, V.set2 ? "II" : "I", *(uint8_t *)(b + GA_DIFFICULTY), *(uint32_t *)(pl + 4)); dump_getter(b, "player", pl, pl); dump_list(b, "player", pl);
}
static int dress_one(uint8_t *b, uint64_t pl, int hands_only, int defer_2h, uint64_t now) {
    int m[16]; worn_map(b, pl, m); static int prep, prep1; static unsigned sockdone;
    if (!hands_only && env_int("DM_TEST_ITEMCMP_DRESS", 0) == 2)
        for (uint64_t q = first_item(pl); q; q = next_item(q)) { int sl, pc; if (!inv_item(q) || !first_item(q) || !pick_slot(b, pl, q, &sl) || (sockdone >> sl & 1) || !((pc = place_code(b, pl, sl, q)) == 1 || pc == 5)) continue;
            sockdone |= 1u << sl; LOG("[cmp] 验算台：先换上镶了东西的 id %u 到栏位 %d（放置判定 %d）", *(uint32_t *)(q + 8), sl, pc); V.wait_id = *(uint32_t *)(q + 8); V.t0 = now; dm_item_equip(V.wait_id); return 1; }
    if (!hands_only && V.reqm) { static unsigned strdone;
        for (uint64_t q = first_item(pl); q; q = next_item(q)) { int sl, pc; if (!inv_item(q) || gstat(b, q, 0, 0) <= 0 || !pick_slot(b, pl, q, &sl) || (strdone >> sl & 1) || !((pc = place_code(b, pl, sl, q)) == 1 || pc == 5)) continue;
            uint64_t w = m[sl] ? item_by_id(b, (uint32_t)m[sl]) : 0; if (w && gstat(b, w, 0, 0) > 0) continue;
            strdone |= 1u << sl; LOG("[cmp] 验算台：先换上带力量的 id %u 到栏位 %d（放置判定 %d）", *(uint32_t *)(q + 8), sl, pc); V.wait_id = *(uint32_t *)(q + 8); V.t0 = now; dm_item_equip(V.wait_id); return 1; } }
    if (!hands_only && defer_2h == 1 && !prep1) { prep1 = 1;
        uint64_t l = m[5] ? item_by_id(b, (uint32_t)m[5]) : 0; int has = 0; for (int i = 0; i < IC_S_N && l; i++) has |= gstat(b, l, dm_ic_stat_id[i], 0) != 0;
        if (l && !has) for (uint64_t q = first_item(pl); q; q = next_item(q)) { int sl, nz = 0; for (int i = 0; i < IC_S_N; i++) nz |= gstat(b, q, dm_ic_stat_id[i], 0) != 0;
            if (!nz || !inv_item(q) || !pick_slot(b, pl, q, &sl) || sl != 5 || place_code(b, pl, sl, q) != 5) continue;
            LOG("[cmp] 验算台：I 组左手先换上带数值的 id %u", *(uint32_t *)(q + 8)); V.wait_id = *(uint32_t *)(q + 8); V.t0 = now; dm_item_equip(V.wait_id); return 1; } }
    if (hands_only && defer_2h == 2 && !prep) { prep = 1;
        uint64_t r = m[4] ? item_by_id(b, (uint32_t)m[4]) : 0;
        if (!two_handed(b, r)) for (uint64_t q = first_item(pl); q; q = next_item(q)) { int sl, pc; if (!inv_item(q) || !two_handed(b, q) || !pick_slot(b, pl, q, &sl) || sl != 4 || !((pc = place_code(b, pl, sl, q)) == 1 || pc == 2 || pc == 5 || pc == 7)) continue;
            LOG("[cmp] 验算台：II 组先换上双手武器 id %u（放置判定 %d）", *(uint32_t *)(q + 8), pc); V.wait_id = *(uint32_t *)(q + 8); V.t0 = now; dm_item_equip(V.wait_id); return 1; } }
    for (int pass = V.reqm ? -1 : 0; pass < 2; pass++)
        for (uint64_t q = first_item(pl); q; q = next_item(q)) { int sl; if (!inv_item(q) || !pick_slot(b, pl, q, &sl) || m[sl] || place_code(b, pl, sl, q) != 1 || (hands_only && sl != 4 && sl != 5)) continue;
            if (pass < 0 && gstat(b, q, 0, 0) <= 0) continue;
            if (!pass && defer_2h && sl == 4 && two_handed(b, q) != (defer_2h == 2)) continue;
            V.wait_id = *(uint32_t *)(q + 8); V.t0 = now; dm_item_equip(V.wait_id); return 1; }
    return 0;
}
static int skill_pred(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int cls, int id, int *now, int *si_out, int *own);
#ifndef DM_PLAYER_BUILD
static void sim_bench_predict(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int sl);
static void sim_bench_check(uint8_t *b, uint64_t pl);
#endif
static int bench_gear_predict(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int sl, uint32_t cid);
static int bench_gear_delta(int st, int ly);
static void bench_gear_deltas(int *d);
static int bench_gear_check(uint8_t *b, uint64_t pl, uint32_t cid);
static int g_ngear_ok, g_ngear_bad;
static void bench_pump(uint8_t *b, uint64_t pl) {
    static int at = -2, dress, sets; if (at == -2) { at = env_int("DM_TEST_ITEMCMP_VERIFY", -1); dress = env_int("DM_TEST_ITEMCMP_DRESS", 0); sets = env_int("DM_TEST_ITEMCMP_SETS", 0); V.reqm = env_int("DM_TEST_ITEMCMP_REQ", 0); }
    static uint64_t start; if (pl && !start) start = now_ns(); uint64_t now = now_ns();
    { static int clr = -2; static uint64_t last; if (clr == -2) clr = env_int("DM_TEST_ITEMCMP_CLEAR", -1);
      if (clr >= 0 && pl && now - start >= (uint64_t)clr * 1000000000ull && now - last > 400000000ull) { last = now; uint32_t id = 0;
          for (uint64_t q = first_item(pl); q && !id; q = next_item(q)) if (inv_item(q) && ((int (*)(uint64_t, int))(b + GA_ITEM_IS_TYPE))(q, 9)) id = *(uint32_t *)(q + 8);
          if (id) dm_item_use(id); else { clr = -1; LOG("[cmp] 验算台：背包里的药水喝完了"); } } }
    if (at < 0 || !pl || V.st == B_DONE) return;
    static int stow_sent; void (*swap0)(int) = (void (*)(int))(b + GA_WEAPON_SWAP); int dm_swapskill_wset(void);
    switch (V.st) {
    case B_OFF: if (now - start < (uint64_t)at * 1000000000ull) return; V.st = B_IDENT; V.wait_id = 0; LOG("[cmp] 验算台：开始（%s%s）", dress ? "先穿上" : "不先穿", sets ? "，含武器 II 组" : ""); return;
    case B_IDENT: {
        if (V.wait_id) { uint64_t it = item_by_id(b, V.wait_id), d = it ? *(uint64_t *)(it + 0x10) : 0; if (d && (*(uint32_t *)(d + 0x18) & 0x10)) V.wait_id = 0; else if (now - V.t0 < 2000000000ull) return; else { LOG("[cmp] 验算台：id %u 没鉴定成，不管它", V.wait_id); V.wait_id = 0; } }
        for (uint64_t q = first_item(pl); q; q = next_item(q)) { uint64_t d = *(uint64_t *)(q + 0x10); if (inv_item(q) && !(*(uint32_t *)(d + 0x18) & 0x10)) { static uint32_t tried[32]; static int nt; int seen = 0; for (int i = 0; i < nt; i++) seen |= tried[i] == *(uint32_t *)(q + 8); if (seen || nt >= 32) continue;
                tried[nt++] = V.wait_id = *(uint32_t *)(q + 8); V.t0 = now; void dm_item_identify_free(uint32_t); dm_item_identify_free(V.wait_id); return; } }
        V.wait_id = 0; if (dress) { V.st = B_DRESS; return; } V.st = B_PICK; collect(b, pl, 0); return; }
    case B_DRESS: case B_DRESS2: {
        if (V.wait_id) { uint64_t it = item_by_id(b, V.wait_id); if (it && *(uint32_t *)(it + 0xc) == 1) { LOG("[cmp] 验算台：先穿上 id %u（第 %d 件，武器 %s 组）", V.wait_id, ++V.dressed, V.set2 ? "II" : "I"); V.wait_id = 0; } else if (now - V.t0 < 3000000000ull) return; else { LOG("[cmp] 验算台：先穿 id %u 没成", V.wait_id); V.wait_id = 0; } }
        if (dress_one(b, pl, V.st == B_DRESS2, sets ? (V.st == B_DRESS2 ? 2 : 1) : 0, now)) return;
        LOG("[cmp] 验算台：先穿完，%d 件（武器 %s 组）", V.dressed, V.set2 ? "II" : "I"); if (V.set2) V.set2 = 2; V.st = B_PICK; collect(b, pl, V.set2 != 0); return; }
    case B_PICK: {
        if (V.ci >= V.ncand) {
            if (V.reqm && V.base_ok && base_stat(pl, 0) != V.base0) { V.req_want = V.base0; atomic_store(&g_req_base, V.base0); V.st = B_REQW; V.t0 = now; V.stable = 0; LOG("[cmp] 验算台：基础力量设回 %d", V.base0); return; }
            if (sets && (V.set2 == 0 || V.set2 == 2)) { int w = dm_swapskill_wset(); if (w < 0) { BENCH_FAIL(LOG("[cmp] 【红】验算台：当前武器组不知道，不换")); return; }
                worn_map(b, pl, V.worn0); V.swap_to = !w; V.set2++; swap0(w); V.st = B_SWAP; V.t0 = now; LOG("[cmp] 验算台：换武器 %d → %d（换前 栏位 4/5 = %d/%d，11/12 = %d/%d）", w, V.swap_to, V.worn0[4], V.worn0[5], V.worn0[11], V.worn0[12]); return; }
            V.st = B_DONE; LOG("[cmp] 验算台：完 — 原始合计准 %d、不准 %d；显示值全准 %d、有不准 %d；需求全准 %d、有不准 %d；战斗值全准 %d、有不准 %d、全不给数 %d；跳过 %d、换回 %d 次（逐项见上）；套装合成全准 %d、有不准 %d", V.nexact, V.nmis, V.ndisp_ok, V.ndisp_bad, V.nreq_ok, V.nreq_bad, V.ncb_ok, V.ncb_bad, V.ncb_none, V.nskip, V.nrest, g_ngear_ok, g_ngear_bad); return; }
        if (held_item(pl)) { BENCH_FAIL(LOG("[cmp] 【红】验算台：手上有东西，中止")); return; }
        if (merc_open(b)) { BENCH_FAIL(LOG("[cmp] 【红】验算台：雇佣兵面板开着，中止")); return; }
        V.cid = V.cand[V.ci++]; uint64_t it = item_by_id(b, V.cid); int sl;
        if (!it || !inv_item(it) || !pick_slot(b, pl, it, &sl) || !may_place(b, pl, sl, it)) { LOG("[cmp] 验算台：id %u 此刻不能穿，跳过", V.cid); V.nskip++; return; }
        V.slot = sl; worn_map(b, pl, V.worn0); V.hash0 = totals(b, pl, V.tot0);
        int ot = dm_ic_other_hand(sl); V.place = place_code(b, pl, sl, it);
        int dmk = dm_ic_displaced(V.place, sl, V.worn0[sl] != 0, ot >= 0 && V.worn0[ot] != 0);
        if (dmk < 0) { LOG("[cmp] 验算台：id %u 放置判定 %d 与身上（栏位 %d = %d，另一只手 %d）对不上，跳过", V.cid, V.place, sl, V.worn0[sl], ot >= 0 ? V.worn0[ot] : -1); V.nskip++; return; }
        V.nold = 0; for (int s2 = 0; s2 < 16 && V.nold < 2; s2++) if (dmk >> s2 & 1) { V.olds[V.nold] = (uint32_t)V.worn0[s2]; V.oslot[V.nold++] = s2; }
        V.old = V.nold ? V.olds[0] : 0;
        uint64_t ob[2] = { V.nold > 0 ? item_by_id(b, V.olds[0]) : 0, V.nold > 1 ? item_by_id(b, V.olds[1]) : 0 };
        if (V.reqm && V.adj_for != V.cid) {
            if (!V.base_ok) { V.base0 = base_stat(pl, 0); V.base_ok = 1; LOG("[cmp] 验算台：需求翻转模式，原基础力量 %d", V.base0); }
            int ds = gstat(b, it, 0, 0) - gstat(b, ob[0], 0, 0) - gstat(b, ob[1], 0, 0), R = 0, bs = base_stat(pl, 0), want = V.base0, rs, rd;
            if (ds) for (int s2 = 1; s2 <= 10; s2++) { int out_ = s2 == sl; for (int k = 0; k < V.nold; k++) out_ |= V.oslot[k] == s2; uint64_t w = V.worn0[s2] && !out_ ? item_by_id(b, (uint32_t)V.worn0[s2]) : 0; if (!w) continue; req_table(b, pl, w, &rs, &rd); if (rs > R) R = rs; }
            if (R > 0) want = bs + ((ds < 0 ? R : R - 1) - gstat(b, pl, 0, 0));
            V.adj_for = V.cid;
            if (want != bs) { V.req_want = want; atomic_store(&g_req_base, want); V.ci--; V.st = B_REQW; V.t0 = now; V.stable = 0; LOG("[cmp] 验算台：候选 id %u Δ力量 %d，最高力量需求 %d → 基础力量 %d → %d", V.cid, ds, R, bs, want); return; } }
        static char who[64]; snprintf(who, sizeof who, "bench cand %d", V.ci); dump_item(b, who, it, pl); for (int k = 0; k < V.nold; k++) dump_item(b, k ? "bench old 2" : "bench old", ob[k], pl);
        for (int i = 0; i < NMS; i++) { int L = ms_layer(pl, k_ms[i].st); V.pred[i] = V.tot0[i] + gstat(b, it, k_ms[i].st, L); for (int k = 0; k < V.nold; k++) V.pred[i] -= gstat(b, ob[k], k_ms[i].st, L); }
        V.vit_d = gstat(b, it, 3, 0) - gstat(b, ob[0], 3, 0) - gstat(b, ob[1], 3, 0); V.ene_d = gstat(b, it, 1, 0) - gstat(b, ob[0], 1, 0) - gstat(b, ob[1], 1, 0);
        { int cls = (int)*(uint32_t *)(pl + 4); memset(&V.in, 0, sizeof V.in); V.in_ok = bench_coeffs(cls, &V.in.lpv, &V.in.mpm, &V.in.penalty, b);
          for (int i = 0; i < IC_S_N; i++) { int id = dm_ic_stat_id[i], L = id == 83 ? cls : 0; V.in.tot[i] = gstat(b, pl, id, L); V.in.new_[i] = gstat(b, it, id, L); V.in.old_[i] = gstat(b, ob[0], id, L) + gstat(b, ob[1], id, L); }

          if (bench_gear_predict(b, pl, it, ob, V.nold, sl, V.cid)) { V.in.composed = 1;
              for (int i = 0; i < NMS; i++) V.pred[i] = V.tot0[i] + bench_gear_delta(k_ms[i].st, ms_layer(pl, k_ms[i].st));
              for (int i = 0; i < IC_S_N; i++) { int id = dm_ic_stat_id[i]; V.in.old_[i] = 0; V.in.new_[i] = bench_gear_delta(id, id == 83 ? cls : 0); } } }
        { uint64_t wi[16] = { 0 }; int om = 0; for (int s2 = 1; s2 < 16; s2++) if (V.worn0[s2]) wi[s2] = item_by_id(b, (uint32_t)V.worn0[s2]); for (int k = 0; k < V.nold; k++) om |= 1 << V.oslot[k];
          int dstr = V.in.new_[IC_S_STR] - V.in.old_[IC_S_STR], ddex = V.in.new_[IC_S_DEX] - V.in.old_[IC_S_DEX]; req_flip rf[8]; V.nrq = 0; V.rq_closed = 1;
          if (dstr || ddex) { V.nrq = req_scan(b, pl, it, wi, om, sl, V.in.tot[IC_S_STR], V.in.tot[IC_S_DEX], dstr, ddex, rf, 8, &V.rq_closed); if (V.nrq > 8) V.nrq = 8; }
          for (int k = 0; k < V.nrq; k++) { V.rq_slot[k] = rf[k].slot; V.rq_stat[k] = rf[k].stat; V.rq_oc[k] = rf[k].oc; LOG("[cmp] 验算台 需求预测 id %u：栏位 %d %s 需求 %d → %s（%s）", V.cid, rf[k].slot, rf[k].stat ? "敏捷" : "力量", rf[k].req,
              rf[k].oc == IC_REQ_GAIN ? "之后够" : rf[k].oc == IC_REQ_LOSS ? "之后不够" : "说不准", V.rq_closed ? "闭合" : "不闭合"); }
          for (int s2 = 1; s2 <= 10; s2++) { V.vs0[s2] = V.vd0[s2] = -1; if (!wi[s2]) continue; int so = 1, dx = 1, lv = 1; ((int (*)(uint64_t, uint64_t, int, int *, int *, int *, int))(b + GA_ITEM_REQS_MET))(wi[s2], pl, 1, &so, &dx, &lv, 0); V.vs0[s2] = so; V.vd0[s2] = dx; } }
        { static char sk[900]; int q = 0; uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0;
          for (int k = 0; nd && k < 1024 && q < (int)sizeof sk - 40; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (!r) continue; int bs = *(int32_t *)(nd + 0x38), bo = ((int (*)(uint64_t, uint64_t, int))(b + GA_SKILL_BONUS_LEVEL))(pl, nd, 0);
              if (bs > 0 || bo > 0 || *(int32_t *)(nd + 0x44) != -1) q += snprintf(sk + q, sizeof sk - q, " %d:%d+%d(%d)", *(int16_t *)r, bs, bo, *(int32_t *)(nd + 0x44)); }
          LOG("[cmp] 验算台 技能节点（号:基础+加成(来源物品)）:%s", q ? sk : " 无"); }
        { uint64_t r4 = V.worn0[4] ? item_by_id(b, (uint32_t)V.worn0[4]) : 0, r5 = V.worn0[5] ? item_by_id(b, (uint32_t)V.worn0[5]) : 0; int om = 0; for (int k = 0; k < V.nold; k++) om |= 1 << V.oslot[k];
          int dsk = gstat(b, it, 127, 0) - gstat(b, ob[0], 127, 0) - gstat(b, ob[1], 127, 0) + gstat(b, it, 107, 0) - gstat(b, ob[0], 107, 0) - gstat(b, ob[1], 107, 0);
          if (V.in.composed) { static int gd[512]; bench_gear_deltas(gd); cb_gather_scoped(b, pl, it, ob, V.nold, sl, om, r4, r5, dsk, 1, &V.cb, 0, gd, 0); } else cb_gather(b, pl, it, ob, V.nold, sl, om, r4, r5, dsk, 1, &V.cb); const cb_res *R = &V.cb;
          LOG("[cmp] 验算台 战斗预测 id %u：受影响 %#x 来源 %#x 不给数 %#x（判据 %#x）未核 %#x；现在 模型 %d/%d/%d/%d-%d 游戏 %d/%d/%d/%d-%d（物理 %d-%d；R0 %d%d%d%d）；之后 预测 %d/%d/%d/%d-%d；组合 %08llx %08llx %08llx %08llx",
              V.cid, R->affected, R->cat, R->v.bad, R->live, R->unproven, R->nowv[0], R->nowv[1], R->nowv[2], R->nowv[3], R->nowv[4], R->gamev[0], R->gamev[1], R->gamev[2], R->gamev[3], R->gamev[4], R->gamev[5], R->gamev[6],
              R->r0[0], R->r0[1], R->r0[2], R->r0[3], R->aftv[0], R->aftv[1], R->aftv[2], R->aftv[3], R->aftv[4], (unsigned long long)R->cell[0], (unsigned long long)R->cell[1], (unsigned long long)R->cell[2], (unsigned long long)R->cell[3]);
          { static char fl[600]; int q = 0; const int *pn = (const int *)&R->now; for (int i = 0; i < (int)(sizeof R->now / sizeof(int)) && q < (int)sizeof fl - 12; i++) q += snprintf(fl + q, sizeof fl - q, " %d", pn[i]);
            LOG("[cmp] 验算台 战斗输入 id %u 现在（字段次序同 dm_ic_cb）:%s", V.cid, fl); } }
        V.nsk = skill_levels(b, pl, V.skid, V.sklv, 64);
        { int cls = (int)*(uint32_t *)(pl + 4), nw, si, own; for (int j = 0; j < V.nsk; j++) { int r = skill_pred(b, pl, it, ob, V.nold, cls, V.skid[j], &nw, &si, &own); V.skpred[j] = !own || (r < 0 && r != -2) ? -1 : r; } }
#ifndef DM_PLAYER_BUILD
        sim_bench_predict(b, pl, it, ob, V.nold, sl);
#endif
        dm_item_equip(V.cid); V.st = B_SENT; V.t0 = now;
        LOG("[cmp] 验算台：穿 id %u 到栏位 %d（武器 %s 组；放置判定 %d；双手 候选 %d / 这格 %d / 另一只手 %d；换下 %u@%d %u@%d）", V.cid, sl, V.set2 ? "II" : "I", V.place, two_handed(b, it),
            two_handed(b, V.worn0[sl] ? item_by_id(b, (uint32_t)V.worn0[sl]) : 0), ot >= 0 && V.worn0[ot] ? two_handed(b, item_by_id(b, (uint32_t)V.worn0[ot])) : -1, V.nold > 0 ? V.olds[0] : 0, V.nold > 0 ? V.oslot[0] : 0, V.nold > 1 ? V.olds[1] : 0, V.nold > 1 ? V.oslot[1] : 0);
        return; }
    case B_SENT: case B_CONF: {
        uint64_t it = item_by_id(b, V.cid), d = it ? *(uint64_t *)(it + 0x10) : 0; int on = it && d && *(uint32_t *)(it + 0xc) == 1 && *(uint8_t *)(d + 0x54) == V.slot;
        for (int k = 0; k < V.nold && on; k++) { uint64_t o = item_by_id(b, V.olds[k]); if (!o || *(uint32_t *)(o + 0xc) == 1) on = 0; }
        if (!on) { if (now - V.t0 > 3000000000ull) { int m[16]; worn_map(b, pl, m); BENCH_FAIL(LOG("[cmp] 【红】验算台：3 秒内没按预测穿上（栏位 4/5 = %d/%d，%d = %d），中止", m[4], m[5], V.slot, m[V.slot])); } return; }
        int t[NMS]; uint64_t hh = totals(b, pl, t); if (V.st == B_SENT || hh != V.hash) { V.st = B_CONF; V.hash = hh; V.stable = 0; }
        if (++V.stable < 10) { if (now - V.t0 > 6000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：数值 6 秒内不稳定，中止")); return; }
        static char line[1400]; int k = 0, mis = 0;
        for (int i = 0; i < NMS; i++) { int ok = t[i] == V.pred[i]; if (t[i] == V.tot0[i] && ok) continue; mis += !ok; k += snprintf(line + k, sizeof line - k, " %s %d->%d pred %d %s;", k_ms[i].nm, V.tot0[i], t[i], V.pred[i], ok ? "exact" : "MISMATCH"); if (k > (int)sizeof line - 80) break; }
        { int id[64], lv[64], n = skill_levels(b, pl, id, lv, 64); static char sk[700]; int q = 0;
          for (int i = 0; i < n && q < (int)sizeof sk - 40; i++) { int was = -999; for (int j = 0; j < V.nsk; j++) if (V.skid[j] == id[i]) was = V.sklv[j]; if (was != lv[i]) q += snprintf(sk + q, sizeof sk - q, " %d:%d->%d", id[i], was, lv[i]); }
          LOG("[cmp] 验算台 技能 id %u：已学 %d 个，有变化的:%s", V.cid, n, q ? sk : " 无");
          static char sp[700]; int r = 0, ok = 0, bad = 0, nc = 0;
          for (int j = 0; j < V.nsk; j++) { int act = -999; for (int i = 0; i < n; i++) if (id[i] == V.skid[j]) act = lv[i]; if (act == -999) act = 0;
              if (V.skpred[j] == -1) { nc++; continue; } int pv = V.skpred[j] == -2 ? V.sklv[j] : V.skpred[j]; if (pv == act) { ok++; continue; } bad++;
              if (r < (int)sizeof sp - 40) r += snprintf(sp + r, sizeof sp - r, " %d:%d->%d pred %d;", V.skid[j], V.sklv[j], act, pv); }
          LOG("[cmp] 验算台 技能预测 id %u：%s 准 %d、不准 %d、不给数 %d%s", V.cid, bad ? "【红】有不准的" : "全准", ok, bad, nc, r ? sp : ""); }
        uint64_t h = held_item(pl); static char where[120]; int w = 0; where[0] = 0;
        for (int k2 = 0; k2 < V.nold; k2++) { uint64_t oi = item_by_id(b, V.olds[k2]); w += snprintf(where + w, sizeof where - w, "%s%u@%s", k2 ? ", " : "", V.olds[k2], h && *(uint32_t *)(h + 8) == V.olds[k2] ? "hand" : oi && inv_item(oi) ? "inv" : "other"); }
        LOG("[cmp] 验算台 结果 id %u 栏位 %d（放置判定 %d；Δ体力 %d Δ精力 %d；换下的件现在 %s）:%s", V.cid, V.slot, V.place, V.vit_d, V.ene_d, V.nold ? where : "（原来空位）", k ? line : " 没有变化");
        if (mis) V.nmis++; else V.nexact++;
        { int gb = bench_gear_check(b, pl, V.cid); if (gb == 0) g_ngear_ok++; else if (gb > 0) g_ngear_bad++; }
#ifndef DM_PLAYER_BUILD
        sim_bench_check(b, pl);
#endif
        { int m2[16]; worn_map(b, pl, m2); static char rl[600]; int q = 0, ok = 0, bad = 0;
#define RL(...) do { if (q < (int)sizeof rl - 1) { int w_ = snprintf(rl + q, sizeof rl - q, __VA_ARGS__); q += w_ > 0 ? w_ : 0; if (q > (int)sizeof rl - 1) q = (int)sizeof rl - 1; } } while (0)
          for (int s2 = 1; s2 <= 10; s2++) { uint64_t w = m2[s2] ? item_by_id(b, (uint32_t)m2[s2]) : 0; if (!w) continue; int out_ = 0; for (int k = 0; k < V.nold; k++) out_ |= V.oslot[k] == s2 && (uint32_t)m2[s2] != V.cid; if (out_) continue;
              int so = 1, dx = 1, lv = 1; ((int (*)(uint64_t, uint64_t, int, int *, int *, int *, int))(b + GA_ITEM_REQS_MET))(w, pl, 1, &so, &dx, &lv, 0);
              for (int st = 0; st < 2; st++) { int was = s2 == V.slot && (uint32_t)m2[s2] == V.cid ? 1 : st ? V.vd0[s2] : V.vs0[s2], is = st ? dx : so, want = was, pr = -1; if (was < 0) continue;
                  for (int k = 0; k < V.nrq; k++) if (V.rq_slot[k] == s2 && V.rq_stat[k] == (st ? 2 : 0)) pr = V.rq_oc[k];
                  if (pr == IC_REQ_AMBIG) { RL(" %d/%s %d->%d ambiguous;", s2, st ? "dex" : "str", was, is); continue; }
                  if (pr == IC_REQ_GAIN) want = 1; else if (pr == IC_REQ_LOSS) want = 0;
                  if (is == want) { ok++; if (pr >= 0 || is != was) RL(" %d/%s %d->%d exact;", s2, st ? "dex" : "str", was, is); }
                  else { bad++; RL(" %d/%s %d->%d pred %d MISMATCH;", s2, st ? "dex" : "str", was, is, want); } } }
          if (V.nrq && V.rq_closed) for (int st = 0; st < 2; st++) { int pv = V.in.tot[st ? IC_S_DEX : IC_S_STR] + V.in.new_[st ? IC_S_DEX : IC_S_STR] - V.in.old_[st ? IC_S_DEX : IC_S_STR], av = gstat(b, pl, st ? 2 : 0, 0);
              if (pv == av) { ok++; RL(" %s %d exact;", st ? "dex" : "str", av); } else { bad++; RL(" %s %d pred %d MISMATCH;", st ? "dex" : "str", av, pv); } }
          if (V.nrq || bad) { if (bad) V.nreq_bad++; else V.nreq_ok++; LOG("[cmp] 验算台 需求 id %u：%s（判定准 %d、不准 %d；预测翻转 %d 个，%s；力量 %d → %d 预测 %d）%s", V.cid, bad ? "【红】有不准的" : "全准", ok, bad, V.nrq, V.rq_closed ? "闭合" : "不闭合",
              V.in.tot[IC_S_STR], gstat(b, pl, 0, 0), V.in.tot[IC_S_STR] + V.in.new_[IC_S_STR] - V.in.old_[IC_S_STR], q ? rl : ""); }
#undef RL
        }
        if (V.in_ok && V.nrq) LOG("[cmp] 验算台 显示值 id %u：不给数（需求翻转，产品只写需求行）", V.cid);
        else if (V.in_ok) { int act[IC_S_N], prd[IC_S_N], cls = (int)*(uint32_t *)(pl + 4), bad = 0; static char dl[600]; int q = 0; dm_ic_after(&V.in, prd);
            for (int i = 0; i < IC_S_N; i++) act[i] = gstat(b, pl, dm_ic_stat_id[i], dm_ic_stat_id[i] == 83 ? cls : 0);
            for (int m = 0; m < IC_M_N; m++) { int pv = dm_ic_shown(prd, m, V.in.penalty), av = dm_ic_shown(act, m, V.in.penalty), bv = dm_ic_shown(V.in.tot, m, V.in.penalty);
                if (pv != av) { bad++; q += snprintf(dl + q, sizeof dl - q, " m%d %d->%d pred %d;", m, bv, av, pv); } else if (av != bv) q += snprintf(dl + q, sizeof dl - q, " m%d %d->%d ok;", m, bv, av); if (q > (int)sizeof dl - 60) break; }
            if (bad) V.ndisp_bad++; else V.ndisp_ok++;
            LOG("[cmp] 验算台 显示值 id %u：%s（产品算法，含体力 / 精力系数与难度惩罚；放置判定 %d，换下 %d 件）%s", V.cid, bad ? "【红】有不准的" : "全准", V.place, V.nold, q ? dl : " 没有变化"); }
        { int m3[16]; worn_map(b, pl, m3); uint64_t r4 = m3[4] ? item_by_id(b, (uint32_t)m3[4]) : 0, r5 = m3[5] ? item_by_id(b, (uint32_t)m3[5]) : 0, z2[2] = { 0, 0 }; static cb_res A;
          cb_gather(b, pl, 0, z2, 0, 0, 0, r4, r5, 0, 1, &A); const cb_res *R = &V.cb; static char cl[900]; int q = 0, ok = 0, bad = 0, nn = 0;
#define CL(...) do { if (q < (int)sizeof cl - 1) { int w_ = snprintf(cl + q, sizeof cl - q, __VA_ARGS__); q += w_ > 0 ? w_ : 0; if (q > (int)sizeof cl - 1) q = (int)sizeof cl - 1; } } while (0)
          static const char *nm[IC_C_N] = { "def", "blk", "ar", "dmg" };
          for (int m = 0; m < IC_C_N; m++) { int pv = R->aftv[m], px = m == IC_C_DMG ? R->aftv[IC_C_N] : 0, gv = A.gamev[m], gx = m == IC_C_DMG ? A.gamev[IC_C_N] : 0, bv = R->gamev[m], bx = m == IC_C_DMG ? R->gamev[IC_C_N] : 0;
              if (R->v.bad >> m & 1) { nn++; CL(" %s %d-%d->%d-%d none(%s);", nm[m], bv, bx, gv, gx, R->v.why[m] ? R->v.why[m] : "?"); continue; }
              if (pv == gv && px == gx) { ok++; CL(" %s %d-%d->%d-%d exact%s [%08llx];", nm[m], bv, bx, gv, gx, R->unproven >> m & 1 ? " unproven" : "", (unsigned long long)R->cell[m]); }
              else { bad++; CL(" %s %d-%d->%d-%d pred %d-%d MISMATCH [%08llx];", nm[m], bv, bx, gv, gx, pv, px, (unsigned long long)R->cell[m]); } }
          { const int *pa = (const int *)&R->aft, *aa = (const int *)&A.now; int nf = (int)(sizeof R->aft / sizeof(int)), nd = 0;
            for (int i = 0; i < nf; i++) if (pa[i] != aa[i]) { if (!nd++) CL(" inputs-differ:"); CL(" f%d %d/%d", i, pa[i], aa[i]); } }
#undef CL
          if (bad) V.ncb_bad++; else if (ok) V.ncb_ok++; else V.ncb_none++;
          LOG("[cmp] 验算台 战斗值 id %u：%s（准 %d、不准 %d、不给数 %d；换后 R0 %d%d%d%d）%s", V.cid, bad ? "【红】有不准的" : ok ? "全准" : "全不给数", ok, bad, nn, A.r0[0], A.r0[1], A.r0[2], A.r0[3], cl); }


        if (env_int("DM_TEST_ITEMCMP_HOLD", 0) > 0) { LOG("[cmp] 验算台：停 id %u（等拍角色页）", V.cid); V.st = B_HOLD; V.t0 = now; return; }
        V.ri = 0; V.unc_tried = 0; stow_sent = 0; V.st = B_RSTOW; V.t0 = now; return; }
    case B_HOLD: if (now - V.t0 < (uint64_t)env_int("DM_TEST_ITEMCMP_HOLD", 0) * 1000000ull) return;
        V.ri = 0; V.unc_tried = 0; stow_sent = 0; V.st = B_RSTOW; V.t0 = now; return;
    case B_RSTOW: {
        uint64_t h = held_item(pl);
        for (int k = 0; h && !stow_sent && k < V.nold; k++) if (*(uint32_t *)(h + 8) == V.olds[k]) {
            V.ri = k; dm_held_equip(V.oslot[k]); V.st = B_RHELD; V.t0 = now; return; }
        if (h) { if (!stow_sent) { stow_sent = 1; V.t0 = now; dm_held_stow(); } else if (now - V.t0 > 3000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：手上的 id %u 收不进背包，中止", *(uint32_t *)(h + 8))); return; }
        stow_sent = 0;
        if (V.ri < V.nold) { V.st = B_ROLD; return; }
        uint64_t it = item_by_id(b, V.cid); if (it && *(uint32_t *)(it + 0xc) == 1) { dm_item_unequip(V.cid); V.st = B_RUNC; V.t0 = now; return; }
        V.st = B_VERIFY; V.t0 = now; return; }
    case B_RHELD: {
        uint64_t oi = item_by_id(b, V.olds[V.ri]), od = oi ? *(uint64_t *)(oi + 0x10) : 0;
        if (oi && od && *(uint32_t *)(oi + 0xc) == 1 && *(uint8_t *)(od + 0x54) == V.oslot[V.ri]) { V.ri = 0; V.st = B_RSTOW; return; }
        if (now - V.t0 > 3000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：手上的旧件 id %u 3 秒内没放回栏位 %d，中止", V.olds[V.ri], V.oslot[V.ri])); return; }
    case B_ROLD: {
        uint64_t oi = item_by_id(b, V.olds[V.ri]), od = oi ? *(uint64_t *)(oi + 0x10) : 0; int sl2 = 0;
        if (oi && od && *(uint32_t *)(oi + 0xc) == 1 && *(uint8_t *)(od + 0x54) == V.oslot[V.ri]) { V.ri++; V.st = B_RSTOW; return; }
        if (!oi || !inv_item(oi)) { BENCH_FAIL(LOG("[cmp] 【红】验算台：换下的 id %u 既不在身上原位也不在背包，中止", V.olds[V.ri])); return; }
        if (pick_slot(b, pl, oi, &sl2) && sl2 == V.oslot[V.ri] && may_place(b, pl, sl2, oi)) { dm_item_equip(V.olds[V.ri]); V.st = B_ROLDW; V.t0 = now; return; }
        for (int k = 0; k < V.nold; k++) { uint64_t ok_ = item_by_id(b, V.olds[k]); int s3 = 0;
            if (k != V.ri && ok_ && inv_item(ok_) && pick_slot(b, pl, ok_, &s3) && s3 == V.oslot[k] && may_place(b, pl, s3, ok_)) { LOG("[cmp] 验算台：id %u 游戏给栏位 %d（要 %d），先放 id %u 回栏位 %d", V.olds[V.ri], sl2, V.oslot[V.ri], V.olds[k], s3); V.ri = k; dm_item_equip(V.olds[k]); V.st = B_ROLDW; V.t0 = now; return; } }
        uint64_t it = item_by_id(b, V.cid);
        if (!V.unc_tried && it && *(uint32_t *)(it + 0xc) == 1) { V.unc_tried = 1; LOG("[cmp] 验算台：游戏给 id %u 选栏位 %d（要 %d），先卸下候选再试", V.olds[V.ri], sl2, V.oslot[V.ri]); dm_item_unequip(V.cid); V.st = B_RUNC; V.t0 = now; return; }
        LOG("[cmp] 验算台：游戏给旧件 id %u 选栏位 %d，不是 %d", V.olds[V.ri], sl2, V.oslot[V.ri]); BENCH_FAIL(LOG("[cmp] 【红】验算台：换回的栏位不确定，中止")); return; }
    case B_ROLDW: {
        uint64_t oi = item_by_id(b, V.olds[V.ri]), od = oi ? *(uint64_t *)(oi + 0x10) : 0;
        if (oi && od && *(uint32_t *)(oi + 0xc) == 1 && *(uint8_t *)(od + 0x54) == V.oslot[V.ri]) { V.ri = 0; V.st = B_RSTOW; return; }
        if (now - V.t0 > 3000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：旧件 id %u 3 秒内没穿回栏位 %d，中止", V.olds[V.ri], V.oslot[V.ri])); return; }
    case B_RUNC: {
        uint64_t it = item_by_id(b, V.cid); if (it && *(uint32_t *)(it + 0xc) != 1) { V.st = B_RSTOW; return; }
        if (now - V.t0 > 3000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：候选卸不下（背包没地方？），中止")); return; }
    case B_VERIFY: {
        int m[16]; worn_map(b, pl, m); uint64_t h = held_item(pl); int t[NMS]; uint64_t hh = totals(b, pl, t);
        uint64_t ci = item_by_id(b, V.cid); int cand_home = ci && inv_item(ci);
        if (memcmp(m, V.worn0, sizeof m) || h || hh != V.hash0 || !cand_home) { if (now - V.t0 > 4000000000ull) { static char d[400]; int k = 0; for (int i = 1; i < 16; i++) if (m[i] != V.worn0[i]) k += snprintf(d + k, sizeof d - k, " slot %d: %d (want %d)", i, m[i], V.worn0[i]);
                    for (int i = 0; i < NMS && k < 300; i++) if (t[i] != V.tot0[i]) k += snprintf(d + k, sizeof d - k, " %s %d (want %d)", k_ms[i].nm, t[i], V.tot0[i]);
                    LOG("[cmp] 验算台：换回不对：手上 %u，候选 %s%s", h ? *(uint32_t *)(h + 8) : 0, cand_home ? "在背包" : "不在背包", d); BENCH_FAIL(LOG("[cmp] 【红】验算台：4 秒内没换回原样，中止")); } return; }
        V.nrest++; LOG("[cmp] 验算台：id %u 已换回，栏位表、手上、合计同穿之前", V.cid); V.st = B_PICK; return; }
    case B_SWAP: {
        int m[16]; worn_map(b, pl, m); int w = dm_swapskill_wset();
        if (w != V.swap_to || held_item(pl)) { if (now - V.t0 > 4000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：4 秒内没换成武器 %d 组（现在 %d），中止", V.swap_to, w)); return; }
        if (now - V.t0 < 500000000ull) return;
        int moved = m[4] == V.worn0[11] && m[5] == V.worn0[12] && m[11] == V.worn0[4] && m[12] == V.worn0[5];
        for (int i = 1; i < 16 && moved; i++) if (i != 4 && i != 5 && i != 11 && i != 12 && m[i] != V.worn0[i]) moved = 0;
        if (!moved) { if (now - V.t0 > 4000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：换了武器组但栏位 4/5 = %d/%d，11/12 = %d/%d 没与换前对调，中止", m[4], m[5], m[11], m[12])); return; }
        LOG("[cmp] 验算台：换好武器 %d 组：栏位 4/5 = %d/%d，11/12 = %d/%d（物品真挪了栏位：在用的总在 4/5）", w, m[4], m[5], m[11], m[12]);
        if (V.set2 == 1) { V.dressed = 0; V.wait_id = 0; if (dress) { V.st = B_DRESS2; return; } V.set2 = 2; V.st = B_PICK; collect(b, pl, 1); return; }
        V.st = B_DONE; LOG("[cmp] 验算台：完 — 原始合计准 %d、不准 %d；显示值全准 %d、有不准 %d；需求全准 %d、有不准 %d；战斗值全准 %d、有不准 %d、全不给数 %d；跳过 %d、换回 %d 次（逐项见上）；套装合成全准 %d、有不准 %d", V.nexact, V.nmis, V.ndisp_ok, V.ndisp_bad, V.nreq_ok, V.nreq_bad, V.ncb_ok, V.ncb_bad, V.ncb_none, V.nskip, V.nrest, g_ngear_ok, g_ngear_bad); return; }
    case B_FAILREST: {
        if (base_stat(pl, 0) == V.base0) { LOG("[cmp] 验算台：停台，基础力量已回到 %d", V.base0); V.st = B_DONE; return; }
        if (now - V.t0 > 4000000000ull) { LOG("[cmp] 【红】验算台：停台时基础力量 4 秒内没回到 %d（现在 %d）", V.base0, base_stat(pl, 0)); V.st = B_DONE; } return; }
    case B_REQW: {
        int t[NMS]; uint64_t hh = totals(b, pl, t);
        if (base_stat(pl, 0) != V.req_want) { if (now - V.t0 > 4000000000ull) BENCH_FAIL(LOG("[cmp] 【红】验算台：基础力量 4 秒内没变成 %d（现在 %d），中止", V.req_want, base_stat(pl, 0))); return; }
        if (now - V.t0 > 8000000000ull) { BENCH_FAIL(LOG("[cmp] 【红】验算台：改基础力量后合计 8 秒内不稳定，中止")); return; }
        if (hh != V.hash) { V.hash = hh; V.stable = 0; return; } if (++V.stable < 10) return;
        LOG("[cmp] 验算台：基础力量 %d，力量合计 %d", base_stat(pl, 0), gstat(b, pl, 0, 0)); V.st = B_PICK; return; }
    default: return; }
}















enum { PAD_SHORT = 15 };
static _Atomic int g_lock, g_held_own = -1;
static int own_ok(uint8_t *b) {
    int dm_runeword_list_open(void), dm_item_targeting(void); uint64_t root = *(uint64_t *)(b + GA_UI_ROOT), fm = root ? *(uint64_t *)(root + 0xd0) : 0, w = fm ? *(uint64_t *)(fm + 0x190) : 0;
    if (!w || *(uint64_t *)w - (uint64_t)(uintptr_t)b != GA_VT_ITEM_GRID || !*(uint8_t *)(w + 0x631)) return 0;
    if (!dm_ui_panel_shown("PlayerInventoryExpansionLayout") || dm_ui_panel_shown("HirelingInventoryPanel")) return 0;
    if (dm_ui_panel_state() & (DM_PS_BANK | DM_PS_VENDOR | DM_PS_IMBUE | DM_PS_CUBE | DM_PS_VAULT | DM_PS_DEAD)) return 0;
    uint64_t pl = player(b); return pl && !held_item(pl) && !dm_item_targeting() && !dm_runeword_list_open();
}
uint64_t dm_itemcmp_pad_filter(uint64_t bt) {
    static uint64_t prev, t0; static int down, owned, dead; const uint64_t KEY = DM_BTN_X; uint64_t dn = bt & ~prev, up = prev & ~bt; prev = bt;
    int dm_padmode_current(void), dm_padmode_busy(void); uint8_t *b = mb();
    if (!b || !dm_padmode_current() || dm_padmode_busy() || !dm_custom_get("item_cmp")) { if (atomic_exchange(&g_lock, 0)) LOG("物品对比：手柄锁解开（离开手柄模式 / 开关关了）"); down = owned = 0; atomic_store(&g_held_own, -1); return bt; }
    if (!dm_ui_panel_shown("PlayerInventoryExpansionLayout")) { if (atomic_exchange(&g_lock, 0)) LOG("物品对比：物品栏关了，手柄锁解开"); if (down && owned && !dead) { dead = 1; atomic_store(&g_held_own, -1); } }
    uint64_t f = dm_guest_frame();
    if (dn & KEY) { down = 1; t0 = f; owned = own_ok(b); dead = 0; }
    if (down && owned) {
        if (!dead) atomic_store(&g_held_own, (bt & KEY) != 0);
        if (up & KEY) { down = 0; atomic_store(&g_held_own, -1);
            if (dead) LOG("物品对比：按着对比键时物品栏关了，这一下作废");
            else if (f - t0 < PAD_SHORT) { if (!atomic_load(&g_lock)) { atomic_store(&g_lock, 1); LOG("物品对比：手柄短按对比键 → 锁上"); }
                else if (!dm_tipx_pad_page()) { atomic_store(&g_lock, 0); LOG("物品对比：手柄短按对比键（最后一页）→ 解锁"); } }
            else LOG("物品对比：手柄按住对比键 %llu 帧后松开 → 回到%s", (unsigned long long)(f - t0), atomic_load(&g_lock) ? "锁着" : "关着"); }
        return bt & ~KEY;
    }
    if (up & KEY) { down = 0; atomic_store(&g_held_own, -1); }
    return bt;
}
int dm_itemcmp_pad_compare(void) { return atomic_load(&g_lock) || atomic_load(&g_held_own) == 1; }


int dm_itemcmp_pad_held(void) { int h = atomic_load(&g_held_own); return h < 0 && atomic_load(&g_lock) ? 0 : h; }




#define D_NOPS 64
#define D_NSK 320
#define D_NDESC 32
static struct { int ok, lpv[8], mpm[8], pen[3][2], nops, nsk, ndesc; struct { int stat, op, tg[3], ntg; } op[D_NOPS]; struct { int id, cls, page, ps[12], nps, row, col; char key[48]; } sk[D_NSK];
                struct { int stat, func, dgrp, gfunc, shift; char pos[48], neg[48], gpos[48], gneg[48]; } desc[D_NDESC]; } D;
static void data_load(void) {
    if (D.ok) return; D.ok = -1;
    char p[1200]; FILE *f = dm_overlay_file("itemcmp.tsv", p, sizeof p) ? fopen(p, "r") : NULL;
    if (!f) { LOG("物品对比：覆盖层里没有 itemcmp.tsv（旧导入包或表不对），不出对比块"); return; }
    char line[512]; int n = 0, head = 0, end = -1, ncls = 0, ndiff = 0;
    while (fgets(line, sizeof line, f)) { n++; line[strcspn(line, "\r\n")] = 0; char *t[12] = { 0 }; int k = 0; for (char *q = line; k < 12 && q; ) { t[k++] = q; q = strchr(q, '\t'); if (q) *q++ = 0; }
        if (n == 1) { head = k >= 3 && !strcmp(t[0], "DM_ITEMCMP") && atoi(t[1]) == 1; continue; }
        if (!strcmp(t[0], "class") && k >= 5) { int c = atoi(t[1]); if (c >= 0 && c < 8) { D.lpv[c] = atoi(t[2]); D.mpm[c] = atoi(t[3]); ncls++; } }
        else if (!strcmp(t[0], "diff") && k >= 4) { int d = atoi(t[1]); if (d >= 0 && d < 3) { D.pen[d][0] = atoi(t[2]); D.pen[d][1] = atoi(t[3]); ndiff++; } }
        else if (!strcmp(t[0], "opstat") && k >= 4 && D.nops < D_NOPS) { D.op[D.nops].stat = atoi(t[1]); D.op[D.nops].op = atoi(t[2]); int m = 0; for (char *q = t[3]; q && *q && m < 3; ) { D.op[D.nops].tg[m++] = atoi(q); q = strchr(q, ','); if (q) q++; } D.op[D.nops].ntg = m; D.nops++; }
        else if (!strcmp(t[0], "skill") && k >= 6 && D.nsk < D_NSK) { int i = D.nsk++; D.sk[i].id = atoi(t[1]); D.sk[i].cls = atoi(t[2]); D.sk[i].page = atoi(t[3]); strlcpy(D.sk[i].key, t[4], sizeof D.sk[i].key);
            int m = 0; if (strcmp(t[5], "-")) for (char *q = t[5]; q && *q && m < 12; ) { D.sk[i].ps[m++] = atoi(q); q = strchr(q, ','); if (q) q++; } D.sk[i].nps = m; if(k>=8){D.sk[i].row=atoi(t[6]);D.sk[i].col=atoi(t[7]);} }
        else if (!strcmp(t[0], "desc") && k >= 10 && D.ndesc < D_NDESC) { int i = D.ndesc++; D.desc[i].stat = atoi(t[1]); D.desc[i].func = atoi(t[2]); D.desc[i].dgrp = atoi(t[5]);
            D.desc[i].gfunc = atoi(t[6]); D.desc[i].shift = atoi(t[9]);
            char *dst[4] = { D.desc[i].pos, D.desc[i].neg, D.desc[i].gpos, D.desc[i].gneg }; const char *src[4] = { t[3], t[4], t[7], t[8] };
            for (int j = 0; j < 4; j++) strlcpy(dst[j], strcmp(src[j], "-") ? src[j] : "", sizeof D.desc[i].pos); }
        else if (!strcmp(t[0], "end") && k >= 2) end = atoi(t[1]); }
    fclose(f);
    if (!head || end != n || ncls != 8 || ndiff != 3) { LOG("【红】物品对比：itemcmp.tsv 不完整（头 %d、行 %d / 尾记 %d、职业 %d、难度 %d），不出对比块", head, n, end, ncls, ndiff); return; }
    D.ok = 1; LOG("物品对比：itemcmp.tsv 读好——%d 个带 op 的属性、%d 个技能", D.nops, D.nsk);
    if (!D.ndesc) LOG("ASSUME itemcmp.desc_rows: itemcmp.tsv has no stat descriptions, every effect goes under the heading");
}

int dm_itemcmp_skill_class(int skill) { data_load(); if (D.ok <= 0) return -2; for (int j = 0; j < D.nsk; j++) if (D.sk[j].id == skill) return D.sk[j].cls; return -2; }

static int bench_coeffs(int cls, int *lpv, int *mpm, int *pen, uint8_t *b) {
    data_load(); if (D.ok <= 0 || cls < 0 || cls > 7) return 0; *lpv = D.lpv[cls]; *mpm = D.mpm[cls];
    int df = *(uint8_t *)(b + GA_DIFFICULTY); if (df > 2) df = 2; uint64_t gi = *(uint64_t *)(b + GA_GAME_INFO); *pen = D.pen[df][gi && *(uint8_t *)(gi + 0x5c) == 1]; return 1;
}

#define CC "\xee\x81\xbe"
static int raw_has1(uint64_t it, int st) {
    uint64_t sl = it ? *(uint64_t *)(it + 0x88) : 0; if (!sl) return 0;
    for (int part = 0; part < 2; part++) { if (part && *(int32_t *)(sl + 0x1c) >= 0) break; uint64_t a = sl + (part ? 0xe8 : 0x30), arr = *(uint64_t *)a, cnt = *(uint64_t *)(a + 8);
        for (uint64_t i = 0; arr && i < cnt && i < 512; i++) if ((*(uint32_t *)(arr + 8 * i) >> 16) == (uint32_t)st && *(int32_t *)(arr + 8 * i + 4)) return 1; }
    return 0;
}




#define KIDS(it, c, n) for (uint64_t c = first_item(it); c && n < 8; c = next_item(c), n++)
static int raw_has(uint64_t it, int st) { if (raw_has1(it, st)) return 1; int n = 0; KIDS(it, c, n) if (raw_has1(c, st)) return 1; return 0; }
static uint64_t list_hash(uint64_t u, uint64_t h) {
    uint64_t sl = u ? *(uint64_t *)(u + 0x88) : 0; if (!sl) return h * 31 + 7;
    for (int part = 0; part < 2; part++) { if (part && *(int32_t *)(sl + 0x1c) >= 0) break; uint64_t a = sl + (part ? 0xe8 : 0x30), arr = *(uint64_t *)a, cnt = *(uint64_t *)(a + 8);
        for (uint64_t i = 0; arr && i < cnt && i < 512; i++) { uint32_t st = *(uint32_t *)(arr + 8 * i) >> 16; if (st == 6 || st == 8 || st == 10) continue; h = (h ^ *(uint64_t *)(arr + 8 * i)) * 1099511628211ull; } }
    return h;
}

static int skill_sig_but(uint64_t it, int cls, int skip, int64_t *acc) {
    int64_t s = 0; uint64_t sl = it ? *(uint64_t *)(it + 0x88) : 0; if (!sl) return 0;
    uint64_t a = *(int32_t *)(sl + 0x1c) < 0 ? sl + 0xe8 : sl + 0x30, arr = *(uint64_t *)a, cnt = *(uint64_t *)(a + 8);
    for (uint64_t i = 0; arr && i < cnt && i < 512; i++) { uint32_t key = *(uint32_t *)(arr + 8 * i); int st = key >> 16, ly = key & 0xffff, v = *(int32_t *)(arr + 8 * i + 4);
        if ((st == 107 || st == 97) && ly == skip) continue;
        if (st == 107 || st == 97 || st == 188 || st == 127 || (st == 83 && ly == cls)) s += (int64_t)v * (int64_t)((st * 7919 + ly) * 2654435761u % 1000003 + 1); }
    *acc += s; return 1;
}
static int skill_sig(uint64_t it, int cls, int64_t *acc) { return skill_sig_but(it, cls, -1, acc); }
static uint64_t item_hash(uint64_t it, uint64_t h) { h = list_hash(it, h); int n = 0; KIDS(it, c, n) h = list_hash(c, h * 31 + *(uint32_t *)(c + 8)); return h; }

static uint64_t item_hash_stable(uint64_t it, uint64_t h) { h = list_hash(it, h); int n = 0; KIDS(it, c, n) h = list_hash(c, h * 31 + (uint32_t)n); return h; }
static int item_skill_sig_but(uint64_t it, int cls, int skip, int64_t *acc) { int r = skill_sig_but(it, cls, skip, acc), n = 0; KIDS(it, c, n) skill_sig_but(c, cls, skip, acc); return r; }
static int item_skill_sig(uint64_t it, int cls, int64_t *acc) { return item_skill_sig_but(it, cls, -1, acc); }
static void read_stats(uint8_t *b, uint64_t u, int cls, int *v) { for (int i = 0; i < IC_S_N; i++) v[i] = u ? gstat(b, u, dm_ic_stat_id[i], dm_ic_stat_id[i] == 83 ? cls : 0) : 0; }
static int item_effective(uint8_t *b, uint64_t pl, uint64_t it) { int s = 0, d = 0, l = 0; return ((int (*)(uint64_t, uint64_t, int, int *, int *, int *, int))(b + GA_ITEM_REQS_MET))(it, pl, 1, &s, &d, &l, 0) != 0; }

static void req_table_with(uint8_t *b, uint64_t pl, uint64_t it, int mastery, int *rs, int *rd) {
    *rs = *rd = 0; uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
    if (!t || ci >= *(uint32_t *)(t + 0x15c8)) return; uint64_t rec = *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0, d = *(uint64_t *)(it + 0x10);
    int pct = gstat(b, it, 91, 0) + (mastery ? ((int (*)(uint64_t, uint64_t, int, int))(b + GA_SKILL_MASTERY))(pl, it, 0, 6) : 0), eth = d && (*(uint32_t *)(d + 0x18) & 0x400000);
    int s0 = *(uint16_t *)(rec + 0x11a), d0 = *(uint16_t *)(rec + 0x11c); *rs = s0 ? dm_ic_req_value(s0, pct, eth) : 0; *rd = d0 ? dm_ic_req_value(d0, pct, eth) : 0;
}
static void req_table(uint8_t *b, uint64_t pl, uint64_t it, int *rs, int *rd) { req_table_with(b, pl, it, 1, rs, rd); }
static int any_has(const uint64_t *o, int n, int st) { for (int k = 0; k < n; k++) if (raw_has(o[k], st)) return 1; return 0; }
static int gsum(uint8_t *b, const uint64_t *o, int n, int st, int ly) { int v = 0; for (int k = 0; k < n; k++) v += gstat(b, o[k], st, ly); return v; }
#include "dm_gear_live.inc"







static int skill_pred(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *olds, int nold, int cls, int id, int *now, int *si_out, int *own) {
    int si = -1; *now = 0; *own = 0; for (int j = 0; j < D.nsk && si < 0; j++) if (D.sk[j].id == id) si = j; *si_out = si; if (si < 0 || D.sk[si].cls != cls) return -2; *own = 1;
    int pg = D.sk[si].page, ly = cls * 8 + pg - 1, d107 = gstat(b, cand, 107, id) - gsum(b, olds, nold, 107, id), dt = pg >= 1 && pg <= 3 ? gstat(b, cand, 188, ly) - gsum(b, olds, nold, 188, ly) : 0;
    int dall = gstat(b, cand, 127, 0) - gsum(b, olds, nold, 127, 0) + gstat(b, cand, 83, cls) - gsum(b, olds, nold, 83, cls);
    uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0, node = 0; for (int k = 0; nd && k < 1024 && !node; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (r && *(int16_t *)r == id && *(int32_t *)(nd + 0x44) == -1) node = nd; }
    int base = node ? *(int32_t *)(node + 0x38) : 0, after; *now = node ? base + ((int (*)(uint64_t, uint64_t, int))(b + GA_SKILL_BONUS_LEVEL))(pl, node, 0) : 0;
    if (!d107 && !((dt || dall) && node)) return -2;
    if (raw_has(cand, 126) || any_has(olds, nold, 126) || gstat(b, cand, 97, id) || gsum(b, olds, nold, 97, id)) return -1;
    if (base >= 1) after = *now + d107 + dall + dt; else { if (dall || dt) return -4; after = *now + d107; }
    return after < 0 ? 0 : after;
}
static const char *slot_name(uint8_t *b, int sl) { static char k[16]; snprintf(k, sizeof k, "dmCmpSlot%d", sl >= 1 && sl <= 10 ? sl : 1); return dm_str(b, k); }



static const char *game_str(uint8_t *b, const char *key, int nfmt, int maxg, const char *fallback) {
    struct { const char *k; uint64_t n; } s = { key, strlen(key) }; const char *r = ((const char *(*)(void *))(b + GA_LOC_STRING_GET))(&s);
    int ok = r && *r && strcmp(r, key) != 0, nd = 0, g = 0;
    for (const unsigned char *p = (const unsigned char *)r; ok && *p; p++) { if (*p == '\n' || *p == '\r') ok = 0; else if (*p == '%') { if (p[1] == 'd') { nd++; p++; } else ok = 0; }
        if ((*p & 0xc0) != 0x80) g++; }
    return ok && nd == nfmt && g <= maxg ? r : dm_str(b, fallback);
}








static int req_scan(uint8_t *b, uint64_t pl, uint64_t cand, const uint64_t *wit, int omask, int sl, int tstr, int tdex, int dstr, int ddex, req_flip *out, int max, int *closed) {
    int n = 0; *closed = 1;
    (void)cand;
    for (int s2 = 1; s2 <= 10; s2++) { uint64_t w = (omask >> s2 & 1) || s2 == sl ? 0 : wit[s2]; if (!w) continue;
        int rs, rd; req_table(b, pl, w, &rs, &rd); if (!rs && !rd) continue;
        int so = 1, dx = 1, lv = 1; ((int (*)(uint64_t, uint64_t, int, int *, int *, int *, int))(b + GA_ITEM_REQS_MET))(w, pl, 1, &so, &dx, &lv, 0);
        for (int k = 0; k < 2; k++) { int req = k ? rd : rs, t = k ? tdex : tstr, d = k ? ddex : dstr, own = gstat(b, w, k ? 2 : 0, 0), game = k ? dx : so; if (!req || !d) continue;
            int oc = dm_ic_req_outcome(req, t, t + d, own);
            if (game != (t >= req) && game != (t - own >= req)) oc = IC_REQ_AMBIG;
            if (oc == IC_REQ_SAME) continue;
            if (gstat(b, w, 0, 0) || gstat(b, w, 2, 0)) *closed = 0;
            if (n < max) { out[n].it = w; out[n].slot = s2; out[n].stat = k ? 2 : 0; out[n].req = req; out[n].oc = oc; } n++; } }
    return n;
}







static struct { uint64_t key, cur, ann_key; int n, dots, spl_kind, spl_share, spl_width; char txt[DM_TIPX_MAXL][DM_TIPX_LINEB]; int flags[DM_TIPX_MAXL]; char ann[16384]; size_t annlen; dm_tipx_span span[48]; int nspan; } C;
enum { K_HEAD, K_WARN, K_REQ, K_NOTE, K_VAL, K_MORE, K_WHY, K_END };
#define S_MAX 192
static struct { int n, lost; struct { int kind, flags; char s[DM_TIPX_LINEB]; } l[S_MAX]; } S;
static void add(int kind, int flags, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
static void add(int kind, int flags, const char *fmt, ...) {
    if (S.n >= S_MAX) { S.lost++; return; }
    va_list ap; va_start(ap, fmt); vsnprintf(S.l[S.n].s, sizeof S.l[S.n].s, fmt, ap); va_end(ap); S.l[S.n].kind = kind; S.l[S.n].flags = flags; S.n++;
}
static void put(int flags, const char *s) { if (C.n < DM_TIPX_MAXL) { snprintf(C.txt[C.n], sizeof C.txt[C.n], "%s", s); C.flags[C.n++] = flags; } }
static void pack(uint8_t *b) {
    static int kind[S_MAX]; static char keep[S_MAX]; int at;
    for (int i = 0; i < S.n; i++) { int k = S.l[i].kind; kind[i] = k == K_VAL ? IC_K_VAL : k == K_WHY ? IC_K_WHY : k == K_END ? IC_K_END : k == K_HEAD || k == K_WARN ? IC_K_MUST : IC_K_FIXED; }
    { static char drop[S_MAX]; if (dm_ic_hints(kind, S.n, drop)) { int w = 0; for (int i = 0; i < S.n; i++) if (!drop[i]) { if (w != i) S.l[w] = S.l[i]; kind[w++] = kind[i]; } S.n = w; } }
    int more = dm_ic_pack(kind, S.n, DM_TIPX_MAXL, S.lost, keep, &at); C.n = 0;
    static char f[DM_TIPX_LINEB], u[DM_TIPX_LINEB]; if (more) { char w[DM_TIPX_LINEB]; snprintf(f, sizeof f, CC "5%s", dm_strn(b, "dmCmpMore", more, w, sizeof w)); snprintf(u, sizeof u, f, more); }
    for (int i = 0; i < S.n; i++) { if (more && i == at) put(0, u); if (keep[i]) put(S.l[i].flags, S.l[i].s); }
    if (more && at >= S.n) put(0, u);
}

#include "dm_sim_live.inc"

static void splash_own(const char *s) {
    if (S.n >= S_MAX) { S.lost++; return; }
    S.n = dm_ic_insert(S.l, sizeof S.l[0], S.n, S_MAX, 0); S.l[0].kind = K_NOTE; S.l[0].flags = 0; snprintf(S.l[0].s, sizeof S.l[0].s, "%s", s);
}


static const char *const k_metric_key[IC_M_N] = { "dmCmpFcr", "dmCmpFhr", "dmCmpFbr", "dmCmpIas", "dmCmpResFire", "dmCmpResCold", "dmCmpResLight", "dmCmpResPois",
                                                  "dmCmpLife", "dmCmpMana", NULL, NULL, NULL, NULL, NULL, NULL, "dmCmpMf", "dmCmpGf", "dmCmpFrw", "dmCmpLl", "dmCmpMl" };

static const char *attack_name(uint8_t *b) {
    static char atk[64]; atk[0] = 0;
    int si = -1; for (int j = 0; j < D.nsk && si < 0; j++) if (D.sk[j].id == 0) si = j;
    if (si >= 0) dm_ic_label(dm_str(b, D.sk[si].key), 0, atk, sizeof atk);
    return atk;
}


static void cb_label(uint8_t *b, int m, char *lab, size_t n) {
    static const struct { const char *g; int kind; const char *fb; } lbk[IC_C_N] = { { "strchrdef", 0, "dmCmpDef" }, { "ItemStats1r", 2, "dmCmpBlock" }, { "strchratr", 1, "dmCmpAr" }, { "strchrskm", 0, "dmCmpDmg" } };
    if (!dm_ic_label(raw_str(b, lbk[m].g), lbk[m].kind, lab, n)) snprintf(lab, n, "%s", dm_str(b, lbk[m].fb));
}




static void pkg_block(uint8_t *b, uint64_t pl, int cls, const dm_gtab_pkg *pk, const char *splash, int *dots) {
    *dots = 0;
    if (dm_custom_get("item_cmp")) {
        const char *lb[IC_M_N]; for (int m = 0; m < IC_M_N; m++) lb[m] = k_metric_key[m] ? dm_str(b, k_metric_key[m]) : "";

        static dm_ic_sk sk[96]; static dm_ic_pkg_sk ps[96]; int nsk = 0;
        for (int j = 0; j < D.nsk && nsk < 96; j++) {
            int id = D.sk[j].id; if (D.sk[j].cls != cls || id < 0 || id >= DM_SIM_MAXSKILL || pk->lvl_before[id] == pk->lvl_after[id]) continue;
            uint64_t nd = skill_node(pl, id); sk[nsk++] = (dm_ic_sk){ id, nd ? *(int32_t *)(nd + 0x38) : 0, -1 };
        }
        { int keys[DM_BUILD_KEYS], nkeys = dm_build_keys_get(keys, DM_BUILD_KEYS);
          for (int i = 0; i < nsk; i++) for (int k = 0; k < nkeys && sk[i].key < 0; k++) if (keys[k] == sk[i].id) sk[i].key = k; }
        dm_ic_skill_order(sk, nsk);
        for (int i = 0; i < nsk; i++) { int si = -1; for (int j = 0; j < D.nsk && si < 0; j++) if (D.sk[j].id == sk[i].id) si = j;
            ps[i] = (dm_ic_pkg_sk){ si >= 0 ? dm_str(b, D.sk[si].key) : "?", pk->lvl_before[sk[i].id], pk->lvl_after[sk[i].id], sk[i].key }; }
        static char cbl[IC_C_N][96]; const char *atk = attack_name(b);
        dm_ic_pkg_words w = { lb, game_str(b, "max", 0, 8, "dmCmpCap"), dm_str(b, "dmCmpTo"), dm_str(b, "dmCmpMixed"), dm_str(b, "dmGearPkgWith"), dm_str(b, "dmCmpSkillLvl"),
                              dm_str(b, "dmCmpSkillGone"), dm_str(b, "dmCmpSkillMore"), { 0 } };
        for (int m = 0; m < IC_C_N; m++) { cb_label(b, m, cbl[m], sizeof cbl[m]);
            if ((m == IC_C_AR || m == IC_C_DMG) && atk[0]) { size_t L = strlen(cbl[m]); snprintf(cbl[m] + L, sizeof cbl[m] - L, " (%s)", atk); }
            w.cb_label[m] = cbl[m]; }
        int df = *(uint8_t *)(b + GA_DIFFICULTY); if (df > 2) df = 2;
        dm_ic_pkg_in in = { pk->before, pk->after, D.pen[df][cb_classic(b)], pk->with, ps, nsk, pk->cb, pk->cb_before, pk->cb_after, splash };
        static dm_ic_pkg_line out[64]; int n = dm_ic_pkg_lines(&in, &w, out, 64);
        for (int i = 0; i < n; i++) add(out[i].kind == IC_PKG_NOTE ? K_NOTE : out[i].kind == IC_PKG_MORE ? K_MORE : K_VAL, 0, "%s", out[i].text);
    }
    if (dm_custom_get("combat_sim")) {
        if (pk->rated == DM_GEAR_TIP_DONE) { gear_lines(b, &pk->tip); *dots = dm_gear_dots_code(&pk->tip.score); }
        else add_unrated(b, pk->rated == DM_GEAR_TIP_GRANTS ? "dmGearNoAura" : "dmGearNoSim");
    }
}



static int stat_line(uint8_t *b, uint64_t it, int st, int group, char *out, size_t n) {
    int v = gstat(b, it, st, 0); if (!v) return 0;
    for (int i = 0; i < D.ndesc; i++) if (D.desc[i].stat == st) {
        if ((group ? D.desc[i].gfunc : D.desc[i].func) != 19) return 0;
        const char *key = group ? (v < 0 ? D.desc[i].gneg : D.desc[i].gpos) : (v < 0 ? D.desc[i].neg : D.desc[i].pos), *fmt = key[0] ? raw_str(b, key) : NULL;
        return fmt && dm_ic_fill(fmt, v >> D.desc[i].shift, out, n) > 0;
    }
    return 0;
}




static const int k_line_stat[IC_M_N + 1] = { [IC_M_FCR] = 105, [IC_M_FHR] = 99, [IC_M_FBR] = 102, [IC_M_IAS] = 93, [IC_M_RES_FIRE] = 39, [IC_M_RES_COLD] = 43,
    [IC_M_RES_LIGHT] = 41, [IC_M_RES_POIS] = 45, [IC_M_LIFE] = 7, [IC_M_MANA] = 9, [IC_M_MF] = 80, [IC_M_GF] = 79, [IC_M_FRW] = 96 };

static struct { int ready, at, ne, hypo; uint64_t it; struct { int metric, stat, ann, before, after; dm_ic_fx fx; char line[160]; } e[IC_M_N + 2]; } X;

static void inline_collect(uint8_t *b, uint64_t it, const dm_ic_in *in, const dm_ic_line *out, int nl, const int *blk) {
    static const char *const key[IC_M_N + 1] = { [IC_M_RES_FIRE] = "dmImpFire", [IC_M_RES_COLD] = "dmImpCold", [IC_M_RES_LIGHT] = "dmImpLight", [IC_M_RES_POIS] = "dmImpPois",
        [IC_M_LIFE] = "dmImpLife", [IC_M_MANA] = "dmImpMana", [IC_M_MF] = "dmImpMf", [IC_M_GF] = "dmImpGf", [IC_M_FRW] = "dmImpRun", [IC_X_BLOCK] = "dmImpBlock" };
    static char speed[3][96];
    dm_ic_fx_words w = { .brief = dm_str(b, "dmImpShort"), .cap = dm_str(b, "dmImpCap"), .over = dm_str(b, "dmImpOver") };
    for (int m = 0; m <= IC_M_N; m++) if (key[m]) w.fmt[m] = dm_str(b, key[m]);
    for (int m = IC_M_FCR; m <= IC_M_FBR; m++) { snprintf(speed[m], sizeof speed[m], "%%s %s", dm_str(b, k_metric_key[m])); w.fmt[m] = speed[m]; }
    int ne = 0; X.it = it; X.hypo = in->hypothetical;
    for (int i = 0; i < nl && ne < IC_M_N; i++) { int m = out[i].metric; if (m == IC_M_LL || m == IC_M_ML) continue;

        if (m == IC_M_IAS || dm_ic_fx_text(m, out[i].before, out[i].after, out[i].cap, 0, in->hypothetical, &w, &X.e[ne].fx)) {
            if (m == IC_M_IAS) memset(&X.e[ne].fx, 0, sizeof X.e[ne].fx);
            X.e[ne].before = out[i].before; X.e[ne].after = out[i].after; X.e[ne++].metric = m; } }
    { int after[IC_S_N]; dm_ic_after(in, after);
      static const int rs[4] = { IC_S_FIRE, IC_S_COLD, IC_S_LIGHT, IC_S_POIS }, mx[4] = { IC_S_MAXFIRE, IC_S_MAXCOLD, IC_S_MAXLIGHT, IC_S_MAXPOIS };
      for (int k = 0; k < 4; k++) { int m = IC_M_RES_FIRE + k, seen = 0; for (int i = 0; i < nl; i++) seen |= out[i].metric == m;
          int raw = after[rs[k]] + in->penalty, cap = dm_ic_res_cap(after[mx[k]]);
          if (seen || (in->blocked >> m & 1) || gstat(b, it, k_line_stat[m], 0) <= 0 || raw <= cap || ne >= IC_M_N + 1) continue;
          int v = dm_ic_shown(in->tot, m, in->penalty);
          if (dm_ic_fx_text(m, v, v, 0, raw - cap, in->hypothetical, &w, &X.e[ne].fx)) { X.e[ne].before = X.e[ne].after = v; X.e[ne++].metric = m; } } }
    if (blk && ne < IC_M_N + 2 && dm_ic_fx_text(IC_X_BLOCK, blk[0], blk[1], blk[1] >= 75 && blk[0] < 75, 0, in->hypothetical, &w, &X.e[ne].fx)) {
        X.e[ne].before = blk[0]; X.e[ne].after = blk[1]; X.e[ne++].metric = IC_X_BLOCK; }
    X.ne = ne; X.at = S.n; X.ready = 1;
}

static const char *skill_name(uint8_t *b, int id) { for (int j = 0; j < D.nsk; j++) if (D.sk[j].id == id) return dm_str(b, D.sk[j].key); return NULL; }


static int inline_finish(uint8_t *b, const dm_tipx_ctx *c, const dm_gear_tip *tip) {
    uint64_t it = X.it; int ne = X.ne;
    dm_ic_frame_words fw = { dm_str(b, "dmImpFrames"), dm_str(b, "dmImpKeep"), dm_str(b, "dmImpKeepNext") };
    for (int i = 0; i < ne; i++) {
        int m = X.e[i].metric;
        if (m != IC_M_FCR && m != IC_M_IAS && m != IC_M_FHR && m != IC_M_FBR) continue;


        const dm_gear_frames *f = !tip ? NULL : m == IC_M_FHR ? &tip->recover : m == IC_M_FBR ? &tip->block : tip->main.cast == (m == IC_M_FCR) ? &tip->main : NULL;
        dm_ic_fx fx;
        if (f && f->ok && dm_ic_frames_text(f->before, f->after, f->next, X.hypo, m == IC_M_FCR || m == IC_M_IAS ? skill_name(b, f->skill) : dm_str(b, k_metric_key[m]), &fw, &fx)) X.e[i].fx = fx;
        else if (m == IC_M_IAS) X.e[i].fx.rest[0] = 0;
        else if (dm_ic_line_says(m, X.e[i].before, X.e[i].after, gstat(b, it, k_line_stat[m], 0))) X.e[i].fx.rest[0] = 0;
    }
    for (int i = 0; i < ne; i++) { int m = X.e[i].metric; if (m < IC_M_N && m > IC_M_FBR && dm_ic_line_says(m, X.e[i].before, X.e[i].after, gstat(b, it, k_line_stat[m], 0))) X.e[i].fx.rest[0] = 0; }

    static dm_tipx_ann ann[IC_M_N + 2]; static char group[160]; int na = 0, gi = -1, grouped;
    { int v[4]; const dm_ic_fx *f[4] = { 0 }; static const int order[4] = { IC_M_RES_FIRE, IC_M_RES_COLD, IC_M_RES_LIGHT, IC_M_RES_POIS };
      for (int k = 0; k < 4; k++) { v[k] = gstat(b, it, k_line_stat[order[k]], 0); for (int i = 0; i < ne; i++) if (X.e[i].metric == order[k]) f[k] = &X.e[i].fx; }
      grouped = v[0] && v[0] == v[1] && v[0] == v[2] && v[0] == v[3];
      if (dm_ic_res_group(v, f) && stat_line(b, it, 39, 1, group, sizeof group)) { gi = na; ann[na++] = (dm_tipx_ann){ group, f[0]->brief, NULL }; } }
    for (int i = 0; i < ne; i++) {
        int m = X.e[i].metric; X.e[i].ann = -1; X.e[i].line[0] = 0;
        X.e[i].stat = m == IC_X_BLOCK ? (gstat(b, it, 20, 0) ? 20 : 2) : k_line_stat[m];
        if (grouped && m >= IC_M_RES_FIRE && m <= IC_M_RES_POIS) { X.e[i].ann = gi; continue; }
        if (!X.e[i].fx.bracket[0] || !stat_line(b, it, X.e[i].stat, 0, X.e[i].line, sizeof X.e[i].line)) continue;
        X.e[i].ann = na; ann[na++] = (dm_tipx_ann){ X.e[i].line, X.e[i].fx.bracket, X.e[i].fx.brief };
    }
    int placed[IC_M_N + 2] = { 0 };
    C.annlen = 0; C.nspan = 0;
    if (na && c->orig) C.annlen = dm_tipx_annotate(c->orig, ann, na, c->max_chars, C.ann, sizeof C.ann, C.span, (int)(sizeof C.span / sizeof *C.span), &C.nspan, placed);
    static uint32_t said; uint32_t id = *(uint32_t *)(it + 8);
    for (int a = 0; a < na; a++) if ((placed[a] == DM_TIPX_ANN_NONE || placed[a] == DM_TIPX_ANN_MANY || placed[a] == DM_TIPX_ANN_COLOUR) && said != id) {
        said = id; LOG("ASSUME itemcmp.line_match: item %u, line \"%s\" found %s in the tooltip; its effect goes under the heading", id, ann[a].line,
                       placed[a] == DM_TIPX_ANN_MANY ? "more than once" : placed[a] == DM_TIPX_ANN_COLOUR ? "without a colour before it" : "nowhere"); }

    const char *rest[IC_M_N + 2]; int nr = 0, put = 0;
    for (int i = 0; i < ne; i++) if (!(X.e[i].ann >= 0 && placed[X.e[i].ann] == DM_TIPX_ANN_PLACED) && X.e[i].fx.rest[0]) rest[nr++] = X.e[i].fx.rest;
    for (int i = 0; i < nr && S.n < S_MAX; ) { static char line[DM_TIPX_LINEB]; int k = dm_tipx_join(rest + i, nr - i, CC "5 \xc2\xb7 ", c->max_chars, line, sizeof line);
        if (k <= 0) break;
        int at = X.at + put; S.n = dm_ic_insert(S.l, sizeof S.l[0], S.n, S_MAX, at); S.l[at].kind = K_VAL; S.l[at].flags = 0; snprintf(S.l[at].s, sizeof S.l[at].s, "%s", line);
        put++; i += k; }
    X.ready = 0;
    return put;
}



static dm_splash_area splash_of(uint8_t *b, uint64_t pl, int cls, uint64_t right, uint64_t left, int slot, int str, int dex) {
    dm_splash_area a = { 0, 0, 0 }; uint64_t hand[2] = { right, left }; char type[5]; int range;
    int weapon[2] = { is_type(b, right, 45), is_type(b, left, 45) }, active = sim_list_idle(right) ? 1 : sim_list_idle(left) ? 0 : -1;
    int k = dm_ic_splash_hand(weapon, slot, active);
    if (k < 0 || dm_splash_weapon_type(hand[k], type, &range)) return a;
    return dm_splash_area_of(dm_splash_group_of(type, hand_mode(b, pl, right, left) == 2), cls, str, dex, range);
}

static int splash_text(uint8_t *b, dm_splash_area a0, dm_splash_area a1, int weapon, int change_ok, int hypo, int known, char *out, size_t n) {
    return dm_ic_splash_line(a0.share, a0.width, a1.share, a1.width, weapon, change_ok, hypo, known, dm_str(b, "dmCmpSplash"), dm_str(b, "dmCmpCone"), dm_str(b, "dmCmpTo"), out, n);
}

int dm_itemcmp_block(const dm_tipx_ctx *c, dm_tipx_line *ln, int max, uint64_t *key) {
    uint8_t *b = c->b; uint64_t pl = c->player, it = c->item, d = *(uint64_t *)(it + 0x10);
    C.cur = 0;
    const int splash_on = dm_custom_splash_on();
    if (!d || !dm_ic_block_on(dm_custom_get("item_cmp"), dm_custom_get("combat_sim"), splash_on, is_type(b, it, 45))) return 0;
    uint32_t iid = c->ident ? c->ident : *(uint32_t *)(it + 8);
    data_load(); if (D.ok <= 0) return 0;
    int cls = (int)*(uint32_t *)(pl + 4); if (cls < 0 || cls > 7) return 0;
    if (dm_ui_panel_shown("HirelingInventoryPanel")) return 0;
    int unequip = 0, sl = 0, charm = IC_CHARM_NONE;
    const dm_gtab_pkg *pk = c->producer == DM_TIPX_CELL && c->pkg && c->pkg->ok ? c->pkg : NULL;
    const int inl = c->orig && !pk;
    if (c->producer == DM_TIPX_SLOT) {
        if (c->pad || *(uint32_t *)(it + 0xc) != 1) return 0;
        int mine = 0; for (uint64_t q = first_item(pl); q && !mine; q = next_item(q)) mine = q == it; if (!mine) return 0;
        unequip = 1; sl = *(uint8_t *)(d + 0x54);
    } else {
        if (c->widget && !*(uint8_t *)(c->widget + 0x631)) return 0;

        if (*(uint32_t *)(it + 0xc) == 1) return 0;

        int own = 0; for (uint64_t q = first_item(pl); q && !own; q = next_item(q)) own = q == it;
        charm = dm_ic_charm_move(is_type(b, it, 13), own, (int)*(uint32_t *)(it + 0xc), *(uint8_t *)(d + 0x55));
        if (charm && !c->widget) return 0;
    }
    const int removing = unequip || charm == IC_CHARM_OUT;
    int worn[16] = { 0 }; uint64_t wit[16] = { 0 };
    for (uint64_t q = first_item(pl); q; q = next_item(q)) { uint64_t qd = *(uint64_t *)(q + 0x10); if (qd && *(uint32_t *)(q + 0xc) == 1 && *(uint8_t *)(qd + 0x54) < 16) { worn[*(uint8_t *)(qd + 0x54)] = (int)*(uint32_t *)(q + 8); wit[*(uint8_t *)(qd + 0x54)] = q; } }
    int ident = (*(uint32_t *)(d + 0x18) & 0x10) != 0;
    if (!ident && !unequip) {
        uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4); int loc = 0;
        if (t && ci < *(uint32_t *)(t + 0x15c8)) { int16_t ty = *(int16_t *)(*(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0 + 0x12e); if (ty >= 0 && (uint64_t)ty < *(uint64_t *)(t + 0x1370)) loc = *(uint8_t *)(*(uint64_t *)(t + 0x1368) + (uint64_t)ty * 0xe8 + 0xa); }
        if ((!loc && !charm) || !dm_custom_get("item_cmp")) return 0;
        *key = 0x5eed ^ iid; C.key = 0; C.dots = 0; S.n = S.lost = 0;
        add(K_HEAD, DM_TIPX_HEAD, CC "0%s", charm ? dm_str(b, charm == IC_CHARM_OUT ? "dmCmpCharmOut" : "dmCmpCharmIn") : dm_str(b, "dmCmpEquip"));
        add(K_NOTE, 0, CC "5%s", dm_str(b, "dmCmpUnid")); pack(b);
        int n = C.n < max ? C.n : max; for (int i = 0; i < n; i++) { ln[i].s = C.txt[i]; ln[i].flags = C.flags[i]; } return n; }

    if (charm == IC_CHARM_OUT && !item_effective(b, pl, it)) return 0;

    int l1 = 0, l2 = 0;
    { uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
      if (t && ci < *(uint32_t *)(t + 0x15c8)) { int16_t ty = *(int16_t *)(*(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0 + 0x12e); if (ty >= 0 && (uint64_t)ty < *(uint64_t *)(t + 0x1370)) { uint64_t tr = *(uint64_t *)(t + 0x1368) + (uint64_t)ty * 0xe8; l1 = *(uint8_t *)(tr + 0xa); l2 = *(uint8_t *)(tr + 0xb); } } }



    if (charm) { int gsl = 0;
        if (pick_slot(b, pl, it, &gsl)) { static uint32_t said; if (said != iid) { said = iid; LOG("ASSUME itemcmp.charm_slot: charm %u got body slot %d", *(uint32_t *)(it + 8), gsl); } }
        sl = 0; }
    else if (!unequip && c->slot > 0 && dm_ic_slot(c->slot, l1, l2, worn) == c->slot) sl = c->slot;
    else if (!unequip && !pick_slot(b, pl, it, &sl)) {
        sl = dm_ic_slot(0, l1, l2, worn);


        if (!sl && l1 == 4 && l2 == 5 && is_type(b, it, 45)) sl = 4;
        if (!sl) return 0; }






    int place = 0, dmask = 0, ot = dm_ic_other_hand(sl);
    if (!unequip && !charm) { int (*pf_)(uint64_t, int, uint64_t, int, int) = (int (*)(uint64_t, int, uint64_t, int, int))(b + GA_ITEM_PLACE_CHECK); place = pf_(pl, sl, it, 1, 1); if (!place) place = pf_(pl, sl, it, 0, 1);
        if (!place) return 0;
        dmask = dm_ic_displaced(place, sl, sl > 0 && sl < 16 && worn[sl], ot >= 0 && worn[ot]); }


    uint64_t olds[4] = { 0, 0, 0, 0 }; int nold = 0, omask = unequip ? 1 << sl : dmask;
    if (removing) olds[nold++] = it;
    else if (dmask > 0) { for (int s2 = 0; s2 < 16 && nold < 2; s2++) if ((dmask >> s2 & 1) && wit[s2]) olds[nold++] = wit[s2]; }
    else if (dmask < 0 && sl > 0 && sl < 16 && wit[sl]) olds[nold++] = wit[sl];
    if (dmask < 0) omask = sl > 0 && sl < 16 && wit[sl] ? 1 << sl : 0;
    uint64_t old = olds[0], cand = removing ? 0 : it;

    uint64_t h = 1469598103934665603ull;
    h = (h ^ iid) * 1099511628211ull; h = (h ^ *(uint32_t *)(d + 0x18)) * 1099511628211ull; h = (h ^ (uint64_t)(sl | unequip << 8 | c->pad << 9 | charm << 10 | dm_lang_group() << 12)) * 1099511628211ull;
    h = c->ident ? item_hash_stable(it, h) : item_hash(it, h); for (int k = 0; k < nold; k++) h = item_hash(olds[k], h); h = list_hash(pl, h); h = (h ^ (uint64_t)(uint32_t)place << 20) * 1099511628211ull;
    for (int i = 1; i < 13; i++) h = (h ^ (uint32_t)worn[i]) * 1099511628211ull;
    { int kk[DM_BUILD_KEYS], nk = dm_build_keys_get(kk, DM_BUILD_KEYS); for (int i = 0; i < nk; i++) h = (h ^ (uint32_t)kk[i]) * 1099511628211ull; h = (h ^ (uint32_t)nk << 24) * 1099511628211ull; }
    { uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0;
      for (int k = 0; nd && k < 1024; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (r) h = (h ^ ((uint64_t)(uint16_t)*(int16_t *)r << 32 | (uint32_t)*(int32_t *)(nd + 0x38) << 8 | (uint8_t)*(int32_t *)(nd + 0x44))) * 1099511628211ull; } }
    {uint64_t list=*(uint64_t *)(pl+0x100);for(int i=0;list && i<2;i++){uint64_t node=*(uint64_t *)(list+8+8*i),record=node?*(uint64_t *)node:0;h=(h ^ (record?(uint16_t)*(int16_t *)record:65535))*1099511628211ull;h=(h ^ (node?(uint32_t)*(int32_t *)(node+0x44):0))*1099511628211ull;}}
    h=(h ^ (unsigned)dm_build_session())*1099511628211ull;
    h = (h ^ (unsigned)dm_custom_get("combat_sim")) * 1099511628211ull;
    if (inl) { for (const char *p = c->orig; *p; p++) h = (h ^ (uint8_t)*p) * 1099511628211ull; h = (h ^ (uint64_t)(c->max_chars + 1) << 40) * 1099511628211ull; }
    h=(h ^ (unsigned)dm_custom_get("item_cmp"))*1099511628211ull;
    h = dm_ic_splash_key(h, splash_on, IC_SPLASH_NONE, 0, 0);
    for(int i=0;i<sim_nbound;i++)h=(h ^ (unsigned)sim_bound[i])*1099511628211ull;
    h = (h ^ *(uint8_t *)(b + GA_DIFFICULTY)) * 1099511628211ull;
    h = sim_tip_key(b, pl, h);
    if (pk) h = (h ^ (0x6b67ull << 32 | pk->gen)) * 1099511628211ull;
    { dm_gear_goal g; dm_gear_goal_get(dm_build_session(), &g); h = (h ^ (uint64_t)lround(g.survival_hits * 1000 + g.boss_share * 100)) * 1099511628211ull; }
    { int gv[IC_C_N + 3]; cb_game_now(b, pl, gv); for (int i = 0; i <= IC_C_N; i++) h = (h ^ (uint32_t)gv[i]) * 1099511628211ull;
      uint64_t l = *(uint64_t *)(pl + 0x88); uint8_t ver = *(uint8_t *)(pl + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + GA_DATA_TABLES + (uint64_t)ver * 16) : 0;
      if (l && t && *(int32_t *)(l + 0x1c) < 0) { uint64_t nw = (*(uint64_t *)(t + 0x2d0) + 31) / 32; for (uint64_t i = 0; i < nw && i < 32; i++) h = (h ^ *(uint32_t *)(l + 0xb30 + 4 * i)) * 1099511628211ull; } }

    *key = ((uint64_t)iid << 32 ^ (uint64_t)((old ? *(uint32_t *)(old + 8) : 0) ^ (olds[1] ? *(uint32_t *)(olds[1] + 8) << 12 : 0)) << 8 ^ (uint64_t)(sl | unequip << 5 | c->pad << 6 | charm << 7)) * 1099511628211ull;
    C.cur = h;
    if (h != C.key || !C.n) {
        C.key = h; S.n = S.lost = 0; C.annlen = 0; C.nspan = 0; C.ann_key = h; X.ready = 0; C.spl_kind = C.spl_share = C.spl_width = 0;

        enum { SIMDEP = 8 }; static char simdep[SIMDEP][DM_TIPX_LINEB]; int nsimdep = 0;
        static char spl[DM_TIPX_LINEB];



        static const char *why[12], *cbwhy[IC_C_N]; int nwhy = 0, ncbwhy = 0, gear_skip = 1, block_late = 0;
        const char *unrated = NULL; const int *gdelta_shown = NULL;
        unsigned unproven = 0; static char unproven_label[IC_C_N][96];
        int attack_passive = 0;
        const char *hd = dm_str(b, charm ? (charm == IC_CHARM_OUT ? "dmCmpCharmOut" : "dmCmpCharmIn") : unequip ? "dmCmpUnequip" : "dmCmpEquip");
        if (unequip || charm) add(K_HEAD, DM_TIPX_HEAD, CC "0%s", hd);
        else { static char sub[160], nm[96]; int k = 0; nm[0] = 0;
            for (int s2 = 1; s2 < 16; s2++) if ((omask >> s2 & 1) && wit[s2]) k += snprintf(nm + k, sizeof nm - k > 0 ? sizeof nm - k : 0, "%s%s", k ? " + " : "", slot_name(b, s2));
            if (!k) snprintf(nm, sizeof nm, "%s", sl > 0 ? slot_name(b, sl) : "?");
            snprintf(sub, sizeof sub, dm_str(b, nold ? "dmCmpInstead" : "dmCmpEmpty"), nm); add(K_HEAD, DM_TIPX_HEAD, CC "0%s " CC "5%s", hd, sub); }
        if (pk) {
            if (splash_on) { C.spl_kind = splash_text(b, pk->splash_before, pk->splash_after, is_type(b, it, 45), dm_custom_get("item_cmp"), 0, 1, spl, sizeof spl);
                C.spl_share = pk->splash_after.share; C.spl_width = pk->splash_after.width; }
            int dots; pkg_block(b, pl, cls, pk, C.spl_kind == IC_SPLASH_CHANGE ? spl : NULL, &dots); C.dots = dots;
            int dm_rw_bases_for(uint8_t *, uint64_t, char *, size_t); static char names[DM_TIPX_LINEB], line[DM_TIPX_LINEB];
            if (dm_custom_get("item_cmp") && dm_rw_bases_for(b, it, names, sizeof names - 16) > 0) { snprintf(line, sizeof line - 8, dm_str(b, "dmGearBases"), names); add(K_NOTE, 0, CC "4%s", line); }
            if (C.spl_kind == IC_SPLASH_OWN) { if (S.n == 1 && S.l[0].kind == K_HEAD) S.n = 0; splash_own(spl); }
            pack(b);
            static uint32_t said; if (said != iid) { said = iid; LOG("best gear: row %#x compared within the recommendation (%d lines, rated %d)", iid, C.n, pk->rated); }
            goto out;
        }
        if (unequip && (sl == 11 || sl == 12)) { add(K_NOTE, 0, CC "5%s", dm_str(b, "dmCmpInactive")); goto done; }
        {
        dm_ic_in in; memset(&in, 0, sizeof in); read_stats(b, pl, cls, in.tot); read_stats(b, cand, cls, in.new_);
        for (int k = 0; k < nold; k++) { int v[IC_S_N]; read_stats(b, olds[k], cls, v); for (int i = 0; i < IC_S_N; i++) in.old_[i] += v[i]; }
        in.lpv = D.lpv[cls]; in.mpm = D.mpm[cls];
        { int df = *(uint8_t *)(b + GA_DIFFICULTY); if (df > 2) df = 2; uint64_t gi = *(uint64_t *)(b + GA_GAME_INFO); int classic = gi && *(uint8_t *)(gi + 0x5c) == 1;
          in.penalty = D.pen[df][classic]; }
        int nonum = 0;
        unsigned cbop = 0, cbpas = 0; int skchg = 0;
#define WHY(k) do { const char *w_ = (k); int s_ = 0; for (int j_ = 0; j_ < nwhy; j_++) s_ |= !strcmp(why[j_], w_); if (!s_ && nwhy < 12) why[nwhy++] = w_; } while (0)







        static gear_swap_out GS; static uint64_t gs_key; static int gdelta[DM_GEAR_NSTAT]; int composed = 0;
        if (gear_involved(cand, olds, nold) && dmask >= 0) {
            if (h != gs_key || GS.ok < 0) { uint64_t g0 = now_ns(); gear_swap(b, pl, cand, olds, nold, sl, &GS); gs_key = h;
                LOG("[cmp] gear: swap composed in %.3f ms (%d worn, %s)", (now_ns() - g0) / 1e6, GS.nworn, GS.ok > 0 ? "rated" : "unrated"); }
            composed = GS.ok > 0; }
        uint64_t two[3] = { cand, olds[0], olds[1] };
        for (int k = 0; k < 3; k++) { uint64_t x = two[k], xd = x ? *(uint64_t *)(x + 0x10) : 0; if (!xd) continue;
            if (*(uint32_t *)xd == 5 && !composed) { WHY("dmCmpSet"); nonum = 1; }
            if (*(uint32_t *)(xd + 0x18) & 0x100) { WHY("dmCmpBroken"); nonum = 1; }
            if (raw_has(x, 151) || raw_has(x, 97)) { WHY("dmCmpAura"); nonum = 1; } }
        for (int k = 0; k < nold; k++) if (!item_effective(b, pl, olds[k])) { WHY("dmCmpOldOff"); nonum = 1; }
        if (!unequip && dmask < 0) { WHY("dmCmpTwoHand"); nonum = 1; }



        if (!removing) { int so = 1, dx = 1, lv = 1, ok = ((int (*)(uint64_t, uint64_t, int, int *, int *, int *, int))(b + GA_ITEM_REQS_MET))(it, pl, 1, &so, &dx, &lv, 0);
            int rs, rd, hs = in.tot[IC_S_STR], hx = in.tot[IC_S_DEX]; req_table(b, pl, it, &rs, &rd);
            if (ok) { hs -= in.old_[IC_S_STR]; hx -= in.old_[IC_S_DEX]; so = dm_ic_req_kept(rs, in.tot[IC_S_STR], in.old_[IC_S_STR]); dx = dm_ic_req_kept(rd, in.tot[IC_S_DEX], in.old_[IC_S_DEX]); ok = so && dx; }


            int64_t sa = 0, sz = 0; item_skill_sig(cand, cls, &sa); for (int k = 0; k < nold; k++) item_skill_sig(olds[k], cls, &sz);
            if (ok && sa != sz && ((int (*)(uint64_t, uint64_t, int, int))(b + GA_SKILL_MASTERY))(pl, it, 0, 6)) { int ms, md; req_table_with(b, pl, it, 0, &ms, &md);
                if (!dm_ic_req_kept(ms, in.tot[IC_S_STR], in.old_[IC_S_STR]) || !dm_ic_req_kept(md, in.tot[IC_S_DEX], in.old_[IC_S_DEX])) { static char rq[96], ln[DM_TIPX_LINEB]; int st = md != rd;
                    snprintf(rq, sizeof rq, game_str(b, st ? "ItemStats1f" : "ItemStats1e", 1, 40, st ? "dmCmpReqDexT" : "dmCmpReqStrT"), st ? rd : rs);
                    snprintf(ln, sizeof ln, dm_str(b, "dmCmpReqMaybe"), slot_name(b, sl), rq); add(K_REQ, 0, CC "5%s", ln); WHY("dmCmpChainNote"); nonum = 1; } }
            if (!ok) { static char rq[200]; int k = 0;
                if (!lv) k += snprintf(rq + k, sizeof rq - k, dm_str(b, "dmCmpReqLvl"), ((int (*)(uint64_t, uint64_t))(b + GA_ITEM_REQ_LEVEL))(it, pl), gstat(b, pl, 12, 0));
                if (!so) k += snprintf(rq + k, sizeof rq - k, "%s", k ? " \xc2\xb7 " : ""), k += snprintf(rq + k, sizeof rq - k, dm_str(b, "dmCmpReqStr"), rs, hs);
                if (!dx) k += snprintf(rq + k, sizeof rq - k, "%s", k ? " \xc2\xb7 " : ""), k += snprintf(rq + k, sizeof rq - k, dm_str(b, "dmCmpReqDex"), rd, hx);
                if (!k) snprintf(rq, sizeof rq, "?");
                add(K_WARN, DM_TIPX_WARN, CC "1%s", ({ static char u[240]; snprintf(u, sizeof u, dm_str(b, "dmCmpUnusable"), rq); u; }));
                in.hypothetical = 1; } }


        if (composed && cand && (in.hypothetical || !GS.aft.active[GS.nafter - 1])) composed = 0;
        if (!composed && gear_involved(cand, olds, nold)) { WHY("dmCmpSet"); nonum = 1; }
        if (composed) { gear_deltas(&GS, gdelta); in.composed = 1;
            for (int i = 0; i < IC_S_N; i++) { int id = dm_ic_stat_id[i]; in.old_[i] = 0; in.new_[i] = gear_d(&GS, id, id == 83 ? cls : 0); } }

        for (int i = 0; i < D.nops; i++) { if (D.op[i].op == 8 || D.op[i].op == 9 || (composed && (D.op[i].op == 2 || D.op[i].op == 4))) continue; if (!raw_has(cand, D.op[i].stat) && !any_has(olds, nold, D.op[i].stat)) continue;
            if (D.op[i].op == 13) continue;
            { unsigned cm = 0; int other = 0; for (int k = 0; k < D.op[i].ntg; k++) { int t_ = D.op[i].tg[k]; unsigned dd = cb_dep(t_); cm |= dd; other |= !dd || t_ == 0 || t_ == 2; } cbop |= cm; if (cm && !other) continue; }
            for (int k = 0; k < D.op[i].ntg; k++) { int m = dm_ic_metric_of_stat(D.op[i].tg[k]); if (m >= 0) in.blocked |= 1u << m;
                if (D.op[i].tg[k] == 3) in.blocked |= 1u << IC_M_LIFE; if (D.op[i].tg[k] == 1) in.blocked |= 1u << IC_M_MANA; if (D.op[i].tg[k] == 0 || D.op[i].tg[k] == 2) nonum = 1; }
            WHY("dmCmpOp"); }

        if (in.tot[IC_S_HPPCT] || in.tot[IC_S_MANAPCT]) { int dv = in.new_[IC_S_VIT] - in.old_[IC_S_VIT], de = in.new_[IC_S_ENE] - in.old_[IC_S_ENE];
            if (in.tot[IC_S_HPPCT] && (dv || in.new_[IC_S_HP] != in.old_[IC_S_HP])) { in.blocked |= 1u << IC_M_LIFE; WHY("dmCmpPct"); }
            if (in.tot[IC_S_MANAPCT] && (de || in.new_[IC_S_MANA] != in.old_[IC_S_MANA])) { in.blocked |= 1u << IC_M_MANA; WHY("dmCmpPct"); } }


        unsigned sim_metrics = in.blocked;
        {

          int64_t ab = 0, zb = 0; item_skill_sig_but(cand, cls, 263, &ab); for (int k = 0; k < nold; k++) item_skill_sig_but(olds[k], cls, 263, &zb);
          if (ab != zb) { skchg = 1; int learned[64], lvl[64], nl = skill_levels(b, pl, learned, lvl, 64);
              for (int i = 0; i < D.nsk; i++) { if (!D.sk[i].nps) continue; int use = 0; for (int j = 0; j < nl && !use; j++) use = learned[j] == D.sk[i].id;
                  if (!use && D.sk[i].cls != cls) continue; if (!use && !(gstat(b, cand, 107, D.sk[i].id) || gsum(b, olds, nold, 107, D.sk[i].id) || gstat(b, cand, 97, D.sk[i].id) || gsum(b, olds, nold, 97, D.sk[i].id))) continue;
                  for (int k = 0; k < D.sk[i].nps; k++) { int ps = D.sk[i].ps[k], m = dm_ic_metric_of_stat(ps); unsigned before = in.blocked;
                      if (m >= 0) in.blocked |= 1u << m; cbpas |= cb_dep(ps);
                      if (ps == 3 || ps == 76) in.blocked |= 1u << IC_M_LIFE; if (ps == 1 || ps == 77) in.blocked |= 1u << IC_M_MANA;
                      if (ps == 0 || ps == 2) { nonum = 1; WHY("dmCmpPassive"); }
                      for (int o = 0; o < D.nops; o++) if (D.op[o].stat == ps) for (int t = 0; t < D.op[o].ntg; t++) { int mt = dm_ic_metric_of_stat(D.op[o].tg[t]); if (mt >= 0) in.blocked |= 1u << mt; }
                      if ((in.blocked & ~IC_M_HIDDEN) != (before & ~IC_M_HIDDEN)) WHY("dmCmpPassive"); } } } }








        int dstr = in.new_[IC_S_STR] - in.old_[IC_S_STR], ddex = in.new_[IC_S_DEX] - in.old_[IC_S_DEX];
        int attr_ok = !nonum;
        if ((dstr || ddex) && !in.hypothetical && !composed) { req_flip rf[24]; int closed, nrf = req_scan(b, pl, cand, wit, omask, sl, in.tot[IC_S_STR], in.tot[IC_S_DEX], dstr, ddex, rf, 24, &closed), nother = 0;
            if (nrf > 24) nrf = 24;
            for (int i = 1; i < nrf; i++) { req_flip x = rf[i]; int j = i - 1; for (; j >= 0 && x.oc == IC_REQ_LOSS && rf[j].oc != IC_REQ_LOSS; j--) rf[j + 1] = rf[j]; rf[j + 1] = x; }
            if (!attr_ok) for (int i = 0; i < nrf; i++) rf[i].oc = IC_REQ_AMBIG;
            int nloss = 0;
            for (int i = 0; i < nrf; i++) { const req_flip *f = &rf[i]; if (f->oc != IC_REQ_LOSS && nother++ >= 3) continue; static char rq[96], ln1[DM_TIPX_LINEB], ln2[DM_TIPX_LINEB]; int st = f->stat == 2;
                snprintf(rq, sizeof rq, game_str(b, st ? "ItemStats1f" : "ItemStats1e", 1, 40, st ? "dmCmpReqDexT" : "dmCmpReqStrT"), f->req);
                const char *nm = slot_name(b, f->slot), *k1 = f->oc == IC_REQ_AMBIG ? "dmCmpReqMaybe" : !closed ? "dmCmpReqChain" : f->oc == IC_REQ_GAIN ? "dmCmpReqMet" : "dmCmpReqLost";
                snprintf(ln1, sizeof ln1, dm_str(b, k1), nm, rq);
                if (closed && f->oc != IC_REQ_AMBIG) { int t = st ? in.tot[IC_S_DEX] : in.tot[IC_S_STR]; snprintf(ln2, sizeof ln2, dm_str(b, "dmCmpReqTotal"), dm_str(b, st ? "dmCmpDex" : "dmCmpStr"), t, t + (st ? ddex : dstr), ln1); }
                else snprintf(ln2, sizeof ln2, "%s", ln1);
                if (f->oc == IC_REQ_LOSS) { if (nloss++ < 2) add(K_WARN, DM_TIPX_WARN, CC "1%s", ln2); else add(K_REQ, 0, CC "1%s", ln2); }
                else add(K_REQ, 0, CC "%c%s", f->oc == IC_REQ_GAIN && closed ? '3' : '5', ln2); }
            if (nother > 3) add(K_REQ, 0, ({ static char f_[DM_TIPX_LINEB]; char w_[DM_TIPX_LINEB]; snprintf(f_, sizeof f_, CC "5%s", dm_strn(b, "dmCmpMore", nother - 3, w_, sizeof w_)); f_; }), nother - 3);



            int open_ = 0, said = 1;
            for (int i = 0; i < nrf; i++) { uint64_t x = rf[i].it, xd = x ? *(uint64_t *)(x + 0x10) : 0; int dup = 0; for (int k = 0; k < nold; k++) dup |= olds[k] == x; if (dup) continue;
                int64_t sig = 0; item_skill_sig(x, cls, &sig); int ops = 0; for (int o = 0; o < D.nops; o++) ops |= raw_has(x, D.op[o].stat);
                if (closed && attr_ok && rf[i].oc == IC_REQ_LOSS && nold < 4 && xd && *(uint32_t *)xd != 5 && !(*(uint32_t *)(xd + 0x18) & 0x100) && !raw_has(x, 151) && !raw_has(x, 97) && !ops && !sig) {
                    int v[IC_S_N]; read_stats(b, x, cls, v); for (int k = 0; k < IC_S_N; k++) in.old_[k] += v[k]; olds[nold++] = x; omask |= 1 << rf[i].slot; continue; }
                open_ = 1; if (rf[i].oc == IC_REQ_AMBIG || closed) said = 0; }
            if (open_) { if (!said) WHY("dmCmpChainNote"); nonum = 1; } }


        if (splash_on && sl != 11 && sl != 12) {
            int weapon = is_type(b, it, 45); uint64_t ha[2]; dm_ic_hands_after(sl, omask, cand, wit[4], wit[5], ha);
            dm_splash_area a0 = splash_of(b, pl, cls, wit[4], wit[5], sl, in.tot[IC_S_STR], in.tot[IC_S_DEX]);
            dm_splash_area a1 = unequip && weapon ? a0 : splash_of(b, pl, cls, ha[0], ha[1], sl, in.tot[IC_S_STR] + in.new_[IC_S_STR] - in.old_[IC_S_STR],
                                                                   in.tot[IC_S_DEX] + in.new_[IC_S_DEX] - in.old_[IC_S_DEX]);

            C.spl_kind = splash_text(b, a0, a1, weapon, dm_custom_get("item_cmp") && !nonum, in.hypothetical, (unequip && weapon) || !nonum, spl, sizeof spl);
            C.spl_share = a1.share; C.spl_width = a1.width; }
        if (!nonum) {
            const char *lb[IC_M_N]; for (int m = 0; m < IC_M_N; m++) lb[m] = k_metric_key[m] ? dm_str(b, k_metric_key[m]) : "";
            static dm_ic_line out[IC_M_N]; int nl = dm_ic_compute(&in, lb, game_str(b, "max", 0, 8, "dmCmpCap"), dm_str(b, "dmCmpTo"), out, IC_M_N);







            { int tabd[3]; for (int p = 0; p < 3; p++) tabd[p] = gstat(b, cand, 188, cls * 8 + p) - gsum(b, olds, nold, 188, cls * 8 + p);
              int dall = gstat(b, cand, 127, 0) - gsum(b, olds, nold, 127, 0) + gstat(b, cand, 83, cls) - gsum(b, olds, nold, 83, cls);
              static dm_ic_sk sk[96]; int nsk = 0;
#define SKADD(id_, base_) do { int dup_ = 0; for (int j_ = 0; j_ < nsk; j_++) dup_ |= sk[j_].id == (id_); if (!dup_ && nsk < 96) { sk[nsk].id = (id_); sk[nsk].base = (base_); sk[nsk].key = -1; nsk++; } } while (0)
              { uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0;
                for (int k = 0; nd && k < 1024; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (!r || *(int32_t *)(nd + 0x44) != -1) continue;
                    int id = *(int16_t *)r, bs = *(int32_t *)(nd + 0x38), si = -1; for (int j = 0; j < D.nsk && si < 0; j++) if (D.sk[j].id == id) si = j;
                    if (si < 0 || D.sk[si].cls != cls) continue;
                    int pg = D.sk[si].page, tch = pg >= 1 && pg <= 3 && tabd[pg - 1];
                    if ((dall || tch) && (bs >= 1 || bs + ((int (*)(uint64_t, uint64_t, int))(b + GA_SKILL_BONUS_LEVEL))(pl, nd, 0) > 0)) SKADD(id, bs); } }
              for (int k = 0; k < 3; k++) { uint64_t x = k ? olds[k - 1] : cand, sl3 = x ? *(uint64_t *)(x + 0x88) : 0; if (!sl3) continue;
                  uint64_t a3 = *(int32_t *)(sl3 + 0x1c) < 0 ? sl3 + 0xe8 : sl3 + 0x30, arr = *(uint64_t *)a3, cnt = *(uint64_t *)(a3 + 8);
                  for (uint64_t i = 0; arr && i < cnt && i < 512; i++) { uint32_t kk = *(uint32_t *)(arr + 8 * i); if ((kk >> 16) != 107) continue; int id = (int)(kk & 0xffff), bs = 0;
                      uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0; for (int j = 0; nd && j < 1024; j++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (r && *(int16_t *)r == id && *(int32_t *)(nd + 0x44) == -1) { bs = *(int32_t *)(nd + 0x38); break; } }
                      SKADD(id, bs); } }
#undef SKADD
              { int keys[DM_BUILD_KEYS], nkeys = dm_build_keys_get(keys, DM_BUILD_KEYS);
                for (int i = 0; i < nsk; i++) for (int k = 0; k < nkeys && sk[i].key < 0; k++) if (keys[k] == sk[i].id) sk[i].key = k; }
              dm_ic_skill_order(sk, nsk);
              static char skl[48][DM_TIPX_LINEB]; int nskl = 0, nonkey = 0, hidden = 0;
              for (int q = 0; q < nsk; q++) { int now, si, own, after = skill_pred(b, pl, cand, olds, nold, cls, sk[q].id, &now, &si, &own);
                  if (!own) continue; if (after == -1) { WHY("dmCmpSkillMix"); continue; } if (after == -4) { WHY("dmCmpSkillItem"); continue; }
                  if (after < 0 || after == now) continue;
                  if (sk[q].key < 0 && nonkey >= 6) { hidden++; continue; }
                  if (nskl >= 48) { hidden++; continue; }
                  static char s1[DM_TIPX_LINEB]; snprintf(s1, sizeof s1, dm_str(b, "dmCmpSkillLvl"), dm_str(b, D.sk[si].key), now, after);
                  snprintf(skl[nskl++], sizeof skl[0], CC "%c%s%s%s", in.hypothetical ? '5' : after > now ? '6' : '1', s1, after ? "" : " ", after ? "" : dm_str(b, "dmCmpSkillGone"));
                  if (sk[q].key < 0) nonkey++; }





              static char cbl[IC_C_N][DM_TIPX_LINEB]; int ncb = 0, blk[2] = { -1, -1 }; const char *atk = attack_name(b);
              { int dsk = gstat(b, cand, 127, 0) - gsum(b, olds, nold, 127, 0) + gstat(b, cand, 107, 0) - gsum(b, olds, nold, 107, 0);
                cb_res R; if (sl == 11 || sl == 12) memset(&R, 0, sizeof R); else cb_gather_scoped(b, pl, cand, olds, nold, sl, omask, wit[4], wit[5], dsk, skchg || composed, &R, 0, composed ? gdelta : NULL, charm);
                R.affected |= cbop | cbpas; cb_why(&R, cbop, "dmCmpOp"); cb_why(&R, cbpas, "dmCmpPassive");



                if (skchg && state101(pl)) { R.affected |= 1u << IC_C_DEF; cb_why(&R, 1u << IC_C_DEF, "dmCmpPassive"); }
                if (skchg && (R.now.ar_mast || R.now.dmast || R.aft.ar_mast || R.aft.dmast)) { R.affected |= 1u << IC_C_AR | 1u << IC_C_DMG; cb_why(&R, 1u << IC_C_AR | 1u << IC_C_DMG, "dmCmpPassive"); }



                if (dsk) { R.affected |= 1u << IC_C_AR | 1u << IC_C_DMG; cb_why(&R, 1u << IC_C_AR | 1u << IC_C_DMG, "dmCmpSkillCombat"); }

                if (in.hypothetical) R.affected = 0;


                uint64_t ha[2]; dm_ic_hands_after(sl, omask, cand, wit[4], wit[5], ha);
                int claws_after = cls == 6 && sim_dual_class(b, cls, ha[0], ha[1]);
                int claws = cls == 6 && dm_custom_get("combat_sim") && (sim_dual_class(b, cls, wit[4], wit[5]) || claws_after);
                for (int m = 0; m < IC_C_N; m++) {
                    if (m == IC_C_BLK && claws) {
                        int bk[2], known = in.hypothetical ? 0 : sim_block_pair(b, pl, cand, olds, nold, sl, omask, composed ? gdelta : NULL, charm, claws_after, bk);
                        if (known < 0) block_late = 1;
                        if (known <= 0 || bk[0] == bk[1]) continue;
                        if (inl) { blk[0] = bk[0]; blk[1] = bk[1]; continue; }
                        static char lab[96]; cb_label(b, m, lab, sizeof lab);
                        dm_ic_cb_line(m, bk[0], 0, bk[1], 0, lab, game_str(b, "max", 0, 8, "dmCmpCap"), dm_str(b, "dmCmpMixed"), dm_str(b, "dmCmpTo"), 0, cbl[ncb++], sizeof cbl[0]);
                        continue;
                    }
                    if (!(R.affected >> m & 1)) continue;
                    static char lab[96]; cb_label(b, m, lab, sizeof lab);

                    const char *reason = R.v.bad >> m & 1 ? R.v.why[m] : R.unproven >> m & 1 ? "dmCmpUnproven" : NULL;
                    if (reason && !strcmp(reason, "dmCmpUnproven")) { unproven |= 1u << m; snprintf(unproven_label[m], sizeof unproven_label[m], "%s", lab); continue; }
                    if (reason && (m == IC_C_AR || m == IC_C_DMG) && !strcmp(reason, "dmCmpPassive")) { attack_passive = 1; continue; }
                    if (reason) { int s_ = 0; for (int j_ = 0; j_ < ncbwhy; j_++) s_ |= !strcmp(cbwhy[j_], reason); if (!s_ && ncbwhy < IC_C_N) cbwhy[ncbwhy++] = reason; continue; }
                    int bmn = R.nowv[m], bmx = m == IC_C_DMG ? R.nowv[IC_C_N] : 0, amn = R.aftv[m], amx = m == IC_C_DMG ? R.aftv[IC_C_N] : 0; if (bmn == amn && bmx == amx) continue;
                    if (inl && m == IC_C_BLK) { blk[0] = bmn; blk[1] = amn; continue; }
                    if ((m == IC_C_AR || m == IC_C_DMG) && atk[0]) { size_t L = strlen(lab); snprintf(lab + L, sizeof lab - L, " (%s)", atk); }
                    dm_ic_cb_line(m, bmn, bmx, amn, amx, lab, game_str(b, "max", 0, 8, "dmCmpCap"), dm_str(b, "dmCmpMixed"), dm_str(b, "dmCmpTo"), 0, cbl[ncb++], sizeof cbl[0]); } }


              if ((nl || nskl || ncb || blk[0] != blk[1] || C.spl_kind == IC_SPLASH_CHANGE) && in.hypothetical) add(K_NOTE, 0, CC "5%s", dm_str(b, "dmCmpHypo"));
              if (!inl) {
                  for (int i = 0; i < nl; i++) if (out[i].metric <= IC_M_MANA) add(K_VAL, 0, "%s", out[i].text);
                  for (int i = 0; i < nskl; i++) add(K_VAL, 0, "%s", skl[i]);
                  if (hidden) add(K_MORE, 0, ({ static char f_[DM_TIPX_LINEB]; char w_[DM_TIPX_LINEB]; snprintf(f_, sizeof f_, CC "5%s", dm_strn(b, "dmCmpSkillMore", hidden, w_, sizeof w_)); f_; }), hidden);
                  for (int i = 0; i < ncb; i++) add(K_VAL, 0, "%s", cbl[i]);
                  if (C.spl_kind == IC_SPLASH_CHANGE) add(K_VAL, 0, "%s", spl);
                  for (int i = 0; i < nl; i++) if (out[i].metric > IC_M_MANA) add(K_VAL, 0, "%s", out[i].text);
              } else {


                  inline_collect(b, it, &in, out, nl, blk[0] != blk[1] ? blk : NULL);
                  for (int i = 0; i < nskl; i++) add(K_VAL, 0, "%s", skl[i]);
                  if (hidden) add(K_MORE, 0, ({ static char f_[DM_TIPX_LINEB]; char w_[DM_TIPX_LINEB]; snprintf(f_, sizeof f_, CC "5%s", dm_strn(b, "dmCmpSkillMore", hidden, w_, sizeof w_)); f_; }), hidden);
                  if (C.spl_kind == IC_SPLASH_CHANGE) add(K_VAL, 0, "%s", spl);
                  for (int i = 0; i < ncb && nsimdep < SIMDEP; i++) snprintf(simdep[nsimdep++], sizeof simdep[0], "%s", cbl[i]);
                  for (int i = 0; i < nl && nsimdep < SIMDEP; i++) if (out[i].metric == IC_M_LL || out[i].metric == IC_M_ML) snprintf(simdep[nsimdep++], sizeof simdep[0], "%s", out[i].text);
              } }
        }
        gear_skip = in.hypothetical;
        gdelta_shown = composed ? gdelta : NULL;
        if (nonum || sim_metrics || cbop) {
            static const char *const map[][2] = { { "dmCmpSet", "dmGearNoSet" }, { "dmCmpBroken", "dmGearNoBroken" }, { "dmCmpAura", "dmGearNoAura" },
                { "dmCmpOldOff", "dmGearNoOldOff" }, { "dmCmpTwoHand", "dmGearNoTwoHand" }, { "dmCmpChainNote", "dmGearNoChain" }, { "dmCmpOp", "dmGearNoOp" },
                { "dmCmpPct", "dmGearNoPct" }, { "dmCmpPassive", "dmGearNoPassive" } };

            int grants_only = !sim_metrics && !cbop;
            for (int i = 0; i < nwhy; i++) grants_only &= !strcmp(why[i], "dmCmpAura");
            for (int i = 0; i < nwhy && !unrated && !grants_only; i++) for (size_t k = 0; k < sizeof map / sizeof *map && !unrated; k++) if (!strcmp(why[i], map[k][0])) unrated = map[k][1];
            if (!unrated && !grants_only) unrated = cbop ? "dmGearNoOp" : "dmGearNoSim";
        }
#undef WHY
        }
    done:;


        int gear = GEAR_OFF, dots = 0, comparison = dm_custom_get("item_cmp");
        if (!comparison) { S.n = S.n && S.l[0].kind == K_HEAD; S.lost = 0; unproven = 0; nwhy = ncbwhy = 0; attack_passive = 0; nsimdep = 0; C.annlen = 0; C.nspan = 0; X.ready = 0; }
        int g0 = S.n;
        if (!gear_skip && dm_custom_get("combat_sim")) {
            if (!unrated) gear = gear_tip(b, pl, h, cand, olds, nold, sl, omask, gdelta_shown, charm, &dots, &unrated);
            else gear = GEAR_UNRATED;

            if (gear == GEAR_UNRATED) { add_unrated(b, unrated); nwhy = 0; }
        }
        if (X.ready) g0 += inline_finish(b, c, gear == GEAR_SHOWN ? gear_shown() : NULL);
        if (gear != GEAR_SHOWN && gear != GEAR_PENDING) {
            for (int i = 0; i < nsimdep && S.n < S_MAX; i++) { S.n = dm_ic_insert(S.l, sizeof S.l[0], S.n, S_MAX, g0); S.l[g0].kind = K_VAL; S.l[g0].flags = 0; snprintf(S.l[g0].s, sizeof S.l[g0].s, "%s", simdep[i]); g0++; } }
        if (gear == GEAR_PENDING || block_late) C.key = 0;
        if (removing || gear != GEAR_SHOWN) dots = 0;
        C.dots = dots;
        if (gear != GEAR_OFF) { unproven = 0; attack_passive = 0; ncbwhy = 0; }
        if (gear == GEAR_SHOWN || gear == GEAR_PENDING) { int k = 0; for (int i = 0; i < nwhy; i++) if (strcmp(why[i], "dmCmpAura")) why[k++] = why[i]; nwhy = k; }
        int g1 = S.n;
        for (int i = 0; i < nwhy; i++) add(K_WHY, 0, CC "5%s", dm_str(b, why[i]));
        for (int i = 0; i < ncbwhy; i++) add(K_WHY, 0, CC "5%s", dm_str(b, cbwhy[i]));
        {
            static typeof(S.l[0]) moved[S_MAX]; int nb = g1 - g0, nn = S.n - g1;
            memcpy(moved, &S.l[g0], (size_t)nb * sizeof S.l[0]); memmove(&S.l[g0], &S.l[g1], (size_t)nn * sizeof S.l[0]); memcpy(&S.l[g0 + nn], moved, (size_t)nb * sizeof S.l[0]);
        }
        int at = g0 + (S.n - g1), attack = 0;
        if(attack_passive && !attack && S.n<S_MAX) {
            char line[DM_TIPX_LINEB];snprintf(line,sizeof line,CC "5%s",dm_str(b,"dmCmpPassive"));int seen=0;
            for(int i=0;i<S.n;i++)seen|=!strcmp(S.l[i].s,line);
            if(!seen){S.n=dm_ic_insert(S.l,sizeof S.l[0],S.n,S_MAX,at);S.l[at].kind=K_WHY;S.l[at].flags=0;snprintf(S.l[at].s,sizeof S.l[at].s,"%s",line);}
        }
        const char *unproven_names[IC_C_N];for(int m=0;m<IC_C_N;m++)unproven_names[m]=unproven_label[m];
        char unproven_text[DM_TIPX_LINEB];
        if(dm_ic_unproven_text(unproven_text,sizeof unproven_text,dm_str(b,"dmCmpUnprovenOf"),unproven,attack,unproven_names) && S.n<S_MAX) {
            S.n=dm_ic_insert(S.l,sizeof S.l[0],S.n,S_MAX,at);
            S.l[at].kind=K_WHY;S.l[at].flags=0;snprintf(S.l[at].s,sizeof S.l[at].s,CC "5%s",unproven_text);
        } else if(S.n && S.l[0].kind==K_HEAD && !dm_ic_head_stays(removing,at-1,S.n-at,(attack_passive && !attack) || C.nspan)){memmove(&S.l[0],&S.l[1],(size_t)(S.n-1)*sizeof S.l[0]);S.n--;}
        if (!removing && comparison) {
            int dm_rw_bases_for(uint8_t *, uint64_t, char *, size_t); static char names[DM_TIPX_LINEB], line[DM_TIPX_LINEB];
            if (dm_rw_bases_for(b, it, names, sizeof names - 16) > 0) { snprintf(line, sizeof line - 8, dm_str(b, "dmGearBases"), names); add(K_NOTE, 0, CC "4%s", line); }
        }
        if (C.spl_kind == IC_SPLASH_OWN) splash_own(spl);
        pack(b);
        static int lg; if (lg++ < 30) LOG("物品对比：%s 物品 id %u 栏位 %d（%s）→ %d 行（暂存 %d、丢 %d）", unequip ? "卸下" : charm == IC_CHARM_OUT ? "护符拿出" : charm == IC_CHARM_IN ? "护符放入" : "穿上", *(uint32_t *)(it + 8), sl, c->pad ? "手柄" : "触屏", C.n, S.n, S.lost);
    }
out:;
    *key = dm_ic_splash_key(*key, splash_on, C.spl_kind, C.spl_share, C.spl_width);
    int n = C.n < max ? C.n : max; for (int i = 0; i < n; i++) { ln[i].s = C.txt[i]; ln[i].flags = C.flags[i]; }
    return n;
}

int dm_itemcmp_dots(void) { return C.dots; }
const char *dm_itemcmp_marks(const dm_tipx_span **span, int *nspan) { *span = C.span; *nspan = C.nspan; return C.annlen && C.nspan && C.cur && C.ann_key == C.cur ? C.ann : NULL; }




static void probe_records(const char *where) {
    static uint64_t sig[2] = { ~0ull, ~0ull }; int wi = where[0] == 'r'; uint64_t arr = *(uint64_t *)(g_panel + 0x168), n = *(uint64_t *)(g_panel + 0x170); uint64_t s = n * 131 + (n && arr ? *(uint64_t *)(arr + 8) * 7 + *(uint32_t *)(arr + 0x6c) : 0);
    if (s == sig[wi]) return; sig[wi] = s;
    if (!n || !arr) { LOG("[cmp] 面板记录（%s）0 条", where); return; }
    for (uint64_t i = 0; i < n && i < 3; i++) { uint64_t r = arr + 0x2e8 * i; const int32_t *q = (const int32_t *)(uintptr_t)(r + 0x60); uint64_t p = *(uint64_t *)r;
        static char e[160]; esc(e, sizeof e, (const char *)(uintptr_t)p, *(uint64_t *)(r + 8) < 120 ? *(uint64_t *)(r + 8) : 120);
        LOG("[cmp] 面板记录（%s）%llu/%llu：字指针 %#llx%s 长 %llu 框 %d %d %d %d %d %d %d %d 字号 %d %d「%s」", where, (unsigned long long)i + 1, (unsigned long long)n, (unsigned long long)p,
            p >= (uint64_t)(uintptr_t)g_mark && p < (uint64_t)(uintptr_t)g_mark + sizeof g_mark ? "（= 我们的缓冲！）" : "（不是我们的缓冲）", (unsigned long long)*(uint64_t *)(r + 8), q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7], *(int32_t *)(r + 0x80), *(int32_t *)(r + 0x84), e); }
}
void dm_itemcmp_render_probe(void) {
    if (g_spike <= 0 || !g_panel) return;
    probe_records("render");
    if (g_spike == 2) memset(g_mark, 'X', 48);
}
static int is_filler(uint8_t *b, uint64_t it) { uint32_t c = item_code(b, it); const char *k = (const char *)&c; return (k[0] == 'g' && k[1] != 'l') || (k[0] == 'r' && k[1] >= '0' && k[1] <= '9') || (k[0] == 'j' && k[1] == 'e' && k[2] == 'w'); }
#ifndef DM_PLAYER_BUILD









int dm_test_peel_stats(uint8_t *b, uint64_t u) {
    (void)b;
    uint64_t sl = u ? *(uint64_t *)(u + 0x88) : 0; if (!sl) return 0;
    uint64_t ba = *(uint64_t *)(sl + 0x30), bc = *(uint64_t *)(sl + 0x38);
    uint64_t fa = *(int32_t *)(sl + 0x1c) < 0 ? *(uint64_t *)(sl + 0xe8) : 0, fc = fa ? *(uint64_t *)(sl + 0xf0) : 0;
    if (!fa || bc > 64 || fc > 64) return 0;
    static uint32_t nk[65]; static int32_t nv[65]; int n = 0;
    for (uint64_t i = 0; i < bc; i++) { nk[n] = *(uint32_t *)(ba + 8 * i); nv[n] = *(int32_t *)(ba + 8 * i + 4); n++; }
    for (uint64_t i = 0; i < fc; i++) { uint32_t k = *(uint32_t *)(fa + 8 * i);
        if ((int)(k >> 16) != 194) continue;
        int dup = 0; for (int j = 0; j < n && !dup; j++) dup = nk[j] == k;
        if (!dup) { nk[n] = k; nv[n] = *(int32_t *)(fa + 8 * i + 4); n++; } }
    for (int i = 1; i < n; i++) { uint32_t k = nk[i]; int32_t v = nv[i]; int j = i - 1;
        while (j >= 0 && nk[j] > k) { nk[j + 1] = nk[j]; nv[j + 1] = nv[j]; j--; } nk[j + 1] = k; nv[j + 1] = v; }
    if (n > (int)fc) return 0;
    if (n == (int)fc) { int i = 0; while (i < n && *(uint32_t *)(fa + 8 * i) == nk[i] && *(int32_t *)(fa + 8 * i + 4) == nv[i]) i++;
        if (i == n) return 0; }
    for (int i = 0; i < n; i++) { *(uint32_t *)(fa + 8 * i) = nk[i]; *(int32_t *)(fa + 8 * i + 4) = nv[i]; }
    *(uint64_t *)(sl + 0xf0) = (uint64_t)n;
    return 1;
}

static void mk_peel(uint8_t *b, uint64_t u, int q) {
    uint64_t d = *(uint64_t *)(u + 0x10); if (!d) return;
    int n = dm_test_peel_stats(b, u);
    int q0 = (int)*(uint32_t *)d, pfx = *(uint16_t *)(d + 0x42), sfx = *(uint16_t *)(d + 0x44);
    if (q0 == q && !pfx && !sfx && !n) return;
    *(uint32_t *)d = (uint32_t)q; *(uint16_t *)(d + 0x42) = 0; *(uint16_t *)(d + 0x44) = 0;
    LOG("[cmp] 带孔物品：剥掷 id %u 品质 %d→%d、前后缀 %d/%d、累计表重写 %d", *(uint32_t *)(u + 8), q0, q, pfx, sfx, n);
}






static struct { uint32_t id; int q; } g_peel_ids[8];
static int g_peel_n;
void dm_test_peel_client(uint8_t *b) {
    int n = __atomic_load_n(&g_peel_n, __ATOMIC_ACQUIRE);
    for (int k = n - 1; k >= 0; k--) { uint32_t id = g_peel_ids[k].id;
        uint64_t u = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + GA_UNIT_FIND))((uint64_t)(uintptr_t)(b + GA_CLIENT_UNIT_TABLE + 4 * 0x400), id & 0x7f, id, 4);
        if (!u) continue; uint32_t mode = *(uint32_t *)(u + 0xc);
        if (mode != 0 && mode != 1) continue;
        int p = dm_test_peel_stats(b, u);
        uint64_t d = *(uint64_t *)(u + 0x10);
        int was = d ? (int)*(uint32_t *)d : g_peel_ids[k].q; uint16_t pfx = d ? *(uint16_t *)(d + 0x42) : 0, sfx = d ? *(uint16_t *)(d + 0x44) : 0;
        if (d) { *(uint32_t *)d = (uint32_t)g_peel_ids[k].q; *(uint16_t *)(d + 0x42) = 0; *(uint16_t *)(d + 0x44) = 0; }
        if (p || was != g_peel_ids[k].q || pfx || sfx) LOG("[cmp] 带孔物品：客户端剥掷 id %u 品质 %d、累计表重写 %d", id, was, p);
        g_peel_ids[k] = g_peel_ids[n - 1]; n--; __atomic_store_n(&g_peel_n, n, __ATOMIC_RELEASE); }
}
void dm_test_peel_register(uint32_t id, int q) {
    int k = __atomic_load_n(&g_peel_n, __ATOMIC_RELAXED); if (k >= 8) return;
    g_peel_ids[k].id = id; g_peel_ids[k].q = q; __atomic_store_n(&g_peel_n, k + 1, __ATOMIC_RELEASE);
}
#endif














static void mk_socketed(uint64_t G, uint64_t player, uint8_t *b) {
    static int done; const char *e = getenv("DM_TEST_ITEMCMP_MK"); if (done || !e) return;
    char buf[200]; strlcpy(buf, e, sizeof buf); char *ap = strrchr(buf, '@'); uint32_t at = ap ? (uint32_t)atoi(ap + 1) : 60; if (ap) *ap = 0;
    if (*(uint32_t *)(G + 0x170) < at) return; done = 1;
    uint64_t T = *(uint64_t *)(b + GA_DATA_TABLES + *(uint8_t *)(G + 0x106) * 16), ix = T ? *(uint64_t *)(T + 0x258) : 0, ar = ix ? *(uint64_t *)ix : 0, cnt = ix ? *(uint64_t *)(ix + 8) : 0;
    for (char *sv, *t = strtok_r(buf, ",", &sv); t; t = strtok_r(NULL, ",", &sv)) { char code4[5] = "    "; int q = 2, il = 20, socks = -1;
        if (sscanf(t, "%4[^:]:%d:%d=%d", code4, &q, &il, &socks) < 1) continue;
        for (int k = (int)strlen(code4); k < 4; k++) code4[k] = ' '; uint32_t code; memcpy(&code, code4, 4); int cls = -1;
        for (uint64_t k = 0; ar && k < cnt; k++) if (*(uint32_t *)(ar + k * 8) == code) { cls = *(int32_t *)(ar + k * 8 + 4); break; }
        if (cls < 0) { LOG("[cmp] 带孔物品：码 %.4s 查不到", code4); continue; }
        uint64_t pth = *(uint64_t *)(player + 0x38), room0 = pth ? *(uint64_t *)(pth + 0x20) : 0;
        int32_t p0[2] = { 0, 0 }, p1[2] = { 0, 0 }; ((void (*)(uint64_t, int32_t *))(b + GA_UNIT_GET_POS))(player, p0);
        uint64_t room = ((uint64_t (*)(uint64_t, uint64_t, int32_t *, int32_t *, int))(b + GA_ROOM_FIND_DROP_SPOT))(G, room0, p0, p1, 1);
        static uint8_t prm[0xd8]; memset(prm, 0, sizeof prm);
        *(uint64_t *)(prm + 0) = player; *(uint64_t *)(prm + 0x10) = G; *(int32_t *)(prm + 0x18) = il; *(int32_t *)(prm + 0x20) = cls; *(int32_t *)(prm + 0x24) = 3;
        *(int32_t *)(prm + 0x28) = p1[0]; *(int32_t *)(prm + 0x2c) = p1[1]; *(uint64_t *)(prm + 0x30) = room; *(uint16_t *)(prm + 0x38) = 1; *(uint16_t *)(prm + 0x3a) = *(uint16_t *)(G + 0x128); *(int32_t *)(prm + 0x40) = q;
        *(uint32_t *)(prm + 0x54) = socks == 0 ? 0 : 0x800;
        { const char *eq = strchr(t, '='); if (eq && strchr(eq, 'e')) *(uint32_t *)(prm + 0x54) |= 0x400000; }





        uint64_t u = room ? ((uint64_t (*)(uint64_t, void *, int))(b + GA_ITEM_SPAWN))(G, prm, 0) : 0;
        uint64_t ip = u ? *(uint64_t *)(u + 0x38) : 0; int inv = 0;
        if (u) {
#ifndef DM_PLAYER_BUILD
            mk_peel(b, u, q);
            dm_test_peel_register(*(uint32_t *)(u + 8), q);
            char code3[5] = ""; sscanf(code4, "%4s", code3);
            dm_test_give_stats_server(b, u, code3);
#endif
            uint8_t pk[17] = { 0x16 }; uint32_t one = 1; memcpy(pk + 1, (void *)(u + 8), 4); if (ip) { memcpy(pk + 5, (void *)(ip + 0x10), 4); memcpy(pk + 9, (void *)(ip + 0x14), 4); } memcpy(pk + 13, &one, 4);
            *(uint32_t *)(G + 0x19050) = 1; ((void (*)(uint64_t, uint64_t, uint8_t *, uint32_t))(b + GA_C2S_DISPATCH))(G, player, pk, 17); *(uint32_t *)(G + 0x19050) = 0; inv = *(uint32_t *)(u + 0xc) != 3; }
        LOG("[cmp] 带孔物品：%.4s 品质 %d 物品等级 %d → %s（id %u，孔 %d）%s", code4, q, il, u ? "成" : "【红】败", u ? *(uint32_t *)(u + 8) : 0, u ? gstat(b, u, 194, 0) : 0, u && !inv ? "，【注意】没捡进背包" : ""); }
}
void dm_itemcmp_srv_tick(uint64_t G, uint64_t player, uint8_t *b) {
    if (!player || !b) return; mk_socketed(G, player, b);
    int v = atomic_exchange(&g_req_base, -1); if (v >= 0) { ((void (*)(uint64_t, int, int, int))(b + GA_STAT_SET))(player, 0, v, 0); LOG("[cmp] 验算台（服务端）：基础力量设成 %d", v); }
}









static int nkids(uint64_t it) { int n = 0; for (uint64_t c = first_item(it); c && n < 8; c = next_item(c)) n++; return n; }
static void grid_xy(uint64_t it, int *x, int *y) { uint64_t pp = *(uint64_t *)(it + 0x38); *x = pp ? *(int32_t *)(pp + 0x10) : 0; *y = pp ? *(int32_t *)(pp + 0x14) : 0; }


static int sock_use(uint32_t code) {
    static int got; static char list[128]; if (!got) { got = 1; const char *e = getenv("DM_TEST_ITEMCMP_SOCK_USE"); if (e) strlcpy(list, e, sizeof list); }
    if (!list[0]) return 1;
    char c[4]; memcpy(c, &code, 3); c[3] = 0;
    for (const char *p = list; *p; ) { size_t n = strcspn(p, ","); if (n == strlen(c) && !strncmp(p, c, n)) return 1; p += n; if (*p) p++; }
    return 0;
}
static void sock_pump(uint8_t *b, uint64_t pl) {
    static int at = -2, st, fx, fy, nk; static uint32_t fid, gid; static uint64_t start, t0; if (at == -2) at = env_int("DM_TEST_ITEMCMP_SOCK", -1);
    if (at < 0 || !pl) return; uint64_t now = now_ns(); if (!start) start = now; if (now - start < (uint64_t)at * 1000000000ull) return;
    void dm_rw_send(uint8_t *, const uint8_t *, int);
    int dm_ui_panel_shown(const char *);
    if (st == 0) {

        if (held_item(pl)) return; uint64_t gear = 0, fill = 0;
        if (!dm_ui_panel_shown("PlayerInventoryExpansionLayout")) { static int said; if (!said++) LOG("[cmp] 镶孔：等背包面板打开"); if (now - start > (uint64_t)(at + 120) * 1000000000ull) { LOG("[cmp] 【红】镶孔：背包面板 2 分钟没开，停"); at = -1; } return; }
        for (uint64_t q = first_item(pl); q; q = next_item(q)) { if (!inv_item(q)) continue;
            if (is_filler(b, q)) { uint32_t c = item_code(b, q), fc = fill ? item_code(b, fill) : 0; if (sock_use(c) && (!fill || memcmp(&c, &fc, 4) < 0)) fill = q; continue; }
            int fr = gstat(b, q, 194, 0) - nkids(q); if (fr > 0 && (!gear || fr > gstat(b, gear, 194, 0) - nkids(gear))) gear = q; }
        { uint64_t g0 = gid ? item_by_id(b, gid) : 0; if (g0 && inv_item(g0) && gstat(b, g0, 194, 0) > nkids(g0)) gear = g0; }
        if (!gear || !fill) { if (now - start < (uint64_t)(at + 40) * 1000000000ull) return;
            LOG("[cmp] 镶孔：没有%s了，停", !gear ? "带空孔的装备" : "能镶的宝石 / 符文 / 珠宝"); at = -1; return; }
        fid = *(uint32_t *)(fill + 8); gid = *(uint32_t *)(gear + 8); nk = nkids(gear); grid_xy(fill, &fx, &fy);
        uint8_t p[17] = { 0x19 }; uint32_t v[4] = { fid, (uint32_t)fx, (uint32_t)fy, 0 }; memcpy(p + 1, v, 16); dm_rw_send(b, p, 17); st = 1; t0 = now; return; }
    if (st == 1) {
        uint64_t h = held_item(pl); if (!h || *(uint32_t *)(h + 8) != fid) { if (now - t0 > 3000000000ull) { LOG("[cmp] 【红】镶孔：id %u 3 秒没到手上，停", fid); at = -1; } return; }
        uint64_t g = item_by_id(b, gid); if (!g) { LOG("[cmp] 【红】镶孔：底材 id %u 不见了，停", gid); at = -1; return; }
        int gx, gy; grid_xy(g, &gx, &gy); uint8_t p[21] = { 0x28 }; uint32_t v[4] = { fid, gid, 0, 0 }, w5 = (uint32_t)(uint16_t)gx | (uint32_t)(uint16_t)gy << 16;
        memcpy(p + 1, v, 16); memcpy(p + 17, &w5, 4); dm_rw_send(b, p, 21); st = 2; t0 = now; return; }
    uint64_t f = item_by_id(b, fid), g = item_by_id(b, gid);
    if (f && g && *(uint32_t *)(f + 0xc) == 6 && nkids(g) > nk) { uint32_t fc = item_code(b, f), gc = item_code(b, g); uint64_t d = *(uint64_t *)(g + 0x10);
        LOG("[cmp] 镶孔：%.3s（id %u，孔 %d/%d）← %.3s（id %u）镶上%s", (const char *)&gc, gid, nkids(g), gstat(b, g, 194, 0), (const char *)&fc, fid, d && (*(uint32_t *)(d + 0x18) & 0x04000000) ? "，成了符文之语" : ""); st = 0; return; }
    if (now - t0 < 3000000000ull) return;
    if (held_item(pl) && *(uint32_t *)(held_item(pl) + 8) == fid) { uint8_t p[17] = { 0x18 }; uint32_t v[4] = { fid, (uint32_t)fx, (uint32_t)fy, 0 }; memcpy(p + 1, v, 16); dm_rw_send(b, p, 17); }
    LOG("[cmp] 【红】镶孔：id %u 3 秒没进底材 id %u 的孔，放回原格，停", fid, gid); at = -1;
}
#ifndef DM_PLAYER_BUILD



static int carried(uint64_t pl, uint32_t id) { for (uint64_t q = first_item(pl); q; q = next_item(q)) if (*(uint32_t *)(q + 8) == id) return 1; uint64_t h = held_item(pl); return h && *(uint32_t *)(h + 8) == id; }
static void drop_charm_pump(uint8_t *b, uint64_t pl) {
    static int at = -2, st; static uint32_t id; static uint64_t start, t0; if (at == -2) at = env_int("DM_TEST_DROP_CHARM", -1);
    if (at < 0 || !pl) return; uint64_t now = now_ns(); if (!start) start = now; if (now - start < (uint64_t)at * 1000000000ull) return;
    void dm_rw_send(uint8_t *, const uint8_t *, int);
    int dm_ui_panel_shown(const char *);
    if (st == 0) {
        if (held_item(pl) || dm_ui_panel_shown("PlayerInventoryExpansionLayout")) return;
        uint64_t charm = 0; for (uint64_t q = first_item(pl); q && !charm; q = next_item(q)) if (inv_item(q) && is_type(b, q, 13)) charm = q;
        if (!charm) { LOG("DM_TEST_DROP_CHARM: no charm in the inventory"); at = -1; return; }
        int x, y; grid_xy(charm, &x, &y); id = *(uint32_t *)(charm + 8);
        LOG("DM_TEST_DROP_CHARM: charm %u at %d,%d counted %d, to the cursor", id, x, y, item_effective(b, pl, charm));
        uint8_t p[17] = { 0x19 }; uint32_t v[4] = { id, (uint32_t)x, (uint32_t)y, 0 }; memcpy(p + 1, v, 16); dm_rw_send(b, p, 17); st = 1; t0 = now; return; }
    if (st != 1) return;
    if (!carried(pl, id)) { LOG("DM_TEST_DROP_CHARM: charm %u dropped", id); st = 2; return; }
    if (now - t0 > 5000000000ull) { LOG("DM_TEST_DROP_CHARM: charm %u still with the player after 5 s", id); st = 2; }
}
#else
#define drop_charm_pump(b, pl) ((void)0)
#endif
#include "dm_gear_stock.inc"
#include "dm_gear_pages.inc"

void dm_itemcmp_pump(uint8_t *b, uint64_t pl) {
    if (!b) return;
    { static int td = -2; if (td == -2) { const char *e = getenv("DM_TEST_ITEMCMP_DIFF"); td = e ? atoi(e) : -1; }
      if (td >= 0 && td <= 2 && pl) { LOG("[cmp] 测试 DM_TEST_ITEMCMP_DIFF：客户端难度字节 %u → %d", *(uint8_t *)(b + GA_DIFFICULTY), td); *(uint8_t *)(b + GA_DIFFICULTY) = (uint8_t)td; td = -1; } }
    if (g_spike > 0 && g_panel) probe_records("pump");
    sock_pump(b, pl);
    drop_charm_pump(b, pl);
    bench_pump(b, pl);
    gear_tab_pump(b, pl);
    if (pl) gear_tables();
}
