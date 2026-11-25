























#include "dm_bossrush.h"
#include "dm_feats.h"
#include "dm_input.h"
#include "dm_cursor.h"
#include "dm.h"
#include "dm_rift.h"
#include "hle.h"
#include "dm_i18n.h"
#include "dm_widget.h"
#include "dm_tab_text.h"
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

const char *dm_save_workdir(void);

enum { K_CLS, K_SU, K_GRP };
typedef struct { const char *key, *str; uint8_t act, kind; int16_t id, id2; } kb_row;
static const kb_row k_rows[] = {
    {"andariel", "Andariel", 1, K_CLS, 156, -1}, {"countess", "The Countess", 1, K_SU, 6, -1}, {"bloodraven", "Bloodraven", 1, K_CLS, 267, -1},
    {"cowking", "The Cow King", 1, K_SU, 39, -1}, {"bishibosh", "Bishibosh", 1, K_SU, 0, -1}, {"corpsefire", "Corpsefire", 1, K_SU, 40, -1},
    {"smith", "The Smith", 1, K_SU, 20, -1}, {"griswold", "Griswold", 1, K_SU, 5, -1}, {"rakanishu", "Rakanishu", 1, K_SU, 3, -1},
    {"coldcrow", "Coldcrow", 1, K_SU, 2, -1}, {"treehead", "Treehead WoodFist", 1, K_SU, 4, -1},
    {"duriel", "Duriel", 2, K_CLS, 211, -1}, {"summoner", "Summoner", 2, K_CLS, 250, 18}, {"radament", "Radament", 2, K_CLS, 229, 10},
    {"kaa", "Ancient Kaa the Soulless", 2, K_SU, 19, -1}, {"coldworm", "Coldworm the Burrower", 2, K_SU, 15, -1}, {"fangskin", "Fangskin", 2, K_SU, 12, -1},
    {"bloodwitch", "Bloodwitch the Wild", 2, K_SU, 11, -1}, {"beetleburst", "Beetleburst", 2, K_SU, 13, -1}, {"darkelder", "Dark Elder", 2, K_SU, 17, -1},
    {"fireeye", "Fire Eye", 2, K_SU, 16, -1},
    {"mephisto", "Mephisto", 3, K_CLS, 242, -1}, {"council", "Council Member", 3, K_GRP, 0, -1},
    {"bremm", "Bremm Sparkfist", 3, K_SU, 28, -1}, {"wyand", "Wyand Voidfinger", 3, K_SU, 30, -1}, {"maffer", "Maffer Dragonhand", 3, K_SU, 31, -1},
    {"sszark", "Web Mage the Burning", 3, K_SU, 21, -1}, {"endugu", "Witch Doctor Endugu", 3, K_SU, 22, -1}, {"stormtree", "Stormtree", 3, K_SU, 23, -1},
    {"sarina", "Sarina the Battlemaid", 3, K_SU, 24, -1}, {"icehawk", "Icehawk Riftwing", 3, K_SU, 25, -1}, {"wingeddeath", "Winged Death", 3, K_SU, 32, -1},
    {"tormentor", "The Tormentor", 3, K_SU, 33, -1}, {"taintbreeder", "Taintbreeder", 3, K_SU, 34, -1}, {"riftwraith", "Riftwraith the Cannibal", 3, K_SU, 35, -1},
    {"diablo", "Diablo", 4, K_CLS, 243, -1}, {"vizier", "Grand Vizier of Chaos", 4, K_SU, 38, -1}, {"deseis", "Lord De Seis", 4, K_SU, 37, -1},
    {"infector", "Infector of Souls", 4, K_SU, 36, -1}, {"izual", "izual", 4, K_CLS, 256, -1}, {"hephasto", "The Feature Creep", 4, K_SU, 41, -1},
    {"baal", "Baal Crab", 5, K_CLS, 544, -1}, {"nihlathak", "Nihlathak", 5, K_SU, 60, -1}, {"shenk", "Siege Boss", 5, K_SU, 42, -1},
    {"eldritch", "Megaflow Rectifier", 5, K_SU, 49, -1}, {"pindleskin", "Pindleskin", 5, K_SU, 52, -1}, {"dacfarren", "Dac Farren", 5, K_SU, 48, -1},
    {"threshsocket", "Threash Socket", 5, K_SU, 51, -1}, {"snapchip", "Snapchip Shatter", 5, K_SU, 53, -1}, {"frozenstein", "Frozenstein", 5, K_SU, 59, -1},
    {"ancients", NULL, 5, K_GRP, 1, -1},
    {"colenzo", "Baal Subject 1", 5, K_SU, 61, -1}, {"achmel", "Baal Subject 2", 5, K_SU, 62, -1}, {"bartuc", "Baal Subject 3", 5, K_SU, 63, -1},
    {"ventar", "Baal Subject 4", 5, K_SU, 64, -1}, {"lister", "Baal Subject 5", 5, K_SU, 65, -1},
    {"bonesaw", "Bonesaw Breaker", 5, K_SU, 47, -1}, {"axedweller", "Axe Dweller", 5, K_SU, 46, -1}, {"eyeback", "Eyeback Unleashed", 5, K_SU, 50, -1},
    {"sharptooth", "Sharp Tooth Sayer", 5, K_SU, 56, -1}, {"vinvear", "Vinvear Molech", 5, K_SU, 55, -1}, {"magma", "Magma Torquer", 5, K_SU, 57, -1},
    {"hellsbelle", "Anodized Elite", 5, K_SU, 54, -1}, {"blazeripper", "Blaze Ripper", 5, K_SU, 58, -1},
};
enum { NROW = sizeof k_rows / sizeof *k_rows };
_Static_assert(NROW == DM_FT_BOARD_ROWS && DM_FT_DIFFS == 3, "the achievements know the board's rows");
static const int16_t k_grp_su[2][3] = { {26, 27, 29}, {43, 44, 45} };
static const char *k_grp_str[2][3] = { {"Ismail Vilehand", "Geleb Flamefinger", "Toorc Icefist"}, {"Ancient Barbarian 1", "Ancient Barbarian 2", "Ancient Barbarian 3"} };

