















#include "dm_custom.h"
#include "game_addrs.h"
#include "dm_widget.h"
#include "dm_tab_text.h"
#include "dm_res_scale.h"
#include "dm_cursor.h"
#include "hle.h"
#include "dm_input.h"
#include "dm_i18n.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>





#ifdef DM_MOVIE_FFMPEG
#define VOICE_ROW 1
#else
#define VOICE_ROW 0
#endif


static const char *const k_keys[] = { "lang",
#if VOICE_ROW
    "voice",
#endif
    "auto_mana", "merc_pot", "drop_light", "map_mark", "item_notes", "item_cmp", "icloud", "pot_pick", "drop_haptic", "gem_pick", "store_btn", "eco_fps", "q3d", "loot_filter", "auto_buff", "touch_swap_above", "cam", "bright", "dmg_num", "build_hints",
    "fps", "status", "res", "s3d", "res_pct", "status_fx", "shoulder_l", "shoulder_r", "combat_sim", "fight_log", "streak", "melee_splash", "shrine_stacking", "shrine_run_speed", "resume_pos" };
#define NK (int)(sizeof k_keys / sizeof *k_keys)
#define FPS_AUTO 1
static const int k_def[NK] = { 0,
#if VOICE_ROW
    1,
#endif
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2, 1,
    FPS_AUTO, 7, 0, 0, DM_RES_AUTO, 1, 6, 7, 1, 1, 2, 0, 1, 1, 1 };











#define K_VOICE (VOICE_ROW ? 1 : -2)
#define K_MERC (2 + VOICE_ROW)
#define K_ECO (12 + VOICE_ROW)
#define K_Q3D (13 + VOICE_ROW)
#define K_TOUCH_SWAP (16 + VOICE_ROW)




#define K_CAM (17 + VOICE_ROW)
#define CAM_N 3





#define K_BRIGHT (18 + VOICE_ROW)
#define BRIGHT_N 3





#define K_DMG (19 + VOICE_ROW)
#define DMG_N 3
































#define K_BUILD (20 + VOICE_ROW)
#define K_FPS (21 + VOICE_ROW)
#define K_STATUS (22 + VOICE_ROW)
#define K_RES (23 + VOICE_ROW)
#define K_S3D (24 + VOICE_ROW)
#define K_RESP (25 + VOICE_ROW)
#define RES_AUTO DM_RES_AUTO





#define K_STATUS_FX (26 + VOICE_ROW)












#define K_SHL (27 + VOICE_ROW)
#define K_SHR (28 + VOICE_ROW)











#define K_STREAK (31 + VOICE_ROW)
#define STREAK_N 3
#define K_SPLASH (32 + VOICE_ROW)
#define K_STACK (33 + VOICE_ROW)
#define K_SPEED (34 + VOICE_ROW)
#define K_RESUME (35 + VOICE_ROW)
#define RULE_N 2
_Static_assert(K_RESUME == NK - 1, "resume_pos is the last key");
#define SH_N 15
#define SH_RESERVED 13
#define ROW_TECH NK


#define ROW_S3Q (NK + 1)


#define ROW_DRIVER (NK + 2)
#define ROW_N (NK + 3)
#define S3Q_N 4
#define S3Q_OWN DM_RES_PRESET_N
#define STATUS_N 3
static const char *const k_status_box[STATUS_N] = { "TouchCustom_status_clock", "TouchCustom_status_battery", "TouchCustom_status_fps" };
void dm_cam_set_mode(int m);
void dm_3d_quality_mode_set(int mode);


void dm_3d_scale_menu_set(int pct);
int dm_3d_scale_default_pct(void);
int dm_3d_scale_pinned(void);
float dm_3d_scale_cfg(void);
float dm_3d_scale_cur(void);
int dm_3d_quality_mode_get(void);
double mtl_display_refresh(void);


























#define VCAP_FPS_ALL 1
#define VCAP_STATUS 2
#define VCAP_RES 4
#define VCAP_DRIVER 8


#define XCAP_TECH 1
#define XCAP_SHOULDER 2
#ifndef DM_CUSTOM_TEST_PLATFORM
__attribute__((weak)) int dm_extras_caps(void) { return 0; }
__attribute__((weak)) int dm_video_caps(void) { return 0; }
__attribute__((weak)) void dm_fps_apply(int fps) { (void)fps; }
__attribute__((weak)) int dm_res_screen(int *W, int *H) { *W = *H = 0; return 0; }
__attribute__((weak)) int dm_res_running(int *w, int *h) { *w = *h = 0; return 0; }
__attribute__((weak)) int dm_res_fallback(void) { return 0; }
__attribute__((weak)) int dm_res_pinned(void) { return 0; }
__attribute__((weak)) int dm_res_chosen(int pct) { (void)pct; return 0; }
__attribute__((weak)) int dm_res_live_limit(int *w, int *h) { *w = *h = 0; return 0; }
__attribute__((weak)) int dm_res_live_broken(void) { return 0; }
__attribute__((weak)) int dm_res_dev_pct(void) { return 0; }
__attribute__((weak)) int dm_res_full(void) { return 0; }
__attribute__((weak)) int dm_display_max_hz(void) { return 0; }
__attribute__((weak)) int dm_display_modes(int *hz, int cap) { (void)hz; (void)cap; return 0; }
__attribute__((weak)) int dm_fps_vote_policy(void) { return 0; }
__attribute__((weak)) int dm_driver_label(char *buf, int cap) { (void)buf; (void)cap; return 0; }
__attribute__((weak)) void dm_driver_open(void) { }
#else
int dm_extras_caps(void); int dm_video_caps(void); void dm_fps_apply(int fps); int dm_res_screen(int *W, int *H); int dm_res_running(int *w, int *h); int dm_res_fallback(void); int dm_res_pinned(void); int dm_res_chosen(int pct);
int dm_res_live_limit(int *w, int *h); int dm_res_live_broken(void); int dm_res_dev_pct(void); int dm_res_full(void);
int dm_display_max_hz(void); int dm_display_modes(int *hz, int cap); int dm_fps_vote_policy(void);
int dm_driver_label(char *buf, int cap); void dm_driver_open(void);
#endif
static _Atomic int g_val[NK];
static _Atomic int g_pend[NK];
static _Atomic int g_loaded;


static pthread_mutex_t g_mu; static pthread_once_t g_mu_once = PTHREAD_ONCE_INIT;
static void mu_init(void) { pthread_mutexattr_t a; pthread_mutexattr_init(&a); pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE); pthread_mutex_init(&g_mu, &a); pthread_mutexattr_destroy(&a); }
static void mu_lock(void) { pthread_once(&g_mu_once, mu_init); pthread_mutex_lock(&g_mu); }
static void mu_unlock(void) { pthread_mutex_unlock(&g_mu); }









#define LANG_N 7
static const char *const k_lang_hint[LANG_N] = { "TouchCustomLangHintZh", "TouchCustomLangHintEn", "TouchCustomLangHintEs", "TouchCustomLangHintRu", "TouchCustomLangHintKo", "TouchCustomLangHintDe", "TouchCustomLangHintPt" };
static const char *const k_lang_name[LANG_N] = { "zh", "en", "es", "ru", "ko", "de", "pt" };
static _Atomic int g_lang_explicit = -1;
static int g_lang_test;
static int lang_running(void) { int g = dm_lang_group(); return g == DM_LG_ZH ? 0 : g == DM_LG_ES || g == DM_LG_MX ? 2 : g == DM_LG_RU ? 3 : g == DM_LG_KO ? 4 : g == DM_LG_DE ? 5 : g == DM_LG_PT ? 6 : 1; }
static _Atomic int g_voice_explicit = -1;
static int g_voice_test;
#if VOICE_ROW
static int voice_running(void) { return lang_running() == 0 ? 0 : 1; }
#endif






static _Atomic int g_merc_explicit = -1;
static _Atomic int g_repaint;


static _Atomic int g_legacy_fps = -1, g_legacy_status = -1;
static _Atomic int g_legacy_shl = -1, g_legacy_shr = -1;
static _Atomic int g_has_triggers;
static _Atomic int g_tech_req;
static _Atomic int g_extras_live;
static _Atomic int g_video_live;
static _Atomic int g_sh_shown;
static _Atomic uint64_t g_absent;
_Static_assert(NK <= 64, "g_absent holds one bit per key");
static _Atomic int g_tech_shown;
static _Atomic int g_res_keep = -1;

const char *dm_save_commitdir(void);
static int key_index(const char *k) { if (k) for (int i = 0; i < NK; i++) if (!strcmp(k, k_keys[i])) return i; return -1; }
static char g_dir[1024];
void dm_custom_init(const char *dir) { if (dir) strlcpy(g_dir, dir, sizeof g_dir); }
static int dir_path(const char *name, char *p, size_t n) {
    if (g_dir[0]) { snprintf(p, n, "%s/%s", g_dir, name); return 1; }
    const char *c = dm_save_commitdir(); if (!c) return 0; snprintf(p, n, "%s/../%s", c, name); return 1; }
static int file_path(char *p, size_t n) { return dir_path("sw_custom.txt", p, n); }
int dm_custom_path(const char *name, char *p, size_t n) { return dir_path(name, p, n); }



static int fps_all(void) { return (dm_video_caps() & VCAP_FPS_ALL) != 0; }
static int fps_auto(void) {
    static _Atomic int a; int x = atomic_load(&a); if (x) return x;
    if (!fps_all()) return 60;
    double hz = mtl_display_refresh(); if (hz <= 0) return 60;
    x = hz > 130 ? 72 : 60; int z = 0; if (atomic_compare_exchange_strong(&a, &z, x)) LOG("自定义设置：帧率按屏幕 fps auto: max=%d current=%.2f -> %d", dm_display_max_hz(), hz, x);
    return x; }

static int fps_max_hz(void) { int m = dm_display_max_hz(); if (m > 0) return m; double r = mtl_display_refresh(); return r > 0 ? (int)(r + 0.5) : 60; }
static int fps_ok(int v) { return v == 0 || (v >= 30 && v <= DM_FPS_EXPLICIT_MAX && v % 5 == 0); }


static int fps_norm(int v) {
    if (v == FPS_AUTO || v <= 0 || !fps_all()) return v;
    int mx = dm_display_max_hz(); if (mx <= 0) return v;
    if (v >= mx) return 0;
    if (v % 5) v = (v + 2) / 5 * 5;
    if (v < 30) v = 30;
    return v >= mx ? 0 : v; }
static int res_base(int *W, int *H) { *W = *H = 0; return (dm_video_caps() & VCAP_RES) && dm_res_screen(W, H) && *W > 0 && *H > 0; }



static int res_pct_eff(int v, int W, int H) { int d = dm_res_dev_pct(); return dm_res_pct_clamp_x(v == RES_AUTO && d ? d : v, W, H, dm_res_full()); }
uint64_t dm_game_res(int docked);


