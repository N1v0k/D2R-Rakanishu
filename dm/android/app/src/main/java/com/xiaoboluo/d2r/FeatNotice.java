package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import java.nio.charset.StandardCharsets;








final class FeatNotice extends View implements Choreographer.FrameCallback {

    static final float ROW_BASE = KillStreak.ROW_BASE - KillStreak.COUNT_PX - 0.012f, TEXT_PX = KillStreak.TEXT_PX;
    private static final String SEP = " · ";

    private final TouchOverlay overlay;
    private final PotionButtons potions;
    private final GameText gold, white, grey;
    private final Paint mark = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path diamond = new Path();
    private final FeatQueue queue = new FeatQueue();
    private final int[] hud = new int[FeatQueue.N];
    private final float[] buttons = new float[3 * 32];
    private final GameLang.Watch lang = new GameLang.Watch();
    private FeatQueue.Note note;
    private String label = "", name = "", cond = "";
    private float px, markX, labelX, nameX, sepX, condX, placedFor = -1, placedIn = -1;
    private int alpha;
    private boolean running, shown;

    FeatNotice(Context context, TouchOverlay overlay, PotionButtons potions, GameStyle style) {
        super(context);
        this.overlay = overlay;
        this.potions = potions;
        gold = style.text(GameStyleData.TITLE);
        white = style.text(GameStyleData.HUD_TEXT);
        grey = style.text(GameStyleData.HUD_TEXT).colour("FontColorLightGray");
        gold.prepare(); white.prepare(); grey.prepare();
        mark.setColor(style.colour("FontColorGoldYellow"));
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; Choreographer.getInstance().removeFrameCallback(this); queue.hidden(); shown = false; invalidate(); }

    @Override public boolean onTouchEvent(MotionEvent e) { return false; }



    private String[] texts(int seq, int id, int arg) {
        byte[] raw = DmActivity.nativeFeatNames(seq);
        String[] games = (raw == null ? "\n" : new String(raw, StandardCharsets.UTF_8) + "\n").split("\n", -1);
        return FeatTexts.of(getContext(), id, arg, games[0], games.length > 1 ? games[1] : "");
    }


    private boolean place(float h, float room) {
        if (h == placedFor && room == placedIn) return true;
        placedFor = -1;
        if (!(h > 0) || !(room > 0) || !gold.ready() || !white.ready() || !grey.ready()) return false;
        float tp = TEXT_PX * h, m = 0.75f * tp;
        float wl = gold.measure(label + SEP, k(gold, tp)), wn = white.measure(name, k(white, tp)), ws = gold.measure(SEP, k(gold, tp)), wc = grey.measure(cond, k(grey, tp));
        float w = m + wl + wn + ws + wc, s = w > room ? room / w : 1f;
        px = tp * s;
        markX = -w * s / 2; labelX = markX + m * s; nameX = labelX + wl * s; sepX = nameX + wn * s; condX = sepX + ws * s;
        placedFor = h; placedIn = room;
        return true;
    }

    @Override public void doFrame(long ns) {
        if (!running) return;
        int n = DmActivity.nativeFeatNotice(hud);
        queue.take(hud, n);
        boolean game = overlay.displaysShown(ns);
        float h = getHeight(), top = (ROW_BASE - TEXT_PX) * h, bottom = ROW_BASE * h;
        FeatQueue.Note now = queue.at(ns, game && h > 0);
        if (queue.textsDue(now, lang.changed()) != null) {
            note = now; placedFor = -1;
            String[] t = texts(now.seq, now.id, now.arg);
            if (t == null) { label = name = cond = ""; android.util.Log.w("dm", "feat notice " + now.id + " has no text"); }
            else { label = AppText.get(getContext(), R.string.dmAndroidFeat); name = t[0]; cond = t[1]; }
            invalidate();
        }
        float room = now != null ? KillStreak.room(overlay, potions, buttons, getWidth(), h, top, bottom) : 0;
        boolean on = now != null && !name.isEmpty() && place(h, room);
        int a = on ? queue.alpha() : 0;
        if (on != shown || a != alpha) { alpha = a; invalidate(); }
        shown = on;
        Choreographer.getInstance().postFrameCallback(this);
    }

    @Override protected void onDraw(Canvas canvas) {
        boolean on = shown && placedFor == getHeight();
        if (BuildConfig.DM_QA_BUILD) report(on);
        if (!on) return;
        float h = getHeight(), cx = getWidth() / 2f, base = ROW_BASE * h, r = 0.28f * px;
        float mx = cx + markX + 0.5f * r, my = base - 0.36f * px;
        diamond.rewind(); diamond.moveTo(mx, my - r); diamond.lineTo(mx + r * 0.75f, my); diamond.lineTo(mx, my + r); diamond.lineTo(mx - r * 0.75f, my); diamond.close();
        mark.setAlpha(alpha); canvas.drawPath(diamond, mark);
        gold.draw(canvas, label + SEP, cx + labelX, base, k(gold, px), Paint.Align.LEFT, alpha);
        white.draw(canvas, name, cx + nameX, base, k(white, px), Paint.Align.LEFT, alpha);
        gold.draw(canvas, SEP, cx + sepX, base, k(gold, px), Paint.Align.LEFT, alpha);
        grey.draw(canvas, cond, cx + condX, base, k(grey, px), Paint.Align.LEFT, alpha);
    }

    private static float k(GameText t, float px) { return t.units() > 0 ? px / t.units() : 0; }




    private final int[] at = new int[2];
    private String place = "", state = "", row = "";
    private void report(boolean on) {
        getLocationInWindow(at);
        String p = at[0] + "_" + at[1] + "_" + getWidth() + "_" + getHeight();
        if (!p.equals(place)) { place = p; TestHooks.overlay(this, "feat_notice_at", p); }
        if (on) {
            float cx = at[0] + getWidth() / 2f;
            String r = Math.round(cx + markX) + "_" + Math.round(cx - markX) + "_" + Math.round(at[1] + ROW_BASE * getHeight()) + "_" + Math.round(px * 10);
            if (!r.equals(row)) { row = r; TestHooks.overlay(this, "feat_notice_row", r); }
        }
        String s = on ? "shown_" + note.seq + "_" + note.id : "empty";
        if (!s.equals(state)) { state = s; TestHooks.overlay(this, "feat_notice", s); }
    }
}
