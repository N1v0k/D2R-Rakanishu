














#include "t_custom_carousel_fake.h"
#include "../game_addrs.h"
#include "../dm_custom.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
float dm_3d_scale_cur(void) { return 1.0f; }
void dm_3d_scale_menu_set(int pct) { (void)pct; }
int dm_3d_scale_default_pct(void) { return 85; }
int dm_3d_scale_pinned(void) { return 0; }
float dm_3d_scale_cfg(void) { return 0.85f; }
double mtl_display_refresh(void) { return 60; }
uint64_t dm_game_res(int docked) { (void)docked; return 900ull << 32 | 1992; }

const char *dm_str(uint8_t *b, const char *k) { (void)b;
    static const char *const f[][2] = { { "dmVideoS3dLive", "%d x %d x%d.%02d" }, { "dmVideoResSize", "%d x %d %s" }, { "dmVideoResRestart", "%s -> %s" },
        { "dmVideoResPinned", "%d x %d" }, { "dmVideoS3dPinned", "%d %%" }, { "dmVideoS3dAuto", "%d %%" } };
    for (unsigned i = 0; i < sizeof f / sizeof *f; i++) if (!strcmp(k, f[i][0])) return f[i][1]; return k; }
int dm_lang_group(void) { return DM_LG_DE; }
int dm_ui_top_chk_on(void) { return 0; }
void dm_ui_top_chk(uint64_t root, const char *name, uint64_t got) { (void)root; (void)name; (void)got; }
void dm_ui_show_ours(uint64_t w, int on) { (void)w; (void)on; }
void dm_ui_move_ours(uint64_t w, int x) { (void)w; (void)x; }
void dm_ui_send_now(const char *cat, const char *act, const char *arg) { (void)cat; (void)act; (void)arg; }


typedef struct { uint8_t b[0xb00]; } widget;
static uint8_t main_mem[16];
static uint64_t page_vt[4];




static const char *const k_rows[] = { "TouchCustom_lang", "TouchCustom_auto_mana", "TouchCustom_merc_pot", "TouchCustom_drop_light", "TouchCustom_map_mark",
                                      "TouchCustom_item_cmp", "TouchCustom_pot_pick",
                                      "TouchCustom_touch_swap_above", "TouchCustom_cam", "TouchCustom_lang_FontStd", "TouchCustom_lang_FontCjk",
                                      "TouchCustom_lang_Text", "TouchCustomSel" };
