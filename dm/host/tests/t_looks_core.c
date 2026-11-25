







#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../dm_looks_core.h"

#undef realloc
void *realloc(void *p, size_t n);
static int g_no_memory;
void *t_realloc(void *p, size_t n) { return g_no_memory ? NULL : realloc(p, n); }

static int checks, fails;
#define CHECK(c, msg, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: " msg " (line %d)\n", ##__VA_ARGS__, __LINE__); } } while (0)

static uint32_t C(const char *s) { uint32_t c = 0x20202020; memcpy(&c, s, strlen(s)); return c; }

static const char TABLE[] =
    "# made-up rows\n"
    "cap\t3\t2\t2\t-1\t0\tcap\tItems/x/cap\n"
    "ghm\t3\t2\t2\t-1\t0\tghm\tItems/x/ghm\n"
    "crn\t3\t2\t2\t-1\t0\tcrn\tItems/x/crn\n"
    "ba1\t3\t2\t2\t4\t0\tba1\tItems/x/ba1\n"
    "buc\t2\t2\t2\t-1\t0\tbuc\tItems/x/buc\n"
    "sml\t2\t2\t2\t-1\t0\tsml\tItems/x/sml\n"
    "tow\t2\t2\t3\t-1\t0\ttow\tItems/x/tow\n"
    "ssd\t1\t1\t3\t-1\t11\tssd\tItems/x/ssd\n"
    "scm\t1\t1\t3\t-1\t11\tscm\tItems/x/scm\n"
    "spr\t1\t2\t4\t-1\t22\tspr\tItems/x/spr\n"
    "dgr\t1\t1\t3\t-1\t12\tdgr\tItems/x/dgr\n"
    "7dg\t1\t1\t3\t-1\t12\tdgr\tItems/x/dgr\n"
    "dir\t1\t1\t3\t-1\t12\tdir\tItems/x/dir\n"
    "lgl\t5\t2\t2\t-1\t0\tlgl\tItems/x/lgl\n"
    "qui\t4\t2\t3\t-1\t0\tqui\tItems/x/qui\n";



static const char FIXTURE[] = "../../android/tests/looks-host.swlooks", FIXTURE2[] = "../../android/tests/looks-host-v2.swlooks";

static dm_look_item item(uint32_t fp, const char *code, int q, int row) { return (dm_look_item){ fp, C(code), (uint8_t)q, row }; }
static dm_look base(const char *code) { return (dm_look){ DM_LOOK_BASE, C(code) }; }

static void test_table(dm_look_table *t) {
    CHECK(dm_looks_table_parse(TABLE, t) == 0 && t->n == 15, "table parses");
    dm_look_table bad;
    CHECK(dm_looks_table_parse("cap\t3\t2\n", &bad) == -1 && !bad.n, "a short row is refused");
    CHECK(dm_looks_table_parse("cap\t9\t2\t2\t-1\t0\tcap\tItems/x/cap\n", &bad) == -1, "an unknown slot group is refused");
    CHECK(dm_looks_table_parse("cap\t3\t2\t2\t-1\t0\tghm\tItems/x/ghm\n", &bad) == -1, "a picture the table does not have is refused");
    CHECK(dm_looks_table_parse("cap\t3\t2\t2\t-1\t0\tcap\nbuc\t2\t2\t2\t-1\t0\tcap\n", &bad) == -1, "a picture of another fit is refused");
    CHECK(dm_looks_table_parse("cap\t3\t2\t2\t-1\t0\tcap\ncap\t3\t2\t2\t-1\t0\tcap\n", &bad) == -1, "a code twice is refused");
    CHECK(dm_looks_table_parse("", &bad) == -1, "an empty table is refused");
}

static void test_fits(const dm_look_table *t) {
    CHECK(dm_looks_fits(t, C("cap"), C("ghm"), 0), "helm on helm");
    CHECK(!dm_looks_fits(t, C("cap"), C("buc"), 0), "shield never on a helm");
    CHECK(!dm_looks_fits(t, C("buc"), C("tow"), 0), "another cell size is not offered");
    CHECK(dm_looks_fits(t, C("buc"), C("sml"), 0), "shield on shield of the same size");
    CHECK(dm_looks_fits(t, C("ssd"), C("scm"), 0), "one-hand swing on one-hand swing");
    CHECK(!dm_looks_fits(t, C("ssd"), C("dgr"), 0), "swing and thrust stay apart even with equal cells");
    CHECK(!dm_looks_fits(t, C("ssd"), C("spr"), 0), "one-hand never takes a two-hand look");
    CHECK(dm_looks_fits(t, C("cap"), C("ba1"), 4) && !dm_looks_fits(t, C("cap"), C("ba1"), 0), "a class helm only for its class");
    CHECK(dm_looks_fits(t, C("ba1"), C("cap"), 4), "a class helm takes a common helm look");
    CHECK(!dm_looks_fits(t, C("cap"), C("zzz"), 0) && !dm_looks_fits(t, C("zzz"), C("cap"), 0), "unknown codes never fit");
    CHECK(dm_looks_invisible_ok(t, C("cap")) && dm_looks_invisible_ok(t, C("buc")), "helm and shield may be invisible");
    CHECK(!dm_looks_invisible_ok(t, C("ssd")) && !dm_looks_invisible_ok(t, C("qui")) && !dm_looks_invisible_ok(t, C("lgl")), "weapon, armor and gloves may not");
}

