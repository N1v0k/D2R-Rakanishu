













#include "../dm_vault_ui.c"
#include "../game_addrs.h"
#include <stddef.h>

module_t g_mod[MAX_MOD];
int g_nmod;
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)


static uint32_t generation = 1; static int busy, reloading;
uint32_t dm_ui_gen(void) { return generation; }
int dm_padmode_busy(void) { return busy; }
int dm_ui_reloading(void) { return reloading; }
int dm_ui_top_chk_on(void) { return 0; }
void dm_ui_top_chk(uint64_t root, const char *name, uint64_t got) { (void)root; (void)name; (void)got; }

unsigned long long dm_guest_frame(void) { return 0; }
int dm_padmode_current(void) { return 0; }
uint64_t dm_padmode_page_keys(uint64_t m) { (void)m; return 0; }
uint64_t dm_padmode_page_held(void) { return 0; }
void dm_padmode_page_pulse(uint64_t m) { (void)m; }
void dm_padmode_vault_hint(int on) { (void)on; }
void dm_padmode_legend(int act, const char *t) { (void)act; (void)t; }
void dm_ui_touch(int phase, float x, float y) { (void)phase; (void)x; (void)y; }
int dm_ui_vault_drop(void) { return 0; }
int dm_cursor_item(dm_cursor_item_info *o) { (void)o; return 0; }
const char *dm_untag(const char *s) { return s; }
const char *dm_str(uint8_t *b, const char *k) { (void)b; return k; }
int dm_lang_zh(void) { return 0; }
int dm_vault_has(int kind, int idx) { (void)kind; (void)idx; return 0; }
int dm_vault_gen(void) { return 0; }
void dm_vault_request_deposit(uint32_t id) { (void)id; }
void dm_vault_request_desc(int kind, int row) { (void)kind; (void)row; }
int dm_vault_take_desc(char *o, size_t n) { (void)o; (void)n; return 0; }
int dm_vault_take_full_sound(void) { return 0; }


typedef struct { _Alignas(8) uint8_t b[0x1000]; } widget;
typedef struct { widget vt, box[3]; uint64_t kids[3]; } page_mem;
static uint8_t *base;
static widget root_w, bank, tabs;
static uint64_t root_kids[1], bank_kids[2];
static page_mem mem_a, mem_b;
static page_mem *cur;
static uint64_t vt_table[4]; static int relayouts;
static void relayout(uint64_t w) { (void)w; relayouts++; }
static uint64_t A(const void *p) { return (uint64_t)(uintptr_t)p; }
static void put64(widget *w, size_t at, uint64_t v) { memcpy(w->b + at, &v, 8); }
static void put32(widget *w, size_t at, int32_t v) { memcpy(w->b + at, &v, 4); }
static void named(widget *w, const char *name, uint64_t parent) { memset(w, 0, sizeof *w); put64(w, 8, A(name)); put64(w, 0x30, parent); w->b[0x50] = w->b[0x51] = 1; }
static int in_tree(uint64_t at, uint64_t want, int dep) {
    if (!at || dep > 16) return 0; if (at == want) return 1;
    uint64_t ch, n; memcpy(&ch, (void *)(uintptr_t)(at + 0x58), 8); memcpy(&n, (void *)(uintptr_t)(at + 0x60), 8);
    for (uint64_t i = 0; ch && i < n; i++) { uint64_t c; memcpy(&c, (void *)(uintptr_t)(ch + 8 * i), 8); if (in_tree(c, want, dep + 1)) return 1; }
    return 0;
}

uint64_t dm_wref_get(dm_wref *r) { if (!r->w) return 0; if (r->gen == generation) return r->w; if (in_tree(A(&root_w), r->w, 0)) { r->gen = generation; return r->w; } return 0; }

static const char *const k_box[3] = { "TouchV_T0", "TouchV_T1", "TouchV_T2" };
static void build_page(page_mem *m) {
    named(&m->vt, "TouchVault", A(&bank)); put64(&m->vt, 0, A(vt_table));
    for (int i = 0; i < 3; i++) { named(&m->box[i], k_box[i], A(&m->vt)); put64(&m->box[i], 0, A(base) + DM_VT_TOGGLE);
        put32(&m->box[i], 0x6e8, 0); put32(&m->box[i], 0x6ec, 1); put32(&m->box[i], 0x6f0, 3); put32(&m->box[i], 0x6f4, 2); m->kids[i] = A(&m->box[i]); }
    put64(&m->vt, 0x58, A(m->kids)); put64(&m->vt, 0x60, 3);
    cur = m; bank_kids[1] = A(&m->vt);
}
static void build_tree(void) {
    named(&root_w, "Root", 0); named(&bank, "BankExpansionLayout", A(&root_w)); named(&tabs, "BankTabs", A(&bank)); put64(&tabs, 0, A(base) + DM_VT_TABBAR);
    root_kids[0] = A(&bank); put64(&root_w, 0x58, A(root_kids)); put64(&root_w, 0x60, 1);
    bank_kids[0] = A(&tabs); put64(&bank, 0x58, A(bank_kids)); put64(&bank, 0x60, 2);
    memcpy(base + GA_UI_ROOT, &(uint64_t){ A(&root_w) }, 8);
    put32(&tabs, 0xdc4, 5);
}
static int shown(void) { int m = 0; for (int i = 0; i < 3; i++) m |= (cur->box[i].b[0xa88] != 0) << i; return m; }
static int frames_ok(void) { for (int i = 0; i < 3; i++) { int32_t f; memcpy(&f, cur->box[i].b + 0x6e8, 4); if (f != (cur->box[i].b[0xa88] ? 4 : 0)) return 0; } return 1; }
static void tick(int i, int on) { cur->box[i].b[0xa88] = (uint8_t)on; }
static void visible_frame(void) { vu_tiers_pump(A(&cur->vt)); }
static void hidden_frame(int how) {
    if (how == 0) { bank.b[0x50] = 0; dm_vault_ui_pump(); bank.b[0x50] = 1; }
    else if (how == 1) { put32(&tabs, 0xdc4, 0); dm_vault_ui_pump(); put32(&tabs, 0xdc4, 5); }
    else if (how == 2) { put64(&bank, 0x60, 1); dm_vault_ui_pump(); put64(&bank, 0x60, 2); }
    else { put64(&tabs, 0, A(base) + DM_VT_TOGGLE); dm_vault_ui_pump(); put64(&tabs, 0, A(base) + DM_VT_TABBAR); }
}
static void rebuild_in_place(void) { build_page(cur); generation++; }

