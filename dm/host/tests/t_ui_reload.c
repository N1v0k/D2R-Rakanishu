










#include "../dm_cursor.c"
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
static widget root, settings, other, other2;
static uint64_t root_kids[2], vt_plain[16];
static uint64_t P(const void *p) { return (uint64_t)(uintptr_t)p; }
static void make(widget *w, const char *name) { memset(w, 0, sizeof *w); *(const char **)(w->b + 8) = name; *(uint64_t **)(w->b) = vt_plain; w->b[0x50] = w->b[0x51] = 1; }
static void next_frame(void) { frame++; (void)dm_ui_gen(); }
static void swap_top(void) { root_kids[1] = root_kids[1] == P(&other) ? P(&other2) : P(&other); }
static int frames_reloading(int max) { int n = 0; while (n < max && dm_ui_reloading()) { n++; next_frame(); } return n; }

int main(void) {
    unsigned char *base = calloc(1, 0x2f05460); if (!base) return 2;
    g_nmod = 1; strcpy(g_mod[0].name, "main"); g_mod[0].base = base;
    *(int32_t *)(base + 0x211da38) = 1920; *(int32_t *)(base + 0x211da3c) = 1080;
    make(&root, "Root"); make(&settings, "SettingsPanel"); make(&other, "SaveIndicatorPanel"); make(&other2, "SaveIndicatorPanel");
    root_kids[0] = P(&settings); root_kids[1] = P(&other); *(uint64_t *)(root.b + 0x58) = P(root_kids); *(uint64_t *)(root.b + 0x60) = 2;
    *(uint64_t *)(base + 0x2f05458) = P(&root);
    next_frame(); next_frame();
    CHECK(!dm_ui_reloading(), "开始时不在重载");


    dm_ui_gen_bump(DM_UIGEN_PERFMODE);
    CHECK(frames_reloading(100) == 100, "性能模式变了：等重建期间应一直答 1");
    swap_top(); next_frame();
    int n = frames_reloading(300); CHECK(n == 2, "看到重建后应再答 2 帧（答了 %d 帧）", n);

    for (int i = 0; i < 5; i++) next_frame();
    uint32_t g0 = dm_ui_gen(); dm_wref ref; dm_wref_set(&ref, P(&settings));
    dm_ui_gen_bump(DM_UIGEN_RESIZE);
    CHECK(dm_ui_gen() == g0 + 1, "改尺寸也要加界面代号（%u → %u）", g0, dm_ui_gen());
    n = frames_reloading(300); CHECK(n == 2, "改尺寸只该答 2 帧（答了 %d 帧；原先按整套重载答 240 帧）", n);
    for (int i = 0; i < 10; i++) next_frame();
    CHECK(!dm_ui_reloading(), "改尺寸之后不该再答 1");
    CHECK(dm_wref_get(&ref) == P(&settings), "改尺寸后还挂在树上的控件引用应照用");

    swap_top(); next_frame(); CHECK(!dm_ui_reloading(), "改尺寸后的普通面板更换不该开重载窗口");


    for (int i = 0; i < 5; i++) next_frame();
    dm_ui_gen_bump(DM_UIGEN_LANG); for (int i = 0; i < 3; i++) next_frame();
    dm_ui_gen_bump(DM_UIGEN_RESIZE); for (int i = 0; i < 10; i++) next_frame();
    CHECK(dm_ui_reloading(), "改尺寸不该提前结束正在等的整套重载");
    swap_top(); next_frame(); n = frames_reloading(300); CHECK(n == 2, "整套重载的重建到了再答 2 帧（答了 %d 帧）", n);

    for (int i = 0; i < 5; i++) next_frame();
    dm_ui_gen_bump(DM_UIGEN_RESIZE); next_frame(); dm_ui_gen_bump(DM_UIGEN_OPMODE);
    CHECK(frames_reloading(50) == 50, "改尺寸后紧接着的整套重载应照等重建");
    swap_top(); next_frame(); n = frames_reloading(300); CHECK(n == 2, "重建到了再答 2 帧（答了 %d 帧）", n);


    for (int i = 0; i < 5; i++) next_frame();
    dm_ui_gen_bump(DM_UIGEN_PERFMODE); n = frames_reloading(1000); CHECK(n == 240, "等不到重建应答 240 帧（答了 %d 帧）", n);

    printf("t_ui_reload: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
