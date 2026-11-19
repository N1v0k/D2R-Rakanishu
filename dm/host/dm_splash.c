

















#include "dm.h"
#include "hle.h"
#include "dm_custom.h"
#include "dm_splash.h"
#include "dm_splash_rule.h"
#include "dm_unit.h"
#include <stdlib.h>
#include <string.h>

#define SPLASH_TTL 50
#define SPLASH_SLOTS 8
#define SPLASH_VEC_MAX 16
#define SPLASH_TARGETS 64


#define REC_HITFL 0x00
#define REC_RESFL 0x04
#define REC_PHYS 0x18
#define REC_BURN 0x24
#define REC_FIRE 0x20
#define REC_LITNG 0x2c
#define REC_MAGIC 0x30
#define REC_COLD 0x34
#define REC_POIS 0x38
#define REC_LLEECH 0x120
#define REC_MLEECH 0x124
#define REC_TOTAL 0x134
#define REC_VEC 0x40
#define REC_VEC_N 0x48
#define REC_VEC_CAP 0x50
#define REC_VEC_AT 0x58
#define REC_CLONE 0x158
#define REC_INLINE_CAP 0x8000000000000010ull

static uint8_t *base(void) {
    static uint8_t *b;
    if (!b) {
        extern module_t g_mod[]; extern int g_nmod;
        for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base;
    }
    return b;
}

#ifndef DM_SPLASH_GAME_RESIST
static void call_resist(const uint64_t *r) { ((void (*)(uint64_t, uint64_t, uint64_t, uint64_t))(uintptr_t)(base() + 0x7d3c50))(r[0], r[1], r[2], r[3]); }
#else
#define call_resist DM_SPLASH_GAME_RESIST
#endif
#ifndef DM_SPLASH_GAME_RESULT
static void call_result(const uint64_t *r) { ((void (*)(uint64_t, uint64_t, uint64_t, uint64_t))(uintptr_t)(base() + 0x7d8880))(r[0], r[1], r[2], r[3]); }
#else
#define call_result DM_SPLASH_GAME_RESULT
#endif

static uint32_t rd32(uint64_t a) { return *(volatile uint32_t *)(uintptr_t)a; }
static uint64_t rd64(uint64_t a) { return *(volatile uint64_t *)(uintptr_t)a; }
static int owned(uint64_t u) { return rd32(u + 0x124) >> 31; }
static int live(const uint64_t *r) { return r[0] && r[1] && r[2] && r[3]; }
static const uint32_t REC_DMG_AT[] = { REC_PHYS, REC_BURN, REC_FIRE, REC_LITNG, REC_MAGIC, REC_COLD, REC_POIS };


static int splash_on(void) { return dm_custom_splash_on(); }
static int g_depth;
int dm_splash_running(void) { return g_depth; }



#ifndef DM_PLAYER_BUILD
static int obs_on(void) { static int v = -1; if (v < 0) { const char *e = getenv("DM_TEST_SPLASH_OBS"); v = e && !strcmp(e, "1"); } return v; }
unsigned dm_dmgnum_gate_calls(void);

enum { PAIRS = 16 };
typedef struct { uint64_t game; uint32_t at, ai, dt, di, tick; unsigned calls; int used; } pair_ent;
static pair_ent pairs[PAIRS];

static void pair_remember(const uint64_t *r, uint32_t tick, unsigned calls) {
    uint32_t at = rd32(r[1]), ai = rd32(r[1] + 8), dt = rd32(r[2]), di = rd32(r[2] + 8);
    pair_ent *t = NULL, *oldest = &pairs[0];
    for (int i = 0; i < PAIRS; i++) {
        pair_ent *p = &pairs[i];
        if (p->used && p->game == r[0] && p->at == at && p->ai == ai && p->dt == dt && p->di == di) { t = p; break; }
        if (!p->used && !t) t = p;
        if ((int32_t)(p->tick - oldest->tick) < 0) oldest = p;
    }
    if (!t) t = oldest;
    *t = (pair_ent){ .game = r[0], .at = at, .ai = ai, .dt = dt, .di = di, .tick = tick, .calls = calls, .used = 1 };
}
static int pair_take(const uint64_t *r, uint32_t *tick, unsigned *calls) {
    for (int i = 0; i < PAIRS; i++) {
        pair_ent *p = &pairs[i];
        if (!p->used || p->game != r[0] || p->at != rd32(r[1]) || p->ai != rd32(r[1] + 8) || p->dt != rd32(r[2]) || p->di != rd32(r[2] + 8)) continue;
        p->used = 0; *tick = p->tick; *calls = p->calls; return 1;
    }
    return 0;
}

