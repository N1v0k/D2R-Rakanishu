

#pragma once
#include <stdint.h>



enum { NVN_SWZ_ZERO = 0, NVN_SWZ_ONE = 1, NVN_SWZ_R = 2, NVN_SWZ_G = 3, NVN_SWZ_B = 4, NVN_SWZ_A = 5 };









int nvn_fmt_to_mtl(int nvn_format, uint32_t *mtl_pixel_format, uint8_t swz[4]);
