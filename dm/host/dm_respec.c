















#include "dm.h"
#include "game_addrs.h"
#include <stdint.h>
enum { RESPEC_BYTE = 0x52, RESPEC_USED = 1, RESPEC_CAN = 2 };
static uint8_t *quest_flags(uint64_t player, unsigned diff) {
    if (!player || *(uint32_t *)player != 0 || diff > 2) return NULL;
    uint64_t pd = *(uint64_t *)(player + 0x10), q = pd ? *(uint64_t *)(pd + 0x40 + 8 * (uint64_t)diff) : 0;
    return q ? *(uint8_t **)q : NULL;
}
static int keep(uint8_t *f, int client, unsigned diff) {
    uint8_t was = f[RESPEC_BYTE], now = (uint8_t)((was | RESPEC_CAN) & ~RESPEC_USED); if (now == was) return 0;
    f[RESPEC_BYTE] = now;
    static int n; if (n++ < 16) LOG("洗点：%s难度 %u 阿卡拉洗点位 %#x → %#x（原版每难度一次，这里留着可以再洗）", client ? "客户端" : "服务端", diff, was, now);
    return 1;
}
void dm_respec_server_step(uint8_t *b, uint64_t G, uint64_t player) {
    if (!b || !G) return;
    unsigned diff = *(uint8_t *)(G + 0x104); uint8_t *f = quest_flags(player, diff); if (!f) return;
    keep(f, 0, diff);
    uint64_t c = *(uint64_t *)(b + GA_CLIENT_QUEST_FLAGS); uint8_t *cf = c ? *(uint8_t **)c : NULL; if (cf) keep(cf, 1, diff);
}
