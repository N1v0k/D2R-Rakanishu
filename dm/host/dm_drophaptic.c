






















#include "dm_custom.h"
#include "game_addrs.h"
#include "hle.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { K_CODE, K_QUAL, K_TYPE_QMIN, K_SOCK };
typedef struct { uint8_t tier, kind; uint16_t a, b; const char *code; const char *note; } dh_rule;
enum { T_RING = 10, T_AMUL = 12, T_WEAP = 45, T_ARMO = 50, T_JEWL = 58, T_SCHA = 83, T_MCHA = 84, T_LCHA = 85 };
static const dh_rule k_rules[] = {
    { 1, K_CODE, 0, 0, "r27 ", "Ohm" }, { 1, K_CODE, 0, 0, "r28 ", "Lo" }, { 1, K_CODE, 0, 0, "r29 ", "Sur" }, { 1, K_CODE, 0, 0, "r30 ", "Ber" }, { 1, K_CODE, 0, 0, "r31 ", "Jah" },
    { 2, K_CODE, 0, 0, "r23 ", "Mal" }, { 2, K_CODE, 0, 0, "r24 ", "Ist" }, { 2, K_CODE, 0, 0, "r25 ", "Gul" }, { 2, K_CODE, 0, 0, "r26 ", "Vex" }, { 2, K_CODE, 0, 0, "r32 ", "Cham" }, { 2, K_CODE, 0, 0, "r33 ", "Zod" },
    { 2, K_QUAL, 7, 0, NULL, "暗金（含暗金护身符 / 珠宝）" },
    { 2, K_CODE, 0, 0, "pk1 ", "恐惧之钥" }, { 2, K_CODE, 0, 0, "pk2 ", "憎恨之钥" }, { 2, K_CODE, 0, 0, "pk3 ", "毁灭之钥" },
    { 2, K_CODE, 0, 0, "tes ", "痛苦精华" }, { 2, K_CODE, 0, 0, "ceh ", "憎恨精华" }, { 2, K_CODE, 0, 0, "bet ", "恐惧精华" }, { 2, K_CODE, 0, 0, "fed ", "毁灭精华" }, { 2, K_CODE, 0, 0, "toa ", "赦免之证" },
    { 3, K_CODE, 0, 0, "r21 ", "Pul" }, { 3, K_CODE, 0, 0, "r22 ", "Um" },
    { 3, K_QUAL, 5, 0, NULL, "套装" },
    { 3, K_TYPE_QMIN, T_SCHA, 4, NULL, "魔法及以上小护身符" }, { 3, K_TYPE_QMIN, T_MCHA, 4, NULL, "魔法及以上中护身符" }, { 3, K_TYPE_QMIN, T_LCHA, 4, NULL, "魔法及以上大护身符" },
    { 3, K_TYPE_QMIN, T_JEWL, 4, NULL, "魔法及以上珠宝" }, { 3, K_TYPE_QMIN, T_RING, 6, NULL, "稀有戒指" }, { 3, K_TYPE_QMIN, T_AMUL, 6, NULL, "稀有项链" },
    { 3, K_SOCK, 4, 0, NULL, "4 孔及以上的白 / 灰底材（符文之语底材）" },
};

static _Atomic int g_pending;

static int tier_of(uint8_t *b, uint64_t u, const char **why) {
    int (*is)(uint64_t, int) = (int (*)(uint64_t, int))(b + GA_ITEM_IS_TYPE);
    uint32_t code = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(u);
    const uint8_t *d = (const uint8_t *)(uintptr_t)*(uint64_t *)(u + 0x10); uint32_t q = d ? *(const uint32_t *)d : 0;
    for (unsigned i = 0; i < sizeof k_rules / sizeof *k_rules; i++) { const dh_rule *r = &k_rules[i]; int hit = 0;
        switch (r->kind) {
        case K_CODE: hit = !memcmp(&code, r->code, 4); break;
        case K_QUAL: hit = q == r->a; break;
        case K_TYPE_QMIN: hit = q >= r->b && is(u, r->a); break;
        case K_SOCK: hit = q <= 3 && (is(u, T_WEAP) || is(u, T_ARMO)) && ((int (*)(uint64_t, int, int))(b + GA_STAT_GET))(u, 194, 0) >= r->a; break; }
        if (hit) { *why = r->note; return r->tier; } }
    return 0;
}



void dm_drop_haptic_land(uint8_t *b, uint64_t u) {
    static int env = -2; if (env == -2) { const char *e = getenv("DM_DROP_HAPTIC"); env = e ? e[0] != '0' : -1; }
    if (!b || !u || *(uint32_t *)u != 4 || !(env >= 0 ? env : dm_custom_get("drop_haptic"))) return;
    uint32_t id = *(uint32_t *)(u + 8);
    const char *why = NULL; int t = tier_of(b, u, &why); if (!t) return;
    int cur = atomic_load(&g_pending); while ((!cur || t < cur) && !atomic_compare_exchange_weak(&g_pending, &cur, t)) {}
    LOG("掉落震动：物品 id %u 第 %d 档（%s）", id, t, why);
}

int dm_drop_haptic_take(void) { return atomic_exchange(&g_pending, 0); }
