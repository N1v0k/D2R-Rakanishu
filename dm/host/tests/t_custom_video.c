

















#define DM_CUSTOM_TEST_PLATFORM 1
#define DM_CUSTOM_SET_TEXT test_set_text
#define DM_CUSTOM_SLIDER_SET test_slider_set
#define DM_CUSTOM_SLIDER_ENABLE test_slider_enable
#define DM_CUSTOM_NOW_MS test_now
#include "t_custom_carousel_fake.h"
#include "../game_addrs.h"
#include <stdint.h>
static void test_set_text(uint64_t w, const char *s);
static void test_slider_set(uint64_t w, int v);
static void test_slider_enable(uint64_t w, int on);
static uint64_t test_now(void);
#include "../dm_custom.c"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

module_t g_mod[MAX_MOD];
int g_nmod;
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)


static uint32_t generation = 1; static uint64_t frame, now_t; static int pad, busy, reloading; static uint64_t key_ev, key_held; static int sent_check;
uint32_t dm_ui_gen(void) { return generation; }
uint64_t dm_guest_frame(void) { return frame; }
static uint64_t test_now(void) { return now_t; }
int dm_padmode_current(void) { return pad; }
int dm_padmode_busy(void) { return busy; }
int dm_ui_reloading(void) { return reloading; }
uint64_t dm_padmode_page_keys(uint64_t want) { uint64_t e = key_ev & want; key_ev = 0; return e; }
uint64_t dm_padmode_page_held(void) { return key_held; }
void dm_ui_wheel_on(uint64_t sv, int dir) { (void)sv; (void)dir; }
const char *dm_save_commitdir(void) { return NULL; }
void dm_cam_set_mode(int m) { (void)m; }
static int q3d_mode; void dm_3d_quality_mode_set(int m) { q3d_mode = m; }
int dm_3d_quality_mode_get(void) { return q3d_mode; }
float dm_3d_scale_cur(void) { return 0.85f; }
static int s3d_menu = -1, s3d_pin; void dm_3d_scale_menu_set(int pct) { s3d_menu = pct; }
static int dev_pct, dev_s3d;
int dm_res_dev_pct(void) { return dev_pct; }
static int test_full; int dm_res_full(void) { return test_full; }
int dm_3d_scale_default_pct(void) { return dev_s3d ? dev_s3d : 85; }
int dm_3d_scale_pinned(void) { return s3d_pin; }
float dm_3d_scale_cfg(void) { return s3d_pin ? 0.9f : (s3d_menu > 0 ? s3d_menu : dm_3d_scale_default_pct()) / 100.0f; }
static double refresh_hz = 60; double mtl_display_refresh(void) { return refresh_hz; }
uint64_t dm_game_res(int docked) { (void)docked; return 900ull << 32 | 1992; }
int dm_lang_group(void) { return DM_LG_DE; }
int dm_ui_top_chk_on(void) { return 0; }
void dm_ui_top_chk(uint64_t root, const char *name, uint64_t got) { (void)root; (void)name; (void)got; }
void dm_ui_show_ours(uint64_t w, int on) { *(uint8_t *)(uintptr_t)(w + 0x50) = on != 0; *(uint8_t *)(uintptr_t)(w + 0x51) = on != 0; }
void dm_ui_move_ours(uint64_t w, int x) { (void)w; (void)x; }
void dm_ui_send_now(const char *cat, const char *act, const char *arg) { (void)cat; (void)arg; sent_check += !strcmp(act, "CheckChanges"); }
const char *dm_str(uint8_t *b, const char *k) { (void)b;
    static const char *const f[][2] = { { "dmVideoS3dLive", "%d x %d x%d.%02d" }, { "dmVideoS3dLiveNext", "after %d x %d x%d.%02d" }, { "dmVideoResRestart", "%d x %d -> %d x %d" },
        { "dmVideoResPinned", "pinned %d x %d" }, { "dmVideoS3dPinned", "%d %%" }, { "dmVideoS3dAuto", "%d %%" }, { "dmVideoFpsStd", "%d FPS · Standard" }, { "dmVideoFpsMaxHz", "Max · %d FPS" },
        { "dmVideoFpsValue", "%d FPS" }, { "dmVideoResStd", "Standard · %d x %d" }, { "dmVideoResValue", "%d %% · %d x %d" }, { "dmVideoS3dValue", "%d %%" }, { "dmVideoS3dStd", "%d %% · Standard" },
        { "dmVideoFpsEven", "screen %d Hz even" }, { "dmVideoFpsUneven", "screen %d Hz %d FPS uneven" }, { "dmVideoResLive", "switches on apply" }, { "dmVideoResBroken", "broken" },
        { "dmVideoS3qSaveHint", "save %d x %d %d" }, { "dmVideoS3qBalHint", "balanced %d x %d %d" }, { "dmVideoS3qSharpHint", "sharp %d x %d %d" },
        { "dmVideoS3qOwnHint", "custom %d x %d %d" } };
    for (unsigned i = 0; i < sizeof f / sizeof *f; i++) if (!strcmp(k, f[i][0])) return f[i][1]; return k; }

static int caps = 7, applied_fps = -2, res_chosen = -9, scr_w = 2688, scr_h = 1216, run_w = 1992, run_h = 900, res_pin, res_fb, vote_policy, nmodes = 1, modes[8] = { 60 };
static int live_w, live_h, live_broken;
int dm_video_caps(void) { return caps; }
int dm_extras_caps(void) { return 0; }
const char *dm_data_source(void) { return NULL; }
void dm_fps_apply(int fps) { applied_fps = fps; }
int dm_res_screen(int *W, int *H) { *W = scr_w; *H = scr_h; return scr_w > 0; }
int dm_res_running(int *w, int *h) { *w = run_w; *h = run_h; return run_w > 0; }
int dm_res_live_limit(int *w, int *h) { *w = live_w; *h = live_h; return live_w > 0; }
int dm_res_live_broken(void) { return live_broken; }
int dm_res_fallback(void) { return res_fb; }
int dm_res_pinned(void) { return res_pin; }
int dm_res_chosen(int pct) { res_chosen = pct; int w, h; dm_res_scale_size(dm_res_pct_clamp(pct == DM_RES_AUTO && dev_pct ? dev_pct : pct, scr_w, scr_h), scr_w, scr_h, &w, &h);
    if (live_w > 0 && w <= live_w && h <= live_h) { run_w = w; run_h = h; return 1; } return 0; }
int dm_display_max_hz(void) { int m = 0; for (int i = 0; i < nmodes; i++) if (modes[i] > m) m = modes[i]; return m; }
int dm_display_modes(int *hz, int cap) { int n = nmodes < cap ? nmodes : cap; memcpy(hz, modes, sizeof *hz * n); return n; }
int dm_fps_vote_policy(void) { return vote_policy; }

static int driver_opens, driver_own;
void dm_driver_open(void) { driver_opens++; }
int dm_driver_label(char *buf, int cap) { if (!driver_own) return 0; snprintf(buf, cap, "Turnip V34"); return 1; }
static void panel(int n, const int *m) { nmodes = n; memcpy(modes, m, sizeof *m * n); }



static uint8_t *main_mem;
typedef struct { uint8_t b[0xe00]; } widget;
static uint64_t page_vt[4], sel_vt[4], cont_vt[4];
static int slider_init_calls, slider_sets;
static void relayout_noop(uint64_t w) { (void)w; }
static void slider_init(uint64_t w) { (void)w; slider_init_calls++; }
static const char *const k_ext[] = { "TouchCustom_lang_zh", "TouchCustom_lang_en", "TouchCustom_lang_es", "TouchCustom_lang_ru", "TouchCustom_lang_ko", "TouchCustom_lang_de", "TouchCustom_lang_Text",
    "TouchCustom_auto_mana", "TouchCustom_auto_mana_Text", "TouchCustom_merc_pot", "TouchCustom_merc_pot_Text", "TouchCustom_cam", "TouchCustom_cam_ots", "TouchCustom_cam_fp", "TouchCustom_cam_Text" };

