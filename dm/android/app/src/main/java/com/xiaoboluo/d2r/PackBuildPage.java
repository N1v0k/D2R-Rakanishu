package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.net.Uri;
import android.os.FileObserver;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;















final class PackBuildPage extends View {
    interface Host { void pick(PackRows.Row row); void importPack(); void startGame(); }

    enum Screen { GAME_FILES, PICK, CHECK, BUILD }
    static final String PAGE = "pack_build";

    static final int BUILD_HERE = 1, IMPORT_PC = 2, NEXT = 4, START = 5, RETRY = 6, CONTINUE = 7, LEAVE = 8, ROW = 10, PICK = 20, BAR = 100;

    private final Host host;
    private final Handler ui = new Handler(Looper.getMainLooper());
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final PackSession session;
    private final PageLayout page = new PageLayout();
    private final Map<String, PackRows.Row> picked = new LinkedHashMap<>();
    private Screen screen = Screen.GAME_FILES;
    private String message;
    private String version;
    private PackEntry entry;
    private PackRows rows = PackRows.of(new ArrayList<>());
    private PackRows.Missing missing;
    private boolean keptPick;
    private boolean keptCheck;
    private boolean busy;
    private PackSpace.Check check;
    private String compact;
    private PackBuildState state;
    private String info;
    private boolean doneLine;
    private boolean doneAsked, styleLoading;
    private long styleTried;
    private volatile boolean stageRefused;
    private static final long STYLE_RETRY_MS = 2000;
    private int pressed;
    private FileObserver observer;

    PackBuildPage(Context c, Host host) {
        super(c);
        this.host = host;
        session = PackBuildService.session(c);
        setBackgroundColor(0xFF000000);
        PackLog.open(PackBuildService.dir(c));
        page.style(GameStyle.builtIn(c));
        loadStyle(false);
    }

    Screen screen() { return screen; }






    private boolean loadStyle(boolean stage) {
        if (styleLoading || !page.style.builtIn()) return false;
        styleLoading = true;
        Context c = getContext();
        worker.execute(() -> {
            GameStyle s = GameStyle.of(c, new File(c.getFilesDir(), "dm/appart"));
            if (s == null && stage && !stageRefused && appStepDone(c)) {
                s = GameStyle.of(c, new File(PackBuildService.dir(c), "stage/appart"));
                stageRefused = s == null;
            }
            GameStyle found = s;
            ui.post(() -> {
                styleLoading = false;
                if (found == null || !page.style.builtIn() || !page.style(found)) return;
                AppArt.log("pack build: the build pages use the game art now");
                relayout();
                invalidate();
            });
        });
        return true;
    }

    private static boolean appStepDone(Context c) {
        try { return new PackJournal(new File(PackBuildService.dir(c), PackJournal.FILE), PackBuildService.SYNC, PackBuildService.STAMP).load().containsKey("app"); }
        catch (IOException | RuntimeException e) { return false; }
    }



    void showGameFiles(String message) { showGameFiles(message, null); }


    void showGameFiles(String message, String version) {
        this.message = message;
        if (version != null) this.version = version;
        entry = null;
        Context c = getContext();
        String other = this.version;
        worker.execute(() -> {
            PackEntry e = entry(c, other);
            ui.post(() -> { entry = e; relayout(); invalidate(); });
        });
        show(Screen.GAME_FILES);
    }


    private static PackEntry entry(Context c, String version) {
        File dm = new File(c.getFilesDir(), "dm");
        String have = read(new File(dm, "pack_info.txt"));
        PackSource.Record record = null;
        try { record = PackSource.parse(read(new File(dm, PackSource.FILE))); } catch (IllegalArgumentException e) { AppArt.log("pack build: the pack source record is unreadable"); }
        PackEntry.Kept kept;
        try {
            List<PackJournal.Unit> units = PackBuildRun.units(PackReader.nativeUnits(TestHooks.packContract(c)), PackBuildService.exefsDir(c), PackBuildService.dataDir(c));
            kept = PackEntry.kept(new PackJournal(new File(PackBuildService.dir(c), PackJournal.FILE), PackBuildService.SYNC, PackBuildService.STAMP).load(), units, PackEntry::present);
        } catch (IOException | RuntimeException e) {
            AppArt.log("pack build: the kept data cannot be looked at (" + e.getClass().getSimpleName() + ")");
            kept = PackEntry.Kept.NONE;
        }
        PackEntry e = PackEntry.decide(have, record, BuildConfig.DM_PRODUCT, version, kept);
        AppArt.log("pack build: game files page, kept data " + kept.name().toLowerCase(Locale.ROOT) + ", first button " + e.target.name().toLowerCase(Locale.ROOT));
        return e;
    }

