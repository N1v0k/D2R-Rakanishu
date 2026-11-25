package com.xiaoboluo.d2r;


public final class DpsLogTest {
    private static int checks;
    private static void check(boolean value, String what) { checks++; if (!value) throw new AssertionError(what); }

    static final String VIEW = "dps\t38\t9300\t4130\t2800\t6300\t5890\t4000\t8800\n"
        + "sim\t5200\t794\t14\n"
        + "src\ts\t84\t-\t20000\t4720\t3500\t6600\t610\n"
        + "src\tc\t289\tClayGolem\t5300\t960\t0\t0\t160\n"
        + "src\tu\t65522\t-\t700\t120\t0\t0\t20\n"
        + "seg\t0\t3000\t20\t2000\t1500\t2500\t0\t22\t0\n"
        + "seg\t1\t6000\t40\t3500\t2600\t4500\t1\t23\t0\n"
        + "seg\t2\t9300\t38\t4130\t2800\t6300\t9\t24\t0\n";

    public static void main(String[] args) {
        DpsLog d = DpsLog.parse(VIEW);
        check(d.fights == 38 && d.enough() && d.range() && Math.abs(d.own - 41.3) < 1e-9 && Math.abs(d.lo - 28) < 1e-9 && Math.abs(d.hi - 63) < 1e-9, "head");
        check(d.companions() && d.sim && Math.abs(d.reached - 79.4) < 1e-9 && d.simFights == 14 && Math.abs(d.seconds() - 372) < 1e-9, "companions and simulation");
        check(d.table() && d.sources.get(0).kind == 's' && d.sources.get(0).range() && !d.sources.get(1).range() && d.sources.get(2).kind == 'u' && Math.abs(d.sources.get(0).share - 61) < 1e-9, "sources");

        DpsLog few = DpsLog.parse("dps\t4\t500\t1000\t0\t0\t1000\t0\t0\nsrc\ts\t0\t-\t200\t1000\t0\t0\t1000\n");
        check(few.enough() && !few.range() && !few.table() && !few.companions() && !few.sim, "four fights");
        check(!DpsLog.parse("dps\t2\t100\t500\t0\t0\t500\t0\t0\n").enough(), "two fights: the grey line");

        check(Double.isNaN(d.change(0)) && Math.abs(d.change(2) - 18) < 1e-9 && Math.abs(d.change(1) - 75) < 1e-9, "change " + d.change(2));
        check((d.segments.get(2).changed & DpsLog.CHANGE_LEVEL) != 0 && (d.segments.get(2).changed & DpsLog.CHANGE_GEAR) != 0 && d.segments.get(2).level == 24, "what changed");

        DpsLog.Curve c = d.curve(0, 2, 0, 0, 1830, 600);
        check(c.stepX0[0] == 0 && Math.abs(c.stepX1[0] - 300) < 1e-3 && Math.abs(c.stepX0[2] - 900) < 1e-3 && Math.abs(c.stepX1[2] - 1830) < 1e-3, "steps by time");
        check(Float.isNaN(c.markX[0]) && c.markX[1] == c.stepX0[1] && c.markX[2] == c.stepX0[2], "markers");
        check(c.top >= 63 && c.stepY[2] < c.stepY[0] && !Float.isNaN(c.bandY0[2]) && c.bandY0[2] < c.stepY[2] && c.bandY1[2] > c.stepY[2] && !c.ticks.isEmpty(), "heights");
        for (String bad : new String[]{"sim\t1\t2\t3\n", "dps\t1\t2\n", "dps\t1\t1\t1\t5\t1\t1\t1\t1\n", VIEW + "seg\t7\t1\t1\t1\t1\t1\t0\t1\t0\n", VIEW + "src\tx\t1\t-\t1\t1\t1\t1\t1\n"}) {
            boolean thrown = false;
            try { DpsLog.parse(bad); } catch (IllegalArgumentException e) { thrown = true; }
            check(thrown, "rejected: " + bad.substring(0, Math.min(20, bad.length())));
        }
        System.out.println("DpsLogTest OK, checks=" + checks);
    }
}
