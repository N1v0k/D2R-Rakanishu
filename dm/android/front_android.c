







#include "../host/dm.h"
#include "../host/front.h"
#include "../host/dm_input.h"
#include "touch_android.h"
#include "front_android.h"
#include "front_view.h"
#include "../ios/Sources/dm_touch_game_state.h"
#include "../host/dm_res_scale.h"
#include "../host/dm_uiscreen.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system_properties.h>
#include <time.h>
#include <unistd.h>

static void *_Atomic g_window;
static _Atomic int g_screen_w, g_screen_h, g_last_w, g_last_h;
static void fold_check(int w, int h);
void front_android_set_window(void *win, int w, int h) {
    int ow = atomic_load(&g_last_w), oh = atomic_load(&g_last_h);
    atomic_store(&g_window, win); atomic_store(&g_screen_w, w); atomic_store(&g_screen_h, h);
    if (!win || w <= 0 || h <= 0) return;
    atomic_store(&g_last_w, w); atomic_store(&g_last_h, h);
    if (ow > 0 && oh > 0 && (w != ow || h != oh)) fold_check(w, h);
}
int front_android_window_size(int *w, int *h) {
    if (!w || !h) return 0;
    *w = atomic_load(&g_last_w); *h = atomic_load(&g_last_h);
    return *w > 0 && *h > 0;
}



static _Atomic int g_refresh_millihz;
void front_android_set_refresh(float hz) {
    int v = isfinite(hz) && hz > 0 && hz < 1000 ? (int)lroundf(hz * 1000) : 0, old = atomic_exchange(&g_refresh_millihz, v);
    if (v && v != old) LOG("display: refresh %.2f Hz", v / 1000.0);
}





#define MODES_MAX 16
static _Atomic int g_modes[MODES_MAX], g_nmodes, g_panel_w, g_panel_h;
void front_android_set_modes(const float *hz, int n, int pw, int ph) {
    int m[MODES_MAX], k = 0, changed = 0;
    for (int i = 0; i < n; i++) {
        if (!isfinite(hz[i]) || hz[i] <= 0 || hz[i] >= 1000) continue;
        int r = (int)lroundf(hz[i]), j = 0;
        while (j < k && m[j] < r) j++;
        if ((j < k && m[j] == r) || k == MODES_MAX) continue;
        for (int t = k; t > j; t--) m[t] = m[t - 1];
        m[j] = r; k++; }
    if (pw < ph) { int t = pw; pw = ph; ph = t; }
    if (k != atomic_load(&g_nmodes) || pw != atomic_load(&g_panel_w) || ph != atomic_load(&g_panel_h)) changed = 1;
    for (int i = 0; i < k; i++) { if (atomic_load(&g_modes[i]) != m[i]) changed = 1; atomic_store(&g_modes[i], m[i]); }
    atomic_store(&g_nmodes, k); atomic_store(&g_panel_w, pw > 0 ? pw : 0); atomic_store(&g_panel_h, ph > 0 ? ph : 0);
    if (changed) { char t[128]; int o = 0; t[0] = 0; for (int i = 0; i < k && o < (int)sizeof t - 8; i++) o += snprintf(t + o, sizeof t - o, "%s%d", i ? "/" : "", m[i]);
        LOG("display: modes %s Hz, panel %dx%d, window %dx%d", k ? t : "?", pw, ph, atomic_load(&g_screen_w), atomic_load(&g_screen_h)); }
}
int dm_display_modes(int *hz, int cap) { int n = atomic_load(&g_nmodes); if (n > cap) n = cap; for (int i = 0; i < n; i++) hz[i] = atomic_load(&g_modes[i]); return n; }
int dm_display_max_hz(void) { int n = atomic_load(&g_nmodes); return n ? atomic_load(&g_modes[n - 1]) : 0; }
double front_display_refresh(char *name, size_t n) {
    if (name && n) *name = 0;
    return atomic_load(&g_refresh_millihz) / 1000.0;
}

void *front_metal_layer(void) { return atomic_load(&g_window); }

void front_run(void (*guest)(void)) {


    if (front_metal_layer()) {
        void uimsg_subscribe(void (*)(const char *, const char *, const char *));
        uimsg_subscribe(dm_android_touch_uimsg);
        dm_pad_hook = dm_android_touch_step;
        dm_touch_game_attach();
        dm_touch_game_live_check(1);
    }
    guest();
}













