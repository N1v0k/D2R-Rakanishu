






package com.xiaoboluo.d2r;

import android.view.KeyEvent;

interface PadPage {
    boolean padOpen();
    void onPad(int cmd);
    void back();


    default boolean takesKey(KeyEvent e) { return false; }

    default int backdrop() { return GameStyleGeometry.Panel.BACKDROP; }
}
