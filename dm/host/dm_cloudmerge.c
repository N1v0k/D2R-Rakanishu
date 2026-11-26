

































#include "hle.h"
#include "game_addrs.h"
#include "dm_i18n.h"
#include <dirent.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

const char *dm_save_workdir(void); const char *dm_stash_name(uint8_t *b); int dm_request_save(void); uint64_t dm_fs_commit_count(void);
int dm_item_write(uint8_t *b, uint64_t it, uint8_t *buf); uint64_t dm_item_make_n(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out, int *bytes_out);
void *dm_memfind(const uint8_t *h, size_t hn, const uint8_t *n, size_t nn); void dm_game_notice(uint8_t *main_base, const char *text); int dm_ui_panel_state(void);
int dm_stash_pages(uint64_t G, uint64_t player, uint64_t pg[8], uint32_t id[8]); void dm_stash_gold_notify(uint8_t *b, uint64_t G, uint64_t player, uint32_t uid, int value); uint64_t dm_unit_inv(uint64_t u);
int dm_cloud_pack_each(const char *path, void (*cb)(const char *rel, const uint8_t *d, uint32_t n, void *ctx), void *ctx);

#if __has_include("stash_seed.h")
#include "stash_seed.h"
#define CM_HAVE_STASH_SEED 1
#endif
#define VB 1024
#define PAGE_GOLD 2500000
typedef struct { uint16_t len; uint8_t *bits; } cm_item;
typedef struct { cm_item *v; int n, cap; } cm_list;
static void cl_add(cm_list *l, const uint8_t *b, int len) { if (l->n == l->cap) { l->cap = l->cap ? l->cap * 2 : 64; l->v = realloc(l->v, sizeof *l->v * (size_t)l->cap); }
    l->v[l->n].len = (uint16_t)len; l->v[l->n].bits = malloc((size_t)len); memcpy(l->v[l->n].bits, b, (size_t)len); l->n++; }
static void cl_free(cm_list *l) { for (int i = 0; i < l->n; i++) free(l->v[i].bits); free(l->v); memset(l, 0, sizeof *l); }
static void cl_del(cm_list *l, int i) { free(l->v[i].bits); l->v[i] = l->v[--l->n]; }





static void key_of(const uint8_t *b, int len, uint8_t *k) { memcpy(k, b, (size_t)len); if (len < 7) return;
    k[1] &= (uint8_t)~0x20; k[4] &= 0x07; k[5] = 0; k[6] &= (uint8_t)~0x1f; }
static int key_eq(const cm_item *a, const cm_item *b) { if (a->len != b->len) return 0; uint8_t x[VB], y[VB]; key_of(a->bits, a->len, x); key_of(b->bits, b->len, y); return !memcmp(x, y, a->len); }
static int cl_count(const cm_list *l, const cm_item *it) { int c = 0; for (int i = 0; i < l->n; i++) c += key_eq(&l->v[i], it); return c; }



typedef struct { long long gold; int chron, items, bad; } d2i_sum;
static uint8_t *B;
static d2i_sum d2i_split(uint64_t G, const uint8_t *d, uint32_t n, cm_list *out) {
    d2i_sum s = {0};
    for (uint32_t o = 0; o + 68 <= n && !memcmp(d + o, "\x55\xaa\x55\xaa", 4); ) { uint32_t ln, g; memcpy(&ln, d + o + 16, 4); if (ln < 68 || o + ln > n) { s.bad++; break; }
        memcpy(&g, d + o + 12, 4);
        if (!memcmp(d + o + 64, "JM", 2)) { uint16_t c; memcpy(&c, d + o + 66, 2); s.gold += g; uint32_t p = o + 68;
            for (int i = 0; i < c; i++) { int used = 0, socks = 0; uint64_t it = out && p < o + ln ? dm_item_make_n(B, G, 0, d + p, (int)(o + ln - p), &socks, &used) : 0;
                if (!it || used <= 0 || used > VB) { if (out) s.bad++; if (it) ((void (*)(uint64_t, uint64_t))(B + GA_ITEM_FREE))(G, it); break; }
                uint8_t buf[VB]; int len = dm_item_write(B, it, buf); ((void (*)(uint64_t, uint64_t))(B + GA_ITEM_FREE))(G, it);
                if (len > 0) { cl_add(out, buf, len); s.items++; } else s.bad++;
                p += (uint32_t)used; } }
        else s.chron += (int)ln;
        o += ln; }
    return s;
}


