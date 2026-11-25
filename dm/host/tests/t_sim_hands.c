

#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_sim_internal.h"
#include <math.h>
#include <string.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

enum { ATTACK = 0, MULTI_SHOT = 12, FIRE_BOLT = 36, SMITE = 97, HOLY_SHIELD = 117, BASH = 126, DOUBLE_SWING = 133, DOUBLE_THROW = 140, FRENZY = 147,
       FISTS = 259, DRAGON_CLAW = 260, HEX_BANE = 385 };

static const char *BASE = "level=80\ndiff=2\nlife=1200\nmana=600\nstr=150\ndex=120\nar=4000\ndef=1600\nfcr=0\nias=0\nres=15,40,40,40,40,0\n"
                          "kick_min=20\nkick_max=30\nact=1\n";
#define STRONG "100,160,100,0,0,1,0,0"
#define WEAK "40,60,100,0,0,1,0,0"
#define CLAW "100,160,100,0,0,5,0,0"
#define WEAK_CLAW "40,60,100,0,0,5,0,0"
#define SWORDS "weapon=" STRONG ",1SS\nweapon_type=swor\noffhand=" STRONG ",1HS\noffhand_type=swor\n"
#define CLAWS "weapon=" CLAW ",HT2\nweapon_type=h2h\noffhand=" CLAW ",HT1\noffhand_type=h2h\n"
#define ONE_CLAW "weapon=" CLAW ",HT1\nweapon_type=h2h\n"

static dm_sim_char profile(const dm_sim_data *d, int cls, const char *extra) {
    char text[4096]; snprintf(text, sizeof text, "class=%d\n%s%s", cls, BASE, extra);
    dm_sim_char c; char err[128];
    if (dm_sim_profile_parse(d, text, &c, err, sizeof err)) { fprintf(stderr, "profile: %s\n%s\n", err, extra); exit(2); }
    return c;
}
static int hands(const dm_sim_data *d, int cls, const char *extra, int skill) { dm_sim_char c = profile(d, cls, extra); return dm_sim_skill_hands(d, &c, skill); }
static dm_sim_target target(const dm_sim_data *d, const dm_sim_char *c) {
    dm_sim_target t; if (dm_sim_target_act(d, c, &t) < 0) { fputs("no standard enemy\n", stderr); exit(2); }
    return t;
}
static dm_sim_skill_out eval(const dm_sim_data *d, const dm_sim_char *c, int skill) {
    dm_sim_target t = target(d, c); dm_sim_skill_out o;
    if (dm_sim_skill_eval(d, c, &t, skill, &o) < 0) { fprintf(stderr, "skill %d failed\n", skill); exit(2); }
    return o;
}
static dm_sim_skill_out evalp(const dm_sim_data *d, int cls, const char *text, int skill) { dm_sim_char c = profile(d, cls, text); return eval(d, &c, skill); }
static double raw(const dm_sim_skill_out *o) { return sim_sum(o->raw); }
static int near(double a, double b, double rel) { return fabs(a - b) <= rel * fmax(1, fmax(fabs(a), fabs(b))); }


