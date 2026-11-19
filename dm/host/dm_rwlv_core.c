







#include "dm_rwlv_core.h"
#include <string.h>



static const struct { const char *code; uint8_t req; } k_anchor[] = {
    { "uhm", 59 },
    { "xhm", 25 },
    { "ci3", 64 },
    { "urn", 66 },
    { "ulm", 51 },
    { "r07", 17 },
    { "r13", 29 },
};
#define NANCHOR (int)(sizeof k_anchor / sizeof k_anchor[0])

int dm_rw_find_lvoff(const uint8_t *rows, uint32_t n, uint32_t rowsize) {
    const uint8_t *at[NANCHOR]; memset(at, 0, sizeof at);
    if (!rows || rowsize < 0x84) return -1;
    for (uint32_t i = 0; i < n; i++) { const uint8_t *r = rows + (uint64_t)i * rowsize;
        for (int k = 0; k < NANCHOR; k++) if (!at[k] && !memcmp(r + 0x80, k_anchor[k].code, 3) && (r[0x83] == 0 || r[0x83] == ' ')) at[k] = r; }
    for (int k = 0; k < NANCHOR; k++) if (!at[k]) return -1;
    int found = -1;
    for (uint32_t o = 0; o < rowsize; o++) { int ok = 1; for (int k = 0; k < NANCHOR && ok; k++) ok = at[k][o] == k_anchor[k].req;
        if (ok) { if (found >= 0) return -1; found = (int)o; } }
    return found;
}

int dm_rw_need_level(int base_req, const int *rune_req, int n) { int lv = base_req; for (int i = 0; i < n; i++) if (rune_req[i] > lv) lv = rune_req[i]; return lv; }


int dm_rw_allowed(uint8_t bits, int16_t first, int16_t last, int known, int offline, int ladder) {
    if (!known) return !bits && (first == 0 || last < 0);
    uint8_t mask = (offline || !ladder) ? 4 : 2;
    if (bits & mask) return 0;
    return offline || first == 0 || last < 0 || (first < 0 && ladder);
}
