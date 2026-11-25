






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

static int calls[TEXT_RECS], made_calls, fail_rec = -1;
int dm_rw_text(uint8_t *b, int rec, char *out, size_t n) {
    (void)b; calls[rec]++;
    if (rec == fail_rec) return -1;
    if (rec % 3 != 1) return 0;
    made_calls++; snprintf(out, n, "R%d L%d\nzweite Zeile", rec, atomic_load(&g_dm_lang)); return 1;
}
#define WITH_TEXT 85

int dm_item_desc(uint8_t *b, int kind, int row, int grp, char *out, size_t n) { (void)b; (void)kind; (void)row; (void)grp; (void)n; out[0] = 0; return 0; }
static uint8_t game[16], player[0x200];
static char dir[512];
static void frame(void) { text_pump(game, (uint64_t)(uintptr_t)player); }
static int get(int rec, char *buf, int n) { return dm_recipes_text(rec, buf, n); }
static int have(int rec) { char t[64]; return get(rec, t, sizeof t) > 0; }
static void start(void) { dm_recipes_text_start(dir); }
static int done_frames(int max) { int f = 0; while (!g_tdone && atomic_load(&g_twant) && f < max) { frame(); f++; } return f; }

int main(void) {
    player[0x1bd] = 3;
    const char *tmp = getenv("TMPDIR"); snprintf(dir, sizeof dir, "%s/t_recipes_text.XXXXXX", tmp && *tmp ? tmp : "/tmp");
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }
    char t[256], path[700];



    start();
    CHECK(get(250, t, sizeof t) == 0, "cold: nothing before the first frame");
    frame();
    CHECK(get(250, t, sizeof t) > 0 && !strncmp(t, "R250 L1\nzweite Zeile", 20), "cold: the record a preview waits for is made on the first frame");
    CHECK(made_calls == 3 && have(1) && have(4) && !have(7), "the priority text counts inside the three per frame");
    CHECK(calls[250] == 1, "priority record made once");
    int f = done_frames(400);
    CHECK(g_tdone && g_tcount == WITH_TEXT, "the pass finishes with every text");
    CHECK(calls[250] == 1, "the sequential pass skips the priority record");
    CHECK(f > 20 && f < 40, "about 85 / 3 frames for the rest");
    snprintf(path, sizeof path, "%s/recipe_text_1_d3_3.3.94012.tsv", dir);
    FILE *fp = fopen(path, "r"); char head[64] = "";
    CHECK(fp && fgets(head, sizeof head, fp) && !strcmp(head, "#recipe text 3.3.94012 85\n"), "the file's header count = the texts present");
    if (fp) fclose(fp);


    text_clear(); pthread_mutex_lock(&g_tmu); g_tlang = -1; pthread_mutex_unlock(&g_tmu);
    int before = made_calls;
    start(); frame();
    CHECK(g_tdone && made_calls == before && have(1) && have(253) && !have(2), "loaded back from the file");
    CHECK(get(250, t, sizeof t) > 0 && !strncmp(t, "R250 L1", 7), "loaded text intact (line break restored)");
    CHECK(strchr(t, '\n') != NULL, "line breaks survive the file");




    atomic_store(&g_dm_lang, 2);
    CHECK(get(250, t, sizeof t) == 0, "stale language: nothing handed out before the pump runs");
    start(); frame();
    CHECK(get(250, t, sizeof t) > 0 && !strncmp(t, "R250 L2", 7), "new language: the waiting record first, in the new language");
    CHECK(have(1) && !strncmp((get(1, t, sizeof t), t), "R1 L2", 5), "sequential texts in the new language too");
    done_frames(400);
    snprintf(path, sizeof path, "%s/recipe_text_2_d3_3.3.94012.tsv", dir);
    CHECK(access(path, R_OK) == 0 && g_tcount == WITH_TEXT, "the new language gets its own file");
    atomic_store(&g_dm_lang, 1); start(); frame();
    CHECK(get(250, t, sizeof t) > 0 && !strncmp(t, "R250 L1", 7), "back to language 1: its file again");



    atomic_store(&g_dm_lang, 3); start();
    CHECK(get(250, t, sizeof t) == 0, "language 3 cold");
    frame();
    CHECK(have(250) && g_tcount == 3 && !g_tdone, "three made, not done");
    epoch_now++; before = made_calls;
    frame();
    CHECK(!have(250) && g_tcount == 0 && !atomic_load(&g_twant), "run ended: unfinished texts cleared, the pass stopped");
    for (int i = 0; i < 5; i++) { have(250); frame(); }
    CHECK(made_calls == before && !have(250), "the getter never restarts a stopped pass");
    start(); frame();
    CHECK(have(250), "opening the book restarts it, the waiting record first");
    done_frames(400);




    atomic_store(&g_dm_lang, 4); fail_rec = 10; start();
    have(250); frame();
    CHECK(have(250), "priority text handed out");
    int calls10 = calls[10]; f = 0;
    while (atomic_load(&g_twant) && f < 50) { frame(); f++; }
    CHECK(calls[10] == calls10 + 1 && !have(250) && g_tcount == 0 && !atomic_load(&g_twant), "a later failure clears the published priority text and stops");
    snprintf(path, sizeof path, "%s/recipe_text_4_d3_3.3.94012.tsv", dir);
    CHECK(access(path, F_OK) != 0, "no file after a failure");
    before = made_calls; calls10 = calls[10];
    for (int i = 0; i < 10; i++) { have(250); frame(); }
    CHECK(made_calls == before && calls[10] == calls10, "a waiting preview does not restart a failed pass");
    fail_rec = -1;


    atomic_store(&g_dm_lang, 5); player[0x1bd] = 2; start(); before = made_calls; have(250); frame();
    CHECK(made_calls == before && !atomic_load(&g_twant), "data version 2: no work, stopped");
    player[0x1bd] = 3; start(); text_pump(game, 0);
    CHECK(made_calls == before && atomic_load(&g_twant), "no player: waits");


    CHECK(get(-1, t, sizeof t) == 0 && get(TEXT_RECS, t, sizeof t) == 0 && get(250, t, 0) == 0, "out of range / no room");

    for (int l = 1; l <= 4; l++) { snprintf(path, sizeof path, "%s/recipe_text_%d_d3_3.3.94012.tsv", dir, l); unlink(path); }
    rmdir(dir);
    printf("t_recipes_text: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
