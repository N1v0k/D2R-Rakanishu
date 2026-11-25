package com.xiaoboluo.d2r;








final class KeepAliveStatus {
    static final int OK = 0, NO_NOTIFY = 1, BATTERY = 2, RESTRICTED = 3, FAILED = 4;

    static final int NOTE = 0xFFADA894, WARN = 0xFFFC4646;
    private KeepAliveStatus() {}

    static int of(boolean fgs, boolean notificationsVisible, boolean batteryExempt, boolean backgroundRestricted, boolean failed) {
        if (backgroundRestricted) return RESTRICTED;
        if (failed || !fgs) return FAILED;
        if (!batteryExempt) return BATTERY;
        if (!notificationsVisible) return NO_NOTIFY;
        return OK;
    }

    static boolean warning(int status) { return status >= BATTERY; }
    static int colour(int status) { return warning(status) ? WARN : NOTE; }


    static String name(int status) {
        switch (status) {
            case OK: return "ok";
            case NO_NOTIFY: return "notifications_off";
            case BATTERY: return "battery_optimised";
            case RESTRICTED: return "restricted";
            default: return "failed";
        }
    }
}
