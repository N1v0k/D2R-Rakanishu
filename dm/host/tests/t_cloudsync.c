

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
int dm_cloud_sync(const char *save, const char *cloud, const char *bak, const char *statef, int allow_down, uint64_t now_ms, int (*ready)(const char *unit));
int dm_cloud_conflict_take(void);
int dm_cloud_char_conflict_take(char *out, size_t n);
int dm_cloud_inbox_add(const char *save, const char *statef, const char *pack);
#if __has_include("../stash_seed.h")
#include "../stash_seed.h"
#define T_HAVE_STASH_SEED 1
#endif

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ "); printf(__VA_ARGS__); printf("\n"); } } while (0)
static char R[512];
static void path(char *o, const char *a, const char *b) { snprintf(o, 1024, "%s/%s/%s", R, a, b); }
static void put(const char *dev, const char *rel, const char *txt, long sec) {
    char p[1024]; path(p, dev, rel); char d[1024]; strcpy(d, p); for (char *s = d + strlen(R) + 1; (s = strchr(s, '/')); s++) { *s = 0; mkdir(d, 0755); *s = '/'; }
    FILE *f = fopen(p, "w"); fputs(txt, f); fclose(f); struct timeval tv[2] = { { sec, 0 }, { sec, 0 } }; utimes(p, tv);
}
static const char *get(const char *dev, const char *rel) { static char b[256]; char p[1024]; path(p, dev, rel); FILE *f = fopen(p, "r"); if (!f) return NULL; size_t n = fread(b, 1, 255, f); b[n] = 0; fclose(f); return b; }
static long mtime(const char *dev, const char *rel) { char p[1024]; struct stat st; path(p, dev, rel); return stat(p, &st) ? -1 : st.st_mtimespec.tv_sec; }
static int dsync(const char *dev, int down, uint64_t now) { char s[1024], c[1024], b[1024], t[1024]; path(s, dev, "save"); snprintf(c, sizeof c, "%s/cloud", R); path(b, dev, "bak"); path(t, dev, "state"); return dm_cloud_sync(s, c, b, t, down, now, NULL); }
static int count_bak(const char *dev, const char *pre) { char d[1024]; path(d, dev, "bak"); DIR *dp = opendir(d); struct dirent *e; int n = 0; while (dp && (e = readdir(dp))) n += !strncmp(e->d_name, pre, strlen(pre)); if (dp) closedir(dp); return n; }
static int no_skip(const char *u) { return strcmp(u, "Alice"); }


static void put_bin(const char *dev, const char *rel, const uint8_t *d, size_t n, long sec) {
    char p[1024]; path(p, dev, rel); char dd[1024]; strcpy(dd, p); for (char *x = dd + strlen(R) + 1; (x = strchr(x, '/')); x++) { *x = 0; mkdir(dd, 0755); *x = '/'; }
    FILE *f = fopen(p, "wb"); fwrite(d, 1, n, f); fclose(f); struct timeval tv[2] = { { sec, 0 }, { sec, 0 } }; utimes(p, tv);
}
static void put_d2i(const char *dev, int items, int chron, long sec) {
    static uint8_t b[8192]; memset(b, 0, sizeof b); uint32_t l1 = 68, l2 = (uint32_t)chron;
    memcpy(b, "\x55\xaa\x55\xaa", 4); memcpy(b + 16, &l1, 4); memcpy(b + 64, "JM", 2); b[66] = (uint8_t)items;
    memcpy(b + 68, "\x55\xaa\x55\xaa", 4); memcpy(b + 68 + 16, &l2, 4); b[68 + 64] = 0xc0;
    put_bin(dev, "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i", b, 68 + l2, sec);
}
static void put_vault(const char *dev, int n, long sec) { uint8_t b[8] = { 'S', 'W', 'V', 'T', 1, 0, (uint8_t)n, 0 }; put_bin(dev, "save/ModernSharedStashSoftCoreV2.swvault", b, 8, sec); }
static int inbox_n(const char *dev) { char d[1024]; path(d, dev, "save/_cloudinbox"); DIR *dp = opendir(d); struct dirent *e; int n = 0; while (dp && (e = readdir(dp))) n += e->d_name[0] != '.'; if (dp) closedir(dp); return n; }
static int inbox_has(const char *dev, const char *f) {
    char d[1024]; path(d, dev, "save/_cloudinbox"); DIR *dp = opendir(d); struct dirent *e; int k = 0; struct stat st;
    while (dp && (e = readdir(dp))) { if (e->d_name[0] == '.') continue; char p[1400]; snprintf(p, sizeof p, "%s/%s/%s", d, e->d_name, f); k |= !stat(p, &st); } if (dp) closedir(dp); return k; }
