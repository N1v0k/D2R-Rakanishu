














#pragma once
#include <stdint.h>
#include <string.h>

#define DM_RES_AUTO 1
#define DM_RES_PCT_MIN 50
#define DM_RES_MIN_LINES 720
#define DM_RES_W_MAX 4096

static inline int dm_res_wider(int W, int H) { return (int64_t)W * 9 > (int64_t)H * 16; }
static inline int dm_res_base_h(int W, int H) { int hb = 2 * (9 * W / 32); return hb < H ? hb : H; }
static inline int dm_res_auto_lines(int H) { (void)H; return 900; }



static inline int dm_res_width(int h, int W, int H, int panel) {
    int w, hb = dm_res_base_h(W, H), cap = panel && W / 8 * 8 < DM_RES_W_MAX ? W / 8 * 8 : DM_RES_W_MAX;
    if (hb <= 0 || h <= 0) return 0;
    if (dm_res_wider(W, H)) w = (int)(((int64_t)2 * h * W + 8 * (int64_t)hb) / (16 * (int64_t)hb)) * 8;
    else w = (h * 16 / 9 + 7) / 8 * 8;
    return w > cap ? cap : w;
}

static inline int dm_res_scale_size(int pct, int W, int H, int *w, int *h) {
    if (W < H) { int t = W; W = H; H = t; }
    if (W <= 0 || H <= 0) { *w = *h = 0; return 0; }
    int hb = dm_res_base_h(W, H), hh, auto_ = pct == DM_RES_AUTO;
    if (auto_) hh = dm_res_auto_lines(H);
    else { if (pct > 100) pct = 100; if (pct < DM_RES_PCT_MIN) pct = DM_RES_PCT_MIN; hh = 2 * ((hb * pct + 100) / 200); }
    *h = hh; *w = dm_res_width(hh, W, H, !auto_); return *w > 0;
}

static inline int dm_res_scale_lo(int W, int H) {
    for (int p = DM_RES_PCT_MIN; p < 100; p += 5) { int w, h; if (dm_res_scale_size(p, W, H, &w, &h) && h >= DM_RES_MIN_LINES) return p; }
    return 100;
}

static inline int dm_res_pct_clamp(int pct, int W, int H) {
    if (pct == DM_RES_AUTO) return pct;
    int lo = dm_res_scale_lo(W, H); if (pct < lo) pct = lo; if (pct > 100) pct = 100; return pct;
}

static inline int dm_res_scale_nearest_lines(int lines, int W, int H) {
    int best = 100, bd = -1;
    for (int p = dm_res_scale_lo(W, H); p <= 100; p += 5) { int w, h, d; if (!dm_res_scale_size(p, W, H, &w, &h)) return 100; d = h > lines ? h - lines : lines - h; if (bd < 0 || d < bd) { bd = d; best = p; } }
    return best;
}

static inline int dm_res_legacy_lines(int level, int H) { return level == 1 && H > 1080 ? 1080 : level == 2 && H > 900 ? H : 900; }

static inline void dm_res_handheld(int dw, int dh, int is_auto, int W, int H, int *w, int *h) {
    if (W < H) { int t = W; W = H; H = t; }
    int hh = is_auto ? DM_RES_MIN_LINES : (dh * 4 / 5) & ~1; if (hh < DM_RES_MIN_LINES) hh = DM_RES_MIN_LINES; if (hh > dh) hh = dh;
    *h = hh; *w = hh == dh ? dw : dm_res_width(hh, W, H, !is_auto);
}





#define DM_RES_FULL_UI_W 1280
static inline int dm_res_full_on(int W, int H) { if (W < H) { int t = W; W = H; H = t; } return W > 0 && H > 0 && (int64_t)W * 900 < (int64_t)H * 1591; }
static inline int dm_res_full_width(int h, int W, int H, int panel) {
    int cap = panel && W / 8 * 8 < DM_RES_W_MAX ? W / 8 * 8 : DM_RES_W_MAX;
    if (H <= 0 || h <= 0) return 0;
    int w = (int)(((int64_t)2 * h * W + 8 * (int64_t)H) / (16 * (int64_t)H)) * 8;
    return w > cap ? cap : w;
}
static inline int dm_res_full_min(int W, int H) {
    if (W < H) { int t = W; W = H; H = t; }
    if (W <= 0 || H <= 0) return DM_RES_MIN_LINES;
    int h = 2 * (int)(((int64_t)DM_RES_FULL_UI_W * H + 2 * (int64_t)W - 1) / (2 * (int64_t)W));
    if (h < DM_RES_MIN_LINES) h = DM_RES_MIN_LINES;
    while (dm_res_full_width(h, W, H, 0) < DM_RES_FULL_UI_W) h += 2;
    return h;
}
static inline int dm_res_auto_lines_x(int W, int H, int full) { int m = full ? dm_res_full_min(W, H) : 0; return m > 900 ? m : 900; }

