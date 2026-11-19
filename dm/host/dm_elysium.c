
















































#include "dm_elysium.h"
#include "hle.h"
#include "dm.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RATIO_REC 0x44
#define UNIQUE_MUL 5
#define SET_MUL 3
void uimsg_subscribe(void (*fn)(const char *cat, const char *act, const char *arg));
const char *dm_save_workdir(void);

static _Atomic int g_arm, g_created, g_active;
static uint8_t *g_orig[4]; static uint64_t g_orig_n[4], g_orig_sum; static int g_have_orig;

static _Atomic int g_topup;
static _Atomic int g_topup_done;
static char g_topup_name[64];
void dm_elysium_arm(int on) { atomic_store(&g_arm, on != 0); LOG("极乐世界：建角勾选 = %d", on != 0); }
int dm_elysium_active(void) { return atomic_load(&g_active); }
static void on_uimsg(const char *cat, const char *act, const char *arg) { (void)cat; (void)arg; if (!strcmp(act, "TryCreateCharacter")) atomic_store(&g_created, 1); }

static uint8_t *main_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t table(uint8_t *b, int ver, uint64_t *n) { uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16); if (!t) { *n = 0; return 0; } *n = *(uint64_t *)(t + 0x16c8); return *(uint64_t *)(t + 0x16c0); }
static uint64_t fnv(uint64_t h, const uint8_t *p, size_t n) { for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 0x100000001b3ull; } return h; }
static uint64_t tc_sum(uint8_t *b, uint64_t h);
static uint64_t mon_sum(uint8_t *b, uint64_t h);
static uint64_t tables_sum(uint8_t *b) { uint64_t h = 0xcbf29ce484222325ull; for (int v = 1; v <= 3; v++) { uint64_t n, r = table(b, v, &n); if (r && n) h = fnv(h, (const uint8_t *)(uintptr_t)r, n * RATIO_REC); } return mon_sum(b, tc_sum(b, h)); }

#define TC_REC 0x50
#define TC_ENT 0x1c
#define RUNE_MUL 5
#define NODROP_DIV 10
typedef struct { uint64_t arr, n; uint8_t *recs; uint8_t **ents; } tc_snap;
static tc_snap g_tc[4];
static uint64_t tc_table(uint8_t *b, int ver, uint64_t *n) { uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16); if (!t) { *n = 0; return 0; } *n = *(uint64_t *)(t + 0xdb8); return *(uint64_t *)(t + 0xdb0); }
static uint64_t tc_sum(uint8_t *b, uint64_t h) { for (int v = 1; v <= 3; v++) { uint64_t n, a = tc_table(b, v, &n); for (uint64_t i = 0; a && i < n; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(a + i * TC_REC); h = fnv(h, r + 0x10, 0x10); uint64_t e = *(uint64_t *)(r + 0x38), ne = *(uint64_t *)(r + 0x40); if (e && ne) h = fnv(h, (const uint8_t *)(uintptr_t)e, ne * TC_ENT); } } return h; }
static int tc_snapshot(uint8_t *b) {
    for (int v = 1; v <= 3; v++) { uint64_t n, a = tc_table(b, v, &n); if (!a || !n) continue; tc_snap *s = &g_tc[v]; s->arr = a; s->n = n; s->recs = malloc(n * TC_REC); s->ents = calloc(n, sizeof *s->ents); if (!s->recs || !s->ents) return 0; memcpy(s->recs, (void *)(uintptr_t)a, n * TC_REC);
        for (uint64_t i = 0; i < n; i++) { uint8_t *r = s->recs + i * TC_REC; uint64_t e = *(uint64_t *)(r + 0x38), ne = *(uint64_t *)(r + 0x40); if (e && ne) { s->ents[i] = malloc(ne * TC_ENT); if (!s->ents[i]) return 0; memcpy(s->ents[i], (void *)(uintptr_t)e, ne * TC_ENT); } } }
    return 1;
}


static int tc_same(uint8_t *b, int v) { tc_snap *s = &g_tc[v]; uint64_t n, a = tc_table(b, v, &n); if (!s->recs || a != s->arr || n != s->n) return 0;
    for (uint64_t i = 0; i < n; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(a + i * TC_REC), *o = s->recs + i * TC_REC; if (*(uint64_t *)(r + 0x38) != *(uint64_t *)(o + 0x38) || *(uint64_t *)(r + 0x40) != *(uint64_t *)(o + 0x40)) return 0; } return 1; }
static void tc_restore(uint8_t *b) { for (int v = 1; v <= 3; v++) { if (!tc_same(b, v)) continue; tc_snap *s = &g_tc[v];
        for (uint64_t i = 0; i < s->n; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(s->arr + i * TC_REC), *o = s->recs + i * TC_REC; memcpy(r + 0x10, o + 0x10, 0x10); uint64_t ne = *(uint64_t *)(o + 0x40); if (s->ents[i]) memcpy((void *)(uintptr_t)*(uint64_t *)(o + 0x38), s->ents[i], ne * TC_ENT); } } }


