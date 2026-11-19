package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;












final class KillStreak extends View implements Choreographer.FrameCallback {


    static final float ROW_BASE = 0.790f, BAR_BOTTOM = 0.815f, BAR_H = 0.012f, BAR_W = 0.30f, ROW_W = 0.80f, GAP = 0.010f;

    static final float COUNT_PX = 0.046f, TEXT_PX = 0.034f;
    private static final String SEP = " · ";
    private static final long TICK_NS = 40_000_000L, FLASH_NS = 180_000_000L, END_NS = 3_600_000_000L, FADE_NS = 600_000_000L;
    private static final int MAX_EXTRAPOLATE = 10;
    private static final int TICK = 0, KILLS = 1, LEFT = 2, WINDOW = 3, PCT = 4, END_SEQ = 5, END_KILLS = 6, END_BONUS = 7, END_KIND = 8, END_RECORD = 9,
        TIER = 10, END_TIER = 11, N = 12, END_DEATH = 2;

    private final TouchOverlay overlay;
    private final PotionButtons potions;
    private final GameText count, name, bonus;
    private final Row live = new Row(), result = new Row();
    private final Paint back = new Paint(), fill = new Paint(), edge = new Paint(), frame = new Paint();
    private final int fillColour, flashColour;
    private final int[] hud = new int[N];
    private final RectF bar = new RectF();
    private final float[] buttons = new float[3 * 32];
    private final GameLang.Watch lang = new GameLang.Watch();
    private String pctText = "", tierText = "";
    private int kills, left, window = 1, shownPct, tier = -1, tick = Integer.MIN_VALUE, endSeq = -1, endKills, endBonus, endRecord;
    private long anchorNs, flashNs, endNs;
    private float fraction, barW;
    private boolean running, shown, endOn;
    private String state = "";

    KillStreak(Context context, TouchOverlay overlay, PotionButtons potions, GameStyle style) {
        super(context);
        this.overlay = overlay;
        this.potions = potions;
        count = style.text(GameStyleData.ANNOUNCE).colour("FontColorGoldYellow");
        name = style.text(GameStyleData.TITLE);
        bonus = style.text(GameStyleData.HUD_TEXT).colour("FontColorPartyGreen");
        count.prepare(); name.prepare(); bonus.prepare();

        int red = style.colour("FontColorRed");
        fillColour = 0xFF000000 | ((red >> 16 & 0xFF) * 3 / 5) << 16 | ((red >> 8 & 0xFF) * 3 / 10) << 8 | (red & 0xFF) * 3 / 10;
        flashColour = red;
        back.setColor(0xBF000000);
        edge.setColor(red);
        frame.setColor(style.colour("FontColorDarkGold")); frame.setStyle(Paint.Style.STROKE);
    }

    private static final int[] TITLES = { R.string.dmAndroidStreakTier1, R.string.dmAndroidStreakTier2, R.string.dmAndroidStreakTier3, R.string.dmAndroidStreakTier4,
        R.string.dmAndroidStreakTier5, R.string.dmAndroidStreakTier6, R.string.dmAndroidStreakTier7, R.string.dmAndroidStreakTier8, R.string.dmAndroidStreakTier9 };

    static String title(Context c, int tier) {
        return AppText.get(c, tier >= 1 && tier <= TITLES.length ? TITLES[tier - 1] : R.string.dmAndroidStreakTitle);
    }



    private final class Row {
        String countText = "", nameText = "", tailText = "";
        float countPx, textPx, countX, nameX, tailX;
        private float placedFor = -1, placedIn = -1;

        void set(String c, String n, String t) {
            if (c.equals(countText) && n.equals(nameText) && t.equals(tailText)) return;
            countText = c; nameText = n; tailText = t; placedFor = -1;
        }

        void again() { placedFor = -1; }


