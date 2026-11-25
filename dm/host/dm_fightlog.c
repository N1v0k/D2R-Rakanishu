
























#include "dm_fightlog.h"
#include "dm_feats.h"
#include "dm_fightlog_text.h"
#include "dm_fightlog_hist.h"
#include "dm.h"
#include "hle.h"
#include "dm_custom.h"
#include "dm_effects.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

int dm_dmgnum_over_time(uint64_t G, uint64_t u);
int dm_dmgnum_dot_pass(uint64_t G, uint32_t tick, const uint32_t **ids);
int dm_rift_swept(uint32_t id);
const char *dm_save_workdir(void);
uint64_t dm_fs_commit_count(void);

static dm_fl_parts g_open_parts;


static void store_hook(const char *wd) {
    static dm_fl_fight pend; uint64_t c = dm_fs_commit_count(); int has = dm_fl_pending(dm_fl_core(), &pend);
    dm_fl_store_commit(dm_fl_core(), wd, c); dm_dh_store_commit(dm_dh_core(), wd, c, has ? &pend : NULL, &g_open_parts);
}
__attribute__((constructor)) static void store_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[5] = store_hook; }



enum { Q_HIT, Q_DOT, Q_ENGAGE };
typedef struct {
    uint8_t kind; uint64_t unit;
    union { dm_fl_hit h; struct { uint32_t enemy; int before, after; dm_fl_dot dot; } w; struct { uint32_t enemy; int16_t skill; uint8_t delayed; } e; };
} queued;
static queued g_q[DM_FL_HITS];
static int g_nq, g_lost, g_on, g_rec = -1;
static unsigned g_games;
unsigned dm_fl_games(void) { return g_games; }
static uint8_t *B;
static uint64_t g_player;

static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static uint32_t rd32(uint64_t a) { return *(volatile uint32_t *)(uintptr_t)a; }
static int owned(uint64_t u) { return rd32(u + 0x124) >> 31; }
int dm_threat_tier(uint64_t unit);
static int ustat(uint64_t u, int id) { return ((int (*)(uint64_t, int, int))(uintptr_t)(B + 0x5b9290))(u, id, 0); }

#ifndef DM_PLAYER_BUILD

