










#include <dirent.h>
#include <errno.h>
static int readdir_fail;
static struct dirent *test_readdir(DIR *d);
#define readdir(d) test_readdir(d)
#include "../dm_feats_core.c"
#include "../dm_feats_store.c"
#undef readdir
static struct dirent *test_readdir(DIR *d) { if (readdir_fail) { errno = EIO; return NULL; } return readdir(d); }
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static char root[1024], dir[1100], file[1200], aside[1220], tmp[1220];
static dm_ft_store *S;
static dm_ft_note N[DM_FT_NOTICES]; static int nn;
static uint8_t img[DM_FT_IMAGE_MAX + 64];
static dm_ft_facts X;
static uint64_t refused;

static size_t slurp(const char *p, uint8_t *buf, size_t cap) { FILE *f = fopen(p, "rb"); if (!f) return 0; size_t n = fread(buf, 1, cap, f); fclose(f); return n; }
static void spill(const char *p, const uint8_t *buf, size_t n) { FILE *f = fopen(p, "wb"); fwrite(buf, 1, n, f); fclose(f); }
static int exists(const char *p) { struct stat st; return !stat(p, &st); }
static int kills(uint64_t n) { return dm_ft_store_count(S, DM_FT_KILLS, n, 100, N, &nn); }
static int facts(void) { return dm_ft_store_facts(S, &X, 100, N, &nn); }
static void fix_crc(uint8_t *b, size_t n) { put32(b + 16, ft_crc32(b + DM_FT_IMAGE_HEAD, n - DM_FT_IMAGE_HEAD)); }
static void paths(const char *who) {
    snprintf(dir, sizeof dir, "%s/%s", root, who); snprintf(file, sizeof file, "%s/%s.sw_feats", dir, who);
    snprintf(aside, sizeof aside, "%s.bad", file); snprintf(tmp, sizeof tmp, "%s.tmp", file);
}


static size_t image_of(uint8_t *b, const char *who, const dm_ft_rec *rec, int n, uint32_t version) {
    memset(b, 0, DM_FT_IMAGE_HEAD + DM_FT_OWNER);
    put32(b, DM_FT_MAGIC); put32(b + 4, version); put32(b + 8, DM_FT_REV); put32(b + 12, (uint32_t)n);
    memcpy(b + DM_FT_IMAGE_HEAD, who, strlen(who)); memcpy(b + DM_FT_IMAGE_HEAD + DM_FT_OWNER, rec, (size_t)n * sizeof *rec);
    size_t m = DM_FT_IMAGE_HEAD + DM_FT_OWNER + (size_t)n * sizeof *rec; fix_crc(b, m);
    return m;
}

static void hero(const char *base, const char *who, int save, const uint8_t *b, long n) {
    char p[1300]; snprintf(p, sizeof p, "%s/%s", base, who); mkdir(p, 0700);
    if (save) { snprintf(p, sizeof p, "%s/%s/%s.d2s", base, who, who); spill(p, (const uint8_t *)"d2s", 3); }
    if (n >= 0) { snprintf(p, sizeof p, "%s/%s/%s.sw_feats", base, who, who); spill(p, b, (size_t)n); }
}
static int line(const char *text, const char *l) { char want[256]; snprintf(want, sizeof want, "\n%s\n", l); return strstr(text, want) || !strncmp(text, l, strlen(l)); }
static int lines_of(const char *text, const char *kind) { int n = 0; size_t k = strlen(kind); for (const char *p = text; p && *p; p = strchr(p, '\n'), p = p ? p + 1 : p) n += !strncmp(p, kind, k) && p[k] == '\t'; return n; }

