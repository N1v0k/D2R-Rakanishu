









#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void dm_gate(void);
void gate_call(uint64_t *in, uint64_t *out);

static int g_kind, g_arg;
enum { K_CNT = 1, K_SVC = 2, K_OBS = 3 };
static const uint64_t MARK = 0x5157000000000000ull;
static uint64_t g_ret_seen;

uint64_t gate_dispatch(uint64_t ret_addr, uint64_t *regs) {
    g_ret_seen = ret_addr;
    uint64_t r = 0;
    if (g_kind == K_CNT) { regs[g_arg] = MARK | (uint64_t)g_arg; r = regs[g_arg]; }
    if (g_kind == K_SVC) { for (int i = 0; i < 8; i++) regs[i] = MARK | 0x100 | (uint64_t)i; r = regs[0]; }


    __asm__ volatile("mov x9, #0xdead\n mov x10, x9\n mov x11, x9\n mov x12, x9\n mov x13, x9\n mov x14, x9\n mov x15, x9\n mov x16, x9\n mov x17, x9\n"
                     "mov x9, #0x90000000\n msr nzcv, x9\n mov x9, #0xdead" ::: "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "cc");
    return r;
}
void dm_thread_returned(void) { abort(); }
void unimpl_c(void) { abort(); }

int main(int argc, char **argv) {
    int n = 0, bad = 0, nc = 0, ns = 0, no = 0;
    for (int f = 1; f < argc; f++) {
        FILE *fp = fopen(argv[f], "r"); if (!fp) { perror(argv[f]); return 2; }
        char kind[16]; unsigned long long ret, orig; int arg;
        while (fscanf(fp, "%llx %15s %d %llx", &ret, kind, &arg, &orig) == 4) {
            g_kind = !strcmp(kind, "svc") ? K_SVC : !strcmp(kind, "obs") ? K_OBS : (!strcmp(kind, "cntpct") || !strcmp(kind, "cntvct")) ? K_CNT : 0;
            if (!g_kind) { printf("FAIL 未知门种类 %s @%llx\n", kind, orig); bad++; continue; }
            g_arg = arg;
            uint64_t in[30], out[32], want[30];
            for (int i = 0; i < 30; i++) in[i] = 0xA000000000000000ull | ((uint64_t)i << 32) | (uint64_t)(n * 31 + i);
            memcpy(want, in, sizeof want);
            if (g_kind == K_CNT) want[arg] = MARK | (uint64_t)arg;
            if (g_kind == K_SVC) for (int i = 0; i < 8; i++) want[i] = MARK | 0x100 | (uint64_t)i;
            gate_call(in, out);
            int ok = 1;
            for (int i = 0; i < 30; i++) {
                if (i == 18) continue;
                if (out[i] != want[i]) {
                    if (ok) printf("FAIL %s 门 @%#llx arg=%d：", kind, orig, arg);
                    printf(" x%d=%#llx(应 %#llx)", i, (unsigned long long)out[i], (unsigned long long)want[i]); ok = 0;
                }
            }
            if (out[31] != 0x60000000ull) { printf("%sNZCV=%#llx(应 0x60000000)", ok ? "FAIL NZCV " : " ", (unsigned long long)out[31]); ok = 0; }
            if (out[30] != g_ret_seen) { printf("%s x30=%#llx(应 %#llx)", ok ? "FAIL x30 " : "", (unsigned long long)out[30], (unsigned long long)g_ret_seen); ok = 0; }
            if (!ok) { printf("\n"); bad++; }
            n++; nc += g_kind == K_CNT; ns += g_kind == K_SVC; no += g_kind == K_OBS;
        }
        fclose(fp);
    }
    printf("GATEFN %s 门=%d（svc %d / 计时 %d / 观测 %d）失败=%d\n", bad ? "FAIL" : "OK", n, ns, nc, no, bad);
    return bad ? 1 : 0;
}
