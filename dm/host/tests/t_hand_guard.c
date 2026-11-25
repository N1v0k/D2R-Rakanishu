















#include "../dm_cursor.c"
module_t g_mod[MAX_MOD]; int g_nmod;
static uint64_t t_ns = 5000000000ull; static int t_hold, t_busy, t_rw, t_eq, t_pad, t_valid = 1, t_stow_calls, t_stow_res = 0, t_stow_why = 4, t_stow_lands;
uint64_t now_ns(void) { return t_ns; }
uint64_t dm_guest_frame(void) { return t_ns / 16000000ull; }
void dm_hud_get(dm_hud_info *out) { memset(out, 0, sizeof *out); out->valid = t_valid; out->area = 40; out->holding = t_hold; }
int dm_itemjob_owner(void) { return t_busy ? DM_ITEMJOB_RECIPE : t_rw ? DM_ITEMJOB_RUNEWORD : t_eq ? DM_ITEMJOB_EQUIP : DM_ITEMJOB_NONE; }
int dm_padmode_current(void) { return t_pad; }
int dm_padmode_busy(void) { return 0; }
int dm_held_pending(void) { return 0; }





static int t_sim, t_sim_frames = 90, t_sim_left, t_sim_tick, t_sim_stall, t_frame_ms = 16;
void dm_held_stow(void) { t_stow_calls++; if (t_stow_lands) t_hold = 0; if (t_sim) t_sim_left = t_sim_frames; }
int dm_held_stow_result(void) { return t_sim && t_sim_left > 0 ? -1 : t_stow_res; }
int dm_held_stow_why(void) { return t_sim && t_sim_left > 0 ? 0 : t_stow_why; }
int dm_held_stow_tick(void) { return t_sim_tick; }
static void sim_frame(void) { if (t_sim && t_sim_left > 0 && !t_sim_stall) { t_sim_tick++; if (!--t_sim_left && t_stow_res == 1) t_hold = 0; } }
int dm_held_size(int *w, int *h) { *w = *h = 1; return 1; }
void dm_held_equip(int bodyloc) { (void)bodyloc; }
int dm_ui_hover_now(int x, int y) { (void)x; (void)y; return 0; }
int dm_cursor_item(dm_cursor_item_info *out) { (void)out; return 0; }
int dm_uiscreen_ingame(void) { return 1; }
void dm_ui_rect(int *x,int *y,int *w,int *h) { *x=0; *y=0; *w=2480; *h=1116; }
void dm_uiscreen_set_ingame(int on) { (void)on; }
void dm_custom_on_uimsg(const char *cat,const char *act,const char *arg) { (void)cat; (void)act; (void)arg; }
void dm_elysium_arm(int on) { (void)on; }
static int checks, fails; static const char *g_case;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s line %d: %s\n", g_case, __LINE__, #c); } } while (0)
static uint64_t t_buttons;
static void apply(void) { uint64_t b = t_buttons; int32_t lx = 0, ly = 0; cursor_apply(&b, &lx, &ly); }
static void step_ms(int ms) { t_ns += (uint64_t)ms * 1000000ull; hand_guard(NULL); }
static void run_ms(int ms) { for (int i = 0; i < ms / t_frame_ms; i++) { t_ns += (uint64_t)t_frame_ms * 1000000ull; sim_frame(); apply(); } }
static int back_set(void) { return atomic_load(&g_set_back_at) != 0; }
static void fresh(const char *name) {
    g_case = name; t_hold = 0; t_busy = t_rw = t_eq = t_pad = 0; t_valid = 1; t_stow_calls = 0; t_stow_res = 0; t_stow_why = 4; t_stow_lands = 0; t_buttons = 0;
    t_sim = t_sim_left = t_sim_stall = 0; t_sim_frames = 90; t_frame_ms = 16;
    run_ms(64); t_ns += 20000000000ull; run_ms(64);
    g_rt = (drop_retry){0}; g_naq = 0; atomic_store(&g_naq_pub, 0); atomic_store(&g_set_back_at, 0); atomic_store(&g_pick_xy, 0); dm_hand_hint_done();
}

static void job_leaves(int id, int *busy) { *busy = 1; t_hold = id; run_ms(2000); *busy = 0; }

