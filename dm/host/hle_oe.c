





#include "dm_cursor.h"
#include "hle.h"
#include "dm_uiscreen.h"



#include <stdatomic.h>
static _Atomic int g_opmode = -1;
int dm_opmode(void) {
    int m = g_opmode;
    if (m < 0) { const char *e = getenv("DM_OPMODE"); int v = e ? atoi(e) : 1; int exp = -1; atomic_compare_exchange_strong(&g_opmode, &exp, v); m = g_opmode; }
    return m;
}




enum { MSG_FOCUS = 15, MSG_OPMODE = 30, MSG_PERFMODE = 31 };
#define MQ 32
static pthread_mutex_t g_mq_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t g_mq[MQ]; static int g_mqh, g_mqn;
static _Atomic int g_ntf_opmode, g_ntf_perfmode;







static int g_perf_reload;
static void mq_put(uint32_t m, int reload) {
    pthread_mutex_lock(&g_mq_lock);
    if (m == MSG_PERFMODE && reload) g_perf_reload = 1;








    if (m == MSG_FOCUS || m == MSG_OPMODE || m == MSG_PERFMODE)
        for (int i = 0; i < g_mqn; i++)
            if (g_mq[(g_mqh + i) % MQ] == m) { pthread_mutex_unlock(&g_mq_lock); return; }
    if (g_mqn == MQ) die("oe 通知队列溢出（游戏没在取？）");
    g_mq[(g_mqh + g_mqn++) % MQ] = m;
    pthread_mutex_unlock(&g_mq_lock);
}
static void mq_push(uint32_t m) { mq_put(m, 1); }
static void res_push(int reload) { if (getenv("DM_TEST_OE_FOCUS_FIRST")) mq_push(MSG_FOCUS);
    if (g_ntf_perfmode) mq_put(MSG_PERFMODE, reload); else LOG("【红】分辨率要切，但游戏没打开性能模式变化通知"); }
void dm_oe_res_changed(void) { res_push(1); }





void dm_oe_res_resized(void) { res_push(0); }






static _Atomic int g_ui_opmode = -1;
static _Atomic int g_ui_armed; static _Atomic uint32_t g_ui_reads;
uint32_t dm_oe_ui_opmode_reads(void) { return atomic_load(&g_ui_reads); }
void dm_oe_ui_opmode(int m) { g_ui_opmode = m < 0 ? -1 : m; atomic_store(&g_ui_armed, 1); if (g_ntf_opmode) mq_push(MSG_OPMODE); }
void dm_set_opmode(int m) {
    int old = dm_opmode(); if (old == m) return;
    g_opmode = m;
    LOG("oe 运行模式 %d → %d", old, m);
    if (g_ntf_opmode) mq_push(MSG_OPMODE);
    if (g_ntf_perfmode) mq_push(MSG_PERFMODE);
}


static _Atomic int g_focus = 1; static _Atomic uint64_t g_focus_in_ns;
int dm_focus_now(void) { return atomic_load(&g_focus); }
uint64_t dm_focus_in_since(void) { return atomic_load(&g_focus_in_ns); }
void dm_focus_set(int st) {
    if (st < 1 || st > 3) die("dm_focus_set 非法状态 %d", st);
    if (st == 1) atomic_store(&g_focus_in_ns, now_ns());
    if (atomic_exchange(&g_focus, st) == st) return;
    LOG("oe 焦点状态 → %d", st);
    mq_push(MSG_FOCUS);
}
static int opmode(void) { return dm_opmode(); }



HLE_FORCE("_ZN2nn2oe15EnableRecordingEv", "录像开关；宿主无系统录像", void, h_oe_EnableRec, void) {}
HLE_FORCE("_ZN2nn2oe16DisableRecordingEv", "录像开关；宿主无系统录像", void, h_oe_DisableRec, void) {}
HLE_FORCE("_ZN2nn2oe16EnableScreenShotEv", "截图开关；宿主无系统截图", void, h_oe_EnableShot, void) {}
HLE_FORCE("_ZN2nn2oe17DisableScreenShotEv", "截图开关；宿主无系统截图", void, h_oe_DisableShot, void) {}



