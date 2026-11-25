package com.xiaoboluo.d2r;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.Process;
import android.os.SystemClock;
import java.io.File;





















public class KeepAliveService extends Service {
    static final int FOREGROUND = 0, BACKGROUND_GAME = 1, BACKGROUND_MENU = 2;
    static final String CHANNEL = "dm_game";
    private static final int ID = 7;
    private static final String ACTION_SHOW = "com.xiaoboluo.d2r.keepalive.SHOW", ACTION_QUIT = "com.xiaoboluo.d2r.keepalive.QUIT";

    private static KeepAliveService instance;
    private static volatile boolean foreground;
    private static volatile String failure;
    private static boolean starting;
    private static volatile int state = FOREGROUND;
    private static volatile long backgroundSince;
    private static Icon icon;




    static void start(Activity a) {
        if (Quit.committed() || starting) return;
        state = FOREGROUND;
        backgroundSince = 0;
        try { a.startService(new Intent(a, KeepAliveService.class).setAction(ACTION_SHOW)); starting = true; }
        catch (RuntimeException e) { refused("start " + e); }
    }
    static boolean starting() { return starting; }



    static void update(Context c, int s) {
        int was = state;
        state = s;
        backgroundSince = s == FOREGROUND ? 0 : backgroundSince != 0 ? backgroundSince : SystemClock.uptimeMillis();
        if (!foreground || instance == null) return;
        post(c);
        if (s != was) { AppArt.log("keepalive: state " + name(s)); TestHooks.overlay("keepalive", name(s)); }
    }


    static void relabel(Context c) { if (foreground && instance != null) post(c); }

    static boolean foreground() { return foreground; }

    static boolean foreground(Context c) { boolean f = runningForeground(c); if (!f) foreground = false; return f; }
    static long backgroundMs() { long t = backgroundSince; return t == 0 ? 0 : SystemClock.uptimeMillis() - t; }


    static int status(Context c) {
        boolean fgs = runningForeground(c), failed = failure != null && !fgs;
        NotificationManager nm = c.getSystemService(NotificationManager.class);
        boolean shown = nm != null && nm.areNotificationsEnabled();
        if (shown) { NotificationChannel ch = nm.getNotificationChannel(CHANNEL); shown = ch == null || ch.getImportance() != NotificationManager.IMPORTANCE_NONE; }
        PowerManager pm = c.getSystemService(PowerManager.class);
        boolean exempt = pm == null || pm.isIgnoringBatteryOptimizations(c.getPackageName());
        ActivityManager am = c.getSystemService(ActivityManager.class);
        boolean restricted = am != null && am.isBackgroundRestricted();
        return KeepAliveStatus.of(fgs, shown, exempt, restricted, failed);
    }



    private static boolean runningForeground(Context c) {
        ActivityManager am = c.getSystemService(ActivityManager.class);
        try {
            if (am != null) for (ActivityManager.RunningServiceInfo r : am.getRunningServices(64))
                if (r.pid == Process.myPid() && KeepAliveService.class.getName().equals(r.service.getClassName())) return r.foreground;
            return false;
        } catch (RuntimeException e) { return foreground; }
    }


    static void stopNow() {
        KeepAliveService s = instance;
        foreground = false;
        if (s == null) return;
        s.stopForeground(STOP_FOREGROUND_REMOVE);
        s.stopSelf();
    }

    private static void refused(String why) {
        foreground = false;
        failure = why;
        AppArt.log("keepalive: promotion refused " + why);
        TestHooks.overlay("keepalive", "failed");
        DmActivity.keepAliveChanged();
    }

    static String name(int s) { return s == FOREGROUND ? "foreground" : s == BACKGROUND_GAME ? "background_game" : "background_menu"; }




    private static Uri quitData() { return Uri.parse("dmquit:" + Process.myPid() + "-" + Process.getStartElapsedRealtime()); }



    @Override public IBinder onBind(Intent intent) { return null; }

    @Override public int onStartCommand(Intent intent, int flags, int startId) {
        instance = this;
        if (intent != null && ACTION_QUIT.equals(intent.getAction())) {
            if (!quitData().equals(intent.getData())) {
                AppArt.log("keepalive: quit action from an earlier process ignored");
                if (!foreground) stopSelf(startId);
            } else Quit.request("notification", true);
            return START_NOT_STICKY;
        }
        starting = false;
        if (Quit.committed()) { stopSelf(); return START_NOT_STICKY; }
        promote();
        return START_NOT_STICKY;
    }

