

































#include "dm.h"
#include "dm_custom.h"
#include "dm_hud.h"
#include "dm_unit.h"
#include <TargetConditionals.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int16_t skill, state; int8_t grp, intown; const char *nm; } ab_def;
static const ab_def k_ab[] = {
    {  40,  10, 1, 1, "寒冰装甲" }, {  50,  88, 1, 1, "碎冰甲" }, {  60,  20, 1, 1, "寒冰护甲" }, {  52,  16, 0, 1, "魔法附加" }, {  58,  30, 0, 1, "能量护盾" },
    {  68,  14, 0, 1, "骨甲" }, { 117, 101, 0, 1, "圣盾" },
    { 155,  51, 0, 0, "战斗指挥" }, { 149,  32, 0, 0, "战斗体制" }, { 138,  26, 0, 0, "大叫" },
    { 235, 151, 0, 1, "飓风装甲" }, { 258, 157, 2, 1, "速度爆发" }, { 267, 159, 2, 1, "能量消解" }, { 278,  31, 0, 1, "毒牙" },
    { 385, 198, 6, 1, "Hex Bane" }, { 386, 200, 6, 1, "Hex Siphon" }, { 389, 202, 6, 1, "Hex Purge" }, { 387, 206, 0, 1, "Psychic Ward" },
    {  57,  38, 0, 0, "雷云风暴" }, { 277, 158, 0, 0, "刀刃之盾" },
};


static const struct { int16_t skill; const char *nm, *why; } k_skip[] = {
    { 381, "吞噬 Consume", "有负面作用，产品方点名不自动放" },
};






static const struct { int16_t skill, need, excl; } k_equip[] = { { 117, 51, 0 }, { 385, 45, 69 }, { 386, 45, 69 }, { 389, 45, 69 } };
static uint32_t g_hand_sig;
static int equip_ok(uint8_t *b, uint64_t pl, int skill) {
    int need = 0, excl = 0; for (unsigned i = 0; i < sizeof k_equip / sizeof *k_equip; i++) if (k_equip[i].skill == skill) { need = k_equip[i].need; excl = k_equip[i].excl; }
    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; int ok = need == 0; uint32_t sig = 0;
    for (int k = 0; it && k < 512; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; int loc = *(uint8_t *)(d + 0x54);
        if (*(uint32_t *)(it + 0xc) == 1 && (loc == 4 || loc == 5)) { sig += *(uint32_t *)(it + 8) * (uint32_t)(loc == 4 ? 3 : 7);
            int (*is)(uint64_t, int) = (int (*)(uint64_t, int))(b + 0x574bc0); if (need && is(it, need) && !(excl && is(it, excl))) ok = 1; }
        it = *(uint64_t *)(d + 0xb0); }
    g_hand_sig = sig; return ok;
}


static int swap_gives(uint8_t *b, uint64_t pl, int skill) {
    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
    for (int k = 0; it && k < 512; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; int loc = *(uint8_t *)(d + 0x54);
        if (*(uint32_t *)(it + 0xc) == 1 && (loc == 11 || loc == 12) && ((int (*)(uint64_t, int, int))(b + 0x5b9290))(it, 97, skill) > 0) return 1;
        it = *(uint64_t *)(d + 0xb0); }
    return 0;
}


