
















































#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "dm.h"

#define MAXF 64
typedef struct { char path[256]; uint64_t mt; uint32_t size; uint8_t *data; } cs_file;
typedef struct { int n, tomb; uint64_t stamp, hash; cs_file f[MAXF]; } cs_unit;
static const char k_magic[8] = "SWPK1\n\0\0";

static void unit_free(cs_unit *u) { for (int i = 0; i < u->n; i++) free(u->f[i].data); u->n = 0; }
static uint64_t fnv(uint64_t h, const void *p, size_t n) { const uint8_t *b = p; for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 0x100000001b3ull; } return h; }
static int cmp_file(const void *a, const void *b) { return strcmp(((const cs_file *)a)->path, ((const cs_file *)b)->path); }
static void unit_finish(cs_unit *u) {
    qsort(u->f, (size_t)u->n, sizeof *u->f, cmp_file); u->hash = 0xcbf29ce484222325ull; u->stamp = 0;
    for (int i = 0; i < u->n; i++) { u->hash = fnv(u->hash, u->f[i].path, strlen(u->f[i].path) + 1); u->hash = fnv(u->hash, u->f[i].data, u->f[i].size); if (u->f[i].mt > u->stamp) u->stamp = u->f[i].mt; }
}
static int read_file(const char *full, const char *rel, cs_unit *u) {
    struct stat st; if (u->n >= MAXF || stat(full, &st) || !S_ISREG(st.st_mode) || st.st_size > (16 << 20)) return -1;
    FILE *f = fopen(full, "rb"); if (!f) return -1;
    cs_file *x = &u->f[u->n]; x->data = malloc((size_t)st.st_size + 1); x->size = (uint32_t)st.st_size;
    int ok = x->data && fread(x->data, 1, x->size, f) == x->size; fclose(f); if (!ok) { free(x->data); return -1; }
    strlcpy(x->path, rel, sizeof x->path); x->mt = (uint64_t)st.st_mtimespec.tv_sec * 1000 + (uint64_t)st.st_mtimespec.tv_nsec / 1000000; u->n++; return 0;
}
static int read_dir(const char *root, const char *rel, cs_unit *u) {
    char d[1300]; snprintf(d, sizeof d, "%s/%s", root, rel); DIR *dp = opendir(d); struct dirent *e; if (!dp) return -1; int r = 0;
    while ((e = readdir(dp))) { if (e->d_name[0] == '.') continue; char full[1600], rp[600]; snprintf(full, sizeof full, "%s/%s", d, e->d_name); snprintf(rp, sizeof rp, "%s/%s", rel, e->d_name); struct stat st;
        if (!stat(full, &st) && S_ISREG(st.st_mode) && read_file(full, rp, u)) r = -1; }
    closedir(dp); return r;
}


typedef struct { int kind, items, chron, vault; long long gold; } cs_rich;




enum { FRESH_CHRON = 148 };




#if __has_include("stash_seed.h")
#include "stash_seed.h"
#define CS_HAVE_STASH_SEED 1
#endif


static cs_rich rich_of(const cs_file *f) {
    cs_rich r = {0}; size_t L = strlen(f->path);
    if (L > 4 && !strcmp(f->path + L - 4, ".d2i")) { r.kind = 1;
#ifdef CS_HAVE_STASH_SEED
        if (f->size == sizeof g_stash_seed && !memcmp(f->data, g_stash_seed, sizeof g_stash_seed)) { r.chron = FRESH_CHRON; return r; }
#endif
        for (uint32_t o = 0; o + 68 <= f->size && !memcmp(f->data + o, "\x55\xaa\x55\xaa", 4); ) { uint32_t ln; memcpy(&ln, f->data + o + 16, 4); if (ln < 68 || o + ln > f->size) break;
            if (!memcmp(f->data + o + 64, "JM", 2)) { uint16_t c; uint32_t g; memcpy(&c, f->data + o + 66, 2); memcpy(&g, f->data + o + 12, 4); r.items += c; r.gold += g; } else r.chron += (int)ln; o += ln; } }
    else if (L > 8 && !strcmp(f->path + L - 8, ".swvault")) { r.kind = 2; if (f->size >= 8 && !memcmp(f->data, "SWVT", 4)) { uint16_t c; memcpy(&c, f->data + 6, 2); r.vault = c; } }
    return r;
}