static inline int dm_res_scale_size_x(int pct, int W, int H, int full, int *w, int *h) {
    if (!full) return dm_res_scale_size(pct, W, H, w, h);
    if (W < H) { int t = W; W = H; H = t; }
    if (W <= 0 || H <= 0) { *w = *h = 0; return 0; }
    int hh, auto_ = pct == DM_RES_AUTO;
    if (auto_) hh = dm_res_auto_lines_x(W, H, 1);
    else { if (pct > 100) pct = 100; if (pct < DM_RES_PCT_MIN) pct = DM_RES_PCT_MIN; hh = 2 * ((H * pct + 100) / 200); }
    *h = hh; *w = dm_res_full_width(hh, W, H, !auto_);
    if (!auto_ && *w < DM_RES_FULL_UI_W) return dm_res_scale_size_x(DM_RES_AUTO, W, H, 1, w, h);
    return *w > 0;
}
static inline int dm_res_scale_lo_x(int W, int H, int full) {
    if (!full) return dm_res_scale_lo(W, H);
    int m = dm_res_full_min(W, H);
    if (W < H) { int t = W; W = H; H = t; }
    for (int p = DM_RES_PCT_MIN; p < 100; p += 5) { int h = 2 * ((H * p + 100) / 200); if (h >= m && dm_res_full_width(h, W, H, 1) >= DM_RES_FULL_UI_W) return p; }
    return 100;
}
static inline int dm_res_pct_clamp_x(int pct, int W, int H, int full) {
    if (pct == DM_RES_AUTO) return pct;
    int lo = dm_res_scale_lo_x(W, H, full); if (pct < lo) pct = lo; if (pct > 100) pct = 100; return pct;
}
static inline int dm_res_scale_nearest_lines_x(int lines, int W, int H, int full) {
    if (!full) return dm_res_scale_nearest_lines(lines, W, H);
    int best = 100, bd = -1;
    for (int p = dm_res_scale_lo_x(W, H, 1); p <= 100; p += 5) { int w, h, d; if (!dm_res_scale_size_x(p, W, H, 1, &w, &h)) return 100; d = h > lines ? h - lines : lines - h; if (bd < 0 || d < bd) { bd = d; best = p; } }
    return best;
}
static inline void dm_res_handheld_x(int dw, int dh, int is_auto, int W, int H, int full, int *w, int *h) {
    if (!full) { dm_res_handheld(dw, dh, is_auto, W, H, w, h); return; }
    if (W < H) { int t = W; W = H; H = t; }
    int m = dm_res_full_min(W, H), hh = is_auto ? m : (dh * 4 / 5) & ~1; if (hh < m) hh = m; if (hh > dh) hh = dh;
    *h = hh; *w = hh == dh ? dw : dm_res_full_width(hh, W, H, !is_auto);
}






#define DM_FPS_EXPLICIT_MAX 240
static inline int dm_fps_notches(int max_hz, int all, int *out, int cap) {
    int n = 0;
    if (!all) { if (cap >= 2) { out[0] = 30; out[1] = 60; return 2; } return 0; }
    if (max_hz <= 0) max_hz = 60;
    for (int f = 30; f < max_hz && f <= DM_FPS_EXPLICIT_MAX && n < cap - 1; f += 5) out[n++] = f;
    if (n < cap) out[n++] = 0;
    return n;
}





