

















#include "dm_prof.h"
#include "dm_actions.h"
#include "dm_swapskill_core.h"
#include "dm_cursor_item.h"
#include "dm_elysium.h"
#include "dm_custom.h"
#include "dm_input.h"
#include "dm_hud.h"
#include <TargetConditionals.h>
#include <unistd.h>
#include "dm_uiscreen.h"
#include "hle.h"
#include "dm_rift.h"
#include "dm_i18n.h"
int dm_skill_cap(void);
#include "dm.h"
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>

#define H_STATS  0x105f47414eb99892ull
#define H_TREE   0xc50d69aa61850d12ull
#define H_USEPT  0x290e587494892995ull
static uint8_t *ab(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER; static dm_shop_info g_shop; static dm_points_info g_pts = { .sel_skill = -1 }; static _Atomic int g_open;
static _Atomic int g_buy, g_sell, g_stat = -1, g_skill;
void dm_shop_get(dm_shop_info *o) { pthread_mutex_lock(&g_mu); *o = g_shop; pthread_mutex_unlock(&g_mu); }
void dm_points_get(dm_points_info *o) { pthread_mutex_lock(&g_mu); *o = g_pts; pthread_mutex_unlock(&g_mu); }
int dm_shop_open(void) { return atomic_load(&g_open); }
void dm_shop_buy(void) { atomic_store(&g_buy, 1); }
void dm_shop_sell_cursor(void) { atomic_store(&g_sell, 1); }
void dm_stat_add(int stat, int n) { if (stat >= 0 && stat <= 3 && (n == 1 || n == 5 || n == -1)) atomic_store(&g_stat, stat << 8 | (n == 1 ? 2 : n == 5 ? 3 : 1)); }
void dm_skill_add_selected(void) { atomic_store(&g_skill, 1); }
static _Atomic uint32_t g_belt; static _Atomic int g_test_hold, g_test_sell;
void dm_belt_put(uint32_t item_id) { if (item_id) atomic_store(&g_belt, item_id); }
void dm_test_hold_first_sellable(void) { atomic_store(&g_test_hold, 1); }
static _Atomic int g_stow, g_stow_res = -1, g_stow_why, g_stow_tick;
static _Atomic int g_test_swap;
void dm_test_swap_held(void) { atomic_store(&g_test_swap, 1); }
void dm_held_stow(void) { atomic_store(&g_stow_why, 0); atomic_store(&g_stow_res, -1); atomic_store(&g_stow, 1); }
int dm_held_stow_result(void) { return atomic_load(&g_stow_res); }
int dm_held_stow_why(void) { return atomic_load(&g_stow_why); }
int dm_held_stow_tick(void) { return atomic_load(&g_stow_tick); }
void dm_test_sell_first(void) { atomic_store(&g_test_sell, 1); }
static _Atomic int g_beltx = -1, g_belt_state;
void dm_belt_expand(int on) { atomic_store(&g_beltx, on ? 1 : 0); }
static _Atomic int g_belt_use;
static _Atomic int g_belt_drink, g_belt_kinds;
void dm_belt_drink(int kind) { if (kind >= 1 && kind <= 3) atomic_store(&g_belt_drink, kind); }
int dm_belt_kinds(void) { return atomic_load(&g_belt_kinds); }
void dm_belt_use(int col, int row) { if (col >= 0 && col < 4 && row >= 0 && row < 4) atomic_store(&g_belt_use, 1 + row * 4 + col); }
void dm_belt_use_merc(int col, int row) { if (col >= 0 && col < 4 && row >= 0 && row < 4) atomic_store(&g_belt_use, 0x100 | (1 + row * 4 + col)); }
static _Atomic int g_use_merc;
int dm_belt_rows(int *expanded) { int v = atomic_load(&g_belt_state); if (expanded) *expanded = v >> 8 & 1; return v & 0xff; }
static _Atomic int g_sort = -1, g_rname;
static _Atomic int g_held_equip = -1;
void dm_held_equip(int bodyloc) { if ((bodyloc & 0xff) > 0 && (bodyloc & 0xff) < 16) atomic_store(&g_held_equip, bodyloc); }
void dm_inv_sort(int page) { if (page == 0 || page == 3 || page == 4) atomic_store(&g_sort, page); }
void dm_name_random(void) { atomic_store(&g_rname, 12); }
int dm_name_taken(const char *name) {
    const char *dm_save_workdir(void); const char *wd = dm_save_workdir(); char pth[1200];
    if (!wd || !name || !*name || strchr(name, '/')) return 0;
    snprintf(pth, sizeof pth, "%s/%s", wd, name); return access(pth, F_OK) == 0;
}
static _Atomic uint32_t g_equip, g_unequip; static _Atomic int g_test_eq;
void dm_item_equip(uint32_t id) { if (id) atomic_store(&g_equip, id); }
void dm_item_unequip(uint32_t id) { if (id) atomic_store(&g_unequip, id); }
void dm_test_equip(int what) { atomic_store(&g_test_eq, what); }
static _Atomic uint32_t g_ident; static _Atomic int g_tcancel, g_test_ident;
void dm_item_identify(uint32_t id) { if (id) atomic_store(&g_ident, id); }


static _Atomic uint32_t g_ident_free;
void dm_item_identify_free(uint32_t id) { if (id) atomic_store(&g_ident_free, id); }
uint32_t dm_item_identify_free_take(void) { return atomic_exchange(&g_ident_free, 0); }
void dm_item_target_cancel(void) { atomic_store(&g_tcancel, 1); }
static _Atomic uint32_t g_repair;
static _Atomic int g_bfill;
void dm_belt_fill(void) { atomic_store(&g_bfill, 1); }




static uint64_t belt_find(uint64_t w, uint64_t h, int dep) {
    if (!w || dep > 14 || !*(uint8_t *)(w + 0x50)) return 0;
    if (*(uint64_t *)(*(uint64_t *)w + 0x28) == h) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = belt_find(*(uint64_t *)(ch + 8 * i), h, dep + 1); if (r) return r; }
    return 0;
}
static uint64_t belt_now(uint8_t *b, uint64_t root) {
    uint64_t ch = root ? *(uint64_t *)(root + 0x58) : 0, n = root ? *(uint64_t *)(root + 0x60) : 0;
    for (uint64_t i = 0; ch && n <= 256 && i < n; i++) { uint64_t p = *(uint64_t *)(ch + 8 * i); const char *nm = p ? *(const char **)(p + 8) : NULL;
        if (nm && !strcmp(nm, "PlayerInventoryExpansionLayout")) return belt_find(p, (uint64_t)(uintptr_t)(b + 0x3911d8), 0); }
    return 0;
}
void dm_item_repair(uint32_t id) { if (id) atomic_store(&g_repair, id); }
void dm_test_identify(int what) { atomic_store(&g_test_ident, what); }
static _Atomic uint64_t g_take; static _Atomic int g_test_take;
void dm_stash_take_id(uint32_t id, int n) { if (id && n > 0) atomic_store(&g_take, (uint64_t)id << 32 | (uint32_t)n); }
static _Atomic int g_take_cur;
void dm_stash_take(int n) { if (n > 0) atomic_store(&g_take_cur, n); }
void dm_test_stash_take(int n) { atomic_store(&g_test_take, n); }
static _Atomic int g_cmp;
void dm_item_compare(int on) { atomic_store(&g_cmp, on ? 1 : 0); }
static _Atomic uint32_t g_use; static _Atomic int g_test_use;
void dm_item_use(uint32_t item_id) { if (item_id) atomic_store(&g_use, item_id); }
void dm_test_use_first(int kind) { atomic_store(&g_test_use, kind); }




int dm_ui_hover_now(int x, int y) {
    uint8_t *b = ab(); if (!b) return 0; uint64_t root = *(uint64_t *)(b + 0x2f05458), fm = root ? *(uint64_t *)(root + 0xd0) : 0;
    if (!fm || !*(uint8_t *)(b + 0x2d128f0) || *(uint8_t *)(b + 0x2d128ec)) return 0;
    ((void (*)(int, int))(b + 0x2aa480))(x, y); ((void (*)(uint64_t, int))(b + 0xabd6bc))(fm, 0); return 1;
}


void dm_test_hover_at(float nx, float ny) {
    uint8_t *b = ab(); if (!b) return; uint64_t root = *(uint64_t *)(b + 0x2f05458), fm = root ? *(uint64_t *)(root + 0xd0) : 0; if (!fm) return;
    int W = *(int32_t *)(b + 0x211da38), H = *(int32_t *)(b + 0x211da3c), x = (int)(nx * W), y = (int)(ny * H);
    uint64_t f0 = *(uint64_t *)(fm + 0x190); int ok = dm_ui_hover_now(x, y); uint64_t h1 = *(uint64_t *)(fm + 0x178), f1 = *(uint64_t *)(fm + 0x190);
    uint64_t it = f1 && *(uint64_t *)f1 - (uint64_t)(uintptr_t)b == 0x2089ab8 ? ((uint64_t (*)(uint64_t, void *))(b + 0x3cc87c))(f1, NULL) : 0; uint32_t code = it ? ((uint32_t (*)(uint64_t))(b + 0x570260))(it) : 0;
    LOG("测试：hover_now (%d,%d) → %d；焦点 %s → %s，悬停 %s，选中格 %d,%d，物品 id %u（%.3s）", x, y, ok, f0 && *(const char **)(f0 + 8) ? *(const char **)(f0 + 8) : "-", f1 && *(const char **)(f1 + 8) ? *(const char **)(f1 + 8) : "-",
        h1 && *(const char **)(h1 + 8) ? *(const char **)(h1 + 8) : "-", f1 ? *(int32_t *)(f1 + 0x544) : -1, f1 ? *(int32_t *)(f1 + 0x548) : -1, it ? *(uint32_t *)(it + 8) : 0, (const char *)&code);
}
static _Atomic int g_test_belt;
void dm_test_belt_first_potion(void) { atomic_store(&g_test_belt, 1); }

static uint64_t a_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 14) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = a_find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}


static uint64_t a_panel(uint64_t root, const char *name) {
    uint64_t ch = *(uint64_t *)(root + 0x58), n = *(uint64_t *)(root + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t p = *(uint64_t *)(ch + 8 * i); const char *nm = p ? *(const char **)(p + 8) : NULL; if (nm && !strcmp(nm, name)) return p; }
    return 0;
}


static int a_skill_base(uint64_t pl, int skill) {
    uint64_t l = *(uint64_t *)(pl + 0x100), n = l ? *(uint64_t *)l : 0;
    for (int k = 0; n && k < 512; k++, n = *(uint64_t *)(n + 8)) { uint64_t r = *(uint64_t *)n; if (r && *(int16_t *)r == skill && *(int32_t *)(n + 0x44) == -1) { int v = *(int32_t *)(n + 0x38); return v < 0 ? 0 : v; } }
    return 0;
}






int dm_skill_base(uint64_t pl, int skill) { return a_skill_base(pl, skill); }
static int a_skill_block(uint8_t *b, uint64_t pl, int skill, int points) {
    if (points < 1) return 1;
    uint8_t ver = *(uint8_t *)(pl + 0x1bd); if (ver < 1 || ver > 3 || skill < 0) return 4;
    uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16); if ((uint64_t)skill >= *(uint64_t *)(t + 0x11f0)) return 4;
    uint64_t rec = *(uint64_t *)(t + 0x11e8) + (uint64_t)skill * 0x2ec;
    if (((int (*)(uint64_t, int, int))(b + 0x5b9290))(pl, 12, 0) < *(int16_t *)(rec + 0x214) + a_skill_base(pl, skill)) return 2;
    for (int i = 0; i < 3; i++) { int q = *(int16_t *)(rec + 0x21e + 2 * i); if (q >= 0 && a_skill_base(pl, q) < 1) return 3; }
    if (((int (*)(int))(b + 0xc368b0))(skill) & 1) return 0;


    if (dm_skill_cap() > 20 && !dm_rift_cleared_any() && a_skill_base(pl, skill) >= 20 && a_skill_base(pl, skill) < dm_skill_cap() && *(int16_t *)(uintptr_t)(rec + 0x184) == 20) return 5;
    return 4;
}

















#define SKILL_CAP 40
int dm_skill_cap(void) { static int c; if (!c) { const char *e = getenv("DM_SKILL_CAP"); c = e ? atoi(e) : SKILL_CAP; if (c < 20 || c > 99) c = SKILL_CAP; } return c; }






