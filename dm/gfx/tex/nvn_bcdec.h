



#pragma once
#include <stddef.h>
#include <stdint.h>



int nvn_bc_unpacked(uint32_t nvn_fmt, uint32_t *mtl_pf, int *bpp);


int nvn_bc_decode(uint32_t nvn_fmt, const uint8_t *src, size_t pitch, uint32_t w, uint32_t h, uint8_t *dst, size_t dpitch);
