

#include "dm_fightlog.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum { M_KILLED = 1, M_DEAD = 2, M_GONE = 4, M_DOT = 8, M_OURS = 16 };
enum { HASH = 2 * DM_FL_MEMBERS, COOL = 64 };
typedef struct { uint32_t id; int16_t slot; uint8_t group, flags; } member;



typedef struct { uint32_t id; uint16_t member; int32_t life, max, read_life; int64_t direct; int32_t atk; int16_t pskill, bskill; } slot;
typedef struct { int64_t lo, hi, last; uint8_t lo8[2], hi8[2], last8[2], me, en, skill; } wbucket;

struct dm_fl_state {
    pthread_mutex_t mu;
    dm_fl_fight ring[DM_FL_RING];
    int head, count;
    dm_fl_fight pend; int has_pend;
    char who[64];
    uint32_t gen, saved;
    uint32_t build, state_key;

    uint32_t tick, game_tick0; dm_fl_player p; int have_player, was_dead, last_tick_hp;
    struct { uint32_t t; uint8_t bits; int16_t skill; } pre[DM_FL_PREROLL]; unsigned npre;
    uint32_t states[DM_FL_STATE_WORDS], fight_states[DM_FL_STATE_WORDS];
    struct { uint32_t id, t; } cool[COOL]; unsigned ncool;

    int open, area, hits_lost, norecord;
    uint32_t kills[DM_FL_MEMBERS]; int nkills;
    int nhits;
    uint32_t start, t0, last_damage, last_end;
    int won; uint32_t won_t, kill_t;
    int took; uint32_t took_kills;
    dm_fl_fight wonf;
    dm_fl_fight f;
    member mem[DM_FL_MEMBERS]; int nmem; uint16_t hash[HASH];
    slot sl[DM_FL_TRACK];
    int64_t lost, life_total, glost[DM_FL_GROUPS], glife[DM_FL_GROUPS], gdealt[DM_FL_GROUPS], dealt[3], taken;
    int alive, galive[DM_FL_GROUPS], last_hp, me, en, skill;
    uint8_t gelite[DM_FL_GROUPS]; int elites_folded;
    int64_t ploss, phits;



    int64_t mine, merc, rest, unknown, sk[DM_FL_SKILLS], sknone, skover, gmine[DM_FL_GROUPS], gact[DM_FL_GROUPS];
    struct { uint16_t cls; int64_t dealt; } comp[DM_FL_COMPS]; int ncomp;
    uint32_t t_act, act_t; int act_skill, have_act; int64_t gap;
    struct { uint32_t id; int64_t at; } eng[32]; unsigned neng;
    wbucket wb[DM_FL_BUCKETS]; int64_t wg[DM_FL_BUCKETS][DM_FL_GROUPS]; int nb, width;
    dm_fl_fight last;
};

static dm_fl_state g_core = { .mu = PTHREAD_MUTEX_INITIALIZER };
dm_fl_state *dm_fl_core(void) { return &g_core; }

static uint16_t sat16(int64_t v) { return v < 0 ? 0 : v > 0xffff ? 0xffff : (uint16_t)v; }
static uint8_t frac8(int64_t v, int64_t whole) { if (whole <= 0) return 0; int64_t r = v * 255 / whole; return r < 0 ? 0 : r > 255 ? 255 : (uint8_t)r; }
static uint8_t pct8(int v, int whole) { return frac8(v, whole); }

void dm_fl_reset(dm_fl_state *s) {
    pthread_mutex_lock(&s->mu);
    s->head = s->count = 0; s->who[0] = 0; s->has_pend = 0; s->saved = ++s->gen;
    pthread_mutex_unlock(&s->mu);
    s->open = 0; s->npre = 0; s->ncool = 0;
}
void dm_fl_build(dm_fl_state *s, uint32_t key, uint32_t state) { s->build = key; s->state_key = state; }
int dm_fl_open(const dm_fl_state *s) { return s->open; }
int dm_fl_flags(const dm_fl_state *s) { return s->open ? s->f.h.flags : 0; }
void dm_fl_discard(dm_fl_state *s) {
    s->open = 0;
    pthread_mutex_lock(&s->mu); if (s->has_pend) { s->has_pend = 0; s->gen++; } pthread_mutex_unlock(&s->mu);
}


static unsigned hslot(uint32_t id) { return (id * 2654435761u) % HASH; }
static int find(const dm_fl_state *s, uint32_t id) {
    for (unsigned h = hslot(id), k = 0; k < HASH; k++, h = (h + 1) % HASH) { int m = s->hash[h]; if (!m) return -1; if (s->mem[m - 1].id == id) return m - 1; }
    return -1;
}
int dm_fl_known(const dm_fl_state *s, uint32_t id) { return s->open && find(s, id) >= 0; }
static int elite(uint8_t rank, int boss) { return rank == DM_FL_RANK_UNIQUE || rank == DM_FL_RANK_SUPER || boss; }

static void regroup(dm_fl_state *s, int from, int to) {
    for (int m = 0; m < s->nmem; m++) if (s->mem[m].group == from) s->mem[m].group = (uint8_t)to;
    for (int i = 0; i < s->f.h.nmarks; i++) if (s->f.m[i].kind == DM_FL_MARK_KILL && s->f.m[i].a == from) s->f.m[i].a = (uint8_t)to;
}
#define SWAP(T, x, y) do { T t_ = (x); (x) = (y); (y) = t_; } while (0)

static void swap_groups(dm_fl_state *s, int a, int b) {
    SWAP(dm_fl_group, s->f.g[a], s->f.g[b]); SWAP(int64_t, s->glost[a], s->glost[b]); SWAP(int64_t, s->glife[a], s->glife[b]);
    SWAP(int64_t, s->gdealt[a], s->gdealt[b]); SWAP(int64_t, s->gmine[a], s->gmine[b]); SWAP(int64_t, s->gact[a], s->gact[b]); SWAP(int, s->galive[a], s->galive[b]); SWAP(uint8_t, s->gelite[a], s->gelite[b]);
    for (int k = 0; k < s->nb; k++) SWAP(int64_t, s->wg[k][a], s->wg[k][b]);
    regroup(s, a, DM_FL_GROUPS); regroup(s, b, a); regroup(s, DM_FL_GROUPS, b);
}
#undef SWAP

