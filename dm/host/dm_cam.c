















#include "dm.h"
#include "dm_cursor.h"
#include "dm_uiscreen.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>




static _Atomic int g_mode, g_mode_env = -1;
int dm_cam_mode(void) {
    int e = atomic_load(&g_mode_env);
    if (e < 0) { const char *s = getenv("DM_CAM"); e = s ? atoi(s) : 0x100; atomic_store(&g_mode_env, e); if (e != 0x100) LOG("DM_CAM=%d：镜头档按环境变量定（1 越肩 / 2 第一人称），自定义页改不动", e); }
    return e != 0x100 ? e : atomic_load(&g_mode); }
















#if defined(__ANDROID__)
#define DM_CAM_STYLE_DEFAULT 1
#else
#define DM_CAM_STYLE_DEFAULT 0
#endif
int dm_cam_style(void) { static _Atomic int st = -1; int v = atomic_load(&st); if (v < 0) { const char *e = getenv("DM_CAM_STYLE"); v = e ? atoi(e) : DM_CAM_STYLE_DEFAULT; atomic_store(&st, v); if (v) LOG("DM_CAM_STYLE=%d：越肩档用原神式操作（镜头被拖着走、摇杆按此刻镜头、右半屏左右拖转镜头）", v); } return v; }
void dm_cam_set_mode(int m) { if (m < 0 || m > 2) m = 0; if (atomic_exchange(&g_mode, m) != m) LOG("镜头视角 → %s（自定义页）%s", m == 1 ? "越肩" : m == 2 ? "第一人称" : "原版", atomic_load(&g_mode_env) >= 0 && atomic_load(&g_mode_env) != 0x100 ? "；环境变量 DM_CAM 设着，以它为准" : ""); }






typedef struct { float vpo[16], ivpo[16], vpp[16], ivpp[16], pl[2], py; int ak; } cam_t;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static cam_t g_pub; static double g_pub_t;
static double now_s(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }
static _Atomic int g_ahead_found = -1, g_ahead_applied = -1;
void dm_cam_publish(const float vpo[16], const float ivpo[16], const float vpp[16], const float ivpp[16], float px, float pz) {
    pthread_mutex_lock(&g_mu); memcpy(g_pub.vpo, vpo, 64); memcpy(g_pub.ivpo, ivpo, 64); memcpy(g_pub.vpp, vpp, 64); memcpy(g_pub.ivpp, ivpp, 64);
    g_pub.pl[0] = px; g_pub.pl[1] = pz; g_pub.ak = atomic_load(&g_ahead_found); g_pub_t = now_s(); pthread_mutex_unlock(&g_mu); }




void dm_cam_publish_height(float py) { pthread_mutex_lock(&g_mu); g_pub.py = py; pthread_mutex_unlock(&g_mu); }









enum { CAM_DIRS = 32 };
static float g_clear[CAM_DIRS], g_clear_p[2]; static double g_clear_t;
void dm_cam_set_clear(const float d[32], float px, float pz) {
    pthread_mutex_lock(&g_mu); memcpy(g_clear, d, sizeof g_clear); g_clear_p[0] = px; g_clear_p[1] = pz; g_clear_t = now_s(); pthread_mutex_unlock(&g_mu); }
int dm_cam_clearance(float bx, float bz, float *dist, float *px, float *pz) {
    pthread_mutex_lock(&g_mu); int ok = g_clear_t > 0 && now_s() - g_clear_t < 0.5;
    if (ok) { float a = atan2f(bz, bx) / 6.2831853f * CAM_DIRS; if (a < 0) a += CAM_DIRS; int i = (int)a % CAM_DIRS, j = (i + 1) % CAM_DIRS;



        const int far = dm_cam_style() == 1;
        *dist = (g_clear[i] < g_clear[j]) != far ? g_clear[i] : g_clear[j]; *px = g_clear_p[0]; *pz = g_clear_p[1]; }
    pthread_mutex_unlock(&g_mu); return ok; }

















enum { AHEAD_N = 24 };
static _Atomic int g_ahead_k = -1, g_ahead_cm = -1;
static _Atomic long long g_ahead_seen_ms;





