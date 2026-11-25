










#include "../dm_recipes.c"
#include <stdio.h>

static int checks, fails;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)

_Atomic int g_dm_lang = 1;
static uint32_t epoch_now = 1;
uint32_t dm_diag_epoch(void) { return epoch_now; }
uint64_t dm_diag_game(void) { return 0; }
uint64_t dm_diag_player(void) { return 0; }
void dm_held_stow(void) {}
void dm_hud_get(dm_hud_info *o) { memset(o, 0, sizeof *o); }
void dm_item_use(uint32_t id) { (void)id; }
int dm_pages_char(char *out, size_t n) { (void)out; (void)n; return -1; }
int dm_pages_count(void) { return 0; }
int dm_unsocket_ready(void) { return 0; }
int dm_pages_cur(void) { return 0; }
int dm_pouch_each(void (*cb)(uint32_t, void *), void *a) { (void)cb; (void)a; return -1; }
int dm_vault_each(void (*cb)(int, int, void *), void *a) { (void)cb; (void)a; return -1; }
uint64_t dm_rw_held(uint8_t *b, uint64_t pl) { (void)b; (void)pl; return 0; }
uint64_t dm_rw_item(uint8_t *b, uint32_t id) { (void)b; (void)id; return 0; }
int dm_rw_owners(uint8_t *b, uint64_t pl, uint64_t us[3]) { (void)b; (void)pl; us[0] = us[1] = us[2] = 0; return 3; }
int dm_rw_panel_open(uint8_t *b) { (void)b; return 0; }
void dm_rw_send(uint8_t *b, const uint8_t *p, int n) { (void)b; (void)p; (void)n; }
void dm_rw_take_stack(uint8_t *b, uint64_t it) { (void)b; (void)it; }
int dm_ui_panel_shown(const char *name) { (void)name; return 0; }
int dm_rw_text(uint8_t *b, int rec, char *out, size_t n) { (void)b; (void)rec; (void)out; (void)n; return 0; }



static int desc_calls, later_left;
static void (*during)(void);
int dm_item_desc(uint8_t *b, int kind, int row, int grp, char *out, size_t n) {
    (void)b; desc_calls++;
    if (during) { void (*f)(void) = during; during = NULL; f(); }
    if (row == 7) { out[0] = 0; return 0; }
    if (row == 8 && later_left > 0) { later_left--; out[0] = 0; return DM_ITEM_DESC_LATER; }
    return snprintf(out, n, "K%d R%d G%d L%d\nzweite Zeile", kind, row, grp, atomic_load(&g_dm_lang));
}

static uint8_t game[16], player[0x200];
static void pump(void) { item_pump(game, (uint64_t)(uintptr_t)player); }
static int ask(int kind, int row, int grp, char *t, int n) { return dm_item_text(kind, row, grp, t, n); }
static void ask_other(void) { char t[64]; ask(1, 300, -1, t, sizeof t); }
static void switch_lang(void) { atomic_store(&g_dm_lang, 0); }

