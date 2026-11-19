



#include "../hle_audio.c"
#include <time.h>
uint64_t now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }
static struct { const void *k; void *v; } om[8];
void *objmap_get(const void *k) { for (int i = 0; i < 8; i++) if (om[i].k == k) return om[i].v; return NULL; }
void objmap_put(const void *k, void *v) { for (int i = 0; i < 8; i++) if (!om[i].k) { om[i].k = k; om[i].v = v; return; } }
void *objmap_del(const void *k) { for (int i = 0; i < 8; i++) if (om[i].k == k) { om[i].k = 0; return om[i].v; } return NULL; }
static _Atomic int g_sig;
void hle_signal_system_event(void *g) { (void)g; atomic_fetch_add_explicit(&g_sig, 1, memory_order_seq_cst); }
void hle_bind_system_event(void *g, int autoclear) { (void)g; (void)autoclear; }
int main(void) {
    {
        static const struct { int16_t in[6]; int16_t l, r; const char *what; } c[] = {
            { { 0, 0, 10000, 0, 0, 0 }, 5000, 5000, "只有中置（对白）→ 左右各 5000" },
            { { 0, 0, 0, 0, 10000, 0 }, 5000, 0, "只有左后 → 左 5000、右 0" },
            { { 10000, 0, 0, 0, 0, 0 }, 7070, 0, "只有左前 → 左 7070（−3 dB）" },
            { { 0, 0, 0, 30000, 0, 0 }, 0, 0, "只有低音 → 不并入，0" },
            { { 32767, 32767, 32767, 0, 32767, 32767 }, 32767, 32767, "满幅叠加 → 饱和到 32767" },
            { { -32768, -32768, -32768, 0, -32768, -32768 }, -32768, -32768, "负满幅叠加 → 饱和到 −32768" } };
        int bad = 0;
        for (unsigned i = 0; i < sizeof c / sizeof *c; i++) { int16_t o[2]; downmix51(c[i].in, o, 1); int ok = o[0] == c[i].l && o[1] == c[i].r; bad |= !ok;
            printf("下混：%s：期望 (%d,%d) → 真结果 (%d,%d) %s\n", c[i].what, c[i].l, c[i].r, o[0], o[1], ok ? "✓" : "✗"); }
        if (bad) return 1; }
    char out[64], ev[64]; uint8_t prm[16]; gbuf b[3]; static int16_t pcm[3][4800 * 2];
    h_au_InitParam(prm);
    if (h_au_Open(out, ev, NULL, prm)) return 1;
    if (h_au_Ch(out) != 2 || h_au_Rate(out) != 48000 || h_au_State(out) != STATE_STOPPED) { puts("FAIL 参数"); return 1; }
    for (int i = 0; i < 3; i++) { h_au_SetBuf(&b[i], pcm[i], sizeof pcm[i], sizeof pcm[i]); if (!h_au_Append(out, &b[i])) return 1; }
    if (h_au_Released(out)) { puts("FAIL 未开始就有释放"); return 1; }
    uint64_t t0 = now_ns(); h_au_Start(out);
    int got = 0, fail = 0;
    while (got < 3 && now_ns() - t0 < 10000000000ull) {
        gbuf *r = h_au_Released(out);
        if (!r) { usleep(1000); continue; }
        int ok = r == &b[got];
        printf("释放 #%d：期望 块%d 按序 → 真结果 块%ld %s\n", got, got, (long)(r - b), ok ? "✓" : "✗");
        fail |= !ok; got++;
    }
    h_au_Close(out);
    int sig1 = atomic_load_explicit(&g_sig, memory_order_acquire);
    printf("释放 %d/3，事件触发 %d 次（期望 3）\n", got, sig1);
    if (fail || got != 3 || sig1 != 3) return 1;





    setenv("DM_AUDIO", "1", 1);
    char out2[64]; gbuf b2[6]; static int16_t pcm2[6][4800 * 2]; g_sig = 0;
    if (h_au_Open(out2, ev, NULL, prm)) return 1;
    for (int i = 0; i < 6; i++) { h_au_SetBuf(&b2[i], pcm2[i], sizeof pcm2[i], sizeof pcm2[i]); h_au_Append(out2, &b2[i]); }
    t0 = now_ns(); h_au_Start(out2);
    int got2 = 0, order_ok = 1, susp = 0, resumed = 0;
    while (got2 < 6 && now_ns() - t0 < 10000000000ull) {
        uint64_t ms = (now_ns() - t0) / 1000000;
        if (!susp && ms >= 150) { dm_audio_suspend(1); susp = 1; }
        if (susp && !resumed && ms >= 350) { dm_audio_suspend(0); resumed = 1; }
        gbuf *r = h_au_Released(out2);
        if (!r) { usleep(1000); continue; }
        order_ok &= r == &b2[got2]; got2++;
    }
    double total = (now_ns() - t0) / 1e6;
    h_au_Close(out2);
    int sig2 = atomic_load_explicit(&g_sig, memory_order_acquire);
    int ok2 = got2 == 6 && order_ok && sig2 == 6 && total > 520;
    printf("暂停 / 恢复：释放 %d/6 按序=%d 事件 %d（期望 6）总时长下限 520 ms，真结果 %.0f ms %s\n", got2, order_ok, sig2, total, ok2 ? "✓" : "✗");
    if (!ok2) return 1;








    alarm(30);
    setenv("DM_AUDIO", "0", 1);
    char out3[64], out4[64]; static gbuf b3[10], b4[10]; static int16_t pcm3[10][4800 * 2];
    if (h_au_Open(out3, ev, NULL, prm) || h_au_Open(out4, ev, NULL, prm)) return 1;
    for (int i = 0; i < 10; i++) { h_au_SetBuf(&b3[i], pcm3[i], sizeof pcm3[i], sizeof pcm3[i]); h_au_Append(out3, &b3[i]);
                                   h_au_SetBuf(&b4[i], pcm3[i], sizeof pcm3[i], sizeof pcm3[i]); h_au_Append(out4, &b4[i]); }
    int rel3 = 0, rel4 = 0;
#define DRAIN34() do { while (h_au_Released(out3)) rel3++; while (h_au_Released(out4)) rel4++; } while (0)
#define WAIT34(ms) do { uint64_t e_ = now_ns() + (uint64_t)(ms) * 1000000ull; while (now_ns() < e_) { DRAIN34(); usleep(2000); } DRAIN34(); } while (0)
    h_au_Start(out3); h_au_Start(out4);
    WAIT34(250);
    dm_audio_idle(1);
    WAIT34(30);
    int p3 = rel3, p4 = rel4;
    WAIT34(600);
    int ok3 = rel3 == p3 && rel4 == p4 && p3 >= 1 && p4 >= 1;
    printf("后台安静：停下前释放 %d / %d 块，停着 600 ms 后 %d / %d（期望不变）%s\n", p3, p4, rel3, rel4, ok3 ? "✓" : "✗");
    dm_audio_idle(0);
    uint64_t u0 = now_ns(); int q3 = rel3;
    while (rel3 < q3 + 2 && now_ns() - u0 < 3000000000ull) { DRAIN34(); usleep(1000); }
    double two = (now_ns() - u0) / 1e6;
    int ok4 = rel3 >= q3 + 2 && two >= 90;
    printf("放开后节拍从头算：再释放两块用时 %.0f ms（期望 ≥ 90，不连放）%s\n", two, ok4 ? "✓" : "✗");
    dm_audio_idle(1); WAIT34(30);
    uint64_t s0 = now_ns(); h_au_Stop(out3); double stop_ms = (now_ns() - s0) / 1e6;
    s0 = now_ns(); h_au_Close(out3); h_au_Close(out4); double close_ms = (now_ns() - s0) / 1e6;
    int ok5 = stop_ms < 100 && close_ms < 100;
    printf("停着时 Stop %.1f ms、两路 Close %.1f ms（期望都 < 100）%s\n", stop_ms, close_ms, ok5 ? "✓" : "✗");
    dm_audio_idle(0);
    setenv("DM_AUDIO", "1", 1);
    char out5[64]; static gbuf b5[30];
    if (h_au_Open(out5, ev, NULL, prm)) return 1;
    for (int i = 0; i < 30; i++) { h_au_SetBuf(&b5[i], pcm3[i % 10], sizeof pcm3[0], sizeof pcm3[0]); h_au_Append(out5, &b5[i]); }
    h_au_Start(out5); usleep(150000);
    dm_audio_suspend(1); dm_audio_idle(1); usleep(100000);
    s0 = now_ns(); dm_audio_idle(0); dm_audio_suspend(0); double back_ms = (now_ns() - s0) / 1e6;
    int rel5 = 0; uint64_t r0 = now_ns(); while (rel5 < 2 && now_ns() - r0 < 2000000000ull) { while (h_au_Released(out5)) rel5++; usleep(1000); }
    h_au_Close(out5);
    int ok6 = back_ms < 100 && rel5 >= 2;
    printf("后台 + 停下后回前台：%.1f ms 返回，之后照常释放 %d 块（期望 < 100 ms、≥ 2）%s\n", back_ms, rel5, ok6 ? "✓" : "✗");
    alarm(0);
    return !(ok3 && ok4 && ok5 && ok6);
}
