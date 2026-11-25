





#include "../dm_rwlv_core.h"
#include <stdio.h>
#include <string.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define ROW 0x1c0
static uint8_t tab[16][ROW]; static int nrow;


static void row(const char *code, uint8_t level, uint8_t req, int lv_off, int req_off) { uint8_t *r = tab[nrow++]; memset(r, 0xee, ROW); memset(r + 0x80, 0, 4); memcpy(r + 0x80, code, 3); r[lv_off] = level; r[req_off] = req; }
static void build(int lv_off, int req_off) { nrow = 0; memset(tab, 0, sizeof tab);
    row("hlm", 11, 0, lv_off, req_off); row("xhm", 51, 25, lv_off, req_off); row("uhm", 79, 59, lv_off, req_off); row("ci3", 85, 64, lv_off, req_off); row("urn", 85, 66, lv_off, req_off); row("ulm", 68, 51, lv_off, req_off);
    row("r05", 15, 15, lv_off, req_off); row("r07", 17, 17, lv_off, req_off); row("r13", 29, 29, lv_off, req_off); }






static const struct { uint8_t bits; int16_t first, last; int known, offline, ladder, want; const char *why; } k_allow[] = {

    { 2, 3, 12, 1, 1, 0, 1, "Mosaic offline" },
    { 2, 3, 12, 1, 1, 1, 1, "Mosaic offline, G+0x108 set: offline still picks mask 4" },
    { 0, 0, -1, 1, 1, 0, 1, "plain runeword offline" },
    { 4, 0, -1, 1, 1, 0, 0, "value 4 (forbidden outside ladder) offline" },
    { 6, 0, -1, 1, 1, 1, 0, "both bits offline" },
    { 0, 3, 14, 1, 1, 0, 1, "season runeword (Mania) offline: no season check" },
    { 0, -1, 5, 1, 1, 0, 1, "negative first offline: no season check" },

    { 0, 0, 5, 1, 0, 0, 1, "first == 0" },
    { 2, 0, -1, 1, 0, 0, 1, "value 2 is not in mask 4, no season" },
    { 2, 3, 12, 1, 0, 0, 0, "Mosaic online non-ladder: season 3-12 rejects (first > 0)" },
    { 4, 0, -1, 1, 0, 0, 0, "value 4 in mask 4" },
    { 0, 3, -1, 1, 0, 0, 1, "last < 0" },
    { 0, 3, 12, 1, 0, 0, 0, "first > 0 and last >= 0" },
    { 0, -1, 12, 1, 0, 0, 0, "first < 0 needs a ladder game" },
    { 0, -1, -1, 1, 0, 0, 1, "last < 0 wins before first < 0" },

    { 2, 0, -1, 1, 0, 1, 0, "value 2 in mask 2" },
    { 4, 0, -1, 1, 0, 1, 1, "value 4 is not in mask 2" },
    { 6, 0, -1, 1, 0, 1, 0, "both bits, mask 2" },
    { 0, 3, 12, 1, 0, 1, 0, "first > 0 rejects even in ladder" },
    { 0, -1, 12, 1, 0, 1, 1, "first < 0 in ladder" },
    { 0, -1, 12, 1, 0, 5, 1, "any non-zero G+0x108 counts as ladder" },
    { 0, 0, 12, 1, 0, 1, 1, "first == 0 in ladder" },
    { 4, -1, 5, 1, 0, 1, 1, "value 4 + first < 0 in ladder" },

    { 0, 0, 5, 0, 0, 0, 1, "unknown, no lock, no season" },
    { 0, 3, -1, 0, 0, 0, 1, "unknown, last < 0" },
    { 0, 3, 12, 0, 1, 0, 0, "unknown, season runeword (even if offline was passed)" },
    { 2, 0, -1, 0, 1, 0, 0, "unknown, value 2" },
    { 4, 0, -1, 0, 0, 1, 0, "unknown, value 4" },
    { 2, 3, 12, 0, 1, 0, 0, "unknown, Mosaic" },
    { 0, -1, 3, 0, 0, 1, 0, "unknown, first < 0" },
};


