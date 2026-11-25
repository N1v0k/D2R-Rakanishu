


#include "../dm_gear_pages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;
#define CHECK(c) do { if (!(c)) { fails++; fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)



static int calls, hashes, fail_key = -1;
static uint64_t clock_ns, step_ns, hash_ns, list_ns, big_ns; static uint64_t big_value;
static uint64_t fake_clock(void *ctx) { (void)ctx; return clock_ns; }
static uint64_t value_of(const dm_gpg_in *in) { return in->len ? (uint64_t)in->len : in->key; }
static int fake_profile(void *ctx, const dm_gpg_in *in, uint64_t *hash, dm_gpg_prof *out) {
    (void)ctx; calls++; uint64_t v = value_of(in); clock_ns += v == big_value ? big_ns : step_ns;
    if ((int)in->key == fail_key) return -1;
    *hash = v * 1000003u;
    if (v >= 900) return 1;
    memset(out, 0, sizeof *out);
    out->kind = v % 7 == 0 ? DM_GEAR_K_NONE : DM_GEAR_K_RING;
    out->c.item.req_lvl = (int)v;
    return 0;
}
static int fake_hash(void *ctx, const dm_gpg_in *in, uint64_t *hash) { (void)ctx; hashes++; clock_ns += hash_ns; *hash = value_of(in) * 1000003u; return 0; }
static dm_gpg_budget budget(int max, uint64_t limit) { dm_gpg_budget b; memset(&b, 0, sizeof b); b.max = max; b.limit_ns = limit; b.clock = fake_clock; dm_gpg_budget_start(&b); return b; }
static void items(dm_gpg_in *in, int n, int first) {
    for (int i = 0; i < n; i++) { memset(&in[i], 0, sizeof in[i]); in[i].x = i % 10; in[i].y = i / 10; in[i].key = (uint64_t)(first + i); in[i].unit = 100u + (unsigned)i; }
}
static int find(const dm_gpg_item *it, int n, int where, int index, int x, int y) {
    for (int i = 0; i < n; i++) if (it[i].where == where && it[i].index == index && it[i].x == x && it[i].y == y) return i;
    return -1;
}

static void test_identity_and_reuse(void) {
    dm_gpg_core *c = dm_gpg_new(); dm_gpg_in in[20]; static dm_gpg_item out[64]; dm_gpg_state st;
    items(in, 5, 1);
    dm_gpg_budget b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, 2, 77, in, 5, fake_profile, NULL, NULL, &b) == 1);
    CHECK(calls == 5);
    int n = dm_gpg_items(c, out, 64);
    CHECK(n == 5);
    int k = find(out, n, DM_GPG_PERSONAL, 2, 3, 0);
    CHECK(k >= 0 && out[k].hash == 4 * 1000003u && out[k].unit == 103 && out[k].p.c.item.req_lvl == 4);
    CHECK(k >= 0 && out[k].p.show.where == DM_GTAB_STASH && out[k].p.show.page == 3 && out[k].x == 3 && out[k].y == 0 && out[k].p.c.item.id == 103);

    b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, 2, 77, in, 5, fake_profile, NULL, NULL, &b) == 1 && calls == 0);

    in[1].key = 50; in[4].x = 9; in[4].y = 9;
    b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, 2, 77, in, 5, fake_profile, NULL, NULL, &b) == 1 && calls == 1);
    n = dm_gpg_items(c, out, 64);
    CHECK(find(out, n, DM_GPG_PERSONAL, 2, 9, 9) >= 0 && find(out, n, DM_GPG_PERSONAL, 2, 4, 0) < 0);

    dm_gpg_in one = in[0]; one.x = 0; one.y = 0;
    b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, DM_GPG_PAGES + 1, 77, &one, 1, fake_profile, NULL, NULL, &b) == 1 && calls == 0);
    n = dm_gpg_items(c, out, 64);
    k = find(out, n, DM_GPG_SHARED, 1, 0, 0);
    CHECK(k >= 0 && out[k].p.show.where == DM_GTAB_SHARED && out[k].p.show.page == 2 && out[k].hash == 1 * 1000003u);

    b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, 2, 78, in, 5, fake_profile, NULL, NULL, &b) == 1 && calls == 5);

    items(in, 1, 7);
    b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, 3, 78, in, 1, fake_profile, NULL, NULL, &b) == 1 && calls == 1);
    CHECK(dm_gpg_place(c, 3, 78, in, 1, fake_profile, NULL, NULL, &b) == 1 && calls == 1);
    dm_gpg_state_get(c, &st);
    CHECK(st.present == (1u << 2 | 1u << 3 | 1u << (DM_GPG_PAGES + 1)) && !st.missing && st.items == 6);
    dm_gpg_free(c);
}