static void test_price(void) {
    CHECK(dm_looks_price(3, DM_LOOK_BASE) == 500, "base look has a floor of 500");
    CHECK(dm_looks_price(42, DM_LOOK_BASE) == 4200, "base look is item level x 100");
    CHECK(dm_looks_price(42, DM_LOOK_UNIQUE) == 21000 && dm_looks_price(42, DM_LOOK_SET) == 21000, "unique and set look is item level x 500");
    CHECK(dm_looks_price(42, DM_LOOK_INVISIBLE) == 4200, "invisible is item level x 100");
    CHECK(dm_looks_price(42, DM_LOOK_ORIGINAL) == 0, "back to the original is free");
}

static void test_collection(void) {
    dm_looks_store s = { 0 };
    CHECK(dm_looks_coll_add(&s, base("cap")) == 1, "a new look is added");
    CHECK(dm_looks_coll_add(&s, base("cap")) == 0 && s.ncoll == 1, "the same look twice stays one");
    CHECK(dm_looks_coll_add(&s, (dm_look){ DM_LOOK_UNIQUE, 7 }) == 1, "a unique's own look is its own entry");
    CHECK(dm_looks_coll_has(&s, base("cap")) && !dm_looks_coll_has(&s, base("ghm")), "lookup");
    CHECK(!dm_looks_coll_has(&s, (dm_look){ DM_LOOK_SET, 7 }), "a set row is not the unique row of the same number");
    dm_looks_store_free(&s);
}




static void promote(dm_looks_store *s, uint64_t work_seq) { dm_look_pending p[64]; int n = dm_looks_promotable(s, work_seq, p, 64); dm_looks_settle(s, p, n); }
static int commit(dm_looks_store *s, uint64_t work_seq, int write_ok, uint8_t **disk, size_t *ndisk) {
    promote(s, work_seq);
    uint8_t *b; size_t n = dm_looks_encode(s, &b);
    if (!n || !write_ok) { free(b); return 0; }
    free(*disk); *disk = b; *ndisk = n;
    return 1;
}
static dm_look on_disk(const uint8_t *d, size_t n, const dm_look_item *it) {
    dm_looks_store r; if (!d || dm_looks_decode(d, n, &r)) return (dm_look){ 0xff, 0 };
    dm_look l = dm_looks_current(&r, it); dm_looks_store_free(&r); return l;
}

static void test_payment(void) {
    dm_look_item cap = item(0x1234, "cap", 2, -1);
    uint8_t *disk = NULL; size_t n = 0;
    {
        dm_looks_store s = { 0 };
        dm_looks_choose(&s, &cap, base("ghm"), 0, 10);
        CHECK(dm_looks_current(&s, &cap).id == C("ghm"), "a paid look is drawn at once");
        CHECK(commit(&s, 9, 1, &disk, &n) && on_disk(disk, n, &cap).kind == DM_LOOK_ORIGINAL, "the old character file never carries the new look");
        CHECK(commit(&s, 0, 1, &disk, &n) && on_disk(disk, n, &cap).kind == DM_LOOK_ORIGINAL, "nor a working directory cloned anew");
        CHECK(commit(&s, 11, 1, &disk, &n) && on_disk(disk, n, &cap).id == C("ghm"), "the first save written after the payment carries it");
        dm_looks_store_free(&s);
    }
    free(disk); disk = NULL;
    {
        dm_looks_store s = { 0 };
        commit(&s, 0, 1, &disk, &n);
        dm_looks_choose(&s, &cap, base("ghm"), 0, 20);
        dm_looks_store_free(&s);
        CHECK(on_disk(disk, n, &cap).kind == DM_LOOK_ORIGINAL, "an unsaved payment leaves no look behind");
    }
    {
        dm_looks_store s = { 0 };
        dm_looks_choose(&s, &cap, base("ghm"), 0, 30);
        dm_look_pending p[4]; int k = dm_looks_promotable(&s, 31, p, 4);
        CHECK(k == 1 && s.npend == 1 && on_disk(disk, n, &cap).kind == DM_LOOK_ORIGINAL, "a failed write changes nothing on disk or in memory");
        CHECK(commit(&s, 32, 1, &disk, &n) && on_disk(disk, n, &cap).id == C("ghm"), "the next save after the payment writes it");
        dm_looks_store_free(&s);
    }
    {
        dm_looks_store s = { 0 };
        dm_looks_choose(&s, &cap, base("ghm"), 0, 40);
        CHECK(dm_looks_busy(&s, &cap), "a paid choice not saved keeps the item busy");
        commit(&s, 41, 1, &disk, &n);
        CHECK(!dm_looks_busy(&s, &cap) && on_disk(disk, n, &cap).id == C("ghm"), "saved, the item is free again");
        dm_looks_choose(&s, &cap, base("crn"), 0, 42);
        commit(&s, 41, 1, &disk, &n);
        CHECK(on_disk(disk, n, &cap).id == C("ghm") && dm_looks_current(&s, &cap).id == C("crn"), "a file written before the second payment keeps the first look");
        commit(&s, 43, 1, &disk, &n);
        CHECK(on_disk(disk, n, &cap).id == C("crn"), "then the newer one");
        dm_looks_choose(&s, &cap, (dm_look){ DM_LOOK_ORIGINAL, 0 }, 0, 0);
        CHECK(!dm_looks_busy(&s, &cap), "a free reset does not keep the item busy");
        commit(&s, 0, 1, &disk, &n);
        CHECK(on_disk(disk, n, &cap).kind == DM_LOOK_ORIGINAL, "a reset is saved with the next save");
        dm_looks_store_free(&s);
    }
    free(disk);
}

