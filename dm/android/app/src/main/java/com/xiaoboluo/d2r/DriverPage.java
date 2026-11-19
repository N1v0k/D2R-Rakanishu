package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.net.Uri;
import android.view.MotionEvent;
import android.view.View;
import java.io.FileNotFoundException;
import java.io.InputStream;



















final class DriverPage extends View implements PadPage {
    private final AppArt art;
    private final GameStyle style;
    private final Runnable freeInput, openPicker;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), head = new Paint(Paint.ANTI_ALIAS_FLAG), row = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final GameStyleGeometry.Panel panel = new GameStyleGeometry.Panel();
    private final RectF box = new RectF(), listBox = new RectF(), closeBtn = new RectF(), closeHit = new RectF(), r = new RectF();
    private final RectF[] actBtn = { new RectF(), new RectF(), new RectF() }, actHit = { new RectF(), new RectF(), new RectF() };
    private final int lightGold, goldYellow, white, red, shadowCol = 0xC8000000;
    private final GameLang.Watch lang = new GameLang.Watch();
    private String[] sha = new String[0], base = new String[0], tail = new String[0];
    private boolean[] rowRed = new boolean[0];
    private int rows, visible, focus, rowFocus, held = -1, iAdd, iDelete, iRestart, iClose;
    private float scroll, dragY = -1, dragScroll;
    private boolean padFocus, down, dragged, restarting, adding;
    private String status, notice = "", noticeArg = "", selected = "", running = "", blocked = "";
    private boolean pausing;

    DriverPage(Context context, AppArt art, GameStyle style, Runnable freeInput, Runnable openPicker) {
        super(context);
        this.art = art;
        this.style = style;
        this.freeInput = freeInput;
        this.openPicker = openPicker;
        lightGold = art.uiColour("FontColorLightGold"); goldYellow = art.uiColour("FontColorGoldYellow"); white = art.uiColour("FontColorWhite"); red = style.colour("FontColorRed");
        head.setTypeface(art.role(AppArt.EXOCET)); head.setTextAlign(Paint.Align.CENTER);
        row.setTypeface(art.role(AppArt.FORMAL)); row.setTextAlign(Paint.Align.LEFT);
        setVisibility(GONE);
    }

    private String t(int id) { return AppText.get(getContext(), id); }

    boolean isOpen() { return getVisibility() == VISIBLE; }
    void open() {
        DmActivity.syncLang();
        if (DmActivity.nativeInGame() != 0) { pausing = true; DmActivity.nativePause(DmActivity.PAUSE_DRIVER, 1); }
        padFocus = PadInput.isPadMode();
        down = false; held = -1; dragged = false; dragY = -1; scroll = 0; status = null;
        refresh();
        focusSha(selected);
        setVisibility(VISIBLE);
        freeInput.run();
        invalidate();
    }
    void close() {
        if (restarting || adding) return;
        if (pausing) { pausing = false; DmActivity.nativePause(DmActivity.PAUSE_DRIVER, 0); }
        setVisibility(GONE);
    }


    private void refresh() {
        java.util.List<DriverLibrary.Entry> ds = DriverStore.drivers(getContext());
        selected = DriverStore.selectedDriver(getContext()); running = DriverStore.runningDriver(); blocked = DriverStore.blockedDriver(getContext());
        DriverStore.Info d = DriverStore.info(getContext()); notice = d.notice; noticeArg = d.noticeArg;
        int n = ds.size() + 1;
        sha = new String[n]; base = new String[n]; tail = new String[n]; rowRed = new boolean[n];
        sha[0] = ""; base[0] = t(R.string.dmAndroidDriverSystem);
        tail[0] = running.isEmpty() ? " · " + t(R.string.dmAndroidDriverActive) : selected.isEmpty() ? " · " + t(R.string.dmAndroidDriverChosen) : "";
        for (int i = 0; i < ds.size(); i++) {
            DriverLibrary.Entry e = ds.get(i);
            sha[i + 1] = e.sha;
            base[i + 1] = e.name + (e.version.isEmpty() ? "" : " · " + e.version);
            tail[i + 1] = e.sha.equals(running) ? " · " + t(R.string.dmAndroidDriverActive) : e.sha.equals(selected) ? " · " + t(R.string.dmAndroidDriverChosen) : "";
            rowRed[i + 1] = e.sha.equals(blocked);
            if (rowRed[i + 1]) tail[i + 1] += " · " + t(R.string.dmAndroidDriverBlocked);
        }
        rows = n; iAdd = rows; iDelete = rows + 1; iRestart = rows + 2; iClose = rows + 3;
        focus = Math.min(focus, iClose); rowFocus = Math.min(rowFocus, rows - 1);
        clampScroll();
    }

    private void focusSha(String s) {
        int i = 0;
        for (int k = 0; k < rows; k++) if (sha[k].equals(s)) i = k;
        setFocus(i); ensureVisible();
    }

    private void setFocus(int i) {
        focus = Math.max(0, Math.min(i, iClose));
        if (focus < rows) rowFocus = focus;
    }

    private void activate(int i) {
        if (i == iClose) { close(); return; }
        if (restarting || adding) return;
        if (i == iAdd) { openPicker.run(); return; }
        if (i == iDelete) {
            if (rowFocus < 1 || rowFocus >= rows) return;
            int btn = focus >= rows ? focus - rows : -1;
            if (DriverStore.deleteDriver(getContext(), sha[rowFocus])) {
                refresh();
                status = canRestart() ? t(R.string.dmAndroidDriverAfterRestart) : null;
                rowFocus = Math.min(rowFocus, rows - 1);
                setFocus(btn >= 0 ? rows + Math.min(btn, 3) : rowFocus);
                ensureVisible();
            }
            invalidate();
            return;
        }
        if (i == iRestart) { if (canRestart()) restart(); return; }
        if (i >= 0 && i < rows) {



            if ((!sha[i].isEmpty() && sha[i].equals(blocked)) || !sha[i].equals(selected)) {
                if (DriverStore.selectDriver(getContext(), sha[i])) {
                    selected = sha[i];
                    status = selected.equals(running) ? null : t(R.string.dmAndroidDriverAfterRestart);
                    refresh();
                }
                invalidate();
            }
        }
    }

    private void restart() {
        restarting = true; status = t(R.string.dmAndroidTechRestarting);
        AppArt.log("driver: restart from the driver page, saving and restarting");
        Quit.restart(getContext(), "driver page restart", () -> post(() -> { restarting = false; status = t(R.string.dmAndroidTechSaveFailed); invalidate(); }));
        invalidate();
    }



    void picked(Uri uri) {
        if (uri == null) return;
        InputStream in;
        try { in = getContext().getContentResolver().openInputStream(uri); } catch (FileNotFoundException | RuntimeException e) { in = null; }
        if (in == null) { addFailed("not-readable"); return; }
        adding = true; status = null;
        Context app = getContext().getApplicationContext();
        InputStream stream = in;
        new Thread(() -> {
            DriverLibrary.Add a = DriverStore.addZip(app, stream);
            post(() -> added(a));
        }, "dm-driver-add").start();
        invalidate();
    }

    void addFailed(String reason) { status = AppText.get(getContext(), R.string.dmAndroidDriverInvalid, reason); invalidate(); }

    private void added(DriverLibrary.Add a) {
        adding = false;
        if (a.error != null) { status = AppText.get(getContext(), R.string.dmAndroidDriverInvalid, a.error); invalidate(); return; }
        status = alreadyThere(a.sha) ? null : AppText.get(getContext(), R.string.dmAndroidDriverAdded, a.name);
        refresh();
        focusSha(a.sha);
        invalidate();
    }
    private boolean alreadyThere(String s) { for (int i = 1; i < rows; i++) if (sha[i].equals(s)) return true; return false; }


    @Override public boolean padOpen() { return isOpen(); }
    @Override public void back() { close(); }
    @Override public void onPad(int cmd) {
        if (cmd == PadNav.B || cmd == PadNav.MENU) { close(); return; }
        if (!padFocus && (cmd == PadNav.A || (cmd >= PadNav.UP && cmd <= PadNav.RIGHT))) { padFocus = true; invalidate(); return; }
        if (cmd == PadNav.A) { activate(focus); return; }





        int buttons = rowFocus >= 1 ? iDelete : iAdd;
        if (cmd == PadNav.UP) setFocus(focus == iClose ? buttons : focus >= rows ? rowFocus : focus - 1);
        else if (cmd == PadNav.DOWN) setFocus(focus < rows - 1 ? focus + 1 : focus < rows ? buttons : iClose);
        else if (cmd == PadNav.LEFT && focus > iAdd && focus <= iRestart) setFocus(focus - 1);
        else if (cmd == PadNav.RIGHT && focus < rows) setFocus(buttons);
        else if (cmd == PadNav.RIGHT && focus >= iAdd && focus < iRestart) setFocus(focus + 1);
        ensureVisible();
        invalidate();
    }

    private void ensureVisible() {
        if (focus < scroll) scroll = focus;
        if (focus > scroll + visible - 1) scroll = focus - (visible - 1);
        clampScroll();
    }
    private void clampScroll() { scroll = Math.max(0, Math.min(scroll, Math.max(0, rows - visible))); }

    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }

    private float rowH, x0, xv, right;
    private void geom() {
        float h = getHeight() * 0.86f, w = Math.min(getWidth() * 0.60f, h * 1.35f);
        box.set((getWidth() - w) / 2, (getHeight() - h) / 2, (getWidth() + w) / 2, (getHeight() + h) / 2);
        panel.compute(getWidth(), getHeight(), GameStyleData.CONSOLE_UI_SCALE, 7f / 25.4f * getResources().getDisplayMetrics().ydpi);
        float bw = panel.modalW(), bh = panel.modalH(), th = Math.max(bh, panel.minTap) / 2, gap = bw * 0.12f;
        float cy = box.top + h * 0.735f;
        for (int i = 0; i < 3; i++) {
            float x = box.centerX() + (i - 1) * (bw + gap);
            actBtn[i].set(x - bw / 2, cy - bh / 2, x + bw / 2, cy + bh / 2);
            actHit[i].set(actBtn[i].left, cy - th, actBtn[i].right, cy + th);
        }
        cy = box.top + h * 0.905f;
        closeBtn.set(box.centerX() - bw / 2, cy - bh / 2, box.centerX() + bw / 2, cy + bh / 2);
        closeHit.set(closeBtn.left, Math.max(box.top, cy - th), closeBtn.right, Math.min(box.bottom, cy + th));
        x0 = box.left + w * 0.07f; right = box.right - w * 0.07f; xv = x0 + w * 0.015f;
        rowH = h * 0.049f;
        listBox.set(box.left, box.top + h * 0.165f, box.right, box.top + h * 0.165f + rowH * 8);
        visible = 8;
        clampScroll();
    }

    private int hitAt(float x, float y) {
        if (closeHit.contains(x, y)) return iClose;
        for (int i = 0; i < 3; i++) if (actHit[i].contains(x, y)) return iAdd + i;
        if (y >= listBox.top && y <= listBox.bottom && y < box.bottom && x >= box.left && x <= box.right) {
            int i = (int) (scroll + (y - listBox.top) / rowH);
            if (i >= 0 && i < rows) return i;
        }
        return -1;
    }

    @Override protected void onDraw(Canvas c) {
        if (lang.changed()) { head.setTypeface(art.role(AppArt.EXOCET)); row.setTypeface(art.role(AppArt.FORMAL)); refresh(); }
        c.drawColor(GameStyleGeometry.Panel.BACKDROP);
        geom();
        float H = getHeight(), h = box.height();
        art.nine(c, "build_modal", box, 60, H / 900f, paint);
        head.setTextSize(0.055f * H);
        TechPage.shadowed(c, t(R.string.dmAndroidDriverPage), box.centerX(), box.top + h * 0.10f + head.getTextSize() * 0.36f, head, goldYellow, shadowCol);
        int first = (int) scroll, last = Math.min(rows, first + visible + 1);
        c.save(); c.clipRect(listBox);
        for (int i = first; i < last; i++) {
            float y = listBox.top + (i - scroll) * rowH;
            String full = base[i] + tail[i];
            TechPage.fit(row, full, 0.031f * H, right - xv);
            float by = y + rowH / 2 + row.getTextSize() * 0.36f;
            if (rowRed[i]) TechPage.shadowed(c, full, xv, by, row, red, shadowCol);
            else {
                TechPage.shadowed(c, base[i], xv, by, row, white, shadowCol);
                if (!tail[i].isEmpty()) TechPage.shadowed(c, tail[i], xv + row.measureText(base[i]), by, row, lightGold, shadowCol);
            }
            if (padFocus && focus == i) {
                r.set(x0 - (right - x0) * 0.02f, y + 1, right + (right - x0) * 0.02f, y + rowH - 1);
                android.graphics.Bitmap f = art.image("pad/focus");
                if (f != null) c.drawBitmap(f, null, r, paint);
            }
        }
        c.restore();
        String[] lines = status != null ? new String[]{status}
            : !notice.isEmpty() ? TechPage.wrap(row, DriverStore.noticeText(getContext(), notice, noticeArg), 0.030f * H, right - x0, 2) : null;
        if (lines != null) {
            float hintSize = 0.030f * H;
            for (String line : lines) { TechPage.fit(row, line, 0.030f * H, right - x0); hintSize = Math.min(hintSize, row.getTextSize()); }
            row.setTextSize(hintSize);
            for (int i = 0; i < lines.length; i++)
                TechPage.shadowed(c, lines[i], x0, box.top + h * (0.61f + 0.045f * i) + row.getTextSize() * 0.36f, row, status != null ? white : red, shadowCol);
        }
        int[] names = { R.string.dmAndroidDriverAdd, R.string.dmAndroidDriverDelete, R.string.dmAndroidDriverRestart };
        for (int i = 0; i < 3; i++) {
            boolean off = restarting || adding || i == 1 && (rowFocus < 1 || rowFocus >= rows) || i == 2 && !canRestart();
            style.modalButton(c, actBtn[i], off ? GameStyle.BUTTON_DISABLED : held == iAdd + i ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, t(names[i]));
        }
        r.set(closeBtn);
        android.graphics.Bitmap b = art.image(down ? "button_down" : "button");
        if (b != null) c.drawBitmap(b, null, r, paint);
        head.setTextSize(closeBtn.height() / GameStyleGeometry.Panel.MODAL_H * GameStyleGeometry.Panel.textPx(1));
        TechPage.shadowed(c, t(R.string.dmAndroidQuickClose), closeBtn.centerX(), closeBtn.centerY() + head.getTextSize() * GameStyleData.CAPS_EM / 2, head, lightGold, shadowCol);
        if (padFocus) {
            RectF f0 = focus == iClose ? closeBtn : focus >= iAdd ? actBtn[focus - iAdd] : null;
            if (f0 != null) {
                float dx = f0.width() * 11f / 265f, dy = f0.height() * 10.5f / 51f;
                r.set(f0.left - dx, f0.top - dy, f0.right + dx, f0.bottom + dy);
                android.graphics.Bitmap f = art.image("pad/focus");
                if (f != null) c.drawBitmap(f, null, r, paint);
            }
        }
    }

    private boolean canRestart() { return !selected.equals(running); }

    @Override public boolean onTouchEvent(MotionEvent e) {
        geom();
        int a = e.getActionMasked();
        float x = e.getX(), y = e.getY();
        if (a == MotionEvent.ACTION_DOWN) {
            padFocus = false; held = hitAt(x, y); down = held == iClose; dragY = listBox.contains(x, y) ? y : -1; dragScroll = scroll; dragged = false;
            invalidate();
        } else if (a == MotionEvent.ACTION_MOVE) {
            if (dragY >= 0) {
                float dy = y - dragY;
                if (!dragged && Math.abs(dy) > rowH * 0.4f) dragged = true;
                if (dragged) { scroll = dragScroll - dy / rowH; clampScroll(); invalidate(); }
            }
        } else if (a == MotionEvent.ACTION_UP) {
            int on = !dragged && held >= 0 && hitAt(x, y) == held ? held : -1;
            down = false; held = -1; dragY = -1; dragged = false;
            if (on >= 0) { setFocus(on); activate(on); } else invalidate();
        } else if (a == MotionEvent.ACTION_CANCEL) { down = false; held = -1; dragY = -1; dragged = false; invalidate(); }
        return true;
    }


    @Override public void setVisibility(int visibility) { super.setVisibility(visibility); TestHooks.overlay(this, "driver_page", isOpen() ? "open" : "closed"); }
}
