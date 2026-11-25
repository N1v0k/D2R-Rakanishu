









































#include "dm_widget.h"
#include "hle.h"
#include "dm_i18n.h"
#include <stdatomic.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

int dm_custom_get(const char *key);
uint64_t dm_guest_frame(void);
int dm_ui_panel_state(void);
void dm_game_notice(uint8_t *main_base, const char *text);
const char *dm_save_workdir(void); uint64_t dm_fs_commit_count(void); uint64_t dm_diag_srv_item(uint32_t id); void *dm_memfind(const uint8_t *h, size_t hn, const uint8_t *n, size_t nn);
int dm_item_write(uint8_t *b, uint64_t it, uint8_t *buf); uint64_t dm_item_make(uint8_t *b, uint64_t G, uint64_t room, const uint8_t *buf, int len, int *socks_out);
int dm_request_save(void); void dm_vault_request_deposit(uint32_t item_id); int dm_vault_put_pending(void); int dm_vault_ready(void); int dm_vault_has(int kind, int idx); int dm_item_quest(uint8_t *b, uint64_t it);
#define PICK_R 3
#define T_RUNE 75
#define T_GEM 20
#define T_CHARM 13

static uint8_t *B;
static uint64_t cli_player(void) { return ((uint64_t (*)(uint32_t))(B + 0x45f9d0))(*(uint32_t *)(B + 0x2d20e4c)); }
static int is_rune_gem(uint64_t it) { int (*is)(uint64_t, int) = (int (*)(uint64_t, int))(B + 0x574bc0); return is(it, T_RUNE) || is(it, T_GEM); }




static int is_mat(uint64_t it) { static const char k[][4] = { "pk1", "pk2", "pk3", "dhn", "bey", "mbr", "tes", "ceh", "bet", "fed", "toa", "xa1", "xa2", "xa3", "xa4", "xa5", "ua1", "ua2", "ua3", "ua4", "ua5" };
    uint32_t c = ((uint32_t (*)(uint64_t))(B + 0x570260))(it) & 0xffffff; for (unsigned i = 0; i < sizeof k / sizeof *k; i++) if (!memcmp(&c, k[i], 3)) return 1; return 0; }
static int is_charm(uint64_t it) { return ((int (*)(uint64_t, int))(B + 0x574bc0))(it, T_CHARM); }
static uint32_t item_code(uint64_t it) { return ((uint32_t (*)(uint64_t))(B + 0x570260))(it); }
#define AUTO_RUNE_MAX 18
static int auto_ok(uint64_t it) {
    if (!is_rune_gem(it)) return 0;
    if (!((int (*)(uint64_t, int))(B + 0x574bc0))(it, T_RUNE)) return 1;
    uint32_t c = item_code(it); const char *k = (const char *)&c;
    return k[0] == 'r' && k[1] >= '0' && k[1] <= '9' && k[2] >= '0' && k[2] <= '9' && (k[1] - '0') * 10 + (k[2] - '0') <= AUTO_RUNE_MAX;
}


static uint64_t bag_item(uint64_t pl, uint32_t id) {
    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
    for (int k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; if (*(uint32_t *)(it + 8) == id && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0) return it; it = *(uint64_t *)(d + 0xb0); }
    return 0;
}


#define UMAX 1024
#define UBYTES 64
typedef struct { uint32_t code; uint16_t len; uint8_t bits[UBYTES]; } uitem;
typedef struct { int put; uitem it; uint16_t plen; uint8_t pb[UBYTES]; uint32_t nid; } upend;
static uitem g_u[UMAX], g_ud[UMAX]; static int g_nu, g_nud, g_uload; static char g_uchar[64];
static upend g_up[256]; static int g_nup, g_udirty;
static uint32_t g_ready[64]; static int g_nready;
static pthread_mutex_t g_umu = PTHREAD_MUTEX_INITIALIZER;
static _Atomic int g_ucount, g_ugen;
static void pouch_path(char *p, size_t n, const char *wd) { snprintf(p, n, "%s/%s/%s.swpouch", wd, g_uchar, g_uchar); }
int dm_pouch_each(void (*cb)(uint32_t code, void *arg), void *arg) { if (pthread_mutex_trylock(&g_umu)) return -1; int r = g_uload ? 0 : -1; if (!r) for (int i = 0; i < g_nu; i++) cb(g_u[i].code, arg); pthread_mutex_unlock(&g_umu); return r; }


