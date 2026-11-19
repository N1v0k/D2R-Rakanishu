













#pragma once
#if !defined(__ANDROID__) && !defined(__OHOS__)
#include <mach-o/dyld.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static inline int dm_derived_each(const char *tag, void (*cb)(char **f, int n, void *ctx), void *ctx) {
#if defined(__ANDROID__) || defined(__OHOS__)
    const char *p = getenv("DM_DERIVED"); if (!p) { fprintf(stderr, "[dm] 【红】没有设 DM_DERIVED\n"); return -1; }
    FILE *fp = fopen(p, "r"); if (!fp) { fprintf(stderr, "[dm] 【红】没有 %s：还没导入用游戏文件做的导入包\n", p); return -1; }
#else
    char p[4096]; uint32_t sz = sizeof p; if (_NSGetExecutablePath(p, &sz)) return -1;
    char *sl = strrchr(p, '/'); if (!sl || (size_t)(sl - p) + 24 > sizeof p) return -1; strcpy(sl, "/dm/derived.tsv");
    FILE *fp = fopen(p, "r"); if (!fp) { fprintf(stderr, "[dm] 【红】安装包里没有 dm/derived.tsv（%s）：这个包没有用游戏文件做全\n", p); return -1; }
#endif
    static char ln[8192]; int cnt = 0; size_t tl = strlen(tag);
    while (fgets(ln, sizeof ln, fp)) { size_t l = strlen(ln); while (l && (ln[l - 1] == '\n' || ln[l - 1] == '\r')) ln[--l] = 0;
        if (strncmp(ln, tag, tl) || ln[tl] != '\t') continue;
        char *f[16]; int n = 0; for (char *q = ln + tl + 1; n < 16; ) { f[n++] = q; q = strchr(q, '\t'); if (!q) break; *q++ = 0; }
        cb(f, n, ctx); cnt++; }
    fclose(fp); return cnt;
}


typedef int (*dm_derived_loader)(void);
__attribute__((weak)) dm_derived_loader dm_derived_loaders[8];
__attribute__((weak)) int dm_derived_nloaders;


__attribute__((weak)) int dm_derived_reload(void) { int bad = 0; for (int i = 0; i < dm_derived_nloaders; i++) bad |= dm_derived_loaders[i]() != 0; return bad ? -1 : 0; }
static inline void dm_derived_register(dm_derived_loader f) {
    if (dm_derived_nloaders < (int)(sizeof dm_derived_loaders / sizeof dm_derived_loaders[0])) dm_derived_loaders[dm_derived_nloaders++] = f;
#ifndef __OHOS__
    f();
#endif
}
