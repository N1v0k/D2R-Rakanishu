




















#ifdef DM_ANDROID_TEST_HOOKS
#include "../host/dm.h"
#include "../host/dm_hud.h"
#include "qa_state.h"
#include <errno.h>
#include <fcntl.h>
#include <jni.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

extern void (*dm_qa_panels_hook)(int n, const char *const *open_names);
extern void (*dm_qa_tooltip_hook)(const char *text, uint32_t hovered);
extern void (*dm_qa_frame_hook)(void);
int dm_uiscreen_ingame(void);
int dm_loadscreen_up(void);
int dm_pause_wanted(void);
uint64_t dm_guest_frame(void);

static _Atomic int g_qa = -1;
static int g_state_on;
static char g_dir[1100], g_path[1200], g_tmp[1220];
static long long g_start_ms;

static long long boot_ms(void) { struct timespec ts; clock_gettime(CLOCK_BOOTTIME, &ts); return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000; }
static uint64_t mono_ns(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec; }


static long long age_ms(uint64_t now, uint64_t t) { return !t ? -1 : t >= now ? 0 : (long long)((now - t) / 1000000ull); }


static qa_panels g_pn[3], g_pn_last;
static qa_tri g_pn_tri;
static void qa_panels_hook(int n, const char *const *names) {
    qa_panels *b = &g_pn[g_pn_tri.back]; b->n = 0; b->overflow = 0;
    for (int i = 0; i < n; i++) { if (b->n == QA_PANELS) { b->overflow = 1; break; } strlcpy(b->name[b->n++], names[i], QA_PANEL_NAME); }
    if (qa_panels_same(b, &g_pn_last)) return;
    g_pn_last.n = b->n; g_pn_last.overflow = b->overflow; for (int i = 0; i < b->n; i++) strlcpy(g_pn_last.name[i], b->name[i], QA_PANEL_NAME);
    qa_tri_publish(&g_pn_tri);
}


static qa_tips g_tp[3], g_tp_cur, g_tp_last;
static qa_tri g_tp_tri;
static _Atomic uint64_t g_tp_seen_frame, g_tp_seen_ns;
static void tips_publish(const qa_tips *s) {
    if (qa_tips_same(s, &g_tp_last)) return;
    g_tp[g_tp_tri.back] = *s; g_tp_last = *s; qa_tri_publish(&g_tp_tri);
}
static void qa_tooltip_hook(const char *txt, uint32_t hovered) {
    uint64_t fr = dm_guest_frame();
    if (g_tp_cur.n && g_tp_cur.frame != fr) tips_publish(&g_tp_cur);
    qa_tips_add(&g_tp_cur, txt, fr, hovered, 1);
    atomic_store(&g_tp_seen_frame, fr); atomic_store(&g_tp_seen_ns, mono_ns());
}



static void qa_frame_hook(void) {
    uint64_t fr = dm_guest_frame();
    if (g_tp_cur.n && g_tp_cur.frame != fr) { tips_publish(&g_tp_cur); g_tp_cur.n = 0; g_tp_cur.frame = 0; }
    else if (!g_tp_cur.n && g_tp_last.n && fr >= atomic_load(&g_tp_seen_frame) + 3) { static const qa_tips none; tips_publish(&none); }
}


static pthread_mutex_t g_ov_mu = PTHREAD_MUTEX_INITIALIZER;
static int g_nov; static char g_ov_name[QA_OVERLAYS][QA_NAME], g_ov_state[QA_OVERLAYS][QA_OV_STATE]; static uint64_t g_ov_ns;

JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_TestHooks_nativeEnabled(JNIEnv *env, jclass cls) { (void)env; (void)cls; return atomic_load(&g_qa); }
JNIEXPORT jint JNICALL Java_com_xiaoboluo_d2r_TestHooks_nativeObservationEnabled(JNIEnv *env, jclass cls) { (void)env; (void)cls; return atomic_load(&g_qa) < 0 ? -1 : g_state_on; }
JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_TestHooks_nativeOverlay(JNIEnv *env, jclass cls, jstring jname, jstring jstate, jboolean drawn) {
    (void)cls; if (atomic_load(&g_qa) != 1 || !jname || !jstate) return;
    const char *n = (*env)->GetStringUTFChars(env, jname, NULL), *s = (*env)->GetStringUTFChars(env, jstate, NULL);
    if (n && s && qa_token_ok(n, QA_NAME) && qa_token_ok(s, QA_OV_STATE)) {
        fprintf(stderr, "[qa] %s %s %s\n", drawn ? "drawn" : "overlay", n, s);
        if (!drawn && g_state_on) {
            pthread_mutex_lock(&g_ov_mu);
            int i = 0; while (i < g_nov && strcmp(g_ov_name[i], n)) i++;
            if (i < QA_OVERLAYS) { if (i == g_nov) { strlcpy(g_ov_name[i], n, QA_NAME); g_nov++; } strlcpy(g_ov_state[i], s, QA_OV_STATE); g_ov_ns = mono_ns(); }
            pthread_mutex_unlock(&g_ov_mu);
            if (i >= QA_OVERLAYS) fprintf(stderr, "[qa] overlay table full, %s dropped\n", n);
        }
    } else if (n && s) fprintf(stderr, "[qa] overlay rejected: name or state not a token (state %zu bytes)\n", strlen(s));
    if (n) (*env)->ReleaseStringUTFChars(env, jname, n);
    if (s) (*env)->ReleaseStringUTFChars(env, jstate, s);
}





static int qa_write(const char *data, size_t n) {
    unlink(g_tmp);
    int fd = open(g_tmp, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600), e; if (fd < 0) return errno ? errno : EIO;
    fchmod(fd, 0600);
    for (size_t off = 0; off < n;) {
        ssize_t w = write(fd, data + off, n - off);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) { e = w < 0 ? errno : ENOSPC; close(fd); unlink(g_tmp); return e; }
        off += (size_t)w;
    }
    if (close(fd) || rename(g_tmp, g_path)) { e = errno ? errno : EIO; unlink(g_tmp); return e; }
    return 0;
}
static void *qa_writer(void *arg) {
    (void)arg; pthread_setname_np(pthread_self(), "dm-qa"); setpriority(PRIO_PROCESS, (id_t)gettid(), 10);
    static qa_snapshot snap; static char body[64000], prev[64000], file[72000];
    uint64_t last_write = 0; unsigned seq = 0; int failed = 0;
    for (;;) {
        struct timespec nap = {0, 250000000}; nanosleep(&nap, NULL);
        void dm_combat_log_drain(void);
        dm_combat_log_drain();
        uint64_t hud_ns = 0; dm_hud_get_stamped(&snap.hud, &hud_ns);
        snap.screen = dm_uiscreen_ingame() && snap.hud.valid ? QA_SCREEN_INGAME : dm_loadscreen_up() ? QA_SCREEN_LOADING : dm_guest_frame() ? QA_SCREEN_FRONTEND : QA_SCREEN_BOOT;
        snap.paused = dm_pause_wanted();
        { unsigned dm_dmgnum_qa_kills(void); snap.kills = dm_dmgnum_qa_kills(); }
        qa_tri_take(&g_pn_tri); snap.panels = g_pn[g_pn_tri.front];
        qa_tri_take(&g_tp_tri); snap.tips = g_tp[g_tp_tri.front];
        uint64_t ov_ns;
        pthread_mutex_lock(&g_ov_mu);
        snap.overlays = g_nov; memcpy(snap.ov_name, g_ov_name, sizeof snap.ov_name); memcpy(snap.ov_state, g_ov_state, sizeof snap.ov_state); ov_ns = g_ov_ns;
        pthread_mutex_unlock(&g_ov_mu);
        qa_buf bb = {body, sizeof body, 0, 0}; body[0] = 0;
        if (qa_render_body(&bb, &snap)) continue;
        uint64_t now = mono_ns(); int changed = strcmp(body, prev) != 0;
        if (!qa_write_due(changed, now, last_write, 500000000ull, 5000000000ull)) continue;
        struct timespec wall; clock_gettime(CLOCK_REALTIME, &wall);
        uint64_t seen_ns = atomic_load(&g_tp_seen_ns), seen_fr = atomic_load(&g_tp_seen_frame), fr = dm_guest_frame();
        qa_stamps st = {.pid = getpid(), .start_ms = g_start_ms, .mono_ms = boot_ms(), .wall_ms = (long long)wall.tv_sec * 1000 + wall.tv_nsec / 1000000,
                        .hud_age_ms = age_ms(now, hud_ns), .tip_age_ms = age_ms(now, seen_ns), .tip_frames = !seen_ns ? -1 : fr > seen_fr ? (long long)(fr - seen_fr) : 0,
                        .overlay_age_ms = age_ms(now, ov_ns), .seq = seq + 1, .frame = fr};
        qa_buf fb = {file, sizeof file, 0, 0}; file[0] = 0;
        if (qa_render_file(&fb, &st, body)) continue;
        int werr = qa_write(file, fb.n);
        if (werr) { if (!failed++) LOG("【红】qa: state.json 写不进去（errno %d），保留上一份", werr); last_write = now; continue; }
        if (failed) { LOG("qa: state.json 又写进去了（之前失败 %d 次）", failed); failed = 0; }
        seq++; last_write = now; memcpy(prev, body, bb.n + 1);
    }
    return NULL;
}



