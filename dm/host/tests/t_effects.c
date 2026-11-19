







#define DM_EFFECT_TESTS 1
#include "../dm_effects.c"
#include <assert.h>

module_t g_mod[MAX_MOD];
int g_nmod;
static int checks, fails;
#define CHECK(c, msg) do { checks++; if (!(c)) { fails++; fprintf(stderr, "FAIL: %s (line %d)\n", msg, __LINE__); } } while (0)

static uint8_t unit[0x100], statlist[0xc00], game[0x200], lists[300][0x70];
static uint32_t pairs[300][4][2];
static int statv[512];


static uint8_t cunit[0x120], cstat[0xc00], chain[16], nodes[70][0x48], rows[70][4], ginfo[0x60];
int dm_guest_heap_readable(uint64_t a, size_t n) {
    const struct { const void *p; size_t n; } ok[] = { { unit, sizeof unit }, { statlist, sizeof statlist }, { game, sizeof game }, { lists, sizeof lists }, { pairs, sizeof pairs },
        { cunit, sizeof cunit }, { cstat, sizeof cstat }, { chain, sizeof chain }, { nodes, sizeof nodes }, { rows, sizeof rows }, { ginfo, sizeof ginfo } };
    for (unsigned i = 0; i < sizeof ok / sizeof *ok; i++) { uint64_t b = (uint64_t)(uintptr_t)ok[i].p; if (a >= b && n <= ok[i].n && a - b <= ok[i].n - n) return 1; }
    return 0;
}
static int getstat(uint64_t u, int stat, int layer) { (void)u; (void)layer; return stat >= 0 && stat < 512 ? statv[stat] : 0; }
static int session = 7, calls;

static int shrine_in[3], shrine_calls; static unsigned shrine_ok = 0xd;
unsigned dm_itemcmp_shrine_values(uint8_t *b, uint64_t pl, int d171, int d19, int d25, int *o) {
    (void)b; (void)pl; shrine_calls++; shrine_in[0] = d171; shrine_in[1] = d19; shrine_in[2] = d25;
    for (int i = 0; i < 8; i++) o[i] = 100 + i;
    return shrine_ok;
}
int dm_build_session(void) { return session; }
static uint64_t clock_ms = 50000;
static uint64_t fake_clock(void) { return clock_ms; }

static int skill_bonus(uint64_t pl, uint64_t node, int flag) { (void)pl; (void)flag; calls++; int32_t base; memcpy(&base, (const void *)(uintptr_t)(node + 0x38), 4); return base + 1; }


static uint8_t image[0x2d20c00];

static void put32(void *p, size_t at, uint32_t v) { memcpy((uint8_t *)p + at, &v, 4); }
static void put64(void *p, size_t at, uint64_t v) { memcpy((uint8_t *)p + at, &v, 8); }
static void putf(void *p, size_t at, float v) { memcpy((uint8_t *)p + at, &v, 4); }
static void bit(int state) { uint32_t w; memcpy(&w, statlist + 0xb30 + 4 * (state >> 5), 4); w |= 1u << (state & 31); put32(statlist, 0xb30 + 4 * (state >> 5), w); }
static uint64_t addr(const void *p) { return (uint64_t)(uintptr_t)p; }

static void list(int i, int state, float expiry, int skill, int level, int nstats, const int (*st)[2]) {
    memset(lists[i], 0, sizeof lists[i]);
    put32(lists[i], 0x1c, isnan(expiry) ? 0 : 2); put32(lists[i], 0x20, (uint32_t)state); putf(lists[i], 0x24, isnan(expiry) ? 0 : expiry);
    put32(lists[i], 0x28, (uint32_t)skill); put32(lists[i], 0x2c, (uint32_t)level);
    for (int k = 0; k < nstats; k++) { pairs[i][k][0] = (uint32_t)st[k][0] << 16; pairs[i][k][1] = (uint32_t)st[k][1]; }
    put64(lists[i], 0x30, nstats ? addr(pairs[i]) : 0); put32(lists[i], 0x38, (uint32_t)nstats);
}
static void link(int from, int to) { put64(lists[from], 0x68, to < 0 ? 0 : addr(lists[to])); }
static void reset(void) {
    memset(unit, 0, sizeof unit); memset(statlist, 0, sizeof statlist); memset(game, 0, sizeof game); memset(lists, 0, sizeof lists);
    put64(unit, 0x88, addr(statlist)); put32(statlist, 0x1c, 0x80000000u); put32(game, 0x170, 1000);
}
static int out[DM_EFFECTS_WORDS];
static int run(int ticks) { for (int i = 0; i < ticks; i++) dm_effects_tick(addr(game), addr(unit)); return dm_effects_copy(out, DM_EFFECTS_WORDS); }