static void skill_cap_pump(uint8_t *b) {
    static uint64_t seen[4]; static int applied[4]; static uint8_t ours[4][512]; int cap = dm_skill_cap();
    if (cap == 20) return;
    int want = dm_rift_cleared_any() ? cap : 20;
    for (int v = 1; v <= 3; v++) { uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)v * 16); if (!t || (seen[v] == t && applied[v] == want)) continue;
        uint64_t recs = *(uint64_t *)(t + 0x11e8), n = *(uint64_t *)(t + 0x11f0), gp = *(uint64_t *)(t + 0x1500); if (!recs || !n || n > 4096) continue;
        if (seen[v] != t) { int ch = 0, other = 0; memset(ours[v], 0, sizeof ours[v]);
            for (uint64_t i = 0; i < n; i++) { int16_t m = *(int16_t *)(uintptr_t)(recs + i * 0x2ec + 0x184); if (m == 20) { ours[v][i >> 3] |= (uint8_t)(1 << (i & 7)); ch++; } else if (m > 0) other++; }
            seen[v] = t; applied[v] = 20; LOG("技能上限：数据表版本 %d 共 %llu 个技能，%d 个的硬点上限是 20（解锁后改成 %d；另有 %d 个上限不是 20 的不动）；基础等级读数的全局钳值 %d", v, (unsigned long long)n, ch, cap, other, gp ? *(int32_t *)(uintptr_t)gp : -1); }
        if (applied[v] != want) { int ch = 0; for (uint64_t i = 0; i < n; i++) if (ours[v][i >> 3] & (1 << (i & 7))) { *(int16_t *)(uintptr_t)(recs + i * 0x2ec + 0x184) = (int16_t)want; ch++; }
            applied[v] = want; LOG("技能上限：数据表版本 %d 的 %d 个技能硬点上限 → %d（这个角色%s）", v, ch, want, want == 20 ? "还没打通过秘境" : "打通过秘境"); } }
}
static void a_send(uint8_t *b, uint64_t root, uint64_t cat, uint64_t act, int32_t p0, int32_t p1) {
    static int32_t payload[4]; static uint64_t msg[8]; payload[0] = p0; payload[1] = p1; memset(msg, 0, sizeof msg); msg[0] = cat; msg[1] = act; msg[2] = (uint64_t)(uintptr_t)payload;
    (void)b; ((void (*)(uint64_t, void *))*(uint64_t *)(*(uint64_t *)root + 0x28))(root, msg);
}


















uint32_t dm_cur_tick(void);
static int a_has_state(uint64_t u, int st) {
    uint64_t sl = *(uint64_t *)(u + 0x88); if (!sl || st < 0 || st > 0x1fe || !(*(uint32_t *)(sl + 0x1c) >> 31)) return 0;
    return (*(uint32_t *)(sl + 0xb30 + 4 * (uint64_t)(st >> 5)) >> (st & 31)) & 1;
}






static int ap_pct(int which) { static int v[2] = { -1, -1 }; static const char *const nm[2] = { "DM_AUTOPOT_RED", "DM_AUTOPOT_REJUV" }; static const int def[2] = { 50, 30 };
    if (v[which] < 0) { const char *e = getenv(nm[which]); v[which] = e && atoi(e) > 0 && atoi(e) <= 100 ? atoi(e) : def[which]; } return v[which]; }


static uint32_t ap_find(uint8_t *b, uint64_t pl, int kind, uint32_t *ocode, int *in_bag) {
    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; uint32_t bid = 0, iid = 0, brow = 99, bc = 0, ic = 0;
    for (int k = 0; it && k < 512; k++) { uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d) break;
        uint32_t code = ((uint32_t (*)(uint64_t))(b + 0x570260))(it), cont = *(uint32_t *)(it + 0xc), c3 = code & 0xffffff; int c2 = code >> 16 & 0xff;
        int ok = kind == 2 ? (c3 == 0x737672 || c3 == 0x6c7672) : ((code & 0xffff) == (kind ? 0x706du : 0x7068u) && c2 >= '1' && c2 <= '5');
        if (ok) { if (cont == 2 && pp && *(uint32_t *)(pp + 0x14) < brow) { brow = *(uint32_t *)(pp + 0x14); bid = *(uint32_t *)(it + 8); bc = code; }
                  else if (cont == 0 && *(uint8_t *)(d + 0x55) == 0 && !iid) { iid = *(uint32_t *)(it + 8); ic = code; } }
        it = *(uint64_t *)(d + 0xb0); }
    *ocode = iid ? ic : bc; *in_bag = iid != 0; return iid ? iid : bid;
}
typedef struct { uint32_t last; int warned, extra; } ap_life;


static uint32_t ap_life_pick(uint8_t *b, uint64_t pl, ap_life *s, int v, int mx, int healing, uint32_t t, uint32_t *code, int *in_bag, const char **why) {
    if (mx <= 0 || v <= 0) return 0;
    int low = (long long)v * 100 < (long long)mx * ap_pct(1), mid = (long long)v * 100 < (long long)mx * ap_pct(0);
    if (!low) s->extra = 0;
    if (!mid) { s->warned = 0; if (healing) s->last = 0; return 0; }
    if (s->last && t >= s->last && t - s->last < 25) return 0;
    if (low) { uint32_t id = ap_find(b, pl, 2, code, in_bag); if (id) { *why = "紫药"; return id; }
        if (healing && s->extra) return 0;
        id = ap_find(b, pl, 0, code, in_bag); if (id) { if (healing) s->extra = 1; *why = healing ? "没有紫药，再喝一瓶红药" : "没有紫药，喝红药"; return id; }
        if (!s->warned) { s->warned = 1; *why = "没药"; } return 0; }
    if (healing) return 0;
    { uint32_t id = ap_find(b, pl, 0, code, in_bag); if (id) { *why = "红药"; return id; } if (!s->warned) { s->warned = 1; *why = "没药"; } return 0; }
}
static int auto_pot_life(uint8_t *b, uint64_t pl, uint32_t t) {
    static ap_life s; int (*gs)(uint64_t, int, int) = (void *)(b + 0x5b9290); int v = gs(pl, 6, 0) >> 8, mx = gs(pl, 7, 0) >> 8, bag = 0; uint32_t code = 0; const char *why = NULL;
    uint32_t id = ap_life_pick(b, pl, &s, v, mx, a_has_state(pl, 100), t, &code, &bag, &why);
    if (!id) { if (why) LOG("自动喝药：生命 %d/%d 低于 %d%%，背包和腰带里都没有可喝的生命药水", v, mx, ap_pct(0)); return 0; }
    atomic_store(&g_use, id); s.last = t; s.warned = 0;
    LOG("自动喝药：拍 %u 生命 %d/%d（%d%%）→ %s：喝%s的 %.3s（id %u）", t, v, mx, (int)((long long)v * 100 / mx), why, bag ? "背包" : "腰带", (const char *)&code, id);
    return 1;
}
static int auto_pot_mana(uint8_t *b, uint64_t pl, uint32_t t) {
    static uint32_t last; static int warned; int (*gs)(uint64_t, int, int) = (void *)(b + 0x5b9290);
    if (gs(pl, 26, 0)) { last = 0; return 0; }
    if (last && t >= last && t - last < 25) return 0;
    int v = gs(pl, 8, 0) >> 8, mx = gs(pl, 9, 0) >> 8, bag = 0; uint32_t code = 0;
    if (mx <= 0 || v * 10 >= mx * 3) { warned = 0; return 0; }
    uint32_t id = ap_find(b, pl, 1, &code, &bag);
    if (!id) { if (!warned) { warned = 1; LOG("自动喝药：法力 %d/%d 低于 30%%，背包和腰带里都没有法力药水", v, mx); } return 0; }
    atomic_store(&g_use, id); last = t; warned = 0;
    LOG("自动喝药：拍 %u 法力 %d/%d 低于 30%% → 喝%s的 %.3s（id %u）", t, v, mx, bag ? "背包" : "腰带", (const char *)&code, id);
    return 1;
}








static int auto_merc_pot(uint8_t *b, uint64_t pl, uint32_t t) {
    static int env = -2; if (env == -2) { const char *e = getenv("DM_MERC_POT"); env = e ? e[0] != '0' : -1; }
    if (!(env >= 0 ? env : dm_custom_get("merc_pot"))) return 0;
    static ap_life s; int (*gs)(uint64_t, int, int) = (void *)(b + 0x5b9290);
    { int v = gs(pl, 6, 0), mx = gs(pl, 7, 0); if (mx <= 0 || (long long)v * 100 < (long long)mx * ap_pct(1)) return 0; }
    uint32_t mid = 0xffffffffu; for (uint64_t e = *(uint64_t *)(b + 0x2d33528); e; e = *(uint64_t *)(e + 0x30)) if (*(uint32_t *)(e + 4) == 7 && *(uint32_t *)(e + 0xc) == *(uint32_t *)(pl + 8) && !*(uint32_t *)(e + 0x20)) { mid = *(uint32_t *)(e + 8); break; }
    int dm_diag_srv_merc(uint32_t id, int *hp, int *mx, int *healing); int v = 0, mx = 0, healing = 0, bag = 0; uint32_t code = 0; const char *why = NULL;
    if (mid == 0xffffffffu || !dm_diag_srv_merc(mid, &v, &mx, &healing)) return 0;
    uint32_t id = ap_life_pick(b, pl, &s, v, mx, healing, t, &code, &bag, &why);
    if (!id) { if (why) LOG("佣兵自动喂药：佣兵生命 %d%% 低于 %d%%，背包和腰带里都没有可喂的生命药水", (int)((long long)v * 100 / mx), ap_pct(0)); return 0; }
    atomic_store(&g_use_merc, 1); atomic_store(&g_use, id); s.last = t; s.warned = 0;
    LOG("佣兵自动喂药：拍 %u 佣兵（id %u）生命 %d/%d（%d%%）→ %s：喂%s的 %.3s（id %u）", t, mid, v, mx, (int)((long long)v * 100 / mx), why, bag ? "背包" : "腰带", (const char *)&code, id);
    return 1;
}
static void auto_pot_pump(uint8_t *b, uint64_t pl) {
    { int dm_itemjob_owner(void); if (!pl || dm_itemjob_owner()) return; }
    uint32_t t = dm_cur_tick(); if (!t) return;
    uint32_t mode = *(uint32_t *)(pl + 0xc); if (mode == 0 || mode == 17 || mode == 0x11) return;
    if (((uint64_t (*)(void))(b + 0x2aa7c0))() || atomic_load(&g_use)) return;
    if (dm_custom_get("auto_mana") && (auto_pot_life(b, pl, t) || auto_pot_mana(b, pl, t))) return;
    auto_merc_pot(b, pl, t);
}






















enum { SS_IDLE, SS_REQ, SS_SWAPPING, SS_READY, SS_RELEASED, SS_BACK, SS_REBIND };
static _Atomic int g_ss_phase, g_ss_srv_wset = -1, g_ss_kind[12], g_ss_charges[12], g_ss_alt[12];
static _Atomic uint64_t g_ss_bit;
static _Atomic int g_ss_slot = -1, g_ss_early, g_ss_dead_town, g_ss_cast, g_ss_zl;
static _Atomic uint64_t g_ss_map[6];
static _Atomic uint32_t g_ss_equipped[16]; static _Atomic int g_ss_neq;
void dm_swapskill_srv_wset(int w) { atomic_store(&g_ss_srv_wset, w); }
int dm_swapskill_wset(void) { return atomic_load(&g_ss_srv_wset); }
int dm_swapskill_busy(void) { return atomic_load(&g_ss_phase) != SS_IDLE; }
static _Atomic uint32_t g_ss_itemsig; uint32_t dm_swapskill_itemsig(void) { return atomic_load(&g_ss_itemsig); }
int dm_swapskill_slot(int slot, int *charges) {
    if (slot < 0 || slot >= 12) return 0; int k = atomic_load(&g_ss_kind[slot]); if (charges) *charges = atomic_load(&g_ss_charges[slot]); return k; }





static _Atomic uint64_t g_ss_offgive[8];
static uint64_t g_ss_oskill[8], g_ss_single[8];
static uint8_t *g_ss_b;
static int ss_item_stat(void *it, int stat, int layer) { return ((int (*)(uint64_t, int, int))(g_ss_b + 0x5b9290))((uint64_t)(uintptr_t)it, stat, layer); }
static int ss_skill_class(int skill) { int dm_itemcmp_skill_class(int); return dm_itemcmp_skill_class(skill); }
int dm_swapskill_offhand_gives(int skill) { return skill >= 0 && skill < 512 && (atomic_load(&g_ss_offgive[skill >> 6]) >> (skill & 63) & 1); }
int dm_swapskill_equipped(int32_t item) {
    int n = atomic_load(&g_ss_neq); for (int i = 0; i < n && i < 16; i++) if ((int32_t)atomic_load(&g_ss_equipped[i]) == item) return 1; return 0; }
