


#include "../dm_fightlog_core.c"
#include "../dm_fightlog_text.c"
#include "../dm_fightlog_ui.c"
#include "../dm_fightlog_hist.c"
uint64_t now_ns(void) { return 0; }
#include <stdio.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static dm_sim_snapshot T;
int dm_sim_snapshot_copy(dm_sim_snapshot *out) { *out = T; return out->valid; }
int dm_sim_read(const char *set, dm_sim_reader reader, void *ctx) { (void)set; (void)reader; (void)ctx; return -1; }
int dm_custom_get(const char *key) { (void)key; return 1; }
int dm_sim_target_su(const dm_sim_data *d, int idx, int diff, dm_sim_target *t) { (void)d; (void)idx; (void)diff; (void)t; return -1; }
int dm_sim_target_mon(const dm_sim_data *d, int mon, int diff, int mlvl, int kind, int players, dm_sim_target *t) { (void)d; (void)mon; (void)diff; (void)mlvl; (void)kind; (void)players; (void)t; return -1; }
int dm_sim_area_act(const dm_sim_data *d, int area) { (void)d; (void)area; return 0; }
int dm_sim_skill_hands(const dm_sim_data *d, const dm_sim_char *c, int id) { (void)d; (void)c; (void)id; return DM_SH_OK; }
int dm_sim_fight(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, dm_sim_rot_out *rot, dm_sim_surv_out *surv, dm_sim_trace *trace) { (void)d; (void)c; (void)t; (void)rot; (void)surv; (void)trace; return -1; }


static uint32_t item(const uint32_t *keys, const int32_t *values, int n) { uint32_t h = 0; for (int i = 0; i < n; i++) h += dm_sim_gear_stat(keys[i], values[i]); return h; }

