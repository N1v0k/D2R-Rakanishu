



#pragma once
#include <stdint.h>


enum {
    DM_BTN_A = 1u << 0, DM_BTN_B = 1u << 1, DM_BTN_X = 1u << 2, DM_BTN_Y = 1u << 3,
    DM_BTN_LS = 1u << 4, DM_BTN_RS = 1u << 5, DM_BTN_L = 1u << 6, DM_BTN_R = 1u << 7,
    DM_BTN_ZL = 1u << 8, DM_BTN_ZR = 1u << 9, DM_BTN_PLUS = 1u << 10, DM_BTN_MINUS = 1u << 11,
    DM_BTN_LEFT = 1u << 12, DM_BTN_UP = 1u << 13, DM_BTN_RIGHT = 1u << 14, DM_BTN_DOWN = 1u << 15,
};
#define DM_STICK_MAX 32767


enum { DM_GLYPH_PAD, DM_GLYPH_KBD, DM_GLYPH_XBOX, DM_GLYPH_PS, DM_GLYPH_N };
void dm_padmode_request_glyph(int glyph);
void dm_input_set(uint64_t buttons, int32_t lx, int32_t ly, int32_t rx, int32_t ry);
uint64_t dm_input_samples(void);



void dm_pad_set(uint64_t buttons, int32_t lx, int32_t ly, int32_t rx, int32_t ry);














void dm_input_sampled_right(int32_t *rx,int32_t *ry);



void dm_input_plus_reads(uint64_t *on, uint64_t *off);

void dm_pad_unlatch(uint64_t mask);



extern void (*dm_pad_hook)(uint64_t now, uint64_t *buttons, int32_t *lx, int32_t *ly, int32_t *rx, int32_t *ry);
