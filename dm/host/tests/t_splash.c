




#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../hle.h"

static int checks, fails;
#define CHECK(c, msg, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: " msg " (line %d)\n", ##__VA_ARGS__, __LINE__); } } while (0)

module_t g_mod[MAX_MOD];
int g_nmod;

static int g_share;
int dm_custom_splash_on(void) { return g_share > 0; }
static unsigned g_gate_calls;
unsigned dm_dmgnum_gate_calls(void) { return g_gate_calls; }
uint64_t now_ns(void) { return 0; }
uint64_t guest_ns(void) { return 0; }
uint64_t dm_guest_frame(void) { return 0; }


static void test_resist(const uint64_t *r);
static void test_result(const uint64_t *r);
static int test_gate(uint64_t game, uint64_t att, uint64_t u);
static void test_resist_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf);
static void test_damage_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf);
static void test_exec_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf);
static int test_life(uint64_t u);
#define DM_SPLASH_GAME_RESIST test_resist
#define DM_SPLASH_GAME_RESULT test_result
#define DM_SPLASH_GAME_GATE test_gate
#define DM_SPLASH_GAME_RESIST_ONE test_resist_one
#define DM_SPLASH_GAME_DAMAGE test_damage_one
#define DM_SPLASH_GAME_EXEC test_exec_one
#define DM_SPLASH_GAME_LIFE test_life
#define DM_SPLASH_GAME_XP test_life
#define DM_SPLASH_GAME_FACTS test_facts
#include "../dm_splash.c"


static hit_facts g_facts;
static int test_facts(uint64_t att, hit_facts *f) { (void)att; *f = g_facts; return 0; }
static void axe_facts(int cls, int str) {
    memset(&g_facts, 0, sizeof g_facts);
    g_facts.hand[0].weapon = 1; g_facts.hand[0].range = 2; strcpy(g_facts.hand[0].type, "axe"); g_facts.two_hands = 1;
    g_facts.cls = cls; g_facts.str = str;
}

static void set_share(int share) {
    g_share = share;
    if (share == 25) axe_facts(3, 75); else if (share == 50) axe_facts(4, 125); else if (share == 90) axe_facts(4, 225);
}


#define NMON 16
#define LEVEL_ID 7
static uint8_t game[0x2B00], game2[0x2B00];
static uint8_t mon[NMON][0x300];
static uint8_t path[NMON][0x40];
static uint8_t roomex[0x100], level[0x220];
static uint8_t rec[0x180];
static uint8_t att_u[0x300], att_p[0x40];

static void put32(void *p, size_t at, uint32_t v) { memcpy((uint8_t *)p + at, &v, 4); }
static void put16(void *p, size_t at, uint16_t v) { memcpy((uint8_t *)p + at, &v, 2); }
static void put64(void *p, size_t at, uint64_t v) { memcpy((uint8_t *)p + at, &v, 8); }
static uint32_t get32(void *p, size_t at) { uint32_t v; memcpy(&v, (uint8_t *)p + at, 4); return v; }
static uint64_t get64(void *p, size_t at) { uint64_t v; memcpy(&v, (uint8_t *)p + at, 8); return v; }

static int n_resist, n_damage, n_exec, reentered, kill_next_damage;
static uint64_t depth_probe_from; static uint32_t revalidate_id;
static uint8_t last_rec[0x180]; static int last_inline;
static uint64_t gate_want[8]; static int n_gate_want;
static uint32_t hit_ids[16]; static int n_hit_ids;