static void all_heroes(void) {
    char base[1100], out[8192], p[1300]; uint8_t b[DM_FT_IMAGE_MAX], c[DM_FT_IMAGE_MAX + 8]; size_t m;
    snprintf(base, sizeof base, "%s/all", root); mkdir(base, 0700);


    dm_ft_rec old[] = { { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 3200 } };
    m = image_of(b, "Cur", old, 1, DM_FT_VERSION); hero(base, "Cur", 1, b, (long)m);
    CHECK(!dm_ft_store_load(S, base, "Cur") && dm_ft_store_counter(S, DM_FT_KILLS) == 3200, "the character in the game");
    memset(&X, 0, sizeof X); X.level = 30; X.cls = 2; X.quests[0][6] = 1; facts(); kills(500);

    dm_ft_rec ann[] = { { 1, DM_FT_REC_DONE, 20, 50, 0 }, { 4, DM_FT_REC_DONE, 20, 60, 0 }, { 13, DM_FT_REC_DONE, 90, 70, 0 }, { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 3000 } };
    m = image_of(b, "Ann", ann, 4, DM_FT_VERSION); hero(base, "Ann", 1, b, (long)m);

    m = image_of(b, "Bob", ann, 4, DM_FT_VERSION); b[DM_FT_IMAGE_HEAD + 1] ^= 1; hero(base, "Bob", 1, b, (long)m);
    size_t bob = m; uint8_t bobimg[DM_FT_IMAGE_MAX]; memcpy(bobimg, b, m);
    hero(base, "Cid", 1, b, 0);
    m = image_of(b, "Dan", ann, 4, DM_FT_VERSION + 1); hero(base, "Dan", 1, b, (long)m);
    m = image_of(b, "Ann", ann, 4, DM_FT_VERSION); hero(base, "Eve", 1, b, (long)m);
    m = image_of(b, "Fay", ann, 4, DM_FT_VERSION); hero(base, "Fay", 1, b, (long)m);
    snprintf(p, sizeof p, "%s/Fay/Fay.sw_feats", base); chmod(p, 0);

    hero(base, "Gus", 1, NULL, -1);
    m = image_of(b, "Hal", ann, 4, DM_FT_VERSION); hero(base, "Hal", 0, b, (long)m);
    hero(base, "Settings", 0, NULL, -1);
    snprintf(p, sizeof p, "%s/Ivo.d2s", base); spill(p, (const uint8_t *)"x", 1);

    int n = dm_ft_store_all_text(S, base, out, sizeof out);
    CHECK(n > 0 && !strncmp(out, "all\t2\n", 6), "two characters summed: %.40s", out);
    CHECK(line(out, "skipped\tBob\tunreadable\nskipped\tCid\tunreadable\nskipped\tDan\tnewer\nskipped\tEve\tunreadable\nskipped\tFay\tunreadable")
          && lines_of(out, "skipped") == 5 && lines_of(out, "incomplete") == 0, "the five left out, by name, a newer app's file apart");
    CHECK(line(out, "waiting\tGus") && lines_of(out, "waiting") == 1, "the character without a file waits");
    CHECK(!strstr(out, "Hal") && !strstr(out, "Settings") && !strstr(out, "Ivo") && !strstr(out, "Cur"), "no character, no line; the one in the game is summed");
    CHECK(line(out, "allcounter\t0\t3700"), "the most kills of one character: memory's, not the in-game character's older file");
    CHECK(line(out, "allrow\t1\t1\t1\t500\t1\t50") && line(out, "allrow\t4\t2\t2\t1\t1\t60") && line(out, "allrow\t13\t3\t4\t0\t1\t70")
          && line(out, "allrow\t20\t3\t6\t8\t0\t0"), "rows entered by any of them, on the earliest day");
    CHECK(lines_of(out, "allrow") == dm_ft_nrows, "every table row");
    snprintf(p, sizeof p, "%s/Bob/Bob.sw_feats", base);
    CHECK(slurp(p, c, sizeof c) == bob && !memcmp(c, bobimg, bob), "the damaged file stays as it is");
    snprintf(p, sizeof p, "%s/Bob/Bob.sw_feats.bad", base); CHECK(!exists(p), "and is not set aside");
    snprintf(p, sizeof p, "%s/Cid/Cid.sw_feats", base); CHECK(exists(p), "the empty file stays");
    CHECK(dm_ft_store_all_text(S, base, out, (size_t)n) == -2 && !out[0], "one byte short: nothing");
    CHECK(dm_ft_store_all_text(S, base, NULL, 9) == -1, "no buffer");

    n = dm_ft_store_all_text(S, NULL, out, sizeof out);
    CHECK(n > 0 && !strncmp(out, "all\t1\nincomplete\tunavailable\n", 29), "no save folder: unavailable (%.40s)", out);
    snprintf(p, sizeof p, "%s/none", base);
    n = dm_ft_store_all_text(S, p, out, sizeof out);
    CHECK(n > 0 && line(out, "incomplete\tunavailable"), "a save folder that does not open: unavailable");
    chmod(base, 0300);
    n = dm_ft_store_all_text(S, base, out, sizeof out);
    chmod(base, 0700);
    CHECK(n > 0 && line(out, "incomplete\tunavailable"), "a save folder without the right to list it: unavailable");
    readdir_fail = 1;
    n = dm_ft_store_all_text(S, base, out, sizeof out);
    readdir_fail = 0;
    CHECK(n > 0 && line(out, "incomplete\tunavailable"), "a read error while listing: unavailable");


    CHECK(dm_ft_store_load(S, base, "Dan") == DM_FT_NEWER, "Dan's newer file");
    n = dm_ft_store_all_text(S, base, out, sizeof out);
    CHECK(n > 0 && !strncmp(out, "all\t2\n", 6) && lines_of(out, "skipped") == 5 && line(out, "skipped\tDan\tnewer") && line(out, "allcounter\t0\t3200"),
          "resting: named once, Cur's file now counts: %.120s", out);

    snprintf(p, sizeof p, "%s/Fay/Fay.sw_feats", base); chmod(p, 0600);
    const char *gone[] = { "Cur", "Ann", "Bob", "Cid", "Dan", "Eve", "Fay", "Gus", "Hal" };
    for (size_t i = 0; i < sizeof gone / sizeof *gone; i++) {
        snprintf(p, sizeof p, "%s/%s/%s.sw_feats", base, gone[i], gone[i]); unlink(p);
        snprintf(p, sizeof p, "%s/%s/%s.d2s", base, gone[i], gone[i]); unlink(p);
        snprintf(p, sizeof p, "%s/%s", base, gone[i]); rmdir(p);
    }
    snprintf(p, sizeof p, "%s/Settings", base); rmdir(p); snprintf(p, sizeof p, "%s/Ivo.d2s", base); unlink(p);
    rmdir(base);


    snprintf(base, sizeof base, "%s/many", root); mkdir(base, 0700);
    for (int i = 0; i <= DM_FT_SUM_HEROES; i++) { char who[16]; snprintf(who, sizeof who, "H%03d", i); hero(base, who, 1, NULL, -1); }
    { static char big[65536]; n = dm_ft_store_all_text(S, base, big, sizeof big);
      CHECK(n > 0 && line(big, "incomplete\tlimit") && lines_of(big, "waiting") == DM_FT_SUM_HEROES, "more than the bound: incomplete, %d summed", lines_of(big, "waiting")); }
    snprintf(p, sizeof p, "%s/H000/H000.d2s", base); unlink(p); snprintf(p, sizeof p, "%s/H000", base); rmdir(p);
    { static char big[65536]; n = dm_ft_store_all_text(S, base, big, sizeof big);
      CHECK(n > 0 && !strstr(big, "incomplete") && lines_of(big, "waiting") == DM_FT_SUM_HEROES, "exactly the bound: complete"); }
    for (int i = 1; i <= DM_FT_SUM_HEROES; i++) {
        snprintf(p, sizeof p, "%s/H%03d/H%03d.d2s", base, i, i); unlink(p); snprintf(p, sizeof p, "%s/H%03d", base, i); rmdir(p);
    }
    rmdir(base);
}


