package com.xiaoboluo.d2r;

import java.util.Map;



public class PackPlanTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    public static void main(String[] args) {
        long plenty = 6L << 30, little = PackPlan.LANES_MIN - 1;

        Map<Integer, int[]> table = Map.of(8, new int[]{3, 2}, 6, new int[]{2, 1}, 4, new int[]{1, 1}, 2, new int[]{1, 1});
        for (Map.Entry<Integer, int[]> e : table.entrySet()) {
            int n = e.getKey();
            PackPlan p = PackPlan.decide(n, plenty);
            check(p.tokens == n && p.shaderWorkers == e.getValue()[0] && p.pictureThreads == e.getValue()[1] && p.mode.equals(PackPlan.LANES), n + " cores: " + p);
            int python = n - (p.shaderWorkers + 1) - p.pictureThreads;
            check(n == 2 ? python == -1 : python >= 1, n + " cores: shaders and pictures leave " + python + " tokens for the Python steps");
        }
        check(PackPlan.decide(16, plenty).shaderWorkers == 4, "more than 4 shader workers do not help: " + PackPlan.decide(16, plenty));
        check(PackPlan.decide(1, plenty).tokens == 1 && PackPlan.decide(0, plenty).tokens == 1, "one core at least");
        PackPlan serial = PackPlan.decide(8, little);
        check(serial.mode.equals(PackPlan.SERIAL) && serial.shaderWorkers == 4 && serial.pictureThreads == 7 && serial.tokens == 8, "low memory: one step at a time: " + serial);
        check(PackPlan.decide(8, PackPlan.LANES_MIN).mode.equals(PackPlan.LANES), "at the line the lanes run");
        check(PackPlan.decide(2, little).shaderWorkers == 1 && PackPlan.decide(2, little).pictureThreads == 1, "two cores in serial: " + PackPlan.decide(2, little));
        try { new PackPlan(8, 0, 1, PackPlan.LANES); check(false, "no shader worker is refused"); } catch (IllegalArgumentException ok) { check(true, ""); }
        try { new PackPlan(8, 1, 1, "fast"); check(false, "an unknown mode is refused"); } catch (IllegalArgumentException ok) { check(true, ""); }
        System.out.println("PackPlanTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
