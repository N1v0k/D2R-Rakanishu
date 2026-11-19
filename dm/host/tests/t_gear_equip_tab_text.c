



#include "../dm_gear_equip_tab.c"

static int checks, fails; static const char *g_case;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s line %d: %s\n", g_case, __LINE__, #c); } } while (0)


static int st_open = 1, st_ready = 1, st_cap = 0, st_start = 0, st_state = DM_GE_RUNNING, st_done = 0, st_why = 0, st_plan[DM_GO_SLOTS];
static uint64_t st_hash = 0x77;
void dm_gear_tab_extra(dm_gear_tab_extra_fn fn) { (void)fn; }
int dm_gear_tab_wanted(void) { return 1; }
void dm_gear_tab_log(const char *line) { (void)line; }
int dm_gear_equip_open(uint8_t *b) { (void)b; return st_open; }
int dm_gear_equip_ready(uint8_t *b, uint64_t pl) { (void)b; (void)pl; return st_ready; }
void dm_gear_equip_fresh(void) {}
void dm_gear_equip_builds(int n) { (void)n; }
const char *dm_gear_equip_fail(void) { return "test"; }
int dm_gear_equip_capture_want(uint8_t *b, uint64_t pl, const dm_gew_target *t, dm_gear_equip_cap *cap) {
    (void)b; (void)pl; (void)t; memset(cap, 0, sizeof *cap); cap->hash = st_hash; return st_cap;
}

int dm_gear_equip_plan_seq(const dm_gear_snap *s, dm_gear_equip_cap *cap, const int8_t *slots, int nslots, dm_go_plan *plan) {
    (void)s; (void)cap; memset(plan, 0, sizeof *plan);
    for (int k = 0; k < nslots && k < DM_GO_MAXCHANGE; k++) {
        plan->change[k].slots = (uint16_t)(1u << slots[k] | (slots[k] == DM_GO_RIGHT ? 1u << DM_GO_LEFT : 0)); plan->nchange++; plan->nstep += 2;
    }
    return nslots ? st_plan[slots[0]] : DM_GO_E_INPUT;
}
int dm_ge_check(const dm_go_world *w, const dm_go_plan *p, const dm_ge_place *gp) { (void)w; (void)p; (void)gp; return 0; }
int dm_gear_equip_start(uint8_t *b, uint64_t pl, const dm_gear_equip_cap *cap, const dm_go_plan *plan) { (void)b; (void)pl; (void)cap; (void)plan; return st_start; }
int dm_gear_equip_state(int *why, int *done, int *changes) {
    if (why) *why = st_why;
    if (done) *done = st_done;
    if (changes) *changes = 0;
    return st_state;
}
const char *dm_ge_why_name(int why) { return why == DM_GE_WHY_FAULT ? "fault" : "none"; }


static dm_gear_snap S; static dm_gtab_show SHOW[16]; static dm_gtab_row R[16]; static int NR;
static char out[4096];
static void snap(const char *name) {
    g_case = name; memset(&S, 0, sizeof S); memset(SHOW, 0, sizeof SHOW); memset(R, 0, sizeof R); NR = 0;
    S.show = SHOW; S.p.ncand = 16;
    for (int k = 0; k < DM_GEAR_P_N; k++) S.now.at[k] = -1;
    for (int k = 0; k < DM_GO_SLOTS; k++) st_plan[k] = DM_GO_OK;
    st_open = st_ready = 1; st_cap = 0; st_hash++;
}
static void cand(int c, int where, uint32_t id, uint64_t vh) { SHOW[c].where = where; SHOW[c].id = id; SHOW[c].vhash = vh; }
static int row(int place, int c, int charm, int unit) { int i = NR++; R[i].place = place; R[i].cand = c; R[i].charm = charm; R[i].unit = unit; return i; }
static uint32_t stock(int place, int x, int y) { return DM_GTAB_STOCK_ID | (uint32_t)place << 16 | (uint32_t)y << 8 | (uint32_t)x; }

static const char *text_of(uint64_t key, int rows) {
    int n = text(rows ? &S : NULL, rows ? key : 0, rows ? R : NULL, rows ? NR : 0, out, sizeof out);
    out[n < 0 ? 0 : n] = 0;
    return out;
}

static const char *settle(uint64_t key) {
    text_of(key, 1);
    for (int f = 0; f <= CAPTURE_FRAMES; f++) dm_gear_equip_tab_pump(NULL, 0);
    return text_of(key, 1);
}
static void key_text(uint64_t key, char *k) { snprintf(k, 17, "%016llx", (unsigned long long)key); }
static int has(const char *what) { return strstr(out, what) != NULL; }