static void state_tests(const dm_sim_data *d) {


    CHECK(hands(d, 6, "", FISTS) == DM_SH_OK && hands(d, 6, "", ATTACK) == DM_SH_OK && hands(d, 4, "", BASH) == DM_SH_OK, "bare hands: first hand");
    CHECK(hands(d, 6, "", DRAGON_CLAW) == DM_SH_SECOND && hands(d, 4, "", FRENZY) == DM_SH_SECOND, "bare hands: no second hand");
    CHECK(hands(d, 0, "", MULTI_SHOT) == DM_SH_FIRST && hands(d, 7, "", HEX_BANE) == DM_SH_FIRST && hands(d, 3, "", HOLY_SHIELD) == DM_SH_FIRST, "bare hands: no bow, no weapon, no shield");

    const char *unread = "weapon=" STRONG ",1HS\n";
    CHECK(hands(d, 0, unread, MULTI_SHOT) == DM_SH_OK && hands(d, 6, unread, FISTS) == DM_SH_OK && hands(d, 7, unread, HEX_BANE) == DM_SH_OK, "unread type: first hand");
    CHECK(hands(d, 6, unread, DRAGON_CLAW) == DM_SH_SECOND, "unread type: one item, no second hand");
    CHECK(hands(d, 3, unread, HOLY_SHIELD) == DM_SH_FIRST && hands(d, 3, unread, SMITE) == DM_SH_FIRST, "unread type: no shield");
    dm_sim_char unknown = profile(d, 0, unread); strcpy(unknown.hand[0].stype, "zzz");
    CHECK(dm_sim_skill_hands(d, &unknown, MULTI_SHOT) == DM_SH_OK, "a type the tables do not list counts as unread");

    const char *sword = "weapon=" STRONG ",1HS\nweapon_type=swor\n", *sword_shield = "weapon=" STRONG ",1HS\nweapon_type=swor\nshield=1\n";
    CHECK(hands(d, 3, sword_shield, HOLY_SHIELD) == DM_SH_OK && hands(d, 3, sword_shield, SMITE) == DM_SH_OK, "the shield meets the shield requirement");
    CHECK(hands(d, 3, sword, HOLY_SHIELD) == DM_SH_FIRST && hands(d, 3, sword, SMITE) == DM_SH_FIRST, "no shield, no Holy Shield or Smite");
    CHECK(hands(d, 0, "shield=1\n", MULTI_SHOT) == DM_SH_FIRST && hands(d, 4, "shield=1\n", BASH) == DM_SH_OK, "a shield alone: no bow, bare hand melee");
    CHECK(hands(d, 6, ONE_CLAW "shield=1\n", DRAGON_CLAW) == DM_SH_SECOND && hands(d, 6, ONE_CLAW "shield=1\n", FISTS) == DM_SH_OK, "claw and shield");

    CHECK(hands(d, 7, ONE_CLAW, HEX_BANE) == DM_SH_FIRST && hands(d, 7, "weapon=" CLAW ",HT1\nweapon_type=h2h2\n", HEX_BANE) == DM_SH_FIRST &&
          hands(d, 7, sword, HEX_BANE) == DM_SH_OK, "the hand-to-hand exclusion");

    CHECK(hands(d, 4, SWORDS, FRENZY) == DM_SH_OK && hands(d, 4, sword_shield, FRENZY) == DM_SH_SECOND && hands(d, 4, sword, FRENZY) == DM_SH_SECOND, "Frenzy");
    CHECK(hands(d, 4, "weapon=" STRONG ",1SS\nweapon_type=tkni\noffhand=" STRONG ",1HS\noffhand_type=jave\n", DOUBLE_THROW) == DM_SH_OK &&
          hands(d, 4, "weapon=" STRONG ",1SS\nweapon_type=swor\noffhand=" STRONG ",1HS\noffhand_type=tkni\n", DOUBLE_THROW) == DM_SH_SECOND &&
          hands(d, 4, "weapon=" STRONG ",1SS\nweapon_type=tkni\noffhand=" STRONG ",1HS\noffhand_type=swor\n", DOUBLE_THROW) == DM_SH_SECOND, "Double Throw");
    CHECK(hands(d, 6, "weapon=" CLAW ",HT2\nweapon_type=h2h\noffhand=" CLAW ",HT1\noffhand_type=h2h2\n", DRAGON_CLAW) == DM_SH_OK, "Dragon Claw with two claw types");
}


