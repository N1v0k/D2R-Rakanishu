package com.xiaoboluo.d2r;








final class BatteryAsk {
    private BatteryAsk() {}



    static boolean shouldAsk(boolean exempt, long askedVersion, long currentVersion) {
        if (exempt) return false;
        if (askedVersion >= 0) return askedVersion != currentVersion;
        return true;
    }
}