static void fold(dm_fl_state *s, int o, int into) {
    dm_fl_group *x = &s->f.g[into], *y = &s->f.g[o];
    if (y->t_first < x->t_first) x->t_first = y->t_first;
    x->joined += y->joined; x->kills += y->kills; x->tracked += y->tracked;
    x->t_done = s->galive[into] || s->galive[o] ? 0 : x->t_done > y->t_done ? x->t_done : y->t_done;
    s->glost[into] += s->glost[o]; s->glife[into] += s->glife[o]; s->gdealt[into] += s->gdealt[o]; s->gmine[into] += s->gmine[o]; s->galive[into] += s->galive[o];
    s->glost[o] = s->glife[o] = s->gdealt[o] = s->gmine[o] = 0; s->galive[o] = 0;
    { int64_t a = s->gact[into], b = s->gact[o]; s->gact[into] = a == -2 || b == -2 ? -2 : a < 0 ? b : b < 0 || a < b ? a : b; s->gact[o] = -1; }
    for (int k = 0; k < s->nb; k++) { s->wg[k][into] += s->wg[k][o]; s->wg[k][o] = 0; }
    regroup(s, o, into);
}



static int group_of(dm_fl_state *s, uint32_t cls, uint8_t rank, uint16_t su, int boss) {
    dm_fl_head *h = &s->f.h; const int last = DM_FL_GROUPS - 1;
    for (int g = 0; g < h->ngroups; g++) { dm_fl_group *x = &s->f.g[g]; if (x->cls == cls && x->rank == rank && x->su == su) return g; }
    dm_fl_group fresh = { .cls = (uint16_t)cls, .su = su, .rank = rank };
    if (h->ngroups < DM_FL_GROUPS) { int g = h->ngroups++; s->f.g[g] = fresh; s->gelite[g] = (uint8_t)elite(rank, boss); return g; }
    if (!(s->f.g[last].flags & DM_FL_G_OTHERS)) {
        if (s->gelite[last]) for (int g = last - 1; g >= 0; g--) if (!s->gelite[g]) { swap_groups(s, g, last); break; }
        h->flags |= DM_FL_F_MORE_KINDS; s->f.g[last].flags |= DM_FL_G_OTHERS;
    }
    if (elite(rank, boss)) {
        for (int g = last - 1; g >= 0; g--) if (!s->gelite[g]) { fold(s, g, last); s->f.g[g] = fresh; s->gelite[g] = 1; return g; }
        s->elites_folded = 1;
    }
    return last;
}
static uint16_t ticks_in(const dm_fl_state *s) { return sat16((int64_t)(s->tick - s->t0)); }
static int free_slot(dm_fl_state *s) {
    int reuse = -1;
    for (int i = 0; i < DM_FL_TRACK; i++) {
        if (!s->sl[i].id) return i;
        if (reuse < 0 && (s->mem[s->sl[i].member].flags & (M_DEAD | M_GONE))) reuse = i;
    }
    if (reuse >= 0) s->mem[s->sl[reuse].member].slot = -1;
    return reuse;
}
static void taking(dm_fl_state *s);

static int join(dm_fl_state *s, uint32_t id, uint32_t cls, uint8_t rank, uint16_t su, int boss, int life, int max) {
    if (s->nmem >= DM_FL_MEMBERS) { s->f.h.flags |= DM_FL_F_MEMBERS_FULL; return -1; }
    taking(s);
    int m = s->nmem++, g = group_of(s, cls, rank, su, boss);
    s->mem[m] = (member){ id, -1, (uint8_t)g, 0 };
    for (unsigned h = hslot(id);; h = (h + 1) % HASH) if (!s->hash[h]) { s->hash[h] = (uint16_t)(m + 1); break; }
    dm_fl_group *x = &s->f.g[g];
    if (!x->joined) x->t_first = ticks_in(s);
    x->joined++; x->t_done = 0; s->galive[g]++; s->alive++;
    for (unsigned k = 0; s->gact[g] == -1 && k < s->neng && k < 32; k++) if (s->eng[k].id == id) s->gact[g] = s->eng[k].at;
    if (max <= 0) max = life;
    if (life > max) life = max;
    int i = free_slot(s);
    if (i < 0) { s->f.h.flags |= DM_FL_F_TABLE_FULL; return m; }
    s->sl[i] = (slot){ id, (uint16_t)m, life, max, life, 0, -1, -1, -1 };
    s->mem[m].slot = (int16_t)i; x->tracked++;
    s->life_total += max; s->glife[g] += max;
    s->lost += max - life; s->glost[g] += max - life;
    return m;
}
static void mark(dm_fl_state *s, uint8_t kind, uint8_t a, uint16_t b) {
    dm_fl_head *h = &s->f.h;
    if (h->nmarks >= DM_FL_MARKS) { h->flags |= DM_FL_F_MARKS_FULL; return; }
    s->f.m[h->nmarks++] = (dm_fl_mark){ ticks_in(s), kind, a, b };
}
static void died(dm_fl_state *s, int m) {
    member *x = &s->mem[m]; if (x->flags & M_DEAD) return;
    s->kill_t = s->tick;
    if (!(x->flags & M_GONE)) { s->alive--; s->galive[x->group]--; }
    x->flags = (uint8_t)((x->flags | M_DEAD) & ~M_GONE);
    if (!(x->flags & M_KILLED)) { x->flags |= M_KILLED; s->f.g[x->group].kills++; s->f.h.kills++; }
    if ((x->flags & M_OURS) && s->nkills < DM_FL_MEMBERS) s->kills[s->nkills++] = x->id;
    x->flags &= (uint8_t)~M_OURS;
    if (!s->galive[x->group]) { s->f.g[x->group].t_done = ticks_in(s) ? ticks_in(s) : 1; mark(s, DM_FL_MARK_KILL, x->group, 0); }
}
static void revived(dm_fl_state *s, int m) {
    member *x = &s->mem[m]; if (!(x->flags & (M_DEAD | M_GONE))) return;
    taking(s);
    x->flags &= (uint8_t)~(M_DEAD | M_GONE); s->alive++; s->galive[x->group]++; s->f.g[x->group].t_done = 0;
}
static void set_life(dm_fl_state *s, int i, int life) {
    slot *t = &s->sl[i]; if (life < 0) life = 0; if (life > t->max) life = t->max;
    int64_t d = (int64_t)t->life - life; t->life = life;
    s->lost += d; s->glost[s->mem[t->member].group] += d;
}


