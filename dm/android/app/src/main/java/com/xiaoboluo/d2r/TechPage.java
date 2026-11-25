package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.function.BooleanSupplier;
























final class TechPage extends View implements PadPage {
    private static final long POLL_MS = 100;
    private final AppArt art;
    private final Runnable freeInput;
    private final Runnable openDrivers;
    private final BooleanSupplier canOpen;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), head = new Paint(Paint.ANTI_ALIAS_FLAG), text = new Paint(Paint.ANTI_ALIAS_FLAG), value = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF box = new RectF(), btn = new RectF(), btnHit = new RectF(), r = new RectF();
    private final GameStyleGeometry.Panel panel = new GameStyleGeometry.Panel();
    private final GameStyle style;
    private final File game;
    private final RectF[] saveBtn = { new RectF(), new RectF() }, saveHit = { new RectF(), new RectF() };

    private final RectF filesBtn = new RectF(), filesHit = new RectF();
    private static final int EXPORT = 0, IMPORT = 1, CLOSE = 2, FILES = 3;
    private static final boolean FILES_OFFERED = DmActivity.PACK_BUILD_OFFERED;
    private int focus = CLOSE, armed = -1, held = -1;
    private boolean importReady, restarting;
    private String status;
    private final int lightGold, goldYellow, white, note, red, shadowCol = 0xC8000000;
    private boolean running, pausing, down, padFocus;
    private String[][] now = new String[0][];



    private String notice = "";
    private boolean noticeOpened;
    private int ticks;
    private final Runnable poll = this::pollTick;

    TechPage(Context context, AppArt art, GameStyle style, File game, Runnable freeInput, Runnable openDrivers, BooleanSupplier canOpen) {
        super(context);
        this.art = art;
        this.style = style;
        this.game = game;
        this.freeInput = freeInput;
        this.openDrivers = openDrivers;
        this.canOpen = canOpen;
        lightGold = art.uiColour("FontColorLightGold"); goldYellow = art.uiColour("FontColorGoldYellow"); white = art.uiColour("FontColorWhite"); note = art.uiColour("FontColorDarkGrayGold");
        red = style.colour("FontColorRed");
        head.setTypeface(art.role(AppArt.EXOCET)); head.setTextAlign(Paint.Align.CENTER);
        text.setTypeface(art.role(AppArt.FORMAL));
        value.setTypeface(art.role(AppArt.EXOCET));
        setVisibility(GONE);
    }

    private String t(int id) { return AppText.get(getContext(), id); }
    private final GameLang.Watch lang = new GameLang.Watch();

    boolean isOpen() { return getVisibility() == VISIBLE; }
    void start() {

        if (isOpen() && !restarting) { importReady = new File(game, SaveTransfer.IMPORT_DIR).isDirectory(); invalidate(); }
        if (!running) { running = true; removeCallbacks(poll); postDelayed(poll, POLL_MS); }
    }
    void stop() { running = false; removeCallbacks(poll); }
    private void pollTick() {
        if (!running) return;
        if (DmActivity.hostStarted() && DmActivity.nativeTechRequest() != 0 && !isOpen() && canOpen.getAsBoolean()) open();


        if (DmActivity.hostStarted() && DmActivity.nativeDriverRequest() != 0 && !isOpen() && canOpen.getAsBoolean()) openDrivers.run();


        if (DmActivity.hostStarted() && ++ticks % 10 == 0) {
            DriverStore.poll(getContext());
            if (!noticeOpened && ticks >= 80 && !isOpen() && canOpen.getAsBoolean() && DriverStore.noticeDue(getContext())) { noticeOpened = true; open(); }
        }
        postDelayed(poll, POLL_MS);
    }

    void open() {
        DmActivity.syncLang();
        now = facts();
        if (DmActivity.nativeInGame() != 0) { pausing = true; DmActivity.nativePause(DmActivity.PAUSE_TECH, 1); }
        padFocus = PadInput.isPadMode();
        down = false;
        focus = CLOSE; armed = -1; held = -1; status = null;
        importReady = new File(game, SaveTransfer.IMPORT_DIR).isDirectory();
        setVisibility(VISIBLE);
        freeInput.run();
        invalidate();
    }
    void close() {
        if (restarting) return;
        armed = -1;
        if (pausing) { pausing = false; DmActivity.nativePause(DmActivity.PAUSE_TECH, 0); }
        if (!notice.isEmpty()) { DriverStore.noticeShown(getContext()); notice = ""; }
        setVisibility(GONE);
    }


    private String[][] facts() {
        String pack = "–";
        try {
            String info = new String(Files.readAllBytes(new File(getContext().getFilesDir(), "dm/pack_info.txt").toPath()), StandardCharsets.UTF_8), product = null, made = null;
            for (String line : info.split("\n")) { if (line.startsWith("PRODUCT=")) product = line.substring(8).trim(); else if (line.startsWith("MADE=")) made = line.substring(5).trim().replace('T', ' '); }
            if (made != null && made.length() > 16) made = made.substring(0, 16);
            if (product != null) pack = product + (made != null ? " · " + made : "") + (FILES_OFFERED ? " · " + t(phoneBuilt() ? R.string.dmAndroidTechSourcePhone : R.string.dmAndroidTechSourcePc) : "");
        } catch (IOException e) { pack = "–"; }
        String data = DmActivity.nativeDataSource();
        if (data == null || data.isEmpty()) data = "–";
        for (String p : new String[]{"/storage/emulated/0/", "/sdcard/"}) if (data.startsWith(p)) { data = data.substring(p.length()); break; }


        DriverStore.Info d = DriverStore.info(getContext());
        boolean own = d.state.startsWith("custom");
        String gpu = DmActivity.nativeGpuLabel(), driver = t(own ? R.string.dmAndroidDriverOwn : R.string.dmAndroidTechSystem) + (gpu != null && !gpu.isEmpty() ? " · " + gpu : "");
        notice = DriverStore.noticeText(getContext(), d.notice, d.noticeArg);
        String[] app = {t(R.string.dmAndroidTechApp), BuildConfig.VERSION_NAME + " (" + BuildConfig.VERSION_CODE + ")"}, pk = {t(R.string.dmAndroidTechPack), pack},
            gd = {t(R.string.dmAndroidTechData), data}, dr = {t(R.string.dmAndroidTechDriver), driver};
        if (!d.selected || d.label.isEmpty()) return new String[][]{app, pk, gd, dr};
        String state = t(own ? R.string.dmAndroidDriverRunning : R.string.dmAndroidDriverIdle);
        return new String[][]{app, pk, gd, dr, {t(R.string.dmAndroidDriverPackage), d.label + (d.sha.isEmpty() ? "" : " · " + d.sha) + " · " + state}};
    }


    private boolean phoneBuilt() { return PackBuildService.phonePack(new File(getContext().getFilesDir(), "dm")); }

    @Override public boolean padOpen() { return isOpen(); }
    @Override public void back() { close(); }

    @Override public void onPad(int cmd) {
        if (cmd == PadNav.B || cmd == PadNav.MENU) { close(); return; }
        if (!padFocus && (cmd == PadNav.A || (cmd >= PadNav.UP && cmd <= PadNav.RIGHT))) { padFocus = true; invalidate(); return; }
        if (cmd == PadNav.A) { activate(focus); return; }
        if (cmd == PadNav.UP && (focus == CLOSE || focus == FILES)) focus = EXPORT;
        else if (cmd == PadNav.DOWN) focus = CLOSE;
        else if (cmd == PadNav.LEFT && focus == CLOSE && FILES_OFFERED) focus = FILES;
        else if (cmd == PadNav.RIGHT && focus == FILES) focus = CLOSE;
        else if (cmd == PadNav.LEFT && focus == IMPORT) focus = EXPORT;
        else if (cmd == PadNav.RIGHT && focus == EXPORT) focus = IMPORT;
        invalidate();
    }

    private static String gamePath(String name) { return DmActivity.GAME_DIR + name; }


    private void activate(int which) {
        if (which == CLOSE) { close(); return; }
        if (restarting || (which == IMPORT && !importReady)) return;
        if (armed != which) { armed = which; status = null; invalidate(); return; }
        if (which == FILES) { toGameFiles(); return; }
        File marker = new File(game, SaveTransfer.EXPORT_MARKER);
        boolean made = false;
        if (which == EXPORT) {
            try { game.mkdirs(); made = !marker.exists() && marker.createNewFile(); if (!made && !marker.exists()) throw new IOException("create"); }
            catch (IOException | SecurityException e) { armed = -1; status = AppText.get(getContext(), R.string.dmAndroidTechNoAccess, gamePath(SaveTransfer.EXPORT_MARKER)); invalidate(); return; }
        }
        boolean remove = made;
        restarting = true; armed = -1; status = t(R.string.dmAndroidTechRestarting);
        AppArt.log("save transfer: " + (which == EXPORT ? "export" : "import") + " from the tech page, saving and restarting");
        Quit.restart(getContext(), "tech page " + (which == EXPORT ? "export" : "import"), () -> post(() -> {
            if (remove) marker.delete();
            restarting = false; status = t(R.string.dmAndroidTechSaveFailed); invalidate();
        }));
        invalidate();
    }

    private void toGameFiles() {
        File mark = new File(PackBuildService.dir(getContext()), DmActivity.GAME_FILES_NEXT);
        try { DriverLibrary.writeFile(mark, "1", PackBuildService.SYNC); }
        catch (IOException e) { armed = -1; AppArt.log("tech page: the game files mark cannot be written (" + e.getClass().getSimpleName() + ")"); status = t(R.string.dmAndroidTechSaveFailed); invalidate(); return; }
        restarting = true; armed = -1; status = t(R.string.dmAndroidTechRestarting);
        AppArt.log("tech page: saving and restarting into the game files page");
        Quit.restart(getContext(), "tech page game files", () -> post(() -> {
            if (!mark.delete()) AppArt.log("tech page: the game files mark cannot be removed");
            restarting = false; status = t(R.string.dmAndroidTechSaveFailed); invalidate();
        }));
        invalidate();
    }

    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }

    private void geom() {
        float h = getHeight() * 0.86f, w = Math.min(getWidth() * 0.60f, h * 1.35f);
        box.set((getWidth() - w) / 2, (getHeight() - h) / 2, (getWidth() + w) / 2, (getHeight() + h) / 2);


        panel.compute(getWidth(), getHeight(), GameStyleData.CONSOLE_UI_SCALE, 7f / 25.4f * getResources().getDisplayMetrics().ydpi);
        float bw = panel.modalW(), bh = panel.modalH(), cy = box.top + h * 0.905f, th = Math.max(bh, panel.minTap) / 2;
        float cx = FILES_OFFERED ? box.centerX() + (bw + bw * 0.12f) / 2 : box.centerX();
        btn.set(cx - bw / 2, cy - bh / 2, cx + bw / 2, cy + bh / 2);
        btnHit.set(btn.left, Math.max(box.top, cy - th), btn.right, Math.min(box.bottom, cy + th));
        filesBtn.set(btn.left - bw * 1.12f, btn.top, btn.right - bw * 1.12f, btn.bottom);
        filesHit.set(filesBtn.left, btnHit.top, filesBtn.right, btnHit.bottom);
        float sy = box.top + h * 0.625f, gap = bw * 0.12f;
        for (int i = 0; i < saveBtn.length; i++) {
            float x = box.centerX() + (i - (saveBtn.length - 1) / 2f) * (bw + gap);
            saveBtn[i].set(x - bw / 2, sy - bh / 2, x + bw / 2, sy + bh / 2);
            saveHit[i].set(saveBtn[i].left, sy - th, saveBtn[i].right, sy + th);
        }
    }

    static void shadowed(Canvas c, String s, float x, float y, Paint p, int colour, int shadow) {
        float off = p.getTextSize() * 3f / 32f;
        p.setColor(shadow); c.drawText(s, x + off, y + off, p);
        p.setColor(colour); c.drawText(s, x, y, p);
        TestHooks.text(c, s, x, y, p);
    }


    static final int HIT_CLOSE = 1, HIT_FILES = 2;
    static void fit(Paint p, String s, float size, float room) { p.setTextSize(size); while (p.getTextSize() > size * 0.6f && p.measureText(s) > room) p.setTextSize(p.getTextSize() - 1); }



    static String[] wrap(Paint p, String s, float size, float room, int max) {
        for (float k = 1f; ; k -= 0.05f) {
            p.setTextSize(size * k);
            java.util.ArrayList<String> out = new java.util.ArrayList<>();
            int start = 0;
            while (start < s.length()) {
                int end = start + p.breakText(s, start, s.length(), true, room, null);
                if (end >= s.length()) { out.add(s.substring(start)); break; }
                int sp = s.lastIndexOf(' ', end);
                if (sp > start) end = sp;
                if (end <= start) end = start + 1;
                out.add(s.substring(start, end).trim());
                start = end;
                while (start < s.length() && s.charAt(start) == ' ') start++;
            }
            if (out.size() <= max || k <= 0.76f) return out.subList(0, Math.min(max, out.size())).toArray(new String[0]);
        }
    }

    @Override protected void onDraw(Canvas c) {
        if (lang.changed()) { head.setTypeface(art.role(AppArt.EXOCET)); text.setTypeface(art.role(AppArt.FORMAL)); value.setTypeface(art.role(AppArt.EXOCET)); now = facts(); }
        TestHooks.begin(this, "tech_page");
        c.drawColor(GameStyleGeometry.Panel.BACKDROP);
        geom();
        float H = getHeight(), h = box.height(), w = box.width();
        art.nine(c, "build_modal", box, 60, H / 900f, paint);
        head.setTextSize(0.055f * H);
        shadowed(c, t(R.string.dmAndroidTechTitle), box.centerX(), box.top + h * 0.10f + head.getTextSize() * 0.36f, head, goldYellow, shadowCol);
        float x0 = box.left + w * 0.08f, xv = box.left + w * 0.34f, right = box.right - w * 0.07f;
        text.setTextAlign(Paint.Align.LEFT);
        text.setTextSize(0.036f * H);
        shadowed(c, t(R.string.dmAndroidTechNow), x0, box.top + h * 0.165f + text.getTextSize() * 0.36f, text, goldYellow, shadowCol);
        for (int i = 0; i < now.length; i++) {
            float y = box.top + h * (0.225f + (now.length > 4 ? 0.045f : 0.052f) * i);
            fit(text, now[i][0], 0.031f * H, xv - x0 - w * 0.02f);
            shadowed(c, now[i][0], x0, y + text.getTextSize() * 0.36f, text, lightGold, shadowCol);
            fit(value, now[i][1], 0.029f * H, right - xv);
            shadowed(c, now[i][1], xv, y + value.getTextSize() * 0.36f, value, white, shadowCol);
        }
        text.setTextSize(0.036f * H);
        shadowed(c, t(R.string.dmAndroidSaveTitle), x0, box.top + h * 0.445f + text.getTextSize() * 0.36f, text, goldYellow, shadowCol);
        String[] hint = status != null ? new String[]{status}
            : armed >= 0 ? new String[]{t(armed == EXPORT ? R.string.dmAndroidTechExportConfirm : armed == FILES ? R.string.dmAndroidTechGameFilesConfirm : R.string.dmAndroidTechImportConfirm)}
            : new String[]{AppText.get(getContext(), R.string.dmAndroidTechExportWhere, gamePath(SaveTransfer.EXPORT_DIR)),
                AppText.get(getContext(), importReady ? R.string.dmAndroidTechImportReady : R.string.dmAndroidTechImportNone, gamePath(SaveTransfer.IMPORT_DIR))};
        float hintSize = 0.030f * H;
        for (String line : hint) { fit(text, line, 0.030f * H, right - x0); hintSize = Math.min(hintSize, text.getTextSize()); }
        text.setTextSize(hintSize);
        for (int i = 0; i < hint.length; i++) {
            shadowed(c, hint[i], x0, box.top + h * (0.50f + 0.045f * i) + text.getTextSize() * 0.36f, text, status != null || armed >= 0 ? white : lightGold, shadowCol);
        }
        int[] names = { R.string.dmAndroidTechExport, R.string.dmAndroidTechImport };
        for (int i = 0; i < names.length; i++) {
            boolean off = restarting || (i == IMPORT && !importReady);
            style.modalButton(c, saveBtn[i], off ? GameStyle.BUTTON_DISABLED : held == i || armed == i ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, t(names[i]));
        }
        if (!notice.isEmpty()) {
            String[] lines = wrap(text, notice, 0.030f * H, right - x0, 3);
            for (int i = 0; i < lines.length; i++) shadowed(c, lines[i], x0, box.top + h * (0.715f + 0.045f * i) + text.getTextSize() * 0.36f, text, red, shadowCol);
        } else {
            text.setTextSize(0.036f * H);
            shadowed(c, t(R.string.dmAndroidTechLater), x0, box.top + h * 0.715f + text.getTextSize() * 0.36f, text, note, shadowCol);


            String[] later = {t(R.string.dmAndroidTechLaterData)};
            for (int i = 0; i < later.length; i++) {
                fit(text, "· " + later[i], 0.030f * H, right - x0);
                shadowed(c, "· " + later[i], x0, box.top + h * (0.765f + 0.045f * i) + text.getTextSize() * 0.36f, text, note, shadowCol);
            }
        }
        if (FILES_OFFERED) style.modalButton(c, filesBtn, restarting ? GameStyle.BUTTON_DISABLED : held == FILES || armed == FILES ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, t(R.string.dmAndroidPbTitle));
        r.set(btn);
        android.graphics.Bitmap b = art.image(down ? "button_down" : "button");
        if (b != null) c.drawBitmap(b, null, r, paint);
        head.setTextSize(btn.height() / GameStyleGeometry.Panel.MODAL_H * GameStyleGeometry.Panel.textPx(1));
        shadowed(c, t(R.string.dmAndroidQuickClose), btn.centerX(), btn.centerY() + head.getTextSize() * GameStyleData.CAPS_EM / 2, head, lightGold, shadowCol);
        if (padFocus) {
            RectF f0 = focus == CLOSE ? btn : focus == FILES ? filesBtn : saveBtn[focus];
            float dx = f0.width() * 11f / 265f, dy = f0.height() * 10.5f / 51f;
            r.set(f0.left - dx, f0.top - dy, f0.right + dx, f0.bottom + dy);
            android.graphics.Bitmap f = art.image("pad/focus");
            if (f != null) c.drawBitmap(f, null, r, paint);
        }
        TestHooks.hit(btn, HIT_CLOSE, 0);
        if (FILES_OFFERED && !restarting) TestHooks.hit(filesBtn, HIT_FILES, 0);
        TestHooks.end(this);
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        geom();
        int a = e.getActionMasked();
        if (a == MotionEvent.ACTION_DOWN) { padFocus = false; down = btnHit.contains(e.getX(), e.getY());
            held = saveHit[EXPORT].contains(e.getX(), e.getY()) ? EXPORT : saveHit[IMPORT].contains(e.getX(), e.getY()) ? IMPORT
                : FILES_OFFERED && filesHit.contains(e.getX(), e.getY()) ? FILES : -1; invalidate(); }
        else if (a == MotionEvent.ACTION_UP) {
            boolean hit = down && btnHit.contains(e.getX(), e.getY());
            int on = held >= 0 && (held == FILES ? filesHit : saveHit[held]).contains(e.getX(), e.getY()) ? held : -1;
            down = false; held = -1;
            if (hit) close(); else if (on >= 0) { focus = on; activate(on); } else invalidate();
        }
        else if (a == MotionEvent.ACTION_CANCEL) { down = false; held = -1; invalidate(); }
        return true;
    }


    @Override public void setVisibility(int visibility) { super.setVisibility(visibility); TestHooks.overlay(this, "tech_page", isOpen() ? "open" : "closed"); }
}
