#include "gear_brute.h"
#include <stdlib.h>
#include <string.h>

static int mains(int k) { return k == DM_GEAR_K_WEAPON1 || k == DM_GEAR_K_WEAPON2 || k == DM_GEAR_K_BOW || k == DM_GEAR_K_XBOW; }
static int offs(int k) { return k == DM_GEAR_K_SHIELD || k == DM_GEAR_K_ARROWS || k == DM_GEAR_K_BOLTS; }

static int one_hand(const dm_gear_problem *p, int i) {
    const dm_gear_cand *c = &p->cand[i];
    return c->kind == DM_GEAR_K_WEAPON1 || (p->now.cls == 4 && c->kind == DM_GEAR_K_WEAPON2 && c->weapon >= 0 && p->weapon[c->weapon].has_one);
}

static int pair_hand(const dm_gear_problem *p, int i) {
    const dm_gear_cand *c = &p->cand[i];
    if (p->now.cls == 4) return one_hand(p, i);
    return p->now.cls == 6 && c->kind == DM_GEAR_K_WEAPON1 && c->weapon >= 0 && p->weapon[c->weapon].w.kind == DM_WK_CLAW;
}

static int pair_ok(const dm_gear_problem *p, int mi, int o) {
    if (o < 0) return 1;
    int m = mi >= 0 ? p->cand[mi].kind : -1;
    switch (m) {
    case DM_GEAR_K_WEAPON2: return one_hand(p, mi) && o == DM_GEAR_K_SHIELD;
    case DM_GEAR_K_BOW: return o == DM_GEAR_K_ARROWS;
    case DM_GEAR_K_XBOW: return o == DM_GEAR_K_BOLTS;
    default: return o == DM_GEAR_K_SHIELD;
    }
}
static void put(brute_choices *c, int a, int b) { c->a[c->n] = (int16_t)a; c->b[c->n] = (int16_t)b; c->n++; }

void brute_make_plan(const dm_gear_problem *p, brute_plan *pl) {
    memset(pl, 0, sizeof *pl);
    int n = p->ncand;

    uint8_t stay[DM_GEAR_MAXCAND] = { 0 }, out[DM_GEAR_MAXCAND] = { 0 };
    for (int i = 0; i < n; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (!c->unrated) continue;
        int works = c->kind == DM_GEAR_K_CHARM ? c->inv && !(c->unrated & DM_GEAR_U_LEVEL) : c->worn;
        if (works) stay[i] = 1; else out[i] = 1;
    }
    int worn[DM_GEAR_K_N][2]; memset(worn, -1, sizeof worn);
    int wmain = -1, woff = -1, shield = 0;
    for (int i = 0; i < n; i++) shield |= p->cand[i].worn && offs(p->cand[i].kind);
    for (int i = 0; i < n; i++) {
        const dm_gear_cand *c = &p->cand[i];
        if (!c->worn || c->kind == DM_GEAR_K_CHARM) continue;
        if (one_hand(p, i) && c->item.slot == 5 && !shield) woff = i;
        else if (one_hand(p, i) && c->item.slot == 4) wmain = i;
        else if (mains(c->kind)) { if (wmain < 0) wmain = i; else woff = i; }
        else if (offs(c->kind)) woff = i;
        else { int k = worn[c->kind][0] < 0 ? 0 : 1; worn[c->kind][k] = i; }
    }
    if ((wmain >= 0 && stay[wmain]) || (woff >= 0 && stay[woff])) put(&pl->place[0], wmain, woff);
    else {
        for (int m = -1; m < n; m++) {
            if (m >= 0 && (out[m] || !mains(p->cand[m].kind))) continue;
            for (int o = -1; o < n; o++) {
                if (o >= 0 && (out[o] || !offs(p->cand[o].kind))) continue;
                if (pair_ok(p, m, o >= 0 ? p->cand[o].kind : -1)) put(&pl->place[0], m, o);
            }
        }

        if (wmain < 0 && woff >= 0 && mains(p->cand[woff].kind)) put(&pl->place[0], -1, woff);

        for (int m = 0; m < n; m++) {
            if (out[m] || !pair_hand(p, m)) continue;
            for (int o = 0; o < n; o++) if (o != m && !out[o] && pair_hand(p, o)) put(&pl->place[0], m, o);
        }
    }
    static const int kinds[8] = { 0, DM_GEAR_K_ARMOR, DM_GEAR_K_AMULET, DM_GEAR_K_RING, DM_GEAR_K_HEAD, DM_GEAR_K_BELT, DM_GEAR_K_BOOTS, DM_GEAR_K_GLOVES };
    for (int k = 1; k < 8; k++) {
        brute_choices *c = &pl->place[k];
        int kind = kinds[k];
        if (kind == DM_GEAR_K_RING) {
            int u[2], nu = 0;
            for (int j = 0; j < 2; j++) if (worn[kind][j] >= 0 && stay[worn[kind][j]]) u[nu++] = worn[kind][j];
            int list[DM_GEAR_MAXCAND], nl = 0;
            for (int i = 0; i < n; i++) if (p->cand[i].kind == kind && !out[i] && !stay[i]) list[nl++] = i;
            if (nu == 2) put(c, u[0], u[1]);
            else if (nu == 1) { put(c, u[0], -1); for (int i = 0; i < nl; i++) put(c, u[0], list[i]); }
            else { put(c, -1, -1); for (int i = 0; i < nl; i++) { put(c, list[i], -1); for (int j = i + 1; j < nl; j++) put(c, list[i], list[j]); } }
            continue;
        }
        if (worn[kind][0] >= 0 && stay[worn[kind][0]]) { put(c, worn[kind][0], -1); continue; }
        put(c, -1, -1);
        for (int i = 0; i < n; i++) if (p->cand[i].kind == kind && !out[i]) put(c, i, -1);
    }
    for (int i = 0; i < n; i++) {
        if (p->cand[i].kind != DM_GEAR_K_CHARM) continue;

        if (p->charms_fixed) { pl->keep[i] = p->cand[i].inv && !(p->cand[i].unrated & DM_GEAR_U_LEVEL); continue; }
        if (stay[i]) pl->keep[i] = 1;
        else if (!out[i]) pl->freec[pl->nfree++] = (int16_t)i;
    }
}