static void make_mon(int k, uint32_t id, int x, int y) {
    uint8_t *u = mon[k];
    put32(u, 0, 1); put32(u, 4, 5 + (uint32_t)k); put32(u, 8, id); put32(u, 0xc, 1);
    put64(u, 0x38, (uint64_t)(uintptr_t)path[k]);
    put16(path[k], 2, (uint16_t)x); put16(path[k], 6, (uint16_t)y);
    put64(path[k], 0x20, (uint64_t)(uintptr_t)roomex);
    uint64_t bucket = (uint64_t)(uintptr_t)game + 0x2230 + 0x400 + 8ull * (id & 0x7f);
    put64(u, 0x158, get64((void *)(uintptr_t)bucket, 0)); put64((void *)(uintptr_t)bucket, 0, (uint64_t)(uintptr_t)u);
}
static void world_reset(void) {
    memset(game, 0, sizeof game); memset(game2, 0, sizeof game2); memset(mon, 0, sizeof mon); memset(path, 0, sizeof path);
    memset(roomex, 0, sizeof roomex); memset(level, 0, sizeof level); memset(rec, 0, sizeof rec);
    memset(att_u, 0, sizeof att_u); memset(att_p, 0, sizeof att_p);
    put32(game, 0x170, 1000); put32(game2, 0x170, 1001); put32(level, 0x1f8, LEVEL_ID);
    put64(roomex, 0x18, (uint64_t)(uintptr_t)roomex); put64(roomex, 0x90, (uint64_t)(uintptr_t)level);
    put32(att_u, 0, 0); put32(att_u, 8, 1);
    put64(att_u, 0x38, (uint64_t)(uintptr_t)att_p); put16(att_p, 2, 100); put16(att_p, 6, 100);
    n_gate_want = 0; g_depth = 0; g_share = 0; memset(&g_facts, 0, sizeof g_facts);
    n_resist = n_damage = n_exec = 0; reentered = 0; kill_next_damage = 0; revalidate_id = 0; depth_probe_from = 0; memset(last_rec, 0, sizeof last_rec); last_inline = 0; n_hit_ids = 0;
    for (int i = 0; i < SPLASH_SLOTS; i++) g_snaps[i].used = 0;
    for (int i = 0; i < PAIRS; i++) pairs[i].used = 0;
}

static int n_call_resist;
static void test_resist(const uint64_t *r) { (void)r; n_call_resist++; }
static void test_result(const uint64_t *r) { (void)r; }
static int test_gate(uint64_t game, uint64_t att, uint64_t u) {
    (void)game; (void)att; uint32_t id = get32((void *)(uintptr_t)u, 8);
    for (int i = 0; i < n_gate_want; i++) if (gate_want[i] == id) return 1;
    return 0;
}
static void test_resist_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf) {
    (void)game; (void)att; n_resist++;
    memcpy(last_rec, (const void *)(uintptr_t)buf, sizeof last_rec); last_inline = get64((void *)(uintptr_t)buf, REC_VEC) == buf + REC_VEC_AT;
    if (n_hit_ids < 16) hit_ids[n_hit_ids++] = get32((void *)(uintptr_t)u, 8);
}
static void test_damage_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf) {
    n_damage++;
    if (kill_next_damage) { put32((void *)(uintptr_t)buf, REC_RESFL, get32((void *)(uintptr_t)buf, REC_RESFL) | 2); kill_next_damage = 0; }
    if (revalidate_id && get32((void *)(uintptr_t)u, 8) != revalidate_id)
        for (int k = 0; k < NMON; k++) if (mon[k][0] && get32(mon[k], 8) == revalidate_id) put32(mon[k], 0xc, 12);
    if (depth_probe_from && u == depth_probe_from) {

        uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att, (uint64_t)(uintptr_t)mon[0], (uint64_t)(uintptr_t)rec };
        int before = 0; for (int i = 0; i < SPLASH_SLOTS; i++) before += g_snaps[i].used;
        svc_melee_resist(r);
        int after = 0; for (int i = 0; i < SPLASH_SLOTS; i++) after += g_snaps[i].used;
        CHECK(after == before, "the depth guard let a nested snapshot through");
        int d0 = n_damage; svc_melee_result(r);
        CHECK(n_damage == d0, "the depth guard let a nested splash run (%d more damage calls)", n_damage - d0);
        reentered++;
    }
}
static void test_exec_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf) { (void)game; (void)att; (void)u; (void)buf; n_exec++; }
static int test_life(uint64_t u) { (void)u; return 1000; }