static void table_tests(const dm_sim_data *d) {
    int shld = d->itype_shld, checked = 0;
    for (int i = 0; i < d->nskill; i++) {
        const dm_sim_skill *s = &d->skills[i];
        if (s->passive || (!s->hand[DM_HAND_A] && !s->hand[DM_HAND_B])) continue;
        int fit = -1, unfit = -1;
        for (int t = 0; t < d->nitype; t++) {
            uint64_t m = d->itype[t].mask;
            if (m >> shld & 1) continue;
            int a = (m & s->hand[DM_HAND_A]) && !(m & s->hand[DM_HAND_EA]), b = !s->hand[DM_HAND_B] || ((m & s->hand[DM_HAND_B]) && !(m & s->hand[DM_HAND_EB]));
            if (a && b && fit < 0) fit = t;
            if (!a && unfit < 0) unfit = t;
        }
        int shield = s->hand[DM_HAND_A] == d->itype[shld].mask;
        CHECK((fit >= 0 || shield) && unfit >= 0, "skill %d: no fitting or no unfitting weapon type", s->id);
        if (unfit < 0) continue;
        dm_sim_char c; dm_sim_char_init(&c, 0);
        c.hand[0].kind = DM_WK_MELEE; strcpy(c.hand[0].stype, d->itype[unfit].code);
        CHECK(dm_sim_skill_hands(d, &c, s->id) == DM_SH_FIRST, "skill %d with %s: %d", s->id, d->itype[unfit].code, dm_sim_skill_hands(d, &c, s->id));
        if (shield) c.shield = 1;
        else {
            strcpy(c.hand[0].stype, d->itype[fit].code);
            if (s->hand[DM_HAND_B]) { c.nhands = 2; c.hand[1] = c.hand[0]; }
        }
        CHECK(dm_sim_skill_hands(d, &c, s->id) == DM_SH_OK, "skill %d with %s: %d", s->id, shield ? "a shield" : d->itype[fit].code, dm_sim_skill_hands(d, &c, s->id));
        checked++;
    }
    CHECK(checked >= 50, "only %d skills with a hand requirement", checked);
}



static void both_hands_tests(const dm_sim_data *d) {
    const char *c4 = "skill=147,20,20\nbar=147\nskill=133,20,20\nbar=133\nskill=140,20,20\nbar=140\nskill=126,20,20\nbar=126\n";
    char text[2048];
    snprintf(text, sizeof text, "%s%s", c4, SWORDS);
    dm_sim_char swords = profile(d, 4, text);
    dm_sim_skill_out fr = eval(d, &swords, FRENZY), ds = eval(d, &swords, DOUBLE_SWING);
    CHECK(near(raw(&fr), 1392.3, 1e-9) && fr.frames == 16 && fr.hits == 2, "Frenzy: %g in %d frames", raw(&fr), fr.frames);
    CHECK(near(raw(&ds), 994.5, 1e-9) && ds.frames == 15 && ds.hits == 2, "Double Swing: %g in %d frames", raw(&ds), ds.frames);
    snprintf(text, sizeof text, "%sweapon=" STRONG ",1SS\nweapon_type=tkni\noffhand=" STRONG ",1HS\noffhand_type=tkni\n", c4);
    dm_sim_skill_out dt = evalp(d, 4, text, DOUBLE_THROW);

    CHECK(near(raw(&dt), 2 * 668.746, 1e-9) && dt.frames == 18 && dt.hits == 2, "Double Throw: %g in %d frames", raw(&dt), dt.frames);
    snprintf(text, sizeof text, "%sweapon=" STRONG ",1HS\nweapon_type=swor\nshield=1\n", c4);
    dm_sim_char shield = profile(d, 4, text);
    CHECK(eval(d, &shield, FRENZY).flags & DM_SF_NOWEAPON && eval(d, &shield, DOUBLE_SWING).flags & DM_SF_NOWEAPON &&
          eval(d, &shield, DOUBLE_THROW).flags & DM_SF_NOWEAPON && !(eval(d, &shield, BASH).flags & DM_SF_NOWEAPON), "weapon and shield");
    const char *c6 = "skill=260,20,20\nbar=260\nskill=259,20,20\nbar=259\n";
    snprintf(text, sizeof text, "%s" CLAWS, c6);
    dm_sim_skill_out dc = evalp(d, 6, text, DRAGON_CLAW);

    CHECK(near(raw(&dc), 2 * 714.87, 1e-9) && dc.frames == 18 && dc.hits == 2, "Dragon Claw: %g in %d frames", raw(&dc), dc.frames);
    snprintf(text, sizeof text, "%s" ONE_CLAW, c6);
    CHECK(evalp(d, 6, text, DRAGON_CLAW).flags & DM_SF_NOWEAPON, "Dragon Claw with one claw");
    snprintf(text, sizeof text, "%s" ONE_CLAW "shield=1\n", c6);
    CHECK(evalp(d, 6, text, DRAGON_CLAW).flags & DM_SF_NOWEAPON, "Dragon Claw with claw and shield");

    snprintf(text, sizeof text, "%scb=40\now=30\n%s", c4, SWORDS);
    dm_sim_skill_out both = evalp(d, 4, text, FRENZY);
    snprintf(text, sizeof text, "%scb=40\now=30\n%sweapon_extra=-40,-30,0,0,0,0,0\n", c4, SWORDS);
    dm_sim_skill_out left = evalp(d, 4, text, FRENZY);
    CHECK(left.cb_frac > 0 && near(1 - (1 - left.cb_frac) * (1 - left.cb_frac), both.cb_frac, 1e-12), "crushing blow of the left hand: %g, both %g", left.cb_frac, both.cb_frac);
    CHECK(left.ow_chance > 0 && near(1 - (1 - left.ow_chance) * (1 - left.ow_chance), both.ow_chance, 1e-12), "open wounds of the left hand: %g, both %g", left.ow_chance,
          both.ow_chance);
}



