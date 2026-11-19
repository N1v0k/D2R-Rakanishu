#pragma once
#include "xbg_draw_plan.h"
#ifdef __cplusplus
extern "C" {
#endif

int xbg_quad_index_count(int32_t vertices,uint32_t *indices);

int xbg_quad_plan(const xbg_draw_call *source,xbg_draw_call *triangles,uint32_t *indices,size_t capacity);
#ifdef __cplusplus
}
#endif