static float ahead_len(void) {
    int v = atomic_load(&g_ahead_cm);
    if (v < 0) { float dm_cam_zoom_want(void); int dm_cam_zoom_on(void); const char *e = getenv("DM_CAM_AHEAD"); float f = e ? (float)atof(e) : 0.5f; if (!(f > 0) || !dm_cam_zoom_on()) f = 0; else if (f > 0.9f) f = 0.9f;
        float s = f * 0.75f * 70.7107f * atanf(0.03f / dm_cam_zoom_want()) / atanf(0.03f); if (s > 0 && s < 30) s = 30;
        v = (int)lroundf(s * 100); atomic_store(&g_ahead_cm, v); LOG("原版画面前移：比例 %.2f，挪 %.0f 个单位（%d 个方向）", f, s, AHEAD_N); }
    return v / 100.0f; }
static void ahead_vec(int k, float *x, float *z) { float s = ahead_len(), a = (float)k * (6.2831853f / AHEAD_N); *x = k < 0 ? 0 : s * cosf(a); *z = k < 0 ? 0 : s * sinf(a); }

void svc_cam_ahead(uint64_t *regs) {
    static uint8_t *B;
    if (!B) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) B = (uint8_t *)g_mod[i].base;
    regs[0] = ((uint64_t (*)(uint64_t))(B + 0x1628978))(regs[0]);
    int k = atomic_load(&g_ahead_k);
    uint8_t *ctl = (uint8_t *)(uintptr_t)regs[19], *cam = (uint8_t *)(uintptr_t)regs[20];
    if (k < 0 || !ctl || !cam || dm_cam_mode() <= 0 || (long long)(now_s() * 1000) - atomic_load(&g_ahead_seen_ms) > 500) { atomic_store(&g_ahead_applied, -1); return; }
    atomic_store(&g_ahead_applied, k);
    float x, z; ahead_vec(k, &x, &z);
    *(float *)(cam + 0x100) += x; *(float *)(cam + 0x108) += z; *(float *)(ctl + 0xc) += x; *(float *)(ctl + 0x14) += z; }





int dm_cam_ahead_pick(float rx, float rz, int host_ok, float hx, float hz, float fx, float fz, float *ox, float *oz) {
    static int found = -1, have_last, misses, matched; static float lx, lz;
    const float s = ahead_len(), step = 6.2831853f / AHEAD_N, tol = 0.45f * 2 * s * sinf(step / 2);
    if (s <= 0) found = -1;
    else if (host_ok || have_last) {
        const float tx = host_ok ? hx : lx, tz = host_ok ? hz : lz; int best = found; float bd = 1e30f;
        for (int c = -1; c < AHEAD_N; c++) { float vx, vz; ahead_vec(c, &vx, &vz); float d = hypotf(rx - vx - tx, rz - vz - tz); if (d < bd) { bd = d; best = c; } }
        if (bd < tol) { found = best; misses = 0; if (best >= 0 && !matched++) LOG("原版画面前移：认出第一帧挪动（方向 %d / %d）", best, AHEAD_N); }
        else if (++misses == 30) LOG("【黄】原版画面前移：连续 30 帧认不出挪动（最近 %.1f 个单位，宿主位置 %s），沿用方向 %d", bd, host_ok ? "有" : "没有", found); }
    float vx, vz; ahead_vec(found, &vx, &vz); *ox = vx; *oz = vz; lx = rx - vx; lz = rz - vz; have_last = 1; atomic_store(&g_ahead_found, found);
    if (s > 0 && fx * fx + fz * fz > 0.25f) {
        const float a = atan2f(fz, fx); int k = atomic_load(&g_ahead_k); const int kn = (((int)lroundf(a / step)) % AHEAD_N + AHEAD_N) % AHEAD_N;
        if (k < 0 || fabsf(remainderf(a - (float)k * step, 6.2831853f)) > 0.65f * step) atomic_store(&g_ahead_k, kn); }
    atomic_store(&g_ahead_seen_ms, (long long)(now_s() * 1000));
    return found >= 0; }







