




#include "dm_gear_tab.h"
#include "dm_gear_pages.h"
#include "dm_itemjob.h"
const char *dm_vault_item_name(uint8_t *b, int kind, int row);
int dm_rw_owners(uint8_t *b, uint64_t pl, uint64_t us[3]);
int dm_pages_cur(void);
int dm_pages_count(void);
int dm_swapskill_wset(void);

#define GT_MAX 512
#define GT_PER_FRAME 12
#define GT_HOLD_NS 1500000000ull

typedef struct { uint64_t it; uint32_t id, sig, owner; int where, page, x, y, slot, inv; } gt_item;


#define GT_INV_MAX (DM_GEAR_GRID_W * DM_GEAR_GRID_H)
static dm_gear_inv_cells g_gt_inv[GT_INV_MAX];
static int g_gt_ninv;
static uint8_t g_gt_grid[DM_GEAR_GRID_H][DM_GEAR_GRID_W];
typedef struct { uint32_t id, sig; uint64_t psig; int kind, hasw; dm_gear_cand c; dm_gear_weapon w; dm_gtab_show show; } gt_prof;
static gt_prof *g_gt_cache;
static int g_gt_ncache;
static uint64_t g_gt_offered;
static unsigned g_gt_ends;
static unsigned g_gt_gen;
static dm_gear_snap *g_gt_held;
static uint64_t g_gt_nodes[DM_SIM_MAXSKILL];

static void gt_nodes(uint64_t pl) {
    memset(g_gt_nodes, 0, sizeof g_gt_nodes);
    uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0;
    for (int k = 0; nd && k < 1024; k++, nd = *(uint64_t *)(nd + 8)) {
        uint64_t r = *(uint64_t *)nd; if (!r || *(int32_t *)(nd + 0x44) != -1) continue;
        int id = *(int16_t *)r; if (id >= 0 && id < DM_SIM_MAXSKILL && !g_gt_nodes[id]) g_gt_nodes[id] = nd;
    }
}

static const int k_gt_slot[DM_GEAR_P_N] = { 4, 5, 3, 2, 6, 7, 1, 8, 9, 10 };
static int gt_mastery(uint8_t *b, uint64_t pl, uint64_t w, uint64_t node, int which) { return ((int (*)(uint64_t, uint64_t, uint64_t, int))(b + 0x5b2a30))(pl, w, node, which); }

static uint32_t gt_sig(uint64_t it, uint64_t d) {
    uint32_t h = sim_worn_stats(it) * 31u + *(uint32_t *)(d + 0x18) * 7u + *(uint32_t *)d;
    uint64_t c = first_item(it); for (int n = 0; c && n < 6; n++, c = next_item(c)) h += sim_worn_stats(c) * 31u + *(uint32_t *)(c + 4);
    return h;
}

static uint64_t gt_psig(uint8_t *b, uint64_t pl) {
    uint64_t h = (uint64_t)gstat(b, pl, 12, 0) * 1099511628211ull ^ *(uint32_t *)(pl + 4);
    uint64_t l = *(uint64_t *)(pl + 0x100), nd = l ? *(uint64_t *)l : 0;
    for (int k = 0; nd && k < 1024; k++, nd = *(uint64_t *)(nd + 8)) { uint64_t r = *(uint64_t *)nd; if (!r) continue;
        int lv = *(int32_t *)(nd + 0x38) + ((int (*)(uint64_t, uint64_t, int))(b + 0x5b1710))(pl, nd, 0);
        h = (h ^ ((uint64_t)(uint16_t)*(int16_t *)r << 40 | (uint64_t)(uint32_t)lv << 8 | (uint8_t)*(int32_t *)(nd + 0x44))) * 1099511628211ull; }
    return h;
}


static int gt_shared_tab(uint64_t pl, uint32_t unit) {
    uint64_t inv = *(uint64_t *)(pl + 0x90); int n = 0;
    if (!inv || *(uint32_t *)inv != 0x01020304) return 0;
    for (uint64_t en = *(uint64_t *)(inv + 0x68); en && n < 16; en = *(uint64_t *)(en + 0x10)) {
        if (!*(uint8_t *)(en + 9) || *(uint8_t *)(en + 0xa)) continue;
        n++; if (*(uint32_t *)en == unit) return n;
    }
    return 0;
}

static void gt_cells(uint8_t *b, uint64_t it, int *w, int *h) {
    uint64_t r = item_rec(b, it); *w = r ? *(uint8_t *)(r + 0x11e) : 1; *h = r ? *(uint8_t *)(r + 0x11f) : 1;
    if (*w < 1 || *w > DM_GEAR_GRID_W) *w = 1;
    if (*h < 1 || *h > DM_GEAR_GRID_H) *h = 1;
}