#ifndef DM_SPLASH_GAME_XP
static int player_xp(uint64_t u) { return ((int (*)(uint64_t, int, int))(uintptr_t)(base() + 0x5b9290))(u, 13, 0); }
#else
#define player_xp DM_SPLASH_GAME_XP
#endif
#ifndef DM_SPLASH_GAME_LIFE
static int mon_life(uint64_t u) { return ((int (*)(uint64_t, int, int))(uintptr_t)(base() + 0x5b9290))(u, 6, 0); }
#else
#define mon_life DM_SPLASH_GAME_LIFE
#endif
static void log_rec(const char *ev, const uint64_t *r) {
    uint64_t game = r[0], att = r[1], def = r[2], rec = r[3];
    uint64_t vec = rd64(rec + REC_VEC), cap = rd64(rec + REC_VEC_CAP);
    uint32_t n = rd32(rec + REC_VEC_N), e[12] = {0};
    if (vec) for (int i = 0; i < 12 && (uint32_t)i < 3 * n; i++) e[i] = rd32(vec + 12 * (uint64_t)(i / 3) + 4 * (i % 3));
    LOG("splash %s: game=%#llx tick=%u att=%u/%u/%u def=%u/%u/%u hitfl=%#x res=%#x phys=%d burn=%d fire=%d lightning=%d magic=%d cold=%d poison=%d total=%d leech=%d/%d vec=%#llx/%u/%#llx e0=%08x:%08x:%08x e1=%08x:%08x:%08x t158=%#llx",
        ev, (unsigned long long)game, rd32(game + 0x170),
        rd32(att), rd32(att + 4), rd32(att + 8), rd32(def), rd32(def + 4), rd32(def + 8),
        rd32(rec), rd32(rec + REC_RESFL) & 0xffff,
        (int)rd32(rec + REC_PHYS), (int)rd32(rec + REC_BURN), (int)rd32(rec + REC_FIRE), (int)rd32(rec + REC_LITNG), (int)rd32(rec + REC_MAGIC),
        (int)rd32(rec + REC_COLD), (int)rd32(rec + REC_POIS), (int)rd32(rec + REC_TOTAL), (int)rd32(rec + REC_LLEECH), (int)rd32(rec + REC_MLEECH),
        (unsigned long long)vec, n, (unsigned long long)cap, e[0], e[1], e[2], e[3], e[4], e[5], (unsigned long long)rd64(rec + REC_CLONE));
}
#endif


typedef struct {
    uint64_t game;
    uint32_t at, ai, dt, di, tick;
    uint8_t rec[0x180];
    uint32_t nvec; uint32_t vec[SPLASH_VEC_MAX][3];
    dm_splash_area area;
    int used;
} snap_ent;
static snap_ent g_snaps[SPLASH_SLOTS];

