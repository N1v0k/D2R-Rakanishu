#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xbg_framebuffer_attachment {
    uint32_t active,width,height,levels,layers;
    uint32_t base_level,level_count,base_layer,layer_count,samples;
} xbg_framebuffer_attachment;
typedef struct xbg_framebuffer_extent { uint32_t width,height,layers,samples; } xbg_framebuffer_extent;




int xbg_framebuffer_plan(const xbg_framebuffer_attachment *attachments,uint32_t count,
    uint32_t max_width,uint32_t max_height,xbg_framebuffer_extent *out,uint32_t *bad_index);
const char *xbg_framebuffer_error(int result);
#ifdef __cplusplus
}
#endif
