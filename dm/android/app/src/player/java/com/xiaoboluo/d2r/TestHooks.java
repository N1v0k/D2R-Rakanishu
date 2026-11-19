package com.xiaoboluo.d2r;

import android.view.View;

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
}