static void t_all(void) {
    snap("an inventory amulet and a stash ring: both go, equip all");
    cand(0, DM_GTAB_INV, 77, 0); cand(1, DM_GTAB_STASH, stock(1, 3, 4), 0xabc);
    row(DM_GEAR_P_AMULET, 0, 0, 0); row(DM_GEAR_P_RING1, 1, 0, 1);
    text_of(0x100, 1);
    CHECK(has("\"eq\":[2,2]") && has("\"eqall\":0"));
    settle(0x100);
    CHECK(has("\"eqkey\":\"0000000000000100\"") && has("\"stash\":1") && has("\"eq\":[1,1]") && has("\"eqall\":1"));
    CHECK(!has("\"job\""));
}
static void t_room_and_unknown(void) {
    snap("no room for the amulet's displaced item, no candidate for the ring");
    cand(0, DM_GTAB_INV, 78, 0); cand(1, DM_GTAB_STASH, stock(1, 0, 0), 0xabd); cand(2, DM_GTAB_INV, 79, 0);
    row(DM_GEAR_P_AMULET, 0, 0, 0); row(DM_GEAR_P_RING1, 1, 0, 1); row(DM_GEAR_P_BOOTS, 2, 0, 2);
    st_plan[DM_GO_NECK] = DM_GO_E_ROOM; st_plan[DM_GO_RRING] = -10;
    settle(0x200);
    CHECK(has("\"eq\":[0,2,1]") && has("\"eqall\":0"));
}
static void t_charm_and_one(void) {
    snap("a charm row has no button and does not stop equip all; one button alone is no equip all");
    cand(0, DM_GTAB_INV, 80, 0); cand(1, DM_GTAB_STASH, stock(1, 1, 1), 0xabe); cand(2, DM_GTAB_INV, 81, 0);
    row(-1, 1, 1, 5); row(DM_GEAR_P_AMULET, 0, 0, 0); row(DM_GEAR_P_BOOTS, 2, 0, 2);
    settle(0x300);
    CHECK(has("\"eq\":[-1,1,1]") && has("\"eqall\":1"));
    snap("one row");
    cand(0, DM_GTAB_INV, 82, 0); row(DM_GEAR_P_AMULET, 0, 0, 0);
    settle(0x310);
    CHECK(has("\"eq\":[1]") && has("\"eqall\":0"));
    snap("the capture fails: not known");
    cand(0, DM_GTAB_INV, 83, 0); cand(1, DM_GTAB_INV, 84, 0); row(DM_GEAR_P_AMULET, 0, 0, 0); row(DM_GEAR_P_BOOTS, 1, 0, 1);
    st_cap = -1;
    settle(0x320);
    CHECK(has("\"eq\":[2,2]") && has("\"eqall\":0"));
}
static void t_taps(void) {
    snap("taps: another result, away from the stash, a job that waits or runs; the job's members with and without a result");
    cand(0, DM_GTAB_INV, 90, 0); cand(1, DM_GTAB_INV, 91, 0);
    int amu = row(DM_GEAR_P_AMULET, 0, 0, 0); row(DM_GEAR_P_HEAD, -1, 0, 1);
    char k[17], other[17]; key_text(0x400, k); key_text(0x401, other);
    st_open = 0; settle(0x400);
    CHECK(has("\"stash\":0") && dm_gear_equip_tab_tap(k, amu) == -2);
    st_open = 1; settle(0x400);
    CHECK(has("\"stash\":1") && has("\"eq\":[1,1]") && has("\"eqall\":1"));
    CHECK(dm_gear_equip_tab_tap(other, amu) == -1);
    CHECK(dm_gear_equip_tab_tap(k, 7) == -1);
    int seq = dm_gear_equip_tab_tap(k, -1);
    CHECK(seq > 0 && dm_gear_equip_tab_tap(k, amu) == -3);
    text_of(0x400, 1);
    char want[96]; snprintf(want, sizeof want, "\"job\":{\"seq\":%d,\"st\":\"queued\",\"k\":0,\"n\":2,\"off\":1,", seq);
    CHECK(has(want));
    st_start = 0; st_state = DM_GE_RUNNING; st_done = 1;
    dm_gear_equip_tab_pump(NULL, 0);
    CHECK(dm_gear_equip_tab_tap(k, amu) == -3);
    text_of(0, 0);
    snprintf(want, sizeof want, "\"job\":{\"seq\":%d,\"st\":\"run\",\"k\":1,\"n\":2,\"off\":1,", seq);
    CHECK(has(want) && !has("\"eq\""));
    st_state = DM_GE_PARTIAL; st_why = DM_GE_WHY_FAULT;
    text_of(0, 0);
    snprintf(want, sizeof want, "\"job\":{\"seq\":%d,\"st\":\"partial\",\"k\":1,\"n\":2,\"off\":1,\"why\":\"fault\"}", seq);
    CHECK(has(want));
    st_state = DM_GE_DONE; st_why = 0;
    text_of(0, 0);
    snprintf(want, sizeof want, "\"job\":{\"seq\":%d,\"st\":\"done\",\"k\":2,\"n\":2,\"off\":1,", seq);
    CHECK(has(want) && !has("\"rows\"") && !has("\"eqall\""));

    settle(0x400);
    int again = dm_gear_equip_tab_tap(k, amu);
    CHECK(again > seq);
    st_start = 1; dm_gear_equip_tab_pump(NULL, 0);
    text_of(0, 0);
    snprintf(want, sizeof want, "\"job\":{\"seq\":%d,\"st\":\"changed\",\"k\":0,\"n\":1,\"off\":0,", again);
    CHECK(has(want));
    st_start = 0;
}
static void t_no_job_without_result(void) {
    snap("no result and no tap since the start of the process: nothing but what was there");

    CHECK(text(NULL, 0, NULL, 0, out, 8) == -1);
}

int main(void) {
    t_all(); t_room_and_unknown(); t_charm_and_one(); t_taps(); t_no_job_without_result();
    printf("t_gear_equip_tab_text: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
