package com.xiaoboluo.d2r;

import java.util.Arrays;






public class OverviewFlowTest {
    static int failed;

    static void check(String what, Object got, Object want) {
        boolean ok = String.valueOf(got).equals(String.valueOf(want));
        if (!ok) failed++;
        System.out.println((ok ? "ok   " : "FAIL ") + what + ": " + got + (ok ? "" : " (want " + want + ")"));
    }

    static String place(float[] h, int[] s, boolean[] k, float first, float rest) { return Arrays.toString(OverviewFlow.place(h, s, k, first, rest)); }

    public static void main(String[] a) {


        float[] h = { 300, 100, 50, 100, 100, 70, 100, 100, 100, 50, 60, 100, 60, 100, 50, 50 };
        int[] s = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3 };
        boolean[] k = new boolean[h.length]; k[5] = true; k[10] = true; k[12] = true;
        int[] col = OverviewFlow.place(h, s, k, 520, 780);
        check("the rest of the weaknesses moves to the next column, phases and notes follow", Arrays.toString(col), "[0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4]");
        check("five columns = three pages", OverviewFlow.pages(col), 3);
        boolean all = true; for (int c : col) all &= c >= 0;
        check("every entry has a column", all, true);


        int[] two = OverviewFlow.place(new float[] { 300, 100, 70, 100, 160, 50 }, new int[] { 0, 1, 1, 1, 2, 3 }, new boolean[] { false, false, true, false, false, false }, 520, 780);
        check("everything fits: summary | strengths, phases | notes", Arrays.toString(two), "[0, 1, 1, 1, 2, 3]");
        check("two pages", OverviewFlow.pages(two), 2);


        check("heading moves with its entry", place(new float[] { 100, 400, 70, 100 }, new int[] { 0, 1, 1, 1 }, new boolean[] { false, false, true, false }, 520, 780), "[0, 1, 2, 2]");

        check("heading at the end of its section", place(new float[] { 100, 400, 70, 100 }, new int[] { 0, 1, 1, 2 }, new boolean[] { false, false, true, false }, 520, 780), "[0, 1, 1, 2]");

        check("too tall entry alone", place(new float[] { 100, 600, 900, 50 }, new int[] { 0, 1, 1, 1 }, new boolean[4], 520, 780), "[0, 1, 2, 3]");

        check("later columns are taller", place(new float[] { 100, 500, 500, 250, 100 }, new int[] { 0, 1, 1, 1, 1 }, new boolean[5], 520, 780), "[0, 1, 2, 2, 3]");
        check("no entries: one page", OverviewFlow.pages(new int[0]), 1);

        if (failed > 0) { System.out.println("OverviewFlowTest: " + failed + " failed"); System.exit(1); }
        System.out.println("OverviewFlowTest: all passed");
    }
}
