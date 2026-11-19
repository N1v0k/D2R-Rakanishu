


#include <math.h>
#include <stdio.h>
#include "../dm_splash_rule.h"

static int checks, fails;
#define CHECK(c, msg, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: " msg " (line %d)\n", ##__VA_ARGS__, __LINE__); } } while (0)

enum { AMA, SOR, NEC, PAL, BAR, DRU, ASN, WAR };

static void groups(void) {
    static const struct { const char *type; int two; dm_splash_group g; } t[] = {
        { "pole", 1, DM_SPLASH_HEAVY }, { "axe", 1, DM_SPLASH_HEAVY }, { "axe ", 1, DM_SPLASH_HEAVY }, { "hamm", 1, DM_SPLASH_HEAVY },
        { "axe", 0, DM_SPLASH_MEDIUM }, { "swor", 1, DM_SPLASH_MEDIUM },
        { "swor", 0, DM_SPLASH_LIGHT }, { "hamm", 0, DM_SPLASH_LIGHT }, { "club", 0, DM_SPLASH_LIGHT }, { "mace", 0, DM_SPLASH_LIGHT },
        { "scep", 0, DM_SPLASH_LIGHT }, { "staf", 1, DM_SPLASH_LIGHT }, { "h2h", 0, DM_SPLASH_LIGHT }, { "h2h2", 0, DM_SPLASH_LIGHT },
        { "spea", 1, DM_SPLASH_THRUST }, { "aspe", 1, DM_SPLASH_THRUST }, { "jave", 0, DM_SPLASH_THRUST }, { "ajav", 0, DM_SPLASH_THRUST },
        { "knif", 0, DM_SPLASH_NONE }, { "wand", 0, DM_SPLASH_NONE }, { "orb", 0, DM_SPLASH_NONE }, { "taxe", 0, DM_SPLASH_NONE },
        { "tkni", 0, DM_SPLASH_NONE }, { "tpot", 0, DM_SPLASH_NONE }, { "bow", 1, DM_SPLASH_NONE }, { "abow", 1, DM_SPLASH_NONE },
        { "xbow", 1, DM_SPLASH_NONE }, { NULL, 0, DM_SPLASH_NONE },
        { "grim", 0, DM_SPLASH_UNKNOWN }, { "ax", 0, DM_SPLASH_UNKNOWN }, { "", 0, DM_SPLASH_UNKNOWN },
    };
    for (unsigned i = 0; i < sizeof t / sizeof *t; i++)
        CHECK(dm_splash_group_of(t[i].type, t[i].two) == t[i].g, "group of %s two=%d: %d, want %d", t[i].type ? t[i].type : "(none)", t[i].two,
              dm_splash_group_of(t[i].type, t[i].two), t[i].g);
}

static void shares(void) {
    static const struct { dm_splash_group g; int cls, str, share; } t[] = {
        { DM_SPLASH_HEAVY, BAR, 225, 90 }, { DM_SPLASH_HEAVY, BAR, 226, 90 }, { DM_SPLASH_HEAVY, BAR, 150, 60 },
        { DM_SPLASH_HEAVY, BAR, 75, 30 }, { DM_SPLASH_HEAVY, BAR, 74, 30 }, { DM_SPLASH_HEAVY, BAR, 0, 30 }, { DM_SPLASH_HEAVY, BAR, 190, 75 },
        { DM_SPLASH_MEDIUM, BAR, 190, 50 }, { DM_SPLASH_LIGHT, BAR, 190, 30 }, { DM_SPLASH_THRUST, BAR, 150, 25 },
        { DM_SPLASH_LIGHT, PAL, 160, 20 }, { DM_SPLASH_LIGHT, PAL, 180, 25 },
        { DM_SPLASH_HEAVY, AMA, 150, 50 }, { DM_SPLASH_HEAVY, DRU, 150, 50 }, { DM_SPLASH_HEAVY, ASN, 150, 50 },
        { DM_SPLASH_HEAVY, NEC, 150, 30 }, { DM_SPLASH_HEAVY, WAR, 150, 30 }, { DM_SPLASH_HEAVY, SOR, 225, 0 },
        { DM_SPLASH_LIGHT, NEC, 225, 15 }, { DM_SPLASH_LIGHT, NEC, 150, 10 }, { DM_SPLASH_LIGHT, NEC, 119, 0 },
        { DM_SPLASH_THRUST, WAR, 75, 0 },
        { DM_SPLASH_NONE, BAR, 225, 0 }, { DM_SPLASH_UNKNOWN, BAR, 225, 0 }, { DM_SPLASH_HEAVY, -1, 150, 0 }, { DM_SPLASH_HEAVY, 8, 150, 0 },
    };
    for (unsigned i = 0; i < sizeof t / sizeof *t; i++) {
        dm_splash_area a = dm_splash_area_of(t[i].g, t[i].cls, t[i].str, 0, 1);
        CHECK(a.share == t[i].share, "share group %d class %d str %d: %d, want %d", t[i].g, t[i].cls, t[i].str, a.share, t[i].share);
        if (!a.share) CHECK(a.width == 0 && a.depth == 0, "no splash has no cone (group %d class %d)", t[i].g, t[i].cls);
    }
}

