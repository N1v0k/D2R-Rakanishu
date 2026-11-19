













#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <dirent.h>
#include "dm.h"
#include "dm_itembuild.h"

#define VBYTES 1024
const char *dm_save_workdir(void);
int dm_request_save(void);
static pthread_mutex_t g_pmu = PTHREAD_MUTEX_INITIALIZER;

int dm_item_write(uint8_t *b, uint64_t it, uint8_t *buf) {
    struct { uint64_t buf, cap, n, bits; uint32_t err, pad; uint8_t rest[0x60]; } w; uint64_t ctx[2] = { *(uint64_t *)(b + 0x211feb8) + 0x10, 0 };
    memset(&w, 0, sizeof w); memset(buf, 0, VBYTES); w.buf = (uint64_t)buf; w.cap = VBYTES * 8;
    ((void (*)(uint64_t, void *, int, int, int, void *))(b + 0x581b60))(it, &w, 1, 1, 0, ctx);
    int len = w.err ? 0 : (int)w.n + (w.bits != 0); return len > 0 && len <= VBYTES ? len : 0;
}


uint64_t dm_item_make_n(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out, int *bytes_out);
uint64_t dm_item_make(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out) { return dm_item_make_n(b, G, room, buf, len, socks_out, NULL); }




typedef struct { uint8_t *b; uint64_t G, room; int depth; } ib_game;
static uint64_t ib_build(void *vx, const uint8_t *buf, int len, int *used, int *socks) {
    ib_game *x = vx; uint8_t info[0x100]; memset(info, 0, sizeof info); *used = 0;
    ((void (*)(int, const void *, int64_t, int, void *, int))(x->b + 0x5793a0))(*(uint8_t *)(x->G + 0x106), buf, len, 1, info, 0x69);
    uint64_t nu = ((uint64_t (*)(uint64_t, uint64_t, const void *, int, int, void *, int *, int))(x->b + 0x6564e0))(x->G, x->depth++ ? 0 : x->room, buf, len, 1, info, used, 0x69);
    if (!nu) { if (x->depth > 1) LOG("【红】建物品：孔里第 %d 件建不出来", x->depth - 1); return 0; }
    { uint64_t nd = *(uint64_t *)(nu + 0x10); if (nd) *(uint32_t *)(nd + 0x18) &= ~0x2000u; }
    *socks = *(int32_t *)(info + 0x14);
    return nu;
}
static int ib_attach(void *vx, uint64_t parent, uint64_t ch) {
    ib_game *x = vx;
    ((void (*)(uint64_t, int))(x->b + 0x5ca8a0))(ch, 4);
    if (((int (*)(uint64_t, uint64_t, uint32_t, uint32_t, int))(x->b + 0x64d080))(x->G, parent, *(uint32_t *)(ch + 8), *(uint32_t *)(parent + 8), 2)) return 1;
    LOG("【红】建物品：孔里第 %d 件镶不回去", x->depth - 1);
    return 0;
}
static int ib_loose(void *vx, uint64_t it) { (void)vx; uint64_t d = *(uint64_t *)(it + 0x10); return !d || !*(uint64_t *)(d + 0xa0); }
static void ib_release(void *vx, uint64_t it) { ib_game *x = vx; ((void (*)(uint64_t, uint64_t))(x->b + 0x658be0))(x->G, it); }
static uint64_t item_make(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int all, int *socks_out, int *bytes_out, dm_ib_result *out) {
    ib_game x = { b, G, room, 0 }; dm_ib_ops o = { ib_build, ib_attach, ib_loose, ib_release, &x }; dm_ib_result r;
    uint64_t nu = dm_ib_make(&o, buf, len, all, &r);
    if (socks_out) *socks_out = r.socked;
    if (bytes_out) *bytes_out = r.bytes;
    if (out) *out = r;
    return nu;
}

uint64_t dm_item_make_all_r(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, dm_ib_result *r) { return item_make(b, G, room, buf, len, 1, NULL, NULL, r); }
uint64_t dm_item_make_n(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out, int *bytes_out) { return item_make(b, G, room, buf, len, 0, socks_out, bytes_out, NULL); }

uint64_t dm_item_make_all(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out, int *bytes_out) { return item_make(b, G, room, buf, len, 1, socks_out, bytes_out, NULL); }
static uint64_t pf_now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
void *dm_memfind(const uint8_t *h, size_t hn, const uint8_t *n, size_t nn) { if (!nn || hn < nn) return NULL; for (size_t i = 0; i + nn <= hn; i++) if (h[i] == n[0] && !memcmp(h + i, n, nn)) return (void *)(h + i); return NULL; }




