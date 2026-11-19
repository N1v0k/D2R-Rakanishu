


#pragma once
#include <stdint.h>


enum {
    DM_GLOSS_VERSION = 1,
    DM_GL_VER = 0, DM_GL_VALID = 1, DM_GL_SESSION = 2, DM_GL_CLASS = 3, DM_GL_LEVEL = 4,
    DM_GL_DIFF = 5,
    DM_GL_EXPANSION = 6,
    DM_GL_DEF = 7, DM_GL_BLOCK = 8, DM_GL_AR = 9,
    DM_GL_AC = 10, DM_GL_DEX = 11, DM_GL_ACPCT = 12, DM_GL_ACOVR = 13,
    DM_GL_STATE101 = 14,
    DM_GL_SKILLPCT = 15,
    DM_GL_GEN = 16,
    DM_GL_ARMOR_SHRINE = 17,
    DM_GL_STATS = 18,
    DM_GL_NSTATS = 22,
    DM_GLOSS_INTS = DM_GL_STATS + DM_GL_NSTATS,
};
#define DM_GL_STAT_IDS { 19, 20, 34, 35, 36, 39, 40, 41, 42, 43, 44, 45, 46, 60, 62, 93, 99, 105, 110, 135, 136, 141 }


void dm_glossary_want(int on);


void dm_glossary_pump(uint8_t *b, uint64_t pl);


int dm_glossary_copy(int *out, int n);