HLE_FORCE("_ZN2nn2oe16GetOperationModeEv", "运行模式查询；按 DM_OPMODE 给固定值", int, h_oe_GetOpMode, void) { int u = g_ui_opmode; if (atomic_exchange(&g_ui_armed, 0)) atomic_fetch_add(&g_ui_reads, 1); return u >= 0 ? u : opmode(); }
HLE_FORCE("_ZN2nn2oe18GetPerformanceModeEv", "性能模式查询；与运行模式一致", int, h_oe_GetPerfMode, void) { return opmode(); }
HLE_FORCE("_ZN2nn2oe27SetPerformanceConfigurationENS0_15PerformanceModeEi", "设 CPU/GPU 频率档；宿主不调频", void, h_oe_SetPerfCfg, int mode, int cfg) {
    LOG("oe::SetPerformanceConfiguration mode=%d cfg=%#x（忽略）", mode, cfg);
}





#include <unistd.h>
static _Atomic int g_ls_unloads;
void dm_note_loadscreen_unload(void) { atomic_fetch_add(&g_ls_unloads, 1); }
static void *oe_script(void *arg) {
    char *buf = arg; uint64_t t0 = now_ns();
    for (char *save, *tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
        char key[16]; int v; double at;
        if (sscanf(tok, "%15[a-z]=%d@ingame+%lf", key, &v, &at) == 3) {
            static int need;
            need++; while (atomic_load(&g_ls_unloads) < need) usleep(20000);
            usleep((useconds_t)(at * 1e6));
        } else {
            if (sscanf(tok, "%15[a-z]=%d@%lf", key, &v, &at) != 3) die("DM_OE_SCRIPT 项「%s」格式应为 键=值@秒 或 键=值@ingame+秒", tok);
            uint64_t due = t0 + (uint64_t)(at * 1e9), now = now_ns();
            if (due > now) usleep((useconds_t)((due - now) / 1000));
        }
        if (!strcmp(key, "focus")) dm_focus_set(v);
        else if (!strcmp(key, "opmode")) dm_set_opmode(v);
        else die("DM_OE_SCRIPT 未知键「%s」（focus / opmode）", key);
    }
    return NULL;
}
static void oe_script_start(void) {
    const char *e = getenv("DM_OE_SCRIPT"); if (!e) return;
    pthread_t th; if (pthread_create(&th, NULL, oe_script, strdup(e))) die("DM_OE_SCRIPT 线程");
    pthread_detach(th);
}
static pthread_once_t g_oe_once = PTHREAD_ONCE_INIT;

HLE_FORCE("_ZN2nn2oe20GetCurrentFocusStateEv", "前后台状态；由前端 dm_focus_set 驱动", int, h_oe_GetFocus, void) { return g_focus; }
HLE_FORCE("_ZN2nn2oe20SetFocusHandlingModeENS0_17FocusHandlingModeE", "前后台切换策略；游戏设 1（收焦点通知），宿主按通知语义实现", void, h_oe_SetFocusMode, int m) { LOG("oe::SetFocusHandlingMode %d", m); }





























#include <TargetConditionals.h>




static const float g_s3_levels[] = {0.95f, 0.90f, 0.85f, 0.80f, 0.75f, 0.70f, 0.65f, 0.60f, 0.55f, 0.50f};
#define S3_NLV ((int)(sizeof g_s3_levels / sizeof g_s3_levels[0]))


static float s3_level_scale(float cfg, int lv) {
    float last = cfg; int n = 0; if (lv <= 0) return cfg;
    for (int k = 0; k < S3_NLV; k++) if (g_s3_levels[k] < cfg - 1e-4f) { last = g_s3_levels[k]; if (++n == lv) return last; }
    return last;
}


static int s3_max_level(float cfg, float floor) {
    int n = 0; for (int k = 0; k < S3_NLV; k++) if (g_s3_levels[k] < cfg - 1e-4f && g_s3_levels[k] >= floor - 1e-4f) n++;
    return n;
}








#ifdef __ANDROID__
#define S3_DEF_PCT 85
#else
#define S3_DEF_PCT (DM_PHONE ? 75 : 100)
#endif
static _Atomic int g_s3_menu_pct;
static _Atomic unsigned g_s3_rev;
static _Atomic unsigned g_s3_rev_ack;
static float s3_env(void) {
    static float env = -1;
    if (env < 0) { const char *e = getenv("DM_3D_SCALE"); float v = 0;
        if (e) { char *end; v = strtof(e, &end); if (end == e || *end || !(v >= 0.5f && v <= 1.0f)) die("DM_3D_SCALE 非法：%s（要求 0.5–1.0）", e); }
        env = v; }
    return env;
}


