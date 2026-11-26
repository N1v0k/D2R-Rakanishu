








#include <stdio.h>
#include "../game_addrs.h"
#include <stdint.h>
#include <string.h>
#include "../hle.h"

static int checks, fails;
#define CHECK(c, msg, ...) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: " msg " (line %d)\n", ##__VA_ARGS__, __LINE__); } } while (0)

module_t g_mod[MAX_MOD];
int g_nmod;


static int sw_stack, sw_speed;
int dm_custom_get(const char *key) {
    if (!strcmp(key, "shrine_stacking")) return sw_stack;
    if (!strcmp(key, "shrine_run_speed")) return sw_speed;
    return 0;
}


#define NL 24
#define POOL 22
static uint8_t unit[0x400], base_list[0xc00], lists[NL][0x70];
static uint8_t D[0x400], curse_bits[0x100], timer_obj[0x200], arg[0x40], arg2[0x40], image_win[0x100];

int dm_guest_heap_readable(uint64_t a, size_t n) {
    const struct { const void *p; size_t n; } ok[] = {
        { unit, sizeof unit }, { base_list, sizeof base_list }, { lists, sizeof lists },
        { D, sizeof D }, { curse_bits, sizeof curse_bits }, { timer_obj, sizeof timer_obj }, { arg, sizeof arg }, { arg2, sizeof arg2 },
    };
    for (unsigned i = 0; i < sizeof ok / sizeof *ok; i++) { uint64_t b = (uint64_t)(uintptr_t)ok[i].p; if (a >= b && n <= ok[i].n && a - b <= ok[i].n - n) return 1; }
    return 0;
}

static uint64_t addr(const void *p) { return (uint64_t)(uintptr_t)p; }
static void put32(void *p, size_t at, uint32_t v) { memcpy((uint8_t *)p + at, &v, 4); }
static void put64(void *p, size_t at, uint64_t v) { memcpy((uint8_t *)p + at, &v, 8); }
static void putf(void *p, size_t at, float v) { memcpy((uint8_t *)p + at, &v, 4); }
static uint32_t get32(const void *p, size_t at) { uint32_t v; memcpy(&v, (const uint8_t *)p + at, 4); return v; }
static uint64_t get64(const void *p, size_t at) { uint64_t v; memcpy(&v, (const uint8_t *)p + at, 8); return v; }
static float getf(const void *p, size_t at) { float v; memcpy(&v, (const uint8_t *)p + at, 4); return v; }


static uint64_t test_apply(uint64_t a, uint64_t entry);
static void test_unlink(unsigned ver, uint64_t l);
static void test_free(unsigned ver, uint64_t l);
static void test_timer(uint64_t t, uint64_t u, int end);
static void test_stat(unsigned ver, uint64_t l, int stat, int value);
static void test_statebit(uint64_t u, int state);
#define DM_SHRINE_GAME_APPLY test_apply
#define DM_SHRINE_GAME_UNLINK test_unlink
#define DM_SHRINE_GAME_FREE test_free
#define DM_SHRINE_GAME_TIMER test_timer
#define DM_SHRINE_GAME_STAT test_stat
#define DM_SHRINE_GAME_STATEBIT test_statebit
#include "../dm_shrine.c"


static void link_list(int idx, int state, float end, uint32_t flags) {
    uint8_t *l = lists[idx];
    memset(l, 0, sizeof lists[idx]);
    put32(l, 0x1c, flags); put32(l, 0x20, (uint32_t)state); putf(l, 0x24, end);
    uint64_t t = 0;
    for (uint64_t h = get64(base_list, 0xd0); h; h = get64((void *)(uintptr_t)h, 0x68)) t = h;
    if (t) put64((void *)(uintptr_t)t, 0x68, addr(l)); else put64(base_list, 0xd0, addr(l));
    put64(l, 0x78, addr(base_list));
}
static void chain_remove(uint64_t l) {
    uint64_t prev = get64((void *)(uintptr_t)l, 0x70), next = get64((void *)(uintptr_t)l, 0x68);
    if (prev) put64((void *)(uintptr_t)prev, 0x68, next);
    else if (get64(base_list, 0xd0) == l) put64(base_list, 0xd0, next);
    if (next) put64((void *)(uintptr_t)next, 0x70, prev);
    put64((void *)(uintptr_t)l, 0x68, 0); put64((void *)(uintptr_t)l, 0x70, 0);
}


