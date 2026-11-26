




#include "../dm_dmgnum.c"
#include "../game_addrs.h"
#include <stdio.h>

module_t g_mod[MAX_MOD];
int g_nmod;
static int checks, fails;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)

static uint64_t clock_ns = 1000000000ull;
uint64_t now_ns(void) { return clock_ns; }
uint64_t guest_ns(void) { return clock_ns; }
int gate_svc_present(int num) { return num == SVC_HOST_DAMAGE; }
static void (*test_observer[OBS_MAX])(const uint64_t *);
int gate_obs_present(int id) { return id>0 && id<OBS_MAX; }
void gate_obs_set(int id,void (*fn)(const uint64_t *)) { test_observer[id]=fn; }
uint64_t dm_guest_frame(void) { return 0; }
static int mode = MODE_COLOUR;
static int splash_running;
int dm_splash_running(void) { return splash_running; }
int dm_custom_get(const char *key) { return strcmp(key, "dmg_num") ? 1 : mode; }
void dm_fightlog_hit(const uint64_t *regs) { (void)regs; }
void dm_fightlog_obs_chain(int id, void (*fn)(const uint64_t *regs)) { test_observer[id] = fn; }
int dm_world_unit_head(uint32_t id, float *xy) { xy[0] = 0.5f; xy[1] = 0.4f; return id != 99; }
int dm_resume_shields(uint64_t unit) { (void)unit; return 0; }


typedef struct { uint8_t b[0x200]; int life; } unit;
static void put32(void *p, size_t at, uint32_t v) { memcpy((uint8_t *)p + at, &v, 4); }
static void put64(void *p, size_t at, uint64_t v) { memcpy((uint8_t *)p + at, &v, 8); }
static void make(unit *u, uint32_t type, uint32_t id, int owned_flag) { memset(u, 0, sizeof *u); put32(u->b, 0, type); put32(u->b, 8, id); put32(u->b, 0xc, 1); put32(u->b, 0x124, owned_flag ? 0x80000000u : 0); }
static int life_stub(uint64_t u, int stat, int z) { (void)stat; (void)z; return ((unit *)(uintptr_t)u)->life; }
static uint8_t game[0x2a40];
static uint8_t rec[0x200];
static void record(int32_t phys, int32_t fire, int32_t cold, int32_t ltng, int32_t pois, int32_t mag, int32_t total) {
    memset(rec, 0, sizeof rec); put32(rec, 4, 0x5); put32(rec, 0x18, (uint32_t)phys); put32(rec, 0x20, (uint32_t)fire); put32(rec, 0x34, (uint32_t)cold); put32(rec, 0x2c, (uint32_t)ltng);
    put32(rec, 0x38, (uint32_t)pois); put32(rec, 0x30, (uint32_t)mag); put32(rec, 0x134, (uint32_t)total); }
static void hit(unit *att, unit *def) { uint64_t r[32] = { 0 }; r[19] = (uint64_t)(uintptr_t)rec; r[20] = (uint64_t)(uintptr_t)def->b; r[21] = (uint64_t)(uintptr_t)att->b; r[22] = (uint64_t)(uintptr_t)game; read_hit(r); }
static unsigned queued(void) { return atomic_load(&g_head) - atomic_load(&g_tail); }
static float out[LIVE * FIELDS];
static int frame(void) { dm_dmgnum_pump(); return dm_dmgnum_copy(out, LIVE); }
static int32_t val(int g, int e) { return (int)out[g * FIELDS + 5] == e ? (int32_t)out[g * FIELDS + 6] : 0; }
static int kind_of(int g) { return (int)out[g * FIELDS + 5]; }
static int flags_of(int g) { return (int)out[g * FIELDS + 3] >> 5; }

