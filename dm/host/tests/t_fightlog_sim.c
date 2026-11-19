





#include "../dm_fightlog_core.c"
#include "../dm_fightlog_text.c"
#include "../dm_fightlog_ui.c"
#include "../dm_fightlog_hist.c"
#include <stdio.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

enum { CLS = 7, MON = 78, RIGHT = 395, SUMMON = 373 };
static dm_sim_snapshot T;
static dm_sim_data D;
static dm_sim_mon M[MON + 1];
static dm_sim_char seen;
static dm_sim_target seen_t;
static dm_sim_skill S_RIGHT = { .fam = DM_FAM_Z1 }, S_SUMMON = { .fam = DM_FAM_P };
static int simulated; static double sim_ttk = 10;
uint64_t now_ns(void) { static uint64_t t; return t += 1000000000ull * 60; }
int dm_sim_snapshot_copy(dm_sim_snapshot *out) { *out = T; return out->valid; }
int dm_sim_read(const char *set, dm_sim_reader reader, void *ctx) { (void)set; return reader(&D, ctx); }
int dm_custom_get(const char *key) { (void)key; return 1; }
int dm_sim_target_su(const dm_sim_data *d, int idx, int diff, dm_sim_target *t) { (void)d; (void)idx; (void)diff; (void)t; return -1; }
int dm_sim_target_mon(const dm_sim_data *d, int mon, int diff, int mlvl, int kind, int players, dm_sim_target *t) {
    (void)d; (void)mlvl; (void)kind; (void)players; memset(t, 0, sizeof *t); t->mon = mon; t->diff = diff; t->mlvl = 14; t->hp = 62; t->ac = 70; t->ar = 160; return 0;
}
int dm_sim_area_act(const dm_sim_data *d, int area) { (void)d; (void)area; return 1; }
int dm_sim_fight(const dm_sim_data *d, const dm_sim_char *c, const dm_sim_target *t, dm_sim_rot_out *rot, dm_sim_surv_out *surv, dm_sim_trace *trace) {
    (void)d; (void)surv; seen = *c; seen_t = *t; simulated++;
    rot->dps = 6.5; rot->ttk = sim_ttk; if (trace) { trace->n = 2; trace->width = 125; trace->end = 250; trace->end_kind = DM_END_KILL; }
    return 0;
}


static void record(dm_fl_fight *f, const uint16_t *skills, int nskills, uint32_t build) {
    memset(f, 0, sizeof *f);
    f->h.lead = 50; f->h.width = 4; f->h.nbuckets = 60; f->h.duration = 190; f->h.end = DM_FL_END_KILL; f->h.kills = 1; f->h.enemies = 1; f->h.ngroups = 1;
    f->h.area = 2; f->h.life_total = 62; f->h.dealt[0] = 62; f->h.build = build;
    for (int i = 0; i < nskills; i++) f->h.skill[i] = skills[i];
    f->g[0].cls = MON; f->g[0].su = 0xffff; f->g[0].joined = 1; f->g[0].kills = 1;
    f->b[50 / 4].en = DM_FL_EN_HIT;
    f->b[(50 + 75) / 4].me = DM_FL_ME_ATTACK; f->b[(50 + 75) / 4].skill = 1;
    dm_fl_state *s = dm_fl_core(); pthread_mutex_lock(&s->mu); s->ring[0] = *f; s->head = 1; s->count = 1; pthread_mutex_unlock(&s->mu);
}

static void record_group(dm_fl_fight *f, uint32_t build) {
    record(f, (const uint16_t[]){ 1 }, 1, build);
    f->h.enemies = 5; f->h.kills = 5; f->h.ngroups = 3; f->h.life_total = 3 * 20 + 500 + 30;
    f->g[0] = (dm_fl_group){ .cls = 5, .su = 0xffff, .joined = 3, .kills = 3, .tracked = 3, .t_first = 50, .t_done = 120, .dealt = 60, .life = 60 };
    f->g[1] = (dm_fl_group){ .cls = MON, .su = 0xffff, .rank = DM_FL_RANK_UNIQUE, .joined = 1, .kills = 1, .tracked = 1, .t_first = 200, .t_done = 230, .dealt = 500, .life = 500 };
    f->g[2] = (dm_fl_group){ .cls = 6, .su = 0xffff, .flags = DM_FL_G_OTHERS, .joined = 1, .kills = 1, .tracked = 1, .t_first = 60, .t_done = 90, .dealt = 30, .life = 30 };
    dm_fl_state *s = dm_fl_core(); pthread_mutex_lock(&s->mu); s->ring[0] = *f; pthread_mutex_unlock(&s->mu);
}
static int group_of_sim(const char *text) {
    const char *p = strstr(text, "\nsim\t"); if (!p) return -9;
    int kind, end, width, n, offset, group; double dps;
    return sscanf(p + 5, "%d\t%d\t%d\t%d\t%lf\t%d\t%d", &kind, &end, &width, &n, &dps, &offset, &group) == 7 ? group : -9;
}
static int offset_of(const char *text) {
    const char *p = strstr(text, "\nsim\t"); if (!p) return -1;
    int kind, end, width, n, offset; double dps;
    return sscanf(p + 5, "%d\t%d\t%d\t%d\t%lf\t%d", &kind, &end, &width, &n, &dps, &offset) == 6 ? offset : -1;
}