typedef struct { uint8_t used[DM_GEAR_GRID_H][DM_GEAR_GRID_W]; int n, w[DM_GEAR_MAXCAND], h[DM_GEAR_MAXCAND]; uint8_t done[DM_GEAR_MAXCAND]; int left; } cover;
static int cover_from(cover *c, int cell) {
    if (!c->left) return 1;
    int room = 0;
    for (int k = cell; k < DM_GEAR_GRID_W * DM_GEAR_GRID_H; k++) room += !c->used[k / DM_GEAR_GRID_W][k % DM_GEAR_GRID_W];
    int need = 0; for (int i = 0; i < c->n; i++) if (!c->done[i]) need += c->w[i] * c->h[i];
    if (need > room) return 0;
    while (cell < DM_GEAR_GRID_W * DM_GEAR_GRID_H && c->used[cell / DM_GEAR_GRID_W][cell % DM_GEAR_GRID_W]) cell++;
    if (cell == DM_GEAR_GRID_W * DM_GEAR_GRID_H) return 0;
    int x = cell % DM_GEAR_GRID_W, y = cell / DM_GEAR_GRID_W;
    for (int i = 0; i < c->n; i++) {
        if (c->done[i]) continue;
        int dup = 0; for (int j = 0; j < i; j++) dup |= !c->done[j] && c->w[j] == c->w[i] && c->h[j] == c->h[i];
        if (dup || x + c->w[i] > DM_GEAR_GRID_W || y + c->h[i] > DM_GEAR_GRID_H) continue;
        int ok = 1;
        for (int b = 0; b < c->h[i] && ok; b++) for (int a = 0; a < c->w[i]; a++) ok &= !c->used[y + b][x + a];
        if (!ok) continue;
        for (int b = 0; b < c->h[i]; b++) for (int a = 0; a < c->w[i]; a++) c->used[y + b][x + a] = 1;
        c->done[i] = 1; c->left--;
        int r = cover_from(c, cell + 1);
        c->done[i] = 0; c->left++;
        for (int b = 0; b < c->h[i]; b++) for (int a = 0; a < c->w[i]; a++) c->used[y + b][x + a] = 0;
        if (r) return 1;
    }
    c->used[y][x] = 2;
    int r = cover_from(c, cell + 1);
    c->used[y][x] = 0;
    return r;
}
int brute_fit(const dm_gear_problem *p, const uint8_t *take) {
    cover c; memset(&c, 0, sizeof c);
    for (int y = 0; y < DM_GEAR_GRID_H; y++) for (int x = 0; x < DM_GEAR_GRID_W; x++) c.used[y][x] = p->grid[y][x] ? 1 : 0;
    for (int i = 0; i < p->ncand; i++) {
        const dm_gear_cand *k = &p->cand[i];
        if (!take[i] || k->kind != DM_GEAR_K_CHARM) continue;
        if (k->inv) {
            for (int b = 0; b < k->h; b++) for (int a = 0; a < k->w; a++) { if (c.used[k->y + b][k->x + a]) return 0; c.used[k->y + b][k->x + a] = 1; }
        } else { c.w[c.n] = k->w; c.h[c.n] = k->h; c.n++; c.left++; }
    }
    return cover_from(&c, 0);
}