typedef struct { int calls, take, dye_to, starve; } fake_pay;
static int pay(void *ctx, int *dye) {
    fake_pay *p = ctx; p->calls++;
    if (p->starve) g_no_memory = 1;
    if (p->take && p->dye_to >= 0) *dye = p->dye_to;
    return p->take;
}
static void test_pay_choose(void) {
    dm_look_item cap = item(0x1234, "cap", 2, -1);
    dm_looks_store s = { 0 };
    {
        g_no_memory = 1; fake_pay p = { 0, 1, -1, 0 };
        int r = dm_looks_pay_choose(&s, &cap, base("ghm"), 9, 10, pay, &p); g_no_memory = 0;
        CHECK(r == -1 && p.calls == 0 && s.npend == 0 && s.rev == 0, "no room: gold and gem are never taken (%d, %d paid)", r, p.calls);
    }
    {
        for (int i = 0; s.npend < s.cappend || !s.cappend; i++) { dm_look_item o = item(0x5000 + (uint32_t)i, "cap", 2, -1); dm_looks_choose(&s, &o, base("crn"), 0, 0); }
        int full = s.npend; uint64_t rev = s.rev;
        g_no_memory = 1; fake_pay p = { 0, 1, -1, 0 };
        int r = dm_looks_pay_choose(&s, &cap, base("ghm"), 9, 10, pay, &p); g_no_memory = 0;
        CHECK(r == -1 && p.calls == 0 && s.npend == full && s.rev == rev, "a full store refuses before the payment");
        p = (fake_pay){ 0, 1, -1, 1 };
        r = dm_looks_pay_choose(&s, &cap, base("ghm"), 9, 11, pay, &p); g_no_memory = 0;
        CHECK(r == 1 && p.calls == 1 && dm_looks_current(&s, &cap).id == C("ghm") && dm_looks_current_dye(&s, &cap) == 9 && dm_looks_busy(&s, &cap),
              "with room taken first, memory running out during the payment still records it");
    }
    {
        dm_look_item hat = item(0x777, "cap", 2, -1); fake_pay p = { 0, 0, -1, 0 };
        CHECK(dm_looks_pay_choose(&s, &hat, base("ghm"), 9, 12, pay, &p) == 0 && dm_looks_current(&s, &hat).kind == DM_LOOK_ORIGINAL, "nothing paid, nothing chosen");
        p = (fake_pay){ 0, 1, 0, 0 };
        CHECK(dm_looks_pay_choose(&s, &hat, base("ghm"), 9, 12, pay, &p) == 1 && dm_looks_current(&s, &hat).id == C("ghm") && dm_looks_current_dye(&s, &hat) == 0,
              "the look paid with gold is kept with the colour the payment left");
    }
    dm_looks_store_free(&s);
}

static void test_page_grid(void) {

    CHECK(dm_looks_page_cap(2, 3, 0) == 15 && dm_looks_page_cap(2, 3, 1) == 10, "armour page %d / %d", dm_looks_page_cap(2, 3, 0), dm_looks_page_cap(2, 3, 1));
    CHECK(dm_looks_page_cap(1, 1, 0) == DM_LOOKS_TILES && dm_looks_page_cap(1, 2, 1) == 40 && dm_looks_page_cap(11, 1, 0) == 0, "small and invalid sizes");
    int cap = dm_looks_page_cap(2, 3, 1), n = 24;
    CHECK(dm_looks_page_index(cap, 1, 0, n) == 10 && dm_looks_page_index(cap, 1, 9, n) == 19, "page two shows entries 10 to 19");
    CHECK(dm_looks_page_index(cap, 2, 3, n) == 23 && dm_looks_page_index(cap, 2, 4, n) == -1 && dm_looks_page_index(cap, 1, 10, n) == -1, "past the list or the page");

    dm_look list[24]; for (int i = 0; i < n; i++) list[i] = (dm_look){ DM_LOOK_BASE, 0x100u + (uint32_t)i };
    dm_look tapped = list[dm_looks_page_index(cap, 1, 2, n)];
    CHECK(tapped.id == 0x100u + 12, "page two's third tile is entry 12, not 17 (%u)", tapped.id - 0x100u);
}

static void test_dye_pick(void) {

    dm_dye_offer row[3] = { { 0, -1 }, { 6, 2 }, { 9, 4 } };
    CHECK(dm_dyes_offer_at(row, 3, 9) == 2, "the tapped colour at its tile");
    dm_dye_offer after[2] = { { 0, -1 }, { 9, 4 } };
    CHECK(dm_dyes_offer_at(after, 2, 9) == 1 && dm_dyes_offer_at(after, 2, 6) == -1, "kept by value over the rebuild; a colour gone ends its preview");
}

static void test_apply_gate(void) {
    dm_looks_apply_gate g = { 0 };
    CHECK(dm_looks_apply_gate_step(&g, 1, 0, 1) == -1, "X while Apply is hidden: dropped");
    CHECK(dm_looks_apply_gate_step(&g, 1, 1, 1) == -1, "a press queued before Apply showed never pays");
    CHECK(dm_looks_apply_gate_step(&g, 1, 1, 0) == 0 && dm_looks_apply_gate_step(&g, 1, 1, 1) == 1, "a fresh press under Apply pays");
    CHECK(dm_looks_apply_gate_step(&g, 2, 1, 1) == -1, "the choice changed in this read: the press was for the one before");
    CHECK(dm_looks_apply_gate_step(&g, 2, 0, 0) == -1 && dm_looks_apply_gate_step(&g, 3, 1, 1) == -1, "preview ended and a new one: still no payment");
    CHECK(dm_looks_apply_gate_step(&g, 3, 1, 1) == 1, "then a new press pays the new choice");
}