static int gt_collect(uint8_t *b, uint64_t pl, gt_item *out, int max, int *skipped, unsigned *tabs) {
    uint64_t us[3]; dm_rw_owners(b, pl, us); int n = 0, pcur = dm_pages_cur(); *skipped = 0; *tabs = 0;
    g_gt_ninv = 0;
    for (int o = 0; o < 3; o++) {
        if (!us[o]) continue;
        int tab = o ? gt_shared_tab(pl, *(uint32_t *)(us[o] + 8)) : 0;
        if (tab > 0 && tab <= 32) *tabs |= 1u << (tab - 1);
        uint64_t it = first_item(us[o]);
        for (int guard = 0; it && guard < 4096; guard++, it = next_item(it)) {
            uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            if (*(uint32_t *)it != 4) continue;
            int mode = (int)*(uint32_t *)(it + 0xc), where, slot = 0, page = 0;
            if (!o && mode == 1) { slot = *(uint8_t *)(d + 0x54); if (slot < 1 || slot > 10) continue; where = DM_GTAB_WORN; }
            else if (mode == 0) { int pg = *(uint8_t *)(d + 0x55); if (o) { where = DM_GTAB_SHARED; page = tab; } else if (pg == 0) where = DM_GTAB_INV; else if (pg == 4) { where = DM_GTAB_STASH; page = pcur + 1; } else continue; }
            else continue;

            dm_gear_inv_cells *cells = NULL;
            if (where == DM_GTAB_INV && g_gt_ninv < GT_INV_MAX) {
                uint64_t cp = *(uint64_t *)(it + 0x38); cells = &g_gt_inv[g_gt_ninv++];
                gt_cells(b, it, &cells->w, &cells->h);
                cells->x = cp ? *(int32_t *)(cp + 0x10) : -1; cells->y = cp ? *(int32_t *)(cp + 0x14) : -1; cells->charm = is_type(b, it, 13); cells->placed = 0;
            }
            uint32_t fl = *(uint32_t *)(d + 0x18);
            if (!(fl & 0x10) || (fl & 0x100)) { ++*skipped; continue; }
            if (n >= max) { ++*skipped; continue; }
            if (cells) cells->placed = cells->charm;
            gt_item *e = &out[n++]; memset(e, 0, sizeof *e);
            e->it = it; e->id = *(uint32_t *)(it + 8); e->sig = gt_sig(it, d); e->where = where; e->page = page; e->slot = slot; e->owner = *(uint32_t *)(us[o] + 8);
            e->inv = cells ? (int)(cells - g_gt_inv) : -1;
            uint64_t pp = *(uint64_t *)(it + 0x38); if (pp) { e->x = *(int32_t *)(pp + 0x10); e->y = *(int32_t *)(pp + 0x14); }
        }
    }
    dm_gear_grid(g_gt_grid, g_gt_inv, g_gt_ninv);
    return n;
}
static int gt_bodyloc(uint8_t *b, uint64_t it) {
    uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16) : 0; uint32_t ci = *(uint32_t *)(it + 4);
    if (!t || ci >= *(uint32_t *)(t + 0x15c8)) return 0;
    int16_t ty = *(int16_t *)(*(uint64_t *)(t + 0x15c0) + (uint64_t)ci * 0x1c0 + 0x12e);
    return ty >= 0 && (uint64_t)ty < *(uint64_t *)(t + 0x1370) ? *(uint8_t *)(*(uint64_t *)(t + 0x1368) + (uint64_t)ty * 0xe8 + 0xa) : 0;
}