static void t_handoff_40(void) {
    g_case = "hand-over"; dm_ui_hand_idle(1);
    t_busy = 1; t_hold = 12;
    for (int i = 0; i < 20; i++) step_ms(100);
    CHECK(!g_rt.active);
    t_busy = 0; step_ms(16); CHECK(!g_rt.active);
    step_ms(400); CHECK(!g_rt.active);
    step_ms(300); CHECK(g_rt.active && g_rt.id == 12 && g_rt.stage == 1);
    g_rt = (drop_retry){ .active = 1, .pending = 1, .id = 30, .stage = 2, .t = t_ns }; g_naq = 2; atomic_store(&g_naq_pub, 2);
    recipe_handoff(); CHECK(g_rt.active && g_naq == 2 && dm_cursor_pending() == 2);
    t_busy = 1; recipe_handoff(); CHECK(!g_rt.active && !g_rt.pending && g_naq == 0 && dm_cursor_pending() == 0);
    t_busy = 0; g_rt.active = 0;
}
static void t_recipe_full(void) {
    fresh("recipe HELD, full inventory, touch");
    job_leaves(12, &t_busy); CHECK(atomic_load(&g_auto_sess) && g_auto_id == 12);
    run_ms(1500);
    CHECK(t_stow_calls == 1 && !back_set() && !g_rt.active && t_hold == 12);
    CHECK(dm_hand_hint_pending());
    dm_hand_hint_done(); run_ms(8000);
    CHECK(t_stow_calls == 1 && !back_set() && !g_rt.active && !dm_hand_hint_pending() && t_hold == 12);
    t_hold = 0; run_ms(32); CHECK(!atomic_load(&g_auto_sess));
}
static void t_player_full(void) {
    fresh("player item, full inventory, touch (upstream)");
    t_hold = 30; run_ms(1500);
    CHECK(!atomic_load(&g_auto_sess) && t_stow_calls == 1 && back_set() && g_rt.stage >= 3 && !dm_hand_hint_pending());
}
static void t_room(void) {
    fresh("recipe HELD, room appeared: stowed");
    job_leaves(12, &t_busy); t_stow_lands = 1; t_stow_res = 1; t_stow_why = 0; run_ms(1500);
    CHECK(t_stow_calls == 1 && t_hold == 0 && !back_set() && !atomic_load(&g_auto_sess) && !dm_hand_hint_pending());
}
static void t_sent_wait(void) {
    fresh("stow sent, server late");
    job_leaves(12, &t_busy); t_stow_res = 1; t_stow_why = 0; run_ms(3000);
    CHECK(t_stow_calls == 1 && g_rt.active && g_rt.stage == 2 && !back_set() && !dm_hand_hint_pending());
    run_ms(4000); CHECK(!g_rt.active && !back_set() && !dm_hand_hint_pending() && t_hold == 12);
}
static void t_other_why(void) {
    fresh("stow failed, pending operation");
    job_leaves(12, &t_busy); t_stow_why = 2; run_ms(1500);
    CHECK(t_stow_calls == 1 && !back_set() && !g_rt.active && !dm_hand_hint_pending() && t_hold == 12);
}
static void t_swap(void) {
    fresh("session survives a swap");
    job_leaves(12, &t_busy); run_ms(1500); dm_hand_hint_done(); CHECK(t_stow_calls == 1);
    atomic_store(&g_pick_xy, (1ull << 63) | 500 | (300ull << 32));
    t_buttons = DM_BTN_A; run_ms(16); t_buttons = 0;
    g_rt = (drop_retry){ .active = 1, .id = 12, .x = 900, .y = 400, .rows = 1, .t = t_ns }; t_hold = 31;
    run_ms(1500);
    CHECK(atomic_load(&g_auto_sess) && g_naq == 0 && t_stow_calls == 2 && !back_set() && !g_rt.active && t_hold == 31 && dm_hand_hint_pending());
}
static void t_transmute(void) {
    fresh("transmute fallback, automation item");
    job_leaves(12, &t_busy); run_ms(1500); dm_hand_hint_done();
    g_rt = (drop_retry){ .active = 1, .id = 12, .rows = 1, .stage = 2, .t = t_ns - 400000000ull }; run_ms(500);
    CHECK(!g_rt.active && !back_set() && dm_hand_hint_pending());
    fresh("chain already at stage 3, automation item");
    job_leaves(12, &t_busy); run_ms(1500); dm_hand_hint_done();
    g_rt = (drop_retry){ .active = 1, .id = 12, .rows = 1, .stage = 3, .t = t_ns - 400000000ull }; run_ms(500);
    CHECK(!g_rt.active && !back_set());
}
static void t_pad_mode(void) {
    fresh("controller, automation item");
    t_pad = 1; job_leaves(12, &t_busy); run_ms(3000);
    CHECK(t_stow_calls == 1 && !back_set() && !g_rt.active && dm_hand_hint_pending() && t_hold == 12);
    t_hold = 33; run_ms(3000);
    CHECK(t_stow_calls == 1 && !g_rt.active && !back_set());
    fresh("controller, player item (upstream)");
    t_pad = 1; t_hold = 30; run_ms(3000);
    CHECK(t_stow_calls == 0 && !g_rt.active && !back_set());
    fresh("touch -> controller switch, automation item");
    job_leaves(12, &t_busy); run_ms(1500); CHECK(t_stow_calls == 1); t_pad = 1; run_ms(3000);
    CHECK(t_stow_calls == 1 && !back_set());
}
static void t_late(void) {
    fresh("late answer after the job");
    t_rw = 1; run_ms(1000); t_rw = 0; run_ms(3000); t_hold = 40; run_ms(1500);
    CHECK(atomic_load(&g_auto_sess) && g_auto_id == 40 && t_stow_calls == 1 && !back_set() && t_hold == 40);
    fresh("player pick-up right after a job (A pressed)");
    t_rw = 1; run_ms(1000); t_rw = 0; run_ms(3000); t_buttons = DM_BTN_A; run_ms(16); t_buttons = 0; run_ms(300); t_hold = 41; run_ms(1500);
    CHECK(!atomic_load(&g_auto_sess) && t_stow_calls == 1 && back_set());
    fresh("item in hand 12 s after a job, no A");
    t_rw = 1; run_ms(1000); t_rw = 0; run_ms(12000); t_hold = 42; run_ms(1500);
    CHECK(!atomic_load(&g_auto_sess) && back_set());
}
static void t_runeword(void) {
    fresh("runeword job owns the cursor");
    t_rw = 1; t_hold = 50; run_ms(2000);
    CHECK(!g_rt.active && t_stow_calls == 0 && atomic_load(&g_auto_sess));
    g_rt = (drop_retry){ .active = 1, .pending = 1, .id = 50, .stage = 2, .t = t_ns }; g_naq = 1; atomic_store(&g_naq_pub, 1);
    recipe_handoff(); CHECK(!g_rt.active && !g_rt.pending && g_naq == 0);
    t_rw = 0; run_ms(1500); CHECK(t_stow_calls == 1 && !back_set() && t_hold == 50);
}
static void t_equip_owner(void) {
    fresh("equip job owns the cursor");
    t_eq = 1; t_hold = 60; run_ms(2000);
    CHECK(!g_rt.active && t_stow_calls == 0 && atomic_load(&g_auto_sess) && g_auto_id == 60);
    g_rt = (drop_retry){ .active = 1, .pending = 1, .id = 60, .stage = 2, .t = t_ns }; g_naq = 1; atomic_store(&g_naq_pub, 1);
    recipe_handoff(); CHECK(!g_rt.active && !g_rt.pending && g_naq == 0 && dm_cursor_pending() == 0);
    t_eq = 0; run_ms(1500); CHECK(t_stow_calls == 1 && !back_set() && t_hold == 60);
}
static void t_invalid(void) {
    fresh("invalid HUD sample");
    job_leaves(12, &t_busy); t_valid = 0; run_ms(64); t_valid = 1; CHECK(atomic_load(&g_auto_sess));
    run_ms(1500); CHECK(!back_set() && t_stow_calls == 1);
    t_hold = 0; run_ms(32); CHECK(!atomic_load(&g_auto_sess) && !dm_hand_hint_pending());
    t_buttons = DM_BTN_A; run_ms(16); t_buttons = 0; run_ms(300); t_hold = 30; run_ms(1500); CHECK(!atomic_load(&g_auto_sess) && back_set());
}



