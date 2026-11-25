



#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../host/dump_reader.h"
#include "../host/rd_crypto.h"

typedef struct {
    pb_reader *r;
    pb_contract parsed;
    const pb_contract *c;
} handle_t;

static jstring error_text(JNIEnv *env, const pb_error *e) {
    char out[sizeof e->id + sizeof e->detail + 2];
    snprintf(out, sizeof out, "%s\t%s", e->id, e->detail);
    return (*env)->NewStringUTF(env, out);
}


static const pb_contract *contract_of(JNIEnv *env, jstring contract, pb_contract *own, pb_error *e) {
    memset(own, 0, sizeof *own);
    if (!contract) return pb_contract_game();
    const char *t = (*env)->GetStringUTFChars(env, contract, NULL);
    if (!t) { snprintf(e->id, sizeof e->id, "PB-IO"); snprintf(e->detail, sizeof e->detail, "memory"); return NULL; }
    int r = pb_contract_parse(t, strlen(t), own, e);
    (*env)->ReleaseStringUTFChars(env, contract, t);
    return r ? NULL : own;
}


static uint8_t *keys_copy(JNIEnv *env, jbyteArray keys, size_t *n) {
    *n = 0;
    if (!keys) return NULL;
    jsize len = (*env)->GetArrayLength(env, keys);
    uint8_t *b = malloc(len ? (size_t)len : 1);
    if (!b) return NULL;
    (*env)->GetByteArrayRegion(env, keys, 0, len, (jbyte *)b);
    *n = (size_t)len;
    return b;
}
static void keys_free(uint8_t *b, size_t n) {
    if (!b) return;
    rd_wipe(b, n);
    free(b);
}

JNIEXPORT jstring JNICALL Java_com_xiaoboluo_d2r_PackReader_nativeProbe(JNIEnv *env, jclass cls, jint fd, jlong length, jbyteArray keys, jstring contract) {
    (void)cls;
    pb_error e = {{0}, {0}};
    pb_contract own;
    const pb_contract *c = contract_of(env, contract, &own, &e);
    if (!c) return error_text(env, &e);
    size_t kn;
    uint8_t *k = keys_copy(env, keys, &kn);
    if (keys && !k) { pb_contract_free(&own); return (*env)->NewStringUTF(env, "PB-IO\tmemory"); }
    int kind = -1;
    char id[65];
    int r = length < 0 ? -1 : pb_probe(fd, (uint64_t)length, k, kn, c, &kind, id, &e);
    if (length < 0) { snprintf(e.id, sizeof e.id, "PB-SOURCE"); snprintf(e.detail, sizeof e.detail, "length"); }
    keys_free(k, kn);
    pb_contract_free(&own);
    if (r) return error_text(env, &e);
    static const char *kinds[] = {"keys", "package", "base", "update", "addon"};
    char out[96];
    if (kind == PB_KIND_KEYS) snprintf(out, sizeof out, "keys");
    else snprintf(out, sizeof out, "%s\t%s", kinds[kind], id);
    return (*env)->NewStringUTF(env, out);
}

JNIEXPORT jstring JNICALL Java_com_xiaoboluo_d2r_PackReader_nativeUnits(JNIEnv *env, jclass cls, jstring contract) {
    (void)cls;
    pb_error e = {{0}, {0}};
    pb_contract own;
    const pb_contract *c = contract_of(env, contract, &own, &e);
    if (!c) return error_text(env, &e);
    size_t cap = c->n * 192 + 1, used = 0;
    char *out = malloc(cap);
    if (!out) { pb_contract_free(&own); return (*env)->NewStringUTF(env, "PB-IO\tmemory"); }
    out[0] = 0;
    for (size_t i = 0; i < c->n; i++) {
        const pb_file *f = &c->files[i];
        used += (size_t)snprintf(out + used, cap - used, "%s%s\t%s\t%s\t%llu\t%s", i ? "\n" : "", f->unit,
                                 f->target == PB_TARGET_EXEFS ? "exefs" : "data", f->name, (unsigned long long)f->size, f->sha256);
    }
    jstring s = (*env)->NewStringUTF(env, out);
    free(out);
    pb_contract_free(&own);
    return s;
}

