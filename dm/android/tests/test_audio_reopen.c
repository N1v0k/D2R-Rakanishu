


#include "../../host/hle_audio.c"
#include <time.h>

uint64_t now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
static struct { const void *k; void *v; } om[8];
void *objmap_get(const void *k) { for (int i = 0; i < 8; i++) if (om[i].k == k) return om[i].v; return NULL; }
void objmap_put(const void *k, void *v) { for (int i = 0; i < 8; i++) if (!om[i].k) { om[i].k = k; om[i].v = v; return; } }
void *objmap_del(const void *k) { for (int i = 0; i < 8; i++) if (om[i].k == k) { om[i].k = 0; return om[i].v; } return NULL; }
void hle_signal_system_event(void *g) { (void)g; }
void hle_bind_system_event(void *g, int autoclear) { (void)g; (void)autoclear; }


struct AAudioStreamBuilderStruct { int32_t ch; AAudioStream_dataCallback cb; void *user; };
struct AAudioStreamStruct { int32_t ch; AAudioStream_dataCallback cb; void *user; aaudio_stream_state_t state; pthread_t th; _Atomic int run; };
static int g_open_fails;
static _Atomic int g_live, g_started;
static _Atomic int g_opens;
static AAudioStream *g_last;
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **b) { *b = calloc(1, sizeof **b); return AAUDIO_OK; }
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *b, int32_t v) { (void)b; (void)v; }
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *b, int32_t v) { (void)b; (void)v; }
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *b, int32_t v) { b->ch = v; }
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *b, int32_t v) { (void)b; (void)v; }
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder *b, int32_t v) { (void)b; (void)v; }
void AAudioStreamBuilder_setUsage(AAudioStreamBuilder *b, int32_t v) { (void)b; (void)v; }
void AAudioStreamBuilder_setContentType(AAudioStreamBuilder *b, int32_t v) { (void)b; (void)v; }
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *b, AAudioStream_dataCallback cb, void *user) { b->cb = cb; b->user = user; }
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *b, AAudioStream_errorCallback cb, void *user) { (void)b; (void)cb; (void)user; }
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *b, AAudioStream **s) {
    if (g_open_fails) { *s = NULL; return AAUDIO_ERROR_UNAVAILABLE; }
    *s = calloc(1, sizeof **s); (*s)->ch = b->ch; (*s)->cb = b->cb; (*s)->user = b->user; (*s)->state = AAUDIO_STREAM_STATE_OPEN;
    g_live++; g_opens++; g_last = *s; return AAUDIO_OK;
}
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *b) { free(b); return AAUDIO_OK; }
int32_t AAudioStream_getSampleRate(AAudioStream *s) { (void)s; return RATE; }
int32_t AAudioStream_getChannelCount(AAudioStream *s) { return s->ch; }
int32_t AAudioStream_getPerformanceMode(AAudioStream *s) { (void)s; return AAUDIO_PERFORMANCE_MODE_LOW_LATENCY; }
int32_t AAudioStream_getFramesPerBurst(AAudioStream *s) { (void)s; return 240; }
int32_t AAudioStream_getBufferSizeInFrames(AAudioStream *s) { (void)s; return 480; }
int32_t AAudioStream_getBufferCapacityInFrames(AAudioStream *s) { (void)s; return 960; }
aaudio_stream_state_t AAudioStream_getState(AAudioStream *s) { return s->state; }
static void *dev_thread(void *p) {
    AAudioStream *s = p; static int16_t buf[240 * 8];
    while (s->run) { s->cb(s, s->user, buf, 240); usleep(5000); }
    return NULL;
}
aaudio_result_t AAudioStream_requestStart(AAudioStream *s) {
    if (s->state == AAUDIO_STREAM_STATE_DISCONNECTED) return AAUDIO_ERROR_INVALID_STATE;
    if (s->state == AAUDIO_STREAM_STATE_STARTED) return AAUDIO_OK;
    s->state = AAUDIO_STREAM_STATE_STARTED; s->run = 1; g_started++; pthread_create(&s->th, NULL, dev_thread, s); return AAUDIO_OK;
}
aaudio_result_t AAudioStream_requestStop(AAudioStream *s) {
    if (s->state == AAUDIO_STREAM_STATE_STARTED) { s->run = 0; pthread_join(s->th, NULL); g_started--; s->state = AAUDIO_STREAM_STATE_STOPPED; }
    return AAUDIO_OK;
}
aaudio_result_t AAudioStream_close(AAudioStream *s) { AAudioStream_requestStop(s); g_live--; if (g_last == s) g_last = NULL; free(s); return AAUDIO_OK; }
const char *AAudio_convertResultToText(aaudio_result_t r) { return r == AAUDIO_OK ? "AAUDIO_OK" : "AAUDIO_ERROR"; }