static void cones(void) {
    static const struct { dm_splash_group g; int dex, range, width, depth; } t[] = {
        { DM_SPLASH_HEAVY, 0, 1, 45, 1 }, { DM_SPLASH_MEDIUM, 0, 1, 35, 1 }, { DM_SPLASH_LIGHT, 0, 1, 30, 1 }, { DM_SPLASH_THRUST, 0, 3, 20, 2 },
        { DM_SPLASH_HEAVY, 20, 0, 50, 0 }, { DM_SPLASH_MEDIUM, 20, 0, 40, 0 },
        { DM_SPLASH_HEAVY, 130, 1, 77, 1 }, { DM_SPLASH_LIGHT, 130, 0, 62, 0 }, { DM_SPLASH_MEDIUM, 130, 2, 67, 2 },
        { DM_SPLASH_HEAVY, 180, 4, 90, 2 }, { DM_SPLASH_HEAVY, 400, 4, 90, 2 }, { DM_SPLASH_THRUST, 3, 4, 20, 2 }, { DM_SPLASH_THRUST, -20, 2, 20, 2 },
        { DM_SPLASH_LIGHT, 4, 9, 31, 2 }, { DM_SPLASH_THRUST, 180, 4, 65, 2 }, { DM_SPLASH_LIGHT, 0, -1, 30, 0 },
    };
    for (unsigned i = 0; i < sizeof t / sizeof *t; i++) {
        dm_splash_area a = dm_splash_area_of(t[i].g, BAR, 225, t[i].dex, t[i].range);
        CHECK(a.width == t[i].width && a.depth == t[i].depth, "cone group %d dex %d range %d: %d deg / %d, want %d / %d", t[i].g, t[i].dex,
              t[i].range, a.width, a.depth, t[i].width, t[i].depth);
    }
}

static int near(double a, double b) { return a > b - 1e-9 && a < b + 1e-9; }


