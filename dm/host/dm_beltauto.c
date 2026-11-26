






























#include "hle.h"
#include "game_addrs.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int dm_custom_get(const char *key);
uint64_t dm_guest_frame(void);
enum { RED = 1, BLUE = 2, PURPLE = 3, OTHER = 4 };
static _Atomic int g_req;
static int g_inv_open;

void dm_beltauto_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (strcmp(cat, "PanelManager") || strcmp(arg, "PlayerInventoryExpansionLayout")) return;
    if (!strcmp(act, "OpenPanel") || !strcmp(act, "OpenExclusivePanel")) { if (!g_inv_open) atomic_store(&g_req, 1); g_inv_open = 1; }
    else if (!strcmp(act, "ClosePanel") || !strcmp(act, "UnloadPanel")) g_inv_open = 0;
}

static uint64_t rd(uint64_t a) { return a ? *(uint64_t *)(uintptr_t)a : 0; }
static uint32_t code_of(uint8_t *b, uint64_t it) { return ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(it); }
static int kind_rank(uint32_t c, int *rank) {
    char s[4]; memcpy(s, &c, 4);
    if (s[0] == 'h' && s[1] == 'p' && s[2] >= '1' && s[2] <= '5') { *rank = s[2] - '0'; return RED; }
    if (s[0] == 'm' && s[1] == 'p' && s[2] >= '1' && s[2] <= '5') { *rank = s[2] - '0'; return BLUE; }
    if (s[0] == 'r' && s[1] == 'v' && (s[2] == 's' || s[2] == 'l')) { *rank = s[2] == 'l' ? 2 : 1; return PURPLE; }
    *rank = 0; return OTHER;
}
static const char *kind_name(int k) { return k == RED ? "红" : k == BLUE ? "蓝" : k == PURPLE ? "紫" : k == OTHER ? "别的" : "空"; }

typedef struct { uint64_t belt[16]; int boxes, nbelt; uint64_t best[4]; int n[4]; char bag[200]; } snap;




static uint32_t g_bad[8], g_tried[8]; static int g_nbad, g_ntried;
static int in_list(const uint32_t *l, int n, uint32_t id) { for (int i = 0; i < n && i < 8; i++) if (l[i] == id) return 1; return 0; }
static void item_where(uint64_t it, char *o, size_t cap) {
    uint64_t d = it ? *(uint64_t *)(uintptr_t)(it + 0x10) : 0, pp = it ? *(uint64_t *)(uintptr_t)(it + 0x38) : 0;
    if (!it || !d) { snprintf(o, cap, "没有这件"); return; }
    snprintf(o, cap, "容器 %u 页 %u 格 %d,%d", *(uint32_t *)(uintptr_t)(it + 0xc), *(uint8_t *)(uintptr_t)(d + 0x55), pp ? *(int32_t *)(uintptr_t)(pp + 0x10) : -1, pp ? *(int32_t *)(uintptr_t)(pp + 0x14) : -1);
}
static int take_snap(uint8_t *b, uint64_t pl, snap *S) {
    memset(S, 0, sizeof *S);
    uint64_t inv = rd(pl + 0x90); if (!inv || *(uint32_t *)(uintptr_t)inv != 0x01020304) return 0;
    uint64_t pages = rd(inv + 0x20); if (!pages || rd(inv + 0x28) < 2) return 0;
    uint64_t belt_item = rd(rd(pages + 0x18) + 0x40);
    int type = belt_item ? ((int (*)(uint64_t))(b + GA_BELT_TYPE))(belt_item) : 2;
    uint8_t ver = *(uint8_t *)(uintptr_t)(pl + 0x1bd); if (ver < 1 || ver > 3) return 0;
    uint64_t T = rd((uint64_t)(uintptr_t)(b + GA_DATA_TABLES + ver * 16)), rows = T ? rd(T + 0x1578) : 0; if (!rows) return 0;
    S->boxes = *(uint8_t *)(uintptr_t)(rows + (uint64_t)type * 0x120 + 4); if (S->boxes > 16) S->boxes = 16;
    uint64_t bg = rd(pages + 0x20 + 0x18); for (int i = 0; i < S->boxes && bg; i++) { S->belt[i] = rd(bg + 8 * (uint64_t)i); S->nbelt += S->belt[i] != 0; }
    int brank[4] = { -1000, -1000, -1000, -1000 };
    for (uint64_t it = rd(inv + 0x10), k = 0; it && k < 1024; k++) { uint64_t d = rd(it + 0x10); if (!d) break;
        if (*(uint32_t *)(uintptr_t)(it + 0xc) == 0 && *(uint8_t *)(uintptr_t)(d + 0x55) == 0) { int r, kd = kind_rank(code_of(b, it), &r);
            uint32_t iid = *(uint32_t *)(uintptr_t)(it + 8);
            if (kd != OTHER && !in_list(g_tried, g_ntried, iid) && r - (in_list(g_bad, g_nbad, iid) ? 100 : 0) > brank[kd]) { brank[kd] = r - (in_list(g_bad, g_nbad, iid) ? 100 : 0); S->best[kd] = it; }
            if (kd != OTHER) { S->n[kd]++; size_t L = strlen(S->bag); uint32_t cd = code_of(b, it); if (L < sizeof S->bag - 6) snprintf(S->bag + L, sizeof S->bag - L, "%s%.3s", L ? "," : "", (char *)&cd); } }
        it = rd(d + 0xb0); }
    return 1;
}
static void belt_text(uint8_t *b, const snap *S, char *o, size_t cap) {
    size_t k = 0; o[0] = 0;
    for (int c = 0; c < 4; c++) { k += (size_t)snprintf(o + k, k < cap ? cap - k : 0, "%s列%d ", c ? " " : "", c + 1);
        for (int r = 0; r * 4 + c < S->boxes; r++) { uint64_t it = S->belt[r * 4 + c]; int rk, kd = it ? kind_rank(code_of(b, it), &rk) : 0;
            uint32_t cd = it ? code_of(b, it) : 0; (void)kd; k += (size_t)snprintf(o + k, k < cap ? cap - k : 0, "%s%.3s", r ? "," : "", it ? (char *)&cd : "_"); } }
}