static void test_reuse_by_value(void) {
    dm_gpg_core *c = dm_gpg_new(); dm_gpg_in in[8]; static dm_gpg_item out[64];
    items(in, 4, 1); for (int i = 0; i < 4; i++) in[i].len = 40 + i;
    dm_gpg_budget b = budget(100, 0); calls = hashes = 0;
    CHECK(dm_gpg_place(c, 0, 5, in, 4, fake_profile, fake_hash, NULL, &b) == 1 && calls == 4 && hashes == 4);

    for (int i = 0; i < 4; i++) in[i].key = 500 + (uint64_t)i;
    b = budget(100, 0); calls = hashes = 0;
    CHECK(dm_gpg_place(c, 0, 5, in, 4, fake_profile, fake_hash, NULL, &b) == 1 && calls == 0 && hashes == 4);
    int n = dm_gpg_items(c, out, 64), k = find(out, n, DM_GPG_PERSONAL, 0, 1, 0);
    CHECK(n == 3 && k >= 0 && out[k].hash == 41 * 1000003u && out[k].p.c.item.req_lvl == 41);

    b = budget(100, 0); calls = hashes = 0;
    CHECK(dm_gpg_place(c, 1, 5, in, 4, fake_profile, fake_hash, NULL, &b) == 1 && calls == 0);
    b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, 1, 6, in, 4, fake_profile, fake_hash, NULL, &b) == 1 && calls == 4);
    dm_gpg_free(c);
}

static void test_budget(void) {
    dm_gpg_core *c = dm_gpg_new(); dm_gpg_in in[20]; static dm_gpg_item out[64]; dm_gpg_state st;
    items(in, 10, 1); clock_ns = 0; step_ns = 0;

    int ticks = 0, rc = 0;
    for (; ticks < 10 && rc != 1; ticks++) {
        dm_gpg_budget b = budget(3, 1000000); calls = 0;
        rc = dm_gpg_place(c, 0, 1, in, 10, fake_profile, NULL, NULL, &b);
        CHECK(calls <= 3);
        dm_gpg_state_get(c, &st);
        if (rc != 1) CHECK((st.missing & 1) && dm_gpg_items(c, out, 64) == 0);
    }
    CHECK(rc == 1 && ticks == 4);
    dm_gpg_state_get(c, &st);
    CHECK(!st.missing && st.items == 9);

    items(in, 10, 101); clock_ns = 0; step_ns = 400000;
    dm_gpg_budget b = budget(100, 1000000); calls = 0;
    CHECK(dm_gpg_place(c, 0, 1, in, 10, fake_profile, NULL, NULL, &b) == 0 && calls == 2 && dm_gpg_budget_used(&b) <= 1000000);

    calls = 0;
    CHECK(dm_gpg_place(c, 1, 1, in, 10, fake_profile, NULL, NULL, &b) == 0 && calls == 0);

    dm_gpg_state_get(c, &st);
    CHECK((st.missing & 1) && dm_gpg_items(c, out, 64) == 0);

    b.spent = 700000; dm_gpg_budget_start(&b); calls = 0;
    CHECK(dm_gpg_place(c, 0, 1, in, 10, fake_profile, NULL, NULL, &b) == 0 && calls == 0);

    in[9].key = 555;
    b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, 0, 1, in, 10, fake_profile, NULL, NULL, &b) == 1 && calls == 8);
    step_ns = 0;
    dm_gpg_free(c);
}


