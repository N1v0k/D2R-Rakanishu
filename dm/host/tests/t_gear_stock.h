


#ifndef T_GEAR_STOCK_H
#define T_GEAR_STOCK_H
#include "../dm_gear_search.h"
#include "sim_test_io.h"
#include <math.h>
#include <string.h>

#define GS_MAXCAND 640
#define GS_MAXWEAPON 240
typedef struct {
    dm_gear_problem p;
    dm_gear_cand cand[GS_MAXCAND];
    dm_gear_weapon weapon[GS_MAXWEAPON];
} gstock;

typedef struct {
    const char *profile;
    int act;
    int count[DM_GEAR_K_N];
    int variants;
    int charms_inv, charms_stash;
    int blocked;
    int sets;
    int unrated;
    int skills;
    int chain;
    int mana;
    int dual;
    int swords;
    int worn_sword;
    int claws;
    uint64_t seed;
} gspec;

static uint64_t gs_seed;
static uint32_t gs_rnd(void) { gs_seed ^= gs_seed << 13; gs_seed ^= gs_seed >> 7; gs_seed ^= gs_seed << 17; return (uint32_t)(gs_seed >> 11); }
static int gs_int(int a, int b) { return b <= a ? a : a + (int)(gs_rnd() % (uint32_t)(b - a + 1)); }
static double gs_unit(void) { return (gs_rnd() & 0xffffff) / (double)0x1000000; }
static int gs_stat(const dm_sim_data *d, const char *key) {
    for (int i = 0; i < DM_GEAR_NSTAT; i++) if (!strcmp(d->stat_name[i], key)) return i;
    fprintf(stderr, "stock: no stat %s in the tables\n", key); exit(2);
}

typedef struct { const char *key; int id, layer, lo, hi; } gs_val;

static const gs_val gs_armor[] = { { NULL, 31, 0, 40, 400 }, { NULL, 16, 0, 20, 120 }, { "fireresist", 0, 0, 5, 40 }, { "lightresist", 0, 0, 5, 40 },
                                   { "maxhp", 0, 0, 10 * 256, 60 * 256 }, { "item_fastergethitrate", 0, 0, 10, 30 }, { NULL, 3, 0, 5, 20 },
                                   { "normal_damage_reduction", 0, 0, 2, 12 }, { "item_magicbonus", 0, 0, 10, 50 } };
static const gs_val gs_jewel[] = { { NULL, 0, 0, 2, 20 }, { NULL, 2, 0, 2, 20 }, { NULL, 1, 0, 2, 20 }, { NULL, 3, 0, 2, 20 }, { "maxmana", 0, 0, 10 * 256, 60 * 256 },
                                   { "coldresist", 0, 0, 5, 30 }, { "poisonresist", 0, 0, 5, 30 }, { "lifedrainmindam", 0, 0, 2, 8 }, { NULL, 19, 0, 20, 150 },
                                   { "item_fastercastrate", 0, 0, 5, 20 }, { "hpregen", 0, 0, 2 * 256, 8 * 256 }, { NULL, 22, 0, 2, 12 },
                                   { "item_magicbonus", 0, 0, 10, 50 } };
static const gs_val gs_hand[] = { { NULL, 17, 0, 40, 220 }, { NULL, 18, 0, 40, 220 }, { NULL, 19, 0, 40, 300 }, { "item_fasterattackrate", 0, 0, 10, 40 },
                                  { "lifedrainmindam", 0, 0, 3, 10 }, { "item_deadlystrike", 0, 0, 10, 30 }, { NULL, 48, 0, 5, 30 }, { NULL, 49, 0, 20, 60 },
                                  { "item_fastercastrate", 0, 0, 10, 30 } };
static const gs_val gs_shield[] = { { NULL, 31, 0, 30, 300 }, { NULL, 20, 0, 10, 40 }, { "fireresist", 0, 0, 5, 40 }, { "coldresist", 0, 0, 5, 40 },
                                    { "item_fasterblockrate", 0, 0, 10, 40 }, { "maxhp", 0, 0, 10 * 256, 40 * 256 } };