static int ss_node(uint64_t pl, int id, int item, int *charges) {
    uint64_t l = *(uint64_t *)(pl + 0x100), n = l ? *(uint64_t *)l : 0;
    for (int k = 0; n && k < 1024; k++, n = *(uint64_t *)(n + 8)) { uint64_t r = *(uint64_t *)n; if (r && *(int16_t *)r == id && *(int32_t *)(n + 0x44) == item) { if (charges) *charges = *(int32_t *)(n + 0x48); return 1; } }
    return 0;
}



static int ss_live_level(uint8_t *b, uint64_t pl, int id) {
    uint64_t l = *(uint64_t *)(pl + 0x100), n = l ? *(uint64_t *)l : 0;
    for (int k = 0; n && k < 1024; k++, n = *(uint64_t *)(n + 8)) { uint64_t r = *(uint64_t *)n; if (r && *(int16_t *)r == id && *(int32_t *)(n + 0x44) == -1) return *(int32_t *)(n + 0x38) + ((int (*)(uint64_t, uint64_t, int))(b + 0x5b1710))(pl, n, 0); }
    return 0;
}
static int ss_node_any(uint64_t pl, int id) {
    uint64_t l = *(uint64_t *)(pl + 0x100), n = l ? *(uint64_t *)l : 0;
    for (int k = 0; n && k < 1024; k++, n = *(uint64_t *)(n + 8)) { uint64_t r = *(uint64_t *)n; if (r && *(int16_t *)r == id && *(int32_t *)(n + 0x38) > 0) return *(int32_t *)(n + 0x44); }
    return -2;
}
static int ss_item_charges(uint64_t it, int skill) {
    uint64_t sl = *(uint64_t *)(it + 0x88); if (!sl) return -1;
    uint64_t a = (*(int32_t *)(sl + 0x1c) < 0) ? sl + 0xe8 : sl + 0x30, arr = *(uint64_t *)a, cnt = *(uint64_t *)(a + 8);
    for (uint64_t i = 0; arr && i < cnt && i < 512; i++) { uint32_t key = *(uint32_t *)(arr + 8 * i); if ((key >> 16) == 204 && (int)((key & 0xffff) >> 6) == skill) return *(int32_t *)(arr + 8 * i + 4) & 0xff; }
    return -1;
}
static void swapskill_pump(uint8_t *b, uint64_t pl) {
    if (!pl) return;


    for (int i = 0; i < 6; i++) { uint32_t lg = *(uint32_t *)(b + 0x2d13cbc + 12 * i), nm = *(uint32_t *)(b + 0x2d13cbc + 12 * i + 4), al = *(uint32_t *)(b + 0x2d13cbc + 12 * i + 8);
        uint64_t bit = lg == 0x1000 ? DM_BTN_B : lg == 0x2000 ? DM_BTN_A : lg == 0x4000 ? DM_BTN_Y : lg == 0x8000 ? DM_BTN_X : lg == 0x200 ? DM_BTN_R : lg == 0x800 ? DM_BTN_ZR : 0;
        atomic_store(&g_ss_map[i], bit && nm < 12 && al < 12 ? bit | (uint64_t)nm << 32 | (uint64_t)al << 40 : 0); }


    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; int neq = 0; uint64_t eqit[16];
    for (int k = 0; it && k < 512; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; if (*(uint32_t *)(it + 0xc) == 1 && neq < 16) { eqit[neq] = it; atomic_store(&g_ss_equipped[neq], *(uint32_t *)(it + 8)); neq++; } it = *(uint64_t *)(d + 0xb0); }
    atomic_store(&g_ss_neq, neq);
    { static uint64_t osig = 1; static int age; uint64_t sig = 0; for (int e = 0; e < neq; e++) { uint64_t d = *(uint64_t *)(eqit[e] + 0x10); int loc = d ? *(uint8_t *)(d + 0x54) : 0;
          if (loc == 11 || loc == 12) sig = sig * 1099511628211ull ^ ((uint64_t)*(uint32_t *)(eqit[e] + 8) << 8 | (uint64_t)loc); }
      if (sig != osig || ++age >= 120) { osig = sig; age = 0; uint64_t os[8] = {0}, si[8] = {0}; int cls = (int)*(uint32_t *)(pl + 4); g_ss_b = b;
          for (int e = 0; e < neq; e++) { uint64_t d = *(uint64_t *)(eqit[e] + 0x10); int loc = d ? *(uint8_t *)(d + 0x54) : 0; if (loc != 11 && loc != 12) continue;
              dm_ss_scan_item(ss_item_stat, (void *)(uintptr_t)eqit[e], cls, ss_skill_class, os, si); }
          memcpy(g_ss_oskill, os, sizeof os); memcpy(g_ss_single, si, sizeof si); } }
    { uint64_t present[8] = {0}, give[8], l = *(uint64_t *)(pl + 0x100), n = l ? *(uint64_t *)l : 0;
      for (int k = 0; n && k < 1024; k++, n = *(uint64_t *)(n + 8)) { uint64_t r = *(uint64_t *)n; int id = r ? *(int16_t *)r : -1; int32_t src = *(int32_t *)(n + 0x44);
          if (src == -1 && dm_ss_has(g_ss_single, id)) dm_ss_mark_present(present, g_ss_single, id, src, *(int32_t *)(n + 0x38) + ((int (*)(uint64_t, uint64_t, int))(b + 0x5b1710))(pl, n, 0)); }
      dm_ss_given(g_ss_oskill, g_ss_single, present, give); for (int i = 0; i < 8; i++) atomic_store(&g_ss_offgive[i], give[i]); }
    { uint32_t h = 2166136261u; uint64_t l = *(uint64_t *)(pl + 0x100), n = l ? *(uint64_t *)l : 0;
      for (int k = 0; n && k < 1024; k++, n = *(uint64_t *)(n + 8)) { uint64_t r = *(uint64_t *)n; int32_t itm = *(int32_t *)(n + 0x44); if (r && itm != -1) { h = (h ^ (uint16_t)*(int16_t *)r) * 16777619u; h = (h ^ (uint32_t)itm) * 16777619u; } }
      atomic_store(&g_ss_itemsig, h); }
    for (int s = 0; s < 12; s++) { int id = *(int32_t *)(b + 0x2d5f0c0 + 28 * s), item = *(int32_t *)(b + 0x2d5f0c0 + 28 * s + 4), kind = 0, ch = -1;
        if (id >= 0 && item != -1) { uint64_t owner = 0; for (int e = 0; e < neq; e++) if (*(int32_t *)(eqit[e] + 8) == item) owner = eqit[e];
            if (owner) { ch = ss_item_charges(owner, id); if (ch >= 0) kind |= 2;
                if (!ss_node(pl, id, item, NULL)) { int alt = ss_node_any(pl, id);
                    if (alt != -2) { kind = 4; ch = -1; atomic_store(&g_ss_alt[s], alt);
                        for (int e = 0; e < neq && alt != -1; e++) if (*(int32_t *)(eqit[e] + 8) == alt) { ch = ss_item_charges(eqit[e], id); if (ch >= 0) kind |= 2; } }
                    else kind |= 1; } } }
        else if (id >= 0 && item == -1 && dm_ss_learned_offhand(ss_live_level(b, pl, id), dm_swapskill_offhand_gives(id))) kind = 1;
        atomic_store(&g_ss_kind[s], kind); atomic_store(&g_ss_charges[s], ch < 0 ? 0 : ch); }




    { static int alt_face = -1; if (getenv("DM_TEST_SS_ALT")) for (int i = 0; i < 6; i++) { uint64_t m = atomic_load(&g_ss_map[i]); int nm = (int)(m >> 32 & 0xff), al = (int)(m >> 40 & 0xff);
          if (!m || nm >= 12 || al >= 12 || !(alt_face == i || (alt_face < 0 && (atomic_load(&g_ss_kind[nm]) & 1)))) continue;
          if (memcmp(b + 0x2d5f0c0 + 28 * al, b + 0x2d5f0c0 + 28 * nm, 28)) { memcpy(b + 0x2d5f0c0 + 28 * al, b + 0x2d5f0c0 + 28 * nm, 28); LOG("DM_TEST_SS_ALT：面 %d 普通格 %d（技能 %d 物品 %d）的绑定抄到第二套格 %d", i, nm, *(int32_t *)(b + 0x2d5f0c0 + 28 * nm), *(int32_t *)(b + 0x2d5f0c0 + 28 * nm + 4), al); }
          alt_face = i; break; } }
    { dm_hud_info hi; dm_hud_get(&hi); uint32_t md = *(uint32_t *)(pl + 0xc); atomic_store(&g_ss_dead_town, md == 0 || md == 17 || (hi.valid && dm_hud_area_is_town(hi.area))); }


    static uint32_t t0, tback, tidle, treq, ch_ready; static int busy_ready; static int id, item, from, tries, rb_slot = -1, rb_item; uint32_t t = dm_cur_tick();
    if (rb_slot >= 0 && atomic_load(&g_ss_phase) == SS_IDLE) { *(int32_t *)(b + 0x2d5f0c0 + 28 * rb_slot + 4) = rb_item; rb_slot = -1; }
    uint32_t mode = *(uint32_t *)(pl + 0xc); int dead = mode == 0 || mode == 17;
    int ph = atomic_load(&g_ss_phase), w = atomic_load(&g_ss_srv_wset);
    uint64_t sl = *(uint64_t *)(pl + 0x100); int busy = sl && *(uint64_t *)(sl + 0x18);
    void (*swap0)(int) = (void (*)(int))(b + 0x41db90);


#ifndef DM_PLAYER_BUILD
    static int rej_left = -1; if (rej_left < 0) { const char *e = getenv("DM_TEST_SS_REJECT"); rej_left = e ? atoi(e) : 0; }
#endif


    { static int tree_from = -1; int dm_ui_swaptab_take(void), dm_ui_skilltree_open(void);
      if (dm_ui_swaptab_take()) { if (ph == SS_IDLE && w >= 0 && !busy && !dead) { if (tree_from < 0) tree_from = w; swap0(w); LOG("副手组技能：拍 %u 技能树里点 I / II 页签 → 换武器（当前组 %d）", t, w); } else LOG("副手组技能：技能树页签这一下没换（%s）", ph != SS_IDLE ? "连招进行中" : busy ? "动作中" : "读不到当前武器组"); }
      if (tree_from >= 0 && !dm_ui_skilltree_open() && ph == SS_IDLE) { if (w >= 0 && w != tree_from) { from = tree_from; tback = t; tries = 1; swap0(w); atomic_store(&g_ss_phase, SS_BACK); LOG("副手组技能：拍 %u 关技能树 → 切回打开时的第 %d 组", t, from); } tree_from = -1; } }
    switch (ph) {
    case SS_REQ: { int s = atomic_load(&g_ss_slot); id = *(int32_t *)(b + 0x2d5f0c0 + 28 * s); item = *(int32_t *)(b + 0x2d5f0c0 + 28 * s + 4);
        if (atomic_load(&g_ss_kind[s]) & 4) {
            rb_slot = s; rb_item = item; *(int32_t *)(b + 0x2d5f0c0 + 28 * s + 4) = atomic_load(&g_ss_alt[s]); atomic_store(&g_ss_phase, SS_REBIND);
            LOG("副手组技能：拍 %u 槽 %d 技能 %d 两组都有 → 用当前组的（来源物品 %d，不换武器）", t, s, id, atomic_load(&g_ss_alt[s])); break; }
        if (dead || w < 0 || dm_uiscreen_panel()) { LOG("副手组技能：槽 %d 不触发（%s）", s, dead ? "死亡中" : w < 0 ? "读不到当前武器组" : "面板开着"); atomic_store(&g_ss_phase, SS_IDLE); break; }


        if (busy) { if (!treq) { treq = t; LOG("副手组技能：拍 %u 槽 %d 按下时人物在动作中 → 等动作结束再换武器", t, s); }
            if (t - treq < 50) break; LOG("【红】副手组技能：等了 50 拍动作还没结束，放弃这次"); treq = 0; atomic_store(&g_ss_phase, SS_IDLE); break; }
        treq = 0; from = w; t0 = t; tries = 1;
#ifndef DM_PLAYER_BUILD
        if (rej_left > 0) { rej_left--; LOG("DM_TEST_SS_REJECT：这次换过去不发"); } else
#endif
        swap0(w); atomic_store(&g_ss_phase, SS_SWAPPING);
        LOG("副手组技能：拍 %u 槽 %d 技能 %d（物品 %d）在另一组 → 换武器（当前组 %d）", t, s, id, item, w); break; }
    case SS_SWAPPING: { int ch = 0;
        if (dead) { LOG("副手组技能：换武器途中死亡，放弃（不切回，武器组由游戏管）"); atomic_store(&g_ss_phase, SS_IDLE); break; }
        if (atomic_load(&g_ss_srv_wset) != from && ss_node(pl, id, item, &ch) && (item != -1 || ss_live_level(b, pl, id) > 0)) { LOG("副手组技能：拍 %u 技能可放了（换武器 → 可放 %u 拍，剩余 %d 次，等级 %d）→ 放行按键", t, t - t0, ch, item == -1 ? ss_live_level(b, pl, id) : -1); ch_ready = (uint32_t)ch; busy_ready = busy; atomic_store(&g_ss_cast, 0); atomic_store(&g_ss_phase, SS_READY); break; }


        if (t - t0 >= 6 && tries < 5 && atomic_load(&g_ss_srv_wset) == from && !busy) { tries++; t0 = t;
#ifndef DM_PLAYER_BUILD
            if (rej_left > 0) { rej_left--; LOG("DM_TEST_SS_REJECT：这次换过去不发"); } else
#endif
            swap0(from); LOG("副手组技能：拍 %u 换过去没被接受（当前组仍是 %d）→ 第 %d 次再发", t, from, tries); break; }
        if (t - t0 >= 25) {
            { char nb[160] = ""; uint64_t l = *(uint64_t *)(pl + 0x100), n = l ? *(uint64_t *)l : 0; for (int k = 0; n && k < 1024; k++, n = *(uint64_t *)(n + 8)) { uint64_t r = *(uint64_t *)n; if (r && *(int16_t *)r == id) { size_t q = strlen(nb); snprintf(nb + q, sizeof nb - q, " 物品%d/等级%d", *(int32_t *)(n + 0x44), *(int32_t *)(n + 0x38)); } }
                LOG("【红】副手组技能：换武器后 25 拍技能还没出现，放弃这次（当前组 %d；技能 %d 现有节点：%s）", atomic_load(&g_ss_srv_wset), id, nb[0] ? nb : " 无"); atomic_store(&g_ss_phase, atomic_load(&g_ss_srv_wset) != from ? SS_RELEASED : SS_IDLE); } }
        break; }
    case SS_READY: { int ch = -1;
        if (!atomic_load(&g_ss_cast) && ((ss_node(pl, id, item, &ch) && ch >= 0 && (uint32_t)ch < ch_ready) || (busy && !busy_ready))) { atomic_store(&g_ss_cast, 1); if (atomic_load(&g_ss_early)) LOG("副手组技能：拍 %u 看见施法（剩余 %d 次）", t, ch); }
        if (!busy) busy_ready = 0;
        break; }
    case SS_RELEASED:
        if (dead) { atomic_store(&g_ss_phase, SS_IDLE); break; }
        if (busy) { tidle = 0; break; }
        if (!tidle) tidle = t; if (t - tidle < 2) break;
        if (w != from) { tback = t; tries = 1; swap0(w); atomic_store(&g_ss_phase, SS_BACK); LOG("副手组技能：拍 %u 松手、动作结束 → 切回（当前组 %d）", t, w); }
        else atomic_store(&g_ss_phase, SS_IDLE);
        break;
    case SS_BACK:
        if (w == from || dead) { LOG("副手组技能：拍 %u 已切回第 %d 组（用了 %u 拍）", t, from, t - tback); atomic_store(&g_ss_phase, SS_IDLE); break; }
        if (t - tback >= 8) { if (tries < 3) { tries++; tback = t; if (!busy) swap0(w); LOG("副手组技能：切回 25 拍没成，再试一次"); }
            else { LOG("【红】副手组技能：切回三次都没成，停在第 %d 组（玩家可在物品栏自己切）", w); atomic_store(&g_ss_phase, SS_IDLE); } }
        break;
    }
}


