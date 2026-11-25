
#include "dm_gear_equip_core.h"
#include <stdio.h>
#include <string.h>

enum { M_GO, M_BACK, M_SETTLE, M_HOME, M_CHECK, M_END };
enum { S_READY, S_SENT, S_FLIP_WAIT, S_FLIP_SAVE, S_FLIP_ITEMS };

static const char *const k_end[] = { "running", "done", "returned", "partial", "in_hand", "lost" };
static const char *const k_why[] = { "none", "changed", "code", "late", "panel", "page", "save", "hand", "run", "fault", "time", "back" };
const char *dm_ge_end_name(int end) { return end >= 0 && end <= DM_GE_LOST ? k_end[end] : "?"; }
const char *dm_ge_why_name(int why) { return why >= 0 && why < DM_GE_WHY_N ? k_why[why] : "?"; }
static const char *op_name(int op) { return op == DM_GO_PICK_CELL ? "pick cell" : op == DM_GO_PICK_SLOT ? "pick slot" : op == DM_GO_PUT_SLOT ? "put slot" : op == DM_GO_PUT_CELL ? "put cell" : "?"; }

static void note(dm_ge_core *c, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#include <stdarg.h>
static void note(dm_ge_core *c, const char *fmt, ...) {
    if (!c->log) return;
    char line[256]; va_list ap; va_start(ap, fmt); vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    c->log(c->log_ctx, line);
}

int dm_ge_grid_size(int where, int *w, int *h) {
    if (where == DM_GE_INV) { *w = DM_GE_INV_W; *h = DM_GE_INV_H; return 0; }
    if (where == DM_GE_PAGE || where == DM_GE_TAB) { *w = DM_GE_STASH_W; *h = DM_GE_STASH_H; return 0; }
    return -1;
}
_Static_assert(DM_GE_STASH_W <= DM_GO_GRIDW && DM_GE_STASH_H <= DM_GO_GRIDW, "a stash grid fits the plan's grids");

static dm_ge_loc loc_of(const dm_ge_place *gp, const dm_go_world *w, int hand, int i) {
    dm_ge_loc l = { DM_GE_NOWHERE, -1, -1, -1 };
    const dm_go_item *it = &w->item[i];
    if (hand == i) { l.where = DM_GE_HAND; return l; }
    if (it->slot) { l.where = DM_GE_BODY; l.index = it->slot; return l; }
    if (it->cell.grid < 0 || it->cell.grid >= w->ngrid) return l;
    l.where = gp[it->cell.grid].where; l.index = gp[it->cell.grid].index; l.x = it->cell.x; l.y = it->cell.y;
    return l;
}
static int loc_eq(dm_ge_loc a, dm_ge_loc b) {
    if (a.where != b.where) return 0;
    if (a.where == DM_GE_HAND || a.where == DM_GE_NOWHERE) return 1;
    if (a.where == DM_GE_BODY) return a.index == b.index;
    return a.index == b.index && a.x == b.x && a.y == b.y;
}
static int matches(const dm_ge_core *c, const dm_go_world *w, int hand, const dm_ge_seen *s) {
    if (s->hand != hand && !(hand == -1 && s->hand == -2 && c->mode == M_CHECK)) return 0;
    for (int i = 0; i < w->nitem; i++) if (!loc_eq(loc_of(c->gp, w, hand, i), s->at[i])) return 0;
    return 1;
}

static int move_page(const dm_ge_place *gp, const dm_go_world *w, const dm_go_move *m) {
    if (m->op != DM_GO_PICK_CELL && m->op != DM_GO_PUT_CELL) return -1;
    int g = m->cell.grid; if (g < 0 || g >= w->ngrid) return -1;
    return gp[g].where == DM_GE_PAGE ? gp[g].index : -1;
}

static int move_place(const dm_ge_place *gp, const dm_go_world *w, const dm_go_move *m) {
    if (m->op != DM_GO_PICK_CELL && m->op != DM_GO_PUT_CELL) return -1;
    int g = m->cell.grid; if (g < 0 || g >= w->ngrid) return -1;
    return gp[g].where == DM_GE_PAGE ? gp[g].index : gp[g].where == DM_GE_TAB ? DM_GO_MAXGRID + gp[g].index : -1;
}

static int apply_go(const dm_go_plan *p, dm_go_world *w, int *hand, int from, int to) {
    for (int k = from; k < to; k++) if (dm_gear_order_apply(w, &p->step[k].go, hand)) return -1;
    return 0;
}
static int add_page(int pages, int pg) { if (pg < 0) return pages; if (pages == -1) return pg; return pages == pg ? pages : -2; }


static int run_page(const dm_ge_core *c, int back) {
    dm_go_world w = c->w; int hand = c->hand, pages = -1;
    if (!back) {
        int end = c->p.change[c->ch].first + c->p.change[c->ch].n;
        for (int k = c->j; k < end; k++) { const dm_go_move *m = &c->p.step[k].go; pages = add_page(pages, move_page(c->gp, &w, m));
            if (dm_gear_order_apply(&w, m, &hand) || hand < 0) break; }
    } else {
        for (int k = c->j - 1; k >= c->lo; k--) { const dm_go_move *m = &c->p.step[k].back; pages = add_page(pages, move_page(c->gp, &w, m));
            if (dm_gear_order_apply(&w, m, &hand) || hand < 0) break; }
    }
    return pages;
}

int dm_ge_check(const dm_go_world *w0, const dm_go_plan *p, const dm_ge_place *gp) {
    if (!w0 || !p || !gp || p->nstep < 0 || p->nstep > DM_GO_MAXSTEP) return -1;
    dm_go_world w = *w0; int hand = -1;
    for (int ch = 0; ch < p->nchange; ch++) {
        int f = p->change[ch].first, n = p->change[ch].n;
        if (f < 0 || n < 0 || f + n > p->nstep) return -1;
        dm_go_world ws = w;

        int run_pg[DM_GO_MAXSTEP + 1], pages = -1, start = f;
        for (int k = f; k < f + n; k++) {
            const dm_go_move *m = &p->step[k].go; pages = add_page(pages, move_place(gp, &w, m));
            if (pages == -2 || dm_gear_order_apply(&w, m, &hand)) return -1;
            if (hand < 0) { for (int q = start; q <= k; q++) run_pg[q] = pages; pages = -1; start = k + 1; }
        }
        if (hand >= 0) return -1;

        for (int j = f + 1; j <= f + n; j++) {
            dm_go_world b = ws; int bh = -1;
            if (apply_go(p, &b, &bh, f, j)) return -1;
            int first = bh >= 0; pages = -1;
            for (int k = j - 1; k >= f; k--) {
                const dm_go_move *m = &p->step[k].back; pages = add_page(pages, move_place(gp, &b, m));
                if (pages == -2 || dm_gear_order_apply(&b, m, &bh)) return -1;
                if (bh < 0) {
                    if (first && pages >= 0 && run_pg[j - 1] >= 0 && pages != run_pg[j - 1]) return -1;
                    first = 0; pages = -1;
                }
            }
            if (bh >= 0) return -1;
        }
    }
    return 0;
}

int dm_ge_begin(dm_ge_core *c, const dm_go_world *w, const dm_go_plan *p, const dm_ge_place *gp, int home, uint64_t now_ms, const dm_ge_fault *f) {
    void (*log)(void *, const char *) = c->log; void *ctx = c->log_ctx;
    memset(c, 0, sizeof *c); c->log = log; c->log_ctx = ctx;
    c->w0 = c->wc = c->w = *w; c->p = *p; memcpy(c->gp, gp, sizeof c->gp); c->hand = -1; c->home = home;
    c->t0 = c->t_sub = now_ms; c->mode = M_GO; c->sub = S_READY;
    if (f) c->fault = *f;
    if (dm_ge_check(w, p, gp)) { c->mode = M_END; c->end = DM_GE_RETURNED; c->why = DM_GE_WHY_PAGE; note(c, "plan refused: a run touches two stash places"); return -1; }
    note(c, "begin: %d steps in %d changes, page %d shown", p->nstep, p->nchange, home + 1);
    return 0;
}

static const dm_go_move *pending(const dm_ge_core *c) {
    if (c->mode == M_GO && c->j < c->p.nstep) return &c->p.step[c->j].go;
    if (c->mode == M_BACK && c->j > c->lo) return &c->p.step[c->j - 1].back;
    return NULL;
}
const dm_go_move *dm_ge_pending(const dm_ge_core *c) { return c->sub == S_READY || c->sub == S_SENT ? pending(c) : NULL; }
int dm_ge_view(const dm_ge_core *c, dm_ge_place *out) {
    if (!c || !out || (c->mode != M_GO && c->mode != M_BACK) || c->sub != S_READY || c->hand >= 0 || !pending(c)) return 0;
    int back = c->mode == M_BACK; dm_go_world w = c->w; int hand = c->hand;
    for (int k = back ? c->j - 1 : c->j; back ? k >= c->lo : k < c->p.nstep; k += back ? -1 : 1) {
        const dm_go_move *m = back ? &c->p.step[k].back : &c->p.step[k].go;
        int g = m->op == DM_GO_PICK_CELL || m->op == DM_GO_PUT_CELL ? m->cell.grid : -1;
        if (g >= 0 && g < w.ngrid && (c->gp[g].where == DM_GE_PAGE || c->gp[g].where == DM_GE_TAB)) { *out = c->gp[g]; return 1; }
        if (dm_gear_order_apply(&w, m, &hand) || hand < 0) break;
    }
    return 0;
}
dm_ge_loc dm_ge_expect(const dm_ge_core *c, int i, int after) {
    dm_go_world w = c->w; int hand = c->hand; const dm_go_move *m = after && c->sub == S_SENT ? pending(c) : NULL;
    if (m) dm_gear_order_apply(&w, m, &hand);
    if (i < 0 || i >= w.nitem) { dm_ge_loc l = { DM_GE_NOWHERE, -1, -1, -1 }; return l; }
    return loc_of(c->gp, &w, hand, i);
}
int dm_ge_end(const dm_ge_core *c, int *why, int *changes) { if (why) *why = c->why; if (changes) *changes = c->done_changes; return c->mode == M_END ? c->end : DM_GE_RUNNING; }


static int find_point(dm_ge_core *c, const dm_ge_seen *s) {
    int end = c->ch < c->p.nchange ? c->p.change[c->ch].first + c->p.change[c->ch].n : c->lo;
    for (int j = c->lo; j <= end; j++) {
        dm_go_world w = c->wc; int hand = -1;
        if (apply_go(&c->p, &w, &hand, c->lo, j)) break;
        if (matches(c, &w, hand, s)) {
            if (j != c->j) c->tries = 0;
            c->w = w; c->hand = hand; c->j = j; return 1;
        }
    }
    return 0;
}
static void to_check(dm_ge_core *c) { c->mode = M_CHECK; c->sub = S_READY; }

static void to_home(dm_ge_core *c, const dm_ge_seen *s) { if (c->hand < 0 && s->hand == -1) { c->mode = M_HOME; c->sub = S_READY; c->t_sub = s->now_ms; } else to_check(c); }
static void reconcile(dm_ge_core *c, const dm_ge_seen *s) {
    if (find_point(c, s)) {
        note(c, "the world is at step %d of the change (%d .. %d), going back", c->j, c->lo, c->p.change[c->ch].first + c->p.change[c->ch].n);
        if (c->tries >= DM_GE_TRIES) { note(c, "the move back from step %d was sent %d times without effect", c->j, c->tries); to_home(c, s); return; }
        c->mode = M_BACK; c->sub = S_READY; c->t_sub = s->now_ms;
        return;
    }
    note(c, "the world matches no step of the change: waiting up to %d ms", DM_GE_SETTLE_MS);
    c->mode = M_SETTLE; c->sub = S_READY; c->settle_until = s->now_ms + DM_GE_SETTLE_MS;
}
static void fail(dm_ge_core *c, const dm_ge_seen *s, int why) {
    if (!c->why) c->why = why;
    note(c, "failure %s in %s at step %d", dm_ge_why_name(why), c->mode == M_GO ? "forward" : c->mode == M_BACK ? "back" : c->mode == M_HOME ? "home" : "end", c->j);
    if (why == DM_GE_WHY_LATE && (c->mode == M_GO || c->mode == M_BACK)) {
        c->mode = M_SETTLE; c->sub = S_READY; c->quiet_until = s->now_ms + DM_GE_QUIET_MS; c->settle_until = s->now_ms + DM_GE_SETTLE_MS; return; }
    if (c->mode == M_GO || (c->mode == M_BACK && c->sub == S_SENT)) { reconcile(c, s); return; }
    to_check(c);
}

static void start_flip(dm_ge_core *c, const dm_ge_seen *s, dm_ge_act *a, int to) {
    int dir = to > s->shown ? 1 : -1;
    c->flip_to = s->shown + dir; c->flips0 = s->flips; c->commit0 = s->commits; c->nflip++; c->sub = S_FLIP_WAIT; c->t_sub = s->now_ms;
    int mute = c->fault.point == 5 && c->fault.flip == c->nflip;
    note(c, "page turn %d: %d -> %d%s", c->nflip, s->shown + 1, c->flip_to + 1, mute ? " (test: not asked)" : "");
    if (!mute) { a->kind = DM_GE_A_FLIP; a->dir = dir; }
}

static int flip_step(dm_ge_core *c, const dm_ge_seen *s) {
    uint64_t t = s->now_ms - c->t_sub;
    switch (c->sub) {
    case S_FLIP_WAIT:
        if (s->flips > c->flips0 && s->shown == c->flip_to) { note(c, "page %d shown after %llu ms", s->shown + 1, (unsigned long long)t); c->sub = S_FLIP_SAVE; return 1; }
        if (t > DM_GE_FLIP_MS) fail(c, s, DM_GE_WHY_PAGE);
        return 1;
    case S_FLIP_SAVE:
        if (s->settled && s->commits > c->commit0) { note(c, "page turn saved after %llu ms (commit %llu)", (unsigned long long)t, (unsigned long long)s->commits); c->sub = S_FLIP_ITEMS; return 1; }
        if (t > DM_GE_FLIP_MS) fail(c, s, DM_GE_WHY_SAVE);
        return 1;
    case S_FLIP_ITEMS:
        if (matches(c, &c->w, c->hand, s)) { note(c, "every item found after the page turn (%llu ms)", (unsigned long long)t); c->sub = S_READY; c->t_sub = s->now_ms; return 0; }
        if (t > DM_GE_FLIP_MS) fail(c, s, DM_GE_WHY_CHANGED);
        return 1;
    }
    return 0;
}
static int fault_at(const dm_ge_core *c, int point) { return c->mode == M_GO && c->fault.point == point && c->fault.step == c->j + 1; }


static void move_step(dm_ge_core *c, const dm_ge_seen *s, dm_ge_act *a) {
    int back = c->mode == M_BACK;
    if (c->sub >= S_FLIP_WAIT) { if (flip_step(c, s) || c->sub != S_READY) return; }
    const dm_go_move *m = pending(c);
    if (c->sub == S_SENT) {
        if (s->now_ms < c->ignore_until) return;
        dm_go_world w = c->w; int hand = c->hand;
        if (!m || dm_gear_order_apply(&w, m, &hand)) { fail(c, s, DM_GE_WHY_CHANGED); return; }
        if (s->now_ms - c->t_sub > DM_GE_CONFIRM_MS) { fail(c, s, DM_GE_WHY_LATE); return; }
        if (matches(c, &w, hand, s)) {
            int step = back ? c->j : c->j + 1;
            note(c, "%s step %d (%s item %d) confirmed after %llu ms", back ? "back" : "forward", step, op_name(m->op), m->item, (unsigned long long)(s->now_ms - c->t_sub));
            c->w = w; c->hand = hand; c->j += back ? -1 : 1; c->tries = 0; c->sub = S_READY; c->t_sub = s->now_ms;
            if (!back && (c->fault.point == 3 || c->fault.point == 6) && c->fault.step == step) {
                if (c->fault.point == 6) c->back_mute = step;
                fail(c, s, DM_GE_WHY_FAULT); return;
            }
            if (!back && c->j == c->p.change[c->ch].first + c->p.change[c->ch].n) {
                c->done_changes++; note(c, "change %d of %d in place", c->done_changes, c->p.nchange);
                c->ch++; c->lo = c->j; c->wc = c->w;
            }
        }
        return;
    }
    if (!m) { if (back) { c->mode = M_HOME; c->sub = S_READY; c->t_sub = s->now_ms; } else to_check(c); return; }
    if (!matches(c, &c->w, c->hand, s)) {
        if (s->now_ms - c->t_sub > DM_GE_GRACE_MS) { if (back) { reconcile(c, s); if (c->mode == M_BACK) return; if (!c->why) c->why = DM_GE_WHY_CHANGED; return; } fail(c, s, DM_GE_WHY_CHANGED); }
        return;
    }
    if (!s->panel_open) { if (back) { if (!c->why) c->why = DM_GE_WHY_PANEL; to_check(c); return; } fail(c, s, DM_GE_WHY_PANEL); return; }
    if (c->hand < 0) {
        int pg = run_page(c, back);
        if (pg == -2) { fail(c, s, DM_GE_WHY_PAGE); return; }
        if (pg >= 0 && s->shown != pg) {
            if (s->shown < 0) { fail(c, s, DM_GE_WHY_PAGE); return; }
            if (!s->settled) { if (s->now_ms - c->t_sub > DM_GE_FLIP_MS) { if (back) { if (!c->why) c->why = DM_GE_WHY_SAVE; to_check(c); } else fail(c, s, DM_GE_WHY_SAVE); } return; }
            start_flip(c, s, a, pg); return; }
    } else { int mp = move_page(c->gp, &c->w, m); if (mp >= 0 && mp != s->shown) { fail(c, s, DM_GE_WHY_PAGE); return; } }
    if (fault_at(c, 1)) { fail(c, s, DM_GE_WHY_FAULT); return; }
    if (m->op == DM_GO_PUT_SLOT) {
        int want = m->swap >= 0 ? 5 : 1, code = fault_at(c, 4) ? (want == 5 ? 1 : 5) : s->code;
        if (code != want) { note(c, "place code %d for slot %d, %d expected: nothing sent", code, m->slot, want); if (back) { if (!c->why) c->why = DM_GE_WHY_CODE; to_home(c, s); return; } fail(c, s, DM_GE_WHY_CODE); return; }
    }
    if (m->op == DM_GO_PUT_CELL && s->cell_free != 1) { if (back) { if (!c->why) c->why = DM_GE_WHY_CHANGED; to_home(c, s); return; } fail(c, s, DM_GE_WHY_CHANGED); return; }
    int step = back ? c->j : c->j + 1, mute = back && c->back_mute == step;
    if (c->tries >= DM_GE_TRIES) { note(c, "step %d was sent %d times", step, c->tries); to_home(c, s); return; }
    c->sub = S_SENT; c->t_sub = s->now_ms; c->tries++;
    if (fault_at(c, 2)) c->ignore_until = s->now_ms + 2500;
    note(c, "%s step %d: %s item %d slot %d swap %d cell %d:%d,%d%s", back ? "back" : "forward", step, op_name(m->op), m->item, m->slot, m->swap, m->cell.grid,
         m->cell.x, m->cell.y, mute ? " (test: not sent)" : "");
    if (!mute) { a->kind = DM_GE_A_SEND; a->move = *m; }
}

static void classify(dm_ge_core *c, const dm_ge_seen *s) {
    dm_go_world w = c->w0; int hand = -1, b = 0, found = -1;
    for (b = 0; b <= c->p.nchange; b++) {
        if (b) { int f = c->p.change[b - 1].first; if (apply_go(&c->p, &w, &hand, f, f + c->p.change[b - 1].n)) break; }
        if (matches(c, &w, -1, s)) found = b;
    }
    int missing = 0;
    for (int i = 0; i < c->w0.nitem; i++) missing += s->at[i].where == DM_GE_NOWHERE;
    if (found >= 0) { c->done_changes = found; c->end = found == c->p.nchange ? DM_GE_DONE : found == 0 ? DM_GE_RETURNED : DM_GE_PARTIAL; }
    else if (s->hand >= 0) c->end = DM_GE_IN_HAND;
    else if (missing) c->end = DM_GE_LOST;
    else { c->end = DM_GE_PARTIAL; if (!c->why) c->why = DM_GE_WHY_BACK; }
    if (c->end == DM_GE_IN_HAND && !c->why) c->why = DM_GE_WHY_BACK;
    note(c, "end %s (%s), %d of %d changes in place, %d items not found, %llu ms", dm_ge_end_name(c->end), dm_ge_why_name(c->why), c->done_changes, c->p.nchange,
         missing, (unsigned long long)(s->now_ms - c->t0));
    c->mode = M_END;
}

void dm_ge_step(dm_ge_core *c, const dm_ge_seen *s, dm_ge_act *a) {
    memset(a, 0, sizeof *a); a->kind = DM_GE_A_WAIT;
    if (c->mode == M_END) { a->kind = DM_GE_A_END; return; }
    if (!s->run_ok) {
        if (!c->why) c->why = DM_GE_WHY_RUN;
        c->end = c->hand >= 0 ? DM_GE_IN_HAND : c->mode == M_GO && c->j == c->lo && c->done_changes == 0 ? DM_GE_RETURNED : DM_GE_PARTIAL;
        note(c, "end %s: the run is gone", dm_ge_end_name(c->end)); c->mode = M_END; a->kind = DM_GE_A_END; return;
    }
    uint64_t el = s->now_ms - c->t0;
    if (el > DM_GE_ALL_MS && c->mode != M_CHECK) { if (!c->why) c->why = DM_GE_WHY_TIME; note(c, "the job's time limit of %d ms", DM_GE_ALL_MS); to_check(c); }
    if (c->mode == M_GO && el > DM_GE_GO_MS && c->sub == S_READY) fail(c, s, DM_GE_WHY_TIME);
    if (s->hand == -2 && (c->mode == M_GO || c->mode == M_BACK) && c->sub == S_READY) {
        if (c->mode == M_GO) fail(c, s, DM_GE_WHY_HAND); else { if (!c->why) c->why = DM_GE_WHY_HAND; to_check(c); }
    }
    switch (c->mode) {
    case M_GO: case M_BACK: move_step(c, s, a); break;
    case M_SETTLE:
        if (s->now_ms < c->quiet_until) break;
        if (find_point(c, s)) { note(c, "the world reached step %d of the change", c->j); c->mode = M_BACK; c->sub = S_READY; c->t_sub = s->now_ms; }
        else if (s->now_ms > c->settle_until) to_home(c, s);
        break;
    case M_HOME:
        if (c->sub >= S_FLIP_WAIT) { if (flip_step(c, s) || c->mode != M_HOME) break; }
        if (c->home < 0 || s->shown == c->home) { to_check(c); break; }
        if (s->hand != -1 || !matches(c, &c->w, c->hand, s)) { if (!c->why) c->why = DM_GE_WHY_PAGE; to_check(c); break; }
        if (c->nflip > 2 * 5 + 4) { if (!c->why) c->why = DM_GE_WHY_PAGE; to_check(c); break; }
        if (!s->settled) { if (s->now_ms - c->t_sub > DM_GE_FLIP_MS) { if (!c->why) c->why = DM_GE_WHY_SAVE; to_check(c); } break; }
        start_flip(c, s, a, c->home);
        break;
    }
    if (c->mode == M_CHECK) { classify(c, s); a->kind = DM_GE_A_END; }
}