static const gs_val gs_charm[] = { { "maxhp", 0, 0, 5 * 256, 20 * 256 }, { "fireresist", 0, 0, 3, 11 }, { "lightresist", 0, 0, 3, 11 }, { NULL, 19, 0, 10, 40 },
                                   { NULL, 22, 0, 1, 10 }, { "item_fastergethitrate", 0, 0, 5, 12 }, { NULL, 2, 0, 1, 6 }, { "item_magicbonus", 0, 0, 3, 7 } };

static int gs_id(const dm_sim_data *d, const gs_val *v) { return v->key ? gs_stat(d, v->key) : v->id; }
static void gs_values(const dm_sim_data *d, dm_gear_item *it, const gs_val *tab, int ntab, int nval, double scale) {
    for (int k = 0; k < nval; k++) {
        const gs_val *v = &tab[gs_int(0, ntab - 1)];
        int x = (int)lround(gs_int(v->lo, v->hi) * scale);
        dm_gear_list_add(&it->own, gs_id(d, v), v->layer, x);
    }
}

typedef struct { int ok; int set, rows[8], nrows; } gs_set;

static gs_set gs_find_set(const dm_gear_tables *t, int skip) {
    gs_set s = { 0 };
    for (int set = 0; set < t->nset; set++) {
        int bad = 0, n = 0, rows[8];
        for (int k = 0; k < t->nsetb; k++) if (t->setb[k].owner == set && t->setb[k].unmapped) bad = 1;
        for (int r = 0; r < t->nitem; r++) {
            if (t->item[r].set != set) continue;
            if (t->item[r].func == 1) bad = 1;
            for (int k = 0; k < t->nitemb; k++) if (t->itemb[k].owner == r && t->itemb[k].unmapped) bad = 1;
            if (n < 8) rows[n++] = r;
        }
        if (bad || n < 2 || skip-- > 0) continue;
        s.ok = 1; s.set = set; s.nrows = n; memcpy(s.rows, rows, sizeof rows);
        return s;
    }
    return s;
}

static dm_gear_weapon *gs_weapon(gstock *g, const char *wclass, int kind, int wsm, int mode2) {
    if (g->p.nweapon >= GS_MAXWEAPON) { fprintf(stderr, "stock: too many weapons\n"); exit(2); }
    dm_gear_weapon *w = &g->weapon[g->p.nweapon++];
    memset(w, 0, sizeof *w);
    snprintf(w->w.wclass, sizeof w->w.wclass, "%s", wclass); w->w.kind = kind; w->w.wsm = wsm; w->w.start_known = 1;

    static const char *const one[] = { "swor", "axe", "knif", "mace", "hamm", "jave" }, *const two[] = { "axe", "swor", "pole", "spea", "staf" };
    int i = g->p.nweapon - 1;
    const char *type = kind == DM_WK_NONE ? "" : kind == DM_WK_BOW ? "bow" : kind == DM_WK_XBOW ? "xbow" : mode2 ? two[i % 5] : one[i % 6];

    if (!strcmp(wclass, "1HS") && g->p.now.cls == 4 && !strcmp(type, "jave")) snprintf(w->w.wclass, sizeof w->w.wclass, "1HT");
    snprintf(w->w.stype, sizeof w->w.stype, "%s", type); w->w.srange = kind == DM_WK_NONE ? 0 : i % 4;
    w->mode2 = mode2; w->strb = kind == DM_WK_NONE ? 0 : 100; w->dexb = kind == DM_WK_BOW || kind == DM_WK_XBOW ? 100 : 0;
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) { w->mth[id] = g->p.node[id] ? 0 : DM_SIM_NO_NODE; w->mdmg[id] = w->mth[id]; }
    return w;
}
static dm_gear_cand *gs_new(gstock *g, int kind) {
    if (g->p.ncand >= GS_MAXCAND) { fprintf(stderr, "stock: too many candidates\n"); exit(2); }
    dm_gear_cand *c = &g->cand[g->p.ncand]; memset(c, 0, sizeof *c);
    c->kind = kind; c->w = c->h = 1; c->weapon = -1; c->item.setitem = -1; c->item.id = (uint32_t)(1000 + g->p.ncand);
    g->p.ncand++;
    return c;
}