static dm_sim_skill_out fists(const dm_sim_data *d, const char *hands_text, int charges, const char *extra) {
    char text[2048]; snprintf(text, sizeof text, "skill=259,20,20\nbar=259\ncharges=%d\n%s%s", charges, hands_text, extra);
    dm_sim_char c = profile(d, 6, text);
    dm_sim_target t = target(d, &c); dm_sim_skill_out o;
    int id = FISTS;
    if (dm_sim_skills_prepared(d, &c, &t, 1, &id, &o) || (o.flags & DM_SF_UNAVAIL)) { fputs("Fists of Fire failed\n", stderr); exit(2); }
    return o;
}
static void cycle_tests(const dm_sim_data *d) {
    static const char *const kinds[3] = { ONE_CLAW, CLAWS, "weapon=" CLAW ",HT2\nweapon_type=h2h\noffhand=" WEAK_CLAW ",HT1\noffhand_type=h2h\n" };
    static const int per[3] = { 1, 2, 2 }, frames[3] = { 14, 18, 18 };
    int mana = dm_sim_mana_cost(d->skill[FISTS], 20);
    dm_sim_char plain = profile(d, 6, ONE_CLAW);
    int finish_frames = eval(d, &plain, ATTACK).frames, finish_two = evalp(d, 6, CLAWS, ATTACK).frames;
    double action[3];
    for (int k = 0; k < 3; k++) {
        dm_sim_skill_out o[3];
        for (int q = 0; q < 3; q++) o[q] = fists(d, kinds[k], q, "");
        for (int q = 0; q < 3; q++) {
            int build = (3 - q + per[k] - 1) / per[k], finish = k ? finish_two : finish_frames;
            CHECK(o[q].hits == build * per[k] + 1, "hands %d, %d charges: %g hits", k, q, o[q].hits);
            CHECK(o[q].mana_x256 == build * mana, "hands %d, %d charges: mana %d / %d", k, q, o[q].mana_x256, build * mana);
            CHECK(o[q].frames == build * frames[k] + finish, "hands %d, %d charges: %d frames / %d", k, q, o[q].frames, build * frames[k] + finish);
        }

        action[k] = raw(&o[0]) - raw(&o[1]);
        CHECK(action[k] > 0, "hands %d: no damage per action", k);
        if (k) CHECK(near(raw(&o[1]), raw(&o[2]), 1e-12), "hands %d: one action from 1 and 2 charges (%g / %g)", k, raw(&o[1]), raw(&o[2]));
        else CHECK(near(raw(&o[1]) - raw(&o[2]), action[k], 1e-9), "one claw: one action per charge (%g / %g)", raw(&o[1]) - raw(&o[2]), action[k]);
    }

    dm_sim_skill_out weak[2] = { fists(d, "weapon=" WEAK_CLAW ",HT1\nweapon_type=h2h\n", 0, ""), fists(d, "weapon=" WEAK_CLAW ",HT1\nweapon_type=h2h\n", 1, "") };
    CHECK(near(action[1], 2 * action[0], 1e-9), "two claws: %g, one claw %g", action[1], action[0]);
    CHECK(near(action[2], action[0] + raw(&weak[0]) - raw(&weak[1]), 1e-9), "unequal claws: %g, %g + %g", action[2], action[0], raw(&weak[0]) - raw(&weak[1]));

    for (int k = 0; k < 2; k++) {
        dm_sim_skill_out o0 = fists(d, kinds[k], 0, "cb=30\now=30\n"), o1 = fists(d, kinds[k], 1, "cb=30\now=30\n"), o2 = fists(d, kinds[k], 2, "cb=30\now=30\n");
        dm_sim_skill_out one = fists(d, kinds[0], 1, "cb=30\now=30\n"), one2 = fists(d, kinds[0], 2, "cb=30\now=30\n");
        double hit_cb = 1 - (1 - one.cb_frac) / (1 - one2.cb_frac), hit_ow = 1 - (1 - one.ow_chance) / (1 - one2.ow_chance);
        double want_cb = k ? 1 - (1 - hit_cb) * (1 - hit_cb) : hit_cb, want_ow = k ? 1 - (1 - hit_ow) * (1 - hit_ow) : hit_ow;
        CHECK(hit_cb > 0 && near((1 - o0.cb_frac) / (1 - o1.cb_frac), 1 - want_cb, 1e-9), "hands %d: crushing blow per action %g / %g", k,
              1 - (1 - o0.cb_frac) / (1 - o1.cb_frac), want_cb);
        CHECK(hit_ow > 0 && near((1 - o0.ow_chance) / (1 - o1.ow_chance), 1 - want_ow, 1e-9), "hands %d: open wounds per action %g / %g", k,
              1 - (1 - o0.ow_chance) / (1 - o1.ow_chance), want_ow);
        CHECK(!k || near(o1.cb_frac, o2.cb_frac, 1e-12), "two claws: from 1 and 2 charges alike");
    }

    for (int k = 0; k < 2; k++) {
        const char *grant = "grant=hit,36,10,100,0\n";
        dm_sim_skill_out o0 = fists(d, kinds[k], 0, grant), b0 = fists(d, kinds[k], 0, ""), o2 = fists(d, kinds[k], 2, grant), b2 = fists(d, kinds[k], 2, "");
        double r = (raw(&o0) - raw(&b0)) / (raw(&o2) - raw(&b2)), want = o0.hits * o0.cth / (o2.hits * o2.cth);
        CHECK(raw(&o2) > raw(&b2) && near(r, want, 1e-9), "hands %d: hit skill casts %g / %g", k, r, want);
        CHECK(o0.hits == (k ? 5 : 4) && o2.hits == (k ? 3 : 2), "hands %d: cycle hits %g / %g", k, o0.hits, o2.hits);
    }
}


