










#include "../dm_runeword.c"
module_t g_mod[MAX_MOD]; int g_nmod;
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static uint32_t generation = 1;

int dm_diag_ladder(int *offline, int *ladder) { *offline = *ladder = 0; return 0; }
int dm_held_size(int *w, int *h) { *w = *h = 0; return 0; }
int dm_rw_allowed(uint8_t bits, int16_t first, int16_t last, int known, int offline, int ladder) { (void)bits; (void)first; (void)last; (void)known; (void)offline; (void)ladder; return 0; }
int dm_rw_find_lvoff(const uint8_t *rows, uint32_t n, uint32_t rowsize) { (void)rows; (void)n; (void)rowsize; return -1; }
int dm_rw_need_level(int base_req, const int *rune_req, int n) { (void)rune_req; (void)n; return base_req; }
int dm_textbox_ensure_loaded(void) { return 0; }
int dm_textbox_rewrite(const char *s, size_t n, char **out, size_t *on) { (void)s; (void)n; (void)out; (void)on; return 0; }
const char *dm_untag(const char *s) { return s; }
int dm_padmode_current(void) { return 0; }
typedef struct { _Alignas(8) unsigned char b[0x100]; } widget;
static widget root, inv, kid[5], moved, other_inv;
static uint64_t root_kids[1], inv_kids[5];
static uint64_t vt[4]; static int relayouts;
static unsigned char *base;
static const char *const k_names[5] = { "TouchRwBg", "TouchRwTitle", "TouchRwName0", "TouchRwGo0", "TouchRwR0_0w" };
static uint64_t P(const void *p) { return (uint64_t)(uintptr_t)p; }
static void relayout(uint64_t w) { (void)w; relayouts++; }
static int in_tree(uint64_t at, uint64_t want, int dep) {
    if (!at || dep > 16) return 0; if (at == want) return 1;
    uint64_t ch = *(uint64_t *)(uintptr_t)(at + 0x58), n = *(uint64_t *)(uintptr_t)(at + 0x60);
    for (uint64_t i = 0; ch && i < n; i++) if (in_tree(*(uint64_t *)(uintptr_t)(ch + 8 * i), want, dep + 1)) return 1;
    return 0;
}

uint32_t dm_ui_gen(void) { return generation; }
int dm_ui_gen_revalidate(uint32_t *gen, uint64_t anchor) { if (*gen == generation) return 1; if (in_tree(P(&root), anchor, 0)) { *gen = generation; return 1; } return 0; }

static void make(widget *w, const char *name, int x) { memset(w, 0, sizeof *w); *(const char **)(w->b + 8) = name; *(uint64_t **)(w->b) = vt; w->b[0x50] = w->b[0x51] = 1; *(int32_t *)(w->b + 0x70) = x; }
static void build(widget *panel) {
    make(panel, "PlayerInventoryExpansionLayout", 0);
    for (int i = 0; i < 5; i++) { make(&kid[i], k_names[i], 10); inv_kids[i] = P(&kid[i]); }
    *(uint64_t *)(panel->b + 0x58) = P(inv_kids); *(uint64_t *)(panel->b + 0x60) = 5;
    root_kids[0] = P(panel);
}
static int32_t x_of(widget *w) { return *(int32_t *)(w->b + 0x70); }
static void open_list(void) { ui_cache(base); g_list.open = 1; }

int main(void) {
    base = calloc(1, 0x2f05460); if (!base) return 2;
    g_nmod = 1; strcpy(g_mod[0].name, "main"); g_mod[0].base = base;
    vt[0x10 / 8] = P(relayout);
    make(&root, "Root", 0); *(uint64_t *)(root.b + 0x58) = P(root_kids); *(uint64_t *)(root.b + 0x60) = 1; *(uint64_t *)(base + 0x2f05458) = P(&root);
    build(&inv); g_inv_open = 1;


    open_list(); CHECK(W.panel == P(&inv) && W.bg == P(&kid[0]) && W.name[0] == P(&kid[2]), "缓存找齐");
    generation++; CHECK(!w_stale() && W.ui_gen == generation, "没动的缓存不该作废");

    make(&moved, "TouchRwBg", 10); inv_kids[0] = P(&moved); generation++;
    CHECK(w_stale(), "子控件换了地址应作废"); CHECK(w_stale(), "再问一次仍作废（代号没被记上）");

    for (int i = 0; i < 5; i++) *(int32_t *)(kid[i].b + 0x70) = 10;
    dm_runeword_on_uimsg("PanelManager", "ClosePanel", "PlayerInventoryExpansionLayout");
    CHECK(!g_list.open && x_of(&kid[0]) == 10 && x_of(&kid[1]) == 10 && x_of(&kid[2]) == 10, "旧子控件不该被写（Bg x %d、Title x %d、Name0 x %d）", x_of(&kid[0]), x_of(&kid[1]), x_of(&kid[2]));
    CHECK(W.panel == 0, "收起后缓存清空");

    g_inv_open = 1; build(&inv); open_list(); root_kids[0] = P(&other_inv); make(&other_inv, "PlayerInventoryExpansionLayout", 0); generation++;
    CHECK(w_stale(), "面板不在树上应作废");

    build(&inv); g_inv_open = 1; open_list(); build(&inv); generation++;
    CHECK(!w_stale(), "原地重建且全对得上应照用");
    dm_runeword_on_uimsg("PanelManager", "ClosePanel", "PlayerInventoryExpansionLayout");
    CHECK(x_of(&kid[0]) == OFF && x_of(&kid[2]) == OFF, "活控件应挪出屏（Bg x %d、Name0 x %d）", x_of(&kid[0]), x_of(&kid[2]));

    build(&inv); g_inv_open = 1; open_list(); *(uint64_t *)(inv.b + 0x60) = 4; generation++;
    CHECK(w_stale(), "少了子控件应作废");
    printf("t_rw_stale: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