static const char *const k_rowkey[] = { "fps", "status", "res_pct", "s3d_preset", "s3d", "q3d", "bright", "driver" };
static const int k_rowh[] = { 356, 300, 356, 292, 420, 292, 292, 292 };
#define NVID 64
static const char *k_vid[NVID]; static char k_vid_buf[NVID][48]; static int k_vid_x[NVID], k_vid_y[NVID], k_vid_w[NVID], k_vid_h[NVID], nvid;
#define NEXT (int)(sizeof k_ext / sizeof *k_ext)
static widget root_w, settings, tabbar, page, ext_c, vid_c, sel_frame, ext_w[NEXT], vid_w[NVID];
static uint64_t root_kids[2], set_kids[1], page_kids[3], ext_kids[NEXT], vid_kids[NVID];
static char texts[NVID][128];
static uint64_t A(const void *p) { return (uint64_t)(uintptr_t)p; }
static void put64(widget *w, size_t at, uint64_t v) { memcpy(w->b + at, &v, 8); }
static void put32(widget *w, size_t at, int32_t v) { memcpy(w->b + at, &v, 4); }
static int32_t get32(widget *w, size_t at) { int32_t v; memcpy(&v, w->b + at, 4); return v; }
static uint64_t root(void) { return A(&root_w); }
static void test_set_text(uint64_t w, const char *s) { for (int i = 0; i < nvid; i++) if (w == A(&vid_w[i])) strlcpy(texts[i], s, sizeof texts[i]); }
static int vid_index(const char *name) { for (int i = 0; i < nvid; i++) if (!strcmp(k_vid[i], name)) return i; printf("no widget %s\n", name); abort(); }
static const char *text_of(const char *name) { return texts[vid_index(name)]; }

static void test_slider_set(uint64_t w, int v) { int32_t *f = (int32_t *)(uintptr_t)(w + 0x5bc); if (v < f[0]) v = f[0]; if (v > f[1]) v = f[1]; f[2] = v; slider_sets++; }
static void test_slider_enable(uint64_t w, int on) { *(uint8_t *)(uintptr_t)(w + 0x5b4) = on != 0; }
static void vid_add(const char *name, int x, int y, int w, int h) { k_vid[nvid] = name; k_vid_x[nvid] = x; k_vid_y[nvid] = y; k_vid_w[nvid] = w; k_vid_h[nvid] = h; nvid++; }
static const char *nm(const char *fmt, const char *key) { static int k; char *b = k_vid_buf[k++ % NVID]; snprintf(b, 48, fmt, key); return b; }
static void vid_layout(void) {
    nvid = 0; int y = 180;
    for (unsigned r = 0; r < sizeof k_rowkey / sizeof *k_rowkey; r++) { const char *key = k_rowkey[r]; int h = k_rowh[r], oy = y + 100, ny = oy + 112;
        vid_add(nm("TouchCustom_%s_Text", key), 0, y, 1385, h - 16); vid_add(nm("TouchVideo_%s_Label", key), 0, y, 1385, 100);
        if (!strcmp(key, "status")) { static const char *const c[] = { "clock", "battery", "fps" };
            for (int k = 0; k < 3; k++) { char t[48]; snprintf(t, sizeof t, "TouchCustom_status_%s", c[k]); vid_add(nm("%s", t), 400 * k, oy + 21, 70, 70); vid_add(nm("%s_Label", t), 400 * k + 80, oy, 300, 112); } }
        else if (!strcmp(key, "s3d_preset")) vid_add("TouchCustom_s3d_preset", 789, oy + 20, 596, 72);
        else if (!strcmp(key, "driver")) { vid_add(nm("TouchCustom_%s_Value", key), 0, oy, 1016, 112); vid_add("TouchCustomDriverOpen", 1056, oy + 14, 329, 83); }
        else { vid_add(nm("TouchCustom_%s_Value", key), 0, oy, 766, 112); vid_add(nm("TouchCustom_%s_Slider", key), 855, oy + 20, 480, 72); }
        vid_add(nm("TouchVideo_%s_Hint", key), 0, ny, 1385, 64);
        if (!strcmp(key, "fps") || !strcmp(key, "res_pct") || !strcmp(key, "s3d")) vid_add(nm("TouchVideo_%s_Note", key), 0, ny + 64, 1385, 64);
        if (!strcmp(key, "fps") || !strcmp(key, "s3d")) vid_add(nm("TouchVideo_%s_Warn", key), 0, ny + (strcmp(key, "fps") ? 128 : 64), 1385, 64);
        y += h; }
}
static int ends(const char *n, const char *s) { size_t a = strlen(n), b = strlen(s); return a >= b && !strcmp(n + a - b, s); }
static void leaf(widget *w, const char *name, int x, int y, int wd, int ht) {
    int slider = ends(name, "_Slider"), text = !slider && (ends(name, "_Text") || ends(name, "_Label") || ends(name, "_Hint") || ends(name, "_Note") || ends(name, "_Warn") || ends(name, "_Value"));
    memset(w, 0, sizeof *w); put64(w, 0, A(main_mem) + (slider ? DM_VT_SLIDER : text ? DM_VT_TEXT : DM_VT_TOGGLE)); put64(w, 8, A(name)); w->b[0x50] = w->b[0x51] = 1;
    put32(w, 0x70, x); put32(w, 0x74, y); put32(w, 0x78, wd); put32(w, 0x7c, ht);
    static const int32_t fr[4] = { 0, 1, 3, 2 }; if (!text && !slider) memcpy(w->b + 0x6e8, fr, sizeof fr);
    if (!strcmp(name, "TouchCustom_s3d_preset")) { car_fake(w->b, A(main_mem) + DM_VT_CAROUSEL, 4, 0, 0); put64(w, 8, A(name)); }
    if (slider) { put64(w, 0x580, 1); w->b[0x5b4] = 1; put32(w, 0x5b8, INT_MAX); put32(w, 0x5bc, 0); put32(w, 0x5c0, INT_MAX); put32(w, 0x5c4, 0); }
}
static void build(int sel_tab) {
    memset(&root_w, 0, sizeof root_w); root_w.b[0x50] = 1; put64(&root_w, 0x58, A(root_kids)); put64(&root_w, 0x60, 2);
    memset(&settings, 0, sizeof settings); put64(&settings, 8, A("SettingsPanel")); settings.b[0x50] = 1; put64(&settings, 0x58, A(set_kids)); put64(&settings, 0x60, 1);
    memset(&tabbar, 0, sizeof tabbar); put64(&tabbar, 0, A(main_mem) + DM_VT_TABBAR); put64(&tabbar, 8, A("OptionsTabs")); tabbar.b[0x50] = 1; put32(&tabbar, 0x558, 5); put32(&tabbar, 0xdc4, sel_tab);
    put64(&tabbar, 0xb50 + 32 * 3 + 0x10, A("TouchTabExtras")); tabbar.b[0xb50 + 32 * 3 + 0x18] = 1; put64(&tabbar, 0xb50 + 32 * 4 + 0x10, A("TouchTabVideo")); tabbar.b[0xb50 + 32 * 4 + 0x18] = 1;
    set_kids[0] = A(&tabbar);
    memset(&page, 0, sizeof page); put64(&page, 0, A(page_vt)); put64(&page, 8, A("VideoOptionsPanel")); page.b[0x50] = 1; put64(&page, 0x58, A(page_kids)); put64(&page, 0x60, 3);
    memset(&ext_c, 0, sizeof ext_c); put64(&ext_c, 0, A(cont_vt)); put64(&ext_c, 8, A("TouchCustomRows")); ext_c.b[0x50] = 1; put32(&ext_c, 0x7c, 3000); put64(&ext_c, 0x58, A(ext_kids)); put64(&ext_c, 0x60, NEXT);
    memset(&vid_c, 0, sizeof vid_c); put64(&vid_c, 0, A(cont_vt)); put64(&vid_c, 8, A("TouchVideoRows")); vid_c.b[0x50] = 1; put32(&vid_c, 0x70, 99999); put32(&vid_c, 0x7c, 2488);
    memset(&sel_frame, 0, sizeof sel_frame); put64(&sel_frame, 0, A(sel_vt)); put64(&sel_frame, 8, A("TouchCustomSel")); sel_frame.b[0x50] = 1; put32(&sel_frame, 0x70, 99999); put32(&sel_frame, 0x78, 1000); put32(&sel_frame, 0x7c, 100);
    page_kids[0] = A(&ext_c); page_kids[1] = A(&vid_c); page_kids[2] = A(&sel_frame);
    for (int i = 0; i < NEXT; i++) { leaf(&ext_w[i], k_ext[i], 1315, 300 + 100 * i, 70, 70); ext_kids[i] = A(&ext_w[i]); }
    for (int i = 0; i < nvid; i++) { leaf(&vid_w[i], k_vid[i], k_vid_x[i], k_vid_y[i], k_vid_w[i], k_vid_h[i]); vid_kids[i] = A(&vid_w[i]); }
    put64(&vid_c, 0x58, A(vid_kids)); put64(&vid_c, 0x60, nvid);
    root_kids[0] = A(&settings); root_kids[1] = A(&page);
}
static widget *wv(const char *name) { for (int i = 0; i < nvid; i++) if (!strcmp(k_vid[i], name)) return &vid_w[i]; for (int i = 0; i < NEXT; i++) if (!strcmp(k_ext[i], name)) return &ext_w[i]; printf("no widget %s\n", name); abort(); }
static int on(const char *name) { return wv(name)->b[0xa88] != 0; }
static void click(const char *name) { widget *w = wv(name); w->b[0xa88] = !w->b[0xa88]; }
static int pend(const char *key) { return atomic_load(&g_pend[key_index(key)]); }
static int val(const char *key) { return atomic_load(&g_val[key_index(key)]); }
static void tab(int k) { put32(&tabbar, 0xdc4, k); }
static int dirty(void) { return page.b[0x1a0]; }
static int shown(widget *c) { return c->b[0x50] && get32(c, 0x70) == 0; }
static int hidden(widget *c) { return !c->b[0x50] && get32(c, 0x70) == 99999 && get32(c, 0x7c) == 1; }