static _Atomic int g_res_latched, g_res_fb, g_base_w, g_base_h, g_res_w0, g_res_h0, g_res_pct0;
static _Atomic uint64_t g_res_cur;
static uint64_t res_pack(int w, int h, int pct) { return (uint64_t)(pct & 0xffff) << 48 | (uint64_t)(h & 0xffffff) << 24 | (uint64_t)(w & 0xffffff); }
static void res_cur(int *w, int *h, int *pct) { uint64_t r = atomic_load(&g_res_cur); *w = (int)(r & 0xffffff); *h = (int)(r >> 24 & 0xffffff); if (pct) *pct = (int)(r >> 48); }
static int res_pinned(unsigned *w, unsigned *h) { const char *e = getenv("DM_GAME_RES"); return e && sscanf(e, "%ux%u", w, h) == 2; }




static _Atomic int g_dev_pct, g_dev_s3d;
static int res_eff(int pct) { int d = atomic_load(&g_dev_pct); return pct == DM_RES_AUTO && d ? d : pct; }
static int res_plain_auto(int pct) { return pct == DM_RES_AUTO && !atomic_load(&g_dev_pct); }
int dm_res_dev_pct(void) { return atomic_load(&g_dev_pct); }


static _Atomic int g_full, g_full_start, g_full_block, g_full_wait;
int front_android_full(void) { return atomic_load(&g_full); }
int dm_dev_s3d_pct(void) { return atomic_load(&g_dev_s3d); }
static int full_env_off(void) { const char *e = getenv("DM_ANDROID_FULL"); return e && *e == '0'; }
static int full_off_applies(void);
static int base_now(int *W, int *H, int *small);

int dm_res_full(void) {
    if (atomic_load(&g_res_latched)) return atomic_load(&g_full);
    int W, H; return base_now(&W, &H, NULL) && dm_res_full_on(W, H) && !full_env_off() && !full_off_applies(); }
static int base_now(int *W, int *H, int *small) {
    int sw = atomic_load(&g_screen_w), sh = atomic_load(&g_screen_h), pw = atomic_load(&g_panel_w), ph = atomic_load(&g_panel_h); if (sh > sw) { int t = sw; sw = sh; sh = t; }
    if (small) *small = 0;
    if (pw > 0 && ph > 0 && (sw <= 0 || (sw >= pw && sh >= ph))) { *W = pw; *H = ph; return 1; }
    if (sw > 0 && sh > 0) { *W = sw; *H = sh; if (small) *small = pw > 0; return 1; }
    *W = *H = 0; return 0;
}
uint64_t front_game_res(int docked) {
    unsigned pw, ph;
    if (res_pinned(&pw, &ph)) return docked ? (uint64_t)ph << 32 | pw : (uint64_t)(ph * 4 / 5) << 32 | (pw * 4 / 5 + 3) / 4 * 4;
    int W, H, w, h, is_auto = 1;
    if (atomic_load(&g_res_latched)) { int pct; W = atomic_load(&g_base_w); H = atomic_load(&g_base_h); res_cur(&w, &h, &pct); is_auto = res_plain_auto(pct); }
    else if (!base_now(&W, &H, NULL) || !dm_res_scale_size_x(DM_RES_AUTO, W, H, dm_res_full(), &w, &h)) { W = 1600; H = 900; w = 1600; h = 900; }
    if (docked) return (uint64_t)h << 32 | (unsigned)w;
    int hw, hh; dm_res_handheld_x(w, h, is_auto, W, H, dm_res_full(), &hw, &hh); return (uint64_t)hh << 32 | (unsigned)hw;
}


int dm_res_screen(int *W, int *H) {
    if (atomic_load(&g_res_latched)) { *W = atomic_load(&g_base_w); *H = atomic_load(&g_base_h); return *W > 0 && *H > 0; }
    return base_now(W, H, NULL); }
int dm_res_running(int *w, int *h) {
    unsigned pw, ph; if (res_pinned(&pw, &ph)) { *w = (int)pw; *h = (int)ph; return 1; }
    if (!atomic_load(&g_res_latched)) { *w = *h = 0; return 0; }
    res_cur(w, h, NULL); return *w > 0; }
int dm_res_fallback(void) { return atomic_load(&g_res_fb); }
int dm_res_pinned(void) { unsigned w, h; return res_pinned(&w, &h); }





