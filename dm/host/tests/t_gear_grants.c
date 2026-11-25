

#define _POSIX_C_SOURCE 200809L
#include "sim_test_io.h"
#include "../dm_gear_core.h"
#include <math.h>
#include <string.h>

static int failures;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)





enum { MELEE = 106, DAMAGE_AURA_B = 98 };

static const char *PALADIN =
    "class=3\nlevel=80\ndiff=2\nlife=1200\nmana=600\nstr=150\ndex=120\nar=4000\ndef=1600\nfcr=63\nias=40\nweapon=100,160,100,0,0,1,0,0,1HS\n"
    "res=15,40,40,40,40,0\nshield=1\nshield_min=20\nshield_max=30\nkick_min=20\nkick_max=30\nact=1\n";

static dm_sim_char profile(const dm_sim_data *d, const char *extra) {
    char text[4096]; snprintf(text, sizeof text, "%s%s", PALADIN, extra);
    dm_sim_char c; char err[128];
    if (dm_sim_profile_parse(d, text, &c, err, sizeof err)) { fprintf(stderr, "profile: %s\n%s\n", err, extra); exit(2); }
    return c;
}
static dm_sim_target enemy(const dm_sim_data *d, const dm_sim_char *c) {
    dm_sim_target t; if (dm_sim_target_act(d, c, &t) < 0) { fputs("no standard enemy\n", stderr); exit(2); }
    return t;
}
static double rotation(const dm_sim_data *d, const dm_sim_char *c) {
    dm_sim_target t = enemy(d, c); dm_sim_rot_out r; if (dm_sim_rotation(d, c, &t, &r)) return -1; return r.dps;
}
static dm_sim_rot_out fight(const dm_sim_data *d, const dm_sim_char *c, dm_sim_surv_out *s) {
    dm_sim_target t = enemy(d, c); t.hp *= 20; dm_sim_rot_out r; memset(&r, 0, sizeof r); dm_sim_fight(d, c, &t, &r, s, NULL); return r;
}
static int near(double a, double b) { return fabs(a - b) <= 1e-9 * fmax(1, fmax(fabs(a), fabs(b))); }