static struct { int32_t n; int64_t first; int32_t best, last; int64_t sum; int32_t nt, uq, st; } S[NROW][3];


static char g_name[64];
static int g_loaded;
static struct { uint32_t id; int16_t row; int8_t grp, gm; uint8_t announced, dead, skip; uint32_t eng; } E[128];
static int nE;
static struct { uint8_t announced, mask; uint32_t eng; int uq, st; } GS[2];
static uint64_t g_G; static uint32_t g_last_t;




static struct { int16_t row; int8_t d, grp; uint32_t t_die, max_item; int x, y; } P[16]; static int nP; static uint32_t g_maxit;
static uint8_t *B;

static uint64_t rd(uint64_t a) { return a ? *(volatile uint64_t *)(uintptr_t)a : 0; }
static const char *str_of(const char *key) { struct { const char *k; uint64_t n; } s = { key, strlen(key) }; const char *r = dm_untag(((const char *(*)(void *))(B + 0x7f5630))(&s)); return r && *r ? r : key; }
static void row_name(int r, char *out, size_t cap) {
    const kb_row *k = &k_rows[r];
    if (k->kind == K_GRP && k->id == 1) { snprintf(out, cap, dm_str(B, "dmKbJoin3"), str_of(k_grp_str[1][0]), str_of(k_grp_str[1][1]), str_of(k_grp_str[1][2])); return; }
    strlcpy(out, str_of(k->str), cap);
}
static int64_t now_s(void) { const char *e = getenv("DM_FIXED_TIME"); return e ? strtoll(e, 0, 0) : (int64_t)time(NULL); }

static int file_path(char *out, size_t cap) { const char *w = dm_save_workdir(); if (!w || !g_name[0]) return 0; snprintf(out, cap, "%s/%s/%s.sw_kills", w, g_name, g_name); return 1; }
static void load(void) {
    memset(S, 0, sizeof S); g_loaded = 0; char p[1200], ln[256]; if (!file_path(p, sizeof p)) return; errno = 0; FILE *f = fopen(p, "r");
    if (!f) { g_loaded = errno == ENOENT; LOG("kill board: no record for %s yet (%s)%s", g_name, p, g_loaded ? "" : ": cannot open"); return; } int n = 0;
    while (fgets(ln, sizeof ln, f)) { char key[40]; int d, c, b, la = 0, nt = 0, uq = 0, st = 0; long long t, sum = 0;
        if (ln[0] == '#' || sscanf(ln, "%39s %d %d %lld %d %d %lld %d %d %d", key, &d, &c, &t, &b, &la, &sum, &nt, &uq, &st) < 5 || d < 0 || d > 2) continue;
        for (int r = 0; r < NROW; r++) if (!strcmp(k_rows[r].key, key)) { S[r][d].n = c; S[r][d].first = t; S[r][d].best = b; S[r][d].last = la; S[r][d].sum = sum; S[r][d].nt = nt; S[r][d].uq = uq; S[r][d].st = st; n++; } }
    g_loaded = !ferror(f); fclose(f); LOG("kill board: loaded %s, %d rows%s", g_name, n, g_loaded ? "" : " (read error)");
}
static void save(void) {
    char p[1200], t[1220]; if (!file_path(p, sizeof p)) return; snprintf(t, sizeof t, "%s.tmp", p);
    FILE *f = fopen(t, "w"); if (!f) { LOG("【红】刷怪积分牌：写 %s 打不开", t); return; }
    fprintf(f, "# dm_kills 2\n# 键 难度(0 普通/1 噩梦/2 地狱) 次数 首杀时刻(Unix 秒) 最快 最近(交战拍数，25 拍/秒，0 = 无) 量到用时的总拍数 次数 暗金件数 套装件数\n");
    for (int r = 0; r < NROW; r++) for (int d = 0; d < 3; d++) if (S[r][d].n || S[r][d].uq || S[r][d].st)
        fprintf(f, "%s %d %d %lld %d %d %lld %d %d %d\n", k_rows[r].key, d, S[r][d].n, (long long)S[r][d].first, S[r][d].best, S[r][d].last, (long long)S[r][d].sum, S[r][d].nt, S[r][d].uq, S[r][d].st);
    if (fclose(f) || rename(t, p)) { remove(t); LOG("【红】刷怪积分牌：写 %s 失败", p); }
}