static long long live_items(uint64_t G, uint64_t player, cm_list *out) {
    uint64_t pg[8]; uint32_t id[8]; int n = dm_stash_pages(G, player, pg, id); long long gold = 0;
    uint64_t pi = dm_unit_inv(player); int m = n;
    for (uint64_t e = pi ? *(uint64_t *)(pi + 0x68) : 0, k = 0; e && k < 16 && m < 8; e = *(uint64_t *)(e + 0x10), k++) if (*(uint8_t *)(e + 0xa) == 1) {
        uint32_t i = *(uint32_t *)e; uint64_t u = *(uint64_t *)(G + 0x2230 + 8 * (i & 0x7f)); while (u && *(uint32_t *)(u + 8) != i) u = *(uint64_t *)(u + 0x158); if (u) pg[m++] = u; }
    for (int k = 0; k < m; k++) { if (k < n) gold += ((int (*)(uint64_t, int, int))(B + GA_STAT_GET))(pg[k], 15, 0);
        uint64_t iv = dm_unit_inv(pg[k]); int c = 0;
        for (uint64_t it = iv ? *(uint64_t *)(iv + 0x10) : 0; it && c < 2048; c++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            if (*(uint32_t *)(it + 0xc) == 0) { uint8_t buf[VB]; int len = dm_item_write(B, it, buf); if (len > 0) cl_add(out, buf, len); }
            it = *(uint64_t *)(d + 0xb0); } }
    return gold;
}



typedef struct { int idx; uint32_t nid; uint16_t plen; uint8_t pb[VB]; uint64_t h0, seen; } cm_pend;
static pthread_mutex_t g_mu = PTHREAD_MUTEX_INITIALIZER;
static void plan_prune(const char *wd);
int dm_ser_state(void); uint64_t dm_fnv64(const uint8_t *d, size_t n);
static uint64_t char_h(const char *wd);
static char g_entry[600], g_stash[64];
static cm_list g_plan; static long long g_gold; static int g_have, g_dirty;
static cm_pend g_pend[32]; static int g_npend; static uint64_t g_gold_cno; static struct timespec g_gold_at; static long long g_gold_sent;
static void plan_path(char *p, size_t n, const char *wd, const char *entry) { snprintf(p, n, "%s/%s/plan-%s.bin", wd, entry, g_stash); }
static int plan_read(const char *path, cm_list *l, long long *gold) {
    FILE *f = fopen(path, "rb"); if (!f) return 0; uint8_t h[8]; int ok = fread(h, 1, 8, f) == 8 && !memcmp(h, "SWCM", 4) && h[4] == 1 && fread(gold, 8, 1, f) == 1;
    for (int i = 0, n = ok ? *(uint16_t *)(h + 6) : 0; ok && i < n; i++) { uint16_t len; uint8_t buf[VB]; ok = fread(&len, 2, 1, f) == 1 && len && len <= VB && fread(buf, 1, len, f) == len; if (ok) cl_add(l, buf, len); }
    fclose(f); if (!ok) cl_free(l); return ok ? 1 : -1;
}
static int plan_write(const char *path) {
    char t[1400]; snprintf(t, sizeof t, "%s.tmp", path); FILE *f = fopen(t, "wb"); if (!f) return -1;
    uint8_t h[8] = { 'S', 'W', 'C', 'M', 1, 0, 0, 0 }; *(uint16_t *)(h + 6) = (uint16_t)g_plan.n; int ok = fwrite(h, 1, 8, f) == 8 && fwrite(&g_gold, 8, 1, f) == 1;
    for (int i = 0; ok && i < g_plan.n; i++) ok = fwrite(&g_plan.v[i].len, 2, 1, f) == 1 && fwrite(g_plan.v[i].bits, 1, g_plan.v[i].len, f) == g_plan.v[i].len;
    ok = fclose(f) == 0 && ok; if (!ok || rename(t, path)) { remove(t); return -1; } return 0;
}



typedef struct { const char *want; const uint8_t *d; uint32_t n; uint8_t *copy; } pick_ctx;
typedef struct { const char *want; uint8_t *copy; uint32_t n; } vpick_ctx;
static void vpick_cb(const char *rel, const uint8_t *d, uint32_t n, void *ctx) { vpick_ctx *c = ctx; char a[200]; snprintf(a, sizeof a, "%s.swvault", c->want);
    if (!c->copy && !strcmp(rel, a)) { c->copy = malloc(n ? n : 1); memcpy(c->copy, d, n); c->n = n; } }