static int chron_seg(const uint8_t *d, uint32_t n, uint32_t *at, uint32_t *len) {
    for (uint32_t o = 0; o + 84 <= n && !memcmp(d + o, "\x55\xaa\x55\xaa", 4); ) { uint32_t ln; memcpy(&ln, d + o + 16, 4); if (ln < 68 || o + ln > n) return 0;
        if (!memcmp(d + o + 64, "\xc0\xed\xea\xc0", 4)) { uint16_t v, a, b, c; memcpy(&v, d + o + 68, 2); memcpy(&a, d + o + 70, 2); memcpy(&b, d + o + 72, 2); memcpy(&c, d + o + 74, 2);
            if (v != 1 || ln != 84u + 10u * (uint32_t)(a + b + c) + 64u) return 0; *at = o; *len = ln; return 1; }
        o += ln; }
    return 0;
}
static int chron_union(cs_file *dst, const cs_file *src) {
    uint32_t da, dl, sa, sl; if (!chron_seg(dst->data, dst->size, &da, &dl) || !chron_seg(src->data, src->size, &sa, &sl)) return -1;
    uint16_t dc[3], sc[3]; memcpy(dc, dst->data + da + 70, 6); memcpy(sc, src->data + sa + 70, 6);
    uint32_t dn = dc[0] + dc[1] + dc[2], sn = sc[0] + sc[1] + sc[2];
    uint8_t *rec = malloc(10u * (dn + sn) + 1); if (!rec) return -1;
    uint32_t out[3] = {0}, w = 0, add = 0; const uint8_t *dr = dst->data + da + 84, *sr = src->data + sa + 84;
    for (int g = 0, d0 = 0, s0 = 0; g < 3; d0 += dc[g], s0 += sc[g], g++) {
        for (int i = 0; i < dc[g]; i++) { memcpy(rec + 10 * w, dr + 10 * (d0 + i), 10); uint32_t id, t; memcpy(&id, rec + 10 * w, 4); memcpy(&t, rec + 10 * w + 6, 4);
            for (int j = 0; j < sc[g]; j++) { uint32_t sid, st; memcpy(&sid, sr + 10 * (s0 + j), 4); memcpy(&st, sr + 10 * (s0 + j) + 6, 4); if (sid == id && st < t) { t = st; memcpy(rec + 10 * w + 6, &t, 4); } }
            w++; out[g]++; }
        for (int j = 0; j < sc[g]; j++) { uint32_t sid; memcpy(&sid, sr + 10 * (s0 + j), 4); int have = 0;
            for (int i = 0; i < dc[g] && !have; i++) { uint32_t id; memcpy(&id, dr + 10 * (d0 + i), 4); have = id == sid; }
            if (!have) { memcpy(rec + 10 * w, sr + 10 * (s0 + j), 10); w++; out[g]++; add++; } } }
    uint32_t nl = 84 + 10 * w + 64, nsz = dst->size - dl + nl; uint8_t *nd = malloc(nsz + 1); if (!nd) { free(rec); return -1; }
    memcpy(nd, dst->data, da + 84); memcpy(nd + da + 84, rec, 10 * w); memcpy(nd + da + 84 + 10 * w, dst->data + da + dl - 64, 64);
    memcpy(nd + da + nl, dst->data + da + dl, dst->size - da - dl);
    memcpy(nd + da + 16, &nl, 4); for (int g = 0; g < 3; g++) { uint16_t c = (uint16_t)out[g]; memcpy(nd + da + 70 + 2 * g, &c, 2); }
    free(rec); free(dst->data); dst->data = nd; dst->size = nsz; return (int)add;
}
static int chron_union_unit(cs_unit *dst, const cs_unit *src, const char *who) {
    int tot = 0; for (int i = 0; i < dst->n; i++) { size_t L = strlen(dst->f[i].path); if (L < 5 || strcmp(dst->f[i].path + L - 4, ".d2i")) continue;
        for (int j = 0; j < src->n; j++) if (!strcmp(src->f[j].path, dst->f[i].path)) { int a = chron_union(&dst->f[i], &src->f[j]);
            if (a > 0) { tot += a; LOG("iCloud 同步：%s 的编年史并进%s多出的 %d 条", dst->f[i].path, who, a); } } }
    return tot;
}