typedef struct { int n[DM_GPG_PLACES], first[DM_GPG_PLACES], listed; } fake_places;
static int fake_list(void *ctx, int place, dm_gpg_in *in, int max, dm_gpg_profile_fn *fn, dm_gpg_hash_fn *hash) {
    fake_places *f = ctx; clock_ns += list_ns; f->listed++;
    if (f->n[place] < 0) return f->n[place];
    int n = f->n[place] < max ? f->n[place] : max;
    items(in, n, f->first[place]);
    *fn = fake_profile; *hash = fake_hash;
    return f->n[place];
}
static void test_pass(void) {
    dm_gpg_core *c = dm_gpg_new(); static dm_gpg_in in[DM_GPG_PLACE_ITEMS]; static fake_places f; dm_gpg_state st; dm_gpg_pass_out o;
    for (int p = 0; p < DM_GPG_PLACES; p++) { f.n[p] = p < DM_GPG_PAGES || p == DM_GPG_PAGES ? 6 : DM_GPG_ABSENT; f.first[p] = 1 + 10 * p; }
    clock_ns = 0; list_ns = 100000; hash_ns = 20000; step_ns = 400000;
    dm_gpg_budget b; memset(&b, 0, sizeof b); b.max = 100; b.limit_ns = 1000000; b.clock = fake_clock;
    int ticks = 0, swept = 0; uint64_t worst = 0;
    for (; ticks < 200 && !swept; ticks++) {
        dm_gpg_budget_start(&b); calls = 0;
        dm_gpg_pass(c, 9, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o);
        uint64_t used = dm_gpg_budget_used(&b); if (used > worst) worst = used;
        swept = o.swept;
    }

    CHECK(swept && worst <= 1000000 && ticks >= 18);
    dm_gpg_state_get(c, &st);
    CHECK(!st.missing && st.present == (0x1fu | 1u << DM_GPG_PAGES) && st.items == 6 * 6 - 5);

    calls = hashes = 0; f.listed = 0;
    for (ticks = 0, swept = 0; ticks < 20 && !swept; ticks++) { dm_gpg_budget_start(&b); dm_gpg_pass(c, 9, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o); swept = o.swept; }
    CHECK(swept && calls == 0 && hashes == 0 && f.listed == DM_GPG_PLACES);


    f.first[1] = 201; big_value = 203; big_ns = 4000000; worst = 0; int over = 0;
    for (ticks = 0, swept = 0; ticks < 200 && !swept; ticks++) {
        dm_gpg_budget_start(&b); dm_gpg_pass(c, 9, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o); swept = o.swept;
        uint64_t used = dm_gpg_budget_used(&b); over += used > 1000000; if (used > worst) worst = used;
    }
    CHECK(swept && over == 1 && worst < 4000000 + 1000000);

    dm_gpg_budget_start(&b); b.limit_ns = 1; dm_gpg_pass(c, 10, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o);
    dm_gpg_state_get(c, &st);
    CHECK(st.missing == (0x1fu | 1u << DM_GPG_PAGES) && dm_gpg_items(c, NULL, 0) == 0);
    big_value = 0; big_ns = 0; list_ns = hash_ns = step_ns = 0;
    dm_gpg_free(c);
}


static void test_requests(void) {
    dm_gpg_core *c = dm_gpg_new(); static dm_gpg_in in[DM_GPG_PLACE_ITEMS]; static fake_places f; dm_gpg_pass_out o;
    for (int p = 0; p < DM_GPG_PLACES; p++) { f.n[p] = p < 3 ? 4 : DM_GPG_ABSENT; f.first[p] = 1 + 10 * p; }
    clock_ns = 0; step_ns = 0;
    dm_gpg_budget b = budget(2, 0);
    dm_gpg_pass(c, 1, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o);
    CHECK(!o.swept && dm_gpg_answered(c) == 0);
    dm_gpg_ask(c, 1);
    int ticks = 0;
    for (; ticks < 50 && !o.swept; ticks++) { dm_gpg_budget_start(&b); dm_gpg_pass(c, 1, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o); }
    CHECK(o.swept && dm_gpg_answered(c) == 0);

    f.first[2] = 92;
    for (ticks = 0, o.swept = 0; ticks < 50 && !o.swept; ticks++) { dm_gpg_budget_start(&b); dm_gpg_pass(c, 1, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o); }
    static dm_gpg_item out[64]; int n = dm_gpg_items(c, out, 64);
    CHECK(o.swept && dm_gpg_answered(c) == 1 && find(out, n, DM_GPG_PERSONAL, 2, 0, 0) >= 0 && out[find(out, n, DM_GPG_PERSONAL, 2, 0, 0)].p.c.item.req_lvl == 92);

    f.n[0] = f.n[1] = f.n[2] = DM_GPG_UNLISTED; dm_gpg_ask(c, 2);
    for (ticks = 0, o.swept = 0; ticks < 50 && dm_gpg_answered(c) != 2; ticks++) { dm_gpg_budget_start(&b); dm_gpg_pass(c, 1, fake_list, &f, in, DM_GPG_PLACE_ITEMS, &b, &o); }
    dm_gpg_state st; dm_gpg_state_get(c, &st);
    CHECK(dm_gpg_answered(c) == 2 && (st.missing & 7) == 7);
    dm_gpg_free(c);
}

