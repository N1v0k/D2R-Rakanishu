
#include <string.h>
#include "dm_gear_order.h"


static const uint16_t k_group[DM_GO_MAXCHANGE] = { 1u << 1, 1u << 2, 1u << 3, 1u << 4 | 1u << 5, 1u << 6, 1u << 7, 1u << 8, 1u << 9, 1u << 10 };

void dm_gear_order_worn(const dm_go_world *w, int8_t *worn) {
    for (int s = 0; s < DM_GO_SLOTS; s++) worn[s] = -1;
    for (int i = 0; i < w->nitem; i++) if (w->item[i].slot > 0 && w->item[i].slot < DM_GO_SLOTS) worn[w->item[i].slot] = (int8_t)i;
}

static uint16_t row_mask(int x, int w) { return (uint16_t)(((1u << w) - 1u) << x); }
static int rect_in(const dm_go_grid *g, int x, int y, int w, int h) { return x >= 0 && y >= 0 && w > 0 && h > 0 && x + w <= g->w && y + h <= g->h; }
static int rect_free(const dm_go_grid *g, int x, int y, int w, int h) {
    if (!rect_in(g, x, y, w, h)) return 0;
    for (int r = y; r < y + h; r++) if (g->used[r] & row_mask(x, w)) return 0;
    return 1;
}
static void rect_set(dm_go_grid *g, int x, int y, int w, int h, int on) {
    for (int r = y; r < y + h; r++) g->used[r] = on ? (uint16_t)(g->used[r] | row_mask(x, w)) : (uint16_t)(g->used[r] & ~row_mask(x, w));
}

int dm_gear_order_apply(dm_go_world *w, const dm_go_move *m, int *hand) {
    if (m->item < 0 || m->item >= w->nitem) return -1;
    dm_go_item *it = &w->item[m->item];
    int8_t worn[DM_GO_SLOTS]; dm_gear_order_worn(w, worn);
    switch (m->op) {
    case DM_GO_PICK_CELL:
        if (*hand >= 0 || it->slot || it->cell.grid != m->cell.grid || it->cell.x != m->cell.x || it->cell.y != m->cell.y) return -1;
        if (m->cell.grid < 0 || m->cell.grid >= w->ngrid) return -1;
        rect_set(&w->grid[m->cell.grid], m->cell.x, m->cell.y, it->w, it->h, 0); it->cell.grid = -1; *hand = m->item; return 0;
    case DM_GO_PUT_CELL:
        if (*hand != m->item || m->cell.grid < 0 || m->cell.grid >= w->ngrid || !rect_free(&w->grid[m->cell.grid], m->cell.x, m->cell.y, it->w, it->h)) return -1;
        rect_set(&w->grid[m->cell.grid], m->cell.x, m->cell.y, it->w, it->h, 1); it->cell = m->cell; it->slot = 0; *hand = -1; return 0;
    case DM_GO_PICK_SLOT:
        if (*hand >= 0 || m->slot <= 0 || m->slot >= DM_GO_SLOTS || it->slot != m->slot) return -1;
        it->slot = 0; it->cell.grid = -1; *hand = m->item; return 0;
    case DM_GO_PUT_SLOT:
        if (*hand != m->item || m->slot <= 0 || m->slot >= DM_GO_SLOTS || worn[m->slot] != m->swap) return -1;
        if (m->swap >= 0) { w->item[m->swap].slot = 0; w->item[m->swap].cell.grid = -1; }
        it->slot = m->slot; it->cell.grid = -1; *hand = m->swap; return 0;
    }
    return -1;
}

static dm_go_move undo_of(const dm_go_move *m) {
    dm_go_move u = *m;
    switch (m->op) {
    case DM_GO_PICK_CELL: u.op = DM_GO_PUT_CELL; break;
    case DM_GO_PUT_CELL: u.op = DM_GO_PICK_CELL; break;
    case DM_GO_PICK_SLOT: u.op = DM_GO_PUT_SLOT; u.swap = -1; break;
    case DM_GO_PUT_SLOT: if (m->swap >= 0) { u.item = m->swap; u.swap = m->item; } else { u.op = DM_GO_PICK_SLOT; u.swap = -1; } break;
    }
    return u;
}