static int enemy_near(uint8_t *b, uint64_t pl, int r) {
    uint64_t pp = *(uint64_t *)(pl + 0x38); if (!pp) return 0; int px = *(uint16_t *)(pp + 2), py = *(uint16_t *)(pp + 6), best = -1;
    for (int k = 0; k < 128; k++) for (uint64_t u = *(uint64_t *)(b + 0x2d603a0 + 1 * 0x400 + k * 8); u; u = *(uint64_t *)(u + 0x158)) {
        uint32_t m = *(uint32_t *)(u + 0xc); uint64_t up = *(uint64_t *)(u + 0x38); if (m == 0 || m == 12 || !up) continue;
        int dx = abs(*(uint16_t *)(up + 2) - px), dy = abs(*(uint16_t *)(up + 6) - py), d = dx > dy ? dx : dy; if (d > r) continue;
        if (((int (*)(int, uint64_t, uint64_t))(b + 0x2f7b7c))(0x165, pl, u) != 1) continue;
        if (best < 0 || d < best) best = d; }
    return best;
}
static int lr_skill(uint64_t pl, int right) { uint64_t sl = *(uint64_t *)(pl + 0x100), n = sl ? *(uint64_t *)(sl + (right ? 0x10 : 8)) : 0; return n && *(uint64_t *)n ? *(int16_t *)*(uint64_t *)n : -1; }
static int skipped(int skill) { if (getenv("DM_TEST_BUFF_NOSKIP")) return 0;                                                                                                                                                                                  for (unsigned i = 0; i < sizeof k_skip / sizeof *k_skip; i++) if (k_skip[i].skill == skill) return 1; return 0; }
#define NAB (int)(sizeof k_ab / sizeof *k_ab)
uint32_t dm_cur_tick(void);

static int enabled(void) { static int env = -2; if (env == -2) { const char *e = getenv("DM_AUTOBUFF"); env = e ? e[0] != '0' : -1; }
    const char *app = getenv("DM_APP");
    return env >= 0 ? env : ((DM_PHONE || (app && app[0] && app[0] != '0')) && dm_custom_get("auto_buff")); }
static int cfg(const char *nm, int def) { const char *e = getenv(nm); return e && atoi(e) > 0 ? atoi(e) : def; }
static int has_state(uint64_t u, int st) {
    uint64_t sl = *(uint64_t *)(u + 0x88); if (!sl || !(*(uint32_t *)(sl + 0x1c) >> 31)) return 0;
    return (*(uint32_t *)(sl + 0xb30 + 4 * (uint64_t)(st >> 5)) >> (st & 31)) & 1;
}


static void stat_line(uint8_t *b, uint64_t u, char *o, size_t n) { int (*gs)(uint64_t, int, int) = (void *)(b + 0x5b9290);
    snprintf(o, n, "防御 31=%d、16=%d、171=%d，命中 19=%d、119=%d，伤害%% 25=%d", gs(u, 31, 0), gs(u, 16, 0), gs(u, 171, 0), gs(u, 19, 0), gs(u, 119, 0), gs(u, 25, 0)); }
static uint64_t own_node(uint64_t pl, int skill) {
    uint64_t sl = *(uint64_t *)(pl + 0x100);
    for (uint64_t n = sl ? *(uint64_t *)sl : 0, k = 0; n && k < 512; n = *(uint64_t *)(n + 8), k++) { uint64_t rec = *(uint64_t *)n; if (rec && *(int16_t *)rec == skill && *(int32_t *)(n + 0x44) == -1) return n; }
    return 0;
}


static int eff_level(uint8_t *b, uint64_t pl, uint64_t node) { return *(int32_t *)(node + 0x38) + ((int (*)(uint64_t, uint64_t, int))(b + 0x5b1710))(pl, node, 0); }
static int mana_cost(uint64_t node, int lvl) {
    uint64_t rec = *(uint64_t *)node; int sh = *(uint8_t *)(rec + 0x228), base = *(int16_t *)(rec + 0x22a), per = *(int16_t *)(rec + 0x22c), mn = *(int16_t *)(rec + 0x224);
    int c = ((lvl - 1) * per + base) << sh; if (c < 0) c = 0; if ((mn << 8) > c) c = mn << 8; return c;
}

static uint8_t g_fail[NAB], g_stop[NAB]; static int g_last[8]; static uint32_t g_try[NAB];
static _Atomic int g_req = -1, g_busy, g_unusable = -1;
enum { W_IDLE, W_OUT, W_CAST, W_BACK };
static int w_ph, w_from, w_tries, w_list[6], w_n, w_life0, w_lr0[2]; static uint32_t w_t0, w_start, w_next; static uint8_t w_fail[NAB]; static uint32_t w_until[NAB]; static int w_moved;
int dm_swapskill_wset(void); int dm_swapskill_busy(void); static int g_wait = -1, g_mana0; static uint32_t g_wait_until;