int dm_custom_get(const char *key); int dm_custom_path(const char *name, char *p, size_t n); void dm_custom_res_reset(void);
static int res_boot_read(void) { char p[1200]; int n = 0; if (!dm_custom_path("res_boot", p, sizeof p)) return 0; FILE *f = fopen(p, "r"); if (f) { if (fscanf(f, "%d", &n) != 1) n = 0; fclose(f); } return n; }
static void res_boot_write(int n) { char p[1200], t[1210]; if (!dm_custom_path("res_boot", p, sizeof p)) return;
    if (!n) { unlink(p); return; }
    snprintf(t, sizeof t, "%s.tmp", p); FILE *f = fopen(t, "w"); if (!f) return; int ok = fprintf(f, "%d\n", n) > 0; ok = (fflush(f) == 0) & ok; ok = (fsync(fileno(f)) == 0) & ok; ok = (fclose(f) == 0) & ok; if (ok) rename(t, p); }













int dm_game_win_init_size(int *w, int *h); void dm_oe_res_resized(void); void dm_oe_res_changed(void); int dm_opmode(void);
static pthread_mutex_t g_live_mu = PTHREAD_MUTEX_INITIALIZER;
static dm_res_live_t g_live; static int g_live_pct, g_live_prev_pct, g_live_fold; static struct timespec g_live_t0;
static _Atomic int g_live_busy, g_live_failed, g_live_broken, g_live_off;
static int live_env(void) { const char *e = getenv("DM_RES_LIVE"); return !(e && *e == '0'); }
__attribute__((weak)) const char *dm_android_runtime_dir(void);
static int product(char *out, size_t n) {
    char p[1200], line[160]; out[0] = 0; const char *rd = dm_android_runtime_dir ? dm_android_runtime_dir() : NULL; if (!rd || !*rd) return 0;
    snprintf(p, sizeof p, "%s/pack_info.txt", rd); FILE *f = fopen(p, "r"); if (!f) return 0;
    while (fgets(line, sizeof line, f)) if (!strncmp(line, "PRODUCT=", 8)) { snprintf(out, n, "%s", line + 8); out[strcspn(out, "\r\n")] = 0; break; }
    fclose(f); return out[0] != 0; }
static void live_file(int fails, int pending) { char p[1200], t[1210]; if (!dm_custom_path("res_live", p, sizeof p)) return;
    if (!fails && !pending) { unlink(p); return; }
    snprintf(t, sizeof t, "%s.tmp", p); FILE *f = fopen(t, "w"); if (!f) return; int ok = fprintf(f, "%d %d\n", fails, pending) > 0; ok = (fflush(f) == 0) & ok; ok = (fsync(fileno(f)) == 0) & ok; ok = (fclose(f) == 0) & ok; if (ok) rename(t, p); }
static int g_live_fails;
static void live_boot(void) {
    char p[1200], pr[96] = "", off[96] = ""; int fails = 0, pending = 0, o = 0; product(pr, sizeof pr);
    if (dm_custom_path("res_live_off", p, sizeof p)) { FILE *f = fopen(p, "r"); if (f) { if (!fgets(off, sizeof off, f)) off[0] = 0; fclose(f); off[strcspn(off, "\r\n")] = 0;
        if (!pr[0] || !strcmp(off, pr)) { atomic_store(&g_live_off, 1); LOG("res live: 【红】当场切换关着（res_live_off，产物 %s：切换中连续两次没撑住）", off[0] ? off : "?"); }
        else { unlink(p); LOG("res live: 新产物 %s（上次关掉的是 %s），当场切换重新打开", pr, off); } } }
    if (!dm_custom_path("res_live", p, sizeof p)) return; FILE *f = fopen(p, "r"); if (!f) return; if (fscanf(f, "%d %d", &fails, &pending) != 2) fails = pending = 0; fclose(f);
    fails = dm_res_live_boot(fails, pending, &o); g_live_fails = fails;
    if (pending) LOG("res live: 【红】上次切换中进程没了（连续第 %d 次）", fails);
    if (o) { char q[1200]; if (dm_custom_path("res_live_off", q, sizeof q)) { FILE *g = fopen(q, "w"); if (g) { fprintf(g, "%s\n", pr); fclose(g); } } live_file(0, 0); g_live_fails = 0; atomic_store(&g_live_off, 1);
        LOG("res live: 【红】连续两次切换中进程没了 → 这个安装关掉当场切换（res_live_off，产物 %s），选的尺寸重启后生效", pr); }
    else live_file(fails, 0); }
static int live_limit(int *w, int *h, int *gw, int *gh) {
    unsigned pw, ph; *gw = *gh = 0; if (!dm_game_win_init_size(gw, gh)) *gw = *gh = 0;
    int sw = atomic_load(&g_res_w0), sh = atomic_load(&g_res_h0); *w = sw < *gw ? sw : *gw; *h = sh < *gh ? sh : *gh;
    return live_env() && !res_pinned(&pw, &ph) && atomic_load(&g_res_latched) && !atomic_load(&g_live_failed) && !atomic_load(&g_live_off) && *w > 0 && *h > 0; }
