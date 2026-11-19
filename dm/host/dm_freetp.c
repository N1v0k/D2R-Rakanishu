



















#include "hle.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { SK_TP_V3 = 411, SRVDO_TP = 109, ZR_EMPTY = 0xffffffffu };
static _Atomic int g_req;
void dm_tp_request(void) { atomic_store(&g_req, 1); }



static int tp_skill_id(uint8_t *mb, uint64_t player) {
    uint8_t ver = *(uint8_t *)(player + 0x1bd); if (ver < 1 || ver > 3) return -1;
    uint64_t T = *(uint64_t *)(mb + 0x2d64a18 + (uint64_t)ver * 16); if (!T) return -1;
    uint64_t arr = *(uint64_t *)(T + 0x11e8), cnt = *(uint64_t *)(T + 0x11f0);
    if (arr && cnt > SK_TP_V3 && *(uint16_t *)(arr + SK_TP_V3 * 0x2ec + 0x4e) == SRVDO_TP && (*(uint8_t *)(arr + SK_TP_V3 * 0x2ec + 0x25) & 4)) return SK_TP_V3;
    for (uint64_t i = 0; arr && i < cnt; i++) if (*(uint16_t *)(arr + i * 0x2ec + 0x4e) == SRVDO_TP && (*(uint8_t *)(arr + i * 0x2ec + 0x25) & 4)) return (int)i;
    return -1;
}

















static void grant(uint8_t *mb, uint64_t unit, int srv) {
    int sk = tp_skill_id(mb, unit); if (sk < 0) return;
    uint64_t sl = *(uint64_t *)(unit + 0x100); for (uint64_t n = sl ? *(uint64_t *)sl : 0, k = 0; n && k < 512; n = *(uint64_t *)(n + 8), k++) { uint64_t rec = *(uint64_t *)n; if (rec && *(int16_t *)rec == sk) return; }
    uint64_t nd = ((uint64_t (*)(uint64_t, int))(mb + 0x7bf010))(unit, sk); static int lg; if (lg++ < 8) LOG("免费回城：%s玩家技能链挂上回城技能 %d → %s", srv ? "服务端" : "客户端", sk, nd ? "成" : "【红】失败");
}













static _Atomic int g_zr_free[2];
int dm_freetp_zr_free(int zl) { return atomic_load(&g_zr_free[zl != 0]); }
void dm_freetp_client_step(uint8_t *mb, uint64_t pl) {
    if (!mb || !pl) { atomic_store(&g_zr_free[0], 0); atomic_store(&g_zr_free[1], 0); return; }
    grant(mb, pl, 0);
    int dm_swapskill_wset(void); int fr[2] = { 0, 0 }; uint32_t ids[2] = { 0, 0 }, ids2[2] = { 0, 0 };
    for (int i = 0; i < 6; i++) { const uint8_t *e = mb + 0x2d13cbc + 12 * i; uint32_t lg, sl[2]; memcpy(&lg, e, 4); memcpy(sl, e + 4, 8); if (lg != 0x800) continue;
        for (int k = 0; k < 2; k++) if (sl[k] < 12) { memcpy(&ids[k], mb + 0x2d5f0c0 + (uint64_t)sl[k] * 28, 4); memcpy(&ids2[k], mb + 0x2d5f0c0 + (uint64_t)(16 + sl[k]) * 28, 4);
            fr[k] = ids[k] == ZR_EMPTY && ids2[k] == ZR_EMPTY; } }
    static uint32_t last[4] = { 1, 1, 1, 1 }; if (ids[0] != last[0] || ids[1] != last[1] || ids2[0] != last[2] || ids2[1] != last[3]) { last[0] = ids[0]; last[1] = ids[1]; last[2] = ids2[0]; last[3] = ids2[1];
        LOG("免费回城：ZR 这一格绑的技能 I 组 %d / II 组 %d（按着 ZL：%d / %d；当前武器组 %d）→ ZR %s，ZL+ZR %s", (int)ids[0], (int)ids2[0], (int)ids[1], (int)ids2[1], dm_swapskill_wset(),
            fr[0] ? "空着，当免费回城" : "已绑技能，交还原版", fr[1] ? "空着，当免费回城" : "已绑技能，交还原版"); }
    atomic_store(&g_zr_free[0], fr[0]); atomic_store(&g_zr_free[1], fr[1]);
}
void dm_freetp_server_step(uint8_t *mb, uint64_t G, uint64_t player) {
    if (mb && player) grant(mb, player, 1);


#ifndef DM_PLAYER_BUILD
    { static char q[128]; static int init; if (!init) { init = 1; const char *e = getenv("DM_TEST_FREETP"); if (e) strlcpy(q, e, sizeof q); }
      if (q[0] && G) { uint32_t t = *(uint32_t *)(uintptr_t)(G + 0x170), at = (uint32_t)atoi(q); if (t >= at) { const char *c = strchr(q, ','); memmove(q, c ? c + 1 : q + strlen(q), strlen(c ? c + 1 : "") + 1); atomic_store(&g_req, 1); LOG("DM_TEST_FREETP：拍 %u 当作点了回城钮", t); } } }
#endif
    if (!atomic_exchange(&g_req, 0) || !mb || !G || !player) return;
    uint32_t pm = *(uint32_t *)(player + 0xc); if (pm == 0 || pm == 17) { LOG("免费回城：玩家死了，不开"); return; }
    int sk = tp_skill_id(mb, player);
    if (sk < 0) {
        int ok = ((int (*)(uint64_t, uint64_t, int))(mb + 0x64c8e0))(G, player, SK_TP_V3) & 1;
        if (ok) ((void (*)(uint64_t, uint64_t))(mb + 0x64ce50))(G, player);
        LOG("免费回城：数据版本 %u 没有回城技能，直接建门 + 连门 → %s", *(uint8_t *)(player + 0x1bd), ok ? "已开门" : "原版不让开（城里等）");
        return;
    }
    uint64_t sl = *(uint64_t *)(player + 0x100), node = 0;
    for (uint64_t n = sl ? *(uint64_t *)sl : 0, k = 0; n && k < 512 && !node; n = *(uint64_t *)(n + 8), k++) { uint64_t rec = *(uint64_t *)n; if (rec && *(int16_t *)rec == sk) node = n; }
    int added = 0; if (!node) { node = ((uint64_t (*)(uint64_t, int))(mb + 0x7bf010))(player, sk); added = 1; }
    if (!node) { LOG("【红】免费回城：给玩家加回城技能 %d 失败", sk); return; }
    uint64_t pp = *(uint64_t *)(player + 0x38); int x = pp ? *(uint16_t *)(pp + 2) : 0, y = pp ? *(uint16_t *)(pp + 6) : 0;
    uint64_t old_r = sl ? *(uint64_t *)(sl + 0x10) : 0; if (sl) *(uint64_t *)(sl + 0x10) = node;
    int r = ((int (*)(uint64_t, uint64_t, int, int, int, uint64_t))(mb + 0x6e77f0))(G, player, x, y, 1, node);
    if (sl) *(uint64_t *)(sl + 0x10) = old_r;
    uint32_t pm2 = *(uint32_t *)(player + 0xc);
    LOG("免费回城：%s回城技能 %d，脚下 %d,%d → %s（返回 %d，模式 %u → %u）", added ? "加上" : "已有", sk, x, y, pm2 == 10 && pm != 10 ? "开始施法" : "原版没放（城里 / 正在施法等）", r, pm, pm2);
}
