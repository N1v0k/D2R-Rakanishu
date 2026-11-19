

#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_gear_core.h"
#include "../dm_splash_rule.h"
#include <assert.h>
#include <math.h>
#include <string.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static dm_gear_measures M(double group, double single, double survival, double hits, double recovery) {
    dm_gear_measures m = { group, single, survival, hits, recovery, -1 }; return m;
}
static dm_gear_score_out score(dm_gear_measures now, dm_gear_measures after, double hits_goal, double boss) {
    dm_gear_goal g = { hits_goal, boss }; dm_gear_score_out o; dm_gear_score(&now, &after, &g, &o); return o;
}

static dm_gear_score_out damage_change(double v) { return score(M(1000, 50, 60, 20, 0), M(1000 + 10 * v, 50, 60, 20, 0), 0, 0); }

static void score_tests(void) {
    dm_gear_score_out o = score(M(100, 3000, 60, 20, 1), M(110, 2700, 60, 20, 1), 15, 0.5);
    CHECK(fabs(o.change[DM_GEAR_M_GROUP] - 10) < 1e-12 && fabs(o.change[DM_GEAR_M_SINGLE] + 10) < 1e-12, "review example changes %g %g",
          o.change[DM_GEAR_M_GROUP], o.change[DM_GEAR_M_SINGLE]);
    CHECK(o.damage == 0 && o.dots_damage == 0 && o.dots_survival == 0 && o.goal == DM_GEAR_GOAL_NONE, "review example: damage %g dots %d", o.damage,
          o.dots_damage);

    o = score(M(100000, 2, 60, 20, 0), M(120000, 1.8, 60, 20, 0), 0, 0.3);
    CHECK(o.damage == 11 && o.dots_damage == 2, "magnitudes apart: only the percent changes count (damage %g dots %d)", o.damage, o.dots_damage);
    o = score(M(2, 100000, 60, 20, 0), M(1.8, 120000, 60, 20, 0), 0, 0.7);
    CHECK(o.damage == 11 && o.dots_damage == 2, "magnitudes apart, mirrored (damage %g dots %d)", o.damage, o.dots_damage);
    o = score(M(100, 100, 60, 20, 0), M(150, 50, 60, 20, 0), 0, 0);
    CHECK(o.damage == 50 && o.dots_damage == 3, "share 0: group only (%g)", o.damage);
    o = score(M(100, 100, 60, 20, 0), M(150, 50, 60, 20, 0), 0, 1);
    CHECK(o.damage == -50 && o.dots_damage == -3, "share 1: single only (%g)", o.damage);
    o = score(M(100, 100, 60, 20, 0), M(150, 50, 60, 20, 0), 0, 7);
    CHECK(o.damage == -50, "a share above 1 counts as 1 (%g)", o.damage);

    o = score(M(0, 0, 60, 20, 0), M(80, 40, 66, 22, 0), 0, 0.5);
    CHECK(!o.damage_base && o.damage == 0 && o.dots_damage == 0 && o.change[DM_GEAR_M_GROUP] == 0 && o.change[DM_GEAR_M_SINGLE] == 0,
          "no damage now: no damage value or dots (base %d damage %g dots %d)", o.damage_base, o.damage, o.dots_damage);
    CHECK(o.survival == 5 && o.dots_survival == 1 && dm_gear_dots_code(&o) == (5 << 3), "no damage now: no damage row, one green survival dot (%g %d)",
          o.survival, dm_gear_dots_code(&o));
    CHECK(o.key[1] == 10, "no damage now: the search follows the survival change (%g)", o.key[1]);
    o = score(M(0, 10, 60, 20, 0), M(0, 12, 60, 20, 0), 0, 0.5);
    CHECK(o.damage_base && o.damage == 10, "single target damage alone is a damage base (%g)", o.damage);

    static const struct { double change; int dots; } steps[] = {
        { 1.99, 0 }, { 2.0, 1 }, { 9.99, 1 }, { 10, 2 }, { 24.99, 2 }, { 25, 3 }, { 300, 3 },
        { -1.99, 0 }, { -2.0, -1 }, { -9.99, -1 }, { -10, -2 }, { -25, -3 } };
    for (size_t i = 0; i < sizeof steps / sizeof *steps; i++) {
        o = damage_change(steps[i].change);
        CHECK(o.dots_damage == steps[i].dots, "%g %%: %d dots, want %d", steps[i].change, o.dots_damage, steps[i].dots);
    }
    static const struct { double change, damage; } cuts[] = { { 0.49, 0 }, { 0.5, 0.5 }, { -0.49, 0 }, { -0.5, -0.5 }, { 1.74, 1.5 }, { -1.74, -1.5 } };
    for (size_t i = 0; i < sizeof cuts / sizeof *cuts; i++) {
        o = damage_change(cuts[i].change);
        CHECK(o.damage == cuts[i].damage && (cuts[i].damage != 0 || !signbit(o.damage)), "%g %% truncates to %g, want %g",
              cuts[i].change, o.damage, cuts[i].damage);
    }
    o = score(M(100, 100, 60, 20, 0), M(102, 102, 60, 20, 0), 0, 0.3);
    CHECK(o.damage == 2 && o.dots_damage == 1, "0.7 x 2 + 0.3 x 2 sits on the 2 %% step (%.17g)", o.damage);

    o = score(M(100, 100, 60, 16, 0), M(130, 130, 50, 14, 0), 15, 0.5);
    CHECK(o.goal == DM_GEAR_GOAL_MISSED && o.dots_survival == -3 && o.dots_damage == 3, "falls below the goal despite more damage: goal %d dots %d %d",
          o.goal, o.dots_damage, o.dots_survival);
    o = score(M(100, 100, 50, 14, 0), M(70, 70, 60, 16, 0), 15, 0.5);
    CHECK(o.goal == DM_GEAR_GOAL_REACHED && o.dots_survival == 3 && o.dots_damage == -3, "first reaches the goal despite less damage: goal %d dots %d %d",
          o.goal, o.dots_damage, o.dots_survival);
    o = score(M(100, 100, 50, 15, 0), M(100, 100, 50, 15, 0), 15, 0.5);
    CHECK(o.goal == DM_GEAR_GOAL_NONE && o.dots_survival == 0, "exactly on the goal counts as reached: goal %d", o.goal);
    o = score(M(100, 100, 50, 10, 0), M(60, 60, 60, 12, 0), 15, 0.5);
    CHECK(o.goal == DM_GEAR_GOAL_NONE && o.dots_survival == 2 && o.dots_damage == -3, "both below the goal: survival dots by the hits change +20 %% (%d)",
          o.dots_survival);
    o = score(M(100, 100, 50, 10, 0), M(160, 160, 40, 9.5, 0), 15, 0.5);
    CHECK(o.dots_survival == -1, "both below the goal, fewer hits: red by the hits change -5 %% (%d)", o.dots_survival);
    o = score(M(100, 100, 50, 16, 0), M(160, 160, 40, 15.5, 0), 0, 0.5);
    CHECK(o.goal == DM_GEAR_GOAL_NONE && o.dots_damage == 3 && o.survival == -10 && o.dots_survival == -2, "no goal (glass cannon): %d %d", o.dots_damage,
          o.dots_survival);

    o = score(M(100, 100, 60, 20, 3), M(100, 100, 60, 20, 3), 15, 0.5);
    CHECK(o.dots_damage == 0 && o.dots_survival == 0 && o.damage == 0 && o.key[2] == 0 && o.goal == DM_GEAR_GOAL_NONE,
          "unchanged measures (only utility values differ): a grey dot per row");
    CHECK(dm_gear_dots_code(&o) == (4 | 4 << 3), "unchanged: both rows grey (%d)", dm_gear_dots_code(&o));


    o = score(M(100, 1000, 60, 20, 20), M(100, 1045, 60, 20, 8.7), 15, 0.4);
    CHECK(o.damage == 1.5 && o.dots_damage == 0 && o.survival == -5.5 && o.dots_survival == -1 && dm_gear_dots_code(&o) == (4 | 3 << 3),
          "amulet: damage %g (%d), survival %g (%d)", o.damage, o.dots_damage, o.survival, o.dots_survival);
    o = score(M(100, 100, 60, 20, 5), M(100, 100, 61.8, 20, 7.2), 0, 0.5);
    CHECK(o.survival == 2.5 && o.dots_survival == 1, "survival +3 %% and recovery +2.2 points: 2.6 truncates to 2.5, one green dot (%g)", o.survival);
    o = score(M(100, 100, 60, 20, 5), M(100, 100, 60, 20, 8.9), 0, 0.5);
    CHECK(o.survival == 1.5 && o.dots_survival == 0, "recovery +3.9 points alone: 1.95 truncates to 1.5, grey (%g)", o.survival);
    o = score(M(100, 100, 60, 20, 0), M(100, 100, 90, 30, 0), 0, 0.5);
    CHECK(o.survival == 25 && o.dots_survival == 3, "survival +50 %%: 25, three green dots (%g)", o.survival);
    o = score(M(100, 100, 60, 20, 30), M(100, 100, 60, 20, 10), 0, 0.5);
    CHECK(o.survival == -10 && o.dots_survival == -2, "recovery -20 points: -10, two red dots (%g)", o.survival);
    o = score(M(100, 100, 60, 20, 4.5), M(100, 100, 60, 20, 1.5), 0, 0.5);
    CHECK(o.change[DM_GEAR_M_RECOVERY] == -3, "recovery changes in percentage points (%g)", o.change[DM_GEAR_M_RECOVERY]);

    o = score(M(100, 100, 60, 20, 0), M(110, 100, 60, 30, 2), 25, 0.5);
    CHECK(o.key[0] == 25 && o.key[1] == 5 && fabs(o.key[2] - (10 + 0 + 0 + 2) / 4.0) < 1e-12, "search key %g %g %g", o.key[0], o.key[1], o.key[2]);
    o = score(M(100, 100, 60, 20, 0), M(110, 100, 60, 18, 0), 25, 0.5);
    CHECK(o.key[0] == 18, "search key below the goal: the hits (%g)", o.key[0]);
    o = score(M(100, 100, 60, 20, 0), M(110, 100, 60, 18, 0), 0, 0.5);
    CHECK(o.key[0] == 0, "search key without a goal (%g)", o.key[0]);
}