static inline int dm_fps_divides(int hz, int fps) {
    if (hz <= 0 || fps <= 0 || hz < fps) return 0;
    int k = (hz + fps / 2) / fps; int64_t d = (int64_t)hz * 100 - (int64_t)k * fps * 100; if (d < 0) d = -d;
    return k >= 1 && d <= (int64_t)hz;
}
static inline int dm_fps_vote(int fps, const int *modes, int n, int policy, int *even) {
    int hi = 0, pick = 0; for (int i = 0; i < n; i++) if (modes[i] > hi) hi = modes[i];
    if (even) *even = 1;
    if (!n || hi <= 0) return fps > 0 ? fps : 144;
    if (fps <= 0) return hi;
    for (int i = 0; i < n; i++) if (dm_fps_divides(modes[i], fps) && (!pick || (policy ? modes[i] > pick : modes[i] < pick))) pick = modes[i];
    if (pick) return pick;
    if (even) *even = 0;
    return hi;
}





#define DM_RES_PRESET_N 3
typedef struct { int res_pct, s3d; } dm_res_preset_t;
static const dm_res_preset_t dm_res_presets[DM_RES_PRESET_N] = { { DM_RES_AUTO, 60 }, { DM_RES_AUTO, 0 }, { 100, 100 } };
static inline int dm_res_preset_of(int res_pct, int s3d, int res_on) {
    for (int k = 0; k < DM_RES_PRESET_N; k++) if (dm_res_presets[k].s3d == s3d && (!res_on || dm_res_presets[k].res_pct == res_pct)) return k;
    return DM_RES_PRESET_N;
}













#define DM_RES_LIVE_FRAMES 120
enum { DM_RL_IDLE, DM_RL_FORWARD, DM_RL_BACK };
enum { DM_RL_NONE, DM_RL_REACHED, DM_RL_REVERT, DM_RL_REVERTED, DM_RL_BROKEN };
typedef struct { int phase, frames; int tw, th, thw, thh; int pw, ph, phw, phh; } dm_res_live_t;
static inline void dm_res_live_begin(dm_res_live_t *s, int tw, int th, int thw, int thh, int pw, int ph, int phw, int phh) {
    s->phase = DM_RL_FORWARD; s->frames = 0; s->tw = tw; s->th = th; s->thw = thw; s->thh = thh; s->pw = pw; s->ph = ph; s->phw = phw; s->phh = phh; }



static inline int dm_res_live_step(dm_res_live_t *s, int cw, int ch, int ew, int eh, int native, int geom, int docked) {
    if (s->phase == DM_RL_IDLE) return DM_RL_NONE;
    int hit = docked ? cw == s->tw && ch == s->th : cw == s->thw && ch == s->thh;
    if (hit && geom == 0 && (!native || (ew == cw && eh == ch))) { int ph = s->phase; s->phase = DM_RL_IDLE; return ph == DM_RL_FORWARD ? DM_RL_REACHED : DM_RL_REVERTED; }
    if (++s->frames < DM_RES_LIVE_FRAMES) return DM_RL_NONE;
    if (s->phase == DM_RL_FORWARD) { int a = s->tw, b = s->th, c = s->thw, d = s->thh; s->tw = s->pw; s->th = s->ph; s->thw = s->phw; s->thh = s->phh; s->pw = a; s->ph = b; s->phw = c; s->phh = d;
        s->phase = DM_RL_BACK; s->frames = 0; return DM_RL_REVERT; }
    s->phase = DM_RL_IDLE; return DM_RL_BROKEN;
}



static inline int dm_res_live_fits(int w, int h, int sw, int sh, int gw, int gh) {
    if (w <= 0 || h <= 0 || gw <= 0 || gh <= 0) return 0;
    return w <= (sw < gw ? sw : gw) && h <= (sh < gh ? sh : gh);
}


static inline int dm_res_live_boot(int fails, int pending, int *off) { if (pending) fails++; *off = fails >= 2; return fails; }












typedef struct { const char *soc, *platform, *gpu; int64_t budget_px; int s3d; int estimate; } dm_dev_profile_t;












#define DM_DEV_BUDGET_CONSERVATIVE ((int64_t)2000 * 900)
static const dm_dev_profile_t dm_dev_profiles[] = {
    { "SM8550", "kalama", "Adreno 740", DM_DEV_BUDGET_CONSERVATIVE, 85, 0 },
    { "SM8750", "sun", "Adreno 830", DM_DEV_BUDGET_CONSERVATIVE, 85, 1 },
};
static inline int dm_dev_ieq(const char *a, const char *b) {
    if (!a || !b || !*a) return 0;
    for (; *a && *b; a++, b++) { char x = *a, y = *b; if (x >= 'a' && x <= 'z') x -= 32; if (y >= 'a' && y <= 'z') y -= 32; if (x != y) return 0; }
    return *a == *b;
}
static inline const dm_dev_profile_t *dm_dev_profile_find(const char *soc, const char *platform) {
    int n = (int)(sizeof dm_dev_profiles / sizeof dm_dev_profiles[0]);
    for (int i = 0; i < n; i++) if (dm_dev_ieq(soc, dm_dev_profiles[i].soc)) return &dm_dev_profiles[i];
    for (int i = 0; i < n; i++) if (dm_dev_ieq(platform, dm_dev_profiles[i].platform)) return &dm_dev_profiles[i];
    return 0;
}

