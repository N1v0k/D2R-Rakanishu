

#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_gear_core.h"
#include <math.h>
#include <string.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)



enum { ATTACK = 0, RIGHT_ONLY = 126, BOTH = 147, ALTERNATING = 151 };

static const char *CLASS4 =
    "class=4\nlevel=80\ndiff=2\nlife=1200\nmana=600\nstr=150\ndex=120\nar=4000\ndef=1600\nfcr=0\nias=0\nres=15,40,40,40,40,0\nshield=0\n"
    "kick_min=20\nkick_max=30\nact=1\nskill=126,20,20\nbar=126\nskill=147,20,20\nbar=147\nskill=151,20,20\nbar=151\n";

static dm_sim_char profile(const dm_sim_data *d, const char *base, const char *extra) {
    char text[4096]; snprintf(text, sizeof text, "%s%s", base, extra);
    dm_sim_char c; char err[128];
    if (dm_sim_profile_parse(d, text, &c, err, sizeof err)) { fprintf(stderr, "profile: %s\n%s\n", err, extra); exit(2); }
    return c;
}
static dm_sim_skill_out eval(const dm_sim_data *d, const dm_sim_char *c, int skill) {
    dm_sim_target t; if (dm_sim_target_act(d, c, &t) < 0) { fputs("no standard enemy\n", stderr); exit(2); }
    dm_sim_skill_out o; if (dm_sim_skill_eval(d, c, &t, skill, &o) < 0 || (o.flags & DM_SF_UNAVAIL)) { fprintf(stderr, "skill %d unavailable\n", skill); exit(2); }
    return o;
}
static int near(double a, double b, double rel) { return fabs(a - b) <= rel * fmax(1, fmax(fabs(a), fabs(b))); }

#define STRONG "100,160,100,0,0,1,0,0"
#define WEAK "40,60,100,0,0,1,0,0"
#define SLOW "100,160,100,0,20,1,0,0"

static void hand_tests(const dm_sim_data *d) {
    dm_sim_char same = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\n");
    dm_sim_char weak_left = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" WEAK ",1HS\n");
    dm_sim_char weak_both = profile(d, CLASS4, "weapon=" WEAK ",1SS\noffhand=" WEAK ",1HS\n");

    dm_sim_skill_out r1 = eval(d, &same, RIGHT_ONLY), r2 = eval(d, &weak_left, RIGHT_ONLY);
    CHECK(near(r1.raw[0], r2.raw[0], 1e-9) && r1.frames == r2.frames, "the right hand alone: %g / %g", r1.raw[0], r2.raw[0]);

    dm_sim_skill_out a1 = eval(d, &same, ATTACK), a2 = eval(d, &weak_left, ATTACK), a3 = eval(d, &weak_both, ATTACK);
    CHECK(near(a2.raw[0], (a1.raw[0] + a3.raw[0]) / 2, 1e-9), "the normal attack: the mean of both hands (%g, %g, %g)", a1.raw[0], a2.raw[0], a3.raw[0]);

    dm_sim_skill_out b1 = eval(d, &same, BOTH), b2 = eval(d, &weak_left, BOTH), b3 = eval(d, &weak_both, BOTH);
    CHECK(near(b2.raw[0], (b1.raw[0] + b3.raw[0]) / 2, 1e-9), "both weapons: the mean per hit (%g, %g, %g)", b1.raw[0], b2.raw[0], b3.raw[0]);
    dm_sim_skill_out w1 = eval(d, &same, ALTERNATING), w2 = eval(d, &weak_left, ALTERNATING);
    CHECK(w2.raw[0] < w1.raw[0] && w2.raw[0] > w1.raw[0] / 2, "alternating hands: the weak left weapon takes half of the hits (%g, %g)", w1.raw[0], w2.raw[0]);


    dm_sim_char sure_left = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\noffhand_delta=4000,0,0,0\n");
    CHECK(near(eval(d, &sure_left, RIGHT_ONLY).cth, r1.cth, 1e-12), "the right hand's hit chance stays (%g)", r1.cth);
    CHECK(eval(d, &sure_left, ATTACK).cth > a1.cth, "the normal attack hits more often with a surer left hand");

    dm_sim_char hard_left = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\noffhand_delta=0,0,100,50\n");
    CHECK(near(eval(d, &hard_left, RIGHT_ONLY).raw[0], r1.raw[0], 1e-9), "the left hand's damage percent leaves the right hand's skill");
    CHECK(eval(d, &hard_left, ATTACK).raw[0] > a1.raw[0] * 1.2, "the left hand's damage percent and deadly strike raise the normal attack");


    dm_sim_char slow_left = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" SLOW ",1HS\n");
    CHECK(eval(d, &slow_left, RIGHT_ONLY).frames == r1.frames, "the right hand's swing keeps its length");
    CHECK(eval(d, &slow_left, ATTACK).frames > a1.frames, "the normal attack is longer with a slower left weapon (%d vs %d)", eval(d, &slow_left, ATTACK).frames, a1.frames);
}



