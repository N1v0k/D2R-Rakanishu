package com.xiaoboluo.d2r;

import android.content.Context;
import android.net.Uri;
import android.view.View;
import java.io.File;
import java.util.List;

final class TestHooks {
    private TestHooks() {}
    static void overlay(String name, String state) {}
    static void overlay(View view, String name, String state) {}
    static void begin(View view, String page) {}
    static void text(android.graphics.Canvas c, CharSequence s, float x, float y, android.graphics.Paint p) {}
    static void text(android.graphics.Canvas c, char[] s, int off, int len, float x, float y, android.graphics.Paint p) {}
    static void end(View view) {}
    static void foreground() {}
    static void hit(android.graphics.RectF rect, int kind, int index) {  }
    static void pageBegin(View view, String page) {}
    static void pageText(android.graphics.Canvas c, CharSequence s, float x, float y, android.graphics.Paint p) {}
    static void pageHit(android.graphics.RectF rect, int kind, int index) {}
    static void pageEnd(View view) {}
    static String packContract(Context c) { return null; }
    static File packDataDir(Context c, File standard) { return standard; }
    static List<Uri> packPicks(Context c) { return null; }
    static void packHold(Context c, String unit) {}
    static String packDocsAuthority(Context c) { return null; }
    static List<String> packSteps(Context c, List<String> standard) { return standard; }
    static PackBuildJob.Setup packJob(Context c) { return null; }
    static long packFree(Context c, long real) { return real; }
    static int packThermal(Context c, int real) { return real; }
    static boolean packGameFiles(Context c) { return false; }
    static PackPlan packPlan(Context c, PackPlan real) { return real; }
    static File sharedRoot(Context c, File standard) { return standard; }
    static boolean sharedRootSet(Context c) { return false; }
    static boolean packCheckFail(Context c) { return false; }
    static PackSteps packStaged(Context c, File stage) { return null; }
}
