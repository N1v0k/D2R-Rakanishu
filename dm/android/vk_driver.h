
#pragma once
#include <stddef.h>
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif
#define DM_VK_DRIVER_OK_FRAMES 300

void dm_vk_driver_config(const char *dir, const char *lib, const char *hooks, const char *tu_debug, const char *attempt, const char *state_dir, const char *label, int test_fail);
int dm_vk_driver_tu_debug_ok(const char *value);
int dm_vk_driver_open(void);
const char *dm_vk_driver_verify(VkPhysicalDevice physical, uint32_t devices);
int dm_vk_driver_fall_back(const char *reason);
void dm_vk_driver_identity(VkPhysicalDevice physical);
void dm_vk_driver_frame(void);
void dm_vk_driver_state(char *out, size_t bytes);
int dm_vk_driver_label(char *out, size_t bytes);
#ifdef __cplusplus
}
#endif