static const int *gt_class_rows(uint8_t *b, uint64_t it) {
    static const char code[8][5] = { "amaz", "sorc", "necr", "pala", "barb", "drui", "assn", "warl" };
    static int rows[4][8]; static uint64_t seen[4];
    static const int none[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    uint8_t ver = *(uint8_t *)(it + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16) : 0;
    if (!t) return none;
    if (seen[ver] != t) {
        uint64_t recs = *(uint64_t *)(t + 0x1368), n = *(uint64_t *)(t + 0x1370);
        for (int k = 0; k < 8; k++) { rows[ver][k] = -1; for (uint64_t r = 0; recs && r < n && r < 1024 && rows[ver][k] < 0; r++) if (!memcmp((const void *)(recs + r * 0xe8), code[k], 4)) rows[ver][k] = (int)r; }
        seen[ver] = t;
        LOG("best gear: class item types of table set %d at rows %d %d %d %d %d %d %d %d", ver, rows[ver][0], rows[ver][1], rows[ver][2], rows[ver][3], rows[ver][4],
            rows[ver][5], rows[ver][6], rows[ver][7]);
    }
    return rows[ver];
}


static int gt_kind(uint8_t *b, uint64_t pl, uint64_t it) {
    if (is_type(b, it, 13)) return DM_GEAR_K_CHARM;
    switch (gt_bodyloc(b, it)) {
    case 1: return DM_GEAR_K_HEAD;
    case 2: return DM_GEAR_K_AMULET;
    case 3: return DM_GEAR_K_ARMOR;
    case 6: case 7: return DM_GEAR_K_RING;
    case 8: return DM_GEAR_K_BELT;
    case 9: return DM_GEAR_K_BOOTS;
    case 10: return DM_GEAR_K_GLOVES;
    case 4: case 5:
        if (is_type(b, it, 5)) return DM_GEAR_K_ARROWS;
        if (is_type(b, it, 6)) return DM_GEAR_K_BOLTS;
        if (is_type(b, it, 51)) return DM_GEAR_K_SHIELD;
        if (!is_type(b, it, 45)) return DM_GEAR_K_NONE;
        if (is_type(b, it, 27)) return DM_GEAR_K_BOW;
        if (is_type(b, it, 35)) return DM_GEAR_K_XBOW;
        { int r = wield(b, pl, it); return r == 2 || r == 3 ? DM_GEAR_K_WEAPON2 : DM_GEAR_K_WEAPON1; }
    }
    return DM_GEAR_K_NONE;
}



static int gt_cc(const char *s) { return ((uint8_t)s[0] == 0xc3 && (uint8_t)s[1] == 0xbf && s[2] == 'c' && s[3]) ? 3 : ((uint8_t)s[0] == 0xee && (uint8_t)s[1] == 0x81 && (uint8_t)s[2] == 0xbe && s[3]) ? 3 : 0; }
static void gt_name(uint8_t *b, uint64_t it, dm_gtab_show *w) {
    static char buf[1024];
    memset(buf, 0, sizeof buf);
    ((void (*)(uint64_t, char *))(b + 0x41a330))(it, buf);
    buf[sizeof buf - 1] = 0;
    const char *top = strrchr(buf, '\n'); top = top ? top + 1 : buf;
    char colour = 0; size_t n = 0;
    for (const char *p = buf; *p; ) {
        int k = gt_cc(p);
        if (k) { if (p < top || !n) colour = p[k]; p += k + 1; continue; }
        if (p >= top && n + 1 < sizeof w->name) w->name[n++] = *p;
        p++;
    }
    w->name[n] = 0; w->colour = colour;
    static int said; if (said++ < 12) { char e[256]; size_t k = 0; for (const char *p = buf; *p && k + 2 < sizeof e; p++) e[k++] = *p == '\n' ? '|' : (uint8_t)*p < 0x20 ? '?' : *p; e[k] = 0;
        LOG("best gear: item %u name builder \"%s\" -> \"%s\" colour %c", *(uint32_t *)(it + 8), e, w->name, colour ? colour : '-'); }
}

static void gt_clean(const char *s, char *out, size_t n) {
    const char *nl = strrchr(s, '\n'); if (nl) s = nl + 1;
    for (;;) { if ((uint8_t)s[0] == 0xc3 && (uint8_t)s[1] == 0xbf && s[2] == 'c' && s[3]) s += 4; else if ((uint8_t)s[0] == 0xee && (uint8_t)s[1] == 0x81 && (uint8_t)s[2] == 0xbe && s[3]) s += 4; else break; }
    snprintf(out, n, "%s", s);
}
static void gt_show(uint8_t *b, uint64_t it, uint64_t d, dm_gtab_show *w) {
    uint32_t code = item_code(b, it); char key[5] = { 0 }; memcpy(key, &code, 4); for (int i = 3; i >= 0 && (key[i] == ' ' || !key[i]); i--) key[i] = 0;
    snprintf(w->code, sizeof w->code, "%s", key);
    int q = (int)*(uint32_t *)d, row = (int)(*(uint32_t *)(d + 0x34) & 0xffff);
    w->quality = *(uint32_t *)(d + 0x18) & 0x04000000u ? 9 : q;
    w->id = *(uint32_t *)(it + 8);
    w->row = q == 5 || q == 7 ? row : -1;


    uint8_t gfx = *(uint8_t *)(d + 0x5e); w->gfx = gfx < 0xf0 ? gfx : -1;
    gt_name(b, it, w);
    if (!w->name[0]) {
        const char *name = q == 7 ? dm_vault_item_name(b, 1, row) : q == 5 ? dm_vault_item_name(b, 0, row) : NULL;
        if (!name) name = raw_str(b, key);
        gt_clean(name ? name : key, w->name, sizeof w->name);
    }
}

static int gt_profile(uint8_t *b, uint64_t pl, const gt_item *e, const dm_gear_tables *t, int cls, int clvl, gt_prof *g) {
    memset(&g->c, 0, sizeof g->c); memset(&g->w, 0, sizeof g->w); g->hasw = 0;
    uint64_t it = e->it, d = *(uint64_t *)(it + 0x10);
    int kind = gt_kind(b, pl, it); g->kind = kind;

    if (kind == DM_GEAR_K_NONE || (kind == DM_GEAR_K_CHARM && e->where == DM_GTAB_WORN)) return -1;
    dm_gear_cand *c = &g->c;
    if (gear_profile(b, pl, it, e->slot, t, &c->item)) return -1;
    c->kind = kind; c->w = c->h = 1; c->weapon = -1;
    const int *cr = gt_class_rows(b, it);
    for (int k = 0; k < 8 && !c->cls_only; k++) if (cr[k] >= 0 && is_type(b, it, cr[k])) c->cls_only = k + 1;
    if (raw_has(it, 151) || raw_has(it, 97) || raw_has(it, 204) || raw_has(it, 126)) c->unrated |= DM_GEAR_U_GRANT;
    for (int st = 195; st <= 201; st++) if (raw_has(it, st)) c->unrated |= DM_GEAR_U_GRANT;
    if (kind == DM_GEAR_K_CHARM) {
        gt_cells(b, it, &c->w, &c->h);
        if (c->item.req_str || c->item.req_dex) c->unrated |= DM_GEAR_U_CHARMREQ;
        if (c->item.req_lvl > clvl) c->unrated |= DM_GEAR_U_LEVEL;
    }
    if (kind == DM_GEAR_K_SHIELD) {
        c->block = !hand_off(it);
        const sim_gear *row = sim_gear_row(b, it); if (row && !strcmp(row->one, "-")) { c->dmin = row->min; c->dmax = row->max; }
    }
    if (kind == DM_GEAR_K_BOOTS) { const sim_gear *row = sim_gear_row(b, it); if (row) { c->dmin = row->min; c->dmax = row->max; } }
    if (kind >= DM_GEAR_K_WEAPON1 && kind <= DM_GEAR_K_XBOW) {
        dm_gear_weapon *w = &g->w; int r = wield(b, pl, it), mode = (r | 1) == 3 ? 2 : 1;
        if (!sim_weapon(b, it, mode, cls, &w->w)) c->unrated |= DM_GEAR_U_FACTS;
        else {
            w->mode2 = mode == 2;
            uint64_t wr = item_rec(b, it); if (wr) { w->strb = *(int16_t *)(wr + 0x116); w->dexb = *(int16_t *)(wr + 0x118); w->ar_zero = *(uint16_t *)(wr + 0x12e) == 38; }
            w->flat = lstat_(b, it, 111); w->mcrit = gt_mastery(b, pl, it, 0, 2);
            for (int id = 0; id < DM_SIM_MAXSKILL; id++) { uint64_t nd = g_gt_nodes[id]; w->mth[id] = w->mdmg[id] = DM_SIM_NO_NODE;
                if (nd && *(uint64_t *)nd) { w->mth[id] = (int16_t)gt_mastery(b, pl, it, nd, 0); w->mdmg[id] = (int16_t)gt_mastery(b, pl, it, nd, 1); } }
            g->hasw = 1;
        }
    }
    gt_show(b, it, d, &g->show);
    return 0;
}
static gt_prof *gt_cached(uint32_t id, uint32_t sig, uint64_t psig) {
    for (int i = 0; i < g_gt_ncache; i++) if (g_gt_cache[i].id == id && g_gt_cache[i].sig == sig && g_gt_cache[i].psig == psig) return &g_gt_cache[i];
    return NULL;
}



#define GT_PG_MAX DM_GPG_SEL_MAX
static dm_gpg_item g_gt_pg[GT_PG_MAX];
static int g_gt_npg;
static uint64_t g_gt_pgrev = ~0ull;
static uint32_t g_gt_tabs[DM_GPG_TABS];
static void gt_pages(uint64_t pl, uint64_t psig, int pcur, unsigned tabs_read, dm_gpg_sel *o) {
    dm_gpg_state st; uint64_t ps = 0; uint32_t tab[DM_GPG_TABS];
    int known = dm_gear_pages_info(&st, &ps, tab);
    memcpy(g_gt_tabs, tab, sizeof g_gt_tabs); if (!known) memset(g_gt_tabs, 0, sizeof g_gt_tabs);
    if (!known || ps != psig) {
        o->n = o->skipped = 0; o->pages = dm_pages_count() - 1; o->shared = 1; o->busy = 1; return; }
    if (st.rev != g_gt_pgrev) { g_gt_npg = dm_gear_pages_get(g_gt_pg, GT_PG_MAX, &st); g_gt_pgrev = st.rev; }
    int tabno[DM_GPG_TABS];
    for (int t = 0; t < DM_GPG_TABS; t++) tabno[t] = tab[t] ? gt_shared_tab(pl, tab[t]) : 0;
    dm_gpg_select(&st, g_gt_pg, g_gt_npg, pcur, tabno, tabs_read, o);
    static int said; static uint64_t said_rev;
    if (said < 8 && said_rev != st.rev) { said++; said_rev = st.rev;
        char m[160]; size_t k = 0; m[0] = 0;
        for (int t = 0; t < DM_GPG_TABS && tab[t] && k < sizeof m - 16; t++) k += (size_t)snprintf(m + k, sizeof m - k, " %u:%d", tab[t], tabno[t]);
        LOG("best gear: other stash pages and shared tabs give %d candidates, %d pages and %s shared tabs not read; tabs (unit:number)%s, read here %#x",
            o->n, o->pages, o->shared ? "some" : "no", m, tabs_read); }
}

typedef struct { uint8_t *b; uint64_t pl; const gt_item *items; int n, skipped; uint64_t psig, rev; dm_gear_snap *s; double ms[3]; int charms; const dm_gpg_sel *pg; } gt_build_ctx;

static int gt_build(const dm_sim_data *d, gt_build_ctx *q) {
    uint8_t *b = q->b; uint64_t pl = q->pl; dm_gear_snap *s = q->s; dm_gear_problem *p = &s->p;
    const dm_gear_tables *t = gear_tables(); if (!t) { s->state = DM_GTAB_NOSIM; return 0; }
    uint64_t t0 = now_ns();
    static gear_swap_out gs; gear_swap(b, pl, 0, NULL, 0, 0, &gs);
    q->ms[0] = (now_ns() - t0) / 1e6;
    if (gs.ok <= 0) { s->state = DM_GTAB_NOREF; static int said; if (!said++) LOG("ASSUME bestgear.compose: worn gear not composable (flags %#x)", gs.flags); return 0; }
    sim_capture *c = calloc(1, sizeof *c); if (!c) { s->state = DM_GTAB_NOSIM; return 0; }
    c->b = b; c->pl = pl;
    int rc = sim_capture_read(d, c); unsigned bad = c->now.c.capture_bad;
    if (rc || !c->now.valid || ((bad & 1) && !(bad & 16))) { s->state = DM_GTAB_NOSIM; free(c); return 0; }
    p->now = c->now.c;
    cb_res R; cb_snapshot(b, pl, c->hands[0], c->hands[1], &R); p->cb = R.now;
    free(c);
    q->ms[1] = (now_ns() - t0) / 1e6 - q->ms[0];
    p->base = gs.base; p->t = t; p->unarmed = 0; p->splash_on = dm_custom_splash_on();
    int cls = p->now.cls;
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) {
        uint64_t nd = g_gt_nodes[id], rec = nd ? *(uint64_t *)nd : 0;
        p->node[id] = rec && d->skill[id]; p->prog[id] = rec && (*(uint8_t *)(rec + 0x24) & 8); p->tab[id] = p->elem[id] = -1;
    }

    for (int j = 0; j < D.nsk; j++) if (D.sk[j].id >= 0 && D.sk[j].id < DM_SIM_MAXSKILL && D.sk[j].cls == cls && D.sk[j].page >= 1 && D.sk[j].page <= 3)
        p->tab[D.sk[j].id] = (int16_t)(cls * 8 + D.sk[j].page - 1);

    dm_gear_weapon *un = &s->weapon[0]; memset(un, 0, sizeof *un);
    if (!sim_weapon(b, 0, 1, cls, &un->w)) { s->state = DM_GTAB_NOSIM; return 0; }
    for (int id = 0; id < DM_SIM_MAXSKILL; id++) { uint64_t nd = g_gt_nodes[id]; un->mth[id] = un->mdmg[id] = DM_SIM_NO_NODE;
        if (nd && *(uint64_t *)nd) { un->mth[id] = (int16_t)gt_mastery(b, pl, 0, nd, 0); un->mdmg[id] = (int16_t)gt_mastery(b, pl, 0, nd, 1); } }
    p->nweapon = 1; p->ncand = 0;
    for (int k = 0; k < g_gt_ninv; k++) g_gt_inv[k].placed = 0;
    for (int i = 0; i < q->n; i++) {
        const gt_item *e = &q->items[i]; gt_prof *g = gt_cached(e->id, e->sig, q->psig);
        if (!g || g->kind == DM_GEAR_K_NONE || p->ncand >= DM_GEAR_MAXCAND) continue;
        if (g->kind == DM_GEAR_K_CHARM && !q->charms) continue;
        dm_gear_cand *x = &s->cand[p->ncand]; *x = g->c;
        x->worn = e->where == DM_GTAB_WORN; x->item.id = e->id;
        if (g->kind == DM_GEAR_K_CHARM && e->where == DM_GTAB_INV) {
            if (e->x < 0 || e->y < 0 || e->x + x->w > DM_GEAR_GRID_W || e->y + x->h > DM_GEAR_GRID_H) continue;
            x->inv = 1; x->x = e->x; x->y = e->y;

            if (e->inv >= 0 && !(x->unrated & DM_GEAR_U_LEVEL)) g_gt_inv[e->inv].placed = 1;
        }
        if (g->hasw) { s->weapon[p->nweapon] = g->w; x->weapon = p->nweapon++; }
        s->show[p->ncand] = g->show; s->show[p->ncand].where = e->where; s->show[p->ncand].page = e->page; s->show[p->ncand].id = e->id;



        if (e->where == DM_GTAB_STASH || e->where == DM_GTAB_SHARED) s->show[p->ncand].id = dm_gear_pages_row_name(e->id, &s->show[p->ncand].vhash);
        p->ncand++;
    }


    for (int i = 0; q->pg && i < q->pg->n && p->ncand < DM_GEAR_MAXCAND; i++) {
        const dm_gpg_item *e = &g_gt_pg[q->pg->idx[i]];
        if (e->p.kind == DM_GEAR_K_NONE || (e->p.kind == DM_GEAR_K_CHARM && !q->charms)) continue;
        dm_gear_cand *x = &s->cand[p->ncand]; *x = e->p.c;
        x->worn = 0; x->inv = 0; x->item.id = 0; x->weapon = -1;
        if (e->p.hasw) { s->weapon[p->nweapon] = e->p.w; x->weapon = p->nweapon++; }
        s->show[p->ncand] = e->p.show; s->show[p->ncand].where = e->where == DM_GPG_PERSONAL ? DM_GTAB_STASH : DM_GTAB_SHARED;
        int place = e->where == DM_GPG_PERSONAL ? e->index : DM_GPG_PAGES + e->index;
        s->show[p->ncand].page = q->pg->page[i]; s->show[p->ncand].id = dm_gpg_stock_id(place, e->x, e->y);
        s->show[p->ncand].vhash = e->hash;
        p->ncand++;
    }

    static int listed; for (int i = 0; !listed && i < p->ncand && i < 48; i++) { const dm_gear_cand *x = &s->cand[i];
        LOG("best gear: candidate %d kind %d worn %d where %d unrated %#x weapon %d req %d/%d/%d values %d set %d", i, x->kind, x->worn, s->show[i].where, x->unrated, x->weapon,
            x->item.req_str, x->item.req_dex, x->item.req_lvl, x->item.own.n, x->item.setitem); }
    listed = 1;



    p->charms_fixed = !q->charms;
    dm_gear_grid(p->grid, g_gt_inv, g_gt_ninv);
    if (q->charms) {
        static dm_gear_item ch; memset(&ch, 0, sizeof ch); ch.setitem = -1; int any = 0;
        for (int i = 0; i < p->ncand; i++) { const dm_gear_cand *x = &s->cand[i];
            if (x->kind != DM_GEAR_K_CHARM || !x->inv || (x->unrated & DM_GEAR_U_LEVEL)) continue;
            for (int k = 0; k < x->item.own.n; k++) dm_gear_list_add(&ch.own, x->item.own.v[k].stat, x->item.own.v[k].layer, x->item.own.v[k].value);
            if (x->item.own.full) ch.own.full = 1;
            any = 1; }
        static dm_gear_char cc; dm_gear_base empty = p->base; memset(&empty.base, 0, sizeof empty.base);
        if (any && (ch.own.full || dm_gear_compose(t, &empty, &ch, 1, NULL, &cc))) { s->state = DM_GTAB_NOREF; static int said; if (!said++) LOG("ASSUME bestgear.charms: carried charms not composable"); return 0; }
        if (any) {
            for (int st = 0; st < DM_GEAR_NSTAT; st++) if (cc.value[st] && st != 6 && st != 8 && st != 10) dm_gear_list_add(&p->base.base, st, 0, -cc.value[st]);
            for (int i = 0; i < cc.raw.n; i++) if (cc.raw.v[i].layer) dm_gear_list_add(&p->base.base, cc.raw.v[i].stat, cc.raw.v[i].layer, -cc.raw.v[i].value);
            if (p->base.base.full) { s->state = DM_GTAB_NOREF; return 0; }
        }
    }
    for (int k = 0; k < DM_GEAR_P_N; k++) gt_clean(slot_name(b, k_gt_slot[k]), s->slot[k], sizeof s->slot[k]);
    static const char *const measure[DM_GEAR_M_N] = { "dmGearGroup", "dmGearSingle", "dmGearSurvival", "dmGearRecovery" };
    for (int k = 0; k < DM_GEAR_M_N; k++) snprintf(s->measure[k], sizeof s->measure[k], "%s", dm_str(b, measure[k]));
    snprintf(s->missed, sizeof s->missed, "%s", dm_str(b, "dmGearMissed")); snprintf(s->reached, sizeof s->reached, "%s", dm_str(b, "dmGearReached"));
    snprintf(s->below, sizeof s->below, "%s", dm_str(b, "dmGearBelow")); snprintf(s->below_same, sizeof s->below_same, "%s", dm_str(b, "dmGearBelowSame"));
    int lang = dm_lang_group(); s->comma = lang == DM_LG_DE || lang == DM_LG_PT;
    dm_gear_goal_get(dm_build_session(), &s->goal);
    q->ms[2] = (now_ns() - t0) / 1e6 - q->ms[0] - q->ms[1];
    return 0;
}




