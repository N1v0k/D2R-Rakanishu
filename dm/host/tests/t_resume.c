
#include "../dm_resume_core.c"
#include <assert.h>
#include <sys/stat.h>

static int fails;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static char g_root[600];
static void path(char *out, size_t cap, const char *rel) { snprintf(out, cap, "%s/%s", g_root, rel); }
static void put(const char *rel, const char *text) { char p[900]; path(p, sizeof p, rel); FILE *f = fopen(p, "w"); assert(f); fputs(text, f); fclose(f); }
static int exists(const char *rel) { char p[900]; struct stat st; path(p, sizeof p, rel); return !stat(p, &st); }

static dm_resume_pos sample(void) {
    dm_resume_pos p = { .who = "Hero", .level = 3, .x = 5123, .y = 4410, .difficulty = 0, .map_fp = 0x1122334455667788ull, .save_fp = 0 };
    return p;
}

static void test_format(void) {
    dm_resume_pos p = sample(), q; p.save_fp = 0xabcdefull; char t[512];
    size_t n = dm_resume_format(&p, t, sizeof t);
    CHECK(n > 0 && dm_resume_parse(t, &q) && !memcmp(&p, &q, sizeof p));
    char cut[512]; memcpy(cut, t, n - 1); cut[n - 1] = 0;
    CHECK(!dm_resume_parse(cut, &q));
    char old[600]; snprintf(old, sizeof old, "# dm_resume 0%s", strchr(t, '\n'));
    CHECK(!dm_resume_parse(old, &q));
    char twice[700]; snprintf(twice, sizeof twice, "%slevel 4\n", t);
    CHECK(!dm_resume_parse(twice, &q));
    char junk[700]; snprintf(junk, sizeof junk, "%sextra 1\n", t);
    CHECK(!dm_resume_parse(junk, &q));
    CHECK(!dm_resume_parse("# dm_resume 1\nwho Hero\nlevel 3\npos 1 2\ndifficulty 3\nmap 1\nsave 1\n", &q));
    CHECK(!dm_resume_parse("# dm_resume 1\nwho ../x\nlevel 3\npos 1 2\ndifficulty 0\nmap 1\nsave 1\n", &q));
    CHECK(dm_resume_parse("# dm_resume 1\nwho Hero\nlevel 3\npos 1 2\ndifficulty 0\nmap 1\nsave 1\n", &q));
}

static void test_decide(void) {
    dm_resume_pos p = sample(); p.save_fp = 77;
    CHECK(dm_resume_decide(1, 1, &p, "Hero", 77, 0) == DM_RESUME_OFFER);
    CHECK(dm_resume_decide(0, 1, &p, "Hero", 77, 0) == DM_RESUME_SWITCH_OFF);
    CHECK(dm_resume_decide(1, 0, &p, "Hero", 77, 0) == DM_RESUME_NO_MARK);
    CHECK(dm_resume_decide(1, 1, NULL, "Hero", 77, 0) == DM_RESUME_BAD_FILE);
    CHECK(dm_resume_decide(1, 1, &p, "Other", 77, 0) == DM_RESUME_OTHER_CHAR);
    CHECK(dm_resume_decide(1, 1, &p, "Hero", 78, 0) == DM_RESUME_OTHER_SAVE);
    CHECK(dm_resume_decide(1, 1, &p, "Hero", 0, 0) == DM_RESUME_OTHER_SAVE);
    CHECK(dm_resume_decide(1, 1, &p, "Hero", 77, 1) == DM_RESUME_OTHER_DIFFICULTY);
    static const int no[] = { 1, 40, 75, 103, 109, 37, 73, 102, 108, 131, 132, 39, 136, 0, -1, 256 };
    for (size_t i = 0; i < sizeof no / sizeof *no; i++) { p.level = no[i]; CHECK(dm_resume_decide(1, 1, &p, "Hero", 77, 0) == DM_RESUME_LEVEL); }
    static const int yes[] = { 2, 3, 8, 9, 35, 36, 101, 107, 130, 133 };
    for (size_t i = 0; i < sizeof yes / sizeof *yes; i++) { p.level = yes[i]; CHECK(dm_resume_decide(1, 1, &p, "Hero", 77, 0) == DM_RESUME_OFFER); }
}

static void test_fp(void) {
    uint64_t a = dm_resume_fp_room(dm_resume_fp_room(DM_RESUME_FP_START, 1, 2, 8, 8), 9, 2, 8, 8);
    uint64_t b = dm_resume_fp_room(dm_resume_fp_room(DM_RESUME_FP_START, 1, 2, 8, 8), 9, 2, 8, 8);
    uint64_t c = dm_resume_fp_room(dm_resume_fp_room(DM_RESUME_FP_START, 1, 2, 8, 8), 9, 3, 8, 8);
    CHECK(a == b && a != c);
}


