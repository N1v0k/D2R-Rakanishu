











#define DM_CUSTOM_TEST_PLATFORM 1
#define DM_CUSTOM_SET_TEXT test_set_text
#include "t_custom_carousel_fake.h"
#include "../game_addrs.h"
#include <stdint.h>
static void test_set_text(uint64_t w, const char *s);
#include "../dm_custom.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

module_t g_mod[MAX_MOD];
int g_nmod;
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)


static uint32_t generation = 1; static uint64_t frame; static int pad, busy, reloading; static uint64_t key_ev;
uint32_t dm_ui_gen(void) { return generation; }
uint64_t dm_guest_frame(void) { return frame; }
int dm_padmode_current(void) { return pad; }
int dm_padmode_busy(void) { return busy; }
int dm_ui_reloading(void) { return reloading; }
uint64_t dm_padmode_page_keys(uint64_t want) { uint64_t e = key_ev & want; key_ev = 0; return e; }
uint64_t dm_padmode_page_held(void) { return 0; }
void dm_ui_wheel_on(uint64_t sv, int dir) { (void)sv; (void)dir; }
const char *dm_save_commitdir(void) { return NULL; }
void dm_cam_set_mode(int m) { (void)m; }
void dm_3d_quality_mode_set(int m) { (void)m; }
int dm_3d_quality_mode_get(void) { return 0; }
float dm_3d_scale_cur(void) { return 0.85f; }
void dm_3d_scale_menu_set(int pct) { (void)pct; }
int dm_3d_scale_default_pct(void) { return 85; }
int dm_3d_scale_pinned(void) { return 0; }
float dm_3d_scale_cfg(void) { return 0.85f; }
double mtl_display_refresh(void) { return 60; }
uint64_t dm_game_res(int docked) { (void)docked; return 900ull << 32 | 1992; }
int dm_lang_group(void) { return DM_LG_DE; }
int dm_ui_top_chk_on(void) { return 0; }
void dm_ui_top_chk(uint64_t root, const char *name, uint64_t got) { (void)root; (void)name; (void)got; }
void dm_ui_show_ours(uint64_t w, int on) { *(uint8_t *)(uintptr_t)(w + 0x50) = on != 0; *(uint8_t *)(uintptr_t)(w + 0x51) = on != 0; }
void dm_ui_move_ours(uint64_t w, int x) { (void)w; (void)x; }
void dm_ui_send_now(const char *cat, const char *act, const char *arg) { (void)cat; (void)act; (void)arg; }
const char *dm_str(uint8_t *b, const char *k) { (void)b; if (!strcmp(k, "dmCustomTechState")) return "state %s"; if (!strcmp(k, "dmCustomTechDownload")) return "Download"; return k; }
int dm_video_caps(void) { return 7; }
void dm_fps_apply(int fps) { (void)fps; }

int dm_res_screen(int *W, int *H) { *W = 2688; *H = 1216; return 1; }
int dm_res_running(int *w, int *h) { *w = 1992; *h = 900; return 1; }
int dm_res_fallback(void) { return 0; }
int dm_res_pinned(void) { return 0; }
int dm_res_chosen(int pct) { (void)pct; return 0; }
int dm_res_live_limit(int *w, int *h) { *w = *h = 0; return 0; }
int dm_res_live_broken(void) { return 0; }
int dm_res_full(void) { return 0; }
int dm_res_dev_pct(void) { return 0; }
int dm_display_max_hz(void) { return 60; }
int dm_display_modes(int *hz, int cap) { if (cap < 1) return 0; hz[0] = 60; return 1; }
int dm_fps_vote_policy(void) { return 0; }
static int xcaps = 3; int dm_extras_caps(void) { return xcaps; }
static const char *data_src = "/storage/emulated/0/Download/S2X/D2R Mobile/data"; const char *dm_data_source(void) { return data_src; }
void dm_driver_open(void) { } int dm_driver_label(char *buf, int cap) { (void)buf; (void)cap; return 0; }