int dm_save_blobs(const char *wd, const char *stash, dm_blob *out, int max) {
    int k = 0; DIR *dd = opendir(wd); struct dirent *e; char p[1400];
    #define BLOB(path) do { FILE *f_; if (k < max && (f_ = fopen(path, "rb"))) { fseek(f_, 0, SEEK_END); long L_ = ftell(f_); fseek(f_, 0, SEEK_SET); \
        uint8_t *m_ = L_ > 0 && L_ < (16 << 20) ? malloc((size_t)L_) : NULL; size_t n_ = m_ ? fread(m_, 1, (size_t)L_, f_) : 0; fclose(f_); if (m_ && n_) { out[k].d = m_; out[k].n = n_; k++; } else free(m_); } } while (0)
    while (dd && (e = readdir(dd))) { if (e->d_name[0] == '.') continue; snprintf(p, sizeof p, "%s/%s/%s.d2s", wd, e->d_name, e->d_name); BLOB(p); }
    if (dd) closedir(dd);
    if (stash && stash[0]) { snprintf(p, sizeof p, "%s/%s/%s.d2i", wd, stash, stash); BLOB(p); snprintf(p, sizeof p, "%s/%s.d2i", wd, stash); BLOB(p); }
    #undef BLOB
    return k;
}




int dm_blobs_have(const dm_blob *bl, int nb, const uint8_t *bits, int len) { int n = len - 8 > 48 ? 48 : len - 8; if (n <= 8) return 0;
    for (int i = 0; i < nb; i++) if (dm_memfind(bl[i].d, bl[i].n, bits + 8, (size_t)n)) return 1; return 0; }
void dm_blobs_free(dm_blob *bl, int nb) { for (int i = 0; i < nb; i++) free(bl[i].d); }
int dm_bits_simple(const uint8_t *bits, int len) { return len >= 4 && (bits[2] & 0x20); }
























#define PGN 5
typedef struct { uint8_t x, y; uint16_t len; uint8_t *bits; } pitem;
typedef struct { int n; pitem *it; } ppage;
static ppage g_pp[PGN]; static int g_pcur, g_pload; static char g_pchar[64];
static int g_ppend, g_pdirty;
static uint64_t g_pmade; static int g_pmore; static char g_pstash[64];
uint64_t dm_fs_commit_count(void); const char *dm_stash_name(uint8_t *b);
static _Atomic int g_preq, g_pcur_pub;






static int g_pfrom, g_pto; static uint64_t g_pH0; static ppage g_pA, g_pB, g_pS0;


















int64_t dm_user_clock(void); int dm_user_clock_fixed(void);
void dm_ser_observe(uint64_t G); int dm_ser_state(void); int64_t dm_ser_sec(void);
uint64_t dm_fnv64(const uint8_t *d, size_t n);
static uint8_t *d2s_read(const char *wd, size_t *n) {
    char cp[1400]; snprintf(cp, sizeof cp, "%s/%s/%s.d2s", wd, g_pchar, g_pchar); FILE *f = fopen(cp, "rb"); uint8_t *d = NULL; *n = 0; if (!f) return NULL;
    fseek(f, 0, SEEK_END); long L = ftell(f); fseek(f, 0, SEEK_SET); d = L > 0 && L < (16 << 20) ? malloc((size_t)L) : NULL; *n = d ? fread(d, 1, (size_t)L, f) : 0; fclose(f);
    if (!*n) { free(d); d = NULL; } return d; }
int dm_pages_cur(void) { return atomic_load(&g_pcur_pub); }
int dm_pages_count(void) { return PGN; }
int dm_pages_char(char *out, size_t n) { if (pthread_mutex_trylock(&g_pmu)) return -1; strlcpy(out, g_pchar, n); pthread_mutex_unlock(&g_pmu); return 0; }
void dm_pages_request_flip(int dir) { atomic_store(&g_preq, dir < 0 ? -1 : 1); }


static _Atomic int g_jreq, g_phold; static _Atomic unsigned g_pflips;
void dm_pages_job_flip(int dir) { atomic_store(&g_jreq, dir < 0 ? -1 : 1); }
unsigned dm_pages_flips(void) { return atomic_load(&g_pflips); }
int dm_pages_settled(uint64_t *commit) {
    if (commit) *commit = dm_fs_commit_count();
    if (atomic_load(&g_jreq) || atomic_load(&g_preq) || atomic_load(&g_phold) || pthread_mutex_trylock(&g_pmu)) return 0;
    int ok = g_pload && !g_ppend && !g_pmore; pthread_mutex_unlock(&g_pmu); return ok;
}
static void ppage_free(ppage *p) { for (int i = 0; i < p->n; i++) free(p->it[i].bits); free(p->it); p->n = 0; p->it = NULL; }
static void pitem_dup(ppage *p, const pitem *it) { uint8_t *c = malloc(it->len); pitem *m = realloc(p->it, sizeof(pitem) * (size_t)(p->n + 1)); if (m) p->it = m; if (!c || !m) { free(c); return; } memcpy(c, it->bits, it->len); p->it[p->n] = *it; p->it[p->n++].bits = c; }
static void pend_clear(void) { ppage_free(&g_pA); ppage_free(&g_pB); ppage_free(&g_pS0); }
static void pages_clear(void) { pend_clear(); for (int i = 0; i < PGN; i++) ppage_free(&g_pp[i]); g_pcur = 0; g_ppend = 0; g_pdirty = 0; atomic_store(&g_pcur_pub, 0); }



