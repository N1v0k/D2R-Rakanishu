











#include "dm_fightlog_text.h"
#include "dm_fightlog_hist.h"
#include "dm_sim_ui.h"
#include "dm_custom.h"
#include "dm.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

static uint32_t fnv(uint32_t h, const void *p, size_t n) { const uint8_t *b = p; for (size_t i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u; return h; }

static uint32_t points(uint32_t h, const dm_sim_char *c) {
    for (int i = 0; i < DM_SIM_MAXSKILL; i++) if (c->blvl[i]) { uint32_t v = (uint32_t)i << 16 | (uint16_t)c->blvl[i]; h = fnv(h, &v, sizeof v); }
    return h;
}


static uint32_t ident_of(const dm_sim_snapshot *s) {
    const dm_sim_char *c = &s->c; uint32_t h = 2166136261u;
    #define K(x) h = fnv(h, &(x), sizeof(x))
    K(s->gear); K(c->cls); K(c->clvl); K(c->str); K(c->dex); K(c->vit); K(c->ene);
    #undef K
    return points(h, c);
}



static uint32_t key_from(const dm_sim_snapshot *s, uint32_t h) {
    const dm_sim_char *c = &s->c;
    #define K(x) h = fnv(h, &(x), sizeof(x))
    K(c->nhands); for (int i = 0; i < 2; i++) { K(c->hand[i].min); K(c->hand[i].max); K(c->hand[i].elmin); K(c->hand[i].elmax); K(c->hand[i].wsm); K(c->hand[i].kind); K(c->hand[i].ias); }
    K(c->ias); K(c->fcr); K(c->fhr); K(c->fbr); K(c->ed); K(c->ds); K(c->cs); K(c->cb); K(c->ow); K(c->ll); K(c->ml);
    K(c->mastery); K(c->pierce); K(c->sunder); K(c->dmg_demon); K(c->dmg_undead); K(c->shield); K(c->shield_min); K(c->shield_max);
    K(c->maxres); K(c->absorb_pct); K(c->absorb_flat); K(c->dr_flat); K(c->mdr_flat);
    #undef K
    return h ? h : 1;
}
static uint32_t key_of(const dm_sim_snapshot *s) { return key_from(s, ident_of(s)); }
#ifndef DM_PLAYER_BUILD

static uint32_t state_key_of(const dm_sim_snapshot *snap, int difficulty, dm_fl_parts *parts);
static void key_log(const char *at, const dm_sim_snapshot *s, int valid, uint32_t key, int stood) {
    const dm_sim_char *c = &s->c; uint32_t p[5] = { 2166136261u, 2166136261u, 2166136261u, 2166136261u, 2166136261u };
    #define K(n, x) p[n] = fnv(p[n], &(x), sizeof(x))
    K(0, s->gear); K(1, c->cls); K(1, c->clvl); K(1, c->str); K(1, c->dex); K(1, c->vit); K(1, c->ene); K(1, c->blvl);
    K(2, c->nhands); for (int i = 0; i < 2; i++) { K(2, c->hand[i].min); K(2, c->hand[i].max); K(2, c->hand[i].elmin); K(2, c->hand[i].elmax); K(2, c->hand[i].wsm); K(2, c->hand[i].kind); K(2, c->hand[i].ias); }
    K(3, c->ias); K(3, c->fcr); K(3, c->fhr); K(3, c->fbr); K(3, c->ed); K(3, c->ds); K(3, c->cs); K(3, c->cb); K(3, c->ow); K(3, c->ll); K(3, c->ml);
    K(4, c->mastery); K(4, c->pierce); K(4, c->sunder); K(4, c->dmg_demon); K(4, c->dmg_undead); K(4, c->shield); K(4, c->shield_min); K(4, c->shield_max);
    K(4, c->maxres); K(4, c->absorb_pct); K(4, c->absorb_flat); K(4, c->dr_flat); K(4, c->mdr_flat);
    #undef K
    dm_fl_parts parts; int diff = dm_fl_difficulty(dm_fl_core());
    LOG("[fightlog-key] at=%s valid=%d ar_known=%d stood_in=%d bad=%u key=%08x used=%08x gear=%08x points=%08x hands=%08x speed=%08x rest=%08x ias=%d ed=%d state=%08x", at, valid, c->ar_base_known,
        stood, c->capture_bad, valid ? key_of(s) : 0, key, p[0], p[1], p[2], p[3], p[4], c->ias, c->ed, valid && diff >= 0 ? state_key_of(s, diff, &parts) : 0);
}
#else
#define key_log(at, s, valid, key, stood) ((void)stood)
#endif





static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static struct { uint32_t ident, key; int session, set; dm_sim_char c; } g_complete;

static void fill_combat(dm_sim_char *c, const dm_sim_char *from) {
    c->def = from->def; c->block = from->block; c->ar = from->ar; c->ar_base = from->ar_base; c->ar_base_known = from->ar_base_known;
    c->sias += from->hand[0].wsm - c->hand[0].wsm;
    c->nhands = from->nhands; memcpy(c->hand, from->hand, sizeof c->hand);
    c->ed = from->ed; c->shield = from->shield; c->poison_frames = from->poison_frames; c->endpoint_bad = from->endpoint_bad;
    c->capture_bad = (c->capture_bad & ~3u) | (from->capture_bad & 3u);
}

static uint32_t settle(dm_sim_snapshot *s, int valid, int *stood) {
    *stood = 0;
    if (!valid) return 0;
    pthread_mutex_lock(&g_mu);
    uint32_t id = ident_of(s), k = 0;
    if (!(s->c.capture_bad & 3)) {
        g_complete.ident = id; k = g_complete.key = key_from(s, id); g_complete.session = s->session; g_complete.set = s->set; g_complete.c = s->c;
    } else if (g_complete.key && g_complete.ident == id && g_complete.session == s->session && g_complete.set == s->set) {
        k = g_complete.key; fill_combat(&s->c, &g_complete.c); *stood = 1;
    }
    else k = key_from(s, id);
    pthread_mutex_unlock(&g_mu);
    return k;
}
uint32_t dm_fl_build_key(void) {
    static dm_sim_snapshot snap;
    int valid = dm_sim_snapshot_copy(&snap), stood;
    return settle(&snap, valid, &stood);
}

static uint32_t state_key_of(const dm_sim_snapshot *snap, int difficulty, dm_fl_parts *parts) {
    memset(parts, 0, sizeof *parts);
    const dm_sim_char *c = &snap->c; uint32_t h = 2166136261u;
    parts->gear = fnv(h, &snap->gear, sizeof snap->gear);
    parts->points = points(h, c);
    h = fnv(h, &c->cls, sizeof c->cls); h = fnv(h, &c->str, sizeof c->str); h = fnv(h, &c->dex, sizeof c->dex); h = fnv(h, &c->vit, sizeof c->vit); parts->attrs = fnv(h, &c->ene, sizeof c->ene);
    parts->level = (uint16_t)c->clvl; parts->difficulty = (uint8_t)difficulty;
    uint32_t k = fnv(2166136261u, parts, sizeof *parts);
    return k ? k : 1;
}
uint32_t dm_fl_state_key(int difficulty, dm_fl_parts *parts) {
    static dm_sim_snapshot snap;
    if (!dm_sim_snapshot_copy(&snap)) { memset(parts, 0, sizeof *parts); return 0; }
    return state_key_of(&snap, difficulty, parts);
}

static const char *namer(const dm_fl_group *g, int *boss, void *ctx) {
    const dm_sim_data *d = ctx; *boss = 0;
    if (g->su != 0xffff) for (int i = 0; i < d->nsu; i++) if (d->su[i].idx == g->su) return d->su[i].namekey;
    if (g->cls < d->nmon && d->mon[g->cls].id[0]) { *boss = (d->mon[g->cls].flags & DM_MF_BOSS) != 0; return d->mon[g->cls].namekey; }
    return NULL;
}
typedef struct { dm_fl_state *s; int recording; uint32_t build; char *out; size_t cap; int n; const dm_fl_fight *f; const dm_sim_snapshot *snap; int group; } req;
static int list_read(const dm_sim_data *d, void *ctx) { req *q = ctx; return q->n = dm_fl_text_list(q->s, q->recording, q->build, namer, (void *)d, q->out, q->cap); }

static int target_of(const dm_sim_data *d, const dm_fl_fight *f, const dm_fl_group *g, dm_sim_target *t) {
    int diff = f->h.difficulty;
    if (diff > 2) return -1;
    if (g->rank == DM_FL_RANK_SUPER) { for (int i = 0; i < d->nsu; i++) if (d->su[i].idx == g->su) return dm_sim_target_su(d, i, diff, t); return -1; }
    if (g->cls >= d->nmon) return -1;
    int kind = g->rank == DM_FL_RANK_CHAMPION ? DM_TK_CHAMPION : g->rank == DM_FL_RANK_UNIQUE ? DM_TK_UNIQUE : g->rank == DM_FL_RANK_MINION ? DM_TK_MINION : DM_TK_NORMAL;
    static const int boss_kind[] = { DM_TK_ACTBOSS, DM_TK_QUEST, DM_TK_UBER, DM_TK_ANCIENT, DM_TK_CLONE };
    for (int i = 0; kind == DM_TK_NORMAL && i < d->nboss; i++) if (d->boss[i].mon == g->cls && d->boss[i].kind >= 0 && d->boss[i].kind < 5) kind = boss_kind[d->boss[i].kind];
    int mlvl = 0;
    for (int i = 0; i < d->narea; i++) if (d->area[i].id == f->h.area) mlvl = d->area[i].mlvl[diff];
    return dm_sim_target_mon(d, g->cls, diff, mlvl, kind, 1, t);
}

static int sim_run(const dm_sim_data *d, const dm_fl_fight *f, int group, const dm_sim_snapshot *snap, dm_sim_rot_out *out, dm_sim_trace *trace) {
    dm_sim_target t;
    if (target_of(d, f, &f->g[group < 0 ? 0 : group], &t)) return -1;
    uint32_t measured = group < 0 ? f->h.life_total : f->g[group].life;
    if (measured > 0) t.hp = (int)measured;
    int players = snap->players > 0 ? snap->players : 1; double strength = 1 + (players - 1) / 16.0;
    t.ar = (int)(t.ar * strength); t.dmin = (int)(t.dmin * strength); t.dmax = (int)(t.dmax * strength);
    for (int e = 0; e < 3; e++) { t.elmin[e] = (int)(t.elmin[e] * strength); t.elmax[e] = (int)(t.elmax[e] * strength); }
    dm_sim_char c = snap->c; c.diff = f->h.difficulty; c.potions = 0;


    int used = 0;
    for (int k = 0; k < DM_FL_SKILLS && f->h.skill[k]; k++) { int id = f->h.skill[k] - 1; if (id > 0 && id < DM_SIM_MAXSKILL && used < DM_SIM_MAXBAR) c.bar[used++] = id; }



    for (int i = 0; f->h.skill[0] && f->h.dealt[1] && i < snap->c.nbar && used < DM_SIM_MAXBAR; i++) {
        int id = snap->c.bar[i], have = 0;
        if (id <= 0 || id >= DM_SIM_MAXSKILL || !d->skill[id] || !(d->skill[id]->fam & (DM_FAM_P | DM_FAM_F))) continue;
        for (int k = 0; k < used; k++) have |= c.bar[k] == id;
        if (!have) c.bar[used++] = id;
    }
    if (f->h.skill[0]) { c.nbar = used; c.ordered = 0; }
    for (int i = 0; i < f->h.nmarks; i++) c.potions |= f->m[i].kind == DM_FL_MARK_POTION;
    int act = dm_sim_area_act(d, f->h.area); if (act > 0) c.act = act;
    dm_sim_surv_out sv = { 0 }; memset(out, 0, sizeof *out);
    if (dm_sim_fight(d, &c, &t, out, &sv, trace) || (trace && trace->n < 1)) return -1;
#ifndef DM_PLAYER_BUILD
    LOG("[fightlog-sim] group=%d dps=%.2f ttk=%.2f primary=%d skills=%d target=%d/%d hp=%d ac=%d block=%d char_ar=%d clvl=%d complete=%d recorded=%u/%u", group, out->dps, out->ttk,
        out->primary, c.nbar, t.mon, t.mlvl, t.hp, t.ac, t.block, c.ar, c.clvl, c.ar_base_known, f->h.dealt[0] + f->h.dealt[1] + f->h.dealt[2], f->h.duration);
#endif
    return 0;
}
static int sim_read(const dm_sim_data *d, void *ctx) {
    req *q = ctx; const dm_fl_fight *f = q->f; static dm_sim_trace tr; dm_sim_rot_out r;
    if (sim_run(d, f, q->group, q->snap, &r, &tr)) return -1;
    int offset; double dps;
    if (f->s.flags & DM_FL_SPLIT_KNOWN) {



        const dm_fl_group *g = &f->g[q->group < 0 ? 0 : q->group];
        offset = q->group < 0 ? f->s.t_act : f->s.gact[q->group];
        int killed = g->t_done > offset && (q->group >= 0 || f->h.end == DM_FL_END_KILL);
        double shown = q->group < 0 ? (f->h.duration ? f->s.mine * (double)DM_FL_TPS / f->h.duration : 0)
                                    : (g->t_done > g->t_first ? f->s.gmine[q->group] * (double)DM_FL_TPS / (g->t_done - g->t_first) : 0);
        dps = killed && r.ttk > 0 && shown > 0 ? shown * (g->t_done - offset) / DM_FL_TPS / r.ttk : -1;
    } else {
        offset = f->h.lead;
        for (int k = 0; k < f->h.nbuckets; k++) if (f->b[k].me & (DM_FL_ME_ATTACK | DM_FL_ME_CAST)) { offset = k * f->h.width; break; }
        if (q->group >= 0) offset = f->g[q->group].t_first;
        dps = r.dps;
    }
    size_t n = (size_t)q->n; int w;
    #define PUT(...) do { w = snprintf(q->out + n, q->cap - n, __VA_ARGS__); if (w < 0 || (size_t)w >= q->cap - n) return -2; n += (size_t)w; } while (0)
    PUT("sim\t%d\t%d\t%d\t%d\t%.2f\t%d\t%d\n", tr.end_kind, tr.end, tr.width, tr.n, dps, offset, q->group);
    for (int k = 0; k < tr.n; k++) {
        const dm_sim_trace_bucket *b = &tr.b[k];
        PUT("strace\t%d\t%.1f,%.1f,%.1f\t%.1f,%.1f,%.1f\n", k, b->lo[DM_TS_ENEMY], b->hi[DM_TS_ENEMY], b->last[DM_TS_ENEMY], b->lo[DM_TS_LIFE], b->hi[DM_TS_LIFE], b->last[DM_TS_LIFE]);
    }
    #undef PUT
    q->n = (int)n;
    return 0;
}






typedef struct { char *out; size_t cap; int n; const dm_dh_window *w; const dm_sim_snapshot *snap; uint32_t build; } dps_req;

static void hands_check(const dm_sim_data *d, const dm_fl_fight *f, const dm_sim_char *c) {
    static atomic_int said;
    for (int k = 0; k < DM_FL_SKILLS && f->h.skill[k]; k++) {
        int id = f->h.skill[k] - 1, kind = id >= 0 && id < DM_SIM_MAXSKILL && d->skill[id] ? dm_sim_skill_hands(d, c, id) : DM_SH_OK;
        if (kind == DM_SH_OK || atomic_exchange(&said, 1)) continue;
        LOG("ASSUME combat.skill_hands: skill %d was used with the hands of now, which the model says do not hold it (%d)", id, kind);
    }
}


static void compare(const dm_sim_data *d, dps_req *q, double *life, double *real, double *simulated, int *count) {
    static dm_fl_fight f; dm_fl_state *s = dm_fl_core(); *life = *real = *simulated = 0; *count = 0;
    for (int i = 0; i < dm_fl_count(s) && dm_fl_get(s, i, &f); i++) {
        const dm_fl_split *x = &f.s;
        if (!(x->flags & DM_FL_SPLIT_KNOWN) || x->state != q->w->key) continue;
        for (int g = -1; g < f.h.ngroups; g++) {
            if (!dm_fl_simulable(&f, g, q->build)) continue;
            hands_check(d, &f, &q->snap->c);
            const dm_fl_group *x2 = &f.g[g < 0 ? 0 : g];
            if (!x2->t_done || (g < 0 && f.h.end != DM_FL_END_KILL)) continue;
            int from = g < 0 ? x->t_act : x->gact[g];
            if (x2->t_done <= from) continue;
            dm_sim_rot_out r = { 0 };
            if (sim_run(d, &f, g, q->snap, &r, NULL) || r.ttk <= 0) continue;
            *life += x2->life; *real += (x2->t_done - from) / (double)DM_FL_TPS; *simulated += r.ttk; ++*count;
        }
    }
}
static const char *comp_key(const dm_sim_data *d, uint16_t cls) { return cls < d->nmon && d->mon[cls].id[0] ? d->mon[cls].namekey : "-"; }
static int dps_read(const dm_sim_data *d, void *ctx) {
    dps_req *q = ctx; const dm_dh_window *w = q->w; int n = (int)w->n;
    size_t at = 0; int ww;
    #define PUT(...) do { ww = snprintf(q->out + at, q->cap - at, __VA_ARGS__); if (ww < 0 || (size_t)ww >= q->cap - at) return -2; at += (size_t)ww; } while (0)
    uint64_t ticks = 0, total = 0; for (int i = 0; i < n; i++) ticks += w->f[i].ticks;
    double own = dm_dh_value(w->f, n, 0), team = dm_dh_value(w->f, n, 1), lo = 0, hi = 0, tlo = 0, thi = 0;
    dm_dh_range(w->f, n, 0, -1, &lo, &hi); dm_dh_range(w->f, n, 1, -1, &tlo, &thi);
    PUT("dps\t%d\t%llu\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\t%.0f\n", n, (unsigned long long)ticks, own * 100, lo * 100, hi * 100, team * 100, tlo * 100, thi * 100);
    if (d && n) {
        double life, real, simulated; int count;
        compare(d, q, &life, &real, &simulated, &count);
        if (count && real > 0 && simulated > 0) {
            double reached = simulated / real;
            PUT("sim\t%.0f\t%.0f\t%d\n", life / simulated * 100, reached * 1000, count);
            static int logged;
            if (count >= 5 && (reached > 1.5 || reached < 0.4) && !logged) { logged = 1; LOG("ASSUME dps.sim_gap: the fights reach %.0f %% of the simulation over %d fights", reached * 100, count); }
        }
    }

    struct { uint16_t id; uint64_t dealt; } src[64]; int ns = 0;
    for (int i = 0; i < n; i++) for (int k = 0; k < w->f[i].nsrc; k++) {
        const dm_dh_src *x = &w->f[i].src[k]; int j = 0; total += x->dealt;
        while (j < ns && src[j].id != x->id) j++;
        if (j == ns) { if (ns == 64) continue; src[ns].id = x->id; src[ns++].dealt = 0; }
        src[j].dealt += x->dealt;
    }
    for (int a = 0; a < ns; a++) for (int b = a + 1; b < ns; b++) if (src[b].dealt > src[a].dealt) { uint16_t ti = src[a].id; uint64_t td = src[a].dealt; src[a] = src[b]; src[b].id = ti; src[b].dealt = td; }


    int pct[64] = { 0 }, left = total ? 100 : 0; uint8_t got[64] = { 0 };
    for (int j = 0; j < ns && total; j++) { pct[j] = (int)(src[j].dealt * 100 / total); left -= pct[j]; }
    for (; left > 0; left--) {
        int best = -1;
        for (int j = 0; j < ns; j++) if (!got[j] && (best < 0 || src[j].dealt * 100 % total > src[best].dealt * 100 % total)) best = j;
        if (best < 0) break;
        pct[best]++; got[best] = 1;
    }
    for (int j = 0; j < ns; j++) {
        uint16_t id = src[j].id; char kind = id == DM_DH_SRC_MERC ? 'm' : id == DM_DH_SRC_REST ? 'r' : id == DM_DH_SRC_UNKNOWN ? 'u' : id == DM_DH_SRC_NONE ? 'n' : id == DM_DH_SRC_OTHER ? 'o' : id & DM_DH_SRC_COMP ? 'c' : 's';
        double slo = 0, shi = 0; if (kind == 's') dm_dh_range(w->f, n, 0, id, &slo, &shi);
        PUT("src\t%c\t%u\t%s\t%llu\t%.0f\t%.0f\t%.0f\t%.0f\n", kind, kind == 'c' ? id & 0x7fffu : id, kind == 'c' && d ? comp_key(d, id & 0x7fff) : "-", (unsigned long long)src[j].dealt,
            ticks ? src[j].dealt * 100.0 * DM_FL_TPS / ticks : 0, slo * 100, shi * 100, pct[j] * 10.0);
    }

    static dm_dh_window cur; static dm_dh_seg seg[DM_DH_SEGS]; int nseg = dm_dh_read(dm_dh_core(), 0, &cur, seg);
    if (nseg && seg[nseg - 1].fights < DM_DH_MIN) nseg--;
    for (int i = 0; i < nseg; i++) {
        const dm_fl_parts *a = &seg[i].parts, *b = i ? &seg[i - 1].parts : NULL; int changed = 0;
        if (b) changed = (a->level != b->level ? DM_FL_CHANGE_LEVEL : 0) | (a->points != b->points ? DM_FL_CHANGE_POINTS : 0) | (a->attrs != b->attrs ? DM_FL_CHANGE_ATTRS : 0)
                       | (a->gear != b->gear ? DM_FL_CHANGE_GEAR : 0) | (a->difficulty != b->difficulty ? DM_FL_CHANGE_DIFFICULTY : 0);
        PUT("seg\t%d\t%u\t%u\t%u\t%u\t%u\t%d\t%u\t%u\n", i, seg[i].ticks, seg[i].fights, seg[i].value, seg[i].lo, seg[i].hi, changed, a->level, a->difficulty);
    }
    #undef PUT
    q->n = (int)at;
    return 0;
}

static int dps_text(const dm_sim_snapshot *snap, int have, uint32_t build, char *out, size_t cap) {
    static dm_dh_window w; static dm_dh_seg segs[DM_DH_SEGS]; dm_fl_parts parts;
    int diff = dm_fl_difficulty(dm_fl_core());
    dm_dh_read(dm_dh_core(), have && diff >= 0 ? state_key_of(snap, diff, &parts) : 0, &w, segs);
    dps_req q = { out, cap, 0, &w, snap, have ? build : 0 };
    if (!have || dm_sim_read(snap->set ? "rotw" : "lod", dps_read, &q) < 0) { q.n = 0; if (dps_read(NULL, &q) < 0) return -2; }
    return q.n;
}

int dm_fl_ui_query(int fight, int group, uint32_t key, int sim, char *out, size_t cap) {
    static dm_fl_fight f;
    static dm_sim_snapshot snap;
    if (!out || !cap) return -1;
    dm_fl_state *s = dm_fl_core();
    int have = dm_sim_snapshot_copy(&snap); const char *set = !have || snap.set ? "rotw" : "lod";
    int stood; uint32_t build = settle(&snap, have, &stood);
    if (fight == DM_FL_QUERY_DPS) return dps_text(&snap, have, build, out, cap);
    if (fight < 0) key_log("list", &snap, have, build, stood);
    req q = { s, dm_custom_get("fight_log") != 0, build, out, cap, 0, &f, &snap, group < 0 ? -1 : group };
    if (fight < 0) {
        if (dm_sim_read(set, list_read, &q) < 0 || q.n <= 0) q.n = dm_fl_text_list(s, q.recording, build, NULL, NULL, out, cap);
        return q.n;
    }
    if (!dm_fl_get(s, fight, &f) || dm_fl_fight_key(&f) != key) return -1;
    q.n = dm_fl_text_fight(&f, out, cap);
    if (q.n < 0) return q.n;
    if (sim && dm_fl_simulable(&f, q.group, build)) {
        int n = q.n;
        if (dm_sim_read(set, sim_read, &q) < 0) { q.n = n; out[n] = 0; }
    }
    return q.n;
}
