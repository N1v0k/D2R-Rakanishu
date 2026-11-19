

































#include <time.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <errno.h>
#include <pthread.h>
#include "dm.h"

#define VMAX 1024
#define VBYTES 1024
typedef struct { uint8_t kind; uint16_t idx, len; uint8_t *data; } ventry;
static ventry g_v[VMAX]; static int g_nv; static _Atomic int g_loaded, g_vnone; static char g_vname[64];
static ventry g_d[VMAX]; static int g_nd, g_ddirty;
static pthread_mutex_t g_vmu = PTHREAD_MUTEX_INITIALIZER;


typedef struct { int put; uint8_t kind; uint16_t idx, len; uint8_t *data; int n; uint64_t made, t; } vpend;
enum { VPEND = 64 };
static vpend g_pend[VPEND]; static int g_npend, g_pend_cap = VPEND;
uint64_t dm_fs_commit_count(void); void dm_fs_save_poison(int e);
static _Atomic int g_vretry;
static double now_s(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec / 1e9; }
static void pend_add(int put, int kind, int idx, const uint8_t *bits, int len) {
    if (g_npend >= VPEND || len <= 16) { LOG("【红】编年史仓库：待落盘已满 %d 条，这次存取不记（不该到这里：服务端拍在队列快满时先不做存取，见 dm_vault_tick）", VPEND); return; }
    vpend *p = &g_pend[g_npend]; p->data = malloc((size_t)len); if (!p->data) return; memcpy(p->data, bits, (size_t)len);
    p->put = put; p->kind = (uint8_t)kind; p->idx = (uint16_t)idx; p->len = (uint16_t)len; p->n = len - 8 > 48 ? 48 : len - 8; p->made = dm_fs_commit_count(); p->t = (uint64_t)now_s(); g_npend++; }
static _Atomic uint32_t g_req_put; static _Atomic uint32_t g_req_take;
static _Atomic int g_full_snd;
int dm_vault_take_full_sound(void) { return atomic_exchange(&g_full_snd, 0); }
void dm_vault_request_deposit(uint32_t item_id) { atomic_store(&g_req_put, item_id); }
int dm_vault_put_pending(void) { return atomic_load(&g_req_put) != 0; }
int dm_vault_ready(void) { return g_loaded; }
void dm_vault_request_withdraw(int kind, int idx) { atomic_store(&g_req_take, 1u << 24 | (uint32_t)(kind & 0xff) << 16 | (uint32_t)(idx & 0xffff)); }
static _Atomic uint64_t g_take_at;
void dm_vault_request_withdraw_at(int kind, int idx, int fx100, int fy100, int cols, int rows) {
    atomic_store(&g_take_at, 1ull << 63 | (uint64_t)(uint16_t)fx100 << 32 | (uint64_t)(uint16_t)fy100 << 16 | (uint64_t)(cols & 0xff) << 8 | (uint64_t)(rows & 0xff)); dm_vault_request_withdraw(kind, idx); }
static _Atomic int g_vgen;
int dm_vault_gen(void) { return atomic_load(&g_vgen); }

const char *dm_save_workdir(void); int dm_request_save(void); uint64_t dm_diag_srv_item(uint32_t id);
int dm_item_write(uint8_t *b, uint64_t it, uint8_t *buf); uint64_t dm_item_make(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out);
void *dm_memfind(const uint8_t *h, size_t hn, const uint8_t *n, size_t nn);

static const char *vault_name(uint8_t *b) {
    uint8_t mode = *(uint8_t *)(b + 0x2127c48); int hc = (*(uint16_t *)(b + 0x21278d5) >> 2) & 1;
    if (mode == 3) return hc ? "ModernSharedStashHardCoreV2" : "ModernSharedStashSoftCoreV2";
    if (mode == 2) return hc ? "SharedStashHardCoreV2" : "SharedStashSoftCoreV2";
    return NULL;
}
const char *dm_stash_name(uint8_t *b) { return vault_name(b); }
static void vault_clear(void) { for (int i = 0; i < g_nv; i++) free(g_v[i].data); g_nv = 0; for (int i = 0; i < g_nd; i++) free(g_d[i].data); g_nd = 0; g_ddirty = 0;
    for (int i = 0; i < g_npend; i++) free(g_pend[i].data); g_npend = 0; }