enum { KLUT_CLS = 1024, KLUT_SU = 256 };
static int8_t k_cls_row[KLUT_CLS], k_su_row[KLUT_SU], k_su_grp[KLUT_SU];
static void lut_init(void) {
    static int done; if (done) return; done = 1;
    for (int r = NROW - 1; r >= 0; r--) {
        if (k_rows[r].kind == K_CLS && k_rows[r].id >= 0 && k_rows[r].id < KLUT_CLS) k_cls_row[k_rows[r].id] = (int8_t)(r + 1);
        int su = k_rows[r].kind == K_SU ? k_rows[r].id : k_rows[r].kind == K_CLS ? k_rows[r].id2 : -1;
        if (su >= 0 && su < KLUT_SU) k_su_row[su] = (int8_t)(r + 1); }
    for (int g = 1; g >= 0; g--) for (int m = 2; m >= 0; m--) k_su_grp[k_grp_su[g][m]] = (int8_t)(g * 3 + m + 1);
}
static int row_of(uint64_t u, int *grp, int *gm) {
    *grp = -1; int cls = (int)*(uint32_t *)(uintptr_t)(u + 4);
    if (cls >= 0 && cls < KLUT_CLS && k_cls_row[cls]) return k_cls_row[cls] - 1;
    uint64_t md = rd(u + 0x10); if (!md || !(*(uint8_t *)(uintptr_t)(md + 0x1a) & 2)) return -1;
    int su = *(uint16_t *)(uintptr_t)(md + 0x2a); if (su >= KLUT_SU) return -1;
    if (k_su_row[su]) return k_su_row[su] - 1;
    if (k_su_grp[su]) { int g = (k_su_grp[su] - 1) / 3; *grp = g; *gm = (k_su_grp[su] - 1) % 3; for (int r = 0; r < NROW; r++) if (k_rows[r].kind == K_GRP && k_rows[r].id == g) return r; }
    return -1;
}
static int client_has(uint32_t id) {
    int k = 0; for (uint64_t c = rd((uint64_t)(uintptr_t)(B + 0x2d603a0 + 1 * 0x400 + (id & 0x7f) * 8)); c && k < 64; c = rd(c + 0x158), k++) if (*(uint32_t *)(uintptr_t)(c + 8) == id) return 1;
    return 0;
}



int dm_kills_board(const char *who, dm_ft_board_row *out, int max) {
    if (!who || !who[0] || strcmp(g_name, who) || !g_loaded || max < NROW) return -1;
    for (int r = 0; r < NROW; r++) {
        out[r].cls = k_rows[r].kind == K_CLS ? k_rows[r].id : -1;
        for (int d = 0; d < 3; d++) { out[r].n[d] = S[r][d].n; out[r].best[d] = S[r][d].best; }
    }
    return NROW;
}
static void counted(int r, int d, uint32_t eng, uint32_t t) {
    S[r][d].n++; if (S[r][d].n == 1) S[r][d].first = now_s();
    int dt = eng && t >= eng ? (int)(t - eng) : 0; if (dt > 0 && (!S[r][d].best || dt < S[r][d].best)) S[r][d].best = dt;
    if (dt > 0) { S[r][d].last = dt; S[r][d].sum += dt; S[r][d].nt++; }
    char nm[160], line[200]; row_name(r, nm, sizeof nm); snprintf(line, sizeof line, dm_str(B, "dmKbKilledNth"), nm, S[r][d].n); dm_game_notice(B, line);
    LOG("刷怪积分牌：拍 %u %s 难度 %d 第 %d 次（交战 %d 拍，最快 %d 拍）", t, k_rows[r].key, d, S[r][d].n, dt, S[r][d].best); save();
}

