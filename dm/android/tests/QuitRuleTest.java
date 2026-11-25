package com.xiaoboluo.d2r;



public class QuitRuleTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    public static void main(String[] a) {
        check(QuitRule.decide(3, 2, 1, false) == QuitRule.WAIT, "the save of the last leave is not done yet -> wait");
        check(QuitRule.decide(3, 2, -1, true) == QuitRule.WAIT, "an older result never decides");
        check(QuitRule.decide(3, 3, 1, false) == QuitRule.COMMIT, "saved -> quit");
        check(QuitRule.decide(3, 4, 0, false) == QuitRule.COMMIT, "nothing to save (newer leave covered) -> quit");
        check(QuitRule.decide(3, 3, -1, false) == QuitRule.RETRY, "no commit in time -> one more save");
        check(QuitRule.decide(4, 4, -1, true) == QuitRule.ABORT, "no commit after the second save -> stay");
        check(QuitRule.decide(3, 3, -2, false) == QuitRule.ABORT, "recipe book busy -> stay");
        check(QuitRule.decide(0, 0, 0, false) == QuitRule.COMMIT, "no leave ever issued and none needed -> quit");
        check(QuitRule.clean(1) && QuitRule.clean(0) && !QuitRule.clean(-1) && !QuitRule.clean(-2), "only saved / nothing to save are clean");
        System.out.println(fails == 0 ? "QuitRuleTest: all passed" : "QuitRuleTest: " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