static void test_marks(void) {
    dm_resume_marks m = {0};

    dm_resume_left(&m); unsigned g = dm_resume_leave_begin(&m);
    CHECK(dm_resume_sidecar_due(&m)); dm_resume_sidecar_done(&m);
    CHECK(dm_resume_leave_end(&m, g));
    CHECK(!dm_resume_sidecar_due(&m));

    CHECK(dm_resume_back(&m));
    CHECK(!dm_resume_back(&m));


    memset(&m, 0, sizeof m);
    dm_resume_left(&m); g = dm_resume_leave_begin(&m);
    CHECK(dm_resume_back(&m));
    dm_resume_sidecar_done(&m);
    CHECK(!dm_resume_leave_end(&m, g));


    memset(&m, 0, sizeof m);
    dm_resume_left(&m); g = dm_resume_leave_begin(&m);
    dm_resume_back(&m); dm_resume_left(&m);
    dm_resume_sidecar_done(&m);
    CHECK(!dm_resume_leave_end(&m, g));
    unsigned g2 = dm_resume_leave_begin(&m); dm_resume_sidecar_done(&m);
    CHECK(g2 != g && dm_resume_leave_end(&m, g2));


    memset(&m, 0, sizeof m);
    dm_resume_left(&m); g = dm_resume_leave_begin(&m); dm_resume_sidecar_done(&m); CHECK(dm_resume_leave_end(&m, g));
    dm_resume_back(&m); dm_resume_left(&m); g = dm_resume_leave_begin(&m);
    CHECK(!dm_resume_leave_end(&m, g));


    memset(&m, 0, sizeof m);
    g = dm_resume_leave_begin(&m); dm_resume_sidecar_done(&m);
    CHECK(!dm_resume_leave_end(&m, g));



    memset(&m, 0, sizeof m);
    dm_resume_left(&m); g = dm_resume_leave_begin(&m); dm_resume_sidecar_done(&m);
    dm_resume_dropped(&m);
    CHECK(!dm_resume_leave_end(&m, g));
    dm_resume_left(&m); g = dm_resume_leave_begin(&m); dm_resume_sidecar_done(&m);
    CHECK(dm_resume_leave_end(&m, g));


    memset(&m, 0, sizeof m);
    CHECK(!dm_resume_back(&m));
}

static void reset_dir(void) {
    char cmd[800]; snprintf(cmd, sizeof cmd, "chmod -R u+w '%s' 2>/dev/null; rm -rf '%s'", g_root, g_root); CHECK(system(cmd) == 0);
    CHECK(mkdir(g_root, 0755) == 0);
    char p[900]; path(p, sizeof p, "save"); mkdir(p, 0755); path(p, sizeof p, "save/Hero"); mkdir(p, 0755);
    path(p, sizeof p, "marks"); mkdir(p, 0755);
}