static widget *sl(const char *key) { char n[48]; snprintf(n, sizeof n, "TouchCustom_%s_Slider", key); return wv(n); }
static int sl_max(const char *key) { return get32(sl(key), 0x5c0); }
static int sl_at(const char *key) { return get32(sl(key), 0x5c4); }
static int sl_on(const char *key) { return sl(key)->b[0x5b4]; }
static void drag(const char *key, int k) { put32(sl(key), 0x5c4, k); }
static const char *value_of(const char *key) { char n[48]; snprintf(n, sizeof n, "TouchCustom_%s_Value", key); return text_of(n); }
uint64_t dm_wref_get(dm_wref *r) { return r->w && r->gen == generation ? r->w : 0; }
static void step(void) { frame++; now_t += 16; dm_custom_ui_pump(root()); }
static void steps(int n) { while (n--) step(); }
static int row_y(const char *key) { int y = 180; for (unsigned r = 0; r < sizeof k_rowkey / sizeof *k_rowkey; r++) { if (!strcmp(k_rowkey[r], key)) return y; y += k_rowh[r]; } return -1; }
static int sel_row_is(const char *key) { return get32(&sel_frame, 0x74) == row_y(key) + (strcmp(key, "status") ? 0 : 100) - 10; }

static int file_has(const char *dir, const char *line) { char p[1100], ln[128]; snprintf(p, sizeof p, "%s/sw_custom.txt", dir); FILE *f = fopen(p, "r"); int got = 0; while (f && fgets(ln, sizeof ln, f)) got |= !strncmp(ln, line, strlen(line)) && ln[strlen(line)] == '\n'; if (f) fclose(f); return got; }
static int file_key(const char *dir, const char *key) { char p[1100], ln[128]; snprintf(p, sizeof p, "%s/sw_custom.txt", dir); FILE *f = fopen(p, "r"); int got = 0; size_t n = strlen(key); while (f && fgets(ln, sizeof ln, f)) got |= !strncmp(ln, key, n) && ln[n] == '='; if (f) fclose(f); return got; }
static void file_put(const char *dir, const char *body) { char p[1100]; snprintf(p, sizeof p, "%s/sw_custom.txt", dir); if (!body) { unlink(p); return; } FILE *f = fopen(p, "w"); fputs(body, f); fclose(f); }


static int migrate_on(const char *dir, const char *body, int legacy_fps, int legacy_status, int W, int H, int mx) {
    file_put(dir, body); fflush(stdout); pid_t p = fork(); if (p < 0) return -1;
    if (!p) { scr_w = W; scr_h = H; if (mx > 0) { int m[1] = { mx }; panel(1, m); } else nmodes = 0;
        dm_custom_init(dir); dm_custom_legacy(legacy_fps, legacy_status); int f = dm_custom_get("fps"), st = dm_custom_get("status"); _exit((f == FPS_AUTO ? 200 : f) + st); }
    int s = 0; waitpid(p, &s, 0); return WIFEXITED(s) ? WEXITSTATUS(s) : -1; }
static int migrate(const char *dir, const char *body, int legacy_fps, int legacy_status) { return migrate_on(dir, body, legacy_fps, legacy_status, 2688, 1216, 60); }

static int child_fps(const char *dir, int mx, double cur) { fflush(stdout); pid_t p = fork(); if (p < 0) return -1;
    if (!p) { int m[1] = { mx }; panel(1, m); refresh_hz = cur; dm_custom_init(dir); _exit(dm_fps_setting()); }
    int s = 0; waitpid(p, &s, 0); return WIFEXITED(s) ? WEXITSTATUS(s) : -1; }