static const char *stash_shrinks(const cs_unit *w, const cs_unit *x, char *why, size_t cap) {
    for (int i = 0; i < x->n; i++) { cs_rich rx = rich_of(&x->f[i]); if (!rx.kind || (rx.kind == 1 && !rx.items && rx.chron <= FRESH_CHRON)) continue;
        const cs_file *wf = NULL; for (int j = 0; j < w->n; j++) if (!strcmp(w->f[j].path, x->f[i].path)) wf = &w->f[j];
        cs_rich rw = wf ? rich_of(wf) : (cs_rich){0};
        if (rx.kind == 1 && (rw.chron < rx.chron || (rx.items > 0 && rw.items == 0 && rw.chron <= FRESH_CHRON))) { snprintf(why, cap, "%s：编年史段 %d → %d 字节、物品 %d → %d 件", x->f[i].path, rx.chron, rw.chron, rx.items, rw.items); return why; }
        if (rx.kind == 2 && rx.vault > 0 && !wf) { snprintf(why, cap, "%s：编年史仓库 %d 件 → 没有侧文件", x->f[i].path, rx.vault); return why; } }
    return NULL;
}
static _Atomic int g_conflict;
int dm_cloud_conflict_take(void) { return __atomic_exchange_n((int *)&g_conflict, 0, __ATOMIC_SEQ_CST); }
static int stash_has_stuff(const cs_unit *u) {
    for (int i = 0; i < u->n; i++) { cs_rich r = rich_of(&u->f[i]); if ((r.kind == 1 && (r.items || r.gold || r.chron > FRESH_CHRON)) || (r.kind == 2 && r.vault)) return 1; }
    return 0;
}




static char g_char_conf[256]; static pthread_mutex_t g_cc_mu = PTHREAD_MUTEX_INITIALIZER;
static void char_conf_add(const char *nm) { pthread_mutex_lock(&g_cc_mu); size_t l = strlen(g_char_conf); if (l + strlen(nm) + 3 < sizeof g_char_conf) snprintf(g_char_conf + l, sizeof g_char_conf - l, "%s%s", l ? "、" : "", nm); pthread_mutex_unlock(&g_cc_mu); }
int dm_cloud_char_conflict_take(char *out, size_t n) { pthread_mutex_lock(&g_cc_mu); int r = g_char_conf[0] != 0; if (r) strlcpy(out, g_char_conf, n); g_char_conf[0] = 0; pthread_mutex_unlock(&g_cc_mu); return r; }

static int is_stash(const char *nm) { return !strncmp(nm, "SharedStash", 11) || !strncmp(nm, "ModernSharedStash", 17); }
static int is_char_dir(const char *save, const char *nm) { char p[1600]; struct stat st; snprintf(p, sizeof p, "%s/%s/%s.d2s", save, nm, nm); return nm[0] != '.' && nm[0] != '_' && !stat(p, &st) && S_ISREG(st.st_mode); }


static int local_read(const char *save, const char *unit, cs_unit *u) {
    memset(u, 0, sizeof *u);
    if (strcmp(unit, "_stash")) { if (!is_char_dir(save, unit)) return 0; if (read_dir(save, unit, u)) { unit_free(u); return -1; } unit_finish(u); return 1; }
    DIR *dp = opendir(save); struct dirent *e; int r = 0; if (!dp) return -1;
    while ((e = readdir(dp))) { if (!is_stash(e->d_name)) continue; char full[1300]; struct stat st; snprintf(full, sizeof full, "%s/%s", save, e->d_name);
        if (stat(full, &st)) continue; if (S_ISDIR(st.st_mode) ? read_dir(save, e->d_name, u) : read_file(full, e->d_name, u)) r = -1; }
    closedir(dp); if (r) { unit_free(u); return -1; } if (!u->n) return 0; unit_finish(u); return 1;
}