typedef struct { uint8_t b[0xe00]; } widget;
static uint8_t main_mem[16];
static uint64_t page_vt[4], sel_vt[4], cont_vt[4];
enum { T_TOGGLE, T_TEXT, T_BUTTON, T_IMAGE, T_CAROUSEL };
typedef struct { const char *name; int type, x, y, w, h, n; } spec;

static const spec k_ext[] = {
    { "TouchCustom_lang", T_CAROUSEL, 685, 202, 700, 72, 7 }, { "TouchCustom_lang_Text", T_TEXT, 0, 180, 1385, 180 },
    { "TouchCustom_auto_mana", T_TOGGLE, 1315, 535, 70, 70 }, { "TouchCustom_auto_mana_Text", T_TEXT, 0, 480, 1000, 180 },
    { "TouchCustom_icloud", T_TOGGLE, 1315, 715, 70, 70 }, { "TouchCustom_icloud_Text", T_TEXT, 0, 660, 1000, 180 },
    { "TouchCustom_dmg_num", T_CAROUSEL, 685, 3922, 700, 72, 3 }, { "TouchCustom_dmg_num_Text", T_TEXT, 0, 3900, 1385, 180 }, { "TouchCustom_dmg_num_Hint", T_TEXT, 0, 4008, 1385, 64 },
    { "TouchCustom_build_hints", T_TOGGLE, 1315, 4115, 70, 70 }, { "TouchCustom_build_hints_Text", T_TEXT, 0, 4080, 1000, 180 },
    { "TouchCustomTech_Title", T_TEXT, 0, 4260, 1385, 180 }, { "TouchCustomTech_Divider", T_IMAGE, 0, 4410, 1206, 8 },
    { "TouchCustom_tech_Text", T_TEXT, 0, 4440, 1385, 180 }, { "TouchCustom_tech_Label", T_TEXT, 0, 4450, 1020, 100 }, { "TouchCustom_tech_State", T_TEXT, 0, 4540, 1020, 70 },
    { "TouchCustomTechOpen", T_BUTTON, 1056, 4488, 329, 83 },
    { "TouchCustomShoulder_Title", T_TEXT, 0, 4636, 1385, 180 }, { "TouchCustomShoulder_Divider", T_IMAGE, 0, 4786, 1206, 8 },
    { "TouchCustom_shoulder_l_Text", T_TEXT, 0, 4816, 1385, 180 }, { "TouchCustom_shoulder_l_Label", T_TEXT, 0, 4816, 676, 180 }, { "TouchCustom_shoulder_l", T_CAROUSEL, 685, 4870, 700, 72, 15 },
    { "TouchCustom_shoulder_r_Text", T_TEXT, 0, 4996, 1385, 180 }, { "TouchCustom_shoulder_r_Label", T_TEXT, 0, 4996, 676, 180 }, { "TouchCustom_shoulder_r", T_CAROUSEL, 685, 5050, 700, 72, 15 },
    { "TouchCustomShoulder_Hint", T_TEXT, 0, 5176, 1385, 64 } };