static dm_gear_cand *gs_item(gstock *g, int kind, double scale, int chain_str) {
    const dm_sim_data *d = g->p.d;
    dm_gear_cand *c = gs_new(g, kind);
    dm_gear_item *it = &c->item;
    switch (kind) {
    case DM_GEAR_K_HEAD: case DM_GEAR_K_ARMOR: case DM_GEAR_K_BELT: case DM_GEAR_K_BOOTS: case DM_GEAR_K_GLOVES:
        gs_values(d, it, gs_armor, sizeof gs_armor / sizeof *gs_armor, gs_int(1, 4), scale);
        if (kind == DM_GEAR_K_BOOTS) { c->dmin = gs_int(2, 10); c->dmax = c->dmin + gs_int(2, 20); }
        if (kind == DM_GEAR_K_GLOVES && gs_int(0, 2) == 0) dm_gear_list_add(&it->own, gs_stat(d, "item_fasterattackrate"), 0, (int)lround(gs_int(10, 20) * scale));
        break;
    case DM_GEAR_K_AMULET: case DM_GEAR_K_RING:
        gs_values(d, it, gs_jewel, sizeof gs_jewel / sizeof *gs_jewel, gs_int(1, 4), scale);
        break;
    case DM_GEAR_K_SHIELD:
        gs_values(d, it, gs_shield, sizeof gs_shield / sizeof *gs_shield, gs_int(1, 4), scale);
        c->block = 1; c->dmin = gs_int(3, 10); c->dmax = c->dmin + gs_int(5, 20); c->w = 2; c->h = 3;
        break;
    case DM_GEAR_K_ARROWS: case DM_GEAR_K_BOLTS:
        dm_gear_list_add(&it->own, 22, 0, (int)lround(gs_int(1, 8) * scale));
        break;
    case DM_GEAR_K_CHARM:
        c->h = gs_int(1, 3);
        gs_values(d, it, gs_charm, sizeof gs_charm / sizeof *gs_charm, gs_int(1, 2), scale * c->h);
        break;
    default: {
        int two = kind == DM_GEAR_K_WEAPON2, bow = kind == DM_GEAR_K_BOW, xbow = kind == DM_GEAR_K_XBOW;
        dm_gear_weapon *w = gs_weapon(g, bow ? "BOW" : xbow ? "XBW" : two ? "2HS" : "1HS", bow ? DM_WK_BOW : xbow ? DM_WK_XBOW : DM_WK_MELEE,
                                      gs_int(-10, 10), two);
        c->weapon = (int)(w - g->weapon); c->w = two || bow || xbow ? 2 : 1; c->h = 3;
        int lo = (int)lround(gs_int(3, 30) * scale * (two ? 2 : 1)), hi = lo + (int)lround(gs_int(5, 40) * scale * (two ? 2 : 1));
        dm_gear_list_add(&it->own, two ? 23 : 21, 0, lo); dm_gear_list_add(&it->own, two ? 24 : 22, 0, hi);
        gs_values(d, it, gs_hand, sizeof gs_hand / sizeof *gs_hand, gs_int(1, 3), scale);
        it->req_str = gs_int(0, 80);
    } }
    if (chain_str > 0 && gs_int(0, 3) == 0) it->req_str = chain_str + gs_int(0, 25);
    return c;
}