int dm_pouch_holds(uint32_t unit_id) { pthread_mutex_lock(&g_umu); int r = 0; for (int i = 0; i < g_nup; i++) if (!g_up[i].put && g_up[i].nid == unit_id) r = 1; pthread_mutex_unlock(&g_umu); return r; }
static void pouch_pub(void) { atomic_store(&g_ucount, g_nu); atomic_fetch_add(&g_ugen, 1); }
static void pouch_load(void) {
    g_nu = g_nud = g_nup = g_nready = 0; const char *wd = dm_save_workdir(); if (!wd || !g_uchar[0]) { pouch_pub(); return; }
    char p[1200]; pouch_path(p, sizeof p, wd); FILE *f = fopen(p, "rb");
    if (!f) { LOG("百宝囊：%s 还没有侧文件（空）", g_uchar); pouch_pub(); return; }
    uint8_t h[8]; int bad = 0;
    if (fread(h, 1, 8, f) != 8 || memcmp(h, "SWPU", 4) || h[4] != 1) bad = 1;
    else for (int i = 0, n = *(uint16_t *)(h + 6); i < n; i++) { uint8_t e[6]; if (fread(e, 1, 6, f) != 6) { bad = 1; break; } uint16_t len = *(uint16_t *)(e + 4);
        if (!len || len > UBYTES || g_nu >= UMAX) { bad = 1; break; } uitem *u = &g_u[g_nu]; memcpy(&u->code, e, 4); u->len = len; if (fread(u->bits, 1, len, f) != len) { bad = 1; break; } g_nu++; }
    fclose(f); if (bad) LOG("【红】百宝囊：%s 读到第 %d 条坏了，只用前面读好的（文件不动）", p, g_nu);
    memcpy(g_ud, g_u, sizeof(uitem) * (size_t)g_nu); g_nud = g_nu; pouch_pub(); LOG("百宝囊：%s 读入 %d 件", g_uchar, g_nu);
}
static int pouch_save(const char *wd) {
#ifndef DM_PLAYER_BUILD
    { static int nf = -1; if (nf < 0) { const char *e = getenv("DM_TEST_POUCH_SAVE_FAIL"); nf = e ? atoi(e) : 0; } if (nf > 0) { nf--; LOG("【红】百宝囊：测试 DM_TEST_POUCH_SAVE_FAIL 这次当作写失败"); return -1; } }
#endif
    char p[1200], t[1210]; pouch_path(p, sizeof p, wd); snprintf(t, sizeof t, "%s.tmp", p); FILE *f = fopen(t, "wb"); if (!f) return -1;
    uint8_t h[8] = { 'S', 'W', 'P', 'U', 1, 0, 0, 0 }; *(uint16_t *)(h + 6) = (uint16_t)g_nud; int ok = fwrite(h, 1, 8, f) == 8;
    for (int i = 0; i < g_nud && ok; i++) { uint8_t e[6]; memcpy(e, &g_ud[i].code, 4); *(uint16_t *)(e + 4) = g_ud[i].len; ok = fwrite(e, 1, 6, f) == 6 && fwrite(g_ud[i].bits, 1, g_ud[i].len, f) == g_ud[i].len; }
    ok = fclose(f) == 0 && ok; if (!ok || rename(t, p)) { remove(t); LOG("【红】百宝囊：写 %s 失败", p); return -1; }
    return 0;
}
static int char_blob(const char *wd, dm_blob *o) {
    char p[1200]; snprintf(p, sizeof p, "%s/%s/%s.d2s", wd, g_uchar, g_uchar); FILE *f = fopen(p, "rb"); o->d = NULL; o->n = 0; if (!f) return 0;
    fseek(f, 0, SEEK_END); long L = ftell(f); fseek(f, 0, SEEK_SET); o->d = L > 0 && L < (16 << 20) ? malloc((size_t)L) : NULL; o->n = o->d ? fread(o->d, 1, (size_t)L, f) : 0; fclose(f);
    if (!o->n) { free(o->d); o->d = NULL; } return o->n > 0;
}
static void pouch_hook(const char *wd) {
    pthread_mutex_lock(&g_umu);
    if (g_uload && (g_nup || g_udirty)) {
        dm_blob bl; int hb = char_blob(wd, &bl), done = 0; uint64_t cno = dm_fs_commit_count() + 1;
        if (!hb) LOG("【红】百宝囊：提交前核对 #%llu —— 读不到 %s 的角色档，这次不判", (unsigned long long)cno, g_uchar);
        for (int i = 0; hb && i < g_nup; ) { upend *p = &g_up[i]; int have = dm_memfind(bl.d, bl.n, p->pb, p->plen) != NULL;
            if (p->put == have) { i++; continue; }
            if (p->put) { if (g_nud < UMAX) g_ud[g_nud++] = p->it; else LOG("【红】百宝囊：盘上状态已满 %d 件", UMAX); }
            else { int k = -1; for (int j = 0; j < g_nud && k < 0; j++) if (g_ud[j].len == p->it.len && !memcmp(g_ud[j].bits, p->it.bits, p->it.len)) k = j;
                if (k >= 0) g_ud[k] = g_ud[--g_nud]; else LOG("【红】百宝囊：放到背包的 %.3s 在盘上状态里找不到", (const char *)&p->it.code);
                if (g_nready < 64) g_ready[g_nready++] = p->nid; }
            LOG("百宝囊：提交前核对 #%llu —— %.3s %s → 并进盘上状态", (unsigned long long)cno, (const char *)&p->it.code, p->put ? "收进囊：角色档里已没有背包那一串" : "放到背包：角色档里已有放好的那一串");
            g_up[i] = g_up[--g_nup]; done++; }
        free(bl.d);


        if (done || g_udirty) { if (pouch_save(wd) == 0) { LOG("百宝囊：侧文件随这次提交落盘（盘上 %d 件、囊里 %d 件、还有 %d 条待证明）", g_nud, g_nu, g_nup); g_udirty = 0; }
            else { void dm_fs_save_poison(int e); g_udirty = 1; dm_fs_save_poison(EIO); LOG("【红】百宝囊：侧文件没写成 → 这次存档提交拒绝，之后重试（盘上状态 %d 件留在内存）", g_nud); } }
    }
    pthread_mutex_unlock(&g_umu);
}
__attribute__((constructor)) static void pouch_hook_install(void) { extern void (*g_dm_pre_commit_hook[DM_PRE_COMMIT_HOOKS])(const char *); g_dm_pre_commit_hook[2] = pouch_hook; }



static int pouch_put(uint8_t *b, uint64_t G, uint64_t player, uint32_t id) {
    uint64_t it = bag_item(player, id), pp = it ? *(uint64_t *)(it + 0x38) : 0;
    if (!it || !pp || !(is_rune_gem(it) || is_mat(it))) return -1;
    if (g_nu >= UMAX || g_nup >= 256) { LOG("【红】百宝囊：囊里 %d 件 / 待证明 %d 条已满，这件留在背包", g_nu, g_nup); return -2; }
    uint8_t buf[1024]; int len = dm_item_write(b, it, buf); if (!len || len > UBYTES) { LOG("【红】百宝囊：物品 id %u 位流 %d 字节，不收", id, len); return -3; }
    uitem u = { item_code(it), (uint16_t)len, { 0 } }; memcpy(u.bits, buf, (size_t)len); int x = *(int32_t *)(pp + 0x10), y = *(int32_t *)(pp + 0x14);
    ((void (*)(uint64_t))(b + 0x55e430))(it);
    ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, int))(b + 0x642f70))(G, player, it, 0x20, 4, 0, 0, y, x);
    ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, it);
    g_u[g_nu++] = u; upend *p = &g_up[g_nup++]; memset(p, 0, sizeof *p); p->put = 1; p->it = u; p->plen = (uint16_t)len; memcpy(p->pb, buf, (size_t)len);
    pouch_pub(); LOG("百宝囊：收进 %.3s（物品 id %u，背包格 %d,%d，位流 %d 字节）→ 囊里 %d 件", (const char *)&u.code, id, x, y, len, g_nu);
    return 1;
}