static void test_dress(void) {
    uint8_t u[0x40], d[0x40], fu[0x40], fd[0x40];
    for (int i = 0; i < 0x40; i++) { u[i] = (uint8_t)i; d[i] = (uint8_t)(0x80 + i); }
    memset(fu, 0xee, sizeof fu); memset(fd, 0xee, sizeof fd);
    dm_looks_dress(u, d, 321, 7, 55, fu, sizeof fu, fd, sizeof fd);
    uint32_t cls, q, f; int32_t row; uint64_t p;
    memcpy(&cls, fu + DM_LOOK_U_CLASS, 4); memcpy(&p, fu + DM_LOOK_U_DATA, 8); memcpy(&q, fd + DM_LOOK_D_QUALITY, 4); memcpy(&row, fd + DM_LOOK_D_ROW, 4); memcpy(&f, fd + DM_LOOK_D_FLAGS, 4);
    CHECK(cls == 321 && p == (uint64_t)(uintptr_t)fd && q == 7 && row == 55 && (f & DM_LOOK_FL_IDENT), "the copy carries the look and points at its own data");
    CHECK(fu[0x20] == 0x20 && fd[0x3f] == 0xbf && fd[0x10] == 0x90, "the rest is the item's own");
    int left = 0; for (int i = 0; i < 0x40; i++) left += fu[i] == 0xee || fd[i] == 0xee;
    CHECK(!left, "no byte of the copy is left unset (%d)", left);
}

static void test_file(void) {
    dm_looks_store s = { 0 }, r;
    dm_looks_coll_add(&s, base("cap")); dm_looks_coll_add(&s, (dm_look){ DM_LOOK_SET, 3 });
    dm_look_item cap = item(0xdead, "cap", 7, 12), buc = item(0xbeef, "buc", 2, -1);
    dm_looks_choose(&s, &cap, base("ghm"), 0, 0); dm_looks_choose(&s, &buc, (dm_look){ DM_LOOK_INVISIBLE, 0 }, 0, 0);
    promote(&s, 1);
    dm_look_char ch = { .name = "Hero" }; ch.p[0] = (dm_look_place){ C("cap"), 7, 12, base("ghm") };
    dm_looks_char_set(&s, &ch);
    uint8_t *b; size_t n = dm_looks_encode(&s, &b);
    CHECK(n && dm_looks_decode(b, n, &r) == DM_LOOKS_OK, "round trip");
    {
        const char *w = getenv("DM_LOOKS_FIXTURE_WRITE");
        if (w && !strcmp(w, "1")) { FILE *f = fopen(FIXTURE, "wb"); CHECK(f && fwrite(b, 1, n, f) == n && !fclose(f), "fixture written"); }
        uint8_t fx[4096]; FILE *f = fopen(FIXTURE, "rb"); size_t m = f ? fread(fx, 1, sizeof fx, f) : 0; if (f) fclose(f);
        CHECK(m == n && !memcmp(fx, b, n), "the encoder writes the bytes of %s (%zu of %zu bytes)", FIXTURE, m, n);
    }
    CHECK(r.ncoll == 2 && r.nmap == 2 && r.nchars == 1, "round trip keeps every entry");
    CHECK(dm_looks_current(&r, &cap).id == C("ghm") && dm_looks_current(&r, &buc).kind == DM_LOOK_INVISIBLE, "round trip keeps the mapping");
    const dm_look_char *rc = dm_looks_char_get(&r, "Hero");
    CHECK(rc && rc->p[0].look.id == C("ghm") && rc->p[0].row == 12, "round trip keeps the character select places");
    dm_look_item other = cap; other.code = C("skp");
    CHECK(dm_looks_current(&r, &other).kind == DM_LOOK_ORIGINAL, "an entry never fits an item of another base type with the same identity");
    other = cap; other.quality = 2;
    CHECK(dm_looks_current(&r, &other).kind == DM_LOOK_ORIGINAL, "nor one of another quality");
    dm_looks_store_free(&r);
    uint8_t *c = malloc(n); memcpy(c, b, n); c[n / 2] ^= 0x40;
    CHECK(dm_looks_decode(c, n, &r) == DM_LOOKS_CORRUPT && !r.ncoll, "a damaged file is refused whole");
    CHECK(dm_looks_decode(b, n - 3, &r) == DM_LOOKS_CORRUPT, "a cut off file is refused");
    CHECK(dm_looks_decode(b, 5, &r) == DM_LOOKS_CORRUPT, "a stub is refused");
    memcpy(c, b, n); c[4] = DM_LOOKS_VERSION + 1;
    { uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < n - 8; i++) { h ^= c[i]; h *= 1099511628211ull; } for (int i = 0; i < 8; i++) c[n - 8 + i] = (uint8_t)(h >> (8 * i)); }
    CHECK(dm_looks_decode(c, n, &r) == DM_LOOKS_NEWER, "a newer version is told apart from damage");
    free(c); free(b);
    dm_looks_store_free(&s);
}

