package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import java.util.function.BooleanSupplier;

















final class AppMenu extends View implements PadPage {
    private static final String PREFS = "dm_quick", HINT = "hint";
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final GameStyle style;
    private final GameText title, button, announce, modal, keepText;
    private final GameText entry;
    private final TouchOverlay overlay;
    private final AppArt art;
    private final RecipeBook book;
    private final BuildBook builds;
    private final CombatPage combat;
    private final FeatsPage feats;
    private final BookPage monbook;
    private final GlossaryPage glossary;
    private final Runnable freeInput;
    private final QuickLayout layout = new QuickLayout();
    private final QuickModel model = new QuickModel();
    private final MenuFocus focus = new MenuFocus();
    private final RectF r = new RectF(), bar = new RectF();
    private boolean paused, pausing;
    private boolean padUsed;
    private boolean qaQuiet;
    private boolean hint, hintDown;
    private int want = 1;
    private final GameLang.Watch lang = new GameLang.Watch();



    private boolean leftApp;
    private int keep = -1;
    private boolean there;
    private final BooleanSupplier canOpen;
    private final int[] ui = new int[6];
    private boolean polling;
    private static final long POLL_MS = 200;
    static final int HIT_CAMP = 10;
    private final Runnable poll = this::pollThere;

    AppMenu(Context context, TouchOverlay overlay, AppArt art, GameStyle style, RecipeBook book, BuildBook builds, GlossaryPage glossary, CombatPage combat, FeatsPage feats, BookPage monbook, Runnable freeInput,
            BooleanSupplier canOpen) {
        super(context);
        this.canOpen = canOpen;
        this.overlay = overlay;
        this.art = art;
        this.book = book;
        this.builds = builds;
        this.glossary = glossary;
        this.combat = combat;
        this.feats = feats;
        this.monbook = monbook;
        this.freeInput = freeInput;
        this.style = style;
        title = style.text(GameStyleData.TITLE);
        button = style.text(GameStyleData.BUTTON_LABEL);
        entry = style.text(GameStyleData.PANEL_BUTTON);
        entry.prepare();
        announce = style.text(GameStyleData.ANNOUNCE).colour("FontColorGoldYellow");
        modal = style.text(GameStyleData.MODAL_TEXT);
        keepText = style.text(GameStyleData.BUTTON_LABEL);
        for (GameText g : new GameText[]{title, button, announce, modal, keepText}) g.prepare();
        initHint(context);
        setVisibility(GONE);
    }


    private String t(int id) { return AppText.get(getContext(), id); }