    private void promote() {
        try {
            Notification n = build(this);
            if (Build.VERSION.SDK_INT >= 34) startForeground(ID, n, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
            else startForeground(ID, n);
        } catch (RuntimeException e) {
            refused(e.toString());
            stopSelf();
            return;
        }
        boolean was = foreground;
        foreground = true;
        failure = null;
        if (!was) {
            AppArt.log("keepalive: foreground type=specialUse ok state=" + name(state) + " status=" + KeepAliveStatus.name(status(this)));
            DmActivity.keepAliveChanged();
        }
        TestHooks.overlay("keepalive", name(state));
    }

    @Override public void onTaskRemoved(Intent root) { Quit.request("task_removed", false); }




    @Override public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        AppArt.log("keepalive: trim level=" + level + " bg_ms=" + backgroundMs());
    }

    @Override public void onDestroy() {
        if (instance == this) instance = null;
        foreground = false;
        AppArt.log("keepalive: service stopped");
        super.onDestroy();
    }



    private static void post(Context c) {
        NotificationManager nm = c.getSystemService(NotificationManager.class);
        try { if (nm != null) nm.notify(ID, build(c)); } catch (RuntimeException e) { AppArt.log("keepalive: notification update failed " + e); }
    }




    private static void channel(Context c, NotificationManager nm) {
        NotificationChannel ch = new NotificationChannel(CHANNEL, AppText.get(c, R.string.dmAndroidKeepChannel), NotificationManager.IMPORTANCE_DEFAULT);
        ch.setDescription(AppText.get(c, R.string.dmAndroidKeepChannelInfo));
        ch.setSound(null, null);
        ch.enableVibration(false);
        ch.enableLights(false);
        ch.setShowBadge(false);
        ch.setLockscreenVisibility(Notification.VISIBILITY_PUBLIC);
        nm.createNotificationChannel(ch);
    }

    private static Notification build(Context c) {
        NotificationManager nm = c.getSystemService(NotificationManager.class);
        if (nm != null) channel(c, nm);
        int s = state;
        Intent open = new Intent(c, DmActivity.class).setAction(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
        Notification.Builder b = new Notification.Builder(c, CHANNEL)
            .setSmallIcon(icon(c))
            .setContentTitle(AppText.get(c, s == FOREGROUND ? R.string.dmAndroidKeepRunning : s == BACKGROUND_GAME ? R.string.dmAndroidKeepPaused : R.string.dmAndroidKeepBackground))
            .setContentText(AppText.get(c, s == FOREGROUND ? R.string.dmAndroidKeepStays : R.string.dmAndroidKeepTap))
            .setOngoing(true).setOnlyAlertOnce(true).setShowWhen(false).setLocalOnly(true)
            .setCategory(Notification.CATEGORY_SERVICE).setVisibility(Notification.VISIBILITY_PUBLIC)
            .setForegroundServiceBehavior(Notification.FOREGROUND_SERVICE_IMMEDIATE)
            .setContentIntent(PendingIntent.getActivity(c, 0, open, PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT));
        if (s != FOREGROUND) {
            Intent quit = new Intent(ACTION_QUIT, quitData()).setClass(c, KeepAliveService.class);
            PendingIntent pq = PendingIntent.getService(c, 1, quit, PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);
            b.addAction(new Notification.Action.Builder(icon(c), AppText.get(c, R.string.dmAndroidKeepQuit), pq).build());
        }
        return b.build();
    }





    private static Icon icon(Context c) {
        if (icon != null) return icon;
        Bitmap src = BitmapFactory.decodeFile(new File(c.getFilesDir(), "dm/appart/menu_icon.png").getAbsolutePath());
        if (src != null && src.getWidth() >= 24 && src.getHeight() >= 24) {
            int w = src.getWidth(), h = src.getHeight(), m = w * 6 / 36, set = 0;
            int[] px = new int[w * h];
            src.getPixels(px, 0, w, 0, 0, w, h);
            for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
                int p = px[y * w + x], r = p >> 16 & 255, g = p >> 8 & 255, bl = p & 255;
                boolean on = x >= m && y >= m && x < w - m && y < h - m && (p >>> 24) > 128 && (r * 299 + g * 587 + bl * 114) / 1000 > 55 && r - bl > 25;
                px[y * w + x] = on ? 0xFFFFFFFF : 0;
                if (on) set++;
            }
            if (set >= w * h / 20) icon = Icon.createWithBitmap(Bitmap.createScaledBitmap(Bitmap.createBitmap(px, w, h, Bitmap.Config.ARGB_8888), 72, 72, true));
        }
        if (icon == null) icon = Icon.createWithResource(c, android.R.drawable.ic_media_pause);
        return icon;
    }
}