int has(int id) {
    dm_ft_rec r[DM_FT_FILE_RECS]; int k; uint32_t rev;
    if (dm_ft_parse(img, DM_FT_IMAGE_HEAD + DM_FT_OWNER + get32(img + 12) * sizeof(dm_ft_rec), "Hero", r, &k, &rev)) return 0;
    for (int i = 0; i < k; i++) if (r[i].kind == DM_FT_REC_DONE && r[i].id == id) return 1;
    return 0;
}
int main(void) {
    S = dm_ft_store_core();
    const char *t = getenv("TMPDIR"); snprintf(root, sizeof root, "%s/t_feats_store.XXXXXX", t && *t ? t : "/tmp");
    if (!mkdtemp(root)) { perror("mkdtemp"); return 1; }
    memset(&X, 0, sizeof X); X.level = 20; X.cls = 1; X.known = DM_FT_K_ALL;


    paths("Hero");
    CHECK(!dm_ft_store_load(S, root, "Hero") && dm_ft_store_mode(S) == DM_FT_MODE_OWN, "no file: own, empty");
    facts(); CHECK(kills(500) == 1 && nn == 1 && N[0].id == 1, "the 500th kill: a notice copied out");
    CHECK(!dm_ft_store_commit(S, root, 1, refused) && !exists(file), "no character folder: nothing written");
    mkdir(dir, 0700);
    CHECK(dm_ft_store_commit(S, root, 1, refused) && exists(file), "written into the character folder");
    CHECK(!dm_ft_store_commit(S, root, 2, refused), "unchanged: not written again");
    size_t n = slurp(file, img, sizeof img);
    CHECK(n == DM_FT_IMAGE_HEAD + DM_FT_OWNER + 2 * sizeof(dm_ft_rec) && get32(img) == DM_FT_MAGIC && get32(img + 4) == DM_FT_VERSION && get32(img + 8) == DM_FT_REV
          && !strcmp((const char *)img + DM_FT_IMAGE_HEAD, "Hero"), "the image: head, owner, an entry and the counter (%zu bytes)", n);


    kills(1);
    CHECK(dm_ft_store_commit(S, root, 2, refused) && dm_ft_store_commit(S, root, 2, refused), "the commit after the write was refused: written again");
    CHECK(!dm_ft_store_commit(S, root, 3, refused), "after a successful commit nothing is pending");

    kills(1); mkdir(tmp, 0700);
    CHECK(!dm_ft_store_commit(S, root, 4, refused) && dm_ft_store_counter(S, DM_FT_KILLS) == 502, "a failed write writes nothing");
    rmdir(tmp);
    CHECK(dm_ft_store_commit(S, root, 4, refused), "the next save writes it");




    { int has(int id); uint64_t c = 4;
      CHECK(dm_ft_store_stage_count(S, DM_FT_LOOKS, 5, 100, c, refused) == 1 && dm_ft_store_counter(S, DM_FT_LOOKS) == 5, "five items given a look, staged with the save");
      CHECK(dm_ft_store_commit(S, root, c, refused) && slurp(file, img, sizeof img) && has(59), "the save writes the row and the count");
      refused++;
      dm_ft_store_commit(S, root, c, refused);
      CHECK(slurp(file, img, sizeof img) && !has(59) && dm_ft_store_counter(S, DM_FT_LOOKS) == 0, "the next save writes the records without the refused looks");
      CHECK(!dm_ft_store_staged(S, c, refused, N, &nn), "no notice of it");
      CHECK(dm_ft_store_stage_count(S, DM_FT_LOOKS, 4, 100, c, refused) == 0 && dm_ft_store_stage_count(S, DM_FT_LOOKS, 1, 100, c, refused) == 1
            && dm_ft_store_stage_count(S, DM_FT_DYES, 5, 100, c, refused) == 1 && dm_ft_store_commit(S, root, c, refused), "staged again with the next save");
      CHECK(!dm_ft_store_staged(S, c, refused, N, &nn) && slurp(file, img, sizeof img) && has(59), "while the save is in flight: written, no notice");
      c++;
      CHECK(dm_ft_store_staged(S, c, refused, N, &nn) == 2 && nn == 2 && N[0].id == 59 && N[1].id == 60, "committed: both noticed");
      CHECK(!dm_ft_store_staged(S, c, refused, N, &nn), "once");
      dm_ft_store_commit(S, root, c, refused); CHECK(slurp(file, img, sizeof img) && has(59) && has(60), "kept by the saves after");
      CHECK(dm_ft_store_stage(S, DM_FT_E_CALM, 100, c, refused) == 1 && !dm_ft_store_load(S, root, "Hero") && !dm_ft_store_staged(S, c + 1, refused, N, &nn),
            "a character loaded again drops what was staged for the old records"); }


    dm_ft_store_owner(S, "", NULL, 0);
    CHECK(!dm_ft_store_load(S, root, "Hero") && dm_ft_store_counter(S, DM_FT_KILLS) == 502, "loaded on the next entry");
    kills(3);
    CHECK(!dm_ft_store_follow(S, root, "Hero") && dm_ft_store_counter(S, DM_FT_KILLS) == 505, "a new game of the same character keeps the counter");
    dm_ft_store_commit(S, root, 5, refused);
    CHECK(dm_ft_store_follow(S, root, "Other") && dm_ft_store_counter(S, DM_FT_KILLS) == 0, "another character starts with its own");
    CHECK(dm_ft_store_follow(S, root, "Hero") && dm_ft_store_counter(S, DM_FT_KILLS) == 505, "the first character's file comes back");


    n = slurp(file, img, sizeof img);
    { dm_ft_rec u[2] = { { 300, DM_FT_REC_DONE, 40, 12345, 0 }, { 9, DM_FT_REC_COUNT, 0, 0, 777 } };
      memcpy(img + n, u, sizeof u); put32(img + 12, get32(img + 12) + 2); n += sizeof u; fix_crc(img, n); spill(file, img, n); }
    dm_ft_store_owner(S, "", NULL, 0);
    CHECK(!dm_ft_store_load(S, root, "Hero"), "a file with unknown records is valid");
    kills(1); dm_ft_store_commit(S, root, 6, refused);
    { uint8_t back[DM_FT_IMAGE_MAX]; size_t m = slurp(file, back, sizeof back); dm_ft_rec r[DM_FT_FILE_RECS]; int k; uint32_t rev;
      int found = 0; CHECK(!dm_ft_parse(back, m, "Hero", r, &k, &rev), "written back");
      for (int i = 0; i < k; i++) found += (r[i].id == 300 && r[i].kind == DM_FT_REC_DONE && r[i].when == 12345) + (r[i].id == 9 && r[i].value == 777);
      CHECK(found == 2, "unknown records kept (%d)", found); }


    n = slurp(file, img, sizeof img);
    { uint8_t b[DM_FT_IMAGE_MAX + 64]; struct { int at; uint8_t v; int why; } d[] = {
          { 0, 0x00, DM_FT_BAD_MAGIC }, { 4, 0x00, DM_FT_BAD_VERSION }, { DM_FT_IMAGE_HEAD + 1, 'X', DM_FT_BAD_CRC }, { 30 + DM_FT_IMAGE_HEAD, 'q', DM_FT_BAD_CRC } };
      for (size_t i = 0; i < sizeof d / sizeof *d; i++) {
          memcpy(b, img, n); b[d[i].at] = d[i].v; spill(file, b, n);
          CHECK(dm_ft_store_load(S, root, "Hero") == d[i].why && exists(aside) && !exists(file) && dm_ft_store_counter(S, DM_FT_KILLS) == 0, "damage %zu: %d set aside", i, d[i].why);
          unlink(aside);
      }
      memcpy(b, img, n); put32(b + 12, DM_FT_FILE_RECS + 1); fix_crc(b, n); spill(file, b, n);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_COUNT, "count beyond the bound"); unlink(aside);
      memcpy(b, img, n); put32(b + 12, get32(b + 12) - 1); fix_crc(b, n); spill(file, b, n);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_SIZE, "count and size disagree"); unlink(aside);
      memcpy(b, img, n); b[DM_FT_IMAGE_HEAD + DM_FT_OWNER + 2] = 7; fix_crc(b, n); spill(file, b, n);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_RECORD, "unknown record kind"); unlink(aside);
      memcpy(b, img, n); memcpy(b + DM_FT_IMAGE_HEAD + DM_FT_OWNER + 16, b + DM_FT_IMAGE_HEAD + DM_FT_OWNER, 16); fix_crc(b, n); spill(file, b, n);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_RECORD, "a record twice"); unlink(aside);
      memcpy(b, img, n); memset(b + DM_FT_IMAGE_HEAD, 0, DM_FT_OWNER); fix_crc(b, n); spill(file, b, n);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_OWNER, "no owner"); unlink(aside);
      spill(file, img, 5);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_SIZE, "too short for a version"); unlink(aside);
      spill(file, img, n); }


    { uint8_t b[DM_FT_IMAGE_MAX + 64], c[DM_FT_IMAGE_MAX + 64]; memcpy(b, img, n);
      memset(b + DM_FT_IMAGE_HEAD, 0, DM_FT_OWNER); memcpy(b + DM_FT_IMAGE_HEAD, "Herb", 4); fix_crc(b, n); spill(file, b, n);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_FOREIGN && dm_ft_store_mode(S) == DM_FT_MODE_FOREIGN && exists(file) && !exists(aside)
            && dm_ft_store_counter(S, DM_FT_KILLS) == 0, "another character's file: not used, left as it is");
      X.level = 50; for (int a = 0; a < 3; a++) X.quests[0][(int[]){ 6, 14, 22 }[a]] = 1;
      CHECK(facts() == 0 && nn == 0, "this character's met rows enter silently");
      kills(400);
      CHECK(!dm_ft_store_commit(S, root, 7, refused) && slurp(file, c, sizeof c) == n && !memcmp(b, c, n), "silent entries and counts do not replace it");
      kills(100);
      CHECK(nn == 1 && dm_ft_store_commit(S, root, 7, refused) && dm_ft_store_mode(S) == DM_FT_MODE_OWN, "the first notice: written");
      size_t m = slurp(file, c, sizeof c); dm_ft_rec r[DM_FT_FILE_RECS]; int k; uint32_t rev;
      CHECK(!dm_ft_parse(c, m, "Hero", r, &k, &rev) && k == 5, "now this character's: three acts, the kill row and the counter (%d)", k);
      memcpy(img, c, m); n = m; }


    { uint8_t c[DM_FT_IMAGE_MAX + 64]; uint64_t had; dm_ft_store_load(S, root, "Hero"); had = dm_ft_store_counter(S, DM_FT_KILLS);
      chmod(file, 0);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_UNREAD && dm_ft_store_mode(S) == DM_FT_MODE_UNREAD && !exists(aside), "an open error: unread, not set aside");
      X.level = 99; X.quests[2][40] = 1;
      CHECK(facts() == 0 && kills(10000) == 0 && nn == 0 && dm_ft_store_counter(S, DM_FT_KILLS) == 0, "the feats rest: no entries, no counts");
      dm_ft_store_seed(S, DM_FT_KILLS, 7);
      CHECK(!dm_ft_store_commit(S, root, 9, refused), "nothing written while the file is unread");
      chmod(file, 0600);
      CHECK(slurp(file, c, sizeof c) == n && !memcmp(c, img, n), "the file stays as it was");
      CHECK(dm_ft_store_follow(S, root, "Hero") && dm_ft_store_mode(S) == DM_FT_MODE_OWN && dm_ft_store_counter(S, DM_FT_KILLS) == had,
            "the next entry of the same character reads it again");
      CHECK(!dm_ft_store_follow(S, root, "Hero"), "read now: no more retries");

      unlink(file); mkdir(file, 0700);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_UNREAD && exists(file) && !exists(aside), "a read error: unread, left as it is");
      CHECK(!dm_ft_store_commit(S, root, 9, refused), "nothing written");
      rmdir(file); spill(file, img, n); X.level = 50; X.quests[2][40] = 0; }


    { uint8_t c[1]; spill(file, c, 0);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_SIZE && exists(aside) && !exists(file) && dm_ft_store_mode(S) == DM_FT_MODE_OWN, "empty: set aside");
      unlink(aside); spill(file, img, n); }


    { uint8_t b[64]; put32(b, DM_FT_MAGIC); put32(b + 4, DM_FT_VERSION + 1); memset(b + 8, 0xee, sizeof b - 8); spill(file, b, 13);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_NEWER && dm_ft_store_mode(S) == DM_FT_MODE_NEWER && exists(file) && !exists(aside), "newer version, odd size: rests");
      X.level = 99;
      CHECK(facts() == 0 && kills(10000) == 0 && nn == 0 && dm_ft_store_counter(S, DM_FT_KILLS) == 0, "no entries, no counts, no notices");
      dm_ft_store_seed(S, DM_FT_KILLS, 5);
      CHECK(!dm_ft_store_commit(S, root, 8, refused) && slurp(file, img, sizeof img) == 13, "never written");
      { char out[2048]; char head[32]; snprintf(head, sizeof head, "feats\t%d\tHero\ncounter\t0\t0\n", DM_FT_MODE_NEWER);
        CHECK(dm_ft_store_text(S, out, sizeof out) > 0 && !strncmp(out, head, strlen(head)) && strstr(out, "\nrow\t1\t1\t1\t500\t0\t0\n"), "the page's text: resting, nothing entered");
        CHECK(dm_ft_store_text(S, out, 10) == -2 && !out[0], "too small: nothing"); }
      put32(b + 4, 0); spill(file, b, 13);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_BAD_VERSION && exists(aside), "an older version than any is damaged"); unlink(aside); }


    { dm_ft_rec r[2] = { { 1, DM_FT_REC_DONE, 30, 77, 0 }, { DM_FT_KILLS, DM_FT_REC_COUNT, 0, 0, 900 } };
      size_t m = image_of(img, "Hero", r, 2, DM_FT_VERSION); put32(img + 8, DM_FT_REV + 1); spill(file, img, m);
      uint8_t was[sizeof img], now[sizeof img]; memcpy(was, img, m);
      CHECK(dm_ft_store_load(S, root, "Hero") == DM_FT_NEWER && dm_ft_store_mode(S) == DM_FT_MODE_NEWER && !exists(aside), "newer table revision: rests");
      X.level = 99; X.quests[0][6] = 1;
      CHECK(facts() == 0 && kills(10000) == 0 && nn == 0 && dm_ft_store_counter(S, DM_FT_KILLS) == 0, "no entries, no counts, no notices");
      CHECK(!dm_ft_store_commit(S, root, 9, refused) && slurp(file, now, sizeof now) == m && !memcmp(was, now, m), "the file unchanged by facts, counts and a save");
      put32(img + 8, DM_FT_REV); fix_crc(img, m); spill(file, img, m);
      CHECK(!dm_ft_store_load(S, root, "Hero") && dm_ft_store_mode(S) == DM_FT_MODE_OWN, "the table's own revision: own");
      X.quests[0][6] = 0; unlink(file); }

    unlink(file); rmdir(dir); paths("Other"); rmdir(dir);
    all_heroes();
    rmdir(root);
    printf("t_feats_store: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
