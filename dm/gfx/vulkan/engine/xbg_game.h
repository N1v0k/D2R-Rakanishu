#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif




typedef struct xbg_game_config {
    uint32_t size;
    VkPhysicalDevice physical;
    VkDevice device;
    VkQueue queue;
    uint32_t queue_family;
    VkPhysicalDeviceFeatures enabled_core;
    VkBool32 enabled_shader_draw_parameters;
    uint64_t fence_timeout_ns;
    const char *shader_directory;
    void *userdata;

    uint64_t (*invoke_guest)(void *userdata,void *function,const uint64_t *args,uint32_t count);




    int (*present)(void *userdata,void *native_window,VkImage image,VkFormat format,
        uint32_t width,uint32_t height,const int32_t crop[4],int32_t interval);
    void (*log)(void *userdata,const char *message);


    void (*fatal)(void *userdata,const char *message);
} xbg_game_config;

int xbg_game_initialize(const xbg_game_config *config,char *error,size_t error_bytes);




int xbg_game_set_sampler_mirror_clamp_to_edge(VkBool32 enabled);


void xbg_game_event_read(const void *addr);


void xbg_wtrack_use_host_hooks(void);
int xbg_wtrack_fault(void *addr);
void xbg_wtrack_prepare(void *addr,size_t len);








int xbg_game_provenance(const char *path);
int xbg_game_capture_frame(const char *directory,uint64_t frame);








int xbg_game_profile_cpu(uint64_t first_frame,uint32_t frame_count);




int xbg_game_profile_cpu_live(uint32_t frame_count);
int xbg_game_shutdown(void);
void *xbg_nvn_get_proc_address(const char *name);
const char *xbg_game_last_error(void);
typedef struct xbg_game_stats { uint64_t submissions,draws,dispatches,presents,shader_loads; } xbg_game_stats;
void xbg_game_get_stats(xbg_game_stats *out);






enum { XBG_SOURCE_SUPPORT=1,XBG_SOURCE_UNIFORM=2,XBG_SOURCE_STORAGE=3,
    XBG_SOURCE_C1=4,XBG_SOURCE_DRIVER0=5,XBG_SOURCE_DRIVER2=6,
    XBG_SOURCE_TEXTURE=7,XBG_SOURCE_IMAGE=8 };
typedef struct xbg_shader_header {
    uint32_t magic,version,stage,binding_count,input_count,c1_bytes,spirv_bytes,reserved;
} xbg_shader_header;
typedef struct xbg_shader_binding {
    uint32_t binding,type,count,bytes,source,slot,sampler_slot,flags;
} xbg_shader_binding;

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C"
#endif
int xbg_game_profile_descriptor_live(uint32_t frame_count);



#ifdef __cplusplus
extern "C"
#endif
int xbg_game_profile_draw_live(uint32_t frame_count);