int dm_save_blobs(const char *wd, const char *stash, dm_blob *out, int max); int dm_blobs_have(const dm_blob *bl, int nb, const uint8_t *bits, int len); void dm_blobs_free(dm_blob *bl, int nb);
static int dfind(int kind, int idx) { for (int i = 0; i < g_nd; i++) if (g_d[i].kind == kind && g_d[i].idx == idx) return i; return -1; }
static int vault_load(const char *name) {
    vault_clear(); const char *wd = dm_save_workdir(); if (!wd) return -1;
    char p[1200]; snprintf(p, sizeof p, "%s/%s.swvault", wd, name); FILE *f = fopen(p, "rb");
    if (!f) { LOG("编年史仓库：%s 还没有侧文件（空仓库）", name); return 0; }
    uint8_t h[8]; int n = 0, bad = 0;
    if (fread(h, 1, 8, f) != 8 || memcmp(h, "SWVT", 4) || *(uint16_t *)(h + 4) != 1) bad = 1;
    else { n = *(uint16_t *)(h + 6);
        for (int i = 0; i < n && !bad; i++) { uint8_t e[6]; if (fread(e, 1, 6, f) != 6) { bad = 1; break; } uint16_t len = *(uint16_t *)(e + 4);
            if (!len || len > VBYTES || g_nv >= VMAX) { bad = 1; break; } ventry *v = &g_v[g_nv]; v->kind = e[0]; v->idx = *(uint16_t *)(e + 2); v->len = len; v->data = malloc(len);
            if (!v->data || fread(v->data, 1, len, f) != len) { free(v->data); bad = 1; break; } g_nv++; } }
    fclose(f);
    if (bad) LOG("【红】编年史仓库：%s 读到第 %d 条坏了，只用前面读好的 %d 条（文件不动）", p, g_nv, g_nv);


    dm_blob bl[64]; int nb = dm_save_blobs(wd, name, bl, 64), drop = 0;
    for (int i = 0; i < g_nv; ) { if (dm_blobs_have(bl, nb, g_v[i].data, g_v[i].len)) { LOG("【红】编年史仓库：%s 第 %d 件（%s %d）的字节已在原版存档里 → 从编年史去掉（防复制）", name, i, g_v[i].kind ? "暗金" : "套装件", g_v[i].idx); free(g_v[i].data); g_v[i] = g_v[--g_nv]; drop++; } else i++; }
    dm_blobs_free(bl, nb);
    for (int i = 0; i < g_nv; i++) { g_d[i] = g_v[i]; g_d[i].data = malloc(g_v[i].len); if (g_d[i].data) memcpy(g_d[i].data, g_v[i].data, g_v[i].len); else g_d[i].len = 0; } g_nd = g_nv; g_ddirty = drop > 0;
    if (bad) return -2;
    LOG("编年史仓库：%s 读入 %d 件（对原版存档去重 %d 件，比对 %d 份存档）", name, g_nv, drop, nb); return g_nv;
}
static int vault_save(const char *wd, const char *name) {
    if (!wd) return -1;
#ifndef DM_PLAYER_BUILD
    { static int nf = -1; if (nf < 0) { const char *e = getenv("DM_TEST_VAULT_SAVE_FAIL"); nf = e ? atoi(e) : 0; } if (nf > 0) { nf--; LOG("【红】编年史仓库：测试 DM_TEST_VAULT_SAVE_FAIL 这次当作写失败"); return -1; } }
#endif
    char p[1200], t[1210]; snprintf(p, sizeof p, "%s/%s.swvault", wd, name); snprintf(t, sizeof t, "%s.tmp", p);
    FILE *f = fopen(t, "wb"); if (!f) return -1;
    uint8_t h[8] = { 'S', 'W', 'V', 'T', 1, 0, 0, 0 }; int nw = 0; for (int i = 0; i < g_nd; i++) nw += g_d[i].len > 0; *(uint16_t *)(h + 6) = (uint16_t)nw; int ok = fwrite(h, 1, 8, f) == 8;
    for (int i = 0; i < g_nd && ok; i++) { if (!g_d[i].len) continue; uint8_t e[6] = { g_d[i].kind, 0 }; *(uint16_t *)(e + 2) = g_d[i].idx; *(uint16_t *)(e + 4) = g_d[i].len;
        ok = fwrite(e, 1, 6, f) == 6 && fwrite(g_d[i].data, 1, g_d[i].len, f) == g_d[i].len; }
    ok = fclose(f) == 0 && ok; if (!ok || rename(t, p)) { remove(t); LOG("【红】编年史仓库：写 %s 失败", p); return -1; }
    return 0;
}
static int vfind(int kind, int idx) { for (int i = 0; i < g_nv; i++) if (g_v[i].kind == kind && g_v[i].idx == idx) return i; return -1; }




