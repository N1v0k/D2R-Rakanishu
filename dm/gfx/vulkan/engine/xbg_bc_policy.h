

#pragma once
#include "xbg_blit.h"
#include "../../nvn/nvn_copy_pitch.h"
#include <string.h>

static inline VkFormat xbg_bc_native_format(uint32_t f){
    static const VkFormat formats[16]={VK_FORMAT_BC1_RGB_UNORM_BLOCK,VK_FORMAT_BC1_RGBA_UNORM_BLOCK,VK_FORMAT_BC2_UNORM_BLOCK,VK_FORMAT_BC3_UNORM_BLOCK,
        VK_FORMAT_BC1_RGB_SRGB_BLOCK,VK_FORMAT_BC1_RGBA_SRGB_BLOCK,VK_FORMAT_BC2_SRGB_BLOCK,VK_FORMAT_BC3_SRGB_BLOCK,VK_FORMAT_BC4_UNORM_BLOCK,VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,VK_FORMAT_BC5_SNORM_BLOCK,VK_FORMAT_BC7_UNORM_BLOCK,VK_FORMAT_BC7_SRGB_BLOCK,VK_FORMAT_BC6H_SFLOAT_BLOCK,VK_FORMAT_BC6H_UFLOAT_BLOCK};
    return f>=0x41 && f<=0x50?formats[f-0x41]:VK_FORMAT_UNDEFINED;
}



static inline bool xbg_native_bc_option(bool marker_present,int marker,const char *environment){
    return marker_present?marker=='1':environment && environment[0]=='1' && environment[1]=='\0';
}



static inline bool xbg_bc_astc_destination(uint32_t format){
    return (format>=0x41 && format<=0x48) || format==0x4d || format==0x4e;
}
static inline bool xbg_native_bc_eligible(uint32_t format,bool requested,bool enabled_bc,
        VkFormatFeatureFlags features,uint32_t samples,int target,uint32_t flags){
    const VkFormatFeatureFlags required=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT|VK_FORMAT_FEATURE_TRANSFER_SRC_BIT|VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    return requested && enabled_bc && xbg_bc_native_format(format)!=VK_FORMAT_UNDEFINED && !xbg_bc_astc_destination(format) &&
        samples==1 && (target==1 || target==4 || target==8 || target==9) && !(flags&(0x4u|0x10u)) && (features&required)==required;
}
static inline bool xbg_native_bc_extent_fits(const VkImageFormatProperties &p,uint32_t w,uint32_t h,uint32_t levels,uint32_t layers){
    return w && h && levels && layers && w<=p.maxExtent.width && h<=p.maxExtent.height && p.maxExtent.depth>=1 &&
        levels<=p.maxMipLevels && layers<=p.maxArrayLayers && (p.sampleCounts&VK_SAMPLE_COUNT_1_BIT);
}



static inline int xbg_astc_bc_plan(VkFormat source,VkFormat destination,const XbgBlitRegion &s,const XbgBlitRegion &d,
        uint32_t flags,VkFormatFeatureFlags sf,VkFormatFeatureFlags df,VkImageBlit *out,VkFilter *filter){
    if(!((source==VK_FORMAT_ASTC_4x4_UNORM_BLOCK && destination==VK_FORMAT_R8G8B8A8_UNORM) ||
         (source==VK_FORMAT_ASTC_4x4_SRGB_BLOCK && destination==VK_FORMAT_R8G8B8A8_SRGB)) ||
         (flags&~3u) || s.w!=d.w || s.h!=d.h || s.count!=d.count)return -1;
    return xbg_blit_plan(s,d,0,0,sf,df,out,filter);
}

struct XbgBcCopyRows { size_t row,image,tight,rows,slices,guest_bytes,packed_bytes; };
static inline int xbg_bc_copy_rows(int w,int h,int slices,int block_bytes,int64_t row_stride,int64_t image_stride,XbgBcCopyRows *out){
    if(!out || slices<=0 || (block_bytes!=8 && block_bytes!=16))return -1;
    size_t row,image,extent;
    if(!nvn_copy_pitch(w,h,slices,4,4,block_bytes,row_stride,image_stride,&row,&image,&extent) || row%block_bytes || extent>256ull*1024*1024)return -1;
    size_t tight=((size_t(w)+3)/4)*block_bytes,rows=(size_t(h)+3)/4;
    *out={row,image,tight,rows,size_t(slices),extent,tight*rows*size_t(slices)};return 0;
}


static inline int xbg_bc_copy_transfer(void *destination,size_t destination_bytes,const void *source,size_t source_bytes,
        const XbgBcCopyRows &p,bool to_guest){
    if(!destination || !source || !p.rows || !p.slices || !p.tight || p.row<p.tight || p.image>256ull*1024*1024 ||
       p.rows>p.image/p.row || p.slices>256ull*1024*1024/p.image || p.guest_bytes!=p.image*p.slices ||
       p.packed_bytes!=p.tight*p.rows*p.slices || destination_bytes<(to_guest?p.guest_bytes:p.packed_bytes) ||
       source_bytes<(to_guest?p.packed_bytes:p.guest_bytes))return -1;
    for(size_t z=0;z<p.slices;z++)for(size_t y=0;y<p.rows;y++){
        size_t guest=z*p.image+y*p.row,packed=(z*p.rows+y)*p.tight;
        memcpy(static_cast<unsigned char*>(destination)+(to_guest?guest:packed),static_cast<const unsigned char*>(source)+(to_guest?packed:guest),p.tight);
    }
    return 0;
}