static void aura_tests(const dm_sim_data *d) {

    dm_sim_char bare = profile(d, "skill=106,20,20\nbar=106\n");
    dm_sim_char item = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,122,12,0,0\n");
    dm_sim_char held = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,122,12,0,12\n");
    dm_sim_char own = profile(d, "skill=106,20,20\nbar=106\nskill=122,12,12\nbar=122\n");
    double b = rotation(d, &bare), i = rotation(d, &item), h = rotation(d, &held), o = rotation(d, &own);
    CHECK(i > b * 1.05, "an item aura 122 raises the melee damage (%g -> %g)", b, i);
    CHECK(near(i, o), "the item aura acts as the own aura of the same level (%g vs %g)", i, o);
    CHECK(near(h, b), "a held item aura is in the snapshot already (%g vs %g)", h, b);


    dm_sim_char two = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,122,12,0,0\ngrant=aura,98,10,0,0\n");
    CHECK(rotation(d, &two) > i * 1.02, "item aura 98 beside item aura 122 adds damage");


    dm_sim_char both = profile(d, "skill=106,20,20\nbar=106\nskill=122,12,12\nbar=122\ngrant=aura,122,5,0,0\n");
    CHECK(near(rotation(d, &both), o), "a lower item aura under the own one changes nothing");
    dm_sim_char higher = profile(d, "skill=106,20,20\nbar=106\nskill=122,5,5\nbar=122\ngrant=aura,122,12,0,0\n");
    CHECK(near(rotation(d, &higher), o), "a higher item aura over the own one acts at the item's level (%g vs %g)", rotation(d, &higher), o);
    dm_sim_char twice = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,122,12,0,0\ngrant=aura,122,7,0,0\n");
    CHECK(near(rotation(d, &twice), i), "the same aura from two items acts once at the higher level");


    dm_sim_char choose = profile(d, "skill=106,20,20\nbar=106\nskill=122,12,12\nbar=122\nskill=98,12,12\nbar=98\ngrant=aura,122,12,0,0\n");
    dm_sim_target t = enemy(d, &choose); dm_sim_rot_out r; dm_sim_rotation(d, &choose, &t, &r);
    CHECK(r.aura == DAMAGE_AURA_B, "own aura beside the item's aura 122: %d", r.aura);


    dm_sim_char fire = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,102,16,0,0\n");
    double f = rotation(d, &fire);
    double aura, struck; dm_sim_target ft = enemy(d, &fire); dm_sim_grant_dps(d, &fire, &ft, &aura, &struck);

    CHECK(aura > 0 && struck == 0 && f > b + aura, "item damage aura 102 adds its periodic and its attack damage (%g + %g vs %g)", b, aura, f);
    dm_sim_rot_out fr; dm_sim_rotation(d, &fire, &ft, &fr);
    CHECK(fr.primary == MELEE, "the melee attack stays the main action beside an item damage aura (%d)", fr.primary);
    dm_sim_char none = bare; double a0, s0; dm_sim_grant_dps(d, &none, &ft, &a0, &s0);
    CHECK(a0 == 0 && s0 == 0, "no grants, no grant damage");


    dm_sim_char cold = profile(d, "skill=106,20,20\nbar=106\nweapon_elmin=0,0,0,200,0,0\nweapon_elmax=0,0,0,300,0,0\n");
    dm_sim_char conv = profile(d, "skill=106,20,20\nbar=106\nweapon_elmin=0,0,0,200,0,0\nweapon_elmax=0,0,0,300,0,0\ngrant=aura,123,12,0,12\n");
    CHECK(rotation(d, &conv) > rotation(d, &cold) * 1.02, "item aura 123 counts on the target although held");


    dm_sim_char rf = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,100,8,0,0\n");
    dm_sim_char prepared_bare, prepared_rf; dm_sim_tmod m; dm_sim_rot_out pr;
    dm_sim_target tt = enemy(d, &rf);
    double taken0[DM_EL_N], taken1[DM_EL_N]; dm_sim_damage_taken(d, &bare, &tt, taken0); dm_sim_damage_taken(d, &rf, &tt, taken1);
    (void)prepared_bare; (void)prepared_rf; (void)m; (void)pr;
    CHECK(taken1[DM_EL_FIRE] <= taken0[DM_EL_FIRE], "item aura 100 does not raise fire damage taken");

    dm_sim_char capped = profile(d, "skill=106,20,20\nbar=106\nres=15,90,40,40,40,0\n");
    dm_sim_char capped_rf = profile(d, "skill=106,20,20\nbar=106\nres=15,90,40,40,40,0\ngrant=aura,100,20,0,0\n");
    double c0[DM_EL_N], c1[DM_EL_N]; dm_sim_damage_taken(d, &capped, &tt, c0); dm_sim_damage_taken(d, &capped_rf, &tt, c1);
    CHECK(c0[DM_EL_FIRE] == 0 || c1[DM_EL_FIRE] < c0[DM_EL_FIRE], "item aura 100's maximum resistance lowers capped fire damage (%g -> %g)", c0[DM_EL_FIRE], c1[DM_EL_FIRE]);
}

static void buff_tests(const dm_sim_data *d) {

    dm_sim_char bare = profile(d, "skill=106,20,20\nbar=106\n");
    dm_sim_char bo = profile(d, "skill=106,20,20\nbar=106\ngrant=buff,149,10,0,0\n");
    dm_sim_char held = profile(d, "skill=106,20,20\nbar=106\ngrant=buff,149,10,0,10\n");
    dm_sim_surv_out s0, s1, s2; fight(d, &bare, &s0); fight(d, &bo, &s1); fight(d, &held, &s2);
    CHECK(s1.hits_to_death > s0.hits_to_death, "buff 149 from set II raises the hits to death (%g -> %g)", s0.hits_to_death, s1.hits_to_death);
    CHECK(near(s2.hits_to_death, s0.hits_to_death), "buff 149 the snapshot holds at the same level changes nothing");
    dm_sim_char up = profile(d, "skill=106,20,20\nbar=106\ngrant=buff,149,14,0,10\n");
    dm_sim_char all = profile(d, "skill=106,20,20\nbar=106\ngrant=buff,149,14,0,0\n");
    dm_sim_surv_out su, sa; fight(d, &up, &su); fight(d, &all, &sa);
    CHECK(su.hits_to_death > s0.hits_to_death && su.hits_to_death < sa.hits_to_death, "a held buff at a lower level moves by the difference");
}

