

#include "../dm_fightlog.c"
#include "../dm_fightlog_core.c"
#include "../dm_fightlog_store.c"
#include "../dm_fightlog_text.c"
#include "../dm_fightlog_hist.c"
#include <stdio.h>

static void (*obs[OBS_MAX])(const uint64_t *regs);
void gate_obs_set(int id, void (*fn)(const uint64_t *regs)) { obs[id] = fn; }
int dm_custom_get(const char *key) { (void)key; return 1; }
unsigned dm_effects_state_bits(uint32_t out[DM_EFFECTS_STATE_WORDS]) { (void)out; return 0; }
int dm_dmgnum_over_time(uint64_t G, uint64_t u) { (void)G; (void)u; return 0; }
int dm_dmgnum_dot_pass(uint64_t G, uint32_t tick, const uint32_t **ids) { (void)G; (void)tick; (void)ids; return 0; }
int dm_rift_swept(uint32_t id) { (void)id; return 0; }
const char *dm_save_workdir(void) { return ""; }
uint64_t dm_fs_commit_count(void) { return 0; }
int dm_threat_tier(uint64_t unit) { (void)unit; return 0; }
uint32_t dm_fl_build_key(void) { return 0; }
uint32_t dm_fl_state_key(int difficulty, dm_fl_parts *parts) { (void)difficulty; memset(parts, 0, sizeof *parts); return 0; }
uint64_t now_ns(void) { return 0; }
uint64_t dm_guest_frame(void) { return 0; }
void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *);

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL line %d: ", __LINE__); fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

typedef struct { uint8_t b[0x130]; } unit;
static unit player, monster, merc;
static uint64_t at(unit *u) { return (uint64_t)(uintptr_t)u; }
static void make(unit *u, uint32_t type, int owned) { memset(u, 0, sizeof *u); memcpy(u->b, &type, 4); uint32_t o = owned ? 1u << 31 : 0; memcpy(u->b + 0x124, &o, 4); }

static void roll(uint64_t attacker, uint64_t defender, uint32_t passed) {
    uint64_t r[32] = { attacker, defender }; obs[OBS_ATTACK_ROLL_IN](r);
    uint64_t o[32] = { passed }; obs[OBS_ATTACK_ROLL_OUT](o);
}

static void melee_raw(uint64_t attacker, uint64_t defender, int roll_passed, int raw, uint32_t flags) {
    uint64_t r[32] = { 0, attacker, defender }; obs[OBS_MELEE_RESULT_IN](r);
    if (roll_passed >= 0) roll(attacker, defender, (uint32_t)roll_passed);
    if (raw >= 0) { uint64_t a[32] = { (uint32_t)raw }; obs[OBS_MELEE_AVOID](a); }
    uint64_t o[32] = { flags }; obs[OBS_MELEE_RESULT_OUT](o);
}
static void melee(uint64_t attacker, uint64_t defender, int roll_passed, uint32_t flags) { melee_raw(attacker, defender, roll_passed, -1, flags); }
static void missile(uint64_t owner, uint64_t defender, uint32_t result) { uint64_t r[32] = { result }; r[20] = defender; r[22] = owner; obs[OBS_MISSILE_AVOID](r); }

static const char *outcomes(void) {
    static char s[80]; int n = 0;
    for (int i = 0; i < g_navoid; i++) s[n++] = g_avoid[i] == DM_FL_EN_MISS ? 'M' : g_avoid[i] == DM_FL_EN_DODGE ? 'D' : '?';
    s[n] = 0; g_navoid = 0; return s;
}
static int chained[OBS_PRODUCT_MAX]; static uint64_t chained_x0;
static void chain(int id, const uint64_t *r) { chained[id]++; chained_x0 = r[0]; }
static void c26(const uint64_t *r) { chain(26, r); }
static void c27(const uint64_t *r) { chain(27, r); }
static void c28(const uint64_t *r) { chain(28, r); }
static void c29(const uint64_t *r) { chain(29, r); }
static void c32(const uint64_t *r) { chain(32, r); }
static void c39(const uint64_t *r) { chain(39, r); }



