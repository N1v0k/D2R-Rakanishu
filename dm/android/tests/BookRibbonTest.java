package com.xiaoboluo.d2r;



final class BookRibbonTest {
    private static void check(boolean value, String what) { if (!value) throw new AssertionError(what); }
    private static int r(int c) { return c >> 16 & 255; }
    private static int g(int c) { return c >> 8 & 255; }
    private static int b(int c) { return c & 255; }
    private static int apart(int x, int y) { return Math.max(Math.abs(r(x) - r(y)), Math.max(Math.abs(g(x) - g(y)), Math.abs(b(x) - b(y)))); }

    public static void main(String[] args) {

        int green = 0xFF4FC238, orange = 0xFFFFA800, red = 0xFFFC4646;
        int[] body = new int[3], base = {green, orange, red};
        for (int i = 0; i < 3; i++) {
            body[i] = BookRibbon.cloth(base[i], BookRibbon.SHADE[2]);
            check(r(body[i]) < r(base[i]) || r(base[i]) == 0, "darker red channel " + i);
            check(r(body[i]) + g(body[i]) + b(body[i]) < (r(base[i]) + g(base[i]) + b(base[i])) * 0.75, "darker " + i);
            check((body[i] >>> 24) == 255, "opaque " + i);
        }
        check(g(body[0]) > r(body[0]) && g(body[0]) > b(body[0]), "green stays green");
        check(r(body[1]) > g(body[1]) && g(body[1]) > b(body[1]), "orange stays orange");
        check(r(body[2]) > g(body[2]) && Math.abs(g(body[2]) - b(body[2])) <= 2, "red stays red");
        check(apart(body[0], body[1]) > 40 && apart(body[1], body[2]) > 40 && apart(body[0], body[2]) > 40, "the ratings apart at a glance");

        int plain = 0xFF000000 | Math.round(255 * BookRibbon.SHADE[2]) << 16 | Math.round(168 * BookRibbon.SHADE[2]) << 8;
        check(r(body[1]) - b(body[1]) < r(plain) - b(plain), "greyed");
        int[] s = BookRibbon.shades(red);
        check(s.length == BookRibbon.AT.length && BookRibbon.AT[0] == 0 && BookRibbon.AT[BookRibbon.AT.length - 1] == 1, "stops");
        check(r(s[1]) > r(s[0]) && r(s[1]) > r(s[2]) && r(s[2]) > r(s[3]), "dark - light - dark across the band");
        check(BookRibbon.cloth(0xFF000000, 1) == 0xFF000000 && BookRibbon.cloth(0xFFFFFFFF, 2) == 0xFFFFFFFF, "clamped");

        float[][] grids = {{120, 420, 2280, 980}, {80, 360, 1360, 900}};
        for (float[] grid : grids) {
            float[] t = new float[4], p = new float[4], a = new float[2];
            BookLayout.tile(grid[0], grid[1], grid[2], grid[3], 10, 0, t);
            BookLayout.picture(t, 8, p);
            BookRibbon.sash(Math.min(t[2] - t[0], t[3] - t[1]), a);
            check(a[0] > 0 && a[1] > a[0], "a band");
            check(t[1] + a[1] < BookLayout.bandTop(t), "above the name band");
            check(t[2] - a[1] > (p[0] + p[2]) / 2 + (p[2] - p[0]) / 6, "clear of the picture's middle");
            check((a[1] - a[0]) / Math.sqrt(2) >= 12, "wide enough to see");
        }
        System.out.println("BookRibbonTest: cloth colours, shading and the sash's place passed");
    }
}