static void test_learn(uint8_t *b, uint64_t unit, const char *side) {
    const char *e = getenv("DM_TEST_BUFF_LEARN"); if (!e || !unit) return; char bf[160]; strlcpy(bf, e, sizeof bf);
    for (char *sv, *k = strtok_r(bf, ",", &sv); k; k = strtok_r(NULL, ",", &sv)) { int sk = 0, lv = 1; if (sscanf(k, "%d:%d", &sk, &lv) < 1 || sk <= 0) continue;
        uint64_t n = own_node(unit, sk); if (!n) n = ((uint64_t (*)(uint64_t, int))(b + 0x7bf010))(unit, sk);
        if (n && *(int32_t *)(n + 0x38) < lv) { *(int32_t *)(n + 0x38) = lv; LOG("测试 DM_TEST_BUFF_LEARN：%s玩家技能 %d 基础等级设成 %d", side, sk, lv); } }
}


static uint32_t w_said[8];
static int w_due(uint8_t *b, uint64_t pl, const int *has, uint32_t t, int *cooling) {
    int n = 0; if (cooling) *cooling = 0;
    for (int i = 0; i < NAB; i++) { if (has[i] || g_stop[i] || skipped(k_ab[i].skill)) continue; uint64_t node = own_node(pl, k_ab[i].skill); if (node && eff_level(b, pl, node) > 0) continue;
        if (!swap_gives(b, pl, k_ab[i].skill)) continue; if (t < w_until[i]) { if (cooling) (*cooling)++; } else n++; }
    return n;
}
static void w_why(uint8_t *b, uint64_t pl, const int *has, uint32_t t, int why, const char *txt) {
    if (why < 0 || why >= 8 || (w_said[why] && t - w_said[why] < 250) || !w_due(b, pl, has, t, NULL)) return;
    w_said[why] = t; LOG("增益自动补：副手组的增益要补，但%s", txt);
}


