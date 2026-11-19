package com.xiaoboluo.d2r;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.content.SharedPreferences;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Locale;








final class ExitLog {
    private ExitLog() {}

    private static final String[] REASONS = {"unknown", "exit_self", "signaled", "low_memory", "crash", "crash_native", "anr", "initialization_failure",
        "permission_change", "excessive_resource_usage", "user_requested", "user_stopped", "dependency_died", "other", "freezer", "package_state_change",
        "package_updated"};

    static String reason(int r) { return r >= 0 && r < REASONS.length ? REASONS[r] : "reason_" + r; }


    static void logOnce(Context context) {
        Context c = context.getApplicationContext();
        new Thread(() -> {
            try {
                SharedPreferences p = c.getSharedPreferences("dm_keepalive", Context.MODE_PRIVATE);
                long seen = p.getLong("exitSeen", 0), newest = seen;
                ActivityManager am = c.getSystemService(ActivityManager.class);
                List<ApplicationExitInfo> list = am.getHistoricalProcessExitReasons(c.getPackageName(), 0, 16);
                SimpleDateFormat f = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.ROOT);
                int n = 0;
                for (int i = list.size() - 1; i >= 0; i--) {
                    ApplicationExitInfo e = list.get(i);
                    if (e.getTimestamp() <= seen) continue;
                    String d = e.getDescription() == null ? "-" : e.getDescription().replaceAll("\\s+", " ");
                    if (d.length() > 200) d = d.substring(0, 200);
                    AppArt.log("keepalive: last exit " + f.format(new Date(e.getTimestamp())) + " pid=" + e.getPid() + " reason=" + reason(e.getReason())
                        + " status=" + e.getStatus() + " importance=" + e.getImportance() + " rss_kb=" + e.getRss() + " pss_kb=" + e.getPss() + " desc=" + d);
                    newest = Math.max(newest, e.getTimestamp());
                    n++;
                }
                if (newest != seen) p.edit().putLong("exitSeen", newest).apply();
                if (n == 0) AppArt.log("keepalive: no new exit records");
            } catch (RuntimeException e) { AppArt.log("keepalive: exit records unreadable " + e); }
        }, "dm-exitlog").start();
    }
}