static int pouch_take(uint8_t *b, uint64_t G, uint64_t player) {
    if (!g_nu || g_nup >= 256) return 0;
    uitem u = g_u[g_nu - 1]; uint64_t pp = *(uint64_t *)(player + 0x38), room = pp ? *(uint64_t *)(pp + 0x20) : 0; if (!room) return 0;
    uint64_t nu = dm_item_make(b, G, room, u.bits, u.len, NULL); if (!nu) { LOG("【红】百宝囊：%.3s 位流建不出物品，留在囊里", (const char *)&u.code); return 0; }
    uint64_t inv = *(uint64_t *)(player + 0x90); uint32_t cls = *(uint32_t *)(player + 4), fx = 0, fy = 0;
    if (!inv || cls > 7 || !((int (*)(uint64_t, uint64_t, int, uint32_t *, uint32_t *, int))(b + 0x55f620))(inv, nu, *(int32_t *)(b + 0x1dae684 + cls * 8 + 4), &fx, &fy, 0)) {
        ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu); return 0; }
    uint64_t nd = *(uint64_t *)(nu + 0x10), ip = *(uint64_t *)(nu + 0x38); if (!nd || !ip) { ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu); return 0; }
    *(uint8_t *)(nd + 0x55) = 0; *(int32_t *)(ip + 0x10) = (int32_t)fx; *(int32_t *)(ip + 0x14) = (int32_t)fy;
    uint64_t one = nu, ox = 0; ((int (*)(uint64_t, uint64_t, uint64_t *, uint64_t *))(b + 0x6f1940))(G, player, &one, &ox);
    if (!*(uint64_t *)(nd + 0xa0)) { ((void (*)(uint64_t, uint64_t))(b + 0x658be0))(G, nu); LOG("【红】百宝囊：%.3s 放进背包没成（格 %u,%u），留在囊里", (const char *)&u.code, fx, fy); return 0; }
    ((void (*)(uint64_t, uint64_t, uint64_t, int, int, int, int, int, uint64_t))(b + 0x642f70))(G, player, nu, 1, 2, 0, 0, 0, (uint64_t)(fx & 0xffff) | (uint64_t)fy << 16);
    uint8_t buf[1024]; int len = dm_item_write(b, nu, buf);
    upend *p = &g_up[g_nup++]; memset(p, 0, sizeof *p); p->put = 0; p->it = u; p->nid = *(uint32_t *)(nu + 8);
    if (len > 0 && len <= UBYTES) { p->plen = (uint16_t)len; memcpy(p->pb, buf, (size_t)len); } else { p->plen = u.len; memcpy(p->pb, u.bits, u.len); LOG("【红】百宝囊：放好后的位流 %d 字节，用囊里那串作证据", len); }
    g_nu--; pouch_pub(); LOG("百宝囊：%.3s 放到背包格 %u,%u（新 id %u）→ 囊里还剩 %d 件，等落盘证明后进高级仓库", (const char *)&u.code, fx, fy, p->nid, g_nu);
    return 1;
}
static uint32_t adv_stash_unit(uint64_t pl) {
    uint64_t inv = *(uint64_t *)(pl + 0x90); int k = 0;
    for (uint64_t en = inv ? *(uint64_t *)(inv + 0x68) : 0; en && k < 32; en = *(uint64_t *)(en + 0x10), k++) if (*(uint8_t *)(en + 0xa) == 1) return *(uint32_t *)en;
    return 0;
}
static void move_to_stash(uint8_t *b, uint64_t G, uint64_t player, uint32_t id, uint32_t unit) {
    uint8_t pk[25] = { 0x63 }; memcpy(pk + 5, &id, 4); memcpy(pk + 9, &unit, 4);
    *(uint32_t *)(G + 0x19050) = 1; ((void (*)(uint64_t, uint64_t, uint8_t *, uint32_t))(b + 0x6e5b40))(G, player, pk, 25); *(uint32_t *)(G + 0x19050) = 0;
}









static int stash_sweep(uint8_t *b, uint64_t G, uint64_t player, uint32_t unit, uint32_t tq, int *saved) {
    static uint32_t old[768]; static int nold, was_open; typedef struct { uint32_t id, t; } dm_t; static dm_t dm[16]; static int nsw;
    if (!unit) { was_open = 0; nsw = 0; return 0; }
    uint32_t cand[256]; int nc = 0; uint64_t inv = *(uint64_t *)(player + 0x90);
    for (uint64_t it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0, k = 0; it && k < 2048 && nc < 256; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
        if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 4 && (is_rune_gem(it) || is_mat(it))) cand[nc++] = *(uint32_t *)(it + 8); it = *(uint64_t *)(d + 0xb0); }


    if (!was_open) { const char *e = getenv("DM_TEST_STACK99");
        for (uint64_t en = e && inv ? *(uint64_t *)(inv + 0x68) : 0, n = 0; en && n < 16; en = *(uint64_t *)(en + 0x10), n++) { if (*(uint8_t *)(en + 0xa) != 1) continue;
            uint32_t i = *(uint32_t *)en; uint64_t u = *(uint64_t *)(G + 0x2230 + 8 * (i & 0x7f)); while (u && *(uint32_t *)(u + 8) != i) u = *(uint64_t *)(u + 0x158);
            uint64_t iv = u ? *(uint64_t *)(u + 0x90) : 0; if (!iv || *(uint32_t *)iv != 0x01020304) continue;
            for (uint64_t it = *(uint64_t *)(iv + 0x10), k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t c = item_code(it);
                if (!memcmp(&c, e, strlen(e))) { LOG("测试 DM_TEST_STACK99：高级仓库里 %s 的叠数 %u → 99", e, *(uint8_t *)(d + 0x9c)); *(uint8_t *)(d + 0x9c) = 99; } it = *(uint64_t *)(d + 0xb0); } } }
    if (!was_open) { was_open = 1; nold = 0; for (int i = 0; i < nc && nold < 768; i++) old[nold++] = cand[i]; if (nc) LOG("珍宝匣：开箱时个人页里已有符文 / 宝石 / 材料 %d 件（不动）", nc); return 0; }
    int full = 0;
    for (int i = 0; i < nsw; ) { int still = 0; for (int k = 0; k < nc; k++) still |= cand[k] == dm[i].id;
        if (!still) { LOG("珍宝匣：物品 id %u 已从仓库页转进高级仓库", dm[i].id); *saved = 1; dm[i] = dm[--nsw]; continue; }
        if (tq - dm[i].t >= 50) { if (nold < 768) old[nold++] = dm[i].id; LOG("珍宝匣：物品 id %u 发了移到仓库 2 秒还在仓库页（那一格叠满 / 不收）→ 留在原处，不再发", dm[i].id); full = 1; dm[i] = dm[--nsw]; continue; }
        i++; }
    for (int k = 0; k < nc && nsw < 16; k++) { int skip = 0; for (int i = 0; i < nold && !skip; i++) skip = old[i] == cand[k]; for (int i = 0; i < nsw && !skip; i++) skip = dm[i].id == cand[k];
        if (skip) continue; move_to_stash(b, G, player, cand[k], unit); dm[nsw++] = (dm_t){ cand[k], tq }; LOG("珍宝匣：仓库页里新落下的物品 id %u → 原版移到仓库（高级仓库单位 %u）", cand[k], unit); break; }
    return full;
}