static int apply_calls, apply_seq;
static uint64_t apply_entry;
static uint32_t flags_at_apply[NL];
static int body_creates, body_remove_first, body_refresh_writes;
static uint64_t body_return;
static uint32_t body_dur_written;
static int created;
static int unlinks[NL * 2], n_unlinks, frees, timers, timer_end, bits;
static uint64_t timer_unit_seen, stat_list; static int stat_calls, stat_id, stat_val, stat_ver, bit_state;
static int reenter; static uint64_t reenter_arg; static int apply_reenter;

static void test_unlink(unsigned ver, uint64_t l) {
    (void)ver;
    if (n_unlinks < NL * 2) unlinks[n_unlinks++] = (int)((l - addr(&lists[0][0])) / 0x70);
    chain_remove(l);
    if (reenter > 0) { reenter--; uint64_t r[1] = { reenter_arg }; svc_shrine_apply(r); }
}
static void test_free(unsigned ver, uint64_t l) { (void)ver; (void)l; frees++; }
static void test_timer(uint64_t t, uint64_t u, int end) { (void)t; timers++; timer_unit_seen = u; timer_end = end; }
static void test_stat(unsigned ver, uint64_t l, int stat, int value) { stat_calls++; stat_ver = (int)ver; stat_list = l; stat_id = stat; stat_val = value; }
static void test_statebit(uint64_t u, int state) { (void)u; bits++; bit_state = state; }

static uint64_t test_apply(uint64_t a, uint64_t entry) {
    apply_calls++; apply_seq++; apply_entry = entry;
    for (int i = 0; i < NL; i++) flags_at_apply[i] = get32(lists[i], 0x1c);
    if (apply_reenter > 0) { apply_reenter--; uint64_t r[1] = { addr(arg) }; uint64_t rr = 0; (void)rr; svc_shrine_apply(r); }
    if (body_remove_first) {
        uint32_t st = get32((void *)(uintptr_t)a, 0x24);
        for (uint64_t l = get64(base_list, 0xd0); l; l = get64((void *)(uintptr_t)l, 0x68))
            if ((get32((void *)(uintptr_t)l, 0x1c) & 0x20) && get32((void *)(uintptr_t)l, 0x20) != st) { game_unlink(1, l); test_free(1, l); break; }
    }
    put32((void *)(uintptr_t)a, 0x18, body_dur_written);
    if (!body_creates) {
        if (body_refresh_writes && body_return) putf((void *)(uintptr_t)body_return, 0x24, 3100.0f);
        return body_return;
    }
    if (created >= 2) return addr(lists[POOL]);
    uint8_t *l = lists[POOL + created++];
    memset(l, 0, sizeof lists[POOL]);
    put32(l, 0x1c, 0x22u);
    put32(l, 0x20, get32((void *)(uintptr_t)a, 0x24));
    putf(l, 0x24, 3100.0f);
    uint64_t head = get64(base_list, 0xd0);
    put64(l, 0x68, head); if (head) put64((void *)(uintptr_t)head, 0x70, addr(l));
    put64(l, 0x78, addr(base_list)); put64(base_list, 0xd0, addr(l));
    return addr(l);
}