static int snap_same(const snap_ent *p, const uint64_t *r) {
    return p->used && p->game == r[0] && p->at == rd32(r[1]) && p->ai == rd32(r[1] + 8) && p->dt == rd32(r[2]) && p->di == rd32(r[2] + 8);
}
static void snap_drop(const uint64_t *r) { for (int i = 0; i < SPLASH_SLOTS; i++) if (snap_same(&g_snaps[i], r)) g_snaps[i].used = 0; }
static void snap_store(const uint64_t *r, const dm_splash_area *area) {
    uint64_t rec = r[3], game = r[0];
    uint64_t cap = rd64(rec + REC_VEC_CAP); uint32_t n = rd32(rec + REC_VEC_N);
    if (n > SPLASH_VEC_MAX || !(cap & (1ull << 63))) {
        static int once; if (once++ < 5) LOG("splash: hit not splashed, record vector holds %u entries (cap %#llx)", n, (unsigned long long)cap);
        snap_drop(r);
        return;
    }
    snap_ent *t = NULL, *oldest = &g_snaps[0];
    for (int i = 0; i < SPLASH_SLOTS; i++) {
        snap_ent *p = &g_snaps[i];
        if (p->used && p->game != game) p->used = 0;
        if (snap_same(p, r)) { t = p; break; }
        if (!p->used && !t) t = p;
        if ((int32_t)(p->tick - oldest->tick) < 0) oldest = p;
    }
    if (!t) t = oldest;
    memset(t, 0, sizeof *t);
    t->game = game; t->at = rd32(r[1]); t->ai = rd32(r[1] + 8); t->dt = rd32(r[2]); t->di = rd32(r[2] + 8);
    t->tick = rd32(game + 0x170); t->used = 1; t->area = *area;
    memcpy(t->rec, (const void *)(uintptr_t)rec, 0x40);
    memcpy(t->rec + 0x118, (const void *)(uintptr_t)(rec + 0x118), 0x40);
    t->nvec = n;
    for (uint32_t i = 0; i < n; i++) for (int k = 0; k < 3; k++) t->vec[i][k] = rd32(rd64(rec + REC_VEC) + 12 * (uint64_t)i + 4 * (uint64_t)k);
}
static int snap_take(const uint64_t *r, snap_ent *out) {
    uint32_t tick = rd32(r[0] + 0x170);
    for (int i = 0; i < SPLASH_SLOTS; i++) {
        snap_ent *p = &g_snaps[i];
        if (!p->used || p->game != r[0]) { p->used = 0; continue; }
        if (!snap_same(p, r)) continue;
        p->used = 0;
        if (tick - p->tick > SPLASH_TTL) return 0;
        *out = *p;
        return 1;
    }
    return 0;
}




static void build_rec(uint64_t buf, const snap_ent *s, int share) {
    uint8_t *b = (uint8_t *)(uintptr_t)buf;
    memset(b, 0, 0x180);
    memcpy(b, s->rec, 0x40);
    memcpy(b + 0x118, s->rec + 0x118, 0x40);
    *(uint64_t *)(b + REC_VEC) = buf + REC_VEC_AT;
    *(uint32_t *)(b + REC_VEC_N) = s->nvec;
    *(uint64_t *)(b + REC_VEC_CAP) = REC_INLINE_CAP;
    for (uint32_t i = 0; i < s->nvec; i++) {
        uint32_t e[3] = { s->vec[i][0], (uint32_t)((uint64_t)s->vec[i][1] * (uint64_t)share / 1000), s->vec[i][2] };
        memcpy(b + REC_VEC_AT + 12 * (size_t)i, e, 12);
    }
    *(uint64_t *)(b + REC_CLONE) = 1;
    *(uint32_t *)(b + REC_TOTAL) = 0;
    for (unsigned k = 0; k < sizeof REC_DMG_AT / sizeof *REC_DMG_AT; k++) {
        uint32_t v = *(uint32_t *)(b + REC_DMG_AT[k]);
        *(uint32_t *)(b + REC_DMG_AT[k]) = (uint32_t)((uint64_t)v * (uint64_t)share / 1000);
    }
    *(uint32_t *)(b + REC_LLEECH) >>= 1;
    *(uint32_t *)(b + REC_MLEECH) >>= 1;
    *(uint32_t *)(b + REC_HITFL) = 0x20;
    *(uint32_t *)(b + REC_RESFL) = 1 | (s->rec[REC_RESFL + 1] & 0x20 ? 0x2000 : 0);
}