void dm_test_hooks_start(const char *files_dir) {
    snprintf(g_dir, sizeof g_dir, "%s/dm", files_dir); snprintf(g_path, sizeof g_path, "%s/state.json", g_dir); snprintf(g_tmp, sizeof g_tmp, "%s/state.json.tmp", g_dir);
    const char *qa = getenv("DM_QA"), *st = getenv("DM_QA_STATE");
    int on = !(qa && !strcmp(qa, "0")), state = on && st && !strcmp(st, "1");
    g_start_ms = boot_ms();
    if (state) {
        struct stat sd; mkdir(g_dir, 0700);
        if (lstat(g_dir, &sd) || !S_ISDIR(sd.st_mode) || sd.st_uid != getuid()) { LOG("【红】qa: %s 不是本应用的目录，状态文件不写", g_dir); state = 0; }
        else if ((sd.st_mode & 077) && chmod(g_dir, 0700)) { LOG("【红】qa: %s 收不回组 / 其他人的权限，状态文件不写", g_dir); state = 0; }
    }
    if (!state) { unlink(g_path); unlink(g_tmp); }
    else {
        qa_tri_init(&g_pn_tri); qa_tri_init(&g_tp_tri); g_state_on = 1;
        dm_qa_panels_hook = qa_panels_hook; dm_qa_tooltip_hook = qa_tooltip_hook; dm_qa_frame_hook = qa_frame_hook;
        pthread_t th;
        if (pthread_create(&th, NULL, qa_writer, NULL)) { dm_qa_panels_hook = NULL; dm_qa_tooltip_hook = NULL; dm_qa_frame_hook = NULL; g_state_on = 0; state = 0; LOG("【红】qa: 写状态文件的线程起不来"); }
        else {
            void dm_combat_log_start(void);
            dm_combat_log_start();
            pthread_detach(th);
        }
    }
    atomic_store(&g_qa, on);
    LOG("boot: test hooks %s: [qa] overlay lines %s, state file %s (files/dm/env: DM_QA=0 off, DM_QA_STATE=1 state file)", on ? "on" : "off", on ? "on" : "off", state ? g_path : "off");
}
#else

typedef int dm_test_hooks_absent;
#endif
