




#include "vk_loader.h"
#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

static int fails;
static void check(int ok, const char *what) { printf("%s %s\n", ok ? "ok  " : "FAIL", what); if (!ok) fails++; }

static void *draw_many(void *arg) { (void)arg; for (int i = 0; i < 20000; i++) vkCmdDraw(VK_NULL_HANDLE, 3, 1, 0, 0); return NULL; }

int main(int argc, char **argv) {
    if (argc != 3) { fprintf(stderr, "usage: %s <fake1.so> <fake2.so>\n", argv[0]); return 2; }
    void *a = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL), *b = dlopen(argv[2], RTLD_NOW | RTLD_LOCAL);
    if (!a || !b) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }
    atomic_long *draws_a = dlsym(a, "fake_draws"), *draws_b = dlsym(b, "fake_draws");
    VkInstance *last_a = dlsym(a, "fake_last_instance");
    char err[160];
    check(dm_vk_loader_use_handle(NULL, "none", err, sizeof err) != 0, "a NULL handle is refused");
    check(dm_vk_loader_use_handle(a, "fake 1", err, sizeof err) == 0, "loader 1 picked");
    uint32_t v = 0; vkEnumerateInstanceVersion(&v);
    check(v == VK_API_VERSION_1_1 + 1 && *last_a == VK_NULL_HANDLE, "global command resolved with a NULL instance through loader 1");
    VkInstanceCreateInfo ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; VkInstance inst = VK_NULL_HANDLE;
    check(vkCreateInstance(&ci, NULL, &inst) == VK_SUCCESS && inst == (VkInstance)(uintptr_t)0x1000, "vkCreateInstance through loader 1");
    uint32_t n = 0; vkEnumeratePhysicalDevices(inst, &n, NULL);
    check(n == 1 && *last_a == inst, "instance command resolved with the new instance");
    pthread_t t[8];
    for (int i = 0; i < 8; i++) pthread_create(&t[i], NULL, draw_many, NULL);
    for (int i = 0; i < 8; i++) pthread_join(t[i], NULL);
    check(atomic_load(draws_a) == 8 * 20000, "8 threads, first calls at once: every draw reached loader 1");
    vkDestroyInstance(inst, NULL);
    check(dm_vk_loader_use_handle(b, "fake 2", err, sizeof err) == 0 && !strcmp(dm_vk_loader_name(), "fake 2"), "switched to loader 2");
    VkInstance inst2 = VK_NULL_HANDLE; vkCreateInstance(&ci, NULL, &inst2);
    check(inst2 == (VkInstance)(uintptr_t)0x2000, "vkCreateInstance resolved again after the switch");
    vkCmdDraw(VK_NULL_HANDLE, 3, 1, 0, 0);
    check(atomic_load(draws_b) == 1 && atomic_load(draws_a) == 8 * 20000, "cached device command resolved again after the switch");
    PFN_vkCmdDraw draw = (PFN_vkCmdDraw)vkGetInstanceProcAddr(inst2, "vkCmdDraw");
    if (draw) draw(VK_NULL_HANDLE, 3, 1, 0, 0);
    check(draw && atomic_load(draws_b) == 2, "vkGetInstanceProcAddr answered by loader 2");
    pid_t pid = fork();
    if (pid == 0) { int fd = open("/dev/null", 1); if (fd >= 0) dup2(fd, 2); vkCmdDispatch(VK_NULL_HANDLE, 1, 1, 1); _exit(0); }
    int st = 0; waitpid(pid, &st, 0);
    check(WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT, "a function the loader lacks stops with abort (no jump to null)");
    extern const unsigned dm_vk_shim_count;
    check(dm_vk_shim_count > 400, "the jump table covers the whole header");
    printf("%s: %d failure(s)\n", argv[0], fails);
    return fails != 0;
}