static void mastery_crit_tests(const dm_sim_data *d) {
    dm_sim_char axes = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\n");
    dm_sim_char clubs = profile(d, CLASS4, "weapon=" WEAK ",1SS\noffhand=" WEAK ",1HS\n");
    dm_sim_char mixed = profile(d, CLASS4, "mastery_cs=10\nweapon=" STRONG ",1SS\noffhand=" WEAK ",1HS\nweapon_mastery_cs=-10\noffhand_mastery_cs=10\n");
    dm_sim_char swapped = profile(d, CLASS4, "mastery_cs=10\nweapon=" WEAK ",1SS\noffhand=" STRONG ",1HS\nweapon_mastery_cs=10\noffhand_mastery_cs=-10\n");
    double axe = eval(d, &axes, ATTACK).raw[0], club = eval(d, &clubs, ATTACK).raw[0];

    double right_axe = eval(d, &mixed, RIGHT_ONLY).raw[0], right_club = eval(d, &swapped, RIGHT_ONLY).raw[0];
    CHECK(near(right_axe, eval(d, &axes, RIGHT_ONLY).raw[0], 1e-9), "the axe's skill takes no critical chance: %g / %g", right_axe, eval(d, &axes, RIGHT_ONLY).raw[0]);
    CHECK(near(right_club, 1.2 * eval(d, &clubs, RIGHT_ONLY).raw[0], 1e-9), "the club's skill takes its 20 %%: %g / %g", right_club, 1.2 * eval(d, &clubs, RIGHT_ONLY).raw[0]);

    double attack = eval(d, &mixed, ATTACK).raw[0], want = (axe + 1.2 * club) / 2, spread = (1.1 * axe + 1.1 * club) / 2;
    CHECK(near(attack, want, 1e-9) && !near(attack, spread, 1e-6), "the normal attack: %g, per hand %g, spread over both %g", attack, want, spread);
    double both = eval(d, &mixed, BOTH).raw[0], both_want = (eval(d, &axes, BOTH).raw[0] + 1.2 * eval(d, &clubs, BOTH).raw[0]) / 2;
    CHECK(near(both, both_want, 1e-9), "both weapons: %g / %g", both, both_want);
}