int dm_res_live_limit(int *w, int *h) { int gw, gh; return live_limit(w, h, &gw, &gh); }
int dm_res_live_broken(void) { return atomic_load(&g_live_broken); }
static void handheld_of(int w, int h, int pct, int *hw, int *hh) { dm_res_handheld_x(w, h, res_plain_auto(pct), atomic_load(&g_base_w), atomic_load(&g_base_h), atomic_load(&g_full), hw, hh); }


int dm_res_chosen(int pct) {
    res_boot_write(0);
    int W = atomic_load(&g_base_w), H = atomic_load(&g_base_h), w = 0, h = 0, cw, ch, cpct, lw, lh, gw, gh;
    int full = atomic_load(&g_full);
    if (!atomic_load(&g_res_latched) || !dm_res_scale_size_x(dm_res_pct_clamp_x(res_eff(pct), W, H, full), W, H, full, &w, &h)) return 0;
    int can = live_limit(&lw, &lh, &gw, &gh); res_cur(&cw, &ch, &cpct);
    LOG("res live: want %dx%d, now %dx%d, start %dx%d, game limit %dx%d → %s", w, h, cw, ch, atomic_load(&g_res_w0), atomic_load(&g_res_h0), gw, gh,
        !can ? (live_env() ? "restart (in-place switching off)" : "restart (DM_RES_LIVE=0)") : dm_res_live_fits(w, h, atomic_load(&g_res_w0), atomic_load(&g_res_h0), gw, gh) ? "live" : "restart (larger than the limit)");
    if (!can || !dm_res_live_fits(w, h, atomic_load(&g_res_w0), atomic_load(&g_res_h0), gw, gh)) return 0;
    pthread_mutex_lock(&g_live_mu);
    if (g_live.phase == DM_RL_BACK) { pthread_mutex_unlock(&g_live_mu); LOG("res live: 正在退回上一个尺寸，这次重启后生效"); return 0; }
    if (w == cw && h == ch) { atomic_store(&g_res_cur, res_pack(w, h, pct)); pthread_mutex_unlock(&g_live_mu); LOG("res live: 已是 %dx%d", w, h); return 1; }
    int pw = cw, ph = ch, ppct = cpct; if (g_live.phase == DM_RL_FORWARD) { pw = g_live.pw; ph = g_live.ph; ppct = g_live_prev_pct; }
    int thw, thh, phw, phh; handheld_of(w, h, pct, &thw, &thh); handheld_of(pw, ph, ppct, &phw, &phh);
    dm_res_live_begin(&g_live, w, h, thw, thh, pw, ph, phw, phh); g_live_pct = pct; g_live_prev_pct = ppct; g_live_fold = 0; clock_gettime(CLOCK_MONOTONIC, &g_live_t0);
    live_file(g_live_fails, 1);
    atomic_store(&g_res_cur, res_pack(w, h, pct)); atomic_store(&g_live_busy, 1);
    pthread_mutex_unlock(&g_live_mu);
    if (getenv("DM_TEST_RES_LIVE_DROP")) LOG("res live: 测试开关 DM_TEST_RES_LIVE_DROP：31 不推");
    else dm_oe_res_resized();
    return 1;
}