void dm_kills_tick(uint64_t G, uint64_t player, uint8_t *main_base) {
    B = main_base; if (!G || !player || !B) return;
    lut_init();
    uint32_t t = *(volatile uint32_t *)(uintptr_t)(G + 0x170);
    if (G != g_G || t < g_last_t) { g_G = G; nE = 0; nP = 0; g_maxit = 0; memset(GS, 0, sizeof GS); g_name[0] = 0; }
    g_last_t = t;
    if (!g_name[0]) { uint64_t pd = rd(player + 0x10); if (!pd) return; char nm[64]; strlcpy(nm, (const char *)(uintptr_t)pd, sizeof nm); if (!nm[0] || strchr(nm, '/')) return; strlcpy(g_name, nm, sizeof g_name); load(); }
    int d = *(uint8_t *)(uintptr_t)(G + 0x104); if (d > 2) return;


    { static int at = -1; if (at < 0) { const char *e = getenv("DM_TEST_SLAY"); at = e ? atoi(e) : 0; }
      if (at > 0 && t >= (uint32_t)at) { at = 0; int nk = 0;
          for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 1 * 0x400 + 8 * b); u; u = rd(u + 0x158)) { int gg, gm; uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc);
              if (row_of(u, &gg, &gm) < 0 || m == 0 || m == 12) continue; nk++; ((void (*)(uint64_t, int, int, int))(B + 0x5bc1f0))(u, 6, 256, 0); }
          LOG("测试 DM_TEST_SLAY：拍 %u 名单上 %d 只生命设成 1 点", t, nk); } }




    { static int at = -1; static uint32_t ids[8]; static int n; if (at < 0) { const char *e = getenv("DM_TEST_PICK_UQ"); at = e ? atoi(e) : 0; }
      if (at > 0 && t == (uint32_t)at) { uint64_t pp = rd(player + 0x38); int px = pp ? *(uint16_t *)(uintptr_t)(pp + 2) : 0, py = pp ? *(uint16_t *)(uintptr_t)(pp + 6) : 0;
          for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2e30 + 8 * b); u; u = rd(u + 0x158)) { uint64_t dd = rd(u + 0x10), ip = rd(u + 0x38);
              if (*(uint32_t *)(uintptr_t)u != 4 || *(uint32_t *)(uintptr_t)(u + 0xc) != 3 || !dd || !ip || (*(uint32_t *)(uintptr_t)dd != 7 && *(uint32_t *)(uintptr_t)dd != 5) || n >= 8) continue;
              if (abs(*(int32_t *)(uintptr_t)(ip + 0x10) - px) > 20 || abs(*(int32_t *)(uintptr_t)(ip + 0x14) - py) > 20) continue;
              uint8_t pk[17] = { 0x16 }; uint32_t one = 1; memcpy(pk + 1, (void *)(uintptr_t)(u + 8), 4); memcpy(pk + 5, (void *)(uintptr_t)(ip + 0x10), 4); memcpy(pk + 9, (void *)(uintptr_t)(ip + 0x14), 4); memcpy(pk + 13, &one, 4);
              ids[n++] = *(uint32_t *)(uintptr_t)(u + 8);
              *(uint32_t *)(uintptr_t)(G + 0x19050) = 1; ((void (*)(uint64_t, uint64_t, uint8_t *, uint32_t))(B + 0x6e5b40))(G, player, pk, 17); *(uint32_t *)(uintptr_t)(G + 0x19050) = 0;
              LOG("测试 DM_TEST_PICK_UQ：拍 %u 捡暗金 / 套装 id %u（类号 %u）→ 模式 %u", t, ids[n - 1], *(uint32_t *)(uintptr_t)(u + 4), *(uint32_t *)(uintptr_t)(u + 0xc)); } }
      if (at > 0 && t == (uint32_t)at + 5) for (int i = 0; i < n; i++) { uint64_t dm_diag_srv_item(uint32_t); uint64_t si = dm_diag_srv_item(ids[i]);
          int r = si ? ((int (*)(uint64_t, uint64_t, uint64_t))(B + 0x78acf0))(G, player, si) : -1; LOG("测试 DM_TEST_PICK_UQ：鉴定 id %u → %d", ids[i], r); } }
    int alive_any = 0;
    for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2230 + 1 * 0x400 + 8 * b); u; u = rd(u + 0x158)) {
        int grp, gm = 0, r = row_of(u, &grp, &gm); if (r < 0) continue;
        uint32_t id = *(uint32_t *)(uintptr_t)(u + 8), m = *(uint32_t *)(uintptr_t)(u + 0xc); int dead = m == 0 || m == 12;
        int e = 0; while (e < nE && E[e].id != id) e++;
        if (e == nE) { if (nE == (int)(sizeof E / sizeof *E)) { memmove(E, E + 1, sizeof E - sizeof *E); nE--; e--; }
            memset(&E[e], 0, sizeof *E); E[e].id = id; E[e].row = (int16_t)r; E[e].grp = (int8_t)grp; E[e].gm = (int8_t)gm; nE++;
            E[e].skip = dead || dm_bossrush_owns(id); E[e].dead = (uint8_t)dead;
            LOG("刷怪积分牌：拍 %u 见到 %s（单位 id %u，模式 %u%s）", t, k_rows[r].key, id, m, E[e].skip ? "，不计" : ""); }
        if (E[e].skip || E[e].dead) continue;
        alive_any = 1;
        if (!dead) {
            int hp = ((int (*)(uint64_t, int, int))(B + 0x5b9290))(u, 6, 0), mx = ((int (*)(uint64_t, int, int))(B + 0x5b9290))(u, 7, 0);
            uint32_t *eng = grp >= 0 ? &GS[grp].eng : &E[e].eng;
            if (hp < mx) { if (!*eng) *eng = t; } else if (grp < 0 || !GS[grp].mask) *eng = 0;
            if (!(grp >= 0 ? GS[grp].announced : E[e].announced) && client_has(id)) {
                if (grp >= 0) GS[grp].announced = 1; else E[e].announced = 1;
                char nm[160], line[200]; row_name(r, nm, sizeof nm); char f[200]; snprintf(line, sizeof line, dm_strn(B, "dmKbKilledTotal", S[r][d].n, f, sizeof f), nm, S[r][d].n); if (S[r][d].n > 0) dm_game_notice(B, line);
                LOG("刷怪积分牌：拍 %u 提示出现 %s（难度 %d 已 %d 次）", t, k_rows[r].key, d, S[r][d].n); }
            continue; }
        E[e].dead = 1; if (dm_rift_swept(id)) { E[e].skip = 1; LOG("刷怪积分牌：拍 %u %s（单位 id %u）是秘境结算后清掉的，不计", t, k_rows[r].key, id); continue; }
        if (nP < (int)(sizeof P / sizeof *P)) { uint64_t pa = rd(u + 0x38); P[nP].row = (int16_t)r; P[nP].d = (int8_t)d; P[nP].grp = (int8_t)grp; P[nP].t_die = t; P[nP].max_item = g_maxit;
            P[nP].x = pa ? *(uint16_t *)(uintptr_t)(pa + 2) : 0; P[nP].y = pa ? *(uint16_t *)(uintptr_t)(pa + 6) : 0; nP++; }




        { const char *e = getenv("DM_TEST_KILL_DROP"); if (e) { char buf[128]; strlcpy(buf, e, sizeof buf);
            for (char *sv, *tk = strtok_r(buf, ",", &sv); tk; tk = strtok_r(NULL, ",", &sv)) { char code[8] = ""; int q = 2; if (sscanf(tk, "%4[^:]:%d", code, &q) < 1) continue;
                uint32_t c = 0x20202020; memcpy(&c, code, strlen(code) > 4 ? 4 : strlen(code)); uint64_t it = ((uint64_t (*)(uint64_t, uint64_t, uint32_t, int))(B + 0x7c0b20))(G, u, c, q);
                LOG("测试 DM_TEST_KILL_DROP：%s 死时补掉「%s」品质 %d → %s", k_rows[r].key, code, q, it ? "成" : "败"); } } }
        if (grp < 0) { counted(r, d, E[e].eng, t); continue; }
        GS[grp].mask |= (uint8_t)(1 << gm); LOG("刷怪积分牌：拍 %u %s 第 %d 只死（%x）", t, k_rows[r].key, gm + 1, GS[grp].mask);
        if (GS[grp].mask == 7) { S[r][d].uq += GS[grp].uq; S[r][d].st += GS[grp].st; GS[grp].uq = GS[grp].st = 0; counted(r, d, GS[grp].eng, t); }
    }
    for (int i = 0; i < nP; i++) { if (t - P[i].t_die < 25) continue;
        int uq = 0, st = 0;
        for (int b = 0; b < 128; b++) for (uint64_t u = rd(G + 0x2e30 + 8 * b); u; u = rd(u + 0x158)) { uint64_t dd = rd(u + 0x10), pa = rd(u + 0x38); uint32_t m = *(uint32_t *)(uintptr_t)(u + 0xc);
            if (*(uint32_t *)(uintptr_t)u != 4 || *(uint32_t *)(uintptr_t)(u + 8) <= P[i].max_item || (m != 3 && m != 5) || !dd || !pa) continue;
            if (abs(*(int32_t *)(uintptr_t)(pa + 0x10) - P[i].x) > 20 || abs(*(int32_t *)(uintptr_t)(pa + 0x14) - P[i].y) > 20) continue;
            uint32_t q = *(uint32_t *)(uintptr_t)dd; uq += q == 7; st += q == 5; }
        int r = P[i].row, dg = P[i].d, g = P[i].grp;
        if (g >= 0 && GS[g].mask != 7) { GS[g].uq += uq; GS[g].st += st; } else { S[r][dg].uq += uq; S[r][dg].st += st; if (uq || st) save(); }
        LOG("刷怪积分牌：拍 %u %s 死后那一批掉落：暗金 %d、套装 %d", t, k_rows[r].key, uq, st);
        P[i--] = P[--nP]; }
    if (alive_any) { uint32_t mx = 0; for (int b = 0; b < 128; b++) for (uint64_t v = rd(G + 0x2e30 + 8 * b); v; v = rd(v + 0x158)) { uint32_t i = *(uint32_t *)(uintptr_t)(v + 8); if (i > mx) mx = i; } g_maxit = mx; }
}