static void reset(void) {
    memset(unit, 0, sizeof unit); memset(base_list, 0, sizeof base_list); memset(lists, 0, sizeof lists);
    memset(D, 0, sizeof D); memset(curse_bits, 0, sizeof curse_bits);
    memset(timer_obj, 0, sizeof timer_obj); memset(arg, 0, sizeof arg); memset(arg2, 0, sizeof arg2); memset(image_win, 0, sizeof image_win);
    put64(image_win, 16 * 1, addr(D));
    put64(D, 0x360, addr(curse_bits));
    for (int s = 128; s <= 137; s++) put32(curse_bits, 4 * (s >> 5), get32(curse_bits, 4 * (s >> 5)) | (1u << (s & 31)));
    put32(curse_bits, 4 * (9 >> 5), get32(curse_bits, 4 * (9 >> 5)) | (1u << (9 & 31)));
    put32(curse_bits, 4 * (60 >> 5), get32(curse_bits, 4 * (60 >> 5)) | (1u << (60 & 31)));
    unit[0x1bd] = 1;
    put64(unit, 0x88, addr(base_list));
    put64(unit, 0xd8, addr(timer_obj));
    put32(base_list, 0x1c, 0x80000000u);
    for (int i = 0; i < 2; i++) { uint8_t *aa = i ? arg2 : arg; put64(aa, 0x00, addr(unit)); put64(aa, 0x08, addr(unit)); put32(aa, 0x18, 100); put32(aa, 0x1c, 0xffffffffu); }
    g_nmod = 1; strcpy(g_mod[0].name, "main"); g_mod[0].base = image_win - GA_DATA_TABLES;
    sw_stack = 1; sw_speed = 1;
    apply_calls = 0; apply_seq = 0; apply_entry = 0; n_unlinks = 0; frees = 0; timers = 0; timer_end = 0; bits = 0;
    stat_calls = 0; stat_list = 0; stat_id = 0; stat_val = 0; stat_ver = 0; reenter = 0; reenter_arg = 0; apply_reenter = 0;
    body_creates = 1; body_remove_first = 0; body_refresh_writes = 0; body_return = 0; body_dur_written = 100; created = 0;
    memset(flags_at_apply, 0, sizeof flags_at_apply);
}
static uint64_t call(uint64_t a) { uint64_t regs[1] = { a }; svc_shrine_apply(regs); return regs[0]; }
static uint64_t top(void) { return addr(arg); }
static int had_unlink(int idx) { for (int i = 0; i < n_unlinks; i++) if (unlinks[i] == idx) return 1; return 0; }
static int count_unlink(int idx) { int n = 0; for (int i = 0; i < n_unlinks; i++) if (unlinks[i] == idx) n++; return n; }
static int in_chain(uint64_t l) { for (uint64_t h = get64(base_list, 0xd0); h; h = get64((void *)(uintptr_t)h, 0x68)) if (h == l) return 1; return 0; }