static _Atomic int g_desc_req, g_desc_ready; static char g_desc[4096];
void dm_vault_request_desc(int kind, int idx) { atomic_store(&g_desc_ready, 0); atomic_store(&g_desc_req, 1 << 24 | (kind & 0xff) << 16 | (idx & 0xffff)); }
int dm_vault_take_desc(char *out, size_t n) { if (!atomic_exchange(&g_desc_ready, 0)) return 0; strlcpy(out, g_desc, n); return 1; }
static void vault_desc(uint8_t *b, uint64_t G, uint64_t player, uint32_t req) {
    int k = vfind((int)(req >> 16 & 0xff), (int)(req & 0xffff)); if (k < 0) return;
    uint64_t nu = dm_item_make(b, G, 0, g_v[k].data, g_v[k].len, NULL); if (!nu) { LOG("【红】编年史仓库：说明——位流建不出物品"); return; }
    typedef struct { char *p; uint64_t len, cap; } gstr_t; struct { uint64_t n; uint8_t has; } cnt = { 0, 0 };
    gstr_t r = ((gstr_t (*)(uint64_t, uint64_t, uint64_t, int, int, int, void *, void *))(b + 0x374f00))(player, player, nu, 0, 1, 0, NULL, &cnt);
    size_t n = r.p && r.len < sizeof g_desc ? r.len : (r.p ? sizeof g_desc - 1 : 0); if (n) memcpy(g_desc, r.p, n); g_desc[n] = 0;
    if (r.p && !(r.cap >> 63) && (*(volatile uint8_t *)(b + 0x3211700) & 1)) { uint64_t a = *(uint64_t *)(b + 0x32116f8); ((void (*)(uint64_t, void *))(*(uint64_t *)(*(uint64_t *)a + 0x28)))(a, r.p); }
    ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu);
    if (n) atomic_store(&g_desc_ready, 1);
}
int dm_vault_each(void (*cb)(int kind, int idx, void *arg), void *arg) { if (pthread_mutex_trylock(&g_vmu)) return -1; int on = g_loaded, r = on || g_vnone ? 0 : -1; if (on) for (int i = 0; i < g_nv; i++) cb(g_v[i].kind, g_v[i].idx, arg); pthread_mutex_unlock(&g_vmu); return r; }
int dm_vault_has(int kind, int idx) { pthread_mutex_lock(&g_vmu); int r = g_loaded && vfind(kind, idx) >= 0; pthread_mutex_unlock(&g_vmu); return r; }







int dm_item_quest(uint8_t *b, uint64_t it) {
    uint8_t ver = *(uint8_t *)(it + 0x1bd); if (ver < 1 || ver > 3) return 0;
    uint64_t t = *(uint64_t *)(b + 0x2d64a18 + (uint64_t)ver * 16); uint64_t recs = t ? *(uint64_t *)(t + 0x15c0) : 0; uint32_t cls = *(uint32_t *)(it + 4);
    return recs && cls < *(uint32_t *)(t + 0x15c8) && *(uint8_t *)(recs + (uint64_t)cls * 0x1c0 + 0x13a) != 0;
}
static int item_key(uint8_t *b, uint64_t it, int *kind, int *idx) {
    uint64_t d = *(uint64_t *)(it + 0x10); if (*(uint32_t *)it != 4 || !d) return 0;
    uint32_t q = *(uint32_t *)d; if (q != 7 && q != 5) return 0;
    if (!(*(uint32_t *)(d + 0x18) & 0x10)) return 0;
    if (dm_item_quest(b, it)) return 0;
    *kind = q == 7; *idx = (int)*(uint32_t *)(d + 0x34); return 1;
}