static void geometry(void) {
    const dm_splash_area d0 = { 40, 40, 0 }, d1 = { 40, 45, 1 }, d2 = { 40, 20, 2 }, wide = { 40, 90, 0 };
    CHECK(near(dm_splash_cone_factor(&d1, 0, 0, 2, 0, 3, 0), 1), "on the axis, one row behind, depth 1: the full share");
    CHECK(dm_splash_cone_factor(&d0, 0, 0, 2, 0, 3, 0) == 0, "depth 0: the row behind the primary is out");
    CHECK(dm_splash_cone_factor(&d1, 0, 0, 2, 0, 4, 0) == 0, "depth 1: two rows behind is out");
    CHECK(near(dm_splash_cone_factor(&d2, 0, 0, 2, 0, 4, 0), 1), "depth 2 (a spear): two rows behind is in");
    CHECK(dm_splash_cone_factor(&d2, 0, 0, 2, 0, 5, 0) == 0, "depth 2: three rows behind is out");

    CHECK(dm_splash_cone_factor(&d0, 0, 0, 4, 0, 4, 1) > 0, "depth 0: beside the primary (14 degrees) is in");
    CHECK(dm_splash_cone_factor(&d0, 0, 0, 3, 3, 4, 2) > 0, "depth 0, diagonal axis: beside the primary is in");
    CHECK(dm_splash_cone_factor(&d0, 0, 0, 3, 3, 4, 4) == 0, "depth 0, diagonal axis: the next one behind is out");
    CHECK(dm_splash_cone_factor(&wide, 0, 0, 2, 0, 2, 2) > 0, "a 90 degree cone takes the neighbour beside the primary at 45 degrees");
    CHECK(dm_splash_cone_factor(&d0, 0, 0, 2, 0, 2, 2) == 0, "a 40 degree cone does not");
    CHECK(near(dm_splash_cone_factor(&d0, 0, 0, 4, 0, 4, 2), 0.5), "27 degrees: its body reaches into a 40 degree cone, at the edge's half");
    CHECK(dm_splash_cone_factor(&d0, 0, 0, 4, 0, 4, 3) == 0, "37 degrees: its body does not");

    const dm_splash_area start = { 20, 40, 0 }, lax = { 60, 65, 1 }, spear = { 25, 20, 0 };
    CHECK(near(dm_splash_cone_factor(&start, 0, 0, 1, 1, 2, 0), 0.5), "a starting barbarian's hand axe takes the one beside the primary, at half");
    CHECK(dm_splash_cone_factor(&start, 0, 0, 1, 1, 2, 2) == 0 && dm_splash_cone_factor(&start, 0, 0, 1, 1, 3, 1) == 0
          && dm_splash_cone_factor(&start, 0, 0, 1, 1, 1, 3) == 0, "but not the row behind it");
    CHECK(dm_splash_cone_factor(&start, 0, 0, 1, 1, 1, -1) == 0, "nor the one beside the player");
    CHECK(near(dm_splash_cone_factor(&lax, 0, 0, 1, 1, 2, 2), 1) && dm_splash_cone_factor(&lax, 0, 0, 1, 1, 2, 0) > 0,
          "a two-hand axe (range 1) also reaches the one behind the primary, in full");
    CHECK(dm_splash_cone_factor(&spear, 0, 0, 1, 1, 2, 0) == 0, "a thrust's 20 degrees miss the one beside the primary");
    CHECK(dm_splash_cone_factor(&wide, 0, 0, 2, 0, -1, 0) == 0 && dm_splash_cone_factor(&wide, 0, 0, 2, 0, 0, 3) == 0, "behind and beside the player are out");
    CHECK(dm_splash_cone_factor(&wide, 0, 0, 0, 0, 1, 0) == 0, "the primary on the attacker's subtile: no axis, no cone");
    CHECK(dm_splash_cone_factor(&wide, 0, 0, 2, 0, 0, 0) == 0, "a monster on the attacker's subtile is out");

    CHECK(near(dm_splash_cone_factor(&wide, 0, 0, 4, 0, 4, 0), 1), "the primary's spot itself: 1");
    CHECK(near(dm_splash_cone_factor(&wide, 0, 0, 2, 0, 2, 2), 0.5), "at the edge (45 of 45 degrees): 1/2 (%f)", dm_splash_cone_factor(&wide, 0, 0, 2, 0, 2, 2));
    double f = dm_splash_cone_factor(&wide, 0, 0, 4, 0, 4, 1), want = 1 - 0.5 * (atan(0.25) * 180 / 3.14159265358979323846) / 45;
    CHECK(near(f, want), "14 degrees of 45: %f, want %f", f, want);
    CHECK(f > dm_splash_cone_factor(&wide, 0, 0, 4, 0, 4, 2), "further off the axis takes less");
}


static void reach(void) {
    dm_splash_area a = { 60, 45, 0 };
    CHECK(near(dm_splash_reach(&a, 4), 1 + 1 * 0.75 * 0.6), "45 degrees at depth 0: one neighbour (%f)", dm_splash_reach(&a, 4));
    a = (dm_splash_area){ 60, 90, 2 };
    CHECK(near(dm_splash_reach(&a, 4), 1 + 3 * 0.75 * 0.6), "90 degrees at depth 2: three (%f)", dm_splash_reach(&a, 4));
    CHECK(near(dm_splash_reach(&a, 2), 1 + 2 * 0.75 * 0.6), "at most the pack (%f)", dm_splash_reach(&a, 2));
    CHECK(near(dm_splash_reach(&a, 0), 1), "no pack, no reach");
    a = (dm_splash_area){ 25, 20, 2 };
    CHECK(near(dm_splash_reach(&a, 4), 1 + (20 / 45.0 + 1) * 0.75 * 0.25), "a spear: narrow but deep (%f)", dm_splash_reach(&a, 4));
    a = (dm_splash_area){ 0, 0, 0 };
    CHECK(near(dm_splash_reach(&a, 4), 1), "no share, no reach");

    double last = 0;
    for (int w = 20; w <= 90; w += 5) for (int d = 0; d <= 2; d++) {
        dm_splash_area x = { 50, w, d }, y = { 55, w, d };
        double r = dm_splash_reach(&x, 4);
        CHECK(dm_splash_reach(&y, 4) >= r, "more share, no less reach");
        if (d == 0) { CHECK(r >= last, "wider, no less reach (%d)", w); last = r; }
    }
}

static void skills(void) {
    CHECK(dm_splash_skill_excluded(1, 42), "a kick never splashes");
    CHECK(dm_splash_skill_excluded(0, 76), "whirlwind never splashes");
    CHECK(dm_splash_skill_excluded(0, 150), "smite never splashes");
    CHECK(!dm_splash_skill_excluded(0, 1), "the normal attack splashes");
    CHECK(!dm_splash_skill_excluded(0, 0), "a skill without a server function is not excluded by the rule");
}

int main(void) {
    groups(); shares(); cones(); geometry(); reach(); skills();
    printf("t_splash_rule: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