static void reseal(uint8_t *c, size_t n) { uint64_t h = 1469598103934665603ull; for (size_t i = 0; i < n - 8; i++) { h ^= c[i]; h *= 1099511628211ull; } for (int i = 0; i < 8; i++) c[n - 8 + i] = (uint8_t)(h >> (8 * i)); }
static void test_file_colour(void) {
    dm_looks_store s = { 0 }, r;
    dm_look_item cap = item(0xdead, "cap", 4, -1), buc = item(0xbeef, "buc", 2, -1);
    dm_looks_coll_add(&s, base("ghm"));
    dm_looks_choose(&s, &cap, base("ghm"), 10, 0); dm_looks_choose(&s, &buc, (dm_look){ DM_LOOK_ORIGINAL, 0 }, 0, 0);
    promote(&s, 1);
    uint8_t *b; size_t n = dm_looks_encode(&s, &b);
    CHECK(n && b[4] == 2 && dm_looks_decode(b, n, &r) == DM_LOOKS_OK, "a colour makes the file version 2");
    {
        const char *w = getenv("DM_LOOKS_FIXTURE_WRITE");
        if (w && !strcmp(w, "1")) { FILE *f = fopen(FIXTURE2, "wb"); CHECK(f && fwrite(b, 1, n, f) == n && !fclose(f), "fixture written"); }
        uint8_t fx[4096]; FILE *f = fopen(FIXTURE2, "rb"); size_t m = f ? fread(fx, 1, sizeof fx, f) : 0; if (f) fclose(f);
        CHECK(m == n && !memcmp(fx, b, n), "the encoder writes the bytes of %s (%zu of %zu bytes)", FIXTURE2, m, n);
    }
    CHECK(dm_looks_current_dye(&r, &cap) == 10 && dm_looks_current(&r, &cap).id == C("ghm") && !dm_looks_current_dye(&r, &buc), "round trip keeps look and colour");
    dm_looks_store_free(&r);
    dm_looks_choose(&s, &cap, base("ghm"), 0, 0); promote(&s, 1);
    uint8_t *b1; size_t n1 = dm_looks_encode(&s, &b1);
    CHECK(n1 == n - 2 && b1[4] == 1, "without any colour the file is version 1 again");
    free(b1);
    uint8_t *c = malloc(n); memcpy(c, b, n);
    size_t at = 0; for (size_t i = 8; i + 4 <= n; i++) if (!memcmp(c + i, "MAP ", 4)) { at = i; break; }

    c[at + 12 + 18] = DM_DYE_COLOURS + 1; reseal(c, n);
    CHECK(at && dm_looks_decode(c, n, &r) == DM_LOOKS_CORRUPT, "a colour outside the game's colours is refused");
    CHECK(dm_looks_decode(b, n - 9, &r) == DM_LOOKS_CORRUPT, "a cut off version 2 file is refused");
    memcpy(c, b, n); c[4] = 1; reseal(c, n);
    CHECK(dm_looks_decode(c, n, &r) == DM_LOOKS_CORRUPT, "version 2 entries read as version 1 are refused");
    free(c); free(b);
    dm_looks_store_free(&s);
}

static void test_dyes(void) {
    static const char tab[] = "# gem code\tcolour\tgrade\ticon\n"
        "gcr\t8\t0\tItems/misc/gem/chipped_ruby\ngfr\t8\t1\tItems/misc/gem/flawed_ruby\ngsr\t8\t2\tItems/misc/gem/ruby\n"
        "gpr\t9\t4\tItems/misc/gem/perfect_ruby\nskc\t2\t0\tItems/misc/gem/chipped_skull\n";
    dm_dye_table t;
    CHECK(!dm_dyes_parse(tab, &t) && t.n == 5 && dm_dyes_find(&t, C("gpr"))->colour == 9, "the gem table is read");
    CHECK(dm_dyes_parse("gcr\t21\t0\tx\n", &(dm_dye_table){ 0 }) && dm_dyes_parse("gcr\t8\t0\tx\ngcr\t8\t1\ty\n", &(dm_dye_table){ 0 }) &&
          dm_dyes_parse("gcr\t8\t0\n", &(dm_dye_table){ 0 }) && dm_dyes_parse("", &(dm_dye_table){ 0 }), "a colour outside the game's, a gem twice, a short row or no row is refused");
    CHECK(dm_dye_where(2, 2) == (DM_DYE_FIGURE | DM_DYE_PICTURE), "a helm of group 2 shows the colour on the figure and in the picture");
    CHECK(dm_dye_where(5, 0) == DM_DYE_FIGURE && dm_dye_where(1, 0) == DM_DYE_FIGURE, "barbarian helms and necromancer heads: the figure only");
    CHECK(dm_dye_where(0, 8) == DM_DYE_PICTURE, "gloves, boots, belts: the picture only");
    CHECK(dm_dye_where(0, 0) == 0 && dm_dye_where(0, 3) == 0 && dm_dye_where(0, 4) == 0 && dm_dye_where(9, 9) == 0, "paladin shields and unknown groups: nowhere");
    CHECK(dm_dye_source(10, DM_LOOK_UNIQUE) == DM_DYE_CHOSEN && dm_dye_source(10, DM_LOOK_ORIGINAL) == DM_DYE_CHOSEN, "a chosen colour comes first");
    CHECK(dm_dye_source(0, DM_LOOK_UNIQUE) == DM_DYE_LOOK_ROW && dm_dye_source(0, DM_LOOK_SET) == DM_DYE_LOOK_ROW, "then a unique or set look's own colour");
    CHECK(dm_dye_source(0, DM_LOOK_BASE) == DM_DYE_UNCOLOURED, "a base look is drawn without the item's own colour");
    CHECK(dm_dye_source(0, DM_LOOK_ORIGINAL) == DM_DYE_GAME && dm_dye_source(0, DM_LOOK_INVISIBLE) == DM_DYE_GAME, "else the game's colour");
    uint32_t inv[] = { C("gsr"), C("cap"), C("gcr"), C("gpr"), C("skc") }, none[] = { C("cap"), C("r01") };
    dm_dye_offer o[8];
    int n = dm_dyes_offer(&t, inv, 5, o, 8);
    CHECK(n == 3 && o[0].dye == 9 && t.g[o[0].gem].code == C("gcr") && o[1].dye == 10 && t.g[o[1].gem].code == C("gpr") && o[2].dye == 3,
          "one tile per colour, in the table's order, using up the lowest gem of the colour");
    CHECK(dm_dyes_offer(&t, none, 2, o, 8) == 0, "no gem, no colour offered");
    dm_dyes_free(&t);
}