static char *profile_file(const char *name) {
    char path[256]; snprintf(path, sizeof path, "sim_profiles/%s", name);
    size_t n; char *text = sim_read(path, &n); assert(text);
    return text;
}
static dm_sim_char parse(const dm_sim_data *d, const char *text) {
    dm_sim_char c; char err[128];
    if (dm_sim_profile_parse(d, text, &c, err, sizeof err)) { fprintf(stderr, "profile: %s\n", err); exit(1); }
    return c;
}

static const char *SORC = "class=1\nlevel=80\ndiff=2\nact=1\nlife=1200\nmana=600\nstr=60\ndex=60\nar=500\ndef=800\nfcr=63\n"
                          "weapon=1,2,100,0,0,1,0,0,1HS\nres=0,75,75,75,75,0\nskill=59,20,20\nbar=59\n";


static const char *PAL = "class=3\nlevel=80\ndiff=2\nact=2\nlife=1500\nmana=300\nstr=150\ndex=120\nar=4000\ndef=1600\n"
                         "weapon=100,160,100,0,0,1,0,0,1HS\nres=15,75,75,75,75,0\ned=230\naura=98\nactive_level=98,20\nskill=98,20,20\nbar=98\n";




static const char *WARLOCK = "class=7\nlevel=17\ndiff=0\nact=1\nlife=260\nmana=150\nstr=35\ndex=30\nar=200\ndef=120\n"
                             "weapon=8,15,100,0,0,1,0,0,1HS\nskill=394,10,10\nbar=394\nskill=395,3,3\nbar=395\n";
enum { MAIN_SKILL = 394, NEW_PASSIVE = 378, NEW_SKILL = 375 };

static const char *PAL_BOW = "class=3\nlevel=80\ndiff=2\nact=2\nlife=1500\nmana=300\nstr=150\ndex=120\nar=4000\ndef=1600\n"
                             "weapon=100,160,100,0,0,2,0,0,BOW\nres=15,75,75,75,75,0\n";
static const char *ASSA = "class=6\nlevel=80\ndiff=2\nact=2\nlife=1500\nmana=300\nstr=150\ndex=120\nar=4000\ndef=1600\n"
                          "weapon=100,160,100,0,0,1,0,0,HT1\nres=15,75,75,75,75,0\nskill=280,20,20\nbar=280\n";


