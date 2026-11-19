




#include "vk_loader.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static void *g_lib;
static PFN_vkGetInstanceProcAddr g_gipa;
static VkInstance g_instance;
static unsigned g_gen = 1;
static char g_name[48] = "none";

static int use(void *handle, const char *what, char *error, size_t bytes) {
    PFN_vkGetInstanceProcAddr gipa = handle ? (PFN_vkGetInstanceProcAddr)dlsym(handle, "vkGetInstanceProcAddr") : NULL;
    if (!gipa) { if (error && bytes) snprintf(error, bytes, "%s: no vkGetInstanceProcAddr (%s)", what, handle ? "dlsym" : dlerror()); return -1; }
    pthread_mutex_lock(&g_mu);
    g_lib = handle; g_gipa = gipa; g_instance = VK_NULL_HANDLE; snprintf(g_name, sizeof g_name, "%s", what);
    __atomic_add_fetch(&g_gen, 1, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&g_mu);
    fprintf(stderr, "vk loader: %s\n", what);
    return 0;
}
int dm_vk_loader_use_system(char *error, size_t bytes) { return use(dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL), "system", error, bytes); }
int dm_vk_loader_use_handle(void *handle, const char *what, char *error, size_t bytes) { return use(handle, what, error, bytes); }
const char *dm_vk_loader_name(void) { return g_name; }

static PFN_vkGetInstanceProcAddr loader(void) {
    PFN_vkGetInstanceProcAddr gipa = __atomic_load_n(&g_gipa, __ATOMIC_ACQUIRE);
    if (gipa) return gipa;
    char error[160];
    if (dm_vk_loader_use_system(error, sizeof error)) { fprintf(stderr, "vk loader: %s\n", error); abort(); }
    return g_gipa;
}

PFN_vkVoidFunction dm_vk_loader_gipa(VkInstance instance, const char *name) { return loader()(instance, name); }

void dm_vk_loader_instance(VkInstance instance) {
    pthread_mutex_lock(&g_mu);
    g_instance = instance;
    __atomic_add_fetch(&g_gen, 1, __ATOMIC_RELEASE);
    pthread_mutex_unlock(&g_mu);
}

void *dm_vk_resolve(dm_vk_slot *s) {
    unsigned gen = __atomic_load_n(&g_gen, __ATOMIC_ACQUIRE);
    if (__atomic_load_n(&s->gen, __ATOMIC_ACQUIRE) == gen) return __atomic_load_n(&s->fn, __ATOMIC_RELAXED);
    PFN_vkGetInstanceProcAddr gipa = loader();
    gen = __atomic_load_n(&g_gen, __ATOMIC_ACQUIRE);
    VkInstance instance = s->global ? VK_NULL_HANDLE : __atomic_load_n(&g_instance, __ATOMIC_ACQUIRE);
    void *fn = (void *)gipa(instance, s->name);
    if (!fn) {


        fprintf(stderr, "vk loader: %s has no %s (instance %p)\n", g_name, s->name, (void *)instance);
        abort();
    }
    __atomic_store_n(&s->fn, fn, __ATOMIC_RELAXED);
    __atomic_store_n(&s->gen, gen, __ATOMIC_RELEASE);
    return fn;
}