static void extra_tests(const dm_sim_data *d) {
    dm_sim_char plain = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\n");
    dm_sim_char cb_left = profile(d, CLASS4, "cb=40\nweapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\nweapon_extra=-40,0,0,0,0,0,0\n");
    dm_sim_char cb_both = profile(d, CLASS4, "cb=40\nweapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\n");
    CHECK(eval(d, &cb_left, RIGHT_ONLY).cb_frac == 0, "the left hand's crushing blow leaves the right hand's skill (%g)", eval(d, &cb_left, RIGHT_ONLY).cb_frac);
    double either = eval(d, &cb_left, ATTACK).cb_frac, both = eval(d, &cb_both, ATTACK).cb_frac;
    CHECK(either > 0 && either < both, "the normal attack swings the left hand half of the time (%g, both hands %g)", either, both);
    CHECK(eval(d, &cb_left, BOTH).cb_frac > 0, "a skill that swings both weapons takes the left hand's crushing blow");
    dm_sim_char ow_left = profile(d, CLASS4, "ow=50\nweapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\nweapon_extra=0,-50,0,0,0,0,0\n");
    CHECK(eval(d, &ow_left, RIGHT_ONLY).ow_chance == 0 && eval(d, &ow_left, ATTACK).ow_chance > 0, "open wounds of the left hand: its swings only");



    dm_sim_char ll_left = profile(d, CLASS4, "ll=10\nml=6\nweapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\nweapon_extra=0,0,-10,-6,0,0,0\n");
    dm_sim_char ll_right = profile(d, CLASS4, "ll=10\nml=6\nweapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\noffhand_extra=0,0,-10,-6,0,0,0\n");
    dm_sim_skill_out l1 = eval(d, &ll_left, RIGHT_ONLY), l2 = eval(d, &ll_left, ATTACK), l3 = eval(d, &ll_right, RIGHT_ONLY);
    CHECK(l1.ll == 0 && l1.ml == 0, "the left hand's leech leaves the right hand's skill (%g, %g)", l1.ll, l1.ml);
    CHECK(near(l2.ll, 5, 1e-9) && near(l2.ml, 3, 1e-9), "equal weapons share the leech of the normal attack (%g, %g)", l2.ll, l2.ml);
    CHECK(l3.ll == 10 && l3.ml == 6, "the right hand's leech for its skill (%g, %g)", l3.ll, l3.ml);
    CHECK(eval(d, &plain, RIGHT_ONLY).ll == 0 && eval(d, &ll_left, BOTH).ll > 0, "no leech without one, the left hand's for both weapons");
    dm_sim_char pmh_left = profile(d, CLASS4, "prevent_heal=1\nweapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\nweapon_extra=0,0,0,0,-1,0,0\n");
    CHECK(!eval(d, &pmh_left, RIGHT_ONLY).pmh && eval(d, &pmh_left, ATTACK).pmh, "prevent heal of the left hand: its swings only");


    dm_sim_char left = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\noffhand_elmin=0,0,0,0,512,0\noffhand_elmax=0,0,0,0,768,0\n");
    dm_sim_char right = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\nweapon_elmin=0,0,0,0,512,0\nweapon_elmax=0,0,0,0,768,0\n");
    dm_sim_dual_poison(&left, (const int[2]){0, 75});
    dm_sim_dual_poison(&right, (const int[2]){75, 0});
    CHECK(left.hand[1].elmin[DM_EL_POIS] == 150 && left.hand[1].elmax[DM_EL_POIS] == 225 && left.poison_frames == 75,
          "the left hand's poison over its own length (%d-%d over %d frames)", left.hand[1].elmin[DM_EL_POIS], left.hand[1].elmax[DM_EL_POIS], left.poison_frames);
    dm_sim_skill_out p1 = eval(d, &left, ATTACK), p2 = eval(d, &right, ATTACK);
    CHECK(p1.dot > 0 && near(p1.dot, p2.dot, 1e-9) && p1.dot_frames == p2.dot_frames, "mirrored poison: %g / %g", p1.dot, p2.dot);
    CHECK(eval(d, &left, RIGHT_ONLY).dot == 0 && eval(d, &right, RIGHT_ONLY).dot > 0, "the right hand's skill takes the right hand's poison only");
    dm_sim_char longer = profile(d, CLASS4, "weapon=" STRONG ",1SS\noffhand=" STRONG ",1HS\nweapon_elmin=0,0,0,0,512,0\nweapon_elmax=0,0,0,0,768,0\n"
                                 "offhand_elmin=0,0,0,0,256,0\noffhand_elmax=0,0,0,0,256,0\n");
    dm_sim_dual_poison(&longer, (const int[2]){75, 150});
    CHECK(longer.poison_frames == 150 && longer.hand[0].elmin[DM_EL_POIS] == 150 && longer.hand[1].elmin[DM_EL_POIS] == 150,
          "each hand's total over its own length, spread over the longer one (%d frames)", longer.poison_frames);
}

int main(void) {
    const char *path = getenv("DM_SIM_DATA");
    if (!path || !*path) { fputs("DM_SIM_DATA is required\n", stderr); return 2; }
    size_t n; char *text = sim_read(path, &n); dm_sim_data *d = calloc(1, sizeof *d);
    if (!text || !d || dm_sim_load(d, text, n, "rotw")) { fprintf(stderr, "tables: %s\n", d ? d->err : "memory"); return 2; }
    free(text);
    CHECK(d->skill[ATTACK]->weapsel == 0 && d->skill[RIGHT_ONLY]->weapsel == 0 && d->skill[BOTH]->weapsel == 3 && d->skill[ALTERNATING]->weapsel == 2,
          "dual wield column of the tables: %d %d %d %d", d->skill[ATTACK]->weapsel, d->skill[RIGHT_ONLY]->weapsel, d->skill[BOTH]->weapsel,
          d->skill[ALTERNATING]->weapsel);
    hand_tests(d);
    extra_tests(d);
    mastery_crit_tests(d);
    dm_sim_free(d); free(d);
    if (failures) { fprintf(stderr, "%d failed\n", failures); return 1; }
    puts("gear dual: hands by skill, mean of either hand, both and alternating hands, own attack rating, damage, speed, crushing blow, open wounds, leech, prevent heal, poison and mastery critical chance per hand passed");
    return 0;
}