static int vault_split(const uint8_t *d, uint32_t n, cm_list *out) {
    if (!d || n < 8 || memcmp(d, "SWVT", 4) || d[4] != 1 || d[5] != 0) return -1; uint16_t c; memcpy(&c, d + 6, 2); uint32_t o = 8;
    for (int i = 0; i < c; i++) { if (o + 6 > n) return -1; uint16_t len; memcpy(&len, d + o + 4, 2); if (!len || len > VB || o + 6 + len > n) return -1; cl_add(out, d + o + 6, len); o += 6u + len; }
    return c;
}
static void pick_cb(const char *rel, const uint8_t *d, uint32_t n, void *ctx) { pick_ctx *c = ctx; char a[200], b2[200];
    snprintf(a, sizeof a, "%s.d2i", c->want); snprintf(b2, sizeof b2, "%s/%s.d2i", c->want, c->want);
    if (!c->copy && (!strcmp(rel, a) || !strcmp(rel, b2))) { c->copy = malloc(n ? n : 1); memcpy(c->copy, d, n); c->n = n; } }
static int cmp_name(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }
static int entry_pick(const char *wd) {
    char ib[1200]; snprintf(ib, sizeof ib, "%s/_cloudinbox", wd); DIR *dp = opendir(ib); if (!dp) return 0;

    char (*nm)[128] = NULL; int n = 0, cap = 0; struct dirent *e;
    while ((e = readdir(dp))) { if (e->d_name[0] == '.' || strlen(e->d_name) >= 128) continue;
        if (n == cap) { int c2 = cap ? cap * 2 : 64; char (*m)[128] = realloc(nm, (size_t)c2 * 128); if (!m) break; nm = m; cap = c2; }
        strlcpy(nm[n++], e->d_name, 128); }
    closedir(dp); if (n) qsort(nm, (size_t)n, 128, cmp_name);
    for (int i = 0; i < n; i++) { char ent[300], pp[1500]; snprintf(ent, sizeof ent, "_cloudinbox/%s", nm[i]); plan_path(pp, sizeof pp, wd, ent);
        cm_list l = {0}; long long gold = 0; int r = plan_read(pp, &l, &gold);
        if (r < 0) { LOG("【红】iCloud 并回：清单 %s 坏了，跳过这个条目（文件不动）", pp); continue; }
        if (r == 1) { if (!l.n && gold <= 0) { cl_free(&l); continue; }
            strlcpy(g_entry, ent, sizeof g_entry); cl_free(&g_plan); g_plan = l; g_gold = gold; g_have = 1; plan_prune(wd); free(nm); return 1; }
        char mp[1500]; snprintf(mp, sizeof mp, "%s/%s/mine.pack", wd, ent); pick_ctx c = { g_stash, 0, 0, 0 };
        if (dm_cloud_pack_each(mp, pick_cb, &c) < 0) { LOG("【红】iCloud 并回：%s 读不了，跳过", mp); continue; }
        vpick_ctx vc = { g_stash, 0, 0 }; if (!c.copy) dm_cloud_pack_each(mp, vpick_cb, &vc);
        if (!c.copy && !vc.copy) continue;
        free(c.copy); free(vc.copy); strlcpy(g_entry, ent, sizeof g_entry); g_have = 0; free(nm); return 1; }
    free(nm); return 0;
}
static int file_blob(const char *p, uint8_t **d, size_t *n);