static void tc_weights(uint8_t *r, int64_t *wc, int64_t *we) { uint8_t *e = (uint8_t *)(uintptr_t)*(uint64_t *)(r + 0x38); uint64_t ne = *(uint64_t *)(r + 0x40);
    for (uint64_t k = 0; k < ne; k++) { int32_t nc = k + 1 < ne ? *(int32_t *)(e + (k + 1) * TC_ENT) : *(int32_t *)(r + 0x10), nx = k + 1 < ne ? *(int32_t *)(e + (k + 1) * TC_ENT + 4) : *(int32_t *)(r + 0x14); wc[k] = nc - *(int32_t *)(e + k * TC_ENT); we[k] = nx - *(int32_t *)(e + k * TC_ENT + 4); } }
static void tc_write(uint8_t *r, const int64_t *wc, const int64_t *we) { uint8_t *e = (uint8_t *)(uintptr_t)*(uint64_t *)(r + 0x38); uint64_t ne = *(uint64_t *)(r + 0x40); int64_t c = 0, x = 0;
    for (uint64_t k = 0; k < ne; k++) { *(int32_t *)(e + k * TC_ENT) = (int32_t)c; *(int32_t *)(e + k * TC_ENT + 4) = (int32_t)x; c += wc[k]; x += we[k]; } *(int32_t *)(r + 0x10) = (int32_t)c; *(int32_t *)(r + 0x14) = (int32_t)x; }
#define MON_REC 0x1fc
static uint8_t *g_mon[4]; static uint64_t g_mon_arr[4], g_mon_n[4];
static uint64_t mon_table(uint8_t *b, int ver, uint64_t *n) { uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16); if (!t) { *n = 0; return 0; } *n = *(uint64_t *)(t + 0xf98); return *(uint64_t *)(t + 0xf90); }
static int mon_snapshot(uint8_t *b) { for (int v = 1; v <= 3; v++) { uint64_t n, a = mon_table(b, v, &n); if (!a || !n) continue; g_mon[v] = malloc(n * 0x30); if (!g_mon[v]) return 0; g_mon_arr[v] = a; g_mon_n[v] = n;
        for (uint64_t i = 0; i < n; i++) memcpy(g_mon[v] + i * 0x30, (void *)(uintptr_t)(a + i * MON_REC + 0xc0), 0x30); } return 1; }