int main(void) {
    static char out[1 << 16]; dm_fl_fight f;
    D.ok = 1; D.nmon = MON + 1; D.mon = M; snprintf(M[MON].id, sizeof M[MON].id, "m%d", MON);
    T.valid = 1; T.gear = 11; T.c.cls = CLS; T.c.clvl = 7; T.c.str = 20; T.c.ar_base_known = 1; T.c.hand[0].min = 2; T.c.hand[0].max = 5;
    T.c.bar[0] = RIGHT; T.c.bar[1] = 0; T.c.nbar = 2; T.c.ordered = 1;
    uint32_t build = dm_fl_build_key();


    record(&f, (const uint16_t[]){ 1 }, 1, build);
    int n = dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(n > 0 && simulated == 1, "the single fight is simulated");
    CHECK(seen.nbar == 0 && !seen.ordered, "only the normal attack: no bar skill (got %d skills, first %d)", seen.nbar, seen.bar[0]);
    CHECK(offset_of(out) == (50 + 75) / 4 * 4, "the simulated fight starts at the player's first attack, not at the enemy's first hit (offset %d)", offset_of(out));


    record(&f, (const uint16_t[]){ 1, RIGHT + 1 }, 2, build);
    simulated = 0; n = dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(n > 0 && simulated == 1 && seen.nbar == 1 && seen.bar[0] == RIGHT, "the skill used in the fight is simulated (%d skills)", seen.nbar);


    record(&f, (const uint16_t[]){ 0 }, 0, build);
    simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 1 && seen.nbar == 2 && seen.bar[0] == RIGHT, "no own action: the bar as captured");


    T.c.ar_base_known = 0; T.c.capture_bad = 3; T.c.hand[0].min = T.c.hand[0].max = 0;
    record(&f, (const uint16_t[]){ 1 }, 1, build);
    simulated = 0; n = dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(n > 0 && simulated == 1 && seen.ar_base_known && seen.hand[0].min == 2 && seen.hand[0].max == 5, "simulated with the complete capture's weapon (%d-%d)",
          seen.hand[0].min, seen.hand[0].max);

    T.gear = 12; simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 0 && offset_of(out) < 0, "other gear: the fight without a simulation");


    T.gear = 11; T.c.capture_bad = 0; T.c.ar_base_known = 1; T.c.hand[0].min = 2; T.c.hand[0].max = 5; T.players = 1; T.c.belt[0] = 4;
    simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 1 && seen_t.ar == 160 && seen.belt[0] == 4, "complete capture, one player (enemy attack rating %d, belt %d)", seen_t.ar, seen.belt[0]);
    T.c.capture_bad = 2; T.c.ar_base_known = 0; T.c.hand[0].min = T.c.hand[0].max = 0; T.players = 3; T.c.belt[0] = 1;
    simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 1 && seen.hand[0].min == 2 && seen.ar_base_known, "the missing weapon values come from the complete capture (%d)", seen.hand[0].min);
    CHECK(seen_t.ar == (int)(160 * (1 + 2 / 16.0)) && seen.belt[0] == 1, "the party size and the belt of now (enemy attack rating %d, belt %d)", seen_t.ar, seen.belt[0]);

    T.session = 2; simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 0, "an incomplete capture of another session: no stand-in, no simulation");
    T.session = 0; T.set = 1; simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 0, "an incomplete capture of another rule set: no stand-in, no simulation");
    T.set = 0; T.c.capture_bad = 0; T.c.ar_base_known = 1; T.c.hand[0].min = 2; T.c.hand[0].max = 5;


    D.skill[RIGHT] = &S_RIGHT; D.skill[SUMMON] = &S_SUMMON; T.c.bar[2] = SUMMON; T.c.nbar = 3;
    record(&f, (const uint16_t[]){ 1 }, 1, build); f.h.dealt[1] = 30;
    { dm_fl_state *s = dm_fl_core(); pthread_mutex_lock(&s->mu); s->ring[0] = f; pthread_mutex_unlock(&s->mu); }
    simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 1 && seen.nbar == 1 && seen.bar[0] == SUMMON, "minion damage: the bar's summon is simulated beside the normal attack (%d skills, first %d)",
          seen.nbar, seen.bar[0]);

    record(&f, (const uint16_t[]){ 1, SUMMON + 1 }, 2, build); f.h.dealt[1] = 30;
    { dm_fl_state *s = dm_fl_core(); pthread_mutex_lock(&s->mu); s->ring[0] = f; pthread_mutex_unlock(&s->mu); }
    simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 1 && seen.nbar == 1 && seen.bar[0] == SUMMON, "the summon cast in the fight once (%d skills)", seen.nbar);

    record(&f, (const uint16_t[]){ 1 }, 1, build);
    simulated = 0; dm_fl_ui_query(0, -1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 1 && seen.nbar == 0, "no minion damage: only the normal attack (%d skills)", seen.nbar);



    record_group(&f, build);
    simulated = 0; n = dm_fl_ui_query(0, 1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(n > 0 && simulated == 1 && seen_t.mon == MON && seen_t.hp == 500 && group_of_sim(out) == 1, "the group's single enemy (target %d, life %d, group %d)", seen_t.mon, seen_t.hp,
          group_of_sim(out));
    CHECK(offset_of(out) == 200, "the simulated fight starts at the group's first contact, after the player's first attack (offset %d)", offset_of(out));
    for (int g = -1; g <= 2; g += 3) {
        simulated = 0; n = dm_fl_ui_query(0, g, dm_fl_fight_key(&f), 1, out, sizeof out);
        CHECK(n > 0 && simulated == 0 && group_of_sim(out) == -9, "no simulation for group %d of the group fight", g);
    }
    simulated = 0; n = dm_fl_ui_query(0, 0, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(n > 0 && simulated == 0, "no simulation for a group of three");
    record_group(&f, build + 1);
    simulated = 0; dm_fl_ui_query(0, 1, dm_fl_fight_key(&f), 1, out, sizeof out);
    CHECK(simulated == 0, "other gear: no simulation for the group either");
    record_group(&f, build);
    n = dm_fl_ui_query(-1, -1, 0, 0, out, sizeof out);
    { int sims[3] = { -1, -1, -1 };
      for (char *l = strstr(out, "\ng\t"); l; l = strstr(l + 1, "\ng\t")) {
          int idx, g; char key[32]; unsigned v[8]; int others, sim;
          if (sscanf(l + 3, "%d\t%d\t%31s\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d", &idx, &g, key, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &others, &sim) == 13 && g >= 0 && g < 3) sims[g] = sim;
      }
      CHECK(sims[0] == 0 && sims[1] == 1 && sims[2] == 0, "the list marks the single enemy's group (%d %d %d)", sims[0], sims[1], sims[2]); }




    { static char out[16384]; dm_fl_fight f; record(&f, (const uint16_t[]){ RIGHT + 1 }, 1, build);
      dm_fl_state *st0 = dm_fl_core(); dm_fl_player pl = { 500, 500, 200, 200, 0, 0, 2, 0 }; dm_fl_begin(st0, 1, &pl);
      dm_fl_parts parts; uint32_t key = dm_fl_state_key(0, &parts);
      f.s.flags = DM_FL_SPLIT_KNOWN; f.s.state = key; f.s.mine = 62; f.s.skill[0] = 62; f.s.t_act = 75; f.s.gact[0] = 75; f.g[0].life = 62; f.g[0].t_first = 50; f.g[0].t_done = 75 + 250;
      f.h.duration = 300; f.h.lead = 50;
      dm_dh *h = dm_dh_core(); dm_dh_reset(h); dm_dh_owner(h, "Hero", NULL, 0);
      #define PUT_RING(ff) do { dm_fl_state *st = dm_fl_core(); pthread_mutex_lock(&st->mu); st->ring[0] = (ff); st->head = 1; st->count = 1; pthread_mutex_unlock(&st->mu); } while (0)
      #define SIMLINE() (dm_fl_ui_query(DM_FL_QUERY_DPS, -1, 0, 0, out, sizeof out) > 0 ? strstr(out, "\nsim\t") : (char *)0)
      for (int i = 0; i < 5; i++) dm_dh_add(h, key, &parts, &f);
      PUT_RING(f); sim_ttk = 10;
      int n = dm_fl_ui_query(DM_FL_QUERY_DPS, -1, 0, 0, out, sizeof out); double sv = 0, reached = 0; int count = 0;
      const char *p = strstr(out, "\nsim\t");
      CHECK(n > 0 && p && sscanf(p + 5, "%lf\t%lf\t%d", &sv, &reached, &count) == 3 && reached == 1000 && count == 1 && sv == 620, "the same time on both sides reaches 100 %% (%s)", out);
      CHECK(!strncmp(out, "dps\t5\t1500\t517\t517\t517\t517\t517\t517\n", 34), "the window: 5 fights, 62 points in 12 s each (%.40s)", out);
      CHECK(strstr(out, "\nsrc\ts\t395\t-\t310\t517\t517\t517\t1000\n") != NULL, "one source with its range and all of the share");
      sim_ttk = 5; SIMLINE(); p = strstr(out, "\nsim\t"); sscanf(p + 5, "%lf\t%lf\t%d", &sv, &reached, &count);
      CHECK(reached == 500, "half the simulated speed is 50 %% (%g)", reached); sim_ttk = 10;
      dm_fl_fight g = f; g.h.end = DM_FL_END_PAUSE; PUT_RING(g); CHECK(!SIMLINE(), "a fight left without the kill: no comparison");
      g = f; g.s.merc = 5; PUT_RING(g); CHECK(!SIMLINE(), "the mercenary dealt damage: no comparison");
      g = f; g.s.comp[0].cls = SUMMON; g.s.comp[0].dealt = 5; PUT_RING(g); CHECK(!SIMLINE(), "a companion dealt damage, even a summon of the bar: no comparison");
      g = f; g.s.unknown = 2; PUT_RING(g); CHECK(!SIMLINE(), "an unknown part: no comparison");
      g = f; g.s.t_act = 0xffff; PUT_RING(g); CHECK(!SIMLINE(), "no own action known: no comparison");
      g = f; g.s.state = key + 1; PUT_RING(g); CHECK(!SIMLINE(), "another state: no comparison");

      g = f; g.s.state = key + 1;
      for (int i = 0; i < 4; i++) dm_dh_add(h, key + 1, &(dm_fl_parts){ 2, 1, 1, 7, 0, 0 }, &g);
      dm_fl_ui_query(DM_FL_QUERY_DPS, -1, 0, 0, out, sizeof out);
      CHECK(strstr(out, "\nseg\t0\t") && !strstr(out, "\nseg\t1\t"), "four fights of a new state: no point of its own yet");
      dm_dh_add(h, key + 1, &(dm_fl_parts){ 2, 1, 1, 7, 0, 0 }, &g); dm_fl_ui_query(DM_FL_QUERY_DPS, -1, 0, 0, out, sizeof out);
      CHECK(strstr(out, "\nseg\t1\t1500\t5\t") != NULL, "the fifth fight shows it (%s)", out);

      CHECK(!strncmp(out, "dps\t5\t1500\t517\t", 14), "the window of the state of now, not of the last fight's (%.30s)", out);
      T.gear = 99; dm_fl_ui_query(DM_FL_QUERY_DPS, -1, 0, 0, out, sizeof out); T.gear = 11;
      CHECK(!strncmp(out, "dps\t0\t0\t", 8), "a state of now without fights: none (%.30s)", out);

      { T.gear = 55; dm_fl_parts p3; uint32_t k3 = dm_fl_state_key(0, &p3); dm_fl_fight e = f;
        e.s.state = k3; e.h.skill[0] = 397; e.h.skill[1] = 398; e.h.skill[2] = 399; e.s.skill[0] = e.s.skill[1] = e.s.skill[2] = 20; e.s.mine = 60;
        dm_dh_add(h, k3, &p3, &e); dm_fl_ui_query(DM_FL_QUERY_DPS, -1, 0, 0, out, sizeof out); T.gear = 11;
        int sum = 0, n34 = 0, n33 = 0; for (char *l = strstr(out, "\nsrc\t"); l; l = strstr(l + 1, "\nsrc\t")) {
            char kind; unsigned id; char key[32]; unsigned long long dealt; double dps, lo, hi, share;
            if (sscanf(l + 5, "%c\t%u\t%31s\t%llu\t%lf\t%lf\t%lf\t%lf", &kind, &id, key, &dealt, &dps, &lo, &hi, &share) == 8) { sum += (int)share; n34 += share == 340; n33 += share == 330; } }
        CHECK(sum == 1000 && n34 == 1 && n33 == 2, "three equal sources: 34, 33 and 33 %% (%d)", sum); }
      #undef SIMLINE
      #undef PUT_RING
    }



    { static char out[16384]; dm_fl_state *st = dm_fl_core(); dm_fl_player pl = { 500, 500, 200, 200, 0, 0, 2, 0 }; int kept = 0;
      dm_fl_reset(st); dm_fl_game(st, 1000); dm_fl_parts parts; dm_fl_begin(st, 1000, &pl); uint32_t key = dm_fl_state_key(0, &parts);
      dm_fl_build(st, build, key);
      #define HIT(id, cls, before, after, side, sk) do { dm_fl_hit hh = { id, cls, before, after, 0xffff, side, 0, .skill = (sk), .by = 0xffff }; dm_fl_on_hit(st, &hh, before, 30 * 256); } while (0)
      for (uint32_t t = 1000; t <= 1200; t++) {
          dm_fl_begin(st, t, &pl);
          if (t == 1030) HIT(2, MON, 500 * 256, 495 * 256, DM_FL_IN_PLAYER, -1);
          if (t == 1040) { HIT(1, 5, 10 * 256, 0, DM_FL_OUT_PLAYER, RIGHT); HIT(3, 5, 10 * 256, 0, DM_FL_OUT_PLAYER, RIGHT); }
          if (t == 1075) HIT(2, MON, 30 * 256, 20 * 256, DM_FL_OUT_PLAYER, RIGHT);
          if (t == 1100) HIT(2, MON, 20 * 256, 10 * 256, DM_FL_OUT_PLAYER, RIGHT);
          if (t == 1125) HIT(2, MON, 10 * 256, 0, DM_FL_OUT_PLAYER, RIGHT);
          if (t == 1031 || t == 1050 || t == 1075 || t == 1100) dm_fl_action(st, t, DM_FL_ME_CAST, RIGHT);
          if (dm_fl_end(st, &kept)) break;
      }
      #undef HIT
      dm_fl_fight f; CHECK(kept && dm_fl_get(st, 0, &f) && f.h.end == DM_FL_END_KILL && f.h.ngroups == 2, "the fight ended with the kill (%d groups)", f.h.ngroups);
      int gb = f.g[0].cls == MON ? 0 : 1;
      CHECK(f.s.gact[gb] == f.g[gb].t_first + 20 && f.g[gb].t_done - f.s.gact[gb] == 75, "the strong one from the action on it, 3 s before its kill (%u, contact %u, kill %u)",
            f.s.gact[gb], f.g[gb].t_first, f.g[gb].t_done);
      dm_dh *h = dm_dh_core(); dm_dh_reset(h); dm_dh_owner(h, "Hero", NULL, 0); dm_dh_add(h, key, &parts, &f);
      sim_ttk = 3; int n = dm_fl_ui_query(DM_FL_QUERY_DPS, -1, 0, 0, out, sizeof out); double sv = 0, reached = 0; int count = 0;
      const char *p = n > 0 ? strstr(out, "\nsim\t") : NULL;
      CHECK(p && sscanf(p + 5, "%lf\t%lf\t%d", &sv, &reached, &count) == 3 && reached == 1000 && count == 1, "three hits 1, 2 and 3 s after the action: 100 %% (%s)", out);
      n = dm_fl_ui_query(0, gb, dm_fl_fight_key(&f), 1, out, sizeof out);
      p = n > 0 ? strstr(out, "\nsim\t") : NULL; int kind, end, width, nb, offset, grp; double dps = 0;
      double own = f.s.gmine[gb] * 25.0 / (f.g[gb].t_done - f.g[gb].t_first);
      CHECK(p && sscanf(p + 5, "%d\t%d\t%d\t%d\t%lf\t%d\t%d", &kind, &end, &width, &nb, &dps, &offset, &grp) == 7 && offset == f.s.gact[gb] && grp == gb && dps > own - 0.01 && dps < own + 0.01,
            "the opened fight: the simulation from the action, its DPS on the clock of the group's own DPS (%g against %g)", dps, own);
      sim_ttk = 10;
    }
    printf("t_fightlog_sim: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
