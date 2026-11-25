


#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <unistd.h>
#include "../dm_itemjob.h"

static int checks, fails;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static _Atomic int g_go, g_won[2];
static void *racer(void *arg) {
    int me = (int)(long)arg, kind = me ? DM_ITEMJOB_EQUIP : DM_ITEMJOB_RUNEWORD;
    while (!atomic_load(&g_go)) ;
    if (dm_itemjob_acquire(kind)) atomic_store(&g_won[me], 1);
    return NULL;
}


typedef struct { int arrive, placed_at, source_left; } story;
static int play(const story *st, int tracked, int limit, int *ticks, int *backs, int *released_early) {
    dm_itemjob_settle s; dm_itemjob_settle_begin(&s, limit, 120);
    int in_hand = tracked && st->arrive < 0, pending = !tracked, placed_t = -1; *backs = 0; *released_early = 0;
    for (int t = 0; t < 10 * limit; t++) {
        if (pending && st->arrive >= 0 && t >= st->arrive) { pending = 0; in_hand = 1; tracked = 1; }
        if (in_hand && placed_t >= 0 && st->placed_at >= 0 && t >= placed_t + st->placed_at) in_hand = 0;
        dm_itemjob_seen v = { in_hand ? DM_ITEMJOB_HAND_OWN : DM_ITEMJOB_HAND_EMPTY, tracked, tracked && !in_hand && placed_t >= 0, pending,
                              pending && !(st->source_left >= 0 && t >= st->source_left) };
        int r = dm_itemjob_settle_step(&s, &v);
        if (r == DM_ITEMJOB_PUT_BACK) { (*backs)++; if (placed_t < 0) placed_t = t; }
        else if (r != DM_ITEMJOB_WAIT) { *ticks = t; if (in_hand && r == DM_ITEMJOB_SETTLED) *released_early = 1; return r; }
    }
    return -1;
}

static _Atomic int g_saved, g_save_go;
static void *saver(void *arg) {
    (void)arg; while (!atomic_load(&g_save_go)) usleep(100);
    while (dm_itemjob_owner() != DM_ITEMJOB_NONE) usleep(100);
    atomic_store(&g_saved, 1); return NULL;
}