static void test_merge(void) {

    dm_looks_store dev = { 0 }, bak = { 0 };
    dm_look_item kept = item(1, "cap", 2, -1), reset = item(2, "buc", 2, -1), incoming = item(3, "ssd", 2, -1);
    dm_looks_coll_add(&dev, base("ghm")); dm_looks_coll_add(&bak, base("crn")); dm_looks_coll_add(&bak, base("ghm"));
    dm_looks_choose(&dev, &kept, base("ghm"), 0, 0); dm_looks_choose(&dev, &reset, (dm_look){ DM_LOOK_ORIGINAL, 0 }, 0, 0); promote(&dev, 1);
    dm_looks_choose(&bak, &kept, base("crn"), 4, 0); dm_looks_choose(&bak, &reset, base("sml"), 0, 0); dm_looks_choose(&bak, &incoming, base("scm"), 9, 0); promote(&bak, 1);
    dm_look_char dc = { .name = "Hero" }, bc = { .name = "Hero" }, nc = { .name = "Neu" };
    dc.p[0].look = base("ghm"); bc.p[0].look = base("crn"); nc.p[0].look = base("crn");
    dm_looks_char_set(&dev, &dc); dm_looks_char_set(&bak, &bc); dm_looks_char_set(&bak, &nc);
    CHECK(dm_looks_merge(&dev, &bak) == 0, "merge");
    CHECK(dev.ncoll == 2 && dm_looks_coll_has(&dev, base("crn")), "collections are united");
    CHECK(dm_looks_current(&dev, &kept).id == C("ghm"), "an item that stays keeps the device's look");
    CHECK(dm_looks_current(&dev, &reset).kind == DM_LOOK_ORIGINAL, "an item reset on the device stays original");
    CHECK(dm_looks_current(&dev, &incoming).id == C("scm"), "an item that comes with the import brings its look");
    CHECK(dm_looks_current_dye(&dev, &incoming) == 9 && !dm_looks_current_dye(&dev, &kept), "and its colour; the device's item keeps its own");
    CHECK(dm_looks_char_get(&dev, "Hero")->p[0].look.id == C("ghm") && dm_looks_char_get(&dev, "Neu"), "character select: device first, new characters added");
    dm_looks_store copy = { 0 };
    uint8_t *b; size_t n = dm_looks_encode(&dev, &b); dm_looks_decode(b, n, &copy); free(b);
    CHECK(dm_looks_merge(&dev, &bak) == 0 && dev.ncoll == copy.ncoll && dev.nmap == copy.nmap && dev.nchars == copy.nchars, "merging the same backup again changes nothing");
    dm_looks_store_free(&copy); dm_looks_store_free(&dev); dm_looks_store_free(&bak);
}


typedef struct { uint8_t d[64]; int at; } bitw;
static void wb(bitw *w, uint32_t v, int n) { for (int i = 0; i < n; i++, w->at++) if (v >> i & 1) w->d[w->at >> 3] |= (uint8_t)(1 << (w->at & 7)); }
static void wcode(bitw *w, const char *c) {
    static const char *const tab[] = { " 10", "a11110", "b0101", "c01000", "d110001", "g11010", "h00011", "l11101", "m01101", "p11001", "u00001", 0 };
    for (int k = 0; k < 4; k++) for (int i = 0; tab[i]; i++) if (tab[i][0] == c[k]) for (const char *b = tab[i] + 1; *b; b++) wb(w, (uint32_t)(*b - '0'), 1);
}
static int make_bits(bitw *w, const char *code, uint32_t fp, int quality, int row, int identified) {
    memset(w, 0, sizeof *w);
    wb(w, identified ? 1u << 4 : 0, 32); wb(w, 5, 3); wb(w, 0, 3); wb(w, 0, 4); wb(w, 2, 4); wb(w, 1, 4); wb(w, 5, 3);
    wcode(w, code); wb(w, 0, 3); wb(w, fp, 32); wb(w, 30, 7); wb(w, (uint32_t)quality, 4); wb(w, 0, 1); wb(w, 0, 1);
    if (quality == 5 || quality == 7) wb(w, (uint32_t)row, 12);
    wb(w, 0, 40);
    return (w->at + 7) / 8;
}
static uint32_t base_of_row(dm_look l) { return l.kind == DM_LOOK_UNIQUE && l.id == 12 ? C("cap") : 0; }

