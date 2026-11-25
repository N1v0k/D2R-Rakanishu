package com.xiaoboluo.d2r;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.os.BatteryManager;
import android.os.Build;
import android.util.Log;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import java.util.Calendar;




















final class StatusLine extends View implements Choreographer.FrameCallback {

    private static final float SIZE = StatusPlace.SIZE, LINE = StatusPlace.LINE;
    private final TouchOverlay overlay;
    private final GameStyle style;
    private final GameText value, label, warn;
    private final String[] parts = {"", ""};
    private final GameLang.Watch lang = new GameLang.Watch();


    private static final String FPS = " FPS", FPS_NONE = "– FPS";
    private final StatusModel model = new StatusModel();
    private final StatusPlace place = new StatusPlace();


    private static int placed = StatusPlace.UNKNOWN;
    private final BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            String action = intent == null ? null : intent.getAction();
            if (Intent.ACTION_BATTERY_CHANGED.equals(action)) battery(intent);
            else if (Intent.ACTION_TIME_TICK.equals(action) || Intent.ACTION_TIME_CHANGED.equals(action) || Intent.ACTION_TIMEZONE_CHANGED.equals(action)) clock();
        }
    };
    private final StatusModel.Receiver hook = new StatusModel.Receiver() {
        @Override public boolean register() {
            IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
            filter.addAction(Intent.ACTION_TIME_TICK);
            filter.addAction(Intent.ACTION_TIME_CHANGED);
            filter.addAction(Intent.ACTION_TIMEZONE_CHANGED);
            Intent sticky;
            try {

                sticky = Build.VERSION.SDK_INT >= 33 ? getContext().registerReceiver(receiver, filter, Context.RECEIVER_NOT_EXPORTED) : getContext().registerReceiver(receiver, filter);
            } catch (RuntimeException e) { Log.w("dm", "status line: receiver not registered: " + e); return false; }
            battery(sticky);
            clock();
            return true;
        }
        @Override public void unregister() {
            try { getContext().unregisterReceiver(receiver); } catch (IllegalArgumentException e) { Log.w("dm", "status line: receiver was not registered: " + e); }
        }
    };
    private String battery = "", fps;
    private boolean running, shown, lineClock, lineBattery, lineFps;

    StatusLine(Context context, TouchOverlay overlay, GameStyle style) {
        super(context);
        this.overlay = overlay;
        this.style = style;
        fps = FPS_NONE;
        placed = StatusPlace.UNKNOWN;
        value = style.text(GameStyleData.STATUS_VALUE);
        label = style.text(GameStyleData.STATUS_LABEL);
        warn = style.text(GameStyleData.STATUS_VALUE).colour("FontColorRed");
        value.prepare(); label.prepare(); warn.prepare();
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() {
        running = false;
        Choreographer.getInstance().removeFrameCallback(this);
        model.reconcile(false, false, false, false, hook);
    }

    private void battery(Intent status) {
        if (status == null) return;
        boolean changed = model.battery(status.getIntExtra(BatteryManager.EXTRA_LEVEL, -1), status.getIntExtra(BatteryManager.EXTRA_SCALE, 100),
            status.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0), status.getBooleanExtra(BatteryManager.EXTRA_BATTERY_LOW, false));
        if (!changed) return;
        batteryText();
        if (shown) invalidate();
    }


    private void batteryText() {
        int kind = model.batteryKind();
        int id = kind == StatusModel.BATTERY_CHARGING ? R.string.dmAndroidStatusCharging : R.string.dmAndroidStatusBattery;
        battery = kind == StatusModel.BATTERY_NONE ? "" : AppText.get(getContext(), id, model.percent);
        StatusText.split(kind == StatusModel.BATTERY_NONE ? null : AppText.get(getContext(), id), battery, parts);
    }

    private void clock() {
        Calendar c = Calendar.getInstance();
        if (model.clock(c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE)) && shown) invalidate();
    }

    @Override public void doFrame(long ns) {
        if (!running) return;
        Context c = getContext();
        int mask = ShoulderKeys.statusMask();
        boolean master = mask != 0, lc = (mask & ShoulderKeys.STATUS_CLOCK) != 0, lb = (mask & ShoulderKeys.STATUS_BATTERY) != 0, lf = (mask & ShoulderKeys.STATUS_FPS) != 0;
        model.reconcile(true, master, lc, lb, hook);
        boolean active = master && (lc || lb || lf) && overlay.displaysShown(ns), dirty = false;

        if (place.update(toBlock(DmActivity.nativePortraitBottom()), ns)) { placed = place.shown(); dirty = shown || active; }
        if (lang.changed()) { style.refresh(); batteryText(); dirty = true; }
        if (active && lf && model.fps(DmActivity.nativeFpsMeasured())) {
            fps = model.fps < 0 ? FPS_NONE : model.fps + FPS;
            dirty = true;
        }
        if (active != shown || lc != lineClock || lb != lineBattery || lf != lineFps) { shown = active; lineClock = lc; lineBattery = lb; lineFps = lf; dirty = true; }
        if (dirty) invalidate();
        Choreographer.getInstance().postFrameCallback(this);
    }



    @Override protected void onDetachedFromWindow() { stop(); shown = false; super.onDetachedFromWindow(); }

    @Override public boolean onTouchEvent(MotionEvent e) { return false; }


    private static int toBlock(int raw) { GameRect g = GameRect.ui(); return raw < 0 || g.whole() ? raw : Math.round(g.pictureToBlock(raw)); }

    @Override protected void onDraw(Canvas c) {
        if (!shown) return;
        float size = getHeight() * SIZE, left = 0, top = 0, k = size / GameStyleData.STATUS_VALUE.units;
        android.view.WindowInsets insets = getRootWindowInsets();
        if (insets != null) { android.graphics.Insets safe = insets.getInsets(android.view.WindowInsets.Type.displayCutout()); left = safe.left; top = safe.top; }
        float x = StatusPlace.left(getHeight(), left);
        float y = StatusPlace.baseline(placed, getHeight(), top);
        if (lineClock) { value.draw(c, model.clock, x, y, k, Paint.Align.LEFT, 255); y += size * LINE; }
        if (lineBattery) {
            if (!parts[0].isEmpty()) label.draw(c, parts[0], x, y, k, Paint.Align.LEFT, 255);
            (model.low ? warn : value).draw(c, parts[1], x + label.measure(parts[0], k), y, k, Paint.Align.LEFT, 255);
            y += size * LINE;
        }
        if (lineFps) value.draw(c, fps, x, y, k, Paint.Align.LEFT, 255);
    }







    static float reservedBottom(View v) {
        android.view.WindowInsets insets = v.getRootWindowInsets();
        float top = insets == null ? 0 : insets.getInsets(android.view.WindowInsets.Type.displayCutout()).top;
        return StatusPlace.reserved(placed, v.getHeight(), top, lines());
    }



    private static int lines() { return Integer.bitCount(ShoulderKeys.statusMask() & 7); }





    static float effectsTop(View v) {
        android.view.WindowInsets insets = v.getRootWindowInsets();
        float top = insets == null ? 0 : insets.getInsets(android.view.WindowInsets.Type.displayCutout()).top;
        return StatusPlace.effectsTop(placed, v.getHeight(), top, lines());
    }
    static float effectsLeft(View v) {
        android.view.WindowInsets insets = v.getRootWindowInsets();
        return StatusPlace.left(v.getHeight(), insets == null ? 0 : insets.getInsets(android.view.WindowInsets.Type.displayCutout()).left);
    }
}
