

#include "../dm_fightlog_hist.c"
#include <stdio.h>
#include <unistd.h>

uint64_t now_ns(void) { static uint64_t t; return t += 60ull * 1000000000ull; }

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)


static dm_fl_fight fight(uint32_t mine, uint32_t ticks, uint32_t merc) {
    static dm_fl_fight f; memset(&f, 0, sizeof f);
    f.h.duration = ticks; f.h.skill[0] = 85; f.s.flags = DM_FL_SPLIT_KNOWN; f.s.mine = mine; f.s.skill[0] = mine; f.s.merc = merc;
    return f;
}
static dm_fl_parts parts(uint16_t level, uint32_t gear) { dm_fl_parts p = { gear, 1, 1, level, 0, 0 }; return p; }
static dm_dh_window W; static dm_dh_seg S[DM_DH_SEGS];

int main(void) {
    dm_dh *h = dm_dh_core(); dm_dh_reset(h); dm_dh_owner(h, "Hero", NULL, 0);

    { dm_dh_fight f[10]; memset(f, 0, sizeof f); double lo, hi;
      for (int i = 0; i < 10; i++) { f[i].ticks = 100; f[i].mine = f[i].team = (uint32_t)(i + 1) * 4; }
      CHECK(dm_dh_range(f, 10, 0, -1, &lo, &hi) && lo == 1 && hi == 9, "P10 %g, P90 %g", lo, hi);
      CHECK(dm_dh_value(f, 10, 0) == 5.5, "value %g", dm_dh_value(f, 10, 0));
      f[9].ticks = 1000; f[9].mine = 400;
      CHECK(dm_dh_range(f, 10, 0, -1, &lo, &hi) && lo == 2 && hi == 10, "weighted by length: %g .. %g", lo, hi);
      CHECK(!dm_dh_range(f, 4, 0, -1, &lo, &hi) && lo == 0 && hi == 0, "no range under five fights"); }

    { dm_fl_fight f = fight(100, 10, 0); dm_dh_add(h, 7, &(dm_fl_parts){ 0 }, &f);
      f = fight(100, 100, 0); f.s.flags = 0; f.s.mine = 0; dm_dh_add(h, 7, &(dm_fl_parts){ 0 }, &f);
      CHECK(dm_dh_read(h, 7, &W, S) == 0 && W.n == 0, "nothing from a fight under a second or with an unknown split"); }

    dm_fl_parts pa = parts(24, 1), pb = parts(25, 1), pc = parts(25, 2);
    for (int i = 0; i < 10; i++) { dm_fl_fight f = fight(250, 250, 50); dm_dh_add(h, 0xa, &pa, &f); }
    for (int i = 0; i < 3; i++) { dm_fl_fight f = fight(500, 250, 0); dm_dh_add(h, 0xb, &pb, &f); }
    for (int i = 0; i < 60; i++) { dm_fl_fight f = fight(400, 250, 0); dm_dh_add(h, 0xc, &pc, &f); }
    int n = dm_dh_read(h, 0xc, &W, S);
    CHECK(n == 2 && S[0].key == 0xa && S[1].key == 0xc && S[1].ticks == 63 * 250, "the short state folds its time into the next segment (%d)", n);
    CHECK(W.key == 0xc && W.n == DM_DH_WINDOW && S[1].value == 4000 && S[1].lo == 4000 && S[1].hi == 4000, "the state's window holds its last 50 fights");
    dm_dh_read(h, 0xa, &W, S); CHECK(W.key == 0xa && W.n == 10, "an earlier state keeps its own window (%u)", W.n);
    dm_dh_read(h, 0xd, &W, S); CHECK(W.n == 0, "a state without fights has no window");
    dm_dh_read(h, 0, &W, S); CHECK(W.n == 0, "no state, no window");

    { static uint8_t img[DM_DH_IMAGE_MAX]; char who[64]; uint32_t gen; size_t m = dm_dh_image(h, NULL, NULL, who, img, sizeof img, &gen);
      CHECK(m > DM_DH_HEAD && !strcmp(who, "Hero"), "an image of %zu bytes", m);
      dm_dh_reset(h); CHECK(!dm_dh_owner(h, "Hero", img, m) && dm_dh_read(h, 0, &W, S) == 2, "loaded again");
      for (int i = 0; i < 2; i++) { dm_fl_fight f = fight(500, 250, 0); dm_dh_add(h, 0xa, &pa, &f); }
      n = dm_dh_read(h, 0xa, &W, S);
      CHECK(W.key == 0xa && W.n == 12 && n == 3 && S[2].key == 0xa && S[2].value == (uint32_t)(3500.0 / 3000 * 25 * 100 + 0.5), "A's window goes on after fights elsewhere and a restart (%u)", S[2].value);
      CHECK(W.f[0].team == 300 && W.f[0].nsrc == 2 && W.f[0].src[0].id == 84 && W.f[0].src[1].id == DM_DH_SRC_MERC, "a fight's sources: its skill and the mercenary");

      img[DM_DH_HEAD + 40] ^= 0x5a; CHECK(dm_dh_owner(h, "Hero", img, m) == DM_DH_BAD_CRC && dm_dh_read(h, 0, &W, S) == 0, "a damaged image is rejected");
      img[DM_DH_HEAD + 40] ^= 0x5a; dh_put32(img + 4, 9); CHECK(dm_dh_owner(h, "Hero", img, m) == DM_DH_BAD_VERSION, "an unknown version"); }

    { dm_dh_reset(h); dm_dh_owner(h, "Hero", NULL, 0);
      for (uint32_t k = 1; k <= DM_DH_SEGS + 20; k++) for (int i = 0; i < DM_DH_MIN; i++) { dm_fl_fight f = fight(100, 100, 0); dm_fl_parts p = parts((uint16_t)k, 1); dm_dh_add(h, k, &p, &f); }
      n = dm_dh_read(h, DM_DH_SEGS + 20, &W, S);
      CHECK(n == DM_DH_SEGS && S[0].key == 21 && S[DM_DH_SEGS - 1].key == DM_DH_SEGS + 20, "at most %d segments, the oldest dropped (%d, %u)", DM_DH_SEGS, n, S[0].key);
      CHECK(h->b.nwin == DM_DH_WINDOWS && h->b.win[DM_DH_WINDOWS - 1].key == DM_DH_SEGS + 20 - DM_DH_WINDOWS + 1, "eight windows, the newest states"); }

    { char root[256], dir[300], file[400]; const char *tmp = getenv("TMPDIR"); struct stat st;
      snprintf(root, sizeof root, "%s/t_dh.XXXXXX", tmp && tmp[0] ? tmp : "/tmp"); CHECK(mkdtemp(root) != NULL, "temporary folder");
      snprintf(dir, sizeof dir, "%s/Hero", root); snprintf(file, sizeof file, "%s/Hero.sw_dps", dir); mkdir(dir, 0700);
      CHECK(dm_dh_store_commit(h, root, 1, NULL, NULL) && !stat(file, &st), "written into the character folder");
      CHECK(!dm_dh_store_commit(h, root, 2, NULL, NULL), "unchanged: not written again");

      { dm_fl_fight f = fight(100, 100, 0); dm_fl_parts p = parts(1, 1); f.s.state = 9;
        CHECK(dm_dh_store_commit(h, root, 3, &f, &p) && dm_dh_store_commit(h, root, 4, &f, &p), "a waiting fight is written on every save");
        CHECK(dm_dh_store_commit(h, root, 5, NULL, NULL) && !dm_dh_store_commit(h, root, 6, NULL, NULL), "once more without it, then not again"); }
      dm_dh_reset(h); CHECK(dm_dh_store_follow(h, root, "Hero") && dm_dh_read(h, 0, &W, S) == DM_DH_SEGS, "loaded on the next entry");
      unlink(file); rmdir(dir); rmdir(root); }

    { dm_dh_reset(h); dm_dh_owner(h, "Hero", NULL, 0); dm_fl_parts p = parts(3, 3); static uint8_t img[DM_DH_IMAGE_MAX]; char who[64]; uint32_t gen;
      dm_fl_fight f = fight(100, 100, 0); f.s.state = 0x77; dm_dh_add(h, 0x77, &p, &f);
      dm_fl_fight w = fight(200, 100, 0); w.s.state = 0x77; dm_fl_fight pend = w;
      dm_dh_add(h, 0x77, &p, &w);
      size_t m = dm_dh_image(h, &pend, &p, who, img, sizeof img, &gen);
      dm_dh_reset(h); CHECK(m && !dm_dh_owner(h, "Hero", img, m) && dm_dh_read(h, 0x77, &W, S) == 1 && W.n == 2, "the fight once, not twice (%u)", W.n);
      dm_fl_fight other = fight(300, 100, 0); other.s.state = 0x77; m = dm_dh_image(h, &other, &p, who, img, sizeof img, &gen);
      dm_dh_reset(h); CHECK(m && !dm_dh_owner(h, "Hero", img, m) && dm_dh_read(h, 0x77, &W, S) == 1 && W.n == 3, "another waiting fight is in the image (%u)", W.n); }
    printf("t_fightlog_hist: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