typedef struct { uint32_t id, level; int ax, ay, px, py; dm_splash_area area; } splash_prim;

static int unit_xy(uint64_t u, int *x, int *y) {
    uint64_t p = rd64(u + 0x38); if (!p) return 0;
    *x = (int)*(volatile uint16_t *)(uintptr_t)(p + 2); *y = (int)*(volatile uint16_t *)(uintptr_t)(p + 6);
    return 1;
}

static int cone_share(int ax, int ay, int px, int py, int nx, int ny, const dm_splash_area *c) {
    return (int)(c->share * 10 * dm_splash_cone_factor(c, ax, ay, px, py, nx, ny));
}

#ifndef DM_SPLASH_GAME_GATE
static int may_damage(uint64_t game, uint64_t att, uint64_t u) {
    return ((int (*)(uint64_t, uint64_t, uint64_t))(uintptr_t)(base() + 0x7cff80))(game, att, u) != 0;
}
#else
#define may_damage DM_SPLASH_GAME_GATE
#endif
#ifndef DM_SPLASH_GAME_RESIST_ONE
static void resist_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf) {
    ((void (*)(uint64_t, uint64_t, uint64_t, uint64_t))(uintptr_t)(base() + 0x7d3c50))(game, att, u, buf);
}
#else
#define resist_one DM_SPLASH_GAME_RESIST_ONE
#endif
#ifndef DM_SPLASH_GAME_DAMAGE
static void damage_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf) {
    ((void (*)(uint64_t, uint64_t, uint64_t, uint32_t, uint64_t))(uintptr_t)(base() + 0x7d5bb0))(game, att, u, 0, buf);
}
#else
#define damage_one DM_SPLASH_GAME_DAMAGE
#endif
#ifndef DM_SPLASH_GAME_EXEC
static void exec_one(uint64_t game, uint64_t att, uint64_t u, uint64_t buf) {
    ((void (*)(uint64_t, uint64_t, uint64_t, uint64_t))(uintptr_t)(base() + 0x7d8880))(game, att, u, buf);
}
#else
#define exec_one DM_SPLASH_GAME_EXEC
#endif
static uint64_t find_monster(uint64_t game, uint32_t id) {
    for (int bkt = 0; bkt < 128; bkt++) for (uint64_t u = rd64(game + 0x2230 + 0x400 + 8 * (uint64_t)bkt); u; u = rd64(u + 0x158))
        if (rd32(u + 8) == id) return u;
    return 0;
}

static int target_share(uint64_t game, uint64_t att, uint64_t u, const splash_prim *p) {
    uint32_t mode = u ? rd32(u + 0xc) : 0;
    if (!u || mode == 0 || mode == 12 || owned(u) || rd32(u + 8) == p->id || (uint32_t)dm_unit_level(u) != p->level || p->level == 0) return 0;
    int x, y, share;
    if (!unit_xy(u, &x, &y) || !(share = cone_share(p->ax, p->ay, p->px, p->py, x, y, &p->area)) || !may_damage(game, att, u)) return 0;
    return share;
}

