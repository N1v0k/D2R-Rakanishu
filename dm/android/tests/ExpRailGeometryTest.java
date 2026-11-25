package com.xiaoboluo.d2r;

import java.util.Arrays;





public class ExpRailGeometryTest {
    static int failed;

    static void check(String what, Object got, Object want) {
        boolean ok = String.valueOf(got).equals(String.valueOf(want));
        if (!ok) failed++;
        System.out.println((ok ? "ok   " : "FAIL ") + what + ": " + got + (ok ? "" : " (want " + want + ")"));
    }


    static int[] edges() {
        int[] w = new int[ExpRailGeometry.PIECES];
        for (int i = 0; i < w.length; i++) w[i] = i == 0 || i == 20 ? 20 : i % 2 == 0 ? 8 : 88;
        w[1] = 86; w[19] = 85;
        int[] e = new int[w.length + 1];
        for (int i = 0; i < w.length; i++) e[i + 1] = e[i] + w[i];
        return e;
    }

    public static void main(String[] args) {
        int[] e = edges();
        check("source width", e[ExpRailGeometry.PIECES], 987);
        check("valid edges", ExpRailGeometry.validEdges(e, 987), true);
        check("wrong width", ExpRailGeometry.validEdges(e, 988), false);
        check("too few edges", ExpRailGeometry.validEdges(Arrays.copyOf(e, 21), 987), false);
        int[] bad = e.clone(); bad[5] = bad[4];
        check("not increasing", ExpRailGeometry.validEdges(bad, 987), false);


        for (float[] dev : new float[][] {{604, 1116}, {657, 1216}, {845, 1600}, {300, 540}}) {
            int width = (int) dev[0]; float px = .95f * dev[1] / 1320;
            int[] t = ExpRailGeometry.target(e, width, px);
            String at = " @" + width;
            check("fits" + at, t != null, true);
            check("starts at 0" + at, t[0], 0);
            check("ends at the width" + at, t[ExpRailGeometry.PIECES], width);
            int studW = t[3] - t[2]; boolean sameStuds = true, sameCaps = t[1] - t[0] == t[21] - t[20];
            for (int i = 2; i < 20; i += 2) sameStuds &= t[i + 1] - t[i] == studW;
            check("studs all " + Math.round(8 * px) + " px" + at, sameStuds && studW == Math.round(8 * px), true);
            check("caps equal" + at, sameCaps, true);

            int fixed = 0; for (int i = 0; i < 21; i += 2) fixed += t[i + 1] - t[i];
            boolean near = true; for (int i = 1; i < 21; i += 2) near &= Math.abs((t[i + 1] - t[i]) - (width - fixed) * (e[i + 1] - e[i]) / 875.0) < 1;
            check("gutters proportional" + at, near, true);

            check("0 permille" + at, ExpRailGeometry.clip(t, 0), 0.0f);
            check("negative" + at, ExpRailGeometry.clip(t, -5), 0.0f);
            check("100 permille = end of gutter 0" + at, ExpRailGeometry.clip(t, 100), (float) t[2]);
            check("101 permille just past stud 0" + at, ExpRailGeometry.clip(t, 101) > t[3] && ExpRailGeometry.clip(t, 101) < t[3] + 1, true);
            check("500 permille = end of gutter 4" + at, ExpRailGeometry.clip(t, 500), (float) t[10]);
            check("450 permille = middle of gutter 4" + at, ExpRailGeometry.clip(t, 450), t[9] + (t[10] - t[9]) / 2f);
            check("999 permille inside gutter 9" + at, ExpRailGeometry.clip(t, 999) < t[20] && ExpRailGeometry.clip(t, 999) > t[19], true);
            check("1000 permille = all" + at, ExpRailGeometry.clip(t, 1000), (float) width);
            check("over 1000 = all" + at, ExpRailGeometry.clip(t, 1400), (float) width);
            boolean monotonic = true; float last = -1;
            for (int p = 0; p <= 1000; p++) { float c = ExpRailGeometry.clip(t, p); monotonic &= c >= last; last = c; }
            check("clip never goes back" + at, monotonic, true);
        }
        check("too narrow -> flat bar", ExpRailGeometry.target(e, 90, .8f), null);
        check("no scale -> flat bar", ExpRailGeometry.target(e, 604, 0), null);
        check("studs under 1 px -> flat bar", ExpRailGeometry.target(e, 604, .05f), null);
        check("NaN scale -> flat bar", ExpRailGeometry.target(e, 604, Float.NaN), null);
        check("no edges -> flat bar", ExpRailGeometry.target(null, 604, .8f), null);
        check("no target -> nothing clipped", ExpRailGeometry.clip(null, 500), 0.0f);
        System.out.println(failed == 0 ? "ALL OK" : failed + " FAILED");
        System.exit(failed == 0 ? 0 : 1);
    }
}