static int mon_same(uint8_t *b, int v) { uint64_t n, a = mon_table(b, v, &n); return g_mon[v] && a == g_mon_arr[v] && n == g_mon_n[v]; }
static uint64_t mon_sum(uint8_t *b, uint64_t h) { for (int v = 1; v <= 3; v++) { uint64_t n, a = mon_table(b, v, &n); for (uint64_t i = 0; a && i < n; i++) h = fnv(h, (const uint8_t *)(uintptr_t)(a + i * MON_REC + 0xc0), 0x30); } return h; }
static void mon_restore(uint8_t *b) { for (int v = 1; v <= 3; v++) if (mon_same(b, v)) for (uint64_t i = 0; i < g_mon_n[v]; i++) memcpy((void *)(uintptr_t)(g_mon_arr[v] + i * MON_REC + 0xc0), g_mon[v] + i * 0x30, 0x30); }
static void mon_apply(uint8_t *b) {
    for (int v = 1; v <= 3; v++) { if (!mon_same(b, v)) continue; { int dup = 0; for (int u = 1; u < v; u++) if (g_mon_arr[u] == g_mon_arr[v]) dup = 1; if (dup) continue; } char list[640] = ""; int nb = 0;
        for (uint64_t i = 0; i < g_mon_n[v]; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(g_mon_arr[v] + i * MON_REC); if (!r[0xf0]) continue; int any = 0;
            for (int d = 0; d < 3; d++) { uint16_t *blk = (uint16_t *)(r + 0xc0 + d * 0x10), q = blk[3]; if (!q) continue; any = 1; for (int k = 0; k < 8; k++) if (k != 3) blk[k] = q; }
            if (any) { nb++; size_t L = strlen(list); snprintf(list + L, sizeof list - L, " %llu（任务 %u 位 %u，三个难度的普通格 %u/%u/%u → %u/%u/%u）", i, r[0xf0], r[0xf1], *(uint16_t *)(g_mon[v] + i * 0x30), *(uint16_t *)(g_mon[v] + i * 0x30 + 0x10), *(uint16_t *)(g_mon[v] + i * 0x30 + 0x20), *(uint16_t *)(r + 0xc0), *(uint16_t *)(r + 0xd0), *(uint16_t *)(r + 0xe0)); } }
        if (v == 2 || v == 3 || getenv("DM_ELYSIUM_LOG")) LOG("极乐世界：版本 %d 怪物表——永远按首杀掉落的 %d 行：%s", v, nb, list); }
}
#define TC_MAXE 64
static void tc_apply(uint8_t *b) {
    for (int v = 1; v <= 3; v++) { if (!tc_same(b, v)) continue; tc_snap *s = &g_tc[v]; uint64_t n = s->n; { int dup = 0; for (int u = 1; u < v; u++) if (g_tc[u].arr == s->arr) dup = 1; if (dup) continue; }
        uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)v * 16), items = *(uint64_t *)(t + 0x15c0); uint32_t nitems = *(uint32_t *)(t + 0x15c8);
        uint8_t *rune = calloc(n, 1); int64_t *fa = calloc(n, sizeof *fa), *fd = calloc(n, sizeof *fd); if (!rune || !fa || !fd) { free(rune); free(fa); free(fd); continue; }


        for (uint64_t i = 0; i < n; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(s->arr + i * TC_REC), *e = (uint8_t *)(uintptr_t)*(uint64_t *)(r + 0x38); uint64_t ne = *(uint64_t *)(r + 0x40); int ok = e && ne > 0 && ne <= TC_MAXE, any = 0;
            for (uint64_t k = 0; ok && k < ne; k++) { uint16_t id = *(uint16_t *)(e + k * TC_ENT + 8); if (e[k * TC_ENT + 0xc] & 4) continue; const char *c = id < nitems ? (const char *)(uintptr_t)(items + (uint64_t)id * 0x1c0 + 0x80) : "";
                if (c[0] == 'r' && c[1] >= '0' && c[1] <= '9' && c[2] >= '0' && c[2] <= '9' && (c[3] == ' ' || !c[3])) any = 1; else ok = 0; }
            rune[i] = ok ? (any ? 1 : 2) : 0; }
        for (int ch = 1; ch; ) { ch = 0; for (uint64_t i = 0; i < n; i++) { if (!rune[i]) continue; uint8_t *r = (uint8_t *)(uintptr_t)(s->arr + i * TC_REC), *e = (uint8_t *)(uintptr_t)*(uint64_t *)(r + 0x38); uint64_t ne = *(uint64_t *)(r + 0x40);
                for (uint64_t k = 0; k < ne; k++) if (e[k * TC_ENT + 0xc] & 4) { uint16_t id = *(uint16_t *)(e + k * TC_ENT + 8); if (id >= n || !rune[id]) { rune[i] = 0; ch = 1; break; } } } }
        for (uint64_t i = 0; i < n; i++) if (rune[i] == 2) { uint8_t *r = (uint8_t *)(uintptr_t)(s->arr + i * TC_REC), *e = (uint8_t *)(uintptr_t)*(uint64_t *)(r + 0x38); uint64_t ne = *(uint64_t *)(r + 0x40); int direct = 0;
            for (uint64_t k = 0; k < ne; k++) if (!(e[k * TC_ENT + 0xc] & 4)) direct = 1; if (!direct) rune[i] = 0; }
        int nrune = 0, nx5 = 0, nfull = 0, npar = 0, nshort = 0;
        for (uint64_t i = 0; i < n; i++) nrune += rune[i] != 0;


        for (uint64_t i = 0; i < n; i++) { if (rune[i]) continue; uint8_t *r = (uint8_t *)(uintptr_t)(s->arr + i * TC_REC), *e = (uint8_t *)(uintptr_t)*(uint64_t *)(r + 0x38); uint64_t ne = *(uint64_t *)(r + 0x40); if (!e || !ne || ne > TC_MAXE) continue;
            if (*(int32_t *)(r + 0x18) < 0) continue;
            int64_t wc[TC_MAXE], we[TC_MAXE], T = 0, w = 0; tc_weights(r, wc, we);
            for (uint64_t k = 0; k < ne; k++) { T += we[k]; if ((e[k * TC_ENT + 0xc] & 4) && *(uint16_t *)(e + k * TC_ENT + 8) < n && rune[*(uint16_t *)(e + k * TC_ENT + 8)]) w += we[k]; }
            if (w <= 0) continue;
            if (w < T) {
                for (uint64_t k = 0; k < ne; k++) { int isr = (e[k * TC_ENT + 0xc] & 4) && *(uint16_t *)(e + k * TC_ENT + 8) < n && rune[*(uint16_t *)(e + k * TC_ENT + 8)]; if (isr) we[k] *= RUNE_MUL; }
                tc_write(r, wc, we); fa[i] = T + (RUNE_MUL - 1) * w; fd[i] = T; nx5++; }
            else { *(int32_t *)(r + 0x1c) = 0; nfull++; } }


        for (uint64_t i = 0; i < n; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(s->arr + i * TC_REC), *e = (uint8_t *)(uintptr_t)*(uint64_t *)(r + 0x38); uint64_t ne = *(uint64_t *)(r + 0x40); if (!e || !ne || ne > TC_MAXE) continue;
            if (*(int32_t *)(r + 0x18) < 0) continue;
            for (uint64_t j = 0; j < ne; j++) { uint16_t id = *(uint16_t *)(e + j * TC_ENT + 8); if (!(e[j * TC_ENT + 0xc] & 4) || id >= n || !fd[id]) continue;
                int64_t wc[TC_MAXE], we[TC_MAXE], A = fa[id], D = fd[id], N = *(int32_t *)(r + 0x1c); tc_weights(r, wc, we); int64_t g = we[j];
                for (uint64_t k = 0; k < ne; k++) { wc[k] *= D; we[k] *= D; } we[j] = g * A; N = N * D - g * (A - D); if (N < 0) { N = 0; nshort++; }
                tc_write(r, wc, we); *(int32_t *)(r + 0x1c) = (int32_t)N; npar++; } }






        int ndiv = 0; for (uint64_t i = 0; i < n; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(s->arr + i * TC_REC); int32_t N = *(int32_t *)(r + 0x1c); if (N <= 0) continue; int32_t q = (N + NODROP_DIV / 2) / NODROP_DIV; *(int32_t *)(r + 0x1c) = q ? q : 1; ndiv++; }
        if (v == 2 || getenv("DM_ELYSIUM_LOG")) LOG("极乐世界：版本 %d 宝藏表——符文组 %d 张；引用它的表里符文权重 ×%d 的 %d 张、整表只出符文改成必出的 %d 张；上一层补偿 %d 项（其中「不掉落」不够扣的 %d 项）；「不掉落」÷%d 的 %d 张", v, nrune, RUNE_MUL, nx5, nfull, npar, nshort, NODROP_DIV, ndiv);
        free(rune); free(fa); free(fd); }
}