static void test_join(void) {
    char save[900], mark[900]; path(save, sizeof save, "save"); path(mark, sizeof mark, "marks/mark");
    dm_resume_pos p = sample(), got; int failed = 0;
    reset_dir();
    put("save/Hero/Hero.d2s", "character bytes one");
    p.save_fp = dm_resume_save_fp(save, "Hero");
    CHECK(p.save_fp != 0 && dm_resume_sidecar_write(save, &p) && !exists("save/Hero/Hero.sw_resume.tmp"));
    CHECK(dm_resume_mark_write(mark, "Hero"));


    CHECK(dm_resume_join(mark, save, "Other", 1, 0, &got, &failed) == DM_RESUME_NO_MARK && !failed && exists("marks/mark"));

    CHECK(dm_resume_join(mark, save, "Hero", 1, 0, &got, &failed) == DM_RESUME_OFFER && !failed && !exists("marks/mark"));
    CHECK(got.level == p.level && got.x == p.x && got.y == p.y && got.map_fp == p.map_fp);
    CHECK(dm_resume_join(mark, save, "Hero", 1, 0, &got, &failed) == DM_RESUME_NO_MARK);


    CHECK(dm_resume_mark_write(mark, "Hero"));
    put("save/Hero/Hero.d2s", "character bytes two");
    CHECK(dm_resume_join(mark, save, "Hero", 1, 0, &got, &failed) == DM_RESUME_OTHER_SAVE && !exists("marks/mark"));
    put("save/Hero/Hero.d2s", "character bytes one");

    CHECK(dm_resume_mark_write(mark, "Hero"));
    put("save/Hero/Hero.sw_resume", "# dm_resume 1\nwho Hero\nlevel 3\n");
    CHECK(dm_resume_join(mark, save, "Hero", 1, 0, &got, &failed) == DM_RESUME_BAD_FILE);

    CHECK(dm_resume_sidecar_write(save, &p) && dm_resume_mark_write(mark, "Hero"));
    CHECK(dm_resume_join(mark, save, "Hero", 0, 0, &got, &failed) == DM_RESUME_SWITCH_OFF && !exists("marks/mark"));


    CHECK(dm_resume_sidecar_write(save, &p) && dm_resume_committed(save, "Hero"));
    put("save/Hero/Hero.d2s", "character bytes two");
    CHECK(!dm_resume_committed(save, "Hero") && !dm_resume_committed(save, "Other"));
    put("save/Hero/Hero.d2s", "character bytes one");
    put("save/Hero/Hero.sw_resume", "# dm_resume 1\nwho Hero\n");
    CHECK(!dm_resume_committed(save, "Hero"));
    CHECK(dm_resume_sidecar_write(save, &p));


    CHECK(dm_resume_mark_write(mark, "Hero") && dm_resume_mark_delete(mark));
    CHECK(dm_resume_join(mark, save, "Hero", 1, 0, &got, &failed) == DM_RESUME_NO_MARK && !failed);


    CHECK(dm_resume_mark_write(mark, "Hero"));
    char dir[900]; path(dir, sizeof dir, "marks"); CHECK(chmod(dir, 0555) == 0);
    for (int start = 0; start < 2; start++) {
        failed = 0;
        CHECK(dm_resume_join(mark, save, "Hero", 1, 0, &got, &failed) != DM_RESUME_OFFER && failed && exists("marks/mark"));
    }
    CHECK(chmod(dir, 0755) == 0);

    CHECK(dm_resume_join(mark, save, "Hero", 1, 0, &got, &failed) == DM_RESUME_OFFER && !failed && !exists("marks/mark"));


    CHECK(chmod(dir, 0555) == 0);
    CHECK(!dm_resume_mark_write(mark, "Hero") && !exists("marks/mark") && !exists("marks/mark.tmp"));
    CHECK(chmod(dir, 0755) == 0);
    dm_resume_pos q = p; strcpy(q.who, "Nobody");
    CHECK(!dm_resume_sidecar_write(save, &q));
    CHECK(dm_resume_mark_delete(mark));
}


static void test_way_back(void) {
    dm_resume_pos p = sample(); dm_resume_run r = {0};
    CHECK(dm_resume_step(&r, 10, 1, 1) == DM_RESUME_ACT_NONE);
    dm_resume_ask(&r, &p);
    CHECK(r.st == DM_RESUME_ASKING);
    for (uint32_t t = 11; t < 400; t++) CHECK(dm_resume_step(&r, t, 1, 1) == DM_RESUME_ACT_NONE);
    CHECK(dm_resume_choose(&r, 1) && r.st == DM_RESUME_GO);
    CHECK(!dm_resume_choose(&r, 0));
    CHECK(dm_resume_step(&r, 400, 1, 1) == DM_RESUME_ACT_WARP);
    CHECK(dm_resume_step(&r, 401, 1, 1) == DM_RESUME_ACT_NONE);
    CHECK(dm_resume_step(&r, 410, 3, 0) == DM_RESUME_ACT_NONE);
    CHECK(dm_resume_step(&r, 420, 3, 3) == DM_RESUME_ACT_PLACE);
    CHECK(r.st == DM_RESUME_DONE && dm_resume_step(&r, 421, 3, 3) == DM_RESUME_ACT_NONE);


    dm_resume_ask(&r, &p);
    CHECK(dm_resume_choose(&r, 0) && r.st == DM_RESUME_DONE);
    for (uint32_t t = 0; t < 300; t++) CHECK(dm_resume_step(&r, t, 1, 1) == DM_RESUME_ACT_NONE);


    dm_resume_ask(&r, &p); dm_resume_choose(&r, 1);
    CHECK(dm_resume_step(&r, 1000, 1, 1) == DM_RESUME_ACT_WARP);
    CHECK(dm_resume_step(&r, 1000 + DM_RESUME_ARRIVE_TICKS, 3, 0) == DM_RESUME_ACT_NONE);
    CHECK(dm_resume_step(&r, 1001 + DM_RESUME_ARRIVE_TICKS, 3, 0) == DM_RESUME_ACT_GIVE_UP);
    CHECK(dm_resume_step(&r, 2000, 3, 3) == DM_RESUME_ACT_NONE);
}

int main(void) {
    const char *t = getenv("TMPDIR");
    snprintf(g_root, sizeof g_root, "%s/t_resume.%d", t && *t ? t : "/tmp", (int)getpid());
    test_format(); test_decide(); test_fp(); test_marks(); test_join(); test_way_back();
    char cmd[800]; snprintf(cmd, sizeof cmd, "chmod -R u+w '%s'; rm -rf '%s'", g_root, g_root); if (system(cmd)) fails++;
    if (fails) { fprintf(stderr, "t_resume: %d failed\n", fails); return 1; }
    printf("t_resume: ok\n");
    return 0;
}
