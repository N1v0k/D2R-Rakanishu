



#include <stdio.h>
#include <string.h>
#include "../dm_unsocket.h"

static int checks, fails;
#define CHECK(c, msg, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: " msg " (line %d)\n", ##__VA_ARGS__, __LINE__); } } while (0)

static dm_uns_view recipe(int w, int h, int fillers) {
    dm_uns_view v = { .items = 3, .bases = 1, .ral = 1, .scroll = 1, .base_w = w, .base_h = h, .fillers = fillers, .fillers_small = fillers, .cube_w = 3, .cube_h = 4, .inv_free = 0 };
    return v;
}

static void test_decide(void) {
    int over;
    dm_uns_view v = recipe(2, 2, 2);
    CHECK(dm_uns_decide(&v, &over) == DM_UNS_APPLY && over == 0, "a cap with two inserts");
    v.items = 4; CHECK(dm_uns_decide(&v, &over) == DM_UNS_PASS, "a fourth item: not the recipe");
    v = recipe(2, 2, 2); v.ral = 0; v.items = 3; CHECK(dm_uns_decide(&v, &over) == DM_UNS_PASS, "another rune (Hel): the game's own recipe");
    v = recipe(2, 2, 2); v.bases = 2; v.ral = 0; CHECK(dm_uns_decide(&v, &over) == DM_UNS_PASS, "two socketed items, no Ral");
    v = recipe(2, 2, 0); v.bases = 0; CHECK(dm_uns_decide(&v, &over) == DM_UNS_PASS, "no filled socket");
    v = recipe(2, 2, 1); CHECK(dm_uns_decide(&v, &over) == DM_UNS_APPLY, "partly filled: one insert is enough");
    v = recipe(2, 2, 2); v.scroll = 0; CHECK(dm_uns_decide(&v, &over) == DM_UNS_PASS, "no scroll");
    v = recipe(2, 2, 2); v.job_busy = 1; CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_BUSY, "an item job owns the items");
    v = recipe(2, 2, 2); v.fillers_small = 1; CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_SHAPE, "an insert larger than one cell");
    v = recipe(2, 2, 2); v.cube_w = 0; CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_READ, "cube grid unread");
    v = recipe(0, 2, 2); CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_READ, "item size unread");
    v = recipe(4, 2, 2); CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_READ, "item wider than the cube");


    static const int sz[][2] = { { 1, 1 }, { 1, 2 }, { 1, 3 }, { 2, 2 }, { 2, 3 }, { 2, 4 }, { 1, 4 } };
    for (unsigned s = 0; s < sizeof sz / sizeof *sz; s++)
        for (int n = 1; n <= 6; n++)
            for (int inv = 0; inv <= 2; inv++) {
                v = recipe(sz[s][0], sz[s][1], n); v.inv_free = inv;
                int free = 12 - sz[s][0] * sz[s][1], spill = n > free ? n - free : 0, want = spill > inv ? DM_UNS_REFUSE_ROOM : DM_UNS_APPLY;
                CHECK(dm_uns_decide(&v, &over) == want && over == (want == DM_UNS_APPLY ? spill : spill - inv), "%dx%d with %d inserts, %d free in the inventory", sz[s][0], sz[s][1], n, inv);
            }
    v = recipe(2, 4, 5); CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_ROOM && over == 1, "2x4 with 5, full inventory: one cell missing");
    v = recipe(2, 4, 6); CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_ROOM && over == 2, "2x4 with 6, full inventory: two cells missing");
    v = recipe(2, 4, 6); v.inv_free = 1; CHECK(dm_uns_decide(&v, &over) == DM_UNS_REFUSE_ROOM && over == 1, "2x4 with 6, one free: one missing");
    v = recipe(2, 4, 6); v.inv_free = 2; CHECK(dm_uns_decide(&v, &over) == DM_UNS_APPLY && over == 2, "2x4 with 6, two free: two go to the inventory");
    v = recipe(2, 4, 5); v.inv_free = 30; CHECK(dm_uns_decide(&v, &over) == DM_UNS_APPLY && over == 1, "2x4 with 5: one goes to the inventory");
    v = recipe(2, 4, 4); CHECK(dm_uns_decide(&v, &over) == DM_UNS_APPLY && over == 0, "2x4 with 4 fits exactly, nothing to the inventory");
    v = recipe(2, 3, 6); CHECK(dm_uns_decide(&v, &over) == DM_UNS_APPLY && over == 0, "2x3 with 6 fits exactly");
    CHECK(dm_uns_overflow(3, 4, 2, 4, 3) == 0, "overflow never negative");
}