enum { KB_ROW_H = 188 };
static _Atomic int g_kb_on, g_kb_act, g_kb_dirty = 1;
static uint64_t kb_find(uint64_t w, const char *name, int dep) {
    if (!w || dep > 12) return 0; const char *nm = *(const char **)(w + 8); if (nm && !strcmp(nm, name)) return w;
    uint64_t ch = *(uint64_t *)(w + 0x58), n = *(uint64_t *)(w + 0x60); if (!ch || n > 256) return 0;
    for (uint64_t i = 0; i < n; i++) { uint64_t r = kb_find(*(uint64_t *)(ch + 8 * i), name, dep + 1); if (r) return r; }
    return 0;
}
static uint8_t *kb_base(void) { static uint8_t *b; if (!b) for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) b = (uint8_t *)g_mod[i].base; return b; }
static uint64_t kb_board(void) { uint8_t *b = kb_base(); uint64_t root = b ? *(uint64_t *)(b + 0x2f05458) : 0, cp = root ? dm_ui_top(root, "ChroniclePanel") : 0; uint64_t bd = cp ? kb_find(cp, "TouchKillBoard", 1) : 0; return bd ? kb_find(bd, "TouchKB_Body", 0) : 0; }
static void kb_show(uint64_t w, int on) { const char *nm = (const char *)*(uint64_t *)(w + 8); if (!nm || strncmp(nm, "Touch", 5)) { LOG("【红】积分牌：要改可见的控件「%s」不是我们的，不写", nm ? nm : "?"); return; } *(uint8_t *)(w + 0x50) = (uint8_t)on; *(uint8_t *)(w + 0x51) = (uint8_t)on; }
static void kb_rect(uint64_t w, int x, int y, int h);
static void kb_body(uint64_t w, int on) { kb_rect(w, on ? 0 : 99999, 0, -1); }
void dm_kills_on_uimsg(const char *cat, const char *act, const char *arg) {
    if (!strcmp(cat, "DropdownListWidgetMessage")) { atomic_store(&g_kb_dirty, 1); return; }
    if (!strcmp(cat, "ChroniclePanelMessage") && !strcmp(act, "ToggleChildPanel")) { int on = !strcmp(arg, "TouchKillBoard"); atomic_store(&g_kb_on, on); atomic_store(&g_kb_dirty, 1);
        uint64_t w = kb_board(); if (w) kb_body(w, on); return; }
    if (strcmp(cat, "PanelManager") || strcmp(act, "ClosePanel") || strncmp(arg, "TouchKB_", 8)) return;
    if (!strncmp(arg + 8, "Act", 3)) atomic_store(&g_kb_act, atoi(arg + 11));
    atomic_store(&g_kb_dirty, 1);
}
static void kb_text(uint8_t *b, uint64_t w, const char *s) {
    if (!w || *(uint64_t *)w - (uint64_t)(uintptr_t)b != 0x20ba4c8) return;
    size_t n = strlen(s); if (*(uint64_t *)(w + 0x90) == n && !memcmp(*(const char **)(w + 0x88), s, n)) return;
    ((void *(*)(uint64_t, const char *, uint64_t))(b + 0x136e5c))(w + 0x88, s, n); ((void (*)(uint64_t))(b + 0xadc434))(w);
}
static void kb_rect(uint64_t w, int x, int y, int h) {
    int32_t *r = (int32_t *)(w + 0x70); if (r[0] == x && r[1] == y && (h < 0 || r[3] == h)) return; r[0] = x; r[1] = y; if (h >= 0) r[3] = h; ((void (*)(uint64_t))*(uint64_t *)(*(uint64_t *)w + 0x10))(w);
}
DM_WIDGET_RED_DEF
static void kb_toggle(uint64_t w, int on, int on_frame) {
    if (!DM_WIDGET_OK(kb_base(), w, DM_VT_TOGGLE, "刷怪积分牌")) return;
    int32_t f[4]; for (int i = 0; i < 4; i++) f[i] = on ? on_frame : 0;
    if (on_frame == 4) { static const int32_t fr[2][4] = { { 0, 1, 3, 2 }, { 4, 5, 6, -1 } }; memcpy(f, fr[on != 0], sizeof f); }
    *(uint8_t *)(w + 0xa88) = on != 0; memcpy((void *)(w + 0x6e8), f, sizeof f); *(int32_t *)(w + 0x610) = f[0];
}