#define NEXT (int)(sizeof k_ext / sizeof *k_ext)
#define LAYOUT_H 5256
static widget root_w, settings, tabbar, page, ext_c, vid_c, sel_frame, ext_w[NEXT];
static uint64_t root_kids[2], set_kids[1], page_kids[3], ext_kids[NEXT];
static char texts[NEXT][96];
static int with_icloud;
static uint64_t A(const void *p) { return (uint64_t)(uintptr_t)p; }
static void put64(widget *w, size_t at, uint64_t v) { memcpy(w->b + at, &v, 8); }
static void put32(widget *w, size_t at, int32_t v) { memcpy(w->b + at, &v, 4); }
static int32_t get32(widget *w, size_t at) { int32_t v; memcpy(&v, w->b + at, 4); return v; }
static void relayout_noop(uint64_t w) { (void)w; }
static void test_set_text(uint64_t w, const char *s) { for (int i = 0; i < NEXT; i++) if (w == A(&ext_w[i])) strlcpy(texts[i], s, sizeof texts[i]); }
static int idx(const char *name) { for (int i = 0; i < NEXT; i++) if (!strcmp(k_ext[i].name, name)) return i; abort(); }
static widget *wv(const char *name) { return &ext_w[idx(name)]; }
static const char *text_of(const char *name) { return texts[idx(name)]; }
static void build(void) {
    memset(&root_w, 0, sizeof root_w); root_w.b[0x50] = 1; put64(&root_w, 0x58, A(root_kids)); put64(&root_w, 0x60, 2);
    memset(&settings, 0, sizeof settings); put64(&settings, 8, A("SettingsPanel")); settings.b[0x50] = 1; put64(&settings, 0x58, A(set_kids)); put64(&settings, 0x60, 1);
    memset(&tabbar, 0, sizeof tabbar); put64(&tabbar, 0, A(main_mem) + DM_VT_TABBAR); put64(&tabbar, 8, A("OptionsTabs")); tabbar.b[0x50] = 1; put32(&tabbar, 0x558, 5); put32(&tabbar, 0xdc4, 3);
    put64(&tabbar, 0xb50 + 32 * 3 + 0x10, A("TouchTabExtras")); tabbar.b[0xb50 + 32 * 3 + 0x18] = 1; put64(&tabbar, 0xb50 + 32 * 4 + 0x10, A("TouchTabVideo")); tabbar.b[0xb50 + 32 * 4 + 0x18] = 1;
    set_kids[0] = A(&tabbar);
    memset(&page, 0, sizeof page); put64(&page, 0, A(page_vt)); put64(&page, 8, A("VideoOptionsPanel")); page.b[0x50] = 1; put64(&page, 0x58, A(page_kids)); put64(&page, 0x60, 3);
    memset(&ext_c, 0, sizeof ext_c); put64(&ext_c, 0, A(cont_vt)); put64(&ext_c, 8, A("TouchCustomRows")); ext_c.b[0x50] = 1; put32(&ext_c, 0x7c, LAYOUT_H); put64(&ext_c, 0x58, A(ext_kids)); put64(&ext_c, 0x60, NEXT);
    memset(&vid_c, 0, sizeof vid_c); put64(&vid_c, 0, A(cont_vt)); put64(&vid_c, 8, A("TouchVideoRows")); put32(&vid_c, 0x70, 99999); put32(&vid_c, 0x7c, 2820);
    memset(&sel_frame, 0, sizeof sel_frame); put64(&sel_frame, 0, A(sel_vt)); put64(&sel_frame, 8, A("TouchCustomSel")); sel_frame.b[0x50] = 1; put32(&sel_frame, 0x70, 99999); put32(&sel_frame, 0x78, 1000); put32(&sel_frame, 0x7c, 100);
    page_kids[0] = A(&ext_c); page_kids[1] = A(&vid_c); page_kids[2] = A(&sel_frame);
    static const uint64_t vt[5] = { DM_VT_TOGGLE, DM_VT_TEXT, DM_VT_BUTTON, DM_VT_IMAGE, DM_VT_CAROUSEL };
    for (int i = 0; i < NEXT; i++) { widget *w = &ext_w[i]; const spec *s = &k_ext[i]; memset(w, 0, sizeof *w);
        const char *nm = s->name; if (!with_icloud && !strncmp(nm, "TouchCustom_icloud", 18)) nm = nm[18] ? "TouchGone_icloud_Text" : "TouchGone_icloud";
        put64(w, 0, A(main_mem) + vt[s->type]); put64(w, 8, A(nm)); w->b[0x50] = w->b[0x51] = 1;
        put32(w, 0x70, s->x); put32(w, 0x74, s->y); put32(w, 0x78, s->w); put32(w, 0x7c, s->h);
        static const int32_t fr[4] = { 0, 1, 3, 2 }; if (s->type == T_TOGGLE || s->type == T_BUTTON) memcpy(w->b + 0x6e8, fr, sizeof fr);
        if (s->type == T_CAROUSEL) { car_fake(w->b, A(main_mem) + DM_VT_CAROUSEL, s->n, 0, 0); put32(w, 0x70, s->x); put32(w, 0x74, s->y); put32(w, 0x78, s->w); put32(w, 0x7c, s->h); }
        ext_kids[i] = A(w); }
    root_kids[0] = A(&settings); root_kids[1] = A(&page);
    memset(texts, 0, sizeof texts);
}
uint64_t dm_wref_get(dm_wref *r) { return r->w && r->gen == generation ? r->w : 0; }
static void step(void) { frame++; dm_custom_ui_pump(A(&root_w)); }
static void steps(int n) { while (n--) step(); }
static int pend(const char *key) { return atomic_load(&g_pend[key_index(key)]); }
static int val(const char *key) { return atomic_load(&g_val[key_index(key)]); }
static int shown(const char *name) { return wv(name)->b[0x50] && wv(name)->b[0x51]; }
static int cont_h(void) { return get32(&ext_c, 0x7c); }
static void tap(const char *name) { dm_custom_on_uimsg("PanelManager", "ClosePanel", name); }
static int cs(const char *name) { return car_index(wv(name)->b); }
static int can(const char *name, int k) { return wv(name)->b[0x7d8 + k] != 0; }
static void ctap(const char *name, int fwd) { car_tap(wv(name)->b, fwd); }
static int sel_top(void) { return get32(&sel_frame, 0x74); }
static void key(uint64_t k) { key_ev = k; step(); }


