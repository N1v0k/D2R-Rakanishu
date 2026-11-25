package com.xiaoboluo.d2r;



final class BookLayoutTest {
    private static void check(boolean value, String what) { if (!value) throw new AssertionError(what); }

    public static void main(String[] args) {
        check(BookLayout.pages(0) == 1 && BookLayout.pages(15) == 1 && BookLayout.pages(16) == 2 && BookLayout.pages(440) == 30, "pages");
        check(BookLayout.onPage(16, 0) == 15 && BookLayout.onPage(16, 1) == 1 && BookLayout.onPage(16, 2) == 0, "tiles on a page");
        check(BookLayout.clamp(5, 2) == 1 && BookLayout.clamp(-1, 2) == 0, "clamp");

        float[][] grids = {{120, 420, 2280, 980}, {80, 360, 1360, 900}};
        for (float[] grid : grids) {
            float[][] t = new float[BookLayout.PER_PAGE][4];
            for (int i = 0; i < BookLayout.PER_PAGE; i++) {
                BookLayout.tile(grid[0], grid[1], grid[2], grid[3], 10, i, t[i]);
                check(t[i][0] >= grid[0] - 1e-3 && t[i][2] <= grid[2] + 1e-3 && t[i][1] >= grid[1] - 1e-3 && t[i][3] <= grid[3] + 1e-3, "inside the grid");
                check(t[i][2] - t[i][0] >= 110 && t[i][3] - t[i][1] >= 110, "large enough to tap");
                float[] p = new float[4];
                BookLayout.picture(t[i], 8, p);
                check(Math.abs((p[2] - p[0]) - (p[3] - p[1])) < 1e-3 && p[3] <= BookLayout.bandTop(t[i]) + 1e-3 && p[0] >= t[i][0] && p[2] <= t[i][2], "picture square above the band");
                for (int j = 0; j < i; j++) check(t[i][2] <= t[j][0] || t[j][2] <= t[i][0] || t[i][3] <= t[j][1] || t[j][3] <= t[i][1], "no overlap");
            }
            check(t[4][1] == t[0][1] && t[5][0] == t[0][0] && t[5][1] > t[0][3], "five a row, then the next row");
        }

        float[][] r = {{0, 0, 100, 40}, {120, 0, 220, 40}, {0, 60, 100, 160}, {110, 60, 210, 160}, {220, 60, 320, 160}, {0, 170, 100, 270}};
        check(BookLayout.near(r, 2, 1, 0) == 3 && BookLayout.near(r, 3, 1, 0) == 4 && BookLayout.near(r, 4, 1, 0) == 4, "right along the row, stays at its end");
        check(BookLayout.near(r, 3, -1, 0) == 2 && BookLayout.near(r, 2, -1, 0) == 2, "left");
        check(BookLayout.near(r, 2, 0, -1) == 0 && BookLayout.near(r, 4, 0, -1) == 1, "up to the nearest button");
        check(BookLayout.near(r, 0, 0, 1) == 2 && BookLayout.near(r, 2, 0, 1) == 5 && BookLayout.near(r, 4, 0, 1) == 5, "down");
        check(BookLayout.near(r, -1, 0, 1) == 0 && BookLayout.near(new float[0][], 0, 0, 1) == -1, "no focus yet");
        System.out.println("BookLayoutTest: grid, tap sizes, picture square, pages and controller steps passed");
    }
}