static int kb_sort(uint8_t *b, uint64_t dd) {
    uint64_t t = dd && *(uint64_t *)(dd + 0x718) - (uint64_t)(uintptr_t)b == 0x20ba4c8 ? dd + 0x718 : 0; if (!t) { static int once; if (!once++) LOG("【红】刷怪积分牌：排序下拉里找不到文字控件，按幕顺序"); return 0; }
    const char *s = *(const char **)(t + 0x88); size_t n = *(uint64_t *)(t + 0x90); if (!s || n > 64) return 0;
    const char *c = dm_str(b, "dmKbSortCount"), *f = dm_str(b, "dmKbSortFast");
    return n == strlen(c) && !memcmp(s, c, n) ? 1 : n == strlen(f) && !memcmp(s, f, n) ? 2 : 0;
}
static void mmss(char *o, size_t cap, int ticks) { int s = (ticks + 12) / 25; snprintf(o, cap, "%d:%02d", s / 60, s % 60); }
static void fmt_thousands(char *o, size_t cap, long v) { char t[32]; int n = snprintf(t, sizeof t, "%ld", v), k = 0; size_t j = 0;
    for (int i = 0; i < n && j + 2 < cap; i++) { if (i && (n - i) % 3 == 0) o[j++] = ','; o[j++] = t[i]; k++; } o[j] = 0; (void)k; }