static int ahead_delta(int ak, float *dx, float *dz) {
    const int k = atomic_load(&g_ahead_applied); if (k == ak) return 0;
    float ax, az, bx, bz; ahead_vec(k, &ax, &az); ahead_vec(ak, &bx, &bz); *dx = ax - bx; *dz = az - bz; return 1; }
static void ahead_fix(float vpo[16], float ivpo[16], float dx, float dz) {
    for (int j = 0; j < 4; j++) vpo[12 + j] -= dx * vpo[j] + dz * vpo[8 + j];
    if (ivpo) for (int i = 0; i < 4; i++) { ivpo[i * 4] += ivpo[i * 4 + 3] * dx; ivpo[i * 4 + 2] += ivpo[i * 4 + 3] * dz; } }
static int cam_get(cam_t *c) {
    if (dm_cam_mode() <= 0) return 0;
    pthread_mutex_lock(&g_mu); int ok = g_pub_t > 0 && now_s() - g_pub_t < 0.5; if (ok) *c = g_pub; pthread_mutex_unlock(&g_mu);
    float dx, dz; if (ok && ahead_delta(c->ak, &dx, &dz)) ahead_fix(c->vpo, c->ivpo, dx, dz);
    return ok; }

static void xf(const float *M, const float v[4], float o[4]) { for (int j = 0; j < 4; j++) o[j] = v[0] * M[j] + v[1] * M[4 + j] + v[2] * M[8 + j] + v[3] * M[12 + j]; }











static void m4mul(const float *a, const float *b, float *o) { for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { double s = 0; for (int k = 0; k < 4; k++) s += (double)a[i * 4 + k] * b[k * 4 + j]; o[i * 4 + j] = (float)s; } }


int dm_cam_label_matrix(const float p[16], float out[16]) {
    if (dm_cam_mode() <= 0) return 0;
    float t[16]; pthread_mutex_lock(&g_mu); int ok = g_pub_t > 0 && now_s() - g_pub_t < 0.5;
    if (ok) { m4mul(p, g_pub.ivpo, t); float dx, dz; if (ahead_delta(g_pub.ak, &dx, &dz)) for (int i = 0; i < 4; i++) { t[i * 4] += t[i * 4 + 3] * dx; t[i * 4 + 2] += t[i * 4 + 3] * dz; } m4mul(t, g_pub.vpp, out); }
    pthread_mutex_unlock(&g_mu); return ok; }

static void n2c(float nx, float ny, float *x, float *y) { *x = nx * 2 - 1; *y = 1 - ny * 2; }
static int proj(const float *VP, const float w[3], float *nx, float *ny) { float v[4] = { w[0], w[1], w[2], 1 }, o[4]; xf(VP, v, o); if (o[3] <= 1e-6f) return 0; *nx = (o[0] / o[3] + 1) / 2; *ny = (1 - o[1] / o[3]) / 2; return 1; }

static int ray(const cam_t *c, float nx, float ny, float o[3], float d[3]) {
    float x, y; n2c(nx, ny, &x, &y); float a[4] = { x, y, 1, 1 }, b[4] = { x, y, 0.001f, 1 }, p[4], q[4]; xf(c->ivpp, a, p); xf(c->ivpp, b, q);
    if (fabsf(p[3]) < 1e-9f || fabsf(q[3]) < 1e-9f) return 0;
    for (int i = 0; i < 3; i++) { o[i] = p[i] / p[3]; d[i] = q[i] / q[3] - o[i]; } return 1; }

static int ortho_ground(const cam_t *c, float nx, float ny, float w[3]) {
    float x, y; n2c(nx, ny, &x, &y); float a[4] = { x, y, 0, 1 }, b[4] = { x, y, 1, 1 }, p[4], q[4]; xf(c->ivpo, a, p); xf(c->ivpo, b, q);
    float dy = q[1] / q[3] - p[1] / p[3]; if (fabsf(dy) < 1e-9f) return 0; float t = -(p[1] / p[3]) / dy;
    for (int i = 0; i < 3; i++) w[i] = p[i] / p[3] + t * (q[i] / q[3] - p[i] / p[3]); return 1; }