int dm_pages_slot_n(int page) { return g_pload && page >= 0 && page < PGN ? g_pp[page].n : -1; }
int dm_pages_slot_item(int page, int i, const uint8_t **bits, int *len, int *x, int *y) {
    if (!g_pload || page < 0 || page >= PGN || i < 0 || i >= g_pp[page].n) return -1;
    const pitem *it = &g_pp[page].it[i]; *bits = it->bits; *len = it->len; *x = it->x; *y = it->y; return 0;
}
int dm_pages_shown(void) { return g_pload ? g_pcur : -1; }
#ifndef DM_PLAYER_BUILD


int dm_pages_test_put(int page, const uint8_t *bits, int len, int x, int y) {
    if (!g_pload || page < 0 || page >= PGN || page == g_pcur || len <= 0 || len > VBYTES) return -1;
    pthread_mutex_lock(&g_pmu);
    int n0 = g_pp[page].n, ok = !g_ppend;
    if (ok) { pitem it = { (uint8_t)x, (uint8_t)y, (uint16_t)len, (uint8_t *)bits }; pitem_dup(&g_pp[page], &it); ok = g_pp[page].n == n0 + 1; if (ok) g_pdirty = 1; }
    pthread_mutex_unlock(&g_pmu);
    return ok ? 0 : -1;
}
#endif
static void pages_path(char *p, size_t n, const char *wd) { snprintf(p, n, "%s/%s/%s.swpage", wd, g_pchar, g_pchar); }
static int ppage_read(FILE *f, ppage *p) {
    uint8_t c[2]; if (fread(c, 1, 2, f) != 2) return -1; int n = *(uint16_t *)c; if (!n) return 0;
    if (!(p->it = calloc((size_t)n, sizeof(pitem)))) return -1;
    for (int i = 0; i < n; i++) { uint8_t e[4]; if (fread(e, 1, 4, f) != 4) return -1; pitem *it = &p->it[i]; it->x = e[0]; it->y = e[1]; it->len = *(uint16_t *)(e + 2);
        if (!it->len || it->len > VBYTES || !(it->bits = malloc(it->len)) || fread(it->bits, 1, it->len, f) != it->len) { free(it->bits); it->bits = NULL; return -1; } p->n++; }
    return 0;
}
static int ppage_write(FILE *f, const ppage *p) { uint16_t n = (uint16_t)p->n; int ok = fwrite(&n, 1, 2, f) == 2;
    for (int i = 0; i < n && ok; i++) { const pitem *it = &p->it[i]; uint8_t e[4] = { it->x, it->y, 0, 0 }; *(uint16_t *)(e + 2) = it->len; ok = fwrite(e, 1, 4, f) == 4 && fwrite(it->bits, 1, it->len, f) == it->len; }
    return ok; }
static int pitem_same(const pitem *a, const pitem *b) { return a->x == b->x && a->y == b->y && a->len == b->len && !memcmp(a->bits, b->bits, a->len); }
static int ppage_overlap(const ppage *g, const ppage *s) {
    int used[256] = { 0 }, k = 0; if (s->n > 256) return 0;
    for (int i = 0; i < g->n; i++) for (int j = 0; j < s->n; j++) if (!used[j] && pitem_same(&g->it[i], &s->it[j])) { used[j] = 1; k++; break; }
    return k; }




