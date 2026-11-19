











#include "hle.h"
#include <pthread/qos.h>
#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#include <TargetConditionals.h>
#include <dispatch/dispatch.h>
typedef AudioUnit dm_audev;
typedef dispatch_semaphore_t dm_sem;
static dm_sem sem_new(void) { return dispatch_semaphore_create(0); }
static void sem_up(dm_sem s) { dispatch_semaphore_signal(s); }
static void sem_down(dm_sem s) { dispatch_semaphore_wait(s, DISPATCH_TIME_FOREVER); }
static void sem_free(dm_sem s) { dispatch_release(s); }
#elif defined(__OHOS__)
#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>
#include <semaphore.h>
typedef OH_AudioRenderer *dm_audev;
typedef sem_t *dm_sem;
static dm_sem sem_new(void) { dm_sem s = malloc(sizeof *s); if (!s || sem_init(s, 0, 0)) die("音频信号量"); return s; }
static void sem_up(dm_sem s) { sem_post(s); }
static void sem_down(dm_sem s) { while (sem_wait(s)) {} }
static void sem_free(dm_sem s) { sem_destroy(s); free(s); }
#else
#include <aaudio/AAudio.h>
#include <semaphore.h>
typedef AAudioStream *dm_audev;
typedef sem_t *dm_sem;
static dm_sem sem_new(void) { dm_sem s = malloc(sizeof *s); if (!s || sem_init(s, 0, 0)) die("音频信号量"); return s; }
static void sem_up(dm_sem s) { sem_post(s); }
static void sem_down(dm_sem s) { while (sem_wait(s)) {} }
static void sem_free(dm_sem s) { sem_destroy(s); free(s); }
#endif
#include <stdatomic.h>
#include <unistd.h>

enum { FMT_PCM16 = 2, STATE_STARTED = 0, STATE_STOPPED = 1, RATE = 48000 };

typedef struct { void *ptr; uint64_t cap, size; } gbuf;
#define QMAX 64
#define RELMAX QMAX
typedef struct {
    pthread_mutex_t m;
    void *event;
    int ch;
    _Atomic int state;
    gbuf *q[QMAX]; _Atomic uint32_t q_head, q_tail;
    gbuf *rel[RELMAX]; _Atomic uint32_t rel_head, rel_tail;
    uint64_t pos;
    dm_audev au;
    int mix;
    int16_t *mixbuf;
    pthread_t null_th; _Atomic int null_run;
    dm_sem notify_sem;
    pthread_t notify_th; _Atomic int notify_run, notify_pending, notify_armed;
    _Atomic uint64_t notify_ns;
    int dry;
} aout;



#define AOMAX 8
static aout *g_aos[AOMAX];
static pthread_mutex_t g_aos_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_suspended;
static aout *ao(const void *g) { aout *a = objmap_get(g); if (!a) die("AudioOut %p 不是宿主建的", g); return a; }









static int g_nq_mode = -1, g_nq_alt; static qos_class_t g_nq_base = QOS_CLASS_UNSPECIFIED;
static _Atomic uint64_t g_nq_dry[2], g_nq_calls[2], g_nq_wake[2], g_nq_lat_sum[2], g_nq_lat_max[2], g_nq_lat_2ms[2];
static int nq_phase(void) { if (g_nq_mode < 0) { const char *e = getenv("DM_AUDIO_NQOS"); g_nq_mode = e && !strcmp(e, "0") ? 0 : e && !strncmp(e, "alt:", 4) ? 2 : 1; if (g_nq_mode == 2) { g_nq_alt = atoi(e + 4); if (g_nq_alt < 1) g_nq_alt = 30; } }
    return g_nq_mode < 2 ? g_nq_mode : (int)((now_ns() / 1000000000ull / (uint64_t)g_nq_alt) & 1); }