static void merge(dm_fl_state *s) {
    int n = 0;
    for (int j = 0; j < s->nb; j += 2) {
        wbucket a = s->wb[j]; int64_t ga[DM_FL_GROUPS]; memcpy(ga, s->wg[j], sizeof ga);
        if (j + 1 < s->nb) {
            const wbucket *b = &s->wb[j + 1];
            if (b->lo < a.lo) a.lo = b->lo;
            if (b->hi > a.hi) a.hi = b->hi;
            a.last = b->last;
            for (int k = 0; k < 2; k++) { if (b->lo8[k] < a.lo8[k]) a.lo8[k] = b->lo8[k]; if (b->hi8[k] > a.hi8[k]) a.hi8[k] = b->hi8[k]; a.last8[k] = b->last8[k]; }
            a.me |= b->me; a.en |= b->en; if (b->skill) a.skill = b->skill;
            memcpy(ga, s->wg[j + 1], sizeof ga);
        }
        s->wb[n] = a; memcpy(s->wg[n], ga, sizeof ga); n++;
    }
    s->nb = n; s->width *= 2;
}
static void put(dm_fl_state *s, uint32_t t, int me, int en, int skill) {
    while ((t - s->t0) / (uint32_t)s->width >= DM_FL_BUCKETS) merge(s);
    int k = (int)((t - s->t0) / (uint32_t)s->width);
    uint8_t v8[2] = { pct8(s->p.hp, s->p.hp_max), pct8(s->p.mp, s->p.mp_max) };
    for (; s->nb <= k; s->nb++) {
        wbucket *b = &s->wb[s->nb]; *b = (wbucket){ s->lost, s->lost, s->lost, { v8[0], v8[1] }, { v8[0], v8[1] }, { v8[0], v8[1] }, 0, 0, 0 };
        memcpy(s->wg[s->nb], s->glost, sizeof s->glost);
    }
    wbucket *b = &s->wb[k];
    if (s->lost < b->lo) b->lo = s->lost;
    if (s->lost > b->hi) b->hi = s->lost;
    b->last = s->lost;
    for (int j = 0; j < 2; j++) { if (v8[j] < b->lo8[j]) b->lo8[j] = v8[j]; if (v8[j] > b->hi8[j]) b->hi8[j] = v8[j]; b->last8[j] = v8[j]; }
    b->me |= (uint8_t)me; b->en |= (uint8_t)en; if (skill) b->skill = (uint8_t)skill;
    memcpy(s->wg[k], s->glost, sizeof s->glost);
}
static int skill_index(dm_fl_state *s, int skill) {
    if (skill < 0) return 0;
    dm_fl_head *h = &s->f.h;
    for (int i = 0; i < DM_FL_SKILLS; i++) { if (h->skill[i] == (uint16_t)(skill + 1)) return i + 1; if (!h->skill[i]) { h->skill[i] = (uint16_t)(skill + 1); return i + 1; } }
    return 0;
}


static int can_start(const dm_fl_state *s) { return s->have_player && !s->p.dead && !s->p.town; }

static int can_start_hit(const dm_fl_state *s) { return s->have_player && !s->p.town && (!s->p.dead || !s->was_dead); }


enum { REACH = 10 * DM_FL_TPS };
static void start(dm_fl_state *s) {
    uint32_t t = s->tick, lead = t - s->game_tick0 < DM_FL_LEAD ? t - s->game_tick0 : DM_FL_LEAD, from = t - lead, floor = s->last_end > s->game_tick0 ? s->last_end : s->game_tick0;
    if (t - floor > REACH) floor = t - REACH;
    for (unsigned i = 0; i < DM_FL_PREROLL && i < s->npre; i++) { uint32_t a = s->pre[i].t; if (a >= floor && a < from) from = a; }
    for (unsigned k = 0; k < s->neng && k < 32; k++) if (s->eng[k].at >= (int64_t)floor && s->eng[k].at < (int64_t)from) from = (uint32_t)s->eng[k].at;
    lead = t - from;
    s->open = 1; s->won = s->took = 0; s->start = t; s->t0 = t - lead; s->last_damage = t; s->area = s->p.area; s->hits_lost = 0;
    memset(&s->f.h, 0, sizeof s->f.h); memset(s->f.g, 0, sizeof s->f.g);
    s->f.h.lead = (uint16_t)lead; s->f.h.area = sat16(s->p.area); s->f.h.difficulty = (uint8_t)s->p.difficulty; s->f.h.min_life = 255;
    s->nmem = 0; memset(s->hash, 0, sizeof s->hash); memset(s->sl, 0, sizeof s->sl);
    s->lost = s->life_total = s->taken = s->ploss = s->phits = 0;
    memset(s->glost, 0, sizeof s->glost); memset(s->glife, 0, sizeof s->glife); memset(s->gdealt, 0, sizeof s->gdealt); memset(s->dealt, 0, sizeof s->dealt);
    s->mine = s->merc = s->rest = s->unknown = s->sknone = s->skover = 0; memset(s->sk, 0, sizeof s->sk); memset(s->gmine, 0, sizeof s->gmine); s->ncomp = 0; s->t_act = 0;
    for (int g = 0; g < DM_FL_GROUPS; g++) s->gact[g] = -1;
    s->alive = 0; memset(s->galive, 0, sizeof s->galive); memset(s->gelite, 0, sizeof s->gelite); s->elites_folded = 0; s->last_hp = s->last_tick_hp;
    s->nb = 0; s->width = 1;
    for (uint32_t u = s->t0; u < t; u++) put(s, u, 0, 0, 0);
    for (unsigned i = 0; i < DM_FL_PREROLL && i < s->npre; i++) {
        unsigned k = (s->npre - 1 - i) % DM_FL_PREROLL; if (s->pre[k].t - s->t0 >= lead) continue;
        put(s, s->pre[k].t, s->pre[k].bits, 0, skill_index(s, s->pre[k].skill));
        if (!s->t_act || s->pre[k].t < s->t_act) s->t_act = s->pre[k].t;
    }
    memcpy(s->fight_states, s->states, sizeof s->states);
}
static void cool(dm_fl_state *s, uint32_t id, uint32_t t) { unsigned k = s->ncool++ % COOL; s->cool[k].id = id; s->cool[k].t = t; }
static int cooling(const dm_fl_state *s, uint32_t id) {
    for (unsigned i = 0; i < COOL && i < s->ncool; i++) if (s->cool[i].id == id && s->tick - s->cool[i].t < DM_FL_PAUSE) return 1;
    return 0;
}