    private static String read(File f) {
        try { return f.isFile() ? new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8) : null; } catch (IOException e) { return null; }
    }

    void showPick() { show(Screen.PICK); }


    void picked(PackRows.Row row, List<Uri> uris) {
        for (Uri u : uris) { picked.remove(u.toString()); picked.put(u.toString(), row); }
        reclassify();
    }

    private void reclassify() {
        List<Map.Entry<String, PackRows.Row>> all = new ArrayList<>();
        for (Map.Entry<String, PackRows.Row> e : picked.entrySet()) all.add(Map.entry(e.getKey(), e.getValue()));
        PackRows.Missing m = missing;
        Context c = getContext();
        String contract = TestHooks.packContract(c);
        worker.execute(() -> {
            List<PackRows.Pick> picks = classify(c, all, contract);
            PackRows result = PackRows.of(picks, m);
            ui.post(() -> {
                rows = result;

                for (PackRows.Pick p : picks) if (!p.kept()) picked.remove(p.uri);
                relayout();
                invalidate();
            });
        });
    }


    private void repick(PackRows.Row row, String error, String detail) {
        PackPick.Sources src = PackPick.sources(getContext());
        picked.clear();
        if (src != null) for (PackRows.Row r : PackRows.Row.values()) if (r != row) picked.put(src.uri(r), r);
        missing = new PackRows.Missing(row, error, detail);
        show(Screen.PICK);
        reclassify();
    }


    private void keptMissing() {
        picked.clear();
        missing = null;
        keptPick = true;
        show(Screen.PICK);
        reclassify();
    }

    private static List<PackRows.Pick> classify(Context c, List<Map.Entry<String, PackRows.Row>> all, String contract) {
        List<PackRows.Pick> first = new ArrayList<>();
        PackRows.Pick key = null;
        for (Map.Entry<String, PackRows.Row> u : all) {
            PackRows.Pick p = PackPick.probe(c, Uri.parse(u.getKey()), null, contract).in(u.getValue());
            first.add(p);
            if (p.kind().equals("keys") || p.kind().equals("PB-KEYMISSING")) key = p;
        }
        byte[] keys = null;
        List<PackRows.Pick> out = new ArrayList<>();
        if (key != null) {
            if (key.kind().equals("keys")) {
                keys = PackPick.readKeys(c, Uri.parse(key.uri));
                if (keys == null) key = new PackRows.Pick(key.uri, key.name, "PB-NOTLOCAL\t", key.row);
            }
            out.add(key);
        }
        try {
            for (PackRows.Pick p : first) {
                if (p == key || p.kind().equals("keys") || p.kind().equals("PB-KEYMISSING") && p.rest().equals("header_key")) continue;
                out.add(keys != null && p.kind().equals("package") ? PackPick.probe(c, Uri.parse(p.uri), keys, contract).in(p.row) : p);
            }
        } finally { if (keys != null) java.util.Arrays.fill(keys, (byte) 0); }
        return out;
    }



    private void showCheck(boolean kept) {
        Context c = getContext();
        String contract = TestHooks.packContract(c);
        check = null;
        compact = null;
        keptCheck = kept;
        if (kept) busy = true; else show(Screen.CHECK);
        worker.execute(() -> {
            PackSpace.Check result;
            String holes;
            try {
                List<PackJournal.Unit> units = PackBuildRun.units(PackReader.nativeUnits(contract), PackBuildService.exefsDir(c), PackBuildService.dataDir(c));
                holes = PackPick.compact(units);
                long reserve = TestHooks.packSteps(c, PackSteps.all()).isEmpty() ? 0 : PackBuildService.stepsReserve(c);
                result = PackBuildService.space(c, units, o -> !(o.file.isFile() && o.file.length() == o.size), reserve, PackBuildService.reclaimable(c));
            } catch (RuntimeException e) {
                AppArt.log("pack build: the space check failed (" + e.getClass().getSimpleName() + ")");
                result = null;
                holes = null;
            }
            PackSpace.Check r = result;
            String h = holes;
            ui.post(() -> {
                busy = false;
                if (r == null) { state = new PackBuildState(PackBuildState.Phase.ERROR, 0, null, "PB-IO", ""); show(Screen.BUILD); return; }
                check = r;
                compact = h;
                if (kept && h == null && r.fits()) { startKept(); return; }
                if (kept) show(Screen.CHECK);
                relayout();
                invalidate();
            });
        });
    }



    private void start() {
        Context c = getContext();
        PackRows.Line b = rows.base, u = rows.update, kf = rows.keys;
        try {
            for (PackRows.Line l : new PackRows.Line[]{b, u, kf}) PackPick.keep(c, Uri.parse(l.pick.uri));

            File old = new File(PackBuildService.dir(c), PackBuildState.FILE);
            if (old.exists() && !old.delete()) throw new IOException("cannot remove the old state");
            PackPick.Sources before = PackPick.sources(c), now = new PackPick.Sources(b.pick.uri, b.pick.rest(), u.pick.uri, u.pick.rest(), kf.pick.uri);
            PackPick.saveSources(c, now);
            session.keptDone();
            if (before != null) for (String o : before.uris()) if (!now.uris().contains(o)) PackPick.release(c, o);
            missing = null;
            keptPick = false;
        } catch (IOException | RuntimeException e) {
            AppArt.log("pack build: the sources could not be recorded " + e);
            state = new PackBuildState(PackBuildState.Phase.ERROR, 0, null, "PB-IO", "");
            show(Screen.BUILD);
            return;
        }
        state = null;
        info = null;
        showBuild();
    }


    private void startKept() {
        try { session.startKept(); }
        catch (IOException e) {
            AppArt.log("pack build: the rebuild could not be recorded " + e);
            state = new PackBuildState(PackBuildState.Phase.ERROR, 0, null, "PB-IO", "");
            show(Screen.BUILD);
            return;
        }
        keptCheck = false;
        state = null;
        info = null;
        showBuild();
    }


    void showBuild() {
        show(Screen.BUILD);
        PackBuildState s = PackBuildService.state(getContext());
        if (s == null || s.phase == PackBuildState.Phase.RUNNING || s.phase == PackBuildState.Phase.PAUSED || s.phase == PackBuildState.Phase.STOPPED)
            PackBuildService.start(getContext());
        watch(true);
    }


    private void follow(PackBuildState s) {
        state = s;
        info = s == null ? null : info(s.step, s.file);
        if (s != null && s.phase == PackBuildState.Phase.ERROR && screen == Screen.BUILD) {
            if (PackBuildRun.KEPT.equals(s.error) && session.kept()) { watch(false); keptMissing(); return; }
            PackRows.Row row = PackRows.repick(s.error, s.detail);
            if (row != null) { watch(false); repick(row, s.error, s.detail); return; }
        }
        if (s != null && s.phase == PackBuildState.Phase.DONE) doneLine = new File(PackBuildService.dir(getContext()), PackSession.SOURCES).isFile();


        if (s != null && page.style.builtIn()) {
            long now = android.os.SystemClock.uptimeMillis();
            if (s.phase == PackBuildState.Phase.DONE) { if (!doneAsked && loadStyle(false)) doneAsked = true; }
            else if (s.step != null && s.step != PackProgress.Step.EXTRACT && now - styleTried >= STYLE_RETRY_MS && loadStyle(true)) styleTried = now;
        }
        relayout();
        invalidate();
    }

    void watch(boolean on) {
        if (observer != null) { observer.stopWatching(); observer = null; }
        if (!on || screen != Screen.BUILD) return;
        File dir = PackBuildService.dir(getContext());
        observer = new FileObserver(dir, FileObserver.MOVED_TO | FileObserver.CLOSE_WRITE) {
            @Override public void onEvent(int event, String path) {
                if (!PackBuildState.FILE.equals(path)) return;
                ui.post(() -> { if (screen == Screen.BUILD) follow(PackBuildService.state(getContext())); });
            }
        };
        observer.startWatching();
        follow(PackBuildService.state(getContext()));
    }

    private void show(Screen s) {
        screen = s;
        pressed = 0;
        if (s != Screen.BUILD) watch(false);
        relayout();
        invalidate();
    }


    boolean backTakes() {
        return screen == Screen.PICK || screen == Screen.CHECK || screen == Screen.BUILD && state != null && state.phase == PackBuildState.Phase.ERROR;
    }

    boolean back() {
        if (busy) return true;
        if (screen == Screen.CHECK && !keptCheck) { show(Screen.PICK); return true; }
        if (screen == Screen.CHECK || screen == Screen.PICK) { keptCheck = false; showGameFiles(null); return true; }
        if (screen == Screen.BUILD && state != null && state.phase == PackBuildState.Phase.ERROR) { leave(); return true; }
        return false;
    }



    private void leave() {
        busy = true;
        watch(false);
        Context c = getContext();
        PackBuildService.stop(c);
        worker.execute(() -> {
            try { session.end(); } catch (IOException e) { AppArt.log("pack build: the session could not be ended " + e); }
            ui.post(() -> {
                busy = false;
                state = null;
                info = null;
                keptPick = false;
                missing = null;
                picked.clear();
                showGameFiles(null);
            });
        });
    }



    @Override protected void onSizeChanged(int w, int h, int ow, int oh) { relayout(); }

    private int colour(String name) { return page.colour(name); }

    private void relayout() {
        int w = getWidth(), h = getHeight();
        if (!page.begin(w, h)) return;
        if (screen != Screen.BUILD) page.line(page.title, AppText.get(getContext(), R.string.dmAndroidPbTitle), w / 2f, h * 0.17f, Paint.Align.CENTER, colour("FontColorGoldYellow"), w * 0.8f, 1);
        switch (screen) {
            case GAME_FILES -> layoutGameFiles(w, h);
            case PICK -> layoutPick(w, h);
            case CHECK -> layoutCheck(w, h);
            case BUILD -> layoutBuild(w, h);
        }
    }

    private void layoutGameFiles(int w, int h) {
        Context c = getContext();
        float bh = h * 0.11f, y = h * 0.40f;
        if (entry == null) return;
        String head = switch (entry.head) {
            case INTRO -> AppText.get(c, R.string.dmAndroidPbIntro);
            case UPDATED -> AppText.get(c, R.string.dmAndroidPbUpdated);
            case VERSION -> AppText.get(c, R.string.dmAndroidPbOtherVersion, entry.version);
            case NONE -> null;
        };
        if (head != null) {
            int at = page.mark();
            page.line(page.body, head, w / 2f, y - bh * 0.45f, Paint.Align.CENTER, colour(entry.head == PackEntry.Head.VERSION ? "FontColorRed" : "FontColorBeige"), w * 0.72f, 2);
            page.lift(at, y - bh * 0.45f);
        }
        String first = AppText.get(c, entry.rebuild ? R.string.dmAndroidPbRebuild : R.string.dmAndroidPbBuildHere), second = AppText.get(c, R.string.dmAndroidPbImportPc);
        float bw = page.buttonWidth(w * 0.30f, w * 0.60f, first, second);
        page.button(BUILD_HERE, first, w / 2f - bw / 2, y, bw, bh, !busy);
        page.button(IMPORT_PC, second, w / 2f - bw / 2, y + bh * 1.45f, bw, bh, !busy);
        if (message != null) page.line(page.body, message, w / 2f, y + bh * 3.4f, Paint.Align.CENTER, colour("FontColorRed"), w * 0.8f, 2);
    }

    private void layoutPick(int w, int h) {
        Context c = getContext();
        float left = w * 0.14f, right = w * 0.86f, y = h * 0.34f, step = h * 0.105f, bh = step * 0.72f;
        GameText body = page.body;
        if (keptPick) page.line(body, AppText.get(c, R.string.dmAndroidPbKept), w / 2f, h * 0.255f, Paint.Align.CENTER, colour("FontColorBeige"), w * 0.76f, 1);
        String[] labels = {AppText.get(c, R.string.dmAndroidPbBase), AppText.get(c, R.string.dmAndroidPbUpdate), AppText.get(c, R.string.dmAndroidPbKeys)};
        String choose = AppText.get(c, R.string.dmAndroidPbChoose);
        float labelW = 0;
        for (String l : labels) labelW = Math.max(labelW, body.measure(l, page.k));
        labelW = Math.min(labelW, w * 0.30f);
        float bw = page.buttonWidth(w * 0.16f, w * 0.24f, choose), bx = right - bw;
        float statusX = Math.max(w * 0.32f, left + labelW + w * 0.02f), statusW = bx - w * 0.02f - statusX;
        for (int i = 0; i < 3; i++) {
            float ty = y + i * step;
            page.line(body, labels[i], left, ty, Paint.Align.LEFT, colour("FontColorBeige"), labelW, 1);
            PackRows.Line l = rows.line(PackRows.Row.values()[i]);
            String status; int col;
            switch (l.state) {
                case OK -> { status = page.middle(body, l.pick.name, "", statusW); col = colour("FontColorPartyGreen"); }
                case ERROR -> { status = messageFor(l.error, l.detail); col = colour("FontColorRed"); }
                case WAITING -> { status = page.middle(body, l.pick.name, " · " + AppText.get(c, R.string.dmAndroidPbWaitKeys), statusW); col = colour("FontColorLightGray"); }
                default -> { status = AppText.get(c, R.string.dmAndroidPbNotChosen); col = colour("FontColorLightGray"); }
            }
            page.line(body, status, statusX, ty, Paint.Align.LEFT, col, statusW, 1);
            page.row(ROW, new RectF(left, ty - step * 0.7f, right, ty + step * 0.3f));
            page.button(PICK + i, choose, bx, ty - body.size(page.k) * 0.36f - bh / 2, bw, bh, true);
        }
        if (rows.refused != null) {
            String tail = " · " + messageFor(rows.refused.kind(), rows.refused.rest());
            page.line(page.small, page.middle(page.small, rows.refused.name, tail, w * 0.72f), left, y + 2.75f * step, Paint.Align.LEFT, colour("FontColorRed"), w * 0.72f, 1);
        }
        String next = AppText.get(c, R.string.dmAndroidPbNext);
        float nw = page.buttonWidth(w * 0.22f, w * 0.5f, next);
        page.button(NEXT, next, w / 2f - nw / 2, h * 0.76f, nw, h * 0.11f, rows.ready());
    }

    private void layoutCheck(int w, int h) {
        Context c = getContext();
        if (compact != null) page.line(page.body, messageFor("PB-COMPACT", compact), w / 2f, h * 0.36f, Paint.Align.CENTER, colour("FontColorRed"), w * 0.76f, 2);
        else if (check != null) page.line(page.body, AppText.get(c, R.string.dmAndroidPbSpace, PackSpace.size(check.need, Locale.getDefault()), PackSpace.size(check.free, Locale.getDefault())),
            w / 2f, h * 0.36f, Paint.Align.CENTER, colour(check.fits() ? "FontColorBeige" : "FontColorRed"), w * 0.76f, 2);
        String start = AppText.get(c, R.string.dmAndroidPbStart);
        float bw = page.buttonWidth(w * 0.30f, w * 0.6f, start);
        page.button(START, start, w / 2f - bw / 2, h * 0.47f, bw, h * 0.15f, check != null && check.fits() && compact == null);
        page.line(page.small, AppText.get(c, R.string.dmAndroidPbHint), w / 2f, h * 0.76f, Paint.Align.CENTER, colour("FontColorLightGray"), w * 0.76f, 2);
    }

    private void layoutBuild(int w, int h) {
        Context c = getContext();
        int percent = state == null ? 0 : state.percent;
        float k = page.k;
        page.bar(BAR, w * 0.16f, h * 0.42f, w * 0.76f, h * 0.42f + Math.max(8, h * 0.034f), percent);
        RectF bar = page.bar();
        page.line(page.percent, percent + " %", bar.right + w * 0.02f, bar.centerY() + page.percent.size(k) * 0.36f, Paint.Align.LEFT, colour("FontColorGoldYellow"), w * 0.1f, 1);
        float infoY = bar.bottom + h * 0.075f, room = w * 0.84f - bar.left, bh = h * 0.1f, buttonsY = h * 0.68f;
        if (state == null) return;
        switch (state.phase) {
            case ERROR -> {
                float after = page.line(page.small, messageFor(state.error, state.detail), bar.left, infoY, Paint.Align.LEFT, colour("FontColorRed"), room, 2);
                if (state.error.startsWith("PB-STEP-")) page.line(page.id, state.error, bar.left, after + page.id.size(k) * 1.6f, Paint.Align.LEFT, colour("FontColorLightGray"), room, 1);
                String leave = AppText.get(c, R.string.dmAndroidPbTitle), retry = AppText.get(c, R.string.dmAndroidPbRetry);
                float bw = page.buttonWidth(w * 0.17f, w * 0.34f, leave, retry), gap = w * 0.03f;
                page.button(LEAVE, leave, w / 2f - gap / 2 - bw, buttonsY, bw, bh, !busy);
                page.button(RETRY, retry, w / 2f + gap / 2, buttonsY, bw, bh, !busy);
            }
            case DONE -> {
                String go = AppText.get(c, R.string.dmAndroidPbStartGame);
                float bw = page.buttonWidth(w * 0.20f, w * 0.4f, go);
                page.button(CONTINUE, go, w / 2f - bw / 2, buttonsY, bw, bh, true);
                if (doneLine) page.line(page.small, AppText.get(c, R.string.dmAndroidPbDoneDelete), w / 2f, buttonsY + bh + h * 0.075f, Paint.Align.CENTER, colour("FontColorBeige"), w * 0.76f, 2);
            }
            case PAUSED -> page.line(page.small, AppText.get(c, R.string.dmAndroidPbCooling), bar.left, infoY, Paint.Align.LEFT, colour("FontColorLightGray"), room, 1);
            default -> { if (info != null) page.line(page.small, page.middle(page.small, info, "", room), bar.left, infoY, Paint.Align.LEFT, colour("FontColorLightGray"), room, 1); }
        }
    }

    @Override protected void onDraw(Canvas c) {
        TestHooks.pageBegin(this, PAGE);
        page.draw(c, pressed);
        TestHooks.pageEnd(this);
    }


    String messageFor(String id, String detail) {
        Context c = getContext();
        return switch (id) {
            case "PB-NOTLOCAL" -> AppText.get(c, R.string.dmAndroidPbNotLocal);
            case "PB-NOTGAME" -> AppText.get(c, R.string.dmAndroidPbNotGame);
            case "PB-KEYMISSING" -> AppText.get(c, R.string.dmAndroidPbKeyMissing, detail);
            case "PB-KEYWRONG" -> AppText.get(c, R.string.dmAndroidPbKeyWrong, detail);
            case "PB-TICKET" -> AppText.get(c, R.string.dmAndroidPbTicket);
            case "PB-NOBASE" -> AppText.get(c, R.string.dmAndroidPbNoBase);
            case "PB-NOUPDATE" -> AppText.get(c, R.string.dmAndroidPbNoUpdate);
            case "PB-VERSION" -> AppText.get(c, R.string.dmAndroidPbVersion);
            case "PB-DAMAGED" -> AppText.get(c, R.string.dmAndroidPbDamaged, detail);
            case "PB-SPACE" -> {
                Long bytes = PackSpace.missing(detail);
                yield bytes == null ? AppText.get(c, R.string.dmAndroidPbSpaceShort) : AppText.get(c, R.string.dmAndroidPbSpaceMore, PackSpace.size(bytes, Locale.getDefault()));
            }
            case "PB-COMPACT" -> AppText.get(c, R.string.dmAndroidPbCompact);
            case "PB-INPLACE" -> detail.isEmpty() ? AppText.get(c, R.string.dmAndroidPbInPlace) : rowLabel(c, detail) + ": " + AppText.get(c, R.string.dmAndroidPbInPlace);
            case "PB-SOURCE" -> AppText.get(c, R.string.dmAndroidPbSource, rowLabel(c, detail));
            case "PB-KEPT" -> AppText.get(c, R.string.dmAndroidPbKept);
            case "PB-IO" -> AppText.get(c, R.string.dmAndroidPbIo);
            case "PB-STEP-rewrite" -> AppText.get(c, R.string.dmAndroidPbFailRewrite);
            case "PB-STEP-art" -> AppText.get(c, R.string.dmAndroidPbFailArt);
            case "PB-STEP-overlay" -> AppText.get(c, R.string.dmAndroidPbFailOverlay);
            case "PB-STEP-app" -> AppText.get(c, R.string.dmAndroidPbFailApp);
            case "PB-STEP-pictures" -> AppText.get(c, R.string.dmAndroidPbFailPictures);
            case "PB-STEP-shaders" -> AppText.get(c, R.string.dmAndroidPbFailShaders);
            case "PB-STEP-runner" -> AppText.get(c, R.string.dmAndroidPbFailRunner);
            case "PB-STEP-runtime" -> AppText.get(c, R.string.dmAndroidPbFailRuntime);
            case "PB-STEP-install" -> AppText.get(c, R.string.dmAndroidPbFailInstall);
            case "PB-STEP-check" -> AppText.get(c, R.string.dmAndroidPbFailCheck);
            default -> { AppArt.log("pack build: no message for " + id); yield AppText.get(c, R.string.dmAndroidPbIo); }
        };
    }


    private String info(PackProgress.Step step, String file) {
        if (step == null) return file;
        String what = AppText.get(getContext(), stepText(step));
        return file == null ? what : what + " · " + file;
    }

    static int stepText(PackProgress.Step step) {
        return switch (step) {
            case EXTRACT -> R.string.dmAndroidPbStepExtract;
            case REWRITE -> R.string.dmAndroidPbStepRewrite;
            case ART -> R.string.dmAndroidPbStepArt;
            case OVERLAY -> R.string.dmAndroidPbStepOverlay;
            case APP_ART -> R.string.dmAndroidPbStepAppArt;
            case PICTURES -> R.string.dmAndroidPbStepPictures;
            case SHADERS -> R.string.dmAndroidPbStepShaders;
            case INSTALL -> R.string.dmAndroidPbStepInstall;
        };
    }


    private static String rowLabel(Context c, String row) {
        return AppText.get(c, row.equals("update") ? R.string.dmAndroidPbUpdate : row.equals("keys") ? R.string.dmAndroidPbKeys : R.string.dmAndroidPbBase);
    }



    @Override public boolean onTouchEvent(MotionEvent e) {
        int at = hit(e.getX(), e.getY());
        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_DOWN -> { pressed = at; invalidate(); }
            case MotionEvent.ACTION_UP -> { int p = pressed; pressed = 0; invalidate(); if (p != 0 && p == at && !busy) press(p); }
            case MotionEvent.ACTION_CANCEL -> { pressed = 0; invalidate(); }
            default -> { }
        }
        return true;
    }

    private int hit(float x, float y) { return page.hit(x, y); }

    private void press(int id) {
        switch (id) {
            case BUILD_HERE -> {
                if (entry == null) return;
                if (entry.target == PackEntry.Target.KEPT) { showCheck(true); relayout(); invalidate(); }
                else { keptPick = entry.missing; showPick(); }
            }
            case IMPORT_PC -> host.importPack();
            case PICK, PICK + 1, PICK + 2 -> host.pick(PackRows.Row.values()[id - PICK]);
            case NEXT -> showCheck(false);
            case START -> { if (keptCheck) startKept(); else start(); }
            case RETRY -> {
                state = state == null ? null : new PackBuildState(PackBuildState.Phase.RUNNING, state.percent, null, null, null);
                info = null;
                PackBuildService.start(getContext());
                relayout();
                invalidate();
            }
            case LEAVE -> leave();
            case CONTINUE -> finishBuild();
            default -> throw new IllegalStateException("unknown button " + id);
        }
    }


    private void finishBuild() {
        try { session.end(); }
        catch (IOException e) { AppArt.log("pack build: the session could not be ended " + e); return; }
        watch(false);
        worker.shutdown();
        host.startGame();
    }


    static boolean active(Context c) { return PackBuildService.session(c).active(); }
}