static int plan_compute(uint64_t G, uint64_t player, const char *wd) {
    char mp[1500], bp[1500]; snprintf(mp, sizeof mp, "%s/%s/mine.pack", wd, g_entry); snprintf(bp, sizeof bp, "%s/%s/base.pack", wd, g_entry);
    pick_ctx cm = { g_stash, 0, 0, 0 }, cb = { g_stash, 0, 0, 0 }; dm_cloud_pack_each(mp, pick_cb, &cm); int hbp = dm_cloud_pack_each(bp, pick_cb, &cb) >= 0, hb = hbp && cb.copy;
    vpick_ctx vm = { g_stash, 0, 0 }, vb = { g_stash, 0, 0 }; dm_cloud_pack_each(mp, vpick_cb, &vm); if (hbp) dm_cloud_pack_each(bp, vpick_cb, &vb);
    if (!cm.copy && !vm.copy) { free(cb.copy); free(vb.copy); return -1; }
    cm_list mine = {0}, base = {0}, cloud = {0}; d2i_sum sm = cm.copy ? d2i_split(G, cm.copy, cm.n, &mine) : (d2i_sum){0}, sb = {0};
    if (hb) sb = d2i_split(G, cb.copy, cb.n, &base);
    long long cg = live_items(G, player, &cloud);


    int nvm = vm.copy ? vault_split(vm.copy, vm.n, &mine) : 0, nvb = vb.copy ? vault_split(vb.copy, vb.n, &base) : 0, nvc = 0;
    { char vp[1400]; uint8_t *d; size_t n; snprintf(vp, sizeof vp, "%s/%s.swvault", wd, g_stash); if (file_blob(vp, &d, &n)) { nvc = vault_split(d, (uint32_t)n, &cloud); free(d); } }
    if (nvm < 0 || nvb < 0 || nvc < 0) LOG("【红】iCloud 并回：编年史仓库侧文件坏了（本机 %d、基准 %d、云端 %d；-1 = 坏），坏的那份不算", nvm, nvb, nvc);
    if (vb.copy && !cb.copy) hb = 1;


    d2i_sum sc = {0}; { char cp[1400], cp2[1400]; snprintf(cp, sizeof cp, "%s/%s/%s.d2i", wd, g_stash, g_stash); snprintf(cp2, sizeof cp2, "%s/%s.d2i", wd, g_stash);
        FILE *f = fopen(cp, "rb"); if (!f) f = fopen(cp2, "rb");
        if (f) { fseek(f, 0, SEEK_END); long L = ftell(f); fseek(f, 0, SEEK_SET); uint8_t *m = L > 0 && L < (16 << 20) ? malloc((size_t)L) : NULL; if (m && fread(m, 1, (size_t)L, f) == (size_t)L) sc = d2i_split(G, m, (uint32_t)L, NULL); free(m); fclose(f); } }
    int use_base = hb && !sb.bad && sc.chron >= sb.chron;
    long long seed = 0;
#ifdef CM_HAVE_STASH_SEED
    { d2i_sum ss = d2i_split(G, g_stash_seed, sizeof g_stash_seed, NULL); seed = ss.gold; }
#endif
    cl_free(&g_plan);
    for (int i = 0; i < mine.n; i++) { cm_item *it = &mine.v[i]; int dup = 0; for (int j = 0; j < i; j++) dup |= key_eq(&mine.v[j], it); if (dup) continue;
        int have = cl_count(&mine, it), ref = cl_count(use_base ? &base : &cloud, it);
        for (int k = 0; k < have - ref; k++) cl_add(&g_plan, it->bits, it->len); }
    long long g = sm.gold - (use_base ? sb.gold : seed); g_gold = g > 0 ? g : 0;

    { char np[1500]; struct stat st; snprintf(np, sizeof np, "%s/%s/nogold-%s", wd, g_entry, g_stash); if (!stat(np, &st) && g_gold) { LOG("iCloud 并回：%s 带 nogold 标记 → 金币 %lld 不还，只还物品", g_entry, g_gold); g_gold = 0; } }
    LOG("iCloud 并回：%s 的 %s —— 本机 %d 件、金币 %lld；云端 %d 件、金币 %lld、编年史 %d 字节 → 要还 %d 件、金币 %lld", g_entry, g_stash, mine.n, sm.gold, cloud.n, cg, sc.chron, g_plan.n, g_gold);
    if (use_base) LOG("iCloud 并回：按基准（%d 件、金币 %lld）只还本机新增", base.n, sb.gold);
    else if (hb) LOG("iCloud 并回：基准不能用（云端编年史比基准短 / 基准坏）→ 按并集，金币比预置 %lld 多出的才还", seed);
    else LOG("iCloud 并回：没有基准 → 按并集，金币比预置 %lld 多出的才还", seed);
    if (sm.bad) LOG("【红】iCloud 并回：本机那份有 %d 处拆不开的物品（拆开的照还）", sm.bad);
    if (nvm > 0) LOG("iCloud 并回：本机那份编年史仓库 %d 件、基准 %d 件、云端 %d 件，一起按同一件比", nvm, nvb, nvc);
    cl_free(&mine); cl_free(&base); cl_free(&cloud); free(cm.copy); free(cb.copy); free(vm.copy); free(vb.copy);
    g_have = 1; g_dirty = 1; plan_prune(wd); return 0;
}