static int snapshot(uint8_t *b) {
    if (g_have_orig) return 1; int any = 0;
    for (int v = 1; v <= 3; v++) { uint64_t n, r = table(b, v, &n); if (!r || !n) continue; g_orig[v] = malloc(n * RATIO_REC); if (!g_orig[v]) return 0; memcpy(g_orig[v], (void *)(uintptr_t)r, n * RATIO_REC); g_orig_n[v] = n; any = 1; }
    if (!any) return 0;
    if (!tc_snapshot(b) || !mon_snapshot(b)) return 0;
    g_orig_sum = tables_sum(b); g_have_orig = 1; LOG("极乐世界：记下品质比例表与宝藏表原值（版本 1–3 各 %llu / %llu / %llu 条，校验和 %016llx）", g_orig_n[1], g_orig_n[2], g_orig_n[3], g_orig_sum); return 1;
}
static void restore(uint8_t *b) { tc_restore(b); mon_restore(b); for (int v = 1; v <= 3; v++) { uint64_t n, r = table(b, v, &n); if (r && g_orig[v] && n == g_orig_n[v]) memcpy((void *)(uintptr_t)r, g_orig[v], n * RATIO_REC); } }
static void apply(uint8_t *b) {
    for (int v = 1; v <= 3; v++) { uint64_t n, r = table(b, v, &n); if (!r || !g_orig[v] || n != g_orig_n[v]) continue;
        for (uint64_t i = 0; i < n; i++) { int32_t *o = (int32_t *)(g_orig[v] + i * RATIO_REC), *d = (int32_t *)(uintptr_t)(r + i * RATIO_REC);
            d[0] = o[0] / UNIQUE_MUL; d[1] = o[1] * UNIQUE_MUL; d[2] = o[2] / UNIQUE_MUL;
            d[6] = o[6] / SET_MUL; d[7] = o[7] * SET_MUL; d[8] = o[8] / SET_MUL; } }
}
static void dump(uint8_t *b, const char *when) {
    for (int v = 1; v <= 3; v++) { uint64_t n, r = table(b, v, &n); for (uint64_t i = 0; r && i < n; i++) { int32_t *d = (int32_t *)(uintptr_t)(r + i * RATIO_REC); uint8_t *c = (uint8_t *)d;
        LOG("极乐世界[%s] 版本 %d 第 %llu 条（版本号 %u 特殊 %u 职业专属 %u）：暗金 %d / %d / %d，套装 %d / %d / %d", when, v, i, *(uint16_t *)(c + 0x40), c[0x42], c[0x43], d[0], d[1], d[2], d[6], d[7], d[8]); } }
}



static int list_path(const char *file, char *out, size_t cap) { const char *w = dm_save_workdir(); if (!w) return 0; snprintf(out, cap, "%s/%s", w, file); return 1; }
#define LIST_ELY "sw_elysium.txt"
#define LIST_TOPUP "dm_stash_topup.txt"
static int list_has(const char *file, const char *name) { char p[1200], ln[128]; if (!list_path(file, p, sizeof p)) return 0; FILE *f = fopen(p, "r"); if (!f) return 0; int hit = 0;
    while (fgets(ln, sizeof ln, f)) { ln[strcspn(ln, "\r\n")] = 0; if (!strcmp(ln, name)) hit = 1; } fclose(f); return hit; }