void dm_present_done(int cw, int ch, int ew, int eh, int native, int geom) {
    if (!atomic_load(&g_live_busy)) return;
    pthread_mutex_lock(&g_live_mu);
    int docked = dm_opmode() == 1;
    if (g_live.phase == DM_RL_FORWARD && getenv("DM_TEST_RES_LIVE_EXTENT") && cw == (docked ? g_live.tw : g_live.thw) && ch == (docked ? g_live.th : g_live.thh)) ew += 8;

    if (atomic_load(&g_full)) { int tw = docked ? g_live.tw : g_live.thw, th = docked ? g_live.th : g_live.thh; if (cw == tw && ch != th && ch == dm_res_full_frontend_h(tw, th)) { if (ew == cw && eh == ch) eh = th; ch = th; } }
    int r = dm_res_live_step(&g_live, cw, ch, ew, eh, native, geom, docked); struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    double ms = (t.tv_sec - g_live_t0.tv_sec) * 1e3 + (t.tv_nsec - g_live_t0.tv_nsec) / 1e6;
    if (g_live_fold && (r == DM_RL_REACHED || r == DM_RL_REVERTED)) { LOG("res live: window shape change: %dx%d reached swapchain (crop %dx%d, extent %dx%d) %.0f ms after the change", g_live.tw, g_live.th, cw, ch, ew, eh, ms); g_live_fold = 0; atomic_store(&g_live_busy, 0); }
    else if (r == DM_RL_REACHED) { LOG("res live: %dx%d reached swapchain (crop %dx%d, extent %dx%d, geometry %d) %.0f ms after Apply", g_live.tw, g_live.th, cw, ch, ew, eh, geom, ms); g_live_fails = 0; live_file(0, 0); atomic_store(&g_live_busy, 0); }
    else if (r == DM_RL_REVERT) { atomic_store(&g_res_cur, res_pack(g_live.tw, g_live.th, g_live_prev_pct)); g_live_t0 = t;
        LOG("res live: 【红】%d 帧里没到 %dx%d（crop %dx%d, extent %dx%d, geometry %d）→ 退回 %dx%d，再推一次 31", DM_RES_LIVE_FRAMES, g_live.pw, g_live.ph, cw, ch, ew, eh, geom, g_live.tw, g_live.th);
        if (g_live_fold) dm_oe_res_changed(); else dm_oe_res_resized(); }
    else if (r == DM_RL_REVERTED) { atomic_store(&g_live_failed, 1); live_file(g_live_fails, 0); atomic_store(&g_live_busy, 0);
        LOG("res live: 【红】已退回 %dx%d（extent %dx%d）；这次运行不再当场切换，选的尺寸重启后生效", cw, ch, ew, eh); }
    else if (r == DM_RL_BROKEN) { g_live_fold = 0; atomic_store(&g_live_failed, 1); atomic_store(&g_live_broken, 1); live_file(g_live_fails, 0); atomic_store(&g_live_busy, 0);
        LOG("res live: 【红】退回 %dx%d 也没核上（crop %dx%d, extent %dx%d, geometry %d）：请重启 App", g_live.tw, g_live.th, cw, ch, ew, eh, geom); }
    pthread_mutex_unlock(&g_live_mu);
}

void front_android_res_live_stop(void) {
    if (!atomic_load(&g_live_busy)) return;
    pthread_mutex_lock(&g_live_mu); if (g_live.phase != DM_RL_IDLE) { live_file(g_live_fails, 0); LOG("res live: 切换核对中退到后台，不算切换中出事"); } pthread_mutex_unlock(&g_live_mu); }



static int dev_off_read(char *off, size_t n) { char p[1200]; off[0] = 0; if (!dm_custom_path("res_dev_off", p, sizeof p)) return 0; FILE *f = fopen(p, "r"); if (!f) return 0;
    if (!fgets(off, (int)n, f)) off[0] = 0; fclose(f); off[strcspn(off, "\r\n")] = 0; return 1; }
static int dev_off_write(void) {
    char p[1200], t[1210], pr[96] = ""; product(pr, sizeof pr); if (!dm_custom_path("res_dev_off", p, sizeof p)) return 0;
    snprintf(t, sizeof t, "%s.tmp", p); FILE *f = fopen(t, "w"); if (!f) return 0; int ok = fprintf(f, "%s\n", pr) > 0; ok = (fflush(f) == 0) & ok; ok = (fsync(fileno(f)) == 0) & ok; ok = (fclose(f) == 0) & ok;
    return ok && rename(t, p) == 0; }
