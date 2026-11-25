


#include "dm_res_scale.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks, fails;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static int size_is(int pct, int W, int H, int ew, int eh) { int w, h; return dm_res_scale_size(pct, W, H, &w, &h) && w == ew && h == eh; }
static void size_of(int pct, int W, int H, int *w, int *h) { dm_res_scale_size(pct, W, H, w, h); }

int main(void) {

    static const int p2688[][3] = { { 60, 1616, 730 }, { 65, 1744, 790 }, { 70, 1880, 852 }, { 75, 2016, 912 }, { 80, 2152, 972 }, { 85, 2288, 1034 }, { 90, 2416, 1094 }, { 95, 2552, 1156 }, { 100, 2688, 1216 } };
    static const int p2480[][3] = { { 65, 1616, 726 }, { 70, 1736, 782 }, { 75, 1864, 838 }, { 80, 1984, 892 }, { 85, 2104, 948 }, { 90, 2232, 1004 }, { 95, 2352, 1060 }, { 100, 2480, 1116 } };
    for (unsigned i = 0; i < sizeof p2688 / sizeof *p2688; i++) { int w, h; size_of(p2688[i][0], 2688, 1216, &w, &h); CHECK(w == p2688[i][1] && h == p2688[i][2], "2688 × 1216 %d %%：应 %d × %d，得 %d × %d", p2688[i][0], p2688[i][1], p2688[i][2], w, h); }
    for (unsigned i = 0; i < sizeof p2480 / sizeof *p2480; i++) { int w, h; size_of(p2480[i][0], 2480, 1116, &w, &h); CHECK(w == p2480[i][1] && h == p2480[i][2], "2480 × 1116 %d %%：应 %d × %d，得 %d × %d", p2480[i][0], p2480[i][1], p2480[i][2], w, h); }
    CHECK(size_is(75, 2688, 1216, 2016, 912), "规格里的例子 75 %% = 2016 × 912");
    CHECK(size_is(75, 1216, 2688, 2016, 912), "竖着报的屏（短边在前）一样算");
    CHECK(size_is(DM_RES_AUTO, 2688, 1216, 1992, 900) && size_is(DM_RES_AUTO, 2480, 1116, 2000, 900), "缺省 = 900 行：2688×1216 1992 × 900、2480×1116 2000 × 900（同 2.2.8）");
    { int w, h; CHECK(!dm_res_scale_size(80, 0, 0, &w, &h) && !w && !h, "屏幕尺寸未知：返回 0"); }



    { static const int scr[][2] = { { 2688, 1216 }, { 2480, 1116 }, { 2400, 1080 }, { 3200, 1440 }, { 2412, 1080 }, { 2048, 1536 }, { 2560, 1600 }, { 1920, 1080 }, { 3120, 1440 }, { 2340, 1080 } };
      for (unsigned s = 0; s < sizeof scr / sizeof *scr; s++) { int W = scr[s][0], H = scr[s][1], hb = dm_res_base_h(W, H), pw = 0, ph = 0, bad = 0;
          for (int p = dm_res_scale_lo(W, H); p <= 100; p += 5) { int w, h; size_of(p, W, H, &w, &h);
              double ar = (double)w / h, want = dm_res_wider(W, H) ? (double)W / H : 16.0 / 9;
              if (w % 8 || h % 2 || (ar / want > 1.005 || ar / want < 0.995) || w < pw || h <= ph || w > W || w > 4096) { bad++; printf("    %d × %d %d %% → %d × %d\n", W, H, p, w, h); }
              pw = w; ph = h; }
          int w, h; size_of(100, W, H, &w, &h);
          CHECK(!bad, "%d × %d：%d 格不合规矩", W, H, bad);
          CHECK(h == hb && (W % 8 || W > 4096 || !dm_res_wider(W, H) || w == W), "%d × %d 100 %% = %d × %d（基准高 %d）", W, H, w, h, hb); } }


    CHECK(dm_res_scale_lo(2688, 1216) == 60 && dm_res_scale_lo(2480, 1116) == 65 && dm_res_scale_lo(2400, 1080) == 70 && dm_res_scale_lo(3200, 1440) == 50,
          "最低格：2688×1216 60（%d）、2480×1116 65（%d）、1080 行 70（%d）、1440 行 50（%d）", dm_res_scale_lo(2688, 1216), dm_res_scale_lo(2480, 1116), dm_res_scale_lo(2400, 1080), dm_res_scale_lo(3200, 1440));
    CHECK(dm_res_scale_lo(1600, 720) == 100 && dm_res_scale_lo(1280, 600) == 100, "720 行及以下的屏只有 100 %%");
    CHECK(size_is(70, 2400, 1080, 1680, 756) && size_is(50, 3200, 1440, 1600, 720), "2400 × 1080 最低 70 %% = 1680 × 756；3200 × 1440 最低 50 %% = 1600 × 720");

    CHECK(dm_res_base_h(2048, 1536) == 1152 && size_is(100, 2048, 1536, 2048, 1152) && dm_res_scale_lo(2048, 1536) == 65 && size_is(65, 2048, 1536, 1336, 748),
          "4:3 2048 × 1536：基准高 1152，100 %% = 2048 × 1152（不比屏宽），最低 65 %% = 1336 × 748");
    CHECK(dm_res_base_h(2560, 1600) == 1440 && size_is(100, 2560, 1600, 2560, 1440), "16:10 2560 × 1600：基准高 1440，100 %% = 2560 × 1440");
    CHECK(size_is(100, 2412, 1080, 2408, 1080), "屏宽不是 8 的倍数（2412）：100 %% = 2408 × 1080，不比屏宽");
    CHECK(size_is(DM_RES_AUTO, 1600, 720, 2000, 900), "比 900 行矮的屏：缺省照旧 900 行、宽按屏比例（2000 × 900，同 2.2.8，不卡屏宽）");


    CHECK(dm_res_scale_nearest_lines(900, 2688, 1216) == 75 && dm_res_scale_nearest_lines(900, 2480, 1116) == 80, "900 行最近：2688×1216 75、2480×1116 80");
    CHECK(dm_res_scale_nearest_lines(1080, 2688, 1216) == 90 && dm_res_scale_nearest_lines(1080, 2480, 1116) == 95, "1080 行最近：2688×1216 90（%d）、2480×1116 95（%d）", dm_res_scale_nearest_lines(1080, 2688, 1216), dm_res_scale_nearest_lines(1080, 2480, 1116));
    CHECK(dm_res_scale_nearest_lines(500, 2688, 1216) == 60 && dm_res_scale_nearest_lines(5000, 2688, 1216) == 100, "超出范围：夹到最低 / 最高格");

    CHECK(dm_res_legacy_lines(0, 1216) == 900 && dm_res_legacy_lines(1, 1216) == 1080 && dm_res_legacy_lines(2, 1216) == 1216, "2.2.8 档：900 / 1080 / 原生");
    CHECK(dm_res_legacy_lines(1, 1080) == 900 && dm_res_legacy_lines(2, 720) == 900, "屏不够高：res=1 在 1080 行屏、res=2 在 720 行屏上都跑 900p");


    { int w, h, dw, dh;
      size_of(60, 2688, 1216, &dw, &dh); dm_res_handheld(dw, dh, 0, 2688, 1216, &w, &h); CHECK(h == 720 && w <= dw, "2688×1216 60 %%（%d 行）掌机档 720 行（%d × %d），不超过主机档", dh, w, h);
      size_of(100, 2688, 1216, &dw, &dh); dm_res_handheld(dw, dh, 0, 2688, 1216, &w, &h); CHECK(h == 972 && w == 2152, "2688×1216 100 %% 掌机档 972 行（%d × %d）", w, h);
      size_of(DM_RES_AUTO, 2688, 1216, &dw, &dh); dm_res_handheld(dw, dh, 1, 2688, 1216, &w, &h); CHECK(h == 720 && w == 1592, "缺省档的掌机档照旧 720 行、1592 宽（%d × %d）", w, h);
      for (int p = 50; p <= 100; p += 5) { size_of(p, 1600, 720, &dw, &dh); dm_res_handheld(dw, dh, 0, 1600, 720, &w, &h); CHECK(h <= dh && w <= dw, "掌机档不超过主机档（%d %%）", p); } }


    { int f[64], n;
      n = dm_fps_notches(60, 1, f, 64); CHECK(n == 7 && f[0] == 30 && f[5] == 55 && f[6] == 0, "60 Hz：30 … 55、Max，共 7 格（%d）", n);
      n = dm_fps_notches(120, 1, f, 64); CHECK(n == 19 && f[17] == 115 && f[18] == 0, "120 Hz：30 … 115、Max，共 19 格（%d）", n);
      n = dm_fps_notches(144, 1, f, 64); CHECK(n == 24 && f[22] == 140 && f[23] == 0, "144 Hz：30 … 140、Max，共 24 格（%d）", n);
      n = dm_fps_notches(90, 1, f, 64); CHECK(n == 13 && f[11] == 85 && f[12] == 0, "90 Hz：30 … 85、Max（%d）", n);
      n = dm_fps_notches(0, 1, f, 64); CHECK(n == 7, "刷新率不知道：按 60 Hz（%d）", n);
      n = dm_fps_notches(144, 0, f, 64); CHECK(n == 2 && f[0] == 30 && f[1] == 60, "只有 30 / 60 的平台（Metal）：两格、没有 Max");
      n = dm_fps_notches(360, 1, f, 64); CHECK(n == 44 && f[42] == DM_FPS_EXPLICIT_MAX && f[43] == 0, "360 Hz：明确的格到 240 为止（存盘读盘同一上限），再往上只有 Max（%d）", n);
      int ok = 1; n = dm_fps_notches(240, 1, f, 64); for (int i = 0; i + 1 < n; i++) ok &= f[i] % 5 == 0 && (!i || f[i] == f[i - 1] + 5); CHECK(ok && f[n - 1] == 0 && f[n - 2] == 235, "每格 5 的倍数、连续，最后一格 Max"); }


    { static const int m3[] = { 60, 90, 120 }, m4[] = { 60, 90, 120, 144 }, m1[] = { 144 }; int e;
      static const int c[][3] = { { 30, 60, 1 }, { 40, 120, 1 }, { 45, 90, 1 }, { 60, 60, 1 }, { 90, 90, 1 }, { 75, 120, 0 }, { 115, 120, 0 }, { 0, 120, 1 }, { 35, 120, 0 }, { 50, 120, 0 }, { 55, 120, 0 } };
      for (unsigned i = 0; i < sizeof c / sizeof *c; i++) { int v = dm_fps_vote(c[i][0], m3, 3, 0, &e); CHECK(v == c[i][1] && e == c[i][2], "{60,90,120} %d FPS → %d Hz %s（得 %d %d）", c[i][0], c[i][1], c[i][2] ? "均匀" : "不均匀", v, e); }
      CHECK(dm_fps_vote(72, m4, 4, 0, &e) == 144 && e, "{60,90,120,144} 72 → 144 均匀");
      CHECK(dm_fps_vote(70, m4, 4, 0, &e) == 144 && !e, "{60,90,120,144} 70 → 144 不均匀");
      CHECK(dm_fps_vote(30, m1, 1, 0, &e) == 144 && !e && dm_fps_vote(0, m1, 1, 0, &e) == 144 && e, "只有 144 Hz：30 不均匀，Max 144");
      CHECK(dm_fps_vote(45, NULL, 0, 0, &e) == 45 && dm_fps_vote(0, NULL, 0, 0, &e) == 144, "没有刷新率表：投 fps 本身，Max 投 144（同以前）");

      CHECK(dm_fps_vote(30, m3, 3, 1, &e) == 120 && dm_fps_vote(60, m3, 3, 1, &e) == 120 && dm_fps_vote(45, m3, 3, 1, &e) == 90, "最大规则：30 → 120、60 → 120、45 → 90");
      CHECK(dm_fps_divides(119, 60) && dm_fps_divides(121, 60) && !dm_fps_divides(120, 70) && !dm_fps_divides(50, 60), "1 %% 余量：119 / 121 Hz 仍算 60 的两倍；120 / 70、比 fps 低的刷新率不算"); }


    { dm_res_live_t s = {0}; int r = 0, i;
      CHECK(dm_res_live_step(&s, 1880, 852, 1880, 852, 1, 0, 1) == DM_RL_NONE, "没在切：什么都不报");
      dm_res_live_begin(&s, 1880, 852, 1504, 720, 1992, 900, 1600, 720);
      CHECK(dm_res_live_step(&s, 1992, 900, 1992, 900, 1, 0, 1) == DM_RL_NONE, "游戏还没切：等");
      CHECK(dm_res_live_step(&s, 1880, 852, 1992, 900, 1, 0, 1) == DM_RL_NONE, "裁剪区对了、交换链还是旧的：不算");
      CHECK(dm_res_live_step(&s, 1880, 852, 1880, 852, 1, -22, 1) == DM_RL_NONE, "setBuffersGeometry 失败：不算");
      CHECK(dm_res_live_step(&s, 1880, 852, 1880, 852, 1, 0, 1) == DM_RL_REACHED && s.phase == DM_RL_IDLE, "裁剪区、交换链都是目标、几何 0：切好了");
      dm_res_live_begin(&s, 1880, 852, 1504, 720, 1992, 900, 1600, 720);
      CHECK(dm_res_live_step(&s, 1504, 720, 1504, 720, 1, 0, 1) == DM_RL_NONE && dm_res_live_step(&s, 1504, 720, 1504, 720, 1, 0, 0) == DM_RL_REACHED, "掌机档（DM_OPMODE=0）只认掌机档的目标尺寸");
      dm_res_live_begin(&s, 2688, 1216, 2152, 972, 2152, 972, 1720, 776);
      CHECK(dm_res_live_step(&s, 2152, 972, 2152, 972, 1, 0, 1) == DM_RL_NONE && dm_res_live_step(&s, 2688, 1216, 2688, 1216, 1, 0, 1) == DM_RL_REACHED, "主机档 80 %% → 100 %%：没切的旧画面（= 目标的掌机档尺寸）不算切好");
      dm_res_live_begin(&s, 1880, 852, 1504, 720, 1992, 900, 1600, 720);
      CHECK(dm_res_live_step(&s, 1880, 852, 2688, 1216, 0, 0, 1) == DM_RL_REACHED, "不按游戏尺寸出图（标记关）：交换链是窗口大小，不比");
      dm_res_live_begin(&s, 1880, 852, 1504, 720, 1992, 900, 1600, 720);
      for (i = 0; i < DM_RES_LIVE_FRAMES && (r = dm_res_live_step(&s, 1880, 852, 1888, 852, 1, 0, 1)) == DM_RL_NONE; i++) {}
      CHECK(r == DM_RL_REVERT && i == DM_RES_LIVE_FRAMES - 1 && s.phase == DM_RL_BACK && s.tw == 1992 && s.th == 900 && s.thw == 1600 && s.pw == 1880, "120 帧没核上：退回上一个尺寸（目标换成 1992 × 900，第 %d 帧）", i + 1);
      CHECK(dm_res_live_step(&s, 1992, 900, 1992, 900, 1, 0, 1) == DM_RL_REVERTED && s.phase == DM_RL_IDLE, "退回核上了");
      dm_res_live_begin(&s, 1880, 852, 1504, 720, 1992, 900, 1600, 720);
      for (i = 0; i < 2 * DM_RES_LIVE_FRAMES && (r = dm_res_live_step(&s, 1700, 800, 1700, 800, 1, 0, 1)) != DM_RL_BROKEN; i++) {}
      CHECK(r == DM_RL_BROKEN && i == 2 * DM_RES_LIVE_FRAMES - 1 && s.phase == DM_RL_IDLE, "去和回都没核上：坏了（第 %d 帧）", i + 1);
      CHECK(dm_res_live_fits(1880, 852, 1992, 900, 1992, 900) && dm_res_live_fits(1992, 900, 1992, 900, 1992, 900) && !dm_res_live_fits(2016, 912, 1992, 900, 1992, 900)
            && !dm_res_live_fits(1880, 852, 1992, 900, 0, 0) && !dm_res_live_fits(1880, 852, 2152, 972, 1800, 972), "上限 = min(启动尺寸, 游戏窗口尺寸)，游戏的不知道 = 不能");
      int off; CHECK(dm_res_live_boot(0, 1, &off) == 1 && !off && dm_res_live_boot(1, 1, &off) == 2 && off && dm_res_live_boot(1, 0, &off) == 1 && !off, "切换中进程没了：第一次记 1、连续第二次关掉；没在切的启动不加"); }



    { const dm_dev_profile_t *a = dm_dev_profile_find("sm8550", ""), *b = dm_dev_profile_find("", "kalama"), *c = dm_dev_profile_find("SM8750", "sun"), *d = dm_dev_profile_find("SM8750", "kalama");
      CHECK(a && b && a == b && !strcmp(a->gpu, "Adreno 740") && !a->estimate, "2480×1116：按 ro.soc.model（不分大小写）或 ro.board.platform 都认出 Adreno 740，实测过（不是估计）");
      CHECK(c && !strcmp(c->gpu, "Adreno 830") && c->estimate, "2688×1216：Adreno 830，标成估计");
      CHECK(d == c, "ro.soc.model 优先于 ro.board.platform");
      CHECK(!dm_dev_profile_find("SM8650", "pineapple") && !dm_dev_profile_find(NULL, NULL) && !dm_dev_profile_find("SM855", "") && !dm_dev_profile_find("", "") && !dm_dev_profile_find("ranchu", "ranchu"),
            "不在表里（别的芯片、模拟器、空、前缀）= 没有设备缺省");
      CHECK(c->budget_px <= a->budget_px, "2688×1216 的预算不比 2480×1116 的高（保守）");
      CHECK(!dm_dev_default_pct(a->budget_px, 2480, 1116) && !dm_dev_default_pct(c->budget_px, 2688, 1216), "表里现在的值（实测后的保守退路）：2480×1116 与 2688×1216 都没有比 900 行高的格（%d / %d）",
            dm_dev_default_pct(a->budget_px, 2480, 1116), dm_dev_default_pct(c->budget_px, 2688, 1216));
      for (unsigned i = 0; i < sizeof dm_dev_profiles / sizeof *dm_dev_profiles; i++) CHECK(dm_dev_profiles[i].s3d >= 75 && dm_dev_profiles[i].s3d <= 100 && dm_dev_profiles[i].s3d % 5 == 0 && dm_dev_profiles[i].budget_px > 0, "表第 %u 行：3D 在滑块的格上、预算 > 0", i); }
    { const int64_t nat8 = (int64_t)2480 * 1116, p95_8 = (int64_t)2352 * 1060;
      CHECK(dm_dev_default_pct(nat8, 2480, 1116) == 100 && dm_dev_default_pct(nat8 - 1, 2480, 1116) == 95, "2480×1116：预算 = 原生的精确像素 → 100 %%，少一个像素 → 95 %%（不取整）");
      CHECK(dm_dev_default_pct(p95_8, 2480, 1116) == 95, "2480×1116：预算 = 95 %% 格 → 95 %% = 2352 × 1060");
      CHECK(dm_dev_default_pct(nat8, 2688, 1216) == 90 && dm_dev_default_pct(p95_8, 2688, 1216) == 85, "2688×1216：2480×1116 原生的预算 → 90 %% = 2416 × 1094（%d），2480×1116 95 %% 的预算 → 85 %% = 2288 × 1034（%d）",
            dm_dev_default_pct(nat8, 2688, 1216), dm_dev_default_pct(p95_8, 2688, 1216));
      CHECK(dm_dev_default_pct(p95_8, 1216, 2688) == 85, "竖着报的屏一样算");
      CHECK(dm_dev_default_pct((int64_t)2000 * 900, 2480, 1116) == 0 && dm_dev_default_pct((int64_t)2104 * 948 - 1, 2480, 1116) == 0 && dm_dev_default_pct((int64_t)2104 * 948, 2480, 1116) == 85,
            "预算只够 900 行或更低 = 没有设备缺省；刚够 85 %% 的格（948 行）= 85 %%");
      CHECK(dm_dev_default_pct(p95_8, 2400, 1080) == 95 && dm_dev_default_pct(p95_8, 1600, 720) == 0 && dm_dev_default_pct(p95_8, 1920, 1080) == 100, "别的屏：2400 × 1080 → 95 %%，720 行的屏 → 没有，1920 × 1080 → 100 %%");
      CHECK(dm_dev_default_pct(0, 2480, 1116) == 0 && dm_dev_default_pct(p95_8, 0, 0) == 0, "预算 0 / 屏幕不知道 = 没有");
      { int pct = dm_dev_default_pct(p95_8, 2048, 1536), w, h; size_of(pct, 2048, 1536, &w, &h); CHECK(pct && h > 900 && (int64_t)w * h <= p95_8 && w <= 2048, "4:3 平板：设备缺省的格比 900 行高、不超预算、不比屏宽（%d %% = %d × %d）", pct, w, h); }
      for (int H = 720; H <= 1600; H += 2) { int W = H * 20 / 9, pct = dm_dev_default_pct(p95_8, W, H), w, h;
          if (pct) { size_of(pct, W, H, &w, &h); if (!(h > 900 && (int64_t)w * h <= p95_8 && pct % 5 == 0 && pct >= dm_res_scale_lo(W, H))) { CHECK(0, "%d × %d：%d %% = %d × %d 不合规矩", W, H, pct, w, h); break; }
              int w2, h2; if (pct < 100) { size_of(pct + 5, W, H, &w2, &h2); if ((int64_t)w2 * h2 <= p95_8) { CHECK(0, "%d × %d：%d %% 不是预算内最高的格", W, H, pct); break; } } } } }
    { CHECK(!dm_dev_off_applies(0, "3.3.94012-r45.n6", "3.3.94012-r45.n6"), "没有 res_dev_off = 设备缺省开着");
      CHECK(dm_dev_off_applies(1, "3.3.94012-r45.n6", "3.3.94012-r45.n6"), "同一个产物 = 关着");
      CHECK(!dm_dev_off_applies(1, "3.3.94012-r45.n6", "3.3.94012-r45.n7"), "换了产物的新版本 = 再试一次");
      CHECK(dm_dev_off_applies(1, "", "3.3.94012-r45.n6") && dm_dev_off_applies(1, "3.3.94012-r45.n6", "") && dm_dev_off_applies(1, NULL, NULL), "文件空 / 产物读不到 = 照旧关着");
      CHECK(dm_res_boot_step(900, 5) == DM_RB_NONE && dm_res_boot_step(852, 0) == DM_RB_NONE, "不超过 900 行的启动不计（计数再高也不退）");
      CHECK(dm_res_boot_step(1060, 0) == DM_RB_COUNT && dm_res_boot_step(1060, 1) == DM_RB_COUNT && dm_res_boot_step(1060, 2) == DM_RB_FALLBACK && dm_res_boot_step(1116, 7) == DM_RB_FALLBACK,
            "比 900 行高：前两次记数，连续第三次（前两次没出满 300 帧）退回");
      int w, h; CHECK(dm_res_scale_size(DM_RES_AUTO, 2480, 1116, &w, &h) && h == 900, "退回用的 auto 永远是 900 行（设备缺省不经过 dm_res_scale_size 的 auto）"); }


    { CHECK(dm_res_full_on(2048, 1536) && dm_res_full_on(2208, 1840) && dm_res_full_on(2560, 1600) && dm_res_full_on(1536, 2048), "4:3, foldable inner and 16:10 are narrower than 16:9");
      CHECK(!dm_res_full_on(1920, 1080) && !dm_res_full_on(2480, 1116) && !dm_res_full_on(2688, 1216) && !dm_res_full_on(1910, 1080) && dm_res_full_on(1909, 1080), "16:9 and wider never; the tolerance is 0.01 (1909 x 1080 is the first)");
      CHECK(!dm_res_full_on(0, 0), "unknown screen: off");
      CHECK(dm_res_full_min(2048, 1536) == 960 && dm_res_full_min(2208, 1840) == 1068 && dm_res_full_min(2560, 1600) == 800, "minimum: 4:3 960, foldable 1068, 16:10 800 (%d %d %d)",
            dm_res_full_min(2048, 1536), dm_res_full_min(2208, 1840), dm_res_full_min(2560, 1600));
      static const int scr[][2] = { { 2048, 1536 }, { 2208, 1840 }, { 2560, 1600 }, { 2800, 2000 }, { 1920, 1200 }, { 2000, 1500 } };
      for (unsigned s = 0; s < sizeof scr / sizeof *scr; s++) { int W = scr[s][0], H = scr[s][1], m = dm_res_full_min(W, H), bad = 0, pw = 0, ph = 0, w, h;
          for (int p = dm_res_scale_lo_x(W, H, 1); p <= 100; p += 5) { dm_res_scale_size_x(p, W, H, 1, &w, &h);
              double ar = (double)w / h, want = (double)W / H;
              if (w % 8 || h % 2 || ar / want > 1.006 || ar / want < 0.994 || w < pw || h <= ph || w > W || h < m || w < DM_RES_FULL_UI_W) { bad++; printf("    full %d x %d %d %% -> %d x %d\n", W, H, p, w, h); }
              pw = w; ph = h; }
          CHECK(!bad, "full %d x %d: %d notches break the rules", W, H, bad);
          dm_res_scale_size_x(100, W, H, 1, &w, &h); CHECK(h == H && (W % 8 || w == W), "full %d x %d: 100 %% = the panel (%d x %d)", W, H, w, h);
          dm_res_scale_size_x(DM_RES_AUTO, W, H, 1, &w, &h); CHECK(h == (m > 900 ? m : 900) && w >= DM_RES_FULL_UI_W && w * 9 / 16 >= 720, "full %d x %d: auto = max(900, minimum) with a 16:9 UI of at least 720 (%d x %d)", W, H, w, h);
          int hw, hh, dw, dh; dm_res_scale_size_x(DM_RES_AUTO, W, H, 1, &dw, &dh); dm_res_handheld_x(dw, dh, 1, W, H, 1, &hw, &hh);
          CHECK(hh >= m && hh <= dh && hw >= DM_RES_FULL_UI_W, "full %d x %d: handheld keeps the minimum (%d x %d)", W, H, hw, hh);
          dm_res_scale_size_x(100, W, H, 1, &dw, &dh); dm_res_handheld_x(dw, dh, 0, W, H, 1, &hw, &hh); CHECK(hh >= m && hh <= dh && hw <= dw, "full %d x %d: handheld of 100 %% (%d x %d)", W, H, hw, hh); }
      int w, h;
      CHECK(dm_res_scale_size_x(DM_RES_AUTO, 2048, 1536, 1, &w, &h) && w == 1280 && h == 960, "4:3 auto = 1280 x 960 (%d x %d)", w, h);
      CHECK(dm_res_scale_size_x(DM_RES_AUTO, 2208, 1840, 1, &w, &h) && w == 1280 && h == 1068, "foldable auto = 1280 x 1068 (%d x %d)", w, h);
      CHECK(dm_res_scale_lo_x(2048, 1536, 1) == 65 && dm_res_scale_lo_x(2208, 1840, 1) == 60, "lowest notch: 4:3 65, foldable 60 (%d %d)", dm_res_scale_lo_x(2048, 1536, 1), dm_res_scale_lo_x(2208, 1840, 1));
      CHECK(dm_res_pct_clamp_x(50, 2048, 1536, 1) == 65 && dm_res_pct_clamp_x(DM_RES_AUTO, 2048, 1536, 1) == DM_RES_AUTO, "a saved notch below the lowest is clamped, auto stays");

      for (int p = 50; p <= 100; p += 5) { int a, b, c, d; dm_res_scale_size(p, 2480, 1116, &a, &b); dm_res_scale_size_x(p, 2480, 1116, 0, &c, &d); CHECK(a == c && b == d, "full = 0 equals the old size (%d %%)", p); }
      CHECK(dm_res_scale_lo_x(2480, 1116, 0) == dm_res_scale_lo(2480, 1116) && dm_res_boot_step_x(852, 0, 0) == DM_RB_NONE, "full = 0: lowest notch and crash guard unchanged");
      CHECK(dm_res_boot_step_x(852, 0, 1) == DM_RB_COUNT && dm_res_boot_step_x(720, 1, 1) == DM_RB_COUNT && dm_res_boot_step_x(960, 2, 1) == DM_RB_FALLBACK, "full starts count at any height; the third in a row falls back");
      CHECK(dm_res_full_frontend_h(1280, 960) == 720 && dm_res_full_frontend_h(2048, 1536) == 1152 && dm_res_full_frontend_h(1080, 900) == 720 && dm_res_full_frontend_h(2000, 900) == 900,
            "out of a run: 16:9 of the width, floored at 720, wide pictures unchanged");
      CHECK(dm_dev_default_pct_x(DM_DEV_BUDGET_CONSERVATIVE, 2048, 1536, 1) == 75, "2480×1116 budget on 4:3 fullscreen: the highest notch inside it is 75 %% = 1536 x 1152");
      CHECK(dm_res_scale_size_x(100, 1024, 768, 1, &w, &h) && w == 1280 && h == 960 && dm_res_scale_lo_x(1024, 768, 1) == 100, "panel narrower than 1280 (1024 x 768): every size keeps the 1280 x 960 minimum (%d x %d)", w, h);
      w = 2208; h = 1840; CHECK(dm_res_fit_cap(&w, &h, 1600, 1200) && w <= 1600 && h <= 1200 && w % 8 == 0 && h % 2 == 0 && w * 1840 / h >= 2190 && w * 1840 / h <= 2226, "fit into the start size keeps the aspect (%d x %d)", w, h);
      w = 1280; h = 960; CHECK(!dm_res_fit_cap(&w, &h, 1600, 1200) && w == 1280 && h == 960, "already inside: unchanged");

      { int sw, sh, tw, th, now, wait, fit; dm_res_scale_size(DM_RES_AUTO, 2480, 1116, &sw, &sh);
        CHECK(sw == 2000 && sh == 900, "wide start 2000 x 900 (%d x %d)", sw, sh);
        CHECK(dm_res_fold_plan(2048, 1536, DM_RES_AUTO, 1, 0, 0, 1, sw, sh, &tw, &th, &now, &wait, &fit) && !now && wait && tw == 1600 && th == 900,
              "unfolded: the start size holds no 1280 wide fullscreen picture, 16:9 with bars at 1600 x 900 and waiting (%d x %d, full %d, wait %d)", tw, th, now, wait);
        CHECK(dm_res_fold_plan(2480, 1116, DM_RES_AUTO, 0, 0, 1, 1, sw, sh, &tw, &th, &now, &wait, &fit) && !now && !wait && tw == 2000 && th == 900,
              "folded back: the wide target 2000 x 900 again and waiting stopped (%d x %d, wait %d)", tw, th, wait);
        CHECK(!dm_res_fold_plan(2480, 1116, DM_RES_AUTO, 0, 0, 0, 1, sw, sh, &tw, &th, &now, &wait, &fit), "wide to wide without a fallback: nothing changes");
        CHECK(dm_res_fold_plan(2048, 1536, DM_RES_AUTO, 1, 0, 0, 1, 1280, 1068, &tw, &th, &now, &wait, &fit) && now && !wait && tw == 1280 && th == 960,
              "started on the foldable inner screen, then 4:3: fullscreen 1280 x 960 (%d x %d)", tw, th);
        CHECK(dm_res_fold_plan(2688, 1216, DM_RES_AUTO, 0, 1, 0, 1, 1280, 1068, &tw, &th, &now, &wait, &fit) && !now && !wait && fit && tw <= 1280 && th <= 1068,
              "fullscreen start folded to a wide screen: scaled into the start size (%d x %d)", tw, th); } }


    { static const int pair[DM_RES_PRESET_N][2] = { { DM_RES_AUTO, 60 }, { DM_RES_AUTO, 0 }, { 100, 100 } };
      for (int k = 0; k < DM_RES_PRESET_N; k++) {
          CHECK(dm_res_presets[k].res_pct == pair[k][0] && dm_res_presets[k].s3d == pair[k][1], "entry %d = res_pct %d, s3d %d", k, dm_res_presets[k].res_pct, dm_res_presets[k].s3d);
          CHECK(dm_res_preset_of(pair[k][0], pair[k][1], 1) == k && dm_res_preset_of(pair[k][0], pair[k][1], 0) == k, "pair of entry %d -> entry %d", k, k); }
      static const int own[][2] = { { 100, 0 }, { 100, 60 }, { DM_RES_AUTO, 100 }, { 95, 100 }, { 75, 0 }, { DM_RES_AUTO, 85 }, { 100, 85 } };
      for (unsigned k = 0; k < sizeof own / sizeof *own; k++) CHECK(dm_res_preset_of(own[k][0], own[k][1], 1) == DM_RES_PRESET_N, "res_pct %d s3d %d = Custom", own[k][0], own[k][1]);
      CHECK(dm_res_preset_of(DM_RES_AUTO, 100, 0) == 2 && dm_res_preset_of(100, 0, 0) == 1 && dm_res_preset_of(75, 60, 0) == 0 && dm_res_preset_of(100, 85, 0) == DM_RES_PRESET_N,
            "without the output resolution: s3d alone decides");

      static const int sharp[][5] = { { 2688, 1216, 0, 2688, 1216 }, { 2400, 1080, 0, 2400, 1080 }, { 3200, 1440, 0, 3200, 1440 }, { 1920, 1080, 0, 1920, 1080 },
                                      { 2772, 1240, 0, 2768, 1240 }, { 2048, 1536, 0, 2048, 1152 }, { 2560, 1600, 0, 2560, 1440 }, { 2048, 1536, 1, 2048, 1536 }, { 2208, 1840, 1, 2208, 1840 } };
      for (unsigned k = 0; k < sizeof sharp / sizeof *sharp; k++) { int w, h, W = sharp[k][0], H = sharp[k][1], f = sharp[k][2];
          CHECK(dm_res_scale_size_x(dm_res_pct_clamp_x(dm_res_presets[2].res_pct, W, H, f), W, H, f, &w, &h) && w == sharp[k][3] && h == sharp[k][4],
                "Sharp on %d x %d (full %d) = %d x %d (%d x %d)", W, H, f, sharp[k][3], sharp[k][4], w, h); } }

    printf("t_res_scale：%d 项，%d 项不通\n", checks, fails); return fails != 0;
}