static int res_out(int v, int *w, int *h) {
    int W, H; if (res_base(&W, &H) && !dm_res_pinned()) { dm_res_scale_size_x(res_pct_eff(v, W, H), W, H, dm_res_full(), w, h); return 1; }
    uint64_t g = dm_game_res(1); *w = (int)(uint32_t)g; *h = (int)(g >> 32); return 0; }
static int eff(int i, int v) {
    if (i == K_FPS) return v == FPS_AUTO ? fps_auto() : v;
    if (i == K_S3D) return v ? v : dm_3d_scale_default_pct();
    return v; }
static int raw_key(int i) { return i == K_FPS || i == K_RESP || i == K_S3D; }
static int changed(int i, int pd) {
    if (pd < 0 || i == K_RES || i == K_ECO) return 0;
    int v = atomic_load(&g_val[i]); return raw_key(i) ? pd != v : eff(i, pd) != eff(i, v); }

static int res_legacy_level(int pct) {
    int W, H, w, h; if (pct == RES_AUTO || !res_base(&W, &H) || !dm_res_scale_size_x(dm_res_pct_clamp_x(pct, W, H, dm_res_full()), W, H, dm_res_full(), &w, &h) || h <= 990) return 0;
    if (W < H) H = W;
    return H > 1080 && abs(h - 1080) <= abs(h - H) ? 1 : 2; }



static int parse_num(const char *s, int *v) { char *e; long x = strtol(s, &e, 10); if (e == s) return 0; while (*e == '\n' || *e == '\r' || *e == ' ') e++; if (*e || x < -1000 || x > 1000) return 0; *v = (int)x; return 1; }
static int parse_val(int i, const char *s, int *v) {
    int x;
    if (i == K_FPS) { if (!strncmp(s, "auto", 4) && (!s[4] || s[4] == '\n' || s[4] == '\r')) { *v = FPS_AUTO; return 1; } return parse_num(s, &x) && (fps_ok(x) || x == 72) && (*v = x, 1); }
    if (i == K_RESP) { if (!strncmp(s, "auto", 4) && (!s[4] || s[4] == '\n' || s[4] == '\r')) { *v = RES_AUTO; return 1; } return parse_num(s, &x) && x >= DM_RES_PCT_MIN && x <= 100 && x % 5 == 0 && (*v = x, 1); }
    if (i == K_STATUS) return parse_num(s, &x) && x >= 0 && x <= 7 && (*v = x, 1);
    if (i == K_RES) return parse_num(s, &x) && x >= 0 && x <= 2 && (*v = x, 1);
    if (i == K_S3D) return parse_num(s, &x) && (x == 0 || (x >= 60 && x <= 100 && x % 5 == 0)) && (*v = x, 1);
    if (i == 0) return parse_num(s, &x) && x >= 0 && x < LANG_N && (*v = x, 1);
    if (i == K_CAM) { *v = parse_num(s, &x) && x >= 0 && x < CAM_N ? x : 0; return 1; }
    if (i == K_BRIGHT) return parse_num(s, &x) && x >= 0 && x < BRIGHT_N && (*v = x, 1);
    if (i == K_SHL || i == K_SHR) return parse_num(s, &x) && x >= 0 && x < SH_N && (*v = x, 1);
    if (i == K_DMG) return s[0] >= '0' && s[0] < '0' + DMG_N && (s[1] == '\n' || !s[1]) && (*v = s[0] - '0', 1);
    if (i == K_STREAK) return parse_num(s, &x) && x >= 0 && x < STREAK_N && (*v = x, 1);
    if (i == K_SPLASH) return parse_num(s, &x) && x >= 0 && x <= 3 && (*v = x > 0, 1);
    if (i == K_STACK || i == K_SPEED || i == K_RESUME) return parse_num(s, &x) && x >= 0 && x < RULE_N && (*v = x, 1);
#if VOICE_ROW
    if (i == K_VOICE) return parse_num(s, &x) && (x == 0 || x == 1) && (*v = x, 1);
#endif
    *v = s[0] != '0'; return 1; }

static void save_locked(void);
static void apply_side_effects(int i, int v, int on_load);
static void load_locked(void) {
    if (atomic_load(&g_loaded)) return;
    char p[1100]; if (!file_path(p, sizeof p)) return;
    int v[NK], seen[NK] = { 0 }; for (int i = 0; i < NK; i++) v[i] = k_def[i];
    FILE *f = fopen(p, "r"); char ln[128];
    int merc = -1;
    while (f && fgets(ln, sizeof ln, f)) { char *eq = strchr(ln, '='); if (!eq) continue; *eq = 0; int i = key_index(ln), x;
        if (i >= 0 && parse_val(i, eq + 1, &x)) { v[i] = x; seen[i] = 1; }
        if (!strcmp(ln, "merc_pot_v2")) merc = eq[1] != '0'; }
    if (f) fclose(f);
    int lang = seen[0] ? v[0] : -1, voice = -1;
#if VOICE_ROW
    voice = seen[K_VOICE] ? v[K_VOICE] : -1;
#endif
    { const char *tl = getenv("DM_TEST_LANG"); if (tl && *tl >= '0' && *tl < '0' + LANG_N) { lang = v[0] = *tl - '0'; g_lang_test = 1; LOG("自定义设置：测试开关 DM_TEST_LANG=%d（当作语言行选了 %s 并应用，不写回文件）", lang, k_lang_name[lang]); } }
#if VOICE_ROW
    { const char *tv = getenv("DM_TEST_VOICE"); if (tv && *tv >= '0' && *tv <= '1') { voice = v[K_VOICE] = *tv - '0'; g_voice_test = 1; LOG("自定义设置：测试开关 DM_TEST_VOICE=%d（当作配音行选了 %s 并应用，不写回文件）", voice, voice ? "English" : "中文"); } }
#endif
    if (lang < 0) v[0] = lang_running();
#if VOICE_ROW
    if (voice < 0) v[K_VOICE] = voice_running();
    char vlog[32]; snprintf(vlog, sizeof vlog, " voice=%d%s", v[K_VOICE], voice < 0 ? "(auto)" : "");
#else
    const char *vlog = "";
#endif







    int moved = 0;
    if (!seen[K_FPS]) { int lf = atomic_load(&g_legacy_fps);
        if (v[K_ECO]) { v[K_FPS] = 30; moved = 1; LOG("自定义设置：迁移 帧率 ← 省电模式 eco_fps=1 → fps=30"); }
        else if (lf >= 0 && (fps_ok(lf) || lf == 72)) { v[K_FPS] = lf; moved = 1; LOG("自定义设置：迁移 帧率 ← App 菜单 fps_cap=%d", lf); }
        else v[K_FPS] = FPS_AUTO; }
    v[K_ECO] = 0;
    if (!seen[K_STATUS]) { int ls = atomic_load(&g_legacy_status); if (ls >= 0 && ls <= 7) { v[K_STATUS] = ls; moved = 1; LOG("自定义设置：迁移 状态行 ← App 偏好 %d（位 1 时钟 / 2 电量 / 4 帧率）", ls); } }





    { int f = fps_norm(v[K_FPS]);
      if (f != v[K_FPS]) { LOG("自定义设置：迁移 帧率 fps=%d → %s%d（屏最高 %d Hz）", v[K_FPS], f ? "" : "Max ", f, dm_display_max_hz()); v[K_FPS] = f; moved = 1; }
      else if (v[K_FPS] != FPS_AUTO && v[K_FPS] > 0 && fps_all() && dm_display_max_hz() <= 0) LOG("自定义设置：屏的刷新率还不知道，fps=%d 先不迁移", v[K_FPS]); }
    atomic_store(&g_res_keep, -1);
    if (!seen[K_RESP] && seen[K_RES]) { int W, H;
        if (!res_base(&W, &H)) { if (dm_video_caps() & VCAP_RES) { atomic_store(&g_res_keep, v[K_RES]); LOG("自定义设置：屏幕尺寸还不知道，旧档 res=%d 先不迁移（原样写回）", v[K_RES]); } }
        else { int lines = dm_res_legacy_lines(v[K_RES], H < W ? H : W), pct = lines <= dm_res_auto_lines_x(W, H, dm_res_full()) ? RES_AUTO : v[K_RES] == 2 ? 100 : dm_res_scale_nearest_lines_x(lines, W, H, dm_res_full());
            v[K_RESP] = pct; moved = 1;
            if (pct == RES_AUTO) LOG("自定义设置：迁移 输出分辨率 res=%d（2.2.8 在这块屏上跑 %d 行）→ res_pct=auto", v[K_RES], lines);
            else LOG("自定义设置：迁移 输出分辨率 res=%d（2.2.8 在这块屏上跑 %d 行）→ res_pct=%d", v[K_RES], lines, pct); } }


    { int lg[2] = { atomic_load(&g_legacy_shl), atomic_load(&g_legacy_shr) };
      for (int h = 0; h < 2; h++) { int i = h ? K_SHR : K_SHL;
          if (!seen[i] && lg[h] >= 0 && lg[h] < SH_N) { v[i] = lg[h]; moved = 1; LOG("自定义设置：迁移 %s ← App 偏好 %d", k_keys[i], lg[h]); } } }
    atomic_store(&g_lang_explicit, lang);
    atomic_store(&g_voice_explicit, voice);
    v[K_MERC] = merc < 0 ? k_def[K_MERC] : merc; atomic_store(&g_merc_explicit, merc);
    for (int i = 0; i < NK; i++) { atomic_store(&g_val[i], v[i]); atomic_store(&g_pend[i], -1); }
    atomic_store(&g_loaded, 1);
    LOG("自定义设置：读盘 %s%s lang=%d%s%s auto_mana=%d merc_pot=%d drop_light=%d map_mark=%d item_notes=%d item_cmp=%d icloud=%d pot_pick=%d drop_haptic=%d gem_pick=%d store_btn=%d fps=%d（生效 %d）status=%d res_pct=%d（%s）s3d=%d shoulder=%d/%d",
        p, f ? "" : "（没有文件，全按缺省）", v[0], lang < 0 ? "（没写，按当前语言）" : "", vlog, v[1 + VOICE_ROW], v[2 + VOICE_ROW], v[3 + VOICE_ROW], v[4 + VOICE_ROW], v[5 + VOICE_ROW], v[6 + VOICE_ROW], v[7 + VOICE_ROW], v[8 + VOICE_ROW], v[9 + VOICE_ROW], v[10 + VOICE_ROW], v[11 + VOICE_ROW],
        v[K_FPS], eff(K_FPS, v[K_FPS]), v[K_STATUS], v[K_RESP], v[K_RESP] == RES_AUTO ? "auto" : "%", v[K_S3D], v[K_SHL], v[K_SHR]);
    apply_side_effects(K_CAM, v[K_CAM], 1); apply_side_effects(K_Q3D, v[K_Q3D], 1); apply_side_effects(K_BRIGHT, v[K_BRIGHT], 1); apply_side_effects(K_DMG, v[K_DMG], 1);
    apply_side_effects(K_FPS, v[K_FPS], 1); apply_side_effects(K_S3D, v[K_S3D], 1);
    if (moved) save_locked();
}
static void load_once(void) { if (atomic_load(&g_loaded)) return; mu_lock(); load_locked(); mu_unlock(); }
int dm_custom_get(const char *key) {
    load_once(); int i = key_index(key); if (i < 0) return 1;
    return atomic_load(&g_loaded) ? atomic_load(&g_val[i]) : k_def[i];
}
int dm_custom_lang_explicit(void) { load_once(); return atomic_load(&g_loaded) ? atomic_load(&g_lang_explicit) : -1; }
int dm_custom_voice_explicit(void) { load_once(); return atomic_load(&g_loaded) ? atomic_load(&g_voice_explicit) : -1; }
int dm_fps_setting(void) { return eff(K_FPS, dm_custom_get("fps")); }
int dm_custom_status(void) { return dm_custom_get("status"); }
int dm_custom_splash_on(void) { return dm_custom_get("melee_splash") == 1; }
void dm_custom_legacy(int fps_cap, int status) { atomic_store(&g_legacy_fps, fps_cap); atomic_store(&g_legacy_status, status); }
void dm_custom_legacy_shoulder(int left, int right) { atomic_store(&g_legacy_shl, left); atomic_store(&g_legacy_shr, right); }
void dm_custom_has_triggers(int on) { if (atomic_exchange(&g_has_triggers, on != 0) != (on != 0)) { atomic_store(&g_repaint, 1); LOG("自定义设置：这台设备%s手机肩键（Extras 的肩键一节%s）", on ? "有" : "没有", on ? "显示" : "藏起"); } }
int dm_custom_tech_take(void) { return atomic_exchange(&g_tech_req, 0); }
static void save_locked(void) {
    char p[1100], t[1120]; if (!file_path(p, sizeof p)) { LOG("【红】自定义设置：存档目录没挂上，没存盘"); return; }
    snprintf(t, sizeof t, "%s.tmp", p); FILE *f = fopen(t, "w"); int ok = f != NULL;
    for (int i = 0; ok && i < NK; i++) { if (i == 0 && (atomic_load(&g_lang_explicit) < 0 || g_lang_test)) continue; if (i == K_VOICE && (atomic_load(&g_voice_explicit) < 0 || g_voice_test)) continue;
        if (i == K_RESP && atomic_load(&g_res_keep) >= 0) continue;
        int x = atomic_load(&g_val[i]); if (i == K_RES) x = atomic_load(&g_res_keep) >= 0 ? atomic_load(&g_res_keep) : res_legacy_level(atomic_load(&g_val[K_RESP]));
        ok = ((i == K_FPS && x == FPS_AUTO) || (i == K_RESP && x == RES_AUTO) ? fprintf(f, "%s=auto\n", k_keys[i]) : fprintf(f, "%s=%d\n", k_keys[i], x)) > 0; }
    if (ok && atomic_load(&g_merc_explicit) >= 0) ok = fprintf(f, "merc_pot_v2=%d\n", atomic_load(&g_merc_explicit)) > 0;
    if (f) ok = fclose(f) == 0 && ok;
    if (ok) ok = rename(t, p) == 0;
    LOG("自定义设置：存盘%s", ok ? "" : "【红】失败");
}