int main(void) {
    T.valid = 1; T.gear = 11; T.c.cls = 1; T.c.clvl = 30; T.c.str = 60; T.c.ar_base_known = 1; T.c.hand[0].min = 4; T.c.hand[0].max = 9; T.c.ed = 25; T.c.shield = 1;
    uint32_t k0 = dm_fl_build_key();
    T.c.res[DM_EL_FIRE] += 30; T.c.def += 200; T.c.lvl[5] += 3; T.c.life += 100; T.c.mana += 50;
    CHECK(k0 && dm_fl_build_key() == k0, "a buff that moves resistances, defence, skill levels, life and mana keeps the key");
    T.gear = 12; CHECK(dm_fl_build_key() != k0, "other gear with the same other values changes it");
    T.gear = 11; T.c.blvl[5]++; CHECK(dm_fl_build_key() != k0, "a hard point changes it");
    T.c.blvl[5]--; CHECK(dm_fl_build_key() == k0, "back to the fight's gear and points: the same key");

    dm_sim_snapshot full = T;
    T.c.ar_base_known = 0; T.c.capture_bad = 3; T.c.hand[0].min = T.c.hand[0].max = 0; T.c.ed = 0;
    CHECK(dm_fl_build_key() == k0, "an incomplete capture with the fight's gear and points keeps the key");
    T.gear = 12; uint32_t other = dm_fl_build_key(); CHECK(other && other != k0, "an incomplete capture with other gear does not take the fight's key");
    T.gear = 11; T.c.blvl[5]++; CHECK(dm_fl_build_key() != k0, "an incomplete capture with another hard point does not take the fight's key");
    T.c.blvl[5]--; T.c.ar_base_known = 1; T.c.hand[0].min = 4; T.c.hand[0].max = 9; T.c.ed = 25; T.c.shield = 0; T.c.capture_bad = 2;
    CHECK(dm_fl_build_key() == k0, "the shield unable to block for a moment (defence and block unknown) keeps the key");
    T = full; CHECK(dm_fl_build_key() == k0, "complete again: the same key");


    { dm_fl_parts p0, p; uint32_t s0 = dm_fl_state_key(1, &p0);
      T.c.ed += 200; T.c.res[DM_EL_COLD] += 20; T.c.life += 80; T.c.lvl[3] += 2;
      CHECK(s0 && dm_fl_state_key(1, &p) == s0, "a shrine or buff keeps the state"); T = full;
      T.c.clvl++; CHECK(dm_fl_state_key(1, &p) != s0 && p.level != p0.level && p.gear == p0.gear && p.points == p0.points && p.attrs == p0.attrs, "a level"); T = full;
      T.c.blvl[7]++; CHECK(dm_fl_state_key(1, &p) != s0 && p.points != p0.points && p.level == p0.level, "a hard point"); T = full;
      T.c.str++; CHECK(dm_fl_state_key(1, &p) != s0 && p.attrs != p0.attrs && p.points == p0.points, "an attribute"); T = full;
      T.gear = 13; CHECK(dm_fl_state_key(1, &p) != s0 && p.gear != p0.gear && p.attrs == p0.attrs, "gear"); T = full;
      CHECK(dm_fl_state_key(2, &p) != s0 && p.difficulty == 2 && p.gear == p0.gear, "the difficulty");
      T.valid = 0; CHECK(!dm_fl_state_key(1, &p), "no capture, no state"); T = full; }
    T.valid = 0; CHECK(dm_fl_build_key() == 0, "no capture: no key");


    CHECK(!dm_sim_gear_stat(72u << 16, 40) && !dm_sim_gear_stat(70u << 16, 12), "durability and quantity stay out");
    CHECK(dm_sim_gear_stat(204u << 16 | 0x1234, 20 << 8 | 7) == dm_sim_gear_stat(204u << 16 | 0x1234, 20 << 8 | 3), "the charges left stay out");
    CHECK(dm_sim_gear_stat(204u << 16 | 0x1234, 20 << 8 | 7) != dm_sim_gear_stat(204u << 16 | 0x1234, 25 << 8 | 7), "the charges a skill holds count");
    CHECK(dm_sim_gear_stat(39u << 16, 30) != dm_sim_gear_stat(39u << 16, 40) && dm_sim_gear_stat(39u << 16, 30) != dm_sim_gear_stat(41u << 16, 30), "a resistance counts by stat and value");
    { uint32_t k[] = { 31u << 16, 39u << 16, 72u << 16 }, r[] = { 72u << 16, 39u << 16, 31u << 16 }; int32_t v[] = { 120, 30, 40 }, w[] = { 12, 30, 120 }, worn[] = { 120, 30, 3 };
      CHECK(item(k, v, 3) == item(r, w, 3), "the order of the stats does not matter");
      CHECK(item(k, v, 3) == item(k, worn, 3), "a worn-down item keeps its identity"); }


    { uint32_t life[] = { 7u << 16 }, res[] = { 39u << 16 }; int32_t twenty[] = { 20 << 8 }, thirty[] = { 30 };
      dm_sim_gear_item armour = { .cls = 300, .mode = 1, .slot = 3, .stats = item(res, thirty, 1) }, charm = { .cls = 603, .mode = 0, .page = 0, .charm = 1, .counted = 1, .stats = item(life, twenty, 1) };
      uint32_t worn = dm_sim_gear_add(0, &armour), with = dm_sim_gear_add(worn, &charm);
      T = full; T.gear = with; uint32_t kc = dm_fl_build_key();
      T.gear = worn; CHECK(kc && dm_fl_build_key() != kc, "a pure life charm dropped from the inventory changes the key");
      CHECK(dm_sim_gear_add(dm_sim_gear_add(0, &charm), &armour) == with, "the order of the items does not matter");
      dm_sim_gear_item c = charm; c.page = 4; CHECK(dm_sim_gear_add(worn, &c) == worn, "a charm in the stash stays out");
      c = charm; c.mode = 2; CHECK(dm_sim_gear_add(worn, &c) == worn, "a charm in the belt or held stays out");
      c = charm; c.counted = 0; CHECK(dm_sim_gear_add(worn, &c) == worn, "a charm the game does not count (requirements not met) stays out");
      c = charm; c.charm = 0; CHECK(dm_sim_gear_add(worn, &c) == worn, "other items in the inventory stay out");
      c = charm; c.slot = 7; CHECK(dm_sim_gear_add(worn, &c) == with, "a charm moved to another cell keeps the identity");
      c = charm; c.stats = item(life, (int32_t[]){ 15 << 8 }, 1); CHECK(dm_sim_gear_add(worn, &c) != with, "a charm with other life changes it");
      c = armour; c.page = 2; CHECK(dm_sim_gear_add(0, &c) == worn, "a worn item counts by its slot alone"); }

    printf("t_fightlog_key: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
