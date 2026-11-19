package com.xiaoboluo.d2r;

import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.Rect;
import java.util.HashMap;
import android.graphics.Typeface;












final class GameText {
    private final GameStyle style;
    private final GameStyleData.Preset preset;
    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG), shade = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final float units, floor;
    private final float kerning;
    private final GameStyleData.Shadow shadow;
    private int generation = -1, colour, shadowColour;
    private boolean ready;
    private float px = -1, dx, dy;

    GameText(GameStyle style, GameStyleData.Preset preset) {
        this.style = style;
        this.preset = preset;
        GameStyleData d = style.data();
        units = preset.size != null ? d.size(preset.size) : preset.units;
        floor = d.size("SmallFontSize");
        kerning = d.spacing(preset.spacing);
        shadow = d.shadow(preset.shadow);
        colour = style.colour(preset.colour);
        shadowColour = shadow == null ? 0 : shadow.argb;
        shade.setStyle(Paint.Style.FILL);
        fill.setStyle(Paint.Style.FILL);
    }


    boolean ready() { sync(); return ready; }



    void prepare() { sync(); }

    private final HashMap<String, float[]> tabInk = new HashMap<>();


    void prepareTabInk(String label, int minimumSize, int maximumSize) {
        sync();
        minimumSize = Math.max(1, minimumSize);
        if (!ready || label == null || label.isEmpty() || maximumSize < minimumSize) return;
        float[] previous = tabInk.get(label);
        if (previous != null && previous.length > maximumSize && !Float.isNaN(previous[minimumSize])) return;
        float[] offsets = new float[maximumSize + 1];
        java.util.Arrays.fill(offsets, Float.NaN);
        Paint metrics = new Paint(fill);
        metrics.setColor(0xffffffff);
        metrics.setTextAlign(Paint.Align.LEFT);
        Rect bounds = new Rect();
        metrics.setTextSize(maximumSize);
        metrics.getTextBounds(label, 0, label.length(), bounds);
        int width = Math.max(1, (int) Math.ceil(metrics.measureText(label)) + 8);
        int height = Math.max(1, bounds.height() + 8);
        Bitmap ink = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(ink);
        int[] raster = new int[width * height];
        for (int pixels = minimumSize; pixels <= maximumSize; pixels++) {
            metrics.setTextSize(pixels);
            metrics.getTextBounds(label, 0, label.length(), bounds);
            int baseline = 4 - bounds.top;
            ink.eraseColor(0);
            canvas.drawText(label, 4, baseline, metrics);
            ink.getPixels(raster, 0, width, 0, 0, width, height);

            int first = 0, last = height - 1;
            while (first < height && !opaqueRow(raster, width, first)) first++;
            while (last > first && !opaqueRow(raster, width, last)) last--;
            offsets[pixels] = first == height ? 0 : baseline - (first + last) / 2f - pixels * GameStyleData.CAPS_EM / 2;
        }
        ink.recycle();
        tabInk.put(label, offsets);
    }

    private static boolean opaqueRow(int[] raster, int width, int y) {
        for (int x = y * width, end = x + width; x < end; x++) if ((raster[x] >>> 24) >= 204) return true;
        return false;
    }

    float tabInkOffset(String label, float k) {
        float[] offsets = tabInk.get(label);
        int pixels = Math.round(size(k));
        return offsets == null || pixels >= offsets.length || Float.isNaN(offsets[pixels]) ? 0 : offsets[pixels];
    }

    float units() { return units; }



    float shadeRatio() { return shadow == null || !(units > 0) ? 0 : Math.max(0, Math.max(shadow.dx, shadow.dy)) / units; }


    GameText colour(String name) { colour = style.colour(name); return this; }
    GameText colourArgb(int argb) { colour = argb; return this; }
    int colour() { return colour; }


    float size(float k) { return units * k; }

    private void sync() {
        int g = style.generation();
        if (g == generation) return;
        generation = g;
        tabInk.clear();
        Typeface t = style.face(preset.face);
        ready = t != null && units > 0;
        if (t == null) return;
        fill.setTypeface(t); shade.setTypeface(t);
        fill.setLetterSpacing(0);
        float em = 0;
        if (kerning != 1) { fill.setTextSize(100); em = fill.measureText("0123456789") / 10 / 100; }
        float spacing = GameStyleGeometry.letterSpacing(kerning, em);
        fill.setLetterSpacing(spacing); shade.setLetterSpacing(spacing);
        px = -1;
    }



    private void apply(float pixels, Paint.Align align) {
        if (pixels != px) {
            px = pixels;
            fill.setTextSize(pixels); shade.setTextSize(pixels);
            float k = units > 0 ? pixels / units : 0;
            dx = shadow == null ? 0 : GameStyleGeometry.shadow(shadow.dx, k);
            dy = shadow == null ? 0 : GameStyleGeometry.shadow(shadow.dy, k);
        }
        if (fill.getTextAlign() != align) { fill.setTextAlign(align); shade.setTextAlign(align); }
    }


    Paint paint(float k) { return paintPx(units * k); }
    Paint paintPx(float pixels) { sync(); apply(pixels, fill.getTextAlign()); return fill; }



    Paint solid(float pixels, int alpha) { sync(); apply(pixels, fill.getTextAlign()); alpha(alpha); return fill; }

    float measure(CharSequence s, float k) { sync(); if (!ready) return 0; apply(units * k, fill.getTextAlign()); return fill.measureText(s, 0, s.length()); }
    float measure(char[] buf, int off, int len, float k) { sync(); if (!ready) return 0; apply(units * k, fill.getTextAlign()); return fill.measureText(buf, off, len); }
    float measurePx(char[] buf, int off, int len, float pixels) { sync(); if (!ready) return 0; apply(pixels, fill.getTextAlign()); return fill.measureText(buf, off, len); }


    float fitTo(CharSequence s, float max, float k) { return GameStyleGeometry.fit(measure(s, k), max, k, units, floor); }

    float floorK(float k) { return units > floor ? k * floor / units : k; }

    void draw(Canvas c, CharSequence s, float x, float baseline, float k, Paint.Align align, int alpha) {
        sync();
        if (!ready || s.length() == 0) return;
        apply(units * k, align);
        alpha(alpha);
        if (shadowColour != 0) c.drawText(s, 0, s.length(), x + dx, baseline + dy, shade);
        c.drawText(s, 0, s.length(), x, baseline, fill);
        if (BuildConfig.DM_QA_BUILD) TestHooks.text(c, s, x, baseline, fill);
    }

    void draw(Canvas c, char[] buf, int off, int len, float x, float baseline, float k, Paint.Align align, int alpha) { drawPx(c, buf, off, len, x, baseline, units * k, align, alpha); }

    void drawPx(Canvas c, char[] buf, int off, int len, float x, float baseline, float pixels, Paint.Align align, int alpha) {
        sync();
        if (!ready || len <= 0) return;
        apply(pixels, align);
        alpha(alpha);
        if (shadowColour != 0) c.drawText(buf, off, len, x + dx, baseline + dy, shade);
        c.drawText(buf, off, len, x, baseline, fill);
        if (BuildConfig.DM_QA_BUILD) TestHooks.text(c, buf, off, len, x, baseline, fill);
    }


    private void alpha(int alpha) {
        int a = Math.max(0, Math.min(255, alpha));
        fill.setColor(colour & 0x00FFFFFF | ((colour >>> 24) * a / 255) << 24);
        shade.setColor(shadowColour & 0x00FFFFFF | ((shadowColour >>> 24) * a / 255 * (colour >>> 24) / 255) << 24);
    }
}