int main(void) {
    base = calloc(1, GA_UI_ROOT + 8); if (!base) return 2;
    g_nmod = 1; strcpy(g_mod[0].name, "main"); g_mod[0].base = base;
    vt_table[0x10 / 8] = A(relayout);
    build_tree(); build_page(&mem_a);


    visible_frame(); CHECK(shown() == 7 && g_tiers == 7 && frames_ok(), "首次出现：显示 %d，筛选 %d", shown(), g_tiers);

    tick(1, 0); visible_frame(); CHECK(g_tiers == 5 && shown() == 5, "去勾后筛选 %d，显示 %d", g_tiers, shown());

    for (int how = 0; how < 4; how++) { hidden_frame(how); CHECK(g_vt_fresh == 1, "页不在屏上（路 %d）要置重摆", how); visible_frame(); CHECK(g_tiers == 5 && shown() == 5 && frames_ok(), "路 %d 回来后筛选 %d，显示 %d", how, g_tiers, shown()); }

    hidden_frame(0); rebuild_in_place(); CHECK(shown() == 0, "前提：重建出来的页三个都不勾");
    visible_frame(); CHECK(g_tiers == 5 && shown() == 5 && frames_ok(), "同地址重建（有空档）后筛选 %d，显示 %d", g_tiers, shown());

    rebuild_in_place(); visible_frame(); CHECK(g_tiers == 5 && shown() == 5 && frames_ok(), "同地址重建（无空档）后筛选 %d，显示 %d", g_tiers, shown());

    busy = 1; reloading = 1; visible_frame(); rebuild_in_place(); visible_frame(); tick(0, 0); visible_frame(); CHECK(g_tiers == 5 && shown() == 5, "事务中点的勾作废：筛选 %d，显示 %d", g_tiers, shown());
    rebuild_in_place(); visible_frame(); busy = 0; visible_frame(); CHECK(g_tiers == 5 && shown() == 5 && frames_ok(), "两遍重建后（窗口还在）筛选 %d，显示 %d", g_tiers, shown());
    reloading = 0; visible_frame(); CHECK(g_tiers == 5 && shown() == 5, "窗口结束那一帧再摆一次：筛选 %d，显示 %d", g_tiers, shown());
    tick(0, 0); visible_frame(); CHECK(g_tiers == 4 && shown() == 4, "窗口过后去勾照常认：筛选 %d，显示 %d", g_tiers, shown());
    tick(0, 1); visible_frame(); CHECK(g_tiers == 5, "再勾回：筛选 %d", g_tiers);

    reloading = 1; visible_frame(); rebuild_in_place(); visible_frame(); reloading = 0; visible_frame(); CHECK(g_tiers == 5 && shown() == 5 && frames_ok(), "晚到的重建后筛选 %d，显示 %d", g_tiers, shown());

    build_page(&mem_b); generation++; visible_frame(); CHECK(g_tiers == 5 && shown() == 5 && frames_ok(), "换地址重建后筛选 %d，显示 %d", g_tiers, shown());
    build_page(&mem_a); generation++; visible_frame(); CHECK(g_tiers == 5 && shown() == 5, "换回旧地址（当年那块）后筛选 %d，显示 %d", g_tiers, shown());

    tick(0, 0); tick(2, 0); visible_frame(); CHECK(g_tiers == 7 && shown() == 0, "三个都不勾：筛选 %d（全看），显示 %d", g_tiers, shown());
    hidden_frame(1); visible_frame(); CHECK(g_tiers == 7 && shown() == 7, "再出现：筛选 %d，显示 %d（全勾）", g_tiers, shown());

    for (int want = 1; want <= 7; want++) {
        for (int i = 0; i < 3; i++) tick(i, want >> i & 1); visible_frame();
        rebuild_in_place(); visible_frame(); CHECK(g_tiers == want && shown() == want && frames_ok(), "筛选 %d 同地址重建后变成 %d，显示 %d", want, g_tiers, shown());
        rebuild_in_place(); hidden_frame(want & 3); visible_frame(); CHECK(g_tiers == want && shown() == want, "筛选 %d 重建 + 页不在一帧后变成 %d，显示 %d", want, g_tiers, shown()); }
    printf("t_vault_tiers: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