static void add_anim(dm_sim_data *d, const char *token, const char *mode, const char *wclass, int frames, int speed) {
    d->anim = realloc(d->anim, (size_t)(d->nanim + 1) * sizeof *d->anim); assert(d->anim);
    dm_sim_anim *a = &d->anim[d->nanim++]; memset(a, 0, sizeof *a);
    snprintf(a->token, sizeof a->token, "%s", token); snprintf(a->mode, sizeof a->mode, "%s", mode); snprintf(a->wclass, sizeof a->wclass, "%s", wclass);
    a->frames = (int16_t)frames; a->speed = (int16_t)speed;
}
static void frames_tests(dm_sim_data *d) {


    static const int pts[][6] = { { 7, 256, 50, 0, 13, 4 }, { 7, 256, 50, 4, 13, 0 }, { 7, 256, 50, 5, 12, 0 }, { 7, 256, 50, 10, 11, 0 }, { 7, 256, 50, 377, 4, 0 },
                                  { 3, 88, 50, 3, 17, 0 }, { 3, 88, 50, 4, 16, 0 }, { 3, 88, 50, 480, 5, 0 }, { 3, 256, 100, 85, 2, 0 }, { 3, 256, 100, 86, 1, 0 },
                                  { 3, 256, 50, 12, 5, 0 }, { 3, 256, 50, 13, 4, 0 }, { 3, 256, 50, 600, 1, 0 } };
    for (size_t i = 0; i < sizeof pts / sizeof *pts; i++)
        CHECK(dm_sim_frames_recover(pts[i][0], pts[i][1], pts[i][3], pts[i][2]) == pts[i][4], "fpd %d speed %d base %d at %d: %d frames, want %d", pts[i][0], pts[i][1],
              pts[i][2], pts[i][3], dm_sim_frames_recover(pts[i][0], pts[i][1], pts[i][3], pts[i][2]), pts[i][4]);
    dm_gear_goal goal = { 0, 0.5 }; dm_gear_tip tip;


    char text[512]; snprintf(text, sizeof text, "%s", SORC); char *at = strstr(text, "skill=59,20,20\nbar=59\n"); assert(at); strcpy(at, "skill=47,20,20\nbar=47\n");
    dm_sim_char sorc = parse(d, text), fast = sorc;
    fast.fcr = 200;
    CHECK(!dm_gear_tip_rate(d, &sorc, &fast, &goal, 0, &tip) && tip.main.ok && tip.main.cast && tip.main.skill == 47 && tip.main.before == 9 && tip.main.after == 7 && !tip.main.next,
          "63 > 200 cast rate: %d > %d frames", tip.main.before, tip.main.after);
    dm_sim_char top = fast; top.fcr = 220;
    CHECK(!dm_gear_tip_rate(d, &fast, &top, &goal, 0, &tip) && tip.main.ok && tip.main.before == tip.main.after && tip.main.next == 0, "200 > 220: the same frames, no next step");
    dm_sim_char step = sorc; step.fcr = 64;
    CHECK(!dm_gear_tip_rate(d, &sorc, &step, &goal, 0, &tip) && tip.main.ok && tip.main.before == tip.main.after && tip.main.next == 105,
          "63 > 64: the same frames, next at %d", tip.main.next);
    if (tip.main.next > 0) { dm_sim_char at = sorc; at.fcr = tip.main.next; dm_gear_tip t2; dm_sim_char below = sorc; below.fcr = tip.main.next - 1;
        CHECK(!dm_gear_tip_rate(d, &sorc, &at, &goal, 0, &t2) && t2.main.after < t2.main.before && !dm_gear_tip_rate(d, &sorc, &below, &goal, 0, &t2) && t2.main.after == t2.main.before,
              "the next step is the lowest speed with fewer frames"); }

    dm_sim_char pal = parse(d, PAL); pal.hand[0].start_known = 1; pal.hand[0].start_frame = 0;
    dm_sim_char pal_ias = pal, pal_fcr = pal;
    pal_ias.ias = 60; pal_fcr.fcr = 60;
    CHECK(!dm_gear_tip_rate(d, &pal, &pal_ias, &goal, 0, &tip) && tip.main.ok && !tip.main.cast && tip.main.skill == 0 && tip.main.after < tip.main.before,
          "attack speed on the normal attack: %d > %d", tip.main.before, tip.main.after);
    CHECK(!dm_gear_tip_rate(d, &pal, &pal_fcr, &goal, 0, &tip) && !tip.main.cast && tip.main.after == tip.main.before && !tip.main.next, "cast rate does not move an attack");

    dm_sim_char wl = parse(d, WARLOCK), wl_after = wl; wl_after.lvl[395] = 60; wl_after.lvl[MAIN_SKILL] = 1;
    CHECK(!dm_gear_tip_rate(d, &wl, &wl_after, &goal, 0, &tip) && tip.now.skill == MAIN_SKILL && tip.after.skill != MAIN_SKILL && tip.main.skill == MAIN_SKILL, "the skill now (%d) is compared, best after %d",
          tip.now.skill, tip.after.skill);

    dm_sim_char odd = pal; snprintf(odd.hand[0].wclass, sizeof odd.hand[0].wclass, "ZZZ");
    CHECK(!dm_gear_tip_rate(d, &odd, &odd, &goal, 0, &tip) && !tip.recover.ok && !tip.block.ok, "no animation in the tables: no frames");
    add_anim(d, d->cls[pal.cls].token, "GH", pal.hand[0].wclass, 9, 256); add_anim(d, d->cls[pal.cls].token, "BL", pal.hand[0].wclass, 3, 256);
    dm_sim_char shield = pal, shield_after = pal; shield.block = shield_after.block = 40; shield_after.fbr = 86; shield_after.fhr = 3;
    CHECK(!dm_gear_tip_rate(d, &shield, &shield_after, &goal, 0, &tip) && tip.block.ok && tip.block.before == 5 && tip.block.after == 2, "block 0 > 86: %d > %d",
          tip.block.before, tip.block.after);
    CHECK(tip.recover.ok && tip.recover.before == tip.recover.after && tip.recover.next > 3, "hit recovery 0 > 3: the same frames, next at %d", tip.recover.next);
    int hs = -1; for (int id = 0; id < DM_SIM_MAXSKILL && hs < 0; id++) if (d->skill[id] && !strcmp(d->skill[id]->aurastate, "holyshield")) hs = id;
    CHECK(hs >= 0, "Holy Shield in the tables");
    if (hs >= 0) { dm_sim_char holy = shield, holy_after = shield_after; holy.lvl[hs] = holy_after.lvl[hs] = holy.active_lvl[hs] = holy_after.active_lvl[hs] = 10;
        CHECK(!dm_gear_tip_rate(d, &holy, &holy_after, &goal, 0, &tip) && tip.block.ok && tip.block.before == 2 && tip.block.after == 1, "with Holy Shield: %d > %d",
              tip.block.before, tip.block.after); }

    int smite = -1; for (int id = 0; id < DM_SIM_MAXSKILL && smite < 0; id++) if (d->skill[id] && !strcmp(d->skill[id]->name, "Smite")) smite = id;
    CHECK(smite >= 0, "Smite in the tables");
    if (smite >= 0) { dm_sim_char sm = shield; sm.shield = 1; sm.shield_min = 40; sm.shield_max = 80; sm.lvl[smite] = sm.blvl[smite] = 20; sm.bar[0] = smite; sm.nbar = 1; dm_sim_char sm_after = sm; sm_after.shield = 0; sm_after.block = 0; sm_after.ias = sm.ias - 15;
        CHECK(!dm_gear_tip_rate(d, &sm, &sm_after, &goal, 0, &tip) && tip.now.skill == smite && !tip.main.ok, "Smite without its shield after: no frames (%d: %d > %d, ok %d)",
              tip.now.skill, tip.main.before, tip.main.after, tip.main.ok); }
    dm_sim_char bare = shield_after; bare.block = 0;
    CHECK(!dm_gear_tip_rate(d, &shield, &bare, &goal, 0, &tip) && !tip.block.ok, "nothing to block with after: no block frames");
}

static void new_skill_tests(const dm_sim_data *d) {
    assert(d->skill[NEW_PASSIVE] && d->skill[NEW_PASSIVE]->passive && d->skill[NEW_PASSIVE]->cls == 7 && d->skill[NEW_SKILL] && d->skill[NEW_SKILL]->cls == 7);
    dm_sim_char now = parse(d, WARLOCK), after = now;
    assert(now.nbar + 2 <= DM_SIM_MAXBAR);
    after.lvl[MAIN_SKILL] += 2; after.lvl[NEW_PASSIVE] = 1; after.lvl[NEW_SKILL] = 1; after.bar[after.nbar++] = NEW_PASSIVE; after.bar[after.nbar++] = NEW_SKILL;
    after.ll = 5; after.hand[0].min = 3; after.hand[0].max = 10; after.hand[0].elmin[DM_EL_LTNG] = 1; after.hand[0].elmax[DM_EL_LTNG] = 6;
    int th[DM_SIM_MAXSKILL], dmg[DM_SIM_MAXSKILL];
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) th[id] = dmg[id] = id == 0 || id == MAIN_SKILL || id == 395 ? 0 : DM_SIM_NO_NODE;
    dm_sim_passive_shift shift;
    int passive_unknown = dm_sim_passive_levels(d, &now, &after, th, dmg, 0, &shift) != 0;
    CHECK(!passive_unknown, "the passive the swap gives from level 0 to 1 follows its passive formula");
    unsigned char has_node[DM_SIM_MAXSKILL] = { 0 }, known[DM_SIM_MAXSKILL] = { 0 };
    has_node[MAIN_SKILL] = has_node[395] = 1;
    CHECK(!dm_sim_swap_skills_ok(d, &now, &after, has_node, known, passive_unknown), "a new skill whose level after is not known leaves the swap unrated");
    known[NEW_PASSIVE] = known[NEW_SKILL] = 1;
    CHECK(dm_sim_swap_skills_ok(d, &now, &after, has_node, known, passive_unknown), "skills from 0 to 1 by single skill bonuses alone are modelled");
    dm_gear_goal goal = { 15, 0.5 }; dm_gear_tip tip;
    int rc = dm_gear_tip_rate(d, &now, &after, &goal, 0, &tip);
    CHECK(!rc && tip.score.change[DM_GEAR_M_GROUP] > 2 && tip.score.damage_base && tip.score.dots_damage > 0, "+2 to the main skill on another weapon class is rated: rc %d group %+.1f %% damage dots %d",
          rc, tip.score.change[DM_GEAR_M_GROUP], tip.score.dots_damage);
}



