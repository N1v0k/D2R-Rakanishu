



































#include "../dm_feats_core.c"
#include "../dm_streak_core.c"
#include "../dm_streak_hist.c"
#include <stdio.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

enum { WARLOCK = 7 };
static dm_ft F;
static dm_ft_facts X;
static uint32_t T = 1000;
static void facts(int level, int cls) { memset(&X, 0, sizeof X); X.level = level; X.cls = cls; X.known = DM_FT_K_ALL; }
static void act(int difficulty, int a) { static const int w[5] = { 6, 14, 22, 26, 40 }; X.quests[difficulty][w[a - 1]] |= 1; }
static int done(int id) { return dm_ft_done(&F, id) != NULL; }
static int entered(void) { int n = 0; for (int i = 0; i < F.nrec; i++) n += F.rec[i].kind == DM_FT_REC_DONE; return n; }
static int step(void) { return dm_ft_update(&F, &X, ++T); }
static int kill(uint64_t n) { return dm_ft_count(&F, DM_FT_KILLS, n, ++T); }
static void fresh(void) { dm_ft_clear(&F); facts(10, 0); }

static struct { uint64_t sig[DM_FT_PAGES]; unsigned owns[DM_FT_PAGES]; int fail[DM_FT_PAGES], nosig[DM_FT_PAGES], built[DM_FT_PAGES], builds; } pg;
static void fake_reset(void) { memset(&pg, 0, sizeof pg); for (int p = 0; p < DM_FT_PAGES; p++) pg.sig[p] = 100 + p; }
static int fake_sig(int p, uint64_t *sig, void *ctx) { (void)ctx; *sig = pg.sig[p]; return pg.nosig[p]; }
static int fake_items(int p, unsigned *owned, void *ctx) { (void)ctx; pg.built[p]++; pg.builds++; if (pg.fail[p]) return -1; *owned |= pg.owns[p]; return 0; }

static uint32_t C(const char *s) { uint32_t c; char b[4] = { ' ', ' ', ' ', ' ' }; memcpy(b, s, strlen(s)); memcpy(&c, b, 4); return c; }

static dm_ft_cube_item I(uint32_t id, const char *code, int quality, int filled) { return (dm_ft_cube_item){ id, C(code), (uint8_t)quality, (uint8_t)filled }; }
#define CUBE(b, a) dm_ft_cube_events(b, (int)(sizeof b / sizeof *b), a, (int)(sizeof a / sizeof *a))

static struct { uint32_t id[8], code[8]; int n, unread, placed, runeword; uint32_t target; dm_ft_cube_item cube[DM_FT_CUBE]; int ncube; uint64_t gold; } W;
static int w_item(uint32_t id, uint32_t *code, void *ctx) { (void)ctx; if (W.unread) return -1; for (int i = 0; i < W.n; i++) if (W.id[i] == id) { *code = W.code[i]; return 1; } return 0; }
static int w_target(uint32_t t, uint32_t item, int *placed, int *runeword, void *ctx) {
    (void)ctx; (void)item; if (W.unread) return -1; if (t != W.target) return 0; *placed = W.placed; *runeword = W.runeword; return 1;
}
static int w_cube(dm_ft_cube_item *out, int max, void *ctx) { (void)ctx; if (W.unread || W.ncube > max) return -1; memcpy(out, W.cube, sizeof *out * (size_t)W.ncube); return W.ncube; }

static int w_top(uint32_t *id, void *ctx) {
    (void)ctx; if (W.unread) return -1; *id = 0;
    for (int i = 0; i < W.n; i++) if (W.id[i] > *id) *id = W.id[i];
    for (int i = 0; i < W.ncube; i++) if (W.cube[i].id > *id) *id = W.cube[i].id;
    return 1;
}
static int w_gold(uint64_t *g, void *ctx) { (void)ctx; if (W.unread) return -1; *g = W.gold; return 1; }
static const dm_ft_reader WR = { w_item, w_target, w_cube, w_top, w_gold };
static void w_hold(uint32_t id, const char *code) { W.id[W.n] = id; W.code[W.n++] = C(code); }
static void w_drop(uint32_t id) { for (int i = 0; i < W.n; i++) if (W.id[i] == id) { W.id[i] = W.id[--W.n]; W.code[i] = W.code[W.n]; return; } }