static int deposit(uint8_t *b, uint64_t G, uint64_t player, uint32_t id, const char *name) {
    uint64_t it = dm_diag_srv_item(id); int kind, idx; if (!it || !item_key(b, it, &kind, &idx)) return -1;
    uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d || !*(uint64_t *)(d + 0xa0) || *(uint32_t *)(it + 0xc) != 0) return -2;
    static uint8_t buf[VBYTES]; int len = dm_item_write(b, it, buf); if (!len) return -3;
    int old = vfind(kind, idx); uint8_t *keep = malloc((size_t)len); if (!keep) return -3; memcpy(keep, buf, (size_t)len);
    int page = *(uint8_t *)(d + 0x55), x = pp ? *(int32_t *)(pp + 0x10) : 0, y = pp ? *(int32_t *)(pp + 0x14) : 0;
    ((void (*)(uint64_t))(b + 0x55e430))(it);
    ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, it, 0x20, 4, 0, page, y, x);
    ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, it);
    pend_add(1, kind, idx, keep, len);
    if (old >= 0) { free(g_v[old].data); g_v[old].data = keep; g_v[old].len = (uint16_t)len; }
    else { if (g_nv >= VMAX) { free(keep); return -4; } g_v[g_nv++] = (ventry){ (uint8_t)kind, (uint16_t)idx, (uint16_t)len, keep }; }
    LOG("编年史仓库：存入 %s %d（物品 id %u，原在页 %d 位置 %d,%d，位流 %d 字节）%s", kind ? "暗金" : "套装件", idx, id, page, x, y, len, old >= 0 ? "，替换旧件" : ""); (void)name;
    return 1;
}






