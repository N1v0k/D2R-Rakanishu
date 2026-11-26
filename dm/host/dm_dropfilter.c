









#include "dm_custom.h"
#include "game_addrs.h"
#include "hle.h"
#include "dm.h"
#include <stdlib.h>
#include <string.h>

static uint8_t *main_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }



enum { T_RING = 10, T_AMUL = 12, T_SCHA = 83, T_MCHA = 84, T_LCHA = 85 };
static int code_in(uint32_t c, const char *const *l) { for (; *l; l++) if (!memcmp(&c, *l, 4)) return 1; return 0; }















static int light_tier(uint8_t *b, uint64_t u) {
    static int force = -2; if (force == -2) { const char *e = getenv("DM_DROP_LIGHT"); force = e ? strcmp(e, "0") != 0 : -1; }
    if (!(force >= 0 ? force : dm_custom_get("drop_light"))) return 0;
    int (*is)(uint64_t, int) = (int (*)(uint64_t, int))(b + GA_ITEM_IS_TYPE);
    uint32_t code = ((uint32_t (*)(uint64_t))(b + GA_ITEM_CODE))(u);
    const uint8_t *d = (const uint8_t *)(uintptr_t)*(uint64_t *)(u + 0x10); uint32_t q = d ? *(const uint32_t *)d : 0;
    static const char *const high_rune[] = {"r23 ", "r24 ", "r25 ", "r26 ", "r27 ", "r28 ", "r29 ", "r30 ", "r31 ", "r32 ", "r33 ", 0},
        *const quest[] = {"pk1 ", "pk2 ", "pk3 ", "tes ", "ceh ", "bet ", "fed ", "toa ", 0};
    if (q == 7 || code_in(code, high_rune)) return 1;
    if (q == 5 || is(u, T_RING) || is(u, T_AMUL) || is(u, T_SCHA) || is(u, T_MCHA) || is(u, T_LCHA) || code_in(code, quest)) return 2;
    return 0;
}
void svc_ground_asset(uint64_t *regs) {
    uint8_t *b = main_base(); uint64_t u = regs[1]; uint8_t *out = (uint8_t *)(uintptr_t)regs[8];
    {   register uint64_t x0 __asm__("x0") = regs[0], x1 __asm__("x1") = u; register uint64_t x8 __asm__("x8") = regs[8]; register uint64_t x16 __asm__("x16") = (uint64_t)(uintptr_t)(b + GA_UNIT_MODEL_PATH);
        __asm__ volatile("blr x16" : "+r"(x0), "+r"(x1), "+r"(x8), "+r"(x16) : : "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x17", "x30",
            "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31", "cc", "memory"); }
    if (!u || *(uint32_t *)u != 4 || (*(uint32_t *)(u + 0xc) != 3 && *(uint32_t *)(u + 0xc) != 5)) return;
    int tier = light_tier(b, u); if (!tier) return;


    char *s = *(char **)out; uint64_t len = *(uint64_t *)(out + 8), cap = *(uint64_t *)(out + 0x10) & 0x7FFFFFFFFFFFFFFFull;
    static const char pre[] = "data/hd/items/"; if (!s || len < sizeof pre + 5 || strncmp(s, pre, sizeof pre - 1) || strcmp(s + len - 5, ".json")) return;
    const char *root = s + sizeof pre - 1, *slash = strchr(root, '/'); if (!slash || slash >= s + len - 5) return;
    char nw[256]; int n = snprintf(nw, sizeof nw, "%.*szswg/%d__", (int)(slash + 1 - s), s, tier);
    for (const char *p = slash + 1; p < s + len - 5 && n < (int)sizeof nw - 8; p++) {
        if (*p == '/') { nw[n++] = '_'; nw[n++] = '_'; } else nw[n++] = (char)(*p >= 'A' && *p <= 'Z' ? *p + 32 : *p); }
    n += snprintf(nw + n, sizeof nw - n, ".json");
    static int log = -1; if (log < 0) log = getenv("DM_DROPF_LOG") != NULL;
    if ((uint64_t)n >= cap) { if (log) LOG("掉落光：%s 改指 %s 放不下（容量 %llu）", s, nw, (unsigned long long)cap); return; }
    if (log) LOG("掉落光：物品 id %u 模式 %u 第 %d 档 %s → %s", *(uint32_t *)(u + 8), *(uint32_t *)(u + 0xc), tier, s, nw);
    memcpy(s, nw, (size_t)n + 1); *(uint64_t *)(out + 8) = (uint64_t)n;
}









enum { TIER1_SOUND = 40 };
void svc_drop_sound(uint64_t *regs) {
    uint8_t *b = main_base(); uint64_t u = regs[0];
    uint32_t snd = ((uint32_t (*)(uint64_t, uint64_t))(b + GA_ITEM_DROP_SOUND))(u, regs[1]); regs[0] = snd;
    { void dm_drop_haptic_land(uint8_t *, uint64_t); dm_drop_haptic_land(b, u); }
    if (!u || *(uint32_t *)u != 4 || light_tier(b, u) != 1) return;
    static int alt = -2; if (alt == -2) { const char *e = getenv("DM_TEST_DROP_SOUND"); alt = e ? atoi(e) : -1; }
    regs[0] = (uint32_t)(alt > 0 ? alt : TIER1_SOUND);
    static int log = -1; if (log < 0) log = getenv("DM_DROPF_LOG") != NULL;
    if (log) LOG("掉落音：物品 id %u 第 1 档 原音 %u → 新音 %u", *(uint32_t *)(u + 8), snd, (uint32_t)regs[0]);
}