static void fight_record(dm_fl_state *s, dm_fl_fight *f, int kind, uint32_t end) {
    if (f != &s->f) *f = s->f;
    dm_fl_head *h = &f->h;
    int nb = (int)((end - s->t0) / (uint32_t)s->width) + 1; if (nb > s->nb) nb = s->nb;
    h->end = (uint8_t)kind; h->duration = end - s->start; h->width = (uint16_t)s->width; h->nbuckets = (uint16_t)nb;
    h->enemies = (uint16_t)s->nmem; h->build = s->build;
    for (int k = 0; k < 3; k++) h->dealt[k] = (uint32_t)(s->dealt[k] >> 8);
    h->taken = (uint32_t)(s->taken >> 8); h->life_total = (uint32_t)(s->life_total >> 8);
    if (s->hits_lost) h->flags |= DM_FL_F_HITS_LOST;
    for (int k = 0; k < nb; k++) {
        const wbucket *w = &s->wb[k]; dm_fl_bucket *b = &f->b[k];
        b->lo[DM_FL_S_ENEMY] = frac8(s->life_total - w->hi, s->life_total); b->hi[DM_FL_S_ENEMY] = frac8(s->life_total - w->lo, s->life_total); b->last[DM_FL_S_ENEMY] = frac8(s->life_total - w->last, s->life_total);
        for (int j = 0; j < 2; j++) { b->lo[1 + j] = w->lo8[j]; b->hi[1 + j] = w->hi8[j]; b->last[1 + j] = w->last8[j]; }
        b->me = w->me; b->en = w->en; b->skill = w->skill;
        for (int g = 0; g < DM_FL_GROUPS; g++) f->glife[k][g] = h->ngroups >= 2 && g < h->ngroups ? frac8(s->glife[g] - s->wg[k][g], s->glife[g]) : 0;
    }
    for (int g = 0; g < h->ngroups; g++) { f->g[g].dealt = (uint32_t)(s->gdealt[g] >> 8); f->g[g].life = (uint32_t)(s->glife[g] >> 8); }
    dm_fl_split *x = &f->s; memset(x, 0, sizeof *x);
    x->mine = (uint32_t)(s->mine >> 8); x->merc = (uint32_t)(s->merc >> 8); x->rest = (uint32_t)(s->rest >> 8); x->unknown = (uint32_t)(s->unknown >> 8);
    for (int k = 0; k < DM_FL_SKILLS; k++) x->skill[k] = (uint32_t)(s->sk[k] >> 8);
    x->none = (uint32_t)(s->sknone >> 8); x->over = (uint32_t)(s->skover >> 8);
    for (int k = 0; k < s->ncomp; k++) { x->comp[k].cls = s->comp[k].cls; x->comp[k].dealt = (uint32_t)(s->comp[k].dealt >> 8); }
    for (int g = 0; g < DM_FL_GROUPS; g++) x->gact[g] = g < h->ngroups && s->gact[g] >= (int64_t)s->t0 ? sat16(s->gact[g] - s->t0) : 0xffff;
    for (int g = 0; g < h->ngroups; g++) x->gmine[g] = (uint32_t)(s->gmine[g] >> 8);
    x->state = s->state_key; x->t_act = s->t_act && s->t_act >= s->t0 ? sat16((int64_t)(s->t_act - s->t0)) : 0xffff; x->flags = DM_FL_SPLIT_KNOWN;
    int64_t parts = s->mine + s->merc + s->rest + s->unknown;
    for (int k = 0; k < s->ncomp; k++) parts += s->comp[k].dealt;
    s->gap = parts - (s->dealt[0] + s->dealt[1] + s->dealt[2]);
    size_t used = offsetof(dm_fl_fight, b) + (size_t)nb * sizeof(dm_fl_bucket);
    memset((uint8_t *)f + used, 0, offsetof(dm_fl_fight, glife) - used);
    memset(f->glife[nb], 0, sizeof f->glife - (size_t)nb * sizeof f->glife[0]);
    memset(&f->m[h->nmarks], 0, sizeof f->m - h->nmarks * sizeof f->m[0]);
}


static void taking(dm_fl_state *s) {
    if (!s->won || s->alive) return;
    s->took = 1; s->took_kills = s->f.h.kills;
}
static int finish(dm_fl_state *s, int kind, uint32_t end) {


    int back = s->took && kind != DM_FL_END_KILL && kind != DM_FL_END_DEATH && s->f.h.kills == s->took_kills;

    dm_fl_fight *f = kind == DM_FL_END_KILL || back ? &s->wonf : &s->f;
    if (f == &s->f) fight_record(s, f, kind, end);
    for (int m = 0; m < s->nmem; m++) if (!(s->mem[m].flags & M_DEAD)) cool(s, s->mem[m].id, end);
    int kept = !s->norecord && (f->h.end == DM_FL_END_KILL || f->h.end == DM_FL_END_DEATH || f->h.duration >= DM_FL_MIN || s->took);
    pthread_mutex_lock(&s->mu);
    if (kept) { s->ring[s->head] = *f; s->head = (s->head + 1) % DM_FL_RING; if (s->count < DM_FL_RING) s->count++; s->gen++; }
    if (s->has_pend) { s->has_pend = 0; s->gen++; }
    pthread_mutex_unlock(&s->mu);
    s->last = *f; s->open = 0; s->neng = 0; s->last_end = s->tick;
    return kept;
}
int dm_fl_game(dm_fl_state *s, uint32_t tick) {
    int kept = 0;
    if (s->open) kept = s->won ? finish(s, DM_FL_END_KILL, s->won_t) : finish(s, DM_FL_END_LEFT, s->tick);
    s->game_tick0 = tick; s->tick = tick; s->npre = 0; s->ncool = 0; s->have_player = 0; s->have_act = 0; s->neng = 0; s->last_end = 0;
    return kept;
}
int dm_fl_pending(dm_fl_state *s, dm_fl_fight *out) {
    pthread_mutex_lock(&s->mu); int has = s->has_pend; if (has) *out = s->pend; pthread_mutex_unlock(&s->mu);
    return has;
}
int dm_fl_difficulty(const dm_fl_state *s) { return s->have_player ? s->p.difficulty : -1; }
const dm_fl_fight *dm_fl_last(const dm_fl_state *s) { return &s->last; }
void dm_fl_player_loss(const dm_fl_state *s, uint32_t *lost, uint32_t *hits) { *lost = (uint32_t)s->ploss; *hits = (uint32_t)(s->phits >> 8); }
int64_t dm_fl_split_gap(const dm_fl_state *s) { return s->gap; }