static void apply_side_effects(int i, int v, int on_load) {
    if (i == K_CAM) dm_cam_set_mode(v);
    if (i == K_Q3D) { dm_3d_quality_mode_set(v); LOG("自定义设置：3D 画质%s为%s（q3d=%d，已交给 3D 比例控制）", on_load ? "读盘" : " → ", v ? "画质优先" : "流畅优先", v); }
    if (i == K_FPS) { int e = eff(K_FPS, v); dm_fps_apply(e); LOG("自定义设置：帧率%s%d%s（0 = 不限）", on_load ? "读盘 " : " → ", e, v == FPS_AUTO ? "（按屏幕）" : ""); }
    if (i == K_S3D) { dm_3d_scale_menu_set(v); LOG("自定义设置：3D 清晰度%s%d%%%s", on_load ? "读盘 " : " → ", eff(K_S3D, v), v ? "" : "（平台缺省）"); }
    if (i == K_RESP && !on_load) { int W, H, w = 0, h = 0; if (res_base(&W, &H)) dm_res_scale_size_x(res_pct_eff(v, W, H), W, H, dm_res_full(), &w, &h);
        int live = dm_res_chosen(v); if (v == RES_AUTO) LOG("自定义设置：输出分辨率 → res_pct=auto (%dx%d, %s)", w, h, live ? "live" : "restart"); else LOG("自定义设置：输出分辨率 → res_pct=%d (%dx%d, %s)", v, w, h, live ? "live" : "restart"); }
    if (i == K_BRIGHT && on_load) LOG("自定义设置：暗处加亮读盘为 %d（bright）", v);
    if (i == K_DMG && on_load) LOG("自定义设置：伤害数字读盘为 %d（dmg_num）", v);
}


static int g_has_video;
static int g_view;
static int key_view(int i) { return i == K_FPS || i == K_STATUS || i == K_RESP || i == K_S3D || i == K_Q3D || i == K_BRIGHT; }
void dm_custom_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (!cat || !act) return;





    if (!strcmp(cat, "PanelManager") && !strcmp(act, "ClosePanel") && arg && !strncmp(arg, "TouchCustom", 11)) {
        if (!strcmp(arg, "TouchCustomTechOpen")) { if (atomic_load(&g_extras_live) && atomic_load(&g_tech_shown)) { atomic_store(&g_tech_req, 1); LOG("自定义设置：技术行点了「打开」"); } return; }
        if (!strcmp(arg, "TouchCustomDriverOpen")) { if (atomic_load(&g_video_live) && (dm_video_caps() & VCAP_DRIVER)) { dm_driver_open(); LOG("自定义设置：驱动行点了「打开」"); } return; }
        return; }
    if (strcmp(cat, "OptionsPanelMessage")) return;
    mu_lock(); load_locked();
    if (!strcmp(act, "AcceptChanges")) { int ch = 0;
        for (int i = 0; i < NK; i++) { int pd = atomic_exchange(&g_pend[i], -1); if (!changed(i, pd)) continue;
            atomic_store(&g_val[i], pd); ch = 1; apply_side_effects(i, pd, 0);
            if (i == 0) atomic_store(&g_lang_explicit, pd); if (i == K_VOICE) atomic_store(&g_voice_explicit, pd); if (i == K_MERC) atomic_store(&g_merc_explicit, pd);
            if (i == 0) LOG("自定义设置：语言 → %s（应用；下次启动生效）", k_lang_name[pd]); else if (i == K_VOICE) LOG("自定义设置：配音 → %s（应用；下次启动生效）", pd ? "English" : "中文");
            else if (i != K_Q3D && i != K_FPS && i != K_S3D && i != K_RESP) LOG("自定义设置：%s → %d（应用）", k_keys[i], pd); }
        if (ch) save_locked(); }
    else if (!strcmp(act, "CancelChanges")) { for (int i = 0; i < NK; i++) atomic_store(&g_pend[i], -1); atomic_store(&g_repaint, 1); }
    else if (!strcmp(act, "ResetToDefaults")) { for (int i = 1 + VOICE_ROW; i < NK; i++) if (i != K_ECO && i != K_RES && (!g_has_video || key_view(i) == g_view) && ((i != K_SHL && i != K_SHR) || atomic_load(&g_sh_shown)) && !(atomic_load(&g_absent) >> i & 1)) atomic_store(&g_pend[i], changed(i, k_def[i]) ? k_def[i] : -1); atomic_store(&g_repaint, 1); }
    mu_unlock();
}


void dm_custom_res_reset(void) { mu_lock(); load_locked(); atomic_store(&g_val[K_RESP], RES_AUTO); atomic_store(&g_pend[K_RESP], -1); atomic_store(&g_repaint, 1); if (atomic_load(&g_loaded)) save_locked(); mu_unlock(); }



static uint8_t *main_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }












static uint64_t find_w(uint64_t w, const char *name, int dep) {
    if (!w || dep > 14) return 0; const char *nm = (const char *)*(uint64_t *)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find_w(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static int vis(uint64_t w) { return w && *(uint8_t *)(w + 0x50); }






DM_WIDGET_RED_DEF
#define TOGGLED(w) (dm_widget_is(main_base(), (w), DM_VT_TOGGLE) && *(uint8_t *)((w) + 0xa88) != 0)
static void set_toggle(uint64_t w, int on) {
    if (!DM_WIDGET_OK(main_base(), w, DM_VT_TOGGLE, "自定义页")) return;
    static const int32_t fr[2][4] = { { 0, 1, 3, 2 }, { 4, 5, 6, -1 } };
    *(uint8_t *)(w + 0xa88) = on != 0; memcpy((void *)(w + 0x6e8), fr[on != 0], sizeof fr[0]); *(int32_t *)(w + 0x610) = fr[on != 0][0];
}




#ifndef DM_CUSTOM_SET_TEXT
static void txt_set(uint64_t w, const char *s) {
    uint8_t *b = main_base(); const char *nm = w ? (const char *)*(uint64_t *)(w + 8) : NULL;
    if (!w || !nm || strncmp(nm, "Touch", 5) || !DM_WIDGET_OK(b, w, DM_VT_TEXT, "自定义页文字")) return;
    size_t n = strlen(s); if (*(uint64_t *)(w + 0x90) == n && (!n || !memcmp(*(const char **)(w + 0x88), s, n))) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(b + GA_TEXT_SET))(w + 0x88, s, n); ((void (*)(uint64_t))(b + GA_TEXT_LAYOUT))(w);
}
#else
#define txt_set DM_CUSTOM_SET_TEXT
#endif











enum { R_TOGGLE, R_CAROUSEL, R_BITS, R_SLIDER, R_ACTION, R_PRESET };
typedef struct { int kind, n; const char *const *box; } vrow;
static const char *const k_tech_box[1] = { "TouchCustomTechOpen" };
static const char *const k_driver_box[1] = { "TouchCustomDriverOpen" };
static const char *row_key(int i) { return i == ROW_S3Q ? "s3d_preset" : i == ROW_TECH ? "tech" : i == ROW_DRIVER ? "driver" : k_keys[i]; }
static vrow row_of(int i) {
    vrow r = { R_TOGGLE, 1, NULL };
    if (i == ROW_TECH) return (vrow){ R_ACTION, 1, k_tech_box };
    if (i == ROW_DRIVER) return (vrow){ R_ACTION, 1, k_driver_box };
    if (i == ROW_S3Q) return (vrow){ R_PRESET, S3Q_N, NULL };
    if (i == K_SHL || i == K_SHR) r = (vrow){ R_CAROUSEL, SH_N, NULL };
    else if (i == 0) r = (vrow){ R_CAROUSEL, LANG_N, NULL };
#if VOICE_ROW
    else if (i == K_VOICE) r = (vrow){ R_CAROUSEL, 2, NULL };
#endif
    else if (i == K_TOUCH_SWAP) r = (vrow){ R_CAROUSEL, 2, NULL };
    else if (i == K_CAM) r = (vrow){ R_CAROUSEL, CAM_N, NULL };
    else if (i == K_DMG) r = (vrow){ R_CAROUSEL, DMG_N, NULL };
    else if (i == K_STREAK) r = (vrow){ R_CAROUSEL, STREAK_N, NULL };
    else if (i == K_SPLASH || i == K_STACK || i == K_SPEED || i == K_RESUME) r = (vrow){ R_CAROUSEL, RULE_N, NULL };
    else if (i == K_STATUS) r = (vrow){ R_BITS, STATUS_N, k_status_box };
    else if (i == K_FPS || i == K_RESP || i == K_S3D || i == K_Q3D || i == K_BRIGHT) r = (vrow){ R_SLIDER, 1, NULL };
    return r; }
static const char *box_name(int i, const vrow *r, int k, char *buf, size_t n) { if (r->box) return r->box[k]; (void)k; snprintf(buf, n, r->kind == R_SLIDER ? "TouchCustom_%s_Slider" : "TouchCustom_%s", row_key(i)); return buf; }

static int sh_avail(void) { return (dm_extras_caps() & XCAP_SHOULDER) && atomic_load(&g_has_triggers); }
static int tech_avail(void) { return (dm_extras_caps() & XCAP_TECH) != 0; }
static int row_ok(int i) {
    if (i == ROW_TECH) return tech_avail();
    if (i == ROW_DRIVER) return (dm_video_caps() & VCAP_DRIVER) != 0;
    if (i == ROW_S3Q) return !dm_3d_scale_pinned();
    if (i == K_SHL || i == K_SHR) return sh_avail();
    int caps = dm_video_caps(), W, H;
    if (i == K_STATUS) return (caps & VCAP_STATUS) != 0;
    if (i == K_RESP) return (caps & VCAP_RES) && !dm_res_pinned() && res_base(&W, &H) && dm_res_scale_lo_x(W, H, dm_res_full()) < 100;
    if (i == K_S3D) return !dm_3d_scale_pinned();
    return 1; }


static int view_rows(int view, int *out) {
    int n = 0;
    if (g_has_video && view) { static const int v[] = { K_FPS, K_STATUS, K_RESP, ROW_S3Q, K_S3D, K_Q3D, K_BRIGHT, ROW_DRIVER }; for (unsigned k = 0; k < sizeof v / sizeof *v; k++) out[n++] = v[k]; return n; }
    for (int i = 0; i < NK; i++) { if (i == K_ECO || i == K_FPS || i == K_STATUS || i == K_RES || i == K_S3D || i == K_RESP || i == K_SHL || i == K_SHR) continue; if (g_has_video && key_view(i)) continue; out[n++] = i; }
    out[n++] = ROW_TECH; out[n++] = K_SHL; out[n++] = K_SHR;
    return n; }



static void row_pump(uint64_t sp, int i, dm_wref *seen) {
    vrow r = row_of(i); uint64_t bx[8]; char nb[64]; if (r.kind != R_BITS || r.n > 8) return;
    for (int k = 0; k < r.n; k++) if (!(bx[k] = find_w(sp, box_name(i, &r, k, nb, sizeof nb), 0))) return;
    int pd0 = atomic_load(&g_pend[i]), cur = pd0 >= 0 ? pd0 : atomic_load(&g_val[i]), fresh = dm_wref_get(seen) != bx[0];
    if (fresh) dm_wref_set(seen, bx[0]);
    int to = cur;
    if (!fresh) for (int k = 0; k < r.n; k++) if (TOGGLED(bx[k]) != ((cur >> k) & 1)) to ^= 1 << k;
    if (to != cur) { atomic_store(&g_pend[i], to); LOG("自定义设置：%s → %d（点「应用」生效）", k_keys[i], to); }
    for (int k = 0; k < r.n; k++) if (TOGGLED(bx[k]) != ((to >> k) & 1)) set_toggle(bx[k], (to >> k) & 1);
}



























#define CAR_IDX(w) (*(int32_t *)(uintptr_t)((w) + 0x7ec))
#define CAR_CNT(w) (*(int32_t *)(uintptr_t)((w) + 0x7e8))
#define CAR_ON(w, k) (*(uint8_t *)(uintptr_t)((w) + 0x7d8 + (k)))
#ifndef DM_CUSTOM_CAROUSEL_SET
static void car_set(uint64_t w, int k) { ((void (*)(uint64_t, int))(main_base() + GA_CAROUSEL_SET))(w, k); }
static void car_step(uint64_t w, int fwd) { ((void (*)(uint64_t, int))(main_base() + GA_CAROUSEL_STEP))(w, fwd); }
static void car_refresh(uint64_t t) { ((void (*)(uint64_t))(main_base() + GA_TEXT_LAYOUT))(t); }
#else
#define car_set DM_CUSTOM_CAROUSEL_SET
#define car_step DM_CUSTOM_CAROUSEL_STEP
#define car_refresh DM_CUSTOM_CAROUSEL_REFRESH
#endif
static const int k_lang_order[LANG_N] = { 1, 5, 2, 4, 6, 3, 0 };

static int car_tab(int i, const int **ord, const char *const **hint) {
    static const char *const swap_h[2] = { "dmCustomTouchSwapNearHint", "dmCustomTouchSwapFarHint" };
    static const char *const cam_h[CAM_N] = { "dmCustomCamOffHint", "dmCustomCamOtsHint", "dmCustomCamFpHint" };
    static const char *const dmg_h[DMG_N] = { "dmCustomDmgNumOffHint", "dmCustomDmgNumSimpleHint", "dmCustomDmgNumColorHint" };
    static const char *const streak_h[STREAK_N] = { "dmCustomStreakOffHint", "dmCustomStreakShowHint", "dmCustomStreakBonusHint" };
    static const char *const splash_h[RULE_N] = { "dmCustomSplashOffHint", "dmCustomSplashOnHint" };
    static const char *const stack_h[RULE_N] = { "dmCustomShrineStackOffHint", "dmCustomShrineStackOnHint" };
    static const char *const run_h[RULE_N] = { "dmCustomShrineRunOffHint", "dmCustomShrineRunOnHint" };
    static const char *const resume_h[RULE_N] = { "dmCustomResumeOffHint", "dmCustomResumeOnHint" };
#if VOICE_ROW
    static const char *const voice_h[2] = { "dmCustomVoiceZhHint", "dmCustomVoiceEnHint" };
    if (i == K_VOICE) { *ord = NULL; *hint = voice_h; return 2; }
#endif
    *ord = NULL; *hint = NULL;
    if (i == 0) { *ord = k_lang_order; return LANG_N; }
    if (i == K_TOUCH_SWAP) { *hint = swap_h; return 2; }
    if (i == K_CAM) { *hint = cam_h; return CAM_N; }
    if (i == K_DMG) { *hint = dmg_h; return DMG_N; }
    if (i == K_STREAK) { *hint = streak_h; return STREAK_N; }
    if (i == K_SPLASH) { *hint = splash_h; return RULE_N; }
    if (i == K_STACK) { *hint = stack_h; return RULE_N; }
    if (i == K_SPEED) { *hint = run_h; return RULE_N; }
    if (i == K_RESUME) { *hint = resume_h; return RULE_N; }
    if (i == K_SHL || i == K_SHR) return SH_N;
    return 0; }
static int car_idx(int i, int v) { const int *o; const char *const *h; int n = car_tab(i, &o, &h); for (int k = 0; k < n; k++) if ((o ? o[k] : k) == v) return k; return -1; }
static int car_val(int i, int k) { const int *o; const char *const *h; int n = car_tab(i, &o, &h); return k >= 0 && k < n ? (o ? o[k] : k) : -1; }
static int car_cjk(int v) { return v == 0 || v == 4; }
static uint64_t car_find(uint64_t sp, int i) { char nb[48]; snprintf(nb, sizeof nb, "TouchCustom_%s", k_keys[i]); return find_w(sp, nb, 0); }
static int car_ok(int i, uint64_t w) {
    if (!w || !DM_WIDGET_OK(main_base(), w, DM_VT_CAROUSEL, "自定义页轮选")) return 0;
    if (CAR_CNT(w) == row_of(i).n) return 1;
    static int n; if (n++ < 5) LOG("【红】自定义设置：轮选 %s 有 %d 项，应为 %d（布局与宿主不一致），不动它", k_keys[i], CAR_CNT(w), row_of(i).n);
    return 0; }
static int g_car_last[NK];
static void car_font(uint64_t sp, uint64_t w, int cjk) {
    uint8_t *b = main_base(); uint64_t ref = find_w(sp, cjk ? "TouchCustom_lang_FontCjk" : "TouchCustom_lang_FontStd", 0), t = *(uint64_t *)(uintptr_t)(w + 0x7f8);
    if (!ref || !t || !DM_WIDGET_OK(b, ref, DM_VT_TEXT, "自定义页字体参照") || !DM_WIDGET_OK(b, t, DM_VT_TEXT, "自定义页轮选的字")) return;
    int32_t face = *(int32_t *)(uintptr_t)(ref + 0xb4), *cs = (int32_t *)(uintptr_t)(w + 0x800), *ts = (int32_t *)(uintptr_t)(t + 0xb4);
    if (*cs != face) *cs = face;
    if (*ts != face) { *ts = face; car_refresh(t); static int n; if (n++ < 8) LOG("自定义设置：语言行的字体 → %s（%d）", cjk ? "BlizzardGlobal" : "缺省", face); } }
static void car_text(uint64_t sp, int i, uint64_t w, int v) {
    const int *o; const char *const *h; car_tab(i, &o, &h); char nb[48];
    if (h && v >= 0 && v < row_of(i).n) { snprintf(nb, sizeof nb, "TouchCustom_%s_Hint", k_keys[i]); txt_set(find_w(sp, nb, 0), dm_str(main_base(), h[v])); }
    if (i == 0) car_font(sp, w, car_cjk(v)); }
static void car_pump(uint64_t sp, int i, dm_wref *seen) {
    uint64_t w = car_find(sp, i); if (!vis(w) || !car_ok(i, w)) return;
    int n = row_of(i).n, pd = atomic_load(&g_pend[i]), shown = eff(i, pd >= 0 ? pd : atomic_load(&g_val[i])), want = car_idx(i, shown);
    if (i == K_SHL || i == K_SHR) for (int k = 0; k < n; k++) { uint8_t on = k != SH_RESERVED || shown == SH_RESERVED; if (CAR_ON(w, k) != on) CAR_ON(w, k) = on; }
    if (dm_wref_get(seen) != w) { dm_wref_set(seen, w); if (want >= 0) car_set(w, want); g_car_last[i] = CAR_IDX(w); car_text(sp, i, w, shown); return; }
    int now = CAR_IDX(w);
    if (now != g_car_last[i] && now >= 0 && now < n) { int v = car_val(i, now); g_car_last[i] = now;
        if (v != shown) { atomic_store(&g_pend[i], v); shown = v; LOG("自定义设置：%s → %d（轮选第 %d 项；点「应用」生效）", k_keys[i], v, now); } }
    else if (want >= 0 && now != want) { car_set(w, want); g_car_last[i] = CAR_IDX(w); }
    car_text(sp, i, w, shown); }



















#define NOTCH_MAX 64
#define SL_MIN 0x5bc
#define SL_ON 0x5b4
#define SL_IND 0x580
#ifndef DM_CUSTOM_SLIDER_SET
static void slider_set(uint64_t w, int v) { ((void (*)(uint64_t, int, int, int))(main_base() + GA_SLIDER_SET_VALUE))(w, v, 0, 0); }
static void slider_enable(uint64_t w, int on) { ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + (on ? 0xc8 : 0xc0)))(w); }
#else
#define slider_set DM_CUSTOM_SLIDER_SET
#define slider_enable DM_CUSTOM_SLIDER_ENABLE
#endif
#ifndef DM_CUSTOM_NOW_MS
#include <time.h>
static uint64_t now_ms(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000 + (uint64_t)t.tv_nsec / 1000000; }
#else
#define now_ms DM_CUSTOM_NOW_MS
#endif