static inline int dm_dev_default_pct(int64_t budget_px, int W, int H) {
    int best = 0, aw, ah;
    if (budget_px <= 0 || !dm_res_scale_size(DM_RES_AUTO, W, H, &aw, &ah)) return 0;
    for (int p = dm_res_scale_lo(W, H); p <= 100; p += 5) { int w, h; if (dm_res_scale_size(p, W, H, &w, &h) && h > ah && (int64_t)w * h <= budget_px) best = p; }
    return best;
}



static inline int dm_dev_off_applies(int file_exists, const char *off_product, const char *cur_product) {
    if (!file_exists) return 0;
    if (!cur_product || !*cur_product) return 1;
    return !off_product || !*off_product || !strcmp(off_product, cur_product);
}


enum { DM_RB_NONE, DM_RB_COUNT, DM_RB_FALLBACK };
static inline int dm_res_boot_step(int h, int n) { return h <= 900 ? DM_RB_NONE : n >= 2 ? DM_RB_FALLBACK : DM_RB_COUNT; }

static inline int dm_res_boot_step_x(int h, int n, int full) { return full ? (n >= 2 ? DM_RB_FALLBACK : DM_RB_COUNT) : dm_res_boot_step(h, n); }
static inline int dm_dev_default_pct_x(int64_t budget_px, int W, int H, int full) {
    if (!full) return dm_dev_default_pct(budget_px, W, H);
    int best = 0, aw, ah;
    if (budget_px <= 0 || !dm_res_scale_size_x(DM_RES_AUTO, W, H, 1, &aw, &ah)) return 0;
    for (int p = dm_res_scale_lo_x(W, H, 1); p <= 100; p += 5) { int w, h; if (dm_res_scale_size_x(p, W, H, 1, &w, &h) && h > ah && (int64_t)w * h <= budget_px) best = p; }
    return best;
}

static inline int dm_res_full_frontend_h(int w, int h) {
    if ((int64_t)w * 9 >= (int64_t)h * 16) return h;
    int u = (w * 9 / 16) & ~1;
    if (u < DM_RES_MIN_LINES) u = DM_RES_MIN_LINES < h ? DM_RES_MIN_LINES : h;
    return u > h ? h : u;
}

static inline int dm_res_fit_cap(int *w, int *h, int cw, int ch) {
    if (*w <= 0 || *h <= 0 || cw <= 0 || ch <= 0 || (*w <= cw && *h <= ch)) return 0;
    int64_t a = *w, b = *h;
    int nw = a * ch > b * cw ? cw : (int)(a * ch / b);
    nw &= ~7; if (nw < 8) nw = 8;
    int nh = (int)((int64_t)nw * b / a) & ~1; if (nh > ch) nh = ch & ~1; if (nh < 2) nh = 2;
    *w = nw; *h = nh; return 1;
}





static inline int dm_res_fold_plan(int W, int H, int eff, int full_ok, int was, int waiting, int live, int capw, int caph, int *tw, int *th, int *now, int *wait, int *fit) {
    if (W < H) { int t = W; W = H; H = t; }
    *now = full_ok != 0; *wait = 0; *fit = 0; *tw = *th = 0;
    if (!was && !*now && !waiting) return 0;
    if (!dm_res_scale_size_x(dm_res_pct_clamp_x(eff, W, H, *now), W, H, *now, tw, th)) return 0;
    *fit = live && dm_res_fit_cap(tw, th, capw, caph);
    if (*now && *fit && *tw < DM_RES_FULL_UI_W) {
        *wait = 1; *now = 0; dm_res_scale_size_x(dm_res_pct_clamp_x(eff, W, H, 0), W, H, 0, tw, th); *fit = dm_res_fit_cap(tw, th, capw, caph); }
    return 1;
}
