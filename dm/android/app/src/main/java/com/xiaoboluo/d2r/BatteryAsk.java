package com.xiaoboluo.d2r;





final class BatteryAsk {
    private BatteryAsk() {}

    static boolean shouldAsk(boolean exempt, boolean askedBefore) {
        return !exempt && !askedBefore;
    }
}