static void test_penalty(void) {
    dm_uns_item it = { .max_dur = 12 };
    CHECK(dm_uns_penalty(&it) == 1, "normal item pays");
    it.ethereal = 1; CHECK(dm_uns_penalty(&it) == 0, "ethereal is spared");
    it = (dm_uns_item){ .max_dur = 0 }; CHECK(dm_uns_penalty(&it) == 0, "no durability at all: spared");
    it = (dm_uns_item){ .max_dur = 20, .no_durability = 1 }; CHECK(dm_uns_penalty(&it) == 0, "a bow carries durability but its type never wears: spared");
    it = (dm_uns_item){ .max_dur = 30, .indestructible_before = 1, .indestructible_after = 1 }; CHECK(dm_uns_penalty(&it) == 0, "indestructible on its own: spared");
    it = (dm_uns_item){ .max_dur = 30, .indestructible_before = 1, .indestructible_after = 0 };
    CHECK(dm_uns_penalty(&it) == DM_UNS_PENALTY_INSERT_INDESTRUCTIBLE, "indestructible only through the insert: the owner's choice");
    it.ethereal = 1; CHECK(dm_uns_penalty(&it) == 0, "ethereal Zod item: spared");
}

static void test_outcome(void) {
    dm_uns_after a = { .base_found = 1, .base_fillers = 2, .ral_left = 1, .scroll_left = 1, .fillers_noted = 2, .fillers_home = 0 };
    CHECK(dm_uns_outcome(&a) == DM_UNS_NOT_RUN, "nothing changed");
    a = (dm_uns_after){ .base_found = 1, .base_fillers = 0, .fillers_noted = 2, .fillers_home = 2 };
    CHECK(dm_uns_outcome(&a) == DM_UNS_DONE, "ran: empty, consumed, all home");
    a.fillers_home = 1; CHECK(dm_uns_outcome(&a) == DM_UNS_ODD, "an insert not home");
    a = (dm_uns_after){ .base_found = 0, .fillers_noted = 2, .fillers_home = 2 }; CHECK(dm_uns_outcome(&a) == DM_UNS_ODD, "the item gone");
    a = (dm_uns_after){ .base_found = 1, .ral_left = 1, .fillers_noted = 2, .fillers_home = 2 }; CHECK(dm_uns_outcome(&a) == DM_UNS_ODD, "the rune left over");
}


