






#ifndef DM_SWAPSKILL_CORE_H
#define DM_SWAPSKILL_CORE_H
#include <stdint.h>

enum { DM_SS_SKILLS = 512, DM_SS_STAT_OSKILL = 97, DM_SS_STAT_SINGLE = 107 };



static inline void dm_ss_scan_item(int (*get)(void *, int, int), void *ctx, int char_class, int (*skill_class)(int), uint64_t oskill[8], uint64_t single[8]) {
    for (int sk = 0; sk < DM_SS_SKILLS; sk++) {
        if (get(ctx, DM_SS_STAT_OSKILL, sk) > 0) oskill[sk >> 6] |= 1ull << (sk & 63);
        if (char_class >= 0 && get(ctx, DM_SS_STAT_SINGLE, sk) > 0 && skill_class(sk) == char_class) single[sk >> 6] |= 1ull << (sk & 63);
    }
}

static inline int dm_ss_has(const uint64_t mask[8], int skill) { return skill >= 0 && skill < DM_SS_SKILLS && (mask[skill >> 6] >> (skill & 63) & 1); }




static inline void dm_ss_mark_present(uint64_t present[8], const uint64_t single[8], int skill, int32_t item, int level) {
    if (item == -1 && level > 0 && dm_ss_has(single, skill)) present[skill >> 6] |= 1ull << (skill & 63);
}



static inline void dm_ss_given(const uint64_t oskill[8], const uint64_t single[8], const uint64_t present[8], uint64_t out[8]) {
    for (int i = 0; i < 8; i++) out[i] = oskill[i] | (single[i] & ~present[i]);
}



static inline int dm_ss_learned_offhand(int learned_level, int swap_set_gives) { return learned_level <= 0 && swap_set_gives; }
#endif