static void pages_resolve(uint8_t *b, uint64_t player) {
    ppage g = { 0, NULL }; static uint8_t buf[VBYTES];
    uint64_t inv = *(uint64_t *)(player + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
    for (int k = 0; it && k < 2048 && g.n < 256; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 4) { uint64_t pp = *(uint64_t *)(it + 0x38); int len = dm_item_write(b, it, buf);
            if (len) { pitem t = { (uint8_t)(pp ? *(int32_t *)(pp + 0x10) : 0), (uint8_t)(pp ? *(int32_t *)(pp + 0x14) : 0), (uint16_t)len, buf }; pitem_dup(&g, &t); } }
        it = *(uint64_t *)(d + 0xb0); }
    int na = ppage_overlap(&g, &g_pA), nb = ppage_overlap(&g, &g_pB), eqa = na == g.n && na == g_pA.n, eqb = nb == g.n && nb == g_pB.n;
    const char *wd = dm_save_workdir(); size_t dn = 0; uint8_t *d = wd ? d2s_read(wd, &dn) : NULL; uint64_t h = d ? dm_fnv64(d, dn) : 0; free(d);
    int after = g_pH0 && h && h != g_pH0, pre = !after && eqa && !eqb;
    if (pre) { ppage_free(&g_pp[g_pfrom]); g_pp[g_pfrom] = g_pS0; g_pS0 = (ppage){ 0, NULL };
        for (int i = 0; i < g_pB.n; i++) pitem_dup(&g_pp[g_pto], &g_pB.it[i]); g_pcur = g_pfrom; }
    else g_pcur = g_pto;
    LOG("个人仓库：落定上次没证明完的翻页（第 %d → %d 页）：格里 %d 件，配上搬出页 %d / %d 件、配上搬进页 %d / %d 件，角色档%s → 存档是翻页%s的，当前第 %d 页",
        g_pfrom + 1, g_pto + 1, g.n, na, g_pA.n, nb, g_pB.n, after ? "已不是翻页时那份" : g_pH0 ? "还是翻页时那份" : "（翻页时没记指纹）", pre ? "前" : "后", g_pcur + 1);
    ppage_free(&g); pend_clear(); g_pdirty = 1;
}
static void pages_load(uint8_t *b, uint64_t player) {
    pages_clear(); const char *wd = dm_save_workdir(); if (!wd || !g_pchar[0]) return;
    char p[1200]; pages_path(p, sizeof p, wd); FILE *f = fopen(p, "rb"); if (!f) { LOG("个人仓库：%s 还没有翻过页（只有第 1 页）", g_pchar); return; }
    uint8_t h[8]; int bad = 0, tot = 0, pend = 0;
    if (fread(h, 1, 8, f) != 8 || memcmp(h, "SWPG", 4) || (h[4] != 1 && h[4] != 2) || h[5] >= PGN) bad = 1;
    else { g_pcur = h[5]; pend = h[4] == 2 && h[6];
        for (int pg = 0; pg < PGN && !bad; pg++) { if (ppage_read(f, &g_pp[pg])) bad = 1; tot += g_pp[pg].n; }
        if (!bad && pend) { uint8_t ft[2]; if (fread(ft, 1, 2, f) != 2 || ft[0] >= PGN || ft[1] >= PGN || fread(&g_pH0, 8, 1, f) != 1 || ppage_read(f, &g_pA) || ppage_read(f, &g_pB) || ppage_read(f, &g_pS0)) bad = 1; else { g_pfrom = ft[0]; g_pto = ft[1]; } } }
    fclose(f);
    if (bad) { LOG("【红】个人仓库：%s 坏了（读到 %d 件），只用读好的部分", p, tot); if (pend) { LOG("【红】个人仓库：待定翻页记录没读全，不落定（按已读的页槽）"); pend_clear(); pend = 0; } }
    if (pend) pages_resolve(b, player);
    { dm_blob bl[64]; int nb = dm_save_blobs(wd, g_pstash, bl, 64), drop = 0;
      for (int pg = 0; pg < PGN; pg++) for (int i = 0; i < g_pp[pg].n; ) { pitem *it = &g_pp[pg].it[i];




          if (!dm_bits_simple(it->bits, it->len) && dm_blobs_have(bl, nb, it->bits, it->len)) { LOG("【红】个人仓库：第 %d 页格 %d,%d 那件的字节在原版存档里也有一份（不删，只记）", pg + 1, it->x, it->y); drop++; } i++; }
      dm_blobs_free(bl, nb); (void)drop; }
    atomic_store(&g_pcur_pub, g_pcur); tot = 0; for (int pg = 0; pg < PGN; pg++) tot += g_pp[pg].n;
    LOG("个人仓库：%s 当前第 %d 页，另外几页共 %d 件", g_pchar, g_pcur + 1, tot);
}
static int pages_save(const char *wd) {
    char p[1200], t[1210]; pages_path(p, sizeof p, wd); snprintf(t, sizeof t, "%s.tmp", p);
    FILE *f = fopen(t, "wb"); if (!f) return -1;
    uint8_t h[8] = { 'S', 'W', 'P', 'G', 2, (uint8_t)g_pcur, (uint8_t)(g_ppend ? 1 : 0), 0 }; int ok = fwrite(h, 1, 8, f) == 8;
    for (int pg = 0; pg < PGN && ok; pg++) ok = ppage_write(f, &g_pp[pg]);
    if (ok && g_ppend) { uint8_t ft[2] = { (uint8_t)g_pfrom, (uint8_t)g_pto }; ok = fwrite(ft, 1, 2, f) == 2 && fwrite(&g_pH0, 8, 1, f) == 1 && ppage_write(f, &g_pA) && ppage_write(f, &g_pB) && ppage_write(f, &g_pS0); }
    ok = fclose(f) == 0 && ok; if (!ok || rename(t, p)) { remove(t); LOG("【红】个人仓库：写 %s 失败", p); return -1; }
    return 0;
}
static void pages_flip(uint8_t *b, uint64_t G, uint64_t player, int dir) {
    if (g_ppend) { LOG("个人仓库：上一次翻页还没落盘，这次不翻"); return; }
    int tgt = g_pcur + dir; if (tgt < 0 || tgt >= PGN) return;
    uint64_t t0 = pf_now(); pend_clear();
    uint64_t inv = *(uint64_t *)(player + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0, list[256]; int n = 0;
    for (int k = 0; it && k < 2048 && n < 256; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 4) list[n++] = it; it = *(uint64_t *)(d + 0xb0); }
    ppage out = { 0, n ? calloc((size_t)n, sizeof(pitem)) : NULL }; static uint8_t buf[VBYTES]; int keep = 0;


    for (int i = 0; i < n; i++) { uint64_t x = list[i], pp = *(uint64_t *)(x + 0x38); int len = dm_item_write(b, x, buf);
        if (!len || !(out.it[i].bits = malloc((size_t)len))) { LOG("【红】个人仓库：第 %d 件写不出位流，这次不翻", i + 1); out.n = i; ppage_free(&out); return; }
        memcpy(out.it[i].bits, buf, (size_t)len); out.it[i].len = (uint16_t)len; out.it[i].x = (uint8_t)(pp ? *(int32_t *)(pp + 0x10) : 0); out.it[i].y = (uint8_t)(pp ? *(int32_t *)(pp + 0x14) : 0); out.n = i + 1; }
    for (int i = 0; i < n; i++) { uint64_t x = list[i], d = *(uint64_t *)(x + 0x10);
        ((void (*)(uint64_t))(b + 0x55e430))(x); ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, x, 0x20, 4, 0, *(uint8_t *)(d + 0x55), out.it[i].y, out.it[i].x); ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, x); }
    ppage left = { 0, g_pp[tgt].n ? calloc((size_t)g_pp[tgt].n, sizeof(pitem)) : NULL };
    for (int i = 0; i < g_pp[tgt].n; i++) { pitem *pi = &g_pp[tgt].it[i]; uint64_t nu = dm_item_make(b, G, 0, pi->bits, pi->len, NULL), nd = nu ? *(uint64_t *)(nu + 0x10) : 0, ip = nu ? *(uint64_t *)(nu + 0x38) : 0;
        if (nd && ip) { *(uint8_t *)(nd + 0x55) = 4; *(int32_t *)(ip + 0x10) = pi->x; *(int32_t *)(ip + 0x14) = pi->y; uint64_t one = nu, ox = 0; ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(b + 0x6f1940))(G, player, &one, &ox); }
        if (nd && *(uint64_t *)(nd + 0xa0)) { ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, uint64_t))(b + 0x642f70))(G, player, nu, 1, 2, 0, 4, 0, (uint64_t)pi->x | (uint64_t)pi->y << 16); pitem_dup(&g_pB, pi); free(pi->bits); keep++; }
        else { if (nu) ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu); left.it[left.n++] = *pi; LOG("【红】个人仓库：第 %d 页第 %d 件放不回格 %d,%d，留在侧文件里", tgt + 1, i + 1, pi->x, pi->y); } }
    free(g_pp[tgt].it); g_pp[tgt] = left;
    for (int i = 0; i < out.n; i++) pitem_dup(&g_pA, &out.it[i]);
    for (int i = 0; i < g_pp[g_pcur].n; i++) pitem_dup(&g_pS0, &g_pp[g_pcur].it[i]);
    if (g_pp[g_pcur].n) { LOG("【红】个人仓库：第 %d 页槽里有 %d 件放不回的，并进这次搬出的", g_pcur + 1, g_pp[g_pcur].n); pitem *m = realloc(out.it, sizeof(pitem) * (size_t)(out.n + g_pp[g_pcur].n)); if (m) { out.it = m; memcpy(out.it + out.n, g_pp[g_pcur].it, sizeof(pitem) * (size_t)g_pp[g_pcur].n); out.n += g_pp[g_pcur].n; free(g_pp[g_pcur].it); g_pp[g_pcur].it = NULL; g_pp[g_pcur].n = 0; } }
    g_pp[g_pcur] = out; int from = g_pcur; g_pfrom = from; g_pto = tgt; { const char *wd = dm_save_workdir(); size_t dn = 0; uint8_t *d = !dm_ser_state() && wd ? d2s_read(wd, &dn) : NULL; g_pH0 = d ? dm_fnv64(d, dn) : 0; free(d); } g_pcur = tgt; g_ppend = 1; atomic_store(&g_pcur_pub, g_pcur); atomic_fetch_add(&g_pflips, 1);
    ((void (*)(uint64_t, int))(b + 0x5cadb0))(player, 1);
    int sv = getenv("DM_TEST_PG_NOSAVE") ? -9 : dm_request_save();
    g_pmade = dm_fs_commit_count(); g_pmore = 0;
    LOG("个人仓库：第 %d → %d 页，搬出 %d 件、放回 %d 件，用时 %.2f 毫秒；请游戏存盘 → %d", from + 1, tgt + 1, n, keep, (pf_now() - t0) / 1e6, sv);
    if (getenv("DM_TEST_PG_TAKE")) {
        uint64_t iv = *(uint64_t *)(player + 0x90), x = iv && *(uint32_t *)iv == 0x01020304 ? *(uint64_t *)(iv + 0x10) : 0, tk[256]; int m = 0;
        for (int k = 0; x && k < 2048 && m < 256; k++) { uint64_t d = *(uint64_t *)(x + 0x10); if (!d) break; if (*(uint32_t *)(x + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 4) tk[m++] = x; x = *(uint64_t *)(d + 0xb0); }
        for (int i = 0; i < m; i++) { uint64_t y = tk[i], d = *(uint64_t *)(y + 0x10), pp = *(uint64_t *)(y + 0x38);
            ((void (*)(uint64_t))(b + 0x55e430))(y); ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, y, 0x20, 4, 0, *(uint8_t *)(d + 0x55), pp ? *(int32_t *)(pp + 0x14) : 0, pp ? *(int32_t *)(pp + 0x10) : 0); ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, y); }
        LOG("测试 DM_TEST_PG_TAKE：翻完当拍从格里拿走 %d 件", m); }
}
static void pages_pre_commit(const char *wd) {
#ifndef DM_PLAYER_BUILD
    { static int kill = -1; if (kill < 0) { const char *e = getenv("DM_TEST_PG_KILL"); kill = e ? atoi(e) : 0; }
      if (kill && g_pmade && dm_fs_commit_count() + 1 == g_pmade + (uint64_t)kill) { LOG("DM_TEST_PG_KILL：翻页后第 %d 次提交前结束进程（%s）", kill, g_ppend ? "翻页待定" : "翻页已落定"); fflush(stderr); _exit(9); } }
#endif
    if (!g_pload || !g_pchar[0] || (!g_ppend && !g_pdirty)) return;
    if (!g_ppend) { if (pages_save(wd) == 0) { g_pdirty = 0; LOG("个人仓库：进局后改过的页槽（去重 / 落定上次的翻页）随这次提交落盘"); } return; }








    uint64_t cno = dm_fs_commit_count() + 1; const char *why = NULL; char cp[1400]; snprintf(cp, sizeof cp, "%s/%s/%s.d2s", wd, g_pchar, g_pchar);
    uint8_t *d = NULL; size_t dn = 0; FILE *f = fopen(cp, "rb");
    if (f) { fseek(f, 0, SEEK_END); long L = ftell(f); fseek(f, 0, SEEK_SET); d = L > 0 && L < (16 << 20) ? malloc((size_t)L) : NULL; dn = d ? fread(d, 1, (size_t)L, f) : 0; fclose(f); }
    int na = 0, nb = 0;
    for (int i = 0; d && i < g_pA.n; i++) na += dm_memfind(d, dn, g_pA.it[i].bits, g_pA.it[i].len) != NULL;
    for (int i = 0; d && i < g_pB.n; i++) nb += dm_memfind(d, dn, g_pB.it[i].bits, g_pB.it[i].len) != NULL;
    uint64_t h = d ? dm_fnv64(d, dn) : 0; free(d);
    int isA = na == g_pA.n, isB = nb == g_pB.n;
    if (!dn) { g_pmore = 1; LOG("【红】个人仓库：提交前核对 #%llu —— 读不到本角色档 %s，等下一次", (unsigned long long)cno, cp); }
    else if (!g_pA.n && !g_pB.n) why = "两页都空，不涉及物品";
    else if (g_pH0 && h != g_pH0) why = "角色档已不是翻页时那份（翻页时没有在途的存盘，之后的只能是翻页后序列化的）";
    else if (isB && g_pB.n) why = "格里是目标页";
    else if (isA && g_pA.n) { g_pmore = 1; LOG("个人仓库：提交前核对 #%llu —— 格里是搬出页（%d / %d 件都在）→ 角色档是翻页前的，等翻页后的存档", (unsigned long long)cno, na, g_pA.n); }
    else if (!g_pA.n && !nb) { g_pmore = 1; LOG("个人仓库：提交前核对 #%llu —— 从空页翻来、格里还没有目标页的件（0 / %d）→ 像是翻页前的，等翻页后的存档", (unsigned long long)cno, g_pB.n); }
    else why = "格里既不是搬出页也不是目标页（翻页后又动过格）";
    if (why) { g_ppend = 0;
        if (pages_save(wd) == 0) { pend_clear(); g_pdirty = 0; g_pmore = 0; LOG("个人仓库：提交前核对 #%llu —— %s → 翻页后，侧文件随这次提交落盘（当前第 %d 页）", (unsigned long long)cno, why, g_pcur + 1); }
        else g_ppend = 1; }
    else if (pages_save(wd) == 0) LOG("个人仓库：提交前核对 #%llu —— 还没证明，侧文件带待定翻页记录随这次提交落盘（第 %d → %d 页，搬出 %d 件、搬进 %d 件）", (unsigned long long)cno, g_pfrom + 1, g_pto + 1, g_pA.n, g_pB.n);
}


