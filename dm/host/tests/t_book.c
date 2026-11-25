






#define _GNU_SOURCE
#include "../dm_book_core.c"
#include "../dm_book.c"
#include "../dm_fightlog.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)


void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *);
static char root[1024];
static uint64_t commits;
const char *dm_save_workdir(void) { return root; }
uint64_t dm_fs_commit_count(void) { return commits; }
int dm_overlay_file(const char *relative, char *out, size_t cap) { (void)relative; (void)out; (void)cap; return 0; }
static int board_ready; static dm_ft_board_row board_rows[DM_FT_BOARD_ROWS];
int dm_kills_board(const char *who, dm_ft_board_row *out, int max) {
    if (!board_ready || strcmp(who, "Ann") || max < DM_FT_BOARD_ROWS) return -1;
    memcpy(out, board_rows, sizeof board_rows); return DM_FT_BOARD_ROWS;
}
dm_fl_state *dm_fl_core(void) { return NULL; }
int dm_fl_kill_kinds(const dm_fl_state *s, uint32_t tick, const dm_fl_kind **kinds, int *lost) { (void)s; (void)tick; *kinds = NULL; *lost = 0; return 0; }
int dm_fl_difficulty(const dm_fl_state *s) { (void)s; return 0; }
int dm_fl_owned_by(dm_fl_state *s, const char *who) { (void)s; (void)who; return 0; }
int dm_fl_count(dm_fl_state *s) { (void)s; return 0; }
int dm_fl_get(dm_fl_state *s, int newest, dm_fl_fight *out) { (void)s; (void)newest; (void)out; return 0; }


static dm_sim_data D;
static void tables(void) {
    static dm_sim_mon mon[600]; static dm_sim_su su[2]; static dm_sim_boss boss[2];
    D.mon = mon; D.nmon = 600; D.su = su; D.nsu = 2; D.boss = boss; D.nboss = 2;
    su[0].idx = 3; su[0].mon = 20; su[1].idx = 26; su[1].mon = 345;
    boss[0].mon = 156; boss[1].mon = 526;
    snprintf(mon[156].id, sizeof mon[156].id, "andariel"); snprintf(mon[156].namekey, sizeof mon[156].namekey, "Andariel");
    snprintf(mon[526].id, sizeof mon[526].id, "nihlathakboss"); snprintf(mon[526].namekey, sizeof mon[526].namekey, "Nihlathak");
}
static int text_has(const dm_bk *b, const char *line) { static char t[DM_BK_KINDS * 48]; return dm_bk_text(b, 0, t, sizeof t) > 0 && strstr(t, line) != NULL; }
static int exists(const char *p) { struct stat st; return !stat(p, &st); }