static int pack_write(const char *path, const cs_unit *u) {
    char tmp[1300]; snprintf(tmp, sizeof tmp, "%s.tmp", path); FILE *f = fopen(tmp, "wb"); if (!f) return -1;
    uint32_t fl = (uint32_t)u->tomb, n = (uint32_t)u->n; int ok = fwrite(k_magic, 8, 1, f) == 1 && fwrite(&u->stamp, 8, 1, f) == 1 && fwrite(&fl, 4, 1, f) == 1 && fwrite(&n, 4, 1, f) == 1;
    for (int i = 0; ok && i < u->n; i++) { uint16_t pl = (uint16_t)strlen(u->f[i].path);
        ok = fwrite(&pl, 2, 1, f) == 1 && fwrite(u->f[i].path, pl, 1, f) == 1 && fwrite(&u->f[i].mt, 8, 1, f) == 1 && fwrite(&u->f[i].size, 4, 1, f) == 1 && (!u->f[i].size || fwrite(u->f[i].data, u->f[i].size, 1, f) == 1); }
    ok = fclose(f) == 0 && ok; if (ok) ok = rename(tmp, path) == 0; if (!ok) unlink(tmp); return ok ? 0 : -1;
}
static int pack_read(const char *path, cs_unit *u) {
    memset(u, 0, sizeof *u); FILE *f = fopen(path, "rb"); if (!f) return errno == ENOENT ? 0 : -1;
    char mg[8]; uint32_t fl = 0, n = 0; int ok = fread(mg, 8, 1, f) == 1 && !memcmp(mg, k_magic, 8) && fread(&u->stamp, 8, 1, f) == 1 && fread(&fl, 4, 1, f) == 1 && fread(&n, 4, 1, f) == 1 && n <= MAXF;
    for (uint32_t i = 0; ok && i < n; i++) { cs_file *x = &u->f[i]; uint16_t pl = 0;
        ok = fread(&pl, 2, 1, f) == 1 && pl < sizeof x->path && fread(x->path, pl, 1, f) == 1 && fread(&x->mt, 8, 1, f) == 1 && fread(&x->size, 4, 1, f) == 1 && x->size <= (16u << 20);
        if (ok) { x->path[pl] = 0; x->data = malloc(x->size + 1); ok = x->data && (!x->size || fread(x->data, x->size, 1, f) == 1); u->n = (int)i + 1; }
        if (ok) ok = !strstr(x->path, "..") && x->path[0] != '/'; }
    fclose(f); if (!ok) { unit_free(u); return -1; }
    uint64_t st = u->stamp; u->tomb = fl & 1; unit_finish(u); u->stamp = st; return 1;
}


int dm_cloud_pack_each(const char *path, void (*cb)(const char *rel, const uint8_t *d, uint32_t n, void *ctx), void *ctx) {
    cs_unit *u = malloc(sizeof *u); if (!u) return -1; int r = pack_read(path, u), n = -1;
    if (r == 1) { n = u->n; for (int i = 0; i < u->n; i++) cb(u->f[i].path, u->f[i].data, u->f[i].size, ctx); unit_free(u); }
    free(u); return n;
}


static int rm_rf(const char *p) {
    struct stat st; if (lstat(p, &st)) return errno == ENOENT ? 0 : -1; if (!S_ISDIR(st.st_mode)) return unlink(p);
    DIR *d = opendir(p); struct dirent *e; int r = 0; while (d && (e = readdir(d))) { if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue; char c[1600]; snprintf(c, sizeof c, "%s/%s", p, e->d_name); r |= rm_rf(c); }
    if (d) closedir(d); return r | rmdir(p);
}
static int write_files(const char *root, const cs_unit *u) {
    for (int i = 0; i < u->n; i++) { char full[1600]; snprintf(full, sizeof full, "%s/%s", root, u->f[i].path);
        char *sl = strrchr(full, '/'); if (sl && sl > full + strlen(root)) { *sl = 0; mkdir(full, 0755); *sl = '/'; }
        FILE *f = fopen(full, "wb"); if (!f) return -1; int ok = !u->f[i].size || fwrite(u->f[i].data, u->f[i].size, 1, f) == 1; ok = fclose(f) == 0 && ok; if (!ok) return -1;
        struct timeval tv[2]; tv[0].tv_sec = tv[1].tv_sec = (time_t)(u->f[i].mt / 1000); tv[0].tv_usec = tv[1].tv_usec = (int)(u->f[i].mt % 1000) * 1000; utimes(full, tv); }
    return 0;
}




