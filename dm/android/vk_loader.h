




#pragma once
#include <stddef.h>
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif



typedef struct { const char *name; int global; void *fn; unsigned gen; } dm_vk_slot;
void *dm_vk_resolve(dm_vk_slot *slot);
PFN_vkVoidFunction dm_vk_loader_gipa(VkInstance instance, const char *name);
void dm_vk_loader_instance(VkInstance instance);


int dm_vk_loader_use_system(char *error, size_t bytes);
int dm_vk_loader_use_handle(void *handle, const char *what, char *error, size_t bytes);
const char *dm_vk_loader_name(void);

#ifdef __cplusplus
}
#endif