void dm_beltauto_step(uint8_t *b) {
    { int dm_itemjob_owner(void); if (dm_itemjob_owner()) return; }
    static int active, moves, want_slot = -1; static uint64_t next, deadline; static uint32_t want_id; static char want_cli[64]; static int fails;
    if (!b) return;


    { static int st = -1, at; static char cols[8]; static uint64_t nx; void dm_belt_use(int col, int row);
      if (st < 0) { const char *e = getenv("DM_TEST_BELT_DRINK"); st = e && sscanf(e, "%d:%7s", &at, cols) == 2 ? 0 : 2; }
      uint64_t f = dm_guest_frame(); if (!st && f >= (uint64_t)at && f >= nx) { nx = f + 15; snap D; uint64_t p = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(uintptr_t)(b + GA_LOCAL_PLAYER_SLOT)); int any = 0;
          if (p && take_snap(b, p, &D)) for (const char *q = cols; *q; q++) { int c = *q - '1';
              if (*q == '0' && (D.best[RED] || D.best[BLUE])) { ((void (*)(uint64_t, int))(b + GA_ITEM_USE))(D.best[RED] ? D.best[RED] : D.best[BLUE], 3); any = 1; break; }
              if (c >= 0 && c < 4 && D.belt[c]) { dm_belt_use(c, 0); any = 1; break; } }
          if (!any) { st = 2; LOG("DM_TEST_BELT_DRINK：第 %s 列喝空了（帧 %llu）", cols, (unsigned long long)f); } } }
    if (atomic_exchange(&g_req, 0)) {
        static int env = -2; if (env == -2) { const char *e = getenv("DM_POT_PICK"); env = e ? e[0] != '0' : -1; }
        if (env >= 0 ? env : dm_custom_get("pot_pick")) { active = 1; moves = 0; want_slot = -1; next = 0; deadline = 0; g_ntried = 0; fails = 0; }
        else LOG("开背包补腰带：开关关着，不动");
    }
    if (!active) return;
    uint64_t fr = dm_guest_frame(); if (fr < next) return;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + GA_CLIENT_PLAYER_GET))(*(uint32_t *)(uintptr_t)(b + GA_LOCAL_PLAYER_SLOT)); snap S; char t[160];
    if (!pl || !take_snap(b, pl, &S)) { active = 0; LOG("开背包补腰带：读不到玩家 / 物品栏，不动"); return; }
    if (rd(rd(pl + 0x90) + 0x40)) { active = 0; LOG("开背包补腰带：手上拿着东西，停（已放 %d 瓶）", moves); return; }
    int dm_ui_panel_shown(const char *name);
    if (!moves && want_slot < 0 && !dm_ui_panel_shown("PlayerInventoryExpansionLayout")) { if (!deadline) deadline = fr + 60; if (fr < deadline) return; active = 0; LOG("开背包补腰带：物品栏 60 帧没显示出来，不补"); return; }
    if (dm_ui_panel_shown("ImbueItemsPanel") || dm_ui_panel_shown("HoradricCubeLayout")) { active = 0; LOG("开背包补腰带：开着底座 / 方块（物品栏随它一起开），不补（已放 %d 瓶）", moves); return; }
    if (want_slot >= 0 && !S.belt[want_slot]) { if (fr < deadline) return;
        uint64_t dm_diag_srv_item(uint32_t id); char srv[64], now[64]; item_where(dm_diag_srv_item(want_id), srv, sizeof srv);
        { uint64_t ci = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + GA_UNIT_FIND))((uint64_t)(uintptr_t)(b + GA_CLIENT_UNIT_TABLE + 4 * 0x400), want_id & 0x7f, want_id, 4); item_where(ci, now, sizeof now); }
        LOG("【红】开背包补腰带：第 %d 瓶放进槽 %d 后 60 帧没见进腰带——物品 id %u，放之前客户端：%s；现在客户端：%s；服务端：%s。本轮跳过它，换下一瓶", moves, want_slot, want_id, want_cli, now, srv);
        if (g_ntried < 8) g_tried[g_ntried++] = want_id; if (!in_list(g_bad, g_nbad, want_id)) { if (g_nbad == 8) g_nbad = 7; g_bad[g_nbad++] = want_id; }
        fails++; want_slot = -1; if (!take_snap(b, pl, &S)) { active = 0; return; } }
    else if (want_slot >= 0 && in_list(g_bad, g_nbad, want_id)) { for (int i = 0; i < g_nbad; i++) if (g_bad[i] == want_id) { g_bad[i] = g_bad[--g_nbad]; break; } LOG("开背包补腰带：物品 id %u 上次放不进，这次进了，从失败名单拿掉", want_id); }
    if (!moves && want_slot < 0) { belt_text(b, &S, t, sizeof t); LOG("开背包补腰带：开始，腰带 %d 格（%s），背包 红 %d 蓝 %d 紫 %d（%s），药水合计 %d", S.boxes, t, S.n[RED], S.n[BLUE], S.n[PURPLE], S.bag, S.nbelt + S.n[RED] + S.n[BLUE] + S.n[PURPLE]); }
    want_slot = -1;




















    uint64_t inv_c = rd(pl + 0x90);
    static const int order[3] = { RED, BLUE, PURPLE };
    for (int oi = 0; oi < 3; oi++) {
        int kd = order[oi]; uint64_t it = S.best[kd]; uint32_t ns = 0; if (!it) continue;
        if (!((int (*)(uint64_t, uint64_t, uint32_t *, int))(b + GA_BELT_FIND_SLOT))(inv_c, it, &ns, 1) || (int)ns >= S.boxes) continue;
        int c = (int)ns % 4, r = (int)ns / 4;
        if (!r) { int want = c < 2 ? RED : BLUE;
            if (kd != want && S.best[want]) continue;
            if (kd != want && kd != PURPLE && S.best[PURPLE]) continue; }
        if (!(((int (*)(uint64_t))(b + GA_ITEM_CAN_BELT))(it) & 1)) { active = 0; LOG("开背包补腰带：原版此刻不收这瓶进腰带（如开着底座 / 交易面板），停（已放 %d 瓶）", moves - fails); return; }
        int slot = (int)ns; struct { uint32_t slot; uint8_t fixed; } tgt = { (uint32_t)slot, 1 };
#ifndef DM_PLAYER_BUILD
        static int t_noput = -1; if (t_noput < 0) t_noput = getenv("DM_TEST_BELT_NOPUT") != NULL;
        if (t_noput == 1) { t_noput = 2; LOG("测试 DM_TEST_BELT_NOPUT：这一瓶不真发"); }
        else
#endif
        ((void (*)(uint64_t, uint64_t, uint8_t, int, void *))(b + GA_ITEM_TO_BELT))(it, pl, *(uint8_t *)(uintptr_t)(rd(it + 0x10) + 0x55), 1, &tgt);
        moves++; want_slot = slot; next = fr + 6; deadline = fr + 60; want_id = *(uint32_t *)(uintptr_t)(it + 8); item_where(it, want_cli, sizeof want_cli);
        LOG("开背包补腰带：背包「%.4s」→ 列 %d 第 %d 格（槽 %d，%s列）；物品 id %u，%s", (char *)&(uint32_t){ code_of(b, it) }, c + 1, r + 1, slot, r ? "同类" : "空", want_id, want_cli);
        return;
    }
    active = 0; belt_text(b, &S, t, sizeof t);
    LOG("开背包补腰带：完成，放了 %d 瓶；腰带（%s），背包剩 红 %d 蓝 %d 紫 %d（%s），药水合计 %d", moves - fails, t, S.n[RED], S.n[BLUE], S.n[PURPLE], S.bag, S.nbelt + S.n[RED] + S.n[BLUE] + S.n[PURPLE]);
}