#ifndef DM_OE_TEST_DEV_S3D
__attribute__((weak)) int dm_dev_s3d_pct(void) { return 0; }
#else
int dm_dev_s3d_pct(void);
#endif
static int s3_def(void) { int d = dm_dev_s3d_pct(); return d >= 50 && d <= 100 ? d : S3_DEF_PCT; }
static float s3_cfg(void) { float e = s3_env(); if (e > 0) return e; int p = atomic_load(&g_s3_menu_pct); return (p ? p : s3_def()) / 100.0f; }
int dm_3d_scale_default_pct(void) { return s3_def(); }
int dm_3d_scale_pinned(void) { return s3_env() > 0; }
float dm_3d_scale_cfg(void) { return s3_cfg(); }
void dm_3d_scale_menu_set(int pct) {
    if (pct && (pct < 50 || pct > 100)) pct = 0;
    if (atomic_exchange(&g_s3_menu_pct, pct) != pct) { atomic_fetch_add(&g_s3_rev, 1); LOG("3D 比例：选项页设为 %d%%%s，配置值 %.2f", pct ? pct : s3_def(), pct ? "" : dm_dev_s3d_pct() ? "（设备缺省）" : "（平台缺省）", s3_cfg()); }
}
#define S3_FLOOR_DEF 0.60f

















typedef struct { int level, max_level; double period_ms; uint64_t hold_until, last_up; double win_ms; unsigned win_n, win_d;
                 uint16_t wn[30], wd[30]; int nw; int good_run, need_good; double last_r10, last_r60; unsigned run_n; double run_ms; } scale3d_ctl;
#define S3_SEC 1000000000ull


#define S3_BURST_FRAMES 3u
#define S3_BURST_MS 1000.0
static void scale3d_ctl_hold(scale3d_ctl *c, uint64_t now, uint64_t ns) {
    if (now + ns > c->hold_until) c->hold_until = now + ns;
    c->win_ms = 0; c->win_n = c->win_d = 0; c->nw = 0; c->good_run = 0; c->run_n = 0; c->run_ms = 0;
}
static double s3_rate(const scale3d_ctl *c, int k) {
    unsigned n = 0, d = 0; for (int i = 0; i < k && i < c->nw; i++) { n += c->wn[i]; d += c->wd[i]; } return n ? (double)d / n : 0;
}
static int scale3d_ctl_step(scale3d_ctl *c, double iv_ms, uint64_t now) {
    if (now < c->hold_until || iv_ms == 0) return c->level;
    double per = c->period_ms > 0 ? c->period_ms : 1000.0 / 60;


    c->win_n++;
    if (iv_ms < 0 || iv_ms > per * 1.5) { c->run_n++; c->run_ms += iv_ms > 0 ? iv_ms : per; if (c->run_n <= S3_BURST_FRAMES || c->run_ms >= S3_BURST_MS) c->win_d++; }
    else { c->run_n = 0; c->run_ms = 0; }
    c->win_ms += iv_ms > 0 ? iv_ms : per;
    if (c->win_ms < 2000) return c->level;
    memmove(c->wn + 1, c->wn, sizeof c->wn - sizeof c->wn[0]); memmove(c->wd + 1, c->wd, sizeof c->wd - sizeof c->wd[0]);
    c->wn[0] = (uint16_t)c->win_n; c->wd[0] = (uint16_t)c->win_d; if (c->nw < 30) c->nw++;
    int clean = c->win_d == 0; c->win_ms = 0; c->win_n = c->win_d = 0;
    c->last_r10 = s3_rate(c, 5); c->last_r60 = c->nw >= 30 ? s3_rate(c, 30) : 0;
    if (c->level < c->max_level && ((c->nw >= 5 && c->last_r10 > 0.05) || (c->nw >= 30 && c->last_r60 > 0.015))) {
        if (c->last_up && now - c->last_up < 15 * S3_SEC) c->need_good = 45;
        c->level++; scale3d_ctl_hold(c, now, 3 * S3_SEC); return c->level;
    }
    if (clean) { if (++c->good_run >= (c->need_good ? c->need_good : 15) && c->level > 0) { c->level--; c->last_up = now; c->need_good = 0; scale3d_ctl_hold(c, now, 3 * S3_SEC); } }
    else c->good_run = 0;
    return c->level;
}