int main(void) {
    strcpy(g_mod[0].name, "main"); g_mod[0].base = (uint8_t *)(uintptr_t)life_stub - GA_STAT_GET; g_nmod = 1;
    int k;


    { int32_t s[NE] = { 127, 0, 0, 0, 0, 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 1 && k == E_PHYSICAL, "lone share under half a point shows 1"); }
    { int32_t s[NE] = { 465, 414, 0, 307, 0, 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 5 && k == E_PHYSICAL, "dagger + charms: one 5, physical largest = white");
      CHECK(dm_dmgnum_split(s, MODE_SIMPLE, &k) == 5 && k == E_PHYSICAL, "simple: one white 5 (adds first)"); }

    { int32_t s[NE] = { 2 * 256, 5 * 256, 4 * 256, 0, 0, 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 11 && k == E_FIRE, "warlock hit: one fire-coloured 11");
      CHECK(dm_dmgnum_split(s, MODE_SIMPLE, &k) == 11 && k == E_PHYSICAL, "warlock hit, simple: white 11"); }
    { int32_t s[NE] = { 300, 300, 0, 0, 0, 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 2 && k == E_PHYSICAL, "physical / fire tie: physical (white)"); }
    { int32_t s[NE] = { 0, 0, 300, 300, 0, 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 2 && k == E_COLD, "cold / lightning tie: the earlier kind"); }
    { int32_t s[NE] = { 0, 0, 0, 0, 100, 60 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 1 && k == E_POISON, "largest share under half a point: total at least 1 in its kind"); }
    { int32_t s[NE] = { 128, 129, 0, 383, 384, 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 4 && k == E_POISON, "1024/256 = 4, poison largest"); }
    { int32_t s[NE] = { 0x7fffffff, 0x7fffffff, 0, 0, 0, 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 999999, "large sum capped without overflow"); }
    { int32_t s[NE] = { 300, 0, 0, 0, 0, 0 }; CHECK(dm_dmgnum_split(s, MODE_OFF, &k) == 0, "off shows nothing"); }
    { int32_t s[NE] = { 0 }; CHECK(dm_dmgnum_split(s, MODE_COLOUR, &k) == 0, "no damage, no number"); }



    unit player, merc, wild, zombie, ghost; make(&player, 0, 1, 0); make(&merc, 1, 2, 1); make(&wild, 1, 3, 0); make(&zombie, 1, 31, 0); make(&ghost, 1, 99, 0);
    record(465, 414, 0, 307, 0, 0, 1186);
    hit(&player, &zombie); CHECK(queued() == 0, "nothing is read before the front end asks");
    CHECK(frame() == 0 && atomic_load(&g_on), "the first copy switches capture on");
    hit(&player, &zombie); CHECK(queued() == 1 && g_ring[0].unit == 31 && g_ring[0].share[E_FIRE] == 414 && !g_ring[0].flags, "player hit queued with its shares");
    hit(&merc, &zombie); CHECK(queued() == 2 && g_ring[1].flags == F_MINION, "mercenary hit queued as minion");
    hit(&wild, &zombie); hit(&player, &merc); hit(&zombie, &player); CHECK(queued() == 2, "wild attacker, owned defender, player defender: not queued");
    record(0, 0, 0, 0, 0, 0, 0); hit(&player, &zombie); CHECK(queued() == 2, "total under 1: not queued");

    record(256, 256, 0, 0, 0, 0, 1024); hit(&player, &wild); CHECK(queued() == 3 && g_ring[2].share[E_PHYSICAL] == 1024 && !g_ring[2].share[E_FIRE], "mismatch becomes one white total");
    record(0, 0, 0, 0, 0, 0, 640); hit(&player, &wild); CHECK(queued() == 4 && g_ring[3].share[E_PHYSICAL] == 640, "all shares 0 with a positive total: one white total");
    record(100, 0, 0, 0, 0, 0, 100); hit(&player, &ghost); CHECK(queued() == 5, "hit on a monster off the picture is queued");


    int n = frame();
    CHECK(n == 4, "four groups (the off-picture hit is dropped)");
    CHECK(val(0, E_PHYSICAL) == 5 && kind_of(0) == E_PHYSICAL && !(flags_of(0) & F_CRIT), "colour: one 5, physical largest");
    CHECK((int)out[4] + 1 == (int)out[FIELDS + 4] && (int)out[FIELDS + 4] + 1 == (int)out[2 * FIELDS + 4], "arrival numbers published in order");
    CHECK((int)out[7] == 31 && (int)out[FIELDS + 7] == 31 && (int)out[2 * FIELDS + 7] == 3, "stable monster ids published");
    CHECK(((int)out[3] & 31) == ((int)out[FIELDS + 3] & 31) && ((int)out[3] & 31) != ((int)out[2 * FIELDS + 3] & 31), "same monster, same number; another monster, another");
    CHECK(flags_of(1) == F_MINION && flags_of(0) == 0, "minion flag published");
    CHECK(val(2, E_PHYSICAL) == 4 && flags_of(2) == F_WHITE, "mismatch shown as plain white 4");
    CHECK(val(3, E_PHYSICAL) == 3 && ((int)out[2 * FIELDS + 3] & 31) == ((int)out[3 * FIELDS + 3] & 31), "zero-share hit shown as white 3 on the same monster");
    CHECK(queued() == 0, "ring drained");

    mode = MODE_SIMPLE; record(465, 414, 0, 307, 0, 0, 1186); hit(&player, &wild); n = frame();
    CHECK(n == 5 && val(4, E_PHYSICAL) == 5 && flags_of(4) == F_WHITE, "simple group is one plain white 5");
    CHECK(flags_of(0) == 0, "colour groups are not flagged white");
    clock_ns += 1200000000ull; frame();

    mode = MODE_COLOUR; record(2 * 1948, 2322, 419, 0, 8, 0, 2 * 1948 + 2322 + 419 + 8); put32(rec, 4, 0x2005); hit(&player, &wild);
    record(1948, 2322, 419, 0, 8, 0, 1948 + 2322 + 419 + 8); hit(&player, &wild); n = frame();
    CHECK(n == 2 && (flags_of(0) & F_CRIT) && kind_of(0) == E_PHYSICAL && val(0, E_PHYSICAL) == 26, "flagged hit: a white critical 26");
    CHECK(!(flags_of(1) & F_CRIT) && kind_of(1) == E_FIRE && val(1, E_FIRE) == 18, "unflagged hit: no crit, fire largest 18");
    record(256, 0, 0, 0, 0, 0, 256); put32(rec, 4, 0x4021); hit(&player, &wild); n = frame();
    CHECK(n == 3 && !(flags_of(2) & F_CRIT), "other result bits (0x4021) are no crit");

    splash_running = 1; record(256, 0, 0, 0, 0, 0, 256); put32(rec, 4, 1); hit(&player, &wild);
    record(512, 0, 0, 0, 0, 0, 512); put32(rec, 4, 0x2001); hit(&player, &wild); splash_running = 0; n = frame();
    CHECK(n == 5 && flags_of(3) == F_SPLASH && flags_of(4) == (F_SPLASH | F_CRIT), "splash hits flagged, a critical one as both");
    CHECK(!(flags_of(0) & F_SPLASH), "a primary hit is no splash hit");

    clock_ns += 1200000000ull; CHECK(frame() == 0, "groups age out after 1.1 s");


    mode = MODE_COLOUR; for (int i = 0; i < 200; i++) hit(&player, &zombie);
    CHECK(queued() == RING, "a full ring keeps 128 and drops the rest");
    n = frame(); CHECK(n == LIVE, "at most 32 live groups");
    clock_ns += 1200000000ull; frame();

    for (int i = 1; i <= 40; i++) { record(256 * i, 0, 0, 0, 0, 0, 256 * i); hit(&player, &zombie); }
    n = frame(); { int lo = 999, hi = 0; for (int g = 0; g < n; g++) { int v0 = val(g, E_PHYSICAL); lo = v0 < lo ? v0 : lo; hi = v0 > hi ? v0 : hi; }
      CHECK(n == LIVE && lo == 9 && hi == 40, "burst keeps the latest 32 groups"); }
    clock_ns += 1200000000ull; frame();



    hit(&player, &zombie); n = frame(); CHECK(n == 1, "one group before switching off");
    hit(&player, &zombie);
    mode = MODE_OFF; CHECK(dm_dmgnum_copy(out, LIVE) == 0 && !atomic_load(&g_on), "off: nothing published, capture off");
    hit(&player, &zombie); CHECK(queued() == 1, "off: hits are not read");
    dm_dmgnum_pump();
    mode = MODE_COLOUR; CHECK(dm_dmgnum_copy(out, LIVE) == 0 && atomic_load(&g_on), "on again: capture on, the old list is not handed out");
    CHECK(frame() == 0 && queued() == 0, "on again: the hit queued before off and the old group are dropped");
    hit(&player, &zombie); CHECK(frame() == 1, "on again: new hits show");
    clock_ns += 1200000000ull; frame();




    static uint8_t list[0x200], node[0x100];
    put32(list, 0x1c, 0x80000000u); put64(list, 0xd0, (uint64_t)(uintptr_t)node); put32(node, 0x20, STATE_POISON); put32(node, 8, 0); put32(node, 0xc, 1);
    unit sick; make(&sick, 1, 40, 0); put64(sick.b, 0x88, (uint64_t)(uintptr_t)list); sick.life = 100 * 256;
    put64(game, 0x2230 + 0x400 + 8 * 5, (uint64_t)(uintptr_t)sick.b);
    put32(game, 0x170, 25); dm_dmgnum_tick((uint64_t)(uintptr_t)game); CHECK(queued() == 0, "first second: baseline only");
    sick.life -= 3 * 256 + 100; put32(game, 0x170, 50); dm_dmgnum_tick((uint64_t)(uintptr_t)game);
    CHECK(queued() == 1 && g_ring[atomic_load(&g_tail) % RING].flags == F_DOT && g_ring[atomic_load(&g_tail) % RING].share[E_POISON] == 3 * 256, "second: 3 poison points, flagged as damage over time");
    n = frame(); CHECK(n == 1 && val(0, E_POISON) == 3 && kind_of(0) == E_POISON && flags_of(0) == F_DOT, "poison number published as its own small number, never a crit");
    sick.life -= 256 - 100 + 256; record(256, 0, 0, 0, 0, 0, 256); hit(&player, &sick);
    sick.life -= 256; put32(game, 0x170, 75); dm_dmgnum_tick((uint64_t)(uintptr_t)game);
    { unsigned t = atomic_load(&g_tail); CHECK(queued() == 2 && g_ring[(t + 1) % RING].share[E_POISON] == 2 * 256, "carried fraction + hit subtracted: 2 poison points"); }
    put32(game, 0x170, 76); dm_dmgnum_tick((uint64_t)(uintptr_t)game); CHECK(queued() == 2, "only on whole seconds");

    { const uint32_t *ids = NULL; uint64_t G = (uint64_t)(uintptr_t)game;
      CHECK(dm_dmgnum_dot_pass(G, 75, &ids) == 1 && ids[0] == 40, "the pass of tick 75 names the poisoned monster");
      CHECK(dm_dmgnum_dot_pass(G, 76, &ids) < 0 && dm_dmgnum_dot_pass(G, 100, &ids) < 0, "no pass on another tick");
      CHECK(dm_dmgnum_dot_pass(G + 8, 75, &ids) < 0, "no pass for another game"); }
    clock_ns += 1200000000ull; frame();

    mode = MODE_OFF; dm_dmgnum_copy(out, LIVE); mode = MODE_COLOUR; dm_dmgnum_copy(out, LIVE);
    sick.life -= 50 * 256; put32(game, 0x170, 100); dm_dmgnum_tick((uint64_t)(uintptr_t)game); CHECK(queued() == 0, "after off / on the old baseline is not used");

    printf("%s t_dmgnum: %d checks, %d failed\n", fails ? "FAIL" : "PASS", checks, fails);
    return fails != 0;
}