void dm_fl_begin(dm_fl_state *s, uint32_t tick, const dm_fl_player *p) {
    s->was_dead = s->have_player ? s->p.dead : p->dead; s->last_tick_hp = s->have_player ? s->p.hp : p->hp;
    s->tick = tick; s->p = *p; s->have_player = 1; s->me = s->en = s->skill = 0; s->nkills = s->nhits = 0;
}
int dm_fl_kills(const dm_fl_state *s, uint32_t tick, const uint32_t **ids) { *ids = s->kills; return s->have_player && s->tick == tick ? s->nkills : 0; }
int dm_fl_hits(const dm_fl_state *s, uint32_t tick) { return s->have_player && s->tick == tick ? s->nhits : 0; }
void dm_fl_record(dm_fl_state *s, int on) {
    s->norecord = !on;
    if (on) return;
    pthread_mutex_lock(&s->mu); if (s->has_pend) { s->has_pend = 0; s->gen++; } pthread_mutex_unlock(&s->mu);
}
void dm_fl_lost_hits(dm_fl_state *s, int n) { if (s->open && n > 0) s->hits_lost = 1; }


static int64_t engaged(const dm_fl_state *s, int skill, int delayed) { return s->have_act && !delayed && skill >= 0 && skill == s->act_skill ? (int64_t)s->act_t : -2; }
void dm_fl_engage(dm_fl_state *s, uint32_t enemy, int skill, int delayed) {
    int m = s->open ? find(s, enemy) : -1;
    if (m >= 0) { int g = s->mem[m].group; if (s->gact[g] == -1) s->gact[g] = engaged(s, skill, delayed); return; }
    for (unsigned k = 0; k < s->neng && k < 32; k++) if (s->eng[k].id == enemy) return;
    unsigned k = s->neng++ % 32; s->eng[k].id = enemy; s->eng[k].at = engaged(s, skill, delayed);
}

static void credit_mine(dm_fl_state *s, int skill, int g, int64_t v) {
    if (v <= 0) return;
    s->mine += v; if (g >= 0) s->gmine[g] += v;
    if (skill < 0) { s->sknone += v; return; }
    int k = skill_index(s, skill);
    if (k) s->sk[k - 1] += v; else s->skover += v;
}

static void credit_comp(dm_fl_state *s, int merc, uint16_t cls, int64_t v) {
    if (v <= 0) return;
    if (merc) { s->merc += v; return; }
    for (int k = 0; k < s->ncomp; k++) if (s->comp[k].cls == cls) { s->comp[k].dealt += v; return; }
    if (s->ncomp < DM_FL_COMPS) { s->comp[s->ncomp].cls = cls; s->comp[s->ncomp++].dealt = v; } else s->rest += v;
}