static int bag_fits(uint8_t *b, uint64_t player, uint64_t nu, uint64_t at, uint32_t *ox, uint32_t *oy) {
    uint8_t ver = *(uint8_t *)(nu + 0x1bd); uint64_t t = ver >= 1 && ver <= 3 ? *(uint64_t *)(b + 0x2d64a18 + ver * 16) : 0; if (!t) return 0;
    uint64_t ir = *(uint64_t *)(t + 0x15c0); uint32_t in = *(uint32_t *)(t + 0x15c8);
#define ISZ(it, o) (*(uint32_t *)((it) + 4) < in ? *(uint8_t *)(ir + (uint64_t)*(uint32_t *)((it) + 4) * 0x1c0 + (o)) : 1)
    int w = ISZ(nu, 0x11e), h = ISZ(nu, 0x11f), cols = (int)(at >> 8 & 0xff), rows = (int)(at & 0xff);
    int x = (int)lround((int16_t)(at >> 32 & 0xffff) / 100.0 - w / 2.0), y = (int)lround((int16_t)(at >> 16 & 0xffff) / 100.0 - h / 2.0);
    if (x < 0 || y < 0 || x + w > cols || y + h > rows) { LOG("编年史仓库：拖回背包落点 (%d,%d) 大小 %d×%d 出了背包 %d×%d", x, y, w, h, cols, rows); return 0; }
    uint64_t inv = *(uint64_t *)(player + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
    for (int k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d) break;
        if (it != nu && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && pp) { int ix = *(int32_t *)(pp + 0x10), iy = *(int32_t *)(pp + 0x14), iw = ISZ(it, 0x11e), ih = ISZ(it, 0x11f);
            if (x < ix + iw && ix < x + w && y < iy + ih && iy < y + h) { LOG("编年史仓库：拖回背包落点 (%d,%d) 压到背包里 (%d,%d) 的物品", x, y, ix, iy); return 0; } }
        it = *(uint64_t *)(d + 0xb0); }
#undef ISZ
    *ox = (uint32_t)x; *oy = (uint32_t)y; return 1;
}
static int withdraw(uint8_t *b, uint64_t G, uint64_t player, int kind, int idx) {
    int k = vfind(kind, idx); if (k < 0) return -5;
    uint64_t pp = *(uint64_t *)(player + 0x38), room = pp ? *(uint64_t *)(pp + 0x20) : 0; if (!room) return -6;
    uint8_t *buf = g_v[k].data; int len = g_v[k].len; int socks = 0;
    uint64_t nu = dm_item_make(b, G, room, buf, len, &socks); if (!nu) return -7;




    { uint64_t inv = *(uint64_t *)(player + 0x90); uint32_t cls = *(uint32_t *)(player + 4), fx = 0, fy = 0;
      uint64_t at = atomic_exchange(&g_take_at, 0);
      if (at ? !bag_fits(b, player, nu, at, &fx, &fy) : (!inv || cls > 7 || !((int (*)(uint64_t, uint64_t, int, uint32_t *, uint32_t *, int))(b + 0x55f620))(inv, nu, *(int32_t *)(b + 0x1dae684 + cls * 8 + 4), &fx, &fy, 0))) {
          ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu); atomic_store(&g_full_snd, 1); LOG("编年史仓库：取出 %s %d —— 背包放不下，不取（仓库不动）", kind ? "暗金" : "套装件", idx); return -8; }
      uint64_t nd = *(uint64_t *)(nu + 0x10), ip = *(uint64_t *)(nu + 0x38); if (!nd || !ip) { ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu); return -9; }
      *(uint8_t *)(nd + 0x55) = 0; *(int32_t *)(ip + 0x10) = (int32_t)fx; *(int32_t *)(ip + 0x14) = (int32_t)fy;
      uint64_t one = nu, ox = 0; ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(b + 0x6f1940))(G, player, &one, &ox);
      if (!*(uint64_t *)(nd + 0xa0)) { ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu); LOG("【红】编年史仓库：取出 %s %d —— 放进背包没成（格 %u,%u），仓库不动", kind ? "暗金" : "套装件", idx, fx, fy); return -10; }
      ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, uint64_t))(b + 0x642f70))(G, player, nu, 1, 2, 0, 0, 0, (uint64_t)(fx & 0xffff) | (uint64_t)fy << 16);
    }
    uint64_t ip = *(uint64_t *)(nu + 0x38);
    pend_add(0, kind, idx, g_v[k].data, g_v[k].len);
    free(g_v[k].data); g_v[k] = g_v[--g_nv];
    LOG("编年史仓库：取出 %s %d → 背包（新 id %u，位置 %d,%d，孔里 %d 件）", kind ? "暗金" : "套装件", idx, *(uint32_t *)(nu + 8), ip ? *(int32_t *)(ip + 0x10) : -1, ip ? *(int32_t *)(ip + 0x14) : -1, socks);
    return 1;
}