static uint32_t g_pick_id, g_pick_x, g_pick_y; static uint32_t g_c2s_q[8]; static int g_c2s_nq, g_c2s_full;
static int adv_stack_full(uint64_t G, uint64_t player, uint32_t code) {
    uint64_t inv = *(uint64_t *)(player + 0x90);
    for (uint64_t en = inv ? *(uint64_t *)(inv + 0x68) : 0, n = 0; en && n < 16; en = *(uint64_t *)(en + 0x10), n++) { if (*(uint8_t *)(en + 0xa) != 1) continue;
        uint32_t i = *(uint32_t *)en; uint64_t u = *(uint64_t *)(G + 0x2230 + 8 * (i & 0x7f)); while (u && *(uint32_t *)(u + 8) != i) u = *(uint64_t *)(u + 0x158);
        uint64_t iv = u ? *(uint64_t *)(u + 0x90) : 0; if (!iv || *(uint32_t *)iv != 0x01020304) continue;
        for (uint64_t it = *(uint64_t *)(iv + 0x10), k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;
            if (item_code(it) == code && *(uint8_t *)(d + 0x9c) >= 99) return 1; it = *(uint64_t *)(d + 0xb0); } }
    return 0;
}
void dm_pouch_c2s(uint64_t *regs) {
    uint64_t G = regs[0], player = regs[1]; const uint8_t *pk = (const uint8_t *)(uintptr_t)regs[2]; uint32_t len = (uint32_t)regs[3]; if (!B || !G || !player || !pk) return;
    if (len == 17 && pk[0] == 0x19) { uint32_t pg; memcpy(&g_pick_id, pk + 1, 4); memcpy(&g_pick_x, pk + 5, 4); memcpy(&g_pick_y, pk + 9, 4); memcpy(&pg, pk + 13, 4); if (pg) g_pick_id = 0; return; }




    if ((len == 21 && pk[0] == 0x54) || (len == 29 && pk[0] == 0x55)) { uint32_t id; memcpy(&id, pk + 1, 4);
        uint64_t dm_diag_srv_item(uint32_t); uint64_t it = dm_diag_srv_item(id); if (!it || !bag_item(player, id) || !(is_rune_gem(it) || is_mat(it))) return;
        uint32_t unit = adv_stash_unit(player); int near = ((int (*)(uint64_t, uint64_t, int, int))(B + 0x650d10))(G, player, 0x10b, 0x32) & 1; if (!unit || !near) return;
        uint32_t code = item_code(it);
        if (adv_stack_full(G, player, code)) { if (pk[0] == 0x55) g_c2s_full = 1; LOG("珍宝匣：%.3s（id %u）按住确认键移到%s页，高级仓库那一格已叠满 → 照原样移过去", (const char *)&code, id, pk[0] == 0x55 ? "共享" : "个人"); return; }
        static uint8_t st[25]; memset(st, 0, sizeof st); st[0] = 0x63; memcpy(st + 5, &id, 4); memcpy(st + 9, &unit, 4); regs[2] = (uint64_t)(uintptr_t)st; regs[3] = 25;
        LOG("珍宝匣：%.3s（id %u）按住确认键移到%s页 → 改成原版移到仓库（高级仓库单位 %u）", (const char *)&code, id, pk[0] == 0x55 ? "共享" : "个人", unit); return; }
    if (len != 13 || pk[0] != 0x45) return;
    uint32_t id; memcpy(&id, pk + 1, 4); if (!id || id != g_pick_id) return;
    uint64_t dm_diag_srv_item(uint32_t); uint64_t it = dm_diag_srv_item(id); if (!it || !(is_rune_gem(it) || is_mat(it))) return;
    uint32_t unit = adv_stash_unit(player); int near = ((int (*)(uint64_t, uint64_t, int, int))(B + 0x650d10))(G, player, 0x10b, 0x32) & 1; if (!unit || !near || g_c2s_nq >= 8) return;
    uint32_t code = item_code(it);
    if (adv_stack_full(G, player, code)) { g_c2s_full = 1; LOG("珍宝匣：%.3s（id %u）拖进共享页，高级仓库那一格已叠满 → 照原样落在共享页那格，提示一次", (const char *)&code, id); return; }
    static uint8_t alt[17]; uint32_t zero = 0; alt[0] = 0x18; memcpy(alt + 1, &id, 4); memcpy(alt + 5, &g_pick_x, 4); memcpy(alt + 9, &g_pick_y, 4); memcpy(alt + 13, &zero, 4);
    regs[2] = (uint64_t)(uintptr_t)alt; regs[3] = 17; g_c2s_q[g_c2s_nq++] = id; g_pick_id = 0;
    LOG("珍宝匣：%.3s（id %u）拖进共享页 → 改成放回背包格 %u,%u，下一拍代发原版移到仓库", (const char *)&code, id, g_pick_x, g_pick_y);
}



static _Atomic uint32_t g_absq[64]; static _Atomic unsigned g_absw, g_absr;
static _Atomic int g_stash_open, g_near_pub;
static void absorb_push(uint32_t id) { unsigned w = atomic_load(&g_absw); if (w - atomic_load(&g_absr) >= 64) return; atomic_store(&g_absq[w % 64], id); atomic_store(&g_absw, w + 1); }



typedef struct { uint32_t id, code; uint64_t fr; int st; } pick;
static pick g_pk[16];