int dm_cam_on(void) { cam_t c; return cam_get(&c); }





static int cam_settled(void) {
    static _Atomic long long since_ms = -1;
    if (!dm_uiscreen_ingame()) { atomic_store(&since_ms, -1); return 0; }
    long long t = (long long)(now_s() * 1000), s0 = atomic_load(&since_ms);
    if (s0 < 0) { atomic_store(&since_ms, t); return 0; }
    return t - s0 >= 1500; }











int dm_cam_heading(float *fx, float *fz, float *sx, float *sy) {
    cam_t c; float o[3], d[3]; if (!cam_settled() || !cam_get(&c) || !ray(&c, 0.5f, 0.5f, o, d)) return 0;
    float l = hypotf(d[0], d[2]); if (l < 1e-6f) return 0; *fx = d[0] / l; *fz = d[2] / l;
    *sx = 0.5f; *sy = 0.5f;
    return 1; }










static float g_thr_xy[64]; static int g_thr_tier[32], g_thr_n; static double g_thr_t;
void dm_cam_set_threats(const float *xy, const int *tier, int n) {
    if (n < 0) n = 0; if (n > 32) n = 32;
    pthread_mutex_lock(&g_mu); g_thr_n = n; if (n) { memcpy(g_thr_xy, xy, sizeof(float) * 2 * n); memcpy(g_thr_tier, tier, sizeof(int) * n); } g_thr_t = now_s(); pthread_mutex_unlock(&g_mu); }
int dm_cam_threats(float *out, int max) {
    cam_t c; if (max <= 0 || !cam_settled() || !cam_get(&c)) return 0;
    static float sub = -1, range = -1;
    if (sub < 0) { const char *e = getenv("DM_CAM_SUB"); sub = e ? (float)atof(e) : 2.0f; e = getenv("DM_CAM_THREAT_RANGE"); range = e ? (float)atof(e) : 30; }
    float xy[64]; int tier[32], n;
    pthread_mutex_lock(&g_mu); n = g_thr_t > 0 && now_s() - g_thr_t < 0.5 ? g_thr_n : 0; memcpy(xy, g_thr_xy, sizeof xy); memcpy(tier, g_thr_tier, sizeof tier); pthread_mutex_unlock(&g_mu);
    float o[3], d[3]; if (!n || !ray(&c, 0.5f, 0.5f, o, d)) return 0;
    float fx = d[0], fz = d[2], fl = hypotf(fx, fz); if (fl < 1e-6f) return 0; fx /= fl; fz /= fl; float rx = -fz, rz = fx;


    const float L = 0.06f, R = 0.94f, T = 0.08f, B = 0.80f, corner = atan2f(R - 0.5f, B - 0.5f);
    int m = 0;
    for (int i = 0; i < n; i++) {
        float wx = xy[2 * i] * sub, wz = xy[2 * i + 1] * sub, v[4] = { wx, 3, wz, 1 }, p[4]; xf(c.vpp, v, p);
        if (p[3] > 0 && fabsf(p[0] / p[3]) < 0.92f && fabsf(p[1] / p[3]) < 0.92f) continue;
        float ex = wx - c.pl[0], ez = wz - c.pl[1], dist = hypotf(ex, ez), th = atan2f(ex * rx + ez * rz, ex * fx + ez * fz), sx, sy;
        if (fabsf(th) > 3.14159265f - corner) { sx = th > 0 ? R : L; sy = B; }
        else { float dx = sinf(th), dy = -cosf(th), t = 1e9f;
            if (dx > 1e-6f) t = fminf(t, (R - 0.5f) / dx); if (dx < -1e-6f) t = fminf(t, (L - 0.5f) / dx);
            if (dy > 1e-6f) t = fminf(t, (B - 0.5f) / dy); if (dy < -1e-6f) t = fminf(t, (T - 0.5f) / dy);
            sx = 0.5f + dx * t; sy = 0.5f + dy * t; }
        float urg = 1 - dist / (range * sub); if (urg < 0) urg = 0; if (urg > 1) urg = 1;
        int slot = m < max ? m++ : -1;
        if (slot < 0) { int low = 0; for (int k = 1; k < max; k++) if (out[k * 5 + 3] < out[low * 5 + 3]) low = k; if (out[low * 5 + 3] >= urg) continue; slot = low; }
        float *e = out + slot * 5; e[0] = sx; e[1] = sy; e[2] = th; e[3] = urg; e[4] = (float)tier[i];
    }
    return m;
}