static void item_dump_list(uint8_t *b, uint64_t owner, const char *who) {
    uint64_t inv = owner ? *(uint64_t *)(owner + 0x90) : 0, it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0; int n = 0;
    for (int k = 0; it && k < 4096; k++) { uint64_t d = *(uint64_t *)(it + 0x10), pp = *(uint64_t *)(it + 0x38); if (!d) break; uint32_t c = ((uint32_t (*)(uint64_t))(b + 0x570260))(it);
        LOG("物品清点 %s：%.4s 品质 %u 方式 %u 页 %u 格 %d,%d", who, (const char *)&c, *(uint32_t *)d, *(uint32_t *)(it + 0xc), *(uint8_t *)(d + 0x55), pp ? *(int32_t *)(pp + 0x10) : -1, pp ? *(int32_t *)(pp + 0x14) : -1); n++; it = *(uint64_t *)(d + 0xb0); }
    LOG("物品清点 %s：共 %d 件", who, n);
}
static void item_dump(uint8_t *b, uint64_t player) {
    item_dump_list(b, player, "角色");
    uint64_t inv = *(uint64_t *)(player + 0x90); int k = 0;
    for (uint64_t en = inv ? *(uint64_t *)(inv + 0x68) : 0; en && k < 32; en = *(uint64_t *)(en + 0x10), k++) { uint32_t uid = *(uint32_t *)en; char who[48];
        uint64_t u = ((uint64_t (*)(uint64_t, uint32_t, uint32_t, uint32_t))(b + 0x45f5f0))((uint64_t)(uintptr_t)(b + 0x2d603a0), uid & 0x7f, uid, 0);
        snprintf(who, sizeof who, "附属单位 %u（类 %u）", uid, *(uint8_t *)(en + 0xa)); item_dump_list(b, u, who); }
}
void dm_vault_tick(uint64_t G, uint64_t player, uint8_t *b) {
    static uint64_t lastp;
    if (!player) { lastp = 0; g_loaded = 0; g_vnone = 0; return; }
    { static int done; const char *e = getenv("DM_TEST_ITEM_DUMP"); if (e && !done && *(uint32_t *)(G + 0x170) >= (uint32_t)atoi(e)) { done = 1; item_dump(b, player); } }
    if (player != lastp) { lastp = player; pthread_mutex_lock(&g_vmu); const char *nm = vault_name(b); g_loaded = nm != NULL; g_vnone = nm == NULL; if (nm) { strlcpy(g_vname, nm, sizeof g_vname); vault_load(nm); } else vault_clear(); pthread_mutex_unlock(&g_vmu); atomic_fetch_add(&g_vgen, 1); }
    { int dm_itemjob_owner(void); if (dm_itemjob_owner()) return; }
    { uint32_t dq = atomic_exchange(&g_desc_req, 0); if (dq) { pthread_mutex_lock(&g_vmu); vault_desc(b, G, player, dq); pthread_mutex_unlock(&g_vmu); } }
    { static uint32_t last; uint32_t t = *(uint32_t *)(G + 0x170); if (atomic_load(&g_vretry) && t - last >= 250) { last = t; LOG("编年史仓库：侧文件上次没写成，再请一次存盘 → %d", dm_request_save()); } }




    { static int cap = -1; if (cap < 0) { const char *e = getenv("DM_TEST_VAULT_PEND"); cap = e ? atoi(e) : 0; if (cap >= 2 && cap <= VPEND) g_pend_cap = cap; }
      if (atomic_load(&g_req_put) || atomic_load(&g_req_take)) { pthread_mutex_lock(&g_vmu); int np = g_npend; pthread_mutex_unlock(&g_vmu);
          if (np > g_pend_cap - 2) { static uint32_t last; uint32_t t = *(uint32_t *)(G + 0x170); if (t - last >= 25) { last = t; LOG("编年史仓库：待落盘 %d 条（上限 %d），这次存取先等落盘，请游戏存盘 → %d", np, g_pend_cap, dm_request_save()); } return; } } }
    uint32_t put = atomic_exchange(&g_req_put, 0), take = atomic_exchange(&g_req_take, 0); if (!put && !take) return;
    pthread_mutex_lock(&g_vmu);
    if (!g_loaded) { pthread_mutex_unlock(&g_vmu); LOG("编年史仓库：这类角色没有共享仓库，不开编年史仓库"); return; }
    int r = 0;
    if (take) r = withdraw(b, G, player, (int)(take >> 16 & 0xff), (int)(take & 0xffff));
    if (put) { uint64_t it = dm_diag_srv_item(put); int kind, idx, swap = it && item_key(b, it, &kind, &idx) && vfind(kind, idx) >= 0;
        if (swap) { int w = withdraw(b, G, player, kind, idx); if (w < 0) { r = w; LOG("编年史仓库：存入物品 %u —— 同一格的旧件取不出来（%d），这次不存", put, w); goto done; } }
        r = deposit(b, G, player, put, g_vname); if (r < 0 && it) { uint64_t d = *(uint64_t *)(it + 0x10); LOG("编年史仓库：不收的那件 类型 %u 品质 %u 标志 %#x 方式 %u", *(uint32_t *)it, d ? *(uint32_t *)d : 0, d ? *(uint32_t *)(d + 0x18) : 0, *(uint32_t *)(it + 0xc)); }
        if (r < 0) atomic_store(&g_full_snd, 1);
        if (r < 0) LOG("编年史仓库：存入物品 %u 不收（%d：不是已鉴定的暗金 / 套装件、是任务物品、不在容器里或写不出位流）", put, r); }
done:
    atomic_fetch_add(&g_vgen, 1);
    int np = g_npend; pthread_mutex_unlock(&g_vmu);
    if (np) { int s = getenv("DM_TEST_VAULT_NOSAVE") ? -9 : dm_request_save(); LOG("编年史仓库：仓库现有 %d 件、待落盘 %d 条，请游戏存盘 → %d", g_nv, np, s); }
}