static int list_set(const char *file, const char *name, int on) { char p[1200], t[1210], ln[128]; if (!list_path(file, p, sizeof p)) return 0; snprintf(t, sizeof t, "%s.tmp", p);
    FILE *o = fopen(t, "w"); if (!o) return 0; FILE *f = fopen(p, "r"); int kept = 0;
    if (f) { while (fgets(ln, sizeof ln, f)) { ln[strcspn(ln, "\r\n")] = 0; if (ln[0] && strcmp(ln, name)) { fprintf(o, "%s\n", ln); kept++; } } fclose(f); }
    if (on) { fprintf(o, "%s\n", name); kept++; }
    int ok = fclose(o) == 0; if (!ok) { remove(t); return 0; }
    if (kept) return rename(t, p) == 0; remove(t); return remove(p) == 0 || !f; }



static void tc_dump(uint8_t *b) {
    const char *path = getenv("DM_ELYSIUM_TCDUMP"); if (!path) return; FILE *f = fopen(path, "w"); if (!f) return;
    for (int v = 1; v <= 3; v++) { uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)v * 16); if (!t) continue; uint64_t arr = *(uint64_t *)(t + 0xdb0), n = *(uint64_t *)(t + 0xdb8), items = *(uint64_t *)(t + 0x15c0); uint32_t nitems = *(uint32_t *)(t + 0x15c8);
        fprintf(f, "# 版本 %d：%llu 条\n", v, n);
        for (uint64_t i = 0; arr && i < n; i++) { uint8_t *r = (uint8_t *)(uintptr_t)(arr + i * 0x50); uint64_t ent = *(uint64_t *)(r + 0x38), ne = *(uint64_t *)(r + 0x40);
            fprintf(f, "v%d tc %llu 组 %u 级 %u +8=%d +c=%d 累计 %d/%d +18=%d +1c=%d +20=%d 标志 %02x %02x 品质 %u %u %u %u %u %u 项 %llu：", v, i, *(uint16_t *)r, *(uint16_t *)(r + 2), *(int32_t *)(r + 8), *(int32_t *)(r + 0xc), *(int32_t *)(r + 0x10), *(int32_t *)(r + 0x14),
                    *(int32_t *)(r + 0x18), *(int32_t *)(r + 0x1c), *(int32_t *)(r + 0x20), r[0x24], r[0x25], *(uint16_t *)(r + 0x26), *(uint16_t *)(r + 0x28), *(uint16_t *)(r + 0x2a), *(uint16_t *)(r + 0x2c), *(uint16_t *)(r + 0x2e), *(uint16_t *)(r + 0x30), ne);
            for (uint64_t k = 0; ent && k < ne && k < 64; k++) { uint8_t *e = (uint8_t *)(uintptr_t)(ent + k * 0x1c); uint16_t id = *(uint16_t *)(e + 8); char code[5] = "    ";
                if (!(e[0xc] & 4) && id < nitems) memcpy(code, (void *)(uintptr_t)(items + (uint64_t)id * 0x1c0 + 0x80), 4);
                fprintf(f, " [%d/%d id %u +a=%u 标志 %02x %.4s]", *(int32_t *)e, *(int32_t *)(e + 4), id, *(uint16_t *)(e + 0xa), e[0xc], code); }
            fprintf(f, "\n"); } }
    fclose(f);
}

