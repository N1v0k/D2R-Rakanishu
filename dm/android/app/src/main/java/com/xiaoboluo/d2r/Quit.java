package com.xiaoboluo.d2r;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.os.SystemClock;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;











final class Quit {
    static final int NONE = 0, WAITING = 1, COMMITTED = 2;
    private static final Object lock = new Object();
    private static int state = NONE, token;
    private Quit() {}

    static boolean committed() { synchronized (lock) { return state == COMMITTED; } }



    static void request(String reason, boolean fromAction) {
        int my;
        synchronized (lock) {
            if (state != NONE) { AppArt.log("keepalive: quit reason=" + reason + " ignored, already " + (state == WAITING ? "waiting" : "committed")); return; }
            if (fromAction && DmActivity.resumedNow()) { AppArt.log("keepalive: quit reason=" + reason + " ignored, the game is in the foreground"); return; }
            state = WAITING;
            my = ++token;
        }
        AppArt.log("keepalive: quit reason=" + reason + " waiting for the save");
        Handler main = new Handler(Looper.getMainLooper());
        new Thread(() -> run(main, reason, fromAction, my, null, null), "dm-quit").start();
    }



    static void restart(Context context, String reason, Runnable failed) {
        int my;
        synchronized (lock) {
            if (state != NONE) { AppArt.log("keepalive: restart reason=" + reason + " ignored, already " + (state == WAITING ? "waiting" : "committed")); failed.run(); return; }
            state = WAITING;
            my = ++token;
        }
        AppArt.log("keepalive: restart reason=" + reason + " waiting for the save");
        Handler main = new Handler(Looper.getMainLooper());
        Context app = context.getApplicationContext();
        new Thread(() -> run(main, reason, false, my, app, failed), "dm-quit").start();
    }


    static void cancelIfWaiting(String why) {
        synchronized (lock) {
            if (state != WAITING) return;
            state = NONE;
            token++;
        }
        AppArt.log("keepalive: quit cancelled (" + why + ")");
    }

    private static boolean still(int my) { synchronized (lock) { return state == WAITING && token == my; } }

    private static void run(Handler main, String reason, boolean fromAction, int my, Context restart, Runnable failed) {
        int result = 0;
        if (DmActivity.hostStarted()) {
            long[] st = new long[3];
            if (restart != null) DmActivity.nativeLeave();
            DmActivity.nativeLeaveState(st);
            if (st[0] == 0) { DmActivity.nativeLeave(); DmActivity.nativeLeaveState(st); }
            long target = st[0], t0 = SystemClock.uptimeMillis(), told = t0;
            boolean retried = false;
            for (;;) {
                if (!still(my)) { if (failed != null) failed.run(); return; }
                DmActivity.nativeLeaveState(st);
                int d = QuitRule.decide(target, st[1], (int) st[2], retried);
                if (d == QuitRule.COMMIT) { result = (int) st[2]; break; }
                if (d == QuitRule.RETRY) {
                    retried = true;
                    AppArt.log("keepalive: quit save=" + st[2] + ", saving once more");
                    DmActivity.nativeLeave();
                    DmActivity.nativeLeaveState(st);
                    target = st[0];
                    continue;
                }
                if (d == QuitRule.ABORT) {
                    synchronized (lock) { if (state == WAITING && token == my) state = NONE; }
                    AppArt.log("keepalive: quit aborted save=" + st[2] + " (the game stays paused in memory)");
                    if (failed != null) failed.run();
                    return;
                }
                long now = SystemClock.uptimeMillis();
                if (now - told >= 10000) { told = now; AppArt.log("keepalive: quit still waiting for the save, " + (now - t0) / 1000 + " s"); }
                SystemClock.sleep(100);
            }
        }
        synchronized (lock) {
            if (state != WAITING || token != my) return;
            state = COMMITTED;
        }
        AppArt.log("keepalive: quit reason=" + reason + " committed save=" + result + (DmActivity.hostStarted() ? "" : " (no host)"));
        if (fromAction && DmActivity.hostStarted()) DmActivity.nativeResumeDrop();
        CountDownLatch done = new CountDownLatch(1);
        main.post(() -> {
            try {
                if (restart != null) Restart.start(restart);
                DmActivity a = DmActivity.currentActivity();
                if (a != null && !a.isFinishing()) { if (fromAction) a.finishAndRemoveTask(); else a.finish(); }
                KeepAliveService.stopNow();
            } finally { done.countDown(); }
        });
        try { done.await(2, TimeUnit.SECONDS); } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        if (DmActivity.hostStarted()) DmActivity.nativeShutdown();
        AppArt.log("keepalive: quit without host, ending the process");
        Process.killProcess(Process.myPid());
    }
}