int main(void) {
    tables();
    uint8_t type; uint16_t idx;
    CHECK(!dm_bk_key("M526", &type, &idx) && type == 'M' && idx == 526 && !dm_bk_key("S0", &type, &idx) && type == 'S' && idx == 0, "keys");
    CHECK(dm_bk_key("", &type, &idx) && dm_bk_key("X1", &type, &idx) && dm_bk_key("M", &type, &idx) && dm_bk_key("M-1", &type, &idx) && dm_bk_key("M70000", &type, &idx) && dm_bk_key("M1x", &type, &idx), "no key");

    dm_bk *b = calloc(1, sizeof *b), *c = calloc(1, sizeof *c);
    CHECK(!dm_bk_add(b, 'M', 10, 0, 2) && !dm_bk_add(b, 'M', 10, 2, 1) && !dm_bk_add(b, 'S', 3, 1, 5) && b->nrec == 2, "two kinds");
    CHECK(dm_bk_count(b, 'M', 10, 0) == 2 && dm_bk_count(b, 'M', 10, 1) == 0 && dm_bk_count(b, 'M', 10, 2) == 1, "per difficulty");
    CHECK(dm_bk_add(b, 'M', 10, 3, 1) && dm_bk_add(b, 'X', 10, 0, 1), "difficulty and type checked");
    uint32_t g = b->gen; dm_bk_at_least(b, 'M', 10, 0, 1); CHECK(b->gen == g && dm_bk_count(b, 'M', 10, 0) == 2, "at least below: unchanged");
    dm_bk_at_least(b, 'M', 10, 0, 7); CHECK(b->gen != g && dm_bk_count(b, 'M', 10, 0) == 7, "at least above: raised");
    CHECK(text_has(b, "kill\tM10\t7\t0\t1\n") && text_has(b, "kill\tS3\t0\t5\t0\n") && text_has(b, "book\t0\t0\n"), "text");
    for (int i = 0; i < DM_BK_KINDS; i++) dm_bk_add(c, 'M', (uint16_t)i, 0, 1);
    CHECK(c->nrec == DM_BK_KINDS && dm_bk_add(c, 'S', 1, 0, 1) == -1 && !dm_bk_add(c, 'M', 5, 0, 1), "full: no new kind, the known ones count on");


    static uint8_t img[DM_BK_IMAGE_MAX];
    b->flags = DM_BK_F_BACKFILLED;
    size_t n = dm_bk_image(b, "Ann", img, sizeof img);
    CHECK(n == DM_BK_HEAD + DM_BK_OWNER + 2 * sizeof(dm_bk_rec), "image size %zu", n);
    CHECK(!dm_bk_parse(img, n, "Ann", c) && c->nrec == 2 && c->flags == DM_BK_F_BACKFILLED && dm_bk_count(c, 'S', 3, 1) == 5, "round trip");
    CHECK(dm_bk_parse(img, n, "Bob", c) == DM_BK_FOREIGN && !c->nrec, "another character's");
    img[30] ^= 1; CHECK(dm_bk_parse(img, n, "Ann", c) == DM_BK_BAD_CRC, "checksum"); img[30] ^= 1;
    CHECK(dm_bk_parse(img, n - 1, "Ann", c) == DM_BK_BAD_CRC || dm_bk_parse(img, n - 1, "Ann", c) == DM_BK_BAD_SIZE, "cut");
    CHECK(dm_bk_parse(img, 4, "Ann", c) == DM_BK_BAD_SIZE, "too short");
    { uint8_t v[DM_BK_IMAGE_MAX]; memcpy(v, img, n); put32(v + 4, DM_BK_VERSION + 1); CHECK(dm_bk_parse(v, 9, "Ann", c) == DM_BK_NEWER, "newer before its size");
      put32(v + 4, 0); CHECK(dm_bk_parse(v, n, "Ann", c) == DM_BK_BAD_VERSION, "older version");
      memcpy(v, img, n); put32(v, 1); CHECK(dm_bk_parse(v, n, "Ann", c) == DM_BK_BAD_MAGIC, "magic");
      memcpy(v, img, n); v[DM_BK_HEAD + DM_BK_OWNER] = 'Q'; put32(v + 16, crc32(v + DM_BK_HEAD, n - DM_BK_HEAD)); CHECK(dm_bk_parse(v, n, "Ann", c) == DM_BK_BAD_RECORD, "record type");
      memcpy(v, img, n); memcpy(v + DM_BK_HEAD + DM_BK_OWNER + sizeof(dm_bk_rec), v + DM_BK_HEAD + DM_BK_OWNER, sizeof(dm_bk_rec)); put32(v + 16, crc32(v + DM_BK_HEAD, n - DM_BK_HEAD));
      CHECK(dm_bk_parse(v, n, "Ann", c) == DM_BK_BAD_RECORD, "a kind twice");
      memcpy(v, img, n); put32(v + 12, 3); CHECK(dm_bk_parse(v, n, "Ann", c) == DM_BK_BAD_SIZE || dm_bk_parse(v, n, "Ann", c) == DM_BK_BAD_CRC, "count"); }


    dm_bk_seen s;
    s = (dm_bk_seen){ -1, 3, "Rakanishu", 0, 1 }; CHECK(!dm_bk_kind_of(&D, &s, &type, &idx) && type == 'S' && idx == 3, "listed superunique");
    s = (dm_bk_seen){ -1, 60, "Nihlathak", 0, 1 }; CHECK(!dm_bk_kind_of(&D, &s, &type, &idx) && type == 'M' && idx == 526, "boss ranking's Nihlathak: his boss class");
    s = (dm_bk_seen){ 526, 60, NULL, 0, 1 }; CHECK(!dm_bk_kind_of(&D, &s, &type, &idx) && type == 'M' && idx == 526, "a counted kill of Nihlathak: the same kind");
    s = (dm_bk_seen){ -1, 61, "Nobody", 0, 1 }; CHECK(dm_bk_kind_of(&D, &s, &type, &idx), "an unknown superunique without a class: no kind");
    s = (dm_bk_seen){ 77, -1, NULL, 0, 1 }; CHECK(!dm_bk_kind_of(&D, &s, &type, &idx) && type == 'M' && idx == 77, "a class");


    dm_bk *f = calloc(1, sizeof *f);
    dm_bk_add(f, 'M', 156, 0, 4);
    dm_bk_seen board[] = { { 156, -1, NULL, 0, 3 }, { 156, -1, NULL, 2, 1 }, { -1, 60, "Nihlathak", 1, 2 }, { -1, 26, "Ismail Vilehand", 0, 1 }, { -1, 99, "Gone", 0, 5 } };
    dm_bk_seen fights[] = { { 156, -1, NULL, 2, 1 }, { 156, -1, NULL, 2, 1 }, { 526, 60, NULL, 1, 1 }, { 5, -1, NULL, 0, 6 }, { 5, -1, NULL, 0, 2 } };
    int skipped = -1;
    CHECK(!dm_bk_backfill(f, &D, board, 5, fights, 5, &skipped) && (f->flags & DM_BK_F_BACKFILLED) && skipped == 1, "back-filled, one record without a kind (%d)", skipped);
    CHECK(dm_bk_count(f, 'M', 156, 0) == 4, "the live count is higher than the ranking's 3 (%u)", dm_bk_count(f, 'M', 156, 0));
    CHECK(dm_bk_count(f, 'M', 156, 2) == 2, "Hell: two fights against the ranking's one (%u)", dm_bk_count(f, 'M', 156, 2));
    CHECK(dm_bk_count(f, 'M', 526, 1) == 2 && dm_bk_count(f, 'S', 60, 1) == 0, "Nihlathak from both sources is one kind at the higher count");
    CHECK(dm_bk_count(f, 'S', 26, 0) == 1 && dm_bk_count(f, 'M', 5, 0) == 8, "a group member; a kind over several fights summed (%u)", dm_bk_count(f, 'M', 5, 0));
    dm_bk *again = calloc(1, sizeof *again); *again = *f;
    dm_bk_backfill(again, &D, board, 5, fights, 5, NULL);
    CHECK(again->nrec == f->nrec && !memcmp(again->rec, f->rec, (size_t)f->nrec * sizeof *f->rec), "a second back-fill changes nothing");


    const char *tmp = getenv("TMPDIR"); snprintf(root, sizeof root, "%s/t_book.XXXXXX", tmp && *tmp ? tmp : "/tmp");
    CHECK(mkdtemp(root) != NULL, "temporary folder");
    char dir[1100], file[1200], aside[1220]; snprintf(dir, sizeof dir, "%s/Ann", root); snprintf(file, sizeof file, "%s/Ann.sw_book", dir); snprintf(aside, sizeof aside, "%s.bad", file);
    load(root, "Ann");
    CHECK(g_due && g_mode == DM_BK_MODE_OWN && !g_bk.nrec, "no file: empty, the back-fill due");
    dm_bk_add(&g_bk, 'M', 10, 1, 3);
    book_pre_commit(root); CHECK(!exists(file), "no character folder yet: nothing written");
    mkdir(dir, 0700);
    commits = 5; book_pre_commit(root); CHECK(exists(file), "written in the hook");
    unlink(file);
    book_pre_commit(root); CHECK(exists(file), "the commit that held it was refused (no new commit): written again");
    unlink(file);
    commits = 6; book_pre_commit(root); CHECK(!exists(file), "committed and unchanged since: not again");
    g_bk.flags |= DM_BK_F_BACKFILLED; g_bk.gen++; book_pre_commit(root);
    load(root, "Ann"); CHECK(!g_due && dm_bk_count(&g_bk, 'M', 10, 1) == 3, "read back: its kills, the back-fill done");

    FILE *w = fopen(file, "r+b"); fseek(w, 40, SEEK_SET); fputc(0x5a, w); fclose(w);
    load(root, "Ann"); CHECK(exists(aside) && !exists(file) && g_due && !g_bk.nrec && g_mode == DM_BK_MODE_OWN, "damaged: set aside, back-fill due");

    { dm_bk_image(b, "Ann", img, sizeof img); put32(img + 4, DM_BK_VERSION + 1); w = fopen(file, "wb"); fwrite(img, 1, 30, w); fclose(w); }
    load(root, "Ann"); CHECK(g_mode == DM_BK_MODE_NEWER && !g_due, "newer: resting");
    g_bk.gen++; commits = 9; book_pre_commit(root);
    struct stat st; stat(file, &st); CHECK(st.st_size == 30, "a newer file is never replaced");
    { char t[256]; CHECK(dm_book_text(t, sizeof t) > 0 && !strncmp(t, "book\t1\t", 7), "the App hears the resting mode: %s", t); }


    load(root, "Cid");
    CHECK(g_due && g_mode == DM_BK_MODE_OWN, "a new hero: back-fill due");
    g_q[0].cls = 77; g_q[0].su = 0xffff; g_q[0].diff = 1; g_q[1].cls = 526; g_q[1].su = 60; g_q[1].diff = 2; g_nq = 2;
    settling x = { .who = "Cid", .give_up = 0, .server = 1 };
    settle_read(&D, &x);
    CHECK(g_nq == 2 && g_due && !g_bk.nrec, "the sources are not there yet: the kills wait");
    x = (settling){ .who = "Cid", .give_up = 1, .server = 0 };
    settle_read(&D, &x);
    CHECK(!g_nq && !g_due && (g_bk.flags & DM_BK_F_BACKFILLED) && x.dropped && dm_bk_count(&g_bk, 'M', 77, 1) == 1 && dm_bk_count(&g_bk, 'M', 526, 2) == 1,
          "given up: back-fill left out for good, the kills counted once");
    settle_read(&D, &x);
    CHECK(dm_bk_count(&g_bk, 'M', 77, 1) == 1, "a second settle counts nothing again");


    {   FILE *src = fopen("../dm_kills.c", "r"); static char text[200000]; size_t got = src ? fread(text, 1, sizeof text - 1, src) : 0; if (src) fclose(src); text[got] = 0;
        char *at = strstr(text, "static const kb_row k_rows[] = {"); int rows = 0, same = 1;
        for (char *p = at; p && rows < DM_FT_BOARD_ROWS && (p = strstr(p, "{\"")); p++) {
            char key[40], str[64] = "", kind[8]; int id, id2;
            if (sscanf(p, "{\"%39[^\"]\", \"%63[^\"]\", %*d, K_%7[A-Z], %d, %d}", key, str, kind, &id, &id2) != 5 &&
                sscanf(p, "{\"%39[^\"]\", NULL, %*d, K_%7[A-Z], %d, %d}", key, kind, &id, &id2) != 4) continue;
            const board_row *k = &k_board[rows++];
            if (!strcmp(kind, "CLS")) same &= k->kind == 0;
            else if (!strcmp(kind, "SU")) same &= k->kind == 1 && k->id == id && k->name && !strcmp(k->name, str);
            else same &= k->kind == 2 && k->id == id;
        }
        CHECK(at && rows == DM_FT_BOARD_ROWS && same, "the board table matches dm_kills.c (%d rows)", rows);
        memset(board_rows, 0, sizeof board_rows); board_ready = 1;
        board_rows[0].cls = 156; board_rows[0].n[2] = 3;
        board_rows[42].cls = -1; board_rows[42].n[1] = 2;
        board_rows[22].cls = -1; board_rows[22].n[0] = 1;
        dm_bk_seen seen[3 * DM_FT_BOARD_ROWS]; int n = board_seen("Ann", seen, 3 * DM_FT_BOARD_ROWS);
        CHECK(n == 5 && seen[0].cls == 156 && seen[0].diff == 2 && seen[0].n == 3, "a class row");
        int council = 0, nihl = 0;
        for (int i = 0; i < n; i++) { if (seen[i].su == 26 || seen[i].su == 27 || seen[i].su == 29) council++; if (seen[i].su == 60 && seen[i].name && !strcmp(seen[i].name, "Nihlathak") && seen[i].diff == 1) nihl++; }
        CHECK(council == 3 && nihl == 1, "the council's members and Nihlathak with his name key");
        CHECK(board_seen("Bob", seen, 3 * DM_FT_BOARD_ROWS) < 0, "another character's board: none");
    }
    unlink(file); unlink(aside); rmdir(dir); rmdir(root);
    free(b); free(c); free(f); free(again);
    printf("book: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