static _Atomic int g_3d_quality_mode;
void dm_3d_quality_mode_set(int mode) { atomic_store(&g_3d_quality_mode, mode == 1 ? 1 : 0); }
int dm_3d_quality_mode_get(void) { return atomic_load(&g_3d_quality_mode); }
static _Atomic int g_scale3d_level;
static _Atomic int g_scale3d_resumed;
void dm_3d_scale_resumed(void) { atomic_store(&g_scale3d_resumed, 1); }






static const float g_s3g_mult[] = {0.92f, 0.85f, 0.78f, 0.72f, 0.66f, 0.60f};
static _Atomic float g_s3g_scale;
static int g_s3g_reset;
__attribute__((weak)) double xbg_recent_gpu_ms(void) { return -1; }
static int s3g_on(void) { static int on = -1; if (on < 0) { const char *e = getenv("DM_3D_SCALE_GPU"); on = e && *e == '1'; } return on; }
static void s3g_feed(double iv_ms) {
    static double acc; static unsigned n; static uint64_t hold; static int lv, good, maxlv = -1, was_in; static float cfg;
    if (g_s3g_reset) { g_s3g_reset = 0; maxlv = -1; lv = 0; good = 0; atomic_store(&g_s3g_scale, 0.0f); hold = now_ns() + 3 * S3_SEC; }
    if (maxlv < 0) { const char *f = getenv("DM_3D_SCALE_GPU_FLOOR"); cfg = s3_cfg(); float fl = f ? strtof(f, NULL) : 0.40f;
        maxlv = 0; for (int k = 0; k < (int)(sizeof g_s3g_mult / sizeof g_s3g_mult[0]); k++) if (cfg * g_s3g_mult[k] >= fl - 1e-4f) maxlv = k + 1;
        LOG("3D 比例控制（按 GPU 耗时）：配置 %.2f，最多降 %d 档（最低 %.2f）", cfg, maxlv, maxlv ? cfg * g_s3g_mult[maxlv - 1] : cfg); }
    uint64_t now = now_ns();
    if (!dm_ingame_now()) { was_in = 0; acc = 0; n = 0; return; }
    if (!was_in) { was_in = 1; hold = now + 20 * S3_SEC; }
    if (iv_ms > 0) { acc += iv_ms; n++; }
    if (acc < 2000 || !n) return;
    double fr = acc / n, g = xbg_recent_gpu_ms ? xbg_recent_gpu_ms() : -1; acc = 0; n = 0;
    if (now < hold || g <= 0) return;
    int old = lv;
    if (fr > 35.0 && g >= 0.85 * fr && lv < maxlv) { lv++; good = 0; }
    else if (lv > 0 && g < 0.75 * fr) { if (++good >= 5) { lv--; good = 0; } }
    else good = 0;
    if (lv != old) { hold = now + 4 * S3_SEC; atomic_store(&g_s3g_scale, lv ? cfg * g_s3g_mult[lv - 1] : 0.0f);
        LOG("3D 比例控制（按 GPU 耗时）：帧 %.1f ms、GPU %.1f ms → %s到第 %d 档（3D 比例 %.2f）", fr, g, lv > old ? "降" : "升", lv, lv ? cfg * g_s3g_mult[lv - 1] : cfg); }
}
void dm_3d_scale_feed(double iv_ms) {
    static scale3d_ctl c; static int on = -1; static long inj0 = -1, inj1 = -1; static float floor_cfg;
    { unsigned rev = atomic_load(&g_s3_rev);
      if (on >= 0 && rev != atomic_load(&g_s3_rev_ack)) { c.level = 0; c.max_level = s3_max_level(s3_cfg(), floor_cfg); c.need_good = 0; c.last_up = 0; scale3d_ctl_hold(&c, now_ns(), 3 * S3_SEC); atomic_store(&g_scale3d_level, 0); g_s3g_reset = 1;
          LOG("3D 比例控制：配置值改为 %.2f，回到第 0 档（最深第 %d 档 %.2f）", s3_cfg(), c.max_level, s3_level_scale(s3_cfg(), c.max_level)); }
      atomic_store(&g_s3_rev_ack, rev); }
    if (on < 0) { const char *e = getenv("DM_3D_SCALE_DYN"), *j = getenv("DM_3D_SCALE_DYN_INJECT"); on = !(e && !strcmp(e, "0")) && !getenv("DM_THERMAL_NOFALLBACK");
        if (j) { inj0 = atol(j); inj1 = strchr(j, '-') ? atol(strchr(j, '-') + 1) : inj0; }
        const char *fe = getenv("DM_3D_SCALE_FLOOR"); float fl = fe ? strtof(fe, NULL) : S3_FLOOR_DEF;
        floor_cfg = fl; c.max_level = s3_max_level(s3_cfg(), fl);
        LOG("3D 比例控制：%s，配置 %.2f，下限 %.2f（第 %d 档）", on ? "开" : "关", s3_cfg(), s3_level_scale(s3_cfg(), c.max_level), c.max_level); }
    if (!on) return;
    if (s3g_on() && atomic_load(&g_3d_quality_mode) != 1) { s3g_feed(iv_ms); return; }
    if (atomic_load(&g_3d_quality_mode) == 1) { if (c.level) { c.level = 0; LOG("3D 比例控制：画质优先，回到配置值"); } atomic_store(&g_scale3d_level, 0); g_s3g_reset = 1; return; }
    uint64_t now = now_ns();
    if (atomic_exchange(&g_scale3d_resumed, 0)) scale3d_ctl_hold(&c, now, 3 * S3_SEC);
    { static int was; int in = dm_ingame_now();
      if (!in) { was = 0; scale3d_ctl_hold(&c, now, S3_SEC); return; }
      if (!was) { was = 1; if (inj0 < 0) scale3d_ctl_hold(&c, now, 20 * S3_SEC); } }
    { int mtl_fps_target(void); double mtl_display_refresh(void), dm_pace_hz(double, double), r = mtl_display_refresh(), hz = dm_pace_hz(r, 60);
      double p = mtl_fps_target() == 30 ? 1000.0 / dm_pace_hz(r, 30) : hz != 60 ? 1000.0 / hz : 0; if (p != c.period_ms) { c.period_ms = p; scale3d_ctl_hold(&c, now, 2 * S3_SEC); } }
    if (inj0 >= 0) { long f = (long)dm_nvn_frame_count(); double per = c.period_ms > 0 ? c.period_ms : 1000.0 / 60; iv_ms = f >= inj0 && f <= inj1 ? 2 * per : per; }
    int was = c.level, lv = scale3d_ctl_step(&c, iv_ms, now);
    if (lv != was) LOG("3D 比例控制：最近 10 秒掉帧 %.1f%%、60 秒 %.1f%% → %s到第 %d 档（3D 比例 %.2f）%s", c.last_r10 * 100, c.last_r60 * 100, lv > was ? "降" : "升", lv,
                       s3_level_scale(s3_cfg(), lv), c.need_good ? "（刚升又掉：下次升档要 90 秒）" : "");
    atomic_store(&g_scale3d_level, lv);
}
void dm_3d_scale_hot(int hot) { (void)hot; }
static _Atomic float g_scale3d_cur = 1.0f;
float dm_3d_scale_cur(void) { return atomic_load(&g_scale3d_cur); }
static void scale3d_tick(void) {
    static float cur = 1.0f; static module_t *main; static int init;
    if (!init) { init = 1; for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) main = &g_mod[i]; }



    float want_cfg = s3_cfg(); int fresh = atomic_load(&g_s3_rev_ack) == atomic_load(&g_s3_rev), qual = atomic_load(&g_3d_quality_mode) == 1;
    int lv = fresh && !qual ? atomic_load(&g_scale3d_level) : 0; float dyn = s3_level_scale(want_cfg, lv);
    if (s3g_on()) { float g = fresh && !qual ? atomic_load(&g_s3g_scale) : 0; dyn = g > 0 && g < want_cfg ? g : want_cfg; }
    float want = dm_nvn_ruler_truth() ? 1.0f : dyn;
    if (want == cur || !main || dm_nvn_frame_count() < 30) return;
    uintptr_t renderer = *(uintptr_t *)(main->base + 0x31e0848); if (!renderer) return;
    uint32_t ow = *(uint32_t *)(renderer + 0xee88), oh = *(uint32_t *)(renderer + 0xee8c); if (!ow || !oh) return;
    { extern void mtl_rt_force_full(int) __attribute__((weak)); if (mtl_rt_force_full) mtl_rt_force_full(4); }
    float *scale = (float *)(renderer + 0x66a4); dm_guest_write_prepare(scale, sizeof *scale); *scale = want;
    ((void (*)(uint32_t, uint32_t))(main->base + 0x8582f0))(ow, oh);
    cur = want; atomic_store(&g_scale3d_cur, want);
    LOG("3D 渲染比例 %.2f：3D 目标 %ux%u，输出 %ux%u（帧 %llu）", want, *(uint32_t *)(renderer + 0xee78), *(uint32_t *)(renderer + 0xee7c),
        *(uint32_t *)(renderer + 0xee88), *(uint32_t *)(renderer + 0xee8c), (unsigned long long)dm_nvn_frame_count());
}
HLE_FORCE("_ZN2nn2oe25TryPopNotificationMessageEPj", "系统通知队列；前端状态变化入队", unsigned, h_oe_TryPopMsg, uint32_t *msg) {
    scale3d_tick();
    pthread_once(&g_oe_once, oe_script_start);
    pthread_mutex_lock(&g_mq_lock);
    int ok = g_mqn > 0, reload = 1;
    if (ok) { *msg = g_mq[g_mqh]; g_mqh = (g_mqh + 1) % MQ; g_mqn--; }
    if (ok && *msg == MSG_PERFMODE) { reload = g_perf_reload; g_perf_reload = 0; }
    pthread_mutex_unlock(&g_mq_lock);






    if (ok) for (int i = 0; i < 2; i++) if (g_keyres[i]) pthread_setspecific(g_keyres[i], (void *)dm_game_res(i));
    if (ok && *msg == MSG_PERFMODE) dm_uiscreen_res_reread();
    if (ok) LOG("oe 通知被游戏取走：%u", *msg);
    if (ok && (*msg == MSG_OPMODE || *msg == MSG_PERFMODE)) dm_ui_gen_bump(*msg == MSG_OPMODE ? DM_UIGEN_OPMODE : reload ? DM_UIGEN_PERFMODE : DM_UIGEN_RESIZE);
    return ok;
}