static void make_rec(uint32_t resfl, int crit, uint32_t poison_mag) {
    memset(rec, 0, sizeof rec);
    put32(rec, 0, 0x22); put32(rec, 4, resfl | (crit ? 0x2000 : 0));
    put32(rec, REC_PHYS, 1000); put32(rec, REC_FIRE, 400); put32(rec, REC_LITNG, 300);
    put32(rec, REC_MAGIC, 200); put32(rec, REC_COLD, 100); put32(rec, REC_POIS, 50); put32(rec, REC_BURN, 80);
    put32(rec, REC_LLEECH, 7); put32(rec, REC_MLEECH, 3);
    put64(rec, REC_VEC, (uint64_t)(uintptr_t)(rec + REC_VEC_AT)); put64(rec, REC_VEC_CAP, REC_INLINE_CAP);
    if (poison_mag) { put32(rec, REC_VEC_N, 1); put32(rec, REC_VEC_AT, 0x0a0b0c0d); put32(rec, REC_VEC_AT + 4, poison_mag); put32(rec, REC_VEC_AT + 8, 99); }
}


static void hit(uint64_t def_u, uint32_t resfl, int crit) {
    uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att_u, def_u, (uint64_t)(uintptr_t)rec };
    svc_melee_resist(r);
    put32(rec, REC_RESFL, resfl | (crit ? 0x2000 : 0));
    svc_melee_result(r);
}

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);


    const dm_splash_area C45 = { 50, 45, 2 };
    CHECK(cone_share(0, 0, 6, 0, 7, 0, &C45) == 500, "on the axis, one row behind: the full share (%d)", cone_share(0, 0, 6, 0, 7, 0, &C45));
    CHECK(cone_share(0, 0, 6, 0, 6, 2, &C45) == 295, "beside the primary, 18 of 22.5 degrees off the axis: 59 %% of it (%d)", cone_share(0, 0, 6, 0, 6, 2, &C45));
    CHECK(!cone_share(0, 0, 6, 0, 9, 0, &C45), "three rows behind is past the depth");
    CHECK(!cone_share(0, 0, 6, 0, 6, 4, &C45) && !cone_share(0, 0, 6, 0, -2, 0, &C45), "past the edge and behind the player are out");


    world_reset();
    uint64_t def = (uint64_t)(uintptr_t)mon[0];
    make_mon(0, 10, 106, 100);
    make_mon(1, 11, 107, 100);
    make_mon(2, 12, 108, 100);
    make_mon(3, 13, 98, 100);
    make_mon(4, 14, 101, 109);
    make_mon(5, 15, 125, 100);
    for (int i = 0; i < 6; i++) gate_want[i] = 10 + (uint64_t)i; n_gate_want = 6;
    set_share(50); make_rec(1, 0, 0);
    hit(def, 1, 0);
    CHECK(n_damage == 2 && n_resist == 2 && n_exec == 2, "two cone neighbours take the hit, behind / beside / out of reach do not (%d/%d/%d ids %u..%u)", n_resist, n_damage, n_exec, hit_ids[0], n_hit_ids > 1 ? hit_ids[1] : 0);

    uint8_t *b = last_rec;
    CHECK(get32(b, REC_PHYS) == 500 && get32(b, REC_FIRE) == 200 && get32(b, REC_LITNG) == 150 && get32(b, REC_MAGIC) == 100
          && get32(b, REC_COLD) == 50 && get32(b, REC_POIS) == 25 && get32(b, REC_BURN) == 40, "the share scales every damage field (phys %u)", get32(b, REC_PHYS));
    CHECK(get32(b, REC_LLEECH) == 3 && get32(b, REC_MLEECH) == 1, "leech halved (%u / %u)", get32(b, REC_LLEECH), get32(b, REC_MLEECH));
    CHECK(get32(b, REC_HITFL) == 0x20 && get32(b, REC_RESFL) == 1, "melee hit flags, result hit-only without hit recovery (%#x / %#x)", get32(b, REC_HITFL), get32(b, REC_RESFL));
    CHECK(get32(b, REC_TOTAL) == 0 && get64(b, REC_CLONE) == 1, "the fresh record starts without a total and with the tagged buffer valid-empty");
    CHECK(last_inline && get64(b, REC_VEC_CAP) == REC_INLINE_CAP && get32(b, REC_VEC_N) == 0, "the vector is rebuilt inline and empty");


    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(90); make_rec(1, 1, 0);
    hit((uint64_t)(uintptr_t)mon[0], 1 | 0x2000, 1);
    b = last_rec;
    CHECK((get32(b, REC_RESFL) & 0x2000) != 0 && get32(b, REC_PHYS) == 900, "a crit hit splashes crit-flagged at the share (%#x)", get32(b, REC_RESFL));


    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(50); make_rec(1, 0, 400);
    hit((uint64_t)(uintptr_t)mon[0], 1, 0);
    b = last_rec;
    CHECK(get32(b, REC_VEC_N) == 1 && get32(b, REC_VEC_AT) == 0x0a0b0c0d && get32(b, REC_VEC_AT + 4) == 200 && get32(b, REC_VEC_AT + 8) == 99,
          "the poison entry keeps kind and length, its magnitude halves (%u / %u / %u)", get32(b, REC_VEC_AT), get32(b, REC_VEC_AT + 4), get32(b, REC_VEC_AT + 8));

    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(90); make_rec(1, 0, 0); put64(rec, REC_VEC_CAP, 32);
    hit((uint64_t)(uintptr_t)mon[0], 1, 0);
    CHECK(n_damage == 0, "a heap-backed record vector skips the splash (%d)", n_damage);


    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(90); make_rec(1, 0, 400); put32(rec, REC_PHYS, 51200000); put32(rec, REC_VEC_AT + 4, 60000000);
    hit((uint64_t)(uintptr_t)mon[0], 1, 0);
    CHECK(get32(last_rec, REC_PHYS) == 46080000 && get32(last_rec, REC_VEC_AT + 4) == 54000000, "90 %% of large fields without overflow (%u / %u)", get32(last_rec, REC_PHYS), get32(last_rec, REC_VEC_AT + 4));

    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(90); make_rec(1, 0, 0);
    { uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att_u, (uint64_t)(uintptr_t)mon[0], (uint64_t)(uintptr_t)rec };
      svc_melee_resist(r);
      put64(rec, REC_VEC_CAP, 32);
      svc_melee_resist(r);
      put32(rec, REC_RESFL, 1); svc_melee_result(r);
      CHECK(n_damage == 0, "the rejected hit splashed with the older snapshot (%d)", n_damage);
      put64(rec, REC_VEC_CAP, REC_INLINE_CAP);
      svc_melee_resist(r); put32(rec, REC_RESFL, 0); svc_melee_result(r);
      put32(rec, REC_RESFL, 1); svc_melee_result(r);
      CHECK(n_damage == 0, "a later execute took the snapshot a miss left (%d)", n_damage); }


    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(25); make_rec(1, 0, 0);
    { uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att_u, (uint64_t)(uintptr_t)mon[0], (uint64_t)(uintptr_t)rec };
      svc_melee_resist(r);
      put32(rec, REC_RESFL, 0);
      svc_melee_result(r); CHECK(n_damage == 0, "a miss (no result hit bit) does not splash");
      put32(rec, REC_RESFL, 1);
      svc_melee_resist(r); svc_melee_result(r); CHECK(n_damage == 1, "a queued hit pairs with its execute");
      svc_melee_result(r); CHECK(n_damage == 1, "a snapshot is consumed once");
      svc_melee_resist(r);
      put32(game, 0x170, 1000 + SPLASH_TTL + 1);
      svc_melee_result(r); CHECK(n_damage == 1, "an expired snapshot does not splash");
      put32(game, 0x170, 1000);
      r[0] = (uint64_t)(uintptr_t)game2;
      svc_melee_result(r); CHECK(n_damage == 1, "after a game change the snapshot does not splash"); }

    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100); make_mon(2, 12, 103, 106); make_mon(3, 13, 103, 108);
    for (int i = 0; i < 4; i++) gate_want[i] = 10 + (uint64_t)i; n_gate_want = 4;
    set_share(25); make_rec(1, 0, 0);
    { uint64_t ra[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att_u, (uint64_t)(uintptr_t)mon[0], (uint64_t)(uintptr_t)rec };
      uint64_t rb[4] = { ra[0], ra[1], (uint64_t)(uintptr_t)mon[2], ra[3] };
      svc_melee_resist(ra); svc_melee_resist(rb);
      svc_melee_result(rb); CHECK(n_damage == 1, "the second defender splashes its own snapshot");
      svc_melee_result(ra); CHECK(n_damage == 2, "the first defender still splashes its own"); }


    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(90); make_rec(1, 0, 0);
    { uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)mon[0], (uint64_t)(uintptr_t)mon[1], (uint64_t)(uintptr_t)rec };
      svc_melee_resist(r); svc_melee_result(r); CHECK(n_damage == 0, "a monster attacker does not splash"); }
    { uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att_u, (uint64_t)(uintptr_t)att_u, (uint64_t)(uintptr_t)rec };
      svc_melee_resist(r); svc_melee_result(r); CHECK(n_damage == 0, "a player defender (PvP) does not splash"); }
    set_share(0);
    hit((uint64_t)(uintptr_t)mon[0], 1, 0);
    CHECK(n_damage == 0, "with the setting off nothing splashes");
    set_share(90);
    { uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att_u, (uint64_t)(uintptr_t)mon[0], (uint64_t)(uintptr_t)rec };
      n_call_resist = 0; svc_melee_resist(r);
      CHECK(n_call_resist == 1, "cut A always makes the original resistance call, also with the setting off"); }


    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 101); make_mon(2, 12, 107, 99);
    put32(mon[1], 0xc, 12);
    put32(mon[2], 0x124, 0x80000000u);
    gate_want[0] = 10; gate_want[1] = 11; gate_want[2] = 12; n_gate_want = 3;
    set_share(90); make_rec(1, 0, 0);
    hit((uint64_t)(uintptr_t)mon[0], 1, 0);
    CHECK(n_damage == 0, "a corpse and an owned monster are not splash targets (%d)", n_damage);

    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 101); make_mon(2, 12, 107, 99);
    gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
    set_share(90); make_rec(1, 0, 0);
    hit((uint64_t)(uintptr_t)mon[0], 1, 0);
    CHECK(n_damage == 1, "the damage gate decides unit by unit (%d)", n_damage);


    world_reset();
    make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 101); make_mon(2, 12, 107, 99);
    for (int i = 0; i < 3; i++) gate_want[i] = 10 + (uint64_t)i; n_gate_want = 3;
    set_share(90); make_rec(1, 0, 0);
    depth_probe_from = (uint64_t)(uintptr_t)mon[1];
    revalidate_id = 12;
    hit((uint64_t)(uintptr_t)mon[0], 1, 0);
    CHECK(reentered == 1, "the depth-guard probe ran inside the run (%d)", reentered);
    CHECK(n_damage == 1 && n_resist == 1, "the target that died mid-run never reaches its resistance call (%d damage, %d resist)", n_damage, n_resist);


    {

        world_reset();
        make_mon(0, 10, 106, 100); make_mon(1, 11, 106, 102);
        gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
        set_share(50); make_rec(1, 0, 0);
        hit((uint64_t)(uintptr_t)mon[0], 1, 0);
        CHECK(n_damage == 1 && get32(last_rec, REC_PHYS) == 295 && get32(last_rec, REC_FIRE) == 118, "the neighbour beside takes 59 %% of the share (%d, %u)",
              n_damage, get32(last_rec, REC_PHYS));
        dm_splash_area a; hit_facts f;
        axe_facts(4, 150); f = g_facts; area_from_facts(&f, &a);
        CHECK(a.share == 60 && a.width == 45 && a.depth == 2, "a barbarian's two-hand axe at 150 strength: 60 %%, 45 degrees, 2 (%d %d %d)", a.share, a.width, a.depth);
        f.dex = 100; area_from_facts(&f, &a); CHECK(a.width == 70, "dexterity widens the cone (%d)", a.width);
        f = g_facts; f.skill = 151; f.srvdo = 76; area_from_facts(&f, &a); CHECK(a.share == 0, "whirlwind never splashes");
        f = g_facts; f.skill = 97; f.srvdo = 150; area_from_facts(&f, &a); CHECK(a.share == 0, "smite never splashes");
        f = g_facts; f.skill = 255; f.srvdo = 42; f.kick = 1; area_from_facts(&f, &a); CHECK(a.share == 0, "a kick never splashes");
        f = g_facts; f.skill = -1; area_from_facts(&f, &a); CHECK(a.share == 0, "an unreadable skill does not splash");
        f = g_facts; f.hand[0].weapon = 0; area_from_facts(&f, &a); CHECK(a.share == 0, "unarmed does not splash");
        f = g_facts; strcpy(f.hand[0].type, "grim"); area_from_facts(&f, &a); CHECK(a.share == 0, "an unknown weapon type does not splash");
        f = g_facts; strcpy(f.hand[0].type, "knif"); area_from_facts(&f, &a); CHECK(a.share == 0, "a dagger does not splash");

        axe_facts(4, 150); strcpy(g_facts.hand[0].type, "swor"); f = g_facts;
        area_from_facts(&f, &a); CHECK(a.share == 40 && a.width == 35, "a sword held alone is medium (%d %d)", a.share, a.width);
        f.two_hands = 0; area_from_facts(&f, &a); CHECK(a.share == 25 && a.width == 30, "a sword beside a shield is light (%d %d)", a.share, a.width);
        f.hand[1].weapon = 1; f.hand[1].range = 0; strcpy(f.hand[1].type, "knif"); f.hand[1].idle = 1;
        area_from_facts(&f, &a); CHECK(a.share == 25, "beside a second weapon, the swinging sword is light (%d)", a.share);
        f.hand[0].idle = 1; f.hand[1].idle = 0; area_from_facts(&f, &a); CHECK(a.share == 0, "when the dagger hand swings, nothing splashes (%d)", a.share);
        f.hand[0].idle = 0; area_from_facts(&f, &a); CHECK(a.share == 0, "two weapons without one swinging hand: no splash");

        world_reset();
        make_mon(0, 10, 106, 100); make_mon(1, 11, 107, 100);
        gate_want[0] = 10; gate_want[1] = 11; n_gate_want = 2;
        set_share(50); make_rec(1, 0, 0);
        uint64_t r[4] = { (uint64_t)(uintptr_t)game, (uint64_t)(uintptr_t)att_u, (uint64_t)(uintptr_t)mon[0], (uint64_t)(uintptr_t)rec };
        svc_melee_resist(r);
        g_facts.srvdo = 76; svc_melee_resist(r); svc_melee_result(r);
        CHECK(n_damage == 0, "a whirlwind hit after a splashing one of the same pair does not splash (%d)", n_damage);
        g_facts.srvdo = 0; hit((uint64_t)(uintptr_t)mon[0], 1, 0);
        CHECK(n_damage == 1 && get32(last_rec, REC_PHYS) == 500, "the next normal hit splashes at its own share (%d, %u)", n_damage, get32(last_rec, REC_PHYS));
    }
printf("t_splash: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