static dm_gear_cand *gs_sword(gstock *g, double scale, int chain_str) {
    dm_gear_cand *c = gs_item(g, DM_GEAR_K_WEAPON2, scale, chain_str);
    dm_gear_weapon *w = &g->weapon[c->weapon];
    snprintf(w->w.stype, sizeof w->w.stype, "swor");
    w->has_one = 1; w->one = w->w; snprintf(w->one.wclass, sizeof w->one.wclass, "1HS");
    int lo = dm_gear_list_get(&c->item.own, 23, 0), hi = dm_gear_list_get(&c->item.own, 24, 0);
    dm_gear_list_add(&c->item.own, 21, 0, lo * 2 / 5 + 1); dm_gear_list_add(&c->item.own, 22, 0, hi * 3 / 5 + 1);
    return c;
}
static void gs_place_charm(gstock *g, dm_gear_cand *c, uint8_t taken[DM_GEAR_GRID_H][DM_GEAR_GRID_W]) {
    for (int tries = 0; tries < 200; tries++) {
        int x = gs_int(0, DM_GEAR_GRID_W - c->w), y = gs_int(0, DM_GEAR_GRID_H - c->h), ok = 1;
        for (int j = 0; j < c->h; j++) for (int i = 0; i < c->w; i++) ok &= !taken[y + j][x + i] && !g->p.grid[y + j][x + i];
        if (!ok) continue;
        for (int j = 0; j < c->h; j++) for (int i = 0; i < c->w; i++) taken[y + j][x + i] = 1;
        c->inv = 1; c->x = x; c->y = y;
        return;
    }
}

static void gs_skill_bonus(gstock *g, dm_gear_item *it, int bar_skill) {
    const dm_sim_data *d = g->p.d;
    switch (gs_int(0, 3)) {
    case 0: dm_gear_list_add(&it->own, 127, 0, gs_int(1, 2)); break;
    case 1: dm_gear_list_add(&it->own, 83, g->p.now.cls, gs_int(1, 3)); break;
    case 2: if (bar_skill > 0 && g->p.tab[bar_skill] >= 0) dm_gear_list_add(&it->own, 188, g->p.tab[bar_skill], gs_int(1, 3)); break;
    default: if (bar_skill > 0) dm_gear_list_add(&it->own, 107, bar_skill, gs_int(1, 3)); break;
    }
    (void)d;
}




static void gs_dr(dm_gear_problem *p) {
    static const int ids[] = { 17, 18, 19, 21, 22, 23, 24, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 60, 62, 68, 93, 117, 119, 121, 122, 135, 136, 141, 224 };
    memset(p->dr, 0, sizeof p->dr);
    for (size_t k = 0; k < sizeof ids / sizeof *ids; k++) p->dr[ids[k]] = 1;
    p->dr_ok = 1;
}
static int gs_capture(gstock *g);