static void dm_custom_reload_for_test(const char *dir) { atomic_store(&g_loaded, 0); for (int i = 0; i < NK; i++) atomic_store(&g_pend[i], -1); dm_custom_init(dir); }
static int file_has(const char *dir, const char *line) { char p[1100], ln[128]; snprintf(p, sizeof p, "%s/sw_custom.txt", dir); FILE *f = fopen(p, "r"); int got = 0; while (f && fgets(ln, sizeof ln, f)) got |= !strncmp(ln, line, strlen(line)) && ln[strlen(line)] == '\n'; if (f) fclose(f); return got; }
static int file_exists(const char *dir) { char p[1100]; snprintf(p, sizeof p, "%s/sw_custom.txt", dir); return access(p, F_OK) == 0; }
static void file_put(const char *dir, const char *body) { char p[1100]; snprintf(p, sizeof p, "%s/sw_custom.txt", dir); if (!body) { unlink(p); return; } FILE *f = fopen(p, "w"); fputs(body, f); fclose(f); }

static int migrate(const char *dir, const char *body, int l, int r) {
    if (body != (const char *)1) file_put(dir, body); fflush(stdout); pid_t p = fork(); if (p < 0) return -1;
    if (!p) { dm_custom_init(dir); dm_custom_legacy_shoulder(l, r); _exit(dm_custom_get("shoulder_l") * 16 + dm_custom_get("shoulder_r")); }
    int s = 0; waitpid(p, &s, 0); return WIFEXITED(s) ? WEXITSTATUS(s) : -1; }