static void buff_tests(const dm_sim_data *d) {
    const char *hs = "skill=117,20,20\nbar=117\nskill=96,20,20\nbar=96\n";
    char text[2048];
    snprintf(text, sizeof text, "%sweapon=" STRONG ",1HS\nweapon_type=swor\nshield=1\n", hs);
    dm_sim_char with = profile(d, 3, text);
    snprintf(text, sizeof text, "%sweapon=" STRONG ",1HS\nweapon_type=swor\n", hs);
    dm_sim_char without = profile(d, 3, text);
    dm_sim_target t = target(d, &with);
    dm_sim_char c; dm_sim_tmod m; dm_sim_rot_out r;
    sim_prepare(d, &with, &t, &c, &m, &r);
    CHECK(c.def > with.def && c.nactive_buff == 1 && c.active_buff[0] == HOLY_SHIELD, "Holy Shield cast with a shield (defense %d / %d)", c.def, with.def);
    sim_prepare(d, &without, &t, &c, &m, &r);
    CHECK(c.def == without.def && c.nactive_buff == 0, "no Holy Shield without a shield (defense %d / %d)", c.def, without.def);

    snprintf(text, sizeof text, "%sweapon=" STRONG ",1HS\nweapon_type=swor\nshield=1\nactive_level=117,20\n", hs);
    dm_sim_char now = profile(d, 3, text), same = now, other = now;
    other.shield = 0;
    dm_sim_hands_drop(d, &now, &same);
    dm_sim_hands_drop(d, &now, &other);
    CHECK(!memcmp(&same, &now, sizeof now), "the capture's hands keep Holy Shield");
    CHECK(other.active_lvl[HOLY_SHIELD] == 0 && other.def < now.def, "hands without a shield lose Holy Shield (defense %d from %d)", other.def, now.def);
    sim_prepare(d, &other, &t, &c, &m, &r);
    CHECK(c.def == other.def && c.active_lvl[HOLY_SHIELD] == 0, "and the fight does not cast it again (defense %d / %d)", c.def, other.def);

    snprintf(text, sizeof text, "%sweapon=" STRONG ",1HS\nweapon_type=swor\nactive_level=117,20\n", hs);
    dm_sim_char held = profile(d, 3, text), held_other = held;
    dm_sim_hands_drop(d, &held, &held_other);
    CHECK(!memcmp(&held_other, &held, sizeof held), "a buff the captured hands do not hold stays as captured");

    const char *hex = "skill=385,20,20\nbar=385\nskill=391,20,20\nbar=391\n";
    snprintf(text, sizeof text, "%sweapon=" STRONG ",1HS\nweapon_type=swor\n", hex);
    dm_sim_char sword = profile(d, 7, text);
    snprintf(text, sizeof text, "%sweapon=" STRONG ",1HS\nweapon_type=h2h\n", hex);
    dm_sim_char claw = profile(d, 7, text);
    sim_prepare(d, &sword, &t, &c, &m, &r);
    CHECK(r.curse == HEX_BANE, "Hex Bane with a sword (curse %d)", r.curse);
    sim_prepare(d, &claw, &t, &c, &m, &r);
    CHECK(r.curse == -1, "no Hex Bane with a claw (curse %d)", r.curse);
}