static double want_reach(const dm_sim_char *c, const dm_sim_weapon *w) {
    dm_splash_area a = dm_splash_area_of(dm_splash_group_of(w->stype, w->twohand), c->cls, c->str, c->dex, w->srange);
    if (!a.share) return 1;
    double n = a.width / 45.0 + a.depth / 2.0; if (n > DM_GEAR_PACK_EXTRA) n = DM_GEAR_PACK_EXTRA;
    return 1 + n * 0.75 * a.share / 100.0;
}
static void set_type(dm_sim_weapon *w, const char *type, int two_hands) { snprintf(w->stype, sizeof w->stype, "%s", type); w->twohand = two_hands; w->srange = 1; }

static dm_sim_skill_out pack_out(const dm_sim_data *d, const dm_sim_char *c, int id) {
    dm_sim_target pack; assert(dm_sim_target_act(d, c, &pack) >= 1); pack.extra_targets = DM_GEAR_PACK_EXTRA;
    dm_sim_skill_out o; assert(!dm_sim_skills_prepared(d, c, &pack, 1, &id, &o));
    return o;
}



static void splash_tests(const dm_sim_data *d, const dm_sim_char *sorc) {
    dm_gear_measures s0, s1;
    assert(!dm_gear_measure(d, sorc, 0, &s0) && !dm_gear_measure(d, sorc, 1, &s1));
    CHECK(s1.group == s0.group, "a sorceress's spells rate without the splash rule (%g vs %g)", s1.group, s0.group);
    CHECK(dm_splash_skill_ok(d->skill[0]) && !dm_splash_skill_ok(d->skill[59]) && !dm_splash_skill_ok(d->skill[98]), "the attack is splash eligible, its spell and its aura are not");

    CHECK(!dm_splash_skill_ok(d->skill[97]), "Smite does not splash");
    CHECK(!dm_splash_skill_ok(d->skill[255]) && !dm_splash_skill_ok(d->skill[270]), "the Dragon kicks do not splash");
    CHECK(dm_splash_skill_ok(d->skill[260]), "Dragon Claw, a claw strike, splashes");
    int whirl = 0; for (int id = 1; id < DM_SIM_MAXSKILL; id++) if (d->skill[id] && d->skill[id]->srvdofunc == 76) whirl = id;
    CHECK(whirl && !dm_splash_skill_ok(d->skill[whirl]), "Whirlwind (skill %d) does not splash", whirl);


    static const struct { const char *type; int two; } w[] = { { "axe", 1 }, { "swor", 0 }, { "knif", 0 }, { "spea", 1 } };
    double reach[4];
    for (int k = 0; k < 4; k++) {
        dm_sim_char pal = parse(d, PAL); pal.mana = 100000;
        set_type(&pal.hand[0], w[k].type, w[k].two);
        dm_gear_measures q0, q1; assert(!dm_gear_measure(d, &pal, 0, &q0) && !dm_gear_measure(d, &pal, 1, &q1));
        dm_sim_skill_out po = pack_out(d, &pal, 0);
        double ph = fmax(0.0, po.dps - po.ow_dps), want = reach[k] = want_reach(&pal, &pal.hand[0]);
        CHECK(!(po.flags & (DM_SF_UNAVAIL | DM_SF_MODIFIER | DM_SF_UTILITY | DM_SF_NOWEAPON)) && po.dps > 0, "%s: the normal attack rates as damage", w[k].type);
        CHECK(fabs(po.splash - want) <= 1e-12, "%s: the attack's reach %g, the rule's %g", w[k].type, po.splash, want);
        CHECK(fabs(q0.group - po.dps) <= 1e-9 * q0.group, "%s, rule off: the leaf is dps x 1 (%g vs %g)", w[k].type, q0.group, po.dps);
        double on = want > 1 ? ph * want + po.ow_dps * DM_GEAR_PACK_UNITS : po.dps;
        CHECK(fabs(q1.group - on) <= 1e-9 * on, "%s, rule on: group %g, reach %g says %g", w[k].type, q1.group, want, on);
        CHECK(q0.single == q1.single && q0.survival_s == q1.survival_s, "%s: the splash rule only moves the group measure", w[k].type);
    }


    CHECK(fabs(reach[0] - 1.8125) < 1e-12 && fabs(reach[1] - 1.275) < 1e-12 && reach[2] == 1 && fabs(reach[3] - (1 + (50 / 45.0 + 0.5) * 0.15)) < 1e-12,
          "the paladin's reaches: axe %g sword %g dagger %g spear %g", reach[0], reach[1], reach[2], reach[3]);

    {
        dm_sim_char pal = parse(d, PAL); pal.mana = 100000; set_type(&pal.hand[0], "swor", 0); pal.str = 180;
        dm_sim_skill_out po = pack_out(d, &pal, 0);
        CHECK(fabs(po.splash - 1.34375) < 1e-12, "strength 180: the sword's reach %g, not 1.34375", po.splash);
        dm_sim_char sp = parse(d, PAL); sp.mana = 100000; set_type(&sp.hand[0], "spea", 1); sp.dex = 0;
        po = pack_out(d, &sp, 0);
        CHECK(fabs(po.splash - (1 + (20 / 45.0 + 0.5) * 0.15)) < 1e-12, "dexterity 0: the spear's narrow cone (reach %g)", po.splash);
    }

    {
        dm_sim_char pal = parse(d, PAL), bow = parse(d, PAL_BOW); bow.mana = 100000; set_type(&bow.hand[0], "bow", 0);
        dm_gear_measures b0, b1;
        assert(!dm_gear_measure(d, &bow, 0, &b0) && !dm_gear_measure(d, &bow, 1, &b1));
        CHECK(b0.group > 0 && b1.group == b0.group, "a bow's normal attack never splashes (%g vs %g)", b1.group, b0.group);
        CHECK(!dm_splash_char_ok(&bow, d->skill[0]) && dm_splash_char_ok(&pal, d->skill[0]), "the row rule takes every weapon, the character rule the one in hand");
        dm_sim_char wolf = bow; wolf.form = 1;
        CHECK(dm_splash_char_ok(&wolf, d->skill[0]), "a werewolf with a bow in hand attacks in melee and splashes");
    }


    {
        char *text = profile_file("class4-level80.prof"); dm_sim_char barb = parse(d, text); free(text);
        barb.act = 3; barb.mana = 100000;
        assert(barb.nhands == 2);
        int right = -1;
        for (int id = 1; id < DM_SIM_MAXSKILL && right < 0; id++) {
            const dm_sim_skill *s = d->skill[id];
            if (s && s->cls == 4 && s->weapsel == 0 && (s->fam & (DM_FAM_W1 | DM_FAM_W2)) && dm_splash_skill_ok(s) && !s->area) right = id;
        }
        assert(right > 0);
        barb.lvl[right] = 20; barb.nbar = 1; barb.bar[0] = right;
        set_type(&barb.hand[0], "axe", 0); set_type(&barb.hand[1], "knif", 0);
        double axe = want_reach(&barb, &barb.hand[0]);
        dm_sim_skill_out r = pack_out(d, &barb, right), n = pack_out(d, &barb, 0);
        CHECK(fabs(axe - (1 + (65 / 45.0 + 0.5) * 0.3)) < 1e-12 && fabs(r.splash - axe) < 1e-12, "skill %d with the axe right: reach %g, the axe's %g", right, r.splash, axe);
        CHECK(n.splash > 1 && n.splash < axe, "the normal attack with axe and dagger: reach %g between 1 and %g", n.splash, axe);
        set_type(&barb.hand[0], "knif", 0); set_type(&barb.hand[1], "axe", 0);
        r = pack_out(d, &barb, right);
        CHECK(r.splash == 1, "skill %d with the dagger right: the axe's reach leaks in (%g)", right, r.splash);
        dm_gear_measures m0, m1;
        assert(!dm_gear_measure(d, &barb, 0, &m0) && !dm_gear_measure(d, &barb, 1, &m1));
        dm_sim_skill_out a = pack_out(d, &barb, 0);
        double plain = fmax(r.dps, a.dps), split = fmax(r.dps, fmax(0.0, a.dps - a.ow_dps) * a.splash + a.ow_dps * DM_GEAR_PACK_UNITS);
        CHECK(fabs(m0.group - plain) <= 1e-9 * plain && fabs(m1.group - split) <= 1e-9 * split,
              "dagger right, axe left: group off %g (want %g), on %g (want %g)", m0.group, plain, m1.group, split);


        set_type(&barb.hand[0], "axe", 0); set_type(&barb.hand[1], "knif", 0);
        dm_sim_skill_out clean = pack_out(d, &barb, 0);
        barb.hand[1].elmin[DM_EL_POIS] = barb.hand[1].elmax[DM_EL_POIS] = 3000; barb.poison_frames = 75;
        dm_sim_skill_out pz = pack_out(d, &barb, 0);
        double pois = pz.dps_element[DM_EL_POIS];
        CHECK(pois > 0 && fabs(pz.splash - clean.splash) < 1e-12 && pz.splash_pois == 1,
              "axe and poisoned dagger: hit reach %g (without the poison %g), poison reach %g, poison %g per second", pz.splash, clean.splash, pz.splash_pois, pois);
        barb.nbar = 0;
        dm_gear_measures mp;
        assert(!dm_gear_measure(d, &barb, 1, &mp));
        double want = fmax(0.0, pz.dps - pz.ow_dps - pois) * pz.splash + pois + pz.ow_dps * DM_GEAR_PACK_UNITS;
        CHECK(fabs(mp.group - want) <= 1e-9 * want, "axe and poisoned dagger: group %g, want %g (the poison at the dagger's reach)", mp.group, want);
    }

    if (d->skill[280]) {
        dm_sim_char bar = parse(d, ASSA); bar.mana = 100000; set_type(&bar.hand[0], "h2h", 0);
        dm_sim_target bpack; assert(dm_sim_target_act(d, &bar, &bpack) >= 1); bpack.extra_targets = DM_GEAR_PACK_EXTRA;
        int bids[] = { 0, 280 }; dm_sim_skill_out bo[2]; assert(!dm_sim_skills_prepared(d, &bar, &bpack, 2, bids, bo));
        CHECK(dm_splash_skill_ok(d->skill[280]) && d->skill[280]->area && !(bo[1].flags & (DM_SF_UNAVAIL | DM_SF_MODIFIER | DM_SF_UTILITY | DM_SF_NOWEAPON)) && bo[1].dps > 0,
              "Phoenix Strike rates as area melee damage (flags %#x)", bo[1].flags);
        double want = 0, r = want_reach(&bar, &bar.hand[0]);
        CHECK(r > 1, "claws in an assassin's hand splash (reach %g)", r);
        for (int j = 0; j < 2; j++) {
            if (bo[j].flags & (DM_SF_UNAVAIL | DM_SF_MODIFIER | DM_SF_UTILITY | DM_SF_NOWEAPON)) continue;
            CHECK(fabs(bo[j].splash - r) < 1e-12, "skill %d: reach %g, the claw's %g", bids[j], bo[j].splash, r);
            double H = j ? DM_GEAR_AREA_HITS : 1, v = fmax(0.0, bo[j].dps - bo[j].ow_dps) * H * r + bo[j].ow_dps * DM_GEAR_PACK_UNITS;
            if (v > want) want = v;
        }

        if (d->skill[255]) {
            dm_sim_char k = bar;
            k.hand[0].elmin[DM_EL_POIS] = k.hand[0].elmax[DM_EL_POIS] = 3000; k.poison_frames = 75;
            k.lvl[255] = 20; k.nbar = 0; k.bar[k.nbar++] = 280; k.bar[k.nbar++] = 255;
            dm_sim_skill_out ko = pack_out(d, &k, 280);
            CHECK(ko.dps_element[DM_EL_POIS] > 0 && ko.splash < r && fabs(ko.splash_pois - r) < 1e-12,
                  "poisoned claws, kick finisher: hit reach %g below the claw's %g, poison reach %g (want the claw's)", ko.splash, r, ko.splash_pois);
        }
        dm_gear_measures bm; assert(!dm_gear_measure(d, &bar, 1, &bm));
        CHECK(fabs(bm.group - want) <= 1e-9 * want, "Phoenix Strike: group %g, the H x reach model says %g", bm.group, want);
    }
}