void dm_autobuff_pump(uint8_t *b, uint64_t pl) {
    static uint64_t lp; static uint32_t lt;
    if (!pl || !enabled()) { lp = 0; return; }
    uint32_t t = dm_cur_tick(); if (!t) return;
    { static uint64_t tl; if (tl != pl && t > 40) { tl = pl; test_learn(b, pl, "客户端"); } }



    { static int at = -1, at2; static char code[5], code2[5]; if (at < 0) { const char *e = getenv("DM_TEST_EQUIP"); if (!e || sscanf(e, "%d:%4[^,],%d:%4s", &at, code, &at2, code2) < 2) at = 0; }
      if (!at && at2 > 0) { at = at2; at2 = 0; memcpy(code, code2, sizeof code); }
      if (at > 0 && t >= (uint32_t)at) { at = 0; uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; uint32_t iid = 0;
          for (int k = 0; it && k < 512 && !iid; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(it);
              if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && !memcmp(&c, code, strlen(code))) iid = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
          void dm_item_equip(uint32_t); if (iid) dm_item_equip(iid); int (*gs)(uint64_t, int, int) = (void *)(b + 0x5b9290);
          LOG("测试 DM_TEST_EQUIP：拍 %u 背包里的 %s id %u → 交原版装备；此刻生命 %d / 上限 %d", t, code, iid, gs(pl, 6, 0) >> 8, gs(pl, 7, 0) >> 8); } }


    { static int at = -1; static char code[5]; if (at < 0) { const char *e = getenv("DM_TEST_UNEQUIP"); if (!e || sscanf(e, "%d:%4s", &at, code) != 2) at = 0; }
      if (at > 0 && t >= (uint32_t)at) { at = 0; uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; uint32_t iid = 0;
          for (int k = 0; it && k < 512 && !iid; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(it);
              if (*(uint32_t *)(it + 0xc) == 1 && !memcmp(&c, code, strlen(code))) iid = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
          void dm_item_unequip(uint32_t); if (iid) dm_item_unequip(iid);
          LOG("DM_TEST_UNEQUIP: tick %u worn %s id %u -> native unequip", t, code, iid); } }
    if (pl != lp || t < lt) {
        lp = pl; memset(g_fail, 0, sizeof g_fail); memset(g_stop, 0, sizeof g_stop); memset(g_try, 0, sizeof g_try); for (int i = 0; i < 8; i++) g_last[i] = -1;
        g_wait = -1; atomic_store(&g_req, -1); w_ph = W_IDLE; w_next = 0; memset(w_fail, 0, sizeof w_fail); memset(w_until, 0, sizeof w_until); }
    { static uint64_t dumped; if (dumped != pl && t > 60 && getenv("DM_TEST_BUFF_DUMP")) { dumped = pl; char bf[900]; int o = 0; bf[0] = 0;
        for (int i = 0; i < NAB && o < 860; i++) { uint64_t n = own_node(pl, k_ab[i].skill); if (n) o += snprintf(bf + o, sizeof bf - o, " %s(%d)基础%d有效%d", k_ab[i].nm, k_ab[i].skill, *(int32_t *)(n + 0x38), eff_level(b, pl, n)); }
        uint64_t sl = *(uint64_t *)(pl + 0x100); int nn = 0; for (uint64_t n = sl ? *(uint64_t *)sl : 0; n && nn < 512; n = *(uint64_t *)(n + 8)) nn++;
        LOG("增益自动补读数：模式 %u、关卡 %d、技能链 %d 个节点，名单里有：%s", *(uint32_t *)(pl + 0xc), dm_unit_level(pl), nn, o ? bf : " 无"); } }
    if (t == lt) return; lt = t;
    int has[NAB];
    for (int i = 0; i < NAB; i++) { has[i] = has_state(pl, k_ab[i].state); if (has[i]) { if (k_ab[i].grp) g_last[(int)k_ab[i].grp] = i; g_fail[i] = 0; } }
    { int u = atomic_exchange(&g_unusable, -1); if (u >= 0) { g_stop[u] = 1; if (g_wait == u) g_wait = -1; LOG("增益自动补：「%s」此刻用不了（原版没进施法：缺装备条件等），本局不再补它", k_ab[u].nm); } }
    if (g_wait >= 0 && atomic_exchange(&g_busy, 0)) { g_try[g_wait] = 0; g_wait = -1; return; }
    if (g_wait >= 0) {
        if (has[g_wait]) { uint64_t sl = *(uint64_t *)(pl + 0x100), rn = sl ? *(uint64_t *)(sl + 0x10) : 0; LOG("增益自动补：拍 %u「%s」已补上（法力 %d → %d；客户端右键技能 %d、左键技能 %d）", t, k_ab[g_wait].nm, g_mana0 >> 8, ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 8, 0) >> 8, rn && *(uint64_t *)rn ? *(int16_t *)*(uint64_t *)rn : -1, sl && *(uint64_t *)(sl + 8) && *(uint64_t *)*(uint64_t *)(sl + 8) ? *(int16_t *)*(uint64_t *)*(uint64_t *)(sl + 8) : -1); g_wait = -1; }
        else if (t >= g_wait_until) { int i = g_wait; g_wait = -1; g_fail[i]++;
            if (g_fail[i] >= 3) { g_stop[i] = 1; LOG("【红】增益自动补：「%s」连着 3 次没补上，本局不再补它", k_ab[i].nm); } else LOG("增益自动补：拍 %u「%s」这次没补上（第 %d 次），隔一会儿再试", t, k_ab[i].nm, g_fail[i]); }
        return; }
    uint32_t mode = *(uint32_t *)(pl + 0xc); int wset = dm_swapskill_wset(), dead = mode == 0 || mode == 17; void (*swap0)(int) = (void (*)(int))(b + 0x41db90);
    static int swapmax = -1, safe, swaplife; if (swapmax < 0) { swapmax = cfg("DM_AUTOBUFF_SWAPMAX", 150); safe = cfg("DM_AUTOBUFF_SAFE", 40); swaplife = cfg("DM_AUTOBUFF_SWAPLIFE", 50); }
    if (w_ph == W_OUT) {
        if (wset >= 0 && wset != w_from) { w_ph = W_CAST; w_start = t; w_moved = 0; LOG("增益自动补：拍 %u 已换到副手组（第 %d 组），开始补（此刻生命 %d / 上限 %d）", t, wset, ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 6, 0) >> 8, ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 7, 0) >> 8); }
        else if (dead || t - w_start > 60) { w_ph = W_IDLE; w_next = t + 250; LOG("增益自动补：拍 %u 换武器没换过去（%s），这一轮作罢", t, dead ? "人死了" : "2 秒多没变"); return; }
        else if (t - w_t0 >= 8 && w_tries < 4 && (mode == 1 || mode == 5)) { w_tries++; w_t0 = t; swap0(w_from); return; }
        else return; }
    if (w_ph == W_CAST) {
        if (mode == 2 || mode == 3 || mode == 4 || mode == 18) w_moved = 1;
        int left = 0; for (int k = 0; k < w_n; k++) if (!has[w_list[k]] && !g_stop[w_list[k]]) left++;
        if (!left || dead || t - w_start > (uint32_t)swapmax) {


            static int cool = -1; if (cool < 0) cool = cfg("DM_AUTOBUFF_SWAPCOOL", 1500);
            if (left && (w_moved || dead)) LOG("增益自动补：拍 %u 副手组这一轮人在%s，没补完的不算失败", t, dead ? "死亡中" : "走动 / 挨打");
            else for (int k = 0; k < w_n; k++) { int x = w_list[k]; if (has[x]) { w_fail[x] = 0; continue; } if (++w_fail[x] < 2) continue; w_fail[x] = 0; w_until[x] = t + (uint32_t)cool;
                LOG("增益自动补：「%s」换武器连着 2 次没补上，歇 %d 秒再试", k_ab[x].nm, cool / 25); }
            LOG("增益自动补：拍 %u 副手组上%s → 换回第 %d 组", t, !left ? "该补的都补上了" : dead ? "人死了" : "到点了还没补完", w_from); w_ph = W_BACK; w_t0 = 0; w_tries = 0; g_wait = -1; } }
    if (w_ph == W_BACK) {
        if (wset == w_from) { w_ph = W_IDLE; w_next = t + 250; int l1 = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 6, 0) >> 8, mx = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 7, 0) >> 8;
            LOG("增益自动补：拍 %u 已换回第 %d 组（生命 %d → %d / 上限 %d；左键技能 %d → %d、右键技能 %d → %d）", t, w_from, w_life0, l1, mx, w_lr0[0], lr_skill(pl, 0), w_lr0[1], lr_skill(pl, 1)); }
        else { if (!dead && (mode == 1 || mode == 5 || t - w_t0 >= 25) && (!w_t0 || t - w_t0 >= 8)) { w_t0 = t; w_tries++; swap0(wset);
                   if (w_tries % 10 == 0) LOG("【红】增益自动补：换回第 %d 组已试 %d 次还没成（模式 %u），继续试", w_from, w_tries, mode); }
               return; } }
    if (mode != 1 && mode != 5) { if (!dead && w_ph == W_IDLE) w_why(b, pl, has, t, 0, "人没站定（走 / 跑 / 攻击 / 施法 / 受击中），等站住的那一拍"); return; }
    if (dm_hud_area_is_town(dm_unit_level(pl))) return;
    if (((uint64_t (*)(void))(b + 0x2aa7c0))()) { if (w_ph == W_IDLE) w_why(b, pl, has, t, 1, "手上拿着物品"); return; }
    static int retry = -1; if (retry < 0) retry = cfg("DM_AUTOBUFF_RETRY", 75);
    { static uint32_t last_sig; equip_ok(b, pl, 0); if (g_hand_sig != last_sig) { last_sig = g_hand_sig; memset(g_stop, 0, sizeof g_stop); memset(g_fail, 0, sizeof g_fail); } }
    int mana = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 8, 0);
    for (int i = 0; i < NAB; i++) { int g = k_ab[i].grp;
        if (has[i] || g_stop[i] || skipped(k_ab[i].skill) || (g_try[i] && t - g_try[i] < (uint32_t)retry)) continue;
        uint64_t node = own_node(pl, k_ab[i].skill); int lvl = node ? eff_level(b, pl, node) : 0; if (lvl <= 0) continue;
        if (!equip_ok(b, pl, k_ab[i].skill)) continue;
        if (g) { int other = 0, best = i, bl = lvl;
            for (int j = 0; j < NAB; j++) { if (j == i || k_ab[j].grp != g || skipped(k_ab[j].skill)) continue; if (has[j]) other = 1;
                uint64_t n2 = own_node(pl, k_ab[j].skill); int l2 = n2 ? eff_level(b, pl, n2) : 0; if (l2 > bl) { bl = l2; best = j; } }
            if (other || (g_last[g] >= 0 ? g_last[g] != i : best != i)) continue; }
        int cost = mana_cost(node, lvl); if (mana < cost) { static uint32_t said; if (!said || t - said > 250) { said = t; LOG("增益自动补：「%s」不在身上，法力 %d 不够 %d，先不补", k_ab[i].nm, mana >> 8, cost >> 8); } continue; }
        g_try[i] = t; g_wait = i; g_wait_until = t + 50; g_mana0 = mana; atomic_store(&g_busy, 0); atomic_store(&g_req, i);
        { uint64_t sl = *(uint64_t *)(pl + 0x100), rn = sl ? *(uint64_t *)(sl + 0x10) : 0; LOG("增益自动补：拍 %u「%s」（技能 %d，等级 %d）不在身上 → 请服务端补放（法力 %d，要 %d；客户端右键技能 %d）", t, k_ab[i].nm, k_ab[i].skill, lvl, mana >> 8, cost >> 8, rn && *(uint64_t *)rn ? *(int16_t *)*(uint64_t *)rn : -1); }
        return; }


    if (w_ph != W_IDLE || g_wait >= 0 || t < w_next) return;
    if (wset < 0) { w_why(b, pl, has, t, 2, "读不到当前武器组"); return; }
    if (dm_swapskill_busy()) { w_why(b, pl, has, t, 3, "副手组技能的一键放正在进行"); return; }
    w_n = 0; for (int i = 0; i < NAB && w_n < 6; i++) { if (has[i] || g_stop[i] || t < w_until[i] || skipped(k_ab[i].skill)) continue;
        uint64_t node = own_node(pl, k_ab[i].skill); if (node && eff_level(b, pl, node) > 0) continue;
        if (swap_gives(b, pl, k_ab[i].skill)) w_list[w_n++] = i; }
    if (!w_n) { int cooling = 0; if (w_due(b, pl, has, t, &cooling) == 0 && cooling && (!w_said[5] || t - w_said[5] >= 250)) { w_said[5] = t; LOG("增益自动补：副手组的增益要补，但换武器连败后在歇（%d 个）", cooling); } return; }
    { int lf = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 6, 0) >> 8, mx = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 7, 0) >> 8;
      if (mx > 0 && (long long)lf * 100 < (long long)mx * swaplife) { char m[64]; snprintf(m, sizeof m, "生命 %d / %d 不到上限的 %d%%", lf, mx, swaplife); w_why(b, pl, has, t, 4, m); return; } }
    int en = enemy_near(b, pl, safe); if (en >= 0) { static uint32_t said; if (!said || t - said > 250) { said = t; LOG("增益自动补：副手组的增益要补，但 %d 格内有敌对怪（最近 %d 格），先不换武器", safe, en); } return; }
    w_ph = W_OUT; w_from = wset; w_t0 = w_start = t; w_tries = 1; w_life0 = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 6, 0) >> 8; w_lr0[0] = lr_skill(pl, 0); w_lr0[1] = lr_skill(pl, 1);
    { char bf[120]; int o = 0; for (int k = 0; k < w_n; k++) o += snprintf(bf + o, sizeof bf - o, " %s", k_ab[w_list[k]].nm); LOG("增益自动补：拍 %u 副手组给的增益不在身上（%s）→ 换武器（当前第 %d 组，生命 %d / 上限 %d，左键技能 %d、右键技能 %d）", t, bf, wset, w_life0, ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 7, 0) >> 8, w_lr0[0], w_lr0[1]); }
    swap0(wset);
}