int main(void) {

    { int dup = 0, cls = 0; for (int i = 0; i < dm_ft_nrows; i++) { for (int j = 0; j < i; j++) dup += dm_ft_rows[i].id == dm_ft_rows[j].id;
          if (dm_ft_rows[i].rule == DM_FT_R_CLASS_HELL) cls++; CHECK(dm_ft_rows[i].rev >= 1 && dm_ft_rows[i].rev <= DM_FT_REV, "row %d revision", dm_ft_rows[i].id); }
      CHECK(!dup && cls == DM_FT_CLASSES && dm_ft_nrows == 60 && dm_ft_nrows <= DM_FT_ROWS_MAX, "60 rows, unique ids, %d class rows", cls);
      int rev2 = 0, rev3 = 0, rev4 = 0, rev5 = 0;
      for (int i = 0; i < dm_ft_nrows; i++) { rev2 += dm_ft_rows[i].rev == 2; rev3 += dm_ft_rows[i].rev == 3; rev4 += dm_ft_rows[i].rev == 4; rev5 += dm_ft_rows[i].rev == 5; }
      int hunt = 0, rifts = 0, workshop = 0;
      for (int id = 44; id <= 60; id++) { const dm_ft_row *r = dm_ft_row_of(id); if (!r || r->rev != 5) continue; hunt += r->group == DM_FT_G_HUNT; rifts += r->group == DM_FT_G_RIFTS; workshop += r->group == DM_FT_G_CRAFT; }
      CHECK(rev5 == 17 && hunt == 8 && rifts == 5 && workshop == 4, "17 rows of revision 5: 8 hunt, 5 rifts, 4 workshop (%d %d %d %d)", rev5, hunt, rifts, workshop);
      CHECK(dm_sk_tier(dm_ft_row_of(44)->arg) == 3 && dm_sk_tier(dm_ft_row_of(44)->arg - 1) == 2 && dm_sk_tier(dm_ft_row_of(45)->arg) == 6
            && dm_sk_tier(dm_ft_row_of(45)->arg - 1) == 5 && dm_sk_tier(dm_ft_row_of(46)->arg) == DM_SK_TIERS && dm_sk_tier(dm_ft_row_of(46)->arg - 1) == DM_SK_TIERS - 1,
            "the streak rows are the first kills of the streak's tiers 3, 6 and 9");
      CHECK(dm_ft_row_of(47)->arg == DM_FT_BOARD_ROWS && dm_ft_row_of(48)->arg == 250 && dm_ft_row_of(54)->arg == 30 && dm_ft_row_of(55)->arg == 300, "owner thresholds");
      CHECK(rev4 == 3 && dm_ft_row_of(36)->rev == 4 && dm_ft_row_of(37)->rev == 4 && dm_ft_row_of(38)->rev == 4 && dm_ft_row_of(38)->arg == DM_FT_E_GAMBLE
            && dm_ft_row_of(36)->group == DM_FT_G_TREASURE && dm_ft_row_of(38)->group == DM_FT_G_TREASURE, "3 rows of revision 4, treasures");
      CHECK(rev2 == 11 && dm_ft_row_of(12)->rev == 2 && dm_ft_row_of(42)->rev == 2 && dm_ft_row_of(21)->rev == 1, "11 rows of revision 2");
      CHECK(rev3 == 8 && dm_ft_row_of(24)->rev == 3 && dm_ft_row_of(30)->rev == 3 && dm_ft_row_of(35)->rev == 3, "8 rows of revision 3");
      CHECK(dm_ft_row_of(43) && dm_ft_row_of(43)->arg == WARLOCK && dm_ft_row_of(13)->arg == 0 && dm_ft_row_of(19)->arg == 6 && dm_ft_row_of(13)->rule == DM_FT_R_CLASS_HELL, "class ids");
      const dm_ft_row *every = dm_ft_row_of(20);
      CHECK(every && every->rule == DM_FT_R_EVERY_CLASS && every->arg == DM_FT_CLASSES && every->group == DM_FT_G_MASTERY && dm_ft_account_wide(every)
            && !dm_ft_account_wide(dm_ft_row_of(43)), "every class's Hell: account-wide, eight classes"); }


    fresh(); CHECK(step() == 0 && F.reconciled && F.rev == DM_FT_REV, "first facts reconcile");
    CHECK(kill(499) == 0 && !done(1), "499 kills: nothing");
    CHECK(kill(1) == 1 && done(1) && F.seq == 1 && F.note[1].id == 1 && F.note[1].arg == 500, "the 500th kill enters row 1 with a notice");
    CHECK(kill(1) == 0 && step() == 0 && F.seq == 1, "nothing twice");
    CHECK(kill(4499) == 1 && done(2) && !done(3) && kill(5000) == 1 && done(3), "5 000 and 10 000");
    CHECK(dm_ft_counter(&F, DM_FT_KILLS) == 10000, "counter %llu", (unsigned long long)dm_ft_counter(&F, DM_FT_KILLS));
    CHECK(dm_ft_done(&F, 1)->level == 10 && dm_ft_done(&F, 1)->when > 1000, "an entry keeps the level and the time");
    dm_ft_set_counter(&F, DM_FT_KILLS, UINT64_MAX - 2); kill(10);
    CHECK(dm_ft_counter(&F, DM_FT_KILLS) == UINT64_MAX, "the counter saturates");
    CHECK(kill(0) == 0 && dm_ft_count(&F, DM_FT_COUNTERS, 5, T) == 0, "nothing for no kills or an unknown counter");


    fresh(); step();
    act(2, 1); CHECK(step() == 1 && done(4), "act I: its boss word in any difficulty");
    for (int a = 2; a <= 5; a++) act(0, a);
    CHECK(step() == 4 && done(5) && done(6) && done(7) && done(8) && !done(9) && !done(10), "acts II to V of normal: no difficulty row");
    act(1, 5); CHECK(step() == 1 && done(9), "nightmare: word 40 of its set");
    X.level = 98; CHECK(step() == 0 && !done(21), "level 98");
    X.level = 99; CHECK(step() == 1 && done(21), "level 99");
    X.cls = WARLOCK; act(2, 5);
    CHECK(step() == 2 && done(10) && done(43) && !done(19), "hell with the eighth class: the hell row and its class row");
    X.cls = 6; CHECK(step() == 1 && done(19), "another class with hell done (a class switch is only made up here)");


    fresh(); step(); uint32_t gen = F.gen;
    for (int d = 0; d < DM_FT_DIFFS; d++) X.quests[d][41] = 0xffff;
    CHECK(step() == 0 && F.gen == gen && entered() == 0, "word 41 changes nothing");
    for (int d = 0; d < DM_FT_DIFFS; d++) for (int w = 0; w < DM_FT_WORDS; w++) if (w != 6 && w != 14 && w != 22 && w != 26 && w != 40) X.quests[d][w] = 0xfffe;
    CHECK(step() == 0 && entered() == 0, "neither does any other word or any other bit of the boss words");


    fresh(); act(0, 1); act(0, 2); act(0, 3);
    CHECK(step() == 0 && entered() == 3 && done(4) && done(5) && done(6) && F.seq == 0 && !F.noticed, "silent entering, no notice");
    act(0, 4); CHECK(step() == 1 && done(7) && F.seq == 1 && F.note[1].id == 7 && F.noticed, "act IV afterwards: one notice");

    { dm_ft_rec none[1]; dm_ft_take(&F, none, 0, DM_FT_REV); facts(99, 0);
      CHECK(step() == 1 && done(21), "same revision: notice"); }

    { dm_ft_rec old[2] = { { 4, DM_FT_REC_DONE, 30, 77, 0 }, { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 600 } };
      dm_ft_take(&F, old, 2, 0); facts(99, 0); act(0, 1); act(0, 2);
      CHECK(step() == 0 && done(1) && done(5) && done(21) && dm_ft_done(&F, 4)->when == 77 && F.rev == DM_FT_REV, "older revision: silent, the old entry as it was"); }

    { dm_ft_rec old[1] = { { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 499 } };
      dm_ft_take(&F, old, 1, 0); facts(5, 0);
      CHECK(kill(1) == 0 && !done(1), "no entry before the first facts");
      CHECK(step() == 1 && done(1) && F.note[F.seq % DM_FT_NOTICES].id == 1, "the first facts give its notice"); }

    { dm_ft_rec old[2] = { { 200, DM_FT_REC_DONE, 50, 9, 0 }, { 7, DM_FT_REC_COUNT, 0, 0, 1234 } };
      dm_ft_take(&F, old, 2, DM_FT_REV + 1); facts(5, 0); step(); kill(3);
      CHECK(F.nrec == 3 && F.rec[0].id == 200 && F.rec[1].value == 1234 && F.rev == DM_FT_REV + 1, "unknown records kept, a newer revision is not lowered"); }


    fresh(); step(); uint32_t first = F.seq;
    act(0, 1); act(0, 2); act(0, 3); act(0, 4); act(0, 5); act(1, 5); X.level = 99; X.cls = WARLOCK; act(2, 5); kill(10000);
    step();
    CHECK(F.seq == first + 12, "12 notices: %u", F.seq - first);
    { int kept = 0; for (int i = 0; i < DM_FT_NOTICES; i++) kept += F.note[i].seq > first + 4; CHECK(kept == DM_FT_NOTICES, "the last 8 kept"); }


    { fresh(); step(); kill(1234); char out[2048];
      int n = dm_ft_text(&F, out, sizeof out), lines = 0;
      for (int i = 0; i < n; i++) lines += out[i] == '\n';
      CHECK(n > 0 && lines == DM_FT_COUNTERS + dm_ft_nrows - 1 && !strncmp(out, "counter\t0\t1234\ncounter\t1\t0\ncounter\t2\t0\ncounter\t3\t0\ncounter\t4\t0\nrow\t1\t1\t1\t500\t1\t", strlen("counter\t0\t1234\ncounter\t1\t0\ncounter\t2\t0\ncounter\t3\t0\ncounter\t4\t0\nrow\t1\t1\t1\t500\t1\t")), "page lines: %.60s", out);
      char want[64]; snprintf(want, sizeof want, "\nrow\t1\t1\t1\t500\t1\t%u\nrow\t2\t1\t1\t5000\t0\t0\n", dm_ft_done(&F, 1)->when);
      CHECK(strstr(out, want) != NULL, "row 1 entered with its time, row 2 not");
      CHECK(strstr(out, "\nrow\t43\t3\t4\t7\t0\t0\n") && strstr(out, "\nrow\t21\t3\t5\t99\t0\t0\n"), "the eighth class row and level 99");
      CHECK(strstr(out, "\nrow\t12\t2\t7\t0\t0\t0\n") && strstr(out, "\nrow\t22\t3\t8\t99\t0\t0\n") && strstr(out, "\nrow\t33\t4\t11\t10\t0\t0\n")
            && strstr(out, "\nrow\t42\t5\t13\t5\t0\t0\n"), "rows of revision 2 with their groups, rules and args");
      CHECK(dm_ft_text(&F, out, (size_t)n) == -2 && !out[0], "one byte short: nothing");
      CHECK(dm_ft_text(&F, out, 0) == -1 && dm_ft_text(&F, NULL, 9) == -1, "no buffer");
      CHECK(!strstr(out, "\nrow\t20\t"), "the account-wide row is not one character's (the one line fewer above)"); }


    fresh(); facts(99, 0); for (int d = 0; d < 3; d++) for (int a = 1; a <= 5; a++) act(d, a);
    step(); step();
    CHECK(done(13) && !done(20), "an Amazon through Hell: its class row, not every class's");


    { fresh(); step();
      static const uint8_t real[3][3] = { { 0x3f }, { 0xff, 0x1b }, { 0xff, 0xfb, 0x01 } };
      X.waypoints = 39;
      for (int r = 0; r < 3; r++) { memset(X.wp, 0, sizeof X.wp); memcpy(X.wp[0], real[r], 3); CHECK(step() == 0 && !done(12), "real pattern %d: not all", r); }
      memset(X.wp, 0, sizeof X.wp); for (int w = 0; w < 39; w++) { X.wp[0][w / 8] |= (uint8_t)(1 << (w % 8)); X.wp[1][w / 8] |= (uint8_t)(w ? 1 << (w % 8) : 0); }
      X.wp[0][38 / 8] &= (uint8_t)~(1 << (38 % 8));
      CHECK(step() == 0 && !done(12), "38 of 39 in normal, nightmare without its first");
      X.waypoints = 0; X.wp[0][38 / 8] |= (uint8_t)(1 << (38 % 8)); X.wp[0][7] = 0xff;
      CHECK(step() == 0 && !done(12), "all bits set but the game's count unknown: nothing");
      X.waypoints = 39; CHECK(step() == 1 && done(12) && F.note[F.seq % DM_FT_NOTICES].id == 12, "all 39 of normal: one notice");
      fresh(); step(); X.waypoints = 39; for (int w = 0; w < 39; w++) X.wp[2][w / 8] |= (uint8_t)(1 << (w % 8));
      CHECK(step() == 1 && done(12), "all of hell alone"); }


    { fresh(); step(); X.level = 99; act(0, 5);
      X.known &= ~DM_FT_K_HARDCORE; CHECK(step() == 2 && done(21) && done(8) && !done(22) && !done(23), "flag not known: no hardcore row");
      X.known = DM_FT_K_ALL;
      X.hardcore = 0; CHECK(step() == 0 && !done(22) && !done(23), "softcore: no hardcore row");
      X.hardcore = 1; CHECK(step() == 2 && done(22) && done(23), "hardcore at 99 with act V of normal");
      fresh(); X.hardcore = 1; step(); X.level = 98; act(1, 5); CHECK(step() == 3 && done(23) && done(9) && !done(22), "hardcore, act V of nightmare, level 98"); }


    { const uint32_t RIN = 0x206e6972, STD = 0x20647473, CAP = 0x20706163, AXE = 0x20787861;
      int16_t pieces[2] = { 3, 2 };
      dm_ft_item it[16]; memset(it, 0, sizeof it);
      fresh(); step();
      it[0] = (dm_ft_item){ RIN, DM_FT_Q_UNIQUE, 0, 0, 121, -1 };
      dm_ft_items(&X, it, 1, pieces, 2); CHECK(step() == 0 && !X.owned && !done(31), "another unique ring");
      it[1] = (dm_ft_item){ RIN, DM_FT_Q_SET, 0, 0, DM_FT_SOJ_ROW, 0 };
      dm_ft_items(&X, it, 2, pieces, 2); CHECK(step() == 0 && !done(31), "the row of a set item");
      it[2] = (dm_ft_item){ RIN, DM_FT_Q_UNIQUE, 0, 0, DM_FT_SOJ_ROW, -1 };
      dm_ft_items(&X, it, 3, pieces, 2); CHECK(step() == 1 && done(31) && !done(32), "the ring of its row, not worn");
      it[3] = (dm_ft_item){ STD, 2, 0, 0, 0, -1 };
      dm_ft_items(&X, it, 4, pieces, 2); CHECK(step() == 1 && done(32), "the standard on the cursor");
      dm_ft_items(&X, it, 0, pieces, 2); CHECK(step() == 0 && done(31) && done(32) && !X.owned, "sold again: the entries stay");

      memset(it, 0, sizeof it);
      for (int k = 0; k < 10; k++) it[k] = (dm_ft_item){ CAP, DM_FT_Q_UNIQUE, (uint8_t)(k + 1), 0, 1, -1 };
      it[4].quality = 4;
      dm_ft_items(&X, it, 10, pieces, 2); CHECK(X.uniques == 0x3ef && step() == 0 && !done(33), "nine slots");
      it[4] = (dm_ft_item){ RIN, DM_FT_Q_UNIQUE, 0, 0, 5, -1 };
      dm_ft_items(&X, it, 10, pieces, 2); CHECK(step() == 0 && !done(33), "a unique in the inventory");
      it[3] = (dm_ft_item){ AXE, DM_FT_Q_UNIQUE, 4, 1, 9, -1 };
      it[4] = (dm_ft_item){ CAP, 6, 5, 0, 1, -1 };
      dm_ft_items(&X, it, 10, pieces, 2); CHECK(X.uniques == 0x3ef && step() == 0 && !done(33), "a two-hander held in one hand beside a rare (a Barbarian's sword)");
      it[4].slot = 0;
      dm_ft_items(&X, it, 10, pieces, 2); CHECK(X.uniques == 0x3ff && step() == 1 && done(33), "a two-hander with the other hand empty counts for both");
      fresh(); step(); it[3].two_hands = 0;
      dm_ft_items(&X, it, 10, pieces, 2); CHECK(step() == 0 && !done(33), "a one-hander does not");
      it[3].quality = 5; it[3].slot = 5; it[3].two_hands = 1; it[4] = (dm_ft_item){ AXE, DM_FT_Q_UNIQUE, 4, 0, 9, -1 };
      dm_ft_items(&X, it, 10, pieces, 2); CHECK(X.uniques == 0x3ef && step() == 0, "a set two-hander in the left hand fills nothing");

      fresh(); step(); memset(it, 0, sizeof it);
      it[0] = (dm_ft_item){ CAP, DM_FT_Q_SET, 1, 0, 0, 0 }; it[1] = (dm_ft_item){ RIN, DM_FT_Q_SET, 6, 0, 1, 0 }; it[2] = (dm_ft_item){ RIN, DM_FT_Q_SET, 7, 0, 1, 0 };
      dm_ft_items(&X, it, 3, pieces, 2); CHECK(!X.full_set && step() == 0 && !done(34), "a piece twice is one piece: two of three");
      it[3] = (dm_ft_item){ AXE, DM_FT_Q_SET, 0, 0, 2, 0 };
      dm_ft_items(&X, it, 4, pieces, 2); CHECK(!X.full_set && step() == 0, "the third piece not worn");
      it[4] = (dm_ft_item){ CAP, DM_FT_Q_SET, 3, 0, 3, 1 }; it[5] = (dm_ft_item){ CAP, DM_FT_Q_SET, 8, 0, 4, -1 };
      dm_ft_items(&X, it, 6, pieces, 2); CHECK(!X.full_set && step() == 0, "one piece of the other set and a row the set table does not know");
      dm_ft_items(&X, it, 6, NULL, 0); CHECK(!X.full_set, "no set table: no set");
      it[3].slot = 4;
      dm_ft_items(&X, it, 4, pieces, 2); CHECK(X.full_set && step() == 1 && done(34), "all three pieces worn"); }


    { fresh(); X.merc = 2; CHECK(step() == 0 && done(40) && !done(39), "the act II mercenary of a character without a file: silent");
      X.merc = 0; CHECK(step() == 0, "dead or dismissed: nothing");
      X.merc = 3; CHECK(step() == 1 && done(41) && done(40), "act III hired: a notice, act II stays");
      X.merc = 5; CHECK(step() == 1 && done(42), "act V");
      X.merc = 1; CHECK(step() == 1 && done(39), "act I");
      X.merc = 4; CHECK(step() == 0, "no mercenary of act IV"); }


    { dm_ft_rec old[2] = { { 4, DM_FT_REC_DONE, 30, 77, 0 }, { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 400 } };
      dm_ft_take(&F, old, 2, 1); uint32_t seq = F.seq;
      facts(40, 0); X.merc = 2; X.owned = DM_FT_OWN_SOJ; X.waypoints = 39; memset(X.wp[1], 0xff, DM_FT_WP_BYTES); act(0, 1); act(0, 2);
      CHECK(step() == 1 && done(5) && F.note[F.seq % DM_FT_NOTICES].id == 5, "a row revision 1 knew comes with a notice");
      CHECK(done(40) && done(31) && done(12) && F.seq == seq + 1 && F.rev == DM_FT_REV, "the rows of revision 2 silently");
      X.merc = 1; CHECK(step() == 1 && done(39) && F.seq == seq + 2, "then a new mercenary with a notice"); }


    {
      dm_ft_rec old[1] = { { 4, DM_FT_REC_DONE, 30, 77, 0 } };
      dm_ft_take(&F, old, 1, 1); uint32_t seq = F.seq;
      facts(50, 0); X.known &= ~DM_FT_K_SETS;
      CHECK(step() == 0 && !done(34) && F.rev == 1 && F.seq == seq, "set table missing: the set row waits, the revision stays");
      X.merc = 2; CHECK(step() == 1 && done(40) && F.seq == seq + 1, "meanwhile a newer row judged with its source comes with a notice when met");
      seq = F.seq;
      dm_ft_rec saved[DM_FT_RECS]; int n = F.nrec; uint32_t rev = F.rev; memcpy(saved, F.rec, sizeof saved);
      dm_ft_take(&F, saved, n, rev); facts(50, 0); X.merc = 2; X.full_set = 1;
      CHECK(step() == 0 && done(34) && F.seq == seq && F.rev == DM_FT_REV, "the next game with the table: the set enters silently, the revision moves on");
      X.full_set = 1; CHECK(step() == 0 && F.seq == seq, "and stays without a notice"); }
    {
      dm_ft_rec old[1] = { { 4, DM_FT_REC_DONE, 30, 77, 0 } };
      dm_ft_take(&F, old, 1, 1); uint32_t seq = F.seq;
      facts(50, 0); X.known &= ~DM_FT_K_PAGES;
      CHECK(step() == 0 && !done(31) && F.rev == 1, "pages not read: the ring's row waits");
      X.known = DM_FT_K_ALL; X.owned = DM_FT_OWN_SOJ;
      CHECK(step() == 0 && done(31) && F.seq == seq && F.rev == DM_FT_REV, "pages read with the ring: entered silently");
      CHECK(step() == 0 && F.seq == seq, "the page shown later: no notice");

      X.known &= ~DM_FT_K_MERC; X.merc = 0; CHECK(step() == 0 && F.rev == DM_FT_REV, "a source missing later: nothing"); }
    {
      fresh(); X.known = 0; act(0, 1); uint32_t seq = F.seq;
      CHECK(step() == 0 && done(4) && F.rev == 0 && F.reconciled, "no source: act I silently, the revision waits");
      act(0, 2); CHECK(step() == 1 && done(5) && F.seq == seq + 1, "act II afterwards: a notice");
      kill(500); CHECK(done(1) && F.seq == seq + 2, "counts after the first facts give notices meanwhile");
      X.known = DM_FT_K_ALL; X.owned = DM_FT_OWN_STD; CHECK(step() == 0 && done(32) && F.rev == DM_FT_REV, "the sources come: their rows silently, the revision moves on"); }



    { int named = 0;
      for (int i = 0; i < dm_ft_nrows; i++) if (dm_ft_rows[i].rule == DM_FT_R_OWN || dm_ft_rows[i].rule == DM_FT_R_MERC) {
          named++; CHECK(dm_ft_name_key_of(dm_ft_rows[i].id) >= 0, "row %u names a game name: it has a key", dm_ft_rows[i].id); }
      CHECK(named == DM_FT_NAME_KEYS && dm_ft_name_key_of(1) == -1, "a key for each named row, none for others (%d)", named);
      for (int i = 0; i < DM_FT_NAME_KEYS; i++) {
          const dm_ft_name_key *k = &dm_ft_name_keys[i];
          CHECK(dm_ft_game_text_ok(k->key, k->key, k->plain) == k->plain, "%s: its own key as the answer is a name only for a plain key", k->key);
          CHECK(dm_ft_game_text_ok("Eisenwolf", k->key, k->plain) && !dm_ft_game_text_ok("", k->key, k->plain) && !dm_ft_game_text_ok(NULL, k->key, k->plain),
                "%s: another text is a name, nothing is not", k->key);
      }
      const dm_ft_name_key *wolf = &dm_ft_name_keys[dm_ft_name_key_of(41)], *barb = &dm_ft_name_keys[dm_ft_name_key_of(42)], *soj = &dm_ft_name_keys[dm_ft_name_key_of(31)];
      CHECK(dm_ft_game_text_ok("Iron Wolf", wolf->key, wolf->plain) && dm_ft_game_text_ok("Barbarian", barb->key, barb->plain)
            && dm_ft_game_text_ok("The Stone of Jordan", soj->key, soj->plain), "the English names of act III, act V and the ring");
      const dm_ft_name_key *rogue = &dm_ft_name_keys[dm_ft_name_key_of(39)];
      CHECK(!dm_ft_game_text_ok("RogueScout", rogue->key, rogue->plain) && dm_ft_game_text_ok("Rogue Scout", rogue->key, rogue->plain), "act I: the key back is missing"); }


    { dm_ft_sum A; dm_ft_sum_clear(&A);
      dm_ft_rec ann[] = { { 1, DM_FT_REC_DONE, 30, 500, 0 }, { 4, DM_FT_REC_DONE, 30, 900, 0 }, { 13, DM_FT_REC_DONE, 80, 700, 0 },
                          { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 3000 }, { 300, DM_FT_REC_DONE, 1, 1, 0 }, { 9, DM_FT_REC_COUNT, 0, 0, 77 } };
      dm_ft_rec bea[] = { { 1, DM_FT_REC_DONE, 20, 400, 0 }, { 4, DM_FT_REC_DONE, 20, 950, 0 }, { 14, DM_FT_REC_DONE, 85, 800, 0 },
                          { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 2500 }, { 20, DM_FT_REC_DONE, 1, 5, 0 } };
      dm_ft_sum_add(&A, ann, 6); dm_ft_sum_add(&A, bea, 5); dm_ft_sum_add(&A, NULL, 0);
      int i1 = -1, i4 = -1, i13 = -1, i14 = -1, i20 = -1, i2 = -1;
      for (int i = 0; i < dm_ft_nrows; i++) switch (dm_ft_rows[i].id) { case 1: i1 = i; break; case 2: i2 = i; break; case 4: i4 = i; break; case 13: i13 = i; break; case 14: i14 = i; break; case 20: i20 = i; break; }
      CHECK(A.heroes == 3 && A.counters[DM_FT_KILLS] == 3000, "three characters: the most kills of one of them, not the sum (%llu)", (unsigned long long)A.counters[DM_FT_KILLS]);
      CHECK(A.done[i1] && A.when[i1] == 400 && A.done[i4] && A.when[i4] == 900 && !A.done[i2], "a row any character entered, on the earliest day");
      CHECK(A.done[i13] && A.when[i13] == 700 && A.done[i14] && A.when[i14] == 800, "each character's class row");
      CHECK(!A.done[i20] && !A.when[i20], "two classes: not every class's Hell, and a file's record of it does not count");

      for (int c = 2; c < DM_FT_CLASSES; c++) {
          int id = c == 7 ? 43 : 13 + c; dm_ft_rec r[] = { { (uint16_t)id, DM_FT_REC_DONE, 90, (uint32_t)(1000 + c), 0 } };
          dm_ft_sum_add(&A, r, 1);
          CHECK(A.done[i20] == (c == 7), "every class's Hell after class %d: %d", c, A.done[i20]);
      }
      CHECK(A.when[i20] == 1007, "on the day the eighth class got there (%u)", A.when[i20]);

      dm_ft_rec huge[] = { { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, UINT64_MAX - 10 } }, few[] = { { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 7 } };
      dm_ft_sum_add(&A, huge, 1); dm_ft_sum_add(&A, few, 1);
      CHECK(A.counters[DM_FT_KILLS] == UINT64_MAX - 10, "the highest count of one character, no sum (%llu)", (unsigned long long)A.counters[DM_FT_KILLS]);
      const char *head = "allcounter\t0\t18446744073709551605\nallcounter\t1\t0\nallcounter\t2\t0\nallcounter\t3\t0\nallcounter\t4\t0\nallrow\t1\t1\t1\t500\t1\t400\n";
      char out[4096]; int n = dm_ft_sum_text(&A, out, sizeof out), lines = 0;
      for (int i = 0; i < n; i++) lines += out[i] == '\n';
      CHECK(n > 0 && lines == DM_FT_COUNTERS + dm_ft_nrows && !strncmp(out, head, strlen(head)), "sum lines: %.70s", out);
      CHECK(strstr(out, "\nallrow\t2\t1\t1\t5000\t0\t0\n") && strstr(out, "\nallrow\t20\t3\t6\t8\t1\t1007\n") && strstr(out, "\nallrow\t43\t3\t4\t7\t1\t1007\n"), "rows of the sum");
      CHECK(dm_ft_sum_text(&A, out, (size_t)n) == -2 && !out[0] && dm_ft_sum_text(&A, NULL, 5) == -1, "too small: nothing"); }



    { dm_ft_pages P; memset(&P, 0, sizeof P);
      unsigned o; int all;
      #define PAGES_OWNED() (o = 0, dm_ft_pages_owned(&P, 0, fake_sig, fake_items, NULL, &o))
      fake_reset();
      pg.fail[1] = 1; pg.owns[3] = DM_FT_OWN_SOJ;
      all = PAGES_OWNED();
      CHECK(!all && pg.builds == 1 && pg.built[1] == 1 && !o, "first call: page 2 read and failed (%d builds)", pg.builds);
      all = PAGES_OWNED();
      CHECK(!all && pg.builds == 2 && pg.built[2] == 1, "the next page next");
      all = PAGES_OWNED(); all = PAGES_OWNED();
      CHECK(!all && pg.built[3] == 1 && pg.built[4] == 1 && o == DM_FT_OWN_SOJ, "pages 4 and 5 read, the ring on page 4 owned");
      all = PAGES_OWNED();
      CHECK(!all && pg.built[1] == 2 && pg.builds == 5 && !pg.built[0], "the failed page read again with unchanged contents; the page shown never");
      all = PAGES_OWNED();
      CHECK(!all && pg.builds == 6 && pg.built[1] == 3, "only the failed page left: tried again (%d builds)", pg.builds);
      pg.fail[1] = 0;
      for (int i = 0; i < 3 && !all; i++) all = PAGES_OWNED();
      CHECK(all && o == DM_FT_OWN_SOJ && pg.built[1] >= 3, "the read succeeds: every page known (%d)", pg.built[1]);
      int b = pg.builds; all = PAGES_OWNED();
      CHECK(all && pg.builds == b && o == DM_FT_OWN_SOJ, "nothing changed: nothing built, still known");

      pg.fail[2] = 1; pg.sig[2]++; pg.sig[4]++; pg.owns[4] = DM_FT_OWN_STD;
      for (int i = 0; i < 4; i++) all = PAGES_OWNED();
      CHECK(!all && (o & DM_FT_OWN_STD) && pg.built[2] >= 2, "page 5 read while page 3 keeps failing (%u)", o);
      pg.fail[2] = 0; for (int i = 0; i < 3 && !all; i++) all = PAGES_OWNED();
      CHECK(all && o == (DM_FT_OWN_SOJ | DM_FT_OWN_STD), "recovered: both owned");

      pg.nosig[3] = 1; all = PAGES_OWNED(); CHECK(!all, "a signature not readable");
      pg.nosig[3] = 0; o = 0; CHECK(!dm_ft_pages_owned(&P, -1, fake_sig, fake_items, NULL, &o) && !o, "the page shown not known");
      #undef PAGES_OWNED
    }


    { dm_ft_cube_item b1[] = { I(1, "r01", 2, 0), I(2, "r01", 2, 0), I(3, "r01", 2, 0) }, a1[] = { I(9, "r02", 2, 0) };
      CHECK(CUBE(b1, a1) == DM_FT_E_RUNE_UP, "three runes into the next one");
      dm_ft_cube_item b2[] = { I(1, "r10", 2, 0), I(2, "r10", 2, 0), I(3, "r10", 2, 0), I(4, "gcy", 2, 0) }, a2[] = { I(9, "r11", 2, 0) };
      CHECK(CUBE(b2, a2) == DM_FT_E_RUNE_UP, "three runes and a chipped gem into the next one");
      dm_ft_cube_item b3[] = { I(1, "r20", 2, 0), I(2, "r01", 2, 0), I(3, "r01", 2, 0), I(4, "r01", 2, 0) }, a3[] = { I(1, "r20", 2, 0), I(9, "r02", 2, 0) };
      CHECK(CUBE(b3, a3) == DM_FT_E_RUNE_UP, "a higher rune lying beside does not hide the upgrade");
      dm_ft_cube_item b4[] = { I(1, "r01", 2, 0), I(2, "r01", 2, 0) };
      CHECK(CUBE(b4, b4) == 0, "near miss: two runes, nothing happens");
      dm_ft_cube_item b5[] = { I(1, "r05", 2, 0), I(2, "jew", 4, 0), I(3, "gpb", 2, 0), I(4, "mbt", 4, 0) }, a5[] = { I(9, "mbt", 8, 0) };
      CHECK(CUBE(b5, a5) == 0, "near miss: a rune consumed for a crafted item, no rune made");
      dm_ft_cube_item b6[] = { I(1, "gzv", 2, 0), I(2, "gzv", 2, 0), I(3, "gzv", 2, 0) }, a6[] = { I(9, "gpv", 2, 0) };
      CHECK(CUBE(b6, a6) == DM_FT_E_PERFECT, "three flawless amethysts into a perfect one");
      dm_ft_cube_item b7[] = { I(1, "skl", 2, 0), I(2, "skl", 2, 0), I(3, "skl", 2, 0) }, a7[] = { I(9, "skz", 2, 0) };
      CHECK(CUBE(b7, a7) == DM_FT_E_PERFECT, "three flawless skulls into a perfect one");
      dm_ft_cube_item b8[] = { I(1, "gsr", 2, 0), I(2, "gsr", 2, 0), I(3, "gsr", 2, 0) }, a8[] = { I(9, "glr", 2, 0) };
      CHECK(CUBE(b8, a8) == 0, "near miss: three rubies into a flawless one");
      dm_ft_cube_item b9[] = { I(1, "amu", 4, 0), I(2, "gpv", 2, 0), I(3, "gpw", 2, 0), I(4, "gpg", 2, 0), I(5, "gpr", 2, 0), I(6, "gpb", 2, 0), I(7, "gpy", 2, 0) },
                      a9[] = { I(9, "amu", 4, 0) };
      CHECK(CUBE(b9, a9) == DM_FT_E_PRISM, "a magic amulet and six perfect gems into the amulet");
      dm_ft_cube_item b10[] = { I(1, "amu", 4, 0), I(2, "gpv", 2, 0), I(3, "gpw", 2, 0), I(4, "gpg", 2, 0) }, a10[] = { I(9, "amu", 4, 0) };
      CHECK(CUBE(b10, a10) == 0, "near miss: three perfect gems reroll the amulet");
      dm_ft_cube_item b11[] = { I(1, "hax", 2, 0), I(2, "r09", 2, 0), I(3, "gcr", 2, 0) }, a11[] = { I(1, "hax", 2, 0) };
      CHECK(CUBE(b11, a11) == DM_FT_E_RECHARGE, "a weapon with the rune of rank 9 and a chipped gem: repaired and recharged");
      dm_ft_cube_item b12[] = { I(1, "hax", 2, 0), I(2, "r09", 2, 0) }, a12[] = { I(1, "hax", 2, 0) };
      CHECK(CUBE(b12, a12) == 0, "near miss: the rune alone only repairs");
      dm_ft_cube_item b13[] = { I(1, "r09", 2, 0), I(2, "r09", 2, 0), I(3, "r09", 2, 0) }, a13[] = { I(9, "r10", 2, 0) };
      CHECK(CUBE(b13, a13) == DM_FT_E_RUNE_UP, "three runes of rank 9: an upgrade, no recharge");
      dm_ft_cube_item b14[] = { I(1, "ghm", 2, 3), I(2, "r15", 2, 0), I(3, "tsc", 2, 0) }, a14[] = { I(1, "ghm", 2, 0) };
      CHECK(CUBE(b14, a14) == DM_FT_E_UNSOCKET, "the rune of rank 15 and a portal scroll clear a helm's sockets");
      dm_ft_cube_item a14b[] = { I(9, "ghm", 2, 0) };
      CHECK(CUBE(b14, a14b) == DM_FT_E_UNSOCKET, "the same when the helm comes back as a new unit");
      dm_ft_cube_item b15[] = { I(1, "ghm", 2, 0), I(2, "r15", 2, 0), I(3, "tsc", 2, 0) };
      CHECK(CUBE(b15, b15) == 0, "near miss: sockets empty, nothing happens");
      dm_ft_cube_item b16[] = { I(1, "ghm", 2, 3), I(2, "r08", 2, 0), I(3, "tsc", 2, 0) },
                      a16[] = { I(1, "ghm", 2, 0), I(21, "r30", 2, 0), I(22, "gpr", 2, 0), I(23, "r10", 2, 0) };
      CHECK(CUBE(b16, a16) == DM_FT_E_UNSOCKET, "the rune of rank 8 and a portal scroll empty a helm, its inserts come back: no upgrade read from them");
      CHECK(CUBE(b16, b16) == 0, "the same refused (nothing consumed): nothing");
      dm_ft_cube_item b17[] = { I(1, "ghm", 2, 0), I(2, "r08", 2, 0), I(3, "tsc", 2, 0) };
      CHECK(CUBE(b17, b17) == 0, "near miss: rank 8 on empty sockets, nothing happens");
      dm_ft_cube_item b18[] = { I(1, "uap", 2, 0), I(2, "r08", 2, 0) }, a18[] = { I(1, "uap", 2, 0) };
      CHECK(CUBE(b18, a18) == 0, "rank 8 repairing an armor: no event");
      dm_ft_cube_item b19[] = { I(1, "r08", 2, 0), I(2, "r08", 2, 0), I(3, "r08", 2, 0) }, a19[] = { I(9, "r09", 2, 0) };
      CHECK(CUBE(b19, a19) == DM_FT_E_RUNE_UP, "three runes of rank 8: still an upgrade");
      CHECK(dm_ft_cube_events(NULL, 0, NULL, 0) == 0, "an empty cube"); }


    CHECK(dm_ft_socket_events(C("gsv"), 1, 0, 0) == DM_FT_E_GEM && dm_ft_socket_events(C("skz"), 1, 0, 0) == DM_FT_E_GEM, "a gem, a skull");
    CHECK(dm_ft_socket_events(C("r04"), 1, 0, 0) == 0 && dm_ft_socket_events(C("jew"), 1, 0, 0) == 0, "a rune, a jewel: no gem");
    CHECK(dm_ft_socket_events(C("r26"), 1, 0, 1) == DM_FT_E_RUNEWORD, "the last rune forms the runeword");
    CHECK(dm_ft_socket_events(C("gsv"), 0, 0, 1) == 0 && dm_ft_socket_events(C("r26"), 1, 1, 1) == 0, "not placed; a runeword already there");


    { fresh(); step(); uint32_t seq = F.seq;
      CHECK(dm_ft_event(&F, DM_FT_E_GEM | DM_FT_E_RUNEWORD, ++T) == 2 && done(24) && done(25) && !done(26) && F.seq == seq + 2, "two events, two notices");
      CHECK(dm_ft_event(&F, DM_FT_E_GEM, ++T) == 0 && F.seq == seq + 2 && step() == 0, "nothing twice; facts never meet an event row");
      CHECK(dm_ft_event(&F, DM_FT_E_UNSOCKET, ++T) == 1 && done(30) && F.note[F.seq % DM_FT_NOTICES].id == 30, "sockets cleared");
      CHECK(dm_ft_count(&F, DM_FT_SALES, 999, ++T) == 0 && !done(35) && dm_ft_count(&F, DM_FT_SALES, 1, ++T) == 1 && done(35), "the 1 000th sale");
      CHECK(dm_ft_counter(&F, DM_FT_KILLS) == 0 && dm_ft_counter(&F, DM_FT_SALES) == 1000, "the counters apart");

      dm_ft_rec r[1] = { { DM_FT_SALES, DM_FT_REC_COUNT, 0, 0, 1200 } }; dm_ft_take(&F, r, 1, 2); seq = F.seq;
      CHECK(step() == 0 && done(35) && F.seq == seq && F.rev == DM_FT_REV, "revision 2 with 1 200 sales: silently"); }



    CHECK(dm_ft_gold_picked(1234, 0, 0) == 1234 && dm_ft_gold_picked(1234, 1, 1) == 1234, "a monster's or the game's pile: what was added");
    CHECK(dm_ft_gold_picked(1234, 1, 0) == 0, "the hero's own gold picked up again: nothing");
    CHECK(dm_ft_gold_picked(0, 0, 0) == 0 && dm_ft_gold_picked(-5, 0, 0) == 0, "nothing added (the purse full): nothing");
    CHECK(dm_ft_gold_picked(0x7fffffff, 0, 0) == 0x7fffffffu, "the largest add");

    { dm_ft_own_gold o; memset(&o, 0, sizeof o); int32_t g;
      CHECK(dm_ft_gold_pickup(&o, 50, 1000, 9000, 1, 0, 51) == 0 && dm_ft_own_gold_pending(&o, &g) == 51 && g == 9000, "own pile, 1 000 fit: nothing, the rest's id waits");
      dm_ft_own_gold_confirm(&o, 1);
      CHECK(dm_ft_own_gold_pending(&o, &g) == 0 && o.n == 1, "the rest confirmed");
      CHECK(dm_ft_gold_pickup(&o, 51, 4000, 5000, 0, 0, 60) == 0 && dm_ft_own_gold_pending(&o, &g) == 60 && g == 5000, "the rest picked up in part: nothing, its rest waits");
      dm_ft_own_gold_confirm(&o, 1);
      CHECK(dm_ft_gold_pickup(&o, 60, 5000, 0, 0, 0, 61) == 0 && o.n == 0 && dm_ft_own_gold_pending(&o, &g) == 0, "the last rest: nothing, nothing left");
      CHECK(dm_ft_gold_pickup(&o, 70, 300, 700, 0, 0, 71) == 300 && dm_ft_own_gold_pending(&o, &g) == 0, "a monster's pile in part: what fit counts, its rest is no one's");
      CHECK(dm_ft_gold_pickup(&o, 71, 700, 0, 0, 0, 72) == 700, "and the rest counts when picked up");
      dm_ft_gold_pickup(&o, 80, 0, 500, 1, 0, 81); dm_ft_own_gold_confirm(&o, 0);
      CHECK(o.n == 0 && dm_ft_gold_pickup(&o, 81, 500, 0, 0, 0, 82) == 500, "a rest not found on its tick: not remembered");
      for (uint32_t i = 0; i < DM_FT_OWN_PILES + 2; i++) { dm_ft_gold_pickup(&o, 100 + i, 1, 1, 1, 0, 200 + i); dm_ft_own_gold_confirm(&o, 1); }
      CHECK(o.n == DM_FT_OWN_PILES && dm_ft_gold_pickup(&o, 200, 1, 0, 0, 0, 0) == 1 && dm_ft_gold_pickup(&o, 209, 1, 0, 0, 0, 0) == 0, "full: the oldest rest forgotten, the newest kept"); }
    { fresh(); step(); uint32_t seq = F.seq;
      CHECK(dm_ft_count(&F, DM_FT_GOLD, 499999, ++T) == 0 && !done(36) && dm_ft_count(&F, DM_FT_GOLD, 1, ++T) == 1 && done(36) && !done(37), "500 000 gold");
      CHECK(dm_ft_count(&F, DM_FT_GOLD, 499999, ++T) == 0 && !done(37) && dm_ft_count(&F, DM_FT_GOLD, 7, ++T) == 1 && done(37) && F.seq == seq + 2, "1 000 000 gold");
      CHECK(dm_ft_counter(&F, DM_FT_GOLD) == 1000006 && dm_ft_counter(&F, DM_FT_SALES) == 0 && dm_ft_counter(&F, DM_FT_KILLS) == 0 && !done(1) && !done(35), "the counters apart");
      CHECK(dm_ft_count(&F, DM_FT_GOLD, 1, ++T) == 0 && F.seq == seq + 2, "nothing twice");

      dm_ft_rec r[1] = { { DM_FT_SALES, DM_FT_REC_COUNT, 0, 0, 100 } }; dm_ft_take(&F, r, 1, 3); seq = F.seq;
      CHECK(step() == 0 && !done(35) && !done(36) && !done(38) && F.seq == seq && F.rev == DM_FT_REV, "revision 3: nothing of revision 4 met");
      CHECK(dm_ft_count(&F, DM_FT_GOLD, 500000, ++T) == 1 && done(36) && F.seq == seq + 1, "then gold counts with a notice");

      char out[8192]; CHECK(dm_ft_text(&F, out, sizeof out) > 0 && strstr(out, "counter\t2\t500000\n") && strstr(out, "row\t37\t4\t16\t1000000\t0\t0\n")
            && strstr(out, "row\t38\t4\t14\t128\t0\t0\n"), "page lines: gold counter, rows 37 and 38"); }


    { dm_ft_cmds q; memset(&q, 0, sizeof q); memset(&W, 0, sizeof W); int sold, unread;
      uint8_t drop[5] = { 0x17, 90 }, buy[24] = { 0x32 };
      CHECK(dm_ft_gamble_seen(&q, 90, DM_FT_Q_UNIQUE, 100) == 1 && q.n == 1, "a gambled unique taken");
      CHECK(dm_ft_cmd_judge(&q, 101, &WR, NULL, &sold, &unread) == 0 && q.n == 1, "not held yet: waits");
      CHECK(dm_ft_cmd_seen(&q, drop, 5, 101, &WR, NULL) == 0 && dm_ft_cmd_seen(&q, buy, 24, 101, &WR, NULL) == 0 && q.n == 1, "item commands leave it waiting");
      w_hold(90, "ci0");
      CHECK(dm_ft_cmd_judge(&q, 102, &WR, NULL, &sold, &unread) == DM_FT_E_GAMBLE && q.n == 0 && sold == 0, "held: the event, once");
      CHECK(dm_ft_cmd_judge(&q, 103, &WR, NULL, &sold, &unread) == 0, "nothing after");
      CHECK(dm_ft_gamble_seen(&q, 91, 6, 200) == 1, "a gambled rare");
      w_hold(91, "rin"); CHECK(dm_ft_cmd_judge(&q, 201, &WR, NULL, &sold, &unread) == 0 && q.n == 0, "held, not unique: judged, nothing");
      CHECK(dm_ft_gamble_seen(&q, 92, DM_FT_Q_UNIQUE, 300) == 1, "a unique handed over, the purchase not finished");
      CHECK(dm_ft_cmd_judge(&q, 300 + DM_FT_CMD_TICKS - 1, &WR, NULL, &sold, &unread) == 0 && q.n == 1, "not held: waits its ticks");
      CHECK(dm_ft_cmd_judge(&q, 300 + DM_FT_CMD_TICKS, &WR, NULL, &sold, &unread) == 0 && q.n == 0, "never held: dropped, nothing");
      CHECK(dm_ft_gamble_seen(&q, 93, DM_FT_Q_UNIQUE, 400) == 1, "another");
      W.unread = 1; CHECK(dm_ft_cmd_judge(&q, 401, &WR, NULL, &sold, &unread) == 0 && unread == 1 && q.n == 0, "not readable: dropped and counted"); W.unread = 0;
      for (int i = 0; i < DM_FT_CMDS; i++) dm_ft_gamble_seen(&q, 100 + (uint32_t)i, 4, 500);
      CHECK(dm_ft_gamble_seen(&q, 200, DM_FT_Q_UNIQUE, 500) == -1, "a full queue takes no more");
      fresh(); step(); uint32_t seq = F.seq;
      CHECK(dm_ft_event(&F, DM_FT_E_GAMBLE, ++T) == 1 && done(38) && F.seq == seq + 1 && dm_ft_event(&F, DM_FT_E_GAMBLE, ++T) == 0, "the gamble row once"); }


    { dm_ft_cmds q; memset(&q, 0, sizeof q); memset(&W, 0, sizeof W); int sold, unread;
      uint8_t sock[21] = { 0x28, 7, 0, 0, 0, 8, 0, 0, 0 }, cube[8] = { 0x20 }, sell[17] = { 0x33, 1, 0, 0, 0, 7, 0, 0, 0 }, other[5] = { 0x19, 7 };
      w_hold(7, "gsv"); w_hold(8, "ghm"); W.target = 8;
      CHECK(dm_ft_cmd_seen(&q, other, 5, 100, &WR, NULL) == 0 && q.n == 0, "another command: not taken");
      CHECK(dm_ft_cmd_seen(&q, sock, 21, 100, &WR, NULL) == 1 && q.n == 1, "a socketing taken");
      CHECK(dm_ft_cmd_judge(&q, 101, &WR, NULL, &sold, &unread) == 0 && q.n == 1, "not in the socket yet: waits");
      CHECK(dm_ft_cmd_judge(&q, 100 + DM_FT_CMD_TICKS, &WR, NULL, &sold, &unread) == 0 && q.n == 0, "refused: dropped after its ticks, nothing");
      CHECK(dm_ft_cmd_seen(&q, sock, 21, 200, &WR, NULL) == 1, "again");
      W.placed = 1; w_drop(7);
      CHECK(dm_ft_cmd_judge(&q, 201, &WR, NULL, &sold, &unread) == DM_FT_E_GEM && q.n == 0, "the gem in the socket: the event, once");
      CHECK(dm_ft_cmd_judge(&q, 202, &WR, NULL, &sold, &unread) == 0, "nothing after");
      CHECK(dm_ft_cmd_seen(&q, sock, 21, 203, &WR, NULL) == 0, "a socketing of an item not held: not taken");
      sock[5] = 9; w_hold(7, "gsv"); W.placed = 0;
      CHECK(dm_ft_cmd_seen(&q, sock, 21, 204, &WR, NULL) == 0 && q.n == 0, "into a target not held: not taken");

      sock[1] = 5; sock[5] = 8; w_hold(5, "r26");
      CHECK(dm_ft_cmd_seen(&q, sock, 21, 210, &WR, NULL) == 1, "the last rune");
      W.placed = 1; W.runeword = 1;
      CHECK(dm_ft_cmd_judge(&q, 211, &WR, NULL, &sold, &unread) == DM_FT_E_RUNEWORD, "the runeword formed");

      W.cube[0] = I(1, "r01", 2, 0); W.cube[1] = I(2, "r01", 2, 0); W.ncube = 2;
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 300, &WR, NULL) == 1, "a transmutation taken");
      CHECK(dm_ft_cmd_judge(&q, 301, &WR, NULL, &sold, &unread) == 0 && q.n == 1, "the cube unchanged: waits");
      CHECK(dm_ft_cmd_judge(&q, 300 + DM_FT_CMD_TICKS, &WR, NULL, &sold, &unread) == 0 && q.n == 0, "refused: dropped");
      W.cube[2] = I(3, "r01", 2, 0); W.ncube = 3;
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 400, &WR, NULL) == 1, "three runes");
      W.cube[0] = I(9, "r02", 2, 0); W.ncube = 1;
      CHECK(dm_ft_cmd_judge(&q, 401, &WR, NULL, &sold, &unread) == DM_FT_E_RUNE_UP && q.n == 0, "upgraded");


      w_hold(20, "hax"); w_hold(21, "bst");
      sell[5] = 20; CHECK(dm_ft_cmd_seen(&q, sell, 17, 500, &WR, NULL) == 1, "a sale taken");
      w_drop(20); W.gold += 100;
      CHECK(dm_ft_cmd_judge(&q, 501, &WR, NULL, &sold, &unread) == 0 && sold == 1 && q.n == 0, "sold on the next tick");
      sell[5] = 21; CHECK(dm_ft_cmd_seen(&q, sell, 17, 502, &WR, NULL) == 1, "the next sale");
      CHECK(dm_ft_cmd_judge(&q, 502, &WR, NULL, &sold, &unread) == 0 && sold == 0 && q.n == 1, "refused so far");
      sell[5] = 20; CHECK(dm_ft_cmd_seen(&q, sell, 17, 503, &WR, NULL) == 0 && q.n == 0, "the sold item's command again: not taken, the waiting sale dropped");
      sell[5] = 21; dm_ft_cmd_seen(&q, sell, 17, 504, &WR, NULL); sell[5] = 22; w_hold(22, "bst");
      CHECK(dm_ft_cmd_seen(&q, sell, 17, 504, &WR, NULL) == 1 && q.n == 1 && q.cmd[0].item == 22, "two sales before the tick: only the second waits");
      q.n = 0;
      sell[5] = 20; CHECK(dm_ft_cmd_seen(&q, sell, 17, 503, &WR, NULL) == 0, "the sold item's command again: not taken");
      sell[5] = 21; CHECK(dm_ft_cmd_seen(&q, sell, 17, 505, &WR, NULL) == 1, "a refused sale");
      CHECK(dm_ft_cmd_judge(&q, 505 + DM_FT_CMD_TICKS, &WR, NULL, &sold, &unread) == 0 && sold == 0 && q.n == 0, "the refused sale dropped, nothing");

      sell[5] = 20; CHECK(dm_ft_cmd_seen(&q, sell, 17, 600, &WR, NULL) == 0, "a sale of an item not held");
      sell[5] = 21; CHECK(dm_ft_cmd_seen(&q, sell, 17, 600, &WR, NULL) == 1, "a sale");
      CHECK(dm_ft_cmd_seen(&q, sell, 17, 600, &WR, NULL) == 1 && q.n == 1, "the same sale again: the waiting one dropped, waiting once");
      W.unread = 1;
      CHECK(dm_ft_cmd_judge(&q, 601, &WR, NULL, &sold, &unread) == 0 && sold == 0 && unread == 1 && q.n == 0, "not readable after: dropped");
      CHECK(dm_ft_cmd_seen(&q, sell, 17, 602, &WR, NULL) == -1 && q.n == 0, "not readable before: not taken");
      W.unread = 0;
      sock[1] = 7; sock[5] = 8; w_hold(7, "gsv"); W.target = 8; W.placed = 0;
      for (int i = 0; i < DM_FT_CMDS; i++) dm_ft_cmd_seen(&q, sock, 21, 700, &WR, NULL);
      CHECK(q.n == DM_FT_CMDS && dm_ft_cmd_seen(&q, cube, 8, 700, &WR, NULL) == -1, "a full queue takes no more");
      CHECK(dm_ft_cmd_seen(&q, sell, 5, 700, &WR, NULL) == 0, "a sale command too short");
      q.n = 0;


      memset(&W, 0, sizeof W); W.cube[0] = I(30, "r01", 2, 0); W.ncube = 1; w_hold(31, "r10");
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 800, &WR, NULL) == 1, "a transmutation of one rune");
      W.cube[0] = I(31, "r10", 2, 0); w_drop(31); w_hold(30, "r01");
      CHECK(dm_ft_cmd_judge(&q, 802, &WR, NULL, &sold, &unread) == 0, "a swap by hand: no upgrade");
      W.cube[0] = I(32, "gzv", 2, 0); w_drop(30); w_hold(33, "gpv");
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 810, &WR, NULL) == 1, "a transmutation of one flawless gem");
      W.cube[0] = I(33, "gpv", 2, 0); w_drop(33); w_hold(32, "gzv");
      CHECK(dm_ft_cmd_judge(&q, 812, &WR, NULL, &sold, &unread) == 0, "a perfect gem swapped in by hand: nothing");

      w_drop(32); CHECK(dm_ft_cmd_judge(&q, 813, &WR, NULL, &sold, &unread) == 0, "moved out and dropped: still nothing");
      q.n = 0;

      memset(&W, 0, sizeof W); W.cube[0] = I(40, "r01", 2, 0); W.cube[1] = I(41, "r01", 2, 0); W.cube[2] = I(42, "r01", 2, 0); W.ncube = 3;
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 900, &WR, NULL) == 1, "three runes");
      W.cube[0] = I(43, "r02", 2, 0); W.ncube = 1;
      CHECK(dm_ft_cmd_judge(&q, 901, &WR, NULL, &sold, &unread) == DM_FT_E_RUNE_UP, "the new unit: upgraded");


      memset(&W, 0, sizeof W); q.n = 0; W.gold = 1000; w_hold(50, "hax"); sell[5] = 50;
      CHECK(dm_ft_cmd_seen(&q, sell, 17, 1000, &WR, NULL) == 1 && dm_ft_cmd_judge(&q, 1001, &WR, NULL, &sold, &unread) == 0 && sold == 0, "refused so far");
      CHECK(dm_ft_cmd_seen(&q, sell, 17, 1002, &WR, NULL) == 1 && q.n == 1, "the second try of the same sale: waiting once");
      w_drop(50); W.gold = 1200;
      CHECK(dm_ft_cmd_judge(&q, 1003, &WR, NULL, &sold, &unread) == 0 && sold == 1 && q.n == 0, "sold once");

      w_hold(50, "hax"); CHECK(dm_ft_cmd_seen(&q, sell, 17, 1100, &WR, NULL) == 1, "the item bought back and sold again");
      w_drop(50); W.gold = 1300;
      CHECK(dm_ft_cmd_judge(&q, 1101, &WR, NULL, &sold, &unread) == 0 && sold == 1, "a second sale");

      w_hold(51, "bst"); sell[5] = 51;
      CHECK(dm_ft_cmd_seen(&q, sell, 17, 1200, &WR, NULL) == 1, "a sale");
      w_drop(51);
      CHECK(dm_ft_cmd_judge(&q, 1203, &WR, NULL, &sold, &unread) == 0 && sold == 0 && q.n == 0, "dropped, not sold: judged and nothing counted");


      memset(&W, 0, sizeof W); q.n = 0; W.gold = 500; w_hold(60, "hax"); sell[5] = 60;
      uint8_t drop[5] = { 0x17, 60 }, split[13] = { 0x63, 61 }, take[17] = { 0x19, 70 }, walk[5] = { 0x03 };
      CHECK(dm_ft_cmd_seen(&q, sell, 17, 1300, &WR, NULL) == 1 && dm_ft_cmd_judge(&q, 1301, &WR, NULL, &sold, &unread) == 0 && sold == 0, "refused so far");
      W.gold = 900;
      CHECK(dm_ft_cmd_seen(&q, walk, 5, 1302, &WR, NULL) == 0 && q.n == 1, "walking leaves the sale waiting");
      CHECK(dm_ft_cmd_seen(&q, drop, 5, 1303, &WR, NULL) == 0 && q.n == 0, "the drop: the waiting sale dropped");
      w_drop(60);
      CHECK(dm_ft_cmd_judge(&q, 1304, &WR, NULL, &sold, &unread) == 0 && sold == 0, "dropped with gold from elsewhere: no sale");

      memset(&W, 0, sizeof W); W.cube[0] = I(60, "r01", 2, 0); W.ncube = 1;
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 1400, &WR, NULL) == 1, "a transmutation of one rune");
      CHECK(dm_ft_cmd_seen(&q, split, 13, 1400, &WR, NULL) == 0 && q.n == 0, "the split: the waiting transmutation dropped");
      W.cube[0] = I(61, "r10", 2, 0);
      CHECK(dm_ft_cmd_judge(&q, 1401, &WR, NULL, &sold, &unread) == 0, "a new unit swapped in by hand: nothing");

      memset(&W, 0, sizeof W); W.cube[0] = I(70, "r01", 2, 0); W.cube[1] = I(71, "r01", 2, 0); W.cube[2] = I(72, "r01", 2, 0); W.ncube = 3;
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 1500, &WR, NULL) == 1, "three runes");
      W.cube[0] = I(73, "r02", 2, 0); W.ncube = 1;
      CHECK(dm_ft_cmd_judge(&q, 1501, &WR, NULL, &sold, &unread) == DM_FT_E_RUNE_UP, "upgraded, judged on the next tick");
      take[1] = 73; CHECK(dm_ft_cmd_seen(&q, take, 17, 1502, &WR, NULL) == 0 && q.n == 0, "the result taken out after: nothing waiting, counted already");
      W.cube[0] = I(80, "r01", 2, 0); W.cube[1] = I(81, "r01", 2, 0); W.cube[2] = I(82, "r01", 2, 0); W.ncube = 3;
      CHECK(dm_ft_cmd_seen(&q, cube, 8, 1600, &WR, NULL) == 1, "three runes again");
      W.cube[0] = I(83, "r02", 2, 0); W.ncube = 1;
      take[1] = 83; CHECK(dm_ft_cmd_seen(&q, take, 17, 1600, &WR, NULL) == 0 && q.n == 0, "the result taken out before the next tick");
      W.ncube = 0;
      CHECK(dm_ft_cmd_judge(&q, 1601, &WR, NULL, &sold, &unread) == 0, "taken out before the tick: missed (the accepted price)"); }



    { static const struct { int *field; int below, reached, id; } th[] = {
          { &X.streak, 19, 20, 44 }, { &X.streak, 64, 65, 45 }, { &X.streak, 249, 250, 46 }, { &X.bosses, 63, 64, 47 },
          { &X.rift_best, 9, 10, 53 }, { &X.rift_best, 29, 30, 54 }, { &X.shrines, 3, 4, 57 } };
      for (unsigned i = 0; i < sizeof th / sizeof *th; i++) {
          fresh(); step(); *th[i].field = th[i].below;
          step(); CHECK(!done(th[i].id), "row %d: %d is not enough", th[i].id, th[i].below);
          *th[i].field = th[i].reached;
          CHECK(step() >= 1 && done(th[i].id) && F.note[F.seq % DM_FT_NOTICES].id == th[i].id, "row %d: %d enters it with a notice", th[i].id, th[i].reached);
      }

      { static const struct { int t, want; } fast[] = { { 250, 0 }, { 0, 0 }, { 249, 1 } };
        for (unsigned k = 0; k < sizeof fast / sizeof *fast; k++) {
            dm_ft_rec none[1]; dm_ft_take(&F, none, 0, 4); facts(10, 0); X.boss_fast = fast[k].t; uint32_t seq = F.seq;
            CHECK(step() == 0 && done(48) == fast[k].want && F.seq == seq, "an act boss in %d ticks on the ranking: %s", fast[k].t, fast[k].want ? "entered silently" : "nothing");
        } }
      fresh(); step(); X.rift_fast = 300; CHECK(step() == 0 && !done(55), "a tier from 10 on in 5:00: not under 5 minutes");
      X.rift_fast = 299; CHECK(step() == 1 && done(55), "4:59: entered");
      fresh(); step(); X.rift_timed = 1; CHECK(step() == 1 && done(52) && !done(53), "a rift cleared in time"); }

    { dm_ft_board_row b[4] = { { 156, { 1, 2, 5 }, { 300, 240, 0 } }, { -1, { 3, 0, 0 }, { 10, 0, 0 } }, { 267, { 1, 0, 0 }, { 100, 0, 0 } }, { 242, { 0, 0, 0 }, { 0, 0, 0 } } };
      dm_ft_facts x; memset(&x, 0, sizeof x); dm_ft_board(&x, b, 4);
      CHECK(x.bosses == 3 && x.boss_fast == 240, "three rows killed (Andariel on three difficulties once), the fastest act boss 240: %d %d", x.bosses, x.boss_fast);
      b[0].best[1] = 0; dm_ft_board(&x, b, 4);
      CHECK(x.boss_fast == 300, "a super unique's and Blood Raven's times are no act boss's: %d", x.boss_fast);
      b[0].n[0] = b[0].n[1] = b[0].n[2] = 0; dm_ft_board(&x, b, 4);
      CHECK(x.bosses == 2 && x.boss_fast == 0, "a time without a kill counts nothing");
      CHECK(dm_ft_act_boss(156) && dm_ft_act_boss(211) && dm_ft_act_boss(242) && dm_ft_act_boss(243) && dm_ft_act_boss(544) && !dm_ft_act_boss(267) && !dm_ft_act_boss(256),
            "the act bosses by class"); }

    { int fast[DM_FT_RIFT_TIERS + 1] = { 0 }; dm_ft_facts x; memset(&x, 0, sizeof x);
      fast[5] = DM_RIFT_SECS; dm_ft_rift(&x, 4, fast);
      CHECK(x.rift_best == 4 && !x.rift_timed && !x.rift_fast, "a time of 15:00 is no clear in time");
      fast[9] = 100; dm_ft_rift(&x, 9, fast);
      CHECK(x.rift_timed && !x.rift_fast, "tier 9 in 1:40: cleared in time, no fast rift below tier 10");
      fast[10] = 299; fast[12] = 250; fast[30] = 0; dm_ft_rift(&x, 12, fast);
      CHECK(x.rift_fast == 250 && x.rift_best == 12, "the fastest from tier 10 on: %d", x.rift_fast);
      fresh(); step(); X.rift_best = 9; X.rift_timed = 1; X.rift_fast = 0; step();
      CHECK(done(52) && !done(53) && !done(55), "only from tier 10: a fast tier 9 enters nothing of tier 10"); }

    { CHECK(dm_ft_equip_worthy(100, 125, 80, 80) && !dm_ft_equip_worthy(100, 124.9, 80, 80) && dm_ft_equip_worthy(100, 100, 80, 100)
            && !dm_ft_equip_worthy(100, 100, 80, 99.9) && dm_ft_equip_worthy(0, 0, 80, 100) && !dm_ft_equip_worthy(0, 500, 0, 500), "the 25 %% damage step, group or single, not from 0"); }

    { uint32_t bits[8] = { 0 }; bits[4] = 3; CHECK(dm_ft_shrines(bits, 8) == 2, "two shrine states");
      bits[4] |= 1u << 9; bits[3] = 1u << 31; bits[4] |= 1u << 10; CHECK(dm_ft_shrines(bits, 8) == 3, "states 128, 129, 137; 127 and 138 are no shrines");
      CHECK(dm_ft_shrines(bits, 4) == 0, "words not read: none");
      CHECK(dm_ft_potion(C("hp1")) && dm_ft_potion(C("mp5")) && dm_ft_potion(C("rvl")) && dm_ft_potion(C("rvs")) && dm_ft_potion(C("yps")) && dm_ft_potion(C("vps"))
            && dm_ft_potion(C("wms")) && !dm_ft_potion(C("hp6")) && !dm_ft_potion(C("tsc")) && !dm_ft_potion(C("isc")) && !dm_ft_potion(C("box")), "potion codes"); }

    { static dm_fl_fight f; dm_ft_duel d;
      #define FIGHT(end_, diff_, cls_, flags_, done_) do { memset(&f, 0, sizeof f); f.h.end = (end_); f.h.difficulty = (diff_); f.h.ngroups = 2; f.h.nbuckets = 4; \
          f.g[0] = (dm_fl_group){ .cls = 5, .su = 0xffff, .kills = 3, .t_done = 40 }; f.g[1] = (dm_fl_group){ .cls = (cls_), .su = 0xffff, .flags = (flags_), .t_first = 20, .kills = (done_) ? 1 : 0, .t_done = (done_) ? 900 : 0 }; } while (0)
      #define WON_END() (dm_ft_duel_won(&d), dm_ft_duel_end(&d, &f))
      memset(&d, 0, sizeof d); dm_ft_duel_life(&d, 80, 100); FIGHT(DM_FL_END_KILL, 0, 156, 0, 1);
      CHECK(WON_END() == DM_FT_E_CALM, "Andariel on Normal, life never below half: calm; no potion counts only from Nightmare on");
      memset(&d, 0, sizeof d); dm_ft_duel_life(&d, 50, 100); FIGHT(DM_FL_END_KILL, 2, 544, 0, 1);
      CHECK(WON_END() == (DM_FT_E_CALM | DM_FT_E_DRY), "Baal on Hell at exactly half, no potion: calm and dry");
      memset(&d, 0, sizeof d); dm_ft_duel_life(&d, 4999, 10000); dm_ft_duel_potion(&d); FIGHT(DM_FL_END_KILL, 2, 544, 0, 1);
      CHECK(WON_END() == 0, "a reading at 49.99 %% and a potion command: nothing");
      memset(&d, 0, sizeof d); dm_ft_duel_below_half(&d); FIGHT(DM_FL_END_KILL, 2, 544, 0, 1);
      CHECK(WON_END() == DM_FT_E_DRY, "a hit below half of the maximum at that hit: not calm");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_KILL, 1, 242, 0, 1); f.b[2].me = DM_FL_ME_POTION;
      CHECK(WON_END() == (DM_FT_E_CALM | DM_FT_E_DRY), "a belt column that only lost a potion (moved, dropped) is no potion drunk: still dry");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_KILL, 2, 243, 0, 1); dm_ft_duel_won(&d);
      dm_ft_duel_potion(&d); dm_ft_duel_life(&d, 10, 100);
      CHECK(dm_ft_duel_end(&d, &f) == (DM_FT_E_CALM | DM_FT_E_DRY), "a potion and a hit in the grace after the victory change nothing");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_KILL, 2, 243, 0, 1);
      CHECK(dm_ft_duel_end(&d, &f) == 0, "a kill end without a victory seen: nothing");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_DEATH, 2, 243, 0, 1);
      CHECK(WON_END() == 0, "Diablo dead, the hero died too: nothing");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_KILL, 2, 243, DM_FL_G_OTHERS, 1);
      CHECK(WON_END() == 0, "the boss among the others: not known to be the boss");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_KILL, 2, 6, 0, 1);
      CHECK(WON_END() == 0, "a fight without an act boss: nothing");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_KILL, 2, 267, 0, 1);
      CHECK(WON_END() == 0, "Blood Raven is no act boss");

      memset(&d, 0, sizeof d); dm_ft_duel_life(&d, 10, 100); dm_ft_duel_potion(&d); FIGHT(DM_FL_END_AREA, 2, 211, 0, 0);
      CHECK(dm_ft_duel_end(&d, &f) == 0 && d.ncarry == 1, "fled from Duriel: nothing, carried");
      dm_ft_duel_idle(&d); dm_ft_duel_potion(&d); dm_ft_duel_idle(&d);
      dm_ft_duel_life(&d, 90, 100); FIGHT(DM_FL_END_KILL, 2, 211, 0, 1);
      CHECK(WON_END() == 0 && d.ncarry == 0, "back with full life: Duriel killed, the first fight still counts against the hero");
      dm_ft_duel_idle(&d); FIGHT(DM_FL_END_KILL, 2, 211, 0, 1);
      CHECK(WON_END() == (DM_FT_E_CALM | DM_FT_E_DRY), "a later Duriel of the game starts clean");

      memset(&d, 0, sizeof d); dm_ft_duel_potion(&d); dm_ft_duel_idle(&d); FIGHT(DM_FL_END_KILL, 1, 156, 0, 1);
      CHECK(WON_END() == (DM_FT_E_CALM | DM_FT_E_DRY), "a potion before the fight: still dry");

      memset(&d, 0, sizeof d); dm_ft_duel_life(&d, 1, 100); FIGHT(DM_FL_END_PAUSE, 2, 243, DM_FL_G_OTHERS, 0);
      CHECK(dm_ft_duel_end(&d, &f) == 0 && d.ncarry == DM_FT_DUEL_BOSSES, "a survivor among the others: every act boss carries");
      dm_ft_duel_idle(&d); FIGHT(DM_FL_END_KILL, 2, 243, 0, 1);
      CHECK(WON_END() == DM_FT_E_DRY, "Diablo killed later: not calm");

      static const struct { int64_t life, max; uint32_t cls; unsigned want; } esc[] = {
          { 4, 100, 242, DM_FT_E_ESCAPE }, { 5, 100, 242, 0 }, { 0, 100, 242, 0 }, { 499, 10000, 242, DM_FT_E_ESCAPE }, { 4, 100, 156, 0 }, { 4, 0, 242, 0 } };
      for (unsigned i = 0; i < sizeof esc / sizeof *esc; i++) {
          memset(&d, 0, sizeof d); dm_ft_duel_life(&d, 3, 100); dm_ft_duel_potion(&d); dm_ft_duel_fatal(&d, esc[i].cls, esc[i].life, esc[i].max);
          FIGHT(DM_FL_END_KILL, 2, 242, 0, 1);
          CHECK(WON_END() == esc[i].want, "the fatal hit on class %u with %lld of %lld life", esc[i].cls, (long long)esc[i].life, (long long)esc[i].max);
      }
      memset(&d, 0, sizeof d); dm_ft_duel_life(&d, 3, 100); FIGHT(DM_FL_END_KILL, 2, 242, 0, 1);
      CHECK(!(WON_END() & DM_FT_E_ESCAPE), "no fatal hit seen (poison): not judged");
      memset(&d, 0, sizeof d); dm_ft_duel_fatal(&d, 242, 2, 100); dm_ft_duel_idle(&d); FIGHT(DM_FL_END_KILL, 2, 242, 0, 1);
      CHECK(!(WON_END() & DM_FT_E_ESCAPE), "a fatal hit before an idle tick is not this fight's");
      memset(&d, 0, sizeof d); dm_ft_duel_fatal(&d, 242, 2, 100); FIGHT(DM_FL_END_DEATH, 2, 242, 0, 1);
      CHECK(WON_END() == 0, "escaped at 2 %% but died in the same fight: nothing");
      memset(&d, 0, sizeof d); dm_ft_duel_fatal(&d, 5, 2, 100); CHECK(d.now.nfatal == 0, "fatal hits on other monsters are not kept");

      memset(&d, 0, sizeof d); dm_ft_duel_potion(&d); dm_ft_duel_life(&d, 1, 100); FIGHT(DM_FL_END_KILL, 0, 156, 0, 1); f.g[1].t_first = 20; f.g[1].t_done = 20 + 249;
      CHECK(dm_ft_duel_judge(&d, &f) == 0 && WON_END() == DM_FT_E_FAST, "Andariel dead 249 ticks after first contact: fast; the judgement is the victory's");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_KILL, 0, 156, 0, 1); f.g[1].t_first = 20; f.g[1].t_done = 20 + 250;
      CHECK(!(WON_END() & DM_FT_E_FAST), "250 ticks: not under 10 s");
      memset(&d, 0, sizeof d); FIGHT(DM_FL_END_DEATH, 0, 156, 0, 1); f.g[1].t_first = 20; f.g[1].t_done = 30;
      CHECK(WON_END() == 0, "both dead within 10 s: a lost fight is not fast");
      #undef FIGHT
      #undef WON_END

      fresh(); step(); uint32_t seq = F.seq;
      CHECK(dm_ft_event(&F, DM_FT_E_CALM | DM_FT_E_ESCAPE, ++T) == 2 && done(49) && done(51) && !done(50) && F.seq == seq + 2, "two fight events");
      CHECK(dm_ft_event(&F, DM_FT_E_CALM, ++T) == 0, "not twice");
      X.boss_fast = 100; CHECK(step() == 0 && !done(48), "the ranking's fast time later (it may be a lost fight's): nothing");
      CHECK(dm_ft_event(&F, DM_FT_E_FAST, ++T) == 1 && done(48), "a won fast fight enters the row"); }

    { fresh(); step(); uint16_t ids[4]; uint32_t seq = F.seq;
      int k = dm_ft_stage(&F, DM_FT_E_CALM | DM_FT_E_RUSH, ++T, ids, 4);
      CHECK(k == 2 && done(49) && done(56) && F.seq == seq, "staged silently");
      CHECK(dm_ft_stage(&F, DM_FT_E_CALM, ++T, ids + 2, 2) == 0, "not twice");
      dm_ft_unstage(&F, ids, 1); CHECK(!done(ids[0]) && done(ids[1]), "unstaged: out again");
      CHECK(dm_ft_notice(&F, ids + 1, 1) == 1 && F.seq == seq + 1 && F.note[F.seq % DM_FT_NOTICES].id == ids[1], "noticed once its save committed"); }

    { dm_sk_hist *h = dm_sk_hist_core(); uint8_t img[DM_SK_IMAGE_MAX]; memset(img, 0, sizeof img);
      dm_sk_rec rec[DM_SK_DIFFS] = { { 30, 400, 0, 8, 0, 1 }, { 0 }, { 70, 900, 0, 107, 2, 1 } };
      memcpy(img + DM_SK_IMAGE_HEAD, rec, sizeof rec);
      uint32_t v = DM_SK_MAGIC, ver = DM_SK_VERSION, cnt = 0; memcpy(img, &v, 4); memcpy(img + 4, &ver, 4); memcpy(img + 8, &cnt, 4);
      size_t n = DM_SK_IMAGE_HEAD + sizeof rec; uint32_t crc = sk_crc32(img + DM_SK_IMAGE_HEAD, n - DM_SK_IMAGE_HEAD); memcpy(img + 12, &crc, 4);
      CHECK(dm_sk_hist_owner(h, "Ann", img, n) == 0, "a made-up streak history file");
      int best = 0; for (int dd = 0; dd < DM_SK_DIFFS; dd++) { dm_sk_rec r; if (dm_sk_hist_record(h, dd, &r) && (int)r.kills > best) best = (int)r.kills; }
      CHECK(best == 70, "its best record: %d", best);
      dm_ft_rec none[1]; dm_ft_take(&F, none, 0, 4); facts(80, 1);
      X.streak = best; X.rift_best = 30; X.rift_timed = 1; X.rift_fast = 250; X.shrines = 4; X.known = DM_FT_K_ALL & ~DM_FT_K_KILLS;
      dm_ft_board_row b[2] = { { 156, { 1, 1, 1 }, { 200, 0, 0 } }, { -1, { 0, 4, 0 }, { 0, 0, 0 } } }; dm_ft_board(&X, b, 2);
      uint32_t seq = F.seq;
      CHECK(step() == 0 && done(44) && done(45) && !done(46) && done(52) && done(53) && done(54) && done(55) && done(57) && !done(47) && !done(48)
            && F.seq == seq && F.rev == 4, "revision 4: met rows enter silently, the boss ranking's wait, the revision stays");
      X.known = DM_FT_K_ALL;
      CHECK(step() == 0 && done(48) && !done(47) && F.seq == seq && F.rev == DM_FT_REV, "the boss ranking read: its met row silently, the revision moves on");
      X.bosses = DM_FT_BOARD_ROWS; CHECK(step() == 1 && done(47), "every row of the ranking afterwards: a notice");
      CHECK(!done(49) && !done(50) && !done(51) && !done(56) && !done(58) && !done(59) && !done(60), "events never enter silently");
      { uint16_t ids[4];
        CHECK(dm_ft_stage_count(&F, DM_FT_LOOKS, 4, ++T, ids, 4) == 0 && !done(59) && dm_ft_counter(&F, DM_FT_LOOKS) == 4, "four items given a look: not enough");
        CHECK(dm_ft_stage_count(&F, DM_FT_LOOKS, 1, ++T, ids, 4) == 1 && ids[0] == 59 && done(59) && !done(60), "the fifth: the look row");
        CHECK(dm_ft_stage_count(&F, DM_FT_DYES, 5, ++T, ids, 4) == 1 && ids[0] == 60 && done(60), "five coloured items at once: the colour row");
        CHECK(dm_ft_stage_count(&F, DM_FT_LOOKS, 3, ++T, ids, 4) == 0 && dm_ft_counter(&F, DM_FT_LOOKS) == 8, "more: counted, nothing twice");
        CHECK(step() == 0 && kill(1) == 0, "nothing else enters them"); }
      X.shrines = 0; X.streak = 0; CHECK(step() == 0 && done(57) && done(44), "entries stay when the facts fall back");
      char out[8192]; CHECK(dm_ft_text(&F, out, sizeof out) > 0 && strstr(out, "\nrow\t44\t7\t17\t20\t1\t") && strstr(out, "\nrow\t52\t8\t20\t1\t1\t")
            && strstr(out, "\nrow\t60\t6\t25\t5\t1\t") && strstr(out, "\nrow\t59\t6\t24\t5\t1\t") && strstr(out, "\ncounter\t3\t8\n") && strstr(out, "\nrow\t47\t7\t18\t64\t1\t"), "page lines of revision 5"); }







    { static const struct { uint8_t word, n, done; uint16_t seq[6]; } seen[] = {
          { 17, 1, 0, { 0x2001 } },
          { 18, 3, 2, { 0x0004, 0x0084, 0x2085 } },
          { 19, 5, 4, { 0x0014, 0x0234, 0x0274, 0x0374, 0x23f5 } },
          { 20, 3, 2, { 0x0040, 0x0042, 0x2021 } },
          { 21, 4, 3, { 0x0008, 0x2018, 0x0018, 0x0009 } },
          { 22, 2, 1, { 0x0040, 0x2841 } },
          { 25, 3, 2, { 0x2002, 0x2022, 0x2001 } },
          { 26, 3, 1, { 0x0004, 0x2001, 0x2201 } },
          { 27, 3, 2, { 0x0008, 0x2002, 0x2001 } },
      };
      static const uint16_t owner[DM_FT_WORDS] = { [0] = 0x0001, [1] = 0x1001, [2] = 0x101d, [3] = 0x000c, [4] = 0x101d, [5] = 0x1055, [6] = 0x1019,
          [7] = 0x0001, [8] = 0x0001, [9] = 0x000c, [10] = 0x0878, [11] = 0x1005, [12] = 0x0184, [41] = 0x8002 };
      static const uint8_t quest[DM_FT_QUESTS] = { 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20, 21, 22, 25, 26, 27, 35, 36, 37, 38, 39, 40 };
      uint16_t all[DM_FT_WORDS] = { 0 };
      for (int i = 0; i < DM_FT_QUESTS; i++) all[quest[i]] = owner[quest[i]] & 1 ? owner[quest[i]] : 0x0001;
      for (size_t k = 0; k < sizeof seen / sizeof *seen; k++) all[seen[k].word] = seen[k].seq[seen[k].n - 1];
      CHECK(dm_ft_quests(owner) == 6 && dm_ft_quests(all) == DM_FT_QUESTS, "a real Normal set: 6 of 27; every done word: 27 (%d %d)", dm_ft_quests(owner), dm_ft_quests(all));
      const dm_ft_row *row = dm_ft_row_of(11);
      CHECK(row && row->rule == DM_FT_R_QUESTS && row->group == DM_FT_G_CAMPAIGN && row->rev == 6 && row->arg == DM_FT_QUESTS && DM_FT_REV == 6, "row 11, revision 6");
      #define MET(words, d) (memset(&X, 0, sizeof X), X.known = DM_FT_K_ALL, memcpy(X.quests[d], words, sizeof X.quests[d]), dm_ft_met(row, &X, F.loaded))
      CHECK(MET(all, 0) && MET(all, 1) && MET(all, 2) && !MET(owner, 0), "all done in any one difficulty; the real set is not");
      { uint16_t w[DM_FT_WORDS]; memcpy(w, all, sizeof w);
        static const uint16_t siege[] = { 0x0004, 0x000c, 0x200e, 0x2022, 0x8022 }, rescue[] = { 0x0004, 0x000c };
        for (size_t i = 0; i < sizeof siege / sizeof *siege; i++) { w[35] = siege[i]; CHECK(!MET(w, 0), "Siege on Harrogath %#x: open", siege[i]); }
        w[35] = all[35];
        for (size_t i = 0; i < sizeof rescue / sizeof *rescue; i++) { w[36] = rescue[i]; CHECK(!MET(w, 0), "Rescue on Mount Arreat %#x: open", rescue[i]); } }
      for (size_t k = 0; k < sizeof seen / sizeof *seen; k++) {
          uint16_t w[DM_FT_WORDS]; memcpy(w, all, sizeof w);
          for (int i = 0; i < seen[k].n; i++) { w[seen[k].word] = seen[k].seq[i]; int met = i >= seen[k].done;
              CHECK(MET(w, 0) == met && dm_ft_quests(w) == DM_FT_QUESTS - !met, "word %d step %#x: %s", seen[k].word, seen[k].seq[i], met ? "done" : "open"); }
          w[seen[k].word] = seen[k].seq[seen[k].n - 1] & ~0x2000; CHECK(MET(w, 0), "word %d as a new game reads it", seen[k].word);
      }
      { uint16_t w[DM_FT_WORDS]; memcpy(w, all, sizeof w);
        for (int i = 0; i < DM_FT_WORDS; i++) { int q = 0; for (int j = 0; j < DM_FT_QUESTS; j++) q |= quest[j] == i; if (!q) w[i] = 0; }
        CHECK(MET(w, 1), "the other words empty: still all");
        for (int i = 0; i < DM_FT_WORDS; i++) { int q = 0; for (int j = 0; j < DM_FT_QUESTS; j++) q |= quest[j] == i; w[i] = q ? 0xfffe : 0xffff; }
        CHECK(!MET(w, 1) && dm_ft_quests(w) == 0, "every bit but bit 0 of the quests, every bit of the others: none"); }
      { memset(&X, 0, sizeof X); X.known = DM_FT_K_ALL; memcpy(X.quests[0], all, sizeof all); X.quests[0][40] = 0; X.quests[1][40] = 0x1001;
        CHECK(!dm_ft_met(row, &X, F.loaded), "done across two difficulties is not one difficulty's"); }
      #undef MET


      fresh(); memcpy(X.quests[1], all, sizeof all);
      CHECK(step() == 0 && done(11) && F.seq == 0 && F.rev == DM_FT_REV, "no file: silent");
      { dm_ft_rec old[5]; for (int i = 0; i < 5; i++) old[i] = (dm_ft_rec){ (uint16_t)(4 + i), DM_FT_REC_DONE, 30, 77, 0 };
        dm_ft_take(&F, old, 5, 5); facts(40, 1); memcpy(X.quests[0], all, sizeof all);
        uint32_t seq = F.seq; CHECK(step() == 0 && done(11) && F.seq == seq && F.rev == DM_FT_REV, "revision 5: silent"); }
      { dm_ft_rec none[1]; dm_ft_take(&F, none, 0, DM_FT_REV); facts(40, 1); memcpy(X.quests[2], all, sizeof all); X.quests[2][19] = 0x0374;
        step(); CHECK(!done(11) && done(8) && done(10), "the Gidbinn's reward pending: open (the acts and Hell entered)");
        X.quests[2][19] = 0x23f5; CHECK(step() == 1 && done(11) && F.note[F.seq % DM_FT_NOTICES].id == 11 && F.note[F.seq % DM_FT_NOTICES].arg == DM_FT_QUESTS, "the last quest: a notice");
        X.quests[2][19] = 0; CHECK(step() == 0 && done(11), "the entry stays"); }
      char out[8192]; CHECK(dm_ft_text(&F, out, sizeof out) > 0 && strstr(out, "\nrow\t11\t2\t26\t27\t1\t"), "the page line of row 11"); }

    printf("t_feats: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
