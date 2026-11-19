package com.xiaoboluo.d2r;

import java.util.List;



public class BestGearPagesTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }


    static boolean covers(List<BestGearPages.Page> p, int rows, int lines, float room, float rowH, float headH, float lineH) {
        int r = 0, l = 0, headings = 0;
        for (BestGearPages.Page g : p) {
            if (g.rowFrom != r || g.lineFrom != l || g.rowTo < g.rowFrom || g.lineTo < g.lineFrom) return false;
            if (g.rowTo == g.rowFrom && g.lineTo == g.lineFrom && !(rows == 0 && lines == 0)) return false;
            if (g.lineTo > g.lineFrom && g.rowTo < rows) return false;
            if (g.heading && (g.lineFrom != 0 || g.lineTo == 0)) return false;
            float y = (g.rowTo - g.rowFrom) * rowH + (g.heading ? headH : 0) + (g.lineTo - g.lineFrom) * lineH;
            int entries = (g.rowTo - g.rowFrom) + (g.lineTo - g.lineFrom);
            if (y > room + 1e-3f && !(entries == 1 || g.heading && entries == 1 + (g.rowTo - g.rowFrom) && g.lineTo - g.lineFrom == 1)) return false;
            r = g.rowTo; l = g.lineTo; headings += g.heading ? 1 : 0;
        }
        return r == rows && l == lines && headings == (lines > 0 ? 1 : 0);
    }

    public static void main(String[] a) {

        List<BestGearPages.Page> ten = BestGearPages.split(10, 6, 640, 88, 44, 36);
        check(covers(ten, 10, 6, 640, 88, 44, 36) && ten.size() == 2, "ten places and six totals lines: two pages, all reachable (" + ten.size() + ")");
        check(ten.get(0).rowTo == 7 && ten.get(1).rowFrom == 7 && ten.get(1).heading, "seven rows on the first page, the rest with the heading on the second");
        List<BestGearPages.Page> one = BestGearPages.split(1, 4, 640, 88, 44, 36);
        check(one.size() == 1 && one.get(0).rowTo == 1 && one.get(0).lineTo == 4 && one.get(0).heading, "one place and its totals on one page");
        check(BestGearPages.split(0, 0, 640, 88, 44, 36).size() == 1, "nothing to show: one empty page");

        List<BestGearPages.Page> edge = BestGearPages.split(2, 3, 2 * 88 + 44 + 10, 88, 44, 36);
        check(covers(edge, 2, 3, 2 * 88 + 44 + 10, 88, 44, 36) && !edge.get(0).heading && edge.get(1).heading, "the heading goes with its first line");

        List<BestGearPages.Page> tiny = BestGearPages.split(3, 2, 20, 88, 44, 36);
        check(covers(tiny, 3, 2, 20, 88, 44, 36) && tiny.size() == 5, "a room smaller than a row: one entry per page (" + tiny.size() + ")");
        int bad = 0;
        java.util.Random rnd = new java.util.Random(7);
        for (int i = 0; i < 2000; i++) {
            int rows = rnd.nextInt(11), lines = rnd.nextInt(13); float room = 40 + rnd.nextInt(900);
            if (!covers(BestGearPages.split(rows, lines, room, 88, 44, 36), rows, lines, room, 88, 44, 36)) bad++;
        }
        check(bad == 0, "2000 random splits cover every entry once within the room (" + bad + " bad)");
        if (fails > 0) { System.out.println(fails + " failed"); System.exit(1); }
        System.out.println("BestGearPagesTest passed");
    }
}