static int gs_build(gstock *g, const dm_sim_data *d, const dm_gear_tables *t, const gspec *spec) {
    memset(g, 0, sizeof *g);
    gs_seed = spec->seed * 2654435761ULL + 88172645463325252ULL;
    dm_gear_problem *p = &g->p;
    p->d = d; p->t = t; p->cand = g->cand; p->weapon = g->weapon;
    gs_dr(p); p->idle = 0;
    char err[128];
    if (dm_sim_profile_parse(d, spec->profile, &p->now, err, sizeof err)) { fprintf(stderr, "stock profile: %s\n", err); return -1; }
    dm_sim_char *c = &p->now;
    c->act = spec->act;
    if (spec->mana == 2) { c->belt[DM_POT_MP1] = 4; c->belt[DM_POT_RVS] = 2; c->potions = 1; }
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) { p->tab[id] = p->elem[id] = -1; p->node[id] = d->skill[id] && c->lvl[id] > 0; }
    p->node[0] = 1; if (c->lvl[0] < 1) c->lvl[0] = 1;
    int bar[DM_SIM_MAXBAR], nbar = 0;
    for (int i = 0; i < c->nbar; i++) if (c->bar[i] > 0 && p->node[c->bar[i]]) { bar[nbar++] = c->bar[i]; p->tab[c->bar[i]] = (int16_t)(c->cls * 8 + c->bar[i] % 3); }

    dm_gear_base *b = &p->base;
    b->cls = c->cls; b->clvl = c->clvl; b->lpv = d->cls[c->cls].lifevit; b->mpm = d->cls[c->cls].manamag;
    dm_gear_list_add(&b->base, 0, 0, c->str); dm_gear_list_add(&b->base, 1, 0, c->ene); dm_gear_list_add(&b->base, 2, 0, c->dex); dm_gear_list_add(&b->base, 3, 0, c->vit);
    dm_gear_list_add(&b->base, 7, 0, (int)(c->life * 256)); dm_gear_list_add(&b->base, 9, 0, (int)(c->mana * 256));
    dm_gear_list_add(&b->base, 19, 0, 200); dm_gear_list_add(&b->base, gs_stat(d, "attackrate"), 0, 100);
    static const char *res[] = { "fireresist", "lightresist", "coldresist", "poisonresist" };
    for (int e = 0; e < 4; e++) dm_gear_list_add(&b->base, gs_stat(d, res[e]), 0, c->res[e + 1] / 3);
    dm_gear_list_add(&b->base, gs_stat(d, "item_fastercastrate"), 0, c->fcr / 2);

    gs_weapon(g, "HTH", DM_WK_NONE, 0, 0); p->unarmed = 0; p->nweapon = 1;
    int chain = spec->chain ? c->str + 10 : 0;

    for (int k = 0; k < spec->blocked; k++) {
        int w = gs_int(1, 2), h = gs_int(1, 3), x = gs_int(0, DM_GEAR_GRID_W - w), y = gs_int(0, DM_GEAR_GRID_H - h);
        for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) p->grid[y + j][x + i] = 1;
    }

    static const int worn_kinds[] = { DM_GEAR_K_WEAPON1, DM_GEAR_K_SHIELD, DM_GEAR_K_ARMOR, DM_GEAR_K_AMULET, DM_GEAR_K_RING, DM_GEAR_K_RING, DM_GEAR_K_HEAD,
                                      DM_GEAR_K_BELT, DM_GEAR_K_BOOTS, DM_GEAR_K_GLOVES };
    for (size_t k = 0; k < sizeof worn_kinds / sizeof *worn_kinds; k++) {
        int kind = worn_kinds[k];
        if (spec->dual && kind == DM_GEAR_K_SHIELD) kind = DM_GEAR_K_WEAPON1;
        if (kind == DM_GEAR_K_SHIELD && c->cls == 0 && spec->count[DM_GEAR_K_BOW]) continue;
        if (gs_int(0, 9) == 0 && kind != DM_GEAR_K_WEAPON1) continue;
        dm_gear_cand *x = !k && spec->worn_sword ? gs_sword(g, 1, 0) : gs_item(g, kind == DM_GEAR_K_WEAPON1 && c->cls == 0 && spec->count[DM_GEAR_K_BOW] ? DM_GEAR_K_BOW : kind, 1, 0);
        x->worn = 1; x->item.req_str = 0;
        if (spec->dual && (x->kind == DM_GEAR_K_WEAPON1 || x->kind == DM_GEAR_K_WEAPON2)) x->item.slot = k ? 5 : 4;
        if (spec->skills && gs_int(0, 2) == 0) gs_skill_bonus(g, &x->item, nbar ? bar[gs_int(0, nbar - 1)] : 0);
    }
    for (int kind = DM_GEAR_K_HEAD; kind < DM_GEAR_K_N; kind++) {
        if (kind == DM_GEAR_K_CHARM) continue;
        for (int i = 0; i < spec->count[kind]; i++) {
            dm_gear_cand *first = gs_item(g, kind, 1, chain);
            if (spec->skills && kind != DM_GEAR_K_SHIELD && gs_int(0, 3) == 0) gs_skill_bonus(g, &first->item, nbar ? bar[gs_int(0, nbar - 1)] : 0);
            dm_gear_cand base = *first;
            for (int v = 1; v < spec->variants; v++) {
                dm_gear_cand *x = gs_new(g, kind);
                int id = (int)x->item.id; *x = base; x->item.id = (uint32_t)id;
                double f = 0.5 + gs_unit();
                for (int k = 0; k < x->item.own.n; k++) {
                    dm_gear_v *e = &x->item.own.v[k];
                    if (e->stat == 83 || e->stat == 107 || e->stat == 127 || e->stat == 188) continue;
                    e->value = (int)lround(e->value * f);
                }
                if (base.weapon >= 0) {
                    dm_gear_weapon *w = gs_weapon(g, g->weapon[base.weapon].w.wclass, g->weapon[base.weapon].w.kind, g->weapon[base.weapon].w.wsm, g->weapon[base.weapon].mode2);
                    memcpy(w->w.stype, g->weapon[base.weapon].w.stype, sizeof w->w.stype); w->w.srange = g->weapon[base.weapon].w.srange;
                    x->weapon = (int)(w - g->weapon);
                }
            }
        }
    }
    for (int i = 0; i < spec->swords; i++) gs_sword(g, 1, chain);

    uint8_t taken[DM_GEAR_GRID_H][DM_GEAR_GRID_W] = { { 0 } };
    for (int i = 0; i < spec->charms_inv + spec->charms_stash; i++) {
        dm_gear_cand *x = gs_item(g, DM_GEAR_K_CHARM, 0.5 + gs_unit(), 0);
        if (spec->skills && x->h == 3 && gs_int(0, 1) == 0 && nbar) dm_gear_list_add(&x->item.own, 188, p->tab[bar[gs_int(0, nbar - 1)]], 1);
        x->item.req_lvl = gs_int(1, c->clvl);
        if (i < spec->charms_inv) gs_place_charm(g, x, taken);

        if (gs_int(0, 3) == 0 && i + 1 < spec->charms_inv + spec->charms_stash) {
            dm_gear_cand *y = gs_new(g, DM_GEAR_K_CHARM); int id = (int)y->item.id; *y = *x; y->item.id = (uint32_t)id; y->inv = 0; i++;
            if (i < spec->charms_inv) gs_place_charm(g, y, taken);
        }
    }

    if (spec->sets) {
        gs_set s = gs_find_set(t, (int)(spec->seed % 3));
        static const int kinds[] = { DM_GEAR_K_AMULET, DM_GEAR_K_RING, DM_GEAR_K_ARMOR, DM_GEAR_K_HEAD, DM_GEAR_K_BELT };
        for (int i = 0; s.ok && i < spec->sets && i < s.nrows; i++) { dm_gear_cand *x = gs_item(g, kinds[i % 5], 1, 0); x->item.setitem = s.rows[i]; }
    }

    int aura = gs_stat(d, "item_aura");
    for (int i = 0; i < spec->unrated; i++) {
        dm_gear_cand *x = gs_item(g, i % 2 ? DM_GEAR_K_HEAD : DM_GEAR_K_WEAPON1, 1.2, 0);
        dm_gear_list_add(&x->item.own, aura, 98, 5); x->unrated = DM_GEAR_U_GRANT;
    }

    if (spec->mana) {
        int st = gs_stat(d, "manadrainmindam");
        for (int i = 0; i < p->ncand; i++) {
            dm_gear_cand *x = &g->cand[i];
            if ((x->kind == DM_GEAR_K_RING || x->kind == DM_GEAR_K_AMULET || x->kind == DM_GEAR_K_GLOVES) && gs_int(0, 1))
                dm_gear_list_add(&x->item.own, st, 0, gs_int(3, 9));
        }
    }
    for (int i = 0; i < p->ncand; i++) if (g->cand[i].kind == DM_GEAR_K_CHARM && g->cand[i].item.req_lvl > c->clvl) g->cand[i].unrated |= DM_GEAR_U_LEVEL;
    if (spec->claws) {
        int keep = -1;
        for (int i = 0; spec->claws == 2 && i < p->ncand; i++) if (g->cand[i].kind == DM_GEAR_K_WEAPON1 && !g->cand[i].worn) keep = i;
        for (int i = 0; i < p->ncand; i++) {
            if (g->cand[i].kind != DM_GEAR_K_WEAPON1 || i == keep || g->cand[i].weapon < 0) continue;
            dm_sim_weapon *w = &g->weapon[g->cand[i].weapon].w;
            snprintf(w->wclass, sizeof w->wclass, "HT1"); w->kind = DM_WK_CLAW; snprintf(w->stype, sizeof w->stype, "%s", i % 2 ? "h2h" : "h2h2");
        }
    }
    return gs_capture(g);
}