uint64_t dm_swapskill_filter(uint64_t raw) {
    static uint64_t prev; static int inj; uint64_t out = raw; int ph = atomic_load(&g_ss_phase); uint64_t bit = atomic_load(&g_ss_bit);
    if (ph == SS_IDLE && !dm_uiscreen_panel() && !atomic_load(&g_ss_dead_town)) {
        uint64_t rise = raw & ~prev;
        for (int i = 0; i < 6 && rise; i++) { uint64_t m = atomic_load(&g_ss_map[i]), fb = m & 0xffffffffu; if (!fb || !(rise & fb)) continue;
            int slot = (int)((raw & DM_BTN_ZL) ? (m >> 40) & 0xff : (m >> 32) & 0xff), k = slot < 12 ? atomic_load(&g_ss_kind[slot]) : 0;
            if ((k & 4) || ((k & 1) && (!(k & 2) || atomic_load(&g_ss_charges[slot]) > 0))) {
                atomic_store(&g_ss_slot, slot); atomic_store(&g_ss_bit, fb); atomic_store(&g_ss_early, 0); atomic_store(&g_ss_zl, (raw & DM_BTN_ZL) != 0); inj = 0; atomic_store(&g_ss_phase, SS_REQ); out &= ~fb; break; } }
    } else if (ph == SS_REBIND) { if (!(raw & bit)) atomic_store(&g_ss_phase, SS_IDLE); }
    else if (ph == SS_REQ || ph == SS_SWAPPING) { out &= ~bit; if (!(raw & bit)) atomic_store(&g_ss_early, 1); if (atomic_load(&g_ss_zl)) out |= DM_BTN_ZL; }
    else if (ph == SS_READY) {


        if (!(raw & bit) && !atomic_load(&g_ss_cast) && inj < 40) { out |= bit; inj++; if (atomic_load(&g_ss_zl)) out |= DM_BTN_ZL; }
        else if (!(raw & bit)) { if (inj >= 40) LOG("【红】副手组技能：补按 40 次取样还没看见施法，放手"); atomic_store(&g_ss_phase, SS_RELEASED); } }
    prev = raw; return out;
}




#ifndef DM_PLAYER_BUILD
static void test_bindtbl(uint8_t *b, uint64_t pl) {
    static int mode = -1; static int clr[8], nclr, bind_slot, bind_skill, bound, bound_cell, prior_skill, bind_side=-1; if (mode < 0) { const char *e = getenv("DM_TEST_BINDTBL"); int end = 0; mode = 0;
        if (e && !strcmp(e, "log")) mode = 1;
        else if (e && sscanf(e, "set:%d:%d:%d%n", &bind_slot, &bind_skill, &bind_side, &end) == 3 && !e[end] && bind_slot >= 1 && bind_slot <= 4 && bind_skill >= 0 && bind_skill < 512 && bind_side >= 0 && bind_side <= 1) mode = 3;
        else if (e && sscanf(e, "set:%d:%d%n", &bind_slot, &bind_skill, &end) == 2 && !e[end] && bind_slot >= 1 && bind_slot <= 4 && bind_skill >= 0 && bind_skill < 512) mode = 3;
        else if (e && !strncmp(e, "clear:", 6)) { mode = 2; for (const char *c = e + 6; c && *c && nclr < 8; c = strchr(c, ',') ? strchr(c, ',') + 1 : NULL) clr[nclr++] = atoi(c); } }
    if (!mode || !pl) return;
    if (mode == 3 && bound == 1) {
        int32_t *cell = (int32_t *)(b + 0x2d5f0c0 + 28 * bound_cell);
        LOG("[qa] binding slot=%d skill=%d item=%d cell=%d previous=%d", bind_slot, cell[0], cell[1], bound_cell, prior_skill);
        bound = 2;
    }
    if (mode == 3 && !bound && ss_node(pl, bind_skill, -1, NULL)) {
        static const uint32_t logical[4] = {0x1000, 0x8000, 0x4000, 0x200};
        for (int i = 0; i < 6; i++) {
            uint32_t *map = (uint32_t *)(b + 0x2d13cbc + 12 * i);
            if (map[0] != logical[bind_slot - 1] || map[1] >= 12) continue;
            int32_t *cell = (int32_t *)(b + 0x2d5f0c0 + 28 * map[1]);
            prior_skill = cell[0]; bound_cell = (int)map[1];
            cell[0] = bind_skill; cell[1] = -1;
            if(bind_side>=0)((uint8_t *)cell)[8]=(uint8_t)bind_side;
            bound = 1;
            break;
        }
    }
    int32_t cur[32]; for (int i = 0; i < 32; i++) memcpy(&cur[i], b + 0x2d5f0c0 + (uint64_t)i * 28, 4);
    static int32_t prev[32]; static int have; static uint64_t t0; uint64_t now_ns(void); if (!t0) t0 = now_ns();
    if (mode == 2 && nclr && now_ns() - t0 > 3000000000ull) { int32_t emp = -1; for (int i = 0; i < 32 && emp == -1; i++) { int k = 0; for (int j = 0; j < nclr; j++) k |= clr[j] == i; if (!k && (cur[i] < 0 || cur[i] == 0xffff)) emp = cur[i]; }
        for (int j = 0; j < nclr; j++) if (clr[j] >= 0 && clr[j] < 32) { memcpy(b + 0x2d5f0c0 + (uint64_t)clr[j] * 28, &emp, 4); cur[clr[j]] = emp; }
        LOG("测试 DM_TEST_BINDTBL：清了 %d 格（写成空格值 %d）", nclr, emp); nclr = 0; }
    if (have && !memcmp(cur, prev, sizeof cur)) return; memcpy(prev, cur, sizeof cur); have = 1;
    char t[400]; int k = 0; for (int i = 0; i < 32; i++) k += snprintf(t + k, sizeof t - k, "%s%d", i == 16 ? " ‖ " : i ? "," : "", cur[i]);
    LOG("测试 DM_TEST_BINDTBL：绑定表 [%s]", t);
}
#endif
static int eq_slot_pick(uint8_t *b, uint64_t pl, uint64_t it, int *sl) {
    return ((int (*)(uint64_t, uint64_t, int *, int, int))(b + 0x583b60))(pl, it, sl, 1, 0) && ((int (*)(uint64_t, int, uint64_t, int, int))(b + 0x565950))(pl, *sl, it, 1, 0);
}
#ifndef DM_PLAYER_BUILD





static void eq_reject_log(uint8_t *b, uint64_t pl, uint64_t it, uint64_t d, uint32_t code, int sl) {
    int slot_ok = ((int (*)(uint64_t, uint64_t, int *, int, int))(b + 0x583b60))(pl, it, &sl, 1, 0);
    int may = slot_ok ? ((int (*)(uint64_t, int, uint64_t, int, int))(b + 0x565950))(pl, sl, it, 1, 0) : 0;
    int so = 0, dx = 0, lv = 0, rq = ((int (*)(uint64_t, uint64_t, int, int *, int *, int *, int))(b + 0x5706d0))(it, pl, 1, &so, &dx, &lv, sl);
    int rl = ((int (*)(uint64_t, uint64_t))(b + 0x571580))(it, pl);
    int (*stat)(uint64_t, int, int) = (void *)(b + 0x5b9290);
    static char o[600]; size_t k = 0; uint64_t s = *(uint64_t *)(it + 0x88); int ext = s && *(int32_t *)(s + 0x1c) < 0;
    uint64_t arr = s ? *(uint64_t *)(s + (ext ? 0xe8 : 0x30)) : 0, cnt = s ? *(uint64_t *)(s + (ext ? 0xf0 : 0x38)) : 0;
    k += snprintf(o + k, sizeof o - k, "属性%s[%llu]:", ext ? "全" : "基", (unsigned long long)cnt);
    for (uint64_t i = 0; arr && i < cnt && i < 64 && k < sizeof o - 24; i++) { uint32_t key = *(uint32_t *)(arr + 8 * i); k += snprintf(o + k, sizeof o - k, " %u/%u=%d", key >> 16, key & 0xffff, *(int32_t *)(arr + 8 * i + 4)); }
    LOG("装备判定明细：id %u（%.4s）选栏位 %d → %d，能放 %d；需求判定 %d（力量够 %d 敏捷够 %d 等级够 %d）等级需求 %d；品质 %u 标志 %#x 孔 %d；玩家 力量 %d 敏捷 %d 等级 %d；%s",
        *(uint32_t *)(it + 8), (const char *)&code, slot_ok, sl, may, rq, so, dx, lv, rl,
        d ? *(uint32_t *)d : 0, d ? *(uint32_t *)(d + 0x18) : 0, stat(it, 194, 0), stat(pl, 0, 0), stat(pl, 2, 0), stat(pl, 12, 0), o);
}
#endif

