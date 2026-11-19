





#include "dm_doorspot.h"
#include <stdio.h>

static int fails, checks;
#define CHECK(c, ...) do { checks++; if (!(c)) { fails++; printf("  ✗ "); printf(__VA_ARGS__); printf("\n"); } } while (0)

static struct ds_grid g;
static void fill(int v) { memset(g.blk, v, sizeof g.blk); }
static void rect(int x0, int y0, int x1, int y1, int v) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) if (x >= -DS_R && x <= DS_R && y >= -DS_R && y <= DS_R) g.blk[(y + DS_R) * DS_W + x + DS_R] = (uint8_t)v; }
static int blk(int x, int y) { return ds_blk(&g, x + DS_R, y + DS_R); }
static int dist(int x, int y) { return ds_dist(&g, x + DS_R, y + DS_R); }

int main(void) {
    int x, y, t;


    fill(0); ds_flood(&g);
    t = ds_pick(&g, 12, -3, 0, 0, 0, 6, &x, &y); CHECK(t == 0 && x == 12 && y == -3, "空地上应取理想点 12,-3 档 0，得到 %d,%d 档 %d", x, y, t);
    t = ds_pick(&g, 2, 12, 1, 12, -3, 6, &x, &y); CHECK(t == 0 && x == 2 && y == 12, "空地上第二扇应取理想点 2,12，得到 %d,%d 档 %d", x, y, t);
    CHECK(dist(12, -3) == 15, "空地上 12,-3 走 15 步，算出 %d", dist(12, -3));



    fill(1); rect(-DS_R, -4, DS_R, 4, 0); ds_flood(&g);
    CHECK(blk(5, 10) && blk(-5, 10), "对照：旧规则的两个落点 ±5,10 在墙里");


    t = ds_pick(&g, 5, 10, 0, 0, 0, 6, &x, &y); CHECK(t == 0 && x == 5 && y == 1, "走廊里应取 5,1 档 0，得到 %d,%d 档 %d", x, y, t);
    CHECK(dist(x, y) >= 0 && !blk(x, y), "落点必须可达且不挡");
    int x2, y2; t = ds_pick(&g, -5, 10, 1, x, y, 6, &x2, &y2); CHECK(t == 0 && x2 == -5 && y2 == 1, "走廊里第二扇应取 -5,1，得到 %d,%d 档 %d", x2, y2, t);



    fill(1); rect(-DS_R, -4, DS_R, 4, 0); rect(-DS_R, 8, DS_R, 16, 0); ds_flood(&g);
    CHECK(!blk(5, 10) && dist(5, 10) < 0, "对照：5,10 不挡、但从人这里走不到");
    t = ds_pick(&g, 5, 10, 0, 0, 0, 6, &x, &y); CHECK(dist(x, y) >= 0 && y <= 1, "隔墙空地不许选，应留在人这一侧，得到 %d,%d", x, y);



    fill(1); rect(-DS_R, -1, DS_R, 1, 0); ds_flood(&g);
    t = ds_pick(&g, 5, 10, 0, 0, 0, 6, &x, &y); CHECK(t == 2 && y == 0 && abs(x) == 1, "窄道里应退到脚下最近的可达格（±1,0）档 2，得到 %d,%d 档 %d", x, y, t);
    t = ds_pick(&g, -5, 10, 1, x, y, 6, &x2, &y2); CHECK(t == 2 && y2 == 0 && abs(x2 - x) >= 2 && dist(x2, y2) >= 0, "窄道里第二扇离第一扇至少 2 格且可达，得到 %d,%d 档 %d", x2, y2, t);



    fill(1); rect(0, 0, 0, 0, 0); ds_flood(&g);
    t = ds_pick(&g, 5, 10, 0, 0, 0, 6, &x, &y); CHECK(t == 3 && x == 0 && y == 0 && g.reach == 1, "围死时落在脚下档 3，得到 %d,%d 档 %d（可达 %d 格）", x, y, t, g.reach);



    fill(0); rect(-20, 6, 20, 7, 1); ds_flood(&g);
    CHECK(dist(0, 12) > DS_FAR, "对照：墙背面 0,12 要走 %d 步（> %d）", dist(0, 12), DS_FAR);
    t = ds_pick(&g, 0, 12, 0, 0, 0, 6, &x, &y); CHECK(t == 0 && abs(x) == 3 && y == 2, "长墙前应取墙这一侧的 ±3,2（y = 2 是周围 2 格都可达的最靠墙一行；离人至少 3 格所以 x = ±3），得到 %d,%d 档 %d", x, y, t);



    fill(0); ds_flood(&g);
    t = ds_pick_ring(&g, 36, 0, 24, 80, &x, &y); CHECK(t == 2 && x == 36 && y == 0, "空地上应取 36,0 圈 2，得到 %d,%d 圈 %d", x, y, t);





    fill(1); rect(-DS_R, -4, DS_R, 4, 0); rect(-DS_R, 26, DS_R, 34, 0); ds_flood(&g);
    CHECK(!blk(0, 30) && dist(0, 30) < 0, "对照：0,30 不挡、但从人这里走不到");
    t = ds_pick_ring(&g, 0, 36, 24, 80, &x, &y); CHECK(t == 2 && abs(x) == 24 && y == 1 && dist(x, y) >= 0, "隔墙走廊不许选，应取 ±24,1 圈 2，得到 %d,%d 圈 %d", x, y, t);



    fill(1); rect(-10, -10, 10, 10, 0); ds_flood(&g);
    t = ds_pick_ring(&g, 36, 0, 24, 80, &x, &y); CHECK(t == -1, "屋子里不到 24 格应返回 -1，得到 %d", t);





    { static const char *pat[5] = { "11111", "01001", "00001", "00011", "00011" }; fill(0);
      for (int r = 0; r < 5; r++) for (int c = 0; c < 5; c++) if (pat[r][c] == '1') rect(c - 2, r - 2, c - 2, r - 2, 1);
      ds_flood_r(&g, 0); CHECK(g.reach == 1, "入口窄道不放宽：可达 1 格（实际 %d）", g.reach);
      ds_flood_r(&g, 2); CHECK(g.reach > 100 && dist(-1, 0) == 1 && dist(0, 1) == 1, "放宽 2 步：走得出去（可达 %d 格，左 %d 步、下 %d 步）", g.reach, dist(-1, 0), dist(0, 1));
      CHECK(dist(1, 1) < 0 && dist(0, -1) >= 0, "墙格仍不可达、空格可达"); }

    printf("t_doorspot：%d 项，%d 项不通\n", checks, fails); return fails != 0;
}