enum { HEL = 0x0123, SCROLL = 0x0234, RAL = 0x0345 };
static void clear_row(uint8_t *r) {
    memset(r, 0, DM_UNS_ROW);
    r[0] = 1; r[0xc] = 3; r[0x60] = 0x5a; r[0x100] = 0xa5;
    static const uint8_t in[24] = { 0x09, 0, 0xff, 0xff, 0, 0, 0, 0, 0x01, 0, HEL & 0xff, HEL >> 8, 0, 0, 0, 0, 0x01, 0, SCROLL & 0xff, SCROLL >> 8, 0, 0, 0, 0 };
    memcpy(r + DM_UNS_ROW_INPUTS, in, sizeof in);
    r[DM_UNS_ROW_FLAGS] = 0x10;
}
static void test_row(void) {
    uint8_t r[DM_UNS_ROW], s[DM_UNS_ROW];
    clear_row(r); CHECK(dm_uns_row_is_clear(r, HEL, SCROLL), "the socket-clearing row");
    CHECK(!dm_uns_row_is_clear(r, RAL, SCROLL), "another rune index");
    memcpy(s, r, sizeof r); s[0] = 0; CHECK(!dm_uns_row_is_clear(s, HEL, SCROLL), "disabled");
    memcpy(s, r, sizeof r); s[0xc] = 2; CHECK(!dm_uns_row_is_clear(s, HEL, SCROLL), "two inputs");
    memcpy(s, r, sizeof r); s[DM_UNS_ROW_INPUTS] = 0x01; CHECK(!dm_uns_row_is_clear(s, HEL, SCROLL), "first input without the socket flag");
    memcpy(s, r, sizeof r); s[DM_UNS_ROW_INPUTS + 3 * DM_UNS_INPUT] = 1; CHECK(!dm_uns_row_is_clear(s, HEL, SCROLL), "a fourth input");
    memcpy(s, r, sizeof r); s[DM_UNS_ROW_INPUTS + DM_UNS_INPUT + 7] = 2; CHECK(!dm_uns_row_is_clear(s, HEL, SCROLL), "a quantity on the rune");
    memcpy(s, r, sizeof r); s[DM_UNS_ROW_FLAGS] = 0x20; CHECK(!dm_uns_row_is_clear(s, HEL, SCROLL), "already remove");
    memcpy(s, r, sizeof r); dm_uns_row_lend(s, RAL);
    CHECK(s[DM_UNS_ROW_INPUTS + DM_UNS_INPUT + 2] == (RAL & 0xff) && s[DM_UNS_ROW_INPUTS + DM_UNS_INPUT + 3] == RAL >> 8, "rune input -> Ral");
    CHECK(s[DM_UNS_ROW_FLAGS] == 0x20 && s[DM_UNS_ROW_FLAGS + 1] == 0, "destroy -> remove");
    int diff = 0; for (int i = 0; i < DM_UNS_ROW; i++) diff += s[i] != r[i];
    CHECK(diff == 3, "only the rune index and the output flag change (%d bytes)", diff);
}


typedef struct {
    dm_uns_view v[2];
    int unreadable, lend_fails, runs_recipe, queues, odd;
    uint8_t row[DM_UNS_ROW], saved[DM_UNS_ROW];
    int lent_during_body, lends, spill, give_backs, bodies, penalties, notices, notice_why, notice_over, mode;
    int notes[16];
    dm_uns_item item;
} fake;
static int f_view(void *x, dm_uns_view *v) { fake *f = x; if (f->unreadable) return 0; *v = f->v[f->mode]; return 1; }
static int f_lend(void *x, int spill) { fake *f = x; f->lends++; f->spill = spill; if (f->lend_fails) return 0; dm_uns_row_lend(f->row, RAL); return 1; }
static void f_give_back(void *x) { fake *f = x; f->give_backs++; memcpy(f->row, f->saved, DM_UNS_ROW); }
static uint64_t f_body(void *x) {
    fake *f = x; f->bodies++;
    int lent = memcmp(f->row, f->saved, DM_UNS_ROW) != 0; f->lent_during_body = lent;
    if (f->mode == 0) return lent && f->queues ? DM_UNS_QUEUED : 1;
    return 0;
}
static void f_after(void *x, dm_uns_after *a, dm_uns_item *it) {
    fake *f = x; int ran = f->lent_during_body && f->runs_recipe;
    *a = (dm_uns_after){ .base_found = 1, .base_fillers = ran ? 0 : 2, .ral_left = !ran, .scroll_left = !ran, .fillers_noted = 2, .fillers_home = ran ? 2 - f->odd : 0 };
    *it = f->item;
}
static void f_penalty(void *x) { ((fake *)x)->penalties++; }
static void f_notice(void *x, int why, int over) { fake *f = x; f->notices++; f->notice_why = why; f->notice_over = over; }
static void f_note(void *x, int what, int mode) { (void)mode; ((fake *)x)->notes[what]++; }
static const dm_uns_ops F = { f_view, f_lend, f_give_back, f_body, f_after, f_penalty, f_notice, f_note };

static void fresh(fake *f) {
    memset(f, 0, sizeof *f); clear_row(f->row); memcpy(f->saved, f->row, DM_UNS_ROW);
    f->v[0] = f->v[1] = recipe(2, 2, 2); f->runs_recipe = 1; f->queues = 1; f->item.max_dur = 12;
}
static uint64_t gate(fake *f, int mode) { f->mode = mode; return dm_uns_gate(&F, f, mode); }

