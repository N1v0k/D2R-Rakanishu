







#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

static int copy_xattrs(const char *src, const char *dst) {
    enum { CAP = 4096 };
    char *names = malloc(2 * CAP), *v = names + CAP; if (!names) return -1;
    ssize_t n = llistxattr(src, names, CAP); int rc = 0;
    if (n < 0) rc = errno == ENOTSUP ? 0 : -1;
    for (char *k = names; n > 0 && !rc && k < names + n; k += strlen(k) + 1) {
        if (strncmp(k, "user.", 5)) continue;
        ssize_t m = lgetxattr(src, k, v, CAP);
        if (m < 0 || lsetxattr(dst, k, v, (size_t)m, 0)) rc = -1;
    }
    { int e = errno; free(names); errno = e; }
    return rc;
}

static int copy_file(const char *src, const char *dst, mode_t mode) {
    int in = open(src, O_RDONLY | O_CLOEXEC), out = -1, rc = -1;
    if (in < 0) return -1;
    char *buf = NULL; ssize_t k;
    if ((out = open(dst, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, mode & 0777)) < 0 || !(buf = malloc(1 << 16))) goto done;
    while ((k = read(in, buf, 1 << 16)) > 0)
        for (ssize_t w = 0; w < k;) { ssize_t z = write(out, buf + w, (size_t)(k - w)); if (z < 0) { if (errno == EINTR) continue; goto done; } w += z; }
    if (k < 0 || fsync(out)) goto done;
    rc = copy_xattrs(src, dst);
done:
    { int e = errno; free(buf); if (out >= 0) close(out); close(in); errno = e; }
    return rc;
}

static int copy_tree(const char *src, const char *dst) {
    struct stat st; if (lstat(src, &st)) return -1;
    if (S_ISREG(st.st_mode)) return copy_file(src, dst, st.st_mode);
    if (!S_ISDIR(st.st_mode)) { errno = ENOTSUP; return -1; }
    if (mkdir(dst, st.st_mode & 0777) || copy_xattrs(src, dst)) return -1;
    DIR *d = opendir(src); if (!d) return -1;
    struct dirent *e; int rc = 0;
    char *a = malloc(2 * PATH_MAX), *b = a + PATH_MAX; if (!a) { closedir(d); return -1; }
    while (!rc && (e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        if (snprintf(a, PATH_MAX, "%s/%s", src, e->d_name) >= PATH_MAX || snprintf(b, PATH_MAX, "%s/%s", dst, e->d_name) >= PATH_MAX) { errno = ENAMETOOLONG; rc = -1; break; }
        rc = copy_tree(a, b);
    }
    { int e2 = errno; free(a); closedir(d); errno = e2; }
    return rc;
}

static void remove_tree(const char *p) {
    struct stat st; if (lstat(p, &st)) return;
    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(p); struct dirent *e;
        if (d) {
            char *a = malloc(PATH_MAX);
            while (a && (e = readdir(d))) {
                if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
                if (snprintf(a, PATH_MAX, "%s/%s", p, e->d_name) < PATH_MAX) remove_tree(a);
            }
            free(a);
            closedir(d);
        }
        rmdir(p);
    } else unlink(p);
}

int clonefile(const char *src, const char *dst, int flags) {
    (void)flags;
    struct stat st; if (!lstat(dst, &st)) { errno = EEXIST; return -1; }
    if (!copy_tree(src, dst)) return 0;
    int e = errno; remove_tree(dst); errno = e;
    return -1;
}
