







#pragma once
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>







enum { DM_LOG_ROTATE_START = 8 << 20, DM_LOG_ROTATE_MAX = 32 << 20, DM_LOG_KEEP_TAIL = 8 << 20 };


static inline int dm_log_rotate_action(long long size) { return size < DM_LOG_ROTATE_START ? 0 : size > DM_LOG_ROTATE_MAX ? 2 : 1; }

static inline int dm_log_name(char *out, size_t n, const char *path, const char *suffix) { return snprintf(out, n, "%s%s", path, suffix) < (int)n ? 0 : -1; }


static inline FILE *dm_log_open(const char *path, long long *size) {
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600); if (fd < 0) return NULL;
    struct stat st; if (fstat(fd, &st) || !S_ISREG(st.st_mode)) { close(fd); return NULL; }
    if ((st.st_mode & 0777) != 0600) fchmod(fd, 0600);
    FILE *f = fdopen(fd, "a"); if (!f) { close(fd); return NULL; }
    if (size) *size = (long long)st.st_size;
    return f;
}






static inline int dm_log_rotate_at_start(const char *path, long long *old, int *trim) {
    char one[1300], tr[1300]; struct stat st; *old = 0; *trim = 0;
    if (dm_log_name(one, sizeof one, path, ".1") || dm_log_name(tr, sizeof tr, path, ".trim")) return 0;
    int a = 0;
    if (!lstat(path, &st) && S_ISREG(st.st_mode)) {
        *old = (long long)st.st_size; a = dm_log_rotate_action(*old);
        if (a == 1 && rename(path, one)) a = -1;
        if (a == 2 && rename(path, tr)) a = -1;
        if (a == 1) unlink(tr);
    }
    *trim = !lstat(tr, &st) && S_ISREG(st.st_mode);
    return a;
}



static inline int dm_log_copy_tail(const char *src, const char *dst, long long keep) {
    char tmp[1300]; if (dm_log_name(tmp, sizeof tmp, dst, ".tmp")) return -1;
    int in = open(src, O_RDONLY | O_CLOEXEC | O_NOFOLLOW); if (in < 0) return -1;
    struct stat st; if (fstat(in, &st) || !S_ISREG(st.st_mode)) { close(in); return -1; }
    unlink(tmp);
    int out = open(tmp, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600); if (out < 0) { close(in); return -1; }
    long long size = (long long)st.st_size, from = size > keep ? size - keep : 0;
    int ok = lseek(in, (off_t)from, SEEK_SET) == (off_t)from, skip = from > 0;
    static char buf[65536]; ssize_t k = 0;
    while (ok && (k = read(in, buf, sizeof buf)) > 0) {
        char *p = buf; size_t n = (size_t)k;
        if (skip) { char *nl = (char *)memchr(p, '\n', n); if (!nl) continue; n -= (size_t)(nl + 1 - p); p = nl + 1; skip = 0; }
        while (n) { ssize_t w = write(out, p, n); if (w <= 0) { ok = 0; break; } p += w; n -= (size_t)w; }
    }
    if (k < 0) ok = 0;
    close(in); if (close(out)) ok = 0;
    if (!ok || rename(tmp, dst)) { unlink(tmp); return -1; }
    return 0;
}


static inline int dm_log_trim(const char *path) {
    char one[1300], tr[1300]; if (dm_log_name(one, sizeof one, path, ".1") || dm_log_name(tr, sizeof tr, path, ".trim")) return -1;
    int r = dm_log_copy_tail(tr, one, DM_LOG_KEEP_TAIL); unlink(tr); return r;
}

















typedef struct { uint64_t period_ns, last_full_ns; unsigned windows, thinned, dropped; int full, after_poolreset, note_due; } dm_perf_sampler;
static const char *const dm_perf_thin_keys[] = {"poolreset", "parts_ms_per_frame", "storage_snapshots_per_frame", "storage_snapshot", "wtrack",
    "uniform_trimmed_bindings", "quad_rewrites", "snapfront", "textures", "bright", "setcache", "keydiff", "binds", "binds_skipped_per_frame",
    "noload_passes_per_frame", "desc_ns_per_frame", "cleanup_ms_per_frame", "async", "events", "block_ms", "pass_ends", "xview", "wbskip", "pipememo",
    "texdirty", "vcache", "pipelines_new"};

static inline int dm_perf_keep(dm_perf_sampler *f, const char *line, uint64_t now_ns) {
    if (strncmp(line, "NVN PERF ", 9) || !f->period_ns) return 1;
    const char *k = line + 9; size_t kl = strcspn(k, " =");
    int poolreset = kl == 9 && !memcmp(k, "poolreset", 9), v1 = kl == 1 && k[0] == 'v';
    if (poolreset || (v1 && !f->after_poolreset)) {
        f->windows++; f->full = f->windows <= 3 || now_ns - f->last_full_ns >= f->period_ns;
        if (f->full) { f->last_full_ns = now_ns; f->note_due = f->thinned > 0; } else f->thinned++;
    }
    f->after_poolreset = poolreset;
    if (v1) return f->note_due ? 2 : 1;
    if (f->full || !f->windows) return 1;
    int thin = 0; for (size_t i = 0; i < sizeof dm_perf_thin_keys / sizeof *dm_perf_thin_keys && !thin; i++) thin = strlen(dm_perf_thin_keys[i]) == kl && !memcmp(k, dm_perf_thin_keys[i], kl);
    if (thin && kl == 13 && !memcmp(k, "pipelines_new", 13) && k[kl] == '=' && strtoul(k + kl + 1, NULL, 10) > 0) thin = 0;
    if (!thin) return 1;
    f->dropped++; return 0;
}

static inline void dm_perf_note(dm_perf_sampler *f, char *out, size_t n) {
    snprintf(out, n, "NVN PERF sampling thinned=%u dropped=%u every=%llus (DM_LOG_PERF_EVERY=0 keeps all)", f->thinned, f->dropped, (unsigned long long)(f->period_ns / 1000000000ull));
    f->thinned = f->dropped = 0; f->note_due = 0;
}