int main(void) {

    reset(); link_list(0, 128, 5000.0f, 0x22); link_list(1, 60, 4000.0f, 0x22);
    sw_stack = 0; sw_speed = 0; put32(arg, 0x24, 128);
    CHECK(call(top()) == addr(lists[POOL]), "forward returns the body's list");
    CHECK(apply_calls == 1 && apply_entry == addr(image_win) - GA_DATA_TABLES + GA_AT_STATE_APPLY_TIMED_BODY, "the forward tail-calls just past the cut");
    CHECK(n_unlinks == 0 && timers == 0 && stat_calls == 0 && bits == 0, "nothing else runs with both switches off");
    CHECK(get32(lists[0], 0x1c) == 0x22 && get32(lists[1], 0x1c) == 0x22, "no list is masked with both switches off");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 62);
    call(top());
    CHECK(apply_calls == 1 && n_unlinks == 0 && bits == 0, "an open-wounds application removes nothing");
    CHECK(flags_at_apply[0] == 0x22, "the curse bit of a shrine stays for a non-curse application");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 129);
    call(top());
    CHECK((flags_at_apply[0] & 0x20u) == 0, "the other shrine's curse bit is masked while the body walks");
    CHECK(get32(lists[0], 0x1c) == 0x22, "the curse bit is restored afterwards");
    CHECK(in_chain(addr(lists[0])) && in_chain(addr(lists[POOL])), "both shrines now hang in the chains");
    CHECK(n_unlinks == 0 && timers == 0 && bits == 0, "a fresh shrine without the same state removes and re-arms nothing");
    CHECK(stat_calls == 0, "no speed stat outside the stamina shrine");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 128);
    call(top());
    CHECK(count_unlink(0) == 1, "the old same-state list is removed once");
    CHECK(bits == 1 && bit_state == 128, "the state bit is set again after removing the old list");
    CHECK(getf(lists[POOL], 0x24) == 5100.0f, "the fresh list ends at old end + granted duration");
    CHECK(timers == 1 && timer_end == 5100 && timer_unit_seen == addr(unit), "the state timer is re-armed to the sum");
    CHECK(in_chain(addr(lists[POOL])) && !in_chain(addr(lists[0])), "only the fresh list survives");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 128); body_dur_written = 40;
    call(top());
    CHECK(getf(lists[POOL], 0x24) == 5040.0f && timer_end == 5040, "resistance reduces the added duration once, not the sum");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 128); body_dur_written = 0;
    uint64_t r6 = call(top());
    CHECK(r6 == addr(lists[0]), "the caller gets the surviving old list back, never the removed fresh one");
    CHECK(n_unlinks == 1 && count_unlink(POOL) == 1 && bits == 1, "the zero-duration fresh list is removed and the bit restored");
    CHECK(in_chain(addr(lists[0])) && !in_chain(addr(lists[POOL])), "the old list survives, the fresh one is gone");
    CHECK(getf(lists[0], 0x24) == 5000.0f && timers == 0, "the old end is untouched, no timer re-armed");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 128); body_creates = 0; body_return = addr(lists[0]); body_refresh_writes = 1;
    call(top());
    CHECK(n_unlinks == 0 && bits == 0, "a body refresh leaves the list alone");
    CHECK(getf(lists[0], 0x24) == 5100.0f && timers == 1 && timer_end == 5100, "the sum is written onto the refreshed list");


    reset(); link_list(0, 60, 4000.0f, 0x22); link_list(1, 128, 5000.0f, 0x22); link_list(2, 130, 5000.0f, 0x22); link_list(3, 131, 5000.0f, 0x22);
    put32(arg, 0x24, 9); body_remove_first = 1;
    call(top());
    CHECK(count_unlink(0) == 1, "the old enemy curse is removed exactly once (by the body, never twice)");
    CHECK(had_unlink(1) && had_unlink(2) && had_unlink(3), "all shrines are stripped by one curse");
    CHECK(n_unlinks == 4, "nothing else is removed");
    CHECK(!in_chain(addr(lists[1])) && !in_chain(addr(lists[2])) && !in_chain(addr(lists[3])), "the chains end without any shrine");


    reset(); link_list(0, 128, 5000.0f, 0x22); link_list(1, 130, 5000.0f, 0x22);
    put32(arg, 0x24, 9); body_creates = 0; body_return = 0;
    call(top());
    CHECK(n_unlinks == 0 && bits == 0, "a rejected curse strips nothing");
    CHECK(in_chain(addr(lists[0])) && in_chain(addr(lists[1])), "both shrines survive a rejected curse");


    reset(); link_list(0, 60, 4000.0f, 0x22); link_list(1, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 129); body_remove_first = 1;
    call(top());
    CHECK(count_unlink(0) == 1, "the enemy curse goes exactly once");
    CHECK((flags_at_apply[1] & 0x20u) == 0 && get32(lists[1], 0x1c) == 0x22, "the other shrine was masked and restored");
    CHECK(in_chain(addr(lists[1])) && in_chain(addr(lists[POOL])), "shrine and fresh list coexist after lifting the curse");


    reset(); link_list(0, 60, 4000.0f, 0x22); link_list(1, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 128); body_remove_first = 1;
    call(top());
    CHECK(count_unlink(0) == 1 && count_unlink(1) == 1, "the curse (by the body) and the old shrine (by the wrapper) each go once");
    CHECK(bits == 1 && bit_state == 128, "the state bit is set for the surviving fresh list");
    CHECK(getf(lists[POOL], 0x24) == 5100.0f && timers == 1 && timer_end == 5100, "the duration still sums onto the fresh list");
    CHECK(!in_chain(addr(lists[0])) && !in_chain(addr(lists[1])) && in_chain(addr(lists[POOL])), "one 128 list survives: the fresh one");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    sw_stack = 0; put32(arg, 0x24, 136);
    call(top());
    CHECK((flags_at_apply[0] & 0x20u) != 0, "without stacking nothing is masked");
    CHECK(stat_calls == 1 && stat_list == addr(lists[POOL]) && stat_id == 96 && stat_val == 50 && stat_ver == 1, "the fresh stamina list carries +50 run speed");
    CHECK(n_unlinks == 0 && bits == 0 && timers == 0, "everything else stays vanilla");


    reset(); link_list(0, 136, 5000.0f, 0x22);
    put32(arg, 0x24, 136);
    call(top());
    CHECK(stat_calls == 1 && stat_list == addr(lists[POOL]), "the fresh stamina list carries +50 run speed");
    CHECK(getf(lists[POOL], 0x24) == 5100.0f && count_unlink(0) == 1, "the stamina shrine sums its duration like any shrine");


    reset(); link_list(0, 136, 5000.0f, 0x22);
    put32(arg, 0x24, 136); body_dur_written = 0;
    call(top());
    CHECK(stat_calls == 1 && stat_list == addr(lists[0]), "the surviving old list carries the run stat");


    reset(); link_list(0, 60, 4000.0f, 0x22); link_list(1, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 9); put32(arg2, 0x24, 9);
    reenter = 1; reenter_arg = addr(arg2); body_remove_first = 0;
    call(top());

    CHECK(count_unlink(0) <= 1 && count_unlink(1) <= 1 && count_unlink(POOL) <= 1, "no list is removed twice across the re-entry");
    CHECK(had_unlink(1), "the shrine went in one of the two passes");
    CHECK(n_unlinks == 2, "two lists gone (the shrine by each pass's strip, counted once) and no more");


    reset();
    for (int i = 0; i < MAX_MASK + 1; i++) link_list(i, 129 + (i % 9), 5000.0f, 0x22);
    put32(arg, 0x24, 128);
    call(top());
    CHECK(n_unlinks == 0 && bits == 0, "a full mask table leaves the application vanilla");
    for (int i = 0; i < MAX_MASK + 1; i++) CHECK(flags_at_apply[i] == 0x22 && get32(lists[i], 0x1c) == 0x22, "no masked bit survives the walk that gave up");


    reset();
    for (int i = 0; i < 9; i++) link_list(i, 128 + i, 5000.0f, 0x22);
    put32(arg, 0x24, 137);
    call(top());
    CHECK(n_unlinks == 0 && bits == 0, "nine stacked shrines simply stack");
    { int ok = 1; for (int i = 0; i < 9; i++) ok &= in_chain(addr(lists[i])); CHECK(ok, "all nine shrine lists survive"); }
    reset();
    for (int i = 0; i < MAX_SAME + 1; i++) link_list(i, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 128);
    call(top());
    CHECK(n_unlinks == 0 && bits == 0, "a same-state table overflow leaves the application vanilla");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put64(arg, 0x00, 0);
    put32(arg, 0x24, 128);
    call(top());
    CHECK(apply_calls == 1 && n_unlinks == 0 && (flags_at_apply[0] & 0x20u) != 0, "a NULL source only forwards");
    reset(); link_list(0, 128, 5000.0f, 0x22);
    unit[0x1bd] = 9;
    put32(arg, 0x24, 128);
    call(top());
    CHECK(apply_calls == 1 && n_unlinks == 0 && (flags_at_apply[0] & 0x20u) != 0, "a bad version byte only forwards");
    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 0x80000000u);
    call(top());
    CHECK(apply_calls == 1 && n_unlinks == 0 && (flags_at_apply[0] & 0x20u) != 0, "state bit 31 only forwards");


    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 129); apply_reenter = MAX_DEPTH + 2;
    CHECK(call(top()) != 0, "the guarded run still returns the body's list");
    CHECK(apply_calls == MAX_DEPTH + 3, "the guard keeps rewriting off past the depth, the body still runs");
    reset(); link_list(0, 128, 5000.0f, 0x22);
    put32(arg, 0x24, 129);
    call(top());
    CHECK((flags_at_apply[0] & 0x20u) == 0, "after a guarded run the next application masks again (depth unwound)");

    fprintf(stderr, "t_shrine: %d checks, %d failures\n", checks, fails);
    return fails != 0;
}