#define NROW (int)(sizeof k_rows / sizeof *k_rows)
enum { FACE_STD = 11, FACE_CJK = 22 };
typedef struct { widget page, box[NROW], ltext; uint64_t kids[NROW]; } page_mem;
static page_mem mem_a, mem_b;
static widget root_w; static uint64_t root_kids[1];
static page_mem *cur, *building;
static widget *box(const char *name) { page_mem *m = building ? building : cur; for (int i = 0; i < NROW; i++) if (!strcmp(k_rows[i], name)) return &m->box[i]; abort(); }
static uint64_t A(const void *p) { return (uint64_t)(uintptr_t)p; }
static void put64(widget *w, size_t at, uint64_t v) { memcpy(w->b + at, &v, 8); }
static void put32(widget *w, size_t at, int32_t v) { memcpy(w->b + at, &v, 4); }
static uint64_t root(void) { return A(&root_w); }
static void relayout_noop(uint64_t w) { (void)w; }
static uint64_t sel_vt[4];
static void build_page(page_mem *m) {
    memset(m, 0, sizeof *m); building = m;
    put64(&m->page, 0, A(page_vt)); put64(&m->page, 8, A("VideoOptionsPanel")); m->page.b[0x50] = m->page.b[0x51] = 1;
    put64(&m->page, 0x58, A(m->kids)); put64(&m->page, 0x60, NROW);
    for (int i = 0; i < NROW; i++) { widget *w = &m->box[i]; m->kids[i] = A(w);
        put64(w, 0, A(main_mem) + DM_VT_TOGGLE); put64(w, 8, A(k_rows[i])); w->b[0x50] = w->b[0x51] = 1;
        put32(w, 0x70, 1315); put32(w, 0x74, 400 + 100 * i); put32(w, 0x78, 70); put32(w, 0x7c, 70);
        static const int32_t fr[4] = { 0, 1, 3, 2 }; memcpy(w->b + 0x6e8, fr, sizeof fr); }

    put64(&m->ltext, 0, A(main_mem) + DM_VT_TEXT);
    car_fake(box("TouchCustom_lang")->b, A(main_mem) + DM_VT_CAROUSEL, 7, 0, A(&m->ltext)); put32(box("TouchCustom_lang"), 0x70, 685); put32(box("TouchCustom_lang"), 0x74, 202); put32(box("TouchCustom_lang"), 0x78, 700); put32(box("TouchCustom_lang"), 0x7c, 72);
    car_fake(box("TouchCustom_touch_swap_above")->b, A(main_mem) + DM_VT_CAROUSEL, 2, 0, 0); car_fake(box("TouchCustom_cam")->b, A(main_mem) + DM_VT_CAROUSEL, 3, 0, 0);
    put64(box("TouchCustom_lang_FontStd"), 0, A(main_mem) + DM_VT_TEXT); put32(box("TouchCustom_lang_FontStd"), 0xb4, FACE_STD);
    put64(box("TouchCustom_lang_FontCjk"), 0, A(main_mem) + DM_VT_TEXT); put32(box("TouchCustom_lang_FontCjk"), 0xb4, FACE_CJK);

    { widget *t = &m->box[NROW - 2]; put64(t, 0, A(main_mem) + DM_VT_TEXT); put32(t, 0x70, 0); put32(t, 0x74, 180); put32(t, 0x78, 1385); put32(t, 0x7c, 180);
      widget *f = &m->box[NROW - 1]; put64(f, 0, A(sel_vt)); put32(f, 0x70, 99999); put32(f, 0x74, 0); put32(f, 0x78, 1000); put32(f, 0x7c, 100); }
    cur = m; building = NULL; root_kids[0] = A(&m->page); put64(&root_w, 0x60, 1);
}
static void page_gone(void) { put64(&root_w, 0x60, 0); }
static int on(const char *name) { return box(name)->b[0xa88] != 0; }
static int dirty(void) { return cur->page.b[0x1a0]; }
static void click(const char *name) { widget *w = box(name); w->b[0xa88] = !w->b[0xa88]; }
static int pend(const char *key) { int i = key_index(key); return atomic_load(&g_pend[i]); }
static int none_pending(void) { for (int i = 0; i < NK; i++) if (atomic_load(&g_pend[i]) >= 0) return 0; return 1; }
static int sel(const char *name) { return car_index(box(name)->b); }
static int face(void) { int32_t a, b; memcpy(&a, box("TouchCustom_lang")->b + 0x800, 4); memcpy(&b, cur->ltext.b + 0xb4, 4); return a == b ? a : -1; }


static int laid_out(void) {
    return on("TouchCustom_auto_mana") && on("TouchCustom_merc_pot") && on("TouchCustom_map_mark") && sel("TouchCustom_touch_swap_above") == 0 && sel("TouchCustom_cam") == 0
        && sel("TouchCustom_lang") == 1 && face() == FACE_STD; }


static int in_tree(uint64_t w, uint64_t want, int dep) {
    if (!w || dep > 14) return 0;
    if (w == want) return 1;
    uint64_t ch = *(uint64_t *)(uintptr_t)(w + 0x58), n = *(uint64_t *)(uintptr_t)(w + 0x60);
    for (uint64_t i = 0; ch && i < n; i++) if (in_tree(*(uint64_t *)(uintptr_t)(ch + 8 * i), want, dep + 1)) return 1;
    return 0;
}
uint64_t dm_wref_get(dm_wref *r) { if (!r->w) return 0; if (r->gen == generation) return r->w; if (in_tree(root(), r->w, 0)) { r->gen = generation; return r->w; } return 0; }

static void step(void) { frame++; dm_custom_ui_pump(root()); }





static void rebuild(int reuse, int gen) { if (gen) generation++; build_page(reuse ? cur : cur == &mem_a ? &mem_b : &mem_a); }
static void switch_layout(int to_pad, int reuse, int gen, int gap) {
    busy = 1; step();
    pad = to_pad; step();
    rebuild(reuse, gen); step();
    if (gap) { page_gone(); step(); root_kids[0] = A(&cur->page); put64(&root_w, 0x60, 1); }
    step(); step();
    rebuild(reuse, gen); step();
    for (int i = 0; i < 4; i++) step();
    busy = 0;
}
static void open_page(void) { generation++; build_page(&mem_a); step(); step(); }
static void reset_all(void) { dm_custom_on_uimsg("OptionsPanelMessage", "CancelChanges", ""); step(); }