static void nq_report(int ph) {
    uint64_t c = atomic_exchange(&g_nq_calls[ph], 0), d = atomic_exchange(&g_nq_dry[ph], 0), w = atomic_exchange(&g_nq_wake[ph], 0), ls = atomic_exchange(&g_nq_lat_sum[ph], 0), lm = atomic_exchange(&g_nq_lat_max[ph], 0), l2 = atomic_exchange(&g_nq_lat_2ms[ph], 0);
    if (!c) return;
    if (ph) LOG("音频通知线程 QoS 对照：相 交互级：声卡回调 %llu 次、欠载（断音）%llu 次；通知 %llu 次，叫醒延迟 平均 %.3f ms、最大 %.3f ms、超过 2 ms %llu 次", (unsigned long long)c, (unsigned long long)d, (unsigned long long)w, w ? ls / 1e6 / (double)w : 0.0, lm / 1e6, (unsigned long long)l2);
    else LOG("音频通知线程 QoS 对照：相 现状：声卡回调 %llu 次、欠载（断音）%llu 次；通知 %llu 次，叫醒延迟 平均 %.3f ms、最大 %.3f ms、超过 2 ms %llu 次", (unsigned long long)c, (unsigned long long)d, (unsigned long long)w, w ? ls / 1e6 / (double)w : 0.0, lm / 1e6, (unsigned long long)l2);
}
static void notify_release(aout *a) {
    atomic_store_explicit(&a->notify_ns, now_ns(), memory_order_relaxed);
    atomic_store_explicit(&a->notify_pending, 1, memory_order_release);
    if (atomic_exchange_explicit(&a->notify_armed, 1, memory_order_acq_rel) == 0)
        sem_up(a->notify_sem);
}

static void *event_notify(void *p) {
    aout *a = p; int applied = -1;
    if (g_nq_base == QOS_CLASS_UNSPECIFIED) g_nq_base = qos_class_self();
#ifdef __OHOS__
    { void front_gtx_register(int); front_gtx_register(2); }
#endif
    for (;;) {
        int ph = nq_phase();
        if (ph != applied) {
            if (applied >= 0) nq_report(applied);
            int e = pthread_set_qos_class_self_np(ph ? QOS_CLASS_USER_INTERACTIVE : (g_nq_base == QOS_CLASS_UNSPECIFIED ? QOS_CLASS_DEFAULT : g_nq_base), 0);
            if (e) LOG("【红】音频通知线程：设 QoS 失败（%d）", e);
            else if (g_nq_mode && ph) LOG("音频通知线程：QoS → 交互级"); else if (g_nq_mode) LOG("音频通知线程：QoS → 现状");
            applied = ph; }
        sem_down(a->notify_sem);
        { uint64_t t0 = atomic_load_explicit(&a->notify_ns, memory_order_relaxed), dt = t0 ? now_ns() - t0 : 0; int q = applied < 0 ? 0 : applied;
          atomic_fetch_add(&g_nq_wake[q], 1); atomic_fetch_add(&g_nq_lat_sum[q], dt); if (dt > 2000000) atomic_fetch_add(&g_nq_lat_2ms[q], 1);
          uint64_t mx = atomic_load(&g_nq_lat_max[q]); while (dt > mx && !atomic_compare_exchange_weak(&g_nq_lat_max[q], &mx, dt)) {} }
        if (atomic_exchange_explicit(&a->notify_pending, 0, memory_order_acq_rel) && a->event)
            hle_signal_system_event(a->event);
        atomic_store_explicit(&a->notify_armed, 0, memory_order_release);
        if (!atomic_load_explicit(&a->notify_run, memory_order_acquire)) break;
        if (atomic_load_explicit(&a->notify_pending, memory_order_acquire) &&
            atomic_exchange_explicit(&a->notify_armed, 1, memory_order_acq_rel) == 0)
            sem_up(a->notify_sem);
    }
    return NULL;
}



static void pull(aout *a, uint8_t *dst, uint64_t n) {
    int released = 0;
    uint32_t qh = atomic_load_explicit(&a->q_head, memory_order_relaxed);
    while (n) {
        uint32_t qt = atomic_load_explicit(&a->q_tail, memory_order_acquire);
        if (qh == qt || atomic_load_explicit(&a->state, memory_order_acquire) != STATE_STARTED) { if (qh == qt && atomic_load_explicit(&a->state, memory_order_acquire) == STATE_STARTED) a->dry = 1; memset(dst, 0, n); break; }
        gbuf *b = a->q[qh & (QMAX - 1)];
        uint64_t k = b->size - a->pos; if (k > n) k = n;
        memcpy(dst, (uint8_t *)b->ptr + a->pos, k);
        dst += k; n -= k; a->pos += k;
        if (a->pos >= b->size) {
            uint32_t rw = atomic_load_explicit(&a->rel_tail, memory_order_relaxed);
            uint32_t rr = atomic_load_explicit(&a->rel_head, memory_order_acquire);
            if (rw - rr == RELMAX) die("AudioOut 已释放队列溢出");
            a->rel[rw & (RELMAX - 1)] = b;
            atomic_store_explicit(&a->rel_tail, rw + 1, memory_order_release);
            qh++;
            atomic_store_explicit(&a->q_head, qh, memory_order_release);
            a->pos = 0; released = 1;
        }
    }
    if (released) notify_release(a);
}