static void test_unreadable_absent_skips(void) {
    dm_gpg_core *c = dm_gpg_new(); static dm_gpg_in in[DM_GPG_PLACE_ITEMS + 10]; static dm_gpg_item out[256]; dm_gpg_state st;
    items(in, 4, 11); fail_key = 13;
    dm_gpg_budget b = budget(100, 0); calls = 0;
    CHECK(dm_gpg_place(c, DM_GPG_PAGES, 1, in, 4, fake_profile, NULL, NULL, &b) == -1);
    dm_gpg_state_get(c, &st);
    CHECK((st.unreadable & 1u << DM_GPG_PAGES) && (st.missing & 1u << DM_GPG_PAGES) && dm_gpg_items(c, out, 256) == 0);

    int before = calls;
    CHECK(dm_gpg_place(c, DM_GPG_PAGES, 1, in, 4, fake_profile, NULL, NULL, &b) == -1 && calls == before);
    fail_key = -1; in[2].key = 16;
    b = budget(100, 0);
    CHECK(dm_gpg_place(c, DM_GPG_PAGES, 1, in, 4, fake_profile, NULL, NULL, &b) == 1);
    dm_gpg_state_get(c, &st);
    CHECK(!st.unreadable && !st.missing && st.items == 3);

    dm_gpg_absent(c, DM_GPG_PAGES);
    dm_gpg_state_get(c, &st);
    CHECK(!st.present && dm_gpg_items(c, out, 256) == 0);

    items(in, DM_GPG_PLACE_ITEMS, 1); in[0].skip = 1; in[1].key = 901;
    b = budget(1000, 0);
    CHECK(dm_gpg_place(c, 4, 1, in, DM_GPG_PLACE_ITEMS, fake_profile, NULL, NULL, &b) == 1);
    dm_gpg_state_get(c, &st);
    CHECK(st.skipped == 2);

    dm_gpg_missing(c, 4);
    dm_gpg_state_get(c, &st);
    CHECK((st.missing & 1u << 4) && dm_gpg_items(c, out, 256) == 0);
    unsigned long long r0 = st.rev; dm_gpg_reset(c); dm_gpg_state_get(c, &st);
    CHECK(!st.present && st.rev != r0);

    b = budget(1, 0);
    CHECK(dm_gpg_place(c, 1, 1, in, 0, fake_profile, NULL, NULL, &b) == 1);
    dm_gpg_free(c);
}



static void test_full_page(void) {
    enum { CELLS = 14 * 10 };
    dm_gpg_core *c = dm_gpg_new(); static dm_gpg_in in[CELLS + 10]; static dm_gpg_item out[2 * CELLS]; dm_gpg_state st;
    CHECK(DM_GPG_PLACE_ITEMS >= CELLS);
    items(in, CELLS, 2);
    for (int i = 0; i < CELLS; i++) { in[i].x = i % 14; in[i].y = i / 14; }
    dm_gpg_budget b = budget(1000, 0);
    CHECK(dm_gpg_place(c, 1, 1, in, CELLS, fake_profile, NULL, NULL, &b) == 1);
    dm_gpg_state_get(c, &st);
    int n = dm_gpg_items(c, out, 2 * CELLS), nones = 0;
    for (int k = 2; k < 2 + CELLS; k++) nones += k % 7 == 0;
    CHECK(st.skipped == 0 && st.place_skipped[1] == 0 && n == CELLS - nones && st.place_items[1] == CELLS - nones);
    CHECK(find(out, n, DM_GPG_PERSONAL, 1, 13, 9) >= 0);

    items(in, CELLS + 1, 2);
    b = budget(1000, 0);
    CHECK(dm_gpg_place(c, 2, 1, in, CELLS + 1, fake_profile, NULL, NULL, &b) == -1);
    dm_gpg_state_get(c, &st);
    CHECK((st.unreadable & 1u << 2) && st.place_skipped[2] == 0 && st.skipped == 0);
    dm_gpg_free(c);
}