int dm_cam_map_heading(float out[4]) {
    float fx, fz, sx, sy, cell, mm[4]; int map = dm_ui_map_geom(&cell);
    if (!map || !dm_cam_heading(&fx, &fz, &sx, &sy)) return 0;
    float mx = (fx - fz) * 1.6f, my = (fx + fz) * 0.8f;
    if (map == 1) {
        if (!dm_ui_minimap_rect(mm) || !(mm[3] > mm[1])) return 0;
        out[0] = (mm[0] + mm[2]) / 2; out[1] = (mm[1] + mm[3]) / 2; out[3] = mm[3] - mm[1];
    } else { out[0] = sx; out[1] = sy; out[3] = 0.5f; }
    out[2] = atan2f(mx, -my);
    return map;
}



static _Atomic int g_want_x1000, g_want_z1000, g_want_on;






static _Atomic int g_atk_x1000, g_atk_z1000; static double g_atk_t;
void dm_cam_attack(float dx, float dz) {
    float l = hypotf(dx, dz); if (dm_cam_mode() != 1 || !(l >= 0.05f)) return;
    atomic_store(&g_atk_x1000, (int)lroundf(dx / l * 1000)); atomic_store(&g_atk_z1000, (int)lroundf(dz / l * 1000));
    pthread_mutex_lock(&g_mu); g_atk_t = now_s(); pthread_mutex_unlock(&g_mu);
    static _Atomic int said; if (atomic_fetch_add(&said, 1) < 8) LOG("镜头：放技能，朝目标转 %.2f,%.2f（距 %.1f 子格）", dx / l, dz / l, l); }
int dm_cam_want_dir(float *wx, float *wz) {
    if (atomic_load(&g_want_on)) { *wx = atomic_load(&g_want_x1000) / 1000.0f; *wz = atomic_load(&g_want_z1000) / 1000.0f; return 1; }
    pthread_mutex_lock(&g_mu); double t = g_atk_t; pthread_mutex_unlock(&g_mu);
    if (!(t > 0 && now_s() - t < 0.8)) return 0;
    *wx = atomic_load(&g_atk_x1000) / 1000.0f; *wz = atomic_load(&g_atk_z1000) / 1000.0f; return 1; }





int dm_cam_attack_dir(float *wx, float *wz) {
    if (atomic_load(&g_want_on)) return 0;
    pthread_mutex_lock(&g_mu); double t = g_atk_t; pthread_mutex_unlock(&g_mu);
    if (!(t > 0 && now_s() - t < 0.8)) return 0;
    *wx = atomic_load(&g_atk_x1000) / 1000.0f; *wz = atomic_load(&g_atk_z1000) / 1000.0f; return 1; }





static int remap_one(const cam_t *c, int32_t *x, int32_t *y, float wdir[2]) {
    if (!*x && !*y) return 0;
    float mag = hypotf((float)*x, (float)*y), sx = *x / mag, sy = *y / mag * 0.5f, sl = hypotf(sx, sy); sx /= sl; sy /= sl;
    float o[3], d[3]; if (!ray(c, 0.5f, 0.5f, o, d)) return 0; float fx = d[0], fz = d[2], fl = hypotf(fx, fz); if (fl < 1e-9f) return 0; fx /= fl; fz /= fl;
    float wx = sx * -fz + sy * fx, wz = sx * fx + sy * fz, wl = hypotf(wx, wz); wdir[0] = wx / wl; wdir[1] = wz / wl;
    int W = 0, H = 0; if (!dm_screen_real(&W, &H) || W <= 0 || H <= 0) return 0; float asp = (float)W / H;
    float P0[3] = { c->pl[0], 0, c->pl[1] }, P1[3] = { c->pl[0] + wdir[0] * 10, 0, c->pl[1] + wdir[1] * 10 }, oa[2], ob[2];
    if (!proj(c->vpo, P0, &oa[0], &oa[1]) || !proj(c->vpo, P1, &ob[0], &ob[1])) return 0;
    float ox = (ob[0] - oa[0]) * asp, oy = -(ob[1] - oa[1]), ol = hypotf(ox, oy); if (ol < 1e-9f) return 0; ox /= ol; oy /= ol;
    float kx = ox, ky = oy * 2, kl = hypotf(kx, ky); kx /= kl; ky /= kl;
    *x = (int32_t)lroundf(kx * mag); *y = (int32_t)lroundf(ky * mag); if (*x > 32767) *x = 32767; if (*x < -32767) *x = -32767; if (*y > 32767) *y = 32767; if (*y < -32767) *y = -32767;
    return 1;
}
















