


#define _POSIX_C_SOURCE 200809L
#include "t_gear_stock.h"
#include "../dm_gear_dump.h"
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static dm_gear_tables g_gt;
static const dm_sim_data *g_d;
static char g_dir[64];

static int run_search(dm_gear_problem *p, const dm_gear_goal *goal, const dm_gear_pick *seed, dm_gear_result *res) {
    dm_gear_search *s = dm_gear_search_new(p);
    if (!s) return -1;
    if (seed) dm_gear_search_seed(s, seed);
    int rc = dm_gear_search_run(s, goal, NULL, NULL, res);
    dm_gear_search_free(s);
    return rc;
}
static int same_result(const dm_gear_result *a, const dm_gear_result *b) {
    return !memcmp(a->score.key, b->score.key, sizeof a->score.key) && !memcmp(&a->best, &b->best, sizeof a->best);
}
static int count_bins(void) {
    DIR *d = opendir(g_dir); int n = 0;
    struct dirent *e;
    while (d && (e = readdir(d))) if (!strncmp(e->d_name, "gear-problem-", 13) && strstr(e->d_name, ".bin")) n++;
    if (d) closedir(d);
    return n;
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    g_d = gs_tables(&g_gt);

    gspec sp; memset(&sp, 0, sizeof sp);
    sp.profile = gs_profile("class5-level80.prof");
    sp.act = 3; sp.seed = 105; sp.variants = 1; sp.skills = 1; sp.sets = 2; sp.chain = 1;
    sp.count[DM_GEAR_K_WEAPON1] = 13; sp.count[DM_GEAR_K_HEAD] = 1; sp.count[DM_GEAR_K_ARMOR] = 4; sp.count[DM_GEAR_K_AMULET] = 7;
    sp.count[DM_GEAR_K_RING] = 8; sp.count[DM_GEAR_K_BELT] = 2; sp.count[DM_GEAR_K_BOOTS] = 1; sp.charms_inv = 6; sp.charms_stash = 6; sp.blocked = 3;
    static gstock g;
    CHECK(!gs_build(&g, g_d, &g_gt, &sp), "stock not built");
    g.p.charms_fixed = 1;
    dm_gear_goal goal = { .survival_hits = 15, .boss_share = 0.5 };

    dm_gear_result r1, r2;
    CHECK(!run_search(&g.p, &goal, NULL, &r1) && r1.status == DM_GEAR_S_DONE, "in-memory search failed");
    fprintf(stderr, "dump round trip: %d candidates, %d weapons, key %.4f/%.4f/%.4f\n",
            g.p.ncand, g.p.nweapon, r1.score.key[0], r1.score.key[1], r1.score.key[2]);

    snprintf(g_dir, sizeof g_dir, "/tmp/d2r-gdump-%d", (int)getpid());
    CHECK(!mkdir(g_dir, 0777) || errno == EEXIST, "no test directory: %s", strerror(errno));
    const uint64_t rev = 0x0123456789abcdefull;
    char path[160];


    CHECK(!dm_gdump_write(g_dir, rev, &g.p, &goal, 3, NULL), "write refused");
    snprintf(path, sizeof path, "%s/gear-problem-%016llx.bin", g_dir, (unsigned long long)rev);
    CHECK(!access(path, F_OK), "dump file missing");
    static dm_gdump dg;
    if (dm_gdump_read(path, g_d, &g_gt, &dg)) { fprintf(stderr, "FAIL: reader refused a good dump\n"); return 1; }
    CHECK(dg.head.rev == rev && dg.head.ncand == g.p.ncand && dg.head.nweapon == g.p.nweapon, "header counts differ");
    CHECK(dg.head.threads == 3 && dg.head.charms_fixed == 1 && !dg.head.has_seed, "header options differ");
    CHECK(dg.head.sim_hash == dm_gdump_sim_hash(g_d) && dg.head.gear_hash == dm_gdump_gear_hash(&g_gt), "header table hashes differ");
    CHECK(dg.p.cand == dg.cand && dg.p.weapon == dg.weapon && dg.p.d == g_d && dg.p.t == &g_gt, "pointers not rebuilt");
    CHECK(!memcmp(dg.p.cand, g.cand, sizeof *g.cand * (size_t)g.p.ncand) && !memcmp(dg.p.weapon, g.weapon, sizeof *g.weapon * (size_t)g.p.nweapon), "arrays not identical");
    CHECK(!run_search(&dg.p, &goal, NULL, &r2) && r2.status == DM_GEAR_S_DONE, "replayed search failed");
    CHECK(same_result(&r1, &r2), "replayed search differs (key %.4f/%.4f/%.4f vs %.4f/%.4f/%.4f)",
          r2.score.key[0], r2.score.key[1], r2.score.key[2], r1.score.key[0], r1.score.key[1], r1.score.key[2]);
    dm_gdump_free(&dg);


    dm_gdump_result(g_dir, rev, "[dm] best gear: search 0123456789abcdef ended 0 after 12 runs, 34 ms; best 1/2/3s/4 hits/5, picks 1 2 3 4 5 6 7 8 9 10\n");
    snprintf(path, sizeof path, "%s/gear-problem-%016llx.txt", g_dir, (unsigned long long)rev);
    size_t n; char *txt = sim_read(path, &n);
    CHECK(txt && strstr(txt, "after 12 runs"), "result line not appended");
    free(txt);
    snprintf(path, sizeof path, "%s/gear-problem-%016llx.txt", g_dir, (unsigned long long)(rev + 1));
    dm_gdump_result(g_dir, rev + 1, "no dump of this revision\n");
    CHECK(access(path, F_OK), "a result line was written beside a missing dump");


    snprintf(path, sizeof path, "%s/gear-problem-%016llx.bin", g_dir, (unsigned long long)(rev + 2));
    CHECK(!dm_gdump_write(g_dir, rev + 2, &g.p, &goal, 1, &r1.best), "write with seed refused");
    CHECK(!dm_gdump_read(path, g_d, &g_gt, &dg), "reader refused the seeded dump");
    CHECK(dg.head.has_seed && !memcmp(&dg.seed, &r1.best, sizeof dg.seed), "seed not round tripped");
    CHECK(!run_search(&dg.p, &goal, NULL, &r2) && r2.status == DM_GEAR_S_DONE && same_result(&r1, &r2), "seeded replay differs");
    CHECK(!run_search(&dg.p, &goal, &dg.seed, &r2) && r2.status == DM_GEAR_S_DONE && same_result(&r1, &r2), "replaying with the seed differs");
    dm_gdump_free(&dg);


    FILE *f = fopen(path, "r+b");
    CHECK(f, "dump unreadable for tampering");
    if (f) {
        int bad = 0;
        fseek(f, (long)offsetof(dm_gdump_head, gear_hash), SEEK_SET);
        uint64_t h; bad |= fread(&h, sizeof h, 1, f) != 1; h ^= 1;
        fseek(f, -(long)sizeof h, SEEK_CUR); bad |= fwrite(&h, sizeof h, 1, f) != 1; fclose(f);
        CHECK(!bad, "gear hash untouchable");
        CHECK(dm_gdump_read(path, g_d, &g_gt, &dg), "reader took a dump of other gear tables");
        f = fopen(path, "r+b");
        fseek(f, (long)offsetof(dm_gdump_head, sz) + 4 * DG_SZ_CAND, SEEK_SET);
        uint32_t one = 7; bad = fwrite(&one, sizeof one, 1, f) != 1; fclose(f);
        CHECK(!bad, "struct size untouchable");
        CHECK(dm_gdump_read(path, g_d, &g_gt, &dg), "reader took a dump of other struct sizes");
        f = fopen(path, "r+b"); CHECK(!ftruncate(fileno(f), 40), "truncate failed"); fclose(f);
        CHECK(dm_gdump_read(path, g_d, &g_gt, &dg), "reader took a truncated dump");
    }


    CHECK(!dm_gdump_write(g_dir, rev, &g.p, &goal, 1, NULL), "rewrite refused");
    for (int i = 0; i < 4; i++) { CHECK(!dm_gdump_write(g_dir, rev + 10 + (uint64_t)i, &g.p, &goal, 1, NULL), "extra dump refused"); }
    CHECK(count_bins() == DM_GDUMP_KEEP, "kept %d dumps, want %d", count_bins(), DM_GDUMP_KEEP);


    DIR *d = opendir(g_dir);
    struct dirent *e;
    while (d && (e = readdir(d))) {
        if (strncmp(e->d_name, "gear-problem-", 13)) continue;
        char p[1200]; snprintf(p, sizeof p, "%s/%s", g_dir, e->d_name);
        unlink(p);
    }
    if (d) closedir(d);
    rmdir(g_dir);
    if (failures) { fprintf(stderr, "%d failures\n", failures); return 1; }
    printf("gear dump round trip ok\n");
    return 0;
}