int main(void) {
    player[0x1bd] = 3;
    char t[256];


    CHECK(ask(1, 248, -1, t, sizeof t) == 0, "miss answers 0");
    CHECK(desc_calls == 0, "nothing made on the UI thread");
    pump();
    CHECK(desc_calls == 1, "made on the next pump");
    CHECK(ask(1, 248, -1, t, sizeof t) > 0 && !strcmp(t, "K1 R248 G-1 L1\nzweite Zeile"), "cached text handed out whole");
    pump();
    CHECK(desc_calls == 1, "nothing made without a request");


    ask(0, 88, -1, t, sizeof t); ask(2, 60, 0, t, sizeof t);
    pump();
    CHECK(desc_calls == 2 && ask(2, 60, 0, t, sizeof t) > 0 && !strncmp(t, "K2 R60 G0", 9), "the later ask was made");
    CHECK(ask(0, 88, -1, t, sizeof t) == 0, "the replaced one is still missing");
    pump();
    CHECK(desc_calls == 3 && ask(0, 88, -1, t, sizeof t) > 0, "made once asked again");
    CHECK(ask(2, 60, 1, t, sizeof t) == 0, "another rune bonus group is another text");


    ask(1, 7, -1, t, sizeof t); pump();
    int before = desc_calls;
    CHECK(ask(1, 7, -1, t, sizeof t) == DM_ITEM_TEXT_NONE, "none answered as none");
    pump(); pump();
    CHECK(desc_calls == before, "none is not made again");



    later_left = 2; ask(1, 8, -1, t, sizeof t);
    pump(); CHECK(ask(1, 8, -1, t, sizeof t) == 0, "not now: still pending, not none");
    pump(); CHECK(ask(1, 8, -1, t, sizeof t) == 0, "not now twice: still pending");
    pump(); CHECK(ask(1, 8, -1, t, sizeof t) > 0, "made once the game can");
    ask(1, 9, -1, t, sizeof t); before = desc_calls;
    item_pump(game, 0); CHECK(desc_calls == before, "no player: nothing made");
    player[0x1bd] = 2; pump(); CHECK(desc_calls == before, "data version 2: nothing made"); player[0x1bd] = 3;
    atomic_store(&g_jst, 1); pump(); CHECK(desc_calls == before, "cube job running: nothing made"); atomic_store(&g_jst, 0);
    CHECK(ask(1, 9, -1, t, sizeof t) == 0, "still pending after the waits");
    pump(); CHECK(ask(1, 9, -1, t, sizeof t) > 0, "made after the waits");


    atomic_store(&g_dm_lang, 0);
    CHECK(ask(1, 248, -1, t, sizeof t) == 0, "after a language switch the old text is gone");
    CHECK(ask(1, 7, -1, t, sizeof t) == 0, "after a language switch an old none is gone too");
    ask(1, 248, -1, t, sizeof t); pump();
    CHECK(ask(1, 248, -1, t, sizeof t) > 0 && strstr(t, " L0\n") != NULL, "made again in the new language");
    atomic_store(&g_dm_lang, 1);


    ask(1, 248, -1, t, sizeof t); pump();
    CHECK(ask(1, 248, -1, t, sizeof t) > 0, "there before the run ends");
    epoch_now++;
    CHECK(ask(1, 248, -1, t, sizeof t) == 0, "gone after the run ended");
    pump(); CHECK(ask(1, 248, -1, t, sizeof t) > 0, "made again in the new run");


    ask(1, 100, -1, t, sizeof t);
    during = switch_lang; pump();
    CHECK(ask(1, 100, -1, t, sizeof t) == 0, "made during a language switch: not handed out");
    pump();
    CHECK(ask(1, 100, -1, t, sizeof t) > 0 && strstr(t, " L0\n") != NULL, "made again in the new language");
    atomic_store(&g_dm_lang, 1);



    ask(1, 200, -1, t, sizeof t);
    during = ask_other; pump();
    CHECK(ask(1, 200, -1, t, sizeof t) > 0, "the one being made is stored");
    pump();
    CHECK(ask(1, 300, -1, t, sizeof t) > 0, "the one asked meanwhile is made next");


    epoch_now++;
    for (int r = 1000; r < 1000 + ITEM_SLOTS; r++) { ask(1, r, -1, t, sizeof t); pump(); }
    CHECK(ask(1, 1000, -1, t, sizeof t) > 0, "the first one is there while not full");
    ask(1, 2000, -1, t, sizeof t); pump();
    CHECK(ask(1, 2000, -1, t, sizeof t) > 0, "the 65th is stored");
    CHECK(ask(1, 1000, -1, t, sizeof t) > 0, "a recently asked one stays");
    CHECK(ask(1, 1001, -1, t, sizeof t) == 0, "the one asked longest ago went");


    atomic_store(&g_ireq, 0);
    CHECK(ask(3, 1, -1, t, sizeof t) == DM_ITEM_TEXT_NONE && ask(1, -1, -1, t, sizeof t) == DM_ITEM_TEXT_NONE && ask(1, 1, 3, t, sizeof t) == DM_ITEM_TEXT_NONE
          && ask(1, 1 << 20, -1, t, sizeof t) == DM_ITEM_TEXT_NONE && ask(1, 1, -1, t, 1) == DM_ITEM_TEXT_NONE, "invalid arguments are none");
    CHECK(atomic_load(&g_ireq) == 0, "invalid arguments note nothing");


    ask(1, 248, -1, t, sizeof t); pump();
    char small[6]; int n = ask(1, 248, -1, small, sizeof small);
    CHECK(n == 5 && !strcmp(small, "K1 R2"), "truncated to the buffer");

    printf("t_item_text: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