static void splash_run(uint64_t game, uint64_t att, const splash_prim *p, const snap_ent *s) {
    int share = s->area.share;
    uint32_t ids[SPLASH_TARGETS]; int n = 0, full = 0;
#ifndef DM_PLAYER_BUILD
    if (obs_on()) LOG("splash prim: id=%u lvl=%d att=%d,%d prim=%d,%d share=%d cone=%d/%d", p->id, (int)p->level, p->ax, p->ay, p->px, p->py, share,
                      p->area.width, p->area.depth);
#endif
    for (int bkt = 0; bkt < 128 && !full; bkt++) for (uint64_t u = rd64(game + 0x2230 + 0x400 + 8 * (uint64_t)bkt); u; u = rd64(u + 0x158)) {
        if (rd32(u) != 1) continue;
        if (!target_share(game, att, u, p)) {
#ifndef DM_PLAYER_BUILD
            if (obs_on() && rd32(u + 8) != p->id) {
                int x = 0, y = 0; unit_xy(u, &x, &y);
                int ddx = x - (int)p->px, ddy = y - (int)p->py;
                if (ddx * ddx + ddy * ddy <= 256)
                    LOG("splash cand: cls=%u id=%u mode=%u own=%d lvl=%d/%d xy=%d,%d cone=%d gate=%d",
                        rd32(u + 4), rd32(u + 8), rd32(u + 0xc), owned(u), (int)dm_unit_level(u), (int)p->level, x, y,
                        cone_share(p->ax, p->ay, p->px, p->py, x, y, &p->area), may_damage(game, att, u) != 0);
            }
#endif
            continue;
        }
        if (n == SPLASH_TARGETS) { full = 1; static int once; if (once++ < 5) LOG("splash: more than %d eligible neighbours, the rest skipped", SPLASH_TARGETS); break; }
        ids[n++] = rd32(u + 8);
    }
    if (!n) return;
    g_depth = 1;
    for (int k = 0; k < n; k++) {
        uint64_t u = find_monster(game, ids[k]);
        int eff = target_share(game, att, u, p);
        if (!eff) continue;
        uint64_t bufstore[0x180 / 8];
        uint64_t buf = (uint64_t)(uintptr_t)bufstore;
        build_rec(buf, s, eff);
#ifndef DM_PLAYER_BUILD
        int life0 = 0, f0 = 0, xp0 = 0, tot1 = 0, tx = 0, ty = 0;
        uint32_t tcls = 0, tid = 0, pre[sizeof REC_DMG_AT / sizeof *REC_DMG_AT];
        memset(pre, 0, sizeof pre);
        if (obs_on()) {
            tcls = rd32(u + 4); tid = rd32(u + 8); unit_xy(u, &tx, &ty);
            life0 = mon_life(u); xp0 = player_xp(att);
            for (unsigned j = 0; j < sizeof REC_DMG_AT / sizeof *REC_DMG_AT; j++) pre[j] = rd32(buf + REC_DMG_AT[j]);
            f0 = (int)dm_dmgnum_gate_calls();
        }
#endif
        resist_one(game, att, u, buf);
#ifndef DM_PLAYER_BUILD
        if (obs_on()) tot1 = (int)rd32(buf + REC_TOTAL);
#endif
        damage_one(game, att, u, buf);
#ifndef DM_PLAYER_BUILD
        int life1 = 0, f1 = 0;
        if (obs_on()) { life1 = mon_life(u); f1 = (int)dm_dmgnum_gate_calls() - f0; }
#endif
        exec_one(game, att, u, buf);
#ifndef DM_PLAYER_BUILD
        if (obs_on()) {
            static const char *const nm[] = { "phys", "burn", "fire", "litng", "magic", "cold", "pois" };
            char parts[256]; parts[0] = 0;
            for (unsigned j = 0; j < sizeof REC_DMG_AT / sizeof *REC_DMG_AT; j++)
                if (pre[j]) snprintf(parts + strlen(parts), sizeof parts - strlen(parts), " %s=%d>%d", nm[j], (int)pre[j], (int)rd32(buf + REC_DMG_AT[j]));
            LOG("splash hit: tick=%u target=%u/%u share=%d eff=%d xy=%d,%d vec=%u%s total=%d life=%d->%d d0f=%d res=0x%x xp=%d->%d",
                rd32(game + 0x170), tcls, tid, share, eff, tx, ty, s->nvec, parts,
                tot1, life0, life1, f1, rd32(buf + REC_RESFL) & 0xffff, xp0, player_xp(att));
        }
#endif
    }
    g_depth = 0;
}





#define SKILL_REC_SRVDO 0x4e
#define SKILL_REC_FLAGS 0x25
#define SKILL_KICK 0x02
#define ITEM_REC_TYPE 0x12e
#define ITEM_REC_RANGE 0x115
#define SIM_LIST_IDLE 0x40000000u