static int notches(int i, int *out) {
    int n = 0, W, H;
    if (i == K_FPS) return dm_fps_notches(fps_max_hz(), fps_all(), out, NOTCH_MAX);
    if (i == K_RESP) { if (!res_base(&W, &H)) return 0; for (int p = dm_res_scale_lo_x(W, H, dm_res_full()); p <= 100; p += 5) out[n++] = p; return n; }
    if (i == K_S3D) { for (int p = 60; p <= 100; p += 5) out[n++] = p; return n; }
    if (i == K_Q3D) { out[0] = 0; out[1] = 1; return 2; }
    if (i == K_BRIGHT) { for (int k = 0; k < BRIGHT_N; k++) out[k] = k; return BRIGHT_N; }
    return 0; }



static int notch_idx(int i, const int *nt, int n, int v) {
    if (n <= 0) return -1;
    int W, H, t = v, mx = fps_max_hz(), max_last = i == K_FPS && nt[n - 1] == 0;
    if (i == K_FPS) { t = eff(i, v); if (max_last && (t == 0 || t >= mx)) return n - 1; if (!t) t = 1 << 20; }
    else if (i == K_RESP) { if (!res_base(&W, &H)) return n - 1; t = v == RES_AUTO && !dm_res_dev_pct() ? dm_res_scale_nearest_lines_x(dm_res_auto_lines_x(W, H, dm_res_full()), W, H, dm_res_full()) : res_pct_eff(v, W, H); }
    else if (i == K_S3D) t = dm_3d_scale_pinned() ? (int)(dm_3d_scale_cfg() * 100 + 0.5f) : eff(i, v);
    int best = 0, bd = -1;
    for (int k = 0; k < n; k++) { int x = max_last && k == n - 1 ? mx : nt[k], d = x > t ? x - t : t - x; if (bd < 0 || d < bd) { bd = d; best = k; } }
    return best; }
static int shown_raw(int i) { int pd = atomic_load(&g_pend[i]); return pd >= 0 ? pd : atomic_load(&g_val[i]); }

static int g_sl_logv[64], g_sl_logf[64];
static void slider_pick(int i, int v, const char *how) {
    atomic_store(&g_pend[i], v == atomic_load(&g_val[i]) ? -1 : v);
    if (i < 64) { g_sl_logv[i] = v; g_sl_logf[i] = 1; }
    (void)how; }
static void slider_log(int i, int frame_tick) {
    if (i >= 64 || !g_sl_logf[i]) return;
    if (g_sl_logf[i]++ < 15 && !frame_tick) return;
    g_sl_logf[i] = 0; LOG("自定义设置：%s → %d（滑块；点「应用」生效）", k_keys[i], g_sl_logv[i]); }
static uint64_t sl_find(uint64_t sp, int i) { char nb[64]; snprintf(nb, sizeof nb, "TouchCustom_%s_Slider", k_keys[i]); return find_w(sp, nb, 0); }
static int sl_ok(uint64_t w) { return w && DM_WIDGET_OK(main_base(), w, DM_VT_SLIDER, "自定义页滑块") && *(uint64_t *)(w + SL_IND); }

static void slider_text(uint64_t sp, int i, int v) {
    uint8_t *b = main_base(); char t[160] = "", nb[64]; int W, H, w, h;
    if (i == K_FPS) { if (v == FPS_AUTO) snprintf(t, sizeof t, dm_str(b, "dmVideoFpsStd"), fps_auto());
        else if (!v) { if (fps_all()) snprintf(t, sizeof t, dm_str(b, "dmVideoFpsMaxHz"), fps_max_hz()); else snprintf(t, sizeof t, "%s", dm_str(b, "dmVideoFpsMax")); }
        else snprintf(t, sizeof t, dm_str(b, "dmVideoFpsValue"), v); }
    else if (i == K_RESP) { if (res_base(&W, &H) && dm_res_scale_size_x(res_pct_eff(v, W, H), W, H, dm_res_full(), &w, &h)) {
        if (v == RES_AUTO) snprintf(t, sizeof t, dm_str(b, "dmVideoResStd"), w, h); else snprintf(t, sizeof t, dm_str(b, "dmVideoResValue"), dm_res_pct_clamp_x(v, W, H, dm_res_full()), w, h); } }
    else if (i == K_S3D) { if (dm_3d_scale_pinned()) snprintf(t, sizeof t, dm_str(b, "dmVideoS3dValue"), (int)(dm_3d_scale_cfg() * 100 + 0.5f));
        else if (!v) snprintf(t, sizeof t, dm_str(b, "dmVideoS3dStd"), dm_3d_scale_default_pct()); else snprintf(t, sizeof t, dm_str(b, "dmVideoS3dValue"), v); }
    else if (i == K_Q3D || i == K_BRIGHT) { static const char *const nm[2][3] = { { "dmCustomQ3dSmooth", "dmCustomQ3dQuality", NULL }, { "dmVideoBrightOff", "dmVideoBrightMid", "dmVideoBrightFull" } };
        static const char *const hint[2][3] = { { "dmCustomQ3dSmoothHint", "dmCustomQ3dQualityHint", NULL }, { "dmCustomBrightOffHint", "dmCustomBrightMidHint", "dmCustomBrightFullHint" } };
        int r = i == K_BRIGHT, k = v < 0 ? 0 : v > (r ? 2 : 1) ? (r ? 2 : 1) : v;
        snprintf(t, sizeof t, "%s", dm_str(b, nm[r][k])); snprintf(nb, sizeof nb, "TouchVideo_%s_Hint", k_keys[i]); txt_set(find_w(sp, nb, 0), dm_str(b, hint[r][k])); }
    snprintf(nb, sizeof nb, "TouchCustom_%s_Value", k_keys[i]); txt_set(find_w(sp, nb, 0), t); }



static void slider_pump(uint64_t sp, int i, dm_wref *seen) {
    uint64_t w = sl_find(sp, i); if (!vis(w) || !sl_ok(w)) return;
    int nt[NOTCH_MAX], n = notches(i, nt); if (n <= 0) return;
    int on = row_ok(i) && n > 1, hi = n > 1 ? n - 1 : 1, shown = notch_idx(i, nt, n, shown_raw(i)), *f = (int32_t *)(w + SL_MIN);
    if (dm_wref_get(seen) != w || f[0] != 0 || f[1] != hi) { dm_wref_set(seen, w); f[0] = 0; f[1] = hi; f[2] = -1; slider_set(w, n > 1 ? shown : 1); }
    else if (on && f[2] != shown && f[2] >= 0 && f[2] < n) { slider_pick(i, nt[f[2]], "touch"); shown = notch_idx(i, nt, n, shown_raw(i)); }
    else if (!on && f[2] != (n > 1 ? shown : 1)) slider_set(w, n > 1 ? shown : 1);
    if ((*(uint8_t *)(w + SL_ON) != 0) != on) slider_enable(w, on);
    slider_log(i, 0);
    slider_text(sp, i, shown_raw(i)); }






static const char *const k_s3q_hint[S3Q_N] = { "dmVideoS3qSaveHint", "dmVideoS3qBalHint", "dmVideoS3qSharpHint", "dmVideoS3qOwnHint" };
static uint64_t s3q_find(uint64_t sp) { return find_w(sp, "TouchCustom_s3d_preset", 0); }
static int s3q_ok(uint64_t w) { return w && DM_WIDGET_OK(main_base(), w, DM_VT_CAROUSEL, "3D quick choice") && CAR_CNT(w) == S3Q_N; }
static int s3q_res_on(void) { return row_ok(K_RESP); }
static void s3q_pick(uint64_t sp, int i, int v) {
    slider_pick(i, v, "preset");
    uint64_t sl = sl_find(sp, i); int nt[NOTCH_MAX], n = notches(i, nt); if (sl_ok(sl) && n > 1) slider_set(sl, notch_idx(i, nt, n, shown_raw(i))); }
static int g_s3q_last;
static void s3q_pump(uint64_t sp, dm_wref *seen) {
    uint64_t w = s3q_find(sp); if (!vis(w) || !s3q_ok(w)) return;
    int pin = dm_3d_scale_pinned(), pin_pct = (int)(dm_3d_scale_cfg() * 100 + 0.5f), res_on = !pin && s3q_res_on();
    int want = dm_res_preset_of(shown_raw(K_RESP), pin ? pin_pct : shown_raw(K_S3D), res_on);
    for (int k = 0; k < S3Q_N; k++) { uint8_t on = pin ? k == want : k != S3Q_OWN || want == S3Q_OWN; if (CAR_ON(w, k) != on) CAR_ON(w, k) = on; }
    if (dm_wref_get(seen) != w) { dm_wref_set(seen, w); car_set(w, want); g_s3q_last = CAR_IDX(w); }
    else { int now = CAR_IDX(w);
        if (now != g_s3q_last && now >= 0 && now < S3Q_OWN && !pin && now != want) {
            const dm_res_preset_t *p = &dm_res_presets[now];
            g_s3q_last = now; want = now; s3q_pick(sp, K_S3D, p->s3d); if (res_on) s3q_pick(sp, K_RESP, p->res_pct);
            if (!res_on) LOG("自定义设置：3D 快选 → s3d=%d（点「应用」生效）", p->s3d);
            else if (p->res_pct == RES_AUTO) LOG("自定义设置：3D 快选 → s3d=%d res_pct=auto（点「应用」生效）", p->s3d);
            else LOG("自定义设置：3D 快选 → s3d=%d res_pct=%d（点「应用」生效）", p->s3d, p->res_pct); }
        else if (now != want) { car_set(w, want); g_s3q_last = CAR_IDX(w); }
        else g_s3q_last = now; }
    char t[160]; int ow = 0, oh = 0; res_out(shown_raw(K_RESP), &ow, &oh);
    if (pin) snprintf(t, sizeof t, dm_str(main_base(), "dmVideoS3dPinned"), pin_pct);
    else snprintf(t, sizeof t, dm_str(main_base(), k_s3q_hint[want]), ow, oh, eff(K_S3D, shown_raw(K_S3D)));
    txt_set(find_w(sp, "TouchVideo_s3d_preset_Hint", 0), t); }



