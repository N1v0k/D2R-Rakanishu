


























#include "dm.h"
#include "game_addrs.h"
#include "hle.h"
#include "dm_hud.h"
#include "dm_uiscreen.h"
#include <math.h>
#include <mach/mach.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static const struct { uint16_t lv; uint8_t act, drlg; int8_t varies; } k_lv[] = {
#include "dm_bright_levels.inc"
};

static int rd(uint64_t a, void *buf, size_t n) { vm_size_t got = 0; return a && !vm_read_overwrite(mach_task_self(), a, n, (vm_address_t)buf, &got) && got == n; }
static uint64_t main_base(void) { static uint64_t b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint64_t)g_mod[i].base; return b; }
static uint64_t env_ptr(void) { uint64_t act = 0, env = 0, mb = main_base(); return mb && rd(mb + GA_CLIENT_ACT, &act, 8) && rd(act + 8, &env, 8) ? env : 0; }



static double target_logk(int *why, int *area, int *I, double *ang) {
    *I = -1; *ang = -1; *area = -1;
    if (!dm_uiscreen_ingame()) { *why = 1; return 0; }
    dm_hud_info h; dm_hud_get(&h); *area = h.valid ? h.area : -1;
    int act = -1, drlg = -1, varies = -1;
    for (unsigned i = 0; i < sizeof k_lv / sizeof *k_lv; i++) if (k_lv[i].lv == *area) { act = k_lv[i].act; drlg = k_lv[i].drlg; varies = k_lv[i].varies; break; }
    uint64_t env = env_ptr(); int32_t iv = -1, flag = 0, rate = 0; float t = 0;
    if (env && rd(env + 0xc, &iv, 4) && rd(env + 0x34, &flag, 4) && rd(env + 0x2c, &rate, 4) && rd(env + 0x8, &t, 4)) { *I = iv; if (rate > 0) *ang = t / rate; }
    if (drlg == 1) { *why = 2; return log(4.0); }
    int pinned = act == 3 || *area == 120 || *area == 137 || flag != 0;
    if (varies == 0 && !pinned) { *why = 3; return log(4.0); }
    *why = 4;
    if (*I < 0) return log(4.0);
    double e = (255.0 - (*I > 255 ? 255 : *I)) / 191.0; if (e < 0) e = 0; if (e > 1) e = 1;
    return e * log(4.0);
}

int dm_custom_get(const char *key);
float dm_bright_k(void) {
    static int fixed = -2; static float kf;
    if (fixed == -2) { const char *e = getenv("DM_BRIGHT_K"); fixed = e != NULL; if (e) { kf = (float)atof(e); LOG("DM_BRIGHT_K=%g：3D 画面曝光由测试开关固定", kf); } }
    if (fixed) return kf;
    static double logk = 0; static uint64_t last; static int lwhy = -1, larea = -2, llvl = -1; static uint64_t llog; static double ltgt = -1;
    uint64_t t = now_ns();
    { int dm_loadscreen_up(void); if (dm_loadscreen_up()) { last = t; static int tr = -1; static unsigned n; if (tr < 0) tr = getenv("DM_BRIGHT_TRACE") != NULL; if (tr && n++ % 6 == 0) LOG("3D 画面加亮：k %.3f（读取画面中，保持）", exp(logk)); return (float)exp(logk); } }
    int why, area, I; double ang; double tgt = target_logk(&why, &area, &I, &ang);


    int lvl = dm_custom_get("bright"); tgt *= lvl <= 0 ? 0 : lvl == 1 ? 0.5 : 1;
    double dt = last ? (t - last) / 1e9 : 0; last = t; if (dt > 0.034) dt = 0.034;
    logk += (tgt - logk) * (1 - exp(-dt / 0.33));
    if (why != lwhy || area != larea || lvl != llvl || fabs(tgt - ltgt) > 0.05 || t - llog > 10000000000ull) {
        LOG("3D 画面加亮：目标 k %.2f（规则 %s，区域 %d，原版环境亮度 %d，太阳角 %.0f°，档 %d），当前 k %.2f", exp(tgt),
            why == 1 ? "①局外" : why == 2 ? "②迷宫类" : why == 3 ? "③原版画面不随昼夜" : "④按原版环境亮度", area, I, ang, lvl, exp(logk));
        lwhy = why; larea = area; llvl = lvl; llog = t; ltgt = tgt; }
    { static int tr = -1; static unsigned n; if (tr < 0) tr = getenv("DM_BRIGHT_TRACE") != NULL; if (tr && n++ % 6 == 0) LOG("3D 画面加亮：k %.3f（目标 %.3f，规则 %d）", exp(logk), exp(tgt), why); }
    return (float)exp(logk);
}

void dm_bright_server_tick(uint64_t G) {
    static int ta = -2; static unsigned at;
    if (ta == -2) { const char *e = getenv("DM_TEST_DAYTIME"); ta = -1; if (e) { int a; unsigned t; if (sscanf(e, "%d@%u", &a, &t) == 2) { ta = a; at = t; } } }
    uint32_t tick = 0; if (ta < 0 || !rd(G + 0x170, &tick, 4) || tick < at) return;
    uint64_t acts = 0; int n = 0; rd(G + 0x190, &acts, 8);




    int idx = -1, kind = 0; uint64_t mb = main_base(); int32_t tbl[18];
    if (mb && rd(mb + GA_DAYLIGHT_PERIODS, tbl, sizeof tbl)) { int best = -1; for (int i = 0; i < 6; i++) if (tbl[3 * i] <= ta && tbl[3 * i] > best) { best = tbl[3 * i]; idx = i; kind = tbl[3 * i + 1]; } }
    uint64_t envs[6]; int ne = 0;
    for (int i = 0; acts && i < 5; i++) { uint64_t act = 0, env = 0; if (rd(acts + 8 * i, &act, 8) && rd(act + 8, &env, 8)) envs[ne++] = env; }
    uint64_t ce = env_ptr(); if (ce) envs[ne++] = ce;
    for (int i = 0; i < ne; i++) { int32_t rate = 0, fl = 0; if (!rd(envs[i] + 0x2c, &rate, 4) || rate <= 0 || !rd(envs[i] + 0x34, &fl, 4) || fl) continue;
        if (idx >= 0) { *(int32_t *)(uintptr_t)envs[i] = idx; *(int32_t *)(uintptr_t)(envs[i] + 4) = kind; }
        *(float *)(uintptr_t)(envs[i] + 8) = (float)(ta * rate); n++; }
    LOG("DM_TEST_DAYTIME：拍 %u 把 %d 份环境（服务端各幕 + 客户端%s）写成太阳角 %d°、时段 %d / 类型 %d", tick, n, ce ? "" : "，客户端没取到", ta, idx, kind);
    ta = -1;
}
