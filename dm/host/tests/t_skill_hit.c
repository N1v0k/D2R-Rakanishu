











#include "../dm_cursor.c"
#include "../game_addrs.h"
module_t g_mod[MAX_MOD]; int g_nmod;
static int busy; static uint64_t frame = 100;
uint64_t dm_guest_frame(void) { return frame; }
uint64_t now_ns(void) { return frame * 16666667; }
int dm_padmode_busy(void) { return busy; }
int dm_uiscreen_ingame(void) { return 1; }
void dm_ui_rect(int *x, int *y, int *w, int *h) { *x = 0; *y = 0; *w = 1920; *h = 1080; }
void dm_uiscreen_set_ingame(int on) { (void)on; }
void dm_custom_on_uimsg(const char *cat, const char *act, const char *arg) { (void)cat; (void)act; (void)arg; }
void dm_elysium_arm(int on) { (void)on; }
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

typedef struct { _Alignas(8) unsigned char b[0x200]; } widget;
static widget root, tree, other, other2, btn[3];
static uint64_t root_kids[2], tree_kids[2];
static uint64_t vt_btn[16], vt_plain[16];
static unsigned char *base;
static uint64_t P(const void *p) { return (uint64_t)(uintptr_t)p; }
static uint64_t hit_fn(uint64_t w, uint64_t xy) { int x = (int)(uint32_t)xy, x0 = *(int32_t *)(uintptr_t)(w + 0x70); return x >= x0 && x < x0 + 100; }
static void make(widget *w, const char *name, uint64_t *vt) { memset(w, 0, sizeof *w); *(const char **)(w->b + 8) = name; *(uint64_t **)(w->b) = vt; w->b[0x50] = w->b[0x51] = 1; }
static void place_btn(widget *w, const char *name, int x0) { make(w, name, vt_btn); *(int32_t *)(w->b + 0x70) = x0; }
static void tree_set(widget *a, widget *b) { tree_kids[0] = P(a); tree_kids[1] = P(b); *(uint64_t *)(tree.b + 0x58) = P(tree_kids); *(uint64_t *)(tree.b + 0x60) = 2; }
static void next_frame(void) { frame++; (void)dm_ui_gen(); }
static uint32_t tap_at(int x) { next_frame(); bd_note_skill(&g_mod[0], P(&root), x, 500); return dm_ui_hit_id(); }


static uint32_t selected_point; static int spend_pending, spends, refused;
static void front_tap(int x) { uint32_t id = tap_at(x); if (id && id == selected_point) spend_pending = 1; else selected_point = id; }
static void front_tick(void) {
    next_frame(); if (!spend_pending) return; spend_pending = 0; if (dm_ui_skill_spend_ok()) spends++; else refused++; }
static void swap_top(void) { root_kids[1] = root_kids[1] == P(&other) ? P(&other2) : P(&other); }
static void rebuild_in_place(const char *new_name) {
    dm_ui_gen_bump(DM_UIGEN_OPMODE); next_frame();
    swap_top(); next_frame();
    place_btn(&btn[0], new_name, 100); place_btn(&btn[1], "SkillButton2", 300); tree_set(&btn[0], &btn[1]);
    for (int i = 0; i < 4; i++) next_frame();
}

int main(void) {
    base = calloc(1, GA_UI_ROOT + 8); if (!base) return 2;
    g_nmod = 1; strcpy(g_mod[0].name, "main"); g_mod[0].base = base;
    *(int32_t *)(base + GA_UI_WIDTH) = 1920; *(int32_t *)(base + GA_UI_HEIGHT) = 1080;
    vt_btn[0x48 / 8] = P(hit_fn);
    make(&root, "Root", vt_plain); make(&tree, "SkillsTreePanel", vt_plain); make(&other, "SaveIndicatorPanel", vt_plain); make(&other2, "SaveIndicatorPanel", vt_plain);
    root_kids[0] = P(&tree); root_kids[1] = P(&other); *(uint64_t *)(root.b + 0x58) = P(root_kids); *(uint64_t *)(root.b + 0x60) = 2;
    *(uint64_t *)(base + GA_UI_ROOT) = P(&root);
    place_btn(&btn[0], "SkillButton1", 100); place_btn(&btn[1], "SkillButton2", 300); tree_set(&btn[0], &btn[1]);
    next_frame();


    front_tap(150); front_tap(150); front_tick(); CHECK(spends == 1 && refused == 0, "同一个图标连点两下应加 1 点（加了 %d，拒 %d）", spends, refused);
    uint32_t a = tap_at(150), b = tap_at(350); CHECK(a && b && a != b && (a >> 31) && (b >> 31), "两个图标编号不同且带最高位：%x %x", a, b);
    CHECK(tap_at(150) != b, "回到 A 不等于 B 的编号");

    selected_point = 0; spends = refused = 0; front_tap(150); uint32_t before = selected_point;
    rebuild_in_place("SkillButton7");
    front_tap(150); front_tick(); CHECK(spends == 0 && selected_point != before, "同地址新图标点一下不该加点（加了 %d；编号 %x → %x）", spends, before, selected_point);
    front_tap(150); front_tick(); CHECK(spends == 1, "新图标再点一下才加（加了 %d）", spends);

    spends = refused = 0; selected_point = 0; front_tap(150); front_tap(150); rebuild_in_place("SkillButton9"); front_tick();
    CHECK(spends == 0 && refused == 1, "确认后重建，延迟加点应拒（加 %d，拒 %d）", spends, refused);

    spends = refused = 0; selected_point = 0; front_tap(150); front_tap(150); busy = 1; front_tick(); busy = 0; CHECK(refused == 1 && spends == 0, "事务中应拒（加 %d，拒 %d）", spends, refused);
    selected_point = 0; front_tap(150); front_tap(150); dm_ui_gen_bump(DM_UIGEN_LANG); front_tick(); CHECK(refused == 2, "重载窗口里应拒（拒 %d）", refused);
    for (int i = 0; i < 300; i++) next_frame();
    selected_point = 0; front_tap(150); front_tap(150); front_tick(); CHECK(spends == 1, "窗口过后照常加（加 %d）", spends);

    spends = refused = 0; selected_point = 0; front_tap(150); *(uint64_t *)(root.b + 0x60) = 1; next_frame(); *(uint64_t *)(root.b + 0x60) = 2;
    front_tap(150); front_tick(); CHECK(spends == 0, "面板卸掉后第二下不加（加 %d）", spends);
    front_tap(150); front_tick(); CHECK(spends == 1, "第三下加（加 %d）", spends);

    spends = 0; selected_point = 0; front_tap(150); uint32_t old = selected_point; dm_ui_gen_bump(DM_UIGEN_OPMODE); next_frame(); swap_top(); place_btn(&btn[2], "SkillButton1", 100); tree_set(&btn[2], &btn[1]); for (int i = 0; i < 6; i++) next_frame();
    front_tap(150); front_tick(); CHECK(selected_point != old && spends == 0, "换地址后第一下只选中（编号 %x → %x，加 %d）", old, selected_point, spends);
    front_tap(150); front_tick(); CHECK(spends == 1, "换地址后第二下加（加 %d）", spends);

    memset(&g_bd_inst, 0, sizeof g_bd_inst); CHECK(dm_ui_skill_spend_ok() == 1, "没有技能图标记录时应放行");
    printf("t_skill_hit: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