        boolean place(float h, float room) {
            if (h == placedFor && room == placedIn) return true;
            placedFor = -1;
            if (!(h > 0) || !(room > 0) || !count.ready() || !name.ready() || !bonus.ready()) return false;
            float cp = COUNT_PX * h, tp = TEXT_PX * h;
            float wc = count.measure(countText, k(count, cp)), wn = name.measure(nameText, k(name, tp)), wt = bonus.measure(tailText, k(bonus, tp));
            float w = wc + wn + wt, s = w > room ? room / w : 1f;
            countPx = cp * s; textPx = tp * s;
            countX = -w * s / 2; nameX = countX + wc * s; tailX = nameX + wn * s;
            placedFor = h; placedIn = room;
            return true;
        }

        void draw(Canvas c, float cx, float base, int alpha) {
            count.draw(c, countText, cx + countX, base, k(count, countPx), Paint.Align.LEFT, alpha);
            name.draw(c, nameText, cx + nameX, base, k(name, textPx), Paint.Align.LEFT, alpha);
            if (!tailText.isEmpty()) bonus.draw(c, tailText, cx + tailX, base, k(bonus, textPx), Paint.Align.LEFT, alpha);
        }
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; Choreographer.getInstance().removeFrameCallback(this); shown = false; endOn = false; invalidate(); }

    @Override public boolean onTouchEvent(MotionEvent e) { return false; }

    private void liveRow() { live.set(kills > 0 ? Integer.toString(kills) : "", SEP + tierText + (pctText.isEmpty() ? "" : SEP), pctText); }

    @Override public void doFrame(long ns) {
        if (!running) return;
        boolean relang = lang.changed();
        if (relang) { shownPct = -1; live.again(); result.again(); invalidate(); }
        int n = DmActivity.nativeStreak(hud);
        boolean game = overlay.displaysShown(ns);
        if (n >= N) {
            if (hud[TICK] != tick) { tick = hud[TICK]; anchorNs = ns; }
            if (hud[KILLS] != kills) {
                if (hud[KILLS] > kills && kills > 0) flashNs = ns;
                if (hud[KILLS] > 0 && kills == 0) endOn = false;
                kills = hud[KILLS]; liveRow();
            }
            if (relang || hud[TIER] != tier) { tier = hud[TIER]; tierText = title(getContext(), tier); liveRow(); }
            if (hud[PCT] != shownPct) { shownPct = hud[PCT]; pctText = shownPct > 0 ? AppText.get(getContext(), R.string.dmAndroidStreakExp, shownPct) : ""; liveRow(); }
            left = hud[LEFT]; window = Math.max(1, hud[WINDOW]);
            if (endSeq < 0) endSeq = hud[END_SEQ];
            else if (hud[END_SEQ] != endSeq) {
                endSeq = hud[END_SEQ];
                if (hud[END_KIND] != END_DEATH) {
                    endRecord = hud[END_RECORD] != 0 ? 1 : 0;
                    String bonusText = hud[END_BONUS] > 0 ? AppText.get(getContext(), R.string.dmAndroidStreakBonus, hud[END_BONUS]) : "";
                    String w = title(getContext(), hud[END_TIER]);
                    String end = AppText.get(getContext(), endRecord != 0 ? R.string.dmAndroidStreakEndTierRecord : R.string.dmAndroidStreakEndTier, w);
                    result.set(Integer.toString(hud[END_KILLS]), SEP + end + (bonusText.isEmpty() ? "" : SEP), bonusText);
                    endWord = w; endTier = hud[END_TIER];
                    endKills = hud[END_KILLS]; endBonus = Math.max(0, hud[END_BONUS]); endNs = ns; endOn = true;
                }
            }
        }
        if (endOn && ns - endNs > END_NS) endOn = false;
        float h = getHeight(), room = game ? room(getWidth(), h) : 0;
        barW = Math.min(BAR_W * h, room);
        boolean counter = game && kills > 0 && live.place(h, room);
        long elapsed = Math.min(Math.max(0, ns - anchorNs), MAX_EXTRAPOLATE * TICK_NS);
        float f = counter ? Math.max(0f, Math.min(1f, (left - (float) elapsed / TICK_NS) / window)) : 0f;
        boolean end = game && endOn && result.place(h, room);
        if (counter || end || shown || f != fraction) { fraction = f; invalidate(); }
        shown = counter || end;
        Choreographer.getInstance().postFrameCallback(this);
    }

