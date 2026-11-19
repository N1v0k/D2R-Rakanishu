




#include "../dm_fightlog_core.c"
#include "../dm_fightlog_store.c"
#include "../dm_fightlog_text.c"
#include "../dm_fightlog_hist.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

uint64_t now_ns(void) { static uint64_t t; return t += 60ull * 1000000000ull; }
static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)


enum { NE = 320, P = 256, MELEE = 4, CAST = 7 };

typedef struct { uint32_t id, cls; uint8_t rank; int life, max, dead, gone, attacking, dot; dm_fl_dot owners; } enemy;
static enemy W[NE]; static int nw;
static dm_fl_hit Q[128]; static int nq;

static struct { uint32_t id; int before, after; dm_fl_dot d; } DW[512]; static int ndw;
static dm_fl_player pl = { 500, 500, 200, 200, 0, 0, 2, 0 };
static uint32_t now = 1000;
static dm_fl_state *S;
static int ended, kept, pend_block, pend_avoid, pend_potion = -1, pend_cls;

static enemy *spawn(uint32_t cls, uint8_t rank, int life) { enemy *e = &W[nw]; *e = (enemy){ 100 + (uint32_t)nw, cls, rank, life * P, life * P, 0, 0, 0, 0, { 0 } }; nw++; return e; }
static enemy *by_id(uint32_t id) { for (int i = 0; i < nw; i++) if (W[i].id == id) return &W[i]; return NULL; }

static int game_after(int before, int damage) { int after = before - damage; return after < P ? 0 : after; }

static void hit(enemy *e, int points, int minion, int crit) {
    int after = game_after(e->life, points * P);
    Q[nq++] = (dm_fl_hit){ e->id, e->cls, e->life, after, 0xffff, (uint8_t)(minion ? DM_FL_OUT_MINION : DM_FL_OUT_PLAYER), e->rank, (uint8_t)crit, .skill = -1, .by = 0xffff };
    e->life = after; if (!e->life) e->dead = 1;
}
static void hurt(enemy *e, int points) {
    int after = game_after(pl.hp * P, points * P);
    Q[nq++] = (dm_fl_hit){ e->id, e->cls, pl.hp * P, after, 0xffff, DM_FL_IN_PLAYER, e->rank, 0 }; pl.hp = after / P; if (!after) pl.dead = 1;
}
static void hurt_minion(enemy *e) { Q[nq++] = (dm_fl_hit){ e->id, e->cls, 100 * P, 95 * P, 0xffff, DM_FL_IN_MINION, e->rank, 0 }; }

static int tick(void) {
    now++;
    dm_fl_begin(S, now, &pl);
    if (pend_block) dm_fl_block(S, now);
    if (pend_avoid) dm_fl_avoided(S, pend_avoid);
    if (pend_potion >= 0) dm_fl_potion(S, now, pend_potion, pend_cls);
    pend_block = pend_avoid = 0; pend_potion = -1;
    for (int i = 0; i < nq; i++) { enemy *e = by_id(Q[i].enemy); dm_fl_on_hit(S, &Q[i], e ? e->life : 0, e ? e->max : 0); }
    nq = 0;
    for (int i = 0; i < ndw; i++) dm_fl_dot_write(S, DW[i].id, DW[i].before, DW[i].after, &DW[i].d);
    ndw = 0;
    if (now % DM_FL_SCAN == 0) for (int i = 0; i < nw; i++) if (W[i].dot && !W[i].dead && !W[i].gone) dm_fl_on_dot(S, W[i].id, W[i].cls, W[i].rank, 0xffff, 0, W[i].life, W[i].max);
    for (int i = 0; i < dm_fl_slots(S); i++) {
        uint32_t id = dm_fl_slot_id(S, i); if (!id) continue;
        enemy *e = by_id(id); int life = now % DM_FL_SAMPLE ? -1 : e->life;
        dm_fl_slot_seen(S, i, e->gone, e->dead, e->attacking, life, life >= 0 && dm_fl_slot_unexplained(S, i, life) > 0 && e->dot);
    }
    if (now % DM_FL_SAMPLE == 0) for (int m = 0; m < dm_fl_members(S); m++) { uint32_t id = dm_fl_member_watch(S, m); enemy *e = id ? by_id(id) : NULL; if (e) dm_fl_member_seen(S, m, e->gone, e->dead); }
    ended = dm_fl_end(S, &kept);
    return ended;
}
static void ticks(int n) { for (int i = 0; i < n; i++) tick(); }

static int settle(void) { for (int i = 0; i <= DM_FL_GRACE && !ended; i++) tick(); return ended; }
static int done(void) { tick(); return settle(); }

static void dot_write(enemy *e, const dm_fl_dot *d) {
    int all = 0; for (int k = 0; k < d->n; k++) all += d->e[k].rate;
    int after = e->life - all; if (after < 0) after = 0;
    DW[ndw].id = e->id; DW[ndw].before = e->life; DW[ndw].after = after; DW[ndw].d = *d; ndw++;
    e->life = after; if (!after) e->dead = 1;
}

static void poison(int points) {
    for (int i = 0; i < nw; i++) if (W[i].dot && !W[i].dead && now % DM_FL_SAMPLE == 0) {
        dm_fl_dot d = W[i].owners; int rel = 0, given = 0;
        if (!d.n) { d.n = 1; d.e[0].kind = DM_FL_DOT_PLAYER; d.e[0].rate = 1; }
        for (int k = 0; k < d.n; k++) rel += d.e[k].rate;
        for (int k = 0; k < d.n; k++) { d.e[k].rate = k == d.n - 1 ? points * P - given : points * P * d.e[k].rate / rel; given += d.e[k].rate; }
        dot_write(&W[i], &d);
    }
}
static void fresh(void) { nw = nq = ndw = 0; pl = (dm_fl_player){ 500, 500, 200, 200, 0, 0, 2, 0 }; dm_fl_reset(S); now += 1000; dm_fl_game(S, now); }
static dm_fl_fight F;
static const dm_fl_fight *newest(void) { static dm_fl_fight f; if (!dm_fl_get(S, 0, &f)) memset(&f, 0, sizeof f); return &f; }