static int row_finds(dm_gpg_core *c, uint32_t name, uint64_t vhash) {
    static dm_gpg_item out[64]; int n = dm_gpg_items(c, out, 64);
    if (!(name & DM_GTAB_STOCK_ID)) { uint64_t h; return dm_gpg_unit_hash(c, name, &h) == 1; }
    int place = (int)(name >> 16 & 0xff), x = (int)(name & 0xff), y = (int)(name >> 8 & 0xff);
    int k = find(out, n, place < DM_GPG_PAGES ? DM_GPG_PERSONAL : DM_GPG_SHARED, place < DM_GPG_PAGES ? place : place - DM_GPG_PAGES, x, y);
    return k >= 0 && out[k].hash == vhash;
}
static void test_row_name_after_flip(void) {
    dm_gpg_core *c = dm_gpg_new(); dm_gpg_in in[4];

    items(in, 3, 1); for (int i = 0; i < 3; i++) { in[i].len = 40 + i; in[i].unit = 2001u + (unsigned)i; in[i].x = 2 * i; in[i].y = 1; }
    dm_gpg_budget b = budget(100, 0); CHECK(dm_gpg_place(c, 1, 5, in, 3, fake_profile, fake_hash, NULL, &b) == 1);
    uint64_t vh = 0; uint32_t name = dm_gpg_row_name(c, 2002, &vh);
    CHECK(name == dm_gpg_stock_id(1, 2, 1) && vh == 41 * 1000003u);
    CHECK(row_finds(c, name, vh));

    items(in, 3, 5001); for (int i = 0; i < 3; i++) { in[i].len = 40 + i; in[i].x = 2 * i; in[i].y = 1; in[i].unit = 0; }
    b = budget(100, 0); CHECK(dm_gpg_place(c, 1, 5, in, 3, fake_profile, fake_hash, NULL, &b) == 1);
    CHECK(row_finds(c, name, vh));

    CHECK(dm_gpg_row_name(c, 9999, &vh) == 9999 && vh == 0);
    dm_gpg_free(c);
}


static void test_select(void) {
    dm_gpg_core *c = dm_gpg_new(); dm_gpg_in in[8]; static dm_gpg_item out[64]; static dm_gpg_sel sel; dm_gpg_state st;
    for (int p = 0; p < DM_GPG_PAGES; p++) {
        items(in, 2, 10 * p + 1); in[1].skip = p == 3;
        dm_gpg_budget b = budget(p == 2 ? 1 : 100, 0);
        dm_gpg_place(c, p, 5, in, 2, fake_profile, NULL, NULL, &b);
    }
    for (int t = 0; t < 2; t++) { items(in, 1, 101 + t); dm_gpg_budget b = budget(100, 0); dm_gpg_place(c, DM_GPG_PAGES + t, 5, in, 1, fake_profile, NULL, NULL, &b); }
    for (int t = 2; t < DM_GPG_TABS; t++) dm_gpg_absent(c, DM_GPG_PAGES + t);
    int n = dm_gpg_items(c, out, 64); dm_gpg_state_get(c, &st);
    CHECK(st.place_items[0] == 2 && st.place_items[3] == 1 && st.place_skipped[3] == 1 && (st.missing & 1u << 2));

    int tabno[DM_GPG_TABS] = { 2, 1 };
    dm_gpg_select(&st, out, n, 0, tabno, 1u << 1, &sel);
    CHECK(sel.pages == 1 && sel.shared == 0 && sel.skipped == 1 && sel.busy == 1);
    int pages[8] = { 0 }, shared_one = 0;
    for (int i = 0; i < sel.n; i++) { const dm_gpg_item *e = &out[sel.idx[i]];
        if (e->where == DM_GPG_PERSONAL) { CHECK(sel.page[i] == e->index + 1 && e->index != 0 && e->index != 2); pages[e->index]++; }
        else { CHECK(e->index == 1 && sel.page[i] == 1); shared_one++; } }
    CHECK(pages[1] == 2 && pages[3] == 1 && pages[4] == 1 && shared_one == 1 && sel.n == 5);

    dm_gpg_select(&st, out, n, 0, NULL, 1u << 0, &sel);
    CHECK(sel.shared == 0 && sel.n == 5);
    for (int i = 0; i < sel.n; i++) if (out[sel.idx[i]].where == DM_GPG_SHARED) CHECK(out[sel.idx[i]].index == 1 && sel.page[i] == 2);

    dm_gpg_select(&st, out, n - 1, 0, tabno, 0, &sel);
    CHECK(sel.shared == 1 && sel.pages == 1);
    for (int i = 0; i < sel.n; i++) CHECK(!(out[sel.idx[i]].where == DM_GPG_SHARED && out[sel.idx[i]].index == 1));

    dm_gpg_select(&st, out, n, 4, tabno, 1u << 1, &sel);
    CHECK(sel.pages == 1 && sel.n == 6 && sel.skipped == 1);
    dm_gpg_free(c);
}