static int rune_stack_full(uint64_t pl, uint64_t it) {
    if (!((int (*)(uint64_t, int))(B + 0x574bc0))(it, T_RUNE)) return 0;
    uint32_t sid = adv_stash_unit(pl), code = item_code(it); if (!sid) return 0;
    for (uint64_t u = *(uint64_t *)(B + 0x2d603a0 + 8 * (sid & 0x7f)); u; u = *(uint64_t *)(u + 0x158)) {
        if (*(uint32_t *)(u + 8) != sid || *(uint32_t *)u != 0) continue;
        uint64_t iv = *(uint64_t *)(u + 0x90); if (!iv || *(uint32_t *)iv != 0x01020304) return 0;
        for (uint64_t x = *(uint64_t *)(iv + 0x10), k = 0; x && k < 2048; k++) { uint64_t d = *(uint64_t *)(x + 0x10); if (!d) break;
            if (item_code(x) == code && *(uint8_t *)(d + 0x9c) >= 99) return 1; x = *(uint64_t *)(d + 0xb0); }
        return 0;
    }
    return 0;
}
static uint32_t g_gnd[128]; static int g_ngnd;
static int gnd_has(uint32_t id) { for (int i = 0; i < 128; i++) if (g_gnd[i] == id) return 1; return 0; }
static void gnd_add(uint32_t id) { if (!gnd_has(id)) g_gnd[g_ngnd++ % 128] = id; }
static void gnd_del(uint32_t id) { for (int i = 0; i < 128; i++) if (g_gnd[i] == id) g_gnd[i] = 0; }
static void auto_pick(uint64_t pl, uint64_t fr) {
    if (!adv_stash_unit(pl)) return;
    for (int i = 0; i < 16; i++) { pick *p = &g_pk[i]; if (!p->st) continue; uint64_t it = bag_item(pl, p->id);
        if (p->st == 1) { if (!it && fr - p->fr > 120) { p->st = 0; LOG("百宝囊：%.3s（id %u）2 秒没捡进背包（背包满？），不再试", (const char *)&p->code, p->id); } }
        else if (!it) p->st = 0;
        else if (fr - p->fr > 120) { p->st = 0; LOG("【红】百宝囊：%.3s（id %u）交服务端 2 秒还在背包", (const char *)&p->code, p->id); } }
    static uint64_t next; if (fr < next) return; next = fr + 10;
    {
        uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
        for (int k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t id = *(uint32_t *)(it + 8);
            if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && gnd_has(id) && is_rune_gem(it)) {
                int slot = -1; for (int i = 0; i < 16; i++) { if (g_pk[i].st == 2 && g_pk[i].id == id) { slot = -2; break; } if (slot == -1 && (!g_pk[i].st || g_pk[i].id == id)) slot = i; }
                if (slot >= 0) { int mine = g_pk[slot].st == 1 && g_pk[slot].id == id; g_pk[slot] = (pick){ id, item_code(it), fr, 2 }; gnd_del(id); absorb_push(id);
                    LOG("百宝囊：%.3s（id %u）%s进了背包 → 交服务端收（%s）", (const char *)&g_pk[slot].code, id, mine ? "" : "玩家自己捡", atomic_load(&g_near_pub) ? "储藏箱旁：进高级仓库" : "进百宝囊"); return; } }
            it = *(uint64_t *)(d + 0xb0); } }
    uint64_t pp = *(uint64_t *)(pl + 0x38); if (!pp) return; int px = *(uint16_t *)(pp + 2), py = *(uint16_t *)(pp + 6);


    { static int at = -1; static char code[5]; if (at < 0) { const char *e = getenv("DM_TEST_PICK"); at = e && sscanf(e, "%d:%4s", &at, code) == 2 ? at : 0; }
      if (at > 0 && fr >= (uint64_t)at) for (int k = 0; k < 128 && at > 0; k++) for (uint64_t it = *(uint64_t *)(B + 0x2d603a0 + 4 * 0x400 + k * 8); it; it = *(uint64_t *)(it + 0x158)) {
          uint32_t c = item_code(it); uint64_t ip = *(uint64_t *)(it + 0x38); if (*(uint32_t *)(it + 0xc) != 3 || !ip || memcmp(&c, code, strlen(code))) continue;
          uint32_t id = *(uint32_t *)(it + 8), one = 1; int x = *(int32_t *)(ip + 0x10), y = *(int32_t *)(ip + 0x14); uint8_t pk[17] = { 0x16 }; memcpy(pk + 1, &id, 4); memcpy(pk + 5, &x, 4); memcpy(pk + 9, &y, 4); memcpy(pk + 13, &one, 4);
          ((void (*)(const uint8_t *, int))(B + 0x2ceab0))(pk, 17); LOG("测试 DM_TEST_PICK：帧 %llu 替玩家捡地上 %s（id %u）", (unsigned long long)fr, code, id); at = 0; break; } }
    uint32_t want = 0, wcode = 0; int wx = 0, wy = 0;
    for (int k = 0; k < 128; k++) for (uint64_t it = *(uint64_t *)(B + 0x2d603a0 + 4 * 0x400 + k * 8); it; it = *(uint64_t *)(it + 0x158)) {
        if (*(uint32_t *)(it + 0xc) != 3 || !is_rune_gem(it)) continue;
        uint32_t id = *(uint32_t *)(it + 8); gnd_add(id);
        uint64_t ip = *(uint64_t *)(it + 0x38); if (want || !ip) continue; int x = *(int32_t *)(ip + 0x10), y = *(int32_t *)(ip + 0x14);
        if (abs(x - px) > PICK_R || abs(y - py) > PICK_R || !auto_ok(it)) continue;


        if (rune_stack_full(pl, it)) {
            static uint32_t told[32]; static int nt; int seen_full = 0; for (int i = 0; i < 32; i++) if (told[i] == id) seen_full = 1;
            if (!seen_full) { told[nt++ % 32] = id; LOG("百宝囊：地上 %.3s（id %u）高级仓库那一叠已满 99 → 不自动捡", (const char *)&(uint32_t){ item_code(it) }, id); }
            continue; }
        { int dm_lootfilter_hidden(uint8_t *, uint64_t); if (dm_lootfilter_hidden(B, it)) continue; }
        int seen = 0; for (int i = 0; i < 16; i++) if (g_pk[i].id == id && (g_pk[i].st || fr - g_pk[i].fr < 120)) seen = 1;
        if (!seen) { want = id; wx = x; wy = y; wcode = item_code(it); }
    }
    if (!want) return;
    int slot = -1; for (int i = 0; i < 16 && slot < 0; i++) if (!g_pk[i].st) slot = i; if (slot < 0) return;
    g_pk[slot] = (pick){ want, wcode, fr, 1 };
    uint8_t pk[17] = { 0x16 }; uint32_t one = 1; memcpy(pk + 1, &want, 4); memcpy(pk + 5, &wx, 4); memcpy(pk + 9, &wy, 4); memcpy(pk + 13, &one, 4);
    ((void (*)(const uint8_t *, int))(B + 0x2ceab0))(pk, 17);
    LOG("百宝囊：地上 %.3s（id %u）在 %d,%d（玩家 %d,%d）→ 发捡起包", (const char *)&wcode, want, wx, wy, px, py);
}



typedef struct { uint32_t id, code; uint8_t to_vault, page; } sitem;
static sitem g_st[128]; static int g_nst, g_ist = -1, g_sv, g_sg, g_sp, g_sleft, g_snear; static uint64_t g_sfr;
static _Atomic int g_store_req, g_pouch_tap;
static char g_notice[160]; static _Atomic int g_notice_req;


