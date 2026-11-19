#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xbg_depth_format { uint32_t packed_bytes,depth_bytes,stencil_bytes; } xbg_depth_format;
int xbg_depth_format_info(uint32_t nvn_format,xbg_depth_format *out);







int xbg_depth_extract(uint32_t format,uint32_t width,uint32_t height,uint32_t slices,
    const void *packed,size_t packed_bytes,size_t row_stride,size_t image_stride,
    void *depth,size_t depth_bytes,void *stencil,size_t stencil_bytes);
int xbg_depth_merge(uint32_t format,uint32_t width,uint32_t height,uint32_t slices,
    void *packed,size_t packed_bytes,size_t row_stride,size_t image_stride,
    const void *depth,size_t depth_bytes,const void *stencil,size_t stencil_bytes);
#ifdef __cplusplus
}
#endif