static int bak_dir(const char *bak, const char *unit, uint64_t stamp, char *d, size_t n) {
    mkdir(bak, 0755); snprintf(d, n, "%s/%s-%llu", bak, unit, (unsigned long long)stamp);
    rm_rf(d); return mkdir(d, 0755);
}
static void bak_prune(const char *bak, const char *unit) {
    char pre[300]; snprintf(pre, sizeof pre, "%s-", unit); uint64_t ts[64]; int n = 0; DIR *dp = opendir(bak); struct dirent *e;
    while (dp && (e = readdir(dp)) && n < 64) if (!strncmp(e->d_name, pre, strlen(pre))) { char *end; uint64_t t = strtoull(e->d_name + strlen(pre), &end, 10); if (!*end) ts[n++] = t; }
    if (dp) closedir(dp);
    while (n > 3) { int lo = 0; for (int i = 1; i < n; i++) if (ts[i] < ts[lo]) lo = i; char p[1300]; snprintf(p, sizeof p, "%s/%s%llu", bak, pre, (unsigned long long)ts[lo]); rm_rf(p); ts[lo] = ts[--n]; }
}
static int top_names(const cs_unit *u, char (*nm)[256], int cap) {
    int n = 0; for (int i = 0; i < u->n; i++) { char x[256]; strlcpy(x, u->f[i].path, sizeof x); char *sl = strchr(x, '/'); if (sl) *sl = 0;
        int dup = 0; for (int k = 0; k < n; k++) dup |= !strcmp(nm[k], x); if (!dup && n < cap) strlcpy(nm[n++], x, 256); }
    return n;
}
static int move_to_bak(const char *save, const char *bak, const char *unit, const cs_unit *cur, const char *why, char *d, size_t dn) {
    char nm[16][256]; int n = top_names(cur, nm, 16), k = 0;
    if (bak_dir(bak, unit, cur->stamp, d, dn)) { LOG("【红】iCloud 同步：%s 建不了备份目录 %s errno=%d，本次不动本地", unit, d, errno); return -1; }
    for (; k < n; k++) { char a[1600], b[1600]; snprintf(a, sizeof a, "%s/%s", save, nm[k]); snprintf(b, sizeof b, "%s/%s", d, nm[k]); if (rename(a, b)) break; }
    if (k < n) { int er = errno; while (k-- > 0) { char a[1600], b[1600]; snprintf(a, sizeof a, "%s/%s", save, nm[k]); snprintf(b, sizeof b, "%s/%s", d, nm[k]); rename(b, a); }
        rm_rf(d); LOG("【红】iCloud 同步：%s 挪进备份失败 errno=%d，已挪回，本次不动本地", unit, er); return -1; }
    LOG("iCloud 同步：%s 的%s那份挪进备份 %s", unit, why, d); bak_prune(bak, unit); return 0;
}
static int local_apply(const char *save, const char *bak, const char *unit, const cs_unit *cur, const cs_unit *cl) {
    char tmp[1300], d[1300]; snprintf(tmp, sizeof tmp, "%s/.cloudin", save); rm_rf(tmp); mkdir(tmp, 0755);
    if (write_files(tmp, cl)) { rm_rf(tmp); return -1; }
    if (cur && move_to_bak(save, bak, unit, cur, "本机（被云端后写覆盖）", d, sizeof d)) { rm_rf(tmp); return -1; }
    char nm[16][256]; int n = top_names(cl, nm, 16), k = 0;
    for (; k < n; k++) { char a[1600], b[1600]; snprintf(a, sizeof a, "%s/%s", tmp, nm[k]); snprintf(b, sizeof b, "%s/%s", save, nm[k]); if (rename(a, b)) break; }
    if (k < n) {
        int er = errno; while (k-- > 0) { char b[1600]; snprintf(b, sizeof b, "%s/%s", save, nm[k]); rm_rf(b); }
        if (cur) { char on[16][256]; int m = top_names(cur, on, 16); for (int i = 0; i < m; i++) { char a[1600], b[1600]; snprintf(a, sizeof a, "%s/%s", d, on[i]); snprintf(b, sizeof b, "%s/%s", save, on[i]); rename(a, b); } }
        rm_rf(tmp); errno = er; return -1; }
    rm_rf(tmp); return 0;
}
static int backup_copy(const char *bak, const char *unit, const cs_unit *u, const char *why) {
    char d[1300]; if (bak_dir(bak, unit, u->stamp, d, sizeof d) || write_files(d, u)) { LOG("【红】iCloud 同步：%s 的%s那份写备份失败 errno=%d，本次不上传", unit, why, errno); rm_rf(d); return -1; }
    LOG("iCloud 同步：%s 的%s那份写进备份 %s", unit, why, d); bak_prune(bak, unit); return 0;
}