static void credit_dot(dm_fl_state *s, const slot *t, int g, const dm_fl_dot *d, int64_t v) {
    int64_t total = 0, given = 0;
    for (int k = 0; k < d->n; k++) if (d->e[k].rate > 0) total += d->e[k].rate;
    for (int k = 0; k < d->n; k++) {
        if (d->e[k].rate <= 0) continue;
        int64_t part = v - (total - d->e[k].rate); if (part > d->e[k].rate) part = d->e[k].rate;
        if (part <= 0) continue;
        switch (d->e[k].kind) {
        case DM_FL_DOT_PLAYER: credit_mine(s, d->e[k].burn ? t->bskill : t->pskill, g, part); break;
        case DM_FL_DOT_MERC: credit_comp(s, 1, 0, part); break;
        case DM_FL_DOT_COMP: credit_comp(s, 0, d->e[k].cls, part); break;
        default: s->unknown += part; break;
        }
        given += part;
    }
    s->unknown += v - given;
}
int dm_fl_on_hit(dm_fl_state *s, const dm_fl_hit *h, int life, int max_life) {
    if (!h->enemy) return 0;
    int began = 0, out = h->side == DM_FL_OUT_PLAYER || h->side == DM_FL_OUT_MINION;
    if (!s->open) { if (!can_start_hit(s)) return 0; start(s); began = 1; }
    int before = out ? h->before : 0;
    if (out && before > 0) s->nhits++;
    int m = find(s, h->enemy);
    if (m < 0) {
        if (out && before <= 0) return began;
        m = join(s, h->enemy, h->cls, h->rank, h->su, h->boss, out ? before : life, max_life);
    } else if (out ? before > 0 : 1) revived(s, m);
    s->last_damage = s->tick;
    if (!out) {
        if (h->side == DM_FL_IN_PLAYER) { int64_t got = (int64_t)h->before - (h->after > 0 ? h->after : 0); if (got > 0) { s->taken += got; s->phits += got; } s->en |= DM_FL_EN_HIT; }
        else s->en |= DM_FL_EN_MINION_HIT;
        return began;
    }
    int64_t dealt = before <= 0 ? 0 : (int64_t)before - (h->after > 0 ? h->after : 0);
    s->dealt[h->side == DM_FL_OUT_MINION] += dealt;
    int grp = m >= 0 ? s->mem[m].group : -1;
    if (h->side == DM_FL_OUT_MINION) credit_comp(s, h->merc, h->by, dealt); else credit_mine(s, h->skill, grp, dealt);
    if (h->side == DM_FL_OUT_PLAYER && grp >= 0 && s->gact[grp] == -1) s->gact[grp] = engaged(s, h->skill, h->delayed);
    s->me |= h->side == DM_FL_OUT_MINION ? DM_FL_ME_MINION : DM_FL_ME_HIT; if (h->crit) s->me |= DM_FL_ME_CRIT;
    if (m < 0) {
        if (before > 0 && h->after <= 0 && s->nkills < DM_FL_MEMBERS) s->kills[s->nkills++] = h->enemy;
        return began;
    }
    s->gdealt[s->mem[m].group] += dealt;
    int i = s->mem[m].slot;
    if (i >= 0) {
        set_life(s, i, h->after); s->sl[i].direct += dealt;
        if (h->side == DM_FL_OUT_PLAYER && (h->lays & DM_FL_LAYS_POISON)) s->sl[i].pskill = h->skill;
        if (h->side == DM_FL_OUT_PLAYER && (h->lays & DM_FL_LAYS_BURN)) s->sl[i].bskill = h->skill;
    }
    if (dealt > 0) s->mem[m].flags |= M_OURS;
    if (h->after <= 0) died(s, m);
    return began;
}
int dm_fl_on_dot(dm_fl_state *s, uint32_t enemy, uint32_t cls, uint8_t rank, uint16_t su, int boss, int life, int max_life) {
    if (!enemy || life <= 0) return 0;
    int began = 0;
    if (!s->open) { if (!can_start(s) || cooling(s, enemy)) return 0; start(s); s->f.h.flags |= DM_FL_F_DOT_START; began = 1; }
    int m = find(s, enemy);
    if (m < 0) m = join(s, enemy, cls, rank, su, boss, life, max_life);
    if (m >= 0) s->mem[m].flags |= M_DOT | M_OURS;
    return began;
}
int dm_fl_slots(const dm_fl_state *s) { return s->open ? DM_FL_TRACK : 0; }
uint32_t dm_fl_slot_id(const dm_fl_state *s, int i) { return s->sl[i].id; }
int dm_fl_slot_unexplained(const dm_fl_state *s, int i, int life) {
    const slot *t = &s->sl[i]; int64_t u = (int64_t)t->read_life - life - t->direct;
    return u > 0x7fffffff ? 0x7fffffff : u > 0 ? (int)u : 0;
}
void dm_fl_slot_seen(dm_fl_state *s, int i, int gone, int dead, int attack, int life, int dot_ours) {
    slot *t = &s->sl[i]; if (!t->id) return;
    int m = t->member; member *x = &s->mem[m];
    if (gone) {
        if (!(x->flags & (M_DEAD | M_GONE))) { x->flags |= M_GONE; s->alive--; s->galive[x->group]--; }
        x->slot = -1; t->id = 0; return;
    }
    if (dead) {
        if (t->life > 0 && (x->flags & M_DOT) && !(x->flags & M_DEAD)) {
            s->dealt[2] += t->life; s->gdealt[x->group] += t->life; s->me |= DM_FL_ME_DOT; s->last_damage = s->tick; s->unknown += t->life;
        }
        set_life(s, i, 0); died(s, m); return;
    }
    if (x->flags & M_DEAD) revived(s, m);
    if (t->atk >= 0 && attack && attack != t->atk) s->en |= DM_FL_EN_ATTACK;
    t->atk = attack;
    if (life < 0) return;
    int u = dm_fl_slot_unexplained(s, i, life);
    if (u > 0 && dot_ours) { s->dealt[2] += u; s->gdealt[x->group] += u; s->me |= DM_FL_ME_DOT; s->last_damage = s->tick; x->flags |= M_DOT | M_OURS; s->unknown += u; }
    set_life(s, i, life); t->read_life = t->life; t->direct = 0;
}
void dm_fl_dot_write(dm_fl_state *s, uint32_t enemy, int before, int after, const dm_fl_dot *dot) {
    if (!s->open || !dot || dot->n <= 0) return;
    int m = find(s, enemy); if (m < 0) return;
    member *x = &s->mem[m]; int i = x->slot;
    if (i < 0 || (x->flags & M_DEAD)) return;
    int64_t loss = (int64_t)before - (after > 0 ? after : 0); if (loss <= 0) return;
    slot *t = &s->sl[i];
    s->dealt[2] += loss; s->gdealt[x->group] += loss; s->me |= DM_FL_ME_DOT; s->last_damage = s->tick; x->flags |= M_DOT | M_OURS;
    credit_dot(s, t, x->group, dot, loss);
    t->direct += loss;
    if (after < t->life) set_life(s, i, after);
}
int dm_fl_members(const dm_fl_state *s) { return s->open ? s->nmem : 0; }
uint32_t dm_fl_member_watch(const dm_fl_state *s, int m) { const member *x = &s->mem[m]; return x->slot < 0 && !(x->flags & (M_DEAD | M_GONE)) ? x->id : 0; }
void dm_fl_member_seen(dm_fl_state *s, int m, int gone, int dead) {
    member *x = &s->mem[m]; if (x->slot >= 0 || (x->flags & (M_DEAD | M_GONE))) return;
    if (gone) { x->flags |= M_GONE; s->alive--; s->galive[x->group]--; }
    else if (dead) died(s, m);
}
void dm_fl_action(dm_fl_state *s, uint32_t tick, int bits, int skill) {
    s->act_t = tick; s->act_skill = skill; s->have_act = 1;
    if (!s->open) { unsigned k = s->npre++ % DM_FL_PREROLL; s->pre[k].t = tick; s->pre[k].bits = (uint8_t)bits; s->pre[k].skill = (int16_t)skill; return; }
    s->me |= bits; int k = skill_index(s, skill); if (k) s->skill = k;
    if (!s->t_act) s->t_act = tick;

}
void dm_fl_block(dm_fl_state *s, uint32_t tick) {
    (void)tick; if (!s->open) return;
    s->en |= DM_FL_EN_BLOCK;
}
void dm_fl_avoided(dm_fl_state *s, int what) { if (s->open && (what == DM_FL_EN_MISS || what == DM_FL_EN_DODGE)) s->en |= what; }
int dm_fl_elites_folded(const dm_fl_state *s) { return s->open && s->elites_folded; }
void dm_fl_potion(dm_fl_state *s, uint32_t tick, int column, int cls) {
    (void)tick; if (!s->open) return;
    s->me |= DM_FL_ME_POTION; mark(s, DM_FL_MARK_POTION, (uint8_t)column, (uint16_t)cls);
}
void dm_fl_states(dm_fl_state *s, uint32_t tick, const uint32_t bits[DM_FL_STATE_WORDS]) {
    (void)tick; memcpy(s->states, bits, sizeof s->states);
    if (!s->open) return;
    for (int w = 0; w < DM_FL_STATE_WORDS; w++) {
        uint32_t ch = s->fight_states[w] ^ bits[w];
        for (int b = 0; ch; b++, ch >>= 1) if (ch & 1) mark(s, bits[w] >> b & 1 ? DM_FL_MARK_STATE_ON : DM_FL_MARK_STATE_OFF, 0, (uint16_t)(w * 32 + b));
    }
    memcpy(s->fight_states, bits, sizeof s->fight_states);
}
int dm_fl_end(dm_fl_state *s, int *kept) {
    *kept = 0;
    if (!s->open) return 0;
    if (s->last_hp > s->p.hp) s->ploss += s->last_hp - s->p.hp;
    s->last_hp = s->p.hp;
    uint8_t life = pct8(s->p.hp, s->p.hp_max); if (life < s->f.h.min_life) s->f.h.min_life = life;
    if (!s->area) s->area = s->p.area;



    int won = !s->alive && s->f.h.kills;
    if (won && (!s->won || s->kill_t == s->tick)) s->won_t = s->tick;
    s->won = won;
    int left = s->p.town || (s->p.area && s->p.area != s->area);
    int kind = s->p.dead ? DM_FL_END_DEATH : won ? (left || s->tick - s->won_t >= DM_FL_GRACE ? DM_FL_END_KILL : 0) : left ? DM_FL_END_AREA
             : s->tick - s->last_damage >= DM_FL_PAUSE ? DM_FL_END_PAUSE : 0;
    if (kind != DM_FL_END_PAUSE) put(s, s->tick, s->me, s->en, s->skill);
    if (won && s->won_t == s->tick) {
        fight_record(s, &s->wonf, DM_FL_END_KILL, s->won_t);
        pthread_mutex_lock(&s->mu);
        if (!s->norecord) { s->pend = s->wonf; s->has_pend = 1; s->gen++; }
        pthread_mutex_unlock(&s->mu);
    }
    if (!kind) return 0;
    *kept = finish(s, kind, kind == DM_FL_END_PAUSE ? s->last_damage : kind == DM_FL_END_KILL ? s->won_t : s->tick);
    return s->last.h.end;
}