static int d2i_items(const char *dev, const char *rel) { char p[1024]; path(p, dev, rel); FILE *f = fopen(p, "rb"); if (!f) return -1; uint8_t b[70]; size_t n = fread(b, 1, 70, f); fclose(f); return n >= 67 ? b[66] : -1; }



static void put_chron(const char *dev, int items, int n, const uint32_t *id, const uint32_t *t, long sec) {
    static uint8_t b[8192]; memset(b, 0, sizeof b); uint32_t l1 = 68, l2 = 84 + 10 * (uint32_t)n + 64; uint16_t one = 1, a = (uint16_t)n;
    memcpy(b, "\x55\xaa\x55\xaa", 4); memcpy(b + 16, &l1, 4); memcpy(b + 64, "JM", 2); b[66] = (uint8_t)items;
    uint8_t *c = b + 68; memcpy(c, "\x55\xaa\x55\xaa", 4); memcpy(c + 16, &l2, 4); memcpy(c + 64, "\xc0\xed\xea\xc0", 4); memcpy(c + 68, &one, 2); memcpy(c + 70, &a, 2);
    for (int i = 0; i < n; i++) { memcpy(c + 84 + 10 * i, &id[i], 4); uint16_t u = 0x6a5d; memcpy(c + 88 + 10 * i, &u, 2); memcpy(c + 90 + 10 * i, &t[i], 4); }
    put_bin(dev, "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i", b, 68 + l2, sec);
}


static const char *chron_str(const char *dev) {
    static char o[512]; char p[1024]; path(p, dev, "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i"); FILE *f = fopen(p, "rb"); uint8_t b[8192]; size_t n = f ? fread(b, 1, sizeof b, f) : 0; if (f) fclose(f);
    if (n < 68 + 148 || memcmp(b + 68 + 64, "\xc0\xed\xea\xc0", 4)) return "坏";
    uint16_t a; uint32_t ln; memcpy(&a, b + 68 + 70, 2); memcpy(&ln, b + 68 + 16, 4); if (ln != 84u + 10u * a + 64u || 68 + ln != n) return "坏";
    uint32_t id[64], t[64]; for (int i = 0; i < a && i < 64; i++) { memcpy(&id[i], b + 68 + 84 + 10 * i, 4); memcpy(&t[i], b + 68 + 90 + 10 * i, 4); }
    for (int i = 0; i < a; i++) for (int j = i + 1; j < a; j++) if (id[j] < id[i]) { uint32_t x = id[i]; id[i] = id[j]; id[j] = x; x = t[i]; t[i] = t[j]; t[j] = x; }
    o[0] = 0; for (int i = 0; i < a; i++) snprintf(o + strlen(o), sizeof o - strlen(o), "%s%u@%u", i ? " " : "", id[i], t[i]); return o;
}