int main(void) {
    S = dm_fl_core();
    fresh();

    { enemy *e = spawn(5, DM_FL_RANK_NORMAL, 100); ticks(100);
      dm_fl_action(S, now, DM_FL_ME_CAST, 36); ticks(5);
      uint32_t first = now + 1; int hits = 0;
      while (!e->dead) { hit(e, e->life > 10 * P ? 10 : 15, 0, hits == 3); tick(); hits++; if (!e->dead) ticks(9); }
      uint32_t kill = now; CHECK(!ended && dm_fl_open(S), "the kill waits the grace for another enemy"); settle();
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && kept && now - kill == DM_FL_GRACE, "kill ends the fight after the grace and keeps it (end %d kept %d after %u)", ended, kept, now - kill);
      CHECK(f->h.duration == kill - first && f->h.lead == DM_FL_LEAD, "duration %u = first hit to kill, lead %u", f->h.duration, f->h.lead);
      CHECK(f->h.dealt[0] == 100 && f->h.dealt[1] == 0 && f->h.dealt[2] == 0, "dealt without overkill %u", f->h.dealt[0]);
      CHECK(f->h.kills == 1 && f->h.enemies == 1 && f->h.ngroups == 1 && f->g[0].cls == 5 && f->g[0].kills == 1 && f->g[0].dealt == 100 && f->g[0].life == 100, "one group, one kill");
      CHECK(f->g[0].t_first == DM_FL_LEAD && f->g[0].t_done == f->h.duration + DM_FL_LEAD, "group times %u %u", f->g[0].t_first, f->g[0].t_done);
      int cast_at = (DM_FL_LEAD - 6) / f->h.width;
      CHECK((f->b[cast_at].me & DM_FL_ME_CAST) && f->b[cast_at].skill == 1 && f->h.skill[0] == 37, "the cast before the hit is in the lead (bucket %d me %#x)", cast_at, f->b[cast_at].me);
      CHECK(f->b[0].last[DM_FL_S_ENEMY] == 255 && f->b[f->h.nbuckets - 1].last[DM_FL_S_ENEMY] == 0, "enemy series full to empty");
      int crit = 0; for (int k = 0; k < f->h.nbuckets; k++) crit += !!(f->b[k].me & DM_FL_ME_CRIT);
      CHECK(crit == 1, "one bucket with the critical hit");
      CHECK(f->h.nbuckets == (f->h.duration + DM_FL_LEAD) / f->h.width + 1 && f->h.nbuckets <= DM_FL_BUCKETS, "bucket count %u width %u", f->h.nbuckets, f->h.width);
      CHECK(dm_fl_count(S) == 1 && !memcmp(newest(), f, sizeof *f), "the fight is in the ring"); }



    fresh(); { enemy *e = spawn(5, 0, 100); ticks(100);
      uint32_t first = now + 1;
      for (int i = 0; i < 10; i++) { hit(e, 10, 1, 0); tick(); if (!e->dead) ticks(9); }
      uint32_t kill = now; settle(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && kept && f->h.duration == kill - first && f->h.duration == 90, "minion fight: duration %u from the minions' first hit", f->h.duration);
      CHECK(f->h.dealt[0] == 0 && f->h.dealt[1] == 100 && f->h.dealt[2] == 0 && f->h.lead == DM_FL_LEAD, "minion damage %u/%u/%u lead %u", f->h.dealt[0], f->h.dealt[1], f->h.dealt[2], f->h.lead);
      CHECK(f->g[0].t_first == DM_FL_LEAD && f->g[0].t_done == DM_FL_LEAD + 90, "minion group times %u %u", f->g[0].t_first, f->g[0].t_done); }

    fresh(); { enemy *e = spawn(5, 0, 100); ticks(100);
      uint32_t first = now + 1;
      for (int i = 0; i < 3; i++) { hit(e, 10, 1, 0); tick(); ticks(9); }
      dm_fl_action(S, now, DM_FL_ME_ATTACK, 0);
      while (!e->dead) { hit(e, 20, 0, 0); tick(); if (!e->dead) ticks(9); }
      uint32_t kill = now; settle(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.duration == kill - first && f->h.dealt[0] == 70 && f->h.dealt[1] == 30, "mixed: duration %u (want %u) player %u minions %u", f->h.duration, kill - first, f->h.dealt[0], f->h.dealt[1]); }

    fresh(); { enemy *a = spawn(5, 0, 3); ticks(100);
      hit(a, 8, 1, 0); done();
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && kept && f->h.duration == 0 && f->h.dealt[1] == 3 && f->h.kills == 1 && f->g[0].t_done == DM_FL_LEAD, "one-hit kill by a minion: duration %u", f->h.duration); }


    fresh(); { enemy *e[5]; const uint32_t *ids; int streak = 0; for (int k = 0; k < 5; k++) e[k] = spawn(5, 0, 3); ticks(100);
      uint32_t first = now + 1, kill = 0;
      for (int k = 0; k < 5; k++) { hit(e[k], 8, 1, 0); tick(); kill = now; streak += dm_fl_kills(S, now, &ids); CHECK(!ended, "kill %d: the fight goes on", k + 1); if (k < 4) ticks(DM_FL_TPS - 1); }
      settle(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && kept && dm_fl_count(S) == 1 && f->h.enemies == 5 && f->h.kills == 5, "one fight of five (fights %d enemies %u kills %u)", dm_fl_count(S), f->h.enemies, f->h.kills);
      CHECK(f->h.ngroups == 1 && f->g[0].joined == 5 && f->g[0].kills == 5 && f->h.dealt[1] == 15, "one group of five (%u), minion damage %u", f->g[0].joined, f->h.dealt[1]);
      CHECK(f->h.duration == kill - first && f->h.duration == 4 * DM_FL_TPS, "time from the first hit to the last kill (%u), without the grace", f->h.duration);
      CHECK(f->h.nbuckets == (f->h.duration + DM_FL_LEAD) / f->h.width + 1, "the timeline ends at the last kill (%u buckets)", f->h.nbuckets);
      CHECK(streak == 5, "the kill streak got %d kills", streak); }

    fresh(); { enemy *e[6]; const uint32_t *ids; int streak = 0; for (int k = 0; k < 6; k++) e[k] = spawn(5, 0, 3); ticks(100);
      for (int k = 0; k < 3; k++) { hit(e[k], 8, 1, 0); tick(); streak += dm_fl_kills(S, now, &ids); }
      for (int i = 0; i < 3 * DM_FL_TPS - 1; i++) { tick(); streak += dm_fl_kills(S, now, &ids); }
      CHECK(dm_fl_count(S) == 1 && !dm_fl_open(S) && dm_fl_last(S)->h.kills == 3, "the first pack ended (%d)", dm_fl_count(S));
      for (int k = 3; k < 6; k++) { hit(e[k], 8, 1, 0); tick(); streak += dm_fl_kills(S, now, &ids); }
      settle();
      CHECK(ended == DM_FL_END_KILL && dm_fl_count(S) == 2 && dm_fl_last(S)->h.kills == 3 && dm_fl_last(S)->h.duration == 2, "the second pack is a fight of its own (%d)", dm_fl_count(S));
      CHECK(streak == 6, "the kill streak got %d kills", streak); }

    fresh(); { enemy *a = spawn(5, 0, 3), *b = spawn(5, 0, 3), *c = spawn(5, 0, 3), *d = spawn(5, 0, 3); ticks(100);
      hit(a, 8, 1, 0); tick(); ticks(DM_FL_TPS - 1); hit(b, 8, 1, 0); tick(); ticks(10); pl.dead = 1; tick();
      CHECK(ended == DM_FL_END_DEATH && kept && dm_fl_last(S)->h.kills == 2, "death ends (end %d, kills %u)", ended, dm_fl_last(S)->h.kills);
      pl.dead = 0; pl.hp = 500; ticks(DM_FL_PAUSE);
      uint32_t first = now + 1; hit(c, 8, 1, 0); tick(); ticks(10); pl.area = 1; pl.town = 1; tick();
      CHECK(ended == DM_FL_END_KILL && dm_fl_last(S)->h.duration == 0 && first == now - 11, "town within the grace: a kill at the last kill (end %d, %u)", ended, dm_fl_last(S)->h.duration);
      pl.area = 2; pl.town = 0; ticks(10); hit(d, 8, 1, 0); tick(); ticks(10); dm_fl_game(S, now + 1000);
      CHECK(dm_fl_last(S)->h.end == DM_FL_END_KILL && dm_fl_last(S)->h.kills == 1 && dm_fl_last(S)->h.duration == 0 && dm_fl_count(S) == 3, "leaving the game within the grace: a kill"); now += 1000; }


    fresh(); { enemy *e = spawn(5, 0, 301); ticks(60);
      while (!e->dead) { if (now % 50 == 0) pl.hp = 250; hit(e, 1, 0, 0); tick(); }
      uint32_t kill = now; CHECK(!ended && dm_fl_open(S), "the kill waits the grace");
      dm_fl_action(S, now, 0, 41);
      pend_potion = 2; pend_cls = 7;
      pl.hp = 25; static uint32_t sb[DM_FL_STATE_WORDS]; sb[0] = 1u << 3; dm_fl_states(S, now, sb); tick();
      settle(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && kept && f->h.width >= 2 && f->h.duration == kill - (kill - 300), "kept kill after the grace (width %u, duration %u)", f->h.width, f->h.duration);
      CHECK(f->h.skill[0] == 0, "the skill first used after the kill is not in the record (%u)", f->h.skill[0]);
      CHECK(f->h.nmarks == 1 && f->m[0].kind == DM_FL_MARK_KILL, "no potion or state mark after the end (%u marks)", f->h.nmarks);
      CHECK(f->h.min_life == pct8(250, 500), "the player's life as at the kill (%u, want %u)", f->h.min_life, pct8(250, 500));
      int pot = 0; for (int k = 0; k < f->h.nbuckets; k++) pot += !!(f->b[k].me & DM_FL_ME_POTION);
      CHECK(!pot, "no potion bit in any recorded bucket (last bucket %u me %#x)", f->h.nbuckets - 1, f->b[f->h.nbuckets - 1].me);
      pl.hp = 500; }



    for (int how = 0; how < 3; how++) {
      fresh(); enemy *e[3], *x; for (int k = 0; k < 3; k++) e[k] = spawn(5, 0, 3); x = spawn(6, 0, 50); ticks(100);
      uint32_t first = now + 1, kill = 0;
      for (int k = 0; k < 3; k++) { hit(e[k], 8, 0, 0); tick(); kill = now; if (k < 2) ticks(9); }
      ticks(20);
      if (how == 2) { x->dot = 1; while (!dm_fl_known(S, x->id) && now - kill < DM_FL_GRACE) tick(); x->dot = 0; } else { hurt(x, 5); tick(); }
      CHECK(dm_fl_open(S) && !ended && dm_fl_known(S, x->id), "way %d: the newcomer joins within the grace", how);
      if (how == 1) { ticks(10); pl.area = 3; tick(); pl.area = 2; } else for (int i = 0; i < 2 * DM_FL_PAUSE && !ended; i++) tick();
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && kept && dm_fl_count(S) == 1 && f->h.end == DM_FL_END_KILL && f->h.kills == 3 && f->h.enemies == 3 && f->h.ngroups == 1 && f->h.taken == 0,
            "way %d: still a kept kill of three (end %d kept %d fights %d kills %u enemies %u taken %u)", how, ended, kept, dm_fl_count(S), f->h.kills, f->h.enemies, f->h.taken);
      CHECK(f->h.duration == kill - first && f->h.duration == 20 && f->h.nbuckets == (f->h.duration + DM_FL_LEAD) / f->h.width + 1, "way %d: it ends at the last kill (%u)", how, f->h.duration);
    }
    fresh(); { enemy *e[3], *x, *y; for (int k = 0; k < 3; k++) e[k] = spawn(5, 0, 3); x = spawn(6, 0, 3); y = spawn(6, 0, 50); ticks(100);
      uint32_t first = now + 1;
      for (int k = 0; k < 3; k++) { hit(e[k], 8, 0, 0); tick(); if (k < 2) ticks(9); }
      ticks(20); hurt(x, 5); tick(); ticks(10); hit(x, 8, 0, 0); tick(); uint32_t kill = now; settle();
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && kept && dm_fl_count(S) == 1 && f->h.kills == 4 && f->h.enemies == 4 && f->h.taken == 5 && f->h.duration == kill - first,
            "the newcomer killed: one fight of four (fights %d kills %u enemies %u duration %u)", dm_fl_count(S), f->h.kills, f->h.enemies, f->h.duration);
      first = now + 1;
      for (int k = 0; k < 3; k++) { e[k] = spawn(5, 0, 3); hit(e[k], 8, 0, 0); tick(); if (k < 2) ticks(9); }
      ticks(20); hurt(x = spawn(6, 0, 3), 5); hurt(y, 5); tick(); ticks(10); hit(x, 8, 0, 0); tick(); kill = now;
      for (int i = 0; i < 2 * DM_FL_PAUSE && !ended; i++) tick();
      f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_PAUSE && kept && dm_fl_count(S) == 2 && f->h.kills == 4 && f->h.enemies == 5 && f->h.duration == kill - first && f->h.duration < DM_FL_MIN,
            "one newcomer killed, one not: a short fight ended by the pause, kept (end %d kept %d kills %u enemies %u duration %u)", ended, kept, f->h.kills, f->h.enemies, f->h.duration); }


    fresh(); { enemy *e = spawn(5, 0, 20); ticks(20); hit(e, 20, 0, 0); done();
      CHECK(ended == DM_FL_END_KILL && dm_fl_last(S)->h.lead == 21, "lead clamped to %u", dm_fl_last(S)->h.lead); }


    fresh(); { enemy *e = spawn(7, 0, 300); e->dot = 1; ticks(60);
      for (int i = 0; i < 4 && !ended; i++) { hit(e, 20, 0, 0); hurt(e, 90); tick(); ticks(12); }
      for (int i = 0; i < 60 && !ended; i++) { hurt(e, 90); tick(); }
      CHECK(ended == DM_FL_END_DEATH && kept && dm_fl_last(S)->h.kills == 0, "death ends the fight (end %d)", ended);
      CHECK(dm_fl_last(S)->h.taken == 500 && dm_fl_last(S)->h.min_life == 0, "taken %u min life %u", dm_fl_last(S)->h.taken, dm_fl_last(S)->h.min_life);
      for (int i = 0; i < 300; i++) { poison(5); tick(); }
      CHECK(e->dead && !dm_fl_open(S) && dm_fl_count(S) == 1, "poison after death starts no fight");
      pl = (dm_fl_player){ 500, 500, 200, 200, 0, 1, 1, 0 }; enemy *f2 = spawn(8, 0, 50); f2->dot = 1; ticks(60);
      CHECK(!dm_fl_open(S), "poison while in town starts no fight"); }


    fresh(); { enemy *e = spawn(5, 0, 1000); ticks(60); hit(e, 5, 0, 0); ticks(DM_FL_PAUSE + 1);
      CHECK(dm_fl_last(S)->h.end == DM_FL_END_PAUSE && dm_fl_count(S) == 0, "a stray hit is not kept");
      uint32_t first = now + 1; for (int i = 0; i < 5; i++) { hit(e, 5, 0, 0); tick(); ticks(24); }
      uint32_t lasthit = now - 24; int n = 0; while (!tick() && n++ < 1000) {}
      CHECK(ended == DM_FL_END_PAUSE && kept && dm_fl_last(S)->h.duration == lasthit - first, "pause ends at the last hit (%u vs %u)", dm_fl_last(S)->h.duration, lasthit - first);
      CHECK(now - lasthit == DM_FL_PAUSE, "after 5 s without damage"); }


    fresh(); { enemy *e = spawn(5, 0, 1000); ticks(60); for (int i = 0; i < 4; i++) { hit(e, 5, 0, 0); ticks(25); }
      pl.area = 1; pl.town = 1; tick();
      CHECK(ended == DM_FL_END_AREA && kept && dm_fl_last(S)->h.kills == 0, "town ends the fight (end %d)", ended);
      hit(e, 5, 0, 0); tick(); CHECK(!dm_fl_open(S), "no fight starts in town"); }


    fresh(); { enemy *e = spawn(5, 0, 1000); ticks(60); hit(e, 5, 0, 0); tick(); pl.area = 0; tick(); CHECK(dm_fl_open(S), "area 0 keeps the fight"); pl.area = 2; }


    fresh(); { enemy *a = spawn(5, 0, 50), *b = spawn(5, 0, 400); ticks(60); hit(a, 50, 0, 0); hit(b, 10, 0, 0); tick();
      CHECK(dm_fl_open(S), "one of two dead: the fight goes on");
      ticks(10); a->dead = 0; a->life = a->max; tick(); a->attacking = MELEE; tick(); a->attacking = 0;
      ticks(10); hit(a, 60, 0, 0); tick(); for (int i = 0; i < 4 && !ended; i++) { hit(b, 100, 0, 0); tick(); }
      settle(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.kills == 2 && f->h.enemies == 2 && f->g[0].joined == 2 && f->g[0].kills == 2, "revived enemy counted once (kills %u enemies %u)", f->h.kills, f->h.enemies);
      CHECK(f->h.dealt[0] == 50 + 50 + 400, "dealt %u counts the revived life", f->h.dealt[0]); }


    fresh(); { enemy *e[11]; for (int k = 0; k < 10; k++) e[k] = spawn(10 + k, 0, 40); e[10] = spawn(10, DM_FL_RANK_CHAMPION, 40); ticks(60);
      for (int k = 0; k < 11; k++) { hit(e[k], 10, k & 1, 0); }
      tick();
      for (int r = 0; r < 3; r++) { ticks(5); for (int k = 0; k < 11; k++) hit(e[k], 10, 0, 0); tick(); }
      settle(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.ngroups == DM_FL_GROUPS && (f->h.flags & DM_FL_F_MORE_KINDS), "8 groups (%u), more kinds flag", f->h.ngroups);
      CHECK(f->g[0].cls == 10 && f->g[0].rank == 0 && f->g[0].joined == 1, "first group: kind 10 normal");
      CHECK(f->g[7].flags & DM_FL_G_OTHERS, "the 8th group holds the others");
      CHECK(f->g[7].joined == 4 && f->g[7].kills == 4, "others: kinds 17, 18, 19 and the champion (%u)", f->g[7].joined);
      CHECK(f->h.dealt[0] + f->h.dealt[1] == 440 && f->h.dealt[1] == 50, "player %u + minions %u", f->h.dealt[0], f->h.dealt[1]);
      CHECK(f->glife[0][0] == 255 && f->glife[f->h.nbuckets - 1][0] == 0, "group life from full to empty"); }




    fresh(); { enemy *e[7], *b, *o, *u, *again; for (int k = 0; k < 7; k++) e[k] = spawn(20 + k, 0, 40);
      b = spawn(90, 0, 300); o = spawn(30, 0, 40); u = spawn(31, DM_FL_RANK_UNIQUE, 300); again = spawn(25, 0, 40); ticks(60);
      for (int k = 0; k < 7; k++) hit(e[k], 10, 0, 0);
      tick(); uint32_t first = now; ticks(9); hit(b, 10, 0, 0); Q[nq - 1].boss = 1; tick(); uint32_t t_boss = now;
      CHECK(S->f.h.ngroups == 8 && S->f.g[7].cls == 90 && !(S->f.g[7].flags & DM_FL_G_OTHERS), "the boss is the eighth group");
      ticks(9); hit(o, 10, 0, 0); tick();
      ticks(9); hit(u, 10, 0, 0); tick();
      ticks(9); hit(again, 10, 0, 0); tick();
      for (int r = 0; r < 3; r++) { ticks(5); for (int k = 0; k < 7; k++) hit(e[k], 10, 0, 0); hit(o, 10, 0, 0); hit(again, 10, 0, 0); tick(); }
      ticks(5); hit(b, 290, 0, 0); tick(); ticks(5); hit(u, 290, 0, 0); done();
      const dm_fl_fight *f = dm_fl_last(S); int bg = -1, ug = -1, joined = 0, kills = 0; uint32_t dealt = 0;
      for (int g = 0; g < f->h.ngroups; g++) {
          if (f->g[g].cls == 90) bg = g;
          if (f->g[g].cls == 31) ug = g;
          joined += f->g[g].joined; kills += f->g[g].kills; dealt += f->g[g].dealt;
      }
      CHECK(ended == DM_FL_END_KILL && f->h.ngroups == 8 && (f->g[7].flags & DM_FL_G_OTHERS) && bg >= 0 && bg < 7 && ug >= 0 && ug < 7,
            "the boss (%d) and the unique (%d) keep groups of their own", bg, ug);
      CHECK(f->g[bg].joined == 1 && f->g[bg].kills == 1 && f->g[bg].dealt == 300 && f->g[ug].joined == 1 && f->g[ug].dealt == 300, "the elites' own counts");
      CHECK(f->g[7].joined == 4 && f->g[7].kills == 4 && f->g[7].dealt == 160, "others: two ordinary kinds moved in, the ninth and its later member (%u, %u)", f->g[7].joined, f->g[7].dealt);
      CHECK(joined == 11 && kills == 11 && dealt == f->h.dealt[0] && f->h.enemies == 11, "sums kept (%d joined, %d kills, %u dealt of %u)", joined, kills, dealt, f->h.dealt[0]);
      uint32_t t0 = first - f->h.lead; int nb = f->h.nbuckets, kb = (int)(t_boss - t0) / f->h.width;
      CHECK(f->g[bg].t_first == t_boss - t0, "the boss's first contact (%u, want %u)", f->g[bg].t_first, t_boss - t0);
      CHECK(f->glife[0][bg] == 255 && f->glife[kb][bg] < 255 && f->glife[nb - 1][bg] == 0 && f->glife[nb - 1][7] == 0, "the boss's life line from full to empty");
      int marks = 0; for (int i = 0; i < f->h.nmarks; i++) if (f->m[i].kind == DM_FL_MARK_KILL) { marks++; CHECK(f->g[f->m[i].a].t_done && f->m[i].t >= f->g[f->m[i].a].t_first, "kill mark %d on a finished group", i); }
      CHECK(marks >= 3, "kill marks (%d)", marks); }


    fresh(); { for (int k = 0; k < 70; k++) spawn(30, 0, 20); ticks(60);
      for (int k = 0; k < 70; k++) { hit(&W[k], 5, 0, 0); }
      tick();
      CHECK(dm_fl_open(S) && (S->f.h.flags & DM_FL_F_TABLE_FULL), "table full flagged");
      ticks(5); for (int k = 0; k < 70; k++) hit(&W[k], 15, 0, 0); done();
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.enemies == 70 && f->h.kills == 70 && f->g[0].tracked == 64 && f->h.life_total == 64 * 20, "70 enemies, 64 tracked (%u), all killed", f->g[0].tracked);
      CHECK(f->h.dealt[0] == 70 * 20, "dealt %u", f->h.dealt[0]); }


    fresh(); { for (int k = 0; k < 64; k++) spawn(30, 0, 10); enemy *keep = spawn(31, 0, 5000); ticks(60);
      for (int k = 0; k < 64; k++) { hit(&W[k], 10, 0, 0); }
      hit(keep, 1, 0, 0); tick();
      for (int k = 0; k < 10; k++) { spawn(32, 0, 10); }
      for (int k = 65; k < 75; k++) { hit(&W[k], 1, 0, 0); }
      tick();
      CHECK(dm_fl_open(S) && !(S->f.h.flags & DM_FL_F_TABLE_FULL), "slots of the dead are reused");
      int tracked = 0; for (int g = 0; g < S->f.h.ngroups; g++) tracked += S->f.g[g].tracked;
      CHECK(tracked == 75, "75 enemies had life followed (%d)", tracked); }


    fresh(); { enemy *e = spawn(40, 0, 60); ticks(60); while ((now + 1) % DM_FL_SCAN) tick();
      e->dot = 1;
      CHECK(!dm_fl_open(S), "no fight before the pass"); tick();
      CHECK(dm_fl_open(S) && (S->f.h.flags & DM_FL_F_DOT_START), "the pass starts a poison fight");
      for (int i = 0; i < 400 && !ended; i++) { poison(2); tick(); }
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.dealt[2] == 60 && f->h.kills == 1, "poison kill, damage over time %u", f->h.dealt[2]);
      int dot = 0; for (int k = 0; k < f->h.nbuckets; k++) dot += !!(f->b[k].me & DM_FL_ME_DOT);
      CHECK(dot > 5, "damage over time marked in the timeline (%d buckets)", dot); }


    fresh(); { enemy *e = spawn(41, 0, 10000); ticks(60); for (int i = 0; i < 4; i++) { hit(e, 5, 0, 0); ticks(25); } pl.area = 3; tick(); pl.area = 2;
      CHECK(ended == DM_FL_END_AREA, "area change ends"); e->dot = 1;
      for (int i = 0; i < DM_FL_PAUSE - 5; i++) tick();
      CHECK(!dm_fl_open(S), "no restart by poison within 5 s");
      ticks(DM_FL_SCAN + 5); CHECK(dm_fl_open(S), "after 5 s the poison starts a new fight"); }




    for (int pass = 0; pass < 2; pass++) {
      fresh(); enemy *e = spawn(5, 0, 100000); pend_avoid = DM_FL_EN_MISS; tick(); ticks(59); hit(e, 1, 0, 0); tick();
      for (int i = 0; i < (pass ? 12 : 0); i++) { ticks(49); hit(e, 1, 0, 0); tick(); }
      ticks(10); e->attacking = MELEE; tick(); uint32_t t_atk = now; ticks(3); hurt(e, 10); tick(); uint32_t t_hit = now; ticks(4); e->attacking = 0; tick();
      ticks(10); e->attacking = MELEE; tick(); uint32_t t_plain = now; ticks(8); e->attacking = 0; tick();
      ticks(10); e->attacking = MELEE; tick(); ticks(3); pend_avoid = DM_FL_EN_MISS; tick(); uint32_t t_miss = now; ticks(4); e->attacking = 0; tick();
      ticks(10); e->attacking = MELEE; tick(); ticks(3); pend_avoid = DM_FL_EN_DODGE; tick(); uint32_t t_dodge = now; ticks(4); e->attacking = 0; tick();
      ticks(10); e->attacking = MELEE; tick(); uint32_t t_block = now; ticks(3); pend_block = 1; tick(); ticks(4); e->attacking = 0; tick();
      ticks(10); e->attacking = MELEE; tick(); uint32_t t_merc = now; ticks(3); hurt_minion(e); tick(); uint32_t t_merc_hit = now; ticks(4); e->attacking = 0; tick();
      ticks(10); e->attacking = CAST; tick(); uint32_t t_cast = now; ticks(8); e->attacking = 0; tick();
      pend_potion = 2; pend_cls = 3; tick(); uint32_t t_potion = now;
      pl.dead = 1; tick(); const dm_fl_fight *f = dm_fl_last(S); int w = f->h.width, miss = 0, dodge = 0;
      for (int k = 0; k < f->h.nbuckets; k++) { miss += !!(f->b[k].en & DM_FL_EN_MISS); dodge += !!(f->b[k].en & DM_FL_EN_DODGE); }
      #define BK(t) f->b[((t) - (now - f->h.duration - f->h.lead)) / w]
      CHECK(ended == DM_FL_END_DEATH && w == (pass ? 4 : 1), "pass %d: width %d", pass, w);
      CHECK((BK(t_atk).en & DM_FL_EN_ATTACK) && (BK(t_hit).en & DM_FL_EN_HIT) && (BK(t_plain).en & DM_FL_EN_ATTACK), "pass %d: attacks and the hit", pass);
      CHECK((BK(t_miss).en & DM_FL_EN_MISS) && (BK(t_dodge).en & DM_FL_EN_DODGE), "pass %d: the reported miss and dodge", pass);
      CHECK(BK(t_block + 4).en & DM_FL_EN_BLOCK, "pass %d: the block", pass);
      CHECK((BK(t_merc_hit).en & DM_FL_EN_MINION_HIT) && !(BK(t_merc_hit).en & DM_FL_EN_HIT) && (BK(t_merc).en & DM_FL_EN_ATTACK), "pass %d: a hit on the mercenary", pass);
      CHECK((BK(t_cast).en & DM_FL_EN_ATTACK), "pass %d: the cast", pass);
      CHECK(miss == 1 && dodge == 1, "pass %d: exactly one miss bucket (%d) and one dodge bucket (%d)", pass, miss, dodge);
      CHECK(f->h.nmarks == 1 && f->m[0].kind == DM_FL_MARK_POTION && f->m[0].a == 2 && f->m[0].b == 3 && (BK(t_potion).me & DM_FL_ME_POTION), "pass %d: potion mark", pass);
      uint32_t lost, hits; dm_fl_player_loss(S, &lost, &hits); CHECK(hits == 10 && lost == 10 && f->h.taken == 10, "pass %d: player loss %u, hits %u", pass, lost, hits);
      #undef BK
    }


    fresh(); { enemy *a = spawn(7, 0, 300), *b = spawn(8, 0, 300); pl.hp = 50; ticks(60);
      hurt(a, 40); hurt(b, 40); hurt(b, 30); tick();
      const dm_fl_fight *f = dm_fl_last(S); uint32_t lost, hits; dm_fl_player_loss(S, &lost, &hits);
      CHECK(ended == DM_FL_END_DEATH && kept && f->h.duration == 0 && f->h.enemies == 2, "fatal first hits make a death (end %d, enemies %u)", ended, f->h.enemies);
      CHECK(f->h.taken == 50 && hits == 50 && lost == 50 && f->h.min_life == 0, "taken %u hits %u lost %u", f->h.taken, hits, lost);
      hurt(a, 10); tick(); CHECK(!dm_fl_open(S), "no fight while the player stays dead"); }


    fresh(); { enemy *e = spawn(5, 0, 21); e->life = 20 * P + 100; ticks(60); hit(e, 20, 0, 0); done();
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.dealt[0] == 20 && f->h.life_total == 21, "dealt %u of the life left, life total %u", f->h.dealt[0], f->h.life_total);
      e = spawn(5, 0, 10); ticks(60); hit(e, 4, 0, 0); tick(); ticks(5); hit(e, 50, 1, 0); done(); f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.dealt[0] == 4 && f->h.dealt[1] == 6 && f->g[0].dealt == 10, "player %u minion %u group %u", f->h.dealt[0], f->h.dealt[1], f->g[0].dealt); }


    fresh(); { enemy *a = spawn(5, 0, 30), *b = spawn(6, 0, 200); ticks(60); hit(a, 10, 0, 0); hit(b, 50, 0, 0); tick(); b->dot = 1; ticks(10);
      hit(a, 20, 0, 0); tick(); CHECK(dm_fl_open(S), "the poisoned one is still there"); ticks(10);
      b->dead = 1; b->gone = 1; done(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.kills == 1 && f->h.dealt[2] == 0 && f->h.dealt[0] == 80 && f->g[1].kills == 0, "kills %u, damage over time %u, dealt %u", f->h.kills, f->h.dealt[2], f->h.dealt[0]); }


    fresh(); { for (int k = 0; k < 70; k++) spawn(30, 0, 20); ticks(60);
      for (int k = 0; k < 70; k++) hit(&W[k], 5, 0, 0);
      tick(); for (int k = 64; k < 70; k++) W[k].dot = 1;
      ticks(5); for (int k = 0; k < 64; k++) hit(&W[k], 15, 0, 0); tick();
      CHECK(dm_fl_open(S) && S->alive == 6, "the 6 without a slot are alive (%d)", S->alive);
      enemy *late[3]; for (int k = 0; k < 3; k++) { late[k] = spawn(31, 0, 30); hit(late[k], 10, 0, 0); } tick();
      int tracked = 0; for (int g = 0; g < S->f.h.ngroups; g++) tracked += S->f.g[g].tracked;
      CHECK(tracked == 67, "the late ones took freed slots (%d)", tracked);
      for (int i = 0; i < 200 && !ended; i++) { poison(2); tick(); if (i == 20) for (int k = 0; k < 3; k++) hit(late[k], 20, 0, 0); }
      const dm_fl_fight *f = dm_fl_last(S);
      CHECK(ended == DM_FL_END_KILL && f->h.kills == 73 && f->h.enemies == 73, "all 73 killed (end %d, kills %u)", ended, f->h.kills);
      CHECK(f->h.dealt[0] == 64 * 20 + 6 * 5 + 3 * 30 && f->h.dealt[2] == 0, "dealt %u, damage over time %u", f->h.dealt[0], f->h.dealt[2]); }


    fresh(); { uint32_t st[DM_FL_STATE_WORDS] = { 0 }; st[0] = 1u << 8 | 1u << 9; dm_fl_states(S, now, st);
      enemy *e = spawn(5, 0, 1000); ticks(60); hit(e, 1, 0, 0); tick(); ticks(30);
      st[0] = 1u << 8; st[1] = 1u << 2; dm_fl_states(S, now + 1, st); tick(); ticks(30); hit(e, 999, 0, 0); done();
      const dm_fl_fight *f = dm_fl_last(S); int off = 0, on = 0, kill = 0;
      for (int i = 0; i < f->h.nmarks; i++) { off += f->m[i].kind == DM_FL_MARK_STATE_OFF && f->m[i].b == 9; on += f->m[i].kind == DM_FL_MARK_STATE_ON && f->m[i].b == 34; kill += f->m[i].kind == DM_FL_MARK_KILL; }
      CHECK(f->h.nmarks == 3 && off == 1 && on == 1 && kill == 1, "marks: state 9 off, 34 on, the kill (%u marks)", f->h.nmarks); }


    fresh(); { enemy *e = spawn(5, 0, 100000); ticks(60); for (int i = 0; i < 300; i++) { hit(e, 10, 0, 0); ticks(5); }
      pl.area = 9; tick(); const dm_fl_fight *f = dm_fl_last(S);
      CHECK(f->h.width == 8 && f->h.nbuckets == (f->h.duration + DM_FL_LEAD) / 8 + 1 && f->h.nbuckets <= DM_FL_BUCKETS, "width %u buckets %u", f->h.width, f->h.nbuckets);
      CHECK(f->b[0].lo[DM_FL_S_ENEMY] == 255 && f->b[f->h.nbuckets - 1].last[DM_FL_S_ENEMY] < 255, "enemy series falls"); pl.area = 2; }


    fresh(); { enemy *e = spawn(5, 0, 1000); ticks(60); for (int i = 0; i < 4; i++) { hit(e, 5, 0, 0); ticks(25); }
      dm_fl_game(S, 5); CHECK(dm_fl_last(S)->h.end == DM_FL_END_LEFT && dm_fl_count(S) == 1, "leaving the game ends the fight");
      now = 5; e = spawn(5, 0, 1000); ticks(60); hit(e, 5, 0, 0); tick(); dm_fl_discard(S); CHECK(!dm_fl_open(S) && dm_fl_count(S) == 1, "switched off: dropped"); }


    fresh(); { for (int n = 0; n < 55; n++) { enemy *e = spawn(5, 0, 10 + n); ticks(30); hit(e, 10 + n, 0, 0); done(); }
      CHECK(dm_fl_count(S) == DM_FL_RING, "ring holds %d", dm_fl_count(S));
      dm_fl_get(S, 0, &F); CHECK(F.h.dealt[0] == 64, "newest first (%u)", F.h.dealt[0]);
      dm_fl_get(S, DM_FL_RING - 1, &F); CHECK(F.h.dealt[0] == 15, "oldest kept is the 6th (%u)", F.h.dealt[0]);
      CHECK(!dm_fl_get(S, DM_FL_RING, &F), "nothing past the ring"); }


    fresh(); { enemy *a = spawn(5, 0, 100), *b = spawn(5, 0, 100), *c = spawn(6, 0, 100); const uint32_t *ids;
      ticks(60); hit(a, 10, 0, 0); hurt(c, 1); tick(); CHECK(dm_fl_kills(S, now, &ids) == 0 && dm_fl_hits(S, now) == 1, "no kill yet; one own hit, the enemy's is none");
      hit(a, 200, 0, 0); hit(b, 200, 1, 0); hit(b, 5, 1, 0); tick(); int n = dm_fl_kills(S, now, &ids);
      CHECK(n == 2 && ids[0] == a->id && ids[1] == b->id, "the player's and the minion's kill (%d)", n);
      CHECK(dm_fl_hits(S, now) == 2 && dm_fl_hits(S, now - 1) == 0, "the fatal hits count as hits, the one on the dead enemy does not (%d)", dm_fl_hits(S, now));
      tick(); CHECK(dm_fl_kills(S, now, &ids) == 0 && dm_fl_hits(S, now) == 0, "a tick later the lists are empty");
      c->dead = 1; tick(); CHECK(dm_fl_kills(S, now, &ids) == 0, "an enemy that only hit the player is no kill");
      enemy *d = spawn(7, 0, 100); d->dot = 1; while (now % DM_FL_SCAN) tick(); tick(); poison(100); while (now % DM_FL_SAMPLE) tick(); poison(100); tick();
      n = dm_fl_kills(S, now, &ids); CHECK(n == 1 && ids[0] == d->id, "a kill by the own poison (%d)", n); }


    fresh(); { enemy *a = spawn(5, 0, 50), *b = spawn(5, 0, 400); const uint32_t *ids; int n;
      ticks(60); hit(a, 50, 0, 0); hit(b, 10, 0, 0); tick(); n = dm_fl_kills(S, now, &ids);
      CHECK(n == 1 && ids[0] == a->id, "first death (%d)", n);
      ticks(10); a->dead = 0; a->life = a->max; tick(); hit(a, 60, 0, 0); tick(); n = dm_fl_kills(S, now, &ids);
      CHECK(n == 1 && ids[0] == a->id && dm_fl_open(S), "the revived enemy's death again, same fight (%d)", n);
      ticks(10); a->dead = 0; a->life = a->max; tick(); a->dead = 1; a->life = 0; tick(); n = dm_fl_kills(S, now, &ids);
      CHECK(n == 0, "a death without own damage since the revival (%d)", n);
      for (int i = 0; i < 4 && !ended; i++) { hit(b, 100, 0, 0); tick(); }
      settle(); CHECK(ended == DM_FL_END_KILL && dm_fl_last(S)->h.kills == 2, "the fight counts the revived enemy once (%u)", dm_fl_last(S)->h.kills); }

    fresh(); { enemy *e[DM_FL_MEMBERS + 4]; const uint32_t *ids; int kills = 0;
      for (int i = 0; i < DM_FL_MEMBERS + 4 && i < NE; i++) e[i] = spawn(5, 0, 10);
      ticks(60);
      for (int i = 0; i < nw; i++) { hit(e[i], 50, 0, 0); if (nq >= 100 || i == nw - 1) { tick(); kills += dm_fl_kills(S, now, &ids); } }
      CHECK(kills == nw, "kills %d of %d", kills, nw); }

    fresh(); { dm_fl_record(S, 0); enemy *e = spawn(5, 0, 100); const uint32_t *ids; ticks(60); hit(e, 200, 0, 0); tick();
      CHECK(dm_fl_kills(S, now, &ids) == 1, "kill seen"); settle(); CHECK(!dm_fl_open(S) && dm_fl_count(S) == 0, "ring empty"); dm_fl_record(S, 1); }



    fresh(); { static dm_fl_fight gotf, keptf; static uint8_t img[DM_FL_IMAGE_MAX], img2[DM_FL_IMAGE_MAX]; uint32_t gen; char who[64];
      dm_fl_owner(S, "Hero", NULL, 0);
      enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); tick();
      size_t n = dm_fl_image(S, who, img, sizeof img, &gen);
      CHECK(n > DM_FL_IMAGE_HEAD && !dm_fl_count(S), "the image already carries the won fight, the ring not yet (%zu)", n);
      settle(); CHECK(dm_fl_count(S) == 1, "finish keeps it in the ring"); keptf = *newest();
      CHECK(!dm_fl_owner(S, "Hero", img, n) && dm_fl_count(S) == 1, "the grace image loads as one fight");
      dm_fl_get(S, 0, &gotf); CHECK(!memcmp(&gotf, &keptf, sizeof gotf), "its newest is byte-identical to what finish keeps");
      dm_fl_owner(S, "Other", NULL, 0);
      CHECK(!dm_fl_count(S) && !dm_fl_image(S, who, img2, sizeof img2, &gen), "a new owner starts without it, nothing to save");
      CHECK(!dm_fl_owner(S, "Hero", img, n) && dm_fl_count(S) == 1, "Hero's saved file still holds the won fight");
      for (int how = 0; how < 2; how++) {
        fresh(); dm_fl_owner(S, "Hero", NULL, 0);
        enemy *a = spawn(5, 0, 10); ticks(30); hit(a, 10, 0, 0); done();
        enemy *b = spawn(6, 0, 10); ticks(30); hit(b, 10, 0, 0); tick();
        size_t nf = dm_fl_image(S, who, img, sizeof img, &gen); CHECK(nf > DM_FL_IMAGE_HEAD, "way %d: file with the pending fight", how);
        dm_fl_saved(S, gen);
        if (how) dm_fl_discard(S); else dm_fl_record(S, 0);
        size_t nr = dm_fl_image(S, who, img2, sizeof img2, &gen);
        CHECK(nr && nr < nf, "way %d: the replacement image is shorter (%zu < %zu)", how, nr, nf);
        CHECK(!dm_fl_owner(S, "Hero", img2, nr) && dm_fl_count(S) == 1, "way %d: reload without the pending fight", how);
        dm_fl_get(S, 0, &gotf); CHECK(gotf.h.end == DM_FL_END_KILL && gotf.g[0].cls == 5, "way %d: the earlier entry stayed", how);
        if (!how) dm_fl_record(S, 1);
      }
      fresh(); dm_fl_owner(S, "Hero", NULL, 0); dm_fl_record(S, 0);
      { enemy *c = spawn(5, 0, 10); ticks(30); hit(c, 10, 0, 0); tick();
        CHECK(!dm_fl_image(S, who, img, sizeof img, &gen), "recording off before the kill: no pending fight in the image"); dm_fl_record(S, 1); settle(); }
      fresh(); dm_fl_owner(S, "Hero", NULL, 0);
      { enemy *a = spawn(5, 0, 10); ticks(30); hit(a, 10, 0, 0); done();
        enemy *b = spawn(6, 0, 10); ticks(30); hit(b, 10, 0, 0); tick(); dm_fl_image(S, who, img, sizeof img, &gen); dm_fl_saved(S, gen);
        pl.dead = 1; tick();
        CHECK(ended == DM_FL_END_DEATH, "death within the grace");
        size_t nd = dm_fl_image(S, who, img2, sizeof img2, &gen);
        CHECK(!dm_fl_owner(S, "Hero", img2, nd) && dm_fl_count(S) == 2 && newest()->h.end == DM_FL_END_DEATH, "the death replaced the pending fight, never both"); }
      fresh(); dm_fl_owner(S, "Hero", NULL, 0);
      for (int i = 0; i < DM_FL_RING; i++) { enemy *e4 = spawn(100 + (uint32_t)i, 0, 10); ticks(5); hit(e4, 10, 0, 0); done(); }
      CHECK(dm_fl_count(S) == DM_FL_RING, "ring full");
      { enemy *f5 = spawn(160, 0, 10); ticks(5); hit(f5, 10, 0, 0); tick(); }
      size_t nn = dm_fl_image(S, who, img, sizeof img, &gen);
      CHECK(!dm_fl_owner(S, "Hero", img, nn) && dm_fl_count(S) == DM_FL_RING, "a full ring with a pending fight loads (count %d)", dm_fl_count(S));
      dm_fl_get(S, 0, &gotf); dm_fl_get(S, DM_FL_RING - 1, &keptf);
      CHECK(gotf.g[0].cls == 160 && keptf.g[0].cls == 101, "the pending fight is newest, the oldest dropped out (%u, %u)", gotf.g[0].cls, keptf.g[0].cls); }


    fresh(); { static dm_fl_fight before[DM_FL_RING], after; static uint8_t img[DM_FL_IMAGE_MAX], bad[DM_FL_IMAGE_MAX + 1]; uint32_t gen; char who[64];
      for (int n = 0; n < 40; n++) { enemy *e = spawn(5, (uint8_t)(n % 5), 10 + n); ticks(30); hit(e, 10 + n, 0, n & 1); done(); }
      { for (int k = 0; k < 10; k++) spawn(40 + (uint32_t)k, 0, 50); ticks(60); for (int k = 0; k < 10; k++) hit(&W[nw - 10 + k], 10, 0, 0); tick();
        pend_potion = 1; pend_cls = 7; tick(); ticks(20); for (int k = 0; k < 10; k++) hit(&W[nw - 10 + k], 40, k & 1, 0); done(); }
      { enemy *e = spawn(5, 0, 100000); ticks(60); for (int i = 0; i < 300; i++) { hit(e, 10, 0, 0); ticks(5); } pl.area = 9; tick(); pl.area = 2; }
      for (int n = 0; n < 9; n++) { enemy *e = spawn(9, 0, 30); ticks(30); hit(e, 30, 0, 0); done(); }
      int count = dm_fl_count(S);
      for (int i = 0; i < count; i++) dm_fl_get(S, i, &before[i]);
      CHECK(count == DM_FL_RING && before[10].h.ngroups == DM_FL_GROUPS && (before[10].h.flags & DM_FL_F_MORE_KINDS) && before[10].h.nmarks > 1 && before[9].h.width > 1, "a varied ring (groups %u, width %u)", before[10].h.ngroups, before[9].h.width);
      CHECK(!dm_fl_image(S, who, img, sizeof img, &gen), "no image without an owner");
      dm_fl_owner(S, "Hero", NULL, 0); CHECK(dm_fl_count(S) == 0 && !dm_fl_image(S, who, img, sizeof img, &gen), "a new owner starts empty and saved");
      for (int i = count - 1; i >= 0; i--) { pthread_mutex_lock(&S->mu); S->ring[S->head] = before[i]; S->head = (S->head + 1) % DM_FL_RING; S->count++; S->gen++; pthread_mutex_unlock(&S->mu); }
      size_t n = dm_fl_image(S, who, img, sizeof img, &gen);
      CHECK(n > DM_FL_IMAGE_HEAD && n <= DM_FL_IMAGE_MAX && !strcmp(who, "Hero"), "image of %zu bytes", n);
      CHECK(DM_FL_IMAGE_MAX < 300 * 1024 && n < 120 * 1024, "sizes: at most %zu, this ring %zu", (size_t)DM_FL_IMAGE_MAX, n);
      CHECK(!dm_fl_owner(S, "Hero", img, n) && dm_fl_count(S) == count, "the image loads (%d)", dm_fl_count(S));
      int same = 1; for (int i = 0; i < count; i++) { dm_fl_get(S, i, &after); same &= !memcmp(&after, &before[i], sizeof after); }
      CHECK(same, "every fight comes back byte for byte, newest first");
      CHECK(!dm_fl_image(S, who, img, sizeof img, &gen), "a loaded ring counts as saved");

      #define REJECT(why, edit, crc) do { memcpy(bad, img, n); size_t m = n; edit; if (crc) put32(bad + 12, crc32(bad + DM_FL_IMAGE_HEAD, m - DM_FL_IMAGE_HEAD)); \
          int r = dm_fl_owner(S, "Hero", bad, m); CHECK(r == (why) && dm_fl_count(S) == 0, "rejected as %d (%d)", why, r); } while (0)
      REJECT(DM_FL_BAD_SIZE, m = 10, 0);
      REJECT(DM_FL_BAD_FIGHT, m = n - 1, 1);
      REJECT(DM_FL_BAD_SIZE, (bad[n] = 0, m = n + 1), 1);
      REJECT(DM_FL_BAD_MAGIC, bad[0] ^= 1, 0);
      REJECT(DM_FL_BAD_VERSION, bad[4] = 3, 0);
      REJECT(DM_FL_BAD_CRC, bad[n - 1] ^= 0x40, 0);
      REJECT(DM_FL_BAD_COUNT, put32(bad + 8, DM_FL_RING + 1), 1);

      REJECT(DM_FL_BAD_COUNT, (put32(bad + 8, 0xffffffffu), m = DM_FL_IMAGE_HEAD), 1);
      REJECT(DM_FL_BAD_COUNT, (put32(bad + 8, 0x80000000u), m = DM_FL_IMAGE_HEAD), 1);
      { now += 1000; dm_fl_game(S, now); enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); done();
        CHECK(dm_fl_count(S) == 1 && S->head == 1 && dm_fl_get(S, 0, &after) && after.h.end == DM_FL_END_KILL, "after a rejected count the ring starts at its first slot"); }
      REJECT(DM_FL_BAD_SIZE, put32(bad + 8, DM_FL_RING - 1), 1);
      size_t ng = DM_FL_IMAGE_HEAD + offsetof(dm_fl_head, ngroups), nbk = DM_FL_IMAGE_HEAD + offsetof(dm_fl_head, nbuckets), wd = DM_FL_IMAGE_HEAD + offsetof(dm_fl_head, width);
      REJECT(DM_FL_BAD_FIGHT, bad[ng] = DM_FL_GROUPS + 1, 1);
      REJECT(DM_FL_BAD_FIGHT, (bad[nbk] = 0, bad[nbk + 1] = 0), 1);
      REJECT(DM_FL_BAD_FIGHT, (bad[nbk] = 1, bad[nbk + 1] = 1), 1);
      REJECT(DM_FL_BAD_FIGHT, (bad[wd] = 0, bad[wd + 1] = 0), 1);
      REJECT(DM_FL_BAD_FIGHT, bad[DM_FL_IMAGE_HEAD + offsetof(dm_fl_head, end)] = 9, 1);
      REJECT(DM_FL_BAD_FIGHT, bad[DM_FL_IMAGE_HEAD + sizeof(dm_fl_head) + sizeof(dm_fl_split) + offsetof(dm_fl_group, rank)] = 9, 1);
      REJECT(DM_FL_BAD_FIGHT, bad[DM_FL_IMAGE_HEAD + sizeof(dm_fl_head) + offsetof(dm_fl_split, flags)] = 7, 1);
      #undef REJECT

      dm_fl_owner(S, "Hero", img, n); now += 1000; dm_fl_game(S, now);
      { enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); done(); }
      size_t n2 = dm_fl_image(S, who, img, sizeof img, &gen); CHECK(n2 > 0, "a kept fight makes the ring unsaved");
      dm_fl_saved(S, gen); CHECK(!dm_fl_image(S, who, img, sizeof img, &gen), "saved");
      dm_fl_unsaved(S); CHECK(dm_fl_image(S, who, img, sizeof img, &gen) == n2, "a refused save makes it unsaved"); dm_fl_saved(S, gen); }



    { char root[256], dir[300], file[400], aside[420]; const char *tmp = getenv("TMPDIR"); struct stat st;
      snprintf(root, sizeof root, "%s/t_fightlog.XXXXXX", tmp && tmp[0] ? tmp : "/tmp"); CHECK(mkdtemp(root) != NULL, "temporary folder");
      snprintf(dir, sizeof dir, "%s/Hero", root); snprintf(file, sizeof file, "%s/Hero.sw_fights2", dir); snprintf(aside, sizeof aside, "%s.bad", file);
      fresh(); CHECK(!dm_fl_store_load(S, root, "Hero") && dm_fl_count(S) == 0, "no file: an empty ring");
      { enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); done(); }
      CHECK(!dm_fl_store_commit(S, root, 1) && stat(file, &st), "no character folder: nothing written");
      mkdir(dir, 0700);
      CHECK(dm_fl_store_commit(S, root, 1) && !stat(file, &st) && st.st_size > DM_FL_IMAGE_HEAD, "written into the character folder");
      off_t first = st.st_size;
      CHECK(!dm_fl_store_commit(S, root, 2), "unchanged: not written again");
      { enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); done(); }
      CHECK(dm_fl_store_commit(S, root, 2) && !stat(file, &st) && st.st_size > first, "a new fight: written again");
      unlink(file);
      CHECK(dm_fl_store_commit(S, root, 2) && !stat(file, &st), "after a refused commit the file is written again");
      CHECK(!dm_fl_store_commit(S, root, 3), "after a successful commit nothing is pending");
      fresh(); CHECK(!dm_fl_store_load(S, root, "Hero") && dm_fl_count(S) == 2, "loaded on the next entry (%d)", dm_fl_count(S));

      { enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); done(); }
      CHECK(!dm_fl_store_follow(S, root, "Hero") && dm_fl_count(S) == 3, "the same character keeps its ring (%d)", dm_fl_count(S));
      CHECK(dm_fl_store_follow(S, root, "Other") && dm_fl_count(S) == 0, "another character starts with its own, empty list (%d)", dm_fl_count(S));
      CHECK(dm_fl_store_follow(S, root, "Hero") && dm_fl_count(S) == 2, "the first character's file comes back (%d)", dm_fl_count(S));


      { enemy *c = spawn(31, 0, 10); ticks(30); hit(c, 10, 0, 0); tick();
        CHECK(dm_fl_count(S) == 2 && dm_fl_store_commit(S, root, 4), "the file takes the fight that still waits its grace"); }
      now += 1000; dm_fl_game(S, now);
      CHECK(dm_fl_store_follow(S, root, "Other") && dm_fl_count(S) == 0, "a new game and another character: its own empty list");
      now += 1000; dm_fl_game(S, now);
      CHECK(dm_fl_store_follow(S, root, "Hero") && dm_fl_count(S) == 3, "Hero's file comes back whole (%d)", dm_fl_count(S));
      { dm_fl_fight cf; int n31 = 0, n5 = 0;
        for (int i = 0; i < 3; i++) { CHECK(dm_fl_get(S, i, &cf), "fight %d from the file", i); n31 += cf.g[0].cls == 31; n5 += cf.g[0].cls == 5; }
        dm_fl_get(S, 0, &cf);
        CHECK(n31 == 1 && n5 == 2 && cf.h.end == DM_FL_END_KILL && cf.g[0].cls == 31, "the grace fight once as the newest, the earlier two kept"); }
      FILE *f = fopen(file, "r+b"); fseek(f, 40, SEEK_SET); fputc(0x5a, f); fclose(f);
      fresh(); CHECK(dm_fl_store_load(S, root, "Hero") == DM_FL_BAD_CRC && dm_fl_count(S) == 0 && !stat(aside, &st) && stat(file, &st), "a damaged file is set aside");
      CHECK(!dm_fl_store_commit(S, root, 5) && stat(file, &st), "an empty loaded ring is not written");
      unlink(aside);


      { char old1[420]; snprintf(old1, sizeof old1, "%s/Hero.sw_fights", dir);
        fresh(); dm_fl_owner(S, "Hero", NULL, 0);
        for (int i = 0; i < 3; i++) { enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); done(); }
        static uint8_t v2[DM_FL_IMAGE_MAX], v1[DM_FL_IMAGE_MAX]; char w[64]; uint32_t g; size_t n2 = dm_fl_image(S, w, v2, sizeof v2, &g), n1 = DM_FL_IMAGE_HEAD;
        for (int i = 0; i < 3; i++) {
            size_t at = DM_FL_IMAGE_HEAD; for (int k = 0; k < i; k++) { dm_fl_head hh; memcpy(&hh, v2 + at, sizeof hh); at += fight_size(&hh, DM_FL_VERSION); }
            dm_fl_head hh; memcpy(&hh, v2 + at, sizeof hh); size_t rest = fight_size(&hh, DM_FL_VERSION_1) - sizeof hh;
            memcpy(v1 + n1, &hh, sizeof hh); memcpy(v1 + n1 + sizeof hh, v2 + at + sizeof hh + sizeof(dm_fl_split), rest); n1 += sizeof hh + rest;
        }
        CHECK(n2 > n1, "the version 2 image is the longer one");
        put32(v1, DM_FL_MAGIC); put32(v1 + 4, DM_FL_VERSION_1); put32(v1 + 8, 3); put32(v1 + 12, crc32(v1 + DM_FL_IMAGE_HEAD, n1 - DM_FL_IMAGE_HEAD));
        unlink(file); FILE *o = fopen(old1, "wb"); fwrite(v1, 1, n1, o); fclose(o);
        fresh(); CHECK(!dm_fl_store_load(S, root, "Hero") && dm_fl_count(S) == 3, "the version 1 file loads (%d)", dm_fl_count(S));
        dm_fl_fight lf; dm_fl_get(S, 0, &lf); CHECK(lf.s.flags == 0 && lf.s.mine == 0, "its fights have an unknown split");
        { enemy *e = spawn(5, 0, 10); ticks(30); hit(e, 10, 0, 0); done(); }
        CHECK(dm_fl_store_commit(S, root, 6) && !stat(file, &st), "a new fight goes into the version 2 file");
        fresh(); CHECK(!dm_fl_store_load(S, root, "Hero") && dm_fl_count(S) == 4, "reloaded from the version 2 file (%d)", dm_fl_count(S));
        int unknown = 0, known = 0; for (int i = 0; i < 4; i++) { dm_fl_get(S, i, &lf); unknown += !lf.s.flags; known += lf.s.flags == DM_FL_SPLIT_KNOWN; }
        CHECK(unknown == 3 && known == 1, "the old fights stay unknown after saving again, the new one is known (%d, %d)", unknown, known);
        static uint8_t back[DM_FL_IMAGE_MAX]; o = fopen(old1, "rb"); size_t nb = fread(back, 1, sizeof back, o); fclose(o);
        CHECK(nb == n1 && !memcmp(back, v1, n1), "the version 1 file is untouched");
        fresh(); CHECK(!dm_fl_owner(S, "Hero", back, nb) && dm_fl_count(S) == 3, "and still loads as before");
        unlink(old1); unlink(file); }
      rmdir(dir); rmdir(root); }




    fresh(); { enemy *a = spawn(5, 0, 400), *b = spawn(6, 0, 400); ticks(30);
      #define SKILLHIT(e, pts, sk, lay) do { int af = game_after((e)->life, (pts) * P); Q[nq++] = (dm_fl_hit){ (e)->id, (e)->cls, (e)->life, af, 0xffff, DM_FL_OUT_PLAYER, (e)->rank, 0, .skill = (sk), .by = 0xffff, .lays = (lay) }; (e)->life = af; if (!af) (e)->dead = 1; } while (0)
      #define COMPHIT(e, pts, cl, mc) do { int af = game_after((e)->life, (pts) * P); Q[nq++] = (dm_fl_hit){ (e)->id, (e)->cls, (e)->life, af, 0xffff, DM_FL_OUT_MINION, (e)->rank, 0, .skill = -1, .by = (cl), .merc = (mc) }; (e)->life = af; if (!af) (e)->dead = 1; } while (0)
      SKILLHIT(a, 10, 36, 0); SKILLHIT(a, 7, 56, 0); SKILLHIT(b, 5, -1, 0); SKILLHIT(a, 3, 92, DM_FL_LAYS_POISON); tick();
      for (int k = 0; k < 6; k++) COMPHIT(b, 2, (uint16_t)(400 + k), 0);
      COMPHIT(b, 4, 271, 1); tick();

      a->dot = 1; a->owners.n = 3;
      a->owners.e[0].kind = DM_FL_DOT_PLAYER; a->owners.e[0].rate = 3;
      a->owners.e[1].kind = DM_FL_DOT_COMP; a->owners.e[1].burn = 1; a->owners.e[1].cls = 401; a->owners.e[1].rate = 1;
      a->owners.e[2].kind = DM_FL_DOT_UNKNOWN; a->owners.e[2].rate = 1;
      for (int k = 0; k < 50; k++) { poison(5); tick(); }
      a->dot = 0; SKILLHIT(a, 400, 36, 0); SKILLHIT(b, 400, 36, 0); done();
      const dm_fl_fight *f = newest(); const dm_fl_split *x = &f->s;
      CHECK(x->flags == DM_FL_SPLIT_KNOWN && !dm_fl_split_gap(S), "a known split that adds up (gap %lld)", (long long)dm_fl_split_gap(S));
      uint32_t comps = 0; for (int k = 0; k < DM_FL_COMPS; k++) comps += x->comp[k].dealt;
      CHECK(x->mine + x->merc + x->rest + x->unknown + comps >= f->h.dealt[0] + f->h.dealt[1] + f->h.dealt[2] - 8 && x->mine + x->merc + x->rest + x->unknown + comps <= f->h.dealt[0] + f->h.dealt[1] + f->h.dealt[2] + 8,
            "whole points add up within their rounding");
      CHECK(x->merc == 4, "the mercenary keeps its own field beside six kinds (%u)", x->merc);
      CHECK(x->comp[0].cls == 400 && x->comp[3].cls == 403 && x->rest == 2 * 2 && x->comp[1].dealt > 2, "four kinds, the other two together; 401 also has its burning (%u)", x->comp[1].dealt);
      int i36 = -1, i56 = -1, i92 = -1; for (int k = 0; k < DM_FL_SKILLS; k++) { if (f->h.skill[k] == 37) i36 = k; if (f->h.skill[k] == 57) i56 = k; if (f->h.skill[k] == 93) i92 = k; }
      CHECK(i36 >= 0 && i56 >= 0 && i92 >= 0 && x->skill[i56] == 7 && x->none == 5, "damage by skill, thorns without one");
      uint32_t dot = f->h.dealt[2];
      CHECK(dot > 0 && x->skill[i92] >= 3 + dot * 3 / 5 - 2 && x->skill[i92] <= 3 + dot * 3 / 5 + 2 && x->unknown >= dot / 5 - 2 && x->unknown <= dot / 5 + 2,
            "poison by rate goes to the skill that laid it (%u of %u), the ownerless part is unknown (%u)", x->skill[i92], dot, x->unknown);
      uint32_t skills = x->none + x->over; for (int k = 0; k < DM_FL_SKILLS; k++) skills += x->skill[k];
      CHECK(skills == x->mine && x->gmine[0] + x->gmine[1] == x->mine, "the skills and the groups make up the player's part"); }




    fresh(); { enemy *a = spawn(5, 0, 400); ticks(30);
      SKILLHIT(a, 2, 92, DM_FL_LAYS_POISON); tick();
      dm_fl_dot both = { 2, { { DM_FL_DOT_PLAYER, 0, 0, 3 * P }, { DM_FL_DOT_COMP, 0, 425, 2 * P } } };
      dm_fl_dot mine1 = { 1, { { DM_FL_DOT_PLAYER, 0, 0, 3 * P } } }, mine2 = { 1, { { DM_FL_DOT_PLAYER, 0, 0, 5 * P } } };
      for (int k = 0; k < 10; k++) { dot_write(a, &both); tick(); }
      for (int k = 0; k < 10; k++) { dot_write(a, &mine1); tick(); }
      for (int k = 0; k < 10; k++) { dot_write(a, &mine2); tick(); }
      { dm_fl_dot over = mine1; int b = a->life; DW[ndw].id = a->id; DW[ndw].before = b; DW[ndw].after = b - 7 * P; DW[ndw].d = over; ndw++; a->life = b - 7 * P; tick(); }
      a->life = 6 * P + 100;
      { DW[ndw].id = a->id; DW[ndw].before = a->life; DW[ndw].after = 0; DW[ndw].d = both; DW[ndw].d.e[1].rate = 5 * P; ndw++; a->life = 0; a->dead = 1; }
      ticks(5); settle();
      const dm_fl_fight *f = newest(); const dm_fl_split *x = &f->s;
      int i92 = -1; for (int k = 0; k < DM_FL_SKILLS; k++) if (f->h.skill[k] == 93) i92 = k;
      uint32_t poppy = 0; for (int k = 0; k < DM_FL_COMPS; k++) if (x->comp[k].cls == 425) poppy = x->comp[k].dealt;


      int64_t fatal = 6 * P + 100, pp = fatal - 5 * P, cp = fatal - 3 * P;
      CHECK(i92 >= 0 && x->skill[i92] == (uint32_t)((2 * P + 30 * P + 30 * P + 50 * P + 3 * P + pp) >> 8), "the player's poison: what its rates prove (%u)", i92 >= 0 ? x->skill[i92] : 0);
      CHECK(poppy == (uint32_t)((20 * P + cp) >> 8), "the companion keeps the 20 points of its poison after it ran out, and its proven part of the fatal tick (%u)", poppy);
      CHECK(x->unknown == (uint32_t)((4 * P + (fatal - pp - cp)) >> 8), "the loss beyond the rates and the unproven part of the fatal tick are unknown (%u)", x->unknown);
      CHECK(!dm_fl_split_gap(S) || (dm_fl_split_gap(S) > -3 * P && dm_fl_split_gap(S) < 3 * P), "the parts add up (%lld)", (long long)dm_fl_split_gap(S));
      CHECK(f->h.kills == 1 && f->h.end == DM_FL_END_KILL, "the poison's kill"); }



    fresh(); { enemy *a = spawn(5, 0, 30), *b = spawn(6, 0, 30), *c = spawn(7, 0, 30); ticks(30);
      hurt(b, 5); tick();
      dm_fl_action(S, now, DM_FL_ME_ATTACK, 36); uint32_t act_a = now; ticks(5);
      SKILLHIT(a, 30, 36, 0); tick(); dm_fl_action(S, now, DM_FL_ME_ATTACK, 36); uint32_t act_b = now; ticks(25);
      SKILLHIT(b, 10, 36, 0); tick(); dm_fl_action(S, now, DM_FL_ME_ATTACK, 36); ticks(24);
      SKILLHIT(b, 20, 36, 0); tick(); dm_fl_action(S, now, DM_FL_ME_CAST, 56); ticks(5);
      SKILLHIT(c, 30, 51, 0); done();
      const dm_fl_fight *f = newest(); uint32_t t0 = now; (void)t0;
      int ga = -1, gb = -1, gc = -1; for (int g = 0; g < f->h.ngroups; g++) { if (f->g[g].cls == 5) ga = g; if (f->g[g].cls == 6) gb = g; if (f->g[g].cls == 7) gc = g; }
      uint32_t start = act_a - f->s.gact[ga];
      CHECK(ga >= 0 && gb >= 0 && gc >= 0 && f->s.gact[gb] - f->s.gact[ga] == act_b - act_a && f->s.gact[gb] < f->g[gb].t_first + 60 && f->s.gact[gb] > f->g[gb].t_first,
            "each group from the action that led to its first hit, not from its first contact (%u, %u, contact %u)", f->s.gact[ga], f->s.gact[gb], f->g[gb].t_first);
      CHECK(f->s.gact[gc] == 0xffff, "a first hit of another skill than the latest action: unknown (%u)", f->s.gact[gc]);
      CHECK(f->s.t_act == act_a - start, "the fight's first action (%u)", f->s.t_act); }




    fresh(); { enemy *a = spawn(5, 0, 40), *b = spawn(6, 0, 40), *c = spawn(7, 0, 40); ticks(30);
      dm_fl_action(S, now, DM_FL_ME_ATTACK, 36); uint32_t act_a = now; ticks(5); dm_fl_engage(S, a->id, 36, 0); ticks(70);
      dm_fl_action(S, now, DM_FL_ME_ATTACK, 36); ticks(5); SKILLHIT(a, 10, 36, 0); tick();
      dm_fl_action(S, now, DM_FL_ME_ATTACK, 36); uint32_t act_b = now; ticks(5); dm_fl_engage(S, b->id, 36, 0); ticks(10);
      hurt(b, 3); tick(); dm_fl_action(S, now, DM_FL_ME_ATTACK, 36); ticks(5); SKILLHIT(b, 10, 36, 0); tick();
      dm_fl_action(S, now, DM_FL_ME_CAST, 56); ticks(10); dm_fl_action(S, now, DM_FL_ME_CAST, 56); ticks(5);
      Q[nq++] = (dm_fl_hit){ c->id, c->cls, c->life, c->life - 10 * P, 0xffff, DM_FL_OUT_PLAYER, 0, 0, .skill = 56, .by = 0xffff, .delayed = 1 }; c->life -= 10 * P; tick();
      SKILLHIT(a, 30, 36, 0); SKILLHIT(b, 30, 36, 0); SKILLHIT(c, 30, 56, 0); done();
      const dm_fl_fight *f = newest(); int ga = -1, gb = -1, gc = -1;
      for (int g = 0; g < f->h.ngroups; g++) { if (f->g[g].cls == 5) ga = g; if (f->g[g].cls == 6) gb = g; if (f->g[g].cls == 7) gc = g; }
      CHECK(ga >= 0 && gb >= 0 && gc >= 0 && f->s.gact[gb] - f->s.gact[ga] == act_b - act_a, "both from the action of their first miss (%u, %u)", f->s.gact[ga], f->s.gact[gb]);
      CHECK(f->s.gact[ga] == f->s.t_act, "the miss before the fight is its first action (%u, %u)", f->s.gact[ga], f->s.t_act);
      CHECK(f->s.gact[gc] == 0xffff, "a missile's impact after a second cast: not known (%u)", f->s.gact[gc]); }



    { static uint8_t fimg[DM_FL_IMAGE_MAX], dimg[DM_DH_IMAGE_MAX]; char who[64]; uint32_t gen; dm_dh *h = dm_dh_core(); static dm_dh_window w; static dm_dh_seg sg[DM_DH_SEGS];
      dm_fl_parts parts = { 1, 2, 3, 24, 0, 0 };
      fresh(); dm_fl_owner(S, "Hero", NULL, 0); dm_dh_reset(h); dm_dh_owner(h, "Hero", NULL, 0); dm_fl_build(S, 77, 0x5a);
      { enemy *e = spawn(5, 0, 50); ticks(30); SKILLHIT(e, 20, 36, 0); ticks(30); SKILLHIT(e, 30, 36, 0); done(); dm_dh_add(h, 0x5a, &parts, newest()); }
      { enemy *e = spawn(6, 0, 50); ticks(30); SKILLHIT(e, 20, 36, 0); ticks(30); SKILLHIT(e, 30, 36, 0); tick(); }
      dm_fl_fight pend; CHECK(dm_fl_pending(S, &pend) && pend.g[0].cls == 6, "a fight waits its grace");
      size_t nf = dm_fl_image(S, who, fimg, sizeof fimg, &gen), nd = dm_dh_image(h, &pend, &parts, who, dimg, sizeof dimg, &gen);
      CHECK(nf && nd && dm_dh_read(h, 0x5a, &w, sg) == 1 && w.n == 1, "the history itself does not hold it yet");
      dm_dh_saved(h, gen); h->saved_pend = 1;
      CHECK(dm_dh_image(h, &pend, &parts, who, dimg, sizeof dimg, &gen) == nd, "while it waits, every save writes it again");

      dm_fl_state keep = *S; dm_fl_reset(S); dm_dh_reset(h);
      CHECK(!dm_fl_owner(S, "Hero", fimg, nf) && dm_fl_count(S) == 2 && !dm_dh_owner(h, "Hero", dimg, nd) && dm_dh_read(h, 0x5a, &w, sg) == 1 && w.n == 2 && sg[0].fights == 2,
            "restarted: the fight in the list and once in the history (%u, %u)", w.n, sg[0].fights);

      *S = keep; pthread_mutex_init(&S->mu, NULL); dm_dh_reset(h); dm_dh_owner(h, "Hero", NULL, 0); dm_dh_add(h, 0x5a, &parts, &keep.ring[(keep.head + DM_FL_RING - 1) % DM_FL_RING]);
      now += 1000;
      if (dm_fl_game(S, now)) dm_dh_add(h, dm_fl_last(S)->s.state, &parts, dm_fl_last(S));
      CHECK(!dm_fl_pending(S, &pend) && dm_fl_count(S) == 2 && dm_dh_read(h, 0x5a, &w, sg) == 1 && w.n == 2, "the next game: once in the ring and the history (%u)", w.n);
      h->saved_pend = 1; nd = dm_dh_image(h, NULL, &parts, who, dimg, sizeof dimg, &gen);
      dm_dh_reset(h); CHECK(nd && !dm_dh_owner(h, "Hero", dimg, nd) && dm_dh_read(h, 0x5a, &w, sg) == 1 && w.n == 2 && sg[0].fights == 2, "the save after it holds it once"); }



    fresh(); { static char out[256 * 1024]; uint32_t st[DM_FL_STATE_WORDS] = { 0 }; enemy *a, *b, *c;
      st[0] = 1u << 30; dm_fl_states(S, now, st);
      int n = dm_fl_text_list(S, 1, 77, NULL, NULL, out, sizeof out);
      CHECK(n > 0 && !strncmp(out, "log\t0\t1\n", 9), "an empty list: %.20s", out);
      dm_fl_build(S, 77, 0); a = spawn(211, 0, 100); ticks(60); dm_fl_action(S, now, DM_FL_ME_CAST, 52); hit(a, 10, 0, 1); tick();
      st[0] = 0; ticks(5); dm_fl_states(S, now, st); st[0] = 1u << 31; ticks(5); dm_fl_states(S, now, st); a->attacking = MELEE; tick(); ticks(3); hurt(a, 30); tick(); a->attacking = 0; tick();
      pend_potion = 0; pend_cls = 2; tick(); ticks(5); hit(a, 90, 0, 0); done();
      dm_fl_build(S, 78, 0); b = spawn(5, DM_FL_RANK_CHAMPION, 20); c = spawn(6, DM_FL_RANK_NORMAL, 20); ticks(30); hit(b, 5, 0, 0); hit(c, 5, 1, 0); tick(); ticks(10); hit(b, 15, 0, 0); hit(c, 15, 0, 0); done();
      n = dm_fl_text_list(S, 1, 77, NULL, NULL, out, sizeof out);
      int f_lines = 0, g_lines = 0, sim0 = -1, sim1 = -1; char *save;
      for (char *l = strtok_r(out, "\n", &save); l; l = strtok_r(NULL, "\n", &save)) {
          int fields = 1; for (char *q = l; *q; q++) fields += *q == '\t';
          if (!strncmp(l, "f\t", 2)) { f_lines++; CHECK(fields == 19, "an f line has 19 fields (%d)", fields); int idx, v[18]; sscanf(l, "f\t%d", &idx);
              char *q = l; for (int k = 0; k < 18; k++) { q = strchr(q, '\t') + 1; v[k] = atoi(q); }
              if (idx == 0) sim0 = v[14]; else sim1 = v[14];
              CHECK(v[15] == 1 && v[16] == v[9] && v[17] == v[10], "a known split: the player's part and the companions' (%d %d %d %d)", v[16], v[9], v[17], v[10]); }
          if (!strncmp(l, "g\t", 2)) { g_lines++; CHECK(fields == 15, "a g line has 15 fields (%d)", fields); CHECK(strstr(l, "\t-\t") != NULL, "without the tables the name key is -"); }
      }
      CHECK(n > 0 && f_lines == 2 && g_lines == 3 && sim0 == 0 && sim1 == 1, "list: 2 fights, 3 groups, the group fight has no simulation, the single one of the same gear has (%d %d %d %d)", f_lines, g_lines, sim0, sim1);
      dm_fl_get(S, 1, &F);
      n = dm_fl_text_fight(&F, out, sizeof out);
      int traces = 0, acts = 0, en = 0, drinks = 0, states = 0, kills = 0, fights = 0; unsigned end = 0, width = 0, nb = 0;
      for (char *l = strtok_r(out, "\n", &save); l; l = strtok_r(NULL, "\n", &save)) {
          if (!strncmp(l, "fight\t", 6)) { fights++; int kind; sscanf(l, "fight\t%d\t%u\t%u\t%u", &kind, &end, &width, &nb); CHECK(kind == 1, "a kill"); }
          traces += !strncmp(l, "trace\t", 6); acts += !strncmp(l, "act\t", 4); drinks += !strncmp(l, "drink\t", 6); states += !strncmp(l, "state\t", 6); kills += !strncmp(l, "kill\t", 5);
          if (!strncmp(l, "en\t", 3)) { unsigned k, e, m; sscanf(l, "en\t%u\t%u\t%u", &k, &e, &m); en |= (int)e | (int)m << 8; }
          if (!strncmp(l, "act\t", 4)) CHECK(!strcmp(l, "act\t0\t52"), "the cast skill: %s", l);
      }
      CHECK(n > 0 && fights == 1 && end == F.h.lead + F.h.duration && width == F.h.width && nb == F.h.nbuckets && traces == F.h.nbuckets, "fight line and one trace per bucket (%u %u)", end, nb);
      CHECK(acts == 1 && drinks == 1 && states == 2 && kills == 1, "acts %d drinks %d states %d kills %d", acts, drinks, states, kills);
      CHECK((en & DM_FL_EN_HIT) && (en & (DM_FL_ME_CRIT << 8)), "the enemy's hit and the own crit are sent (%#x)", en);
      dm_fl_get(S, 0, &F); n = dm_fl_text_fight(&F, out, sizeof out); int glife = 0;
      for (char *l = strtok_r(out, "\n", &save); l; l = strtok_r(NULL, "\n", &save)) if (!strncmp(l, "glife\t", 6)) { glife++; int commas = 0; for (char *q = l; *q; q++) commas += *q == ','; CHECK(commas == F.h.nbuckets - 1, "one value per bucket"); }
      CHECK(n > 0 && glife == 2, "a line per group (%d)", glife);
      CHECK(dm_fl_text_fight(&F, out, 64) == -2 && !out[0], "too small: nothing"); }

    printf("t_fightlog: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