void dm_elysium_pump(void) {
    static int sub, in_game; if (!sub) { sub = 1; uimsg_subscribe(on_uimsg); if (getenv("DM_TEST_ELYSIUM_ARM")) dm_elysium_arm(1); }
    uint8_t *b = main_base(); if (!b || !snapshot(b)) return;
    uint64_t pl = ((uint64_t (*)(uint32_t))(b + 0x45f9d0))(*(uint32_t *)(b + 0x2d20e4c)), pd = pl ? *(uint64_t *)(pl + 0x10) : 0;
    if (!pd) { if (in_game) { in_game = 0; restore(b); if (atomic_exchange(&g_active, 0)) LOG("极乐世界：退局，品质比例表与宝藏表已写回原值（校验和 %s）", tables_sum(b) == g_orig_sum ? "相符" : "【红】不符"); } return; }
    if (in_game) {
        static uint64_t c0, f0, cl, fl; static int w; uint64_t dm_fs_commit_count(void), dm_guest_frame(void); int dm_request_save(void); uint64_t c = dm_fs_commit_count(), f = dm_guest_frame();
        if (atomic_load(&g_topup_done) == 1) { atomic_store(&g_topup_done, 2); c0 = cl = c; f0 = fl = f; w = 1; dm_request_save(); }
        else if (w) { if (c != cl) { cl = c; fl = f; }
            if (c > c0 && f - fl >= 30) { w = 0; atomic_store(&g_topup_done, 0); int ok = list_set(LIST_TOPUP, g_topup_name, 0); dm_request_save(); LOG("极乐世界：共享仓库补齐已随存档提交落盘（%llu 次提交），「%s」的欠条已划掉%s", (unsigned long long)(c - c0), g_topup_name, ok ? "" : "【红】欠条文件写失败"); }
            else if (f - f0 > 600) { w = 0; atomic_store(&g_topup_done, 0); LOG("【红】极乐世界：共享仓库补齐后 10 秒没等到存档提交，「%s」的欠条留着，下次进局再补", g_topup_name); } }
        return; }
    in_game = 1;
    char name[64]; strlcpy(name, (const char *)(uintptr_t)pd, sizeof name); if (!name[0] || strchr(name, '/')) return;
    if (atomic_exchange(&g_created, 0)) { int on = atomic_exchange(&g_arm, 0), ok = list_set(LIST_ELY, name, on); if (ok) list_set(LIST_TOPUP, name, on); if (on || !ok) LOG("极乐世界：新建角色「%s」%s%s", name, on ? "记为极乐世界角色" : "不是极乐世界角色", ok ? "" : "【红】名单文件写失败"); }
    restore(b);
    const char *te = getenv("DM_TEST_ELYSIUM"); int on = list_has(LIST_ELY, name) || (te && !strcmp(te, name));
    if (on && list_has(LIST_TOPUP, name)) { strlcpy(g_topup_name, name, sizeof g_topup_name); atomic_store(&g_topup_done, 0); atomic_store(&g_topup, 1); }
    if (on) { apply(b); tc_apply(b); mon_apply(b); atomic_store(&g_active, 1); LOG("极乐世界：角色「%s」进局，暗金机会值 ÷%d、套装 ÷%d、符文 ×%d、不掉落 ÷%d、boss 永远首杀（品质比例表、宝藏表、怪物表已改，退局还原）", name, UNIQUE_MUL, SET_MUL, RUNE_MUL, NODROP_DIV); }
    else if (tables_sum(b) != g_orig_sum) LOG("【红】极乐世界：普通角色「%s」进局后品质比例表 / 宝藏表校验和与启动时不符", name);
    else if (getenv("DM_ELYSIUM_LOG")) LOG("极乐世界：普通角色「%s」进局，品质比例表与宝藏表校验和与启动时相符", name);
    if (getenv("DM_ELYSIUM_LOG")) dump(b, on ? "极乐" : "普通");
    tc_dump(b);
}
























































#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
const char *dm_save_commitdir(void);
#define STASH_GOLD 2500000
#define STASH_TOA 2
static void stash_backup(void) {
    const char *c = dm_save_commitdir(); if (!c) return; char dir[1100]; snprintf(dir, sizeof dir, "%s/../stash_backup", c); mkdir(dir, 0755);
    static const char *nm[2] = { "ModernSharedStashSoftCoreV2", "ModernSharedStashHardCoreV2" }; char stamp[32]; time_t t = time(NULL); struct tm tm; localtime_r(&t, &tm); strftime(stamp, sizeof stamp, "%Y%m%d_%H%M%S", &tm);
    for (int k = 0; k < 2; k++) { char src[1300], dst[1300]; snprintf(src, sizeof src, "%s/%s/%s.d2i", c, nm[k], nm[k]); snprintf(dst, sizeof dst, "%s/%s.%s.d2i", dir, nm[k], stamp);
        FILE *f = fopen(src, "rb"); if (!f) continue; FILE *o = fopen(dst, "wb"); char buf[4096]; size_t n, tot = 0; int ok = o != NULL;
        while (ok && (n = fread(buf, 1, sizeof buf, f)) > 0) { ok = fwrite(buf, 1, n, o) == n; tot += n; } fclose(f); if (o) ok = fclose(o) == 0 && ok;
        LOG("极乐世界：共享仓库补齐前备份 %s → %s（%zu 字节）%s", src, dst, tot, ok ? "" : "【红】备份写失败");
        char old[64][64]; int no = 0; DIR *dd = opendir(dir); struct dirent *e; size_t pl = strlen(nm[k]);
        while (dd && (e = readdir(dd))) if (no < 64 && !strncmp(e->d_name, nm[k], pl) && e->d_name[pl] == '.' && strlen(e->d_name) < 64) strlcpy(old[no++], e->d_name, 64);
        if (dd) closedir(dd);
        qsort(old, (size_t)no, 64, (int (*)(const void *, const void *))strcmp);
        for (int i = 0; i + 5 < no; i++) { char rm[1300]; snprintf(rm, sizeof rm, "%s/%s", dir, old[i]); remove(rm); } }
}
static uint64_t unit_inv(uint64_t u) { uint64_t a0 = *(uint64_t *)(u + 0xa0), i = a0 && *(int32_t *)(a0 + 8) && *(uint64_t *)(u + 0x98) ? *(uint64_t *)(u + 0x98) : *(uint64_t *)(u + 0x90); return i && *(uint32_t *)i == 0x01020304 ? i : 0; }
static int stash_pages(uint64_t G, uint64_t player, uint64_t pg[8], uint32_t id[8]) {
    uint64_t pi = unit_inv(player); int n = 0;
    for (uint64_t e = pi ? *(uint64_t *)(pi + 0x68) : 0; e && n < 8; e = *(uint64_t *)(e + 0x10)) { if (!*(uint8_t *)(e + 9) || *(uint8_t *)(e + 0xa)) continue;
        uint32_t i = *(uint32_t *)e; uint64_t u = *(uint64_t *)(G + 0x2230 + 8 * (i & 0x7f)); while (u && *(uint32_t *)(u + 8) != i) u = *(uint64_t *)(u + 0x158);
        if (u && *(uint32_t *)u == 0 && u != player && unit_inv(u)) { pg[n] = u; id[n++] = i; } }
    return n;
}








