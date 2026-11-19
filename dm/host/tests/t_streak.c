





#include "../dm_streak_core.c"
#include "../dm_streak_hist.c"
#include "../dm_i18n_tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

static dm_sk_state S;
static uint32_t now, next_id;
static dm_sk_end E[2];
static dm_sk_end L;
static int nend, dead, got;

static int tick(void) { now++; nend = dm_sk_tick(&S, now, dead, E); if (nend) { L = E[nend - 1]; got += nend; } return nend; }
static void ticks(int n) { for (int i = 0; i < n; i++) tick(); }

static uint32_t kill_xp(uint32_t xp) { uint32_t id = ++next_id; dm_sk_credit(&S, now + 1, id, xp); dm_sk_kill(&S, now + 1, id, 0); return id; }
static void fresh(void) { dm_sk_reset(&S); now += 1000; tick(); got = 0; }

static void run(int n, int gap, uint32_t xp) { for (int i = 0; i < n; i++) { kill_xp(xp); tick(); if (i + 1 < n) ticks(gap - 1); } }
static int ended_after(int max) { for (int i = 0; i < max; i++) if (tick()) return 1; return 0; }

int main(void) {

    CHECK(dm_sk_pct(DM_SK_START - 1) == 0 && dm_sk_pct(DM_SK_START) == 10 && dm_sk_pct(11) == 10 && dm_sk_pct(12) == 15 && dm_sk_pct(19) == 15 && dm_sk_pct(20) == 20 && dm_sk_pct(29) == 20, "steps");
    CHECK(dm_sk_pct(30) == 25 && dm_sk_pct(44) == 25 && dm_sk_pct(45) == 30 && dm_sk_pct(64) == 30 && dm_sk_pct(65) == 35 && dm_sk_pct(99) == 35 && dm_sk_pct(100) == 40, "steps up to 100");
    CHECK(dm_sk_pct(149) == 40 && dm_sk_pct(150) == 50 && dm_sk_pct(249) == 50 && dm_sk_pct(250) == 60 && dm_sk_pct(100000) == 60, "cap at 250 kills");
    CHECK(dm_sk_tier(DM_SK_START - 1) == 0 && dm_sk_tier(DM_SK_START) == 1 && dm_sk_tier(12) == 2 && dm_sk_tier(20) == 3 && dm_sk_tier(149) == 7 && dm_sk_tier(250) == DM_SK_TIERS && dm_sk_tier(100000) == DM_SK_TIERS, "tiers");
    CHECK(dm_sk_bonus(8747, 25) == 2186 && dm_sk_bonus(0, 60) == 0 && dm_sk_bonus(5000000000ull, 60) == 3000000000ull, "bonus = base * pct / 100, no overflow");


    fresh();
    run(7, 10, 100); uint32_t k; int left;
    CHECK(!dm_sk_shown(&S, now, &k, &left), "7 kills: no counter");
    CHECK(!ended_after(DM_SK_WINDOW + 5), "7 kills end silently");
    fresh();
    run(8, 10, 100);
    CHECK(dm_sk_shown(&S, now, &k, &left) && k == 8 && left == DM_SK_WINDOW, "8 kills: counter %u, left %d", k, left);
    ticks(DM_SK_WINDOW - 1);
    CHECK(dm_sk_shown(&S, now, &k, &left) && left == 1, "left counts down: %d", left);
    CHECK(tick() == 0 && dm_sk_shown(&S, now, &k, &left) && left == 0, "last tick of the window still runs");
    CHECK(tick() == 1 && E[0].kind == DM_SK_END_TIME && E[0].kills == 8 && E[0].base == 800 && E[0].pct == 10, "ends: kind %d kills %u base %llu", E[0].kind, E[0].kills, (unsigned long long)E[0].base);
    CHECK(!dm_sk_shown(&S, now, &k, &left), "no counter after the end");


    for (int gap = DM_SK_WINDOW - 1; gap <= DM_SK_WINDOW + 1; gap++) {
        fresh(); run(8, gap, 10);
        int alive = dm_sk_shown(&S, now, &k, &left);
        CHECK(alive == (gap <= DM_SK_WINDOW), "gap %d: streak %s", gap, alive ? "kept" : "broken");
    }


    #define HIT(n) (dm_sk_hits(&S, now + 1, n), tick())
    fresh(); run(8, 5, 100); ticks(40);
    CHECK(dm_sk_shown(&S, now, &k, &left) && left == DM_SK_WINDOW - 40, "40 ticks after the kill: %d left", left);
    HIT(1);
    CHECK(dm_sk_shown(&S, now, &k, &left) && left == DM_SK_WINDOW && k == 8, "a hit fills the countdown: %d left", left);
    ticks(30);
    CHECK(dm_sk_shown(&S, now, &k, &left) && left == DM_SK_WINDOW - 30, "it runs down from the hit: %d left", left);
    HIT(10);
    CHECK(dm_sk_shown(&S, now, &k, &left) && left == DM_SK_WINDOW && k == 8, "many hits on one tick: the full countdown, %d", left);
    ticks(DM_SK_WINDOW - 1); HIT(1);
    CHECK(dm_sk_shown(&S, now, &k, &left) && left == DM_SK_WINDOW && !got, "a hit on the countdown's last tick fills it again: %d", left);
    ticks(DM_SK_WINDOW / 2); kill_xp(100); tick();
    CHECK(dm_sk_shown(&S, now, &k, &left) && left == DM_SK_WINDOW && k == 9, "a kill fills it: %d", left);

    fresh(); run(8, 5, 100); { int alive = 1; for (int i = 0; i < 30 && alive; i++) { ticks(89); HIT(1); alive = dm_sk_shown(&S, now, &k, &left) && !got; }
    CHECK(alive && k == 8 && S.last + 30 * 90 <= now, "2700 ticks of hits without a kill: the streak runs"); }
    { uint32_t last_hit = now; int n = 0; while (!tick() && n < 2 * DM_SK_WINDOW) n++;
      CHECK(got == 1 && L.kills == 8 && L.kind == DM_SK_END_TIME && now - last_hit == DM_SK_WINDOW + 1, "without hits it runs out a window after the last hit: %u", now - last_hit); }

    fresh(); run(8, 5, 100); { uint32_t last = S.last; ticks(80); HIT(3); ticks(80); HIT(1);
      while (!tick()) ;
      CHECK(got == 1 && L.kills == 8 && L.kind == DM_SK_END_TIME && L.last == last, "hits do not move the last kill: %u for %u", L.last, last); }

    fresh(); run(8, 5, 100); ticks(DM_SK_WINDOW); HIT(5);
    CHECK(got == 1 && !dm_sk_shown(&S, now, &k, &left) && S.kills == 0, "a hit after the end does not bring it back");
    fresh(); HIT(5); CHECK(S.kills == 0 && !got, "hits alone start nothing");

    fresh(); run(2, 5, 100); ticks(90); HIT(1); ticks(15); kill_xp(100); tick();
    CHECK(S.kills == 3, "a hit kept the first kills: %u", S.kills);
    #undef HIT


    { for (int t = 0; t < DM_SK_TIERS; t++)
          CHECK(dm_sk_tier(k_steps[t].kills - 1) == t && dm_sk_tier(k_steps[t].kills) == t + 1, "tier boundary at %u kills: %d / %d", k_steps[t].kills, dm_sk_tier(k_steps[t].kills - 1), dm_sk_tier(k_steps[t].kills));
      char out[1024], want[64];
      snprintf(want, sizeof want, "tier\t1\t%u\t%d\ntier\t2\t%u\t%d\ntier\t3\t%u\t%d\n", k_steps[0].kills, k_steps[0].pct, k_steps[1].kills, k_steps[1].pct, k_steps[2].kills, k_steps[2].pct);
      int len = dm_sk_tiers_text(out, sizeof out);
      CHECK(len > 0 && (size_t)len == strlen(out) && !strncmp(out, want, strlen(want)) && strstr(out, "\ntier\t9\t250\t60\n") && !strstr(out, "tier\t10"), "tier lines: %.80s", out);
      CHECK(dm_sk_tiers_text(out, 30) == -2 && !out[0], "too small a buffer");
      const char *title[DM_SK_TIERS][9] = { { 0 } }; int extra = 0;
      for (size_t i = 0; i < sizeof k_dm_i18n / sizeof *k_dm_i18n; i++) {
          int t = 0;
          if (sscanf(k_dm_i18n[i].k, "dmAndroidStreakTier%d", &t) != 1) continue;
          if (t < 1 || t > DM_SK_TIERS || title[t - 1][0]) { extra++; continue; }
          const char *cols[9] = { k_dm_i18n[i].zh, k_dm_i18n[i].en, k_dm_i18n[i].es, k_dm_i18n[i].mx, k_dm_i18n[i].ru, k_dm_i18n[i].ko, k_dm_i18n[i].de, k_dm_i18n[i].pt, k_dm_i18n[i].k };
          memcpy(title[t - 1], cols, sizeof cols);
      }
      CHECK(!extra, "%d title keys beyond the tiers", extra);
      for (int t = 0; t < DM_SK_TIERS; t++) {
          CHECK(title[t][0] != NULL, "no title key for tier %d", t + 1);
          if (!title[t][0]) continue;
          for (int l = 0; l < 8; l++) {
              CHECK(title[t][l][0] && !strchr(title[t][l], '%'), "tier %d: empty or formatted title in column %d", t + 1, l);
              for (int u = 0; u < t; u++) if (title[u][0]) CHECK(strcmp(title[t][l], title[u][l]), "tiers %d and %d share the title %s", u + 1, t + 1, title[t][l]);
          }
      } }


    fresh(); run(7, 3, 100);
    dm_sk_credit(&S, now + 1, 7777, 50); CHECK(dm_sk_kill(&S, now + 1, 7777, 0) == 1, "unit 7777 counts"); tick(); ticks(2);
    dm_sk_credit(&S, now + 1, 7777, 50); CHECK(dm_sk_kill(&S, now + 1, 7777, 0) == 0, "revived unit 7777 does not count again"); tick();
    ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.kills == 8 && L.base == 750, "the revived death's credit is not in the base: %llu", (unsigned long long)L.base);
    run(7, 3, 100); dm_sk_credit(&S, now + 1, 7777, 50); CHECK(dm_sk_kill(&S, now + 1, 7777, 0) == 1, "unit 7777 in the next streak counts again"); tick();
    ticks(DM_SK_WINDOW + 1);
    CHECK(got == 2 && L.kills == 8 && L.base == 750, "with its new credit: %llu", (unsigned long long)L.base);


    fresh(); run(12, 5, 100); dead = 1;
    CHECK(tick() == 1 && E[0].kind == DM_SK_END_DEATH && E[0].kills == 12, "death ends the streak");
    dead = 0; fresh(); run(3, 5, 100); dead = 1;
    CHECK(tick() == 0, "death of a short streak is silent"); dead = 0;


    fresh(); run(10, 5, 100); uint32_t at = now;
    CHECK(dm_sk_shown(&S, at, &k, &left) && left == DM_SK_WINDOW, "paused: unchanged");
    run(10, 30, 100);
    CHECK(dm_sk_shown(&S, now, &k, &left) && k == 20, "after an area change the streak goes on: %u", k);



    fresh();
    ticks(1);
    for (int i = 0; i < 10; i++) { kill_xp(100); if (i == 4) dm_sk_credit(&S, now + 1, 900000 + i, 700); tick(); ticks(2); }
    ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.kills == 10 && L.base == 1000, "only the counted kills' credits: base %llu", (unsigned long long)L.base);

    fresh();
    for (int i = 0; i < 10; i++) { kill_xp(i == 3 ? 1500 : i == 6 ? 7 : 100); tick(); ticks(2); }
    ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.base == 2307, "champion and reduced reward as credited: base %llu", (unsigned long long)L.base);


    fresh();
    for (int i = 0; i < 10; i++) {
        uint32_t id = ++next_id;
        if (i == 2) { dm_sk_credit(&S, now + 1, id, 100); ticks(DM_SK_SEEN); dm_sk_kill(&S, now + 1, id, 0); tick(); }
        else if (i == 5) { dm_sk_kill(&S, now + 1, id, 0); tick(); dm_sk_credit(&S, now + 1, id, 100); tick(); }
        else { dm_sk_credit(&S, now + 1, id, 100); dm_sk_kill(&S, now + 1, id, 0); tick(); }
        ticks(3);
    }
    ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.base == 1000, "early and late reports: base %llu", (unsigned long long)L.base);

    fresh(); run(8, 3, 100);
    { uint32_t id = ++next_id; dm_sk_credit(&S, now + 1, id, 100); ticks(DM_SK_SEEN + 1); dm_sk_kill(&S, now + 1, id, 0); tick(); }
    { uint32_t id = ++next_id; dm_sk_kill(&S, now + 1, id, 0); ticks(2); dm_sk_credit(&S, now + 1, id, 100); tick(); }
    ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.kills == 10 && L.base == 800, "too early and too late are not taken: %llu", (unsigned long long)L.base);


    fresh(); run(10, 5, 100); ticks(DM_SK_WINDOW);
    kill_xp(300); CHECK(tick() == 1 && E[0].base == 1000, "the old streak ends on the new kill's tick: %llu", (unsigned long long)E[0].base);
    run(9, 5, 100); ticks(DM_SK_WINDOW + 1);
    CHECK(got == 2 && L.kills == 10 && L.base == 1200, "the new streak's base: %llu", (unsigned long long)L.base);


    fresh(); run(10, 5, 300000000); ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.base == 3000000000ull, "large credits: %llu", (unsigned long long)L.base);
    fresh(); run(15, 5, 0); ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.base == 0 && L.kills == 15, "no credits: base 0");


    fresh(); for (int i = 0; i < 10; i++) { uint32_t id = ++next_id; dm_sk_credit(&S, now + 1, id, 100); dm_sk_kill(&S, now + 1, id, i == 7); tick(); }
    ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.no_bonus && L.kills == 10, "no bonus after a rift kill");


    fresh();
    for (int i = 0; i < DM_SK_CREDITS + 5; i++) dm_sk_credit(&S, now + 1, 500000 + (uint32_t)i, 10);
    CHECK(S.dropped == 5 && S.ncred == DM_SK_CREDITS, "full table: %u dropped", S.dropped);
    for (int i = 0; i < 10; i++) dm_sk_kill(&S, now + 1, 500000 + (uint32_t)i, 0);
    tick(); ticks(DM_SK_WINDOW + 1);
    CHECK(got == 1 && L.base == 100 && S.ncred == 0, "waiting credits matched, the rest expired: base %llu, left %d", (unsigned long long)L.base, S.ncred);


    fresh();
    for (int i = 0; i < DM_SK_IDS + 100; i++) { dm_sk_kill(&S, now + 1, ++next_id, 0); if (i % 50 == 49) tick(); }
    tick(); CHECK(dm_sk_shown(&S, now, &k, &left) && k == DM_SK_IDS + 100 && S.ids_full, "a long streak: %u", k);


    { dm_sk_hist *H = dm_sk_hist_core(); dm_sk_rec r, got_r; static uint8_t img[DM_SK_IMAGE_MAX], bad[DM_SK_IMAGE_MAX + 1]; char who[64]; uint32_t gen;
      dm_sk_hist_owner(H, "Hero", NULL, 0);
      CHECK(dm_sk_hist_count(H) == 0 && !dm_sk_hist_record(H, 0, &got_r) && !dm_sk_hist_image(H, who, img, sizeof img, &gen), "a new owner starts empty and saved");
      #define REC(k, d, e, b) (r = (dm_sk_rec){ k, 100, b, 41, d, e }, &r)
      CHECK(!dm_sk_hist_add(H, REC(12, 0, DM_SK_END_TIME, 50)), "the first streak of a difficulty beats nothing");
      CHECK(dm_sk_hist_record(H, 0, &got_r) && got_r.kills == 12 && got_r.area == 41, "but sets its record");
      CHECK(!dm_sk_hist_add(H, REC(12, 0, DM_SK_END_TIME, 50)), "as many kills as the record do not beat it");
      CHECK(dm_sk_hist_add(H, REC(13, 0, DM_SK_END_DEATH, 0)), "one more kill beats it, also when it ended in death");
      CHECK(!dm_sk_hist_add(H, REC(11, 1, DM_SK_END_TIME, 0)) && dm_sk_hist_record(H, 1, &got_r) && got_r.kills == 11, "each difficulty has its own record");
      CHECK(dm_sk_hist_record(H, 0, &got_r) && got_r.kills == 13 && got_r.end == DM_SK_END_DEATH, "the record of Normal is 13");
      CHECK(!dm_sk_hist_add(H, REC(7, 0, DM_SK_END_TIME, 0)) && !dm_sk_hist_add(H, REC(40, 3, DM_SK_END_TIME, 0)) && dm_sk_hist_count(H) == 4, "no streak below 8 kills, no unknown difficulty");
      CHECK(!dm_sk_hist_add(H, REC(8, 1, DM_SK_END_TIME, 0)) && dm_sk_hist_count(H) == 5, "a streak of 8 is kept");
      for (int i = 0; i < DM_SK_RING; i++) dm_sk_hist_add(H, REC(20 + (uint32_t)i, 2, DM_SK_END_TIME, 7));
      CHECK(dm_sk_hist_count(H) == DM_SK_RING && dm_sk_hist_get(H, 0, &got_r) && got_r.kills == 20 + DM_SK_RING - 1 && dm_sk_hist_get(H, DM_SK_RING - 1, &got_r) && got_r.kills == 20,
            "the ring keeps the last %d, newest first", DM_SK_RING);
      CHECK(dm_sk_hist_record(H, 0, &got_r) && got_r.kills == 13, "a record stays when its streak leaves the ring");

      size_t n = dm_sk_hist_image(H, who, img, sizeof img, &gen);
      CHECK(n == DM_SK_IMAGE_MAX && !strcmp(who, "Hero"), "image of %zu bytes", n);
      dm_sk_rec before[DM_SK_RING], best[DM_SK_DIFFS];
      for (int i = 0; i < DM_SK_RING; i++) dm_sk_hist_get(H, i, &before[i]);
      for (int d = 0; d < DM_SK_DIFFS; d++) dm_sk_hist_record(H, d, &best[d]);
      CHECK(!dm_sk_hist_owner(H, "Hero", img, n) && dm_sk_hist_count(H) == DM_SK_RING, "the image loads");
      int same = 1; for (int i = 0; i < DM_SK_RING; i++) { dm_sk_hist_get(H, i, &got_r); same &= !memcmp(&got_r, &before[i], sizeof got_r); }
      for (int d = 0; d < DM_SK_DIFFS; d++) { dm_sk_hist_record(H, d, &got_r); same &= !memcmp(&got_r, &best[d], sizeof got_r); }
      CHECK(same, "every streak and record comes back as it was");
      CHECK(!dm_sk_hist_image(H, who, img, sizeof img, &gen), "a loaded history counts as saved");
      dm_sk_hist_add(H, REC(14, 0, DM_SK_END_TIME, 3)); n = dm_sk_hist_image(H, who, img, sizeof img, &gen);
      #define REJECT(why, edit, crc) do { memcpy(bad, img, n); size_t m = n; edit; if (crc) sk_put32(bad + 12, sk_crc32(bad + DM_SK_IMAGE_HEAD, m - DM_SK_IMAGE_HEAD)); \
          int rr = dm_sk_hist_owner(H, "Hero", bad, m); CHECK(rr == (why) && dm_sk_hist_count(H) == 0 && !dm_sk_hist_record(H, 0, &got_r), "rejected as %d (%d)", why, rr); } while (0)
      size_t s0 = DM_SK_IMAGE_HEAD + DM_SK_DIFFS * sizeof(dm_sk_rec), b0 = DM_SK_IMAGE_HEAD;
      REJECT(DM_SK_BAD_SIZE, m = 10, 0);
      REJECT(DM_SK_BAD_SIZE, m = n - 1, 1);
      REJECT(DM_SK_BAD_SIZE, (bad[n] = 0, m = n + 1), 1);
      REJECT(DM_SK_BAD_MAGIC, bad[0] ^= 1, 0);
      REJECT(DM_SK_BAD_VERSION, bad[4] = 2, 0);
      REJECT(DM_SK_BAD_CRC, bad[n - 1] ^= 0x40, 0);
      REJECT(DM_SK_BAD_COUNT, sk_put32(bad + 8, DM_SK_RING + 1), 1);
      REJECT(DM_SK_BAD_COUNT, sk_put32(bad + 8, 0xffffffffu), 1);
      REJECT(DM_SK_BAD_SIZE, sk_put32(bad + 8, DM_SK_RING - 1), 1);
      REJECT(DM_SK_BAD_RECORD, sk_put32(bad + s0, 5), 1);
      REJECT(DM_SK_BAD_RECORD, bad[s0 + offsetof(dm_sk_rec, difficulty)] = 3, 1);
      REJECT(DM_SK_BAD_RECORD, bad[s0 + offsetof(dm_sk_rec, end)] = 0, 1);
      REJECT(DM_SK_BAD_RECORD, (bad[s0 + offsetof(dm_sk_rec, end)] = DM_SK_END_DEATH, sk_put32(bad + s0 + offsetof(dm_sk_rec, bonus), 5)), 1);
      REJECT(DM_SK_BAD_RECORD, sk_put32(bad + s0 + offsetof(dm_sk_rec, bonus), 0x80000000u), 1);
      REJECT(DM_SK_BAD_RECORD, sk_put32(bad + s0, 10000), 1);
      REJECT(DM_SK_BAD_RECORD, bad[b0 + offsetof(dm_sk_rec, difficulty)] = 1, 1);
      REJECT(DM_SK_BAD_RECORD, (memset(bad + b0 + sizeof(dm_sk_rec), 0, sizeof(dm_sk_rec)), bad[b0 + sizeof(dm_sk_rec) + offsetof(dm_sk_rec, area)] = 1), 1);
      #undef REJECT
      CHECK(!dm_sk_hist_owner(H, "Hero", img, n) && dm_sk_hist_count(H) == DM_SK_RING, "the good image still loads");

      dm_sk_hist_add(H, REC(15, 0, DM_SK_END_TIME, 0)); size_t n2 = dm_sk_hist_image(H, who, img, sizeof img, &gen);
      CHECK(n2 > 0, "a new streak makes it unsaved");
      dm_sk_hist_saved(H, gen); CHECK(!dm_sk_hist_image(H, who, img, sizeof img, &gen), "saved");
      dm_sk_hist_unsaved(H); CHECK(dm_sk_hist_image(H, who, img, sizeof img, &gen) == n2, "a refused save makes it unsaved"); dm_sk_hist_saved(H, gen);

      { char out[4096]; int len = dm_sk_hist_text(H, out, sizeof out);
        CHECK(len > 0 && !strncmp(out, "streaks\t20\nbest\t0\t15\t100\t0\t41\t1\nbest\t1\t11\t", 40) && strstr(out, "\ns\t0\t15\t100\t0\t41\t0\t1\n"), "text: %.80s", out);
        CHECK(strstr(out, "best\t2\t") != NULL && dm_sk_hist_text(H, out, 20) == -2 && !out[0], "every difficulty with a record; too small a buffer"); }

      { dm_sk_rec body[DM_SK_DIFFS + 1]; uint8_t old[DM_SK_IMAGE_HEAD + sizeof body]; size_t m = sizeof old;
        memset(body, 0, sizeof body);
        body[0] = (dm_sk_rec){ 7, 200, 0, 41, 0, DM_SK_END_TIME };
        body[DM_SK_DIFFS] = (dm_sk_rec){ 6, 100, 0, 41, 0, DM_SK_END_TIME };
        sk_put32(old, DM_SK_MAGIC); sk_put32(old + 4, DM_SK_VERSION); sk_put32(old + 8, 1);
        memcpy(old + DM_SK_IMAGE_HEAD, body, sizeof body);
        sk_put32(old + 12, sk_crc32(old + DM_SK_IMAGE_HEAD, m - DM_SK_IMAGE_HEAD));
        CHECK(!dm_sk_hist_owner(H, "Hero", old, m) && dm_sk_hist_count(H) == 1, "the stored 6-kill streak loads");
        CHECK(dm_sk_hist_get(H, 0, &got_r) && got_r.kills == 6 && dm_sk_hist_record(H, 0, &got_r) && got_r.kills == 7, "the 7-kill record comes back with it"); }
      #undef REC
    }


    { dm_sk_hist *H = dm_sk_hist_core(); dm_sk_rec r = { 18, 50, 9, 2, 0, DM_SK_END_TIME }, got_r;
      char root[256], dir[300], file[400], aside[420]; const char *tmp = getenv("TMPDIR"); struct stat st;
      snprintf(root, sizeof root, "%s/t_streak.XXXXXX", tmp && tmp[0] ? tmp : "/tmp"); CHECK(mkdtemp(root) != NULL, "temporary folder");
      snprintf(dir, sizeof dir, "%s/Hero", root); snprintf(file, sizeof file, "%s/Hero.sw_streaks", dir); snprintf(aside, sizeof aside, "%s.bad", file);
      CHECK(!dm_sk_hist_load(H, root, "Hero") && dm_sk_hist_count(H) == 0, "no file: empty");
      dm_sk_hist_add(H, &r);
      CHECK(!dm_sk_hist_commit(H, root, 1) && stat(file, &st), "no character folder: nothing written");
      mkdir(dir, 0700);
      CHECK(dm_sk_hist_commit(H, root, 1) && !stat(file, &st) && st.st_size == DM_SK_IMAGE_HEAD + 4 * (long)sizeof r, "written into the character folder");
      CHECK(!dm_sk_hist_commit(H, root, 2), "unchanged: not written again");
      r.kills = 30;
      CHECK(!dm_sk_hist_keep(H, &r, 0) && dm_sk_hist_count(H) == 1 && dm_sk_hist_record(H, 0, &got_r) && got_r.kills == 18 && !dm_sk_hist_commit(H, root, 2),
            "while the fight log does not record: no new streak, no record, nothing to write");
      r.kills = 25; CHECK(dm_sk_hist_keep(H, &r, 1), "recording again: kept, and it beats the record");
      CHECK(dm_sk_hist_commit(H, root, 2) && !stat(file, &st), "a new streak: written again");
      unlink(file);
      CHECK(dm_sk_hist_commit(H, root, 2) && !stat(file, &st), "after a refused commit the file is written again");
      CHECK(!dm_sk_hist_commit(H, root, 3), "after a successful commit nothing is pending");
      dm_sk_hist_owner(H, "", NULL, 0);
      CHECK(!dm_sk_hist_load(H, root, "Hero") && dm_sk_hist_count(H) == 2 && dm_sk_hist_record(H, 0, &got_r) && got_r.kills == 25, "loaded on the next entry (%d)", dm_sk_hist_count(H));
      CHECK(!dm_sk_hist_follow(H, root, "Hero") && dm_sk_hist_count(H) == 2, "the same character keeps its history");
      CHECK(dm_sk_hist_follow(H, root, "Other") && dm_sk_hist_count(H) == 0 && !dm_sk_hist_record(H, 0, &got_r), "another character starts with its own");
      CHECK(dm_sk_hist_follow(H, root, "Hero") && dm_sk_hist_count(H) == 2, "the first character's file comes back");
      FILE *f = fopen(file, "r+b"); fseek(f, 20, SEEK_SET); fputc(0x5a, f); fclose(f);
      dm_sk_hist_owner(H, "", NULL, 0);
      CHECK(dm_sk_hist_load(H, root, "Hero") == DM_SK_BAD_CRC && dm_sk_hist_count(H) == 0 && !stat(aside, &st) && stat(file, &st), "a damaged file is set aside");
      CHECK(!dm_sk_hist_commit(H, root, 4) && stat(file, &st), "an empty loaded history is not written");
      unlink(aside); rmdir(dir); rmdir(root); }


    { int chat = 0; for (size_t i = 0; i < sizeof k_dm_i18n / sizeof *k_dm_i18n; i++) chat += !strncmp(k_dm_i18n[i].k, "dmStreakChat", 12) || !strcmp(k_dm_i18n[i].k, "dmStreakRecord");
      CHECK(!chat, "%d chat line texts in the table", chat); }

    printf("t_streak: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
