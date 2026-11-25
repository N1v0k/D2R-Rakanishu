

#include "../host/dm.h"
#include "../host/dm_actions.h"
#include <jni.h>
#include <android/log.h>
#include <stdatomic.h>

static JavaVM *vm;
static jclass activity_class;
static jmethodID show_input, report_failure, drop_haptic;
static pthread_mutex_t input_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t input_done = PTHREAD_COND_INITIALIZER;
static pthread_cond_t work_ready = PTHREAD_COND_INITIALIZER;
static jlong next_request, active_request;
static int pending, result, output_capacity, worker_state;
static char *output;
typedef struct input_job {
    struct input_job *next;
    jlong id;
    char *strings[4];
    int max_chars, name_rule, numeric_max, failure_code;


    int haptic;
} input_job;
static input_job *jobs_head, *jobs_tail;

extern int g_swkbd_name_panel, g_swkbd_numeric_max;
extern _Atomic int g_fe_name_no_accept;
size_t swkbd_u8_to_u16(const char *, size_t, uint16_t *, size_t);
size_t swkbd_u16_to_u8(const uint16_t *, size_t, char *, size_t);

static void free_job(input_job *job) {
    for (unsigned i = 0; i < 4; i++) free(job->strings[i]);
    free(job);
}



static void enqueue(input_job *job) {
    if (jobs_tail) jobs_tail->next = job; else jobs_head = job;
    jobs_tail = job;
    pthread_cond_signal(&work_ready);
}

static void cancel_request(jlong id) {
    pthread_mutex_lock(&input_lock);
    if (active_request == id && pending) {
        result = 0; pending = 0;
        pthread_cond_signal(&input_done);
    }
    pthread_mutex_unlock(&input_lock);
}



static jstring utf8_string(JNIEnv *env, const char *text) {
    size_t capacity = strlen(text) + 1;
    uint16_t *wide = calloc(capacity, sizeof *wide);
    if (!wide) return NULL;
    size_t count = swkbd_u8_to_u16(text, capacity, wide, capacity);
    jstring value = (*env)->NewString(env, wide, (jsize)count);
    free(wide);
    return value;
}

static void *input_worker(void *unused) {
    (void)unused;
    pthread_setname_np(pthread_self(), "dm-java-input");
    JNIEnv *env = NULL;
    int ready = (*vm)->AttachCurrentThreadAsDaemon(vm, &env, NULL) == JNI_OK;
    pthread_mutex_lock(&input_lock);
    worker_state = ready ? 1 : -1;
    pthread_cond_broadcast(&input_done);
    pthread_mutex_unlock(&input_lock);
    __android_log_print(ready ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR, "dm-input", "host worker ready=%d", ready);
    if (!ready) return NULL;
    for (;;) {
        pthread_mutex_lock(&input_lock);
        while (!jobs_head) pthread_cond_wait(&work_ready, &input_lock);
        input_job *job = jobs_head;
        jobs_head = job->next;
        if (!jobs_head) jobs_tail = NULL;
        pthread_mutex_unlock(&input_lock);


        if (job->haptic) {
            (*env)->CallStaticVoidMethod(env, activity_class, drop_haptic, job->haptic);
        } else if (!job->id) {
            (*env)->CallStaticVoidMethod(env, activity_class, report_failure, job->failure_code);
        } else {
            jstring strings[4] = {0};
            int valid = 1;
            for (unsigned i = 0; i < 4; i++) {
                strings[i] = utf8_string(env, job->strings[i]);
                if (!strings[i] || (*env)->ExceptionCheck(env)) { valid = 0; break; }
            }
            __android_log_print(ANDROID_LOG_INFO, "dm-input", "host dispatch id=%lld valid=%d", (long long)job->id, valid);
            if (valid) (*env)->CallStaticVoidMethod(env, activity_class, show_input, job->id,
                strings[0], strings[1], strings[2], strings[3], job->max_chars, job->name_rule, job->numeric_max);
            if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionDescribe(env); (*env)->ExceptionClear(env); valid = 0; }
            for (unsigned i = 0; i < 4; i++) if (strings[i]) (*env)->DeleteLocalRef(env, strings[i]);
            if (!valid) cancel_request(job->id);
        }
        if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionDescribe(env); (*env)->ExceptionClear(env); }
        free_job(job);
    }
    return NULL;
}