static uint64_t page_item(uint64_t pl, uint32_t id, int page) {
    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
    for (int k = 0; it && k < 2048; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; if (*(uint32_t *)(it + 8) == id && *(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == page) return it; it = *(uint64_t *)(d + 0xb0); }
    return 0;
}
static void store_begin(uint64_t pl) {
    int ncube = 0;
    g_nst = 0; g_sv = g_sg = g_sp = g_sleft = 0; int vault = dm_vault_ready(), adv = adv_stash_unit(pl) != 0; g_snear = atomic_load(&g_near_pub);
    uint64_t inv = *(uint64_t *)(pl + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0;
    for (int k = 0; it && k < 2048 && g_nst < 128; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break;





        int page = *(uint32_t *)(it + 0xc) == 0 ? *(uint8_t *)(d + 0x55) : -1;
        if (*(uint32_t *)d == 7 || *(uint32_t *)d == 5) LOG("存放：暗金 / 套装件 id %u 方式 %u 页 %u 已鉴定 %d（收的是方式 0、背包页 0 或魔盒页 3、已鉴定的）", *(uint32_t *)(it + 8), *(uint32_t *)(it + 0xc), *(uint8_t *)(d + 0x55), (*(uint32_t *)(d + 0x18) & 0x10) != 0);
        if (page == 0 || page == 3) { uint32_t q = *(uint32_t *)d, id = *(uint32_t *)(it + 8);
            if (is_rune_gem(it) || is_mat(it)) { if (adv && page == 0) g_st[g_nst++] = (sitem){ id, item_code(it), 0, 0 }; }
            else if ((q == 7 || q == 5) && (*(uint32_t *)(d + 0x18) & 0x10) && !is_charm(it) && !dm_item_quest(B, it)) {
                uint32_t key = (uint32_t)(q == 7) << 16 | (*(uint32_t *)(d + 0x34) & 0xffff); int dup = 0;
                for (int i = 0; i < g_nst; i++) if (g_st[i].to_vault && g_st[i].code == key) dup = 1;
                if (vault && !dup && !dm_vault_has(q == 7, (int)*(uint32_t *)(d + 0x34))) { g_st[g_nst++] = (sitem){ id, key, 1, (uint8_t)page }; ncube += page == 3; } else if (page == 0) g_sleft++; } }
        it = *(uint64_t *)(d + 0xb0); }
    g_ist = 0; g_sfr = 0; LOG("存放：背包里 %d 件、魔盒里 %d 件要存（编年史里已有同件 / 没有编年史仓库的 %d 件不动；符文宝石%s）", g_nst - ncube, ncube, g_sleft, g_snear ? "在储藏箱旁，进高级仓库" : "进百宝囊");
}
static void store_step(uint64_t pl, uint64_t fr) {
    if (g_ist < 0) return;
    if (g_ist < g_nst) { sitem *s = &g_st[g_ist]; uint64_t it = page_item(pl, s->id, s->page);
        if (!g_sfr) { if (!it) { g_ist++; return; }
            if (s->to_vault) { if (dm_vault_put_pending()) return; dm_vault_request_deposit(s->id); } else absorb_push(s->id);
            g_sfr = fr; return; }
        if (!it) { if (s->to_vault) g_sv++; else if (g_snear) g_sg++; else g_sp++; g_ist++; g_sfr = 0; return; }
        if (fr - g_sfr > (!s->to_vault && g_snear ? 240 : 120)) {
            g_sleft++; LOG("【红】存放：物品 id %u 2 秒还在%s（%s不收）", s->id, s->page == 3 ? "魔盒" : "背包", s->to_vault ? "编年史" : g_snear ? "储藏箱" : "百宝囊"); g_ist++; g_sfr = 0; }
        return; }
    {
        int k = snprintf(g_notice, sizeof g_notice, "%s", dm_str(B, "dmStoreHead")), any = 0;
        if (g_sv) { k += snprintf(g_notice + k, sizeof g_notice - k, dm_str(B, "dmStoreVault"), g_sv); any = 1; }
        if (g_sg) { k += snprintf(g_notice + k, sizeof g_notice - k, dm_str(B, "dmStoreStash"), any ? dm_str(B, "dmStoreSep") : "", g_sg); any = 1; }
        if (g_sp) { k += snprintf(g_notice + k, sizeof g_notice - k, dm_str(B, "dmStorePouch"), any ? dm_str(B, "dmStoreSep") : "", g_sp); any = 1; }
        if (!any) k += snprintf(g_notice + k, sizeof g_notice - k, "%s", dm_str(B, "dmStoreNone"));
        else if (g_sleft) { char f[96]; snprintf(g_notice + k, sizeof g_notice - k, dm_strn(B, "dmStoreLeft", g_sleft, f, sizeof f), g_sleft); } }
    int sv = g_sv + g_sg ? dm_request_save() : 0;
    atomic_store(&g_notice_req, 1); LOG("存放：完成——编年史 %d、储藏箱 %d、百宝囊 %d、留在背包 %d → 提示「%s」；请存盘 → %d", g_sv, g_sg, g_sp, g_sleft, g_notice, sv); g_ist = -1;
}



static uint64_t find_w(uint64_t w, const char *name, int dep) {
    if (!w || dep > 14) return 0; const char *nm = (const char *)*(uint64_t *)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = find_w(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static int ours(uint64_t w) { const char *nm = w ? (const char *)*(uint64_t *)(w + 8) : NULL; return nm && (!strncmp(nm, "TouchPouch", 10) || !strcmp(nm, "TouchStore")); }
static void w_move(uint64_t w, int x, int y) { if (!ours(w)) return; int32_t *r = (int32_t *)(w + 0x70); if (r[0] == x && r[1] == y) return; r[0] = x; r[1] = y; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w); }
static void w_text(uint64_t w, const char *s) {
    if (!ours(w) || *(uint64_t *)w - (uint64_t)(uintptr_t)B != 0x20ba4c8) return; size_t n = strlen(s);
    if (*(uint64_t *)(w + 0x90) == n && !memcmp(*(const char **)(w + 0x88), s, n)) return;
    ((void (*)(uint64_t, const char *, uint64_t))(B + 0x136e5c))(w + 0x88, s, n); ((void (*)(uint64_t))(B + 0xadc434))(w); }
static const char *str_of(uint32_t code) { char k[5] = { 0 }; memcpy(k, &code, 4); for (int i = 3; i >= 0 && k[i] == ' '; i--) k[i] = 0;
    struct { const char *k; uint64_t n; } s = { k, strlen(k) }; const char *r = dm_untag(((const char *(*)(void *))(B + 0x7f5630))(&s)); return r && *r ? r : "?"; }
static int g_tip;
static void tip_show(uint64_t root, int on) {
    uint64_t tb = find_w(root, "TouchPouchTip", 1), bg = find_w(root, "TouchPouchTipBg", 1), box = find_w(root, "TouchPouch", 1); if (!tb || !bg || !box) return;
    if (!on) { w_move(tb, 99999, 0); w_move(bg, 99999, 0); g_tip = 0; return; }
    uint32_t cc[64]; int cn[64], nc = 0;
    pthread_mutex_lock(&g_umu); for (int i = 0; i < g_nu; i++) { int j = 0; while (j < nc && cc[j] != g_u[i].code) j++; if (j == nc) { if (nc == 64) continue; cc[nc] = g_u[i].code; cn[nc++] = 0; } cn[j]++; } pthread_mutex_unlock(&g_umu);
    char s[1024]; int k = snprintf(s, sizeof s, "%s", dm_str(B, "dmPouchTitle"));
    if (!nc) k += snprintf(s + k, sizeof s - k, "\n%s", dm_str(B, "dmPouchEmpty"));
    for (int i = 0; i < nc && k < (int)sizeof s - 64; i++) k += snprintf(s + k, sizeof s - k, "\n%s ×%d", str_of(cc[i]), cn[i]);
    w_text(tb, s); g_tip = 1;
    int32_t *br = (int32_t *)(box + 0x70), w = *(int32_t *)(tb + 0x78), h = *(int32_t *)(tb + 0x7c); if (w <= 0 || h <= 0) { w = 500; h = 300; }
    int x = br[0] + br[2] / 2 - w / 2, y = br[1] - h - 70; if (x < 40) x = 40;
    w_move(tb, x, y); int32_t *r = (int32_t *)(bg + 0x70); r[2] = w + 80; r[3] = h + 60; w_move(bg, x - 40, y - 30);
}


int dm_pouch_tip_gate(int x, int y) {
    if (!g_tip || !B) return 0; uint64_t root = *(uint64_t *)(B + 0x2f05458), box = find_w(root, "TouchPouch", 1);
    if (box && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)box + 0x48))(box, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1)) return 0;
    tip_show(root, 0); LOG("百宝囊：点了清单外 → 收起清单，这一下不做别的"); return 1;
}





