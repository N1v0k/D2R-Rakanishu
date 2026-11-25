















#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
static ssize_t fake_pwrite(int fd, const void *buf, size_t n, off_t off);
#define pwrite fake_pwrite
#include "../hle_fs.c"
#undef pwrite
ssize_t pwrite(int, const void *, size_t, off_t);

struct dm_flags g_flag; uint64_t g_t0_ns; uint64_t now_ns(void) { return 0; }
void dm_prof_add(int id, uint64_t ns, uint64_t b) { (void)id; (void)ns; (void)b; } void dm_guest_write_prepare(void *a, size_t n) { (void)a; (void)n; }
void dm_fs_save_poison(int e);
extern int g_nfail, g_lasterr;
static int g_mode, g_calls;
static ssize_t fake_pwrite(int fd, const void *buf, size_t n, off_t off) {
    g_calls++;
    if (g_mode == 1) { if (g_calls > 1000) { printf("  ✗ pwrite 返回 0 后被重试超过 1000 次（死循环）\n"); exit(1); } return 0; }
    if (g_mode == 2 && g_calls == 1 && n > 3) n = 3;
    return pwrite(fd, buf, n, off);
}
static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ "); } else printf("  ✓ "); printf(__VA_ARGS__); printf("\n"); } while (0)
static void put(const char *p, const char *s) { FILE *f = fopen(p, "w"); fputs(s, f); fclose(f); }
static int same(const char *p, const char *s) {
    char b[64] = {0}; FILE *f = fopen(p, "r"); if (!f) return 0; size_t k = fread(b, 1, sizeof b - 1, f); fclose(f);
    return k == strlen(s) && !memcmp(b, s, k);
}
static Result write_save(const char *path, const char *s) {
    uint64_t h; Result r = h_fs_OpenFile(&h, path, MODE_WRITE); if (r) return r;
    h_fs_SetFileSize(h, (int64_t)strlen(s));
    r = h_fs_WriteFile(h, 0, s, strlen(s), NULL); h_fs_CloseFile(h); return r;
}
static int read_fs(const char *path, char *out, size_t n) { uint64_t h; size_t got = 0; if (h_fs_OpenFile(&h, path, MODE_READ)) return 0; Result r = h_fs_ReadFile(&got, h, 0, out, n - 1); h_fs_CloseFile(h); out[got] = 0; return !r; }