static uint64_t until_stow(int max_ms) { for (int i = 0; i < max_ms / t_frame_ms && !t_stow_calls; i++) run_ms(t_frame_ms); return t_ns; }
static void t_fps(void) {
    static const int fps[] = { 120, 60, 45, 40, 30, 20, 9, 6, 4 };
    for (unsigned k = 0; k < sizeof fps / sizeof *fps; k++) {
        static char nm[64]; snprintf(nm, sizeof nm, "recipe HELD, full inventory, %d fps", fps[k]); fresh(nm);
        t_sim = 1; t_frame_ms = 1000 / fps[k];
        job_leaves(12, &t_busy);
        uint64_t t0 = until_stow(3000); CHECK(t_stow_calls == 1);
        uint64_t need = (uint64_t)t_sim_frames * t_frame_ms * 1000000ull;
        if (need > 2200000000ull) { run_ms(2200); CHECK(g_rt.active && g_rt.stage == 2 && !dm_hand_hint_pending() && !back_set()); }
        while (!dm_hand_hint_pending() && t_ns - t0 < 40000000000ull) run_ms(t_frame_ms);
        double s = (t_ns - t0) / 1e9;
        CHECK(dm_hand_hint_pending());
        CHECK(t_ns - t0 >= need && t_ns - t0 <= need + 1000000000ull);
        CHECK(t_stow_calls == 1 && !back_set() && !g_rt.active && t_hold == 12 && atomic_load(&g_auto_sess));
        printf("  %3d fps: stow attempt %.2f s, notice after %.2f s%s\n", fps[k], need / 1e9, s, dm_hand_hint_pending() ? "" : " -- NO NOTICE");
        dm_hand_hint_done(); run_ms(4000); CHECK(t_stow_calls == 1 && !back_set() && t_hold == 12);
    }
}
static void t_fps_player(void) {
    fresh("player item, full inventory, 30 fps"); t_sim = 1; t_frame_ms = 33;
    t_hold = 30; uint64_t t0 = until_stow(3000); CHECK(t_stow_calls == 1);
    run_ms(2500); CHECK(!back_set() && g_rt.active && g_rt.stage == 2);
    run_ms(1000); CHECK(back_set() && !atomic_load(&g_auto_sess) && !dm_hand_hint_pending());
    (void)t0;
}
static void t_fps_room(void) {
    fresh("recipe HELD, room after 5 frames, 6 fps"); t_sim = 1; t_frame_ms = 166; t_sim_frames = 5; t_stow_res = 1; t_stow_why = 0;
    job_leaves(12, &t_busy); until_stow(3000); run_ms(3000);
    CHECK(t_stow_calls == 1 && t_hold == 0 && !back_set() && !atomic_load(&g_auto_sess) && !dm_hand_hint_pending());
}
static void t_stall(void) {
    fresh("stow makes no progress"); t_sim = 1; t_sim_stall = 1;
    job_leaves(12, &t_busy); until_stow(3000);
    run_ms(2500); CHECK(g_rt.active && g_rt.stage == 2);
    run_ms(1500); CHECK(!g_rt.active && !back_set() && !dm_hand_hint_pending() && t_hold == 12 && atomic_load(&g_auto_sess));
    fresh("player item, stow makes no progress"); t_sim = 1; t_sim_stall = 1;
    t_hold = 30; until_stow(3000); run_ms(2500); CHECK(!back_set()); run_ms(1500); CHECK(back_set());
}
static void t_cap(void) {
    fresh("stow never reports"); t_sim = 1; t_sim_frames = 1000000;
    job_leaves(12, &t_busy); uint64_t t0 = until_stow(3000);
    run_ms(29000); CHECK(g_rt.active && g_rt.stage == 2);
    run_ms(2000); CHECK(!g_rt.active && !back_set() && !dm_hand_hint_pending() && t_hold == 12);
    CHECK(t_ns - t0 < 32000000000ull);
}
int main(void) {
    dm_ui_hand_idle(1);
    t_handoff_40();
    t_recipe_full();
    t_player_full();
    t_room();
    t_sent_wait();
    t_other_why();
    t_swap();
    t_transmute();
    t_pad_mode();
    t_late();
    t_runeword();
    t_equip_owner();
    t_invalid();
    t_fps();
    t_fps_player();
    t_fps_room();
    t_stall();
    t_cap();
    printf("t_hand_guard: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