typedef struct {
    dm_go_world w; int hand; int8_t worn[DM_GO_SLOTS];
    dm_go_plan p; const dm_go_rules *r;
} bld;

static int push(bld *b, int op, int item, int slot, int swap, dm_go_cell cell) {
    if (b->p.nstep >= DM_GO_MAXSTEP) return DM_GO_E_INPUT;
    dm_go_move m = { (uint8_t)op, (int8_t)item, (int8_t)slot, (int8_t)swap, cell };
    if (dm_gear_order_apply(&b->w, &m, &b->hand)) return DM_GO_E_INPUT;
    dm_go_step *s = &b->p.step[b->p.nstep++]; s->go = m; s->back = undo_of(&m); s->change = (int8_t)b->p.nchange;
    dm_gear_order_worn(&b->w, b->worn);
    return DM_GO_OK;
}
static const dm_go_cell k_none = { -1, -1, -1 };




#define SEARCH_BUDGET (1 << 14)
typedef struct { const int8_t *t; uint8_t *dead; int budget; int8_t seq[DM_GO_MAXCHANGE]; } srch;
typedef struct { char kind; int8_t slot; dm_go_cell want; } gop;
typedef struct { int phase, x, y; } cit;



static int cand_next(const bld *b, int item, dm_go_cell want, cit *it, dm_go_cell *out) {
    const dm_go_item *m = &b->w.item[item];
    for (;;) {
        if (it->phase == 0) {
            it->phase = 1; it->x = it->y = 0;
            if (want.grid >= 0 && want.grid < b->w.ngrid && want.x >= 0 && b->w.grid[want.grid].place && rect_free(&b->w.grid[want.grid], want.x, want.y, m->w, m->h)) { *out = want; return 1; }
            continue;
        }
        if (it->phase > 2) return 0;
        int g = it->phase == 1 ? want.grid : b->w.inv;
        if (g < 0 || g >= b->w.ngrid || !b->w.grid[g].place || (it->phase == 2 && g == want.grid)) { it->phase++; it->x = it->y = 0; continue; }
        const dm_go_grid *gr = &b->w.grid[g];
        while (it->y + m->h <= gr->h) {
            int x = it->x, y = it->y;
            if (++it->x + m->w > gr->w) { it->x = 0; it->y++; }
            if (rect_free(gr, x, y, m->w, m->h) && !(want.x >= 0 && g == want.grid && x == want.x && y == want.y)) { out->grid = (int8_t)g; out->x = (int8_t)x; out->y = (int8_t)y; return 1; }
        }
        it->phase++; it->x = it->y = 0;
    }
}


static int hands_ok(const bld *b, const int8_t *worn) {
    int r = worn[DM_GO_RIGHT], l = worn[DM_GO_LEFT];
    if (r >= 0 && l >= 0 && (b->w.item[r].hand == DM_GO_H_BOTH || b->w.item[l].hand == DM_GO_H_BOTH)) return 0;
    return 1;
}

static int put_ok(const bld *b, const dm_go_world *w, int slot, int item) {
    int8_t worn[DM_GO_SLOTS], after[DM_GO_SLOTS]; dm_gear_order_worn(w, worn); memcpy(after, worn, sizeof after); after[slot] = (int8_t)item;
    if ((slot == DM_GO_RIGHT || slot == DM_GO_LEFT) && !hands_ok(b, after)) return 0;
    return b->r->can_wear(b->r->ctx, worn, slot, item);
}


static int undo_ok(const bld *b, const dm_go_world *w0, int first) {
    for (int k = first; k < b->p.nstep; k++) {
        dm_go_world w = *w0; int hand = -1;
        for (int j = first; j <= k; j++) if (dm_gear_order_apply(&w, &b->p.step[j].go, &hand)) return 0;
        for (int j = k; j >= first; j--) { const dm_go_move *m = &b->p.step[j].back;
            if (m->op == DM_GO_PUT_SLOT && !put_ok(b, &w, m->slot, m->item)) return 0;
            if (dm_gear_order_apply(&w, m, &hand)) return 0; }
        if (hand != -1) return 0;
    }
    return 1;
}

static int order(bld *b, srch *x, unsigned todo, int *room);
static int run_ops(bld *b, srch *x, int g, const gop *ops, int nops, int k, const dm_go_world *w0, int first, unsigned rest, int *room);