static void test_gate(void) {
    fake f;
    fresh(&f);
    CHECK(gate(&f, 0) == DM_UNS_QUEUED, "mode 0 queues with the lent row");
    CHECK(f.lent_during_body && !memcmp(f.row, f.saved, DM_UNS_ROW), "lent during the body, given back after");
    CHECK(f.penalties == 0 && f.notes[DM_UNS_N_QUEUED] == 1, "mode 0 changes nothing");
    CHECK(gate(&f, 1) == 0 && f.penalties == 1 && f.notes[DM_UNS_N_DONE] == 1 && f.notes[DM_UNS_N_PENALISED] == 1, "mode 1 runs and pays");
    CHECK(!memcmp(f.row, f.saved, DM_UNS_ROW) && f.give_backs == 2, "given back in both modes");

    fresh(&f); f.v[1].items = 4;
    gate(&f, 0); CHECK(gate(&f, 1) == 0 && !f.lent_during_body && f.penalties == 0 && f.lends == 1, "mode 1 checks again: not the recipe now, nothing lent");

    fresh(&f); f.v[1] = recipe(2, 4, 6); f.v[1].inv_free = 2;
    gate(&f, 0); gate(&f, 1);
    CHECK(f.spill == 2 && f.penalties == 1, "mode 1 lends with two inserts for the inventory");

    fresh(&f); f.v[1] = recipe(2, 4, 6);
    gate(&f, 0); gate(&f, 1);
    CHECK(f.notices == 1 && f.notice_why == DM_UNS_REFUSE_ROOM && f.notice_over == 2 && f.penalties == 0 && !f.lent_during_body, "mode 1 refuses on its own reading");

    fresh(&f); f.v[0] = recipe(2, 4, 6);
    CHECK(gate(&f, 0) == 1 && f.lends == 0 && f.notices == 1 && f.notice_over == 2 && f.bodies == 1, "refused: row untouched, the game finds no recipe, a notice");
    CHECK(!memcmp(f.row, f.saved, DM_UNS_ROW), "refused: the row never changed");

    fresh(&f); f.runs_recipe = 0;
    gate(&f, 1); CHECK(f.penalties == 0 && f.notes[DM_UNS_N_NOT_RUN] == 1 && !memcmp(f.row, f.saved, DM_UNS_ROW), "not run: no penalty, row back");

    fresh(&f); f.odd = 1;
    gate(&f, 1); CHECK(f.penalties == 0 && f.notes[DM_UNS_N_ODD] == 1, "an insert missing afterwards: assumption line, no penalty");

    fresh(&f); f.item.ethereal = 1;
    gate(&f, 1); CHECK(f.penalties == 0 && f.notes[DM_UNS_N_SPARED] == 1 && f.notes[DM_UNS_N_DONE] == 1, "ethereal: emptied, spared");

    fresh(&f); f.lend_fails = 1;
    CHECK(gate(&f, 0) == 1 && f.notes[DM_UNS_N_ROW] == 1 && f.notices == 1 && f.notice_why == DM_UNS_REFUSE_READ, "row moved: refused");

    fresh(&f); f.unreadable = 1;
    gate(&f, 0); CHECK(f.lends == 0 && f.notes[DM_UNS_N_UNREAD] == 1 && f.notices == 0 && f.bodies == 1, "unreadable: the game runs as always");

    fresh(&f); f.v[0] = f.v[1] = (dm_uns_view){ .items = 3, .bases = 1, .scroll = 1, .fillers = 2, .fillers_small = 2 };
    gate(&f, 0); gate(&f, 1);
    CHECK(f.lends == 0 && f.penalties == 0 && f.notices == 0 && f.bodies == 2, "Hel: untouched, forwarded");

    fresh(&f);
    gate(&f, 0); gate(&f, 0); gate(&f, 1); f.v[1].items = 4; gate(&f, 1);
    CHECK(f.penalties == 1 && f.lends == 3 && f.give_backs == 3 && !memcmp(f.row, f.saved, DM_UNS_ROW), "second queued run finds the cube changed");
}

int main(void) {
    test_decide(); test_penalty(); test_outcome(); test_row(); test_gate();
    printf("t_unsocket: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