static int cost_on(void) { static int on = -1; if (on < 0) { const char *e = getenv("DM_TEST_FIGHTLOG_COST"); on = e && strcmp(e, "0"); } return on; }
static uint64_t cpu_ns(void) { struct timespec t; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
static struct { uint64_t hit_ns, hits, tick_ns[2], ticks[2], tick_max[2], clock_ns, clocks, frame0, wall0, scan_ns, scans, shared, starts, start_ns, key_ns; } g_cost;
#endif

static int skill_of(uint64_t attacker);
static int scope_delayed(void);
static int scope_triggered(uint64_t attacker);


static int mercenary(uint64_t u) {
    static uint16_t cls[3][32]; static int n[3] = { -1, -1, -1 };
    unsigned v = *(volatile uint8_t *)(uintptr_t)(u + 0x1bd);
    if (v < 1 || v > 3 || !B) return 0;
    if (n[v - 1] < 0) {
        uint64_t t = rd((uint64_t)(uintptr_t)(B + 0x2d64a18 + v * 16)), rows = t ? rd(t + 0x558) : 0, count = t ? rd(t + 0x560) : 0; n[v - 1] = 0;
        for (uint64_t k = 0; rows && count <= 4096 && k < count; k++) {
            uint32_t c = rd32(rows + k * 0x150 + 8); int have = 0;
            for (int i = 0; i < n[v - 1]; i++) have |= cls[v - 1][i] == c;
            if (!have && n[v - 1] < 32) cls[v - 1][n[v - 1]++] = (uint16_t)c;
        }
    }
    uint32_t c = rd32(u + 4);
    for (int i = 0; i < n[v - 1]; i++) if (cls[v - 1][i] == c) return 1;
    return 0;
}
static void queue_hit(const uint64_t *regs) {
    uint64_t d = regs[19], def = regs[20], att = regs[21];
    if (!d || !def || !att) return;
    uint32_t dt = rd32(def), at = rd32(att); uint64_t enemy; uint8_t side;
    if (dt == 1 && !owned(def)) {
        if (at == 0) side = DM_FL_OUT_PLAYER; else if (at == 1 && owned(att)) side = DM_FL_OUT_MINION; else return;
        enemy = def;
    } else if (at == 1 && !owned(att) && (dt == 0 || dt == 1)) { side = dt ? DM_FL_IN_MINION : DM_FL_IN_PLAYER; enemy = att; }
    else return;
    int32_t total = (int32_t)rd32(d + 0x134); if (total < 1) return;
    if (g_nq >= DM_FL_HITS) { g_lost++; return; }
    int32_t after = (int32_t)regs[2], before = after > 0 ? after + total : ustat(def, 6);
    queued *q = &g_q[g_nq++];
    q->kind = Q_HIT; q->unit = enemy;
    q->h = (dm_fl_hit){ .enemy = rd32(enemy + 8), .cls = rd32(enemy + 4), .before = before, .after = after, .su = 0xffff, .side = side, .rank = DM_FL_RANK_NORMAL,
                        .crit = (uint8_t)((rd32(d + 4) & 0x2000) != 0), .skill = -1, .by = 0xffff };
    if (side == DM_FL_OUT_PLAYER) {
        q->h.skill = (int16_t)skill_of(att); q->h.delayed = (uint8_t)scope_delayed(); q->h.triggered = (uint8_t)scope_triggered(att);

        q->h.lays = (uint8_t)((rd32(d + 0x3c) || rd(d + 0x48) ? DM_FL_LAYS_POISON : 0) | (rd32(d + 0x24) && rd32(d + 0x28) ? DM_FL_LAYS_BURN : 0));
    }
    else if (side == DM_FL_OUT_MINION) { q->h.by = (uint16_t)rd32(att + 4); q->h.merc = (uint8_t)mercenary(att); }
    if (side != DM_FL_IN_PLAYER && side != DM_FL_IN_MINION && after <= 0 && g_player && dm_ft_act_boss(q->h.cls)) { q->h.plife = ustat(g_player, 6); q->h.pmax = ustat(g_player, 7); }
    if (side == DM_FL_IN_PLAYER) q->h.pmax = ustat(def, 7);
}
void dm_fightlog_hit(const uint64_t *regs) {
    if (!g_on) return;
#ifndef DM_PLAYER_BUILD
    if (cost_on()) { uint64_t t0 = cpu_ns(); queue_hit(regs); uint64_t t1 = cpu_ns(), t2 = cpu_ns(); g_cost.hit_ns += t1 - t0; g_cost.hits++; g_cost.clock_ns += t2 - t1; g_cost.clocks++; return; }
#endif
    queue_hit(regs);
}


enum { NEST = 16 };
typedef struct { uint64_t attacker, defender; int passed; uint32_t avoid; } scope;
static struct { scope roll[NEST], melee[NEST]; int nroll, nmelee, lost_roll, lost_melee; } g_sc;
static uint8_t g_avoid[64]; static int g_navoid;
static void (*g_chain[OBS_PRODUCT_MAX])(const uint64_t *regs);
void dm_fightlog_obs_chain(int id, void (*fn)(const uint64_t *regs)) { if (id > 0 && id < OBS_PRODUCT_MAX) g_chain[id] = fn; }
static int on_player(uint64_t attacker, uint64_t defender) { return attacker && defender && defender == g_player && rd32(attacker) == 1 && !owned(attacker); }
static void avoided(int what) { if (g_navoid < (int)sizeof g_avoid) g_avoid[g_navoid++] = (uint8_t)what; }
static void push(scope *st, int *n, int *lost, uint64_t attacker, uint64_t defender) { if (*n < NEST) st[(*n)++] = (scope){ attacker, defender, 0, 0 }; else ++*lost; }

static scope *pop(scope *st, int *n, int *lost) { if (*lost) { --*lost; return NULL; } return *n ? &st[--*n] : NULL; }
static void roll_in(const uint64_t *r) {
    if (g_on) push(g_sc.roll, &g_sc.nroll, &g_sc.lost_roll, r[0], r[1]);

    if (g_on && r[0] && r[0] == g_player && r[1] && rd32(r[1]) == 1 && !owned(r[1])) {
        if (g_nq >= DM_FL_HITS) g_lost++;
        else { queued *q = &g_q[g_nq++]; q->kind = Q_ENGAGE; q->unit = r[1]; q->e.enemy = rd32(r[1] + 8); q->e.skill = (int16_t)skill_of(r[0]); q->e.delayed = (uint8_t)scope_delayed(); }
    }
    if (g_chain[OBS_ATTACK_ROLL_IN]) g_chain[OBS_ATTACK_ROLL_IN](r);
}
static void roll_out(const uint64_t *r) {
    scope *c = g_on ? pop(g_sc.roll, &g_sc.nroll, &g_sc.lost_roll) : NULL;
    if (c) {
        int passed = (uint32_t)r[0] != 0;
        if (g_sc.nmelee && g_sc.melee[g_sc.nmelee - 1].attacker == c->attacker && g_sc.melee[g_sc.nmelee - 1].defender == c->defender) g_sc.melee[g_sc.nmelee - 1].passed = passed;
        if (!passed && on_player(c->attacker, c->defender)) avoided(DM_FL_EN_MISS);
    }
    if (g_chain[OBS_ATTACK_ROLL_OUT]) g_chain[OBS_ATTACK_ROLL_OUT](r);
}
static void melee_in(const uint64_t *r) {
    if (g_on) push(g_sc.melee, &g_sc.nmelee, &g_sc.lost_melee, r[1], r[2]);
    if (g_chain[OBS_MELEE_RESULT_IN]) g_chain[OBS_MELEE_RESULT_IN](r);
}
static void melee_out(const uint64_t *r) {
    scope *c = g_on ? pop(g_sc.melee, &g_sc.nmelee, &g_sc.lost_melee) : NULL;
    uint32_t w = (uint32_t)r[0];
    if (c && ((c->avoid & ~1u) || (!(w & 0x11) && ((w & 0xffff8180u) || c->passed))) && on_player(c->attacker, c->defender)) avoided(DM_FL_EN_DODGE);
    if (g_chain[OBS_MELEE_RESULT_OUT]) g_chain[OBS_MELEE_RESULT_OUT](r);
}
static void melee_avoid(const uint64_t *r) {
    if (g_on && g_sc.nmelee && !g_sc.lost_melee) g_sc.melee[g_sc.nmelee - 1].avoid = (uint32_t)r[0];
    if (g_chain[OBS_MELEE_AVOID]) g_chain[OBS_MELEE_AVOID](r);
}
static void missile_avoid(const uint64_t *r) {
    uint32_t w = (uint32_t)r[0];
    if (g_on && (w & 0x1f) && !(w & 1) && on_player(r[22], r[20])) avoided(DM_FL_EN_DODGE);
    if (g_chain[OBS_MISSILE_AVOID]) g_chain[OBS_MISSILE_AVOID](r);
}



static int dot_owners(uint64_t G, uint64_t u, uint64_t player, dm_fl_dot *d);
static void regen_write(const uint64_t *r) {
    uint64_t G = r[19], u = r[20];
    if (g_on && G && u && B && g_player && rd32(u) == 1 && !owned(u) && (g_nq || dm_fl_open(dm_fl_core()))) {
        int before = ustat(u, 6), after = (int)(uint32_t)r[2];
        if (before > after) {
            if (g_nq >= DM_FL_HITS) g_lost++;
            else { queued *q = &g_q[g_nq]; q->kind = Q_DOT; q->unit = u; q->w.enemy = rd32(u + 8); q->w.before = before; q->w.after = after; if (dot_owners(G, u, g_player, &q->w.dot)) g_nq++; }
        }
    }
    if (g_chain[OBS_COMBAT_MON_REGEN]) g_chain[OBS_COMBAT_MON_REGEN](r);
}




enum { SCOPES = 32 };
static struct { uint64_t actor; int skill, missile, proc; } g_scope[SCOPES];

static int g_nscope, g_scope_over, g_scope_logged;
uint64_t (*dm_fl_missile_owner)(uint64_t game, uint64_t missile);
static uint64_t missile_owner(uint64_t game, uint64_t missile) { return B ? ((uint64_t (*)(uint64_t, uint64_t))(uintptr_t)(B + 0x7cd7b0))(game, missile) : 0; }
static void scope_open(uint64_t actor, int skill) { if (g_nscope < SCOPES) { g_scope[g_nscope].actor = actor; g_scope[g_nscope].skill = skill; g_scope[g_nscope].missile = g_scope[g_nscope].proc = 0; g_nscope++; } else g_scope_over++; }
static void scope_proc(uint64_t actor, int skill) { int n = g_nscope; scope_open(actor, skill); if (g_nscope > n) g_scope[n].proc = 1; }
static void scope_close(void) { if (g_scope_over) g_scope_over--; else if (g_nscope) g_nscope--; }
static int skill_of(uint64_t attacker) { return g_nscope && !g_scope_over && g_scope[g_nscope - 1].actor == attacker ? g_scope[g_nscope - 1].skill : -1; }
static void scope_missile(uint64_t game, uint64_t missile, int skill) {
    int n = g_nscope; scope_open(missile && rd32(missile) == 3 && dm_fl_missile_owner ? dm_fl_missile_owner(game, missile) : 0, skill);
    if (g_nscope > n) g_scope[n].missile = 1;
}
static int scope_delayed(void) { return !g_nscope || g_scope_over || g_scope[g_nscope - 1].missile; }


static int scope_triggered(uint64_t attacker) {
    if (!g_nscope) return 1;
    if (g_scope_over) return 0;
    if (g_scope[g_nscope - 1].actor != attacker) return 1;
    for (int i = 0; i < g_nscope; i++) if (g_scope[i].proc) return 1;
    return 0;
}
static int missile_skill(uint64_t missile) { uint64_t d = missile && rd32(missile) == 3 ? rd(missile + 0x10) : 0; return d ? *(volatile int16_t *)(uintptr_t)(d + 0xc) : -1; }
#define CHAINED(id) do { if (g_chain[id]) g_chain[id](r); } while (0)
static void skill_in(const uint64_t *r) { scope_open(r[1], (int)r[2]); CHAINED(OBS_COMBAT_SKILL_IN); }
static void proc_stack_in(const uint64_t *r) { scope_proc(r[1], (int)r[113]); CHAINED(OBS_COMBAT_PROC_STACK_IN); }
static void proc_in(const uint64_t *r) { scope_proc(r[1], (int)r[2]); CHAINED(OBS_COMBAT_PROC_IN); }
static void state_in(const uint64_t *r) { scope_open(r[1], (int)r[2]); CHAINED(OBS_COMBAT_STATE_IN); }
static void missile_stack_in(const uint64_t *r) { scope_missile(r[0], r[19], (int)r[2]); CHAINED(OBS_COMBAT_MISSILE_STACK_IN); }
static void missile_skill_in(const uint64_t *r) { scope_missile(r[0], r[1], (int)r[2]); CHAINED(OBS_COMBAT_MISSILE_SKILL_IN); }
static void missile_hit_in(const uint64_t *r) { scope_missile(r[0], r[1], missile_skill(r[1])); CHAINED(OBS_COMBAT_MISSILE_HIT_IN); }
static void missile_area_in(const uint64_t *r) { scope_missile(r[0], r[1], missile_skill(r[1])); CHAINED(OBS_COMBAT_MISSILE_AREA_IN); }
static void abyss_in(const uint64_t *r) { scope_missile(r[20], r[19], missile_skill(r[19])); CHAINED(OBS_COMBAT_ABYSS_AREA_IN); }
static void abyss_out(const uint64_t *r) { if (r[20] && r[19]) scope_close(); CHAINED(OBS_COMBAT_ABYSS_AREA_OUT); }
static void missile_do_in(const uint64_t *r) { scope_missile(r[20], r[19], missile_skill(r[19])); CHAINED(OBS_MISSILE_DO_IN); }
static void unknown(const uint64_t *r) { for (int i = 0; i < g_nscope; i++) g_scope[i].skill = -1; CHAINED(OBS_COMBAT_UNKNOWN); }
#define CLOSE(name, id) static void name(const uint64_t *r) { scope_close(); CHAINED(id); }
CLOSE(skill_out, OBS_COMBAT_SKILL_OUT) CLOSE(proc_stack_out, OBS_COMBAT_PROC_STACK_OUT) CLOSE(proc_out, OBS_COMBAT_PROC_OUT) CLOSE(state_out, OBS_COMBAT_STATE_OUT)
CLOSE(missile_stack_out, OBS_COMBAT_MISSILE_STACK_OUT) CLOSE(missile_skill_out, OBS_COMBAT_MISSILE_SKILL_OUT) CLOSE(missile_hit_out, OBS_COMBAT_MISSILE_HIT_OUT)
CLOSE(missile_area_out, OBS_COMBAT_MISSILE_AREA_OUT) CLOSE(missile_do_out, OBS_MISSILE_DO_OUT)
#undef CLOSE
#undef CHAINED

static void scopes_settle(void) {
    if ((g_nscope || g_scope_over) && !g_scope_logged) { g_scope_logged = 1; LOG("ASSUME dps.scopes: %d skill scopes still open between server ticks; the skill of own hits may be wrong until they close", g_nscope + g_scope_over); }
    g_nscope = g_scope_over = 0;
}
__attribute__((constructor)) static void gates_install(void) {
    gate_obs_set(OBS_ATTACK_ROLL_IN, roll_in); gate_obs_set(OBS_ATTACK_ROLL_OUT, roll_out);
    gate_obs_set(OBS_MELEE_RESULT_IN, melee_in); gate_obs_set(OBS_MELEE_RESULT_OUT, melee_out); gate_obs_set(OBS_MISSILE_AVOID, missile_avoid);
    gate_obs_set(OBS_MELEE_AVOID, melee_avoid); gate_obs_set(OBS_COMBAT_MON_REGEN, regen_write);
    const struct { int id; void (*fn)(const uint64_t *); } scopes[] = {
        { OBS_COMBAT_SKILL_IN, skill_in }, { OBS_COMBAT_SKILL_OUT, skill_out }, { OBS_COMBAT_PROC_STACK_IN, proc_stack_in }, { OBS_COMBAT_PROC_STACK_OUT, proc_stack_out },
        { OBS_COMBAT_PROC_IN, proc_in }, { OBS_COMBAT_PROC_OUT, proc_out }, { OBS_COMBAT_STATE_IN, state_in }, { OBS_COMBAT_STATE_OUT, state_out }, { OBS_COMBAT_UNKNOWN, unknown },
        { OBS_COMBAT_MISSILE_STACK_IN, missile_stack_in }, { OBS_COMBAT_MISSILE_STACK_OUT, missile_stack_out }, { OBS_COMBAT_MISSILE_SKILL_IN, missile_skill_in },
        { OBS_COMBAT_MISSILE_SKILL_OUT, missile_skill_out }, { OBS_COMBAT_MISSILE_HIT_IN, missile_hit_in }, { OBS_COMBAT_MISSILE_HIT_OUT, missile_hit_out },
        { OBS_COMBAT_MISSILE_AREA_IN, missile_area_in }, { OBS_COMBAT_MISSILE_AREA_OUT, missile_area_out }, { OBS_COMBAT_ABYSS_AREA_IN, abyss_in }, { OBS_COMBAT_ABYSS_AREA_OUT, abyss_out },
        { OBS_MISSILE_DO_IN, missile_do_in }, { OBS_MISSILE_DO_OUT, missile_do_out } };
    for (size_t k = 0; k < sizeof scopes / sizeof *scopes; k++) gate_obs_set(scopes[k].id, scopes[k].fn);
    if (!dm_fl_missile_owner) dm_fl_missile_owner = missile_owner;
}

static uint64_t unit_of(uint64_t G, uint32_t id) {
    uint64_t u = rd(G + 0x2230 + 0x400 + 8 * (uint64_t)(id & 0x7f));
    for (int k = 0; u && k < 256; u = rd(u + 0x158), k++) if (rd32(u + 8) == id) return u;
    return 0;
}
static void rank_of(uint64_t u, uint8_t *rank, uint16_t *su) {
    uint64_t md = rd(u + 0x10); uint8_t fl = md ? *(volatile uint8_t *)(uintptr_t)(md + 0x1a) : 0;
    *su = 0xffff;
    if (fl & 2) { *rank = DM_FL_RANK_SUPER; *su = *(volatile uint16_t *)(uintptr_t)(md + 0x2a); }
    else *rank = fl & 8 ? DM_FL_RANK_UNIQUE : fl & 4 ? DM_FL_RANK_CHAMPION : fl & 0x10 ? DM_FL_RANK_MINION : DM_FL_RANK_NORMAL;
}
static int monster_dead(uint32_t mode) { return mode == 0 || mode == 12; }
static int monster_attack(uint32_t mode) { return mode == 4 || mode == 5 || (mode >= 7 && mode <= 11); }

static int potion_kind(uint64_t item) {
    static const char codes[12][4] = { "hp1", "hp2", "hp3", "hp4", "hp5", "mp1", "mp2", "mp3", "mp4", "mp5", "rvs", "rvl" };
    if (!item) return -1;
    uint32_t code = ((uint32_t (*)(uint64_t))(uintptr_t)(B + 0x570260))(item); const char *c = (const char *)&code;
    for (int k = 0; k < 12; k++) if (!memcmp(c, codes[k], 3) && (c[3] == ' ' || !c[3])) return k;
    return -1;
}
static int active_skill(uint64_t pl) {
    uint64_t sl = rd(pl + 0x100), n = sl ? rd(sl + 0x18) : 0, r = n ? rd(n) : 0;
    return r ? *(volatile int16_t *)(uintptr_t)r : -1;
}




static int dot_owners(uint64_t G, uint64_t u, uint64_t player, dm_fl_dot *d) {
    d->n = 0;
    uint64_t root = rd(u + 0x88); unsigned ver = *(volatile uint8_t *)(uintptr_t)(u + 0x1bd);
    if (!root || !(rd32(root + 0x1c) >> 31) || ver < 1 || ver > 3) return 0;
    for (int pass = 0; pass < 2; pass++) for (uint64_t n = rd(root + (pass ? 0xd8 : 0xd0)), k = 0; n && k < 64; n = rd(n + 0x68), k++) {
        uint32_t state = rd32(n + 0x20); if (state != 2 && state != 0x73) continue;
        if (rd32(n + 0x1c) & 0x2000) continue;
        int rate = -((int (*)(int, uint64_t, int, int))(uintptr_t)(B + 0x5bc690))((int)ver, n, 0x4a, 0); if (rate <= 0) continue;
        uint32_t type = rd32(n + 8), id = rd32(n + 0xc); int kind; uint16_t cls = 0;
        if (type == 0) kind = id == rd32(player + 8) ? DM_FL_DOT_PLAYER : DM_FL_DOT_UNKNOWN;
        else if (type == 1) {
            uint64_t o = unit_of(G, id);
            if (!o) kind = DM_FL_DOT_UNKNOWN;
            else if (!owned(o)) continue;
            else if (mercenary(o)) kind = DM_FL_DOT_MERC;
            else { kind = DM_FL_DOT_COMP; cls = (uint16_t)rd32(o + 4); }
        } else kind = DM_FL_DOT_UNKNOWN;
        if (d->n == DM_FL_DOTS) { d->e[DM_FL_DOTS - 1].kind = DM_FL_DOT_UNKNOWN; d->e[DM_FL_DOTS - 1].rate += rate; continue; }
        d->e[d->n].kind = (uint8_t)kind; d->e[d->n].burn = state == 0x73; d->e[d->n].cls = cls; d->e[d->n].rate = rate; d->n++;
    }
    return d->n;
}


static void drain(dm_fl_state *s, int (*stat)(uint64_t u, int id)) {
    for (int i = 0; i < g_nq; i++) {
        queued *q = &g_q[i];
        if (q->kind == Q_DOT) { dm_fl_dot_write(s, q->w.enemy, q->w.before, q->w.after, &q->w.dot); continue; }
        if (q->kind == Q_ENGAGE) { dm_fl_engage(s, q->e.enemy, q->e.skill, q->e.delayed); continue; }
        int life = 0, max = 0;
        if (rd32(q->unit + 8) == q->h.enemy && rd32(q->unit) == 1) {
            rank_of(q->unit, &q->h.rank, &q->h.su);
            if (!dm_fl_known(s, q->h.enemy)) { life = stat(q->unit, 6); max = stat(q->unit, 7); q->h.boss = dm_threat_tier(q->unit) == 3; }
        }
        dm_fl_on_hit(s, &q->h, life, max);
    }
    g_nq = 0;
}
static void tick(uint64_t G, uint64_t player, const dm_fl_player *p, const int belt[4], const uint64_t belt_item[4]) {
    dm_fl_state *s = dm_fl_core();
    static uint64_t game, litem[4]; static uint32_t last_t, switch_at; static int switch_read, pmode = -1, lbelt[4], lkind[4] = { -1, -1, -1, -1 }, full_logged, elites_logged, hits_logged, was_open; static unsigned state_seq;
    uint32_t t = rd32(G + 0x170);
    if (!switch_read || t - switch_at >= DM_FL_TPS) {
        switch_read = 1; switch_at = t;


        int rec = dm_custom_get("fight_log") != 0;
        if (!g_on || rec != g_rec) {
            dm_fl_record(s, rec); g_rec = rec; LOG("fight log: recording %s", rec ? "on" : "off");
            if (!g_on) { g_on = 1; g_nq = g_navoid = 0; memset(&g_sc, 0, sizeof g_sc); was_open = 0; }
        }
    }
    if (G != game || t < last_t) {
        g_games++;

        if (dm_fl_game(s, t)) dm_dh_add(dm_dh_core(), dm_fl_last(s)->s.state, &g_open_parts, dm_fl_last(s));
        game = G; g_nq = g_lost = g_navoid = 0; memset(&g_sc, 0, sizeof g_sc); pmode = -1; was_open = 0; memset(lbelt, 0, sizeof lbelt); state_seq = 0;
        uint64_t pd = rd(player + 0x10); char name[64] = "";
        if (pd) { memcpy(name, (const void *)(uintptr_t)pd, sizeof name - 1); name[sizeof name - 1] = 0; }
        dm_fl_store_follow(s, dm_save_workdir(), name); dm_dh_store_follow(dm_dh_core(), dm_save_workdir(), name);
    }
    last_t = t;
    if (!g_on) return;
    dm_fl_begin(s, t, p);
    if (g_lost) { dm_fl_lost_hits(s, g_lost); g_lost = 0; }
    drain(s, ustat);
    for (int i = 0; i < g_navoid; i++) dm_fl_avoided(s, g_avoid[i]);
    g_navoid = 0;
    { uint32_t mode = rd32(player + 0xc);
      if ((int)mode != pmode) {

          if (mode == 10) dm_fl_action(s, t, DM_FL_ME_CAST, active_skill(player));
          else if (mode == 7 || mode == 8 || (mode >= 11 && mode <= 16)) dm_fl_action(s, t, DM_FL_ME_ATTACK, active_skill(player));
          else if (mode == 9) dm_fl_block(s, t);
          pmode = (int)mode;
      } }
    if (dm_fl_open(s)) {
        int sample = t % DM_FL_SAMPLE == 0;
        for (int i = 0; i < dm_fl_slots(s); i++) {
            uint32_t id = dm_fl_slot_id(s, i); if (!id) continue;
            uint64_t u = unit_of(G, id);
            uint32_t mode = u ? rd32(u + 0xc) : 0; int dead = u && monster_dead(mode);
            if (!u || (dead && dm_rift_swept(id))) { dm_fl_slot_seen(s, i, 1, 0, 0, -1, 0); continue; }
            int life = sample && !dead ? ustat(u, 6) : -1;
            static dm_fl_dot dot;
            int has = life >= 0 && dm_fl_slot_unexplained(s, i, life) > 0 && dot_owners(G, u, player, &dot);
            int attack = !dead && monster_attack(mode) ? (int)mode : 0;
            dm_fl_slot_seen(s, i, 0, dead, attack, life, has);
        }
        if (sample && (dm_fl_flags(s) & DM_FL_F_TABLE_FULL))
            for (int m = 0; m < dm_fl_members(s); m++) {
                uint32_t id = dm_fl_member_watch(s, m); if (!id) continue;
                uint64_t u = unit_of(G, id); int dead = u && monster_dead(rd32(u + 0xc));
                dm_fl_member_seen(s, m, !u || (dead && dm_rift_swept(id)), dead);
            }
    }
    if (t % DM_FL_SCAN == 0 && !p->dead && !p->town) {
#ifndef DM_PLAYER_BUILD
        uint64_t s0 = cost_on() ? cpu_ns() : 0;
#endif
        const uint32_t *ids; int n = dm_dmgnum_dot_pass(G, t, &ids);
        if (n >= 0)
            for (int i = 0; i < n; i++) {
                uint64_t u = unit_of(G, ids[i]); if (!u || dm_fl_known(s, ids[i])) continue;
                uint8_t rank; uint16_t su; rank_of(u, &rank, &su);
                dm_fl_on_dot(s, ids[i], rd32(u + 4), rank, su, dm_threat_tier(u) == 3, ustat(u, 6), ustat(u, 7));
            }
        else
            for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 0x400 + 8 * (uint64_t)b), k = 0; u && k < 256; u = rd(u + 0x158), k++) {
                uint32_t mode = rd32(u + 0xc), id = rd32(u + 8);
                if (monster_dead(mode) || owned(u) || dm_fl_known(s, id) || !dm_dmgnum_over_time(G, u)) continue;
                uint8_t rank; uint16_t su; rank_of(u, &rank, &su);
                dm_fl_on_dot(s, id, rd32(u + 4), rank, su, dm_threat_tier(u) == 3, ustat(u, 6), ustat(u, 7));
            }
#ifndef DM_PLAYER_BUILD
        if (s0) { g_cost.scan_ns += cpu_ns() - s0; g_cost.scans++; g_cost.shared += n >= 0; }
#endif
    }
    for (int c = 0; c < 4; c++) {
        if (!p->dead && belt[c] == lbelt[c] - 1 && lkind[c] >= 0) dm_fl_potion(s, t, c, lkind[c]);
        if (belt_item[c] != litem[c]) { litem[c] = belt_item[c]; lkind[c] = potion_kind(belt_item[c]); }
        lbelt[c] = belt[c];
    }
    { uint32_t bits[DM_EFFECTS_STATE_WORDS]; unsigned seq = dm_effects_state_bits(bits); if (seq != state_seq) { state_seq = seq; dm_fl_states(s, t, bits); } }
    if (dm_fl_open(s) && (dm_fl_flags(s) & DM_FL_F_TABLE_FULL) && !full_logged) {
        full_logged = 1; LOG("ASSUME fightlog.table_full: more than %d enemies at once in one fight; the rest count for damage and kills, not for the life line", DM_FL_TRACK);
    }
    if (dm_fl_elites_folded(s) && !elites_logged) {
        elites_logged = 1; LOG("ASSUME fightlog.elite_groups: more unique, super unique and boss kinds in one fight than its %d groups; one went into the others", DM_FL_GROUPS);
    }
    if (dm_fl_open(s) && !was_open) {
#ifndef DM_PLAYER_BUILD
        uint64_t k0 = cost_on() ? cpu_ns() : 0;
#endif
        dm_fl_build(s, dm_fl_build_key(), dm_fl_state_key(p->difficulty, &g_open_parts));
#ifndef DM_PLAYER_BUILD
        if (k0) g_cost.key_ns += cpu_ns() - k0;
#endif
    }
    int kept, kind = dm_fl_end(s, &kept);
    was_open = dm_fl_open(s);
    if (!kind) return;
    full_logged = elites_logged = 0;
    const dm_fl_fight *f = dm_fl_last(s);
    if (kept) dm_dh_add(dm_dh_core(), f->s.state, &g_open_parts, f);
    uint32_t lost, hits; dm_fl_player_loss(s, &lost, &hits);
    static int split_logged;
    if (dm_fl_split_gap(s) && split_logged < 5) { split_logged++; LOG("ASSUME dps.split: the fight's sources add up to %lld/256 points more than its damage", (long long)dm_fl_split_gap(s)); }
    uint32_t slack = (uint32_t)(p->hp_max > 250 ? p->hp_max / 5 : 50);
    if (lost > hits + slack && hits_logged < 5) {
        hits_logged++; LOG("ASSUME fightlog.player_hits: the player lost %u life in a fight of %u ticks, the hits seen on the player account for %u", lost, f->h.duration, hits);
    }