int main(void) {
    const char *td = getenv("TMPDIR"); char tmpl[1024]; snprintf(tmpl, sizeof tmpl, "%s/t_custom_extras.XXXXXX", td && *td ? td : "/tmp");
    char *d = mkdtemp(tmpl); if (!d) { perror("mkdtemp"); return 1; }
    strlcpy(g_mod[0].name, "main", sizeof g_mod[0].name); g_mod[0].base = main_mem; g_nmod = 1;
    page_vt[0] = A(main_mem) + GA_OPTIONS_PAGE_UPDATE; sel_vt[2] = A(relayout_noop); cont_vt[2] = A(relayout_noop);


    { int v; CHECK(parse_val(K_SHL, "9\n", &v) && v == 9, "shoulder_l=9 读成 9（不是「非 0 即 1」）");
      CHECK(parse_val(K_SHR, "14", &v) && v == 14 && !parse_val(K_SHR, "15", &v) && !parse_val(K_SHL, "-1", &v) && !parse_val(K_SHL, "x", &v), "0–14 认，15 / -1 / 坏串不认（当没写）"); }
    { int v[5], ok = 1; for (int x = 0; x < 4; x++) { char t[4]; snprintf(t, sizeof t, "%d", x); ok &= parse_val(K_SPLASH, t, &v[x]); }
      CHECK(ok && v[0] == 0 && v[1] == 1 && v[2] == 1 && v[3] == 1 && !parse_val(K_SPLASH, "4", &v[4]) && !parse_val(K_SPLASH, "-1", &v[4]),
            "melee_splash: 0 stays off, the older shares 1-3 (25 / 50 / 100 %%) all read as on, 4 / -1 count as unwritten");
      ok = parse_val(K_SPLASH, "1", &v[4]); CHECK(ok && v[4] == 1, "melee_splash: the new on value 1 reads as on again (a second load changes nothing)"); }
    CHECK(migrate(d, NULL, 12, 3) == 12 * 16 + 3 && file_has(d, "shoulder_l=12") && file_has(d, "shoulder_r=3"), "旧偏好 左 12 / 右 3 → 文件 12 / 3，当场存盘");
    CHECK(migrate(d, (const char *)1, -1, -1) == 12 * 16 + 3, "第二次读盘稳定（不再看旧偏好）");
    CHECK(migrate(d, "shoulder_l=6\n", 12, 3) == 6 * 16 + 3 && file_has(d, "shoulder_l=6") && file_has(d, "shoulder_r=3"), "文件里已有的键（哪怕是缺省 6）不动，只迁移没有的那一个");
    file_put(d, NULL);
    CHECK(migrate(d, NULL, -1, -1) == 6 * 16 + 7 && !file_exists(d), "没有旧偏好：6 / 7，不存盘");
    CHECK(migrate(d, NULL, 99, 13) == 6 * 16 + 13 && file_has(d, "shoulder_r=13"), "坏的旧值（99）当没有；13（配方书）在文件里照样认");
    CHECK(migrate(d, "shoulder_l=abc\nshoulder_r=\n", 2, 5) == 2 * 16 + 5, "坏文件里的坏值当没写，旧偏好补上");
    file_put(d, "shoulder_l=10\nshoulder_r=14\n");


    { int rows[NK + 1], n = view_rows(0, rows);
      CHECK(n >= 3 && rows[n - 3] == ROW_TECH && rows[n - 2] == K_SHL && rows[n - 1] == K_SHR, "行表末尾：技术、左肩、右肩"); }


    dm_custom_init(d); build(); steps(3);
    CHECK(shown("TouchCustomTechOpen") && shown("TouchCustom_tech_State") && !shown("TouchCustomShoulder_Title") && !shown("TouchCustom_shoulder_l") && !shown("TouchCustomShoulder_Hint"), "技术显示，肩键整节藏起");
    CHECK(cont_h() == 4636, "行容器高 = 肩键标题的 y 4636（实际 %d）", cont_h());
    CHECK(!strcmp(text_of("TouchCustom_tech_State"), "state Download"), "技术行灰字：数据在下载文件夹（「%s」）", text_of("TouchCustom_tech_State"));
    ctap("TouchCustom_shoulder_l", 1); step();
    CHECK(pend("shoulder_l") == -1, "藏着的轮选不管（%d）", pend("shoulder_l"));

    pad = 1; step();
    for (int i = 0; i < 8; i++) key(DM_BTN_DOWN);
    CHECK(sel_top() == 4440 - 10, "↓ 停在技术行（选中框上沿 %d）", sel_top());
    key(DM_BTN_LEFT); key(DM_BTN_RIGHT);
    CHECK(!dm_custom_tech_take(), "技术行 ◀▶ 什么都不做");
    key(DM_BTN_A);
    CHECK(dm_custom_tech_take() == 1 && dm_custom_tech_take() == 0, "技术行 A = 打开请求，取走就清");
    { int dirty = 0; for (int i = 0; i < NK; i++) dirty |= atomic_load(&g_pend[i]) >= 0; CHECK(!dirty && !page.b[0x1a0], "伪行的 A / ◀▶ 不留任何待定值，「应用」不亮"); }

    busy = 1; step(); generation++; build(); step(); busy = 0; steps(2);
    CHECK(sel_top() == 180 - 10, "重建后选回第一行（%d）", sel_top());
    pad = 0;


    dm_custom_has_triggers(1); step();
    CHECK(shown("TouchCustomShoulder_Title") && shown("TouchCustom_shoulder_r") && cont_h() == LAYOUT_H, "肩键一节出现，高 %d", cont_h());
    generation++; build(); steps(2);
    CHECK(cs("TouchCustom_shoulder_l") == 10 && cs("TouchCustom_shoulder_r") == 14, "轮选按文件选中 10 / 14（%d %d）", cs("TouchCustom_shoulder_l"), cs("TouchCustom_shoulder_r"));
    CHECK(!can("TouchCustom_shoulder_l", 13) && can("TouchCustom_shoulder_l", 12) && can("TouchCustom_shoulder_l", 14), "配方书 13 不能选，别的能选");


    ctap("TouchCustom_shoulder_l", 1); step();
    CHECK(pend("shoulder_l") == 11, "左 ▶：10 → 11（%d）", pend("shoulder_l"));
    ctap("TouchCustom_shoulder_l", 1); step(); ctap("TouchCustom_shoulder_l", 1); step();
    CHECK(pend("shoulder_l") == 14 && cs("TouchCustom_shoulder_l") == 14, "11 → 12 → 14（跳过配方书 13）（%d）", pend("shoulder_l"));
    ctap("TouchCustom_shoulder_l", 1); step();
    CHECK(pend("shoulder_l") == 0, "到头再点：14 → 0 绕回（%d）", pend("shoulder_l"));
    ctap("TouchCustom_shoulder_l", 0); step(); ctap("TouchCustom_shoulder_l", 0); step();
    CHECK(pend("shoulder_l") == 12, "◀：0 → 14 → 12（绕回、跳过 13）（%d）", pend("shoulder_l"));
    CHECK(page.b[0x1a0], "肩键改了：「应用」亮");

    busy = 1; step(); ctap("TouchCustom_shoulder_l", 0); step(); busy = 0; step();
    CHECK(pend("shoulder_l") == 12 && cs("TouchCustom_shoulder_l") == 12, "切换中点轮选不算，摆回 12（%d，第 %d 项）", pend("shoulder_l"), cs("TouchCustom_shoulder_l"));


    dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); step();
    CHECK(pend("shoulder_l") == -1 && val("shoulder_l") == 10 && cs("TouchCustom_shoulder_l") == 10, "放弃：回到 10（第 %d 项）", cs("TouchCustom_shoulder_l"));
    ctap("TouchCustom_shoulder_r", 0); step();
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
    CHECK(val("shoulder_r") == 12 && file_has(d, "shoulder_r=12") && dm_custom_get("shoulder_r") == 12, "应用：右肩 14 → 12，存盘，dm_custom_get 读到");


    file_put(d, "shoulder_l=13\nshoulder_r=12\n"); dm_custom_reload_for_test(d); generation++; build(); steps(2);
    CHECK(val("shoulder_l") == 13 && cs("TouchCustom_shoulder_l") == 13 && can("TouchCustom_shoulder_l", 13), "文件里的 13：选中第 13 项、能选（第 %d 项）", cs("TouchCustom_shoulder_l"));
    ctap("TouchCustom_shoulder_r", 1); step(); dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); generation++; build(); steps(2);
    CHECK(val("shoulder_l") == 13 && file_has(d, "shoulder_l=13") && cs("TouchCustom_shoulder_l") == 13, "别的键应用、重建后 13 还在");
    ctap("TouchCustom_shoulder_l", 1); step(); step();
    CHECK(pend("shoulder_l") == 14 && !can("TouchCustom_shoulder_l", 13), "13 → 14，之后 13 又不能选");
    ctap("TouchCustom_shoulder_l", 0); step();
    CHECK(pend("shoulder_l") == 12, "14 ◀ 跳过 13 到 12（%d）", pend("shoulder_l"));
    dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); step();
    file_put(d, "shoulder_l=10\nshoulder_r=12\n"); dm_custom_reload_for_test(d); generation++; build(); steps(2);


    pad = 1; step();
    for (int i = 0; i < 10; i++) key(DM_BTN_DOWN);
    CHECK(sel_top() == 4996 - 10, "↓ 到底是右肩（%d）", sel_top());
    key(DM_BTN_UP);
    CHECK(sel_top() == 4816 - 10, "↑ 左肩（%d）", sel_top());
    key(DM_BTN_RIGHT); key(DM_BTN_RIGHT); key(DM_BTN_RIGHT);
    CHECK(pend("shoulder_l") == 14, "▶ 10 → 11 → 12 → 14（%d）", pend("shoulder_l"));
    key(DM_BTN_A);
    CHECK(pend("shoulder_l") == 0, "A 过了 14 绕回 0（%d）", pend("shoulder_l"));
    key(DM_BTN_LEFT);
    CHECK(pend("shoulder_l") == 14, "◀ 在 0 绕回 14（%d）", pend("shoulder_l"));
    key(DM_BTN_A); key(DM_BTN_A);
    CHECK(pend("shoulder_l") == 1, "A 14 → 0 → 1（%d）", pend("shoulder_l"));
    pad = 0;



    dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); step();
    CHECK(cs("TouchCustom_dmg_num") == 2 && !strcmp(text_of("TouchCustom_dmg_num_Hint"), "dmCustomDmgNumColorHint"), "缺省彩色：第 2 项，说明「%s」", text_of("TouchCustom_dmg_num_Hint"));
    ctap("TouchCustom_dmg_num", 0); step();
    CHECK(pend("dmg_num") == 1 && !strcmp(text_of("TouchCustom_dmg_num_Hint"), "dmCustomDmgNumSimpleHint"), "◀ 简单：待定 1，说明跟着（「%s」）", text_of("TouchCustom_dmg_num_Hint"));
    dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
    CHECK(val("dmg_num") == 1 && file_has(d, "dmg_num=1"), "应用：存 dmg_num=1");
    for (int v = 0; v < 3; v++) { char body[64]; snprintf(body, sizeof body, "shoulder_l=10\nshoulder_r=12\ndmg_num=%d\n", v); file_put(d, body); dm_custom_reload_for_test(d); generation++; build(); steps(2);
        CHECK(cs("TouchCustom_dmg_num") == v && pend("dmg_num") == -1 && !page.b[0x1a0], "旧文件 dmg_num=%d → 第 %d 项，没有待定", v, cs("TouchCustom_dmg_num"));
        ctap("TouchCustom_shoulder_r", 1); step(); dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
        snprintf(body, sizeof body, "dmg_num=%d", v); CHECK(file_has(d, body), "别的键应用后文件里仍是 %s", body); }
    file_put(d, "shoulder_l=10\nshoulder_r=12\n"); dm_custom_reload_for_test(d); generation++; build(); steps(2);

    CHECK(cs("TouchCustom_lang") == 1, "语言行：Deutsch 是第 %d 项", cs("TouchCustom_lang"));


    dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", ""); step();
    CHECK(pend("shoulder_l") == 6 && pend("shoulder_r") == 7, "默认：6 / 7（%d %d）", pend("shoulder_l"), pend("shoulder_r"));
    dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); step();
    dm_custom_has_triggers(0); step();
    CHECK(!shown("TouchCustomShoulder_Title") && cont_h() == 4636, "又没有肩键（测试）：一节藏起、高缩回");
    dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", ""); step();
    CHECK(pend("shoulder_l") == -1 && pend("shoulder_r") == -1, "藏着的那一节不归「默认」（%d %d）", pend("shoulder_l"), pend("shoulder_r"));
    dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); step();


    for (int v = 0; v < 2; v++) { char body[80]; snprintf(body, sizeof body, "shoulder_l=10\nshoulder_r=12\nicloud=%d\n", v); file_put(d, body); dm_custom_reload_for_test(d); generation++; build(); steps(2);
        dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", ""); step();
        CHECK(pend("icloud") == -1 && pend("dmg_num") == -1, "row left out, icloud=%d: Default leaves it alone (pending %d)", v, pend("icloud"));
        ctap("TouchCustom_shoulder_r", 1); step(); dm_custom_on_uimsg("OptionsPanelMessage", "AcceptChanges", ""); step();
        dm_custom_reload_for_test(d); snprintf(body, sizeof body, "icloud=%d", v);
        CHECK(dm_custom_get("icloud") == v && file_has(d, body), "row left out: after Default, Apply and a reload icloud is still %d (%d)", v, dm_custom_get("icloud")); }
    with_icloud = 1; file_put(d, "shoulder_l=10\nshoulder_r=12\nicloud=0\n"); dm_custom_reload_for_test(d); generation++; build(); steps(2);
    CHECK(!wv("TouchCustom_icloud")->b[0xa88], "row on the page: the checkbox shows icloud=0");
    dm_custom_on_uimsg("OptionsPanelMessage", "ResetToDefaults", ""); step();
    CHECK(pend("icloud") == 1 && wv("TouchCustom_icloud")->b[0xa88], "row on the page: Default resets icloud to 1 (pending %d)", pend("icloud"));
    dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); step();
    with_icloud = 0; file_put(d, "shoulder_l=10\nshoulder_r=12\n"); dm_custom_reload_for_test(d); generation++; build(); steps(2);


    tap("TouchCustomTechOpen");
    CHECK(dm_custom_tech_take() == 1, "点「打开」= 请求");
    put32(&tabbar, 0xdc4, 4); steps(2); tap("TouchCustomTechOpen");
    CHECK(!dm_custom_tech_take(), "Video 视图时（Extras 藏着）点了不算");
    put32(&tabbar, 0xdc4, 3); steps(2);
    settings.b[0x50] = 0; page.b[0x50] = 0; step(); tap("TouchCustomTechOpen");
    CHECK(!dm_custom_tech_take(), "选项关着时不算");
    settings.b[0x50] = 1; page.b[0x50] = 1; generation++; build(); steps(3);


    xcaps = 0; dm_custom_has_triggers(1); steps(2);
    CHECK(!shown("TouchCustomTechOpen") && !shown("TouchCustomTech_Title") && !shown("TouchCustomShoulder_Title") && cont_h() == 4260, "iOS / Mac：两节都藏，高 %d", cont_h());
    tap("TouchCustomTechOpen");
    CHECK(!dm_custom_tech_take(), "藏着的「打开」点了不算");
    xcaps = 3; data_src = "/data/user/0/io.r4k4n1shu.d2r/files/mydata/"; steps(2);
    CHECK(!strcmp(text_of("TouchCustom_tech_State"), "state mydata"), "不在下载文件夹：最后一段（「%s」）", text_of("TouchCustom_tech_State"));

    char p[1100]; snprintf(p, sizeof p, "%s/sw_custom.txt", d); unlink(p); rmdir(d);
    printf("t_custom_extras：%d 项，%d 项不通\n", checks, fails); return fails != 0;
}
