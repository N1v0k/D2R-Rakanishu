











#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "../hle_fs.c"
#include "../dm_looks.h"
#include "../dm_feats.h"

struct dm_flags g_flag; uint64_t g_t0_ns; uint64_t now_ns(void) { return 0; }
void dm_prof_add(int id, uint64_t ns, uint64_t b) { (void)id; (void)ns; (void)b; } void dm_guest_write_prepare(void *a, size_t n) { (void)a; (void)n; }
int64_t dm_user_clock(void) { return 0; }
int dm_pages_char(char *out, size_t n) { strlcpy(out, "Hero", n); return 0; }
int dm_looks_pay_proof(uint64_t G, uint64_t *seq); void dm_looks_settle_committed(void);
void dm_ser_observe(uint64_t G);
static uint64_t g_feats[DM_FT_COUNTERS];
void dm_feats_stage_count(int counter, uint64_t n) { g_feats[counter] += n; }

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  FAIL "); } else printf("  ok   "); printf(__VA_ARGS__); printf("\n"); } while (0)

static char g_root[256], g_save[600];
static uint8_t g_G[0x5808];
#define GG ((uint64_t)(uintptr_t)g_G)
static void tick(void) { (*(uint32_t *)(g_G + 0x170))++; }
static void serialize(uint64_t t) { *(uint64_t *)(g_G + 0x5800) = t; }
static void write_d2s(const char *s) {
    uint64_t h; if (h_fs_OpenFile(&h, "save:/Hero/Hero.d2s", MODE_WRITE)) { printf("  cannot open the character file\n"); exit(1); }
    h_fs_SetFileSize(h, (int64_t)strlen(s)); h_fs_WriteFile(h, 0, s, strlen(s), NULL); h_fs_CloseFile(h);
}
static void put(const char *p, const char *s) { FILE *f = fopen(p, "w"); fputs(s, f); fclose(f); }
static int committed_d2s(const char *want) {
    char p[800], b[64] = { 0 }; snprintf(p, sizeof p, "%s/Hero/Hero.d2s", g_save);
    FILE *f = fopen(p, "r"); if (!f) return 0; size_t k = fread(b, 1, sizeof b - 1, f); fclose(f);
    return k == strlen(want) && !memcmp(b, want, k);
}
static dm_look committed_look(const dm_look_item *it) {
    char p[800]; snprintf(p, sizeof p, "%s/" DM_LOOKS_FILE, g_save);
    dm_look none = { DM_LOOK_ORIGINAL, 0 }; FILE *f = fopen(p, "rb"); if (!f) return none;
    static uint8_t b[1 << 16]; size_t n = fread(b, 1, sizeof b, f); fclose(f);
    dm_looks_store s; if (dm_looks_decode(b, n, &s) != DM_LOOKS_OK) return none;
    dm_look l = dm_looks_current(&s, it); dm_looks_store_free(&s); return l;
}
static void pay(const dm_look_item *it, dm_look l, uint64_t seq) { pthread_mutex_lock(&g_looks_mu); dm_looks_choose(&g_looks, it, l, 0, seq); pthread_mutex_unlock(&g_looks_mu); }
static int busy(const dm_look_item *it) { pthread_mutex_lock(&g_looks_mu); dm_looks_settle_committed(); int b = dm_looks_busy(&g_looks, it); pthread_mutex_unlock(&g_looks_mu); return b; }

static int pair(const char *gold, const dm_look_item *it, dm_look l) {
    char p[800], b[64] = { 0 }; snprintf(p, sizeof p, "%s/Hero/Hero.d2s", g_save);
    FILE *f = fopen(p, "r"); if (!f) return 0; size_t k = fread(b, 1, sizeof b - 1, f); fclose(f);
    dm_look c = committed_look(it);
    return k >= strlen(gold) && !memcmp(b, gold, strlen(gold)) && c.kind == l.kind && c.id == l.id;
}
static void reload(void) { g_nmnt = 0; if (h_fs_MountSave("save", NULL)) { printf("  cannot mount\n"); exit(1); } pthread_mutex_lock(&g_looks_mu); dm_looks_load(); pthread_mutex_unlock(&g_looks_mu); }

