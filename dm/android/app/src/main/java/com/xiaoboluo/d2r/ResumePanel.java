package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.function.BooleanSupplier;
import org.json.JSONArray;
import org.json.JSONObject;





final class ResumePanel extends View implements PadPage {
    private static final long POLL_MS = 200;
    private final AppArt art;
    private final GameStyle style;
    private final GameText title, body;
    private final Runnable freeInput;
    private final BooleanSupplier canOpen;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final RectF box = new RectF(), r = new RectF();
    private final RectF[] btn = { new RectF(), new RectF() }, hit = { new RectF(), new RectF() };
    private final GameLang.Watch lang = new GameLang.Watch();
    private final int[] ui = new int[6];
    private int level, focus = -1, down = -1;
    private boolean running, loading;
    private String area = "";
    private float k;
    private final Runnable poll = this::pollTick;

    ResumePanel(Context context, AppArt art, GameStyle style, Runnable freeInput, BooleanSupplier canOpen) {
        super(context);
        this.art = art;
        this.style = style;
        this.freeInput = freeInput;
        this.canOpen = canOpen;
        title = style.text(GameStyleData.TITLE);
        body = style.text(GameStyleData.MODAL_TEXT);
        title.prepare();
        body.prepare();
        setVisibility(GONE);
    }

    private String t(int id) { return AppText.get(getContext(), id); }

    boolean isOpen() { return getVisibility() == VISIBLE; }

    void start() { if (!running) { running = true; removeCallbacks(poll); postDelayed(poll, POLL_MS); } }
    void stop() { running = false; removeCallbacks(poll); }
    private void pollTick() {
        if (!running) return;
        if (!isOpen() && !loading && DmActivity.hostStarted() && DmActivity.nativeInGame() != 0 && DmActivity.nativeUiRect(ui) != 0 && canOpen.getAsBoolean()) {
            int lv = DmActivity.nativeResumeOffer();
            if (lv > 0) load(lv);
        }
        postDelayed(poll, POLL_MS);
    }


    private void load(int lv) {
        loading = true;
        int language = GameLang.index();
        new Thread(() -> {
            String name = "";
            try {
                File f = new File(getContext().getFilesDir(), "dm/overlay/sim-names.json");
                JSONArray a = new JSONObject(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)).optJSONArray("@area:" + lv);
                if (a != null) name = a.optString(language >= 0 ? language : 0, a.optString(0, ""));
            } catch (Exception e) { AppArt.log("resume: area names unavailable (" + e.getClass().getSimpleName() + ")"); }
            final String shown = name;
            post(() -> { loading = false; if (running && DmActivity.nativeResumeOffer() == lv) open(lv, shown); });
        }, "dm-resume-name").start();
    }

    private void open(int lv, String name) {
        level = lv;
        area = name;
        focus = PadInput.isPadMode() ? 0 : -1;
        down = -1;
        DmActivity.nativePause(DmActivity.PAUSE_RESUME, 1);
        setVisibility(VISIBLE);
        freeInput.run();
        AppArt.log("resume: panel for area " + lv);
        invalidate();
    }

    private void choose(boolean here) {
        if (!isOpen()) return;
        int taken = DmActivity.nativeResumeChoose(here ? 1 : 0);
        setVisibility(GONE);
        DmActivity.nativePause(DmActivity.PAUSE_RESUME, 0);
        AppArt.log("resume: " + (here ? "continue here" : "stay in camp") + (taken != 0 ? "" : " (offer already closed)") + " for area " + level);
    }

    @Override public boolean padOpen() { return isOpen(); }
    @Override public void back() { choose(false); }
    @Override public void onPad(int cmd) {
        if (cmd == PadNav.B || cmd == PadNav.MENU) { choose(false); return; }
        if (cmd == PadNav.LEFT || cmd == PadNav.RIGHT) { focus = focus < 0 ? 0 : cmd == PadNav.LEFT ? 0 : 1; invalidate(); return; }
        if (cmd == PadNav.A) { if (focus >= 0) choose(focus == 0); else { focus = 0; invalidate(); } }
    }
    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }

    private void geom() {
        float W = getWidth(), H = getHeight();
        GameStyleGeometry.Panel p = style.panel(W, H);
        float bh = p.modalH(), bw = p.modalW(), gap = bw * 0.12f;
        float w = Math.max(Math.min(W * 0.62f, H * 1.25f), 2 * bw + gap + bw * 0.4f), pad = w * 0.07f;
        k = Math.max(GameStyleGeometry.k(H), body.units() > 0 ? 0.031f * H / body.units() : 0);
        float h = pad * 1.2f + (area.isEmpty() ? 0 : title.size(k) * 1.8f) + body.size(k) * 1.45f + pad * 0.8f + bh + pad;
        box.set((W - w) / 2, (H - h) / 2, (W + w) / 2, (H + h) / 2);
        float cy = box.bottom - pad - bh / 2, th = Math.max(bh, p.minTap) / 2;
        for (int i = 0; i < 2; i++) {
            float x0 = box.centerX() + (i == 0 ? -gap / 2 - bw : gap / 2);
            btn[i].set(x0, cy - bh / 2, x0 + bw, cy + bh / 2);
            hit[i].set(btn[i].left, Math.max(box.top, cy - th), btn[i].right, Math.min(box.bottom, cy + th));
        }
    }

    @Override protected void onDraw(Canvas c) {
        if (lang.changed()) style.refresh();
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "resume");
        try {
            style.backdrop(c);
            geom();
            art.nine(c, "build_modal", box, 60, getHeight() / 900f, paint);
            float pad = box.width() * 0.07f, y = box.top + pad * 1.2f;
            if (!area.isEmpty()) { y += title.size(k); title.draw(c, area, box.centerX(), y, k, Paint.Align.CENTER, 255); y += title.size(k) * 0.8f; }
            y += body.size(k) * 1.45f;
            body.draw(c, t(R.string.dmAndroidResumeAsk), box.centerX(), y, k, Paint.Align.CENTER, 255);
            String[] labels = { t(R.string.dmAndroidResumeHere), t(R.string.dmAndroidResumeCamp) };
            for (int i = 0; i < 2; i++) {
                style.modalButton(c, btn[i], down == i ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, labels[i]);
                if (BuildConfig.DM_QA_BUILD) TestHooks.hit(hit[i], i, 0);
            }
            if (focus >= 0) {
                RectF b = btn[focus];
                float dx = b.width() * 11f / 265f, dy = b.height() * 10.5f / 51f;
                r.set(b.left - dx, b.top - dy, b.right + dx, b.bottom + dy);
                Bitmap f = art.image("pad/focus");
                if (f != null) c.drawBitmap(f, null, r, paint);
            }
        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        geom();
        int a = e.getActionMasked();
        if (a == MotionEvent.ACTION_DOWN) { focus = -1; down = hit[0].contains(e.getX(), e.getY()) ? 0 : hit[1].contains(e.getX(), e.getY()) ? 1 : -1; invalidate(); }
        else if (a == MotionEvent.ACTION_UP) { int was = down; down = -1; if (was >= 0 && hit[was].contains(e.getX(), e.getY())) choose(was == 0); else invalidate(); }
        else if (a == MotionEvent.ACTION_CANCEL) { down = -1; invalidate(); }
        return true;
    }

    private void report() { if (DmActivity.hostStarted()) TestHooks.overlay(this, "resume", isOpen() ? "open" : "closed"); }
    @Override public void setVisibility(int visibility) { super.setVisibility(visibility); report(); }
}