static int item_kids(uint64_t it) { uint64_t inv = *(uint64_t *)(it + 0x90); int n = 0; if (!inv || *(uint32_t *)inv != 0x01020304) return 0; for (uint64_t c = *(uint64_t *)(inv + 0x10); c && n < 16; ) { uint64_t d = *(uint64_t *)(c + 0x10); if (!d) break; n++; c = *(uint64_t *)(d + 0xb0); } return n; }
static void pg_sock_test(uint64_t G, uint64_t player, uint8_t *b, uint32_t tq) {
    static uint32_t at; static int got; if (!got) { got = 1; const char *e = getenv("DM_TEST_PG_SOCK"); at = e ? (uint32_t)atoi(e) : 0; }
    if (!at || tq < at) return; at = 0;
    uint64_t inv = *(uint64_t *)(player + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0, gems[64], gear[64]; int ng = 0, nq = 0;
    for (int k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(it);
        if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0) { if (((char *)&c)[0] == 'g' && ng < 64) gems[ng++] = it; else if (nq < 64) gear[nq++] = it; } it = *(uint64_t *)(d + 0xb0); }
    for (int q = 0; q < nq && ng; q++) { uint64_t g = gems[ng - 1], gd = *(uint64_t *)(g + 0x10), gp = *(uint64_t *)(g + 0x38); int gx = gp ? *(int32_t *)(gp + 0x10) : 0, gy = gp ? *(int32_t *)(gp + 0x14) : 0;
        ((void (*)(uint64_t))(b + 0x55e430))(g); ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, g, 0x20, 4, 0, *(uint8_t *)(gd + 0x55), gy, gx);
        ((void (*)(uint64_t, int))(b + 0x5ca8a0))(g, 4);
        if (((int (*)(uint64_t, uint64_t, uint32_t, uint32_t, int))(b + 0x64d080))(G, gear[q], *(uint32_t *)(g + 8), *(uint32_t *)(gear[q] + 8), 2)) {
            ng--; uint64_t x = gear[q], pp = *(uint64_t *)(x + 0x38), d = *(uint64_t *)(x + 0x10); static uint8_t buf[VBYTES]; int len = dm_item_write(b, x, buf); uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(x);
            LOG("镶孔测试：%.3s 镶上一颗宝石（孔里现 %d 件），位流 %d 字节，搬进个人仓库 (5,5)", (const char *)&c, item_kids(x), len);
            ((void (*)(uint64_t))(b + 0x55e430))(x); ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, x, 0x20, 4, 0, *(uint8_t *)(d + 0x55), pp ? *(int32_t *)(pp + 0x14) : 0, pp ? *(int32_t *)(pp + 0x10) : 0); ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, x);
            int sk = 0; uint64_t nu = len ? dm_item_make(b, G, 0, buf, len, &sk) : 0, nd = nu ? *(uint64_t *)(nu + 0x10) : 0, ip = nu ? *(uint64_t *)(nu + 0x38) : 0;
            if (nd && ip) { *(uint8_t *)(nd + 0x55) = 4; *(int32_t *)(ip + 0x10) = 5; *(int32_t *)(ip + 0x14) = 5; uint64_t one = nu, ox = 0; ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(b + 0x6f1940))(G, player, &one, &ox);
                ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, uint64_t))(b + 0x642f70))(G, player, nu, 1, 2, 0, 4, 0, 5 | 5 << 16); }
            LOG("镶孔测试：重建后孔里 %d 件，进个人仓库 %s", nu ? item_kids(nu) : -1, nd && *(uint64_t *)(nd + 0xa0) ? "成" : "没成"); return; }
        ((void (*)(uint64_t, int))(b + 0x5ca8a0))(g, 0); }
    LOG("镶孔测试：背包里没有能镶的装备（%d 件装备、%d 颗宝石）", nq, ng);
}
void dm_pages_tick(uint64_t G, uint64_t player, uint8_t *b) {
    static uint64_t lp; if (!player) { lp = 0; g_pload = 0; return; }
    uint32_t tq = *(uint32_t *)(G + 0x170);
    if (player != lp) { lp = player; uint64_t pd = *(uint64_t *)(player + 0x10); pthread_mutex_lock(&g_pmu); strlcpy(g_pchar, pd ? (const char *)pd : "", sizeof g_pchar);
        { const char *sn = dm_stash_name(b); strlcpy(g_pstash, sn ? sn : "", sizeof g_pstash); } pages_load(b, player); g_pload = 1; pthread_mutex_unlock(&g_pmu); }
    { pthread_mutex_lock(&g_pmu);
      dm_ser_observe(G);
      int more = g_pmore; g_pmore = 0; pthread_mutex_unlock(&g_pmu); if (more && !getenv("DM_TEST_PG_NOSAVE")) { int r = dm_request_save(); LOG("个人仓库：翻页还没证明，再请一次存盘 → %d", r); } }
    pg_sock_test(G, player, b, tq);
    int dir = atomic_exchange(&g_preq, 0);
    const char *e = getenv("DM_TEST_PG_GO");
    if (e) { char buf[128]; strlcpy(buf, e, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t; t = strtok_r(NULL, ",", &sv)) { int at = 0, dv = 0; if (sscanf(t, "%d:%d", &at, &dv) == 2 && (uint32_t)at == tq) dir = dv; } }
#ifndef DM_PLAYER_BUILD
    { const char *ce = getenv("DM_TEST_CLOCK_AT");
      if (ce) { void dm_test_clock_shift(int64_t d); char buf[128]; strlcpy(buf, ce, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t; t = strtok_r(NULL, ",", &sv)) { int at = 0; long long dv = 0; if (sscanf(t, "%d:%lld", &at, &dv) == 2 && (uint32_t)at == tq) dm_test_clock_shift(dv); } } }
#endif
    const char *lg = getenv("DM_TEST_PG_LOG");
    if (lg) { char buf[128]; strlcpy(buf, lg, sizeof buf); for (char *sv, *t = strtok_r(buf, ",", &sv); t; t = strtok_r(NULL, ",", &sv)) if ((uint32_t)atoi(t) == tq) {
        uint64_t inv = *(uint64_t *)(player + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; int n = 0;
        for (int k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d) break;
            if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 4) { uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(it); n++; LOG("个人仓库读数：拍 %u %.3s 格 %d,%d 孔里 %d 件", tq, (const char *)&c, pp ? *(int32_t *)(pp + 0x10) : -1, pp ? *(int32_t *)(pp + 0x14) : -1, item_kids(it)); }
            it = *(uint64_t *)(d + 0xb0); }
        LOG("个人仓库读数：拍 %u 第 %d 页，格里 %d 件", tq, g_pcur + 1, n); } }




    { int dm_itemjob_owner(void), owned = dm_itemjob_owner() != 0, jdir = atomic_exchange(&g_jreq, 0);
      if (owned && dir) { LOG("personal stash: page turn %+d dropped, item job %d owns the grids", dir, dm_itemjob_owner()); dir = 0; }
      if (!owned && jdir) { LOG("personal stash: item job page turn %+d without an owner dropped", jdir); jdir = 0; }
      if (jdir) dir = jdir;
      static int hold, hold_job; static uint32_t hq; if (dir) { hold = dir; hold_job = jdir != 0; hq = tq; }
      if (hold && owned && !hold_job) { LOG("personal stash: held page turn %+d dropped, item job %d owns the grids", hold, dm_itemjob_owner()); hold = 0; }
      atomic_store(&g_phold, hold);
      if (hold) { if ((dm_ser_state() == 1 || (dm_ser_sec() == dm_user_clock() && !dm_user_clock_fixed())) && tq - hq < 50) dir = 0; else { if (tq != hq) LOG("个人仓库：翻页顺延了 %u 拍（等存盘写完 / 这一秒刚存过盘）%s", tq - hq, dm_ser_state() ? "，到上限存盘还在写：这次照翻、只按格判" : ""); dir = hold; hold = 0; atomic_store(&g_phold, 0); } } }
    if (dir) { pthread_mutex_lock(&g_pmu); pages_flip(b, G, player, dir); pthread_mutex_unlock(&g_pmu); }
}
static void pages_hook(const char *wd) { pthread_mutex_lock(&g_pmu); pages_pre_commit(wd); pthread_mutex_unlock(&g_pmu); }
__attribute__((constructor)) static void pages_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[0] = pages_hook; }