static void gs_cb(gstock *g, const int *tot, int tohitpct, int acpct, int main, int off) {
    dm_gear_problem *p = &g->p; const dm_sim_char *c = &p->now; const dm_sim_data *d = p->d;
    const dm_gear_weapon *wf = &g->weapon[main >= 0 ? g->cand[main].weapon : 0];
    dm_ic_cb *v = &p->cb; memset(v, 0, sizeof *v);
    v->str = tot[0]; v->dex = tot[2]; v->lvl = c->clvl; v->thf = d->cls[c->cls].tohitf; v->bf = d->cls[c->cls].blockf; v->expansion = 1;
    v->ac = tot[31]; v->acpct = acpct; v->acovr = tot[182];
    v->toblock = tot[20]; v->can_block = off >= 0 && g->cand[off].block; v->tohit = tot[19]; v->tohitpct = tohitpct;

    v->armed = main >= 0; v->mode2 = wf->mode2 && !(off >= 0 && wf->has_one); v->srcdam = 128; v->strb = wf->strb; v->dexb = wf->dexb;
    for (int h = 0; h < 2; h++) { v->mn[h] = tot[21 + 2 * h]; v->mx[h] = tot[22 + 2 * h]; }
    v->bmn = tot[185]; v->bmx = tot[186]; v->dpct = tot[25]; v->edmin = tot[18]; v->edmax = tot[17];
    for (int i = 0; i < 11; i++) v->el[i] = tot[48 + i];
    for (int i = 0; i < 4; i++) v->mast[i] = tot[329 + i];
    v->plen_ovr = tot[101]; v->plen = tot[59]; v->pcnt = tot[326];
}