static void stash_gold_notify(uint8_t *b, uint64_t G, uint64_t player, uint32_t uid, int value) {
    uint64_t pd = *(uint64_t *)(player + 0x10), cn = pd ? *(uint64_t *)(pd + 0xf0) : 0; if (!cn) return;
    uint64_t obj = (uint64_t)(uintptr_t)(b + 0x2d7f3d8); void (*send)(uint64_t, uint32_t, void *) = (void *)(uintptr_t)*(uint64_t *)(*(uint64_t *)obj + 0x28); struct { uint8_t *a, *z; } rg;
    uint32_t seq = *(uint32_t *)(cn + 0x6c0); *(uint32_t *)(G + 0x19050) = 1;
    if (*(uint32_t *)(cn + 0x6c4) != seq) { uint64_t g2 = *(uint64_t *)(cn + 0x2e0); int busy = g2 && *(uint32_t *)(g2 + 0x19050) != 0;
        if (!(*(int32_t *)(cn + 0x99c) >= 1 && *(uint8_t *)(cn + 0x9a4) && !busy)) { uint8_t p2[5] = { 0x2d }; memcpy(p2 + 1, &seq, 4); *(uint32_t *)(cn + 0x6c4) = seq; rg.a = p2; rg.z = p2 + 5; send(obj, *(uint32_t *)cn, &rg); } }
    uint8_t pk[13] = { 0x20 }; uint32_t st = 15; memcpy(pk + 1, &st, 4); memcpy(pk + 5, &uid, 4); memcpy(pk + 9, &value, 4); rg.a = pk; rg.z = pk + 13; send(obj, *(uint32_t *)cn, &rg);
    *(uint32_t *)(G + 0x19050) = 0;
}


int dm_stash_pages(uint64_t G, uint64_t player, uint64_t pg[8], uint32_t id[8]) { return stash_pages(G, player, pg, id); }
void dm_stash_gold_notify(uint8_t *b, uint64_t G, uint64_t player, uint32_t uid, int value) { stash_gold_notify(b, G, player, uid, value); }
uint64_t dm_unit_inv(uint64_t u) { return unit_inv(u); }
static int inv_toa(uint8_t *b, uint64_t unit) {
    uint64_t iv = unit_inv(unit); int c = 0, k = 0; if (!iv) return 0;
    for (uint64_t it = *(uint64_t *)(iv + 0x10); it && c < 600; c++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; if ((((uint32_t (*)(uint64_t))(b + 0x570260))(it) & 0xffffff) == 0x616f74) k++; it = *(uint64_t *)(d + 0xb0); }
    return k;
}




