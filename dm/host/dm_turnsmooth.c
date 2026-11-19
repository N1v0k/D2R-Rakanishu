





































#include "dm.h"
#include "hle.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TURN_COS 0.94
#define YAW_PAD_MIN 360.0f
#define YAW_TURN_OFF 999.0f
#define ERR_MAX 26214.4f

static uint8_t *g_b; static int g_log, g_dry, g_trace; static float g_yaw_min; static int g_yaw_trace, g_yaw_stat, g_yaw_native; static struct { unsigned off; float v; } g_set[8]; static int g_nset;
static struct { uint64_t path; int ok, moving; int32_t x, y, dx, dy; float tau; double per_tick; } g_p;

static uint64_t local_player(void) { return ((uint64_t (*)(uint32_t))(g_b + 0x45f9d0))(*(uint32_t *)(uintptr_t)(g_b + 0x2d20e4c)); }

static void ts_obs(const uint64_t *regs) {
    float frame = *(const float *)(const void *)&regs[32];
    uint64_t G = *(uint64_t *)(uintptr_t)(g_b + 0x2d20b80); float tau = G ? (float)(*(uint32_t *)(uintptr_t)(G + 8) - *(uint32_t *)(uintptr_t)(G + 0xc)) / 40.0f : 0;
    uint64_t pl = local_player(), pp = pl ? *(uint64_t *)(uintptr_t)(pl + 0x38) : 0;
    if (!G || !pp || !*(uint8_t *)(uintptr_t)(pp + 0x260)) { g_p.ok = 0; return; }
    int32_t x = *(int32_t *)(uintptr_t)pp, y = *(int32_t *)(uintptr_t)(pp + 4), dx, dy; memcpy(&dx, (void *)(uintptr_t)(pp + 0x8e), 4); memcpy(&dy, (void *)(uintptr_t)(pp + 0x92), 4);
    int moving = (*(uint8_t *)(uintptr_t)(pp + 0x48) >> 5) & 1; float *ex = (float *)(uintptr_t)(pp + 0x2a8), *ey = (float *)(uintptr_t)(pp + 0x2ac);
    int same_dir = g_p.ok && g_p.path == pp && dx == g_p.dx && dy == g_p.dy;
    if (g_p.ok && g_p.path == pp && moving && g_p.moving) {
        double n0 = hypot(g_p.dx, g_p.dy), n1 = hypot(dx, dy);

        if (same_dir && frame > 0) g_p.per_tick = hypot((double)x - g_p.x, (double)y - g_p.y) / frame;
        else if (!same_dir && n0 > 0 && n1 > 0 && g_p.per_tick > 0 && ((double)g_p.dx * dx + (double)g_p.dy * dy) / (n0 * n1) < TURN_COS) {

            double k = g_p.per_tick * g_p.tau; float ox = *ex, oy = *ey;
            float tx = (float)(ox + (g_p.dx / n0 - dx / n1) * k), ty = (float)(oy + (g_p.dy / n0 - dy / n1) * k);
            tx = fminf(ERR_MAX, fmaxf(-ERR_MAX, tx)); ty = fminf(ERR_MAX, fmaxf(-ERR_MAX, ty)); if (!g_dry) { *ex = tx; *ey = ty; }
            if (g_log) LOG("转向平滑：方向 (%d,%d) → (%d,%d)，误差 (%.0f,%.0f) → (%.0f,%.0f)（单位 1/65536 格），每拍位移 %.0f，上一帧外推 %.2f 拍%s", g_p.dx, g_p.dy, dx, dy, ox, oy, tx, ty, g_p.per_tick, g_p.tau, g_dry ? "（只看不写）" : "");
        }
    }
    if (g_log) { static int cnt = -1;
        if (g_p.ok && g_p.moving && !moving) { cnt = 30; LOG("转向平滑：停步这一帧误差 (%.0f,%.0f)", *ex, *ey); }
        else if (cnt > 0 && --cnt == 0) LOG("转向平滑：停步后第 30 帧误差 (%.0f,%.0f)，在走 %d", *ex, *ey, moving); }
    if (g_trace) LOG("转向帧迹 %d %d %.0f %.0f %d %.3f %.3f", x, y, *ex, *ey, moving, tau, frame);
    g_p.path = pp; g_p.ok = 1; g_p.moving = moving; g_p.x = x; g_p.y = y; g_p.dx = dx; g_p.dy = dy; g_p.tau = tau;
}