static void row_step(uint64_t sp, int i, int dir, int *sub) {
    vrow r = row_of(i); char nb[64];
    if (r.kind == R_ACTION) { if (dir) return;
        if (i == ROW_DRIVER) { if (atomic_load(&g_video_live) && row_ok(i)) { dm_driver_open(); LOG("自定义设置：驱动行手柄 A「打开」"); } return; }
        if (atomic_load(&g_tech_shown)) { atomic_store(&g_tech_req, 1); LOG("自定义设置：技术行手柄 A「打开」"); } return; }
    if (r.kind == R_CAROUSEL) { uint64_t w = car_find(sp, i); if (car_ok(i, w)) car_step(w, dir >= 0); return; }
    if (r.kind == R_PRESET) { uint64_t w = s3q_find(sp); if (s3q_ok(w)) car_step(w, dir >= 0); return; }
    int pd0 = atomic_load(&g_pend[i]), cur = pd0 >= 0 ? pd0 : atomic_load(&g_val[i]);
    if (r.kind == R_TOGGLE) { if (dir) return; uint64_t w = find_w(sp, box_name(i, &r, 0, nb, sizeof nb), 0); if (w) set_toggle(w, !TOGGLED(w)); return; }
    if (r.kind == R_SLIDER) { if (!dir || !row_ok(i)) return; uint64_t w = sl_find(sp, i); int nt[NOTCH_MAX], n = notches(i, nt); if (!sl_ok(w) || n < 2) return;
        int to = notch_idx(i, nt, n, cur) + dir; if (to < 0 || to >= n) return;
        slider_pick(i, nt[to], "pad"); slider_set(w, to); return; }
    if (r.kind == R_BITS) { if (dir) { int s = *sub + dir; if (s >= 0 && s < r.n) *sub = s; return; }
        int to = cur ^ (1 << *sub); atomic_store(&g_pend[i], to); LOG("自定义设置：%s → %d（手柄 A；点「应用」生效）", k_keys[i], to);
        for (int k = 0; k < r.n; k++) { uint64_t z = find_w(sp, box_name(i, &r, k, nb, sizeof nb), 0); if (z) set_toggle(z, (to >> k) & 1); } }
}























uint64_t dm_guest_frame(void);
typedef struct { uint64_t tb; int n, sel, ext, vid; } opt_tabs;
static int tabs_read(uint64_t st, opt_tabs *t) {
    uint64_t tb = find_w(st, "OptionsTabs", 0); if (!tb || !dm_widget_is(main_base(), tb, DM_VT_TABBAR)) return 0;
    t->tb = tb; t->n = *(int32_t *)(tb + 0x558); t->sel = *(int32_t *)(tb + 0xdc4); t->ext = t->vid = -1; if (t->n < 1 || t->n > 16) return 0;
    for (int k = 0; k < t->n; k++) { uint64_t m = tb + 0xb50 + 32 * (uint64_t)k; const char *a = *(uint8_t *)(m + 0x18) ? *(const char **)(m + 0x10) : NULL;
        if (a && !strcmp(a, "TouchTabExtras")) t->ext = k; else if (a && !strcmp(a, "TouchTabVideo")) t->vid = k; }
    return t->ext >= 0 && t->vid >= 0;
}
static void tab_select(opt_tabs *t, int k) { ((void (*)(uint64_t, int, int))(main_base() + GA_TAB_SELECT))(t->tb, k, 0); }
static int video_route(uint64_t root, uint64_t sp, int spv, int *drop, int relay) {
    static int last_sel = -2, last_spv, last_modal, last_st, routed = -1, resync; static uint64_t restore_at, route_at;
    uint64_t st = dm_ui_top(root, "SettingsPanel"); int stv = vis(st), modal = stv && vis(find_w(root, "ConfirmationModal", 1)); opt_tabs t; int ok = stv && tabs_read(st, &t);
    if (ok) dm_tab_text_center(main_base(), t.tb, dm_lang_index());
    *drop = 0; if (ok) g_has_video = 1;
    if (g_has_video) { static const char *const other[] = { "SoundOptionsPanel", "GameplayOptionsPanel", "OnlineGameOptionsPanel", "KeyBindingMenu" }; int oth = 0;
        for (int k = 0; k < 4; k++) oth |= vis(dm_ui_top(root, other[k]));
        *drop = !stv || oth; }
    if (relay) { *drop = 0; last_spv = spv; last_modal = modal; last_st = stv; restore_at = route_at = 0; routed = -1; if (ok) { last_sel = t.sel; resync = 1; } return 0; }
    if (!ok) { last_sel = -2; last_spv = spv; last_modal = modal; last_st = stv; restore_at = route_at = 0; routed = -1; resync = 0; return 0; }
    uint64_t f = dm_guest_frame(); int ours = t.sel == t.ext || t.sel == t.vid;
    if (resync) { resync = 0; int k = g_view ? t.vid : t.ext;
        if (spv && !modal && t.sel != k) { tab_select(&t, k);
            LOG("自定义设置：切换布局后页签条落在第 %d 个 → 按视图选回%s（第 %d 个，现为 %d）", t.sel, g_view ? "Video" : "Extras", k, *(int32_t *)(t.tb + 0xdc4)); last_sel = *(int32_t *)(t.tb + 0xdc4); return 1; }
        last_sel = t.sel; }
    if (!last_st) LOG("自定义设置：选项打开，%d 个页签，选中 %d（Extras %d、Video %d），视频页%s", t.n, t.sel, t.ext, t.vid, spv ? "在屏上" : "不在");
    if (t.sel != last_sel) {
        if (ours) { if (g_view != (t.sel == t.vid)) LOG("自定义设置：视图 → %s", t.sel == t.vid ? "Video" : "Extras"); g_view = t.sel == t.vid; }
        last_sel = t.sel; }




    if (ours && !spv && !modal) { if (!route_at) route_at = f + 3;
        else if (f >= route_at && routed != t.sel) { route_at = 0; routed = t.sel; LOG("自定义设置：选中%s、视频页不在屏上：替页签发 CheckChanges:VideoOptionsPanel", g_view ? "Video" : "Extras"); dm_ui_send_now("SettingsPanelMessage", "CheckChanges", "VideoOptionsPanel"); return 1; } }
    else { route_at = 0; if (spv || !ours) routed = -1; }
    if ((spv && !last_spv) || (!modal && last_modal) || (stv && !last_st)) restore_at = f + 6;
    last_spv = spv; last_modal = modal; last_st = stv;
    if (restore_at && f >= restore_at) { restore_at = 0;
        if (spv && !ours && !modal) { int k = g_view ? t.vid : t.ext; tab_select(&t, k);
            LOG("自定义设置：视频页在屏上、选中的却是第 %d 个页签 → 选回%s（第 %d 个，现为 %d）", t.sel, g_view ? "Video" : "Extras", k, *(int32_t *)(t.tb + 0xdc4)); last_sel = *(int32_t *)(t.tb + 0xdc4); return 1; } }
    return 0;
}





static int relayout_ok(uint64_t w) { if (!dm_widget_is(main_base(), w, DM_VT_SLIDER)) return 1; static int n; if (n++ < 5) LOG("【红】自定义设置：不对滑块 %s 调虚表 +0x10", (const char *)*(uint64_t *)(w + 8)); return 0; }
static void view_geom(uint64_t w, int on, int h) {
    const char *nm = w ? (const char *)*(uint64_t *)(w + 8) : NULL; if (!nm || strncmp(nm, "Touch", 5) || !relayout_ok(w)) return;
    dm_ui_show_ours(w, on); int32_t *r = (int32_t *)(w + 0x70), x = on ? 0 : 99999, hh = on ? h : 1;
    if (r[0] != x || r[3] != hh) { r[0] = x; r[3] = hh; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); }
}







static const char *const k_tech_names[] = { "TouchCustomTech_Title", "TouchCustomTech_Divider", "TouchCustom_tech_Text", "TouchCustom_tech_Label", "TouchCustom_tech_State", "TouchCustomTechOpen" };
static const char *const k_sh_names[] = { "TouchCustomShoulder_Title", "TouchCustomShoulder_Divider", "TouchCustomShoulder_Hint",
    "TouchCustom_shoulder_l_Text", "TouchCustom_shoulder_l_Label", "TouchCustom_shoulder_l",
    "TouchCustom_shoulder_r_Text", "TouchCustom_shoulder_r_Label", "TouchCustom_shoulder_r" };
static void show_named(uint64_t sp, const char *name, int on);
static void extras_sections(uint64_t sp) {
    int tech = tech_avail(), sh = sh_avail();
    for (unsigned k = 0; k < sizeof k_tech_names / sizeof *k_tech_names; k++) show_named(sp, k_tech_names[k], tech);
    for (unsigned k = 0; k < sizeof k_sh_names / sizeof *k_sh_names; k++) show_named(sp, k_sh_names[k], sh);
    atomic_store(&g_tech_shown, tech && find_w(sp, "TouchCustomTechOpen", 0) != 0);
    atomic_store(&g_sh_shown, sh && find_w(sp, "TouchCustom_shoulder_l", 0) != 0);
}
static int extras_h(uint64_t sp, int h) {
    const char *cut = !tech_avail() ? "TouchCustomTech_Title" : !sh_avail() ? "TouchCustomShoulder_Title" : NULL;
    uint64_t w = cut ? find_w(sp, cut, 0) : 0; if (!w) return h;
    int y = ((int32_t *)(w + 0x70))[1]; return y > 0 && y < h ? y : h; }