int main(void) {
    snprintf(g_root, sizeof g_root, "/tmp/dm-looks-store.XXXXXX"); if (!mkdtemp(g_root)) return 1;
    snprintf(g_save, sizeof g_save, "%s/save", g_root); mkdir(g_save, 0755);
    char p[800]; snprintf(p, sizeof p, "%s/Hero", g_save); mkdir(p, 0755);
    snprintf(p, sizeof p, "%s/Hero/Hero.d2s", g_save); put(p, "gold 1000 save 0");
    setenv("DM_SAVE", g_save, 1);
    CHECK(h_fs_MountSave("save", NULL) == RES_OK, "save mounted");
    pthread_mutex_lock(&g_looks_mu); dm_looks_load(); pthread_mutex_unlock(&g_looks_mu);
    CHECK(g_looks_state == DM_LOOKS_STATE_OK, "a new looks store");
    dm_look_item cap = { 0x1234, 0x20706163, 4, -1 };
    dm_look ghm = { DM_LOOK_BASE, 0x206d6867 }, crn = { DM_LOOK_BASE, 0x206e7263 }, skp = { DM_LOOK_BASE, 0x20706b73 };
    uint64_t seq = 0;

    printf("A: an autosave serialized before the payment and written after it\n");
    dm_look orig = { DM_LOOK_ORIGINAL, 0 };
    tick(); serialize(100); dm_ser_observe(GG);
    write_d2s("gold 1000 save 1"); CHECK(h_fs_Commit("save") == RES_OK, "a save before the payment commits");
    tick(); dm_ser_observe(GG);
    tick(); serialize(200);
    CHECK(dm_looks_pay_proof(GG, &seq) == 0, "the payment in that tick waits for the save in flight");
    write_d2s("gold 1000 save 2");
    tick(); dm_ser_observe(GG);
    CHECK(dm_looks_pay_proof(GG, &seq) == 1, "once written, the payment goes ahead");
    pay(&cap, ghm, seq);
    memset(g_feats, 0, sizeof g_feats);
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 1000 save 2", &cap, orig), "the save serialized before the payment commits without the look");
    CHECK(!g_feats[DM_FT_LOOKS] && !g_feats[DM_FT_DYES], "the look paid but not kept by a save: nothing counted");
    CHECK(busy(&cap), "the item waits for its save");
    tick(); serialize(300); dm_ser_observe(GG); write_d2s("gold 900 save 3");
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 900 save 3", &cap, ghm), "the next save commits the look with the gold paid");
    CHECK(g_feats[DM_FT_LOOKS] == 1 && !g_feats[DM_FT_DYES], "the save keeping the item's first look: one item counted, no colour");
    CHECK(!busy(&cap), "and the item is free again");

    printf("B: a second change before the first is saved\n");
    tick(); dm_ser_observe(GG);
    CHECK(dm_looks_pay_proof(GG, &seq) == 1, "proof for the first change"); pay(&cap, crn, seq);
    tick(); serialize(400); dm_ser_observe(GG); write_d2s("gold 800 save 4"); tick(); dm_ser_observe(GG);
    CHECK(busy(&cap), "written, not committed: the item takes no second change (the page refuses it)");
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 800 save 4", &cap, crn), "the commit of the file written between carries the first change");
    CHECK(!busy(&cap), "then the item is free");
    tick(); dm_ser_observe(GG);
    CHECK(dm_looks_pay_proof(GG, &seq) == 1, "proof for the second change"); pay(&cap, skp, seq);
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 800 save 4", &cap, crn), "a commit without a new character file keeps the pair");
    tick(); serialize(500); dm_ser_observe(GG); write_d2s("gold 700 save 5");
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 700 save 5", &cap, skp), "the next file commits the second change");
    reload();
    CHECK(dm_looks_current(&g_looks, &cap).id == skp.id, "and a reload dresses the item with it");

    printf("C: a save refused after the payment (thrown away), then a commit without a character file\n");
    tick(); dm_ser_observe(GG);
    CHECK(dm_looks_pay_proof(GG, &seq) == 1, "proof"); pay(&cap, ghm, seq);
    tick(); serialize(600); dm_ser_observe(GG); write_d2s("gold 600 save 6");
    dm_fs_save_poison(EIO);
    CHECK(h_fs_Commit("save") == RES_FAIL && pair("gold 700 save 5", &cap, skp), "the save with the gold paid is refused, the pair stays");
    pthread_mutex_lock(&g_looks_mu); int on = dm_looks_latch(); pthread_mutex_unlock(&g_looks_mu);
    CHECK(on && dm_looks_locked() && g_looks.npend == 1 && dm_looks_current(&g_looks, &cap).id == ghm.id, "new changes lock, the paid choice stays");
    CHECK(dm_looks_pay_proof(GG, &seq) == -2, "no new payment");
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 700 save 5", &cap, skp), "a commit without a character file (a stash change) commits nothing of it");
    CHECK(busy(&cap), "the choice still waits");
    tick(); serialize(700); dm_ser_observe(GG); write_d2s("gold 600 save 7");
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 600 save 7", &cap, ghm), "the next character file commits it with its gold");
    reload();
    CHECK(dm_looks_current(&g_looks, &cap).id == ghm.id, "a reload sees it");

    printf("D: the copy at commit fails (the working directory stays), then a commit without a character file\n");
    g_save_fails = 0; reload();
    tick(); serialize(800); dm_ser_observe(GG); write_d2s("gold 600 save 8"); tick(); dm_ser_observe(GG);
    CHECK(dm_looks_pay_proof(GG, &seq) == 1, "proof against a written file not yet committed"); pay(&cap, crn, seq);
    setenv("DM_TEST_FS_FAILAT", "commitclone:1", 1);
    CHECK(h_fs_Commit("save") == RES_FAIL && pair("gold 600 save 7", &cap, ghm), "the copy of the working directory fails");
    unsetenv("DM_TEST_FS_FAILAT");
    CHECK(dm_looks_pay_proof(GG, &seq) == -2 && g_looks.npend == 1, "new changes lock, the paid choice stays");
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 600 save 8", &cap, ghm), "a commit of the file from before the payment keeps the look before it");
    tick(); serialize(900); dm_ser_observe(GG); write_d2s("gold 500 save 9");
    CHECK(h_fs_Commit("save") == RES_OK && pair("gold 500 save 9", &cap, crn), "the next character file commits it");
    reload();
    CHECK(dm_looks_current(&g_looks, &cap).id == crn.id, "a reload sees it");
    { dm_look_pending same = { cap, crn, 0, 0 }, dyed = { cap, crn, 3, 0 }, back = { cap, orig, 0, 0 }, other = { cap, ghm, 0, 0 }; int l, d;
      dm_looks_kept(&g_looks, &same, 1, &l, &d); CHECK(!l && !d, "the committed look again: nothing counted");
      dm_looks_kept(&g_looks, &back, 1, &l, &d); CHECK(!l && !d, "back to the original: nothing counted");
      dm_looks_kept(&g_looks, &other, 1, &l, &d); CHECK(!l && !d, "another look for an item that has one: the same item, not counted again");
      dm_looks_kept(&g_looks, &dyed, 1, &l, &d); CHECK(!l && d == 1, "a first colour on an item with a look: one coloured item");
      dm_look_pending two[2] = { { cap, crn, 3, 0 }, { cap, crn, 3, 0 } }; two[1].it.fp ^= 1;
      dm_looks_kept(&g_looks, two, 2, &l, &d); CHECK(l == 1 && d == 2, "two items in one save: each counted (the other one had no look yet)"); }

    char cmd[400]; snprintf(cmd, sizeof cmd, "rm -rf '%s'", g_root); if (system(cmd)) {}
    printf("t_looks_store: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
