

#pragma once
#include <stdint.h>

typedef struct {
    int target, format, flags, samples;
    uint32_t width, height, depth, levels, stride;
    int pkg_block_h;
} nvn_texdesc;

typedef struct {
    uint64_t size, layer_size;
    uint32_t align, layers, bpb;
    unsigned tileH, tileD;
} nvn_texlayout;

void nvn_tex_layout(const nvn_texdesc *d, nvn_texlayout *o);
uint64_t nvn_tex_level_offset(const nvn_texdesc *d, unsigned level);
int nvn_fmt_bpb(int format, int *bw, int *bh);


void nvn_tex_level_tiles(const nvn_texdesc *d, unsigned level, unsigned *tileH, unsigned *tileD);