typedef struct {
    int skill, kick, srvdo;
    struct { int weapon, idle, range; char type[5]; } hand[2];
    int two_hands;
    int cls, str, dex;
} hit_facts;

static void assume_once(const char *what, const char *msg, int value) {
    static int said; if (said++ < 8) LOG("ASSUME splash.%s: %s (%d)", what, msg, value);
}

static void area_from_facts(const hit_facts *f, dm_splash_area *a) {
    *a = (dm_splash_area){ 0, 0, 0 };
    if (f->skill < 0) { assume_once("skill", "the executing skill of a melee hit is not readable", f->skill); return; }
    if (dm_splash_skill_excluded(f->kick, f->srvdo)) return;
    int w = -1, n = f->hand[0].weapon + f->hand[1].weapon;
    if (n == 1) w = f->hand[0].weapon ? 0 : 1;
    else if (n == 2 && f->hand[0].idle != f->hand[1].idle) w = f->hand[0].idle ? 1 : 0;
    else if (n == 2) { assume_once("hand", "two weapons and not one swinging hand", f->hand[0].idle); return; }
    if (w < 0) return;
    dm_splash_group g = dm_splash_group_of(f->hand[w].type, f->two_hands);
    if (g == DM_SPLASH_UNKNOWN) { assume_once("weapon_type", "a weapon type the rule does not know", f->hand[w].type[0]); return; }
    *a = dm_splash_area_of(g, f->cls, f->str, f->dex, f->hand[w].range);
}

static uint64_t data_table(uint64_t u) {
    uint8_t ver = *(volatile uint8_t *)(uintptr_t)(u + 0x1bd);
    return ver >= 1 && ver <= 3 ? rd64((uint64_t)(uintptr_t)(base() + 0x2d64a18 + (uint64_t)ver * 16)) : 0;
}
int dm_splash_weapon_type(uint64_t item, char type[5], int *range) {
    memset(type, 0, 5); *range = 0;
    uint64_t t = item && rd32(item) == 4 ? data_table(item) : 0; uint32_t ci = item ? rd32(item + 4) : 0;
    if (!t || ci >= rd32(t + 0x15c8)) return -1;
    uint64_t rec = rd64(t + 0x15c0) + (uint64_t)ci * 0x1c0;
    int16_t ty = *(volatile int16_t *)(uintptr_t)(rec + ITEM_REC_TYPE);
    if (ty < 0 || (uint64_t)ty >= rd64(t + 0x1370)) return -1;
    memcpy(type, (const void *)(uintptr_t)(rd64(t + 0x1368) + (uint64_t)ty * 0xe8), 4);
    for (int k = 3; k >= 0 && (type[k] == ' ' || !type[k]); k--) type[k] = 0;
    *range = *(volatile uint8_t *)(uintptr_t)(rec + ITEM_REC_RANGE);
    return 0;
}
#ifndef DM_SPLASH_GAME_FACTS
static int hit_facts_read(uint64_t att, hit_facts *f) {
    uint8_t *b = base();
    memset(f, 0, sizeof *f); f->skill = -1;
    uint64_t sl = rd64(att + 0x100), node = sl ? rd64(sl + 0x18) : 0, row = node ? rd64(node) : 0;
    if (row) {
        f->skill = *(volatile int16_t *)(uintptr_t)row; f->srvdo = *(volatile uint16_t *)(uintptr_t)(row + SKILL_REC_SRVDO);
        f->kick = (*(volatile uint8_t *)(uintptr_t)(row + SKILL_REC_FLAGS) & SKILL_KICK) != 0;
    }
    uint64_t inv = rd64(att + 0x90), hands[2] = { 0, 0 };
    for (uint64_t it = inv && rd32(inv) == 0x01020304 ? rd64(inv + 0x10) : 0, d; it && (d = rd64(it + 0x10)); it = rd64(d + 0xb0)) {
        int slot = *(volatile uint8_t *)(uintptr_t)(d + 0x54);
        if (rd32(it + 0xc) == 1 && (slot == 4 || slot == 5)) hands[slot - 4] = it;
    }
    for (int h = 0; h < 2; h++) {
        uint64_t it = hands[h];
        if (!it || !((int (*)(uint64_t, int))(uintptr_t)(b + 0x574bc0))(it, 45)) continue;
        f->hand[h].weapon = 1;
        uint64_t l = rd64(it + 0x88); f->hand[h].idle = l && (rd32(l + 0x1c) & SIM_LIST_IDLE);
        if (dm_splash_weapon_type(it, f->hand[h].type, &f->hand[h].range)) f->hand[h].type[0] = 0;
    }
    f->two_hands = inv && ((int (*)(uint64_t, uint64_t, int))(uintptr_t)(b + 0x560d20))(att, inv, 0) == 2;
    int (*stat)(uint64_t, int, int) = (int (*)(uint64_t, int, int))(uintptr_t)(b + 0x5b9290);
    f->cls = (int)rd32(att + 4); f->str = stat(att, 0, 0); f->dex = stat(att, 2, 0);
    return 0;
}
#else
static int DM_SPLASH_GAME_FACTS(uint64_t att, hit_facts *f);
#define hit_facts_read DM_SPLASH_GAME_FACTS
#endif