#ifndef DM_CUSTOM_TEST_PLATFORM
__attribute__((weak)) const char *dm_data_source(void) { return NULL; }
#else
const char *dm_data_source(void);
#endif
static void data_label(char *out, size_t n) {
    const char *s = dm_data_source(); uint8_t *b = main_base();
    if (!s || !*s) { snprintf(out, n, "–"); return; }
    if (strstr(s, "/Download/")) { snprintf(out, n, "%s", dm_str(b, "dmCustomTechDownload")); return; }
    size_t l = strlen(s); while (l > 1 && s[l - 1] == '/') l--;
    size_t a = l; while (a > 0 && s[a - 1] != '/') a--;
    snprintf(out, n, "%.*s", (int)(l - a), s + a); }
static void action_pump(uint64_t sp, dm_wref *seen) {
    uint64_t w = find_w(sp, k_tech_box[0], 0); if (!vis(w)) return;
    if (dm_wref_get(seen) != w) dm_wref_set(seen, w);
    char d[96], t[192]; data_label(d, sizeof d); snprintf(t, sizeof t, dm_str(main_base(), "dmCustomTechState"), d); txt_set(find_w(sp, "TouchCustom_tech_State", 0), t); }
static void driver_pump(uint64_t sp, dm_wref *seen) {
    uint64_t w = find_w(sp, k_driver_box[0], 0); if (!vis(w)) return;
    if (dm_wref_get(seen) != w) dm_wref_set(seen, w);
    char lb[128]; if (!dm_driver_label(lb, sizeof lb) || !lb[0]) snprintf(lb, sizeof lb, "%s", dm_str(main_base(), "dmVideoDriverSystem"));
    txt_set(find_w(sp, "TouchCustom_driver_Value", 0), lb); }
static void video_view_pump(uint64_t sp) {
    uint64_t cr = find_w(sp, "TouchCustomRows", 0), vr = find_w(sp, "TouchVideoRows", 0); if (!cr || !vr) return;
    static dm_wref rc, rv; static int hc, hv, shown = -1, top_left;
    if (dm_wref_get(&rc) != cr) { dm_wref_set(&rc, cr); hc = ((int32_t *)(cr + 0x70))[3]; shown = -1; }
    if (dm_wref_get(&rv) != vr) { dm_wref_set(&rv, vr); hv = ((int32_t *)(vr + 0x70))[3]; shown = -1; }
    uint64_t view = *(uint64_t *)(cr + 0x30), sv = view ? *(uint64_t *)(view + 0x30) : 0; uint64_t f = dm_guest_frame();
    view_geom(cr, !g_view, extras_h(sp, hc)); view_geom(vr, g_view, hv);
    if (shown != g_view) { shown = g_view; top_left = 12; }
    if (top_left > 0 && view && sv && f % 3 == 0) { top_left--; int off = -*(int32_t *)(view + 0x74); void dm_ui_wheel_on(uint64_t, int); if (off > 0) dm_ui_wheel_on(sv, 40); else top_left = 0; }
}
















static void own_y(uint64_t w, int y) {
    const char *nm = w ? (const char *)*(uint64_t *)(w + 8) : NULL; if (!nm || strncmp(nm, "Touch", 5) || !relayout_ok(w)) return;
    int32_t *r = (int32_t *)(w + 0x70); if (r[1] != y) { r[1] = y; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); } }
static void show_named(uint64_t sp, const char *name, int on) { uint64_t w = find_w(sp, name, 0); if (w) dm_ui_show_ours(w, on); }
static void show_row(uint64_t sp, int i, int on) {
    vrow r = row_of(i); char nb[80]; static const char *const sfx[] = { "Label", "Hint", "Note", "Warn" };
    for (int k = 0; k < r.n; k++) { const char *bx = box_name(i, &r, k, nb, sizeof nb); char lb[96]; show_named(sp, bx, on); snprintf(lb, sizeof lb, "%s_Label", bx); show_named(sp, lb, on); }
    for (unsigned k = 0; k < sizeof sfx / sizeof *sfx; k++) { snprintf(nb, sizeof nb, "TouchVideo_%s_%s", k_keys[i], sfx[k]); show_named(sp, nb, on); }
    snprintf(nb, sizeof nb, "TouchCustom_%s_Value", k_keys[i]); show_named(sp, nb, on); }
static int res_live_fits(int w, int h) { int lw, lh; return dm_res_live_limit(&lw, &lh) && w <= lw && h <= lh; }
static void video_text_pump(uint64_t sp) {
    uint8_t *b = main_base(); char t[192]; int caps = dm_video_caps(), W, H;
    if (!find_w(sp, "TouchVideoRows", 0)) return;
    show_row(sp, K_STATUS, (caps & VCAP_STATUS) != 0);

    { static const char *const drv[] = { "TouchCustom_driver_Text", "TouchVideo_driver_Label", "TouchCustom_driver_Value", "TouchVideo_driver_Hint", "TouchCustomDriverOpen" };
      for (unsigned k = 0; k < sizeof drv / sizeof *drv; k++) show_named(sp, drv[k], (caps & VCAP_DRIVER) != 0); }

    { char ev[160] = "", un[160] = ""; int e = eff(K_FPS, shown_raw(K_FPS)), hz = 0, even = 1;
      if (fps_all()) {
          if (atomic_load(&g_pend[K_FPS]) < 0) { hz = (int)(mtl_display_refresh() + 0.5); even = !e || dm_fps_divides(hz, e); }
          else { int m[16], nm = dm_display_modes(m, 16); if (nm) hz = dm_fps_vote(e, m, nm, dm_fps_vote_policy(), &even); }
          if (hz > 0) { if (even) snprintf(ev, sizeof ev, dm_str(b, "dmVideoFpsEven"), hz); else snprintf(un, sizeof un, dm_str(b, "dmVideoFpsUneven"), hz, e); } }
      txt_set(find_w(sp, "TouchVideo_fps_Note", 0), ev); txt_set(find_w(sp, "TouchVideo_fps_Warn", 0), un); }

    if (!res_base(&W, &H)) show_row(sp, K_RESP, 0);
    else { int v = shown_raw(K_RESP), w = 0, h = 0, rw = 0, rh = 0, run = dm_res_running(&rw, &rh);
        show_row(sp, K_RESP, 1); dm_res_scale_size_x(res_pct_eff(v, W, H), W, H, dm_res_full(), &w, &h);
        if (dm_res_pinned() && run) snprintf(t, sizeof t, dm_str(b, "dmVideoResPinned"), rw, rh);
        else if (dm_res_fallback() && v == RES_AUTO) snprintf(t, sizeof t, "%s", dm_str(b, "dmVideoResFallback"));
        else if (dm_res_live_broken()) snprintf(t, sizeof t, "%s", dm_str(b, "dmVideoResBroken"));
        else if (run && (w != rw || h != rh)) { if (atomic_load(&g_pend[K_RESP]) >= 0 && res_live_fits(w, h)) snprintf(t, sizeof t, "%s", dm_str(b, "dmVideoResLive"));
            else snprintf(t, sizeof t, dm_str(b, "dmVideoResRestart"), rw, rh, w, h); }
        else t[0] = 0;
        txt_set(find_w(sp, "TouchVideo_res_pct_Note", 0), t); }

    { int pin = dm_3d_scale_pinned(), pd = atomic_load(&g_pend[K_S3D]), sv = eff(K_S3D, shown_raw(K_S3D)), cfg = (int)(dm_3d_scale_cfg() * 100 + 0.5f);
      int pct = pin ? cfg : sv, ow = 0, oh = 0, bw = 0, bh = 0, rw = 0, rh = 0, later = 0;
      if (res_out(shown_raw(K_RESP), &ow, &oh)) { res_out(RES_AUTO, &bw, &bh);
          later = dm_res_running(&rw, &rh) && (rw != ow || rh != oh) && !res_live_fits(ow, oh); }
      else { bw = ow; bh = oh; }
      char live[160] = "", warn[160] = "", note[160] = "";
      if (ow > 0 && oh > 0 && bw > 0 && bh > 0) { int w3 = (int)(ow * pct / 100.0 + 0.5), h3 = (int)(oh * pct / 100.0 + 0.5);
          double dp = dm_3d_scale_default_pct() / 100.0, base = bw * dp * bh * dp; int r100 = (int)((double)w3 * h3 / base * 100 + 0.5);
          snprintf(live, sizeof live, dm_str(b, later ? "dmVideoS3dLiveNext" : "dmVideoS3dLive"), w3, h3, r100 / 100, r100 % 100);
          if (r100 >= 180) snprintf(warn, sizeof warn, "%s", dm_str(b, "dmVideoS3dWarn")); }
      if (pin) snprintf(note, sizeof note, dm_str(b, "dmVideoS3dPinned"), cfg);
      else { int cur = (int)(dm_3d_scale_cur() * 100 + 0.5f), q = shown_raw(K_Q3D);
          if (!q && pd < 0 && dm_3d_quality_mode_get() == 0 && cur > 0 && cur < cfg) snprintf(note, sizeof note, dm_str(b, "dmVideoS3dAuto"), cur); }
      uint64_t hw = find_w(sp, "TouchVideo_s3d_Hint", 0), ww = find_w(sp, "TouchVideo_s3d_Warn", 0), nw = find_w(sp, "TouchVideo_s3d_Note", 0);
      txt_set(hw, live); txt_set(ww, warn); txt_set(nw, note);

      if (hw && ww && nw) { int32_t hy = ((int32_t *)(hw + 0x70))[1], dy = ((int32_t *)(nw + 0x70))[1] - hy; own_y(ww, hy + (note[0] ? 2 : 1) * dy); } }
}


static void toggle_pump(uint64_t sp, int i, dm_wref *seen) {
    char nm[48]; snprintf(nm, sizeof nm, "TouchCustom_%s", k_keys[i]); uint64_t w = find_w(sp, nm, 0); if (!vis(w)) return;
    int pd = atomic_load(&g_pend[i]), shown = pd >= 0 ? pd : atomic_load(&g_val[i]);
    if (dm_wref_get(seen) != w) { dm_wref_set(seen, w); set_toggle(w, shown); return; }
    int on = TOGGLED(w); if (on != shown) { atomic_store(&g_pend[i], on); LOG("自定义设置：%s 勾选框 → %s（点「应用」生效）", k_keys[i], on ? "勾上" : "取消"); }
}


