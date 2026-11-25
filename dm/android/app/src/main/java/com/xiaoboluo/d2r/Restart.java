package com.xiaoboluo.d2r;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Process;
import android.os.SystemClock;
import java.io.File;



public final class Restart extends Activity {
    private static final String PID = "pid";

    static void start(Context c) {
        c.startActivity(new Intent(c, Restart.class).putExtra(PID, Process.myPid()).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_NO_ANIMATION));
    }

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        int pid = getIntent().getIntExtra(PID, -1);
        new Thread(() -> {
            for (int i = 0; i < 100 && pid > 0 && pid != Process.myPid() && new File("/proc/" + pid).exists(); i++) SystemClock.sleep(50);
            runOnUiThread(() -> {
                startActivity(new Intent(this, DmActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK));
                finish();
                overridePendingTransition(0, 0);
            });
        }, "dm-restart").start();
    }
}