static uint64_t view_hash(int shown, const int *value, int moved) {
    dm_gpg_core *c = dm_gpg_new(); dm_gpg_in in[4]; static dm_gpg_item out[64]; static dm_gpg_sel sel; dm_gpg_state st; dm_gpg_id ids[64]; int nid = 0;
    for (int p = 0; p < DM_GPG_PAGES; p++) {
        items(in, 3, 1 + 10 * p);
        for (int i = 0; i < 3; i++) { in[i].len = value[3 * p + i]; in[i].unit = p == shown ? (unsigned)(1000 * (shown + 1) + 10 * p + i) : 0; if (p != shown) in[i].key += 5000; }
        if (moved && p == 1) in[2].x = 7;
        dm_gpg_budget b = budget(100, 0); dm_gpg_place(c, p, 5, in, 3, fake_profile, fake_hash, NULL, &b);
        if (p == shown) for (int i = 0; i < 3; i++) {
            uint64_t h = 0; int rc = dm_gpg_unit_hash(c, in[i].unit, &h);
            CHECK(rc == 1 || rc == 2);
            if (rc == 1) ids[nid++] = (dm_gpg_id){ DM_GTAB_STASH, p + 1, in[i].x, in[i].y, h };
        }
    }
    for (int t = 0; t < DM_GPG_TABS; t++) dm_gpg_absent(c, DM_GPG_PAGES + t);
    int n = dm_gpg_items(c, out, 64); dm_gpg_state_get(c, &st);
    dm_gpg_select(&st, out, n, shown, NULL, 0, &sel);
    for (int i = 0; i < sel.n; i++) { const dm_gpg_item *e = &out[sel.idx[i]]; ids[nid++] = (dm_gpg_id){ DM_GTAB_STASH, sel.page[i], e->x, e->y, e->hash }; }

    uint64_t h = dm_gpg_ids_hash(ids, nid, DM_GPG_FNV0);
    dm_gpg_free(c);
    return h;
}
static void test_identities(void) {
    int v[15] = { 41, 42, 43, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 55, 56 };
    uint64_t a = view_hash(0, v, 0), b = view_hash(1, v, 0), c4 = view_hash(4, v, 0);
    CHECK(a == b && b == c4);
    CHECK(view_hash(1, v, 1) != a);
    int w[15]; memcpy(w, v, sizeof w); w[7] = 60;
    CHECK(view_hash(1, w, 0) != a);

    dm_gpg_core *c = dm_gpg_new(); uint64_t h = 0; CHECK(!dm_gpg_unit_hash(c, 1234, &h)); dm_gpg_free(c);
}

int main(void) {
    test_identity_and_reuse();
    test_reuse_by_value();
    test_budget();
    test_pass();
    test_requests();
    test_unreadable_absent_skips();
    test_full_page();
    test_row_name_after_flip();
    test_select();
    test_identities();
    if (fails) { fprintf(stderr, "t_gear_pages: %d failed\n", fails); return 1; }
    printf("t_gear_pages: identity, change detection, reuse by key and value, the tick's budget, sweeps and requests, unreadable and absent places, the tab's share, identities passed\n");
    return 0;
}