static void charge_tests(const dm_sim_data *d) {

    dm_sim_char few = profile(d, "mana=0\nweapon=1,2,100,0,0,1,0,0,1HS\nskill=47,30,0\nbar=47\ngrant=charge,47,30,3,0\n");
    dm_sim_char many = profile(d, "mana=0\nweapon=1,2,100,0,0,1,0,0,1HS\nskill=47,30,0\nbar=47\ngrant=charge,47,30,200,0\n");
    dm_sim_target t = enemy(d, &few); dm_sim_dealt_out a, b; t.hp = 1000000; t.regen = 0;
    CHECK(!dm_sim_dealt(d, &few, &t, 30 * DM_SIM_FPS, &a) && !dm_sim_dealt(d, &many, &t, 30 * DM_SIM_FPS, &b), "dealt");
    CHECK(a.life > 0 && b.life > a.life * 3, "three charges deal less than many, without mana (%g vs %g)", a.life, b.life);

    dm_gear_measures mf, mm;
    CHECK(!dm_gear_measure(d, &few, 0, &mf) && !dm_gear_measure(d, &many, 0, &mm), "measures");
    CHECK(mf.group > 0 && mm.group > mf.group * 3, "group damage from three charges is less than from many, without mana (%g vs %g)", mf.group, mm.group);
}

static void trigger_tests(const dm_sim_data *d) {

    dm_sim_char bare = profile(d, "skill=106,20,20\nbar=106\n");
    dm_sim_char hit = profile(d, "skill=106,20,20\nbar=106\ngrant=hit,47,15,20,0\n");
    dm_sim_char attack = profile(d, "skill=106,20,20\nbar=106\ngrant=attack,47,15,20,0\n");
    double b = rotation(d, &bare), h = rotation(d, &hit), a = rotation(d, &attack);
    CHECK(h > b && a > h, "missile 47 on hit / attack adds damage, per attack more than per landed hit (%g %g %g)", b, h, a);
    dm_sim_char twice = profile(d, "skill=106,20,20\nbar=106\ngrant=hit,47,15,40,0\n");
    double w = rotation(d, &twice);
    CHECK(fabs((w - b) - 2 * (h - b)) < 1e-6 * w, "the added damage is linear in the chance (%g vs %g)", w - b, 2 * (h - b));

    dm_sim_char struck = profile(d, "skill=106,20,20\nbar=106\ngrant=struck,47,15,100,0\n");
    dm_sim_surv_out s0, s1; dm_sim_rot_out r0 = fight(d, &bare, &s0), r1 = fight(d, &struck, &s1);
    CHECK(r1.ttk > 0 && r1.ttk < r0.ttk, "missile 47 when struck shortens the kill (%g -> %g)", r0.ttk, r1.ttk);
    dm_gear_measures m0, m1; dm_gear_measure(d, &bare, 0, &m0); dm_gear_measure(d, &struck, 0, &m1);
    CHECK(m1.group > m0.group, "struck skills count in the group measure");

    dm_sim_target t = enemy(d, &bare);
    dm_sim_char curse = profile(d, "skill=106,20,20\nbar=106\ngrant=hit,66,3,10,0\n");
    dm_sim_char field = profile(d, "skill=106,20,20\nbar=106\ngrant=hit,42,10,10,0\n");
    CHECK(dm_sim_grants_modelled(d, &hit, &t) && dm_sim_grants_modelled(d, &struck, &t) && dm_sim_grants_modelled(d, &bare, &t), "plain damage triggers have a model");
    CHECK(!dm_sim_grants_modelled(d, &curse, &t) && !dm_sim_grants_modelled(d, &field, &t), "curse and life share triggers have none");
    dm_sim_char auras = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,122,12,0,0\ngrant=buff,149,5,0,0\n");
    CHECK(dm_sim_grants_modelled(d, &auras, &t), "auras and buffs have a model");

    dm_sim_char slow = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,114,12,0,0\n");
    dm_sim_char undead = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,119,12,0,0\n");
    dm_sim_char resist = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,100,8,0,0\n");
    CHECK(!dm_sim_grants_modelled(d, &slow, &t) && !dm_sim_grants_modelled(d, &undead, &t), "auras 114 and 119 have no model");
    CHECK(dm_sim_grants_modelled(d, &resist, &t), "aura 100 has a model");
}