#ifdef DM_PLAYER_BUILD
static void cm_kill(int at) { (void)at; }
#else
static void cm_kill(int at) { static int k = -1; if (k < 0) { const char *e = getenv("DM_TEST_KILL_AT"); k = e && !strncmp(e, "cm:", 3) ? atoi(e + 3) : 0; }
    if (k == at) { LOG("DM_TEST_KILL_AT：iCloud 并回时刻 %d，对自己发 SIGKILL", at); kill(getpid(), SIGKILL); for (;;) pause(); } }
#endif


static int take_one(uint64_t G, uint64_t player, int idx) {
    cm_item *u = &g_plan.v[idx]; uint64_t pp = *(uint64_t *)(player + 0x38), room = pp ? *(uint64_t *)(pp + 0x20) : 0; if (!room) return 0;
    int used = 0; uint64_t nu = dm_item_make_n(B, G, room, u->bits, u->len, NULL, &used);
    if (!nu) { static int said; if (!said++) LOG("【红】iCloud 并回：清单第 %d 件建不出物品，留在清单里不放", idx + 1); return -1; }
    uint64_t inv = *(uint64_t *)(player + 0x90); uint32_t cls = *(uint32_t *)(player + 4), fx = 0, fy = 0;
    if (!inv || cls > 7 || !((int (*)(uint64_t, uint64_t, int, uint32_t *, uint32_t *, int))(B + GA_INV_FIND_FREE))(inv, nu, *(int32_t *)(B + GA_CLASS_INV_GRID + cls * 8 + 4), &fx, &fy, 0)) {
        ((void (*)(uint64_t, uint64_t))(B + GA_ITEM_FREE))(G, nu); return 0; }
    uint64_t nd = *(uint64_t *)(nu + 0x10), ip = *(uint64_t *)(nu + 0x38); if (!nd || !ip) { ((void (*)(uint64_t, uint64_t))(B + GA_ITEM_FREE))(G, nu); return 0; }
    *(uint8_t *)(nd + 0x55) = 0; *(int32_t *)(ip + 0x10) = (int32_t)fx; *(int32_t *)(ip + 0x14) = (int32_t)fy;
    uint64_t one = nu, ox = 0; ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(B + GA_ITEM_PLACE_ON_LOAD))(G, player, &one, &ox);
    if (!*(uint64_t *)(nd + 0xa0)) { ((void (*)(uint64_t, uint64_t))(B + GA_ITEM_FREE))(G, nu); LOG("【红】iCloud 并回：清单第 %d 件放进背包没成（格 %u,%u）", idx + 1, fx, fy); return 0; }
    ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, uint64_t))(B + GA_ITEM_NOTIFY_CLIENT))(G, player, nu, 1, 2, 0, 0, 0, (uint64_t)(fx & 0xffff) | (uint64_t)fy << 16);
    cm_pend *p = &g_pend[g_npend++]; memset(p, 0, sizeof *p); p->idx = idx; p->nid = *(uint32_t *)(nu + 8); { const char *wd = dm_save_workdir(); p->h0 = wd && dm_ser_state() == 0 ? char_h(wd) : 0; }
    int len = dm_item_write(B, nu, p->pb); if (len > 0) p->plen = (uint16_t)len; else { p->plen = u->len; memcpy(p->pb, u->bits, u->len); LOG("【红】iCloud 并回：放好后的位流写不出，用清单那串作证据"); }
    LOG("iCloud 并回：清单第 %d 件（%d 字节）放到背包格 %u,%u（新 id %u），等落盘证明", idx + 1, u->len, fx, fy, p->nid);
    cm_kill(1);
    { cm_item a = { p->plen, p->pb }; if (!key_eq(&a, u)) LOG("【红】iCloud 并回：清单第 %d 件放好后的那一串与清单去位置键不同（落盘证明照用放好后的那一串）", idx + 1); }
    return 1;
}
static int pend_has(int idx) { for (int i = 0; i < g_npend; i++) if (g_pend[i].idx == idx) return 1; return 0; }
static void add_gold(uint64_t G, uint64_t player) {
    uint64_t pg[8]; uint32_t id[8]; int n = dm_stash_pages(G, player, pg, id); long long left = g_gold, put = 0;
    for (int k = 0; k < n && left > 0; k++) { int g = ((int (*)(uint64_t, int, int))(B + GA_STAT_GET))(pg[k], 15, 0), room = PAGE_GOLD - g; if (room <= 0) continue; int add = left < room ? (int)left : room;
        ((void (*)(uint64_t, int, int, int))(B + GA_STAT_SET))(pg[k], 15, g + add, 0); dm_stash_gold_notify(B, G, player, id[k], g + add); left -= add; put += add;
        LOG("iCloud 并回：共享页 %d（单位 id %u）金币 %d → %d", k + 1, id[k], g, g + add); }
    if (put) { g_gold_sent = put; clock_gettime(CLOCK_REALTIME, &g_gold_at); g_gold_cno = dm_fs_commit_count(); }
    if (left) LOG("iCloud 并回：共享页金币都满了，还有 %lld 金币留到下次", left);
}



