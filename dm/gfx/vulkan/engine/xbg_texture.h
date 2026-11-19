#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif




enum xbg_pixels { XBG_RGBA8_UNORM=1, XBG_RGBA8_SRGB=2,
                  XBG_R16_UNORM=3, XBG_RG16_UNORM=4,
                  XBG_R16_SNORM=5, XBG_RG16_SNORM=6, XBG_RGBA16_SFLOAT=7,
                  XBG_R8_UNORM=8, XBG_R8_SNORM=9, XBG_RG8_UNORM=10, XBG_RG8_SNORM=11 };




extern int xbg_bc_8bit_channels;
typedef struct xbg_bc_format {
    uint32_t pixels, bytes_per_pixel, bytes_per_block;
} xbg_bc_format;



int xbg_bc_format_info(uint32_t nvn_format,xbg_bc_format *out);


int xbg_bc_view_compatible(uint32_t source_format,uint32_t view_format);
typedef struct xbg_texture {
    uint32_t width, height, levels, source_code, pixels, bytes_per_pixel;
    uint32_t block_linear, block_height_log2, nvn_format, header_bytes;
    uint64_t offsets[16];
    uint32_t sizes[16];
} xbg_texture;



typedef struct xbg_nvn_texture_desc {
    uint32_t format,width,height,levels,block_height_log2;
} xbg_nvn_texture_desc;
int xbg_nvn_texture_layout(const xbg_nvn_texture_desc *description,size_t storage_bytes,xbg_texture *out);
int xbg_nvn_texture_decode_mip(const void *storage,size_t storage_bytes,
    const xbg_nvn_texture_desc *description,uint32_t level,void *out,size_t out_bytes);

int xbg_texture_parse(const void *source, size_t bytes, xbg_texture *out);
size_t xbg_texture_mip_bytes(const xbg_texture *texture, uint32_t level);
int xbg_texture_decode_mip(const void *source, size_t source_bytes,
    const xbg_texture *texture, uint32_t level, void *out, size_t out_bytes);



int xbg_decode_block(uint32_t nvn_format, const void *block, size_t block_bytes,
                     void *out, size_t out_bytes);



int xbg_decode_bc_linear(uint32_t nvn_format,uint32_t width,uint32_t height,uint32_t depth,
    const void *source,size_t source_bytes,size_t row_stride,size_t image_stride,
    void *out,size_t out_bytes);
#ifdef __cplusplus
}
#endif