#ifndef DM_PLAYER_BUILD
    { char sk[64] = ""; int at = 0, me = 0, en = 0;
      for (int k = 0; k < DM_FL_SKILLS && f->h.skill[k]; k++) at += snprintf(sk + at, sizeof sk - (size_t)at, "%s%u", k ? "," : "", f->h.skill[k]);
      for (int k = 0; k < f->h.nbuckets; k++) { me |= f->b[k].me; en |= f->b[k].en; }
      uint32_t comps = 0; for (int k = 0; k < DM_FL_COMPS; k++) comps += f->s.comp[k].dealt;
      LOG("[fightlog] end=%u kept=%d duration=%u lead=%u area=%u enemies=%u kills=%u groups=%u dealt=%u/%u/%u taken=%u life_total=%u min_life=%u buckets=%u width=%u marks=%u flags=%#x ring=%d skills=%s me=%#x en=%#x build=%08x split=%u/%u/%u/%u/%u/%u state=%08x",
          f->h.end, kept, f->h.duration, f->h.lead, f->h.area, f->h.enemies, f->h.kills, f->h.ngroups, f->h.dealt[0], f->h.dealt[1], f->h.dealt[2], f->h.taken, f->h.life_total,
          f->h.min_life, f->h.nbuckets, f->h.width, f->h.nmarks, f->h.flags, dm_fl_count(s), sk[0] ? sk : "-", me, en, f->h.build, f->s.mine, f->s.none, f->s.merc, comps + f->s.rest, f->s.unknown, f->s.over, f->s.state); }