static int place(bld *b, srch *x, int item, dm_go_cell want, int g, const gop *ops, int nops, int k, const dm_go_world *w0, int first, unsigned rest, int *room) {
    cit it = { 0, 0, 0 }; dm_go_cell c;
    while (cand_next(b, item, want, &it, &c)) {
        if (--x->budget < 0) break;
        bld n = *b; int sub = 0, e = push(&n, DM_GO_PUT_CELL, item, 0, -1, c);
        if (!e) e = run_ops(&n, x, g, ops, nops, k, w0, first, rest, &sub);
        if (!e) { *b = n; return DM_GO_OK; }
        if (e == DM_GO_E_MOVE || e == DM_GO_E_INPUT) return e;
        if (!sub) return e;
    }
    *room = 1; return DM_GO_E_ROOM;
}

static int run_ops(bld *b, srch *x, int g, const gop *ops, int nops, int k, const dm_go_world *w0, int first, unsigned rest, int *room) {
    if (k == nops) {
        if (g == 3 && (b->worn[DM_GO_RIGHT] != x->t[DM_GO_RIGHT] || b->worn[DM_GO_LEFT] != x->t[DM_GO_LEFT])) return DM_GO_E_RULE;
        if (b->hand >= 0 || b->p.nchange >= DM_GO_MAXCHANGE) return DM_GO_E_INPUT;
        if (!b->r->stable(b->r->ctx, b->worn) || !undo_ok(b, w0, first)) return DM_GO_E_RULE;
        b->p.change[b->p.nchange].first = (int8_t)first; b->p.change[b->p.nchange].n = (int8_t)(b->p.nstep - first); b->p.change[b->p.nchange].slots = k_group[g]; b->p.nchange++;
        return order(b, x, rest, room);
    }
    const gop *o = &ops[k]; int s = o->slot, cur = b->worn[s], tgt = o->kind == 'S' ? x->t[s] : -1, e;
    if (o->kind == 'S' && tgt == cur) return run_ops(b, x, g, ops, nops, k + 1, w0, first, rest, room);
    if (tgt < 0) {
        if (cur < 0) return run_ops(b, x, g, ops, nops, k + 1, w0, first, rest, room);
        if ((e = push(b, DM_GO_PICK_SLOT, cur, s, -1, k_none))) return e;
        return place(b, x, cur, o->want, g, ops, nops, k + 1, w0, first, rest, room);
    }

    dm_go_cell from = b->w.item[tgt].cell;

    if (b->w.item[tgt].slot == DM_GO_RIGHT + DM_GO_LEFT - s && (s == DM_GO_RIGHT || s == DM_GO_LEFT)) return DM_GO_E_RULE;
    if (b->w.item[tgt].slot || from.grid < 0) return DM_GO_E_MOVE;
    if (!put_ok(b, &b->w, s, tgt)) return DM_GO_E_RULE;
    if ((e = push(b, DM_GO_PICK_CELL, tgt, 0, -1, from))) return e;
    if ((e = push(b, DM_GO_PUT_SLOT, tgt, s, cur, k_none))) return e;
    if (cur < 0) return run_ops(b, x, g, ops, nops, k + 1, w0, first, rest, room);
    return place(b, x, cur, from, g, ops, nops, k + 1, w0, first, rest, room);
}




static int ways(const bld *b, const int8_t *t, int g, gop ops[][3], int *nops) {
    if (g != 3) { int s = g < 3 ? g + 1 : g + 2; ops[0][0] = (gop){ 'S', (int8_t)s, { (int8_t)b->w.inv, -1, -1 } }; nops[0] = 1; return 1; }
    static const char *tpl[4] = { "RL", "LR", "lRL", "rLR" };

    dm_go_cell want_l = t[DM_GO_RIGHT] >= 0 && !b->w.item[t[DM_GO_RIGHT]].slot ? b->w.item[t[DM_GO_RIGHT]].cell : k_none;
    dm_go_cell want_r = t[DM_GO_LEFT] >= 0 && !b->w.item[t[DM_GO_LEFT]].slot ? b->w.item[t[DM_GO_LEFT]].cell : k_none;
    want_l.x = want_l.y = want_r.x = want_r.y = -1;
    int n = 0;
    for (int k = 0; k < 4; k++) {
        if ((tpl[k][0] == 'l' && (b->worn[DM_GO_LEFT] < 0 || b->worn[DM_GO_LEFT] == t[DM_GO_LEFT])) || (tpl[k][0] == 'r' && (b->worn[DM_GO_RIGHT] < 0 || b->worn[DM_GO_RIGHT] == t[DM_GO_RIGHT]))) continue;
        int m = 0;
        for (const char *q = tpl[k]; *q; q++) {
            int left = *q == 'L' || *q == 'l';
            ops[n][m++] = (gop){ *q == 'R' || *q == 'L' ? 'S' : 'C', (int8_t)(left ? DM_GO_LEFT : DM_GO_RIGHT), left ? want_l : want_r };
        }
        nops[n++] = m;
    }
    return n;
}