static int file_blob(const char *p, uint8_t **d, size_t *n) { FILE *f = fopen(p, "rb"); *d = NULL; *n = 0; if (!f) return 0;
    fseek(f, 0, SEEK_END); long L = ftell(f); fseek(f, 0, SEEK_SET); *d = L > 0 && L < (16 << 20) ? malloc((size_t)L) : NULL; *n = *d ? fread(*d, 1, (size_t)L, f) : 0; fclose(f); return *n > 0; }
static char g_char[64];
int dm_save_blobs(const char *wd, const char *stash, dm_blob *out, int max); void dm_blobs_free(dm_blob *bl, int nb);
int dm_blobs_have(const dm_blob *bl, int nb, const uint8_t *bits, int len); int dm_bits_simple(const uint8_t *bits, int len);




static uint64_t char_h(const char *wd) { char p[1400]; uint8_t *d; size_t n; snprintf(p, sizeof p, "%s/%s/%s.d2s", wd, g_char, g_char); uint64_t h = file_blob(p, &d, &n) ? dm_fnv64(d, n) : 0; free(d); return h; }
static int proof_blobs(const char *wd, dm_blob *bl, int max) {
    int nb = dm_save_blobs(wd, g_stash, bl, max - 3); char p[1400]; uint8_t *d; size_t n;
    const char *ext[2] = { "swpage", "swpouch" };
    for (int k = 0; k < 2 && nb < max; k++) { snprintf(p, sizeof p, "%s/%s/%s.%s", wd, g_char, g_char, ext[k]); if (file_blob(p, &d, &n)) { bl[nb].d = d; bl[nb].n = n; nb++; } else free(d); }
    if (nb < max && g_stash[0]) { snprintf(p, sizeof p, "%s/%s.swvault", wd, g_stash); if (file_blob(p, &d, &n)) { bl[nb].d = d; bl[nb].n = n; nb++; } else free(d); }
    return nb; }


int dm_char_hardcore(const char *wd, const char *name) {
    char p[1400]; uint8_t h[0x28]; snprintf(p, sizeof p, "%s/%s/%s.d2s", wd, name, name); FILE *f = fopen(p, "rb"); if (!f) return -1;
    size_t n = fread(h, 1, sizeof h, f); fclose(f); if (n < sizeof h || memcmp(h, "\x55\xaa\x55\xaa", 4)) return -1;
    uint32_t ver; memcpy(&ver, h + 4, 4); return (h[ver >= 0x69 ? 0x14 : 0x24] >> 2) & 1; }



static int dedup_blobs(const char *wd, dm_blob **out) {
    int max = 16; DIR *dp = opendir(wd); struct dirent *e; char p[1400]; uint8_t *d; size_t n;
    while (dp && readdir(dp)) max += 3;
    dm_blob *bl = *out = calloc((size_t)max, sizeof *bl); if (!bl) { if (dp) closedir(dp); return 0; }
    if (dp) rewinddir(dp);
    int nb = proof_blobs(wd, bl, max), hc = strstr(g_stash, "HardCore") != NULL;
    while (dp && nb < max && (e = readdir(dp))) { if (e->d_name[0] == '.' || e->d_name[0] == '_' || !strcmp(e->d_name, g_char) || dm_char_hardcore(wd, e->d_name) != hc) continue;
        for (int k = 0; k < 2 && nb < max; k++) { snprintf(p, sizeof p, "%s/%s/%s.%s", wd, e->d_name, e->d_name, k ? "swpouch" : "swpage"); if (file_blob(p, &d, &n)) { bl[nb].d = d; bl[nb].n = n; nb++; } else free(d); } }
    if (dp) closedir(dp);
    return nb; }