static char g_out[64], g_ev[64];
static gbuf g_b[8]; static int16_t g_pcm[8][240 * 2];
static int pump(int ms) {
    int released = 0; uint64_t end = now_ns() + (uint64_t)ms * 1000000ull;
    while (now_ns() < end) {
        gbuf *r = h_au_Released(g_out);
        if (r) { released++; h_au_Append(g_out, r); } else usleep(1000);
    }
    return released;
}
static int fails;
static void check(int ok, const char *what) { printf("%s %s\n", ok ? "PASS" : "FAIL", what); fails += !ok; }

int main(void) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    uint8_t prm[16]; h_au_InitParam(prm);

    dm_audio_suspend(1);
    g_open_fails = 1;
    if (h_au_Open(g_out, g_ev, NULL, prm)) return 1;
    for (int i = 0; i < 8; i++) { h_au_SetBuf(&g_b[i], g_pcm[i], sizeof g_pcm[i], sizeof g_pcm[i]); h_au_Append(g_out, &g_b[i]); }
    h_au_Start(g_out);
    check(g_live == 0 && pump(100) > 5, "open failed while away: no stream, the silent sink releases buffers");
    g_open_fails = 0;
    dm_audio_suspend(0);
    uint64_t c0 = dm_audio_render_calls();
    int rel = pump(100);
    check(g_live == 1 && g_started == 1 && dm_audio_render_calls() > c0 && rel > 5, "foreground opens and starts a stream that plays the game's buffers");


    dm_audio_suspend(1);
    check(g_started == 0 && pump(50) > 3, "away: stream stopped, the silent sink releases buffers");
    AAudioStream *lost = g_last;
    if (!lost) { puts("test_audio_reopen: FAIL (no stream to lose)"); return 1; }
    lost->state = AAUDIO_STREAM_STATE_DISCONNECTED;
    int opens = g_opens;
    dm_audio_suspend(0);
    c0 = dm_audio_render_calls(); rel = pump(100);
    check(g_opens == opens + 1 && g_live == 1 && g_started == 1 && dm_audio_render_calls() > c0 && rel > 5, "foreground replaces the lost stream with a started one");


    dm_audio_suspend(1);
    g_last->state = AAUDIO_STREAM_STATE_DISCONNECTED; g_open_fails = 1;
    dm_audio_suspend(0);
    check(g_live == 0 && pump(100) > 5, "reopen failed: no stream, the silent sink keeps releasing buffers");
    g_open_fails = 0;
    dm_audio_suspend(1); dm_audio_suspend(0);
    c0 = dm_audio_render_calls(); rel = pump(100);
    check(g_live == 1 && g_started == 1 && dm_audio_render_calls() > c0 && rel > 5, "next foreground opens and starts a stream");


    h_au_Stop(g_out);
    check(g_started == 0, "stopped by the game");
    g_last->state = AAUDIO_STREAM_STATE_DISCONNECTED;
    h_au_Start(g_out);
    c0 = dm_audio_render_calls(); rel = pump(100);
    check(g_live == 1 && g_started == 1 && dm_audio_render_calls() > c0 && rel > 5, "game start replaces a lost stream");

    h_au_Close(g_out);
    check(g_live == 0 && g_started == 0, "close releases the stream");
    printf("%s\n", fails ? "test_audio_reopen: FAIL" : "test_audio_reopen: all passed");
    return fails != 0;
}