static void check_tests(const dm_sim_data *d) {

    dm_sim_char bare = profile(d, "skill=106,20,20\nbar=106\n");
    dm_sim_char held = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,122,12,0,12\n");
    dm_sim_grant_miss miss;
    CHECK(dm_sim_grants_ok(d, &held, &miss), "an unchecked held aura may be rated");
    held.grant[0].checked = 1;
    CHECK(!dm_sim_grants_ok(d, &held, &miss) && miss.stat >= 0 && miss.skill == 122 && miss.level == 12 && miss.game == 0 && miss.model != 0,
          "state values of zero differ from the model (stat %d: %d vs %d)", miss.stat, miss.model, miss.game);

    const dm_sim_skill *s = d->skill[122]; int rounds = 0;
    while (!dm_sim_grants_ok(d, &held, &miss) && miss.stat >= 0 && rounds++ < 6)
        for (int k = 0; k < 6; k++) if (s->astat[k] == miss.stat) held.grant[0].game[k] = miss.model;
    CHECK(dm_sim_grants_ok(d, &held, NULL) && rounds >= 1, "a held aura whose state holds the model's values may be rated");
    dm_sim_char wrong = held; wrong.grant[0].game[0]++;
    CHECK(!dm_sim_grants_ok(d, &wrong, &miss) && miss.stat == s->astat[0], "one differing value stops it");

    dm_sim_char one = profile(d, "skill=106,20,20\nbar=106\ngrant=aura,122,1,0,1\n"); one.grant[0].checked = 1; rounds = 0;
    while (!dm_sim_grants_ok(d, &one, &miss) && miss.stat >= 0 && rounds++ < 6)
        for (int k = 0; k < 6; k++) if (s->astat[k] == miss.stat) one.grant[0].game[k] = miss.model;
    int pct = -1; for (int k = 0; k < 6; k++) if (s->astat[k] >= 0 && !strcmp(d->stat_name[s->astat[k]], "damagepercent")) pct = one.grant[0].game[k];
    CHECK(pct == 50, "the owner's damage percent of aura 122 at level 1 is the game's 50: %d", pct);
    dm_sim_char curse = profile(d, "skill=106,20,20\nbar=106\ngrant=hit,66,3,10,0\n");
    CHECK(!dm_sim_grants_ok(d, &curse, &miss) && miss.stat == -1, "a grant without a model stops it");
    dm_gear_goal goal = { 15, 0.5 }; dm_gear_tip tip;
    CHECK(dm_gear_tip_rate(d, &bare, &curse, &goal, 0, &tip) == DM_GEAR_RATE_GRANTS && tip.grant.stat == -1, "the rating says a grant stopped it");
    CHECK(dm_gear_tip_rate(d, &wrong, &bare, &goal, 0, &tip) == DM_GEAR_RATE_GRANTS && tip.grant.skill == 122, "on either side");
    CHECK(dm_gear_tip_rate(d, &bare, &held, &goal, 0, &tip) == 0, "checked grants are rated");
}

int main(void) {
    const char *path = getenv("DM_SIM_DATA");
    if (!path || !*path) { fputs("DM_SIM_DATA is required\n", stderr); return 2; }
    size_t n; char *text = sim_read(path, &n); dm_sim_data *d = calloc(1, sizeof *d);
    if (!text || !d || dm_sim_load(d, text, n, "rotw")) { fprintf(stderr, "tables: %s\n", d ? d->err : "memory"); return 2; }
    free(text);
    aura_tests(d); buff_tests(d); charge_tests(d); trigger_tests(d); check_tests(d);
    dm_sim_free(d); free(d);
    if (failures) { fprintf(stderr, "%d failed\n", failures); return 1; }
    puts("gear grants: item auras, same aura twice, own aura choice, damage auras, target auras, set II buffs, charges, triggers, struck, models, state checks passed");
    return 0;
}