static unit missile_unit; static uint8_t missile_data[0x20]; static uint64_t missile_owner_is;
static uint64_t owner_stub(uint64_t game, uint64_t m) { (void)game; return m == at(&missile_unit) ? missile_owner_is : 0; }
static void open_skill(uint64_t actor, int skill) { uint64_t r[128] = { 1, actor, (uint64_t)skill }; obs[OBS_COMBAT_SKILL_IN](r); }
static void close_gate(int id) { uint64_t r[128] = { 1, 0 }; obs[id](r); }
static void open_missile(int id, uint64_t owner, int skill) {
    make(&missile_unit, 3, 0); uint64_t d = (uint64_t)(uintptr_t)missile_data; memcpy(missile_unit.b + 0x10, &d, 8); int16_t sk = (int16_t)skill; memcpy(missile_data + 0xc, &sk, 2);
    missile_owner_is = owner;
    uint64_t r[128] = { 1, at(&missile_unit) }; r[19] = at(&missile_unit); r[20] = 1; obs[id](r);
}

static int hit_skill(uint64_t attacker) {
    static uint8_t rec[0x140]; int32_t total = 512; memcpy(rec + 0x134, &total, 4);
    uint64_t r[32] = { 6, 6, 1000 }; r[19] = (uint64_t)(uintptr_t)rec; r[20] = at(&monster); r[21] = attacker;
    g_nq = 0; dm_fightlog_hit(r); return g_nq == 1 ? g_q[0].h.skill : -99;
}
static int chained_skill;
static int life_100(uint64_t u, int id) { (void)u; (void)id; return 100 * 256; }
static void c51(const uint64_t *r) { (void)r; chained_skill++; }