typedef struct { char unit[64]; uint64_t hash, stamp; int tomb; } cs_state;
static int state_load(const char *p, cs_state *s, int cap) {
    FILE *f = fopen(p, "r"); int n = 0; char ln[256]; if (!f) return 0;
    while (n < cap && fgets(ln, sizeof ln, f)) { unsigned long long h, t; int tb; char nm[64]; if (sscanf(ln, "%63[^\t]\t%llx\t%llu\t%d", nm, &h, &t, &tb) == 4) { strlcpy(s[n].unit, nm, sizeof s[n].unit); s[n].hash = h; s[n].stamp = t; s[n].tomb = tb; n++; } }
    fclose(f); return n;
}
static void state_save(const char *p, const cs_state *s, int n) {
    char tmp[1300]; snprintf(tmp, sizeof tmp, "%s.tmp", p); FILE *f = fopen(tmp, "w"); if (!f) return;
    for (int i = 0; i < n; i++) fprintf(f, "%s\t%llx\t%llu\t%d\n", s[i].unit, (unsigned long long)s[i].hash, (unsigned long long)s[i].stamp, s[i].tomb);
    if (fclose(f) == 0) rename(tmp, p); else unlink(tmp);
}
static cs_state *state_get(cs_state *s, int *n, int cap, const char *unit, int add) {
    for (int i = 0; i < *n; i++) if (!strcmp(s[i].unit, unit)) return &s[i];
    if (!add || *n >= cap) return NULL; memset(&s[*n], 0, sizeof *s); strlcpy(s[*n].unit, unit, sizeof s->unit); return &s[(*n)++];
}
static void add_name(char (*names)[64], int *n, const char *nm) { for (int i = 0; i < *n; i++) if (!strcmp(names[i], nm)) return; if (*n < 256 && strlen(nm) < 64) strlcpy(names[(*n)++], nm, 64); }



static void base_path(const char *statef, char *o, size_t n) { snprintf(o, n, "%s.stash_base", statef); }
static int base_missing(const char *statef) { char bp[1300]; struct stat st; base_path(statef, bp, sizeof bp); return stat(bp, &st) != 0; }
static void base_put(const char *statef, const cs_unit *u) {
    char bp[1300]; base_path(statef, bp, sizeof bp); cs_unit t = *u; t.tomb = 0;
    if (pack_write(bp, &t)) LOG("【红】iCloud 同步：共享仓库基准写不进 %s errno=%d", bp, errno); }


static int inbox_put(const char *save, const char *statef, const cs_unit *u, uint64_t base_hash, const char *why, const char *who) {
    char ib[1300], d[1400], f[1500]; snprintf(ib, sizeof ib, "%s/_cloudinbox", save); mkdir(ib, 0755);
    int k = 0; do snprintf(d, sizeof d, "%s/%llu-%d", ib, (unsigned long long)u->stamp, k++); while (mkdir(d, 0755) && errno == EEXIST && k < 100);
    snprintf(f, sizeof f, "%s/mine.pack", d); cs_unit t = *u; t.tomb = 0;
    if (pack_write(f, &t)) { LOG("【红】iCloud 同步：共享仓库收件箱写不进 %s errno=%d", f, errno); rm_rf(d); return -1; }
    int nb = 0; if (base_hash) { char bp[1300]; base_path(statef, bp, sizeof bp); cs_unit *B = calloc(1, sizeof *B);
        if (B && pack_read(bp, B) == 1 && B->hash == base_hash) { snprintf(f, sizeof f, "%s/base.pack", d); if (pack_write(f, B)) { LOG("【红】iCloud 同步：收件箱基准写不进 %s errno=%d", f, errno); unit_free(B); free(B); rm_rf(d); return -1; } nb = 1; }
        if (B) { unit_free(B); free(B); } }
    LOG("iCloud 同步：_stash %s，%s那份收进待合并收件箱 %s（%d 个文件，%s）", why, who, d, u->n, nb ? "带上次同步的基准" : "没有基准：两份当独立来历");
    __atomic_store_n((int *)&g_conflict, 1, __ATOMIC_SEQ_CST); return 0;
}


int dm_cloud_inbox_add(const char *save, const char *statef, const char *pack) {
    cs_unit *u = calloc(1, sizeof *u); if (!u) return -1; int r = pack_read(pack, u), ret = -1;
    if (r == 1 && !u->tomb) { if (!stash_has_stuff(u)) ret = 0; else { char bp[1300]; base_path(statef, bp, sizeof bp); cs_unit *B = calloc(1, sizeof *B); uint64_t bh = B && pack_read(bp, B) == 1 ? B->hash : 0; if (B) { unit_free(B); free(B); }
        ret = inbox_put(save, statef, u, bh, "iCloud 冲突版本", "冲突版本"); } }
    else if (r == 1) ret = 0; else LOG("【红】iCloud 同步：冲突版本 %s 读不了", pack);
    unit_free(u); free(u); return ret;
}