static void dev_resolve(int W, int H, int full) {
    char soc[PROP_VALUE_MAX] = "", plat[PROP_VALUE_MAX] = "", pr[96] = "", off[96] = ""; const char *e = getenv("DM_DEV_DEFAULT"), *ts = getenv("DM_TEST_DEV_SOC");
    __system_property_get("ro.soc.model", soc); __system_property_get("ro.board.platform", plat);
    if (ts && *ts) { snprintf(soc, sizeof soc, "%s", ts); plat[0] = 0; }
    const dm_dev_profile_t *pf = dm_dev_profile_find(soc, plat);
    if (e && *e == '0') { LOG("res dev: 设备缺省关着（DM_DEV_DEFAULT=0），soc %s / %s", soc, plat); return; }
    if (!pf) { LOG("res dev: soc %s / %s%s 不在表里 → 缺省同以前（900 行、3D 85%%）", soc[0] ? soc : "?", plat[0] ? plat : "?", ts && *ts ? "（DM_TEST_DEV_SOC）" : ""); return; }
    int has = dev_off_read(off, sizeof off); product(pr, sizeof pr);
    if (dm_dev_off_applies(has, off, pr)) { LOG("res dev: 【红】设备缺省关着（res_dev_off，产物 %s：之前用设备缺省连续两次没启动起来）→ 900 行、3D 85%%", off[0] ? off : "?"); return; }
    int pct = dm_dev_default_pct_x(pf->budget_px, W, H, full), w = 0, h = 0; if (pct) dm_res_scale_size_x(pct, W, H, full, &w, &h);
    if (pct) { atomic_store(&g_dev_pct, pct); atomic_store(&g_dev_s3d, pf->s3d); }
    if (pct) LOG("res dev: soc %s / %s%s → %s（预算 %lld 像素%s）：屏 %dx%d 的设备缺省 %d%% = %dx%d，3D %d%%", soc[0] ? soc : "?", plat[0] ? plat : "?", ts && *ts ? "（DM_TEST_DEV_SOC）" : "",
        pf->gpu, (long long)pf->budget_px, pf->estimate ? "，估计" : "", W, H, pct, w, h, pf->s3d);
    else LOG("res dev: soc %s / %s%s → %s（预算 %lld 像素%s）：屏 %dx%d 上没有比 900 行高、又在预算里的格 → 缺省同以前（900 行、3D 85%%）", soc[0] ? soc : "?", plat[0] ? plat : "?",
        ts && *ts ? "（DM_TEST_DEV_SOC）" : "", pf->gpu, (long long)pf->budget_px, pf->estimate ? "，估计" : "", W, H); }


static int full_off_applies(void) {
    char p[1200], off[96] = "", pr[96] = ""; int has = 0;
    if (dm_custom_path("res_full_off", p, sizeof p)) { FILE *f = fopen(p, "r"); if (f) { has = 1; if (!fgets(off, sizeof off, f)) off[0] = 0; fclose(f); off[strcspn(off, "\r\n")] = 0; } }
    product(pr, sizeof pr); return dm_dev_off_applies(has, off, pr); }
static int full_off_write(void) {
    char p[1200], t[1210], pr[96] = ""; product(pr, sizeof pr); if (!dm_custom_path("res_full_off", p, sizeof p)) return 0;
    snprintf(t, sizeof t, "%s.tmp", p); FILE *f = fopen(t, "w"); if (!f) return 0; int ok = fprintf(f, "%s\n", pr) > 0; ok = (fflush(f) == 0) & ok; ok = (fsync(fileno(f)) == 0) & ok; ok = (fclose(f) == 0) & ok;
    return ok && rename(t, p) == 0; }

void xbg_set_ui_rect_fn(void (*fn)(double *y, double *h));
static void ui_rect_frac(double *fy, double *fh) { int x, y, w, h, rw = 0, rh = 0; dm_ui_rect(&x, &y, &w, &h); if (dm_screen_real(&rw, &rh) && rh > 0) { *fy = (double)y / rh; *fh = (double)h / rh; } }
static void enable_full(int on) {
    on = on != 0; if (atomic_exchange(&g_full, on) == on) return;
    if (on) { xbg_set_ui_rect_fn(ui_rect_frac); dm_uiscreen_enable(DM_UISCREEN_BOTTOM); } else dm_uiscreen_enable(DM_UISCREEN_OFF);
    LOG("画面：真全屏 %s（窗口 %dx%d）", on ? "开：局内按屏幕比例铺满、界面 16:9 贴底，前端页 16:9 居中" : "关：按画面比例出图", atomic_load(&g_last_w), atomic_load(&g_last_h)); }