static _Atomic int g_char_data;
int dm_char_data(void) { return atomic_load(&g_char_data); }
void dm_actions_pump(void) {
    void dm_build_pump(uint8_t *, uint64_t);
    void dm_effects_pump(uint8_t *, uint64_t);
    void dm_sim_live_pump(uint8_t *, uint64_t);
    void dm_glossary_pump(uint8_t *, uint64_t);
    void dm_shrine_layout_pump(uint8_t *, uint64_t);
    uint8_t *b = ab(); if (!b) { dm_build_pump(0, 0); dm_effects_pump(0, 0); dm_glossary_pump(0, 0); dm_sim_live_pump(0, 0); dm_shrine_layout_pump(0, 0); return; } uint64_t root = *(uint64_t *)(b + 0x2f05458); if (!root) { dm_build_pump(0, 0); dm_effects_pump(0, 0); dm_glossary_pump(0, 0); dm_sim_live_pump(0, 0); dm_shrine_layout_pump(0, 0); return; }
    void dm_pause_pump(void); dm_pause_pump();
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c));
    dm_build_pump(b, pl);
    dm_effects_pump(b, pl);
    DM_PROF(0, "a.shrine_layout", dm_shrine_layout_pump(b, pl));
    dm_glossary_pump(b, pl);
    dm_sim_live_pump(b, pl);
    { static int test = -1; if (test < 0) { const char *e = getenv("DM_TEST_CHAR_DATA"); test = e ? atoi(e) : 0; }
      atomic_store(&g_char_data, pl ? test ? test : *(uint8_t *)(pl + 0x1bd) : 0); }


    { uint64_t inv = pl ? *(uint64_t *)(pl + 0x90) : 0, held = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0; uint32_t id = 0;
      if (held && *(uint32_t *)held == 4) { uint32_t hid = *(uint32_t *)(held + 8);
          if (((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), hid & 0x7f, hid, 4) == held) id = hid; }
      static uint64_t bad; if (held && !id && held != bad) { bad = held; LOG("【红】手上物品：客户端光标物品字段指着的不是有效物品（类型 %u）→ 当作手上空", *(uint32_t *)held); }
      static uint32_t last; if (id != last) { last = id; LOG("手上物品（客户端）：%s（物品 id %u）", id ? "拿着" : "空了", id); }
      dm_hud_set_held(id); }
#ifndef DM_PLAYER_BUILD
    test_bindtbl(b, pl);