static int gs_capture(gstock *g) {
    dm_gear_problem *p = &g->p; dm_sim_char *c = &p->now; const dm_sim_data *d = p->d;
    p->idle = 0;
    if (dm_gear_problem_ready(p)) { fprintf(stderr, "stock: problem not ready\n"); return -1; }
    const dm_gear_char *nc = &p->nowc;
    for (int i = 0; i < DM_GEAR_NSTAT; i++) if (d->stat_name[i][0]) c->stat_value[i] = nc->value[i];
    dm_gear_pick now; dm_gear_pick_now(p, &now);
    int main = now.at[DM_GEAR_P_MAIN], off = now.at[DM_GEAR_P_OFF];
    int tohitpct = dm_gear_list_get(&nc->raw, 119, 0), acpct = dm_gear_list_get(&nc->raw, 171, 0) + dm_gear_list_get(&nc->raw, 16, 0);
    gs_cb(g, nc->value, tohitpct, acpct, main, off);
    if (main < 0 || off < 0 || g->cand[main].weapon < 0 || g->cand[off].weapon < 0) return dm_gear_problem_ready(p);
    static dm_sim_char dual; static int want_sv[DM_GEAR_NSTAT], tot[DM_GEAR_NSTAT]; unsigned flags;
    if (dm_gear_problem_ready(p) || dm_gear_leaf_char(p, &now, &dual, NULL, &flags)) { fprintf(stderr, "stock: two weapons not rated (%#x)\n", flags); return -1; }
    memcpy(want_sv, c->stat_value, sizeof want_sv);
    memcpy(tot, nc->value, sizeof tot);
    const dm_gear_list *l = &g->cand[off].item.own;
    for (int k = 0; k < l->n; k++) {
        int st = l->v[k].stat; if (l->v[k].layer || !p->dr[st]) continue;
        tot[st] -= l->v[k].value; if (st == 119) tohitpct -= l->v[k].value; if (st == 171 || st == 16) acpct -= l->v[k].value;
    }
    *c = dual; p->idle = 2;
    gs_cb(g, tot, tohitpct, acpct, main, off);
    if (dm_gear_problem_ready(p)) { fprintf(stderr, "stock: problem with two weapons not ready\n"); return -1; }
    for (int i = 0; i < DM_GEAR_NSTAT; i++)
        if ((d->stat_name[i][0] && c->stat_value[i] + p->ref_sv[i] != want_sv[i]) || tot[i] + p->ref_cb[i] != nc->value[i]) {
            fprintf(stderr, "stock: the reference of two weapons is not the totals (stat %d)\n", i); return -1; }
    return 0;
}