void front_android_res_latch(void) {
    if (atomic_load(&g_res_latched)) return;
    int W = 0, H = 0, small = 0, w = 0, h = 0, want = dm_custom_get("res_pct"), pct = want, sw = atomic_load(&g_screen_w), sh = atomic_load(&g_screen_h), full = 0;
    if (sh > sw) { int t = sw; sw = sh; sh = t; }
    if (base_now(&W, &H, &small)) {
        if (dm_res_full_on(W, H)) {
            int env = full_env_off(), file = !env && full_off_applies(); full = !env && !file;
            if (!full) { atomic_store(&g_full_block, 1); LOG("boot: 【红】真全屏关着（%s）→ 16:9 留边", env ? "DM_ANDROID_FULL=0" : "res_full_off：这个产物的真全屏启动连续两次没起来"); } }
        dev_resolve(W, H, full); pct = dm_res_pct_clamp_x(want, W, H, full); dm_res_scale_size_x(dm_res_pct_clamp_x(res_eff(pct), W, H, full), W, H, full, &w, &h); }
    int n = res_boot_read(), step = dm_res_boot_step_x(h, n, full);
    if (step == DM_RB_FALLBACK && full) {
        int ok = full_off_write(); full = 0; atomic_store(&g_full_block, 1); atomic_store(&g_dev_pct, 0); atomic_store(&g_dev_s3d, 0);
        LOG("boot: 【红】真全屏连续 %d 次没启动起来（出图 %d × %d）→ 这个产物关掉真全屏（res_full_off%s），这次 16:9 留边", n, w, h, ok ? "" : " 写不进去：计数留着，下次照样退回");
        dev_resolve(W, H, 0); pct = dm_res_pct_clamp(want, W, H); dm_res_scale_size(dm_res_pct_clamp(res_eff(pct), W, H), W, H, &w, &h);
        step = ok ? dm_res_boot_step(h, 0) : DM_RB_NONE;
        if (ok) res_boot_write(step == DM_RB_COUNT ? 1 : 0); }
    else if (step == DM_RB_FALLBACK) {


        int dev = atomic_exchange(&g_dev_pct, 0); atomic_store(&g_dev_s3d, 0);
        LOG("boot: 【红】输出分辨率 %d × %d 连续 %d 次没启动起来，这次退回缺省 900 行（设置改回缺省%s，页上重新选可再试）", w, h, n, dev ? "，设备缺省关掉" : "");
        pct = DM_RES_AUTO; dm_res_scale_size(pct, W, H, &w, &h); atomic_store(&g_res_fb, 1);
        if (dev_off_write()) { res_boot_write(0); dm_custom_res_reset(); } else { LOG("boot: 【红】res_dev_off 写不进去：防崩计数留着（%d），下次启动照样退回", n); dm_custom_res_reset(); } }
    else if (step == DM_RB_COUNT) res_boot_write(n + 1);
    if (!w) { W = 1600; H = 900; w = 1600; h = 900; pct = DM_RES_AUTO; }
    if (!w) full = 0;
    atomic_store(&g_base_w, W); atomic_store(&g_base_h, H); atomic_store(&g_res_w0, w); atomic_store(&g_res_h0, h); atomic_store(&g_res_pct0, pct); atomic_store(&g_res_cur, res_pack(w, h, pct));
    atomic_store(&g_full_start, full); enable_full(full); atomic_store(&g_res_latched, 1);
    live_boot();
    char ps[24]; if (pct == DM_RES_AUTO && atomic_load(&g_dev_pct)) snprintf(ps, sizeof ps, "auto（设备 %d%%）", atomic_load(&g_dev_pct)); else if (pct == DM_RES_AUTO) snprintf(ps, sizeof ps, "auto"); else snprintf(ps, sizeof ps, "%d", pct);
    { char t[128]; int o = 0, n = atomic_load(&g_nmodes); t[0] = 0; for (int i = 0; i < n && o < (int)sizeof t - 8; i++) o += snprintf(t + o, sizeof t - o, "%s%d", i ? "/" : "", atomic_load(&g_modes[i]));
      LOG("display: modes %s Hz, refresh %.2f Hz, panel %dx%d（日志开之前交来的，这里补记）", n ? t : "?", atomic_load(&g_refresh_millihz) / 1000.0, atomic_load(&g_panel_w), atomic_load(&g_panel_h)); }
    LOG("boot: %s输出分辨率 res_pct=%s (screen %dx%d, panel %dx%d, base %dx%d) → %dx%d%s", small ? "【红】窗口比屏小，按窗口算 " : "", ps, sw, sh, atomic_load(&g_panel_w), atomic_load(&g_panel_h), W, H, w, h, full ? "，真全屏" : "");
}


void dm_present_ok(unsigned w, unsigned h) {
    static int n; unsigned pw, ph; int full = atomic_load(&g_full_start); if (n < 0 || !atomic_load(&g_res_latched) || (!full && atomic_load(&g_res_h0) <= 900) || !atomic_load(&g_window)) return;
    int dw = atomic_load(&g_res_w0), dh = atomic_load(&g_res_h0), hw, hh; handheld_of(dw, dh, atomic_load(&g_res_pct0), &hw, &hh);
    if (res_pinned(&pw, &ph)) { uint64_t r = front_game_res(1), q = front_game_res(0); dw = (int)(uint32_t)r; dh = (int)(r >> 32); hw = (int)(uint32_t)q; hh = (int)(q >> 32); }

    int fe = full && ((w == (unsigned)dw && h == (unsigned)dm_res_full_frontend_h(dw, dh)) || (w == (unsigned)hw && h == (unsigned)dm_res_full_frontend_h(hw, hh)));
    if ((w != (unsigned)dw || h != (unsigned)dh) && (w != (unsigned)hw || h != (unsigned)hh) && !fe) return;
    if (++n == 300) { n = -1; res_boot_write(0); LOG("输出分辨率：%u × %u 已出图 300 帧，防崩计数清零", w, h); }
}