#endif
    DM_PROF(0, "a.elysium", dm_elysium_pump());
    { void dm_recipes_pump(uint8_t *, uint64_t); dm_recipes_pump(b, pl); }
    { void dm_gear_equip_pump(uint8_t *, uint64_t); dm_gear_equip_pump(b, pl); }
    { void dm_runeword_pump(void); DM_PROF(0, "a.runeword", dm_runeword_pump()); }
    { void dm_itemcmp_pump(uint8_t *, uint64_t); dm_itemcmp_pump(b, pl); }






    { int dm_hand_hint_pending(void), dm_pause_wanted(void); void dm_hand_hint_done(void), dm_game_notice(uint8_t *, const char *); uint64_t now_ns(void); static uint64_t last; uint64_t now = now_ns();
      if (pl && dm_hand_hint_pending() && !dm_pause_wanted() && (!last || now - last >= 4000000000ull)) { last = now; dm_hand_hint_done();
          struct { const char *k; uint64_t n; } key = { "TransactionResults4", 19 }; const char *t = dm_untag(((const char *(*)(void *))(b + 0x7f5630))(&key));
          if (t && *t && strcmp(t, key.k)) { dm_game_notice(b, t); LOG("手上物品守门：自动化留下的那件收不进背包 → 原版提示「%s」", t); }
          else LOG("【红】手上物品守门：取不到原版串 TransactionResults4，没有提示"); } }
    dm_uiscreen_pump();








    if (atomic_load(&g_rname) && !*(uint8_t *)(root + 0xb8)) { int left = atomic_load(&g_rname) - 1; atomic_store(&g_rname, 0); uint64_t np = a_find(root, "CharacterCreateNamePanel", 0);
        if (np && *(uint8_t *)(np + 0x50)) { static uint8_t pay[0x40]; static uint64_t msg[8]; memset(pay, 0, sizeof pay); *(int32_t *)(pay + 0x10) = 2; memset(msg, 0, sizeof msg); msg[0] = 0x4ae067c6248b6042ull; msg[1] = 0x1e439768f7dd4594ull; msg[2] = (uint64_t)(uintptr_t)pay;
            ((void (*)(uint64_t, void *))*(uint64_t *)(*(uint64_t *)np + 0x28))(np, msg);
            uint64_t in = *(uint64_t *)(np + 0x168); const char *tx = in ? *(const char **)(in + 0x558) : NULL; uint64_t ln = in ? *(uint64_t *)(in + 0x560) : 0;
            int dup = 0; if (tx && ln && ln < 64) { char nm[64]; snprintf(nm, sizeof nm, "%.*s", (int)ln, tx); dup = dm_name_taken(nm); }
            LOG("起名：随机名 → 名字框「%.*s」%s", tx && ln < 64 ? (int)ln : 0, tx ? tx : "", !dup ? "" : left > 0 ? "（已有同名角色，再取一个）" : "【红】（已有同名角色，试满 12 次仍重名）");
            if (dup && left > 0) atomic_store(&g_rname, left); }
        else LOG("起名：名字面板没开，随机名没发"); }
    dm_shop_info s = {0}; dm_points_info p = { .sel_skill = -1 }; uint64_t sel = 0, held = 0;
    if (pl) {
        int (*stat)(uint64_t, int, int) = (void *)(b + 0x5b9290);
        s.gold = stat(pl, 14, 0); s.stash_gold = stat(pl, 15, 0); p.stat_points = stat(pl, 4, 0); p.skill_points = stat(pl, 5, 0);
        uint64_t fm = *(uint64_t *)(root + 0xd0), vp = a_panel(root, "VendorPanelLayout"); uint32_t npc_id = *(uint32_t *)(b + 0x2d5ebfc);
        uint64_t npc = *(uint32_t *)(b + 0x2d5ec00) ? ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d607a0), npc_id & 0x7f, npc_id, 1) : 0;
        s.open = npc && vp && *(uint8_t *)(vp + 0x50);
        if (s.open) {
            int (*price)(uint64_t, uint64_t, int, uint64_t, int, int) = (void *)(b + 0x572ec0); int diff = *(uint8_t *)(b + 0x2d20b88), cls = *(int16_t *)(npc + 4); uint64_t q = *(uint64_t *)(b + 0x2d5ec18);
            const uint64_t cand[2] = { fm ? *(uint64_t *)(fm + 0x190) : 0, fm ? *(uint64_t *)(fm + 0x178) : 0 };
            for (int i = 0; i < 2 && !sel; i++) { uint64_t w = cand[i]; if (!w || *(uint64_t *)w - (uint64_t)(uintptr_t)b != 0x2089ab8 || *(uint32_t *)(w + 0x5c8) != 1) continue;
                sel = ((uint64_t (*)(uint64_t, void *))(b + 0x3cc87c))(w, NULL); if (sel && *(uint32_t *)sel != 4) sel = 0; }
            if (sel) { s.sel_valid = 1; s.sel_id = *(uint32_t *)(sel + 8); uint32_t code = ((uint32_t (*)(uint64_t))(b + 0x570260))(sel); for (int i = 0; i < 3; i++) { char c = (char)(code >> (8 * i)); s.sel_code[i] = c == ' ' ? 0 : c; }


                { uint64_t pp = *(uint64_t *)(sel + 0x38); uint8_t ver = *(uint8_t *)(sel + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(sel + 4);
                  if (pp) { s.sel_gx = *(int32_t *)(pp + 0x10); s.sel_gy = *(int32_t *)(pp + 0x14); }
                  if (t && ci < *(uint32_t *)(t + 0x15c8)) { uint64_t rec = *(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0; s.sel_gw = *(uint8_t *)(rec + 0x11e); s.sel_gh = *(uint8_t *)(rec + 0x11f); } }
                s.buy_price = price(pl, sel, diff, q, cls, 0); s.can_buy = s.buy_price <= s.gold + s.stash_gold; }
            uint64_t inv = *(uint64_t *)(pl + 0x90); held = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0;
            if (held) { s.held_valid = 1; s.held_id = *(uint32_t *)(held + 8); s.sell_price = ((int (*)(uint64_t))(b + 0x576c60))(held) ? price(pl, held, diff, q, cls, 1) : 0; }
        }
        uint64_t tp = a_panel(root, "SkillsTreePanel"); p.tree_open = tp && *(uint8_t *)(tp + 0x50);
        { uint64_t cp = a_panel(root, "CharacterStatsPanel"), ap = a_panel(root, "AdvancedStatsPanel"); p.char_open = cp && *(uint8_t *)(cp + 0x50) && !(ap && *(uint8_t *)(ap + 0x50)); }
        uint64_t fw = fm ? *(uint64_t *)(fm + 0x190) : 0;
        if (p.tree_open && fw && *(uint64_t *)(fw + 0x558) == H_TREE && *(uint64_t *)(fw + 0x568)) { p.sel_skill = **(int32_t **)(fw + 0x568); p.sel_block = a_skill_block(b, pl, p.sel_skill, p.skill_points); p.sel_can_add = !p.sel_block; }
    }






    { static int was; int on = atomic_load(&g_cmp); uint64_t fm = *(uint64_t *)(root + 0xd0);
      if (fm && (on || was)) for (int i = 0; i < 2; i++) { uint64_t w = *(uint64_t *)(fm + (i ? 0x178 : 0x190)); if (w && *(uint64_t *)w - (uint64_t)(uintptr_t)b == 0x2089ab8) *(uint8_t *)(w + 0x632) = (uint8_t)on; }
      was = on; }




    { int sp = atomic_exchange(&g_sort, -1); int dm_itemjob_owner(void);
      if (sp >= 0 && dm_itemjob_owner()) { LOG("sort: page %d dropped, item job %d owns the grids", sp, dm_itemjob_owner()); sp = -1; }
      if (sp >= 0 && pl) { int ok = ((int (*)(uint64_t, int))(b + 0x3c213c))(pl, sp) & 1; if (ok) ((void (*)(uint32_t, int))(b + 0x3c2320))(*(uint32_t *)(pl + 8), sp);
        LOG("整理：页 %d %s", sp, ok ? "已调原版整理" : "原版判定此刻不能整理（手上拿着物品等），没调"); } }






    if (pl) { int tq = atomic_exchange(&g_test_eq, 0); uint64_t inv = *(uint64_t *)(pl + 0x90), it0 = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
        if (tq) { uint32_t fid = 0; for (uint64_t it = tq < 3 ? it0 : 0; it && !fid; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t md = *(uint32_t *)(it + 0xc); int sl = 0;
                if (tq == 1 ? (md == 0 && *(uint8_t *)(d + 0x55) == 0 && ((int (*)(uint64_t, uint64_t, int *, int, int))(b + 0x583b60))(pl, it, &sl, 1, 0)) : md == 1) fid = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
            if (tq == 3) for (uint64_t it = it0; it && !fid; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; int sl = 0; if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && !((int (*)(uint64_t, uint64_t, int *, int, int))(b + 0x583b60))(pl, it, &sl, 1, 0)) fid = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
            if (fid) atomic_store(tq == 2 ? &g_unequip : &g_equip, fid); else LOG("测试：%s", tq == 1 ? "背包里没有能穿的物品" : "身上没有穿着的物品"); }
        uint32_t eq = atomic_exchange(&g_equip, 0), un = atomic_exchange(&g_unequip, 0), id = eq ? eq : un;
        if (id) { uint64_t it = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), id & 0x7f, id, 4), d = it ? *(uint64_t *)(it + 0x10) : 0;
            int busy = !it || !d || ((uint64_t (*)(void))(b + 0x2aa7c0))() != 0 || (inv && *(uint64_t *)(inv + 0x40)) || (((int (*)(uint64_t))(b + 0x474990))(it) & 0xff) || *(uint8_t *)(b + 0x2d5ff5e);
            uint32_t code = it ? ((uint32_t (*)(uint64_t))(b + 0x570260))(it) : 0; const char *res;
            if (busy) res = !it ? "没找到" : "此刻不能做（手上拿着东西 / 物品有待确认的操作 / 交易中），没调";
            else if (eq) { int sl = 0;
                if (*(uint32_t *)(it + 0xc) != 0) res = "不在容器里，没调";
                else if (!eq_slot_pick(b, pl, it, &sl)) { ((void (*)(int, uint64_t, int, int, int))(b + 0x35f120))(3, 0, 0, 0, 0); res = "原版判定穿不了，只播错误音";
#ifndef DM_PLAYER_BUILD
                    eq_reject_log(b, pl, it, d, code, sl);
#endif
                }
                else if (((int (*)(uint64_t, int))(b + 0x3be960))(it, 0) & 1) { struct { uint32_t a; uint8_t c; } t0 = {0, 0}; ((void (*)(uint64_t, int, void *))(b + 0x3bed60))(it, 1, &t0); res = "雇佣兵面板开着：给雇佣兵装备（已调）"; }
                else { ((void (*)(uint64_t, uint64_t))(b + 0x3cf394))(0, it); res = "已调原版装备"; } }
            else { int mine = 0; for (uint64_t q = it0, k = 0; q && k < 512 && !mine; k++) { uint64_t qd = *(uint64_t *)(q + 0x10); if (!qd) break; mine = q == it; q = *(uint64_t *)(qd + 0xb0); }
                if (*(uint32_t *)(it + 0xc) != 1) res = "不是穿在身上的，没调"; else if (s.open) res = "商人页开着，不卸（原版那条路会变成卖掉）";
                else if (!mine) res = "穿在佣兵身上的，不卸（下面那条是卸玩家同一栏位的入口，会把玩家身上那件卸掉；佣兵的装备拖出来）";
                else { ((void (*)(uint64_t, int, int, uint32_t, int))(b + 0x3bb24c))(0, *(uint8_t *)(d + 0x54), 1, 0xffffffffu, 0); res = "已调原版卸下"; } }
            LOG("%s：物品 id %u（%.3s）%s", eq ? "装备" : "卸下", id, (const char *)&code, res); } }








    if (pl) { int ti = atomic_exchange(&g_test_ident, 0); uint64_t inv = *(uint64_t *)(pl + 0x90);
        if (ti == 1) { uint32_t fid = 0, any = 0; int n = 0, un = 0; for (uint64_t it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; it; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; n++; if (!(*(uint32_t *)(d + 0x18) & 0x10)) { un++; if (!fid) fid = *(uint32_t *)(it + 8); } else if (!any && it != *(uint64_t *)(b + 0x2d12920) && *(uint32_t *)(it + 0xc) == 1) any = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
            if (!fid) fid = any; LOG("测试：物品 %d 件，未鉴定 %d 件，取 id %u%s", n, un, fid, un ? "" : "（没有未鉴定的，拿身上一件已鉴定的试发包）"); if (fid) atomic_store(&g_ident, fid); }
        else if (ti == 3) {
            for (uint64_t it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; it; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
                if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && (((uint32_t (*)(uint64_t))(b + 0x570260))(it) & 0xffff) == 0x6d63) { uint64_t dm_diag_srv_item(uint32_t); uint32_t iid = *(uint32_t *)(it + 8); uint64_t su = dm_diag_srv_item(iid), sd = su ? *(uint64_t *)(su + 0x10) : 0;
                    if (sd) { *(uint32_t *)(sd + 0x18) &= ~0x10u; *(uint32_t *)(d + 0x18) &= ~0x10u; } LOG("测试：物品 id %u 标成未鉴定：服务端单位 %s", iid, sd ? "找到，两端已改" : "没找到，没改"); break; }
                it = *(uint64_t *)(d + 0xb0); } }
        else if (ti == 2) atomic_store(&g_tcancel, 1);














        { static uint64_t bf_next; static int bf_left, bf_done; int dm_ui_unstable(void); int req = atomic_exchange(&g_bfill, 0); uint64_t fr = dm_guest_frame();
          if (req) { bf_left = 16; bf_done = 0; bf_next = 0;




              if (getenv("DM_TEST_BELTFILL_SWITCH")) {
#if TARGET_OS_OSX
                  int dm_padmode_kbd(void); dm_padmode_request_glyph(dm_padmode_kbd() ? DM_GLYPH_PAD : DM_GLYPH_KBD); LOG("测试 DM_TEST_BELTFILL_SWITCH：补满开始，请求换键帽");
#else
                  void dm_padmode_request(int); dm_padmode_request(1); LOG("测试 DM_TEST_BELTFILL_SWITCH：补满开始，请求切到手柄版");
#endif
              } }
          int due = bf_left && fr >= bf_next && !dm_ui_unstable(); uint64_t root = *(uint64_t *)(b + 0x2f05458), fh = root ? *(uint64_t *)(root + 0xd0) : 0, bw = due ? belt_now(b, root) : 0;
          if (due) { const char *res = NULL;
              if (!fh || !bw) res = "物品栏没开着 / 找不到腰带控件，没调";
              else if (!(((int (*)(uint64_t))(b + 0x391484))(bw) & 1)) {


                  uint64_t inv = pl ? *(uint64_t *)(pl + 0x90) : 0, it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0, put = 0;
                  for (int k = 0; it && k < 512 && !put; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
                      if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && ((int (*)(uint64_t, int))(b + 0x574bc0))(it, 9) && (((int (*)(uint64_t))(b + 0x3c1b28))(it) & 1)) put = it; it = *(uint64_t *)(d + 0xb0); }
                  if (put) { struct { uint32_t slot; uint8_t fixed; } tgt = {0, 0}; ((void (*)(uint64_t, uint64_t, uint8_t, int, void *))(b + 0x3c1cd0))(put, pl, 0, 1, &tgt); bf_done++; bf_left--; bf_next = fr + 6; }
                  else res = bf_done ? "腰带满了 / 背包里没有能进腰带的药了" : "原版判定现在不能装（腰带满 / 手上有东西 / 没有能进腰带的），没调"; }
              else { uint8_t pay[0x20] = {0}; *(uint32_t *)(pay + 0x10) = 4; pay[0x19] = 1; uint64_t msg[3] = { 0x4ae067c6248b6042ull, 0x1e439768f7dd4594ull, (uint64_t)(uintptr_t)pay };
                  uint64_t of = *(uint64_t *)(fh + 0x190); uint8_t oh = *(uint8_t *)(root + 0xb8); *(uint64_t *)(fh + 0x190) = bw; *(uint8_t *)(root + 0xb8) = 0;
                  ((void (*)(uint64_t, uint64_t *))(b + 0x3911d8))(bw, msg); *(uint64_t *)(fh + 0x190) = of; *(uint8_t *)(root + 0xb8) = oh; bf_done++; bf_left--; bf_next = fr + 6; }
              if (res || !bf_left) { LOG("腰带补满：调了原版自动装入 %d 次，停（%s）", bf_done, res ? res : "到 16 次上限"); bf_left = 0; } } }
        { uint32_t rid = atomic_exchange(&g_repair, 0);
          if (rid) { uint64_t it = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), rid & 0x7f, rid, 4); const char *res;
              if (!it) res = "没找到物品"; else if (!dm_vendor_repair_mode()) res = "不在修理态，没调";
              else if (inv && *(uint64_t *)(inv + 0x40)) res = "手上拿着东西，没调"; else if (((int (*)(uint64_t))(b + 0x474990))(it) & 0xff) res = "物品有待确认的操作，没调";
              else if (!(((int (*)(uint64_t))(b + 0x435b00))(it) & 1)) res = "原版判定这件不用修 / 修不了，没调";
              else { ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int))(b + 0x40acf4))(0, pl, it, 1, 0, 0); res = "已调原版修理"; }
              LOG("修理：物品 id %u %s", rid, res); } }
        uint32_t id = atomic_exchange(&g_ident, 0); int cancel = atomic_exchange(&g_tcancel, 0); int on = *(uint32_t *)(b + 0x2d12928) == 5; uint64_t sc = on ? *(uint64_t *)(b + 0x2d12920) : 0;
        if (id) { uint64_t it = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), id & 0x7f, id, 4), d = it ? *(uint64_t *)(it + 0x10) : 0, pp = it ? *(uint64_t *)(it + 0x38) : 0;
            uint32_t code = it ? ((uint32_t (*)(uint64_t))(b + 0x570260))(it) : 0, md = it ? *(uint32_t *)(it + 0xc) : 9; const char *res;
            if (!sc) res = "不在等目标状态，没调"; else if (!it || !d || !pp) res = "没找到目标"; else if (md > 1) res = "目标不在背包 / 箱子 / 方块 / 身上，没调";
            else if (md == 0 && (*(uint8_t *)(d + 0x55) == 1 || *(uint8_t *)(d + 0x55) == 2)) res = "目标在交易页，原版不认，没调";
            else if (((int (*)(uint64_t))(b + 0x474990))(it) & 0xff) res = "目标有待确认的操作，没调";
            else { uint8_t col[0x30]; memset(col, 0, sizeof col); uint64_t sd = *(uint64_t *)(sc + 0x10), sp = *(uint64_t *)(sc + 0x38); uint32_t sm = *(uint32_t *)(sc + 0xc); int belt = sm == 2;
                ((void (*)(uint64_t))(b + 0x4747b0))(it);
                if (belt) ((void (*)(void *, uint64_t, uint64_t))(b + 0x47135c))(col, inv, sc);
                uint64_t rec = ((int (*)(uint64_t, int))(b + 0x574bc0))(sc, 0x12) ? 0 : ((uint64_t (*)(uint8_t, uint64_t))(b + 0x4d8580))(*(uint8_t *)(sc + 0x1bd), sc); int flag = rec && !*(uint8_t *)(rec + 8);
                uint32_t sxy = sp ? (*(uint32_t *)(sp + 0x10) & 0xffff) | *(uint32_t *)(sp + 0x14) << 16 : 0, txy = (*(uint32_t *)(pp + 0x10) & 0xffff) | *(uint32_t *)(pp + 0x14) << 16;
                ((void (*)(int, uint32_t, int, int, uint32_t, uint32_t, int, int, uint64_t, void *, void *, void *))(b + 0x2cecf0))(flag, *(uint32_t *)(sc + 8), (int)sm, sd ? *(uint8_t *)(sd + 0x55) : 0xff, sxy, id, 4, md == 0 ? *(uint8_t *)(d + 0x55) : 0xff, txy, belt ? col : NULL, belt ? col + 0xc : NULL, belt ? col + 0x18 : NULL);
                res = "已调原版「对物品使用」"; }
            LOG("鉴定：目标 id %u（%.3s）%s", id, (const char *)&code, res); }
        if (cancel && on) { uint64_t old = *(uint64_t *)(b + 0x2d12920); *(uint64_t *)(b + 0x2d12920) = 0; *(uint32_t *)(b + 0x2d12928) = 7; *(uint32_t *)(b + 0x2d12938) = 0;
            if (old) ((void (*)(uint64_t, uint32_t, int, int))(b + 0x2b47dc))(((uint64_t (*)(void))(b + 0x2acecc))(), *(uint32_t *)(b + 0x2d20e4c), 7, -1);
            ((void (*)(void))(b + 0x2aa5f0))(); LOG("鉴定：退出等目标状态"); } }












    if (pl) { int tn = atomic_exchange(&g_test_take, 0);
        if (tn) { uint32_t fid = 0; int best = 0; uint64_t cu = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)), us[3] = {pl, cu ? ((uint64_t (*)(uint64_t))(b + 0x652650))(cu) : 0, cu ? ((uint64_t (*)(uint64_t))(b + 0x652780))(cu) : 0};
            for (int k = 0; k < 3; k++) { uint64_t inv = us[k] ? *(uint64_t *)(us[k] + 0x90) : 0; if (!inv || *(uint32_t *)inv != 0x01020304) continue;
                int cnt = 0, stk = 0; for (uint64_t it = *(uint64_t *)(inv + 0x10); it; it = *(uint64_t *)(*(uint64_t *)(it + 0x10) + 0xb0)) { if (!*(uint64_t *)(it + 0x10)) break; cnt++; stk += *(uint8_t *)(*(uint64_t *)(it + 0x10) + 0x9c) != 0; } if (getenv("DM_TEST_TAKE_LOG")) LOG("测试：单位 %d（%s，同玩家 %d）物品 %d 件，其中成叠 %d", k, k == 0 ? "玩家" : k == 1 ? "0x652650" : "0x652780", us[k] == pl, cnt, stk);
                for (uint64_t it = *(uint64_t *)(inv + 0x10); it; ) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; if (*(uint8_t *)(d + 0x9c) > best) { best = *(uint8_t *)(d + 0x9c); fid = *(uint32_t *)(it + 8); } it = *(uint64_t *)(d + 0xb0); } }
            if (fid) { LOG("测试：高级仓库里叠数最多的是物品 id %u（%d 件），取 %d 件", fid, best, tn); atomic_store(&g_take, (uint64_t)fid << 32 | (uint32_t)tn); } else LOG("测试：没有成叠的物品"); }
        { int nc = atomic_exchange(&g_take_cur, 0); if (nc) { dm_cursor_item_info ci; if (dm_cursor_item(&ci)) atomic_store(&g_take, (uint64_t)ci.id << 32 | (uint32_t)nc); else LOG("取多件：光标下没有物品，不动"); } }








        static struct { uint32_t id, tick; int left, stack, wait; } seq;
        uint64_t tk = atomic_exchange(&g_take, 0);
        if (tk) { uint32_t id = (uint32_t)(tk >> 32); int n = (int)(uint32_t)tk; uint64_t it = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), id & 0x7f, id, 4), d = it ? *(uint64_t *)(it + 0x10) : 0;
            int st = d ? *(uint8_t *)(d + 0x9c) : 0; uint32_t code = it ? ((uint32_t (*)(uint64_t))(b + 0x570260))(it) : 0; const char *res;
            if (!it || !d) res = "没找到"; else if (!*(uint8_t *)(b + 0x2d5ff60)) res = "箱子没开，没调"; else if (!st) res = "不是成叠物品，没调";
            else { if (n > st) n = st; seq.id = id; seq.left = n; seq.stack = -1; seq.wait = 0; res = "逐件取"; }
            LOG("取多件：物品 id %u（%.3s）叠数 %d 取 %d：%s", id, (const char *)&code, st, n, res); }
        if (seq.left > 0) { uint64_t it = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), seq.id & 0x7f, seq.id, 4), d = it ? *(uint64_t *)(it + 0x10) : 0, inv = *(uint64_t *)(pl + 0x90);
            int st = d ? *(uint8_t *)(d + 0x9c) : 0; uint32_t cls = *(uint32_t *)(pl + 4); uint64_t gt = *(uint64_t *)(b + 0x2d20b80);
            if (!it || !st || !*(uint8_t *)(b + 0x2d5ff60) || !inv || cls > 7) seq.left = 0;
            else if (seq.stack >= 0 && (st >= seq.stack || !gt || *(uint32_t *)(gt + 0xc) == seq.tick)) { if (++seq.wait > 60) { LOG("取多件：上一件 1 秒内没被收下（叠数仍 %d），剩 %d 件不取", st, seq.left); seq.left = 0; } }
            else { uint32_t fx = 0, fy = 0; static uint64_t fake[0x620 / 8];
                if (!((int (*)(uint64_t, uint64_t, int, uint32_t *, uint32_t *, int))(b + 0x55f620))(inv, it, *(int32_t *)(b + 0x1dae684 + cls * 8 + 4), &fx, &fy, 0)) { LOG("取多件：背包放不下了，剩 %d 件不取（叠数 %d）", seq.left, st); seq.left = 0; }
                else { fake[0x608 / 8] = it; ((void (*)(void *, uint64_t, int))(b + 0x39d824))(fake, 0, 0); seq.stack = st; seq.tick = gt ? *(uint32_t *)(gt + 0xc) : 0; seq.wait = 0; seq.left--; LOG("取多件：取 1 件到背包格 %u,%u（叠数 %d，之后还要取 %d）", fx, fy, st, seq.left); } } } }




    if (pl) { int bx = atomic_exchange(&g_beltx, -1); if (bx == 1) ((void (*)(int, int))(b + 0x453670))(0x1a, 0); else if (bx == 0) ((void (*)(int, int))(b + 0x454f40))(0x1a, 0);
        int cells = ((int (*)(uint64_t))(b + 0x3bb068))(pl), ex = *(uint8_t *)(b + 0x2d5ff48 + 0x1a) != 0; atomic_store(&g_belt_state, ((cells + 3) / 4 & 0xff) | ex << 8);
        if (bx >= 0) LOG("腰带：%s → 面板 0x1a 状态 %d，%d 排", bx ? "展开" : "收起", ex, (cells + 3) / 4); }
    pthread_mutex_lock(&g_mu); g_shop = s; g_pts = p; pthread_mutex_unlock(&g_mu); atomic_store(&g_open, s.open);
    int (*trade)(uint64_t, uint64_t, int, int, int, int) = (void *)(b + 0x433e50);




    if (atomic_exchange(&g_buy, 0)) { int r = !sel ? -1 : trade(pl, sel, 0, 0, 1, 0); LOG("商人：买选中的货 id %u（%s，货架格 %d,%d 占 %dx%d，价 %d，身上 %d + 箱内 %d）→ %d（1 已发、-1 没选中）", s.sel_id, s.sel_code, s.sel_gx, s.sel_gy, s.sel_gw, s.sel_gh, s.buy_price, s.gold, s.stash_gold, r); }
    if (atomic_exchange(&g_sell, 0)) { int r = held && s.sell_price > 0 ? trade(pl, held, 1, 0, 1, 0) : -1; LOG("商人：卖手上的物品 id %u（卖价 %d，身上 %d）→ %d", s.held_id, s.sell_price, s.gold, r); }






    if (atomic_exchange(&g_test_belt, 0) && pl) { uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; uint32_t pid = 0;
        for (int k = 0; it && k < 512 && !pid; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && ((int (*)(uint64_t, int))(b + 0x574bc0))(it, 9)) pid = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
        if (pid) atomic_store(&g_belt, pid); else LOG("测试：背包里没有药水"); }







    { int want = atomic_exchange(&g_belt_drink, 0), n[4] = {0}, best = 1000, low[4] = {0, 9, 9, 9}; uint32_t uid = 0; uint64_t inv = pl ? *(uint64_t *)(pl + 0x90) : 0, it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
      for (int k = 0; it && k < 512; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
          if (*(uint32_t *)(it + 0xc) == 2) { uint32_t code = ((uint32_t (*)(uint64_t))(b + 0x570260))(it); char s[4]; memcpy(s, &code, 4);
              int kind = s[0] == 'h' && s[1] == 'p' ? 1 : s[0] == 'm' && s[1] == 'p' ? 2 : s[0] == 'r' && s[1] == 'v' ? 3 : 0, tier = s[2] == 's' ? 1 : s[2] == 'l' ? 2 : s[2] - '0';
              if (kind) { n[kind]++; if (tier < low[kind]) low[kind] = tier; if (kind == want && tier < best) { best = tier; uid = *(uint32_t *)(it + 8); } } }
          it = *(uint64_t *)(d + 0xb0); }
      for (int k = 1; k < 4; k++) if (low[k] == 9) low[k] = 1;
      atomic_store(&g_belt_kinds, (n[1] & 0xff) | (n[2] & 0xff) << 8 | (n[3] & 0xff) << 16 | (low[1] | low[2] << 3 | low[3] << 6) << 24);
      if (want) { LOG("腰带：喝第 %d 类，物品 id %u", want, uid); if (uid) { atomic_store(&g_use_merc, 0); atomic_store(&g_use, uid); } } }
    { int bu = atomic_exchange(&g_belt_use, 0), bm = bu >> 8 & 1; bu &= 0xff; if (bu && pl) { uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; uint32_t uid = 0;
        for (int k = 0; it && k < 512 && !uid; k++) { uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d) break;
            if (*(uint32_t *)(it + 0xc) == 2 && pp && (int)(*(uint32_t *)(pp + 0x10) + 4 * *(uint32_t *)(pp + 0x14)) == bu - 1) uid = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
        LOG("腰带：%s第 %d 排第 %d 列 → %s（物品 id %u）", bm ? "喂雇佣兵 " : "喝", (bu - 1) / 4, (bu - 1) % 4, uid ? "有药" : "空格，不动", uid);
        if (uid) { atomic_store(&g_use_merc, bm); atomic_store(&g_use, uid); } } }
    { int tk = atomic_exchange(&g_test_use, 0); if (tk && pl) { uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; uint32_t uid = 0;
        for (int k = 0; it && k < 512 && !uid; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; int (*ty)(uint64_t, int) = (void *)(b + 0x574bc0); uint32_t code = ((uint32_t (*)(uint64_t))(b + 0x570260))(it);
            if (tk == 5 ? *(uint32_t *)(it + 0xc) == 2 : (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && (tk == 1 ? ty(it, 9) : tk == 2 ? ty(it, 0x16) : tk == 3 ? ty(it, 0x12) : tk == 6 ? (code == 0x206b6269 || code == 0x20637369) : code == 0x20786f62))) uid = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }
        if (uid) atomic_store(&g_use, uid); else LOG("测试：背包里没有第 %d 类物品（1 药水 2 卷轴 3 书 4 方块）", tk); } }






    { int loc = atomic_exchange(&g_held_equip, -1); uint64_t h = pl && loc > 0 ? ((uint64_t (*)(void))(b + 0x2aa7c0))() : 0;
      if (h && (loc & 0x100)) {
          struct { uint32_t a; uint8_t c; } t0 = { (uint32_t)(loc & 0xff), 1 }; ((void (*)(uint64_t, int, void *))(b + 0x3bed60))(h, 0, &t0);
          LOG("装备：手上物品 id %u → 佣兵栏位 %d，已调原版给雇佣兵装备", *(uint32_t *)(h + 8), loc & 0xff); }
      else if (h) { int r = ((int (*)(uint64_t, int, uint64_t, int, int))(b + 0x565950))(pl, loc, h, 0, 0); ((void (*)(uint64_t, int, int, int, int))(b + 0x3bb24c))(h, loc, 0, -1, 0);
          LOG("装备：手上物品 id %u → 栏位 %d（原版判定 %d：0 不能放、1 放入、5 交换）已调原版放下", *(uint32_t *)(h + 8), loc, r); }
      else if (loc > 0) LOG("装备：手上没有物品，不动"); }
    DM_PROF(0, "a.skill_cap", skill_cap_pump(b));
    {



        static int at = -1, n, cur, sent, listed; static struct { int sk, cnt; } q[12]; static uint64_t nx;
        if (at < 0) { const char *e = getenv("DM_TEST_SKILL_ADD"); at = 0; if (e) { at = atoi(e); const char *c = strchr(e, ':'); while (c && n < 12 && sscanf(c + 1, "%d:%d", &q[n].sk, &q[n].cnt) == 2) { n++; c = strchr(c + 1, ','); } } }
        uint64_t f = dm_guest_frame(); int (*gs)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(b + 0x5b9290);
        if (at > 0 && pl) {
            if (!listed && f > 300) { listed = 1; char o[400]; int k2 = 0; o[0] = 0; uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0;
                for (int k = 0; nd && k < 512; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (r && *(int32_t *)(nd + 0x44) == -1 && *(int32_t *)(nd + 0x38) > 0 && k2 < 380) k2 += snprintf(o + k2, sizeof o - k2, " %d=%d", *(int16_t *)r, *(int32_t *)(nd + 0x38)); }
                LOG("测试 DM_TEST_SKILL_ADD：职业 %u 等级 %d 剩余技能点 %d，已学技能（号=基础等级）%s", *(uint32_t *)(pl + 4), gs(pl, 12, 0), gs(pl, 5, 0), o); }
            if (f >= (uint64_t)at && f >= nx && cur < n) {
                if (sent < q[cur].cnt) { uint8_t pk[5] = { 0x3b, (uint8_t)q[cur].sk, (uint8_t)(q[cur].sk >> 8), 0, 0 }; ((void (*)(const uint8_t *, int))(b + 0x2ceab0))(pk, 5); sent++; nx = f + 6; }
                else if (f >= nx + 12) { LOG("测试 DM_TEST_SKILL_ADD：技能 %d 发了 %d 个加点包 → 基础等级 %d（剩余技能点 %d）", q[cur].sk, sent, a_skill_base(pl, q[cur].sk), gs(pl, 5, 0)); cur++; sent = 0; } } } }
    {
        static int n = -1; static struct { uint64_t at; uint8_t pk[32]; int len; } q[8];
        if (n < 0) { n = 0; const char *e = getenv("DM_TEST_C2S"); while (e && *e && n < 8) { q[n].at = strtoull(e, NULL, 10); const char *c = strchr(e, ':'); if (!c) break; c++; int l = 0; unsigned v;
                while (l < 32 && sscanf(c, "%2x", &v) == 1 && c[0] != ';' && c[1] != ';' && c[1]) { q[n].pk[l++] = (uint8_t)v; c += 2; } q[n].len = l; n++; e = strchr(c, ';'); if (e) e++; } }
        uint64_t f = dm_guest_frame(); for (int i = 0; pl && i < n; i++) if (q[i].at && f >= q[i].at) { q[i].at = 0; ((void (*)(const uint8_t *, int))(b + 0x2ceab0))(q[i].pk, q[i].len); LOG("测试 DM_TEST_C2S：帧 %llu 发包 %02x（长 %d）", (unsigned long long)f, q[i].pk[0], q[i].len); } }
    { void dm_freetp_client_step(uint8_t *, uint64_t); DM_PROF(0, "a.freetp_client", dm_freetp_client_step(b, pl)); }
    dm_custom_ui_pump(root); DM_PROF(0, "a.auto_pot", auto_pot_pump(b, pl)); DM_PROF(0, "a.swapskill", swapskill_pump(b, pl)); { void dm_kills_ui_pump(void); DM_PROF(0, "a.kills_ui", dm_kills_ui_pump()); } { void dm_rift_ui_pump(void); DM_PROF(0, "a.rift_ui", dm_rift_ui_pump()); } { void dm_vault_ui_pump(void); DM_PROF(0, "a.vault_ui", dm_vault_ui_pump()); } { void dm_looks_ui_pump(void); DM_PROF(0, "a.looks_ui", dm_looks_ui_pump()); }
    uint32_t use = atomic_exchange(&g_use, 0); int force_merc = use ? atomic_exchange(&g_use_merc, 0) : 0;
    if (use && pl) { uint64_t it = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), use & 0x7f, use, 4);
        int busy = *(uint32_t *)(pl + 0xc) == 0x11 || ((uint64_t (*)(void))(b + 0x2aa7c0))() != 0, can = it && !busy && (((int (*)(uint64_t, int))(b + 0x3bff70))(it, 3) & 1);
        int belt = it && *(uint32_t *)(it + 0xc) == 2;
        uint32_t code = it ? ((uint32_t (*)(uint64_t))(b + 0x570260))(it) : 0;




        int merc = it && !busy && (*(uint8_t *)(b + 0x2d5ff66) || force_merc) && !(((int (*)(uint64_t))(b + 0x474990))(it) & 0xff) && (((int (*)(uint64_t))(b + 0x3bf794))(it) & 1);
        uint32_t tgt = *(uint32_t *)(pl + 8);
        if (merc && belt) { tgt = 0xffffffffu; for (uint64_t e = *(uint64_t *)(b + 0x2d33528); e; e = *(uint64_t *)(e + 0x30)) if (*(uint32_t *)(e + 4) == 7 && *(uint32_t *)(e + 0xc) == *(uint32_t *)(pl + 8) && !*(uint32_t *)(e + 0x20)) { tgt = *(uint32_t *)(e + 8); break; }
            if (tgt == 0xffffffffu) merc = 0, tgt = *(uint32_t *)(pl + 8); }
        if (force_merc && !merc) can = 0;




        if ((can || merc) && belt) { uint8_t col[0x30]; memset(col, 0, sizeof col); uint64_t pp = *(uint64_t *)(it + 0x38); ((void (*)(void *, uint64_t, uint64_t))(b + 0x47135c))(col, *(uint64_t *)(pl + 0x90), it);
            uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t rec = ((int (*)(uint64_t, int))(b + 0x574bc0))(it, 0x12) ? 0 : ((uint64_t (*)(uint8_t, uint64_t))(b + 0x4d8580))(ver, it); int flag = rec && !*(uint8_t *)(rec + 8);
            uint32_t xy = pp ? (*(uint32_t *)(pp + 0x10) & 0xffff) | *(uint32_t *)(pp + 0x14) << 16 : 0;
            ((void (*)(int, uint32_t, int, int, uint32_t, uint32_t, int, int, uint64_t, void *, void *, void *))(b + 0x2cecf0))(flag, use, 2, 0xff, xy, tgt, merc, 0xff, 0, col, col + 0xc, col + 0x18);
            ((void (*)(uint64_t))(b + 0x4747b0))(it);
            if (merc) ((void (*)(uint32_t, int, int))(b + 0x4bd710))(tgt, 1, 0x5b);
            else {




                int snd = ((int (*)(uint64_t))(b + 0x470e00))(it);
                ((void (*)(int, uint64_t, int, int, int))(b + 0x35f120))(snd, 0, 0, 0, 0);
                LOG("使用：腰带物品 id %u（%.3s）已发，播使用音 %d", use, (const char *)&code, snd);
            } }
        else if (merc) ((void (*)(uint64_t))(b + 0x3bf860))(it);
        else if (can) ((void (*)(uint64_t, int))(b + 0x3c0098))(it, 3);
        LOG("使用：物品 id %u（%.3s）%s", use, (const char *)&code, !it ? "没找到" : busy ? "此刻不能用（手上拿着东西 / 玩家状态不允许），没发" : merc ? (force_merc ? "腰带双击：喂给雇佣兵（已调）" : "雇佣兵页开着：喂给雇佣兵（已调）") : force_merc ? "腰带双击：雇佣兵不在 / 这瓶不能喂，不喂也不自己喝" : can ? "已调原版使用" : "原版判定不可使用，没发"); }
    uint32_t bid = atomic_exchange(&g_belt, 0);
    if (bid && pl) { uint64_t it = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0 + 4 * 0x400), bid & 0x7f, bid, 4), d = it ? *(uint64_t *)(it + 0x10) : 0;
        int ok = it && d && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && (((int (*)(uint64_t))(b + 0x3c1b28))(it) & 1);
        if (ok) { struct { uint32_t slot; uint8_t fixed; } tgt = {0, 0}; ((void (*)(uint64_t, uint64_t, uint8_t, int, void *))(b + 0x3c1cd0))(it, pl, *(uint8_t *)(d + 0x55), 1, &tgt); }
        LOG("腰带：物品 id %u %s", bid, !it ? "没找到" : ok ? "放进腰带（已发）" : "不能放（不在背包里 / 类型不对 / 腰带满），不动"); }












    if (atomic_exchange(&g_test_swap, 0) && pl) { uint64_t inv = *(uint64_t *)(pl + 0x90), ok = inv && *(uint32_t *)inv == 0x01020304, held = ok ? *(uint64_t *)(inv + 0x40) : 0, it = ok ? *(uint64_t *)(inv + 0x10) : 0; int done = 0;
        for (int k = 0; held && it && k < 512 && !done; k++) { uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d) break;
            if (it != held && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && pp) { uint8_t pk[21] = {0x1f}; uint32_t hid = *(uint32_t *)(held + 8), id = *(uint32_t *)(it + 8), x = *(uint32_t *)(pp + 0x10), y = *(uint32_t *)(pp + 0x14); uint16_t xy[4] = { (uint16_t)x, (uint16_t)y, (uint16_t)x, (uint16_t)y };
                memcpy(pk + 1, &hid, 4); memcpy(pk + 5, &id, 4); memcpy(pk + 13, xy, 8); ((void (*)(void *, int))(b + 0x2ceab0))(pk, 21); LOG("测试：手上 id %u 与背包里 id %u（格 %u,%u）交换", hid, id, x, y); done = 1; }
            it = *(uint64_t *)(d + 0xb0); }
        if (!done) LOG("测试：没有可交换的（手上没物品 / 背包里没有别的物品）"); }




    enum { STOW_TRY = 90 };
    if (atomic_load(&g_stow)) { static int tries, why0; int r = 0, why = 0; atomic_fetch_add(&g_stow_tick, 1); uint64_t inv = pl ? *(uint64_t *)(pl + 0x90) : 0, held = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x40) : 0, tgt = 0;
        static int noroom = -1; if (noroom < 0) noroom = getenv("DM_TEST_STOW_NOROOM") != NULL;
        if (!held || *(uint32_t *)held != 4) why = 1;
        else if (noroom) why = 4;
        else if (((int (*)(uint64_t))(b + 0x474990))(held) & 1) why = 2;
        else if (!(tgt = pl)) why = 3;
        else { uint64_t d = *(uint64_t *)(held + 0x10); struct { uint64_t pos; uint8_t fixed; } o = {0, 0};
            r = ((int (*)(uint64_t, uint64_t, int, int, int, void *))(b + 0x3bdb10))(held, tgt, 0, d ? *(uint8_t *)(d + 0x55) : 0xff, 0, &o) & 1; if (!r) why = 4; }
        if (!tries) why0 = why;
        if (r || why == 1 || ++tries >= STOW_TRY) { static const char *const wn[] = { "", "手上没有物品", "物品暂时不可放（main+0x474990）", "取不到目标单位", "背包里找不到位置" };
            LOG("手上物品收进背包：%s（试了 %d 帧；第一帧：%s，最后：%s）", r ? "已放入（已发）" : why == 1 && !tries ? "手上没有物品" : "没放成，物品留在手上", tries + 1, r && !tries ? "直接放成" : wn[why0], r ? "放成" : wn[why]);
            tries = 0; atomic_store(&g_stow, 0); atomic_store(&g_stow_why, r ? 0 : why); atomic_store(&g_stow_res, r); } }


    if (atomic_exchange(&g_test_hold, 0) && pl) { uint64_t inv = *(uint64_t *)(pl + 0x90), it0 = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0, it; int done = 0;
        for (int want = 0; want <= 4 && !done; want += 4) { it = it0;
        for (int k = 0; it && k < 512 && !done; k++) { uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d) break;
            if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == want && pp && ((int (*)(uint64_t))(b + 0x576c60))(it)) { uint8_t pk[17] = {0x19}; uint32_t id = *(uint32_t *)(it + 8), x = *(uint32_t *)(pp + 0x10), y = *(uint32_t *)(pp + 0x14), pg = *(uint8_t *)(d + 0x55);
                memcpy(pk + 1, &id, 4); memcpy(pk + 5, &x, 4); memcpy(pk + 9, &y, 4); memcpy(pk + 13, &pg, 4); ((void (*)(void *, int))(b + 0x2ceab0))(pk, 17); ((void (*)(uint64_t))(b + 0x4747b0))(it); LOG("测试：拿起物品 id %u（页 %u 格 %u,%u）", id, pg, x, y); done = 1; }
            it = *(uint64_t *)(d + 0xb0); } }
        if (!done) LOG("测试：背包里没有能卖的物品可拿"); }


    if (atomic_exchange(&g_test_sell, 0) && pl) { uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; int done = 0;
        for (int k = 0; it && k < 512 && !done; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            if (s.open && (*(uint32_t *)(it + 0xc) == 1 || (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0)) && ((int (*)(uint64_t))(b + 0x576c60))(it)) { uint64_t npc = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d607a0), *(uint32_t *)(b + 0x2d5ebfc) & 0x7f, *(uint32_t *)(b + 0x2d5ebfc), 1);
                int pr = npc ? ((int (*)(uint64_t, uint64_t, int, uint64_t, int, int))(b + 0x572ec0))(pl, it, *(uint8_t *)(b + 0x2d20b88), *(uint64_t *)(b + 0x2d5ec18), *(int16_t *)(npc + 4), 1) : 0;
                int r = trade(pl, it, 1, 0, 1, 0); LOG("测试：直接卖物品 id %u（模式 %u，卖价 %d，身上 %d）→ %d", *(uint32_t *)(it + 8), *(uint32_t *)(it + 0xc), pr, s.gold, r); done = 1; }
            it = *(uint64_t *)(d + 0xb0); }
        if (!done) LOG("测试：没有能卖的物品 / 商人页没开"); }
    int st = atomic_exchange(&g_stat, -1);
    if (st >= 0 && pl) { a_send(b, root, H_STATS, H_USEPT, st >> 8, st & 0xff); LOG("加点：属性序号 %d 模式 %d（2 = 1 点、3 = 5 点、1 = 全部），发令前剩余属性点 %d", st >> 8, st & 0xff, p.stat_points); }
    int dm_ui_skill_spend_ok(void); int sk = atomic_exchange(&g_skill, 0); if (sk && !dm_ui_skill_spend_ok()) { sk = 0; LOG("加点：确认之后界面重建过 / 正在切换布局 → 这一下不加（选中的可能已不是点的那个技能，再点一次）"); }
    if (sk) { int go = p.sel_skill >= 0 && p.sel_can_add; if (go) a_send(b, root, H_TREE, H_USEPT, p.sel_skill, 0); LOG("加点：技能 %d（不能加的原因 %d：0 可加 1 没点数 2 等级不够 3 前置没学 4 其它 5 没打通过秘境、已到 20；剩余技能点 %d）%s", p.sel_skill, p.sel_block, p.skill_points, go ? "已发" : p.sel_skill >= 0 ? "不能加，没发" : "没有选中的技能，忽略");
        if (!go && p.sel_skill >= 0 && p.sel_block == 5) { static uint64_t last; uint64_t f = dm_guest_frame(); if (!last || f - last >= 120) { last = f; void dm_game_notice(uint8_t *, const char *); dm_game_notice(b, dm_str(b, "dmSkillCapLocked")); LOG("加点：提示「%s」", dm_str(b, "dmSkillCapLocked")); } } }
}
