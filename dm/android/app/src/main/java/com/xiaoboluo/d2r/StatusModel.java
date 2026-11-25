package com.xiaoboluo.d2r;













final class StatusModel {
    interface Receiver { boolean register(); void unregister(); }

    private boolean registered, failed;
    static final int BATTERY_NONE = 0, BATTERY_PLAIN = 1, BATTERY_CHARGING = 2;
    String clock = "";
    int percent = -1, fps = -1;
    boolean plugged, low;

    boolean registered() { return registered; }


    boolean reconcile(boolean running, boolean master, boolean clockLine, boolean batteryLine, Receiver r) {
        boolean want = running && master && (clockLine || batteryLine);
        if (!want) { failed = false; if (registered) { registered = false; r.unregister(); } }
        else if (!registered && !failed) { registered = r.register(); failed = !registered; }
        return registered;
    }


    boolean clock(int hour, int minute) {
        String text = (hour < 10 ? "0" : "") + hour + (minute < 10 ? ":0" : ":") + minute;
        if (text.equals(clock)) return false;
        clock = text;
        return true;
    }


    boolean battery(int level, int scale, int pluggedExtra, boolean lowExtra) {
        int p = level >= 0 && scale > 0 ? Math.min(100, Math.round(level * 100f / scale)) : -1;
        boolean on = p >= 0 && pluggedExtra != 0, weak = p >= 0 && lowExtra;
        if (p == percent && on == plugged && weak == low) return false;
        percent = p; plugged = on; low = weak;
        return true;
    }



    int batteryKind() { return percent < 0 ? BATTERY_NONE : plugged ? BATTERY_CHARGING : BATTERY_PLAIN; }


    boolean fps(int value) {
        int v = value < 0 ? -1 : value;
        if (v == fps) return false;
        fps = v;
        return true;
    }
}