static void fold_check(int w, int h) {
    unsigned pw_, ph_; if (!atomic_load(&g_res_latched) || res_pinned(&pw_, &ph_)) return;
    int W = w > h ? w : h, H = w > h ? h : w, was = atomic_load(&g_full), ok = dm_res_full_on(W, H) && !atomic_load(&g_full_block), waiting = atomic_load(&g_full_wait);
    if (ok && (full_env_off() || full_off_applies())) { atomic_store(&g_full_block, 1); ok = 0; }
    if (!was && !ok && !waiting) return;
    pthread_mutex_lock(&g_live_mu);
    int cw, ch, pct, tw, th, now, wait, fit, gw = 0, gh = 0, live = dm_uiscreen_res_live(), capw = atomic_load(&g_res_w0), caph = atomic_load(&g_res_h0);
    res_cur(&cw, &ch, &pct);
    if (live && dm_game_win_init_size(&gw, &gh)) { if (gw < capw) capw = gw; if (gh < caph) caph = gh; }

    if (!dm_res_fold_plan(W, H, res_eff(pct), ok, was, waiting, live, capw, caph, &tw, &th, &now, &wait, &fit)) { pthread_mutex_unlock(&g_live_mu); return; }
    atomic_store(&g_full_wait, wait);
    atomic_store(&g_base_w, W); atomic_store(&g_base_h, H);
    enable_full(now);
    if (!live) {
        atomic_store(&g_res_w0, tw); atomic_store(&g_res_h0, th); atomic_store(&g_res_cur, res_pack(tw, th, pct)); pthread_mutex_unlock(&g_live_mu);
        LOG("画面：窗口 %dx%d（游戏窗口还没建）→ 出图 %dx%d，真全屏 %s", w, h, tw, th, now ? "开" : "关"); return; }
    if (tw == cw && th == ch && was == now) { pthread_mutex_unlock(&g_live_mu); return; }
    int thw, thh; handheld_of(tw, th, pct, &thw, &thh); dm_res_fit_cap(&thw, &thh, tw, th);
    dm_res_live_begin(&g_live, tw, th, thw, thh, tw, th, thw, thh); g_live_pct = g_live_prev_pct = pct; g_live_fold = 1; clock_gettime(CLOCK_MONOTONIC, &g_live_t0);
    atomic_store(&g_res_cur, res_pack(tw, th, pct)); atomic_store(&g_live_busy, 1);
    pthread_mutex_unlock(&g_live_mu);
    LOG("画面：窗口 %dx%d → 出图 %dx%d → %dx%d%s，真全屏 %s，通知游戏重读（31，界面重载）", w, h, cw, ch, tw, th, fit ? "（缩进启动时的窗口贴图，重启后更清晰）" : "", now ? "开" : wait ? "等下次启动（启动尺寸放不下 1280 宽，先 16:9 留边）" : "关");
    dm_oe_res_changed();
}


int front_android_letterbox(void) {
    int w = atomic_load(&g_last_w), h = atomic_load(&g_last_h);
    return atomic_load(&g_full) || ((atomic_load(&g_full_block) || atomic_load(&g_full_wait)) && dm_res_full_on(w, h));
}

void front_android_view_rect(int ww, int wh, int out[8]) {
    out[0] = out[1] = out[4] = out[5] = 0; out[2] = out[6] = ww; out[3] = out[7] = wh;
    if (ww <= 0 || wh <= 0 || !front_android_letterbox()) return;
    int rw = 0, rh = 0, x = 0, y = 0, w = 0, h = 0, known = dm_screen_real(&rw, &rh) && rw > 0 && rh > 0;
    if (known && atomic_load(&g_full)) dm_ui_rect(&x, &y, &w, &h);
    dm_front_view(ww, wh, known, rw, rh, x, y, w, h, out);
}



__attribute__((weak)) int front_android_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int out_cap) {
    (void)title; (void)sub; (void)guide; (void)initial; (void)max_chars; (void)out; (void)out_cap; return -1;
}
int front_text_input(const char *title, const char *sub, const char *guide, const char *initial, int max_chars, char *out, int out_cap) {
    if (!front_metal_layer()) return -1;
    return front_android_text_input(title, sub, guide, initial, max_chars, out, out_cap);
}