int main(void) {
    make(&player, 0, 0); make(&monster, 1, 0); make(&merc, 1, 1);
    g_on = 1; g_player = at(&player);
    CHECK(obs[OBS_ATTACK_ROLL_IN] && obs[OBS_ATTACK_ROLL_OUT] && obs[OBS_MELEE_RESULT_IN] && obs[OBS_MELEE_RESULT_OUT] && obs[OBS_MISSILE_AVOID] && obs[OBS_MELEE_AVOID], "the gates are registered");

    roll(at(&monster), at(&player), 0); CHECK(!strcmp(outcomes(), "M"), "a failed roll of an enemy on the player is a miss");
    roll(at(&monster), at(&player), 1); CHECK(!strcmp(outcomes(), ""), "a passed roll is none");
    roll(at(&monster), at(&merc), 0); CHECK(!strcmp(outcomes(), ""), "a miss on the mercenary is not the player's");
    roll(at(&merc), at(&player), 0); CHECK(!strcmp(outcomes(), ""), "an owned monster is no enemy");
    roll(at(&player), at(&monster), 0); CHECK(!strcmp(outcomes(), ""), "the player's own miss is not counted here");
    roll(0, at(&player), 0); CHECK(!strcmp(outcomes(), ""), "an early exit without units");

    melee(at(&monster), at(&player), 1, 0); CHECK(!strcmp(outcomes(), "D"), "a passed roll that does not hit is avoided");
    melee(at(&monster), at(&player), 1, 0x10); CHECK(!strcmp(outcomes(), ""), "a shield block is no dodge");
    melee(at(&monster), at(&player), 1, 1); CHECK(!strcmp(outcomes(), ""), "a hit");
    melee(at(&monster), at(&player), 0, 0); CHECK(!strcmp(outcomes(), "M"), "a failed roll in melee is a miss only");
    melee(at(&monster), at(&player), -1, 0); CHECK(!strcmp(outcomes(), ""), "a melee result that returns before its roll");
    melee(at(&monster), at(&player), -1, 0x100); CHECK(!strcmp(outcomes(), "D"), "an avoid bit without a roll (forced hit eligibility)");
    melee(at(&monster), at(&merc), 1, 0x80); CHECK(!strcmp(outcomes(), ""), "the mercenary's dodge");
    melee_raw(at(&monster), at(&player), -1, 8, 0); CHECK(!strcmp(outcomes(), "D"), "evade of a running player: no roll, the result drops the evade bit");
    melee_raw(at(&monster), at(&player), 1, 2, 0x100); CHECK(!strcmp(outcomes(), "D"), "a dodge seen twice is one");
    melee_raw(at(&monster), at(&player), 1, 1, 0x10); CHECK(!strcmp(outcomes(), ""), "the raw shield block is no dodge");
    melee_raw(at(&monster), at(&player), 1, 0, 1); CHECK(!strcmp(outcomes(), ""), "a raw nothing, then a hit");
    melee_raw(at(&monster), at(&merc), -1, 8, 0); CHECK(!strcmp(outcomes(), ""), "the mercenary's evade");
    { uint64_t r[32] = { 0, at(&monster), at(&player) }; obs[OBS_MELEE_RESULT_IN](r);
      melee(at(&monster), at(&merc), 1, 0); roll(at(&monster), at(&player), 1);
      uint64_t o[32] = { 0 }; obs[OBS_MELEE_RESULT_OUT](o); CHECK(!strcmp(outcomes(), "D"), "nested scopes"); }

    missile(at(&monster), at(&player), 1); CHECK(!strcmp(outcomes(), ""), "a missile blocked");
    missile(at(&monster), at(&player), 4); CHECK(!strcmp(outcomes(), "D"), "a missile avoided");
    missile(at(&monster), at(&player), 0x20); CHECK(!strcmp(outcomes(), ""), "bits above the avoidance result are none");
    missile(at(&merc), at(&player), 2); CHECK(!strcmp(outcomes(), ""), "a missile of an owned monster");

    g_on = 0; roll(at(&monster), at(&player), 0); missile(at(&monster), at(&player), 4); CHECK(!strcmp(outcomes(), ""), "nothing while the fight log is off"); g_on = 1;

    dm_fightlog_obs_chain(OBS_ATTACK_ROLL_IN, c26); dm_fightlog_obs_chain(OBS_ATTACK_ROLL_OUT, c27); dm_fightlog_obs_chain(OBS_MELEE_RESULT_IN, c28);
    dm_fightlog_obs_chain(OBS_MELEE_RESULT_OUT, c29); dm_fightlog_obs_chain(OBS_MISSILE_AVOID, c32);
    melee(at(&monster), at(&player), 1, 1); missile(at(&monster), at(&player), 4);
    CHECK(chained[26] == 1 && chained[27] == 1 && chained[28] == 1 && chained[29] == 1 && chained[32] == 1 && chained_x0 == 4, "the chained observer sees every gate with its registers");
    CHECK(!strcmp(outcomes(), "D"), "and the fight log still counts");

    dm_fightlog_obs_chain(OBS_COMBAT_MON_REGEN, c39);
    g_nq = 0;
    { uint64_t r[32] = { 0 }; r[19] = 1; r[20] = at(&merc); r[2] = 5; obs[OBS_COMBAT_MON_REGEN](r); }
    CHECK(chained[39] == 1 && g_nq == 0, "the regeneration write: nothing queued for an owned monster, passed on (%d, %d)", chained[39], g_nq);


    dm_fl_missile_owner = owner_stub;
    CHECK(hit_skill(at(&player)) == -1, "a hit outside every scope has no skill");
    open_skill(at(&player), 36); CHECK(hit_skill(at(&player)) == 36, "a hit inside the player's skill");
    open_missile(OBS_COMBAT_MISSILE_HIT_IN, at(&player), 56); CHECK(hit_skill(at(&player)) == 56, "a missile's impact names the missile's skill, not the one dispatched now");
    close_gate(OBS_COMBAT_MISSILE_HIT_OUT); CHECK(hit_skill(at(&player)) == 36, "closing the impact returns to the dispatched skill");
    close_gate(OBS_COMBAT_SKILL_OUT);
    open_skill(at(&monster), 5); CHECK(hit_skill(at(&player)) == -1, "thorns: the player's hit inside the enemy's attack has no skill");
    open_skill(at(&player), 0); CHECK(hit_skill(at(&player)) == 0, "the innermost scope decides");
    close_gate(OBS_COMBAT_SKILL_OUT); close_gate(OBS_COMBAT_SKILL_OUT);
    open_missile(OBS_MISSILE_DO_IN, at(&player), 395); CHECK(hit_skill(at(&player)) == 395, "a cloud's damage in the missile do dispatch");
    { uint64_t r[128] = { 0 }; obs[OBS_COMBAT_UNKNOWN](r); } CHECK(hit_skill(at(&player)) == -1, "a callback without an actor clears the open scopes");
    close_gate(OBS_MISSILE_DO_OUT);
    open_missile(OBS_COMBAT_ABYSS_AREA_IN, at(&player), 402); CHECK(hit_skill(at(&player)) == 402, "the abyss area");
    { uint64_t r[128] = { 0 }; obs[OBS_COMBAT_ABYSS_AREA_OUT](r); } CHECK(hit_skill(at(&player)) == 402, "an abyss exit without game or missile closes nothing");
    { uint64_t r[128] = { 0 }; r[19] = r[20] = 1; obs[OBS_COMBAT_ABYSS_AREA_OUT](r); } CHECK(hit_skill(at(&player)) == -1, "the real exit closes it");
    { uint64_t r[128] = { 1, at(&player) }; r[113] = 112; obs[OBS_COMBAT_PROC_STACK_IN](r); CHECK(hit_skill(at(&player)) == 112, "a proc takes its skill from the stack"); close_gate(OBS_COMBAT_PROC_STACK_OUT); }
    for (int i = 0; i < SCOPES + 3; i++) open_skill(at(&player), 1);
    CHECK(hit_skill(at(&player)) == -1, "past the stack's room no skill is guessed");
    for (int i = 0; i < 3; i++) close_gate(OBS_COMBAT_SKILL_OUT);
    CHECK(hit_skill(at(&player)) == 1, "the scopes that fitted are still known");
    scopes_settle(); CHECK(g_nscope == 0 && hit_skill(at(&player)) == -1, "between ticks the stack starts again");
    { uint64_t r[32] = { 6, 6, 1000 }; static uint8_t rec[0x140]; int32_t total = 512; memcpy(rec + 0x134, &total, 4); r[19] = (uint64_t)(uintptr_t)rec; r[20] = at(&monster); r[21] = at(&merc);
      uint32_t cls = 271; memcpy(merc.b + 4, &cls, 4); g_nq = 0; dm_fightlog_hit(r);
      CHECK(g_nq == 1 && g_q[0].h.side == DM_FL_OUT_MINION && g_q[0].h.by == 271 && g_q[0].h.skill == -1, "a companion's hit carries its class"); }
    dm_fightlog_obs_chain(OBS_COMBAT_SKILL_IN, c51); open_skill(at(&player), 7); close_gate(OBS_COMBAT_SKILL_OUT);
    CHECK(chained_skill == 1, "the scope gates pass on to the chained probe");

    { scopes_settle(); g_nq = 0; uint32_t id = 4242; memcpy(monster.b + 8, &id, 4);
      open_skill(at(&player), 36); roll(at(&player), at(&monster), 0); close_gate(OBS_COMBAT_SKILL_OUT);
      open_missile(OBS_COMBAT_MISSILE_HIT_IN, at(&player), 56); roll(at(&player), at(&monster), 1); close_gate(OBS_COMBAT_MISSILE_HIT_OUT);
      roll(at(&monster), at(&player), 1);
      CHECK(g_nq == 2 && g_q[0].kind == Q_ENGAGE && g_q[0].e.enemy == id && g_q[0].e.skill == 36 && !g_q[0].e.delayed && g_q[1].e.skill == 56 && g_q[1].e.delayed,
            "a miss in a skill and an attack in a missile's impact are engagements, the enemy's attack is none (%d)", g_nq);
      g_nq = 0; outcomes(); }


    { dm_fl_state *st = dm_fl_core(); dm_fl_player pl = { 500, 500, 200, 200, 0, 0, 2, 0 }; dm_fl_reset(st); dm_fl_game(st, 1000); dm_fl_begin(st, 1000, &pl);
      uint32_t id = 4343, cls = 9; memcpy(monster.b + 8, &id, 4); memcpy(monster.b + 4, &cls, 4); scopes_settle(); g_nq = 0;
      static uint8_t rec[0x140]; int32_t total = 2 * 256; memcpy(rec + 0x134, &total, 4);
      uint64_t r[32] = { 0 }; r[2] = 98 * 256; r[19] = (uint64_t)(uintptr_t)rec; r[20] = at(&monster); r[21] = at(&player); dm_fightlog_hit(r);
      queued *q = &g_q[g_nq++]; q->kind = Q_DOT; q->unit = at(&monster); q->w.enemy = id; q->w.before = 98 * 256; q->w.after = 88 * 256;
      q->w.dot = (dm_fl_dot){ 1, { { DM_FL_DOT_COMP, 0, 425, 10 * 256 } } };
      q = &g_q[g_nq++]; *q = g_q[0]; q->h.before = 88 * 256; q->h.after = 0;
      drain(st, life_100);
      CHECK(g_nq == 0 && st->dealt[0] + st->dealt[2] == 100 * 256 && st->mine == 90 * 256 && st->ncomp == 1 && st->comp[0].dealt == 10 * 256,
            "poison before a killing hit of the same tick counts (%lld, mine %lld)", (long long)(st->dealt[0] + st->dealt[2]), (long long)st->mine); }
    printf("t_fightlog_gates: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