int main(void) {
    const char *td = getenv("TMPDIR"); char tmpl[1024]; snprintf(tmpl, sizeof tmpl, "%s/t_custom_relayout.XXXXXX", td && *td ? td : "/tmp");
    char *d = mkdtemp(tmpl); if (!d) { perror("mkdtemp"); return 1; }
    dm_custom_init(d);
    strlcpy(g_mod[0].name, "main", sizeof g_mod[0].name); g_mod[0].base = main_mem; g_nmod = 1;
    page_vt[0] = A(main_mem) + GA_OPTIONS_PAGE_UPDATE; sel_vt[2] = A(relayout_noop);
    root_w.b[0x50] = 1; put64(&root_w, 0x58, A(root_kids));


    open_page();
    CHECK(on("TouchCustom_auto_mana") && on("TouchCustom_merc_pot") && on("TouchCustom_drop_light") && on("TouchCustom_map_mark"), "页刚出现：缺省开的几格应勾上");
    CHECK(sel("TouchCustom_lang") == 1 && face() == FACE_STD, "语言行应选在正在跑的 Deutsch（第 %d 项），缺省字体（%d）", sel("TouchCustom_lang"), face());

    car_tap(box("TouchCustom_cam")->b, 1); step();
    CHECK(pend("cam") == 1 && dirty(), "镜头 ▶ = 待定 1（%d）", pend("cam"));
    car_tap(box("TouchCustom_cam")->b, 1); car_tap(box("TouchCustom_cam")->b, 1); step();
    CHECK(sel("TouchCustom_cam") == 0 && pend("cam") == 0 && !dirty(), "绕回原版：待定 = 生效值，「应用」不亮（%d）", pend("cam"));
    reset_all();
    CHECK(!dirty(), "刚打开不该有改动");
    click("TouchCustom_drop_light"); step();
    CHECK(pend("drop_light") == 0 && dirty(), "玩家点掉 drop_light 应记成待定（%d），「应用」点亮", pend("drop_light"));


    switch_layout(1, 1, 1, 0); step();
    CHECK(pend("auto_mana") == -1 && pend("merc_pot") == -1 && pend("map_mark") == -1 && pend("touch_swap_above") == -1 && pend("cam") == -1 && pend("lang") == -1, "原地重建后不该有待定改动（auto_mana %d merc_pot %d map_mark %d touch_swap_above %d cam %d）", pend("auto_mana"), pend("merc_pot"), pend("map_mark"), pend("touch_swap_above"), pend("cam"));
    CHECK(laid_out(), "重建后的页应按生效值重摆（开关行、两项与三项单选、语言）");
    CHECK(pend("drop_light") == 0 && !on("TouchCustom_drop_light"), "切换前玩家的改动应留着并照样显示（%d）", pend("drop_light"));
    CHECK(dirty(), "仍有玩家的改动：「应用」应亮着");


    reset_all(); click("TouchCustom_drop_light"); step();
    switch_layout(0, 0, 1, 1); step();
    CHECK(pend("drop_light") == 0, "切换中页暂时不在，玩家的改动不该丢（%d）", pend("drop_light"));
    CHECK(pend("auto_mana") == -1 && on("TouchCustom_auto_mana") && !on("TouchCustom_drop_light"), "换地址重建后照样按值重摆");
    click("TouchCustom_map_mark"); step();
    CHECK(pend("map_mark") == 0, "切换完成后点击照常算数（%d）", pend("map_mark"));
    reset_all();
    CHECK(pend("drop_light") == -1 && pend("map_mark") == -1 && on("TouchCustom_drop_light") && on("TouchCustom_map_mark") && !dirty(), "「放弃」后回到生效值");


    switch_layout(0, 1, 0, 0); step();
    CHECK(none_pending() && laid_out() && !dirty(), "代号没变的原地重建后不该有待定改动、应按值重摆");

    click("TouchCustom_merc_pot"); step();
    reloading = 1; step(); rebuild(1, 0); step(); step(); reloading = 0; step();
    CHECK(pend("merc_pot") == 0 && pend("auto_mana") == -1 && pend("map_mark") == -1 && !on("TouchCustom_merc_pot") && on("TouchCustom_auto_mana"), "晚到的重建：玩家的改动留着（%d），别的不冒（auto_mana %d）", pend("merc_pot"), pend("auto_mana"));

    page_gone(); step();
    CHECK(none_pending(), "切换后关页应丢掉没应用的改动");
    root_kids[0] = A(&cur->page); put64(&root_w, 0x60, 1); step();



    switch_layout(1, 1, 1, 0);
    key_ev = DM_BTN_A; step();
    { int32_t *r = (int32_t *)(uintptr_t)(A(box("TouchCustomSel")) + 0x70);
      CHECK(r[0] == -30 && r[1] == 170 && r[2] == 1385 + 30 + 30 && r[3] == 180 + 20, "语言行的选中框应框住整行，实际 %d,%d %dx%d", r[0], r[1], r[2], r[3]); }
    CHECK(pend("lang") == 2 && sel("TouchCustom_lang") == 2 && face() == FACE_STD, "语言行上按 A：Deutsch → Español（待定 %d，第 %d 项）", pend("lang"), sel("TouchCustom_lang"));


    { int r0 = car_refreshes; key_ev = DM_BTN_LEFT; step(); key_ev = DM_BTN_LEFT; step(); key_ev = DM_BTN_LEFT; step();
      CHECK(pend("lang") == 0 && sel("TouchCustom_lang") == 6 && face() == FACE_CJK && car_refreshes > r0, "◀ 绕到中文：待定 %d、第 %d 项、字体 %d", pend("lang"), sel("TouchCustom_lang"), face()); }
    switch_layout(0, 1, 1, 0); step();
    CHECK(pend("lang") == 0 && sel("TouchCustom_lang") == 6 && face() == FACE_CJK, "切换后：中文照样显示、BlizzardGlobal（第 %d 项、字体 %d）", sel("TouchCustom_lang"), face());
    reset_all();
    CHECK(pend("lang") == -1 && sel("TouchCustom_lang") == 1 && face() == FACE_STD, "放弃：回到 Deutsch、缺省字体（第 %d 项、字体 %d）", sel("TouchCustom_lang"), face());
    switch_layout(1, 1, 1, 0);


    key_ev = DM_BTN_DOWN; step(); key_ev = DM_BTN_DOWN; step(); key_ev = DM_BTN_A; step(); step();
    CHECK(pend("merc_pot") == 0, "第三行（merc_pot）按 A 应记成待定（%d）", pend("merc_pot"));
    switch_layout(0, 1, 1, 0); switch_layout(1, 1, 1, 0);
    key_ev = DM_BTN_A; step(); step();
    CHECK(pend("merc_pot") == 0 && pend("lang") == 2, "切换后 A 应落在第一行（语言 %d），merc_pot 不变（%d）", pend("lang"), pend("merc_pot"));


    reset_all(); switch_layout(0, 1, 1, 0); switch_layout(1, 1, 1, 0); step();
    { int32_t *r = (int32_t *)(uintptr_t)(A(box("TouchCustomSel")) + 0x70); int y_cmp = 400 + 100 * 5, y_pot = 400 + 100 * 6, y_cam = 400 + 100 * 8;
      for (int i = 0; i < 5; i++) { key_ev = DM_BTN_DOWN; step(); }
      CHECK(r[1] == y_cmp - 10, "five down: item_cmp (frame top %d)", r[1]);
      key_ev = DM_BTN_DOWN; step();
      CHECK(r[1] == y_pot - 10, "down from item_cmp skips the missing iCloud row onto pot_pick (frame top %d)", r[1]);
      key_ev = DM_BTN_UP; step();
      CHECK(r[1] == y_cmp - 10, "up from pot_pick: back on item_cmp (frame top %d)", r[1]);
      for (int i = 0; i < 12; i++) { key_ev = DM_BTN_DOWN; step(); }
      CHECK(r[1] == y_cam - 10, "down to the end: the last row, camera (frame top %d)", r[1]);
      CHECK(none_pending(), "moving over the rows changes nothing"); }

    char p[1100]; snprintf(p, sizeof p, "%s/sw_custom.txt", d); unlink(p); rmdir(d);
    printf("t_custom_relayout：%d 项，%d 项不通\n", checks, fails); return fails != 0;
}