static int test_holes(void) {
    enum { N = 100000 }; static uint8_t orig[N], got[N]; const long hole[3][2] = {{0, 1000}, {30000, 25000}, {99000, 1000}};
    for (long i = 0; i < N; i++) orig[i] = (uint8_t)(i * 131 + (i >> 8) * 7 + 1);
    for (int h = 0; h < 3; h++) memset(orig + hole[h][0], 0, (size_t)hole[h][1]);
    char path[] = "/tmp/dm-holes.XXXXXX"; int tfd = mkstemp(path); if (tfd < 0) return 1; char hp[64]; snprintf(hp, sizeof hp, "%s.holes", path);
    long pos = 0; for (int h = 0; h <= 3; h++) { long end = h < 3 ? hole[h][0] : N; if (end > pos && write(tfd, orig + pos, (size_t)(end - pos)) != end - pos) return 1; if (h < 3) pos = hole[h][0] + hole[h][1]; }
    close(tfd); FILE *f = fopen(hp, "w"); fprintf(f, "size %d\n", N); for (int h = 0; h < 3; h++) fprintf(f, "%ld %ld\n", hole[h][0], hole[h][1]); fclose(f);
    int fd = open(path, O_RDONLY); dm_data_note_fd(fd, path); int bad = 0;
    const long q[][2] = {{0, 500}, {500, 1000}, {999, 2}, {1000, 29000}, {29990, 20}, {30000, 25000}, {40000, 100}, {54990, 20}, {55000, 44000}, {98990, 1010}, {99500, 500}, {99990, 100}, {0, N}, {100000, 10}, {123456, 10}};
    for (size_t k = 0; k < sizeof q / sizeof *q; k++) {
        long off = q[k][0], n = q[k][1], want = off >= N ? 0 : (off + n > N ? N - off : n);
        memset(got, 0xAA, sizeof got); ssize_t r = dm_data_pread(fd, got, (size_t)n, off);
        if (r != want || (want && memcmp(got, orig + off, (size_t)want))) { fprintf(stderr, "失败：剔零读 off=%ld n=%ld 返回 %zd（期望 %ld）\n", off, n, r, want); bad++; }
    }
    if (dm_data_logical_size(path, 49000) != N) { fprintf(stderr, "失败：剔零档案的原始大小\n"); bad++; }
    close(fd); g_hm[fd] = NULL; unlink(path); unlink(hp);
    printf("%s 剔零档案偏移换算 15 种读法逐字节等于原始内容\n", bad ? "✗" : "✓");
    return bad;
}
int main(void) {
    if (test_holes()) return 1;
    char root[1024], save[1100], work[1200], p[1200];
    const char *sp = getenv("TMPDIR"); snprintf(root, sizeof root, "%s/t_fs.XXXXXX", sp ? sp : "/private/tmp");
    if (!mkdtemp(root)) { perror("mkdtemp"); return 1; }
    char rom[1100], overlay[1100], rdata[1200], odata[1200], fn[1300], got[64];
    snprintf(rom, sizeof rom, "%s/rom", root); snprintf(overlay, sizeof overlay, "%s/overlay", root); snprintf(rdata, sizeof rdata, "%s/data", rom); snprintf(odata, sizeof odata, "%s/data", overlay);
    mkdir(rom, 0755); mkdir(overlay, 0755); mkdir(rdata, 0755); mkdir(odata, 0755);
    snprintf(fn, sizeof fn, "%s/same", rdata); put(fn, "base"); snprintf(fn, sizeof fn, "%s/same", odata); put(fn, "overlay"); snprintf(fn, sizeof fn, "%s/base", rdata); put(fn, "only-base"); snprintf(fn, sizeof fn, "%s/over", odata); put(fn, "only-overlay");
    setenv("DM_ROMFS", rom, 1); setenv("DM_ROMFS_OVERLAY", overlay, 1); CHECK(h_fs_MountRom("rom", NULL, 0) == RES_OK, "挂载 RomFS 两层 overlay");
    CHECK(read_fs("rom:/data/same", got, sizeof got) && !strcmp(got, "overlay"), "overlay 文件优先"); CHECK(read_fs("rom:/data/base", got, sizeof got) && !strcmp(got, "only-base"), "overlay 缺失时回退基底");
    uint64_t rh; CHECK(h_fs_OpenFile(&rh, "rom:/data/same", MODE_WRITE) == RES_FAIL, "两层挂载均拒绝写入");
    {
        char l2[1100], two[2300], want[1300]; snprintf(l2, sizeof l2, "%s/pack", root); mkdir(l2, 0755); snprintf(two, sizeof two, "%s:%s", overlay, l2); setenv("DM_ROMFS_OVERLAY", two, 1);
        snprintf(fn, sizeof fn, "%s/mark", overlay); put(fn, "1"); snprintf(fn, sizeof fn, "%s/only2", l2); put(fn, "2"); snprintf(fn, sizeof fn, "%s/mark", l2); put(fn, "2");
        snprintf(want, sizeof want, "%s/mark", overlay); CHECK(dm_overlay_file("mark", p, sizeof p) == 1 && !strcmp(p, want), "两层都有：取第一层的");
        snprintf(want, sizeof want, "%s/only2", l2); CHECK(dm_overlay_file("only2", p, sizeof p) == 1 && !strcmp(p, want), "只有第二层有：取第二层的");
        snprintf(want, sizeof want, "%s/data/over", overlay); CHECK(dm_overlay_file("data/over", p, sizeof p) == 1 && !strcmp(p, want), "带子目录的相对路径");
        CHECK(dm_overlay_file("none", p, sizeof p) == 0 && !p[0], "哪层都没有：返回 0、输出清空");
        setenv("DM_ROMFS_OVERLAY", overlay, 1); snprintf(want, sizeof want, "%s/mark", overlay); CHECK(dm_overlay_file("mark", p, sizeof p) == 1 && !strcmp(p, want), "单层（新机型）照旧");
        unsetenv("DM_ROMFS_OVERLAY"); CHECK(dm_overlay_file("mark", p, sizeof p) == 0, "没设覆盖层：返回 0"); setenv("DM_ROMFS_OVERLAY", overlay, 1); }
    g_nmnt = 0;
    snprintf(save, sizeof save, "%s/save", root); snprintf(work, sizeof work, "%s.work", save);
    mkdir(save, 0755); snprintf(p, sizeof p, "%s/a", save); put(p, "old");
    setenv("DM_SAVE", save, 1); setenv("DM_TEST_FS_FAIL", "write:5:28", 1);
    CHECK(h_fs_MountSave("save", NULL) == RES_OK, "挂载存档");



    printf("① pwrite 返回 0\n");
    g_mode = 1; g_calls = 0;
    CHECK(write_save("save:/a", "xyz") == RES_FAIL, "WriteFile 返回失败 0x2A2");
    CHECK(g_calls == 1, "只调了 1 次 pwrite（实际 %d 次；原先无限重试）", g_calls);

    printf("② pwrite 分段写\n");
    g_mode = 2; g_calls = 0;
    CHECK(write_save("save:/a", "abcdefg") == RES_OK, "WriteFile 成功");
    CHECK(g_calls == 2, "pwrite 调 2 次（3 字节 + 4 字节；实际 %d）", g_calls);
    snprintf(p, sizeof p, "%s/a", work); CHECK(same(p, "abcdefg"), "工作目录里是完整的 abcdefg");
    g_mode = 0;

    printf("③ 事务\n");
    CHECK(h_fs_Commit("save") == RES_FAIL && g_nfail == 1 && g_lasterr == ENOSPC, "①②所在事务 Commit 被拒，前端通知 1 次 errno=ENOSPC（实际 %d 次 errno=%d）", g_nfail, g_lasterr);
    snprintf(p, sizeof p, "%s/a", work); CHECK(same(p, "old"), "工作目录丢弃、回到已提交版 old");
    CHECK(write_save("save:/a", "new") == RES_OK, "下一个事务：写 new");
    snprintf(p, sizeof p, "%s/a", save); CHECK(same(p, "old"), "Commit 前已提交目录仍是 old");
    CHECK(h_fs_Commit("save") == RES_OK, "Commit");
    CHECK(same(p, "new"), "Commit 后已提交目录是 new");
    snprintf(p, sizeof p, "%s.new", save); CHECK(access(p, F_OK) != 0, "换下来的旧版已删（无 .new 残留）");

    printf("④ DM_TEST_FS_FAIL=write:5:28\n");
    CHECK(write_save("save:/b", "4") == RES_PATH_NOT_FOUND, "（b 不存在，打开即失败，不计写）");
    h_fs_CreateFile("save:/b", 0);
    CHECK(write_save("save:/b", "4") == RES_OK, "第 4 次写成功");
    errno = 0; CHECK(write_save("save:/b", "5") == RES_FAIL && errno == 28, "第 5 次写失败 errno=28（实际 errno=%d）", errno);
    CHECK(write_save("save:/b", "6") == RES_OK, "第 6 次写成功");
    CHECK(h_fs_Commit("save") == RES_FAIL && g_nfail == 2 && g_lasterr == 28, "该事务 Commit 被拒，前端通知 errno=28（累计 %d 次）", g_nfail);
    snprintf(p, sizeof p, "%s/b", save); CHECK(access(p, F_OK) != 0, "已提交目录不变（没有 b）");
    snprintf(p, sizeof p, "%s/b", work); CHECK(access(p, F_OK) != 0, "工作目录丢弃（没有 b）");
    snprintf(p, sizeof p, "%s/a", save); CHECK(same(p, "new"), "已提交目录 a 仍是 new");
    h_fs_CreateFile("save:/b", 0);
    CHECK(write_save("save:/b", "7") == RES_OK && h_fs_Commit("save") == RES_OK, "下一个事务：写 b=7，Commit 成功");
    snprintf(p, sizeof p, "%s/b", save); CHECK(same(p, "7"), "最终已提交 b = 7（下一个事务的内容）");
    CHECK(g_nfail == 2, "成功的事务不发通知（累计仍 %d 次）", g_nfail);

    printf("⑤ 重新挂载丢弃未提交内容\n");
    CHECK(write_save("save:/a", "v9") == RES_OK, "写 a=v9 不提交");
    g_nmnt = 0;
    CHECK(h_fs_MountSave("save", NULL) == RES_OK, "重新挂载");
    snprintf(p, sizeof p, "%s/a", work); CHECK(same(p, "new"), "工作目录 a 回到已提交版 new");
    snprintf(p, sizeof p, "%s/b", work); CHECK(same(p, "7"), "工作目录 b = 已提交的 7");

    printf("⑥ 写失败收场不退出（DM_TEST_FS_FAILAT）\n");
    unsetenv("DM_TEST_FS_FAIL");


    setenv("DM_TEST_FS_FAILAT", "reclone:1", 1);
    CHECK(write_save("save:/a", "bad") == RES_OK, "写 a=bad");
    dm_fs_save_poison(ENOSPC); int nf0 = g_nfail;
    CHECK(h_fs_Commit("save") == RES_FAIL && g_nfail == nf0 + 1, "本事务被拒（重新克隆注入失败），进程还在，前端通知 1 次");
    snprintf(p, sizeof p, "%s/a", save); CHECK(same(p, "new"), "已提交目录 a 仍是 new");
    snprintf(p, sizeof p, "%s/a", work); CHECK(same(p, "bad"), "工作目录没删（a 还是 bad）");
    CHECK(h_fs_Commit("save") == RES_FAIL && g_nfail == nf0 + 2, "仍是已污染：下一次提交照样拒绝");
    snprintf(p, sizeof p, "%s/a", work); CHECK(same(p, "new"), "这次重新克隆成了：工作目录 a 回到 new");
    snprintf(p, sizeof p, "%s.re", work); CHECK(access(p, F_OK) != 0, "没有 .re 残留");
    CHECK(write_save("save:/a", "good") == RES_OK && h_fs_Commit("save") == RES_OK, "下一个事务写 a=good，提交成功");
    snprintf(p, sizeof p, "%s/a", save); CHECK(same(p, "good"), "已提交 a = good");


    setenv("DM_TEST_FS_FAILAT", "guestname:1,renameback:1", 1);
    h_fs_CreateFile("save:/c", 0); CHECK(write_save("save:/c", "c") == RES_OK, "写 c");
    CHECK(h_fs_RenameFile("save:/c", "save:/d") != RES_OK, "改名 c → d 返回失败（两步都注入失败），进程还在");
    nf0 = g_nfail; CHECK(h_fs_Commit("save") == RES_FAIL && g_nfail == nf0 + 1, "本事务提交被拒");
    snprintf(p, sizeof p, "%s/c", save); CHECK(access(p, F_OK) != 0, "已提交目录没有 c");
    snprintf(p, sizeof p, "%s/d", save); CHECK(access(p, F_OK) != 0, "已提交目录没有 d");
    snprintf(p, sizeof p, "%s/a", save); CHECK(same(p, "good"), "已提交 a 仍是 good");
    CHECK(write_save("save:/a", "ok2") == RES_OK && h_fs_Commit("save") == RES_OK, "下一个事务照常提交");


    setenv("DM_TEST_FS_FAILAT", "mountclone:1", 1); g_nmnt = 0; nf0 = g_nfail;
    snprintf(p, sizeof p, "rm -rf '%s'", work); system(p);
    CHECK(h_fs_MountSave("save", NULL) == RES_OK && g_nfail == nf0 + 1, "挂载返回成功、前端通知 1 次（实际 %d 次）", g_nfail - nf0);
    CHECK(read_fs("save:/a", got, sizeof got) && !strcmp(got, "ok2"), "只读挂载下读得到已提交的 a=ok2（读到 %s）", got);
    CHECK(write_save("save:/a", "zzz") != RES_OK, "写不进去");
    CHECK(h_fs_Commit("save") == RES_OK, "提交是空操作");
    snprintf(p, sizeof p, "%s/a", save); CHECK(same(p, "ok2"), "已提交 a 仍是 ok2");
    unsetenv("DM_TEST_FS_FAILAT"); g_nmnt = 0;
    CHECK(h_fs_MountSave("save", NULL) == RES_OK && write_save("save:/a", "ok3") == RES_OK && h_fs_Commit("save") == RES_OK, "去掉注入重新挂载：事务式照常，写 a=ok3 提交成功");
    snprintf(p, sizeof p, "%s/a", save); CHECK(same(p, "ok3"), "已提交 a = ok3");


    { extern int g_t_lang, g_t_voice; char sd[1200], sf[1300], gotj[512]; snprintf(sd, sizeof sd, "%s/langt", root); mkdir(sd, 0755); snprintf(sf, sizeof sf, "%s/Settings", sd); mkdir(sf, 0755); snprintf(sf, sizeof sf, "%s/Settings/Settings.json", sd);
      #define LANGCASE(dm, in, want, what) do { g_t_lang = (dm); g_t_voice = -1; put(sf, (in)); settings_lang_apply(sd); FILE *lf = fopen(sf, "rb"); size_t ln_ = lf ? fread(gotj, 1, sizeof gotj - 1, lf) : 0; if (lf) fclose(lf); gotj[ln_] = 0; CHECK(!strcmp(gotj, (want)), "语言开关：%s", (what)); } while (0)
      #define VOICECASE(dm, voice, in, want, what) do { g_t_lang = (dm); g_t_voice = (voice); put(sf, (in)); settings_lang_apply(sd); FILE *lf = fopen(sf, "rb"); size_t ln_ = lf ? fread(gotj, 1, sizeof gotj - 1, lf) : 0; if (lf) fclose(lf); gotj[ln_] = 0; CHECK(!strcmp(gotj, (want)), "单独配音：%s", (what)); } while (0)
      LANGCASE(-1, "{ \"Language\": 8, \"VoiceLanguage\": 8 }", "{ \"Language\": 8, \"VoiceLanguage\": 8 }", "没点过应用：一个字不动");
      LANGCASE(1, "{ \"A\": 1, \"Language\": 8, \"VoiceLanguage\": 8, \"B\": 2 }", "{ \"A\": 1, \"Language\": 0, \"VoiceLanguage\": 0, \"B\": 2 }", "开关 = 英文、文件是简中：文字和语音都改成 0，别的键不动");
      LANGCASE(0, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 8, \"VoiceLanguage\": 8 }", "开关 = 中文、文件是英文：都改成 8");
      LANGCASE(0, "{ \"Language\": 9, \"VoiceLanguage\": 9 }", "{ \"Language\": 9, \"VoiceLanguage\": 9 }", "开关 = 中文、玩家在原版里选了繁中：不改回简中");
      LANGCASE(1, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "开关 = 英文、文件已是英文：不动");
      LANGCASE(1, "{ \"Language\": 0, \"VoiceLanguage\": 8 }", "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "开关 = 英文、文字已英文但语音还是中文：语音跟上");
      LANGCASE(1, "{ \"Language\": 10 }", "{\n    \"VoiceLanguage\": 0, \"Language\": 0 }", "开关 = 英文、两位数的语言号且没有语音键：改写并补上语音键");


      LANGCASE(2, "{ \"Language\": 8, \"VoiceLanguage\": 8 }", "{ \"Language\": 2, \"VoiceLanguage\": 0 }", "开关 = 西语、文件是简中：文字 esES、语音英文");
      LANGCASE(2, "{ \"Language\": 10, \"VoiceLanguage\": 0 }", "{ \"Language\": 10, \"VoiceLanguage\": 0 }", "开关 = 西语、文件已是拉美西语：不动");
      LANGCASE(2, "{ \"Language\": 2, \"VoiceLanguage\": 2 }", "{ \"Language\": 2, \"VoiceLanguage\": 0 }", "开关 = 西语、语音是不在包里的西语配音：语音改英文");
      LANGCASE(1, "{ \"Language\": 2, \"VoiceLanguage\": 0 }", "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "开关 = 英文、文件是西语：文字改英文");
      LANGCASE(0, "{ \"Language\": 10, \"VoiceLanguage\": 0 }", "{ \"Language\": 8, \"VoiceLanguage\": 8 }", "开关 = 中文、文件是拉美西语：都改成 8");


      { char rr[1200], rd[1300]; snprintf(rr, sizeof rr, "%s/romfs_ru", root); snprintf(rd, sizeof rd, "%s/data", rr); mkdir(rr, 0755); mkdir(rd, 0755); setenv("DM_ROMFS", rr, 1);
        LANGCASE(3, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 7, \"VoiceLanguage\": 0 }", "开关 = 俄语、资源里没有俄语配音分块：文字改 ruRU，语音配英文");
        snprintf(rd, sizeof rd, "%s/data/16-00000001.data", rr); put(rd, "x"); }
      LANGCASE(3, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 7, \"VoiceLanguage\": 7 }", "开关 = 俄语、文件是英文：文字与语音都改成 ruRU");
      LANGCASE(3, "{ \"Language\": 7, \"VoiceLanguage\": 7 }", "{ \"Language\": 7, \"VoiceLanguage\": 7 }", "开关 = 俄语、文件已是俄文配俄语语音：不动");
      LANGCASE(3, "{ \"Language\": 7, \"VoiceLanguage\": 0 }", "{ \"Language\": 7, \"VoiceLanguage\": 7 }", "开关 = 俄语、文件是俄文配英文语音：语音跟上");
      LANGCASE(1, "{ \"Language\": 7, \"VoiceLanguage\": 7 }", "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "开关 = 英文、文件是俄文：文字与语音都改英文");

      LANGCASE(4, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 5, \"VoiceLanguage\": 0 }", "开关 = 韩语、资源里没有韩语配音分块：文字改 koKR，语音配英文");
      { char rd[1300]; snprintf(rd, sizeof rd, "%s/romfs_ru/data/11-00000001.data", root); put(rd, "x"); }
      LANGCASE(4, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 5, \"VoiceLanguage\": 5 }", "开关 = 韩语、资源里有 11 号分块：文字与语音都改成 koKR");
      LANGCASE(4, "{ \"Language\": 5, \"VoiceLanguage\": 5 }", "{ \"Language\": 5, \"VoiceLanguage\": 5 }", "开关 = 韩语、文件已是韩文配韩语语音：不动");

      LANGCASE(5, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 1, \"VoiceLanguage\": 0 }", "开关 = 德语、资源里没有德语配音分块：文字改 deDE，语音配英文");
      { char rd[1300]; snprintf(rd, sizeof rd, "%s/romfs_ru/data/06-00000001.data", root); put(rd, "x"); }
      LANGCASE(5, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 1, \"VoiceLanguage\": 1 }", "开关 = 德语、资源里有 6 号分块：文字与语音都改成 deDE");
      LANGCASE(5, "{ \"Language\": 1, \"VoiceLanguage\": 1 }", "{ \"Language\": 1, \"VoiceLanguage\": 1 }", "开关 = 德语、文件已是德文配德语语音：不动");
      LANGCASE(3, "{ \"Language\": 5, \"VoiceLanguage\": 5 }", "{ \"Language\": 7, \"VoiceLanguage\": 7 }", "开关 = 俄语、文件是韩文：文字与语音都改成 ruRU");
      LANGCASE(1, "{ \"Language\": 5, \"VoiceLanguage\": 5 }", "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "开关 = 英文、文件是韩文：文字与语音都改英文");

      LANGCASE(6, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 12, \"VoiceLanguage\": 0 }", "开关 = 葡语、资源里没有葡语配音分块：文字改 ptBR，语音配英文");
      { char rd[1300]; snprintf(rd, sizeof rd, "%s/romfs_ru/data/15-00000001.data", root); put(rd, "x"); }
      LANGCASE(6, "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "{ \"Language\": 12, \"VoiceLanguage\": 12 }", "开关 = 葡语、资源里有 15 号分块：文字与语音都改成 ptBR");
      LANGCASE(6, "{ \"Language\": 12, \"VoiceLanguage\": 12 }", "{ \"Language\": 12, \"VoiceLanguage\": 12 }", "开关 = 葡语、文件已是葡语配葡语语音：不动");
      LANGCASE(1, "{ \"Language\": 12, \"VoiceLanguage\": 12 }", "{ \"Language\": 0, \"VoiceLanguage\": 0 }", "开关 = 英文、文件是葡语：文字与语音都改英文");
      VOICECASE(0, 1, "{ \"Language\": 8, \"VoiceLanguage\": 8 }", "{ \"Language\": 8, \"VoiceLanguage\": 0 }", "中文文字 + English 配音，只改 VoiceLanguage");
      VOICECASE(0, 0, "{ \"Language\": 8, \"VoiceLanguage\": 0 }", "{ \"Language\": 8, \"VoiceLanguage\": 8 }", "中文文字 + 中文配音，只改 VoiceLanguage");
      VOICECASE(-1, 1, "{ \"Language\": 9, \"VoiceLanguage\": 9 }", "{ \"Language\": 9, \"VoiceLanguage\": 0 }", "没动语言行，只把繁中配音改成 English，文字保持繁中");

      CHECK(voice_for(1) == 1 && voice_for(5) == 5 && voice_for(7) == 7 && voice_for(12) == 12 && voice_for(8) == 8, "voice: de ko ru pt with their chunks, zhCN always");
      CHECK(voice_for(3) == 0 && voice_for(9) == 0 && voice_for(11) == 0, "voice: fr zhTW ja without their chunks -> English");
      { char rd[1300]; snprintf(rd, sizeof rd, "%s/romfs_ru/data/08-00000001.data", root); put(rd, "x"); snprintf(rd, sizeof rd, "%s/romfs_ru/data/12-00000001.data", root); put(rd, "x"); }
      CHECK(voice_for(3) == 3 && voice_for(9) == 9 && voice_for(2) == 0, "voice: fr and zhTW with their chunks, es still without");
      unsetenv("DM_ROMFS");
      g_t_lang = g_t_voice = -1; }

    char cmd[1200]; snprintf(cmd, sizeof cmd, "rm -rf '%s'", root); system(cmd);
    printf("== %d 项，失败 %d ==\n", checks, fails);
    return fails != 0;
}