static dm_gpg_id g_gt_ids[GT_MAX + DM_GPG_SEL_MAX];
static gt_item g_gt_unres;
static uint64_t gt_rev(uint8_t *b, uint64_t pl, const gt_item *items, int n, uint64_t psig, int charms, const dm_gpg_sel *pg, int *unresolved) {
    g_gt_gen = dm_sim_generation();
    uint64_t h = (psig ^ (uint64_t)g_gt_gen << 48) + (uint64_t)charms;
    h = (h ^ (uint64_t)dm_custom_splash_on()) * 1099511628211ull;

    h = (h ^ g_gt_ends) * 1099511628211ull;
    for (int y = 0; y < DM_GEAR_GRID_H; y++) for (int x = 0; x < DM_GEAR_GRID_W; x++) h = (h ^ g_gt_grid[y][x]) * 1099511628211ull;
    int nid = 0; *unresolved = 0;
    for (int i = 0; i < n; i++) { const gt_item *e = &items[i];
        int stock = e->where == DM_GTAB_STASH;
        for (int t = 0; e->where == DM_GTAB_SHARED && t < DM_GPG_TABS && !stock; t++) stock = g_gt_tabs[t] && g_gt_tabs[t] == e->owner;
        if (stock) {
            uint64_t vh = 0; int rc = dm_gear_pages_unit_hash(e->id, &vh);
            if (rc == 1) { g_gt_ids[nid++] = (dm_gpg_id){ e->where, e->page, e->x, e->y, vh }; continue; }
            if (rc == 2) continue;
            if (!(*unresolved)++) g_gt_unres = *e;
        }
        h = (h ^ ((uint64_t)e->id << 32 | e->sig)) * 1099511628211ull; h = (h ^ (uint64_t)(e->where | e->page << 4 | e->x << 12 | e->y << 18 | e->slot << 24)) * 1099511628211ull; }
    for (int i = 0; i < pg->n; i++) { const dm_gpg_item *e = &g_gt_pg[pg->idx[i]];
        g_gt_ids[nid++] = (dm_gpg_id){ e->where == DM_GPG_PERSONAL ? DM_GTAB_STASH : DM_GTAB_SHARED, pg->page[i], e->x, e->y, e->hash }; }
    h = dm_gpg_ids_hash(g_gt_ids, nid, h);
    h = (h ^ (uint64_t)(pg->pages | pg->shared << 8)) * 1099511628211ull;
    h = list_hash(pl, h);
    { uint64_t list = *(uint64_t *)(pl + 0x100); for (int i = 0; list && i < 2; i++) { uint64_t node = *(uint64_t *)(list + 8 + 8 * i), record = node ? *(uint64_t *)node : 0;
        h = (h ^ (record ? (uint16_t)*(int16_t *)record : 65535)) * 1099511628211ull; } }
    for (int i = 0; i < sim_nbound; i++) h = (h ^ (unsigned)sim_bound[i]) * 1099511628211ull;
    dm_gear_goal g; dm_gear_goal_get(dm_build_session(), &g); h = (h ^ (uint64_t)lround(g.survival_hits * 1000 + g.boss_share * 100)) * 1099511628211ull;
    h = (h ^ (uint64_t)(*(uint8_t *)(b + 0x2d20b88) | dm_unit_level(pl) << 4 | dm_lang_group() << 16 | (uint64_t)*(uint8_t *)(pl + 0x1bd) << 24)) * 1099511628211ull;
    return h ? h : 1;
}