static void test_bits(const dm_look_table *t) {
    bitw w; dm_look_item it; int id;
    int n = make_bits(&w, "cap ", 0xcafef00d, 7, 12, 1);
    CHECK(dm_looks_bits_item(w.d, n, &it, &id) == 0 && it.code == C("cap") && it.fp == 0xcafef00d && it.quality == 7 && it.row == 12 && id, "a unique cap is read from its bitstream");
    n = make_bits(&w, "buc ", 7, 2, 0, 0);
    CHECK(dm_looks_bits_item(w.d, n, &it, &id) == 0 && it.code == C("buc") && it.row == -1 && !id, "an unidentified buckler");
    CHECK(dm_looks_bits_item(w.d, 6, &it, &id) == -1, "a cut off bitstream is refused");

    it = (dm_look_item){ 1, C("cap"), 7, 12 };
    CHECK(dm_looks_unlocks(t, &it, 1, 1).kind == DM_LOOK_UNIQUE && dm_looks_unlocks(t, &it, 1, 1).id == 12, "a unique with its own look unlocks that look");
    CHECK(dm_looks_unlocks(t, &it, 1, 0).kind == DM_LOOK_BASE && dm_looks_unlocks(t, &it, 1, 0).id == C("cap"), "a unique without its own look unlocks its base type");
    CHECK(dm_looks_unlocks(t, &it, 0, 1).kind == DM_LOOK_ORIGINAL, "an unidentified item unlocks nothing");
    it.code = C("rin");
    CHECK(dm_looks_unlocks(t, &it, 1, 0).kind == DM_LOOK_ORIGINAL, "a ring unlocks nothing");

    dm_looks_store s = { 0 };
    n = make_bits(&w, "ghm ", 99, 2, 0, 1);
    CHECK(dm_looks_bits_item(w.d, n, &it, &id) == 0, "page item read");
    dm_looks_coll_add(&s, dm_looks_unlocks(t, &it, id, 0));
    n = make_bits(&w, "cap ", 98, 7, 12, 1);
    dm_looks_bits_item(w.d, n, &it, &id); dm_looks_coll_add(&s, dm_looks_unlocks(t, &it, id, 1));
    dm_looks_coll_add(&s, base("buc"));
    dm_look offer[8]; int k = dm_looks_offer(&s, t, C("crn"), 0, base_of_row, offer, 8);
    CHECK(k == 2, "a helm is offered the helm looks only (%d)", k);
    int ghm = 0, uniq = 0; for (int i = 0; i < k; i++) { ghm |= offer[i].kind == DM_LOOK_BASE && offer[i].id == C("ghm"); uniq |= offer[i].kind == DM_LOOK_UNIQUE; }
    CHECK(ghm && uniq, "the page's great helm and the unique cap's own look are offered");

    it = (dm_look_item){ 2, C("7dg"), 2, -1 };
    CHECK(dm_looks_unlocks(t, &it, 1, 0).id == C("dgr"), "a bone knife unlocks the dagger's picture");
    dm_looks_coll_add(&s, base("7dg")); dm_looks_coll_add(&s, base("dgr")); dm_looks_coll_add(&s, base("dir"));
    k = dm_looks_offer(&s, t, C("dir"), 0, NULL, offer, 8);
    CHECK(k == 1 && offer[0].id == C("dgr"), "one dagger picture is offered for a dirk (%d)", k);
    k = dm_looks_offer(&s, t, C("7dg"), 0, NULL, offer, 8);
    CHECK(k == 1 && offer[0].id == C("dir"), "a bone knife is not offered the dagger picture it has (%d)", k);
    dm_looks_store_free(&s);
}

static void test_sig(void) {
    dm_look_item cap = item(0xdead, "cap", 4, -1), ring = item(0x1234, "rin", 4, -1);
    uint64_t a = dm_looks_sig_add(dm_looks_sig_add(0, &cap, 7, 0, 0), &ring, 8, 0, 1);
    uint64_t ident = dm_looks_sig_add(dm_looks_sig_add(0, &cap, 7, 0, 1), &ring, 8, 0, 1);
    CHECK(a && a != ident, "identifying an item in place changes the signature");
    CHECK(a == dm_looks_sig_add(dm_looks_sig_add(0, &cap, 7, 0, 0), &ring, 8, 0, 1), "the same items give the same signature");
    dm_look_item q = cap; q.quality = 7; q.row = 3;
    CHECK(dm_looks_sig_add(0, &q, 7, 0, 0) != dm_looks_sig_add(0, &cap, 7, 0, 0), "another quality or row changes it");
    CHECK(dm_looks_sig_add(0, &cap, 7, 1, 0) != dm_looks_sig_add(0, &cap, 7, 0, 0), "another mode changes it");

    dm_look_table t; dm_looks_table_parse(TABLE, &t);
    dm_looks_store s = { 0 };
    CHECK(dm_looks_unlocks(&t, &cap, 0, 0).kind == DM_LOOK_ORIGINAL, "unidentified, the cap unlocks nothing");
    dm_look l = dm_looks_unlocks(&t, &cap, 1, 0);
    CHECK(l.kind == DM_LOOK_BASE && dm_looks_coll_add(&s, l) == 1, "identified, it unlocks its base type");
    CHECK(dm_looks_coll_has(&s, base("cap")), "after the sale the look stays collected");
    dm_looks_store_free(&s); dm_looks_table_free(&t);
}