void dm_autobuff_srv_tick(uint8_t *b, uint64_t G, uint64_t player) {


    { static int at = -1, st; if (at < 0) { const char *e = getenv("DM_TEST_BUFF_DROP"); if (!e || sscanf(e, "%d:%d", &at, &st) != 2) at = 0; }
      if (at > 0 && player && *(uint32_t *)(G + 0x170) >= (uint32_t)at) { at = 0; uint64_t sl = *(uint64_t *)(player + 0x88); if (sl && *(uint32_t *)(sl + 0x1c) >> 31) *(uint32_t *)(sl + 0xb30 + 4 * (uint64_t)(st >> 5)) &= ~(1u << (st & 31)); LOG("测试 DM_TEST_BUFF_DROP：服务端清掉状态 %d", st); } }
    { static uint64_t tl; if (tl != player && player && *(uint32_t *)(G + 0x170) > 40) { tl = player; test_learn(b, player, "服务端"); } }
    static uint32_t chk_at; static int chk_m, chk_i;
    if (chk_at && player && *(uint32_t *)(G + 0x170) >= chk_at) { chk_at = 0; char sb[200]; stat_line(b, player, sb, sizeof sb); LOG("增益自动补读数：「%s」施放后 30 拍，服务端法力 %d → %d，模式 %u；%s", k_ab[chk_i].nm, chk_m >> 8, ((int (*)(uint64_t, int, int))(b + 0x5b9290))(player, 8, 0) >> 8, *(uint32_t *)(player + 0xc), sb); }
    int i = atomic_exchange(&g_req, -1); if (i < 0 || !player) return;
    uint32_t pm = *(uint32_t *)(player + 0xc); uint64_t node = own_node(player, k_ab[i].skill), sl = *(uint64_t *)(player + 0x100);
    if ((pm != 1 && pm != 5) || !node || !sl) { atomic_store(&g_busy, 1); return; }
    uint64_t pp = *(uint64_t *)(player + 0x38); int x = pp ? *(uint16_t *)(pp + 2) : 0, y = pp ? *(uint16_t *)(pp + 6) : 0, m0 = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(player, 8, 0);
    uint64_t old_r = *(uint64_t *)(sl + 0x10); *(uint64_t *)(sl + 0x10) = node;
    int r = ((int (*)(uint64_t, uint64_t, int, int, int, uint64_t))(b + 0x6e77f0))(G, player, x, y, 1, node);
    uint64_t mid_r = *(uint64_t *)(sl + 0x10); *(uint64_t *)(sl + 0x10) = old_r;
    if (*(uint32_t *)(player + 0xc) != 10) atomic_store(&g_unusable, i);
    int m1 = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(player, 8, 0); if (getenv("DM_TEST_BUFF_DUMP")) { char sb[200]; stat_line(b, player, sb, sizeof sb); LOG("增益自动补读数：放「%s」之前服务端玩家 %s", k_ab[i].nm, sb); chk_at = *(uint32_t *)(G + 0x170) + 30; chk_m = m0; chk_i = i; }
    LOG("增益自动补：服务端放「%s」（技能 %d，基础等级 %d）→ 返回 %d，模式 %u → %u，法力 %d → %d；右键技能节点 %#llx（技能 %d）调用后是 %#llx，已放回", k_ab[i].nm, k_ab[i].skill, *(int32_t *)(node + 0x38), r, pm, *(uint32_t *)(player + 0xc), m0 >> 8, m1 >> 8, (unsigned long long)old_r, old_r && *(uint64_t *)old_r ? *(int16_t *)*(uint64_t *)old_r : -1, (unsigned long long)mid_r);
}


void dm_autobuff_test_drop(uint64_t pl) {
    static int at = -1, st; if (at < 0) { const char *e = getenv("DM_TEST_BUFF_DROP"); if (!e || sscanf(e, "%d:%d", &at, &st) != 2) at = 0; }
    if (at > 0 && pl && dm_cur_tick() >= (uint32_t)at) { at = 0; uint64_t sl = *(uint64_t *)(pl + 0x88); if (sl && *(uint32_t *)(sl + 0x1c) >> 31) *(uint32_t *)(sl + 0xb30 + 4 * (uint64_t)(st >> 5)) &= ~(1u << (st & 31)); LOG("测试 DM_TEST_BUFF_DROP：客户端清掉状态 %d", st); }
}