#endif
}


void dm_fightlog_tick(uint64_t G, uint64_t player, uint8_t *main_base, const dm_fl_player *p, const int belt[4], const uint64_t belt_item[4]) {
    if (!G || !player || !main_base) return;
    B = main_base; g_player = player; scopes_settle();
#ifndef DM_PLAYER_BUILD
    if (cost_on()) {
        int open = dm_fl_open(dm_fl_core()); uint64_t t0 = cpu_ns();
        tick(G, player, p, belt, belt_item);
        uint64_t d = cpu_ns() - t0; g_cost.tick_ns[open] += d; g_cost.ticks[open]++; if (d > g_cost.tick_max[open]) g_cost.tick_max[open] = d;
        if (!open && dm_fl_open(dm_fl_core())) { g_cost.starts++; g_cost.start_ns += d; }
        uint64_t frame = dm_guest_frame(), wall = now_ns();
        if (!g_cost.wall0) { g_cost.frame0 = frame; g_cost.wall0 = wall; }
        if (g_cost.ticks[0] + g_cost.ticks[1] >= 10 * DM_FL_TPS) {
            uint64_t fr = frame - g_cost.frame0, ms = (wall - g_cost.wall0) / 1000000;
            LOG("[fightlog-cost] recording=%d ticks=%llu/%llu (outside/in fight) tick_ns=%llu/%llu max=%llu/%llu hits=%llu hit_ns=%llu clock_ns=%llu frames=%llu ms=%llu frame_ms=%.2f scan_ns=%llu scans=%llu/%llu starts=%llu start_ns=%llu key_ns=%llu plain_ns=%llu",
                g_on, (unsigned long long)g_cost.ticks[0], (unsigned long long)g_cost.ticks[1],
                (unsigned long long)(g_cost.ticks[0] ? g_cost.tick_ns[0] / g_cost.ticks[0] : 0), (unsigned long long)(g_cost.ticks[1] ? g_cost.tick_ns[1] / g_cost.ticks[1] : 0),
                (unsigned long long)g_cost.tick_max[0], (unsigned long long)g_cost.tick_max[1], (unsigned long long)g_cost.hits,
                (unsigned long long)(g_cost.hits ? g_cost.hit_ns / g_cost.hits : 0), (unsigned long long)(g_cost.clocks ? g_cost.clock_ns / g_cost.clocks : 0),
                (unsigned long long)fr, (unsigned long long)ms, fr ? (double)ms / (double)fr : 0.0,
                (unsigned long long)(g_cost.scans ? g_cost.scan_ns / g_cost.scans : 0), (unsigned long long)g_cost.scans, (unsigned long long)g_cost.shared,
                (unsigned long long)g_cost.starts, (unsigned long long)(g_cost.starts ? g_cost.start_ns / g_cost.starts : 0), (unsigned long long)(g_cost.starts ? g_cost.key_ns / g_cost.starts : 0),
                (unsigned long long)(g_cost.ticks[0] > g_cost.starts ? (g_cost.tick_ns[0] - g_cost.start_ns) / (g_cost.ticks[0] - g_cost.starts) : 0));
            memset(&g_cost, 0, sizeof g_cost); g_cost.frame0 = frame; g_cost.wall0 = wall;
        }
        return;
    }
#endif
    tick(G, player, p, belt, belt_item);
}