int main(void) {
    CHECK(dm_itemjob_owner() == DM_ITEMJOB_NONE);
    CHECK(!dm_itemjob_acquire(DM_ITEMJOB_NONE) && !dm_itemjob_acquire(DM_ITEMJOB_N) && !dm_itemjob_acquire(-1));
    CHECK(dm_itemjob_acquire(DM_ITEMJOB_RECIPE) && dm_itemjob_owner() == DM_ITEMJOB_RECIPE);
    CHECK(!dm_itemjob_acquire(DM_ITEMJOB_RECIPE));
    CHECK(!dm_itemjob_acquire(DM_ITEMJOB_EQUIP) && !dm_itemjob_acquire(DM_ITEMJOB_RUNEWORD));
    CHECK(!dm_itemjob_release(DM_ITEMJOB_EQUIP) && dm_itemjob_owner() == DM_ITEMJOB_RECIPE);
    CHECK(!dm_itemjob_release(DM_ITEMJOB_NONE) && dm_itemjob_owner() == DM_ITEMJOB_RECIPE);
    unsigned ends = dm_itemjob_ends();
    CHECK(dm_itemjob_release(DM_ITEMJOB_RECIPE) && dm_itemjob_owner() == DM_ITEMJOB_NONE);
    CHECK(dm_itemjob_ends() == ends + 1);
    CHECK(!dm_itemjob_release(DM_ITEMJOB_RECIPE));
    CHECK(dm_itemjob_ends() == ends + 1);
    CHECK(dm_itemjob_acquire(DM_ITEMJOB_EQUIP) && dm_itemjob_owner() == DM_ITEMJOB_EQUIP && dm_itemjob_release(DM_ITEMJOB_EQUIP));
    int both = 0, none = 0;
    for (int i = 0; i < 2000; i++) {
        pthread_t t[2]; atomic_store(&g_go, 0); atomic_store(&g_won[0], 0); atomic_store(&g_won[1], 0);
        for (long k = 0; k < 2; k++) pthread_create(&t[k], NULL, racer, (void *)k);
        atomic_store(&g_go, 1);
        for (int k = 0; k < 2; k++) pthread_join(t[k], NULL);
        int w0 = atomic_load(&g_won[0]), w1 = atomic_load(&g_won[1]);
        both += w0 && w1; none += !w0 && !w1;
        dm_itemjob_release(w0 ? DM_ITEMJOB_RUNEWORD : DM_ITEMJOB_EQUIP);
    }
    CHECK(both == 0 && none == 0 && dm_itemjob_owner() == DM_ITEMJOB_NONE);

    int t = 0, backs = 0, early = 0;
    { dm_itemjob_settle s; dm_itemjob_settle_begin(&s, 600, 120); dm_itemjob_seen v = { 0 };
      CHECK(dm_itemjob_settle_step(&s, &v) == DM_ITEMJOB_SETTLED);
      v.hand = DM_ITEMJOB_HAND_OTHER; dm_itemjob_settle_begin(&s, 600, 120); CHECK(dm_itemjob_settle_step(&s, &v) == DM_ITEMJOB_OTHER); }
    story late_pick = { 300, 1, -1 };
    CHECK(play(&late_pick, 0, 600, &t, &backs, &early) == DM_ITEMJOB_SETTLED && t > 300 && backs == 1 && !early);
    story never = { -1, -1, -1 };
    CHECK(play(&never, 0, 600, &t, &backs, &early) == DM_ITEMJOB_SETTLED && t == 599 && backs == 0);
    story taken = { -1, -1, 50 };
    CHECK(play(&taken, 0, 600, &t, &backs, &early) == DM_ITEMJOB_UNPLACED && t == 599);
    story stuck = { -1, -1, -1 };
    CHECK(play(&stuck, 1, 600, &t, &backs, &early) == DM_ITEMJOB_IN_HAND && t == 599 && backs == 2);
    story slow_back = { -1, 200, -1 };
    CHECK(play(&slow_back, 1, 600, &t, &backs, &early) == DM_ITEMJOB_SETTLED && t >= 200 && backs == 2 && !early);
    { dm_itemjob_settle s; dm_itemjob_settle_begin(&s, 600, 120);
      dm_itemjob_seen v = { DM_ITEMJOB_HAND_EMPTY, 1, 0, 0, 0 }; int r = DM_ITEMJOB_WAIT;
      for (int k = 0; k < 400 && r == DM_ITEMJOB_WAIT; k++) r = dm_itemjob_settle_step(&s, &v);
      CHECK(r == DM_ITEMJOB_WAIT); v.placed = 1; CHECK(dm_itemjob_settle_step(&s, &v) == DM_ITEMJOB_SETTLED); }


    { pthread_t sv; atomic_store(&g_saved, 0); atomic_store(&g_save_go, 0);
      CHECK(dm_itemjob_acquire(DM_ITEMJOB_RUNEWORD)); pthread_create(&sv, NULL, saver, NULL); atomic_store(&g_save_go, 1);
      dm_itemjob_settle s; dm_itemjob_settle_begin(&s, 600, 120); int r = DM_ITEMJOB_WAIT, saved_early = 0, k;
      for (k = 0; k < 2000 && r != DM_ITEMJOB_SETTLED; k++) {
          dm_itemjob_seen v = { k >= 300 && k < 302 ? DM_ITEMJOB_HAND_OWN : DM_ITEMJOB_HAND_EMPTY, k >= 300, k >= 302, k < 300, k < 300 };
          r = dm_itemjob_settle_step(&s, &v); usleep(50); saved_early |= atomic_load(&g_saved); }
      CHECK(r == DM_ITEMJOB_SETTLED && k > 300 && !saved_early);
      dm_itemjob_release(DM_ITEMJOB_RUNEWORD); pthread_join(sv, NULL); CHECK(atomic_load(&g_saved) && dm_itemjob_owner() == DM_ITEMJOB_NONE); }
    printf("t_itemjob: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