static int order(bld *b, srch *x, unsigned todo, int *room) {
    if (!todo) return DM_GO_OK;
    int local_room = 0;
    for (int q = 0; q < DM_GO_MAXCHANGE; q++) {
        int g = x->seq[q];
        if (!(todo >> g & 1)) continue;
        unsigned rest = todo & ~(1u << g);
        if (x->dead[rest]) continue;
        gop ops[4][3]; int nops[4], nw = ways(b, x->t, g, ops, nops);
        for (int k = 0; k < nw; k++) {
            bld c = *b; dm_go_world w0 = b->w; int sub = 0;
            int e = run_ops(&c, x, g, ops[k], nops[k], 0, &w0, c.p.nstep, rest, &sub);
            if (!e) { *b = c; return DM_GO_OK; }
            if (e == DM_GO_E_MOVE || e == DM_GO_E_INPUT) return e;
            if (sub || e == DM_GO_E_ROOM) local_room = 1;
        }
    }
    if (local_room) *room = 1; else x->dead[todo] = 1;
    return local_room ? DM_GO_E_ROOM : DM_GO_E_RULE;
}

static int check_world(const dm_go_world *w) {
    if (w->ngrid < 1 || w->ngrid > DM_GO_MAXGRID || w->nitem < 0 || w->nitem > DM_GO_MAXITEM || w->inv < 0 || w->inv >= w->ngrid) return 0;
    uint16_t occ[DM_GO_MAXGRID][DM_GO_GRIDW]; memset(occ, 0, sizeof occ);
    for (int g = 0; g < w->ngrid; g++) { const dm_go_grid *gr = &w->grid[g]; if (gr->w < 1 || gr->w > DM_GO_GRIDW || gr->h < 1 || gr->h > DM_GO_GRIDW) return 0;
        for (int y = 0; y < DM_GO_GRIDW; y++) if (y >= gr->h ? gr->used[y] : gr->used[y] >> gr->w) return 0; }
    int8_t seen[DM_GO_SLOTS]; memset(seen, 0, sizeof seen);
    for (int i = 0; i < w->nitem; i++) { const dm_go_item *it = &w->item[i];
        if (it->w < 1 || it->h < 1 || it->hand > DM_GO_H_OFF || it->slot < 0 || it->slot >= DM_GO_SLOTS) return 0;
        if (it->slot) { if (seen[it->slot]++) return 0; continue; }
        int g = it->cell.grid; if (g < 0 || g >= w->ngrid || !rect_in(&w->grid[g], it->cell.x, it->cell.y, it->w, it->h)) return 0;
        for (int r = it->cell.y; r < it->cell.y + it->h; r++) { uint16_t m = row_mask(it->cell.x, it->w);
            if ((occ[g][r] & m) || (w->grid[g].used[r] & m) != m) return 0;
            occ[g][r] |= m; } }
    return 1;
}
static int prepare(bld *b, const dm_go_world *w, const int8_t *target, const dm_go_rules *r, int8_t *t) {
    memset(b, 0, sizeof *b); b->hand = -1; b->r = r;
    if (!w || !target || !r || !r->can_wear || !r->stable || !check_world(w)) return DM_GO_E_INPUT;
    b->w = *w; dm_gear_order_worn(&b->w, b->worn);
    t[0] = -1;
    for (int s = 1; s < DM_GO_SLOTS; s++) { t[s] = target[s]; if (t[s] < -1 || t[s] >= w->nitem) return DM_GO_E_INPUT; }
    if (t[11] != b->worn[11] || t[12] != b->worn[12]) return DM_GO_E_INPUT;
    for (int s = 1; s <= DM_GO_GLOVES; s++) for (int q = s + 1; q <= DM_GO_GLOVES; q++) if (t[s] >= 0 && t[s] == t[q]) return DM_GO_E_INPUT;

    if ((t[DM_GO_RRING] >= 0 && t[DM_GO_RRING] == b->worn[DM_GO_LRING]) || (t[DM_GO_LRING] >= 0 && t[DM_GO_LRING] == b->worn[DM_GO_RRING])) {
        int8_t x = t[DM_GO_RRING]; t[DM_GO_RRING] = t[DM_GO_LRING]; t[DM_GO_LRING] = x; }

    for (int s = 1; s <= DM_GO_GLOVES; s++) {
        int from = t[s] >= 0 ? w->item[t[s]].slot : 0;
        if (from && from != s && !((s == DM_GO_RIGHT && from == DM_GO_LEFT) || (s == DM_GO_LEFT && from == DM_GO_RIGHT))) return DM_GO_E_MOVE;
    }
    return DM_GO_OK;
}
static unsigned todo_of(const bld *b, const int8_t *t) {
    unsigned todo = 0;
    for (int g = 0; g < DM_GO_MAXCHANGE; g++) for (int s = 1; s <= DM_GO_GLOVES; s++) if ((k_group[g] >> s & 1) && t[s] != b->worn[s]) todo |= 1u << g;
    return todo;
}


