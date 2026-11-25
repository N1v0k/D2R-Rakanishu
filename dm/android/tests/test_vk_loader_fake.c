

#include <vulkan/vulkan.h>
#include <stdatomic.h>
#include <string.h>

atomic_long fake_draws;
VkInstance fake_last_instance;
int fake_id = FAKE_ID;

static VkResult VKAPI_CALL create_instance(const VkInstanceCreateInfo *ci, const VkAllocationCallbacks *a, VkInstance *out) { (void)ci; (void)a; *out = (VkInstance)(uintptr_t)(0x1000 * FAKE_ID); return VK_SUCCESS; }
static void VKAPI_CALL destroy_instance(VkInstance i, const VkAllocationCallbacks *a) { (void)i; (void)a; }
static VkResult VKAPI_CALL enumerate_version(uint32_t *v) { *v = VK_API_VERSION_1_1 + FAKE_ID; return VK_SUCCESS; }
static VkResult VKAPI_CALL enumerate_devices(VkInstance i, uint32_t *n, VkPhysicalDevice *d) { (void)i; (void)d; *n = FAKE_ID; return VK_SUCCESS; }
static void VKAPI_CALL cmd_draw(VkCommandBuffer c, uint32_t a, uint32_t b, uint32_t d, uint32_t e) { (void)c; (void)a; (void)b; (void)d; (void)e; atomic_fetch_add(&fake_draws, 1); }

PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *name) {
    fake_last_instance = instance;
    if (!strcmp(name, "vkCreateInstance")) return instance ? NULL : (PFN_vkVoidFunction)create_instance;
    if (!strcmp(name, "vkEnumerateInstanceVersion")) return instance ? NULL : (PFN_vkVoidFunction)enumerate_version;
    if (!instance) return NULL;
    if (!strcmp(name, "vkDestroyInstance")) return (PFN_vkVoidFunction)destroy_instance;
    if (!strcmp(name, "vkEnumeratePhysicalDevices")) return (PFN_vkVoidFunction)enumerate_devices;
    if (!strcmp(name, "vkCmdDraw")) return (PFN_vkVoidFunction)cmd_draw;
    if (!strcmp(name, "vkGetInstanceProcAddr")) return (PFN_vkVoidFunction)vkGetInstanceProcAddr;
    return NULL;
}