static _Atomic uint64_t g_render_calls;
uint64_t dm_audio_render_calls(void) { return atomic_load_explicit(&g_render_calls, memory_order_relaxed); }








#define MIXMAX 4096
static void downmix51(const int16_t *in, int16_t *out, unsigned frames) {
    for (unsigned i = 0; i < frames; i++, in += 6, out += 2) {
        int32_t l = (23170 * in[0] + 16384 * in[2] + 16384 * in[4]) >> 15, r = (23170 * in[1] + 16384 * in[2] + 16384 * in[5]) >> 15;
        out[0] = (int16_t)(l > 32767 ? 32767 : l < -32768 ? -32768 : l); out[1] = (int16_t)(r > 32767 ? 32767 : r < -32768 ? -32768 : r); }
}


static void render(aout *a, void *data, uint32_t bytes) {
    atomic_fetch_add_explicit(&g_render_calls, 1, memory_order_relaxed);
    a->dry = 0;
    if (!a->mix) pull(a, data, bytes);
    else { int16_t *o = data; unsigned left = bytes / 4;
        while (left) { unsigned k = left < MIXMAX ? left : MIXMAX; pull(a, (uint8_t *)a->mixbuf, (uint64_t)k * 12); downmix51(a->mixbuf, o, k); o += 2 * k; left -= k; } }


    { static FILE *df; static int on = -1; if (on < 0) { const char *e = getenv("DM_AUDIO_OUT_DUMP"); on = e != NULL; if (on && !(df = fopen(e, "wb"))) LOG("【红】DM_AUDIO_OUT_DUMP 打不开 %s", e); }
      if (df) fwrite(data, 1, bytes, df); }
    { int q = nq_phase(); atomic_fetch_add_explicit(&g_nq_calls[q], 1, memory_order_relaxed); if (a->dry) atomic_fetch_add_explicit(&g_nq_dry[q], 1, memory_order_relaxed); }
}
#ifdef __APPLE__
static OSStatus au_render(void *ctx, AudioUnitRenderActionFlags *f, const AudioTimeStamp *ts, UInt32 bus, UInt32 frames, AudioBufferList *bl) {
    (void)f; (void)ts; (void)bus; (void)frames;
    render(ctx, bl->mBuffers[0].mData, bl->mBuffers[0].mDataByteSize);
    return noErr;
}
#if !TARGET_OS_IPHONE
static void dev_name(AudioObjectID id, char *o, size_t n) {
    CFStringRef nm = NULL; UInt32 sz = sizeof nm; AudioObjectPropertyAddress pa = { kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
    o[0] = 0; if (!AudioObjectGetPropertyData(id, &pa, 0, NULL, &sz, &nm) && nm) { CFStringGetCString(nm, o, (CFIndex)n, kCFStringEncodingUTF8); CFRelease(nm); } }
#endif
static int au_open(aout *a) {
    AudioComponentDescription d = {.componentType = kAudioUnitType_Output,
#if TARGET_OS_IPHONE
                                   .componentSubType = kAudioUnitSubType_RemoteIO,
#else
                                   .componentSubType = kAudioUnitSubType_DefaultOutput,
#endif
                                   .componentManufacturer = kAudioUnitManufacturer_Apple};
    AudioComponent c = AudioComponentFindNext(NULL, &d);
    if (!c || AudioComponentInstanceNew(c, &a->au)) { a->au = NULL; LOG("【红】AudioUnit 建不出来：这一路改用静音接收端（不出声）"); return -1; }
    a->mix = a->ch == 6; if (a->mix && !a->mixbuf) a->mixbuf = malloc(MIXMAX * 12);
    if (a->mix && !a->mixbuf) { AudioComponentInstanceDispose(a->au); a->au = NULL; LOG("【红】下混暂存申请不到：这一路改用静音接收端（不出声）"); return -1; }
    int och = a->mix ? 2 : a->ch;
    AudioStreamBasicDescription f = {.mSampleRate = RATE, .mFormatID = kAudioFormatLinearPCM,
                                     .mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked,
                                     .mBytesPerPacket = 2u * och, .mFramesPerPacket = 1, .mBytesPerFrame = 2u * och,
                                     .mChannelsPerFrame = (UInt32)och, .mBitsPerChannel = 16};
    if (a->mix) LOG("audio：游戏给 6 声道（5.1）→ 交给声卡前自己下混成双声道（中置 / 后环绕 ×0.707 并入左右，低音不并入，整体 −3 dB、饱和截断）");
    AURenderCallbackStruct cb = {au_render, a};
    if (AudioUnitSetProperty(a->au, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &f, sizeof f) ||
        AudioUnitSetProperty(a->au, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb, sizeof cb) ||
        AudioUnitInitialize(a->au)) { AudioComponentInstanceDispose(a->au); a->au = NULL; LOG("【红】AudioUnit 设置失败（没有输出设备？）：这一路改用静音接收端（不出声）"); return -1; }
#if !TARGET_OS_IPHONE
    {
        AudioObjectID cur = 0, def = 0; UInt32 sz = sizeof cur; char cn[128], dn[128];
        AudioUnitGetProperty(a->au, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &cur, &sz);
        AudioObjectPropertyAddress pa = { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain }; sz = sizeof def;
        AudioObjectGetPropertyData(kAudioObjectSystemObject, &pa, 0, NULL, &sz, &def); dev_name(cur, cn, sizeof cn); dev_name(def, dn, sizeof dn);
        LOG("audio：输出设备「%s」（%u），系统缺省输出「%s」（%u）%s", cn, (unsigned)cur, dn, (unsigned)def, cur == def ? "，一致" : "【红】不一致"); }
#endif
    return 0;
}
static int dev_start(aout *a) { return AudioOutputUnitStart(a->au) ? -1 : 0; }
static void dev_stop(aout *a) { AudioOutputUnitStop(a->au); }
static void dev_close(aout *a) { AudioUnitUninitialize(a->au); AudioComponentInstanceDispose(a->au); a->au = NULL; }
#elif defined(__OHOS__)





static int32_t oh_write(OH_AudioRenderer *r, void *ctx, void *data, int32_t length) {
    (void)r; render(ctx, data, (uint32_t)length); return 0;
}
static int32_t oh_event(OH_AudioRenderer *r, void *ctx, OH_AudioStream_Event e) { (void)r; (void)ctx; LOG("audio：OHAudio 流事件 %d", (int)e); return 0; }
static int32_t oh_interrupt(OH_AudioRenderer *r, void *ctx, OH_AudioInterrupt_ForceType t, OH_AudioInterrupt_Hint h) {
    (void)r; (void)ctx; LOG("audio：音频焦点中断 type=%d hint=%d", (int)t, (int)h); return 0;
}
static void *null_sink(void *p);
static int au_open(aout *a);
static void *oh_reopen(void *p) {
    aout *a = p;
    pthread_mutex_lock(&g_aos_lock);
    if (a->au) {
        int playing = !g_suspended && atomic_load_explicit(&a->state, memory_order_acquire) == STATE_STARTED;
        OH_AudioRenderer_Release(a->au); a->au = NULL;
        if (au_open(a)) { LOG("【红】音频流出错后重开失败：这一路改用静音接收端（不出声）");
                          if (playing) { atomic_store_explicit(&a->null_run, 1, memory_order_release); pthread_create(&a->null_th, NULL, null_sink, a); } }
        else if (playing && OH_AudioRenderer_Start(a->au) != AUDIOSTREAM_SUCCESS) LOG("【红】音频流重开后启动失败");
        else LOG("audio：音频流出错，已重开");
    }
    pthread_mutex_unlock(&g_aos_lock);
    return NULL;
}
static int32_t oh_error(OH_AudioRenderer *r, void *ctx, OH_AudioStream_Result err) {
    (void)r; LOG("【红】OHAudio 流错误 %d，交给线程重开", (int)err);
    pthread_t th; if (!pthread_create(&th, NULL, oh_reopen, ctx)) pthread_detach(th);
    return 0;
}
static int au_open(aout *a) {
    a->mix = a->ch == 6; if (a->mix && !a->mixbuf) a->mixbuf = malloc(MIXMAX * 12);
    if (a->mix && !a->mixbuf) { a->au = NULL; LOG("【红】下混暂存申请不到：这一路改用静音接收端（不出声）"); return -1; }
    int och = a->mix ? 2 : a->ch;
    OH_AudioStreamBuilder *b = NULL;
    if (OH_AudioStreamBuilder_Create(&b, AUDIOSTREAM_TYPE_RENDERER) != AUDIOSTREAM_SUCCESS) { a->au = NULL; LOG("【红】OHAudio 建不出来：这一路改用静音接收端（不出声）"); return -1; }
    OH_AudioStreamBuilder_SetSamplingRate(b, RATE);
    OH_AudioStreamBuilder_SetChannelCount(b, och);
    OH_AudioStreamBuilder_SetSampleFormat(b, AUDIOSTREAM_SAMPLE_S16LE);
    OH_AudioStreamBuilder_SetEncodingType(b, AUDIOSTREAM_ENCODING_TYPE_RAW);
    OH_AudioStreamBuilder_SetRendererInfo(b, AUDIOSTREAM_USAGE_GAME);
    OH_AudioStreamBuilder_SetLatencyMode(b, AUDIOSTREAM_LATENCY_MODE_FAST);
    OH_AudioRenderer_Callbacks cb = {.OH_AudioRenderer_OnWriteData = oh_write, .OH_AudioRenderer_OnStreamEvent = oh_event,
                                     .OH_AudioRenderer_OnInterruptEvent = oh_interrupt, .OH_AudioRenderer_OnError = oh_error};
    OH_AudioStreamBuilder_SetRendererCallback(b, cb, a);
    OH_AudioStream_Result r = OH_AudioStreamBuilder_GenerateRenderer(b, &a->au);
    OH_AudioStreamBuilder_Destroy(b);
    if (r != AUDIOSTREAM_SUCCESS) { a->au = NULL; LOG("【红】OHAudio 打开失败 %d：这一路改用静音接收端（不出声）", (int)r); return -1; }
    if (a->mix) LOG("audio：游戏给 6 声道（5.1）→ 交给声卡前自己下混成双声道（中置 / 后环绕 ×0.707 并入左右，低音不并入，整体 −3 dB、饱和截断）");
    int32_t rate = 0, chn = 0, fpc = 0; OH_AudioRenderer_GetSamplingRate(a->au, &rate); OH_AudioRenderer_GetChannelCount(a->au, &chn); OH_AudioRenderer_GetFrameSizeInCallback(a->au, &fpc);
    LOG("audio：OHAudio 流 %d Hz %d 声道，每次回调 %d 帧", (int)rate, (int)chn, (int)fpc);
    return 0;
}
static int dev_start(aout *a) { return OH_AudioRenderer_Start(a->au) == AUDIOSTREAM_SUCCESS ? 0 : -1; }
static void dev_stop(aout *a) { OH_AudioRenderer_Stop(a->au); }
static void dev_close(aout *a) { OH_AudioRenderer_Release(a->au); a->au = NULL; }
#else




static aaudio_data_callback_result_t aa_render(AAudioStream *st, void *ctx, void *data, int32_t frames) {
    render(ctx, data, (uint32_t)frames * (uint32_t)AAudioStream_getChannelCount(st) * 2u);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
static void *null_sink(void *p);
static int au_open(aout *a);
static int dev_start(aout *a);
static void dev_close(aout *a);
static void *aa_reopen(void *p) {
    aout *a = p;
    pthread_mutex_lock(&g_aos_lock);
    if (a->au) {
        int playing = !g_suspended && atomic_load_explicit(&a->state, memory_order_acquire) == STATE_STARTED;
        AAudioStream_close(a->au); a->au = NULL;
        if (au_open(a)) { LOG("【红】音频流断开后重开失败：这一路改用静音接收端（不出声）");
                          if (playing) { atomic_store_explicit(&a->null_run, 1, memory_order_release); pthread_create(&a->null_th, NULL, null_sink, a); } }
        else if (playing && dev_start(a)) { LOG("【红】音频流重开后启动失败");
                                            dev_close(a); atomic_store_explicit(&a->null_run, 1, memory_order_release); pthread_create(&a->null_th, NULL, null_sink, a); }
        else LOG("audio：音频流断开（输出设备变化），已重开");
    }
    pthread_mutex_unlock(&g_aos_lock);
    return NULL;
}
static void aa_error(AAudioStream *st, void *ctx, aaudio_result_t err) {
    (void)st;
    if (err != AAUDIO_ERROR_DISCONNECTED) { LOG("【红】音频流错误 %d（%s）", (int)err, AAudio_convertResultToText(err)); return; }
    pthread_t th; if (!pthread_create(&th, NULL, aa_reopen, ctx)) pthread_detach(th);
}
static int au_open(aout *a) {
    a->mix = a->ch == 6; if (a->mix && !a->mixbuf) a->mixbuf = malloc(MIXMAX * 12);
    if (a->mix && !a->mixbuf) { a->au = NULL; LOG("【红】下混暂存申请不到：这一路改用静音接收端（不出声）"); return -1; }
    int och = a->mix ? 2 : a->ch;
    AAudioStreamBuilder *b = NULL;
    if (AAudio_createStreamBuilder(&b) != AAUDIO_OK) { a->au = NULL; LOG("【红】AAudio 建不出来：这一路改用静音接收端（不出声）"); return -1; }
    AAudioStreamBuilder_setDirection(b, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSampleRate(b, RATE);
    AAudioStreamBuilder_setChannelCount(b, och);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setPerformanceMode(b, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setUsage(b, AAUDIO_USAGE_GAME);
    AAudioStreamBuilder_setContentType(b, AAUDIO_CONTENT_TYPE_SONIFICATION);
    AAudioStreamBuilder_setDataCallback(b, aa_render, a);
    AAudioStreamBuilder_setErrorCallback(b, aa_error, a);
    aaudio_result_t r = AAudioStreamBuilder_openStream(b, &a->au);
    AAudioStreamBuilder_delete(b);
    if (r != AAUDIO_OK) { a->au = NULL; LOG("【红】AAudio 打开失败 %d（%s）：这一路改用静音接收端（不出声）", (int)r, AAudio_convertResultToText(r)); return -1; }
    if (a->mix) LOG("audio：游戏给 6 声道（5.1）→ 交给声卡前自己下混成双声道（中置 / 后环绕 ×0.707 并入左右，低音不并入，整体 −3 dB、饱和截断）");
    LOG("audio：AAudio 流 %d Hz %d 声道，性能模式 %d，每次回调 %d 帧，缓冲 %d / %d 帧", AAudioStream_getSampleRate(a->au), AAudioStream_getChannelCount(a->au),
        (int)AAudioStream_getPerformanceMode(a->au), AAudioStream_getFramesPerBurst(a->au), AAudioStream_getBufferSizeInFrames(a->au), AAudioStream_getBufferCapacityInFrames(a->au));
    return 0;
}

static int dev_start(aout *a) {
    aaudio_result_t r = AAudioStream_requestStart(a->au);
    if (r != AAUDIO_OK) LOG("【红】audio: AAudio start failed %d (%s), stream state %d", (int)r, AAudio_convertResultToText(r), (int)AAudioStream_getState(a->au));
    return r == AAUDIO_OK ? 0 : -1;
}
static void dev_stop(aout *a) { AAudioStream_requestStop(a->au); }
static void dev_close(aout *a) { AAudioStream_close(a->au); a->au = NULL; }
#endif





static pthread_mutex_t g_idle_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_idle_cv = PTHREAD_COND_INITIALIZER;
static int g_idle;
void dm_audio_idle(int on) {
    pthread_mutex_lock(&g_idle_m);
    g_idle = on != 0;
    pthread_cond_broadcast(&g_idle_cv);
    pthread_mutex_unlock(&g_idle_m);
}



static void null_stop(aout *a) {
    atomic_store_explicit(&a->null_run, 0, memory_order_release);
    pthread_mutex_lock(&g_idle_m); pthread_cond_broadcast(&g_idle_cv); pthread_mutex_unlock(&g_idle_m);
    pthread_join(a->null_th, NULL);
}


static void *null_sink(void *p) {
    aout *a = p; uint8_t scratch[RATE / 200 * 2 * 8];
    uint64_t bytes = (uint64_t)RATE / 200 * 2 * a->ch, t = now_ns();
    while (atomic_load_explicit(&a->null_run, memory_order_acquire)) {
        pthread_mutex_lock(&g_idle_m);
        int parked = 0;
        while (g_idle && atomic_load_explicit(&a->null_run, memory_order_acquire)) { parked = 1; pthread_cond_wait(&g_idle_cv, &g_idle_m); }
        pthread_mutex_unlock(&g_idle_m);
        if (parked) { t = now_ns(); continue; }
        t += 5000000; uint64_t now = now_ns(); if (t > now) usleep((useconds_t)((t - now) / 1000));
        pull(a, scratch, bytes);
    }
    return NULL;
}
static int real_audio(void) { const char *e = getenv("DM_AUDIO"); return e ? atoi(e) != 0 : DM_PHONE; }



static int dev_bring_up(aout *a) {
    if (a->au) {
        if (!dev_start(a)) return 0;
        dev_close(a);
    }
    if (!real_audio() || au_open(a)) return -1;
    if (dev_start(a)) { dev_close(a); return -1; }
    LOG("audio: sound card output opened on retry and started");
    return 0;
}
static void null_start(aout *a) { atomic_store_explicit(&a->null_run, 1, memory_order_release); pthread_create(&a->null_th, NULL, null_sink, a); }



HLE_FORCE("_ZN2nn5audio27InitializeAudioOutParameterEPNS0_17AudioOutParameterE", "音频输出参数默认值", void, h_au_InitParam, uint8_t *p) { memset(p, 0, 8); }

HLE_FORCE("_ZN2nn5audio13ListAudioOutsEPNS0_12AudioOutInfoEi", "音频输出设备列表；宿主 1 个", int, h_au_List, char *info, int n) {
    if (n < 1) return 0;
    memset(info, 0, 256); strcpy(info, "DeviceOut"); return 1;
}
HLE_FORCE("_ZN2nn5audio12OpenAudioOutEPNS0_8AudioOutEPNS_2os11SystemEventEPKcRKNS0_17AudioOutParameterE", "连 audout 服务；宿主 AudioUnit / 静音接收端",
          Result, h_au_Open, void *g, void *ev, const char *name, const uint8_t *param) {
    int ch = *(const uint16_t *)(param + 4); int rate = *(const int32_t *)param;
    if (!ch) ch = 2;
    if (rate && rate != RATE) die("AudioOut 采样率 %d 不支持（只做 48 kHz）", rate);
    aout *a = calloc(1, sizeof *a);
    pthread_mutex_init(&a->m, NULL); a->event = ev; a->ch = ch;
    atomic_init(&a->state, STATE_STOPPED);
    atomic_init(&a->q_head, 0); atomic_init(&a->q_tail, 0);
    atomic_init(&a->rel_head, 0); atomic_init(&a->rel_tail, 0);
    atomic_init(&a->null_run, 0); atomic_init(&a->notify_run, 1);
    atomic_init(&a->notify_pending, 0); atomic_init(&a->notify_armed, 0);
    a->notify_sem = sem_new();
    pthread_create(&a->notify_th, NULL, event_notify, a);
    if (ev) hle_bind_system_event(ev, 1);
    objmap_put(g, a);
    pthread_mutex_lock(&g_aos_lock);
    int slot = -1; for (int i = 0; i < AOMAX; i++) if (!g_aos[i]) { slot = i; break; }
    if (slot < 0) die("AudioOut 同时打开超过 %d 个", AOMAX);
    g_aos[slot] = a;
    pthread_mutex_unlock(&g_aos_lock);
    LOG("audio::OpenAudioOut %s 声道 %d %s", name ? name : "(默认)", ch, real_audio() ? "真出声" : "静音接收端");
    if (real_audio()) au_open(a);
    return RES_OK;
}
HLE_FORCE("_ZN2nn5audio13StartAudioOutEPNS0_8AudioOutE", "开始播放", Result, h_au_Start, void *g) {
    aout *a = ao(g);
    pthread_mutex_lock(&a->m); int was = atomic_load_explicit(&a->state, memory_order_relaxed); atomic_store_explicit(&a->state, STATE_STARTED, memory_order_release); pthread_mutex_unlock(&a->m);
    if (was == STATE_STARTED) return RES_OK;
    pthread_mutex_lock(&g_aos_lock);
    int had = a->au != NULL;
    if (g_suspended || dev_bring_up(a)) {
        if (!g_suspended && (had || real_audio())) LOG("【红】声卡启动失败：这一路改用静音接收端（不出声）");
        null_start(a);
    }
    pthread_mutex_unlock(&g_aos_lock);
    return RES_OK;
}
HLE_FORCE("_ZN2nn5audio12StopAudioOutEPNS0_8AudioOutE", "停止播放", void, h_au_Stop, void *g) {
    aout *a = ao(g);
    pthread_mutex_lock(&a->m); int was = atomic_load_explicit(&a->state, memory_order_relaxed); atomic_store_explicit(&a->state, STATE_STOPPED, memory_order_release); pthread_mutex_unlock(&a->m);
    if (was != STATE_STARTED) return;
    pthread_mutex_lock(&g_aos_lock);
    if (atomic_load_explicit(&a->null_run, memory_order_acquire)) null_stop(a);
    else if (a->au) dev_stop(a);
    pthread_mutex_unlock(&g_aos_lock);
}
HLE_FORCE("_ZN2nn5audio13CloseAudioOutEPNS0_8AudioOutE", "关闭音频输出", void, h_au_Close, void *g) {
    h_au_Stop(g);
    aout *a = objmap_del(g);
    atomic_store_explicit(&a->notify_run, 0, memory_order_release);
    sem_up(a->notify_sem);
    pthread_join(a->notify_th, NULL);
    pthread_mutex_lock(&g_aos_lock);
    for (int i = 0; i < AOMAX; i++) if (g_aos[i] == a) g_aos[i] = NULL;
    pthread_mutex_unlock(&g_aos_lock);
    if (a->au) dev_close(a);
    sem_free(a->notify_sem);
    free(a->mixbuf); pthread_mutex_destroy(&a->m); free(a);
}
HLE_FORCE("_ZN2nn5audio16GetAudioOutStateEPKNS0_8AudioOutE", "播放状态", int, h_au_State, const void *g) { return atomic_load_explicit(&ao(g)->state, memory_order_acquire); }
HLE_FORCE("_ZN2nn5audio21GetAudioOutSampleRateEPKNS0_8AudioOutE", "采样率", int, h_au_Rate, const void *g) { (void)ao(g); return RATE; }
HLE_FORCE("_ZN2nn5audio23GetAudioOutChannelCountEPKNS0_8AudioOutE", "声道数", int, h_au_Ch, const void *g) { return ao(g)->ch; }
HLE_FORCE("_ZN2nn5audio23GetAudioOutSampleFormatEPKNS0_8AudioOutE", "采样格式；只做 PCM16", int, h_au_Fmt, const void *g) { (void)ao(g); return FMT_PCM16; }

HLE_FORCE("_ZN2nn5audio21SetAudioOutBufferInfoEPNS0_14AudioOutBufferEPvmm", "缓冲描述；写进客体结构体前 24 字节", void, h_au_SetBuf, gbuf *b, void *p, uint64_t cap, uint64_t size) {
    b->ptr = p; b->cap = cap; b->size = size;
}
HLE_FORCE("_ZN2nn5audio28GetAudioOutBufferDataPointerEPKNS0_14AudioOutBufferE", "缓冲数据指针", void *, h_au_BufPtr, const gbuf *b) { return b->ptr; }
HLE_FORCE("_ZN2nn5audio25GetAudioOutBufferDataSizeEPKNS0_14AudioOutBufferE", "缓冲数据大小", uint64_t, h_au_BufSize, const gbuf *b) { return b->size; }
HLE_FORCE("_ZN2nn5audio20AppendAudioOutBufferEPNS0_8AudioOutEPNS0_14AudioOutBufferE", "追加待播缓冲", unsigned, h_au_Append, void *g, gbuf *b) {
    aout *a = ao(g);




    { static FILE *df; static int on = -1; if (on < 0) { const char *e = getenv("DM_AUDIO_DUMP"); on = e != NULL; if (on && !(df = fopen(e, "wb"))) LOG("【红】DM_AUDIO_DUMP 打不开 %s", e); }
      if (df && b && b->ptr && b->size) fwrite(b->ptr, 1, (size_t)b->size, df); }
    pthread_mutex_lock(&a->m);
    uint32_t qt = atomic_load_explicit(&a->q_tail, memory_order_relaxed);
    uint32_t qh = atomic_load_explicit(&a->q_head, memory_order_acquire);
    int ok = qt - qh < QMAX;
    if (ok) { a->q[qt & (QMAX - 1)] = b; atomic_store_explicit(&a->q_tail, qt + 1, memory_order_release); }
    pthread_mutex_unlock(&a->m);
    return ok;
}
HLE_FORCE("_ZN2nn5audio25GetReleasedAudioOutBufferEPNS0_8AudioOutE", "取已播完的缓冲；没有则 NULL", gbuf *, h_au_Released, void *g) {
    aout *a = ao(g); gbuf *b = NULL;
    pthread_mutex_lock(&a->m);
    uint32_t rr = atomic_load_explicit(&a->rel_head, memory_order_relaxed);
    uint32_t rw = atomic_load_explicit(&a->rel_tail, memory_order_acquire);
    if (rr != rw) { b = a->rel[rr & (RELMAX - 1)]; atomic_store_explicit(&a->rel_head, rr + 1, memory_order_release); }
    pthread_mutex_unlock(&a->m);
    return b;
}





void dm_audio_suspend(int on) {
    pthread_mutex_lock(&g_aos_lock);
    if (g_suspended != on) {
        g_suspended = on; int failed = 0;
        for (int i = 0; i < AOMAX; i++) {
            aout *a = g_aos[i];
            if (!a || atomic_load_explicit(&a->state, memory_order_acquire) != STATE_STARTED) continue;
            int on_sink = atomic_load_explicit(&a->null_run, memory_order_acquire);


            if (on) { if (!on_sink && a->au) { dev_stop(a); null_start(a); } }

            else if (on_sink && (a->au || real_audio())) {
                null_stop(a);
                if (dev_bring_up(a)) {
                    LOG("【红】声卡恢复失败：保持静音接收端，等下次恢复重试");
                    null_start(a); failed = 1; } }
        }
        if (failed) g_suspended = 1;
        LOG("audio %s", on ? "暂停（切后台 / 被打断）" : failed ? "恢复未成" : "恢复（回前台 / 打断结束）");
    }
    pthread_mutex_unlock(&g_aos_lock);
}
