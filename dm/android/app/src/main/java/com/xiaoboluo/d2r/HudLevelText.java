package com.xiaoboluo.d2r;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.SystemClock;













final class HudLevelText {
    private static final float FIT = 0.8f;
    private GameText text;
    private String word, fallbackWord, format;
    private long askedAt = -1;


    void style(GameStyle s) { text = s.text(GameStyleData.HUD_TEXT); text.prepare(); askedAt = -1; }



    void words(String format, String fallbackWord) { this.format = format; this.fallbackWord = fallbackWord; }


    void reload() { askedAt = -1; }

    boolean ready() { return text != null && text.ready(); }




    boolean refresh() {
        if (!ready()) return false;
        long now = SystemClock.uptimeMillis();
        if (askedAt >= 0 && now - askedAt < 1000 && now >= askedAt) return false;
        askedAt = now;
        byte[] w = null;
        try { w = DmActivity.nativeLevelWord(); } catch (UnsatisfiedLinkError e) {                                                }
        String was = word; word = LevelLabel.word(w);
        return word == null ? was != null : !word.equals(was);
    }



    boolean draw(Canvas c, int level, int percent, float x, float baseline, float barWidth, float k) {
        if (!ready()) return false;
        String s = LevelLabel.text(word, fallbackWord, format, level, percent);
        text.draw(c, s, x, baseline, text.fitTo(s, barWidth * FIT, k), Paint.Align.CENTER, 255);
        return true;
    }
}