static void gt_offer(dm_gear_snap *s, int *soon) {
    uint64_t rev = s->rev;
    if (dm_gear_tab_offer(s)) { g_gt_held = s; *soon = 1; return; }
    g_gt_held = NULL; g_gt_offered = rev;
}
static void gt_gfree(uint8_t *b, void *p) { if (!(*(volatile uint8_t *)(b + 0x3211700) & 1)) return; uint64_t a = *(uint64_t *)(b + 0x32116f8); ((void (*)(uint64_t, void *))(*(uint64_t *)(*(uint64_t *)a + 0x28)))(a, p); }







typedef struct { uint8_t *b; uint64_t pl; uint32_t id; int place, ok; const dm_gtab_pkg *pkg; } gt_tip_ctx;
static void gt_tip_item(uint64_t it, void *arg) {
    gt_tip_ctx *q = arg; uint8_t *b = q->b; uint64_t pl = q->pl;
    if (!it || *(uint32_t *)it != 4 || !*(uint64_t *)(it + 0x10)) return;
    typedef struct { char *p; uint64_t len, cap; } gstr_t;
    struct { uint64_t n; uint8_t has; } c0 = { 0, 0 };
    gstr_t g = ((gstr_t (*)(uint64_t, uint64_t, uint64_t, int, int, int, void *, void *))(b + 0x374f00))(pl, pl, it, 0, 1, 0, NULL, &c0);
    static char desc[8192], out[24576];
    size_t k = g.p ? (g.len < sizeof desc - 1 ? g.len : sizeof desc - 1) : 0;
    if (g.p) memcpy(desc, g.p, k);
    desc[k] = 0;
    if (g.p && !(g.cap >> 63)) gt_gfree(b, g.p);

    dm_tipx_ctx c = { .b = b, .widget = 0, .item = it, .player = pl, .producer = DM_TIPX_CELL, .pad = 0, .ident = q->id & DM_GTAB_STOCK_ID ? q->id : 0,
                      .slot = q->place >= 0 && q->place < DM_GEAR_P_N ? k_gt_slot[q->place] : 0, .pkg = q->pkg };
    static dm_tipx_line ln[DM_TIPX_MAXL]; uint64_t key = 0;
    int n = dm_itemcmp_block(&c, ln, DM_TIPX_MAXL, &key), dots = n > 0 ? dm_itemcmp_dots() : 0;
    size_t len = 0; out[0] = 0;
    for (int i = 0; i < n; i++) { int w = snprintf(out + len, sizeof out - len, "%s%s", i ? "\n" : "", ln[i].s); if (w < 0 || (size_t)w >= sizeof out - len) break; len += (size_t)w; }
    uint64_t h = 1469598103934665603ull;
    for (const char *p = desc; *p; p++) h = (h ^ (uint8_t)*p) * 1099511628211ull;
    for (const char *p = out; *p; p++) h = (h ^ (uint8_t)*p) * 1099511628211ull;
    h = (h ^ (uint64_t)(dots + 8)) * 1099511628211ull;
    static uint32_t put_id; static int put_place; static uint64_t put_h;
    if (q->id == put_id && q->place == put_place && h == put_h) { q->ok = 1; return; }
    if (!dm_gear_tab_tip_put(q->id, q->place, desc, out, dots)) { put_id = q->id; put_place = q->place; put_h = h; q->ok = 1; }
}
static void gt_tip(uint8_t *b, uint64_t pl) {
    static uint32_t done; static int done_place, every;
    int place = -1; uint32_t id = dm_gear_tab_tip_wanted(&place);
    if (!id || !pl) { done = 0; return; }
    if (id == done && place == done_place && ++every % 10) return;


    static dm_gtab_pkg pkg; static uint32_t wait_id; static int wait_place, waited;
    const dm_gtab_pkg *use = NULL;
    if (place >= 0) {
        if (wait_id != id || wait_place != place) { wait_id = id; wait_place = place; waited = 0; }
        if (dm_gear_tab_tip_pkg(id, place, &pkg) == 1) use = &pkg;
        else if (++waited < 60) return;
        else if (waited == 60) LOG("ASSUME bestgear.pkg_late: no package for row %#x place %d after 60 frames; compared with the gear worn now", id, place);
    }
    uint64_t t0 = now_ns();
    gt_tip_ctx q = { b, pl, id, place, 0, use };
    if (id & DM_GTAB_STOCK_ID) {
        uint64_t vh = 0;
        if (dm_gear_tab_tip_hash(id, &vh) && dm_gear_pages_with(b, (int)(id >> 16 & 0xff), (int)(id & 0xff), (int)(id >> 8 & 0xff), vh, gt_tip_item, &q)) {
            static uint32_t said_id; if (said_id != id) { said_id = id; LOG("best gear: the item of row %#x is no longer where the row found it; no tooltip", id); } }
    } else gt_tip_item(item_by_id(b, id), &q);
    dm_gear_frame_spend(now_ns() - t0);
    if (q.ok) { done = id; done_place = place; every = 0; }
}
static void gear_tab_pump(uint8_t *b, uint64_t pl) {


    g_gt_ends = dm_itemjob_ends(); dm_gear_tab_jobs(g_gt_ends);
    if (!dm_gear_tab_wanted()) return;
    gt_tip(b, pl);
    static int tick, soon;
    if (dm_itemjob_owner()) { soon = 1; return; }
    if (!soon && ++tick % 10) return;
    soon = 0;
    if (g_gt_held) { gt_offer(g_gt_held, &soon); return; }
    if (!pl || !*(uint64_t *)(pl + 0x88)) return;
    if (dm_swapskill_wset() != 0) {
        if (g_gt_offered != 2) { dm_gear_snap *s = dm_gear_snap_new(0, 1); if (s) { s->rev = 2; s->state = DM_GTAB_SET2; s->jobs = g_gt_ends; gt_offer(s, &soon); } }
        return;
    }
    static gt_item items[GT_MAX]; int skipped = 0; unsigned tabs = 0;
    int n = gt_collect(b, pl, items, GT_MAX, &skipped, &tabs);
    int charms = dm_gear_tab_charms_wanted();
    static dm_gpg_sel pg;
    uint64_t psig = gt_psig(b, pl);
    gt_pages(pl, psig, dm_pages_cur(), tabs, &pg);
    int unresolved = 0; uint64_t rev = gt_rev(b, pl, items, n, psig, charms, &pg, &unresolved);
    static uint64_t hold;
    if (rev == g_gt_offered) { hold = 0; return; }
    const dm_gear_tables *t = gear_tables();
    if (!g_gt_cache) { g_gt_cache = calloc(GT_MAX, sizeof *g_gt_cache); if (!g_gt_cache) return; }

    uint64_t t0 = now_ns(), spent = dm_gear_frame_spent(); int made = 0, missing = 0, nodes = 0;
    for (int i = 0; i < n; i++) {
        if (gt_cached(items[i].id, items[i].sig, psig)) continue;
        if (made >= GT_PER_FRAME || (made ? spent + now_ns() - t0 >= DM_GEAR_FRAME_NS : spent >= DM_GEAR_FRAME_NS / 2) || !t) { missing++; continue; }

        int slot = -1;
        if (g_gt_ncache < GT_MAX) slot = g_gt_ncache++;
        else for (int k = 0; k < g_gt_ncache && slot < 0; k++) { int used = 0; for (int j = 0; j < n && !used; j++) used = g_gt_cache[k].id == items[j].id && g_gt_cache[k].sig == items[j].sig && g_gt_cache[k].psig == psig; if (!used) slot = k; }
        if (slot < 0) { missing++; continue; }
        if (!nodes) { gt_nodes(pl); nodes = 1; }
        gt_prof *g = &g_gt_cache[slot];
        if (gt_profile(b, pl, &items[i], t, (int)*(uint32_t *)(pl + 4), gstat(b, pl, 12, 0), g)) g->kind = DM_GEAR_K_NONE;
        g->id = items[i].id; g->sig = items[i].sig; g->psig = psig; made++;
    }
    dm_gear_frame_spend(now_ns() - t0);
    if (made) { static int said; if (said++ < 8) LOG("best gear: %d profiles in %.2f ms, %d to go", made, (now_ns() - t0) / 1e6, missing); soon = 1; return; }
    if (missing) { if (t) soon = 1; return; }


    int reading = pg.busy || unresolved || dm_gear_pages_answered() != dm_gear_pages_asked();
    if (reading) { uint64_t now = now_ns(); if (!hold) hold = now; if (now - hold < GT_HOLD_NS) return;
        static int said; if (said++ < 8) LOG("best gear: the stash page stock is still reading after %.1f s (%d places, %d items of its own not read yet, the first unit %u where %d page %d stash unit %u); searched without them",
                                             (now - hold) / 1e9, pg.busy, unresolved, unresolved ? g_gt_unres.id : 0, g_gt_unres.where, g_gt_unres.page, g_gt_unres.owner); }
    hold = 0;
    int ncand = 0, nw = 1; for (int i = 0; i < n; i++) { gt_prof *g = gt_cached(items[i].id, items[i].sig, psig); if (g && g->kind != DM_GEAR_K_NONE) { ncand++; nw += g->hasw; } }
    for (int i = 0; i < pg.n; i++) { ncand++; nw += g_gt_pg[pg.idx[i]].p.hasw; }

    dm_gear_snap *s = dm_gear_snap_new(ncand, nw); if (!s) return;
    int busy = 0; const char *set = *(uint8_t *)(pl + 0x1bd) == 3 ? "rotw" : "lod";
    const dm_sim_data *d = dm_sim_try_pin(set, &busy);
    if (busy) { dm_gear_snap_free(s); soon = 1; return; }
    if (d && d->generation != g_gt_gen) rev = gt_rev(b, pl, items, n, psig, charms, &pg, &unresolved);
    s->rev = rev; s->skipped = skipped + pg.skipped; snprintf(s->set, sizeof s->set, "%s", set); s->tables = d; s->p.d = d;
    s->stash_pages = pg.pages; s->shared_tabs = pg.shared; s->jobs = g_gt_ends;
    gt_build_ctx q = { b, pl, items, n, skipped, psig, rev, s, { 0, 0, 0 }, charms, &pg };
    uint64_t t1 = now_ns();
    if (!d) s->state = DM_GTAB_NOSIM;
    else { gt_nodes(pl); gt_build(d, &q); }
    { static int said; if (said++ < 8) LOG("best gear: snapshot %016llx with %d candidates, %d weapons, state %d, built in %.2f ms (worn gear %.2f, capture %.2f, rest %.2f)",
                                           (unsigned long long)rev, s->p.ncand, s->p.nweapon, s->state, (now_ns() - t1) / 1e6, q.ms[0], q.ms[1], q.ms[2]); }
    gt_offer(s, &soon);
}