int dm_fl_count(dm_fl_state *s) { pthread_mutex_lock(&s->mu); int n = s->count; pthread_mutex_unlock(&s->mu); return n; }
int dm_fl_get(dm_fl_state *s, int newest, dm_fl_fight *out) {
    pthread_mutex_lock(&s->mu);
    int ok = newest >= 0 && newest < s->count;
    if (ok) *out = s->ring[(s->head - 1 - newest + DM_FL_RING) % DM_FL_RING];
    pthread_mutex_unlock(&s->mu);
    return ok;
}


static uint32_t crc32(const uint8_t *p, size_t n) {
    static uint32_t table[256]; static int ready;
    if (!ready) { for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1; table[i] = c; } ready = 1; }
    uint32_t c = 0xffffffffu;
    while (n--) c = table[(c ^ *p++) & 0xff] ^ (c >> 8);
    return c ^ 0xffffffffu;
}
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static uint32_t get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
static size_t fight_size(const dm_fl_head *h, int version) {
    return sizeof *h + (version >= DM_FL_VERSION ? sizeof(dm_fl_split) : 0) + h->ngroups * sizeof(dm_fl_group) + h->nbuckets * (sizeof(dm_fl_bucket) + (h->ngroups >= 2 ? h->ngroups : 0)) + h->nmarks * sizeof(dm_fl_mark);
}
static size_t put_fight(uint8_t *p, const dm_fl_fight *f) {
    const dm_fl_head *h = &f->h; uint8_t *o = p;
    memcpy(o, h, sizeof *h); o += sizeof *h;
    memcpy(o, &f->s, sizeof f->s); o += sizeof f->s;
    memcpy(o, f->g, h->ngroups * sizeof f->g[0]); o += h->ngroups * sizeof f->g[0];
    memcpy(o, f->b, h->nbuckets * sizeof f->b[0]); o += h->nbuckets * sizeof f->b[0];
    if (h->ngroups >= 2) for (int k = 0; k < h->nbuckets; k++) { memcpy(o, f->glife[k], h->ngroups); o += h->ngroups; }
    memcpy(o, f->m, h->nmarks * sizeof f->m[0]); o += h->nmarks * sizeof f->m[0];
    return (size_t)(o - p);
}

