#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include "xbg_texture.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct xbg_vk_renderer xbg_vk_renderer;
typedef struct xbg_vk_texture xbg_vk_texture;
typedef struct xbg_vk_info {
    VkPhysicalDevice physical;
    VkDevice device;
    VkQueue queue;
    uint32_t queue_family;
    VkPhysicalDeviceFeatures enabled_core;
    VkBool32 enabled_shader_draw_parameters;
    const uint32_t *vs_words; size_t vs_bytes;
    const uint32_t *ps_words; size_t ps_bytes;
    uint64_t fence_timeout_ns;
} xbg_vk_info;
typedef struct xbg_vk_requirements {
    VkPhysicalDeviceFeatures core;
    VkBool32 shader_draw_parameters;
    VkSubgroupFeatureFlags subgroup_operations;
    VkShaderStageFlags subgroup_stages;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceSubgroupProperties subgroup;
} xbg_vk_requirements;


int xbg_vk_query_2d(VkPhysicalDevice physical,const uint32_t *vs,size_t vs_bytes,
    const uint32_t *ps,size_t ps_bytes,xbg_vk_requirements *requirements,char *error,size_t error_bytes);
int xbg_vk_create(const xbg_vk_info *info,xbg_vk_renderer **out,char *error,size_t error_bytes);


int xbg_vk_wait(xbg_vk_renderer *renderer);
int xbg_vk_destroy(xbg_vk_renderer *renderer);

typedef struct xbg_vertex2d {
    float position[2];
    float uv[2];
    float color[4];
} xbg_vertex2d;
typedef struct xbg_draw2d {
    const xbg_vertex2d *vertices;
    uint32_t vertex_count;
    const uint16_t *indices; uint32_t index_count;
    const void *vertex_uniform0; size_t vertex_uniform_bytes;
    const void *fragment_uniform0; size_t fragment_uniform_bytes;
    const void *vertex_support; const void *fragment_support;
    xbg_vk_texture *texture10;
    VkFilter min_filter,mag_filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_u,address_v;
    float min_lod,max_lod,lod_bias;
    VkPipelineColorBlendAttachmentState blend;
    float blend_constants[4];
    VkRect2D scissor;
    VkClearColorValue clear;
    uint32_t width,height;
    VkFormat target_format;
    VkBool32 load_previous;
} xbg_draw2d;
int xbg_vk_upload_texture(xbg_vk_renderer *renderer,const void *original_texture,
    size_t bytes,xbg_vk_texture **out);
int xbg_vk_upload_nvn_texture(xbg_vk_renderer *renderer,const xbg_nvn_texture_desc *description,
    const void *guest_storage,size_t bytes,xbg_vk_texture **out);
int xbg_vk_release_texture(xbg_vk_renderer *renderer,xbg_vk_texture *texture);
int xbg_vk_draw_2d(xbg_vk_renderer *renderer,const xbg_draw2d *draw);



int xbg_vk_replay_texture(xbg_vk_renderer *renderer,xbg_vk_texture *texture);
int xbg_vk_readback(xbg_vk_renderer *renderer,void *rgba,size_t bytes);



VkImage xbg_vk_output_image(xbg_vk_renderer *renderer,uint32_t *width,uint32_t *height);
const char *xbg_vk_error(const xbg_vk_renderer *renderer);





void xbg_2d_projection(float matrix16[16],float width,float height);
void xbg_2d_fragment_defaults(float constants8[8]);
void xbg_support_defaults(void *bytes1392,float width,float height);




int xbg_nvn_viewport(VkViewport *out,float x,float y,float width,float height,
    float minimum_depth,float maximum_depth);




int xbg_2d_sampler_from_nvn(xbg_draw2d *draw,uint32_t min_filter,uint32_t mag_filter,
    uint32_t wrap_s,uint32_t wrap_t,float min_lod,float max_lod,float lod_bias);
int xbg_2d_blend_from_nvn(xbg_draw2d *draw,uint32_t enabled,uint32_t src_rgb,uint32_t dst_rgb,
    uint32_t src_alpha,uint32_t dst_alpha,uint32_t equation_rgb,uint32_t equation_alpha,uint32_t rgba_mask);
typedef struct xbg_nvn_attribute {
    const void *buffer; size_t buffer_bytes;
    uint32_t nvn_format,offset,stride;
} xbg_nvn_attribute;



int xbg_2d_vertices_from_nvn(const xbg_nvn_attribute attributes[3],uint32_t first,uint32_t count,
    xbg_vertex2d *out,size_t out_bytes);
#ifdef __cplusplus
}
#endif
