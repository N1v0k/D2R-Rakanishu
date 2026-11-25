package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import java.util.ArrayList;
import java.util.List;
import java.util.function.BooleanSupplier;





final class TransferNotice extends View implements PadPage {
    private static final long POLL_MS = 200;
    private final AppArt art;
    private final GameStyle style;
    private final GameText title, body;
    private final Runnable freeInput;
    private final BooleanSupplier canOpen;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final RectF box = new RectF(), btn = new RectF(), hit = new RectF(), r = new RectF();
    private final GameLang.Watch lang = new GameLang.Watch();
    private final int[] ui = new int[6];
    private final List<String> rows = new ArrayList<>();
    private List<String> messages = new ArrayList<>();
    private List<SaveTransfer.Result> results;
    private boolean busy, running, down, padFocus;
    private float k;
    private final Runnable poll = this::pollTick;

    TransferNotice(Context context, AppArt art, GameStyle style, Runnable freeInput, BooleanSupplier canOpen) {
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

    boolean isOpen() { return getVisibility() == VISIBLE && !busy; }


    void busy(boolean on) {
        if (busy == on) return;
        busy = on;
        if (on) { results = null; messages = List.of(t(R.string.dmAndroidSaveBusy)); }
        setVisibility(on ? VISIBLE : GONE);
        report();
        invalidate();
    }

    void start() { if (!running) { running = true; removeCallbacks(poll); postDelayed(poll, POLL_MS); } }
    void stop() { running = false; removeCallbacks(poll); }
    private void pollTick() {
        if (!running) return;
        if (!busy && !isOpen() && DmActivity.hostStarted() && DmActivity.transferPending() && DmActivity.nativeInGame() == 0
            && DmActivity.nativeUiRect(ui) != 0 && canOpen.getAsBoolean()) open();
        postDelayed(poll, POLL_MS);
    }

    void open() {
        DmActivity.syncLang();
        results = DmActivity.takeTransferResults();
        messages = DmActivity.transferText(getContext(), results);
        if (messages.isEmpty()) { results = null; return; }
        padFocus = PadInput.isPadMode();
        down = false;
        setVisibility(VISIBLE);
        freeInput.run();
        invalidate();
    }
    void close() { results = null; setVisibility(GONE); }

    @Override public boolean padOpen() { return isOpen(); }
    @Override public void back() { close(); }
    @Override public void onPad(int cmd) {
        if (cmd == PadNav.B || cmd == PadNav.MENU) { close(); return; }
        if (cmd == PadNav.A) { if (padFocus) close(); else { padFocus = true; invalidate(); } return; }
        if (cmd >= PadNav.UP && cmd <= PadNav.RIGHT && !padFocus) { padFocus = true; invalidate(); }
    }
    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }


    private void wrap(float width) {
        rows.clear();
        for (String m : messages) {
            StringBuilder line = new StringBuilder();
            for (String word : m.split(" ")) {
                String next = line.length() == 0 ? word : line + " " + word;
                if (body.measure(next, k) <= width) { line.setLength(0); line.append(next); continue; }
                if (line.length() > 0) { rows.add(line.toString()); line.setLength(0); }
                for (int i = 0; i < word.length(); i++) {
                    if (line.length() > 0 && body.measure(line.toString() + word.charAt(i), k) > width) { rows.add(line.toString()); line.setLength(0); }
                    line.append(word.charAt(i));
                }
            }
            if (line.length() > 0) rows.add(line.toString());
        }
    }


    private void geom() {
        float W = getWidth(), H = getHeight(), w = Math.min(W * 0.62f, H * 1.25f), pad = w * 0.07f;
        GameStyleGeometry.Panel p = style.panel(W, H);
        float bh = busy ? 0 : p.modalH(), bw = p.modalW();

        float h = 0, k0 = Math.max(GameStyleGeometry.k(H), body.units() > 0 ? 0.031f * H / body.units() : 0);
        float fixed = 0;
        for (int step = 0; step <= 6; step++) {
            k = k0 * (1 - 0.05f * step);
            wrap(w - 2 * pad);
            fixed = pad * 1.2f + title.size(k) * 1.8f + (busy ? pad : pad * 0.8f + bh + pad);
            h = fixed + rows.size() * body.size(k) * 1.45f;
            if (h <= H * 0.9f) break;
        }
        if (h > H * 0.9f) {
            int fit = Math.min(rows.size(), Math.max(1, (int) ((H * 0.9f - fixed) / (body.size(k) * 1.45f))));
            while (rows.size() > fit) rows.remove(rows.size() - 1);
            rows.set(fit - 1, rows.get(fit - 1) + " …");
            h = fixed + rows.size() * body.size(k) * 1.45f;
        }
        box.set((W - w) / 2, (H - h) / 2, (W + w) / 2, (H + h) / 2);
        float cy = box.bottom - pad - bh / 2, th = Math.max(bh, p.minTap) / 2;
        btn.set(box.centerX() - bw / 2, cy - bh / 2, box.centerX() + bw / 2, cy + bh / 2);
        hit.set(btn.left, Math.max(box.top, cy - th), btn.right, Math.min(box.bottom, cy + th));
    }

    @Override protected void onDraw(Canvas c) {
        if (lang.changed()) {
            style.refresh();
            if (busy) messages = List.of(t(R.string.dmAndroidSaveBusy));
            else if (results != null) messages = DmActivity.transferText(getContext(), results);
        }
        style.backdrop(c);
        geom();
        art.nine(c, "build_modal", box, 60, getHeight() / 900f, paint);
        float pad = box.width() * 0.07f, y = box.top + pad * 1.2f + title.size(k);
        title.draw(c, t(R.string.dmAndroidSaveTitle), box.centerX(), y, k, Paint.Align.CENTER, 255);
        y += title.size(k) * 0.8f;
        for (String row : rows) { y += body.size(k) * 1.45f; body.draw(c, row, box.centerX(), y, k, Paint.Align.CENTER, 255); }
        if (busy) return;
        style.modalButton(c, btn, down ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, t(R.string.dmConfirm));
        if (padFocus) {
            float dx = btn.width() * 11f / 265f, dy = btn.height() * 10.5f / 51f;
            r.set(btn.left - dx, btn.top - dy, btn.right + dx, btn.bottom + dy);
            Bitmap f = art.image("pad/focus");
            if (f != null) c.drawBitmap(f, null, r, paint);
        }
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        if (busy) return true;
        geom();
        int a = e.getActionMasked();
        if (a == MotionEvent.ACTION_DOWN) { padFocus = false; down = hit.contains(e.getX(), e.getY()); invalidate(); }
        else if (a == MotionEvent.ACTION_UP) { boolean on = down && hit.contains(e.getX(), e.getY()); down = false; if (on) close(); else invalidate(); }
        else if (a == MotionEvent.ACTION_CANCEL) { down = false; invalidate(); }
        return true;
    }


    private void report() { if (DmActivity.hostStarted()) TestHooks.overlay(this, "save_notice", isOpen() ? "open" : "closed"); }
    @Override public void setVisibility(int visibility) { super.setVisibility(visibility); report(); }
}