static int server_branch(uint8_t bits, int16_t first, int16_t last, int offline, uint32_t g108) {
    static const uint32_t table[4] = { 1, 2, 4, 8 };
    uint32_t idx = offline ? 2 : (g108 == 0 ? 2 : 1);
    if (table[idx] & bits) goto reject;
    if (offline) goto ok;
    if (first == 0) goto ok;
    if (last < 0) goto ok;
    if (first > 0) goto reject;
    if (g108 != 0) goto ok;
reject: return 0;
ok: return 1;
}
static void rw_allowed_cases(void) {
    for (unsigned i = 0; i < sizeof k_allow / sizeof *k_allow; i++) { int got = dm_rw_allowed(k_allow[i].bits, k_allow[i].first, k_allow[i].last, k_allow[i].known, k_allow[i].offline, k_allow[i].ladder);
        if (got != k_allow[i].want) { printf("FAIL dm_rw_allowed row %u (%s): got %d want %d\n", i, k_allow[i].why, got, k_allow[i].want); fails++; } }
    static const uint8_t bits[] = { 0, 2, 4, 6 }; static const int16_t firsts[] = { 3, 0, -1 }, lasts[] = { -1, 0, 12 }; static const uint32_t g108s[] = { 0, 1, 7 };
    int n = 0;
    for (unsigned a = 0; a < 4; a++) for (int off = 0; off < 2; off++) for (unsigned l = 0; l < 3; l++) for (unsigned f = 0; f < 3; f++) for (unsigned z = 0; z < 3; z++, n++) {
        int want = server_branch(bits[a], firsts[f], lasts[z], off, g108s[l]), got = dm_rw_allowed(bits[a], firsts[f], lasts[z], 1, off, g108s[l] != 0);
        if (got != want) { printf("FAIL dm_rw_allowed grid bits %u first %d last %d offline %d G+0x108 %u: got %d want %d\n", bits[a], firsts[f], lasts[z], off, g108s[l], got, want); fails++; }

        int every = 1; for (int o2 = 0; o2 < 2; o2++) for (unsigned l2 = 0; l2 < 3; l2++) every &= server_branch(bits[a], firsts[f], lasts[z], o2, g108s[l2]);
        if (dm_rw_allowed(bits[a], firsts[f], lasts[z], 0, off, g108s[l] != 0) != every) { printf("FAIL dm_rw_allowed unknown bits %u first %d last %d: want %d\n", bits[a], firsts[f], lasts[z], every); fails++; } }
    if (n != 216) { printf("FAIL grid size %d\n", n); fails++; }
}
int main(void) {


    build(0x57, 0x5b); CHECK(dm_rw_find_lvoff(&tab[0][0], (uint32_t)nrow, ROW) == 0x5b);


    build(0x5b, 0x57); CHECK(dm_rw_find_lvoff(&tab[0][0], (uint32_t)nrow, ROW) == 0x57);


    build(0x57, 0x5b); { int o = dm_rw_find_lvoff(&tab[0][0], (uint32_t)nrow, ROW); CHECK(o >= 0 && tab[2][o] == 59); }


    build(0x57, 0x5b); CHECK(dm_rw_find_lvoff(&tab[6][0], 3, ROW) == -1);
    build(0x57, 0x5b); tab[2][0x5b] = 60; CHECK(dm_rw_find_lvoff(&tab[0][0], (uint32_t)nrow, ROW) == -1);
    CHECK(dm_rw_find_lvoff(NULL, 0, ROW) == -1);


    { int hearth[] = {29, 35, 23}, cure[] = {29, 35, 17}, bulwark[] = {29, 35, 27}, delirium[] = {43, 51, 35};
      CHECK(dm_rw_need_level(59, hearth, 3) == 59);
      CHECK(dm_rw_need_level(59, cure, 3) == 59);
      CHECK(dm_rw_need_level(59, bulwark, 3) == 59);
      CHECK(dm_rw_need_level(59, delirium, 3) == 59);


      CHECK(dm_rw_need_level(25, hearth, 3) == 35);
      CHECK(dm_rw_need_level(25, delirium, 3) == 51);
      CHECK(dm_rw_need_level(66, delirium, 3) == 66);
      CHECK(dm_rw_need_level(64, cure, 3) == 64); }
    CHECK(dm_rw_need_level(0, NULL, 0) == 0);
    rw_allowed_cases();
    if (fails) { printf("t_rwlv: %d 处失败\n", fails); return 1; }
    printf("t_rwlv OK\n"); return 0;
}