    private void initHint(Context c) {
        SharedPreferences p = c.getSharedPreferences(PREFS, Context.MODE_PRIVATE);
        if (p.contains(HINT)) return;
        boolean update = false;
        try { PackageInfo i = c.getPackageManager().getPackageInfo(c.getPackageName(), 0); update = i.firstInstallTime < i.lastUpdateTime; } catch (Exception e) {                                      }
        p.edit().putInt(HINT, update ? 1 : 2).apply();
    }
    private boolean hintDue() { return getContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE).getInt(HINT, 2) == 1; }




    void open(boolean byPad) {
        DmActivity.syncLang();
        boolean recipes = book.available();
        if (recipes) RecipeBook.warmText(getContext());
        model.setEntries(recipes, builds.available(), glossary.available(), combat.available(), feats.available(), monbook.available());
        model.cancel();
        int w = DmActivity.nativePlayersWanted();
        want = w > 0 ? w : overlay.players();
        pausing = true;
        padUsed = byPad;
        DmActivity.nativePause(DmActivity.PAUSE_MENU, 1);
        focus.setCells(cells());
        focus.open(byPad);
        hint = hintDue();
        setVisibility(VISIBLE);
        freeInput.run();
        invalidate();
        qa();
    }
    boolean isOpen() { return getVisibility() == VISIBLE; }
    boolean holdsQuickMenu() { return isOpen() && !paused; }


    void close() {
        paused = false;
        leftApp = false;
        there = false;
        hint = false; hintDown = false;
        if (pausing) { pausing = false; DmActivity.nativePause(DmActivity.PAUSE_MENU | DmActivity.PAUSE_LEFT, 0); }
        model.cancel();
        setVisibility(GONE);
    }

    @Override public boolean padOpen() { return isOpen(); }
    @Override public void back() { if (hint) dismissHint(); else close(); }


    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }




    private void toPage(QuickPage p) {
        p.takePause();
        pausing = false;
        DmActivity.nativePause(DmActivity.PAUSE_MENU, 0);
        p.open();
        setVisibility(GONE);
    }


    void background() {
        DmActivity.syncLang();
        qaQuiet = true;
        close();
        qaQuiet = false;
        if (DmActivity.nativeInGame() == 0) { qa(); return; }
        showLeftPause();
    }



    void showLeftPause() {
        pausing = true;
        paused = true;
        leftApp = true;
        padUsed = PadInput.isPadMode();
        DmActivity.nativePause(DmActivity.PAUSE_LEFT, 1);
        setVisibility(VISIBLE);
        invalidate();
    }


    void start() { if (!polling) { polling = true; removeCallbacks(poll); postDelayed(poll, POLL_MS); } }
    void stop() { polling = false; removeCallbacks(poll); }
    private void pollThere() {
        if (!polling) return;
        if (!isOpen() && DmActivity.hostStarted() && DmActivity.nativeInGame() != 0 && DmActivity.nativeUiRect(ui) != 0 && canOpen.getAsBoolean()) {
            int lv = DmActivity.nativeResumeThere();
            if (lv > 0) showThere(lv);
        }
        postDelayed(poll, POLL_MS);
    }
    private void showThere(int level) {
        DmActivity.syncLang();
        pausing = true;
        paused = true;
        leftApp = false;
        there = true;
        padUsed = PadInput.isPadMode();
        DmActivity.nativePause(DmActivity.PAUSE_RESUME, 1);
        setVisibility(VISIBLE);
        freeInput.run();
        invalidate();
        AppArt.log("resume: pause screen in area " + level);
    }

    private void leaveThere(boolean camp) {
        int taken = DmActivity.nativeResumeLeave(camp ? 1 : 0);
        close();
        DmActivity.nativePause(DmActivity.PAUSE_RESUME, 0);
        AppArt.log("resume: " + (camp ? "to camp" : "continue") + (taken != 0 ? "" : " (already left)"));
    }
    private void resumeTap() { if (there) leaveThere(false); else close(); }

    void keepStatus(int status) { if (status != keep) { keep = status; invalidate(); } }




    private void keepLine(Canvas c) {
        if (!leftApp || keep < 0) return;
        int id = keep == KeepAliveStatus.OK ? R.string.dmAndroidKeepOk : keep == KeepAliveStatus.NO_NOTIFY ? R.string.dmAndroidKeepNoNotify
            : keep == KeepAliveStatus.BATTERY ? R.string.dmAndroidKeepBattery : keep == KeepAliveStatus.RESTRICTED ? R.string.dmAndroidKeepRestricted : R.string.dmAndroidKeepFailed;
        String t = t(id);
        float h = getHeight(), u = h / 2160f, size = Math.max(h * 0.03f, 22 * u), k = size / keepText.units();
        k = keepText.fitTo(t, getWidth() * 0.9f, k);
        keepText.colourArgb(KeepAliveStatus.colour(keep));
        keepText.draw(c, t, getWidth() / 2f, h * 0.67f, k, Paint.Align.CENTER, 255);
        if (allowShown()) screenButton(c);
    }





    private final RectF allowBtn = new RectF(), allowHit = new RectF();
    private boolean allowDown, allowArmed;
    private boolean allowShown() { return paused && leftApp && keep == KeepAliveStatus.BATTERY && !hint; }
    private boolean campShown() { return paused && there && !hint; }
    private boolean buttonShown() { return allowShown() || campShown(); }
    private void allowGeom() {

        GameStyleGeometry.Panel p = style.panel(getWidth(), getHeight());
        float h = getHeight(), bw = p.modalW(), bh = p.modalH(), cy = h * 0.67f + h * 0.08f, th = Math.max(bh, p.minTap) / 2;
        allowBtn.set(getWidth() / 2f - bw / 2, cy - bh / 2, getWidth() / 2f + bw / 2, cy + bh / 2);
        allowHit.set(allowBtn.left, cy - th, allowBtn.right, cy + th);
    }
    private void screenButton(Canvas c) {
        allowGeom();
        if (BuildConfig.DM_QA_BUILD && campShown()) TestHooks.hit(allowHit, HIT_CAMP, 0);
        String s = t(campShown() ? R.string.dmAndroidResumeCamp : R.string.dmAndroidKeepAllow);
        if (!padUsed) { style.modalButton(c, allowBtn, allowDown ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, s); return; }



        style.modalButton(c, allowBtn, GameStyle.BUTTON_NORMAL, null);
        float lh = allowBtn.height() * 0.62f;
        Paint legendText = button.solid(allowBtn.height() * 0.42f, 255);
        Bitmap gx = art.glyph(PadInput.glyph(), "x");
        Object[] parts = gx != null ? new Object[]{gx, s} : new Object[]{s};
        float w = PadLegend.width(lh, legendText, parts);
        bar.set(allowBtn.centerX() - w / 2, allowBtn.centerY() - lh / 2, allowBtn.centerX() + w / 2, allowBtn.centerY() + lh / 2);
        PadLegend.draw(c, bar, null, paint, legendText, parts);
    }
    private void allow() { if (getContext() instanceof DmActivity) ((DmActivity) getContext()).allowBattery(); }
    private void pressButton() { if (campShown()) leaveThere(true); else if (allowShown()) allow(); }


    private int[][] cells() {
        int[][] c = new int[model.count()][];
        for (int i = 0; i < c.length; i++) c[i] = new int[]{model.id(i), i, 0, QuickModel.kind(model.id(i)) == QuickModel.VALUE ? MenuFocus.VALUE : MenuFocus.ACTION};
        return c;
    }

    private GameStyleGeometry.Panel relayout() {
        GameStyleGeometry.Panel p = style.panel(getWidth(), getHeight());
        layout.compute(p, model.count());
        return p;
    }



    private static void line(Canvas c, GameText g, String s, float x, float cy, float px) {
        if (px > 0 && g.units() > 0) g.draw(c, s, x, cy + px * 0.36f, px / g.units(), Paint.Align.CENTER, 255);
    }

    private void image(Canvas c, String name, RectF at) { Bitmap b = art.image(name); if (b != null) c.drawBitmap(b, null, at, paint); }

    @Override protected void onDraw(Canvas c) {
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "app_menu");
        try {
        if (lang.changed()) style.refresh();
        style.backdrop(c);
        if (paused) { pauseScreen(c); return; }
        GameStyleGeometry.Panel p = relayout();
        QuickLayout l = layout;
        style.panelFrame(c, p);
        style.panelTab(c, p, 1, 0, t(R.string.dmAndroidQuickTitle), true);
        style.closeX(c, p);
        if (BuildConfig.DM_QA_BUILD) { r.set(p.hitCloseL, p.hitCloseT, p.hitCloseR, p.hitCloseB); TestHooks.hit(r, QuickModel.CLOSE, 0); }
        boolean padFocus = focus.shown() && !hint;
        for (int i = 0; i < model.count(); i++) {
            int id = model.id(i);
            float top = l.btnTop(i);
            r.set(l.bx, top, l.bx + l.bw, top + l.bh);
            if (BuildConfig.DM_QA_BUILD) TestHooks.hit(r, id, 0);
            boolean held = model.heldIndex() == i;
            if (id != QuickModel.PLAYERS) {
                String s = id == QuickModel.COMBAT ? CombatPage.menuName() : id == QuickModel.MONBOOK ? t(R.string.dmAndroidBookMenu) : id == QuickModel.FEATS ? t(R.string.dmAndroidFeatsMenu) : t(id == QuickModel.PAUSE ? R.string.dmAndroidQuickPause : id == QuickModel.RECIPES ? R.string.dmAndroidQuickRecipes
                    : id == QuickModel.BUILDS ? R.string.dmAndroidBuildMenu : R.string.dmAndroidGlossMenu);
                style.panelButton(c, r, held ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, s);
            } else players(c, r, padFocus && focus.focused() == id, held);
            if (padFocus && focus.focused() == id) {
                float k = l.bh / 60f;
                bar.set(r.left - 9 * k - 1.5f * k, r.top - 9 * k, r.right + 9 * k - 1.5f * k, r.bottom + 9 * k);
                image(c, "quick/focus", bar);
            }
        }
        if (hint) hintBox(c);
        else if (focus.shown()) legend(c);
        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }



    private void players(Canvas c, RectF r, boolean focused, boolean held) {
        style.panelButton(c, r, GameStyle.BUTTON_NORMAL, null);
        float k = r.height() / 60f;
        float aw = 27 * k, ah = 22 * k, inset = 26 * k, cy = r.centerY();
        for (int side = 0; side < 2; side++) {
            int dir = side == 0 ? -1 : 1;
            int frame = QuickModel.atEnd(want, dir) ? 2 : held && model.heldHalf() == side ? 1 : focused ? 3 : 0;
            float x0 = side == 0 ? r.left + inset : r.right - inset - aw;
            bar.set(x0, cy - ah / 2, x0 + aw, cy + ah / 2);
            image(c, "quick/arrow_" + (side == 0 ? "l" : "r") + "_" + frame, bar);
        }
        String s = String.format(t(R.string.dmAndroidQuickPlayers), want);
        float size = r.height() / GameStyleGeometry.Panel.BUTTON_H * GameStyleGeometry.Panel.textPx(1), base = r.top + r.height() * GameStyleGeometry.Panel.BUTTON_BASE / GameStyleGeometry.Panel.BUTTON_H;
        float room = r.width() - 2 * (inset + aw + 8 * k);
        float min = 0.026f * getHeight(), w = entry.measure(s, size / entry.units());
        if (w > room) size = Math.max(min, size * room / w);
        entry.draw(c, s, r.centerX(), base, size / entry.units(), Paint.Align.CENTER, 255);
    }


    private void legend(Canvas c) {
        int g = PadInput.glyph();
        Object[] parts = focus.kind() == MenuFocus.VALUE
            ? new Object[]{art.glyph(g, "a"), t(R.string.dmAndroidQuickLegendSelect), null, art.glyph(g, "b"), t(R.string.dmAndroidBack), null,
                art.glyph(g, "left"), art.glyph(g, "right"), t(R.string.dmAndroidQuickLegendChange), null, art.glyph(g, "start"), t(R.string.dmAndroidQuickClose)}
            : new Object[]{art.glyph(g, "a"), t(R.string.dmAndroidQuickLegendSelect), null, art.glyph(g, "b"), t(R.string.dmAndroidBack), null, art.glyph(g, "start"), t(R.string.dmAndroidQuickClose)};
        style.footLegend(c, style.panel(getWidth(), getHeight()), getWidth(), parts);
    }


    private void pauseScreen(Canvas c) {
        float size = getHeight() * 0.12f;
        announce.draw(c, t(R.string.dmAndroidQuickPaused), getWidth() / 2f, getHeight() * 0.48f, size / announce.units(), Paint.Align.CENTER, 255);
        if (campShown()) screenButton(c);
        if (!padUsed) { button.draw(c, t(R.string.dmAndroidQuickResume), getWidth() / 2f, getHeight() * 0.58f, getHeight() * 0.04f / button.units(), Paint.Align.CENTER, 255); keepLine(c); return; }

        String s = t(R.string.dmAndroidQuickResumePad);
        int at = s.indexOf("%s");
        Object[] parts = at < 0 ? new Object[]{s} : new Object[]{s.substring(0, at).trim(), art.glyph(PadInput.glyph(), "a"), s.substring(at + 2).trim()};
        if (parts.length == 3 && ((String) parts[0]).isEmpty()) parts = new Object[]{parts[1], parts[2]};
        float h = getHeight() * 0.058f;
        Paint legendText = button.solid(getHeight() * 0.04f, 255);
        float w = PadLegend.width(h, legendText, parts) + h * 2;
        bar.set(getWidth() / 2f - w / 2, getHeight() * 0.58f - h * 0.75f, getWidth() / 2f + w / 2, getHeight() * 0.58f + h * 0.25f);
        PadLegend.draw(c, bar, art.image("pad/footer"), paint, legendText, parts);
        keepLine(c);
    }


    private final RectF hintBox = new RectF(), hintBtn = new RectF(), hintHit = new RectF();
    private static final float HINT_BAND = 0.09f;
    private void hintGeom() {
        float w = Math.min(getWidth() * 0.56f, getHeight() * 1.23f), h = w * 226f / 622f;
        hintBox.set((getWidth() - w) / 2, (getHeight() * (1 - HINT_BAND) - h) / 2, (getWidth() + w) / 2, (getHeight() * (1 - HINT_BAND) + h) / 2);


        GameStyleGeometry.Panel p = style.panel(getWidth(), getHeight());
        float bw = p.modalW(), bh = p.modalH(), cy = hintBox.top + h * 0.80f;
        hintBtn.set(hintBox.centerX() - bw / 2, cy - bh / 2, hintBox.centerX() + bw / 2, cy + bh / 2);
        float th = Math.max(bh, p.minTap) / 2;
        hintHit.set(hintBtn.left, Math.max(hintBox.top, cy - th), hintBtn.right, Math.min(hintBox.bottom, cy + th));
    }
    private void hintBox(Canvas c) {
        hintGeom();
        style.backdrop(c);
        image(c, "quick/modal_bg", hintBox);
        float h = hintBox.height(), s = h / 226f;
        line(c, title, t(R.string.dmAndroidQuickMovedTitle), hintBox.centerX(), hintBox.top + h * 0.115f, 19 * s);
        String[] lines = {t(ShoulderKeys.hasTriggers(getContext()) ? R.string.dmAndroidQuickMovedShoulder : R.string.dmAndroidQuickMovedTech), t(R.string.dmAndroidQuickMovedVideo), t(R.string.dmAndroidQuickMovedStay)};
        for (int i = 0; i < lines.length; i++) line(c, modal, lines[i], hintBox.centerX(), hintBox.top + h * (0.36f + 0.115f * i), 13 * s);
        style.modalButton(c, hintBtn, hintDown ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, t(R.string.dmAndroidQuickMovedOk));
        if (focus.shown() || padUsed) {
            float dx = hintBtn.width() * 11f / 265f, dy = hintBtn.height() * 10.5f / 51f;
            bar.set(hintBtn.left - dx, hintBtn.top - dy, hintBtn.right + dx, hintBtn.bottom + dy);
            image(c, "pad/focus", bar);
            style.footLegend(c, style.panel(getWidth(), getHeight()), getWidth(), art.glyph(PadInput.glyph(), "a"), t(R.string.dmAndroidQuickMovedOk));
        }
    }

    private void dismissHint() {
        hint = false; hintDown = false;
        getContext().getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().putInt(HINT, 2).apply();
        invalidate();
        qa();
    }


    @Override public void onPad(int cmd) {
        padUsed = true;
        if (paused) { if (cmd == PadNav.X && buttonShown()) pressButton(); else if (cmd == PadNav.A || cmd == PadNav.B || cmd == PadNav.MENU) resumeTap(); invalidate(); return; }
        if (hint) { if (cmd == PadNav.A || cmd == PadNav.B || cmd == PadNav.MENU) dismissHint(); else invalidate(); return; }
        focus.setCells(cells());
        switch (focus.handle(cmd)) {
            case MenuFocus.R_CLOSE: close(); break;
            case MenuFocus.R_ACTIVATE: run(focus.focused(), 1); break;
            case MenuFocus.R_STEP_BACK: step(-1); break;
            case MenuFocus.R_STEP_FORWARD: step(cmd == PadNav.A ? 0 : 1); break;
            default: break;
        }
        invalidate();
        qa();
    }


    private void run(int id, int half) {
        if (id == QuickModel.PAUSE) paused = true;
        else if (id == QuickModel.RECIPES) toPage(book);
        else if (id == QuickModel.BUILDS) toPage(builds);
        else if (id == QuickModel.GLOSSARY) toPage(glossary);
        else if (id == QuickModel.COMBAT) toPage(combat);
        else if (id == QuickModel.FEATS) toPage(feats);
        else if (id == QuickModel.MONBOOK) toPage(monbook);
        else if (id == QuickModel.PLAYERS) step(half == 0 ? -1 : 1);
    }


    private void step(int dir) {
        int to = QuickModel.players(want, dir);
        if (to == want) return;
        want = to;
        DmActivity.nativePlayers(want);
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        int action = e.getActionMasked();
        if (paused) {
            if (action == MotionEvent.ACTION_DOWN) { allowGeom(); allowArmed = buttonShown() && allowHit.contains(e.getX(), e.getY()); allowDown = allowArmed; if (allowArmed) padUsed = false; invalidate(); }
            else if (action == MotionEvent.ACTION_MOVE && allowArmed) { boolean on = allowHit.contains(e.getX(), e.getY()); if (on != allowDown) { allowDown = on; invalidate(); } }
            else if (action == MotionEvent.ACTION_UP) { if (!allowArmed) resumeTap(); else if (allowHit.contains(e.getX(), e.getY()) && buttonShown()) pressButton(); allowArmed = allowDown = false; invalidate(); }
            else if (action == MotionEvent.ACTION_CANCEL) { allowArmed = allowDown = false; invalidate(); }
            return true;
        }
        if (hint) {
            hintGeom();
            if (action == MotionEvent.ACTION_DOWN) { focus.hide(); padUsed = false; hintDown = hintHit.contains(e.getX(), e.getY()); invalidate(); }
            else if (action == MotionEvent.ACTION_UP) { boolean onBtn = hintHit.contains(e.getX(), e.getY()), inBox = hintBox.contains(e.getX(), e.getY());
                if ((hintDown && onBtn) || (!hintDown && !inBox)) dismissHint(); else { hintDown = false; invalidate(); } }
            else if (action == MotionEvent.ACTION_CANCEL) { hintDown = false; invalidate(); }
            return true;
        }
        relayout();
        int hit = layout.hit(e.getX(), e.getY());
        if (action == MotionEvent.ACTION_DOWN) { focus.hide(); model.press(hit); invalidate(); }
        else if (action == MotionEvent.ACTION_MOVE) { int was = model.heldIndex(); model.move(hit); if (was != model.heldIndex()) invalidate(); }
        else if (action == MotionEvent.ACTION_UP) {
            int tap = model.release(hit);
            if (tap == QuickModel.TAP_CLOSE) close();
            else if (tap >= 0) run(tap / 2, tap % 2);
            invalidate();
            qa();
        } else if (action == MotionEvent.ACTION_CANCEL) { model.cancel(); invalidate(); }
        return true;
    }





    private String qaState() { return !isOpen() ? "closed" : paused ? (there ? "there" : "paused") : hint ? "hint" : "open"; }
    private void qa() { if (!qaQuiet) TestHooks.overlay(this, "app_menu", qaState()); }
    @Override public void setVisibility(int visibility) { super.setVisibility(visibility); qa(); }
}
