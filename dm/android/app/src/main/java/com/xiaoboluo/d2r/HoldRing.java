















package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.SystemClock;
import android.view.View;

final class HoldRing extends View {


    static final float RING_DY = 98f, RING_D = 156f;



    static final float PANEL_UNITS = 3322f, EDGE = 0.012f, MIN_FIT = 0.75f;
    private static final long HINT_MS = 8000, FADE_MS = 300, HINT_CHECK_MS = 1000;
    private static final String PREFS = "dm_pad", LEARNED = "hold_learned";

    private final AppArt art;
    private final GameStyle style;
    private final GameText legend;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final RectF bar = new RectF(), dst = new RectF();
    private final Rect src = new Rect();
    private final int[] ui = new int[6];
    private float progress = -1;
    private long hintAt = -1;
    private boolean hintDone;
    private boolean checking;
    private final GameLang.Watch lang = new GameLang.Watch();

    HoldRing(Context context, AppArt art, GameStyle style) {
        super(context);
        this.art = art;
        this.style = style;
        legend = style.text(GameStyleData.LEGEND);
        legend.prepare();
    }

    private static boolean hudShown() { return (DmActivity.nativeDisplayState() & 1) != 0; }

    private SharedPreferences prefs() { return getContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE); }
    private boolean learned() { return prefs().getBoolean(LEARNED, false); }


    void setProgress(float p) {
        if (p < 0 && progress < 0) return;
        progress = p;
        invalidate();
    }


    void learnedNow() {
        if (!learned()) prefs().edit().putBoolean(LEARNED, true).apply();
        hintAt = -1;
        invalidate();
    }


    void watch() {
        if (checking || hintDone || !PadInput.isPadMode() || learned()) return;
        checking = true;
        postDelayed(hintCheck, HINT_CHECK_MS);
    }

    void stop() { removeCallbacks(hintCheck); checking = false; if (hintAt >= 0) { hintAt = -1; hintDone = false; } progress = -1; invalidate(); }

    private final Runnable hintCheck = () -> {
        checking = false;
        if (hintDone || !PadInput.isPadMode() || learned()) return;
        if (DmActivity.hostStarted() && DmActivity.nativeInGame() != 0 && !DmActivity.padPageOpen() && DmActivity.nativePadBusy() == 0 && hudShown()) {
            hintDone = true; hintAt = SystemClock.uptimeMillis(); invalidate(); return;
        }
        watch();
    };

    @Override protected void onDraw(Canvas c) {
        if (lang.changed()) style.refresh();
        long now = SystemClock.uptimeMillis();
        boolean pad = PadInput.isPadMode();
        if (progress >= 0) { if (hudShown()) ringOnHud(c); else { c.save(); frame(c, GameRect.BLOCK); plaque(c); c.restore(); } return; }
        if (hintAt < 0) return;
        long t = now - hintAt;
        if (t >= HINT_MS || !pad || DmActivity.padPageOpen()) { hintAt = -1; return; }
        if (!hudShown()) { postInvalidateOnAnimation(); return; }
        float a = Math.min(1f, Math.min(t, HINT_MS - t) / (float) FADE_MS);
        paint.setAlpha((int) (255 * a));
        Bitmap start = art.glyph(PadInput.glyph(), "start");
        float px = legendPx(), h = px * 1.8f;
        Paint text = legend.solid(px, (int) (255 * a));
        Object[] parts = {start, AppText.get(getContext(), R.string.dmAndroidQuickHoldShort), null, start, AppText.get(getContext(), R.string.dmAndroidQuickHoldLong)};
        float w = PadLegend.width(h, text, parts) + h * 1.6f;
        c.save(); frame(c, GameRect.TOP);
        bar.set(fw / 2f - w / 2, fh * 0.035f, fw / 2f + w / 2, fh * 0.035f + h);
        PadLegend.draw(c, bar, art.image("pad/footer"), paint, text, parts);
        c.restore();
        paint.setAlpha(255);
        postInvalidateOnAnimation();
    }


    private void ringOnHud(Canvas c) {
        if (DmActivity.nativeUiRect(ui) == 0 || ui[4] <= 0 || ui[5] <= 0) return;
        GameRect g = GameRect.ui(); boolean whole = g.whole();
        float ox = whole ? 0 : g.pictureX(), oy = whole ? 0 : g.pictureY(), pw = whole ? getWidth() : g.pictureW(), ph = whole ? getHeight() : g.pictureH();
        float sx = pw / (float) ui[4], sy = ph / (float) ui[5], u = ui[3] / 2160f;
        float cx = ox + (ui[0] + ui[2] / 2f) * sx, cy = oy + (ui[1] + ui[3] - RING_DY * u) * sy, r = RING_D * u * sy / 2;
        dst.set(cx - r, cy - r, cx + r, cy + r);
        ring(c, dst);
    }


    private void plaque(Canvas c) {
        String label = AppText.get(getContext(), R.string.dmAndroidQuickHoldLong);
        float W = fw, H = fh, px = legendPx(), edge = EDGE * H;
        float room = (W - PANEL_UNITS * GameStyleGeometry.k(H)) / 2 - 2 * edge;
        float need = plaqueWidth(label, px), fit = need > room ? room / need : 1;
        boolean side = fit >= MIN_FIT;
        if (side && fit < 1) px *= fit;
        float h = px * 1.8f, ring = h * 0.92f, w = plaqueWidth(label, px);
        float left = side ? W - edge - w : W / 2f - w / 2, top = side ? H / 2f - h / 2 : H * (1 - EDGE) - h;
        bar.set(left, top, left + w, top + h);
        Bitmap footer = art.image("pad/footer");
        if (footer != null) PadLegend.strip(c, footer, bar, paint);
        float x = bar.left + h * 0.6f, cy = bar.centerY();
        Bitmap start = art.glyph(PadInput.glyph(), "start");
        if (start != null) { float gh = h * 0.52f, gw = start.getWidth() * gh / Math.max(1, start.getHeight()); dst.set(x + ring / 2 - gw / 2, cy - gh / 2, x + ring / 2 + gw / 2, cy + gh / 2); c.drawBitmap(start, null, dst, paint); }
        dst.set(x, cy - ring / 2, x + ring, cy + ring / 2);
        ring(c, dst);
        legend.draw(c, label, x + ring + h * 0.25f, cy + px * 0.36f, px / legend.units(), Paint.Align.LEFT, 255);
    }


    private float legendPx() { GameRect g = GameRect.ui(); return legend.units() * GameStyleGeometry.k(g.whole() ? getHeight() : g.uiH()); }

    private float fw, fh;
    private final int[] box = new int[4];
    private void frame(Canvas c, int kind) {
        GameRect g = GameRect.ui();
        if (g.whole()) { fw = getWidth(); fh = getHeight(); return; }
        g.bounds(kind, box); fw = box[2]; fh = box[3]; c.translate(box[0], box[1]);
    }

    private float plaqueWidth(String label, float px) { float h = px * 1.8f; return h * 0.92f + h * 0.25f + legend.measure(label, px / legend.units()) + h * 1.2f; }


    private void ring(Canvas c, RectF where) {
        Bitmap strip = art.image("pad/hold");
        if (strip == null) return;
        int n = Math.max(1, strip.getWidth() / Math.max(1, strip.getHeight())), f = Math.min(n - 1, Math.round(progress * (n - 1))), fw = strip.getWidth() / n;
        src.set(f * fw, 0, (f + 1) * fw, strip.getHeight());
        c.drawBitmap(strip, src, where, paint);
    }
}