static void table_tests(dm_sim_data *d) {
    new_skill_tests(d);
    static const char *files[] = { "class0-level80.prof", "class1-level80.prof", "class2-level80.prof", "class3-level80.prof", "class4-level80.prof",
                                   "class5-level80.prof", "class6-level80.prof", "class7-level12.prof", "class7-level80.prof" };
    for (size_t i = 0; i < sizeof files / sizeof *files; i++) {
        char *text = profile_file(files[i]); dm_sim_char c = parse(d, text); free(text);
        for (int act = 1; act <= 5; act++) {
            c.act = act; dm_gear_measures m;
            int rc = dm_gear_measure(d, &c, 0, &m);
            CHECK(!rc && m.group > 0 && m.single > 0 && m.survival_s > 0 && m.survival_s <= 600 && m.hits > 0 && m.recovery >= 0 && isfinite(m.group) &&
                  isfinite(m.single), "%s act %d: rc %d group %g single %g survival %g hits %g recovery %g", files[i], act, rc, m.group, m.single,
                  m.survival_s, m.hits, m.recovery);
        }
        c.act = 0; dm_gear_measures m;
        CHECK(dm_gear_measure(d, &c, 0, &m) == -1, "%s without an act: no measures", files[i]);
    }

    static const struct { int skill, area; } areas[] = { { 0, 0 }, { 36, 0 }, { 47, 1 }, { 48, 1 }, { 53, 1 }, { 12, 1 }, { 102, 1 }, { 59, 1 }, { 70, 0 },
                                                          { 45, 0 }, { 106, 0 }, { 402, 1 }, { 154, 1 }, { 391, 1 } };
    for (size_t i = 0; i < sizeof areas / sizeof *areas; i++)
        CHECK(d->skill[areas[i].skill]->area == areas[i].area, "skill %d area %d", areas[i].skill, d->skill[areas[i].skill]->area);

    dm_sim_char sorc = parse(d, SORC); dm_gear_measures m3, m1;
    assert(!dm_gear_measure(d, &sorc, 0, &m3));
    dm_sim_target pack; assert(dm_sim_target_act(d, &sorc, &pack) == 1); pack.extra_targets = DM_GEAR_PACK_EXTRA;
    dm_sim_skill_out o; int ids[] = { 59 }; assert(!dm_sim_skills_prepared(d, &sorc, &pack, 1, ids, &o));
    double lasts = o.mana_seconds < 0 ? 1 : fmin(1, o.mana_seconds / 10);
    CHECK(fabs(m3.group - 3 * o.dps * lasts) <= 1e-9 * m3.group, "area skill: three units of the pack (%g vs %g x 3 x %g)", m3.group, o.dps, lasts);
    d->skill[59]->area = 0; assert(!dm_gear_measure(d, &sorc, 0, &m1)); d->skill[59]->area = 1;
    CHECK(fabs(m3.group - 3 * m1.group) <= 1e-9 * m3.group, "the same skill as single target: one unit (%g vs %g)", m3.group, m1.group);
    CHECK(m3.single == m1.single && m3.survival_s == m1.survival_s, "the area rule only moves the group measure");

    dm_sim_char poor = sorc; poor.mana = 120; dm_gear_measures mp;
    assert(!dm_gear_measure(d, &poor, 0, &mp)); assert(!dm_sim_skills_prepared(d, &poor, &pack, 1, ids, &o));
    CHECK(o.mana_seconds >= 0 && o.mana_seconds < 10 && fabs(mp.group - 3 * o.dps * o.mana_seconds / 10) <= 1e-9 * m3.group && mp.group < m3.group,
          "mana for %g s of 10: group %g from dps %g", o.mana_seconds, mp.group, o.dps);

    splash_tests(d, &sorc);

    dm_sim_target lone; assert(dm_sim_target_act(d, &sorc, &lone) == 1); lone.regen = 0;
    dm_sim_dealt_out dealt, longer;
    lone.hp = 1000000000;
    assert(!dm_sim_dealt(d, &sorc, &lone, DM_GEAR_SINGLE_FRAMES, &dealt) && !dm_sim_dealt(d, &sorc, &lone, 2 * DM_GEAR_SINGLE_FRAMES, &longer));
    CHECK(!dealt.killed && dealt.frames == DM_GEAR_SINGLE_FRAMES && dealt.life > 0 && longer.life > dealt.life,
          "a target that outlives the window: %g life in %d frames, %g in twice the window", dealt.life, dealt.frames, longer.life);
    lone.hp = 200;
    assert(!dm_sim_dealt(d, &sorc, &lone, DM_GEAR_SINGLE_FRAMES, &dealt));
    CHECK(dealt.killed && dealt.frames > 0 && dealt.frames < DM_GEAR_SINGLE_FRAMES && dealt.life == 200, "a target that dies: %g life by frame %d", dealt.life,
          dealt.frames);
    CHECK(dm_sim_dealt(d, &sorc, &lone, 0, &dealt) == -1 && dm_sim_dealt(d, &sorc, &lone, DM_SIM_HORIZON + 1, &dealt) == -1, "window outside 1..horizon");

    dm_sim_char tank = sorc; tank.life = 1000000; dm_gear_measures mt;
    assert(!dm_gear_measure(d, &tank, 0, &mt));
    CHECK(mt.survival_s == 600 && mt.hits > m3.hits, "outlives the horizon: 600 s and the hits landed in it (%g s, %g hits)", mt.survival_s, mt.hits);
    CHECK(m3.recovery == 0, "no regeneration and no leech: recovery 0 (%g)", m3.recovery);
    dm_sim_char regen = sorc; regen.liferegen = 256 * 20; dm_gear_measures mr;
    assert(!dm_gear_measure(d, &regen, 0, &mr));
    CHECK(mr.recovery > 0 && mr.survival_s > m3.survival_s, "regeneration: recovery %g, survival %g", mr.recovery, mr.survival_s);


    int sawtooth = 0, slower = 0; double hits0 = -1, last_s = -1;
    for (int def = 0; def <= 4000; def += 25) {
        dm_sim_char x = sorc; x.def = def; dm_gear_measures mx;
        assert(!dm_gear_measure(d, &x, 0, &mx));
        if (hits0 < 0) hits0 = mx.hits;
        sawtooth |= fabs(mx.hits - hits0) > 1e-9 * hits0;
        slower |= mx.survival_s < last_s;
        last_s = mx.survival_s;
    }
    CHECK(!sawtooth && !slower && hits0 > 0, "defense moves the time until death, not the landed hits until death (%g hits at defense 0)", hits0);
    dm_sim_char tank2 = tank; tank2.def = 0; dm_gear_measures mt2;
    assert(!dm_gear_measure(d, &tank2, 0, &mt2));
    CHECK(mt2.survival_s == 600 && mt2.hits == mt.hits, "a survivor of the horizon: the same hits whatever its defense (%g, %g)", mt.hits, mt2.hits);

    dm_sim_char after = sorc; after.lvl[59] = 15; after.life = 900; dm_gear_measures ma; dm_gear_reasons_out why;
    assert(!dm_gear_measure(d, &after, 0, &ma));
    int runs = dm_gear_reasons(d, &sorc, &after, &m3, &ma, 0, &why);
    CHECK(runs == 2, "two value groups differ: %d extra measurements", runs);
    CHECK(why.group[DM_GEAR_M_GROUP] == DM_GEAR_R_SKILLS && why.group[DM_GEAR_M_SINGLE] == DM_GEAR_R_SKILLS && why.share[DM_GEAR_M_GROUP] < 0,
          "damage falls through the skill level: %d %d", why.group[DM_GEAR_M_GROUP], why.group[DM_GEAR_M_SINGLE]);
    CHECK(why.group[DM_GEAR_M_SURVIVAL] == DM_GEAR_R_LIFE && why.share[DM_GEAR_M_SURVIVAL] < 0, "survival falls through life: %d",
          why.group[DM_GEAR_M_SURVIVAL]);
    CHECK(why.group[DM_GEAR_M_RECOVERY] == -1, "recovery unchanged: no reason");
    CHECK(dm_gear_reasons(d, &sorc, &sorc, &m3, &m3, 0, &why) == 0 && why.group[DM_GEAR_M_GROUP] == -1, "the same character: nothing to undo");



    dm_gear_goal goal = { 15, 0.5 }; dm_gear_tip tip;
    CHECK(!dm_gear_tip_rate(d, &sorc, &after, &goal, 0, &tip) && tip.act == 1 && tip.score.change[DM_GEAR_M_GROUP] < 0 && tip.reasons.group[DM_GEAR_M_SURVIVAL] == DM_GEAR_R_LIFE
          && tip.dir[DM_GEAR_M_SURVIVAL] == -1 && tip.dir[DM_GEAR_M_GROUP] == -1 && tip.goal_hits == 15, "rated swap: less life, lower skill (%g %d %d)",
          tip.score.change[DM_GEAR_M_GROUP], tip.dir[DM_GEAR_M_SURVIVAL], tip.dir[DM_GEAR_M_GROUP]);
    CHECK(!dm_gear_tip_rate(d, &after, &sorc, &goal, 0, &tip) && tip.dir[DM_GEAR_M_SURVIVAL] == 1 && tip.score.change[DM_GEAR_M_SURVIVAL] > 0, "the reverse swap: more life");


    dm_sim_char pool = sorc; pool.mana = 120; dm_sim_char pool_after = pool; pool_after.mana = 100; pool_after.manaregen_pct = 100;
    CHECK(!dm_gear_tip_rate(d, &pool, &pool_after, &goal, 0, &tip) && tip.score.change[DM_GEAR_M_GROUP] < -2 &&
          tip.reasons.group[DM_GEAR_M_GROUP] == DM_GEAR_R_MANA && tip.dir[DM_GEAR_M_GROUP] == -1, "less mana, more regeneration: group %+.1f %% through %d, way %d",
          tip.score.change[DM_GEAR_M_GROUP], tip.reasons.group[DM_GEAR_M_GROUP], tip.dir[DM_GEAR_M_GROUP]);
    dm_sim_char pool_regen = pool; pool_regen.mana = 110; pool_regen.manaregen_pct = 300;
    CHECK(!dm_gear_tip_rate(d, &pool, &pool_regen, &goal, 0, &tip) && tip.score.change[DM_GEAR_M_GROUP] > 2 &&
          tip.reasons.group[DM_GEAR_M_GROUP] == DM_GEAR_R_MANA && tip.dir[DM_GEAR_M_GROUP] == 1, "a little less mana, much more regeneration: group %+.1f %% through %d, way %d",
          tip.score.change[DM_GEAR_M_GROUP], tip.reasons.group[DM_GEAR_M_GROUP], tip.dir[DM_GEAR_M_GROUP]);
    dm_sim_char pool_up = pool; pool_up.mana = 300;
    CHECK(!dm_gear_tip_rate(d, &pool, &pool_up, &goal, 0, &tip) && tip.dir[DM_GEAR_M_GROUP] == 1, "one way only: more mana (%d)", tip.dir[DM_GEAR_M_GROUP]);
    dm_sim_char nowhere = sorc; nowhere.act = 0;
    CHECK(dm_gear_tip_rate(d, &nowhere, &nowhere, &goal, 0, &tip) == -1, "without an act the swap is not rated");

    dm_sim_char pal = parse(d, PAL), pal_after = pal; pal_after.lvl[98] = 25; dm_gear_measures mpal, mpal_after;
    assert(!dm_gear_measure(d, &pal, 0, &mpal) && !dm_gear_measure(d, &pal_after, 0, &mpal_after));
    double group_up = 100 * (mpal_after.group / mpal.group - 1), single_up = 100 * (mpal_after.single / mpal.single - 1);
    CHECK(group_up > 2 && fabs(group_up - single_up) < 0.5 * single_up, "active aura 20 -> 25: group %+.2f %%, single %+.2f %%", group_up, single_up);

    dm_sim_char strong = pal; strong.str += 100; dm_gear_measures ms;
    assert(!dm_gear_measure(d, &strong, 0, &ms) && ms.group > mpal.group);
    CHECK(dm_gear_reasons(d, &pal, &strong, &mpal, &ms, 0, &why) == 1 && why.group[DM_GEAR_M_GROUP] == DM_GEAR_R_DAMAGE &&
          why.group[DM_GEAR_M_SINGLE] == DM_GEAR_R_DAMAGE, "strength alone: the damage group (%d %d)", why.group[DM_GEAR_M_GROUP],
          why.group[DM_GEAR_M_SINGLE]);
}

