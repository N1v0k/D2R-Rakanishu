package com.xiaoboluo.d2r;


public class BatteryAskTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    public static void main(String[] args) {
        check(!BatteryAsk.shouldAsk(true, -1, 20211), "exempt: never ask");
        check(!BatteryAsk.shouldAsk(true, 20210, 20211), "exempt after an update: no ask");
        check(BatteryAsk.shouldAsk(false, -1, 20211), "fresh install, not exempt: ask");
        check(BatteryAsk.shouldAsk(false, -1, 20211), "update from a version that only stored 'asked': ask once (the exemption was lost by install -r)");
        check(!BatteryAsk.shouldAsk(false, 20211, 20211), "already asked in this version (denied or cancelled): not again");
        check(BatteryAsk.shouldAsk(false, 20211, 20212), "next update: ask once more");
        check(BatteryAsk.shouldAsk(false, 20212, 20211), "downgrade to another version: once for that version");
        System.out.println("BatteryAskTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