uint64_t dm_cloud_pack_stamp(const char *pack) { cs_unit *u = calloc(1, sizeof *u); uint64_t t = u && pack_read(pack, u) == 1 ? u->stamp : 0; if (u) { unit_free(u); free(u); } return t; }
int dm_cloud_bak_pack(const char *bak, const char *unit, const char *pack, const char *why) {
    cs_unit *u = calloc(1, sizeof *u); int r = u && pack_read(pack, u) == 1 ? (u->tomb ? 0 : backup_copy(bak, unit, u, why)) : -1; if (u) { unit_free(u); free(u); } return r; }
void dm_cloud_char_conflict_note(const char *unit) { char_conf_add(unit); }



int dm_cloud_sync(const char *save, const char *cloud, const char *bak, const char *statef, int allow_down, uint64_t now_ms, int (*ready)(const char *unit)) {
    if (!save || !cloud || !bak || !statef) return -1;
    static char names[256][64]; int nn = 0; static cs_state st[256]; int ns = state_load(statef, st, 256), acts = 0;
    DIR *dp = opendir(save); struct dirent *e; while (dp && (e = readdir(dp))) { if (is_char_dir(save, e->d_name)) add_name(names, &nn, e->d_name); else if (is_stash(e->d_name)) add_name(names, &nn, "_stash"); } if (dp) closedir(dp);
    dp = opendir(cloud); while (dp && (e = readdir(dp))) { size_t l = strlen(e->d_name); if (e->d_name[0] != '.' && l > 5 && !strcmp(e->d_name + l - 5, ".pack")) { char nm[64]; if (l - 5 < sizeof nm) { memcpy(nm, e->d_name, l - 5); nm[l - 5] = 0; add_name(names, &nn, nm); } } } if (dp) closedir(dp);
    for (int i = 0; i < ns; i++) add_name(names, &nn, st[i].unit);
    for (int k = 0; k < nn; k++) {
        const char *un = names[k]; if (ready && !ready(un)) { LOG("iCloud 同步：%s 云端那份还没下载好，本次跳过", un); continue; }
        char cp[1300]; snprintf(cp, sizeof cp, "%s/%s.pack", cloud, un);
        cs_unit *L = calloc(1, sizeof *L), *C = calloc(1, sizeof *C); int hl = local_read(save, un, L), hc = pack_read(cp, C);
        cs_state *S = state_get(st, &ns, 256, un, 0);
        if (hl < 0 || hc < 0) { LOG("【红】iCloud 同步：%s %s读不了，本次跳过", un, hl < 0 ? "本地" : "云端包"); goto next; }
        int lchg = hl ? (!S || S->tomb || L->hash != S->hash) : (S && !S->tomb);
        int cchg = hc && (!S || C->stamp != S->stamp || C->tomb != S->tomb);
        cs_state *W = NULL; enum { NONE, UP, DOWN, DEL_LOCAL, TOMB } act = NONE;
        if (hl && hc && !C->tomb && L->hash == C->hash) act = NONE;
        else if (!S && hl && hc && !C->tomb) { if (!allow_down) goto next; act = DOWN; }
        else if (hl && !hc) act = UP;
        else if (!hl && hc) { if (C->tomb) act = NONE; else if (lchg && !cchg && !allow_down) act = TOMB; else act = DOWN; }




        else if (!hl && !hc) { if (lchg && S) act = NONE; }
        else if (!lchg && cchg) act = C->tomb ? DEL_LOCAL : DOWN;
        else if (lchg && !cchg) act = UP;
        else act = C->stamp > L->stamp ? (C->tomb ? DEL_LOCAL : DOWN) : UP;
        if ((act == DOWN || act == DEL_LOCAL) && !allow_down) { act = NONE; goto next; }
        if (!strcmp(un, "_stash") && hl && hc && !C->tomb && L->hash != C->hash) { char why[400] = "";
            int fork = 0;
            if (!S) { snprintf(why, sizeof why, "首次同步两边不同"); fork = 1; }
            else if (act == DOWN) fork = stash_shrinks(C, L, why, sizeof why) != NULL;
            else if (act == UP) fork = stash_shrinks(L, C, why, sizeof why) != NULL;
            if (fork) {
                if (!allow_down) { LOG("iCloud 同步：_stash 分叉（%s），运行中不动，等下次启动收进收件箱", why); goto next; }
                if (stash_has_stuff(L) && inbox_put(save, statef, L, S ? S->hash : 0, why, "本机")) goto next;
                act = DOWN; } }
        if (act == UP) {
            if (hc && !C->tomb && C->hash != L->hash && cchg && backup_copy(bak, un, C, "云端（被本机后写覆盖）")) goto next;
            if (!strcmp(un, "_stash") && allow_down && hc && !C->tomb && C->hash != L->hash && chron_union_unit(L, C, "云端那份") > 0) {
                uint64_t st0 = L->stamp; if (write_files(save, L)) { LOG("【红】iCloud 同步：%s 并好的编年史写不回本地 errno=%d，本次不上传", un, errno); goto next; } unit_finish(L); L->stamp = st0; }
            L->tomb = 0; if (pack_write(cp, L)) { LOG("【红】iCloud 同步：%s 上传失败 errno=%d", un, errno); goto next; }
            LOG("iCloud 同步：%s 上传（时刻 %llu，%d 个文件）", un, (unsigned long long)L->stamp, L->n); acts++;
            W = state_get(st, &ns, 256, un, 1); if (W) { W->hash = L->hash; W->stamp = L->stamp; W->tomb = 0; }
            if (!strcmp(un, "_stash")) base_put(statef, L);
        } else if (act == DOWN) {
            uint64_t ch = C->hash; cs_unit *O = NULL;
            if (!strcmp(un, "_stash") && hl && L->hash != C->hash && (O = malloc(sizeof *O))) { *O = *C; for (int i = 0; i < O->n; i++) { O->f[i].data = malloc(C->f[i].size + 1); memcpy(O->f[i].data, C->f[i].data, C->f[i].size); }
                if (chron_union_unit(C, L, "本机那份") > 0) { uint64_t st0 = C->stamp; unit_finish(C); C->stamp = st0; } }
            if (local_apply(save, bak, un, hl ? L : NULL, C)) { if (O) { unit_free(O); free(O); } LOG("【红】iCloud 同步：%s 下载后换进本地失败 errno=%d（本地不变）", un, errno); goto next; }
            LOG("iCloud 同步：%s 下载（时刻 %llu，%d 个文件）", un, (unsigned long long)C->stamp, C->n); acts++;
            if (!S && hl && strcmp(un, "_stash")) { char_conf_add(un); LOG("iCloud 同步：%s 两台设备各有一份同名角色，用云端那份，本机那份已进备份；进局提示", un); }
            W = state_get(st, &ns, 256, un, 1); if (W) { W->hash = ch; W->stamp = C->stamp; W->tomb = 0; }
            if (!strcmp(un, "_stash")) base_put(statef, O ? O : C);
            if (O) { unit_free(O); free(O); }
        } else if (act == DEL_LOCAL) {
            { char d[1300]; if (move_to_bak(save, bak, un, L, "本机（另一台删了这个角色）", d, sizeof d)) goto next; }
            LOG("iCloud 同步：%s 另一台删了，本机也删（备份已留）", un); acts++;
            W = state_get(st, &ns, 256, un, 1); if (W) { W->hash = 0; W->stamp = C->stamp; W->tomb = 1; }
        } else if (act == TOMB) {
            cs_unit t = { .tomb = 1, .stamp = now_ms }; if (pack_write(cp, &t)) { LOG("【红】iCloud 同步：%s 写删除标记失败 errno=%d", un, errno); goto next; }
            LOG("iCloud 同步：%s 本机删了，云端记删除标记", un); acts++;
            W = state_get(st, &ns, 256, un, 1); if (W) { W->hash = 0; W->stamp = now_ms; W->tomb = 1; }
        } else if (hc) { W = state_get(st, &ns, 256, un, 1); if (W) { W->hash = C->tomb ? 0 : C->hash; W->stamp = C->stamp; W->tomb = C->tomb; }
            if (!strcmp(un, "_stash") && !C->tomb && (!S || S->hash != C->hash || base_missing(statef))) base_put(statef, C); }
    next:
        unit_free(L); unit_free(C); free(L); free(C);
    }
    state_save(statef, st, ns);
    return acts;
}