static const dm_gear_words WORDS = { { "Schaden Gruppe", "Schaden Einzelziel", "Überleben", "Erholung" },
    { "Verteidigung", "Leben", "Mana", "Lebensregeneration", "Entzug", "Widerstand", "Zaubertempo", "Angriffstempo", "Erholung nach Treffern", "Skill-Stufen",
      "Schaden" }, "mehr %s", "weniger %s", "unter deinem Überlebensziel: %d Treffer", "erreicht dein Überlebensziel: %d Treffer",
    "noch unter deinem Überlebensziel: %s auf %s Treffer", "noch unter deinem Überlebensziel: %s Treffer" };
static dm_gear_tip rated(dm_gear_measures now, dm_gear_measures after, double hits_goal) {
    dm_gear_tip t; memset(&t, 0, sizeof t); t.now = now; t.after = after; t.goal_hits = hits_goal; t.act = 2;
    dm_gear_goal g = { hits_goal, 0.5 }; dm_gear_score(&now, &after, &g, &t.score);
    for (int k = 0; k < DM_GEAR_M_N; k++) t.reasons.group[k] = -1;
    return t;
}
static int text(const dm_gear_tip *t, int comma, char line[][DM_GEAR_LINEB]) {
    int n = dm_gear_tip_text(t, &WORDS, comma, line, DM_GEAR_LINES);
    for (int i = 0; i < n; i++) printf("tip line: %c %s\n", line[i][3], line[i] + 4);
    return n;
}
static void text_tests(void) {
    char line[DM_GEAR_LINES][DM_GEAR_LINEB];
    dm_gear_tip t = rated(M(100, 200, 60, 20, 1), M(110, 180, 54, 18, 1), 0);
    t.reasons.group[DM_GEAR_M_GROUP] = DM_GEAR_R_MANA; t.dir[DM_GEAR_M_GROUP] = 1;
    t.reasons.group[DM_GEAR_M_SINGLE] = DM_GEAR_R_DAMAGE; t.dir[DM_GEAR_M_SINGLE] = -1;
    t.reasons.group[DM_GEAR_M_SURVIVAL] = DM_GEAR_R_RESIST; t.dir[DM_GEAR_M_SURVIVAL] = -1;
    int n = text(&t, 1, line);
    CHECK(n == 5, "three changed measures, two reasons (the damage group under a damage line repeats it): %d lines", n);
    CHECK(n == 5 && !strcmp(line[0] + 4, "Schaden Gruppe +10,0 %") && line[0][3] == '3' && !strcmp(line[1] + 4, "mehr Mana") && line[1][3] == '5'
          && !strcmp(line[2] + 4, "Schaden Einzelziel -10,0 %") && line[2][3] == '1' && !strcmp(line[3] + 4, "Überleben -10,0 %")
          && !strcmp(line[4] + 4, "weniger Widerstand"), "lines in order, colours by direction");
    CHECK(text(&t, 0, line) == 5 && !strcmp(line[0] + 4, "Schaden Gruppe +10.0 %"), "decimal point without the comma");
    t.reasons.group[DM_GEAR_M_SURVIVAL] = DM_GEAR_R_MANA; t.dir[DM_GEAR_M_SURVIVAL] = 1;
    CHECK(text(&t, 1, line) == 4, "a reason that only repeats the one before is left out");
    t.reasons.group[DM_GEAR_M_SURVIVAL] = DM_GEAR_R_RESIST; t.dir[DM_GEAR_M_SURVIVAL] = 0;
    CHECK(text(&t, 1, line) == 4 && !strcmp(line[3] + 4, "Überleben -10,0 %"), "a group no way of which explains the change gives no reason line");

    t = rated(M(100, 200, 60, 20, 1), M(100.04, 200, 60, 20, 1.04), 0);
    CHECK(text(&t, 1, line) == 0, "changes that round to 0,0 give no line and no block");
    t = rated(M(0, 0, 60, 20, 1), M(0, 0, 66, 22, 3.5), 0);
    CHECK(text(&t, 1, line) == 2 && !strcmp(line[0] + 4, "Überleben +10,0 %") && !strcmp(line[1] + 4, "Erholung +2,5 %"),
          "no damage base: no damage lines; recovery in points");
    t = rated(M(0, 50, 60, 20, 1), M(0, 55, 60, 20, 1), 0);
    CHECK(text(&t, 1, line) == 1 && !strcmp(line[0] + 4, "Schaden Einzelziel +10,0 %"), "a damage line only for the measure that changes");

    t = rated(M(100, 200, 60, 20, 0), M(150, 300, 30, 10, 0), 15);
    CHECK(t.score.goal == DM_GEAR_GOAL_MISSED && t.score.dots_survival == -3, "below the goal: three red survival dots");
    CHECK(text(&t, 1, line) == 4 && line[0][3] == '1' && !strcmp(line[0] + 4, "unter deinem Überlebensziel: 15 Treffer"), "the goal line comes first, red");
    t = rated(M(100, 200, 30, 10, 0), M(100, 200, 60, 20, 0), 15);
    CHECK(text(&t, 1, line) == 2 && line[0][3] == '3' && !strcmp(line[0] + 4, "erreicht dein Überlebensziel: 15 Treffer"), "reaching the goal: green line");

    t = rated(M(100, 200, 30, 9.2, 0), M(282.8, 226.8, 34.6, 9.6, 0), 15);
    CHECK(t.score.goal == DM_GEAR_GOAL_NONE && t.score.dots_survival == 1 && t.score.dots_damage == 3, "below on both sides: the hits give the survival dots (%d)",
          t.score.dots_survival);
    n = text(&t, 1, line);
    CHECK(n >= 1 && line[0][3] == '3' && !strcmp(line[0] + 4, "noch unter deinem Überlebensziel: 9 auf 10 Treffer"), "the hits now and after, green");
    t = rated(M(100, 200, 30, 9.6, 0), M(100, 200, 25, 9.2, 0), 15);
    CHECK(text(&t, 1, line) == 2 && line[0][3] == '1' && !strcmp(line[0] + 4, "noch unter deinem Überlebensziel: 10 auf 9 Treffer"), "fewer hits: red");
    t = rated(M(100, 200, 30, 9.04, 0), M(100, 200, 31, 9.31, 0), 15);
    CHECK(text(&t, 1, line) == 2 && !strcmp(line[0] + 4, "noch unter deinem Überlebensziel: 9,0 auf 9,3 Treffer"), "whole hits alike: one decimal");
    t = rated(M(100, 200, 30, 9, 0), M(150, 200, 30, 9, 0), 15);
    CHECK(t.score.dots_survival == 0 && text(&t, 1, line) == 2 && line[0][3] == '5' && !strcmp(line[0] + 4, "noch unter deinem Überlebensziel: 9 Treffer"),
          "the same hits with a damage change that would give dots: grey, why there are none");
    t = rated(M(100, 200, 30, 9, 0), M(100.5, 200, 30, 9, 0), 15);
    CHECK(text(&t, 1, line) == 1 && strncmp(line[0] + 4, "noch", 4), "the same hits and no damage dots: no hits line");
    t = rated(M(100, 200, 30, 20, 0), M(150, 200, 30, 22, 0), 15);
    CHECK(text(&t, 1, line) == 1 && strncmp(line[0] + 4, "noch", 4), "above the goal on both sides: no hits line");
    for (int i = 0; i < 3; i++) {
        t = rated(M(100, 200, 60, 20, 1), M(110, 180, 54, 18, 2), 0);
        CHECK(dm_gear_tip_text(&t, &WORDS, 1, line, i) == i, "at most %d lines", i);
    }
}




