













#define mprotect t_mprotect
#include "../nvn/nvn_dirty.c"
#undef mprotect
int mprotect(void *, size_t, int);
#include <fcntl.h>
#include <errno.h>

module_t g_mod[MAX_MOD];
int g_nmod;
module_t *module_of(const void *p) { (void)p; return NULL; }
void crash_install(void);

static int g_fail;
#define EXPECT(name, want, got) do { \
    long long want_ = (long long)(want), got_ = (long long)(got); \
    printf("%-58s 期望 %-4lld → 真结果 %-4lld %s\n", name, want_, got_, want_ == got_ ? "✓" : "✗"); \
    if (want_ != got_) g_fail++; \
} while (0)

static uint8_t *g_page; static _Atomic int g_armed, g_go, g_b_done;
static void *thread_b(void *arg) { (void)arg; while (!atomic_load(&g_go)) usleep(200); prep_write(g_page, 16); atomic_store(&g_b_done, 1); return NULL; }
int t_mprotect(void *a, size_t n, int prot) {
    if (prot == PROT_READ && atomic_exchange(&g_armed, 0)) { atomic_store(&g_go, 1); usleep(100000); }
    return mprotect(a, n, prot);
}

int main(void) {
    crash_install();
    size_t pg = (size_t)getpagesize();
    g_page = mmap(NULL, 2 * pg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (g_page == MAP_FAILED) { perror("mmap"); return 2; }
    nvn_dirty_add_range(g_page, 2 * pg);
    uint64_t stamp = nvn_dirty_now();
    pthread_t tb; pthread_create(&tb, NULL, thread_b, NULL);
    atomic_store(&g_armed, 1);
    nvn_dirty_protect(g_page, 2 * pg);
    for (int i = 0; i < 2000 && !atomic_load(&g_b_done); i++) usleep(1000);
    EXPECT("乙的写前解保护在 2 秒内返回", 1, atomic_load(&g_b_done));
    if (!atomic_load(&g_b_done)) { printf("t_dirty_race：乙没返回，后面的项不测\n"); return 1; }
    pthread_join(tb, NULL);
    int fd = open("/dev/zero", O_RDONLY); g_page[pg] = 0;
    ssize_t r = read(fd, g_page, 16);
    EXPECT("乙解完保护后往这页读文件：读到的字节数 = 请求的 16", 16, r);
    if (r == 16) { ((volatile uint8_t *)g_page)[1] = 0x5a; EXPECT("直接写这页不卡、读回是写进去的值", 0x5a, g_page[1]); }
    else { printf("  （read 失败 errno=%d；页只读而状态为「无」，直接写会在缺页处理里空转，这一项不执行）\n", errno); g_fail++; }
    EXPECT("这次写被记成脏（对设只读前的戳）", 1, nvn_dirty_since(g_page, 16, stamp));
    close(fd);
    printf("t_dirty_race：%s\n", g_fail ? "不通" : "全通"); return g_fail != 0;
}