int main(void) {
    snprintf(R, sizeof R, "%s/t_cloudsync.%d", getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp", getpid()); mkdir(R, 0755);
    char c[1024]; snprintf(c, sizeof c, "%s/cloud", R); mkdir(c, 0755);


    put("A", "save/Alice/Alice.d2s", "a1", 1000); put("A", "save/Alice/Alice.key", "k", 900);
    put("A", "save/SharedStashSoftCoreV2.d2i", "s1", 1100); put("A", "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i", "m1", 1100); put("A", "save/Settings/Settings.json", "{}", 1100);
    put("B", "save/Settings/Settings.json", "{b}", 1200);
    CHECK(dsync("A", 0, 0) == 2, "① A 上传应为 2 个单位（Alice、_stash）");
    CHECK(dsync("B", 1, 0) == 2, "① B 下载应为 2 个单位");
    CHECK(get("B", "save/Alice/Alice.d2s") && !strcmp(get("B", "save/Alice/Alice.d2s"), "a1"), "① B 的 Alice 应是 a1");
    CHECK(mtime("B", "save/Alice/Alice.d2s") == 1000, "① 修改时间应还原成 1000，实为 %ld", mtime("B", "save/Alice/Alice.d2s"));
    CHECK(get("B", "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i") && !strcmp(get("B", "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i"), "m1"), "① B 的共享仓库应是 m1");
    CHECK(!strcmp(get("B", "save/Settings/Settings.json"), "{b}"), "① B 的 Settings 不该被动");
    CHECK(dsync("A", 1, 0) == 0 && dsync("B", 1, 0) == 0, "① 再同步一次两边都应无动作");


    put("B", "save/Alice/Alice.d2s", "b2", 2000);
    CHECK(dsync("B", 0, 0) == 1, "② B 应上传 1 个");
    CHECK(dsync("A", 0, 0) == 0 && !strcmp(get("A", "save/Alice/Alice.d2s"), "a1"), "② A 运行中不该改本地");
    CHECK(dsync("A", 1, 0) == 1 && !strcmp(get("A", "save/Alice/Alice.d2s"), "b2"), "② A 启动后应是 b2");
    CHECK(get("A", "bak/Alice-1000000/Alice/Alice.d2s") && !strcmp(get("A", "bak/Alice-1000000/Alice/Alice.d2s"), "a1"), "② A 被覆盖的 a1 应在备份 Alice-1000000");


    put("A", "save/Alice/Alice.d2s", "a3", 3000); put("B", "save/Alice/Alice.d2s", "b3", 3500);
    CHECK(dsync("A", 0, 0) == 1, "③ A 先上传");
    CHECK(dsync("B", 0, 0) == 1, "③ B 后写也上传");
    CHECK(get("B", "bak/Alice-3000000/Alice/Alice.d2s") && !strcmp(get("B", "bak/Alice-3000000/Alice/Alice.d2s"), "a3"), "③ 被覆盖的云端 a3 应在 B 的备份");
    CHECK(dsync("A", 1, 0) == 1 && !strcmp(get("A", "save/Alice/Alice.d2s"), "b3"), "③ A 启动后应是 b3");


    put("A", "save/Alice/Alice.d2s", "a4", 5000); CHECK(dsync("A", 0, 0) == 1, "④ A 上传");
    put("B", "save/Alice/Alice.d2s", "b4", 4000);
    CHECK(dsync("B", 1, 0) == 1 && !strcmp(get("B", "save/Alice/Alice.d2s"), "a4"), "④ B 启动后应是 a4（云端 5000 > 本机 4000）");
    CHECK(get("B", "bak/Alice-4000000/Alice/Alice.d2s") && !strcmp(get("B", "bak/Alice-4000000/Alice/Alice.d2s"), "b4"), "④ B 被覆盖的 b4 应在备份");


    put("A", "save/Alice/Alice.d2s", "a5", 6000); CHECK(dsync("A", 0, 0) == 1, "⑤ A 上传");
    { char s[1024], b[1024], t[1024]; path(s, "B", "save"); path(b, "B", "bak"); path(t, "B", "state"); CHECK(dm_cloud_sync(s, c, b, t, 1, 0, no_skip) == 0 && !strcmp(get("B", "save/Alice/Alice.d2s"), "a4"), "⑤ 没下载好时 B 应不动"); }
    CHECK(dsync("B", 1, 0) == 1 && !strcmp(get("B", "save/Alice/Alice.d2s"), "a5"), "⑤ 下载好后 B 应是 a5");


    { char p[1024]; path(p, "A", "save/Alice/Alice.d2s"); unlink(p); path(p, "A", "save/Alice/Alice.key"); unlink(p); path(p, "A", "save/Alice"); rmdir(p); }
    CHECK(dsync("A", 0, 7000000) == 1, "⑥ A 应写删除标记");
    CHECK(dsync("B", 1, 0) == 1 && !get("B", "save/Alice/Alice.d2s"), "⑥ B 启动后 Alice 应被删");
    CHECK(get("B", "bak/Alice-6000000/Alice/Alice.d2s") && !strcmp(get("B", "bak/Alice-6000000/Alice/Alice.d2s"), "a5"), "⑥ B 被删的 a5 应在备份");
    CHECK(count_bak("B", "Alice-") == 3, "⑥ B 的 Alice 备份应留 3 份，实为 %d", count_bak("B", "Alice-"));
    for (long t = 8; t <= 9; t++) { char v[8]; snprintf(v, sizeof v, "x%ld", t); put("B", "save/Bob/Bob.d2s", v, t * 1000); }
    CHECK(dsync("B", 1, 0) == 1, "⑥ B 的新角色 Bob 上传");


    put("A", "save/Alice/Alice.d2s", "a7", 9500);
    CHECK(dsync("A", 1, 0) == 2 && !strcmp(get("A", "save/Bob/Bob.d2s"), "x9"), "⑦ A：Alice 上传 + Bob 下载");
    CHECK(dsync("B", 1, 0) == 1 && get("B", "save/Alice/Alice.d2s") && !strcmp(get("B", "save/Alice/Alice.d2s"), "a7"), "⑦ B 启动后应有新建的 Alice a7");


    put("B", "save/SharedStashSoftCoreV2.d2i", "s8", 10000);
    CHECK(dsync("B", 0, 0) == 1 && dsync("A", 1, 0) == 1 && !strcmp(get("A", "save/SharedStashSoftCoreV2.d2i"), "s8"), "⑧ A 的共享仓库应是 s8");
    CHECK(!strcmp(get("A", "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i"), "m1"), "⑧ 共享仓库的另一个文件应保留 m1");


    { char p[1024]; path(p, "A", "save"); char cmd[1200]; snprintf(cmd, sizeof cmd, "rm -rf '%s'", p); system(cmd); mkdir(p, 0755); }
    CHECK(dsync("A", 1, 0) == 3, "⑨ A 启动应下载 3 个（Alice、Bob、_stash）");
    CHECK(get("A", "save/Alice/Alice.d2s") && !strcmp(get("A", "save/Alice/Alice.d2s"), "a7") && get("A", "save/Bob/Bob.d2s") && !strcmp(get("A", "save/SharedStashSoftCoreV2.d2i"), "s8"), "⑨ A 应拿回 Alice a7、Bob、共享仓库 s8");
    CHECK(dsync("B", 1, 0) == 0 && get("B", "save/Alice/Alice.d2s"), "⑨ B 不受影响（没有删除标记传过来）");


    { char s[1024], c2[1024], b[1024], t2[1024], bf[1024]; snprintf(c2, sizeof c2, "%s/cloud2", R); mkdir(c2, 0755);
      #define SY(dev, down, now) (path(s, dev, "save"), path(b, dev, "bak"), path(t2, dev, "state"), dm_cloud_sync(s, c2, b, t2, down, now, NULL))
      put("F", "save/Settings/Settings.json", "{}", 1);
      put("E", "save/Eve/Eve.d2s", "e1", 1000); CHECK(SY("E", 0, 0) == 1 && SY("F", 1, 0) == 1 && get("F", "save/Eve/Eve.d2s") && !strcmp(get("F", "save/Eve/Eve.d2s"), "e1"), "⑩ 准备：F 拿到 Eve e1");
      put("E", "save/Eve/Eve.d2s", "e2", 2000); CHECK(SY("E", 0, 0) == 1, "⑩ E 上传 e2");
      path(bf, "F", "bak"); FILE *f = fopen(bf, "w"); fclose(f);
      CHECK(SY("F", 1, 0) == 0 && get("F", "save/Eve/Eve.d2s") && !strcmp(get("F", "save/Eve/Eve.d2s"), "e1"), "⑩ 备份写不进时 F 不应覆盖，仍是 e1");
      unlink(bf);
      CHECK(SY("F", 1, 0) == 1 && get("F", "save/Eve/Eve.d2s") && !strcmp(get("F", "save/Eve/Eve.d2s"), "e2") && get("F", "bak/Eve-1000000/Eve/Eve.d2s") && !strcmp(get("F", "bak/Eve-1000000/Eve/Eve.d2s"), "e1"), "⑩ 解开后 F 换成 e2，e1 整份在备份里");
      { char p2[1024]; path(p2, "E", "save/Eve/Eve.d2s"); unlink(p2); path(p2, "E", "save/Eve"); rmdir(p2); }
      CHECK(SY("E", 0, 3000000) == 1, "⑩ E 运行中删了 Eve，写删除标记");
      char bk[1100]; snprintf(bk, sizeof bk, "%s.keep", bf); rename(bf, bk); f = fopen(bf, "w"); fclose(f);
      CHECK(SY("F", 1, 0) == 0 && get("F", "save/Eve/Eve.d2s"), "⑩ 备份写不进时 F 不应删 Eve");
      unlink(bf); rename(bk, bf);
      CHECK(SY("F", 1, 0) == 1 && !get("F", "save/Eve/Eve.d2s") && get("F", "bak/Eve-2000000/Eve/Eve.d2s"), "⑩ 解开后 F 删 Eve，e2 整份在备份里");
      #undef SY
    }


    { char s3[1024], c3[1024], b3[1024], t3[1024]; snprintf(c3, sizeof c3, "%s/cloud3", R); mkdir(c3, 0755);
      #define S3(dev, down) (path(s3, dev, "save"), path(b3, dev, "bak"), path(t3, dev, "state"), dm_cloud_sync(s3, c3, b3, t3, down, 0, NULL))
      #define D2I "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i"
      put_d2i("G", 5, 300, 1000); put_vault("G", 3, 1000);
      CHECK(S3("G", 0) == 1, "⑫ 云端为空：G 首次同步上传共享仓库");
      dm_cloud_conflict_take();
      put_d2i("H", 0, 148, 5000);
      CHECK(S3("H", 0) == 0 && d2i_items("H", D2I) == 0, "⑪ 新设备运行中首次同步：不传不改（不能拿空仓库盖云端）");
      CHECK(S3("H", 1) == 1 && d2i_items("H", D2I) == 5 && get("H", "save/ModernSharedStashSoftCoreV2.swvault"), "⑪ 新设备启动首次同步：以云端为准，拿到 5 件与编年史仓库");
      CHECK(count_bak("H", "_stash-") == 1, "⑪ 新设备本来那份空仓库进备份");
      CHECK(!dm_cloud_conflict_take() && inbox_n("H") == 0, "⑪ 本机那份是新建空档：不进收件箱、不提示");
      put_d2i("G", 6, 400, 6000); CHECK(S3("G", 0) == 1, "⑭ G 编年史变长（400）照常上传");
      CHECK(S3("H", 1) == 1 && d2i_items("H", D2I) == 6, "⑭ H 启动拿到 G 的新版");
      put_d2i("H", 0, 148, 7000);
      CHECK(S3("H", 0) == 0, "⑬ H 运行中要上传变空的那份：分叉，运行中不动");
      CHECK(!dm_cloud_conflict_take(), "⑬ 运行中不提示（不再永久拦）");
      CHECK(S3("G", 1) == 0 && d2i_items("G", D2I) == 6, "⑬ 云端没被盖：G 仍是 6 件");
      CHECK(S3("H", 1) == 1 && d2i_items("H", D2I) == 6, "⑬ H 启动：云端为现行版，拿回 6 件");
      CHECK(inbox_n("H") == 1 && dm_cloud_conflict_take() == 1, "⑬ 本机那份共享仓库空了、但编年史仓库侧文件还有 3 件：进收件箱、提示一次");
      put_d2i("H", 7, 500, 7500); CHECK(S3("H", 0) == 1, "⑮ H 正常玩：编年史 500 上传");
      put_d2i("G", 6, 450, 7600);
      CHECK(S3("G", 0) == 0, "⑮ G 运行中要拿编年史 450 盖云端的 500：分叉，运行中不动");
      CHECK(S3("G", 1) == 1 && d2i_items("G", D2I) == 7, "⑮ G 启动：云端（7 件、500）为现行版");
      CHECK(inbox_n("G") == 1 && inbox_has("G", "mine.pack") && inbox_has("G", "base.pack"), "⑮ G 本机那份（6 件、450）进收件箱，带上次同步的基准（6 件、400）");
      CHECK(dm_cloud_conflict_take() == 1, "⑮ 收进收件箱：进局提示一次");
      CHECK(S3("G", 1) == 0 && inbox_n("G") == 1, "⑮ 再启动：一致、无动作、不再收（不是永久状态）");
      { char p3[1024]; path(p3, "H", "save/ModernSharedStashSoftCoreV2.swvault"); unlink(p3); } put_d2i("H", 7, 500, 8000);
      CHECK(S3("H", 0) == 0 && !dm_cloud_conflict_take(), "⑬ 编年史仓库侧文件没了（云端有 3 件）：分叉，运行中不动");
      put_vault("H", 0, 8100); put_d2i("H", 0, 500, 8100);
      CHECK(S3("H", 0) == 1 && !dm_cloud_conflict_take(), "⑯ 正常玩法：共享仓库物品取空、编年史仓库取空（文件还在，0 件），编年史没变短——照常上传");


      { char cd[1024]; snprintf(cd, sizeof cd, "%s/cloud4", R); mkdir(cd, 0755);
        #define S4(dev, down) (path(s3, dev, "save"), path(b3, dev, "bak"), path(t3, dev, "state"), dm_cloud_sync(s3, cd, b3, t3, down, 0, NULL))
        put_d2i("J", 2, 148, 1000); CHECK(S4("J", 1) == 1, "⑰ 准备：J 上传 2 件、编年史 148 的仓库");
        put_d2i("J", 0, 148, 2000); dm_cloud_conflict_take();
        CHECK(S4("J", 0) == 0 && !dm_cloud_conflict_take(), "⑰ 运行中要拿新建空档（0 件、148）盖 2 件的那份：分叉，运行中不动");
        CHECK(S4("J", 1) == 1 && d2i_items("J", D2I) == 2 && inbox_n("J") == 0, "⑰ 启动：拿回云端 2 件；本机空档不进收件箱");
        put_d2i("J", 0, 300, 3000); CHECK(S4("J", 0) == 1, "⑰ 编年史长过新建大小的仓库取空：照常上传");


#ifdef T_HAVE_STASH_SEED
        { char cd5[1024]; snprintf(cd5, sizeof cd5, "%s/cloud5", R); mkdir(cd5, 0755);
          #define S5(dev, down) (path(s3, dev, "save"), path(b3, dev, "bak"), path(t3, dev, "state"), dm_cloud_sync(s3, cd5, b3, t3, down, 0, NULL))
          #define HC "save/ModernSharedStashHardCoreV2/ModernSharedStashHardCoreV2.d2i"
          put_bin("K", HC, g_stash_seed, sizeof g_stash_seed, 1000); put_d2i("K", 3, 400, 1000); CHECK(S5("K", 1) == 1, "⑱ 准备：K 上传（专家仓库是预置）");
          { char p5[1024]; path(p5, "K", HC); unlink(p5); } put_d2i("K", 3, 400, 2000); dm_cloud_conflict_take();
          S5("K", 0); CHECK(!dm_cloud_conflict_take(), "⑱ 本机还没有专家仓库档、云端只是预置：不算丢东西，不弹");
          put_bin("K", HC, g_stash_seed, sizeof g_stash_seed, 3000); S5("K", 0); CHECK(!dm_cloud_conflict_take(), "⑱ 本机写了预置再上传：不拦");


          put_d2i("K", 5, 600, 4000); S5("K", 0); dm_cloud_conflict_take();
          { uint8_t big[700]; memset(big, 0, sizeof big); uint32_t l1 = 68, l2 = 300; memcpy(big, "\x55\xaa\x55\xaa", 4); memcpy(big + 16, &l1, 4); memcpy(big + 64, "JM", 2); big[66] = 9; memcpy(big + 68, "\x55\xaa\x55\xaa", 4); memcpy(big + 84, &l2, 4); big[132] = 0xc0;
            put_bin("K", HC, big, 68 + 300, 5000); CHECK(S5("K", 0) == 1, "⑲ 准备：专家仓库有 9 件、编年史 300 上传"); }
          put_bin("K", HC, g_stash_seed, sizeof g_stash_seed, 6000);
          CHECK(S5("K", 0) == 0 && !dm_cloud_conflict_take(), "⑲ 本机变回预置要盖 9 件的那份：分叉，运行中不动");
          #undef HC
          #undef S5
        }
#else
        printf("跳过 ⑱ ⑲：没有共享仓库预置 stash_seed.h（公开仓库不带）\n");
#endif
        #undef S4
      }
      #undef D2I
      #undef S3
    }




    { char c6[1024], s6[1024], b6[1024], t6[1024]; snprintf(c6, sizeof c6, "%s/cloud6", R); mkdir(c6, 0755); char nm[256];
      #define S6(dev, down) (path(s6, dev, "save"), path(b6, dev, "bak"), path(t6, dev, "state"), dm_cloud_sync(s6, c6, b6, t6, down, 0, NULL))
      put("P", "save/Foo/Foo.d2s", "fooP", 1000); CHECK(S6("P", 1) == 1, "⑳ 准备：P 上传 Foo");
      dm_cloud_char_conflict_take(nm, sizeof nm);
      put("Q", "save/Foo/Foo.d2s", "fooQ", 5000); put("Q", "save/Bar/Bar.d2s", "barQ", 5000);
      CHECK(S6("Q", 1) == 2, "⑳ Q 首次同步：Foo 下载、Bar 上传，共 2 个动作");
      CHECK(get("Q", "save/Foo/Foo.d2s") && !strcmp(get("Q", "save/Foo/Foo.d2s"), "fooP"), "⑳ Q 的 Foo 应换成云端那份 fooP");
      CHECK(count_bak("Q", "Foo-5000000") == 1, "⑳ Q 本机那份 Foo 应进备份 Foo-5000000");
      CHECK(dm_cloud_char_conflict_take(nm, sizeof nm) == 1 && !strcmp(nm, "Foo"), "⑳ 记下同名角色 Foo 待提示，实为「%s」", nm);
      CHECK(dm_cloud_char_conflict_take(nm, sizeof nm) == 0, "⑳ 取过一次后清空");
      CHECK(S6("Q", 1) == 0 && dm_cloud_char_conflict_take(nm, sizeof nm) == 0, "⑳ 再同步无动作、不再提示");
      #undef S6
    }


    { char c7[1024], s7[1024], b7[1024], t7[1024], cq[1024]; snprintf(c7, sizeof c7, "%s/cloud7", R); mkdir(c7, 0755); snprintf(cq, sizeof cq, "%s/cloud7q", R); mkdir(cq, 0755);
      #define S7(dev, cl, down) (path(s7, dev, "save"), path(b7, dev, "bak"), path(t7, dev, "state"), dm_cloud_sync(s7, cl, b7, t7, down, 0, NULL))
      #define D2I7 "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i"
      put_d2i("A7", 10, 600, 1000); CHECK(S7("A7", c7, 1) == 1, "㉑ 准备：A 上传 10 件");
      dm_cloud_conflict_take();


      put_d2i("N7", 3, 300, 5000);
      CHECK(S7("N7", c7, 1) == 1 && d2i_items("N7", D2I7) == 10, "㉑① N 首次同步：仓库是云端的 10 件");
      CHECK(inbox_n("N7") == 1 && inbox_has("N7", "mine.pack") && !inbox_has("N7", "base.pack") && dm_cloud_conflict_take() == 1, "㉑① N 本机 3 件进收件箱（没有基准）、提示一次");
      CHECK(S7("N7", c7, 1) == 0 && inbox_n("N7") == 1 && d2i_items("A7", D2I7) == 10, "㉑① 再启动无动作（没有永久状态）；A 的 10 件不动");


      put_d2i("M7", 3, 900, 5000);
      CHECK(S7("M7", c7, 0) == 0 && inbox_n("M7") == 0, "㉑② M 运行中：不动、不收（等启动）");
      CHECK(S7("M7", c7, 1) == 1 && d2i_items("M7", D2I7) == 10 && inbox_n("M7") == 1 && dm_cloud_conflict_take() == 1, "㉑② M 启动：仓库 10 件，本机 3 件进收件箱");
      CHECK(S7("M7", c7, 1) == 0 && S7("M7", c7, 0) == 0 && inbox_n("M7") == 1, "㉑② 再启动、再运行中都无动作：不是永久拦截");


      put_d2i("Q7", 3, 300, 5000); CHECK(S7("Q7", cq, 1) == 1, "㉑③ 准备：Q 往看着是空的云端传了 3 件");
      char sp[1300], dp[1300], keep[1300]; snprintf(sp, sizeof sp, "%s/_stash.pack", cq); snprintf(dp, sizeof dp, "%s/_stash.pack", c7); snprintf(keep, sizeof keep, "%s/a_old.pack", R);
      rename(dp, keep); { FILE *fi = fopen(sp, "rb"), *fo = fopen(dp, "wb"); char buf[8192]; size_t k; while ((k = fread(buf, 1, sizeof buf, fi)) > 0) fwrite(buf, 1, k, fo); fclose(fi); fclose(fo); }
      CHECK(S7("A7", c7, 1) == 1 && d2i_items("A7", D2I7) == 3, "㉑③ A 启动：云端（Q 的 3 件）为现行版");
      CHECK(inbox_n("A7") == 1 && inbox_has("A7", "base.pack") && dm_cloud_conflict_take() == 1, "㉑③ A 本机 10 件进收件箱，带上次同步的基准");
      CHECK(S7("A7", c7, 1) == 0 && inbox_n("A7") == 1, "㉑③ 再启动无动作");


      { char ts7[1100]; path(s7, "A7", "save"); path(ts7, "A7", "state");
        CHECK(dm_cloud_inbox_add(s7, ts7, keep) == 0 && inbox_n("A7") == 2 && dm_cloud_conflict_take() == 1, "㉑③ 冲突版本（10 件）收进收件箱");
        put_d2i("E7", 0, 148, 100); { char e7[1100], ee[1100], et[1100], pk[1300]; path(e7, "E7", "save"); path(ee, "E7", "bak"); path(et, "E7", "state"); char ce[1300]; snprintf(ce, sizeof ce, "%s/cloud7e", R); mkdir(ce, 0755);
          dm_cloud_sync(e7, ce, ee, et, 1, 0, NULL); snprintf(pk, sizeof pk, "%s/_stash.pack", ce);
          CHECK(dm_cloud_inbox_add(s7, ts7, pk) == 0 && inbox_n("A7") == 2, "㉑③ 空档的冲突版本：没东西，不收"); }
        CHECK(dm_cloud_inbox_add(s7, ts7, "/nonexistent.pack") == -1, "㉑③ 读不了的冲突版本：返回 -1"); }
      #undef S7
      #undef D2I7
    }


    { char c8[1024], s8[1024], b8[1024], t8[1024]; snprintf(c8, sizeof c8, "%s/cloud8", R); mkdir(c8, 0755);
      #define S8(dev, down) (path(s8, dev, "save"), path(b8, dev, "bak"), path(t8, dev, "state"), dm_cloud_sync(s8, c8, b8, t8, down, 0, NULL))
      uint32_t ia[] = { 1, 2 }, ta[] = { 100, 200 }, in_[] = { 2, 3 }, tn[] = { 150, 300 };
      put_chron("A8", 2, 2, ia, ta, 1000); CHECK(S8("A8", 1) == 1, "㉒ 准备：A 上传（1@100 2@200）");
      put_chron("N8", 0, 2, in_, tn, 5000);
      CHECK(S8("N8", 1) == 1 && !strcmp(chron_str("N8"), "1@100 2@150 3@300"), "㉒① N 首次同步：云端为现行版、编年史并进本机的 → 1@100 2@150 3@300，实为 %s", chron_str("N8"));
      CHECK(d2i_items("N8", "save/ModernSharedStashSoftCoreV2/ModernSharedStashSoftCoreV2.d2i") == 2 && inbox_n("N8") == 1, "㉒① 物品页是云端的 2 件，本机那份照常进收件箱");
      CHECK(S8("N8", 0) == 1, "㉒② N 运行中：并过的本地当改过、传上去");
      CHECK(S8("A8", 1) == 1 && !strcmp(chron_str("A8"), "1@100 2@150 3@300"), "㉒② A 启动拿到并集，实为 %s", chron_str("A8"));
      CHECK(S8("A8", 1) == 0 && S8("N8", 1) == 0 && S8("N8", 0) == 0, "㉒② 之后两边都无动作（不来回传）");


      uint32_t i4[] = { 1, 2, 3, 4 }, t4[] = { 100, 150, 300, 400 }, i5[] = { 1, 2, 3, 5 }, t5[] = { 100, 150, 300, 500 };
      put_chron("N8", 2, 4, i5, t5, 6000); CHECK(S8("N8", 0) == 1, "㉒③ 准备：N 加了 5、上传");
      put_chron("A8", 2, 4, i4, t4, 7000);
      CHECK(S8("A8", 1) == 1 && !strcmp(chron_str("A8"), "1@100 2@150 3@300 4@400 5@500"), "㉒③ A 后写赢、启动时上传：本机先并进云端的 5 → 实为 %s", chron_str("A8"));
      CHECK(S8("N8", 1) == 1 && !strcmp(chron_str("N8"), "1@100 2@150 3@300 4@400 5@500"), "㉒③ N 启动拿到五条，实为 %s", chron_str("N8"));
      #undef S8
    }
    printf("t_cloudsync: %d 项，%d 失败\n", checks, fails);
    return fails != 0;
}