static const char *BARB_SHIELD = "class=4\nlevel=60\ndiff=0\nact=1\nlife=55\nmana=10\nstr=150\ndex=60\nar=300\ndef=22\nblock=9\n"
                                 "weapon=2,7,100,0,0,1,0,0,1HS\nskill=126,5,5\nbar=126\n";
static void shield_tests(const dm_sim_data *d) {
    dm_gear_goal goal = { 15, 0.5 }; dm_gear_tip tip; char line[DM_GEAR_LINES][DM_GEAR_LINEB];
    dm_sim_char weak = parse(d, BARB_SHIELD), weak_after = weak; weak_after.block = 18; weak_after.def = 39;
    CHECK(!dm_gear_tip_rate(d, &weak, &weak_after, &goal, 0, &tip) && tip.now.survival_s < 600 && tip.after.survival_s > tip.now.survival_s &&
          fabs(tip.after.hits - tip.now.hits) <= 1e-9 * tip.now.hits && tip.reasons.group[DM_GEAR_M_SURVIVAL] == DM_GEAR_R_DEFENSE && tip.dir[DM_GEAR_M_SURVIVAL] == 1,
          "life 55, block 9 -> 18: survival %g -> %g s, hits %g -> %g, reason %d way %d", tip.now.survival_s, tip.after.survival_s, tip.now.hits, tip.after.hits,
          tip.reasons.group[DM_GEAR_M_SURVIVAL], tip.dir[DM_GEAR_M_SURVIVAL]);
    int n = text(&tip, 1, line);
    CHECK(n == 2 && line[0][3] == '3' && !strncmp(line[0] + 4, "Überleben +", strlen("Überleben +")) && line[1][3] == '5' && !strcmp(line[1] + 4, "mehr Verteidigung"),
          "life 55: a green survival line and its reason (%d lines)", n);
    dm_sim_char strong = weak, strong_after = weak_after; strong.life = strong_after.life = 793;
    CHECK(!dm_gear_tip_rate(d, &strong, &strong_after, &goal, 0, &tip) && tip.now.survival_s == 600 && tip.after.survival_s == 600 &&
          tip.score.change[DM_GEAR_M_SURVIVAL] == 0, "life 793 outlives the horizon with both shields: survival %g -> %g s", tip.now.survival_s, tip.after.survival_s);
    CHECK(text(&tip, 1, line) == 0, "life 793: no line");
}

