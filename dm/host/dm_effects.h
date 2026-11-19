

#pragma once
#include <stdint.h>




enum {
    DM_EFFECTS_VERSION = 4,


    DM_EFFECTS_MAX = 232, DM_EFFECTS_STATS = 12,
    DM_FX_GEN = 0, DM_FX_COUNT = 1, DM_FX_TICK = 2, DM_FX_VERSION = 3,
    DM_FX_MANA = 4, DM_FX_MANA_LEN = 4,
    DM_FX_PLAYER = 8, DM_FX_PLAYER_LEN = 48,
    DM_FX_SKILLS = 56, DM_FX_SKILLS_MAX = 64, DM_FX_SKILLS_LEN = 8 + 3 * 64,
    DM_FX_COMBAT = 256, DM_FX_COMBAT_LEN = 32,
    DM_EFFECTS_HEAD = 288,
    DM_FX_ENTRY = 6,
    DM_EFFECTS_WORDS = DM_EFFECTS_HEAD + DM_EFFECTS_MAX * (DM_FX_ENTRY + DM_EFFECTS_STATS * 3),
};




enum {
    DM_FXP_FLAGS = 0, DM_FXP_SESSION = 1, DM_FXP_UNIT = 2, DM_FXP_CLASS = 3, DM_FXP_AGE = 4, DM_FXP_DIFF = 5, DM_FXP_CLASSIC = 6, DM_FXP_MODE = 7,
    DM_FXP_STAM_STAND = 8, DM_FXP_STAM_WALK = 9, DM_FXP_STAM_RUN = 10, DM_FXP_STATS = 12, DM_FXP_NSTATS = 30,
    DM_FXP_VALID = 1, DM_FXP_DIFF_OK = 2, DM_FXP_CLASSIC_OK = 4, DM_FXP_STAMINA_OK = 8,
};





#define DM_FX_PLAYER_STAT_IDS { 0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 26, 27, 28, 74, 36, 39, 40, 41, 42, 43, 44, 45, 46, 60, 62, 79, 80, 85 }




enum {
    DM_FXS_FLAGS = 0, DM_FXS_SESSION = 1, DM_FXS_UNIT = 2, DM_FXS_CLASS = 3, DM_FXS_AGE = 4, DM_FXS_COUNT = 5, DM_FXS_DROPPED = 6, DM_FXS_ENTRIES = 8,
    DM_FXS_VALID = 1, DM_FXS_SHRINE = 2, DM_FXS_PLUS2_OK = 4,
};







enum {
    DM_FXC_LEN = 16, DM_FXC_ARMOR = 0, DM_FXC_COMBAT = 16,
    DM_FXC_FLAGS = 0, DM_FXC_SESSION = 1, DM_FXC_UNIT = 2, DM_FXC_AGE = 3, DM_FXC_STATE = 4, DM_FXC_V171 = 5, DM_FXC_V19 = 6, DM_FXC_V25 = 7,
    DM_FXC_DEF_B = 8, DM_FXC_DEF_A = 9, DM_FXC_AR_B = 10, DM_FXC_AR_A = 11, DM_FXC_DMIN_B = 12, DM_FXC_DMAX_B = 13, DM_FXC_DMIN_A = 14, DM_FXC_DMAX_A = 15,
    DM_FXC_VALID = 1, DM_FXC_DEF_OK = 2, DM_FXC_AR_OK = 4, DM_FXC_DMG_OK = 8,
};
_Static_assert(DM_FX_SKILLS + DM_FX_SKILLS_LEN <= DM_FX_COMBAT && DM_FX_COMBAT + DM_FX_COMBAT_LEN <= DM_EFFECTS_HEAD, "combat block overlaps");



unsigned dm_itemcmp_shrine_values(uint8_t *b, uint64_t pl, int d171, int d19, int d25, int *out);
_Static_assert(DM_FX_MANA + DM_FX_MANA_LEN <= DM_FX_PLAYER && DM_FX_PLAYER + DM_FX_PLAYER_LEN <= DM_FX_SKILLS && DM_FX_SKILLS + DM_FX_SKILLS_LEN <= DM_EFFECTS_HEAD, "effects head blocks overlap");
_Static_assert(DM_FXP_STATS + DM_FXP_NSTATS <= DM_FX_PLAYER_LEN && DM_FXS_ENTRIES + 3 * DM_FX_SKILLS_MAX <= DM_FX_SKILLS_LEN, "effects block too small");






int dm_effects_copy(int *out, int max);

enum { DM_EFFECTS_STATE_WORDS = 8 };
unsigned dm_effects_state_bits(uint32_t out[DM_EFFECTS_STATE_WORDS]);
void dm_effects_tick(uint64_t G, uint64_t player);
void dm_effects_pump(uint8_t *b, uint64_t player);
