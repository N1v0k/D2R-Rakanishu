package com.xiaoboluo.d2r;


public class BatteryAskTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    public static void main(String[] args) {
        check(BatteryAsk.shouldAsk(false, false), "first run, not exempt: ask");
        check(!BatteryAsk.shouldAsk(true, false), "first run, already exempt: no ask");
        check(!BatteryAsk.shouldAsk(true, true), "allowed earlier and still exempt after an update: no ask");
        check(!BatteryAsk.shouldAsk(false, true), "denied earlier, then updated: not asked again (the pause screen offers it)");
        System.out.println("BatteryAskTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