static uint32_t crc32_of(const char *b, const char *e) {
    uint32_t crc = ~0u;
    for (; b < e; b++) { crc ^= (unsigned char)*b; for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ ((0u - (crc & 1)) & 0xedb88320u); }
    return ~crc;
}

static void reseal(char *text) {
    char *body = strchr(text, '\n') + 1, *end = strstr(text, "\nend\t") + 1, *tab = memchr(text, '\t', (size_t)(body - text));
    tab = strchr(tab + 1, '\t'); tab = strchr(tab + 1, '\t');
    char crc[9]; snprintf(crc, sizeof crc, "%08x", crc32_of(body, end)); memcpy(tab + 1, crc, 8);
}
static void loader_tests(const char *text, size_t n) {
    char *copy = malloc(n + 1); dm_sim_data *x = calloc(1, sizeof *x);
    if (!copy || !x) exit(2);
    memcpy(copy, text, n + 1); copy[7] = '4';
    CHECK(!strncmp(text, "DM_SIM\t5\t", 9) && dm_sim_load(x, copy, n, "rotw") && !x->ok, "format 4 refused");
    memcpy(copy, text, n + 1);
    char *row = strstr(copy, "\nskill\t260\t");
    char *eol = row ? strchr(row + 1, '\n') : NULL, *req = eol ? eol - strlen("\th2h\t-\th2h\t-") : NULL;
    CHECK(req && !strncmp(req, "\th2h\t-\th2h\t-", strlen("\th2h\t-\th2h\t-")), "Dragon Claw's requirement columns");
    if (req) {
        memcpy(req + 1, "zzz", 3); reseal(copy);
        CHECK(dm_sim_load(x, copy, n, "rotw") && !x->ok && strstr(x->err, "itypea"), "an unknown type refused (%s)", x->err);
        memcpy(req + 1, "h2h", 3); reseal(copy);
        CHECK(!dm_sim_load(x, copy, n, "rotw"), "resealed unchanged: %s", x->err);
        dm_sim_free(x);
    }
    free(copy); free(x);
}