HLE_FORCE("_ZN2nn2oe28SetResumeNotificationEnabledEb", "恢复通知开关；宿主的恢复以焦点通知 15 表达", void, h_oe_SetResumeNtf, unsigned on) { (void)on; }
HLE_FORCE("_ZN2nn2oe42SetOperationModeChangedNotificationEnabledEb", "运行模式变化通知开关", void, h_oe_SetOpModeNtf, unsigned on) { g_ntf_opmode = on & 0xff; }
HLE_FORCE("_ZN2nn2oe44SetPerformanceModeChangedNotificationEnabledEb", "性能模式变化通知开关", void, h_oe_SetPerfModeNtf, unsigned on) { g_ntf_perfmode = on & 0xff; }



HLE_FORCE("_ZN2nn2oe18GetDesiredLanguageEv", "系统语言查询；按 DM_LANG 给值", uint64_t, h_oe_GetLang, void) {
    const char *e = getenv("DM_LANG");
    char code[8] = {0};
    strncpy(code, e ? e : "zh-Hans", sizeof code - 1);
    uint64_t v; memcpy(&v, code, 8);
    return v;
}



HLE_FORCE("_ZN2nn2oe17GetPseudoDeviceIdEPNS_4util4UuidE", "伪设备号；宿主固定值", void, h_oe_GetDevId, uint8_t *uuid) {
    static const uint8_t k[16] = {0x78, 0x62, 0x6c, 0x00, 0x53, 0x57, 0x4e, 0x41, 0x54, 0x49, 0x56, 0x45, 0x31, 0x30, 0x30, 0x31};
    memcpy(uuid, k, 16);
}