static void plan_prune(const char *wd) {
    dm_blob *bl = NULL; int nb = dedup_blobs(wd, &bl), drop = 0;
    for (int i = 0; i < g_plan.n; ) { cm_item *it = &g_plan.v[i]; if (!dm_bits_simple(it->bits, it->len) && dm_blobs_have(bl, nb, it->bits, it->len)) { cl_del(&g_plan, i); drop++; } else i++; }
    dm_blobs_free(bl, nb); free(bl);
    if (drop) { g_dirty = 1; LOG("iCloud 并回：清单里 %d 件已经在存档里（以前还过、没划掉）→ 划掉不再还，还剩 %d 件", drop, g_plan.n); } }
static void cm_hook(const char *wd) {
    pthread_mutex_lock(&g_mu);
    if (g_entry[0] && g_have && (g_npend || g_gold_sent || g_dirty)) {
        uint64_t cno = dm_fs_commit_count() + 1; char p[1400];
        if (g_gold_sent && cno - 1 > g_gold_cno) cm_kill(4);
        if (g_npend) { dm_blob bl[70]; int nb = proof_blobs(wd, bl, 70); uint64_t h = char_h(wd);
            for (int i = 0; i < g_npend; ) { cm_pend *q = &g_pend[i]; int at = 0;
                for (int k = 0; k < nb && !at; k++) if (dm_memfind(bl[k].d, bl[k].n, q->pb, q->plen)) at = 1;




                if (!at && !q->seen && q->h0 && h && h != q->h0) q->seen = cno;
                int fresh = !at && q->seen && cno > q->seen;
                if (!at && !fresh) { i++; continue; }
                cm_kill(2);
                int k = q->idx < g_plan.n ? q->idx : -1;
                if (k >= 0) { for (int j = 0; j < g_npend; j++) if (g_pend[j].idx == g_plan.n - 1) g_pend[j].idx = k; cl_del(&g_plan, k); }
                LOG("iCloud 并回：提交前核对 #%llu —— 新 id %u %s → 从清单划掉，还剩 %d 件", (unsigned long long)cno, q->nid, at ? "已在存档里" : "放好之后游戏已序列化并落盘", g_plan.n);
                g_pend[i] = g_pend[--g_npend]; g_dirty = 1; }
            dm_blobs_free(bl, nb); }
        if (g_gold_sent) { char a[1400], b2[1400]; struct stat st; snprintf(a, sizeof a, "%s/%s/%s.d2i", wd, g_stash, g_stash); snprintf(b2, sizeof b2, "%s/%s.d2i", wd, g_stash);
            if ((!stat(a, &st) || !stat(b2, &st)) && (st.st_mtimespec.tv_sec > g_gold_at.tv_sec || (st.st_mtimespec.tv_sec == g_gold_at.tv_sec && st.st_mtimespec.tv_nsec > g_gold_at.tv_nsec))) { g_gold -= g_gold_sent; if (g_gold < 0) g_gold = 0; g_gold_sent = 0; g_dirty = 1;
                LOG("iCloud 并回：提交前核对 #%llu —— 共享仓库档在加金币之后重写过 → 金币划掉，还剩 %lld", (unsigned long long)cno, g_gold); } }
        if (g_dirty) { plan_path(p, sizeof p, wd, g_entry);
            if (plan_write(p) == 0) { g_dirty = 0; cm_kill(3); LOG("iCloud 并回：清单随这次提交落盘（还剩 %d 件、金币 %lld）%s", g_plan.n, g_gold, !g_plan.n && g_gold <= 0 ? "—— 这一类仓库并完" : ""); }
            else { void dm_fs_save_poison(int e); dm_fs_save_poison(5); LOG("【红】iCloud 并回：清单写不进 %s → 这次存档提交拒绝，之后重试", p); } }
    }
    pthread_mutex_unlock(&g_mu);
}
__attribute__((constructor)) static void cm_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[4] = cm_hook; }



