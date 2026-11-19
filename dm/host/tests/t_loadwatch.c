




#include <stdio.h>
#include "dm_loadwatch.h"
static int fails, cases;
static void expect(const char *what, long got, long want) {
    cases++; if (got == want) printf("✓ %s\n", what); else { fails++; printf("✗ %s: got %ld, want %ld\n", what, got, want); }
}
#define S 1000000000ull
int main(void) {

    static const struct { int wants, saving, loading, want; const char *why; } rows[] = {
        {0, 0, 0, 0, "no wish: runs"},
        {0, 0, 1, 0, "no wish, loading: runs"},
        {0, 1, 0, 0, "no wish, leave save: runs"},
        {0, 1, 1, 0, "no wish, leave save, loading: runs"},
        {1, 0, 0, 1, "Quick Menu wish in a game: held"},
        {1, 0, 1, 0, "Quick Menu wish on the loading screen: runs (the load needs ticks)"},
        {4, 1, 0, 0, "left-app wish during the leave save: runs (the save needs ticks)"},
        {4, 1, 1, 0, "left-app wish, leave save, loading: runs"},
        {4, 0, 1, 0, "left-app wish on the death respawn loading screen: runs"},
        {16, 0, 0, 1, "tech page wish (bit 16) in a game: held"},
        {2 | 8, 0, 0, 1, "recipe book + build guide wishes: held"},
    };
    for (unsigned i = 0; i < sizeof rows / sizeof rows[0]; i++) expect(rows[i].why, dm_pause_holds(rows[i].wants, rows[i].saving, rows[i].loading), rows[i].want);



    dm_pause_state p = {0};
    expect("not paused, no wish: keep", dm_pause_next(&p, 0), DM_PAUSE_KEEP);
    expect("wish in a game: apply", dm_pause_next(&p, dm_pause_holds(1, 0, 0)), DM_PAUSE_APPLY);
    expect("  held deadline", dm_pause_applied(&p, 8000, 50000), (long)0xF0000000u);
    expect("  paused", p.paused, 1);
    expect("still wished: keep", dm_pause_next(&p, dm_pause_holds(1, 0, 0)), DM_PAUSE_KEEP);

    expect("loading screen comes up (leave-save level change): release", dm_pause_next(&p, dm_pause_holds(1, 0, 1)), DM_PAUSE_RELEASE);
    expect("  deadline moved back by 3 ms: 8003", dm_pause_released(&p, 50003), 8003);
    expect("  not paused", p.paused, 0);
    expect("loading, wish still set: keep running", dm_pause_next(&p, dm_pause_holds(1, 0, 1)), DM_PAUSE_KEEP);

    expect("loading screen gone, wish still set: apply again", dm_pause_next(&p, dm_pause_holds(1, 0, 0)), DM_PAUSE_APPLY);
    expect("  held deadline", dm_pause_applied(&p, 8603, 50603), (long)0xF0000000u);
    expect("  new base deadline kept", p.dl0, 8603);
    expect("menu closed: release", dm_pause_next(&p, dm_pause_holds(0, 0, 0)), DM_PAUSE_RELEASE);
    expect("  deadline 8603 + 1 ms", dm_pause_released(&p, 50604), 8604);
    expect("leave save starts while paused: release", (dm_pause_applied(&p, 9000, 60000), dm_pause_next(&p, dm_pause_holds(4, 1, 0))), DM_PAUSE_RELEASE);
    expect("  deadline 9000 + 2 ms", dm_pause_released(&p, 60002), 9002);
    expect("leave save ends during a loading screen: keep running", dm_pause_next(&p, dm_pause_holds(4, 0, 1)), DM_PAUSE_KEEP);
    expect("then the loading screen goes: apply", dm_pause_next(&p, dm_pause_holds(4, 0, 0)), DM_PAUSE_APPLY);

    dm_pause_applied(&p, 100, 0xFFFFFFF0u);
    expect("wrap: 0xFFFFFFF0 -> 0x10 = 32 ms, deadline 132", dm_pause_released(&p, 0x10u), 132);



    dm_loadwatch w = { .first_ns = 30 * S, .again_ns = 60 * S, .max = 3 };
    expect("no loading screen: nothing", dm_loadwatch_step(&w, 0, 5 * S, 10), DM_LOADWATCH_NONE);
    expect("comes up at 100 s: nothing yet", dm_loadwatch_step(&w, 1, 100 * S, 500), DM_LOADWATCH_NONE);
    expect("  start time kept", (long)(w.since_ns / S), 100);
    expect("  start tick kept", w.tick0, 500);
    expect("at 129 s (29 s up): nothing", dm_loadwatch_step(&w, 1, 129 * S, 520), DM_LOADWATCH_NONE);
    expect("at 130 s (30 s up): report", dm_loadwatch_step(&w, 1, 130 * S, 530), DM_LOADWATCH_REPORT);
    expect("at 131 s: no second report", dm_loadwatch_step(&w, 1, 131 * S, 531), DM_LOADWATCH_NONE);
    expect("at 189 s: nothing (59 s after the report)", dm_loadwatch_step(&w, 1, 189 * S, 540), DM_LOADWATCH_NONE);
    expect("at 190 s: second report", dm_loadwatch_step(&w, 1, 190 * S, 550), DM_LOADWATCH_REPORT);
    expect("frames stalled until 400 s: third report", dm_loadwatch_step(&w, 1, 400 * S, 560), DM_LOADWATCH_REPORT);
    expect("at 900 s: max reached, nothing", dm_loadwatch_step(&w, 1, 900 * S, 570), DM_LOADWATCH_NONE);
    expect("  start time still the first frame", (long)(w.since_ns / S), 100);
    expect("goes away: gone", dm_loadwatch_step(&w, 0, 901 * S, 571), DM_LOADWATCH_GONE);
    expect("next frame without it: nothing", dm_loadwatch_step(&w, 0, 902 * S, 572), DM_LOADWATCH_NONE);
    expect("new loading screen at 1000 s: nothing", dm_loadwatch_step(&w, 1, 1000 * S, 600), DM_LOADWATCH_NONE);
    expect("  at 1020 s: nothing (20 s up)", dm_loadwatch_step(&w, 1, 1020 * S, 610), DM_LOADWATCH_NONE);
    expect("short loading screen goes away: no gone line", dm_loadwatch_step(&w, 0, 1021 * S, 611), DM_LOADWATCH_NONE);
    expect("  and the clock is cleared", (long)w.since_ns, 0);
    dm_loadwatch_step(&w, 1, 1100 * S, 700); dm_loadwatch_step(&w, 1, 1130 * S, 710);
    expect("report then gone", dm_loadwatch_step(&w, 0, 1131 * S, 711), DM_LOADWATCH_GONE);
    expect("up again on the next frame: a new one, nothing", dm_loadwatch_step(&w, 1, 1132 * S, 712), DM_LOADWATCH_NONE);
    expect("  its start is 1132 s", (long)(w.since_ns / S), 1132);
    expect("  29 s later: nothing", dm_loadwatch_step(&w, 1, 1161 * S, 720), DM_LOADWATCH_NONE);
    dm_loadwatch z = { .first_ns = 30 * S, .again_ns = 60 * S, .max = 5 };
    expect("up at time 0: nothing", dm_loadwatch_step(&z, 1, 0, 0), DM_LOADWATCH_NONE);
    expect("  at 30 s: report", dm_loadwatch_step(&z, 1, 30 * S, 1), DM_LOADWATCH_REPORT);
    printf("%s: %d cases, %d failed\n", fails ? "FAIL" : "ok", cases, fails);
    return fails != 0;
}