int main(int argc, char **argv) {
    if (argc != 2 || (strcmp(argv[1], "score") && strcmp(argv[1], "tables"))) { fputs("usage: t_gear_core score|tables\n", stderr); return 2; }
    if (!strcmp(argv[1], "score")) {
        score_tests();
        text_tests();
        if (failures) return 1;
        puts("gear score: review example, magnitudes, no damage base, dot steps, truncation, goal crossings, below the goal, search key, tooltip lines passed");
        return 0;
    }
    const char *path = getenv("DM_SIM_DATA");
    if (!path || !*path) { fputs("DM_SIM_DATA is required\n", stderr); return 2; }
    size_t n; char *text = sim_read(path, &n); dm_sim_data *d = calloc(1, sizeof *d);
    if (!text || !d || dm_sim_load(d, text, n, "rotw")) { fprintf(stderr, "tables: %s\n", d ? d->err : "memory"); return 2; }
    free(text);
    table_tests(d);
    frames_tests(d);
    shield_tests(d);
    dm_sim_free(d); free(d);
    if (failures) return 1;
    puts("gear measures: every class and act, area rule, mana limit, single target window, horizon, recovery, reasons, active aura level, attributes, tooltip rating, new skills by single skill bonuses, shield swap by life passed");
    return 0;
}