static void pouch_count_sync(uint64_t boxw) { char n[16]; snprintf(n, sizeof n, "%d", atomic_load(&g_ucount)); w_text(find_w(boxw, "TouchPouchCount", 1), n); }
static void ui_pump(void) {


    uint64_t root = *(uint64_t *)(B + 0x2f05458), inv = root ? dm_ui_top(root, "PlayerInventoryExpansionLayout") : 0, boxw = inv ? find_w(inv, "TouchPouchBox", 3) : 0;
    if (dm_ui_top_chk_on()) { uint64_t old = root ? find_w(root, "TouchPouchBox", 2) : 0; if (old != boxw) { static int k; if (k++ < 20) LOG("【红】百宝囊对拍：TouchPouchBox 新找法 %#llx、旧找法 %#llx", (unsigned long long)boxw, (unsigned long long)old); } }
    if (!boxw) return;
    int gp = dm_custom_get("gem_pick"), sb = dm_custom_get("store_btn");
    w_move(find_w(boxw, "TouchPouch", 1), gp ? 222 : 99999, 1294); w_move(find_w(boxw, "TouchPouchIcon", 1), gp ? 246 : 99999, 1310); w_move(find_w(boxw, "TouchPouchCount", 1), gp ? 350 : 99999, 1294);
    w_move(find_w(boxw, "TouchStore", 1), sb ? 916 : 99999, 1323);
    pouch_count_sync(boxw);
    static int shown = -1; int gen = atomic_load(&g_ugen); if (gen != shown) { shown = gen; if (g_tip) tip_show(root, 1); }
    if (atomic_exchange(&g_pouch_tap, 0)) tip_show(root, !g_tip);
    if (g_tip) tip_show(root, 1);
}