void svc_melee_resist(uint64_t *regs) {
    if (!live(regs)) { call_resist(regs); return; }
#ifndef DM_PLAYER_BUILD
    if (obs_on()) { log_rec("A-pre", regs); if (!g_depth) pair_remember(regs, rd32(regs[0] + 0x170), dm_dmgnum_gate_calls()); }
#endif
    if (!g_depth && splash_on() && rd32(regs[1]) == 0 && !owned(regs[1]) && rd32(regs[2]) == 1) {
        hit_facts f; dm_splash_area a = { 0, 0, 0 };
        if (!hit_facts_read(regs[1], &f)) area_from_facts(&f, &a);
#ifndef DM_PLAYER_BUILD
        if (obs_on()) LOG("splash facts: skill=%d kick=%d func=%d hands=%s:%d:%d/%s:%d:%d two=%d cls=%d str=%d dex=%d -> share=%d cone=%d/%d", f.skill, f.kick,
                          f.srvdo, f.hand[0].type, f.hand[0].weapon, f.hand[0].idle, f.hand[1].type, f.hand[1].weapon, f.hand[1].idle, f.two_hands, f.cls,
                          f.str, f.dex, a.share, a.width, a.depth);
#endif
        if (a.share) snap_store(regs, &a); else snap_drop(regs);
    }
    call_resist(regs);
#ifndef DM_PLAYER_BUILD
    if (obs_on()) log_rec("A-post", regs);
#endif
}

void svc_melee_result(uint64_t *regs) {
    snap_ent s; splash_prim p; int run = 0;
    if (live(regs) && !g_depth && rd32(regs[1]) == 0 && !owned(regs[1]) && rd32(regs[2]) == 1) {
        int have = snap_take(regs, &s);
        if (have && (rd32(regs[3] + REC_RESFL) & 1)) {
            uint64_t def = regs[2];
            p.id = rd32(def + 8); p.level = (uint32_t)dm_unit_level(def); p.area = s.area;
            run = splash_on() && s.area.share && unit_xy(regs[1], &p.ax, &p.ay) && unit_xy(def, &p.px, &p.py);
        }
    }
#ifndef DM_PLAYER_BUILD
    if (obs_on() && live(regs)) {
        log_rec("B", regs);
        uint32_t atick = 0; unsigned acalls = 0;
        int have = pair_take(regs, &atick, &acalls);
        LOG("splash B-pair: tickA=%u frames=%d f0=%d paired=%d", atick, have ? (int)(rd32(regs[0] + 0x170) - atick) : -1,
            have ? (int)(dm_dmgnum_gate_calls() - acalls) : -1, have);
    }
#endif
    call_result(regs);
    if (run) splash_run(regs[0], regs[1], &p, &s);
}
