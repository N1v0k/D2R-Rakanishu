





















#include "hle.h"
#include "game_addrs.h"
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

static _Atomic uint32_t g_req;

void dm_shoprefresh_on_uimsg(const char *cat, const char *act, const char *arg, uint8_t *b) {
    if (strcmp(cat, "PanelManager") || strcmp(act, "ClosePanel") || strcmp(arg, "TouchShopRefresh")) return;
    if (!b) return;
    { int dm_looks_ui_reset(void); if (dm_looks_ui_reset()) return; }
    uint32_t npc = *(volatile uint32_t *)(b + GA_NPC_ID);
    if (!npc) { LOG("商人刷新：点了，但客户端没有在交易的商人，不发"); return; }
    if (*(volatile uint32_t *)(b + GA_SHOP_GAMBLE_MODE)) {
        uint8_t g[9] = {0x38, 2, 0, 0, 0}; memcpy(g + 5, &npc, 4); LOG("商人刷新：赌博页 → 照原版发 0x38 类型 2（商人 %u）", npc);
        ((void (*)(const uint8_t *, int))(b + GA_CLIENT_SEND))(g, 9); return; }
    uint8_t pkt[9] = {0x38, 1, 0, 0, 0}; memcpy(pkt + 5, &npc, 4);
    atomic_store(&g_req, npc);
    LOG("商人刷新：点了 → 发 0x38 类型 1（商人 %u）", npc);
    ((void (*)(const uint8_t *, int))(b + GA_CLIENT_SEND))(pkt, 9);
}

void dm_shoprefresh_c2s(uint64_t G, uint64_t player, const uint8_t *pkt, uint32_t len) {
    if (len != 9 || pkt[0] != 0x38 || !G || !player) return;
    uint32_t type, npc; memcpy(&type, pkt + 1, 4); memcpy(&npc, pkt + 5, 4);
    uint32_t want = atomic_load(&g_req);
    if (type != 1 || !want || npc != want || !atomic_compare_exchange_strong(&g_req, &want, 0)) return;
    if (*(uint32_t *)(uintptr_t)player) { LOG("【红】商人刷新：发包的单位不是玩家，不刷"); return; }
    uint64_t pd = *(uint64_t *)(uintptr_t)(player + 0x10); if (!pd) return;
    uint8_t mode = *(uint8_t *)(uintptr_t)(pd + 0x100); uint32_t cur = *(uint32_t *)(uintptr_t)(pd + 0xf8), cls = *(uint32_t *)(uintptr_t)(pd + 0xfc);
    if (mode != 2 || cur != npc) { LOG("商人刷新：玩家不在跟商人 %u 交易（状态 %u，商人 %u），不刷", npc, mode, cur); return; }
    uint8_t *rec = *(uint8_t **)(uintptr_t)(G + 0x3838); uint64_t n = *(uint64_t *)(uintptr_t)(G + 0x3840);
    for (uint64_t i = 0; rec && i < n; i++, rec += 0x78) if (*(uint16_t *)rec == (uint16_t)cls) {
        rec[0x35] = 1; LOG("商人刷新：商人 %u（类号 %u）店铺记录置待刷新，原版接着清空并重生成货单", npc, cls); return; }
    LOG("【红】商人刷新：没找到商人类号 %u 的店铺记录（共 %llu 条）", cls, (unsigned long long)n);
}