static void rect_add(uint64_t w, int *l, int *t, int *r, int *b, int *any) {
    if (!vis(w)) return; int32_t *q = (int32_t *)(w + 0x70);
    if (!*any || q[0] < *l) *l = q[0]; if (!*any || q[1] < *t) *t = q[1]; if (!*any || q[0] + q[2] > *r) *r = q[0] + q[2]; if (!*any || q[1] + q[3] > *b) *b = q[1] + q[3]; *any = 1;
}
static int row_rect(uint64_t sp, int i, int sub, int *l, int *t, int *r, int *b) {
    vrow rw = row_of(i); char nb[80]; int any = 0;
    if (rw.kind == R_ACTION) { snprintf(nb, sizeof nb, "TouchCustom_%s_Text", row_key(i)); rect_add(find_w(sp, nb, 0), l, t, r, b, &any); rect_add(find_w(sp, rw.box[0], 0), l, t, r, b, &any); return any; }
    if (rw.kind == R_BITS) { const char *bx = box_name(i, &rw, sub, nb, sizeof nb); rect_add(find_w(sp, bx, 0), l, t, r, b, &any); snprintf(nb, sizeof nb, "%s_Label", bx); rect_add(find_w(sp, nb, 0), l, t, r, b, &any); return any; }
    snprintf(nb, sizeof nb, "TouchCustom_%s_Text", row_key(i)); rect_add(find_w(sp, nb, 0), l, t, r, b, &any);
    snprintf(nb, sizeof nb, "TouchVideo_%s_Label", row_key(i)); rect_add(find_w(sp, nb, 0), l, t, r, b, &any);
    for (int k = 0; k < rw.n; k++) rect_add(find_w(sp, box_name(i, &rw, k, nb, sizeof nb), 0), l, t, r, b, &any);
    return any;
}


static int row_here(uint64_t sp, int i) { vrow r = row_of(i); char nb[64]; return row_ok(i) && vis(find_w(sp, box_name(i, &r, 0, nb, sizeof nb), 0)); }
void dm_custom_ui_pump(unsigned long long root) {
    if (!root || !main_base()) return;
    static dm_wref seen[ROW_N];
    int dm_padmode_busy(void); int relay = dm_padmode_busy() || dm_ui_reloading();
    if (relay) { static uint64_t lg_at; uint64_t f = dm_guest_frame(); int was = 0; for (int i = 0; i < NK; i++) was |= seen[i].w != 0;
        if (was && f - lg_at > 60) { lg_at = f; LOG("自定义设置：手柄 / 触屏切换重建界面——这一页按刚出现重摆（待定改动保留，不当玩家点的）"); }
        memset(seen, 0, sizeof seen); }
    uint64_t sp = dm_ui_top(root, "VideoOptionsPanel"); int drop = 0;
    mu_lock(); atomic_store(&g_extras_live, 0); atomic_store(&g_video_live, 0);
    if (video_route(root, sp, vis(sp), &drop, relay)) { mu_unlock(); return; }
    if (drop) { int any = 0; for (int i = 0; i < NK; i++) any |= atomic_exchange(&g_pend[i], -1) >= 0; if (any) { atomic_store(&g_repaint, 1); LOG("自定义设置：选项会话结束（选项关了 / 别的子页上了屏）：丢掉没应用的改动"); } }
    if (!vis(sp)) {
        int was = 0; for (int i = 0; i < NK; i++) was |= seen[i].w != 0;
        if (was && !g_has_video) for (int i = 0; i < NK; i++) atomic_store(&g_pend[i], -1);
        memset(seen, 0, sizeof seen); mu_unlock(); return; }
    load_locked(); if (atomic_exchange(&g_repaint, 0)) memset(seen, 0, sizeof seen);
    extras_sections(sp);
    if (g_has_video) video_view_pump(sp);
    atomic_store(&g_extras_live, !relay && (!g_has_video || !g_view));
    atomic_store(&g_video_live, !relay && g_has_video && g_view);
    if (atomic_load(&g_lang_explicit) < 0) atomic_store(&g_val[0], lang_running());
#if VOICE_ROW
    if (atomic_load(&g_voice_explicit) < 0) atomic_store(&g_val[K_VOICE], voice_running());
#endif
    int rows[ROW_N], nr = view_rows(g_has_video ? g_view : 0, rows);





    { int dm_padmode_current(void); uint64_t dm_padmode_page_keys(uint64_t); static int sel = -1, sub, sel_view = -1; static dm_wref sel_sp; int pad = dm_padmode_current();
      if (vis(find_w(root, "ConfirmationModal", 1))) pad = 0;
      int here = 0; for (int k = 0; k < nr; k++) here |= rows[k] == sel && row_here(sp, sel);
      if (relay || dm_wref_get(&sel_sp) != sp || sel_view != g_view || !here) {
          dm_wref_set(&sel_sp, sp); sel_view = g_view; sel = -1; sub = 0; for (int k = 0; k < nr && sel < 0; k++) if (row_here(sp, rows[k])) sel = rows[k]; }
      uint64_t dm = find_w(sp, "TouchCustomSel", 0); int l = 0, t = 0, r = 0, b = 0, got = sel >= 0 && row_rect(sp, sel, sub, &l, &t, &r, &b);
      if (pad && sel >= 0) { uint64_t ev = dm_padmode_page_keys(DM_BTN_UP | DM_BTN_DOWN | DM_BTN_LEFT | DM_BTN_RIGHT | DM_BTN_A);
          if (ev & (DM_BTN_DOWN | DM_BTN_UP)) { int dir = ev & DM_BTN_DOWN ? 1 : -1, at = 0; for (int k = 0; k < nr; k++) if (rows[k] == sel) at = k;
              for (int k = at + dir; k >= 0 && k < nr; k += dir) if (row_here(sp, rows[k])) { sel = rows[k]; sub = 0; break; } }

          vrow rw = row_of(sel); char nb[64]; uint64_t first = find_w(sp, box_name(sel, &rw, 0, nb, sizeof nb), 0); int ready = first && dm_wref_get(&seen[sel]) == first;
          if (ready && (ev & DM_BTN_LEFT)) row_step(sp, sel, -1, &sub);
          if (ready && (ev & DM_BTN_RIGHT)) row_step(sp, sel, 1, &sub);
          if (ready && (ev & DM_BTN_A)) row_step(sp, sel, 0, &sub);

          { uint64_t dm_padmode_page_held(void); static int hdir; static uint64_t hnext; uint64_t held = dm_padmode_page_held() & (DM_BTN_LEFT | DM_BTN_RIGHT), now = now_ms();
            if (ev & (DM_BTN_LEFT | DM_BTN_RIGHT)) { hdir = ev & DM_BTN_RIGHT ? 1 : -1; hnext = now + 400; }
            else if (hdir && held == (hdir > 0 ? DM_BTN_RIGHT : DM_BTN_LEFT) && ready && row_of(sel).kind == R_SLIDER) { if (now >= hnext) { row_step(sp, sel, hdir, &sub); hnext = now + 80; } }
            else hdir = 0; }
          got = row_rect(sp, sel, sub, &l, &t, &r, &b); }
#ifndef DM_PLAYER_BUILD
      { static int previous = -2, enabled = -1;
        if (enabled < 0) enabled = getenv("DM_QA_STATE") != NULL;
        int current = pad ? sel : -1;
        if (enabled && current != previous) { previous = current; LOG("[qa] option focus=%s", current >= 0 ? row_key(current) : "none"); } }
#endif
      if (dm) { int32_t *q = (int32_t *)(dm + 0x70), want[4] = { 99999, 0, q[2], q[3] };
          if (dm_padmode_current() && got) { want[0] = l - 30; want[1] = t - 10; want[2] = r - l + 60; want[3] = b - t + 20; }
          if (memcmp(q, want, sizeof want)) { memcpy(q, want, sizeof want); ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)dm + 0x10))(dm); } }




      if (pad && got) { uint64_t rc = find_w(sp, g_has_video && g_view ? "TouchVideoRows" : "TouchCustomRows", 0), view = rc ? *(uint64_t *)(rc + 0x30) : 0, sv = view ? *(uint64_t *)(view + 0x30) : 0; static uint64_t lf; uint64_t f = dm_guest_frame();
          if (sv && f - lf >= 4) { int top = t - 10, bot = b + 10, off = -*(int32_t *)(view + 0x74), vh = *(int32_t *)(sv + 0x7c); if (vh <= 0) vh = 1260; void dm_ui_wheel_on(uint64_t, int);
              if (nr && sel == rows[0]) top = 0;
              static int lo, ldir, lsel = -1, stuck; int dir = top < off ? 1 : bot > off + vh ? -1 : 0;
              if (sel != lsel || dir != ldir || off != lo) stuck = 0; else if (dir) stuck++;
              lsel = sel; ldir = dir; lo = off;
              if (dir && stuck < 2) { lf = f; dm_ui_wheel_on(sv, dir); } } } }

    uint64_t absent = 0;
    for (int k = 0; k < nr; k++) { int i = rows[k], kind = row_of(i).kind;
        if (i < NK) { vrow rw = row_of(i); char nb[64]; if (!find_w(sp, box_name(i, &rw, 0, nb, sizeof nb), 0)) absent |= 1ull << i; }
        if (kind == R_TOGGLE) toggle_pump(sp, i, &seen[i]); else if (kind == R_SLIDER) slider_pump(sp, i, &seen[i]); else if (kind == R_CAROUSEL) car_pump(sp, i, &seen[i]);
        else if (kind == R_ACTION) { if (i == ROW_DRIVER) driver_pump(sp, &seen[i]); else action_pump(sp, &seen[i]); }
        else if (kind == R_PRESET) s3q_pump(sp, &seen[i]); else row_pump(sp, i, &seen[i]); }
    atomic_store(&g_absent, absent);
    if (g_has_video && g_view) video_text_pump(sp);


    { uint64_t lw = find_w(sp, "TouchCustom_lang_Text", 0); int pd = atomic_load(&g_pend[0]), shown = pd >= 0 ? pd : atomic_load(&g_val[0]), diff = shown != lang_running();
      for (int k = 0; k < LANG_N && lw; k++) { uint64_t hw = find_w(sp, k_lang_hint[k], 0); if (!hw) continue;
          int x = diff && shown == k ? ((int32_t *)(lw + 0x70))[0] : 99999; int32_t *q = (int32_t *)(hw + 0x70);
          if (q[0] != x) { q[0] = x; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)hw + 0x10))(hw); } } }




    static uint64_t vt_ok_for; static int vt_ok; if (vt_ok_for != *(uint64_t *)sp) { vt_ok_for = *(uint64_t *)sp; vt_ok = 0;
        for (int k = 0; k < 96 && !vt_ok; k++) vt_ok = *(uint64_t *)(vt_ok_for + 8 * k) == (uint64_t)main_base() + GA_OPTIONS_PAGE_UPDATE;
        if (!vt_ok) LOG("【红】自定义设置：借来的视频页虚表里没有 main+GA_OPTIONS_PAGE_UPDATE，不写「有改动」位（原版版本变了？）"); }
    if (vt_ok) { int dirty = 0; for (int i = 0; i < NK; i++) dirty |= changed(i, atomic_load(&g_pend[i]));
        if (*(uint8_t *)(sp + 0x1a0) != dirty) { *(uint8_t *)(sp + 0x1a0) = (uint8_t)dirty; LOG("自定义设置：有未应用的改动 → %d（「应用 / 放弃」%s）", dirty, dirty ? "点亮" : "变灰"); } }
    mu_unlock();
}