int dm_cam_sensor_mode(void) { static int m = -1; if (m < 0) { const char *e = getenv("DM_CAM_SENSOR"); m = e ? atoi(e) : 0; if (m) LOG("DM_CAM_SENSOR=%d：体感转向开（1 转手机看四周 / 2 侧倾转向）", m); } return dm_cam_style() == 1 ? 0 : m; }
static float g_turn, g_turn_total; static double g_turn_t;
void dm_cam_turn(float rad) { if (rad == 0 || dm_cam_mode() <= 0) return; pthread_mutex_lock(&g_mu); g_turn += rad; g_turn_total += rad; g_turn_t = now_s(); pthread_mutex_unlock(&g_mu); }



static _Atomic int g_tilt_x10000;
void dm_cam_tilt(float rad) { int v = atomic_load(&g_tilt_x10000) + (int)lroundf(rad * 10000); if (v > 14000) v = 14000; if (v < -14000) v = -14000; atomic_store(&g_tilt_x10000, v); }
float dm_cam_tilt_get(void) { return atomic_load(&g_tilt_x10000) / 10000.0f; }
void dm_cam_tilt_clamp(float lo, float hi) { int l = (int)lroundf(lo * 10000), h = (int)lroundf(hi * 10000), v = atomic_load(&g_tilt_x10000); if (l <= h && (v < l || v > h)) atomic_store(&g_tilt_x10000, v < l ? l : h); }
float dm_cam_take_turn(void) { pthread_mutex_lock(&g_mu); float r = g_turn; g_turn = 0; pthread_mutex_unlock(&g_mu); return r; }
double dm_cam_turn_age(void) { pthread_mutex_lock(&g_mu); double a = g_turn_t > 0 ? now_s() - g_turn_t : 1e9; pthread_mutex_unlock(&g_mu); return a; }
static float turn_total(void) { pthread_mutex_lock(&g_mu); float t = g_turn_total; pthread_mutex_unlock(&g_mu); return t; }



static _Atomic int g_aim_x1000, g_aim_z1000; static double g_aim_t;
static cam_t g_ref_l, g_ref_r; static int g_ref_l_on, g_ref_r_on; static float g_ref_l_turn, g_ref_r_turn;
void dm_cam_remap_stick(int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry) {
    cam_t c; if (!cam_get(&c)) { atomic_store(&g_want_on, 0); g_ref_l_on = g_ref_r_on = 0; return; }
    float tt = turn_total();



    const int live = dm_cam_style() == 1 && dm_cam_mode() == 1;
    if (!*lx && !*ly) g_ref_l_on = 0; else if (live || !g_ref_l_on || fabsf(tt - g_ref_l_turn) > 0.035f) { g_ref_l = c; g_ref_l_on = 1; g_ref_l_turn = tt; }
    if (!*rx && !*ry) g_ref_r_on = 0; else if (live || !g_ref_r_on || fabsf(tt - g_ref_r_turn) > 0.035f) { g_ref_r = c; g_ref_r_on = 1; g_ref_r_turn = tt; }
    cam_t cl = c, cr = c; memcpy(cl.vpp, g_ref_l.vpp, 64); memcpy(cl.ivpp, g_ref_l.ivpp, 64); memcpy(cr.vpp, g_ref_r.vpp, 64); memcpy(cr.ivpp, g_ref_r.ivpp, 64);
    float wl[2], wr[2]; int l = g_ref_l_on && remap_one(&cl, lx, ly, wl), r = g_ref_r_on && remap_one(&cr, rx, ry, wr);
    if (r) { atomic_store(&g_aim_x1000, (int)lroundf(wr[0] * 1000)); atomic_store(&g_aim_z1000, (int)lroundf(wr[1] * 1000)); pthread_mutex_lock(&g_mu); g_aim_t = now_s(); pthread_mutex_unlock(&g_mu); }
    const float *w = r ? wr : l ? wl : NULL;
    if (w) { atomic_store(&g_want_x1000, (int)lroundf(w[0] * 1000)); atomic_store(&g_want_z1000, (int)lroundf(w[1] * 1000)); }
    atomic_store(&g_want_on, w != NULL);
}