void dm_kills_ui_pump(void) {
    uint8_t *b = kb_base(); uint64_t bw = b ? kb_board() : 0; if (!bw) return;



    { int dm_padmode_current(void); uint64_t root = *(uint64_t *)(b + 0x2f05458), cp = root ? dm_ui_top(root, "ChroniclePanel") : 0, cx = cp ? kb_find(cp, "TouchCloseBox", 1) : 0; if (cx) kb_rect(cx, dm_padmode_current() ? 99999 : -998, 256, -1);
      if (cp) dm_tab_text_center(b, kb_find(cp, "ChronicleTabs", 1), dm_lang_index()); }
    int on = atomic_load(&g_kb_on); kb_body(bw, on); if (!on) return;
    if (!B) B = b;




    static int pad_scroll;
    { int dm_padmode_current(void); uint64_t dm_padmode_page_keys(uint64_t);
      if (dm_padmode_current()) { int rift = atomic_load(&g_kb_act) == 5;
          uint64_t ev = dm_padmode_page_keys(DM_BTN_UP | DM_BTN_DOWN | DM_BTN_LEFT | DM_BTN_RIGHT | DM_BTN_X | DM_BTN_Y); if (rift) ev &= ~(uint64_t)(DM_BTN_X | DM_BTN_Y);
          if (!rift) { void dm_padmode_legend(int, const char *); dm_padmode_legend(2, dm_str(b, "dmPadLegDiff")); dm_padmode_legend(1, dm_str(b, "dmPadLegSort")); }
          if (ev & (DM_BTN_LEFT | DM_BTN_RIGHT)) { int a = atomic_load(&g_kb_act) + ((ev & DM_BTN_RIGHT) ? 1 : -1); if (a >= 0 && a <= 5) { atomic_store(&g_kb_act, a); atomic_store(&g_kb_dirty, 1); if (a < 5) LOG("刷怪积分牌：手柄换到第 %d 幕", a + 1); else LOG("刷怪积分牌：手柄换到秘境项"); } }
          if (ev & DM_BTN_UP) pad_scroll = 1; else if (ev & DM_BTN_DOWN) pad_scroll = -1;
          if (ev & DM_BTN_X) { uint64_t c[3]; int cur = 0; for (int d = 0; d < 3; d++) { char nm[16]; snprintf(nm, sizeof nm, "TouchKB_D%d", d); c[d] = kb_find(bw, nm, 0); if (dm_widget_is(b, c[d], DM_VT_TOGGLE) && *(uint8_t *)(c[d] + 0xa88)) cur = d + 1; }
              int to = (cur + 1) % 4; for (int d = 0; d < 3; d++) if (c[d]) kb_toggle(c[d], to == d + 1, 4); LOG("刷怪积分牌：手柄难度筛选 → %s", to == 0 ? "全部" : to == 1 ? "普通" : to == 2 ? "噩梦" : "地狱"); }
          if (ev & DM_BTN_Y) { const char *const nm[3] = { dm_str(b, "dmKbSortAct"), dm_str(b, "dmKbSortCount"), dm_str(b, "dmKbSortFast") }; uint64_t dd = kb_find(bw, "TouchKB_Sort", 0); int to = (kb_sort(b, dd) + 1) % 3;
              if (dd) { kb_text(b, dd + 0x718, nm[to]); atomic_store(&g_kb_dirty, 1); LOG("刷怪积分牌：手柄排序 → %s", nm[to]); } } } }
    static dm_wref last_bw; static int frame; if (bw != dm_wref_get(&last_bw)) { dm_wref_set(&last_bw, bw); atomic_store(&g_kb_dirty, 1); }
    int dsel[3], nsel = 0; for (int d = 0; d < 3; d++) { char nm[16]; snprintf(nm, sizeof nm, "TouchKB_D%d", d); uint64_t c = kb_find(bw, nm, 0); dsel[d] = dm_widget_is(b, c, DM_VT_TOGGLE) && *(uint8_t *)(c + 0xa88); nsel += dsel[d]; }
    static int dlast = -1; int dm = dsel[0] | dsel[1] << 1 | dsel[2] << 2; if (dm != dlast) { dlast = dm; atomic_store(&g_kb_dirty, 1); }
    if (!atomic_exchange(&g_kb_dirty, 0) && ++frame % 30) return;
    int act = atomic_load(&g_kb_act); if (act < 0 || act > 5) act = 0;
    for (int i = 0; i < 6; i++) { char nm[20]; snprintf(nm, sizeof nm, "TouchKB_Act%d", i); uint64_t a = kb_find(bw, nm, 0); if (a) kb_toggle(a, i == act, 1); }
    int sort = kb_sort(b, kb_find(bw, "TouchKB_Sort", 0));


    struct { int r, n, best, last, uq, st; int64_t first, sum; int nt; } v[NROW]; int nv = 0; long total = 0;
    for (int r = 0; r < NROW; r++) { int n = 0, best = 0; int64_t first = 0;
        int last = 0, uq = 0, st = 0, nt = 0; int64_t sum = 0, lastf = 0;
        for (int d = 0; d < 3; d++) { if (nsel && !dsel[d]) continue; n += S[r][d].n; if (S[r][d].best && (!best || S[r][d].best < best)) best = S[r][d].best; if (S[r][d].first && (!first || S[r][d].first < first)) first = S[r][d].first;
            uq += S[r][d].uq; st += S[r][d].st; sum += S[r][d].sum; nt += S[r][d].nt;
            if (S[r][d].last && S[r][d].first >= lastf) { lastf = S[r][d].first; last = S[r][d].last; } }
        total += n; if (k_rows[r].act == act + 1) { v[nv].r = r; v[nv].n = n; v[nv].best = best; v[nv].first = first; v[nv].last = last; v[nv].uq = uq; v[nv].st = st; v[nv].sum = sum; v[nv].nt = nt; nv++; } }
    if (sort) for (int i = 1; i < nv; i++) for (int j = i; j > 0; j--) {
        int dm = sort == 1 ? v[j].n > v[j - 1].n : (v[j].best && (!v[j - 1].best || v[j].best < v[j - 1].best)); if (!dm) break; typeof(v[0]) t = v[j]; v[j] = v[j - 1]; v[j - 1] = t; }
    uint64_t list = 0;
    for (int i = 0; i < 6; i++) { char nm[20]; snprintf(nm, sizeof nm, "TouchKB_Page%d", i); uint64_t pg = kb_find(bw, nm, 0); if (!pg) continue;
        kb_rect(pg, i == act ? 0 : 99999, 0, -1); if (i == act) list = kb_find(pg, "TouchKB_List", 3); }
    if (!list) return;
    if (pad_scroll) {
        int W = *(int32_t *)(b + 0x211da38), H = *(int32_t *)(b + 0x211da3c), hx[256], hy[256], nh = 0;
        for (int gy = 1; gy < 16 && W > 0 && H > 0; gy++) for (int gx = 1; gx < 16; gx++) { int x = W * gx / 16, y = H * gy / 16;
            if (*(uint8_t *)(list + 0x50) && (((int (*)(uint64_t, uint64_t))*(uint64_t *)(*(uint64_t *)list + 0x48))(list, (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32)) & 1)) { hx[nh] = x; hy[nh] = y; nh++; } }
        if (nh) { void dm_ui_ptr(int, float, float, int); float nx = (float)hx[nh / 2] / (float)W, ny = (float)hy[nh / 2] / (float)H; dm_ui_ptr(1, nx, ny, 0); dm_ui_ptr(7, nx, ny, pad_scroll * 360);                                                                                                                                                                                                                               LOG("刷怪积分牌：手柄滚动 %+d（点 %d,%d）", pad_scroll, hx[nh / 2], hy[nh / 2]); }
        else LOG("【红】刷怪积分牌：手柄滚动时没找到列表的落点");
        pad_scroll = 0; }
    { uint64_t ds = kb_find(bw, "TouchKB_DiffStack", 0), so = kb_find(bw, "TouchKB_Sort", 0);
      if (ds) kb_show(ds, act != 5); if (so) kb_show(so, act != 5); }
    if (act == 5) {
        dm_rift_stats_t r; int have = dm_rift_stats(&r); char t[160];
        int nms = 0; for (int i = 0; i < 10; i++) nms += have && (r.ms >> i & 1);
        uint64_t w = kb_find(list, "TouchKB_RiftSum", 0);
        if (w) { kb_rect(w, 0, 0, -1); kb_text(b, kb_find(w, "Name", 0), dm_str(b, "dmKbRiftName"));
            if (have) snprintf(t, sizeof t, dm_str(b, "dmKbRiftBest"), r.best); else snprintf(t, sizeof t, "%s", dm_str(b, "dmKbRiftNever")); kb_text(b, kb_find(w, "Kills", 0), t);
            if (have) snprintf(t, sizeof t, dm_str(b, "dmKbRiftClears"), r.clears, r.deaths); else t[0] = 0; kb_text(b, kb_find(w, "First", 0), t);
            if (have) snprintf(t, sizeof t, dm_str(b, "dmKbRiftUnlock"), r.unlock); else t[0] = 0; kb_text(b, kb_find(w, "Drops", 0), t); kb_text(b, kb_find(w, "Times", 0), ""); }
        w = kb_find(list, "TouchKB_RiftSum2", 0);
        if (w) { kb_rect(w, have ? 0 : 99999, KB_ROW_H, -1); kb_text(b, kb_find(w, "Name", 0), dm_str(b, "dmKbRiftMs"));
            snprintf(t, sizeof t, "%d / 10", nms); kb_text(b, kb_find(w, "Kills", 0), t);
            { int nx = 0; while (nx < 10 && (r.ms >> nx & 1)) nx++; if (nx < 10) snprintf(t, sizeof t, dm_str(b, "dmKbRiftMsNext"), (nx + 1) * 10); else snprintf(t, sizeof t, "%s", dm_str(b, "dmKbRiftMsAll")); } kb_text(b, kb_find(w, "First", 0), t);
            kb_text(b, kb_find(w, "Drops", 0), ""); kb_text(b, kb_find(w, "Times", 0), ""); }
        int row = 2;
        for (int tier = 100; tier >= 1; tier--) { char nm[24]; snprintf(nm, sizeof nm, "TouchKB_RiftT%d", tier); uint64_t rw = kb_find(list, nm, 0); if (!rw) continue;
            int f = have ? r.fast[tier] : 0; if (!f) { kb_rect(rw, 99999, 0, -1); continue; }
            kb_rect(rw, 0, KB_ROW_H * row++, -1); snprintf(t, sizeof t, dm_str(b, "dmRiftTierN"), tier); kb_text(b, kb_find(rw, "Name", 0), t);
            snprintf(t, sizeof t, dm_str(b, "dmKbRiftFast"), f / 60, f % 60); kb_text(b, kb_find(rw, "Kills", 0), t);
            int two = tier % 10 == 0 && (r.ms >> (tier / 10 - 1) & 1); kb_text(b, kb_find(rw, "First", 0), two ? dm_str(b, "dmKbRiftMsGot") : "");
            kb_rect(kb_find(rw, "Name", 0), 180, two ? 8 : 64, -1); kb_rect(kb_find(rw, "Kills", 0), 760, two ? 8 : 64, -1); }
        if (have) snprintf(t, sizeof t, dm_str(b, "dmKbRiftTotal"), r.best); else t[0] = 0; kb_text(b, kb_find(bw, "TouchKB_Total", 0), t); return; }
    for (int i = 0; i < nv; i++) { int r = v[i].r; char nm[40]; snprintf(nm, sizeof nm, "TouchKB_%s", k_rows[r].key); uint64_t w = kb_find(list, nm, 0); if (!w) continue;
        kb_rect(w, 0, KB_ROW_H * i, -1);
        char t[160]; row_name(r, t, sizeof t); kb_text(b, kb_find(w, "Name", 0), t);
        snprintf(t, sizeof t, dm_str(b, "dmKbKills"), v[i].n); kb_text(b, kb_find(w, "Kills", 0), t);
        if (v[i].first) { time_t tt = (time_t)v[i].first; struct tm lt; localtime_r(&tt, &lt); strftime(t, sizeof t, dm_str(b, "dmKbFirst"), &lt); } else t[0] = 0; kb_text(b, kb_find(w, "First", 0), t);
        if (v[i].uq || v[i].st) snprintf(t, sizeof t, dm_str(b, "dmKbDrops"), v[i].uq, v[i].st); else t[0] = 0; kb_text(b, kb_find(w, "Drops", 0), t);
        if (v[i].best) { char a[16], c[16], e[16]; mmss(a, sizeof a, v[i].best); mmss(c, sizeof c, v[i].last); mmss(e, sizeof e, v[i].nt ? (int)(v[i].sum / v[i].nt) : 0);
            snprintf(t, sizeof t, dm_str(b, "dmKbTimes"), a, c, e); } else t[0] = 0; kb_text(b, kb_find(w, "Times", 0), t); }
    char num[32], t[64]; fmt_thousands(num, sizeof num, total); snprintf(t, sizeof t, dm_str(b, "dmKbTotal"), num); kb_text(b, kb_find(bw, "TouchKB_Total", 0), t);
}