static int adv_toa(uint8_t *b, uint64_t G, uint64_t player) {
    uint64_t pi = unit_inv(player); int k = 0;
    for (uint64_t e = pi ? *(uint64_t *)(pi + 0x68) : 0; e; e = *(uint64_t *)(e + 0x10)) { if (*(uint8_t *)(e + 0xa) != 1) continue;
        uint32_t i = *(uint32_t *)e; uint64_t u = *(uint64_t *)(G + 0x2230 + 8 * (i & 0x7f)); while (u && *(uint32_t *)(u + 8) != i) u = *(uint64_t *)(u + 0x158);
        uint64_t iv = u ? unit_inv(u) : 0; int c = 0;
        for (uint64_t it = iv ? *(uint64_t *)(iv + 0x10) : 0; it && c < 600; c++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            if ((((uint32_t (*)(uint64_t))(b + 0x570260))(it) & 0xffffff) == 0x616f74) { int st = *(uint8_t *)(d + 0x9c); k += st > 0 ? st : 1; }
            it = *(uint64_t *)(d + 0xb0); } }
    return k;
}
static void stash_read(uint8_t *b, uint64_t pg[8], int n, long long *gold, int *toa) {
    *gold = 0; *toa = 0;
    for (int k = 0; k < n; k++) { *gold += ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pg[k], 15, 0); *toa += inv_toa(b, pg[k]); }
}
void dm_elysium_stash_step(uint64_t G, uint64_t player, uint8_t *b) {
    { int dm_itemjob_owner(void); if (dm_itemjob_owner()) return; }
    static int st; static uint32_t at; static uint64_t made[STASH_TOA]; static int nmade, nwant; static int test = -1; uint32_t tq = *(uint32_t *)(G + 0x170);
    if (test < 0) { const char *e = getenv("DM_TEST_STASH_TOPUP"); test = e ? atoi(e) : 0; }
    if (test > 0 && tq >= (uint32_t)test) { test = 0; atomic_store(&g_topup, 1); }
    if (st == 0) { if (!atomic_exchange(&g_topup, 0)) return; st = 1; at = tq + 25; return; }
    if (tq < at) return;
    uint64_t pg[8]; uint32_t id[8]; int n = stash_pages(G, player, pg, id); long long gold; int toa;
    void (*feed)(uint64_t, uint64_t, uint8_t *, uint32_t) = (void *)(b + 0x6e5b40);
    if (st == 1) { st = 0; nmade = 0;
        if (!n) { uint64_t pi = unit_inv(player); char o[400]; int c = 0; o[0] = 0;
            for (uint64_t e = pi ? *(uint64_t *)(pi + 0x68) : 0; e && c < 340; e = *(uint64_t *)(e + 0x10)) { uint32_t i = *(uint32_t *)e; uint64_t u = *(uint64_t *)(G + 0x2230 + 8 * (i & 0x7f)); while (u && *(uint32_t *)(u + 8) != i) u = *(uint64_t *)(u + 0x158);
                c += snprintf(o + c, sizeof o - c, " [id %u 标志 %d/%d/%d 单位%s 类型 %d 物品栏%s]", i, *(uint8_t *)(e + 8), *(uint8_t *)(e + 9), *(uint8_t *)(e + 0xa), u ? "有" : "无", u ? *(int32_t *)u : -1, u && unit_inv(u) ? "有" : "无"); }
            LOG("【红】极乐世界：共享仓库补齐——没找到玩家能用的共享页单位，不动（拍 %u，玩家物品栏%s，链表：%s）", tq, pi ? "有" : "无", o); return; }
        stash_backup(); stash_read(b, pg, n, &gold, &toa); int own = inv_toa(b, player), adv = adv_toa(b, G, player); long long need = STASH_GOLD - gold; int need_toa = STASH_TOA - toa - own - adv; nwant = need_toa > 0 ? need_toa : 0;
        LOG("极乐世界：共享仓库补齐——共享页 %d 个，现有金币合计 %lld、赦免之证 共享页 %d 枚 + 角色身上 %d 枚 + 材料页 %d 枚 → 要补金币 %lld、赦免之证 %d 枚（发到背包）", n, gold, toa, own, adv, need > 0 ? need : 0, nwant);
        for (int k = 0; k < n && need > 0; k++) { int g = ((int (*)(uint64_t, int, int))(b + 0x5b9290))(pg[k], 15, 0), room = STASH_GOLD - g; if (room <= 0) continue; int add = need < room ? (int)need : room;
            ((void (*)(uint64_t, int, int, int))(b + 0x5bc1f0))(pg[k], 15, g + add, 0); need -= add; stash_gold_notify(b, G, player, id[k], g + add); LOG("极乐世界：共享页 %d（单位 id %u）金币 %d → %d", k + 1, id[k], g, g + add); }
        for (int i = 0; i < need_toa && i < STASH_TOA; i++) { uint64_t u = ((uint64_t (*)(uint64_t, uint64_t, uint32_t, int))(b + 0x7c0b20))(G, player, 0x20616f74, 2); if (!u) { LOG("【红】极乐世界：赦免之证生成失败"); continue; }
            uint64_t ip = *(uint64_t *)(u + 0x38); uint8_t pk[17] = { 0x16 }; uint32_t one = 1; memcpy(pk + 1, (void *)(uintptr_t)(u + 8), 4); if (ip) { memcpy(pk + 5, (void *)(uintptr_t)(ip + 0x10), 4); memcpy(pk + 9, (void *)(uintptr_t)(ip + 0x14), 4); } memcpy(pk + 13, &one, 4);
            *(uint32_t *)(G + 0x19050) = 1; feed(G, player, pk, 17); *(uint32_t *)(G + 0x19050) = 0; made[nmade++] = u; }
        st = 3; at = tq + 25;
        return; }
    st = 0; if (!n) return; stash_read(b, pg, n, &gold, &toa);
    int own = inv_toa(b, player), ground = 0; for (int i = 0; i < nmade; i++) if (*(uint32_t *)(made[i] + 0xc) == 3) ground++;
    int ok = gold >= STASH_GOLD && nmade == nwant;
    LOG("极乐世界：共享仓库补齐后——金币合计 %lld、赦免之证 共享页 %d 枚 + 角色身上 %d 枚（本次发 %d 枚，其中 %d 枚背包放不下留在脚边）%s", gold, toa, own, nmade, ground, ok ? "" : "【红】没补齐");
    if (ground) { void dm_game_notice(uint8_t *, const char *); const char *dm_str(uint8_t *, const char *); dm_game_notice(b, dm_str(b, "dmElyToaGround")); }
    if (ok) atomic_store(&g_topup_done, 1);
}