static int at(int s) { for (int i = 0, w = DM_EFFECTS_HEAD; i < out[1]; i++) { if (out[w] == s) return w; w += DM_FX_ENTRY + out[w + 5] * 3; } return -1; }

enum { E_LISTS = 4, E_N = 5, E_STAT = 6 };

int main(void) {
    setenv("DM_TEST_EFFECT_FAKE", "9:50:31=-50:L3:S66,10:-1,999:5,x", 1);
    g_nmod = 1; strcpy(g_mod[0].name, "main"); g_mod[0].base = (uint8_t *)(uintptr_t)getstat - 0x5b9290;
    statv[27] = 400; statv[9] = 100 << 8;
    g_now_ms = fake_clock;
    reset();

    for (int s = 100; s < 140; s++) bit(s);
    static const int mana[1][2] = { { 27, 400 } }, poison[1][2] = { { 74, -430 } }, two[2][2] = { { 26, 5 }, { 74, -100 } };
    list(0, 135, 3400.0f, 0, 0, 1, mana); bit(135);
    list(1, 2, 1100.5f, 0, 0, 1, poison); bit(2);
    list(2, 2, 1050.0f, 0, 0, 2, two);
    list(3, 33, NAN, 98, 8, 0, NULL); bit(33);
    put64(statlist, 0xd0, addr(lists[0])); link(0, 1); link(1, -1);
    put64(statlist, 0xd8, addr(lists[2])); link(2, 3); link(3, -1);
    int w = run(EVERY);
    CHECK(w > DM_EFFECTS_HEAD, "copy wrote entries");
    CHECK(out[1] == 44, "all 42 bitmap states (100-139 incl. 135, 2, 33) plus 2 fake ones (9, 10) are published, none capped at 32");
    CHECK(at(9) > 0 && out[at(9) + 3] == 50 && out[at(9) + 2] == 3 && out[at(9) + E_N] == 1 && out[at(9) + E_STAT] == 31 && out[at(9) + E_STAT + 2] == -50 && out[at(9) + E_LISTS] == 1 && out[at(9) + 1] == 66, "fake curse: 50 ticks, level 3, stat 31 = -50, source skill 66");
    CHECK(at(10) > 0 && out[at(10) + 3] == -1, "fake untimed state");
    CHECK(out[DM_FX_TICK] == 1000, "the head carries the tick of the reading");
    CHECK(out[DM_FX_VERSION] == DM_EFFECTS_VERSION, "the head carries the layout version");
    int p = at(135);
    CHECK(p > 0 && out[p + 3] == 2400 && out[p + E_N] == 1 && out[p + E_STAT] == 27 && out[p + E_STAT + 2] == 400 && out[p + E_LISTS] == 1, "mana shrine: 2400 ticks left, stat 27 = 400, one list");
    p = at(2);
    CHECK(p > 0 && out[p + 3] == 100 && out[p + E_N] == 2 && out[p + E_LISTS] == 2, "poison: two lists, longest time 100, two kinds of stat");
    int v74 = 0; for (int k = 0; p > 0 && k < out[p + E_N]; k++) if (out[p + E_STAT + 3 * k] == 74) v74 = out[p + E_STAT + 2 + 3 * k];
    CHECK(v74 == -530, "poison: life regeneration of both lists adds up to -530");
    p = at(33);
    CHECK(p > 0 && out[p + 1] == 98 && out[p + 2] == 8 && out[p + 3] == -1, "aura: skill 98 level 8, untimed = -1 (unknown, not permanent)");
    CHECK(at(120) > 0 && out[at(120) + E_N] == 0 && out[at(120) + E_LISTS] == 0, "a bitmap state without a list is there, without stats or lists");
    CHECK(out[DM_FX_MANA] == 0, "mana base 0 when the class table is not readable (version 0)");
    CHECK(out[DM_FX_MANA + 1] == 400, "head: stat 27 from the game's getter");
    int gen = out[0];
    run(EVERY);
    CHECK(out[0] == gen, "same content: no new generation");
    put32(game, 0x170, 1010); run(EVERY);
    CHECK(out[0] == gen + 1 && out[at(135) + 3] == 2390, "time goes on: new generation, 10 ticks less");
    CHECK(at(9) > 0 && out[at(9) + 3] == 40, "the fake state counts down with the game");
    put32(game, 0x170, 1060); run(EVERY);
    CHECK(at(9) < 0 && at(10) > 0, "the fake state disappears at 0, the untimed one stays");

    CHECK(ticks_left(NAN, 5) == -1 && ticks_left(INFINITY, 5) == -1, "NaN / infinity are unknown");
    CHECK(ticks_left(3.0f, 5) == 0, "a past expiry is 0");
    CHECK(ticks_left(1e20f, 5) == 0x7fffffff, "a huge expiry is capped");
    CHECK(ticks_left(16777216.0f + 2048.0f, 16777216u) == 2048, "ticks past 2^24 keep their precision");

    link(1, 0); run(EVERY);
    CHECK(at(2) > 0 && out[at(135) + E_STAT + 2] == 400 && out[at(135) + E_LISTS] == 1, "a looping chain stops and does not add a list twice");
    put64(lists[1], 0x68, 0x10); run(EVERY);
    CHECK(at(2) > 0, "an unreadable next pointer ends the chain cleanly");
    put64(lists[0], 0x30, 0x20); run(EVERY);
    CHECK(at(135) > 0 && out[at(135) + E_N] == 0, "an unreadable stat array is skipped, the state stays");

    put32(statlist, 0xb30 + 4 * (33 >> 5), 0); run(EVERY);
    CHECK(at(33) < 0, "a list whose state bit is off is ignored");

    int small[DM_EFFECTS_HEAD + 7]; int ws = dm_effects_copy(small, DM_EFFECTS_HEAD + 7);
    CHECK(ws <= DM_EFFECTS_HEAD + 7 && small[1] >= 1, "a small buffer gets the entries that fit");
    CHECK(dm_effects_copy(small, 3) == 0 && dm_effects_copy(small, DM_EFFECTS_HEAD - 1) == 0, "a buffer smaller than the head gets nothing");

    gen = out[0]; dm_effects_tick(addr(game), 0); for (int i = 1; i < EVERY; i++) dm_effects_tick(addr(game), 0); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[1] == 0 && out[0] != gen, "outside a game nothing is published");

    reset(); put32(statlist, 0x1c, 0); bit(2); run(EVERY);
    CHECK(out[1] == 0, "a stat list without the extended bit gives nothing");

    shrine_t sh[SHRINE_MAX];
    int ns = shrine_parse("12@50, 6@0,3@10,16@5,x,13@-1,15@20,7@4x,8", sh, SHRINE_MAX);
    CHECK(ns == 3 && sh[0].code == 12 && sh[0].at == 50 && sh[1].code == 6 && sh[1].at == 0 && sh[2].code == 15 && sh[2].at == 20 && !sh[0].done, "shrine switch: 12@50, 6@0 and 15@20 are taken, the rest skipped");
    CHECK(shrine_parse(NULL, sh, SHRINE_MAX) == 0 && shrine_parse("", sh, SHRINE_MAX) == 0, "no switch: no shrines");
    CHECK(shrine_parse("6@1,7@1,8@1", sh, 2) == 2, "at most max items");

    static uint8_t table[16 * 16]; uint64_t base = 0x7100000000ull;
    for (int c = SHRINE_FIRST; c <= SHRINE_LAST; c++) { put64(table, 16 * c, base + k_shrine_fn[c - SHRINE_FIRST]); put32(table, 16 * c + 8, 39); put32(table, 16 * c + 12, (uint32_t)k_shrine_state[c - SHRINE_FIRST]); }
    CHECK(shrine_entry_ok(table, base, 12) && shrine_entry_ok(table, base, 6) && shrine_entry_ok(table, base, 15), "the expected entries pass");
    CHECK(k_shrine_state[12 - SHRINE_FIRST] == 134 && k_shrine_fn[12 - SHRINE_FIRST] == 0x6b1870 && k_shrine_state[10 - SHRINE_FIRST] == 130, "code 12 is the skill shrine (134), code 10 lightning (130)");
    put64(table, 16 * 12, base + 0x6b16e0); CHECK(!shrine_entry_ok(table, base, 12), "a different handler fails");
    put64(table, 16 * 12, base + 0x6b1870); put32(table, 16 * 12 + 12, 135); CHECK(!shrine_entry_ok(table, base, 12), "a different state fails");
    CHECK(!shrine_entry_ok(table, base, 5) && !shrine_entry_ok(table, base, 16), "codes outside 6-15 fail");

    reset();
    for (int i = 0; i < DM_FXP_NSTATS; i++) statv[k_player_stats[i]] = 1000 + k_player_stats[i];
    put32(unit, 4, 7); put32(unit, 8, 42); put32(unit, 0xc, 1);
    run(EVERY);
    const int P = DM_FX_PLAYER;
    CHECK(out[P + DM_FXP_FLAGS] == DM_FXP_VALID, "player block valid; without a readable image no difficulty, classic or stamina flag");
    CHECK(out[P + DM_FXP_UNIT] == 42 && out[P + DM_FXP_CLASS] == 7 && out[P + DM_FXP_MODE] == 1 && out[P + DM_FXP_SESSION] == 7, "player block: unit id, class, mode, session");
    int same = 1; for (int i = 0; i < DM_FXP_NSTATS; i++) same &= out[P + DM_FXP_STATS + i] == 1000 + k_player_stats[i];
    CHECK(same, "player block: every stat from the game's getter, in the PLAYER_STATS order");
    CHECK(k_player_stats[11] == 13 && k_player_stats[17] == 39 && k_player_stats[29] == 85, "PLAYER_STATS order: experience at 11, fire resistance at 17, experience bonus at 29 (Java uses these)");
    CHECK(out[P + DM_FXP_AGE] == 0, "a fresh reading is 0 ms old");
    clock_ms += 1500; dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[P + DM_FXP_AGE] == 1500, "the age counts at copy time");
    g_img_test = image; g_img_test_len = sizeof image;
    image[DIFFICULTY] = 1; put64(image, GAME_INFO, addr(ginfo)); ginfo[0x5c] = 1;
    memcpy(image + STAMINA_CODE, k_stamina_code, sizeof k_stamina_code);
    { static const int32_t shifts[6] = { 8, 9, 8, 8, 8, 9 }; memcpy(image + STAMINA_SHIFTS, shifts, sizeof shifts); }
    run(EVERY);
    CHECK(out[P + DM_FXP_FLAGS] == (DM_FXP_VALID | DM_FXP_DIFF_OK | DM_FXP_CLASSIC_OK | DM_FXP_STAMINA_OK), "with the image: difficulty, classic and stamina flags");
    CHECK(out[P + DM_FXP_DIFF] == 1 && out[P + DM_FXP_CLASSIC] == 1, "difficulty nightmare, classic character");
    CHECK(out[P + DM_FXP_STAM_STAND] == 8 && out[P + DM_FXP_STAM_WALK] == 9 && out[P + DM_FXP_STAM_RUN] == 8, "stamina shifts: standing 8, walking 9, running 8");
    image[STAMINA_CODE + 40] ^= 1; run(EVERY);
    CHECK(!(out[P + DM_FXP_FLAGS] & DM_FXP_STAMINA_OK) && (out[P + DM_FXP_FLAGS] & DM_FXP_DIFF_OK) && out[P + DM_FXP_STAM_STAND] == 0, "a changed instruction word turns off only the stamina flag");
    image[STAMINA_CODE + 40] ^= 1; image[DIFFICULTY] = 5; run(EVERY);
    CHECK(!(out[P + DM_FXP_FLAGS] & DM_FXP_DIFF_OK) && (out[P + DM_FXP_FLAGS] & DM_FXP_STAMINA_OK), "a difficulty byte out of range turns off only the difficulty flag");
    image[DIFFICULTY] = 2; put64(image, GAME_INFO, 0); run(EVERY);
    CHECK(!(out[P + DM_FXP_FLAGS] & DM_FXP_CLASSIC_OK) && out[P + DM_FXP_DIFF] == 2, "no game info: only the classic flag is off");
    put64(image, GAME_INFO, addr(ginfo)); ginfo[0x5c] = 0; run(EVERY);
    CHECK(out[P + DM_FXP_CLASSIC] == 0 && (out[P + DM_FXP_FLAGS] & DM_FXP_CLASSIC_OK), "an expansion character");
    { uint8_t buf[8]; CHECK(!rd_main(sizeof image - 2, buf, 4) && rd_main(sizeof image - 4, buf, 4) && !rd_main(UINT64_MAX - 1, buf, 4), "rd_main reads only inside the image"); }
    gen = out[0]; statv[39] += 5; run(EVERY);
    CHECK(out[0] != gen && out[P + DM_FXP_STATS + 17] == 1044, "a changed total is new content: new generation");
    for (int i = 0; i < EVERY; i++) dm_effects_tick(addr(game), 0);
    dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[P + DM_FXP_FLAGS] == 0 && out[P + DM_FXP_AGE] == 0x7fffffff, "outside a game the player block is void and never read");


    const int S = DM_FX_SKILLS; uint8_t *fb = g_mod[0].base;
    put64(cunit, 0x88, addr(cstat)); put32(cstat, 0x1c, 0x80000000u); put32(cunit, 4, 7); put32(cunit, 8, 42); put64(cunit, 0x100, addr(chain)); put64(chain, 0, 0);
    memcpy(image + SKILL_FN, k_skill_fn, sizeof k_skill_fn); memcpy(image + SKILL_PLUS2, k_skill_plus2, sizeof k_skill_plus2);
    g_skill_fn_test = skill_bonus;
    dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_FLAGS] == (DM_FXS_VALID | DM_FXS_PLUS2_OK) && out[S + DM_FXS_COUNT] == 0 && out[S + DM_FXS_UNIT] == 42 && out[S + DM_FXS_SESSION] == 7, "no skills learned: a valid empty block");
    for (int i = 0; i < 66; i++) {
        memset(nodes[i], 0, sizeof nodes[i]); int16_t id = (int16_t)(300 + i); memcpy(rows[i], &id, 2);
        put64(nodes[i], 0, addr(rows[i])); put64(nodes[i], 8, i + 1 < 66 ? addr(nodes[i + 1]) : 0); put32(nodes[i], 0x38, (uint32_t)(i % 7 + 1)); put32(nodes[i], 0x44, 0xffffffffu);
    }
    put32(nodes[3], 0x44, 5); put32(nodes[4], 0x38, 0);
    put64(nodes[1], 8, 0); put64(chain, 0, addr(nodes[0]));
    calls = 0; dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_COUNT] == 2 && out[S + DM_FXS_ENTRIES] == 300 && out[S + DM_FXS_ENTRIES + 1] == 1 && out[S + DM_FXS_ENTRIES + 2] == 2 && out[S + DM_FXS_ENTRIES + 3] == 301 && calls == 2, "two learned skills: id, base level, bonus from the game's function");
    CHECK(out[S + DM_FXS_AGE] == 0 && !(out[S + DM_FXS_FLAGS] & DM_FXS_SHRINE), "fresh, no skill shrine");
    put64(nodes[1], 8, addr(nodes[2]));
    dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_COUNT] == 64 && out[S + DM_FXS_DROPPED] == 0, "66 nodes, one item-granted and one at base 0: 64 learned skills, all kept");
    put32(nodes[4], 0x38, 2);
    dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_COUNT] == 64 && out[S + DM_FXS_DROPPED] == 1, "a 65th learned skill is dropped and counted");
    put32(cstat, 0xb30 + 4 * (134 >> 5), 1u << (134 & 31));
    dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_FLAGS] & DM_FXS_SHRINE, "state 134 on the client player sets the shrine bit");
    image[SKILL_PLUS2 + 24] ^= 1; dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK((out[S + DM_FXS_FLAGS] & DM_FXS_VALID) && !(out[S + DM_FXS_FLAGS] & DM_FXS_PLUS2_OK), "a different +2 instruction turns off only the +2 flag");
    image[SKILL_PLUS2 + 24] ^= 1; image[SKILL_FN + 4] ^= 1; calls = 0;
    dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_FLAGS] == 0 && out[S + DM_FXS_COUNT] == 0 && calls == 0, "a different skill function: never called, no block");
    image[SKILL_FN + 4] ^= 1;


    reset(); put32(unit, 4, 7); put32(unit, 8, 42); run(EVERY);
    dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[P + DM_FXP_SESSION] == 7 && out[S + DM_FXS_SESSION] == 7 && out[P + DM_FXP_UNIT] == out[S + DM_FXS_UNIT], "same session and unit in both blocks");
    session = 8; dm_effects_pump(fb, addr(cunit)); dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[P + DM_FXP_SESSION] == 7 && out[S + DM_FXS_SESSION] == 8, "after a switch the skill block's session is new, the player block's still old");
    run(EVERY);
    CHECK(out[P + DM_FXP_SESSION] == 8, "the next reading brings the player block up to date");
    clock_ms += 2000; dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_AGE] == 2000 && out[P + DM_FXP_AGE] == 2000, "both ages grow while nothing is read");
    gen = out[0]; dm_effects_pump(0, 0); dm_effects_copy(out, DM_EFFECTS_WORDS);
    CHECK(out[S + DM_FXS_FLAGS] == 0 && out[S + DM_FXS_AGE] == 0x7fffffff && out[0] != gen, "an early exit of the main thread pump voids the skill block at once (new generation)");

    {   const int C = DM_FX_COMBAT; static const int armor[][2] = { { 171, 100 } }, combat[][2] = { { 19, 276 }, { 25, 200 } }, mixed[][2] = { { 171, 100 }, { 31, 5 } };
        put64(cunit, 0x88, addr(cstat)); memset(cstat + 0xb30, 0, 32); put64(cstat, 0xd0, 0); put64(cstat, 0xd8, 0);
        shrine_calls = 0; for (int i = 0; i < 10; i++) dm_effects_pump(fb, addr(cunit));
        dm_effects_copy(out, DM_EFFECTS_WORDS);
        CHECK(shrine_calls == 0 && out[C + DM_FXC_FLAGS] == 0, "no shrine on the client player: no combat values, the model is not asked");
        list(200, 128, 9000.f, 0, 0, 1, armor); link(200, -1); put64(cstat, 0xd0, addr(lists[200])); put32(cstat, 0xb30 + 4 * (128 >> 5), 1u << (128 & 31));
        for (int i = 0; i < 10; i++) dm_effects_pump(fb, addr(cunit));
        dm_effects_copy(out, DM_EFFECTS_WORDS);
        CHECK(shrine_calls >= 1 && shrine_in[0] == 100 && shrine_in[1] == 0 && shrine_in[2] == 0, "armor shrine: the client list's 171 = 100 is passed on");
        CHECK(out[C + DM_FXC_FLAGS] == (DM_FXC_VALID | DM_FXC_DEF_OK) && out[C + DM_FXC_STATE] == 128 && out[C + DM_FXC_V171] == 100 && out[C + DM_FXC_DEF_B] == 100 && out[C + DM_FXC_DEF_A] == 101 && out[C + DM_FXC_UNIT] == 42 && out[C + DM_FXC_AGE] == 0,
              "armor shrine: defense only (damage / attack rating not touched), before / after copied, fresh");
        list(200, 128, 9000.f, 0, 0, 2, mixed); for (int i = 0; i < 10; i++) dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
        CHECK(out[C + DM_FXC_FLAGS] == 0, "a shrine list with any other stat: no combat values");
        list(200, 129, 9000.f, 0, 0, 2, combat); put32(cstat, 0xb30 + 4 * (128 >> 5), 1u << (129 & 31));
        for (int i = 0; i < 10; i++) dm_effects_pump(fb, addr(cunit));
        dm_effects_copy(out, DM_EFFECTS_WORDS);
        CHECK(shrine_in[0] == 0 && shrine_in[1] == 276 && shrine_in[2] == 200 && out[C + DM_FXC_COMBAT + DM_FXC_STATE] == 129, "combat shrine: 19 = 276 and 25 = 200 from the client list, on the second slot");
        CHECK(out[C + DM_FXC_COMBAT + DM_FXC_FLAGS] == (DM_FXC_VALID | DM_FXC_AR_OK | DM_FXC_DMG_OK) && out[C + DM_FXC_FLAGS] == 0, "combat shrine: attack rating and damage on its slot, the armor slot stays empty");
        shrine_ok = 0x1; for (int i = 0; i < 10; i++) dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
        CHECK(out[C + DM_FXC_COMBAT + DM_FXC_FLAGS] == DM_FXC_VALID, "values the model may not show (R0 / blocked) stay off");
        put32(cstat, 0xb30 + 4 * (128 >> 5), 1u << (128 & 31)); for (int i = 0; i < 10; i++) dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
        CHECK(out[C + DM_FXC_FLAGS] == 0 && out[C + DM_FXC_COMBAT + DM_FXC_FLAGS] == 0, "state bit 128 but only a 129 list: nothing (the bit and the list must agree)");
        shrine_ok = 0xd;
        list(201, 128, 9000.f, 0, 0, 1, armor); link(201, 200); put64(cstat, 0xd0, addr(lists[201])); put32(cstat, 0xb30 + 4 * (128 >> 5), (1u << (128 & 31)) | (1u << (129 & 31)));
        int before = shrine_calls; for (int i = 0; i < 10; i++) dm_effects_pump(fb, addr(cunit)); dm_effects_copy(out, DM_EFFECTS_WORDS);
        CHECK(shrine_calls - before == 10, "stacked shrines: the model is asked once per pump, for both slots together");
        CHECK(out[C + DM_FXC_FLAGS] == (DM_FXC_VALID | DM_FXC_DEF_OK) && out[C + DM_FXC_STATE] == 128 && out[C + DM_FXC_V171] == 100 && out[C + DM_FXC_V19] == 0,
              "stacked: the armor slot carries 128 with its own 171 only");
        CHECK(out[C + DM_FXC_COMBAT + DM_FXC_FLAGS] == (DM_FXC_VALID | DM_FXC_AR_OK | DM_FXC_DMG_OK) && out[C + DM_FXC_COMBAT + DM_FXC_STATE] == 129 && out[C + DM_FXC_COMBAT + DM_FXC_V19] == 276
              && out[C + DM_FXC_COMBAT + DM_FXC_V171] == 0 && out[C + DM_FXC_COMBAT + DM_FXC_AGE] == 0, "stacked: the combat slot carries 129 with its own 19 / 25, fresh");
        CHECK(out[C + DM_FXC_DEF_A] == 101 && out[C + DM_FXC_COMBAT + DM_FXC_DMIN_A] == 106, "stacked: each slot keeps its own before / after numbers");
        put64(cstat, 0xd0, 0); shrine_ok = 0xd; }
    g_img_test = NULL; g_skill_fn_test = NULL;
    printf("t_effects: %d checks, %d failed\n", checks, fails);
    return fails != 0;
}
