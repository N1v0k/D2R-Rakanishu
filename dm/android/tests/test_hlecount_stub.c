









#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void *hlecount_wrap(const char *kind, const char *name, void *fn);
void hlecount_seal(void);
void hlecount_flush(void);


uint64_t sum10(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e, uint64_t f, uint64_t g, uint64_t h, uint64_t i, uint64_t j) {
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h + 9 * i + 10 * j;
}
double fsum(double a, double b, double c, double d, double e, double f, double g, double h) { return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g + 8 * h; }

__asm__(".text\n.p2align 2\n.globl dm_test_ident\ndm_test_ident:\n    ret\n");
uint64_t dm_test_ident(uint64_t);


static int call_ident(void *fn) {
    uint64_t r, k9, k10, k15, sp0, sp1;
    __asm__ volatile("mov x0, #77\n    mov x9, #0x909\n    mov x10, #0xa0a\n    mov x15, #0xf0f\n    mov %[sp0], sp\n    blr %[fn]\n"
                     "    mov %[r], x0\n    mov %[k9], x9\n    mov %[k10], x10\n    mov %[k15], x15\n    mov %[sp1], sp\n"
                     : [r] "=&r"(r), [k9] "=&r"(k9), [k10] "=&r"(k10), [k15] "=&r"(k15), [sp0] "=&r"(sp0), [sp1] "=&r"(sp1)
                     : [fn] "r"(fn)
                     : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x30", "memory");
    return r != 77 || k9 != 0x909 || k10 != 0xa0a || k15 != 0xf0f || sp0 != sp1;
}

static void *s_sum, *s_fsum, *s_ident;
static long iters = 100000;
static uint64_t bad_total;
static void *worker(void *arg) {
    (void)arg;
    uint64_t bad = 0;
    uint64_t (*fs)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) = s_sum;
    double (*ff)(double, double, double, double, double, double, double, double) = s_fsum;
    for (long i = 0; i < iters; i++) {
        bad += fs(1, 2, 3, 4, 5, 6, 7, 8, 9, (uint64_t)i) != sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, (uint64_t)i);
        bad += ff(1, 2, 3, 4, 5, 6, 7, i * .5) != fsum(1, 2, 3, 4, 5, 6, 7, i * .5);
        bad += call_ident(s_ident);
    }
    __atomic_add_fetch(&bad_total, bad, __ATOMIC_RELAXED);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc > 1) iters = atol(argv[1]);
    char path[128]; snprintf(path, sizeof path, "/data/local/tmp/test_hlecount_stub.%d.tsv", (int)getpid());
    setenv("DM_HLECOUNT", path, 1);
    s_sum = hlecount_wrap("HLE", "sum10", (void *)sum10);
    s_fsum = hlecount_wrap("HLE", "fsum", (void *)fsum);
    s_ident = hlecount_wrap("FORCE", "ident", (void *)dm_test_ident);
    if (!s_sum || s_sum == (void *)sum10 || s_fsum == s_sum || s_ident == s_fsum) { puts("FAIL no distinct stubs"); return 1; }
    if (hlecount_wrap("HLE", "sum10", (void *)sum10) != s_sum) { puts("FAIL same name and implementation should reuse the stub"); return 1; }
    hlecount_seal();
    pthread_t t[8];
    for (int i = 0; i < 8; i++) pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < 8; i++) pthread_join(t[i], NULL);
    hlecount_flush();
    char want[3][64], line[256]; int found = 0;
    snprintf(want[0], sizeof want[0], "HLE\tsum10\t%ld\n", 8 * iters);
    snprintf(want[1], sizeof want[1], "HLE\tfsum\t%ld\n", 8 * iters);
    snprintf(want[2], sizeof want[2], "FORCE\tident\t%ld\n", 8 * iters);
    FILE *f = fopen(path, "r");
    while (f && fgets(line, sizeof line, f)) for (int k = 0; k < 3; k++) found += !strcmp(line, want[k]);
    if (f) fclose(f);
    unlink(path);
    printf("stride %td bytes, wrong calls %llu, exact counts %d/3\n", (char *)s_fsum - (char *)s_sum, (unsigned long long)bad_total, found);
    int failed = bad_total || found != 3;
    puts(failed ? "FAIL" : "PASS hlecount stub: distinct and reused stubs, int/fp/stack arguments, x9-x15/sp preserved, exact counts from 8 threads");
    fflush(stdout);
    _exit(failed);
}