void dm_pouch_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (!cat || !act || !arg || strcmp(cat, "PanelManager") || strcmp(act, "ClosePanel")) return;
    if (!strcmp(arg, "TouchPouch")) atomic_store(&g_pouch_tap, 1);
    else if (!strcmp(arg, "TouchStore") || !strcmp(arg, "TouchPadStore")) atomic_store(&g_store_req, 1);
}
void dm_pouch_step(uint8_t *b) {
    if (!b) return; B = b; uint64_t fr = dm_guest_frame();
    atomic_store(&g_stash_open, (dm_ui_panel_state() & 1) != 0);
    ui_pump();
    uint64_t pl = cli_player(); if (!pl) { memset(g_pk, 0, sizeof g_pk); memset(g_gnd, 0, sizeof g_gnd); g_ist = -1; return; }
    uint32_t pm = *(uint32_t *)(pl + 0xc); if (pm == 0 || pm == 17) return;
    uint64_t inv = *(uint64_t *)(pl + 0x90); if (!inv || *(uint32_t *)inv != 0x01020304) return;
    { static long t_at = -1; if (t_at < 0) { const char *e = getenv("DM_TEST_STORE"); t_at = e ? atol(e) : 0; }
      if (t_at > 0 && fr >= (uint64_t)t_at) { t_at = 0; atomic_store(&g_store_req, 1); LOG("测试 DM_TEST_STORE：帧 %llu 当作点了「存放」", (unsigned long long)fr); } }
    if (atomic_exchange(&g_store_req, 0)) { if (!dm_custom_get("store_btn")) LOG("存放：按钮已在自定义页关掉，不做"); else if (g_ist >= 0) LOG("存放：上一次还没做完"); else store_begin(pl); }
    if (*(uint64_t *)(inv + 0x40)) return;
    store_step(pl, fr);
    if (dm_custom_get("gem_pick") && g_ist < 0) auto_pick(pl, fr);
}
void dm_pouch_srv_tick(uint8_t *b, uint64_t G, uint64_t player) {
    static uint64_t lp; if (!player) { lp = 0; pthread_mutex_lock(&g_umu); g_uload = 0; pthread_mutex_unlock(&g_umu); return; }
    B = b; uint32_t tq = *(uint32_t *)(G + 0x170);
    if (player != lp) { lp = player; uint64_t pd = *(uint64_t *)(player + 0x10); pthread_mutex_lock(&g_umu); strlcpy(g_uchar, pd ? (const char *)pd : "", sizeof g_uchar); pouch_load(); g_uload = 1; pthread_mutex_unlock(&g_umu); }
    { int dm_itemjob_owner(void); if (dm_itemjob_owner()) return; }
    int near = ((int (*)(uint64_t, uint64_t, int, int))(b + 0x650d10))(G, player, 0x10b, 0x32) & 1; atomic_store(&g_near_pub, near);
    uint32_t unit = adv_stash_unit(player); int go63 = near && unit;
    typedef struct { uint32_t id, t; } watch; static watch wl[64]; static int nw;
    int full_notice = 0;
    int saved = 0;
    pthread_mutex_lock(&g_umu);
    for (unsigned r = atomic_load(&g_absr); r != atomic_load(&g_absw); r++) { uint32_t id = atomic_load(&g_absq[r % 64]); atomic_store(&g_absr, r + 1);
        if (go63) { move_to_stash(b, G, player, id, unit); if (nw < 64) wl[nw++] = (watch){ id, tq }; LOG("百宝囊：物品 id %u 在储藏箱旁 → 原版移到仓库（高级仓库单位 %u）", id, unit); }
        else if (!unit) LOG("百宝囊：物品 id %u —— 这个角色没有高级仓库，不收", id);
        else if (pouch_put(b, G, player, id) > 0) saved = 1; }
#ifndef DM_PLAYER_BUILD






    { typedef struct { char code[5]; int kind, left; } holdy; static holdy q[8]; static int nq, iq, at = -1; static const char *file;
      if (at < 0) { const char *e = getenv("DM_TEST_HOLDY"); at = 0;
          if (e && e[0] == '@') file = e + 1;
          else if (e) { holdy h = { { 0 }, 0, 1 }; int k = sscanf(e, "%d:%4[^:]:%x:%d", &at, h.code, &h.kind, &h.left); if (k < 3) at = 0; else { if (k < 4 || h.left < 1) h.left = 1; q[nq++] = h; } } }
      if (file && iq == nq) { FILE *fp = fopen(file, "r");
          if (fp) { char ln[64]; nq = iq = 0;
              while (nq < 8 && fgets(ln, sizeof ln, fp)) { holdy h = { { 0 }, 0, 1 }; int k = sscanf(ln, "%4[^:]:%x:%d", h.code, &h.kind, &h.left); if (k >= 2) { if (k < 3 || h.left < 1) h.left = 1; q[nq++] = h; } }
              fclose(fp); remove(file); at = 1; LOG("DM_TEST_HOLDY: %d moves read from the file", nq); } }
      while (iq < nq && q[iq].left <= 0) iq++;
      if (at > 0 && tq >= (uint32_t)at && iq < nq) { holdy *h = &q[iq]; h->left--; const char *code = h->code; int kind = h->kind;
          uint64_t inv = *(uint64_t *)(player + 0x90), it = inv && *(uint32_t *)inv == 0x01020304 ? *(uint64_t *)(inv + 0x10) : 0, f = 0;
          for (int k = 0; it && k < 2048 && !f; k++) { uint64_t d = *(uint64_t *)(it + 0x10); if (!d) break; uint32_t c = item_code(it); if (*(uint32_t *)(it + 0xc) == 0 && *(uint8_t *)(d + 0x55) == 0 && !memcmp(&c, code, strlen(code))) f = it; else it = *(uint64_t *)(d + 0xb0); }
          if (f) { uint64_t pp = *(uint64_t *)(f + 0x38); uint32_t id = *(uint32_t *)(f + 8), sh = 0; uint16_t x = pp ? (uint16_t)*(int32_t *)(pp + 0x10) : 0, y = pp ? (uint16_t)*(int32_t *)(pp + 0x14) : 0; uint8_t pk[29]; memset(pk, 0, sizeof pk);
              for (uint64_t en = inv ? *(uint64_t *)(inv + 0x68) : 0, n = 0; en && n < 16 && !sh; en = *(uint64_t *)(en + 0x10), n++) if (*(uint8_t *)(en + 9) && !*(uint8_t *)(en + 0xa)) sh = *(uint32_t *)en;
              int ln; pk[0] = (uint8_t)kind; memcpy(pk + 1, &id, 4);
              if (kind == 0x55) { memcpy(pk + 5, &sh, 4); memcpy(pk + 13, &x, 2); memcpy(pk + 15, &y, 2); pk[17] = 4; pk[21] = 0x0d; pk[23] = 9; ln = 29; }
              else { memcpy(pk + 9, &x, 2); memcpy(pk + 11, &y, 2); pk[13] = 4; pk[17] = 0x0d; pk[19] = 9; ln = 21;


                  uint16_t cell[4] = { 0 }; uint32_t cy = 0; int grid = *(uint8_t *)(G + 0x106) == 1 ? 8 : 10;
                  int found = ((int (*)(uint64_t, uint64_t, int, void *, uint32_t *, int))(b + 0x55f620))(inv, f, grid, cell, &cy, 4);
                  if (found) { memcpy(pk + 17, &cell[0], 2); uint16_t y16 = (uint16_t)cy; memcpy(pk + 19, &y16, 2); } }
              *(uint32_t *)(G + 0x19050) = 1; int took = ((int (*)(uint64_t, uint64_t, uint8_t *, uint32_t))(b + 0x6e5b40))(G, player, pk, (uint32_t)ln); *(uint32_t *)(G + 0x19050) = 0;
              uint64_t fd = *(uint64_t *)(f + 0x10);
              LOG("DM_TEST_HOLDY: tick %u, %s id %u from cell %u,%u, packet 0x%02x, dispatch %d, now mode %u page %u", tq, code, id, x, y, kind, took, *(uint32_t *)(f + 0xc), fd ? *(uint8_t *)(fd + 0x55) : 255); }
          else { LOG("DM_TEST_HOLDY: tick %u, no %s left in the backpack", tq, code); h->left = 0; } } }
#endif
    for (int i = 0; i < g_c2s_nq; i++) { uint32_t id = g_c2s_q[i];
        if (go63 && bag_item(player, id)) { move_to_stash(b, G, player, id, unit); if (nw < 64) wl[nw++] = (watch){ id, tq }; LOG("珍宝匣：物品 id %u 已回到背包 → 原版移到仓库（高级仓库单位 %u）", id, unit); }
        else LOG("珍宝匣：物品 id %u 改道后%s，留在背包", id, go63 ? "不在背包里" : "人已离开储藏箱"); }
    g_c2s_nq = 0; if (g_c2s_full) { g_c2s_full = 0; full_notice = 1; }
    for (int i = 0; i < g_nready; i++) {
        if (go63) { move_to_stash(b, G, player, g_ready[i], unit); if (nw < 64) wl[nw++] = (watch){ g_ready[i], tq }; }
        else if (pouch_put(b, G, player, g_ready[i]) > 0) { saved = 1; LOG("百宝囊：物品 id %u 落盘证明时已离开储藏箱，收回囊里", g_ready[i]); } }
    g_nready = 0;
    for (int i = 0; i < nw; ) {
        if (!bag_item(player, wl[i].id)) { wl[i] = wl[--nw]; saved = 1; continue; }
        if (tq - wl[i].t >= 50) {
            LOG("百宝囊：物品 id %u 移到仓库 2 秒还在背包（那一格叠满 / 放不下）→ 留在背包，提示一次", wl[i].id); full_notice = 1; wl[i] = wl[--nw]; continue; }
        i++; }
    {
        static uint32_t last; int puts = 0; for (int j = 0; j < g_nup; j++) puts += g_up[j].put;




        uint64_t pinv = *(uint64_t *)(player + 0x90), phand = pinv && *(uint32_t *)pinv == 0x01020304 ? *(uint64_t *)(pinv + 0x40) : 0;
        static uint64_t lh; if (phand && phand != lh && atomic_load(&g_stash_open) && g_nu) LOG("百宝囊：手上拿着东西 → 先不往背包放，等放下"); lh = phand;
        if (atomic_load(&g_stash_open) && go63 && g_nu && !puts && nw == 0 && !phand && tq - last >= 25) { last = tq; int n = 0; while (pouch_take(b, G, player)) n++;
            if (n) { saved = 1; LOG("百宝囊：开着储藏箱 → 放 %d 件到背包，囊里还剩 %d 件", n, g_nu); } } }
    full_notice |= stash_sweep(b, G, player, go63 && atomic_load(&g_stash_open) ? unit : 0, tq, &saved);
    { static int said_open; if (!atomic_load(&g_stash_open)) said_open = 0;
      if (full_notice && !said_open) { said_open = 1; snprintf(g_notice, sizeof g_notice, "%s", dm_str(b, "dmStoreFull")); atomic_store(&g_notice_req, 1); } }
    int pend = g_nup + g_udirty; pthread_mutex_unlock(&g_umu);
    { static uint32_t lastreq; if (saved || (pend && tq - lastreq >= 50)) { lastreq = tq; int r = getenv("DM_TEST_POUCH_NOSAVE") ? -9 : dm_request_save(); if (!saved) LOG("百宝囊：还有 %d 条没证明，再请一次存盘 → %d", pend, r); } }
    if (atomic_exchange(&g_notice_req, 0)) dm_game_notice(b, g_notice);
    {
        static int per = -1; static uint64_t last; if (per < 0) { const char *e = getenv("DM_TEST_NOTICE"); per = e ? atoi(e) : 0; }
        uint64_t dm_guest_frame(void); uint64_t f = dm_guest_frame(); if (per > 0 && f - last >= (uint64_t)per) { last = f; dm_game_notice(b, "TEST NOTICE 0123456789"); LOG("测试 DM_TEST_NOTICE：帧 %llu 发一条系统提示", (unsigned long long)f); } }
}