static void seq_of(const int8_t *slots, int n, int8_t *seq) {
    int k = 0; uint16_t taken = 0;
    for (int i = 0; slots && i < n; i++) {
        int s = slots[i]; if (s < 1 || s > DM_GO_GLOVES) continue;
        for (int g = 0; g < DM_GO_MAXCHANGE; g++) if ((k_group[g] >> s & 1) && !(taken >> g & 1)) { taken |= (uint16_t)(1u << g); seq[k++] = (int8_t)g; }
    }
    for (int g = 0; g < DM_GO_MAXCHANGE; g++) if (!(taken >> g & 1)) seq[k++] = (int8_t)g;
}
static int search(bld *b, const int8_t *t, unsigned todo, const int8_t *slots, int nslots) {
    uint8_t dead[1u << DM_GO_MAXCHANGE]; srch x = { t, dead, SEARCH_BUDGET, { 0 } }; int room = 0;
    memset(dead, 0, sizeof dead); seq_of(slots, nslots, x.seq);
    return order(b, &x, todo, &room);
}
int dm_gear_order_all(const dm_go_world *w, const int8_t *target, const dm_go_rules *r, dm_go_plan *p) { return dm_gear_order_all_seq(w, target, r, NULL, 0, p); }
int dm_gear_order_all_seq(const dm_go_world *w, const int8_t *target, const dm_go_rules *r, const int8_t *slots, int nslots, dm_go_plan *p) {
    static _Thread_local bld b; int8_t t[DM_GO_SLOTS];
    if (!p) return DM_GO_E_INPUT;
    memset(p, 0, sizeof *p);
    int e = prepare(&b, w, target, r, t); if (e) return e;
    e = search(&b, t, todo_of(&b, t), slots, nslots);
    if (!e) *p = b.p;
    return e;
}
int dm_gear_order_one(const dm_go_world *w, const int8_t *target, int slot, const dm_go_rules *r, dm_go_plan *p) {
    static _Thread_local bld b; int8_t t[DM_GO_SLOTS];
    if (!p) return DM_GO_E_INPUT;
    memset(p, 0, sizeof *p);
    if (slot < 1 || slot > DM_GO_GLOVES) return DM_GO_E_INPUT;
    int e = prepare(&b, w, target, r, t); if (e) return e;
    int g = -1; for (int k = 0; k < DM_GO_MAXCHANGE; k++) if (k_group[k] >> slot & 1) g = k;
    for (int s = 1; s <= DM_GO_GLOVES; s++) if (!(k_group[g] >> s & 1)) t[s] = b.worn[s];
    if (!(todo_of(&b, t) >> g & 1)) return DM_GO_OK;
    e = search(&b, t, 1u << g, NULL, 0);
    if (!e) *p = b.p;
    return e;
}