int main(void) {
    const char *td = getenv("TMPDIR"); char tmpl[1024]; snprintf(tmpl, sizeof tmpl, "%s/t_custom_video.XXXXXX", td && *td ? td : "/tmp");
    char *d = mkdtemp(tmpl); if (!d) { perror("mkdtemp"); return 1; }
    main_mem = calloc(1, DM_VT_SLIDER + 0x100); if (!main_mem) return 1;
    { uint64_t f = A(relayout_noop), g = A(slider_init); memcpy(main_mem + DM_VT_TEXT + 0x10, &f, 8); memcpy(main_mem + DM_VT_SLIDER + 0x10, &g, 8); }
    strlcpy(g_mod[0].name, "main", sizeof g_mod[0].name); g_mod[0].base = main_mem; g_nmod = 1;
    page_vt[0] = A(main_mem) + GA_OPTIONS_PAGE_UPDATE; sel_vt[2] = A(relayout_noop); cont_vt[2] = A(relayout_noop);
    vid_layout();


    CHECK(migrate(d, NULL, -1, -1) == 207 && !file_key(d, "fps"), "没有文件、没有旧偏好：fps = 按屏幕（auto），不存盘");
    CHECK(migrate(d, "eco_fps=1\n", -1, -1) == 30 + 7 && file_has(d, "fps=30") && file_has(d, "eco_fps=0"), "eco_fps=1 → fps=30，当场存盘，eco_fps 退役写 0");
    CHECK(migrate(d, "eco_fps=1\n", 120, -1) == 30 + 7, "省电模式与 App 菜单都有：省电模式优先（以前它压过菜单）");
    CHECK(migrate_on(d, "auto_mana=1\n", 90, -1, 2688, 1216, 120) == 90 + 7 && file_has(d, "fps=90"), "App 菜单 fps_cap=90、120 Hz 屏 → fps=90");
    CHECK(migrate(d, "auto_mana=1\n", 90, -1) == 0 + 7 && file_has(d, "fps=0"), "App 菜单 fps_cap=90、60 Hz 屏（模拟器）→ Max（同样的效果）");
    CHECK(migrate_on(d, "auto_mana=1\n", 72, -1, 2688, 1216, 144) == 70 + 7 && file_has(d, "fps=70"), "App 菜单旧档 fps_cap=72、144 Hz 屏 → 70（不丢成 auto）");
    CHECK(migrate(d, "auto_mana=1\n", 72, -1) == 0 + 7 && file_has(d, "fps=0"), "App 菜单旧档 fps_cap=72、60 Hz 屏 → Max");
    CHECK(migrate_on(d, "fps=72\n", -1, -1, 2688, 1216, 144) == 70 + 7 && file_has(d, "fps=70"), "fps=72、144 Hz 屏 → 70（最近的格）");
    CHECK(migrate_on(d, "fps=72\n", -1, -1, 2480, 1116, 120) == 70 + 7 && file_has(d, "fps=70"), "fps=72、120 Hz 屏 → 70");
    CHECK(migrate(d, "fps=72\n", -1, -1) == 0 + 7 && file_has(d, "fps=0"), "fps=72、60 Hz 屏 → Max（先判「到了屏最高刷新率」）");
    CHECK(migrate_on(d, "fps=120\n", -1, -1, 2480, 1116, 120) == 0 + 7 && file_has(d, "fps=0"), "fps=120、120 Hz 屏 → Max");
    CHECK(migrate_on(d, "fps=72\n", -1, -1, 2688, 1216, 0) == 72 + 7 && file_has(d, "fps=72"), "刷新率不知道：fps=72 不迁移、不存盘");
    CHECK(migrate(d, "fps=45\nstatus=3\n", 120, 0) == 45 + 3, "新键已在：旧来源一律不看，45 在格上不动");
    CHECK(migrate(d, "fps=999\nstatus=9\nres=7\ns3d=40\nres_pct=33\n", -1, -1) == 207, "坏值当没写（fps 回到 auto，status 回到 7）");
    CHECK(migrate(d, NULL, 0, 5) == 0 + 5 && file_has(d, "fps=0") && file_has(d, "status=5"), "App 菜单「不限」fps_cap=0 → fps=0；状态行旧偏好 5（时钟 + 帧率）");
    CHECK(migrate(d, NULL, -1, 0) == 200 && file_has(d, "status=0") && file_has(d, "fps=auto") && file_has(d, "res_pct=auto"), "状态行总开关关着 → status=0；fps 与 res_pct 存回 auto");
    CHECK(migrate(d, NULL, 33, -1) == 207, "不认识的旧帧率（33）不迁移");

    CHECK(migrate(d, "res=1\n", -1, -1) >= 0 && file_has(d, "res_pct=90") && file_has(d, "res=1"), "res=1、1216 行屏 → res_pct=90（离 1080 行最近），旧档照写 1");
    CHECK(migrate_on(d, "res=1\n", -1, -1, 2480, 1116, 120) >= 0 && file_has(d, "res_pct=95"), "res=1、1116 行屏 → res_pct=95");
    CHECK(migrate_on(d, "res=1\n", -1, -1, 2400, 1080, 60) >= 0 && file_has(d, "res_pct=auto") && file_has(d, "res=0"), "res=1、1080 行屏（2.2.8 在这里跑 900p）→ auto");
    CHECK(migrate(d, "res=2\n", -1, -1) >= 0 && file_has(d, "res_pct=100") && file_has(d, "res=2"), "res=2、1216 行屏 → 100 %%");
    CHECK(migrate_on(d, "res=2\n", -1, -1, 1600, 720, 60) >= 0 && file_has(d, "res_pct=auto"), "res=2、720 行屏（2.2.8 在这里跑 900p）→ auto");
    CHECK(migrate(d, "res=0\n", -1, -1) >= 0 && file_has(d, "res_pct=auto") && file_has(d, "res=0"), "res=0 → auto");
    CHECK(migrate(d, "res_pct=80\nres=2\nfps=45\n", -1, -1) >= 0 && file_has(d, "res_pct=80") && file_has(d, "res=2"), "res_pct 已在：旧档 res 不看（没迁移就不存盘，原样）");
    CHECK(migrate_on(d, "res=1\neco_fps=1\n", -1, -1, 0, 0, 60) >= 0 && file_has(d, "res=1") && !file_key(d, "res_pct") && file_has(d, "fps=30"), "屏幕尺寸不知道：旧档 res=1 原样写回、不写 res_pct（下次再迁）");
    CHECK(migrate(d, "res_pct=90\neco_fps=1\n", -1, -1) >= 0 && file_has(d, "res=1"), "退回旧版用的旧档：90 %%（1094 行）→ res=1");
    CHECK(migrate(d, "res_pct=100\neco_fps=1\n", -1, -1) >= 0 && file_has(d, "res=2"), "100 %% → res=2");
    CHECK(migrate(d, "res_pct=80\neco_fps=1\n", -1, -1) >= 0 && file_has(d, "res=0"), "80 %%（972 行）→ res=0");
    file_put(d, NULL);


    CHECK(child_fps(d, 144, 144) == 72 && child_fps(d, 144, 60) == 60, "按屏幕：此刻 144 Hz → 72，此刻 60 Hz（屏最高 144）→ 60");
    { fflush(stdout); pid_t p = fork(); if (!p) { caps = 0; refresh_hz = 144; dm_custom_init(d); _exit(dm_fps_setting()); } int st = 0; waitpid(p, &st, 0); CHECK(WEXITSTATUS(st) == 60, "只有 30 / 60 的平台：按屏幕 60"); }


    dm_custom_init(d); build(3); steps(3);
    CHECK(g_has_video && g_view == 0 && shown(&ext_c) && hidden(&vid_c), "Extras：视图 0、Extras 容器显示、Video 容器藏起（%d %d）", g_has_video, g_view);
    CHECK(applied_fps == 60, "读盘时生效帧率交给平台（60 Hz 屏 auto = 60，实际 %d）", applied_fps);
    click("TouchCustom_auto_mana"); step();
    CHECK(pend("auto_mana") == 0 && dirty(), "Extras 里点掉 auto_mana = 待定改动，「应用」点亮");


    tab(4); steps(2);
    CHECK(g_view == 1 && hidden(&ext_c) && shown(&vid_c) && sent_check == 0, "Video：容器换了，没有替页签发 CheckChanges（%d）", sent_check);
    CHECK(pend("auto_mana") == 0, "换视图不丢 Extras 的待定改动");
    CHECK(sl_max("fps") == 6 && sl_at("fps") == 6 && !strcmp(value_of("fps"), "60 FPS · Standard"), "帧率：60 Hz 屏 7 格（30 … 55、Max），auto = 60 停在 Max，「%s」", value_of("fps"));
    CHECK(sl_max("res_pct") == 8 && sl_at("res_pct") == 3 && !strcmp(value_of("res_pct"), "Standard · 1992 x 900"), "输出分辨率：60–100 %% 9 格，缺省停在 75 %%，「%s」", value_of("res_pct"));
    CHECK(sl_max("s3d") == 8 && sl_at("s3d") == 5 && !strcmp(value_of("s3d"), "85 % · Standard"), "3D sharpness: 60-100 in 9 notches, the platform default 85 sits on 85, \"%s\"", value_of("s3d"));
    CHECK(sl_max("q3d") == 1 && sl_at("q3d") == 0 && !strcmp(value_of("q3d"), "dmCustomQ3dSmooth") && !strcmp(text_of("TouchVideo_q3d_Hint"), "dmCustomQ3dSmoothHint"), "3D 画质：两格，流畅优先与它的说明");
    CHECK(sl_max("bright") == 2 && sl_at("bright") == 2 && !strcmp(value_of("bright"), "dmVideoBrightFull"), "暗处加亮：三格，缺省第三格");
    CHECK(sl_on("fps") && sl_on("res_pct") && sl_on("s3d") && pend("fps") < 0 && pend("res_pct") < 0 && pend("s3d") < 0, "滑块都启用；Video 没有待定改动");
    CHECK(!strcmp(text_of("TouchVideo_fps_Note"), "screen 60 Hz even") && !text_of("TouchVideo_fps_Warn")[0], "帧率说明：屏幕 60 Hz、整除（「%s」）", text_of("TouchVideo_fps_Note"));
    CHECK(!text_of("TouchVideo_res_pct_Note")[0], "选的就是这次启动的尺寸：没有重启提示（「%s」）", text_of("TouchVideo_res_pct_Note"));
    CHECK(!strcmp(text_of("TouchVideo_s3d_Hint"), "1693 x 765 x1.00"), "3D 画面说明：1992 × 900 × 85 %% = 1693 × 765、×1.00（「%s」）", text_of("TouchVideo_s3d_Hint"));
    CHECK(on("TouchCustom_status_clock") && on("TouchCustom_status_battery") && on("TouchCustom_status_fps"), "状态行三格缺省都勾（仍是勾选框）");
    { int n0 = slider_sets; steps(3); CHECK(slider_sets == n0, "摆好以后每帧不再调 SetValue（%d 次）", slider_sets - n0); }


    drag("res_pct", 4); step();
    CHECK(pend("res_pct") == 80 && !strcmp(value_of("res_pct"), "80 % · 2152 x 972") && dirty(), "拖到 80 %%：待定 80（%d），「%s」，「应用」亮", pend("res_pct"), value_of("res_pct"));
    CHECK(!strcmp(text_of("TouchVideo_res_pct_Note"), "1992 x 900 -> 2152 x 972"), "重启提示写两个尺寸（「%s」）", text_of("TouchVideo_res_pct_Note"));
    CHECK(!strcmp(text_of("TouchVideo_s3d_Hint"), "after 1829 x 826 x1.17"), "3D 说明按选的输出分辨率、说的是重启以后（「%s」）", text_of("TouchVideo_s3d_Hint"));
    drag("res_pct", 3); step();
    CHECK(pend("res_pct") == 75 && !strcmp(value_of("res_pct"), "75 % · 2016 x 912") && dirty(), "挪回缺省所在的格：明确的 75 %%（%d，「%s」），「应用」仍亮", pend("res_pct"), value_of("res_pct"));
    { int n0 = slider_sets; step(); CHECK(pend("res_pct") == 75 && slider_sets == n0, "没动就没有新的待定值，也不摆"); }

    scr_w = 2480; scr_h = 1116; run_w = 2000; run_h = 900; atomic_store(&g_pend[K_RESP], -1); atomic_store(&g_repaint, 1); step();
    CHECK(sl_max("res_pct") == 7 && sl_at("res_pct") == 3 && !strcmp(value_of("res_pct"), "Standard · 2000 x 900"), "2000×900 screen：65–100 %% 8 格，缺省停在 80 %%（%d，「%s」）", sl_at("res_pct"), value_of("res_pct"));
    drag("res_pct", 2); step(); drag("res_pct", 3); step();
    CHECK(pend("res_pct") == 80 && !strcmp(value_of("res_pct"), "80 % · 1984 x 892"), "2000×900 screen ◀ ▶ 回到缺省那格：明确的 80 %% · 1984 × 892（「%s」）", value_of("res_pct"));
    scr_w = 2688; scr_h = 1216; run_w = 1992; run_h = 900;


    drag("fps", 3); step();
    dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", ""); step();
    CHECK(pend("fps") < 0 && pend("res_pct") < 0 && pend("auto_mana") == 0, "默认：fps / res_pct 回 auto（存的就是 auto → 没有待定），Extras 的 auto_mana 待定仍是 0");
    CHECK(sl_at("res_pct") == 3 && !strcmp(value_of("res_pct"), "Standard · 1992 x 900") && sl_at("fps") == 6 && !strcmp(value_of("fps"), "60 FPS · Standard"), "默认后滑块回到缺省所在的格、字是「Standard」");


    drag("fps", 5); step(); drag("fps", 6); step();
    CHECK(pend("fps") == 0 && !strcmp(value_of("fps"), "Max · 60 FPS") && dirty(), "60 Hz 屏 ◀ ▶ 回到 Max：明确的 fps=0（%d，「%s」）", pend("fps"), value_of("fps"));
    drag("fps", 3); step();
    CHECK(pend("fps") == 45 && !strcmp(value_of("fps"), "45 FPS") && !strcmp(text_of("TouchVideo_fps_Warn"), "screen 60 Hz 45 FPS uneven") && !text_of("TouchVideo_fps_Note")[0],
          "45 FPS：待定、金黄字说明不整除（「%s」）", text_of("TouchVideo_fps_Warn"));

    { int m[3] = { 60, 90, 120 }; panel(3, m); step();
      CHECK(sl_max("fps") == 18 && !strcmp(text_of("TouchVideo_fps_Note"), "screen 90 Hz even"), "屏 60/90/120：19 格；45 待定 → 将投 90 Hz、整除（「%s」）", text_of("TouchVideo_fps_Note"));
      drag("fps", 18); step(); CHECK(pend("fps") == 0 && !strcmp(value_of("fps"), "Max · 120 FPS"), "最后一格 Max · 120 FPS（「%s」）", value_of("fps"));
      drag("fps", 9); step(); CHECK(pend("fps") == 75 && !strcmp(text_of("TouchVideo_fps_Warn"), "screen 120 Hz 75 FPS uneven"), "75 不整除 120：金黄字（「%s」）", text_of("TouchVideo_fps_Warn"));
      drag("fps", 6); step(); CHECK(pend("fps") == 60, "120 Hz 屏上 60 是一格：明确的 60（auto = 60 也算改动，按原值比）");
      drag("fps", 3); step(); }


    pad = 1; step();
    CHECK(sel_row_is("fps"), "手柄选中帧率行（选中框框住整行）");
    key_ev = DM_BTN_RIGHT; step();
    CHECK(pend("fps") == 50 && sl_at("fps") == 4, "▶：45 → 50（%d，滑块第 %d 格）", pend("fps"), sl_at("fps"));
    for (int k = 0; k < 6; k++) { key_ev = DM_BTN_LEFT; step(); }
    CHECK(pend("fps") == 30 && sl_at("fps") == 0, "◀ 到头停在 30（%d）", pend("fps"));
    key_ev = DM_BTN_A; step(); CHECK(pend("fps") == 30, "A 在滑块行不改值");
    key_ev = DM_BTN_RIGHT; key_held = DM_BTN_RIGHT; step();
    for (int k = 0; k < 20; k++) step();
    CHECK(pend("fps") == 35, "按下一格，400 ms 之前不连发（%d）", pend("fps"));
    for (int k = 0; k < 15; k++) step();
    CHECK(pend("fps") == 50, "按住：400 ms 起每 80 ms 一格（%d）", pend("fps"));
    key_held = 0; steps(5); CHECK(pend("fps") == 50, "松开就停");
    key_ev = DM_BTN_DOWN; step(); key_ev = DM_BTN_DOWN; step();
    CHECK(sel_row_is("res_pct"), "↓↓ 经状态行到输出分辨率");
    key_ev = DM_BTN_RIGHT; step(); key_ev = DM_BTN_RIGHT; step();
    CHECK(pend("res_pct") == 85 && !strcmp(value_of("res_pct"), "85 % · 2288 x 1034"), "输出分辨率 ▶▶：75 → 85 %%（「%s」）", value_of("res_pct"));

    s3d_pin = 1; key_ev = DM_BTN_DOWN; step(); step();
    CHECK(sel_row_is("q3d") && !sl_on("s3d") && !strcmp(value_of("s3d"), "90 %"), "3D 清晰度钉住：↓ 直接到 3D 画质，滑块禁用，「%s」", value_of("s3d"));
    key_ev = DM_BTN_RIGHT; step(); CHECK(pend("q3d") == 1 && !strcmp(value_of("q3d"), "dmCustomQ3dQuality"), "3D 画质 ▶：画质优先");
    key_ev = DM_BTN_RIGHT; step(); CHECK(pend("q3d") == 1, "两格：到头停");
    s3d_pin = 0; step(); CHECK(sl_on("s3d"), "不钉了：滑块启用");

    scr_w = 1600; scr_h = 720; step(); key_ev = DM_BTN_UP; step(); key_ev = DM_BTN_UP; step(); key_ev = DM_BTN_UP; step();
    CHECK(!sl_on("res_pct") && sl_max("res_pct") == 1 && sel_row_is("status"), "720-line screen: output resolution only 100 %%, disabled; up from the 3D quick choice skips it to the status row");
    scr_w = 2688; scr_h = 1216; step();

    { int pr0 = atomic_load(&g_pend[K_RESP]);
      test_full = 1; scr_w = 2048; scr_h = 1536; atomic_store(&g_repaint, 1); step(); drag("res_pct", 7); step();
      CHECK(sl_max("res_pct") == 7 && !strcmp(value_of("res_pct"), "100 % · 2048 x 1536"), "fullscreen 4:3: 8 notches, 100 %% = the panel (\"%s\")", value_of("res_pct"));
      drag("res_pct", 0); step(); CHECK(!strcmp(value_of("res_pct"), "65 % · 1328 x 998"), "fullscreen 4:3: lowest notch 65 %% (\"%s\")", value_of("res_pct"));
      test_full = 0; atomic_store(&g_repaint, 1); step(); CHECK(!strcmp(value_of("res_pct"), "65 % · 1336 x 748"), "without fullscreen: the 16:9 size again (\"%s\")", value_of("res_pct"));
      atomic_store(&g_pend[K_RESP], pr0); scr_w = 2688; scr_h = 1216; atomic_store(&g_repaint, 1); step(); }


    { int pf = pend("fps"), pr = pend("res_pct");
      busy = 1; step(); generation++; build(4); step(); step(); busy = 0; step();
      CHECK(g_view == 1 && pend("fps") == pf && pend("res_pct") == pr && pend("auto_mana") == 0, "切换后视图仍是 Video，待定改动都在（fps %d res_pct %d）", pend("fps"), pend("res_pct"));
      CHECK(sl_max("fps") == 18 && sl_at("fps") == 4 && sl_max("res_pct") == 8 && sl_at("res_pct") == 5, "重建的滑块：格数重写、停在待定值（fps 第 %d 格、res 第 %d 格）", sl_at("fps"), sl_at("res_pct"));
      steps(2); CHECK(sel_row_is("fps"), "切换后选中 Video 第一行"); }

    put32(sl("fps"), 0x5c0, INT_MAX); put32(sl("fps"), 0x5c4, 0); step();
    CHECK(sl_max("fps") == 18 && sl_at("fps") == 4 && pend("fps") == 50, "最大值被换成 INT_MAX：重写格数、值 0 不当玩家挪到了 30（%d）", pend("fps"));

    own_y(A(sl("fps")), 5); view_geom(A(sl("fps")), 1, 10);
    CHECK(slider_init_calls == 0 && get32(sl("fps"), 0x74) != 5, "own_y / view_geom 不碰滑块（初始化调了 %d 次）", slider_init_calls);


    drag("s3d", 6); step();
    pad = 0; step();
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
    CHECK(val("fps") == 50 && applied_fps == 50 && val("res_pct") == 85 && res_chosen == 85 && val("s3d") == 90 && s3d_menu == 90 && val("q3d") == 1 && q3d_mode == 1,
          "应用：fps 50（平台 %d）、res_pct 85（防崩 %d）、s3d 90、q3d 1", applied_fps, res_chosen);
    CHECK(file_has(d, "fps=50") && file_has(d, "res_pct=85") && file_has(d, "res=1") && file_has(d, "s3d=90") && file_has(d, "eco_fps=0") && !dirty(), "存盘：fps=50 res_pct=85 res=1（1034 行离 1080 近）s3d=90，「应用」熄");
    CHECK(!strcmp(value_of("res_pct"), "85 % · 2288 x 1034") && !strcmp(text_of("TouchVideo_res_pct_Note"), "1992 x 900 -> 2288 x 1034"), "应用后仍提示重启（这次启动还是 1992 × 900）");
    drag("fps", 4); step(); CHECK(pend("fps") < 0 && !dirty(), "挪到存着的那一格：没有改动");


    live_w = 1992; live_h = 900; step();
    CHECK(!strcmp(text_of("TouchVideo_res_pct_Note"), "1992 x 900 -> 2288 x 1034"), "存着的 85 %% 比上限大：仍提示重启（「%s」）", text_of("TouchVideo_res_pct_Note"));
    drag("res_pct", 2); step();
    CHECK(pend("res_pct") == 70 && !strcmp(text_of("TouchVideo_res_pct_Note"), "switches on apply") && !strcmp(text_of("TouchVideo_s3d_Hint"), "1692 x 767 x1.00"),
          "拖到 70 %%（1880 × 852）：「点应用就切」（「%s」），3D 说明不说「重启以后」（「%s」）", text_of("TouchVideo_res_pct_Note"), text_of("TouchVideo_s3d_Hint"));
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
    CHECK(val("res_pct") == 70 && res_chosen == 70 && run_w == 1880 && run_h == 852 && !text_of("TouchVideo_res_pct_Note")[0] && file_has(d, "res_pct=70"),
          "应用 70 %%：当场切成 %d × %d，没有提示（「%s」）", run_w, run_h, text_of("TouchVideo_res_pct_Note"));
    drag("res_pct", 8); step();
    CHECK(!strcmp(text_of("TouchVideo_res_pct_Note"), "1880 x 852 -> 2688 x 1216"), "100 %% 比上限大：重启提示写此刻与选的尺寸（「%s」）", text_of("TouchVideo_res_pct_Note"));
    live_broken = 1; step();
    CHECK(!strcmp(text_of("TouchVideo_res_pct_Note"), "broken"), "切换和退回都没核上：请重启（「%s」）", text_of("TouchVideo_res_pct_Note"));
    live_broken = 0; live_w = live_h = 0; drag("res_pct", 5); step(); dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step(); run_w = 1992; run_h = 900; step();
    CHECK(val("res_pct") == 85 && pend("res_pct") < 0 && !strcmp(text_of("TouchVideo_res_pct_Note"), "1992 x 900 -> 2288 x 1034"), "回到 85 %%、没有当场切换：同 8 的状态");


    drag("fps", 6); step(); CHECK(pend("fps") == 60, "又改了帧率（%d）", pend("fps"));
    settings.b[0x50] = 0; page.b[0x50] = 0; step();
    CHECK(pend("fps") == -1, "关掉选项：待定改动丢了（%d）", pend("fps"));


    settings.b[0x50] = 1; page.b[0x50] = 1; generation++; build(4); steps(3);
    caps = 2; atomic_store(&g_repaint, 1); step(); step();
    CHECK(sl_max("fps") == 1 && !sl(("res_pct"))->b[0x50] && !wv("TouchCustom_res_pct_Value")->b[0x50], "Metal：帧率两格，输出分辨率那一行藏起来");
    pad = 1; generation++; build(4); steps(3); key_ev = DM_BTN_DOWN; step(); key_ev = DM_BTN_DOWN; step();
    CHECK(sel_row_is("s3d_preset"), "controller down down from the status row skips the output resolution onto the 3D quick choice");
    caps = 7; pad = 0;






    generation++; build(4); steps(3);
    scr_w = 2480; scr_h = 1116; run_w = 2352; run_h = 1060; dev_pct = 95; dev_s3d = 90; atomic_store(&g_val[K_RESP], DM_RES_AUTO); atomic_store(&g_val[K_S3D], 0); atomic_store(&g_pend[K_RESP], -1); atomic_store(&g_pend[K_S3D], -1); atomic_store(&g_repaint, 1); steps(2);
    CHECK(val("res_pct") == DM_RES_AUTO && sl_at("res_pct") == 6 && !strcmp(value_of("res_pct"), "Standard · 2352 x 1060"), "设备缺省：存的仍是 auto，滑块停在 95 %%（第 %d 格），「%s」", sl_at("res_pct"), value_of("res_pct"));
    CHECK(!text_of("TouchVideo_res_pct_Note")[0], "这次启动就是设备缺省：没有重启提示（「%s」）", text_of("TouchVideo_res_pct_Note"));
    CHECK(sl_at("s3d") == 6 && !strcmp(value_of("s3d"), "90 % · Standard"), "3D device default 90 %%: on the 90 notch, \"%s\"", value_of("s3d"));
    CHECK(!strcmp(text_of("TouchVideo_s3d_Hint"), "2117 x 954 x1.00"), "3D 说明：2352 × 1060 × 90 %% = 2117 × 954、对设备缺省 ×1.00（「%s」）", text_of("TouchVideo_s3d_Hint"));
    drag("res_pct", 7); step();
    CHECK(pend("res_pct") == 100 && !strcmp(value_of("res_pct"), "100 % · 2480 x 1116") && !strcmp(text_of("TouchVideo_res_pct_Note"), "2352 x 1060 -> 2480 x 1116"), "拖到 100 %%：明确值、比这次启动大 → 重启提示（「%s」）", text_of("TouchVideo_res_pct_Note"));
    drag("res_pct", 6); step();
    CHECK(pend("res_pct") == 95 && !strcmp(value_of("res_pct"), "95 % · 2352 x 1060") && !text_of("TouchVideo_res_pct_Note")[0], "挪回设备缺省那格 = 明确的 95 %%（以后表变了它也不跟着变），尺寸同这次启动");
    dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", ""); step();
    CHECK(pend("res_pct") < 0 && sl_at("res_pct") == 6 && !strcmp(value_of("res_pct"), "Standard · 2352 x 1060"), "「默认」：回到 auto = 设备缺省（「%s」）", value_of("res_pct"));
    res_fb = 1; atomic_store(&g_repaint, 1); step();
    CHECK(!strcmp(text_of("TouchVideo_res_pct_Note"), "dmVideoResFallback"), "防崩退回过：auto 下写退回说明（「%s」）", text_of("TouchVideo_res_pct_Note"));
    res_fb = 0; dev_pct = 0; dev_s3d = 0; run_w = 2000; run_h = 900; atomic_store(&g_repaint, 1); steps(2);
    CHECK(sl_at("res_pct") == 3 && !strcmp(value_of("res_pct"), "Standard · 2000 x 900") && !strcmp(value_of("s3d"), "85 % · Standard"), "没有设备缺省（芯片不在表里 / 退回后）：同以前 900 行、85 %%（「%s」）", value_of("res_pct"));
    { int a = dm_res_chosen(DM_RES_AUTO); (void)a; dev_pct = 95; live_w = 2480; live_h = 1116; run_w = 2480; run_h = 1116;
      CHECK(dm_res_chosen(DM_RES_AUTO) == 1 && run_w == 2352 && run_h == 1060, "「应用」auto：按设备缺省的尺寸切（%d × %d）", run_w, run_h);
      dev_pct = 0; live_w = live_h = 0; }
    scr_w = 2688; scr_h = 1216; run_w = 1992; run_h = 900;




    generation++; build(4); atomic_store(&g_repaint, 1); steps(3);
    { widget *qw = wv("TouchCustom_s3d_preset"); const char *qh = "TouchVideo_s3d_preset_Hint";
      CHECK(val("res_pct") == DM_RES_AUTO && run_w == 1992 && run_h == 900 && !live_w, "start: output at the default 1992 x 900, no switching in place");
      CHECK(car_index(qw->b) == 1 && !strcmp(text_of(qh), "balanced 1992 x 900 85") && !qw->b[0x7d8 + 3] && pend("s3d") < 0 && pend("res_pct") < 0,
            "auto + s3d=0: Balanced with the default size and 3D in the note (\"%s\"), Custom not selectable", text_of(qh));
      car_tap(qw->b, 0); step();
      CHECK(pend("s3d") == 60 && pend("res_pct") < 0 && sl_at("s3d") == 0 && !strcmp(value_of("s3d"), "60 %") && car_index(qw->b) == 0 && !strcmp(text_of(qh), "save 1992 x 900 60") && dirty(),
            "left: Power saving = pending s3d 60, output stays at the default, the slider on its first notch (%d, \"%s\", \"%s\")", sl_at("s3d"), value_of("s3d"), text_of(qh));
      step(); CHECK(pend("s3d") == 60 && sl_at("s3d") == 0, "the slider pump does not take the old notch back (%d)", pend("s3d"));
      car_tap(qw->b, 1); step();
      CHECK(pend("s3d") < 0 && pend("res_pct") < 0 && sl_at("s3d") == 5 && sl_at("res_pct") == 3 && !strcmp(value_of("s3d"), "85 % · Standard") && car_index(qw->b) == 1,
            "right: Balanced = the stored pair again, nothing pending, sliders on 85 and the default notch 75 %%");
      car_tap(qw->b, 1); step();
      CHECK(pend("s3d") == 100 && pend("res_pct") == 100 && sl_at("s3d") == 8 && sl_at("res_pct") == 8 && !strcmp(text_of(qh), "sharp 2688 x 1216 100"),
            "right: Sharp = output 100 %% and 3D 100, both sliders on their last notch (%d, %d, \"%s\")", sl_at("s3d"), sl_at("res_pct"), text_of(qh));
      step(); CHECK(pend("res_pct") == 100 && pend("s3d") == 100 && !strcmp(value_of("res_pct"), "100 % · 2688 x 1216"), "the slider pumps keep both values (\"%s\")", value_of("res_pct"));
      CHECK(!strcmp(text_of("TouchVideo_res_pct_Note"), "1992 x 900 -> 2688 x 1216") && !strcmp(text_of("TouchVideo_s3d_Hint"), "after 2688 x 1216 x2.52"),
            "Sharp is larger than this start: the restart line (\"%s\") and the 3D line after the restart (\"%s\")", text_of("TouchVideo_res_pct_Note"), text_of("TouchVideo_s3d_Hint"));
      car_tap(qw->b, 1); step();
      CHECK(pend("s3d") == 60 && pend("res_pct") < 0 && sl_at("res_pct") == 3 && car_index(qw->b) == 0, "right from Sharp wraps to Power saving, Custom is skipped, output back to the default (%d)", pend("s3d"));
      car_tap(qw->b, 0); step();
      CHECK(pend("s3d") == 100 && pend("res_pct") == 100 && car_index(qw->b) == 2, "left from Power saving wraps to Sharp, Custom is skipped (%d)", pend("s3d"));
      drag("s3d", 5); step(); step();
      CHECK(pend("s3d") == 85 && pend("res_pct") == 100 && car_index(qw->b) == 3 && qw->b[0x7d8 + 3] && !strcmp(text_of(qh), "custom 2688 x 1216 85"),
            "3D slider on an explicit 85 with output 100 %%: Custom shown (entry %d, \"%s\")", car_index(qw->b), text_of(qh));
      car_tap(qw->b, 1); step(); step();
      CHECK(pend("s3d") == 60 && pend("res_pct") < 0 && car_index(qw->b) == 0 && !qw->b[0x7d8 + 3] && sl_at("s3d") == 0, "right from Custom: with Power saving on, Custom is not selectable");
      drag("s3d", 6); step(); step();
      CHECK(pend("s3d") == 90 && car_index(qw->b) == 3, "slider 90: Custom");
      dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", ""); steps(2);
      CHECK(pend("s3d") < 0 && pend("res_pct") < 0 && car_index(qw->b) == 1 && sl_at("s3d") == 5, "Default: s3d 0 and auto, Balanced, slider on the default");
      drag("res_pct", 8); step(); step();
      CHECK(pend("res_pct") == 100 && pend("s3d") < 0 && car_index(qw->b) == 3 && !strcmp(text_of(qh), "custom 2688 x 1216 85"), "output slider alone on 100 %%: Custom (\"%s\")", text_of(qh));
      dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); steps(2);

      pad = 1; generation++; build(4); steps(3); qw = wv("TouchCustom_s3d_preset");
      for (int k = 0; k < 3; k++) { key_ev = DM_BTN_DOWN; step(); }
      CHECK(sel_row_is("s3d_preset"), "down x3: the 3D quick choice row is selected");
      key_ev = DM_BTN_RIGHT; step(); step(); CHECK(pend("s3d") == 100 && pend("res_pct") == 100 && sl_at("s3d") == 8 && sl_at("res_pct") == 8, "controller right: Sharp (%d)", pend("s3d"));
      key_ev = DM_BTN_A; step(); step(); CHECK(pend("s3d") == 60 && pend("res_pct") < 0, "controller A: forward, wraps to Power saving (%d)", pend("s3d"));
      key_ev = DM_BTN_LEFT; step(); step(); CHECK(pend("s3d") == 100 && pend("res_pct") == 100, "controller left: back to Sharp (%d)", pend("s3d"));
      key_ev = DM_BTN_DOWN; step(); CHECK(sel_row_is("s3d"), "down: the 3D sharpness slider below");
      pad = 0; dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); steps(2);
      CHECK(pend("s3d") < 0 && pend("res_pct") < 0 && car_index(qw->b) == 1, "Discard: back to Balanced");

      s3d_pin = 1; steps(2);
      CHECK(car_index(qw->b) == 3 && qw->b[0x7d8 + 3] && !qw->b[0x7d8] && !qw->b[0x7d8 + 1] && !qw->b[0x7d8 + 2] && !strcmp(text_of(qh), "90 %") && !row_here(A(&page), ROW_S3Q),
            "pinned: Custom with the pinned note (\"%s\"), nothing else selectable, row skipped", text_of(qh));
      car_tap(qw->b, 1); step(); CHECK(pend("s3d") < 0 && pend("res_pct") < 0 && car_index(qw->b) == 3, "pinned: a tap changes nothing");
      s3d_pin = 0; dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); steps(2);

      car_tap(qw->b, 2); step(); dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
      CHECK(val("res_pct") == 100 && val("s3d") == 100 && res_chosen == 100 && s3d_menu == 100 && file_has(d, "res_pct=100") && file_has(d, "s3d=100") && car_index(qw->b) == 2 && !dirty(),
            "Apply Sharp: output 100 %% (%d) and 3D 100 (%d) handed over and saved", res_chosen, s3d_menu);
      CHECK(run_w == 1992 && !strcmp(text_of("TouchVideo_res_pct_Note"), "1992 x 900 -> 2688 x 1216"), "Sharp takes effect after a restart (\"%s\")", text_of("TouchVideo_res_pct_Note"));

      run_w = live_w = 2688; run_h = live_h = 1216; atomic_store(&g_repaint, 1); steps(2);
      CHECK(car_index(qw->b) == 2 && !text_of("TouchVideo_res_pct_Note")[0] && !strcmp(value_of("res_pct"), "100 % · 2688 x 1216"), "after the restart: Sharp, no restart line (\"%s\")", text_of("TouchVideo_res_pct_Note"));
      car_tap(qw->b, 1); step(); car_tap(qw->b, 1); step();
      CHECK(car_index(qw->b) == 1 && pend("res_pct") == DM_RES_AUTO && pend("s3d") == 0 && !strcmp(text_of("TouchVideo_res_pct_Note"), "switches on apply"),
            "two steps right: Balanced pending, the output switches on Apply (\"%s\")", text_of("TouchVideo_res_pct_Note"));
      dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
      CHECK(val("res_pct") == DM_RES_AUTO && val("s3d") == 0 && res_chosen == DM_RES_AUTO && run_w == 1992 && run_h == 900 && !text_of("TouchVideo_res_pct_Note")[0] && car_index(qw->b) == 1,
            "Apply Balanced: switched in place to %d x %d, no restart line", run_w, run_h);
      live_w = live_h = 0;

      res_pin = 1; atomic_store(&g_val[K_RESP], 100); atomic_store(&g_repaint, 1); steps(2);
      CHECK(car_index(qw->b) == 1 && !strcmp(text_of(qh), "balanced 1992 x 900 85"), "pinned output: s3d 0 = Balanced whatever res_pct holds (\"%s\")", text_of(qh));
      car_tap(qw->b, 1); step(); step();
      CHECK(pend("s3d") == 100 && pend("res_pct") < 0 && car_index(qw->b) == 2 && !strcmp(text_of(qh), "sharp 1992 x 900 100"), "pinned output: Sharp writes s3d only (\"%s\")", text_of(qh));
      dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); res_pin = 0; caps = 3; atomic_store(&g_repaint, 1); steps(2);
      car_tap(qw->b, 1); step(); step();
      CHECK(pend("s3d") == 100 && pend("res_pct") < 0 && car_index(qw->b) == 2, "platform without output resolution: Sharp writes s3d only (%d)", pend("res_pct"));
      dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); caps = 7; atomic_store(&g_val[K_RESP], DM_RES_AUTO); atomic_store(&g_repaint, 1); steps(2);

      static const int st[6][3] = { { 100, 100, 2 }, { DM_RES_AUTO, 0, 1 }, { DM_RES_AUTO, 60, 0 }, { DM_RES_AUTO, 90, 3 }, { DM_RES_AUTO, 100, 3 }, { 100, 0, 3 } };
      for (int k = 0; k < 6; k++) { atomic_store(&g_val[K_RESP], st[k][0]); atomic_store(&g_val[K_S3D], st[k][1]); atomic_store(&g_pend[K_RESP], -1); atomic_store(&g_pend[K_S3D], -1); atomic_store(&g_repaint, 1); steps(2);
          CHECK(car_index(qw->b) == st[k][2] && pend("s3d") < 0 && pend("res_pct") < 0 && !dirty(), "stored res_pct=%d s3d=%d -> entry %d (%d), nothing pending", st[k][0], st[k][1], st[k][2], car_index(qw->b)); }
      atomic_store(&g_val[K_RESP], DM_RES_AUTO); atomic_store(&g_val[K_S3D], 0); atomic_store(&g_repaint, 1); steps(2);
      int v; CHECK(parse_val(K_S3D, "60", &v) && v == 60 && parse_val(K_S3D, "65", &v) && parse_val(K_S3D, "70", &v) && parse_val(K_S3D, "0", &v) && !parse_val(K_S3D, "55", &v) && !parse_val(K_S3D, "62", &v),
            "parser: 0 and 60-100 in steps of 5, 55 / 62 rejected"); }





    caps = 7 | 8; driver_opens = 0; driver_own = 0; generation++; build(4); atomic_store(&g_repaint, 1); steps(3);
    CHECK(g_view == 1 && wv("TouchCustomDriverOpen")->b[0x50] && wv("TouchVideo_driver_Label")->b[0x50] && wv("TouchCustom_driver_Value")->b[0x50], "caps bit 8: the driver row is shown (button, title, value)");
    CHECK(!strcmp(value_of("driver"), "dmVideoDriverSystem"), "system driver running (hook returns 0): the value says \"System\" (\"%s\")", value_of("driver"));
    driver_own = 1; step();
    CHECK(!strcmp(value_of("driver"), "Turnip V34"), "own driver running: the value is its package label (\"%s\")", value_of("driver"));
    dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustomDriverOpen"); step();
    CHECK(driver_opens == 1 && !dirty(), "the tap hands the request to the platform once and changes no setting (%d)", driver_opens);
    tab(3); steps(2); dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustomDriverOpen");
    CHECK(driver_opens == 1 && shown(&ext_c), "Extras on screen: a driver-row tap does not count");
    tab(4); steps(2);
    busy = 1; step(); dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustomDriverOpen"); busy = 0; step();
    CHECK(driver_opens == 1, "during a switch (relay) a driver-row tap does not count");
    pad = 1; generation++; build(4); steps(3);
    for (int k = 0; k < 7; k++) { key_ev = DM_BTN_DOWN; step(); }
    CHECK(sel_row_is("driver"), "controller: seven downs from the frame rate reach the driver row (the view's last)");
    key_ev = DM_BTN_A; step();
    CHECK(driver_opens == 2 && !dirty(), "controller A hands the request over (%d)", driver_opens);
    key_ev = DM_BTN_RIGHT; step(); CHECK(driver_opens == 2 && !dirty(), "left / right on the driver row do nothing");
    pad = 0; caps = 7; atomic_store(&g_repaint, 1); steps(3);
    CHECK(!wv("TouchCustomDriverOpen")->b[0x50] && !wv("TouchCustom_driver_Value")->b[0x50], "without the caps bit: the whole row is hidden");
    dm_custom_on_uimsg("PanelManager", "ClosePanel", "TouchCustomDriverOpen"); CHECK(driver_opens == 2, "without the caps bit a tap does not count");
    pad = 1; generation++; build(4); steps(3);
    for (int k = 0; k < 7; k++) { key_ev = DM_BTN_DOWN; step(); }
    CHECK(sel_row_is("bright"), "without the caps bit the controller stops at the last real row (driver row skipped)");
    key_ev = DM_BTN_A; step(); CHECK(driver_opens == 2, "A on the brightening row is not the driver row");
    pad = 0; caps = 7 | 8;
    char p[1100]; snprintf(p, sizeof p, "%s/sw_custom.txt", d); unlink(p); rmdir(d);
    printf("t_custom_video：%d 项，%d 项不通\n", checks, fails); return fails != 0;
}
