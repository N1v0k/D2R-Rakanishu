









#include "dm_fightlog.h"
#include "dm_fightlog_text.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct { char *s; size_t n, cap; int bad; } out_t;
__attribute__((format(printf, 2, 3)))
static void emit(out_t *w, const char *fmt, ...) {
    if (w->bad) return;
    va_list ap; va_start(ap, fmt); int n = vsnprintf(w->s + w->n, w->cap - w->n, fmt, ap); va_end(ap);
    if (n < 0 || (size_t)n >= w->cap - w->n) w->bad = 1; else w->n += (size_t)n;
}
static double pct(int v) { return v * 100.0 / 255.0; }

uint32_t dm_fl_fight_key(const dm_fl_fight *f) {
    uint32_t h = 2166136261u; const uint8_t *p = (const uint8_t *)&f->h;
    for (size_t i = 0; i < sizeof f->h; i++) h = (h ^ p[i]) * 16777619u;
    return h & 0x7fffffff;
}

int dm_fl_simulable(const dm_fl_fight *f, int group, uint32_t build) {
    const dm_fl_head *h = &f->h; const dm_fl_split *x = &f->s;
    if (!build || h->build != build) return 0;
    if (x->flags & DM_FL_SPLIT_KNOWN) {
        if (x->merc || x->rest || x->unknown) return 0;
        for (int k = 0; k < DM_FL_COMPS; k++) if (x->comp[k].dealt) return 0;
        if ((group < 0 ? x->t_act : group < DM_FL_GROUPS ? x->gact[group] : 0xffff) == 0xffff) return 0;
    }
    if (group < 0) return h->enemies == 1 && h->ngroups == 1;
    return h->ngroups >= 2 && group < h->ngroups && f->g[group].joined == 1 && !(f->g[group].flags & DM_FL_G_OTHERS);
}

int dm_fl_text_list(dm_fl_state *s, int recording, uint32_t build, dm_fl_namer name, void *ctx, char *out, size_t cap) {
    if (!out || !cap) return -1;
    static dm_fl_fight f;
    out_t w = { out, 0, cap, 0 }; out[0] = 0;
    int count = dm_fl_count(s);
    emit(&w, "log\t%d\t%d\n", count, recording);
    for (int i = 0; i < count && dm_fl_get(s, i, &f); i++) {
        const dm_fl_head *h = &f.h;
        const dm_fl_split *x = &f.s; uint32_t comps = x->merc + x->rest; for (int k = 0; k < DM_FL_COMPS; k++) comps += x->comp[k].dealt;
        emit(&w, "f\t%d\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%u\t%u\n", i, dm_fl_fight_key(&f), h->end, h->duration, h->lead, h->area, h->difficulty, h->kills,
            h->enemies, h->dealt[0], h->dealt[1], h->dealt[2], h->min_life, h->ngroups, dm_fl_simulable(&f, -1, build), x->flags & DM_FL_SPLIT_KNOWN, x->mine, comps);
        for (int g = 0; g < h->ngroups; g++) {
            const dm_fl_group *x = &f.g[g]; int boss = 0; const char *key = name ? name(x, &boss, ctx) : NULL;
            emit(&w, "g\t%d\t%d\t%s\t%u\t%d\t%u\t%u\t%u\t%u\t%u\t%u\t%d\t%d\t%u\n", i, g, key && key[0] ? key : "-", x->rank, boss, x->joined, x->kills, x->t_first, x->t_done,
                x->dealt, x->life, (x->flags & DM_FL_G_OTHERS) != 0, dm_fl_simulable(&f, g, build), f.s.gmine[g]);
        }
    }
    if (w.bad) { out[0] = 0; return -2; }
    return (int)w.n;
}

int dm_fl_text_fight(const dm_fl_fight *f, char *out, size_t cap) {
    if (!out || !cap) return -1;
    out_t w = { out, 0, cap, 0 }; out[0] = 0;
    const dm_fl_head *h = &f->h;
    int nskill = 0; while (nskill < DM_FL_SKILLS && h->skill[nskill]) nskill++;
    int unknown = 0;
    for (int k = 0; k < h->nbuckets; k++) if ((f->b[k].me & (DM_FL_ME_ATTACK | DM_FL_ME_CAST)) && !f->b[k].skill) unknown = 1;
    emit(&w, "fight\t%d\t%u\t%u\t%u\t-1\n", h->end == DM_FL_END_KILL ? 1 : h->end == DM_FL_END_DEATH ? 2 : 0, h->lead + h->duration, h->width, h->nbuckets);
    for (int k = 0; k < nskill; k++) emit(&w, "act\t%d\t%d\n", k, h->skill[k] - 1);
    if (unknown) emit(&w, "act\t%d\t-1\n", nskill);
    for (int k = 0; k < h->nbuckets; k++) {
        const dm_fl_bucket *b = &f->b[k];
        unsigned long long acts = 0;
        if (b->me & (DM_FL_ME_ATTACK | DM_FL_ME_CAST)) acts = 1ull << (b->skill ? b->skill - 1 : nskill);
        emit(&w, "trace\t%d", k);
        for (int sr = 0; sr < DM_FL_SERIES; sr++) emit(&w, "\t%.1f,%.1f,%.1f", pct(b->lo[sr]), pct(b->hi[sr]), pct(b->last[sr]));
        emit(&w, "\t0\t%llx\n", acts);
    }
    for (int k = 0; k < h->nbuckets; k++) if (f->b[k].en || (f->b[k].me & (DM_FL_ME_CRIT | DM_FL_ME_MINION))) emit(&w, "en\t%d\t%u\t%u\n", k, f->b[k].en, f->b[k].me);
    int span = h->nbuckets * h->width;
    for (int i = 0; i < h->nmarks; i++) {
        const dm_fl_mark *m = &f->m[i];
        if (m->t >= span) continue;
        int k = m->t / h->width, before = k > 0 ? k - 1 : 0;
        if (m->kind == DM_FL_MARK_POTION && m->b < 12) emit(&w, "drink\t%u\t%u\t%.1f\t%.1f\n", m->t, m->b, pct(f->b[before].last[DM_FL_S_LIFE]), pct(f->b[before].last[DM_FL_S_MANA]));
        else if (m->kind == DM_FL_MARK_STATE_ON || m->kind == DM_FL_MARK_STATE_OFF) emit(&w, "state\t%u\t%d\t%u\n", m->t, m->kind == DM_FL_MARK_STATE_ON, m->b);
        else if (m->kind == DM_FL_MARK_KILL) emit(&w, "kill\t%u\t%u\n", m->t, m->a);
    }
    if (h->ngroups >= 2)
        for (int g = 0; g < h->ngroups; g++) {
            emit(&w, "glife\t%d", g);
            for (int k = 0; k < h->nbuckets; k++) emit(&w, "%c%.1f", k ? ',' : '\t', pct(f->glife[k][g]));
            emit(&w, "\n");
        }
    if (w.bad) { out[0] = 0; return -2; }
    return (int)w.n;
}