JNIEXPORT jlong JNICALL Java_com_xiaoboluo_d2r_PackReader_nativeOpen(JNIEnv *env, jclass cls, jint base_fd, jint update_fd, jbyteArray keys,
                                                                      jstring contract, jobjectArray error) {
    (void)cls;
    pb_error e = {{0}, {0}};
    handle_t *h = calloc(1, sizeof *h);
    size_t kn = 0;
    uint8_t *k = h ? keys_copy(env, keys, &kn) : NULL;
    if (!h || !k) {
        snprintf(e.id, sizeof e.id, "PB-IO");
        snprintf(e.detail, sizeof e.detail, keys ? "memory" : "keys");
    } else if ((h->c = contract_of(env, contract, &h->parsed, &e))) {
        h->r = pb_open(base_fd, update_fd, k, kn, h->c, &e);
    }
    keys_free(k, kn);
    if (h && h->r) return (jlong)(intptr_t)h;
    if (h) { pb_contract_free(&h->parsed); free(h); }
    if (error && (*env)->GetArrayLength(env, error) > 0) (*env)->SetObjectArrayElement(env, error, 0, error_text(env, &e));
    return 0;
}

typedef struct {
    JNIEnv *env;
    jobject sink;
    jmethodID wrote;
    pb_reader *r;
    const char *last;
    jstring name;
} sink_ctx;

static void on_progress(void *ctx, uint64_t bytes, const char *file) {
    sink_ctx *s = ctx;
    JNIEnv *env = s->env;
    if ((*env)->ExceptionCheck(env)) return;
    if (file != s->last) {
        if (s->name) (*env)->DeleteLocalRef(env, s->name);
        s->name = (*env)->NewStringUTF(env, file);
        s->last = file;
        if (!s->name) { pb_cancel(s->r); return; }
    }
    (*env)->CallVoidMethod(env, s->sink, s->wrote, (jlong)bytes, s->name);
    if ((*env)->ExceptionCheck(env)) pb_cancel(s->r);
}

JNIEXPORT jstring JNICALL Java_com_xiaoboluo_d2r_PackReader_nativeExtract(JNIEnv *env, jclass cls, jlong handle, jstring unit, jstring exefs_dir,
                                                                          jstring data_dir, jobject sink) {
    (void)cls;
    handle_t *h = (handle_t *)(intptr_t)handle;
    if (!h || !unit || !exefs_dir || !data_dir) return (*env)->NewStringUTF(env, "PB-IO\targument");
    sink_ctx s = {env, sink, NULL, h->r, NULL, NULL};
    if (sink) {
        jclass sc = (*env)->GetObjectClass(env, sink);
        s.wrote = (*env)->GetMethodID(env, sc, "wrote", "(JLjava/lang/String;)V");
        (*env)->DeleteLocalRef(env, sc);
        if (!s.wrote) return NULL;
    }
    const char *u = (*env)->GetStringUTFChars(env, unit, NULL);
    const char *x = (*env)->GetStringUTFChars(env, exefs_dir, NULL);
    const char *d = (*env)->GetStringUTFChars(env, data_dir, NULL);
    pb_error e = {{0}, {0}};
    int r = !u || !x || !d ? -1 : pb_extract(h->r, u, x, d, sink ? on_progress : NULL, &s, &e);
    if (!u || !x || !d) { snprintf(e.id, sizeof e.id, "PB-IO"); snprintf(e.detail, sizeof e.detail, "memory"); }
    if (u) (*env)->ReleaseStringUTFChars(env, unit, u);
    if (x) (*env)->ReleaseStringUTFChars(env, exefs_dir, x);
    if (d) (*env)->ReleaseStringUTFChars(env, data_dir, d);
    if ((*env)->ExceptionCheck(env)) return NULL;
    if (s.name) (*env)->DeleteLocalRef(env, s.name);
    return r ? error_text(env, &e) : (*env)->NewStringUTF(env, "");
}

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_PackReader_nativeCancel(JNIEnv *env, jclass cls, jlong handle) {
    (void)env; (void)cls;
    handle_t *h = (handle_t *)(intptr_t)handle;
    if (h) pb_cancel(h->r);
}

JNIEXPORT void JNICALL Java_com_xiaoboluo_d2r_PackReader_nativeClose(JNIEnv *env, jclass cls, jlong handle) {
    (void)env; (void)cls;
    handle_t *h = (handle_t *)(intptr_t)handle;
    if (!h) return;
    pb_close(h->r);
    pb_contract_free(&h->parsed);
    free(h);
}