int dm_cam_aim(float out[50]) {
    cam_t c; if (!cam_get(&c)) return 0;
    pthread_mutex_lock(&g_mu); double t = g_aim_t; pthread_mutex_unlock(&g_mu);
    if (!(t > 0 && now_s() - t < 0.15)) return 0;
    float ax = atomic_load(&g_aim_x1000) / 1000.0f, az = atomic_load(&g_aim_z1000) / 1000.0f, l = hypotf(ax, az); if (l < 1e-3f) return 0; ax /= l; az /= l;
    const float rx = -az, rz = ax, cx = c.pl[0] + ax * 12, cz = c.pl[1] + az * 12, r = 2.4f;
    for (int i = 0; i < 5; i++) for (int j = 0; j < 5; j++) {
        const float u = (j / 4.0f - 0.5f) * 2 * r, v = (0.5f - i / 4.0f) * 2 * r;
        const float w[3] = { cx + rx * u + ax * v, c.py + 0.15f, cz + rz * u + az * v };
        if (!proj(c.vpp, w, &out[2 * (i * 5 + j)], &out[2 * (i * 5 + j) + 1])) return 0; }
    return 25;
}



int dm_cam_tap_for_unit(float tx, float ty, float fx, float fy, float *ox, float *oy) {
    cam_t c; if (!cam_get(&c)) return 0;
    float F[3], o[3], d[3]; if (!ortho_ground(&c, fx, fy, F) || !ray(&c, tx, ty, o, d)) return 0;
    float nx = d[0], nz = d[2], nl = hypotf(nx, nz); if (nl < 1e-9f) return 0; nx /= nl; nz /= nl;
    float den = d[0] * nx + d[2] * nz; if (fabsf(den) < 1e-9f) return 0; float t = ((F[0] - o[0]) * nx + (F[2] - o[2]) * nz) / den; if (t <= 0) return 0;
    float Q[3] = { o[0] + t * d[0], o[1] + t * d[1], o[2] + t * d[2] }; return proj(c.vpo, Q, ox, oy);
}


int dm_cam_unit_to_screen(float fx, float fy, float hx, float hy, float *ox, float *oy) {
    cam_t c; if (!cam_get(&c)) return 0;
    float F[3]; if (!ortho_ground(&c, fx, fy, F)) return 0;

    float x, y; n2c(hx, hy, &x, &y); float a[4] = { x, y, 0, 1 }, b[4] = { x, y, 1, 1 }, p[4], q[4]; xf(c.ivpo, a, p); xf(c.ivpo, b, q);
    float P[3] = { p[0] / p[3], p[1] / p[3], p[2] / p[3] }, D[3] = { q[0] / q[3] - P[0], q[1] / q[3] - P[1], q[2] / q[3] - P[2] };
    float dh = D[0] * D[0] + D[2] * D[2]; float s = dh > 1e-12f ? ((F[0] - P[0]) * D[0] + (F[2] - P[2]) * D[2]) / dh : 0; float h = P[1] + s * D[1]; if (!(h > 0 && h < 50)) h = 0;
    float M[3] = { F[0], h / 2, F[2] }; return proj(c.vpp, M, ox, oy);
}