static void vault_pre_commit(const char *wd) {
    pthread_mutex_lock(&g_vmu);
    if ((g_npend || g_ddirty) && g_loaded) {
        dm_blob bl[64]; int nb = dm_save_blobs(wd, g_vname, bl, 64), done = 0; uint64_t cno = dm_fs_commit_count() + 1; double t = now_s();
        while (g_npend > 0) { vpend *p = &g_pend[0]; int have = dm_blobs_have(bl, nb, p->data, p->len), ok = 0; const char *why = "";
            int late = cno >= p->made + 2 && t - (double)p->t >= 10;
            if (p->put) { if (!have) { ok = 1; why = "存入：原版存档里已没有那件"; } else if (late && cno >= p->made + 3) { ok = 2; why = "【红】存入：存入后好几次提交原版存档里那件还在（存入没生效？），不并进、编年史里也去掉"; } }
            else { if (have) { ok = 1; why = "取出：原版存档里已有那件"; } else if (late) { ok = 1; why = "取出：之后的存档里哪都没有那件（卖掉 / 丢下 / 用掉）"; } }
            if (!ok) { LOG("编年史仓库：提交前核对 —— 第 1 条待落盘（%s %s %d）还没证明（这次提交 #%llu，记下于 #%llu 之后），这次及后面的都等", p->put ? "存入" : "取出", p->kind ? "暗金" : "套装件", p->idx, (unsigned long long)cno, (unsigned long long)p->made); break; }
            int di = dfind(p->kind, p->idx);
            if (ok == 1 && p->put) { if (di >= 0) { free(g_d[di].data); g_d[di].data = p->data; g_d[di].len = p->len; } else if (g_nd < VMAX) g_d[g_nd++] = (ventry){ p->kind, p->idx, p->len, p->data }; else free(p->data); p->data = NULL; }
            else if (ok == 1) { if (di >= 0 && g_d[di].len >= 16 && !memcmp(g_d[di].data + 8, p->data + 8, (size_t)p->n)) { free(g_d[di].data); g_d[di] = g_d[--g_nd]; } }
            else { int vi = vfind(p->kind, p->idx); if (vi >= 0 && g_v[vi].len == p->len && !memcmp(g_v[vi].data, p->data, p->len)) { free(g_v[vi].data); g_v[vi] = g_v[--g_nv]; atomic_fetch_add(&g_vgen, 1); } }
            LOG("编年史仓库：提交前核对 #%llu —— %s（%s %d）→ 并进盘上状态", (unsigned long long)cno, why, p->kind ? "暗金" : "套装件", p->idx);
            free(p->data); memmove(&g_pend[0], &g_pend[1], (size_t)(g_npend - 1) * sizeof g_pend[0]); g_npend--; done++; }
        dm_blobs_free(bl, nb);




        if (done || g_ddirty) { if (vault_save(wd, g_vname) == 0) { LOG("编年史仓库：侧文件随这次提交落盘（盘上 %d 件、内存 %d 件、还有 %d 条待证明）", g_nd, g_nv, g_npend); g_ddirty = 0; atomic_store(&g_vretry, 0); }
            else { g_ddirty = 1; atomic_store(&g_vretry, 1); dm_fs_save_poison(EIO); LOG("【红】编年史仓库：侧文件没写成 → 这次存档提交拒绝，之后重试（盘上状态 %d 件留在内存）", g_nd); } }
    }
    pthread_mutex_unlock(&g_vmu);
}

int dm_vault_settled(void) { return atomic_load(&g_loaded) || atomic_load(&g_vnone); }
int dm_vault_records(int (*each)(const uint8_t *bits, int len, void *ctx), void *ctx) {
    pthread_mutex_lock(&g_vmu); int n = 0;
    for (int i = 0; i < g_nv; i++) if (g_v[i].data && g_v[i].len) { each(g_v[i].data, g_v[i].len, ctx); n++; }
    pthread_mutex_unlock(&g_vmu); return n;
}
__attribute__((constructor)) static void vault_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[1] = vault_pre_commit; }