static void yaw_obs(const uint64_t *regs) {
    uint64_t a = regs[1], comp = a ? *(uint64_t *)(uintptr_t)(a + 8) : 0, h = a ? *(uint64_t *)(uintptr_t)(a + 0x20) : 0, unit = h ? *(uint64_t *)(uintptr_t)(h + 8) : 0;
    static uint64_t lp_comp; static uint64_t n, t0, shared;
    if (g_yaw_stat) { uint64_t t = guest_ns(); n++; if (!t0) t0 = t; if (comp && comp == lp_comp && unit != local_player()) shared++;
        if (t - t0 >= 5000000000ull) { LOG("模型转身参数：观测门 %d 在 %.1f 秒内触发 %llu 次（%.0f 次 / 秒），别的单位用到本地玩家那个组件 %llu 次", OBS_MODEL_YAW, (t - t0) / 1e9, (unsigned long long)n, n * 1e9 / (double)(t - t0), (unsigned long long)shared); n = 0; shared = 0; t0 = t; } }
    if (!comp || !unit || unit != local_player()) return;
    lp_comp = comp; float *w = (float *)(uintptr_t)comp; static uint64_t dumped;
    if (g_yaw_stat && dumped != comp) { dumped = comp; uint64_t pp = *(uint64_t *)(uintptr_t)(unit + 0x38);
        LOG("模型转身参数（本地玩家，职业 %u，路径模式 0x%x）：手柄转速（度/秒）+10 %.1f +14 %.1f | +18 %.1f +1c %.1f | 鼠标转速 +20 %.1f +24 %.1f | +28 %.1f +2c %.1f | 弹簧直接到位门限 +30 %.1f | 弹簧参数 +34 %.4f | 原地转身门限 手柄 +38 %.1f 其余 +3c %.1f | +40 %.3f +44 %.3f +48 %.3f +4c %.3f +50 %.3f",
            *(uint32_t *)(uintptr_t)(unit + 4), pp ? *(uint32_t *)(uintptr_t)(pp + 0x50) : 0, w[4], w[5], w[6], w[7], w[8], w[9], w[10], w[11], w[12], w[13], w[14], w[15], w[16], w[17], w[18], w[19], w[20]); }
    if (!g_yaw_native) {
        if (w[14] < 181.0f) w[14] = YAW_TURN_OFF;
        if (w[4] < YAW_PAD_MIN) w[4] = YAW_PAD_MIN;
        if (w[6] < YAW_PAD_MIN) w[6] = YAW_PAD_MIN; }
    if (g_yaw_min > 0) w[4] = w[6] = g_yaw_min;
    for (int i = 0; i < g_nset; i++) *(float *)(uintptr_t)(comp + g_set[i].off) = g_set[i].v;
    if (g_yaw_trace) LOG("转身帧迹 %.1f %.4f %.4f", guest_ns() / 1e6, w[1], w[2]);
}

void dm_turnsmooth_install(uint8_t *main_base) {
    g_b = main_base; const char *e = getenv("DM_TURN_SMOOTH"); g_log = getenv("DM_TEST_TURN_LOG") != NULL; g_trace = getenv("DM_TEST_TURN_TRACE") != NULL;
    if (!gate_obs_present(OBS_ERR_SMOOTH)) LOG("转向平滑：产物没有观测门 %d（r42 起），不处理", OBS_ERR_SMOOTH);
    else if (e && atoi(e) == 0) LOG("转向平滑：测试开关 DM_TURN_SMOOTH=0，不挂门（改前改后对照用）");
    else { g_dry = e && atoi(e) == 2; gate_obs_set(OBS_ERR_SMOOTH, ts_obs); }
    { const char *m = getenv("DM_TEST_YAW_MIN"); g_yaw_min = m ? (float)atof(m) : 0; g_yaw_trace = getenv("DM_TEST_YAW_TRACE") != NULL; g_yaw_stat = getenv("DM_TEST_YAW_DUMP") != NULL;
      const char *nv = getenv("DM_TURN_YAW"); g_yaw_native = nv && atoi(nv) == 0;
      if (g_yaw_native) LOG("模型转身参数：测试开关 DM_TURN_YAW=0，本地玩家的手柄转身照原版（门限 100°、下限 180 度 / 秒）");
      if (g_yaw_min > 0) LOG("模型转身参数：测试开关 DM_TEST_YAW_MIN=%.0f，本地玩家手柄转速下限改成这个值", g_yaw_min); }
    { const char *q = getenv("DM_TEST_YAW_SET"); unsigned off; float v; int k;
      while (q && g_nset < 8 && sscanf(q, "%x=%f%n", &off, &v, &k) == 2) { if (off >= 0x10 && off <= 0x50 && !(off & 3)) { g_set[g_nset].off = off; g_set[g_nset++].v = v; LOG("模型转身参数：测试开关 DM_TEST_YAW_SET，本地玩家组件 +0x%x 逐帧写成 %g", off, v); } q += k; if (*q == ',') q++; else break; } }
    if (!gate_obs_present(OBS_MODEL_YAW)) LOG("模型转身参数：产物没有观测门 %d（r42 起），手柄转身照原版", OBS_MODEL_YAW);
    else if (!g_yaw_native || g_yaw_min > 0 || g_yaw_trace || g_yaw_stat || g_nset) gate_obs_set(OBS_MODEL_YAW, yaw_obs);
}