static void bar_tests(const dm_sim_data *d) {
    int out[4][2];
    dm_sim_char one = profile(d, 4, "weapon=" STRONG ",1HS\nweapon_type=swor\nskill=147,10,10\nskill=133,5,5\nskill=126,3,3\nbar=147\nbar=0\nbar=126\nbar=133\n");
    int n = dm_sim_bar_hands(d, &one, out, 4);
    CHECK(n == 2 && out[0][0] == FRENZY && out[0][1] == DM_SH_SECOND && out[1][0] == DOUBLE_SWING && out[1][1] == DM_SH_SECOND,
          "one sword: Frenzy and Double Swing need two (%d: %d/%d %d/%d)", n, out[0][0], out[0][1], out[1][0], out[1][1]);
    CHECK(dm_sim_bar_hands(d, &one, out, 1) == 1 && out[0][0] == FRENZY, "at most max, in the bar's order");
    dm_sim_char two = profile(d, 4, SWORDS "skill=147,10,10\nskill=133,5,5\nbar=147\nbar=133\n");
    CHECK(dm_sim_bar_hands(d, &two, out, 4) == 0, "two swords hold both");
    dm_sim_char archer = profile(d, 0, "weapon=" STRONG ",1HS\nweapon_type=swor\nskill=12,10,10\nbar=12\nbar=0\n");
    n = dm_sim_bar_hands(d, &archer, out, 4);
    CHECK(n == 1 && out[0][0] == MULTI_SHOT && out[0][1] == DM_SH_FIRST, "a sword: Multiple Shot not with this weapon (%d)", n);
    dm_sim_char bow = profile(d, 0, "weapon=100,160,100,0,0,2,0,0,BOW\nweapon_type=bow\nskill=12,10,10\nbar=12\n");
    CHECK(dm_sim_bar_hands(d, &bow, out, 4) == 0, "a bow holds Multiple Shot");


    dm_sim_char claw = profile(d, 6, ONE_CLAW "skill=260,10,10\nskill=252,10,10\nbar=260\nbar=252\n");
    claw.lvl[FRENZY] = 0; claw.bar[claw.nbar++] = FRENZY;
    n = dm_sim_bar_hands(d, &claw, out, 4);
    CHECK(n == 1 && out[0][0] == DRAGON_CLAW && out[0][1] == DM_SH_SECOND, "one claw: Dragon Claw needs two (%d)", n);
    dm_sim_char claws = profile(d, 6, CLAWS "skill=260,10,10\nbar=260\n");
    CHECK(dm_sim_bar_hands(d, &claws, out, 4) == 0, "two claws hold Dragon Claw");

    claw.bound[DRAGON_CLAW >> 6] &= ~(1ull << (DRAGON_CLAW & 63));
    CHECK(dm_sim_bar_hands(d, &claw, out, 4) == 0, "an item's Dragon Claw the player did not bind");
}
int main(void) {
    const char *path = getenv("DM_SIM_DATA");
    if (!path || !*path) { fputs("DM_SIM_DATA is required\n", stderr); return 2; }
    size_t n; char *text = sim_read(path, &n); dm_sim_data *d = calloc(1, sizeof *d);
    if (!text || !d || dm_sim_load(d, text, n, "rotw")) { fprintf(stderr, "tables: %s\n", d ? d->err : "memory"); return 2; }
    loader_tests(text, n);
    free(text);
    state_tests(d);
    table_tests(d);
    both_hands_tests(d);
    cycle_tests(d);
    buff_tests(d);
    bar_tests(d);
    dm_sim_free(d); free(d);
    if (failures) { fprintf(stderr, "%d failed\n", failures); return 1; }
    puts("sim hands: hand states, every requirement of the tables, one hit per hand, charge-up cycles, buffs and curses by hands, the bar's skills the hands do not hold, loader passed");
    return 0;
}