    @Override protected void onDraw(Canvas canvas) {
        boolean counter = shown && kills > 0 && live.placedFor == getHeight(), end = shown && endOn && !counter && result.placedFor == getHeight();
        if (BuildConfig.DM_QA_BUILD) report(counter, end);
        float h = getHeight(), cx = getWidth() / 2f;
        if (counter) {
            live.draw(canvas, cx, ROW_BASE * h, 255);
            float w = barW, top = (BAR_BOTTOM - BAR_H) * h, bottom = BAR_BOTTOM * h, x0 = cx - w / 2;
            bar.set(x0, top, x0 + w, bottom); canvas.drawRect(bar, back);
            if (fraction > 0) {
                float x1 = x0 + w * fraction, lead = Math.min(x1 - x0, BAR_H * h * 0.35f);
                fill.setColor(frameNs() - flashNs < FLASH_NS ? flashColour : fillColour);
                bar.set(x0, top, x1, bottom); canvas.drawRect(bar, fill);
                bar.set(x1 - lead, top, x1, bottom); canvas.drawRect(bar, edge);
            }
            frame.setStrokeWidth(Math.max(1f, h * 0.0016f));
            bar.set(x0, top, x0 + w, bottom); canvas.drawRect(bar, frame);
        }
        if (end) {
            long age = frameNs() - endNs;
            result.draw(canvas, cx, ROW_BASE * h, age < END_NS - FADE_NS ? 255 : (int) Math.max(0, 255 * (END_NS - age) / FADE_NS));
        }
    }



    private float room(float w, float h) {
        float half = ROW_W * h / 2, cx = w / 2, top = (ROW_BASE - COUNT_PX) * h, bottom = BAR_BOTTOM * h, gap = GAP * h;
        int n = potions.circles(buttons, overlay.controls(buttons, 0));
        for (int i = 0; i < n; i += 3) {
            float x = buttons[i], y = buttons[i + 1], r = buttons[i + 2], dy = y < top ? top - y : y > bottom ? y - bottom : 0;
            if (dy >= r) continue;
            half = Math.min(half, Math.abs(x - cx) - (float) Math.sqrt(r * r - dy * dy) - gap);
        }
        return Math.max(0, 2 * half);
    }

    private static String hex(String w) {
        StringBuilder b = new StringBuilder();
        for (byte x : w.getBytes(java.nio.charset.StandardCharsets.UTF_8)) b.append(String.format(java.util.Locale.ROOT, "%02x", x & 0xff));
        return b.toString();
    }
    private static long frameNs() { return System.nanoTime(); }
    private static float k(GameText t, float px) { return t.units() > 0 ? px / t.units() : 0; }





    private final int[] at = new int[2];
    private String place = "", wordState = "", endWord = "";
    private int endTier;
    private void report(boolean counter, boolean end) {
        getLocationInWindow(at);
        String p = at[0] + "_" + at[1] + "_" + getWidth() + "_" + getHeight();
        if (!p.equals(place)) { place = p; TestHooks.overlay(this, "kill_streak_at", p); }
        Row row = counter ? live : end ? result : null;
        if (row != null) TestHooks.overlay(this, "kill_streak_size", Math.round(row.countPx * 10) + "_" + Math.round(row.textPx * 10) + "_" + Math.round(barW));
        if (row != null) {
            String ws = (counter ? tier : endTier) + "_" + hex(counter ? tierText : endWord);
            if (!ws.equals(wordState)) { wordState = ws; TestHooks.overlay(this, "kill_streak_word", ws); }
        }
        String s = counter ? "counter_" + kills + "_" + Math.max(0, shownPct) : end ? "banner_" + endKills + "_" + endBonus + (endRecord != 0 ? "_record" : "") : "empty";
        if (!s.equals(state)) { state = s; TestHooks.overlay(this, "kill_streak", s); }
    }
}