static int fight_ok(const dm_fl_fight *f) {
    const dm_fl_head *h = &f->h;
    if (h->end < DM_FL_END_KILL || h->end > DM_FL_END_LEFT || h->width < 1 || h->lead > DM_FL_LEAD || h->kills > h->enemies || h->enemies > DM_FL_MEMBERS) return 0;
    if ((uint64_t)h->duration + h->lead >= (uint64_t)h->nbuckets * h->width) return 0;
    for (int k = 0; k < DM_FL_SKILLS; k++) if (!h->skill[k] && k + 1 < DM_FL_SKILLS && h->skill[k + 1]) return 0;
    for (int g = 0; g < h->ngroups; g++) { const dm_fl_group *x = &f->g[g]; if (x->rank > DM_FL_RANK_SUPER || x->flags > DM_FL_G_OTHERS || x->kills > x->joined || x->tracked > x->joined) return 0; }
    for (int k = 0; k < h->nbuckets; k++) { int sk = f->b[k].skill; if (sk > DM_FL_SKILLS || (sk && !h->skill[sk - 1])) return 0; }
    const dm_fl_split *x = &f->s;
    if (x->flags > DM_FL_SPLIT_KNOWN || (!x->flags && memcmp(x, &(dm_fl_split){ 0 }, sizeof *x))) return 0;
    for (int k = 0; k < DM_FL_SKILLS; k++) if (x->skill[k] && !h->skill[k]) return 0;
    for (int g = h->ngroups; g < DM_FL_GROUPS; g++) if (x->gmine[g]) return 0;
    for (int i = 0; i < h->nmarks; i++) {
        const dm_fl_mark *m = &f->m[i];
        if (m->kind < DM_FL_MARK_POTION || m->kind > DM_FL_MARK_KILL) return 0;
        if (m->kind == DM_FL_MARK_KILL && m->a >= h->ngroups) return 0;
        if ((m->kind == DM_FL_MARK_STATE_ON || m->kind == DM_FL_MARK_STATE_OFF) && m->b >= 32 * DM_FL_STATE_WORDS) return 0;
    }
    return 1;
}

static size_t get_fight(const uint8_t *p, size_t left, dm_fl_fight *f, int version) {
    memset(f, 0, sizeof *f);
    if (left < sizeof f->h) return 0;
    memcpy(&f->h, p, sizeof f->h);
    const dm_fl_head *h = &f->h;
    if (h->ngroups > DM_FL_GROUPS || h->nbuckets < 1 || h->nbuckets > DM_FL_BUCKETS || h->nmarks > DM_FL_MARKS) return 0;
    size_t n = fight_size(h, version);
    if (n > left) return 0;
    const uint8_t *o = p + sizeof f->h;
    if (version >= DM_FL_VERSION) { memcpy(&f->s, o, sizeof f->s); o += sizeof f->s; }
    memcpy(f->g, o, h->ngroups * sizeof f->g[0]); o += h->ngroups * sizeof f->g[0];
    memcpy(f->b, o, h->nbuckets * sizeof f->b[0]); o += h->nbuckets * sizeof f->b[0];
    if (h->ngroups >= 2) for (int k = 0; k < h->nbuckets; k++) { memcpy(f->glife[k], o, h->ngroups); o += h->ngroups; }
    memcpy(f->m, o, h->nmarks * sizeof f->m[0]);
    return fight_ok(f) ? n : 0;
}
int dm_fl_owner(dm_fl_state *s, const char *who, const uint8_t *image, size_t n) {
    static dm_fl_fight in[DM_FL_RING];
    int bad = 0, count = 0;
    if (n) {
        if (n < DM_FL_IMAGE_HEAD || n > DM_FL_IMAGE_MAX) bad = DM_FL_BAD_SIZE;
        else if (get32(image) != DM_FL_MAGIC) bad = DM_FL_BAD_MAGIC;
        else if ((get32(image + 4) & 0xffff) != DM_FL_VERSION && (get32(image + 4) & 0xffff) != DM_FL_VERSION_1) bad = DM_FL_BAD_VERSION;
        else if (get32(image + 12) != crc32(image + DM_FL_IMAGE_HEAD, n - DM_FL_IMAGE_HEAD)) bad = DM_FL_BAD_CRC;
        else if (get32(image + 8) > DM_FL_RING) bad = DM_FL_BAD_COUNT;
        else {
            count = (int)get32(image + 8);
            size_t at = DM_FL_IMAGE_HEAD;
            int version = (int)(get32(image + 4) & 0xffff);
            for (int i = 0; i < count && !bad; i++) { size_t used = get_fight(image + at, n - at, &in[i], version); if (!used) bad = DM_FL_BAD_FIGHT; at += used; }
            if (!bad && at != n) bad = DM_FL_BAD_SIZE;
        }
    }
    if (bad) count = 0;
    pthread_mutex_lock(&s->mu);
    for (int i = 0; i < count; i++) s->ring[i] = in[i];
    s->count = count; s->head = count % DM_FL_RING;
    snprintf(s->who, sizeof s->who, "%s", who ? who : "");
    s->has_pend = 0;
    s->gen++; s->saved = s->gen;
    pthread_mutex_unlock(&s->mu);
    s->open = 0; s->npre = 0; s->ncool = 0;
    return bad;
}
int dm_fl_owned_by(dm_fl_state *s, const char *who) {
    pthread_mutex_lock(&s->mu); int same = !strcmp(s->who, who ? who : ""); pthread_mutex_unlock(&s->mu);
    return same;
}
size_t dm_fl_image(dm_fl_state *s, char who[64], uint8_t *buf, size_t cap, uint32_t *gen) {
    size_t n = 0;
    pthread_mutex_lock(&s->mu);
    if (s->gen != s->saved && s->who[0] && cap >= DM_FL_IMAGE_MAX) {


        int total = s->count + (s->has_pend ? 1 : 0), cnt = total > DM_FL_RING ? DM_FL_RING : total, skip = total - cnt;
        n = DM_FL_IMAGE_HEAD;
        for (int i = 0; i < cnt; i++) {
            int j = i + skip;
            n += put_fight(buf + n, j < s->count ? &s->ring[(s->head - s->count + j + DM_FL_RING) % DM_FL_RING] : &s->pend);
        }
        put32(buf, DM_FL_MAGIC); put32(buf + 4, DM_FL_VERSION); put32(buf + 8, (uint32_t)cnt);
        memcpy(who, s->who, 64); *gen = s->gen;
    }
    pthread_mutex_unlock(&s->mu);
    if (n) put32(buf + 12, crc32(buf + DM_FL_IMAGE_HEAD, n - DM_FL_IMAGE_HEAD));
    return n;
}
void dm_fl_saved(dm_fl_state *s, uint32_t gen) { pthread_mutex_lock(&s->mu); s->saved = gen; pthread_mutex_unlock(&s->mu); }
void dm_fl_unsaved(dm_fl_state *s) { pthread_mutex_lock(&s->mu); s->saved = s->gen - 1; pthread_mutex_unlock(&s->mu); }