static char g_notice[256]; static _Atomic int g_notice_req;
void dm_cloudmerge_srv_tick(uint8_t *b, uint64_t G, uint64_t player) {
    static uint64_t lp; static int none;
    if (!player) { if (lp) { pthread_mutex_lock(&g_mu); g_entry[0] = 0; g_have = 0; g_npend = 0; g_gold_sent = 0; cl_free(&g_plan); pthread_mutex_unlock(&g_mu); } lp = 0; return; }
    B = b; uint32_t tq = *(uint32_t *)(G + 0x170); const char *wd = dm_save_workdir(); if (!wd) return;
    if (player != lp) { lp = player; none = 0; const char *sn = dm_stash_name(b); uint64_t pd = *(uint64_t *)(player + 0x10);
        pthread_mutex_lock(&g_mu); g_entry[0] = 0; g_have = 0; g_npend = 0; g_gold_sent = 0; strlcpy(g_char, pd ? (const char *)pd : "", sizeof g_char);
        strlcpy(g_stash, sn ? sn : "", sizeof g_stash); if (!sn || !entry_pick(wd)) none = 1;
        else LOG("iCloud 并回：进局 —— 收件箱条目 %s 有 %s 要并（清单%s）", g_entry, g_stash, g_have ? "已有" : "开箱时算");
        pthread_mutex_unlock(&g_mu); }
    { int dm_itemjob_owner(void); if (dm_itemjob_owner()) return; }
    int open = (dm_ui_panel_state() & 1) != 0, near = ((int (*)(uint64_t, uint64_t, int, int))(b + GA_OBJECT_NEAR))(G, player, 0x10b, 0x32) & 1;


    { static int t = -1; if (t < 0) t = getenv("DM_TEST_CM_KEYS") != NULL; const char *sn = dm_stash_name(b);
      if (t == 1 && open && near && sn) { t = 0; char cp[1400]; snprintf(cp, sizeof cp, "%s/%s/%s.d2i", wd, sn, sn); uint8_t *d; size_t n; cm_list disk = {0}, live = {0};
          if (!file_blob(cp, &d, &n)) { snprintf(cp, sizeof cp, "%s/%s.d2i", wd, sn); file_blob(cp, &d, &n); }
          d2i_sum s = d ? d2i_split(G, d, (uint32_t)n, &disk) : (d2i_sum){0}; long long lg = live_items(G, player, &live); int hit = 0, miss = 0, raw = 0;
          for (int i = 0; i < disk.n; i++) { int c1 = cl_count(&disk, &disk.v[i]), c2 = cl_count(&live, &disk.v[i]); if (c1 == c2) hit++; else miss++;
              for (int j = 0; j < live.n; j++) raw += live.v[j].len == disk.v[i].len && !memcmp(live.v[j].bits, disk.v[i].bits, disk.v[i].len); }
          LOG("测试 DM_TEST_CM_KEYS：%s 盘上 %d 件（拆不开 %d、金币 %lld、编年史 %d 字节），服务端 %d 件（金币 %lld）；去位置键个数一致 %d 件、不一致 %d 件；原串逐字节相同 %d 件",
              sn, disk.n, s.bad, s.gold, s.chron, live.n, lg, hit, miss, raw);
          free(d); cl_free(&disk); cl_free(&live); } }
    if (none || !open || !near || dm_ser_state() == 1) return;
    static uint32_t last; if (tq - last < 25) return; last = tq;
    pthread_mutex_lock(&g_mu); int saved = 0, full = 0;
    if (!g_have) plan_compute(G, player, wd), saved = 1;
    if (g_gold > 0 && !g_gold_sent) { add_gold(G, player); saved |= g_gold_sent != 0; }
    for (int i = 0, miss = 0; i < g_plan.n && g_npend < 32 && miss < 8; i++) { if (pend_has(i)) continue; int r = take_one(G, player, i); if (r > 0) saved = 1; else if (r == 0) { full = 1; miss++; } }
    int left = g_plan.n - g_npend, done = !g_plan.n && g_gold <= 0 && !g_npend && !g_dirty, pend = g_npend || g_gold_sent || g_dirty;

    int imp = strstr(g_entry, "-import") != NULL;
    static int said; if (saved && !said) { said = 1; snprintf(g_notice, sizeof g_notice, "%s", dm_str(b, imp ? "dmImportMergeBack" : "dmCloudMergeBack")); atomic_store(&g_notice_req, 1); }
    if (full && left) { static uint32_t fl; if (!fl || tq - fl > 750) { fl = tq; snprintf(g_notice, sizeof g_notice, "%s", dm_str(b, imp ? "dmImportMergeFull" : "dmCloudMergeFull")); atomic_store(&g_notice_req, 1); } }
    if (done) { LOG("iCloud 并回：%s 的 %s 并完", g_entry, g_stash); none = !entry_pick(wd); said = 0; }
    pthread_mutex_unlock(&g_mu);
    { static uint32_t lastreq; if (saved || (pend && tq - lastreq >= 50)) { lastreq = tq; dm_request_save(); } }
    if (atomic_exchange(&g_notice_req, 0)) { dm_game_notice(b, g_notice); LOG("iCloud 并回：系统提示「%s」", g_notice); }
}
