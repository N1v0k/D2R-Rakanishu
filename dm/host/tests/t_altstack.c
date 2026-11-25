

#include <pthread.h>
#include <stdio.h>
#include <malloc/malloc.h>
void crash_thread_init(void);
static void *body(void *a) { crash_thread_init(); crash_thread_init(); return a; }
static size_t inuse(void) { malloc_statistics_t st; malloc_zone_statistics(NULL, &st); return st.size_in_use; }
int main(void) {
    size_t before = inuse();
    for (int i = 0; i < 2000; i++) { pthread_t t; pthread_create(&t, 0, body, 0); pthread_join(t, 0); }
    double grew = (inuse() - (double)before) / 1048576.0;
    printf("2000 个短命线程后堆增长 %.2f MiB（期望 < 1；修前实测 250）%s\n", grew, grew < 1 ? "✓" : "✗");
    return !(grew < 1);
}