static uint32_t select_base(dm_look l, void *ctx) {
    (*(int *)ctx)++;
    if (l.kind == DM_LOOK_BASE) return l.id;
    return l.kind != DM_LOOK_UNIQUE ? 0 : l.id == 7 ? C("ba1") : l.id == 8 ? C("ghm") : 0;
}
static int sel(const dm_looks_store *s, const dm_look_table *t, dm_look_item it, int pclass, int dyes_on, dm_look *l, int *dye) {
    int calls = 0;
    return dm_looks_select(s, t, &it, pclass, dyes_on, select_base, &calls, l, dye);
}
static void test_select(const dm_look_table *t) {
    dm_looks_store s = { 0 }; dm_look l; int dye;
    dm_look_item cap = item(1, "cap", 2, -1), buc = item(2, "buc", 2, -1), qui = item(3, "qui", 2, -1), dgr = item(4, "dgr", 4, -1), zzz = item(5, "zzz", 2, -1);
    dm_looks_coll_add(&s, base("ghm")); dm_looks_coll_add(&s, base("crn")); dm_looks_coll_add(&s, base("dir")); dm_looks_coll_add(&s, base("sml"));
    dm_looks_coll_add(&s, (dm_look){ DM_LOOK_UNIQUE, 7 }); dm_looks_coll_add(&s, (dm_look){ DM_LOOK_UNIQUE, 8 }); dm_looks_coll_add(&s, (dm_look){ DM_LOOK_UNIQUE, 9 });

    CHECK(!sel(&s, t, cap, 0, 1, &l, &dye) && l.kind == DM_LOOK_ORIGINAL && !dye, "an item without a choice is drawn as itself");
    dm_looks_choose(&s, &cap, base("ghm"), 0, 0); promote(&s, 1);
    CHECK(sel(&s, t, cap, 0, 1, &l, &dye) && l.kind == DM_LOOK_BASE && l.id == C("ghm") && !dye, "the chosen look");
    dm_looks_choose(&s, &cap, base("crn"), 0, 5);
    CHECK(sel(&s, t, cap, 0, 1, &l, &dye) && l.id == C("crn"), "a paid choice not saved yet counts before the committed one");
    dm_looks_choose(&s, &dgr, base("dir"), 0, 0);
    CHECK(sel(&s, t, dgr, -1, 1, &l, &dye) && l.id == C("dir"), "a look every class wears shows for an unknown class");

    dm_looks_choose(&s, &cap, (dm_look){ DM_LOOK_UNIQUE, 7 }, 0, 0);
    CHECK(sel(&s, t, cap, 4, 1, &l, &dye) && l.kind == DM_LOOK_UNIQUE && l.id == 7, "a class look on its class");
    CHECK(!sel(&s, t, cap, 0, 1, &l, &dye) && l.kind == DM_LOOK_ORIGINAL, "a class look on another class stays the original");
    CHECK(!sel(&s, t, cap, -1, 1, &l, &dye) && l.kind == DM_LOOK_ORIGINAL, "a class look stays the original while the class is unknown");
    dm_looks_choose(&s, &cap, (dm_look){ DM_LOOK_UNIQUE, 8 }, 0, 0);
    CHECK(sel(&s, t, cap, -1, 1, &l, &dye) && l.kind == DM_LOOK_UNIQUE && l.id == 8, "a unique look every class wears shows for an unknown class");
    dm_looks_choose(&s, &cap, (dm_look){ DM_LOOK_UNIQUE, 9 }, 0, 0);
    CHECK(!sel(&s, t, cap, 0, 1, &l, &dye), "a look the game does not know stays the original");
    dm_looks_choose(&s, &cap, base("sml"), 0, 0);
    CHECK(!sel(&s, t, cap, 0, 1, &l, &dye), "a look of another slot stays the original");
    dm_looks_choose(&s, &cap, base("ba1"), 0, 0);
    CHECK(!sel(&s, t, cap, 4, 1, &l, &dye), "a look the collection does not have stays the original");

    dm_looks_choose(&s, &cap, (dm_look){ DM_LOOK_INVISIBLE, 0 }, 0, 0); dm_looks_choose(&s, &buc, (dm_look){ DM_LOOK_INVISIBLE, 0 }, 0, 0);
    dm_looks_choose(&s, &qui, (dm_look){ DM_LOOK_INVISIBLE, 0 }, 0, 0);
    CHECK(sel(&s, t, cap, 0, 1, &l, &dye) && l.kind == DM_LOOK_INVISIBLE, "an invisible helm");
    CHECK(sel(&s, t, buc, 0, 1, &l, &dye) && l.kind == DM_LOOK_INVISIBLE, "an invisible shield");
    CHECK(!sel(&s, t, qui, 0, 1, &l, &dye) && l.kind == DM_LOOK_ORIGINAL, "an invisible armour stays the original");

    dm_looks_choose(&s, &buc, (dm_look){ DM_LOOK_ORIGINAL, 0 }, 9, 0);
    CHECK(sel(&s, t, buc, 0, 1, &l, &dye) && l.kind == DM_LOOK_ORIGINAL && dye == 9, "a colour without a look");
    CHECK(!sel(&s, t, buc, 0, 0, &l, &dye) && !dye, "no colour while colours are off");
    dm_looks_choose(&s, &cap, (dm_look){ DM_LOOK_UNIQUE, 7 }, 9, 0);
    CHECK(sel(&s, t, cap, 0, 1, &l, &dye) && l.kind == DM_LOOK_ORIGINAL && dye == 9, "a look turned down keeps its colour");
    dm_looks_choose(&s, &zzz, base("ghm"), 9, 0);
    CHECK(!sel(&s, t, zzz, 0, 1, &l, &dye) && l.kind == DM_LOOK_ORIGINAL && !dye, "an item the table does not know is drawn as itself");
    dm_looks_store_free(&s);

    uint8_t h[0x30] = { 0x55, 0xaa, 0x55, 0xaa, 0x69 };
    h[0x18] = 7; h[0x28] = 3;
    CHECK(dm_looks_header_class(h, sizeof h) == 7, "the class of a file of version 0x69");
    CHECK(dm_looks_header_class(h, 0x19) == 7 && dm_looks_header_class(h, 0x18) == -1, "a header cut short before the class");
    h[4] = 0x68; CHECK(dm_looks_header_class(h, sizeof h) == 7, "version 0x68 keeps the class at 0x18");
    h[4] = 0x60; CHECK(dm_looks_header_class(h, sizeof h) == 3, "the class of a file of version 0x60");
    h[4] = 0x5b; CHECK(dm_looks_header_class(h, sizeof h) == 3 && dm_looks_header_class(h, 0x28) == -1, "version 0x5b, and cut short");
    h[4] = 0x5a; CHECK(dm_looks_header_class(h, sizeof h) == -1, "a version the game reads no class from");
    h[4] = 0x69; h[0] = 0x56; CHECK(dm_looks_header_class(h, sizeof h) == -1, "another magic");
    CHECK(dm_looks_header_class(h, 4) == -1, "a header shorter than its version");
}

int main(void) {
    dm_look_table t;
    test_table(&t);
    test_fits(&t);
    test_price();
    test_collection();
    test_payment();
    test_pay_choose();
    test_page_grid();
    test_dye_pick();
    test_dress();
    test_apply_gate();
    test_file();
    test_file_colour();
    test_dyes();
    test_merge();
    test_sig();
    test_bits(&t);
    test_select(&t);
    dm_looks_table_free(&t);
    printf("t_looks_core: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