JNIEXPORT jint JNI_OnLoad(JavaVM *java_vm, void *reserved) {
    (void)reserved;
    JNIEnv *env;
    if ((*java_vm)->GetEnv(java_vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;
    jclass local = (*env)->FindClass(env, "com/xiaoboluo/d2r/DmActivity");
    if (!local) return JNI_ERR;
    activity_class = (*env)->NewGlobalRef(env, local);
    (*env)->DeleteLocalRef(env, local);
    if (!activity_class) return JNI_ERR;
    show_input = (*env)->GetStaticMethodID(env, activity_class, "showNativeInput",
        "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;III)V");
    report_failure = (*env)->GetStaticMethodID(env, activity_class, "showNativeFailure", "(I)V");
    if (!show_input || !report_failure) return JNI_ERR;


    drop_haptic = (*env)->GetStaticMethodID(env, activity_class, "dropHaptic", "(I)V");
    if (!drop_haptic) {
        (*env)->ExceptionClear(env);
        __android_log_print(ANDROID_LOG_ERROR, "dm-input", "DmActivity.dropHaptic not found: drop vibration is off");
    }
    vm = java_vm;
    pthread_t worker;
    if (pthread_create(&worker, NULL, input_worker, NULL)) worker_state = -1;
    else pthread_detach(worker);
    return JNI_VERSION_1_6;
}

void front_android_boot_failed(int code) {
    input_job *job = calloc(1, sizeof *job);
    if (!job) return;
    job->failure_code = code;
    pthread_mutex_lock(&input_lock);
    while (!worker_state) pthread_cond_wait(&input_done, &input_lock);
    if (worker_state > 0) { enqueue(job); job = NULL; }
    pthread_mutex_unlock(&input_lock);
    if (job) free_job(job);
}

int front_android_text_input(const char *title, const char *sub, const char *guide,
                             const char *initial, int max_chars, char *out, int out_cap) {
    if (!out || out_cap <= 0) return -1;
    input_job *job = calloc(1, sizeof *job);
    if (!job) return -1;
    const char *source[] = {title, sub, guide, initial};
    for (unsigned i = 0; i < 4; i++) {
        job->strings[i] = strdup(source[i] ? source[i] : "");
        if (!job->strings[i]) { free_job(job); return -1; }
    }
    job->max_chars = max_chars; job->name_rule = g_swkbd_name_panel; job->numeric_max = g_swkbd_numeric_max;
    int name_rule = job->name_rule;
    pthread_mutex_lock(&input_lock);
    while (!worker_state) pthread_cond_wait(&input_done, &input_lock);
    if (worker_state < 0 || active_request) { pthread_mutex_unlock(&input_lock); free_job(job); return -1; }
    if (next_request == INT64_MAX) next_request = 0;
    job->id = active_request = ++next_request;
    pending = 1; result = 0; output = out; output_capacity = out_cap; out[0] = 0;
    __android_log_print(ANDROID_LOG_INFO, "dm-input", "queued id=%lld max=%d capacity=%d", (long long)job->id, max_chars, out_cap);
    enqueue(job);
    while (pending) pthread_cond_wait(&input_done, &input_lock);
    int accepted = result;
    output = NULL; active_request = 0;
    pthread_mutex_unlock(&input_lock);
    if (accepted && name_rule && !out[0]) {
        accepted = dm_name_gen(out, out_cap, dm_name_taken);
        if (accepted) atomic_store(&g_fe_name_no_accept, 1);
    }
    return accepted;
}

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_DmActivity_nativeTextResult(
    JNIEnv *env, jclass cls, jlong id, jboolean accepted, jstring text) {
    (void)cls;
    pthread_mutex_lock(&input_lock);


    if (pending && active_request == id) {
        result = accepted && text;
        if (result) {
            const jchar *wide = (*env)->GetStringChars(env, text, NULL);
            if (wide) { swkbd_u16_to_u8(wide, (*env)->GetStringLength(env, text), output, output_capacity); (*env)->ReleaseStringChars(env, text, wide); }
            else result = 0;
        }
        __android_log_print(ANDROID_LOG_INFO, "dm-input", "completed id=%lld accepted=%d", (long long)id, result);
        pending = 0;
        pthread_cond_signal(&input_done);
    }
    pthread_mutex_unlock(&input_lock);
}






void front_android_drop_haptic(int tier) {
    if (!drop_haptic || tier < 1 || tier > 3) return;
    input_job *job = calloc(1, sizeof *job);
    if (!job) return;
    job->haptic = tier;
    pthread_mutex_lock(&input_lock);
    if (worker_state > 0) { enqueue(job); job = NULL; }
    pthread_mutex_unlock(&input_lock);
    if (job) free_job(job);
}