#define GS_HAND_STOCKS 7
static inline int gs_hand_stock(gstock *g, const dm_sim_data *d, const dm_gear_tables *t, int which, uint64_t seed, int *focus) {
    static char prof[1024];
    static const char *const base = "level=70\ndiff=1\nlife=900\nmana=300\nstr=150\ndex=150\nar=2500\ndef=900\nres=15,60,60,60,60,0\n";
    static const char *const skills[GS_HAND_STOCKS] = { "class=4\nskill=147,15,15\nbar=147\n", "class=4\nskill=133,15,15\nbar=133\n", "class=4\nskill=147,15,15\nbar=147\n",
        "class=6\nskill=252,12,12\nskill=260,15,15\nbar=260\n", "class=6\nskill=259,15,15\nbar=259\n", "class=6\nskill=259,15,15\nbar=259\n",
        "class=0\nskill=12,15,15\nbar=12\n" };
    snprintf(prof, sizeof prof, "%s%s", skills[which], base);
    for (uint64_t k = 0; k < 30; k++) {
        gspec s; memset(&s, 0, sizeof s);
        s.profile = prof; s.act = 2; s.seed = seed + k; s.variants = 1; s.count[DM_GEAR_K_RING] = 1;
        s.count[DM_GEAR_K_WEAPON1] = which != 2;
        s.claws = which == 3;
        if (which == 6) { s.count[DM_GEAR_K_BOW] = 1; s.count[DM_GEAR_K_ARROWS] = 1; }
        if (gs_build(g, d, t, &s)) return -1;
        int shield = -1, worn = -1, other = -1;
        for (int q = 0; q < g->p.ncand; q++) {
            const dm_gear_cand *c = &g->cand[q];
            if (c->kind == DM_GEAR_K_SHIELD && c->worn) shield = q;
            if ((c->kind == DM_GEAR_K_WEAPON1 || c->kind == DM_GEAR_K_BOW) && c->worn) worn = q;
            if (c->kind == DM_GEAR_K_WEAPON1 && !c->worn) other = q;
        }
        if ((which != 6 && shield < 0) || worn < 0 || (which != 2 && other < 0)) continue;
        *focus = which == 6 ? worn : which == 2 ? -1 : other;
        if (which == 4 || which == 5) {
            dm_sim_weapon *w = &g->weapon[g->cand[other].weapon].w;
            snprintf(w->wclass, sizeof w->wclass, "HT1"); w->kind = DM_WK_CLAW; snprintf(w->stype, sizeof w->stype, "h2h");
            if (which == 5) { g->cand[worn].worn = 0; g->cand[worn].item.slot = 0; }
        }
        if (which == 6) { dm_gear_list_add(&g->cand[other].item.own, 21, 0, 60); dm_gear_list_add(&g->cand[other].item.own, 22, 0, 90); }
        return gs_capture(g);
    }
    return -1;
}
static char *gs_profile(const char *name) {
    char path[256]; snprintf(path, sizeof path, "sim_profiles/%s", name);
    size_t n; char *text = sim_read(path, &n);
    if (!text) { fprintf(stderr, "no profile %s\n", path); exit(2); }
    return text;
}
static dm_sim_data *gs_tables(dm_gear_tables *gt) {
    const char *sp = getenv("DM_SIM_DATA"), *gp = getenv("DM_GEAR_DATA");
    if (!sp || !*sp || !gp || !*gp) { fprintf(stderr, "DM_SIM_DATA and DM_GEAR_DATA name the tables\n"); exit(2); }
    size_t n; char *text = sim_read(sp, &n);

    dm_sim_data *d = calloc(1, sizeof *d);
    if (!d) exit(2);
    if (!text || dm_sim_load(d, text, n, "rotw")) { fprintf(stderr, "sim tables: %s\n", d->err); exit(2); }
    free(text);
    text = sim_read(gp, &n);
    if (!text || dm_gear_tables_parse(gt, text)) { fprintf(stderr, "gear tables unreadable\n"); exit(2); }
    free(text);
    return d;
}
#endif