typedef struct { const dm_gear_problem *p; const brute_plan *pl; brute_fn fn; void *ctx; dm_gear_pick pk; int choice[8 + DM_GEAR_MAXCAND]; } walk;
static void walk_at(walk *w, int depth) {
    const brute_plan *pl = w->pl;
    if (depth == 8 + pl->nfree) { w->fn(&w->pk, w->choice, depth, w->ctx); return; }
    if (depth >= 8) {
        int c = pl->freec[depth - 8];
        for (int in = 0; in < 2; in++) { w->pk.charm[c] = (uint8_t)in; w->choice[depth] = in; walk_at(w, depth + 1); }
        w->pk.charm[c] = 0;
        return;
    }
    static const int at[8][2] = { { DM_GEAR_P_MAIN, DM_GEAR_P_OFF }, { DM_GEAR_P_ARMOR, -1 }, { DM_GEAR_P_AMULET, -1 }, { DM_GEAR_P_RING1, DM_GEAR_P_RING2 },
                                  { DM_GEAR_P_HEAD, -1 }, { DM_GEAR_P_BELT, -1 }, { DM_GEAR_P_BOOTS, -1 }, { DM_GEAR_P_GLOVES, -1 } };
    const brute_choices *c = &pl->place[depth];
    for (int i = 0; i < c->n; i++) {
        w->pk.at[at[depth][0]] = c->a[i];
        if (at[depth][1] >= 0) w->pk.at[at[depth][1]] = c->b[i];
        w->choice[depth] = i;
        walk_at(w, depth + 1);
    }
}
void brute_walk(const dm_gear_problem *p, const brute_plan *pl, brute_fn fn, void *ctx) {
    walk *w = calloc(1, sizeof *w);
    if (!w) abort();
    w->p = p; w->pl = pl; w->fn = fn; w->ctx = ctx;
    for (int k = 0; k < DM_GEAR_P_N; k++) w->pk.at[k] = -1;
    memcpy(w->pk.charm, pl->keep, sizeof pl->keep);
    walk_at(w, 0);
    free(w);
}

typedef struct { const dm_gear_problem *p; const dm_gear_goal *g; int ng; brute_out *o; int have; dm_gear_pick now; } best_ctx;
static void best_leaf(const dm_gear_pick *pk, const int *choice, int depth, void *ctx) {
    (void)choice; (void)depth;
    best_ctx *b = ctx;
    b->o->combos++;
    if (!brute_fit(b->p, pk->charm)) return;
    b->o->placed++;
    dm_gear_measures m; unsigned flags;
    if (dm_gear_leaf_measure(b->p, pk, &m, &flags)) return;
    b->o->rated++;
    int kept = dm_gear_pick_kept(&b->now, pk);
    for (int k = 0; k < b->ng; k++) {
        dm_gear_score_out s; dm_gear_score(&b->o->now, &m, &b->g[k], &s);
        int c = b->have ? dm_gear_key_cmp(s.key, b->o->key[k]) : 1;
        if (c > 0 || (c == 0 && kept > b->o->kept[k])) { memcpy(b->o->key[k], s.key, sizeof s.key); b->o->kept[k] = kept; b->o->best[k] = *pk; }
    }
    b->have = 1;
}
int gear_brute(const dm_gear_problem *p, const dm_gear_goal *goals, int ngoals, brute_out *o) {
    memset(o, 0, sizeof *o);
    dm_gear_pick now; dm_gear_pick_now(p, &now);
    unsigned flags;
    if (ngoals > BRUTE_MAXGOAL || dm_gear_leaf_measure(p, &now, &o->now, &flags)) return -1;
    brute_plan *pl = malloc(sizeof *pl);
    if (!pl) abort();
    brute_make_plan(p, pl);
    best_ctx b = { p, goals, ngoals, o, 0, now };
    brute_walk(p, pl, best_leaf, &b);
    free(pl);
    return 0;
}
