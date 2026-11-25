



package com.xiaoboluo.d2r;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.hardware.display.DisplayManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.PowerManager;
import android.provider.Settings;
import android.system.ErrnoException;
import android.system.Os;
import android.text.Editable;
import android.text.InputType;
import android.text.SpanWatcher;
import android.text.Spannable;
import android.text.Spanned;
import android.text.TextWatcher;
import android.view.Choreographer;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.BaseInputConnection;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.WeakReference;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

public class DmActivity extends Activity implements SurfaceHolder.Callback, DisplayManager.DisplayListener {
    static native int nativeStart(String filesDir, String extDir, String romfsSrc, String lang);



    static native int nativeGameLang();
    static native void nativeSurface(Object surface, int w, int h);
    static native void nativeRefresh(float hz);

    static native void nativeDisplayModes(float[] hz, int physicalWidth, int physicalHeight);
    static native void nativeTouch(int n, int[] ids, float[] xs, float[] ys);
    static native void nativeFocus(int ev);
    static native void nativeTextResult(long id, boolean accepted, String text);


    static native int nativeCamSensor();
    static native void nativeCamTurn(float rad);


    static native int nativeCamMode();
    static native int nativeCamActive();


    static native int nativeCamStyle();


    static native int nativeCamAim(float[] out);


    static native int nativeCamThreats(float[] out);


    static native int nativeCamHeading(float[] out);
    static native void nativePad(long buttons, int lx, int ly, int rx, int ry);
    static native void nativePadMode(int pad);
    static native void nativePadGlyph(int glyph);

    static native void nativeBeltDrink(int kind);
    static native int nativeBeltKinds();
    static native int nativeDamageNumbers(float[] out);
    static native int nativeSortButtons(float[] out);
    static native void nativeInvSort(int page);
    static native int nativeCharData();
    static native int nativeEffects(int[] out);
    static native int nativeStreak(int[] out);
    static native byte[] nativeStreaks();
    static native int nativeFeatNotice(int[] out);
    static native byte[] nativeFeatNames(int seq);
    static native byte[] nativeBookKills();
    static native byte[] nativeFeats();
    static native int nativeRecipeRequest();
    static native int nativeRecipeStock(java.nio.ByteBuffer out);
    static native int nativeRecipeInsert(int[] plan, int cube, int gen);
    static native int nativeRecipeInsertState();
    static native void nativeRecipeAbort();
    static native void nativeRecipeTextStart(String dir);
    static native byte[] nativeRecipeText(int rec);



    static native byte[] nativeItemText(int kind, int row, int grp);
    static native int nativeRecipeBusy();
    static native void nativePlayers(int players);
    static final int PAUSE_MENU = 1, PAUSE_BOOK = 2, PAUSE_LEFT = 4, PAUSE_BUILDS = 8, PAUSE_TECH = 16, PAUSE_GLOSSARY = 32, PAUSE_COMBAT = 64, PAUSE_DRIVER = 128, PAUSE_RESUME = 256, PAUSE_FEATS = 512, PAUSE_MONBOOK = 1024;


    static native int nativeBuildState(int[] out, byte[] name);

    static native void nativeGlossaryWant(int on);
    static native int nativeGlossary(int[] out);
    static native byte[] nativeCombat(int[] options);
    static native byte[] nativeFightLog(int fight, int group, int key, int sim);
    static native int nativeBuildSkills(int[] ids);
    static native void nativeBuildTarget(int session, int page, int row, int col, int show, int guide);
    static native int nativeBuildOpenRequest();



    static native void nativeBuildCard(int session, byte[] text, int to, int at, int state);
    static native void nativeBuildStat(int session, int stat, int show);
    static native int nativeBuildHints();


    static native void nativeBuildKeys(int session, int[] ids);
    static native int nativeTipDots(float[] box);
    static native void nativeGearGoal(int session, int survivalHits, int bossPercent);

    static native byte[] nativeBestGear(int on);


    static native void nativeBestGearTip(int id, int place);

    static native int nativeBestGearStockAsk();
    static native int nativeBestGearStockAnswered();


    static native int nativeBestGearEquip(String key, int row);

    static native void nativeBestGearCharms(int search);
    static native void nativePause(int reasons, int on);
    static native int nativeInGame();
    static native void nativeLeave();


    static native void nativeResumeLeft();
    static native void nativeResumeDrop();
    static native int nativeResumeThere();
    static native int nativeResumeLeave(int toCamp);




    static native void nativeLeaveState(long[] out);
    static native int nativePauseReasons();
    static native void nativeShutdown();



    static native void nativeLegacySettings(int fpsCap, int statusMask);
    static native int nativeStatusLines();
    static native int nativeFpsChoice();

    static native boolean nativeStatusEffectsOn();





    static native void nativeLegacyShoulder(int left, int right);
    static native int nativeShoulderAction(int hand);
    static native void nativeSetHasTriggers(int on);
    static native int nativeTechRequest();
    static native int nativePlayersWanted();
    static native String nativeDataSource();
    static native String nativeGpuLabel();



    static native void nativeVkDriver(String dir, String library, String hooks, String tuDebug, String attempt, String stateDir, String label, int testFail);
    static native String nativeDriverState();

    static native int nativeDriverRequest();


    static native int nativeFpsMeasured();
    static native int nativeDisplayState();



    static native int nativePadBusy();
    static native void nativePlusReads(long[] out);
    static native void nativePadUnlatch(long mask);
    static native int nativeUiRect(int[] out);
    static native void nativeViewRect(int width, int height, int[] out);


    static native int nativePortraitBottom();
    static native byte[] nativeLevelWord();
    private static volatile boolean hostStarted;
    static boolean hostStarted() { return hostStarted; }





    private static boolean libLoaded;






    private static final long LANG_POLL_MS = 500;
    private final Runnable langPoll = this::pollLang;
    static void syncLang() {
        if (!hostStarted) return;
        int lang = nativeGameLang();
        if (!GameLang.set(lang)) return;
        AppArt.log("Android: game language " + lang + " -> app text " + GameLang.code());
        DmActivity a = current.get();
        if (a != null && a.gameFrame != null) for (int i = 0; i < a.gameFrame.getChildCount(); i++) a.gameFrame.getChildAt(i).invalidate();
        if (a != null) KeepAliveService.relabel(a);
    }
    private void pollLang() {
        navHandler.removeCallbacks(langPoll);
        if (!started || !resumed || bootFailure != 0) return;
        syncLang();
        navHandler.postDelayed(langPoll, LANG_POLL_MS);
    }

    private static WeakReference<DmActivity> current = new WeakReference<>(null);
    static DmActivity currentActivity() { return current.get(); }
    static boolean resumedNow() { DmActivity a = current.get(); return a != null && a.resumed; }
    private static volatile int bootFailure;
    private AlertDialog inputDialog;

    private SurfaceView view;
    private FrameLayout gameFrame;
    private TouchOverlay touchOverlay;
    private boolean started;
    private boolean importing;
    private boolean libraryLoaded;
    private boolean resumed;
    private DisplayManager displays;
    private CamSensor camSensor;
    private CamButtons camButtons;
    private CamThreats camThreats;
    private PotionButtons potionButtons;
    private DamageNumbers damageNumbers;
    private KillStreak killStreak;
    private FeatNotice featNotice;
    private SortButton sortButton;
    private TipDots tipDots;
    private StatusLine statusLine;
    private StatusEffects statusEffects;
    private AppMenu appMenu;
    private TechPage techPage;
    private DriverPage driverPage;
    private TransferNotice transferNotice;
    private GameStyle gameStyle;
    private AppArt art;
    private boolean artChecked;
    private RecipeBook recipeBook;
    private BuildBook buildBook;
    private GlossaryPage glossaryPage;
    private CombatPage combatPage;
    private FeatsPage featsPage;
    private BookPage bookPage;
    private HoldRing holdRing;

    private final PadNav padNav = new PadNav(cmd -> { PadPage p = topPage(); if (p != null) p.onPad(cmd); });
    private final android.os.Handler navHandler = new android.os.Handler(android.os.Looper.getMainLooper());
    private final Runnable navPoll = this::scheduleNav;

    @Override protected void onCreate(Bundle b) {
        super.onCreate(b);
        if (Quit.committed()) { finish(); return; }





        if (BuildConfig.DM_QA_BUILD) {
            if (BuildConfig.DEBUG && getIntent().getBooleanExtra("dmArtDump", false)) {
                try { new TouchOverlay(this, null, null).dumpArt(new File(getFilesDir(), "dm/artdump")); } catch (IOException e) { android.util.Log.e("dm", "art dump: " + e); }
                finish(); return;
            }
        }
        if (b == null) safeStart(getIntent());
        current = new WeakReference<>(this);
        displays = getSystemService(DisplayManager.class);
        if (displays != null) displays.registerDisplayListener(this, null);
        camSensor = new CamSensor(this);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getAttributes().layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS;
        view = new SurfaceView(this);
        view.getHolder().addCallback(this);
        gameFrame = new FrameLayout(this);
        gameFrame.addView(view, new FrameLayout.LayoutParams(-1, -1));
        setContentView(gameFrame);
        immersive();
    }

    private void immersive() {
        WindowInsetsController c = getWindow().getInsetsController();
        if (c != null) { c.hide(WindowInsets.Type.systemBars()); c.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE); }
    }




















    @Override public boolean dispatchKeyEvent(KeyEvent e) {

        if (packPage != null && packPage.getParent() != null && e.getKeyCode() == KeyEvent.KEYCODE_BACK && packPage.backTakes()) {
            if (e.getAction() == KeyEvent.ACTION_UP) packPage.back();
            return true;
        }
        if (libraryLoaded) ShoulderKeys.detect(e, this);
        PadPage page = libraryLoaded ? topPage() : null;
        if (page != null) {
            if (e.getKeyCode() == KeyEvent.KEYCODE_VOLUME_UP || e.getKeyCode() == KeyEvent.KEYCODE_VOLUME_DOWN) return super.dispatchKeyEvent(e);
            if (page == appMenu && (e.getKeyCode() == KeyEvent.KEYCODE_F7 || e.getKeyCode() == KeyEvent.KEYCODE_F8) && e.getAction() == KeyEvent.ACTION_DOWN && e.getRepeatCount() == 0
                && hostStarted && ShoulderKeys.action(e.getKeyCode() == KeyEvent.KEYCODE_F7 ? 0 : 1) == ShoulderKeys.QUICK) { appMenu.close(); return true; }
            if (page.takesKey(e)) return super.dispatchKeyEvent(e);
            if (PadInput.onKey(e)) scheduleNav();
            else if ((e.getKeyCode() == KeyEvent.KEYCODE_BACK || e.getKeyCode() == KeyEvent.KEYCODE_ESCAPE) && e.getAction() == KeyEvent.ACTION_UP) page.back();
            return true; }
        if (libraryLoaded && hostStarted && ShoulderKeys.onKey(e, touchOverlay)) return true;
        if (libraryLoaded && PadInput.onKey(e)) return true;
        return super.dispatchKeyEvent(e);
    }

    @Override public boolean onGenericMotionEvent(MotionEvent e) {
        PadPage page = libraryLoaded ? topPage() : null;
        if (page != null) { if (!ShoulderKeys.fromTrigger(e) && PadInput.onMotion(e)) scheduleNav(); return true; }
        if (libraryLoaded && ShoulderKeys.fromTrigger(e)) return true;
        if (libraryLoaded && PadInput.onMotion(e)) return true;
        return super.onGenericMotionEvent(e);
    }



    private final List<PadPage> padPages = new ArrayList<>();
    private PadPage topPage() {
        for (PadPage p : padPages) if (p.padOpen()) return p;
        return null;
    }
    static boolean padPageOpen() { DmActivity a = current.get(); return a != null && a.topPage() != null; }


    static void padPagesChanged() {
        DmActivity a = current.get();
        if (a == null || !a.libraryLoaded) return;
        PadInput.pagesChanged();
        a.scheduleNav();
        if (a.pageShade != null) a.pageShade.invalidate();
    }


    private void scheduleNav() {
        navHandler.removeCallbacks(navPoll);
        if (topPage() == null) return;
        long next = padNav.poll(android.os.SystemClock.uptimeMillis());
        if (next > 0) navHandler.postAtTime(navPoll, next);
    }




    private final PadInput.Host padHost = new PadInput.Host() {
        @Override public boolean captured() { return topPage() != null; }
        @Override public boolean holdAllowed() {
            return started && resumed && hostStarted && inputDialog == null && topPage() == null && appMenu != null && nativeInGame() != 0 && nativeRecipeBusy() == 0;
        }
        @Override public void holdOpen() { if (openQuickMenu(true) && holdRing != null) holdRing.learnedNow(); }
        @Override public void holdRing(float progress) { if (holdRing != null) holdRing.setProgress(progress); }
    };





    private boolean openQuickMenu(boolean byPad) {
        if (appMenu == null || !hostStarted || topPage() != null || inputDialog != null || nativeInGame() == 0 || nativeRecipeBusy() != 0) return false;
        appMenu.open(byPad);
        return true;
    }
    static void quickMenuFromTrigger(boolean byPad) { DmActivity a = current.get(); if (a != null) a.openQuickMenu(byPad); }





    private final Set<Integer> swallowed = new HashSet<>();




    @Override public boolean dispatchTouchEvent(MotionEvent e) {
        if (ShoulderKeys.strayTouch(e)) return true;
        if (pageGesture(e)) return true;
        int action = e.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN && libraryLoaded && topPage() != null) return super.dispatchTouchEvent(e);
        if (action == MotionEvent.ACTION_DOWN) {
            boolean wasPad = PadInput.isPadMode();


            if (!(wasPad && statusEffects != null && statusEffects.claimPad(e.getX() - statusEffects.getLeft(), e.getY() - statusEffects.getTop()))) {
                PadInput.onTouch();
                if (wasPad) swallowed.add(e.getPointerId(0));
            }
        } else if (action == MotionEvent.ACTION_POINTER_DOWN && PadInput.isPadMode() && !(libraryLoaded && topPage() != null)) {


            if (statusEffects != null) statusEffects.cancelPad();
            PadInput.onTouch();
            swallowed.add(e.getPointerId(e.getActionIndex()));
        }
        boolean swallowThis = false;
        for (int i = 0; i < e.getPointerCount(); i++) if (swallowed.contains(e.getPointerId(i))) swallowThis = true;
        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) swallowed.clear();
        else if (action == MotionEvent.ACTION_POINTER_UP) swallowed.remove(e.getPointerId(e.getActionIndex()));
        if (swallowThis) return true;
        return super.dispatchTouchEvent(e);
    }


    static void onPadMode(boolean pad) {
        DmActivity a = current.get();
        if (a == null) return;
        a.applyPadMode(pad);
        if (a.holdRing != null) a.holdRing.watch();
    }

    private void applyPadMode(boolean pad) {
        if (touchOverlay == null) return;
        touchOverlay.setInputEnabled(!pad);
        touchOverlay.setVisibility(pad ? View.GONE : View.VISIBLE);
    }



    private boolean ensureFileAccess() {
        if (Environment.isExternalStorageManager()) return true;
        startPage().show(null, getString(R.string.dmAndroidPermission), null,
            () -> startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION, Uri.parse("package:" + getPackageName()))));
        return false;
    }


    private StartPage startPage;
    private StartPage startPage() {
        if (startPage == null) startPage = new StartPage(this);
        if (startPage.getParent() == null) setContentView(startPage);
        return startPage;
    }

    private static String readText(File f) {
        try { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8); } catch (IOException e) { return null; }
    }


    static final boolean PACK_BUILD_OFFERED = BuildConfig.DM_PACK_BUILDER;
    private PackBuildPage packPage;
    private boolean importAsked;


    private boolean ensureImported(File source) {
        if (importing) return false;
        File runtime = new File(getFilesDir(), "dm");
        String want = readText(new File(source, "pack_info.txt")), have = readText(new File(runtime, "pack_info.txt"));
        PackSource.Record record = null;
        String recordText = readText(new File(runtime, PackSource.FILE));
        try { record = PackSource.parse(recordText); }
        catch (IllegalArgumentException e) { AppArt.log("PB-SOURCE-RECORD: the pack source record is unreadable (" + e.getMessage() + "), treated as a computer pack"); }
        boolean asked = importAsked;
        importAsked = false;
        PackSource.Decision d = PackSource.decide(have, new File(runtime, "out").isDirectory(), want, record, BuildConfig.DM_PRODUCT, PACK_BUILD_OFFERED, asked);
        File next = new File(PackBuildService.dir(this), GAME_FILES_NEXT);
        boolean gameFiles = PACK_BUILD_OFFERED && next.exists();
        if (gameFiles && !next.delete()) AppArt.log("pack build: the game files mark cannot be removed");
        switch (TestHooks.packGameFiles(this) || gameFiles ? PackSource.Action.GAME_FILES : d.action) {
            case START -> { return true; }
            case VERSION_ERROR -> { showStartError(getString(R.string.dmAndroidImportVersion, d.product)); return false; }
            case GAME_FILES -> {
                if (!PACK_BUILD_OFFERED) return true;

                packPage().showGameFiles(asked && d.product == null ? getString(R.string.dmAndroidPbImportNone, GAME_DIR + "android") : null, d.product);
                return false;
            }
            case IMPORT -> { }
        }
        importing = true;
        StartPage t = startPage();
        t.show(null, getString(R.string.dmAndroidImporting, 0), null, null);
        String marker = DriverLibrary.sha256Bytes(want.getBytes(StandardCharsets.UTF_8));
        new Thread(() -> {
            String error = null;
            try { importPack(source, runtime, PackSource.format(new PackSource.Record(PackSource.Kind.PC, marker)), percent -> runOnUiThread(() -> t.text(getString(R.string.dmAndroidImporting, percent)))); }
            catch (IOException e) { error = e.getMessage() == null ? e.toString() : e.getMessage(); }
            String failure = error;
            runOnUiThread(() -> {
                importing = false;
                if (failure != null) showStartError(getString(R.string.dmAndroidImportFailed, failure));
                else startHost();
            });
        }, "dm-import").start();
        return false;
    }

    private PackBuildPage packPage() {
        if (packPage == null) packPage = new PackBuildPage(this, new PackBuildPage.Host() {
            @Override public void pick(PackRows.Row row) { openPackPicker(row); }
            @Override public void importPack() { importAsked = true; startHost(); }
            @Override public void startGame() { packPage = null; startHost(); }
        });
        if (packPage.getParent() == null) setContentView(packPage);
        return packPage;
    }


    private void openPackPicker(PackRows.Row row) {
        List<Uri> given = TestHooks.packPicks(this);
        if (given != null) { packPage.picked(row, given); return; }
        try { startActivityForResult(PackPick.intent(), REQ_PACK + row.ordinal()); }
        catch (ActivityNotFoundException | SecurityException e) { AppArt.log("pack build: the file picker could not be opened (" + e + ")"); }
    }

    private interface Progress { void percent(int p); }




    private void importPack(File source, File runtime, String record, Progress progress) throws IOException {
        android.content.Context c = getApplicationContext();
        File build = PackBuildService.dir(c);
        try (PackLock lock = PackLock.take(build)) {
            PackInstall in = PackBuildService.installer(c);
            PackInstall.Result open = in.recover(false);
            if (open.outcome == PackInstall.Outcome.STOPPED) throw new IOException(open.reason);
            int[] shown = {-1};
            PackGeneration.Made g = PackGeneration.fromComputer(getFilesDir(), source, record, PackBuildService.SYNC, (d, t) -> {
                int p = (int) (d * 100 / Math.max(1, t));
                if (p != shown[0]) { shown[0] = p; progress.percent(p); }
            });
            PackInstall.Result r = in.commit(g.folder, g.manifest, (d, t) -> {});
            AppArt.log("pack import: " + r);
            if (r.outcome != PackInstall.Outcome.CONFIRMED) throw new IOException(r.reason == null ? r.outcome.name() : r.reason);
        }

        PackBuildService.session(c).finish();
    }








    private static final int REQ_NOTIFY = 0x4b41, REQ_BATTERY = 0x4b42, REQ_ALLOW = 0x4b43, REQ_DRIVER = 0x4b44, REQ_PACK = 0x4b50;
    private boolean promptPending;
    private boolean keepAlivePrompt() {
        if (promptPending) return true;
        SharedPreferences p = getSharedPreferences("dm_keepalive", MODE_PRIVATE);
        if (Build.VERSION.SDK_INT >= 33 && checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED
            && !p.getBoolean("notifyAsked", false)) {
            p.edit().putBoolean("notifyAsked", true).apply();
            promptPending = true;
            requestPermissions(new String[]{android.Manifest.permission.POST_NOTIFICATIONS}, REQ_NOTIFY);
            return true;
        }
        PowerManager pm = getSystemService(PowerManager.class);
        if (pm != null && BatteryAsk.shouldAsk(pm.isIgnoringBatteryOptimizations(getPackageName()), p.getBoolean("batteryAsked", false))) {
            AppArt.log("keepalive: battery prompt");
            try {
                startActivityForResult(new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, Uri.parse("package:" + getPackageName())), REQ_BATTERY);
            } catch (ActivityNotFoundException | SecurityException e) {
                android.util.Log.i("dm", "keepalive: battery prompt unavailable " + e);
                p.edit().putBoolean("batteryAsked", true).apply();
                return false;
            }
            p.edit().putBoolean("batteryAsked", true).apply();
            promptPending = true;
            return true;
        }
        return false;
    }
    @Override public void onRequestPermissionsResult(int code, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(code, permissions, results);
        if (code != REQ_NOTIFY) return;
        android.util.Log.i("dm", "keepalive: notification permission " + (results.length > 0 && results[0] == PackageManager.PERMISSION_GRANTED ? "granted" : "denied"));
        promptPending = false;
        startHost();
    }




    private boolean allowPending;
    void allowBattery() {
        if (allowPending) return;
        PowerManager pm = getSystemService(PowerManager.class);
        if (pm != null && pm.isIgnoringBatteryOptimizations(getPackageName())) { keepAliveChanged(); return; }
        try {
            startActivityForResult(new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS, Uri.parse("package:" + getPackageName())), REQ_ALLOW);
            allowPending = true; AppArt.log("keepalive: allow button opened the battery dialog");
        } catch (ActivityNotFoundException | SecurityException e) {
            try { startActivityForResult(new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS), REQ_ALLOW); allowPending = true; AppArt.log("keepalive: allow button opened the battery settings list (" + e + ")"); }
            catch (ActivityNotFoundException | SecurityException e2) { AppArt.log("keepalive: allow button: no battery dialog or settings (" + e2 + ")"); }
        }
    }

    @Override protected void onActivityResult(int code, int result, Intent data) {
        super.onActivityResult(code, result, data);
        if (code == REQ_DRIVER) { if (driverPage != null) driverPage.picked(result == RESULT_OK && data != null ? data.getData() : null); return; }
        if (code >= REQ_PACK && code < REQ_PACK + PackRows.Row.values().length) {
            if (packPage != null && result == RESULT_OK) packPage.picked(PackRows.Row.values()[code - REQ_PACK], PackPick.uris(data));
            return;
        }
        if (code == REQ_ALLOW) {
            allowPending = false;
            PowerManager pm = getSystemService(PowerManager.class);
            AppArt.log("keepalive: allow button result " + result + ", battery " + (pm != null && pm.isIgnoringBatteryOptimizations(getPackageName()) ? "exempt" : "optimised"));
            keepAliveChanged();
            return;
        }
        if (code != REQ_BATTERY) return;
        promptPending = false;
        startHost();
    }



    private static final Object transferLock = new Object();
    private static int transferState;
    private static List<SaveTransfer.Result> transferResults = new ArrayList<>();
    static final String GAME_DIR = "Download/" + BuildConfig.DM_SHARED_ROOT + "/" + SharedStorage.APP_FOLDER + "/";

    static File shared(Context c, File downloads) { return TestHooks.sharedRoot(c, new File(downloads, BuildConfig.DM_SHARED_ROOT)); }


    private boolean transferSaves(File game) {
        synchronized (transferLock) {
            if (transferState == 2) return true;
            if (transferState == 1) return false;
            if (transferState == 3) {
                List<String> lines = takeTransferText(this);
                showStartError(lines.isEmpty() ? getString(R.string.dmAndroidSaveTransferFailed, "journal") : String.join("\n", lines));
                return false;
            }
            transferState = 1;
        }
        Context app = getApplicationContext();
        File save = new File(getFilesDir(), "save");
        String stamp = new SimpleDateFormat("yyyyMMdd-HHmmss", Locale.US).format(new Date());
        new Thread(() -> {
            List<SaveTransfer.Result> out;
            try { out = SharedStorage.transfer(game, save, stamp, BuildStore.transfer(app)); }
            catch (RuntimeException | OutOfMemoryError e) { out = new ArrayList<>(); out.add(new SaveTransfer.Result(SaveTransfer.FAILED, e.toString())); }
            for (SaveTransfer.Result r : out)
                android.util.Log.i("dm", "save transfer: kind=" + r.kind + " path=" + r.path + " skipped=" + r.skipped + " merge=" + r.merge + " own=" + r.own
                    + " bad=" + r.bad + " held=" + r.held + " detail=" + r.detail + (r.resumed ? " resumed" : ""));
            List<SaveTransfer.Result> done = out;

            boolean blocked = SaveTransfer.unfinished(save);
            synchronized (transferLock) { transferResults = done; transferState = blocked ? 3 : 2; }
            new android.os.Handler(android.os.Looper.getMainLooper()).post(() -> {
                DmActivity a = current.get();
                if (a != null && !a.isDestroyed()) a.startHost();
            });
        }, "dm-transfer").start();
        return false;
    }

    static boolean transferPending() { synchronized (transferLock) { return transferState == 2 && !transferResults.isEmpty(); } }


    static List<SaveTransfer.Result> takeTransferResults() {
        synchronized (transferLock) { List<SaveTransfer.Result> results = transferResults; if (transferState != 3) transferResults = new ArrayList<>(); return results; }
    }
    static List<String> takeTransferText(Context c) { return transferText(c, takeTransferResults()); }


    static List<String> transferText(Context c, List<SaveTransfer.Result> results) {
        List<String> out = new ArrayList<>();
        for (SaveTransfer.Result r : results) {
            if (r.resumed) out.add(AppText.get(c, R.string.dmAndroidSaveResumed));
            switch (r.kind) {
                case SaveTransfer.EXPORTED -> out.add(AppText.get(c, R.string.dmAndroidSaveExported, GAME_DIR + r.path));
                case SaveTransfer.EXPORT_EMPTY -> out.add(AppText.get(c, R.string.dmAndroidSaveExportEmpty));
                case SaveTransfer.IMPORTED -> {
                    if (r.path != null) out.add(r.skipped == null ? AppText.get(c, R.string.dmAndroidSaveImported, r.path) : AppText.get(c, R.string.dmAndroidSaveImportedSome, r.path, r.skipped));
                    else if (r.skipped != null) out.add(AppText.get(c, R.string.dmAndroidSaveImportNothing, r.skipped));
                    if (r.merge == SaveTransfer.MERGE_QUEUED) out.add(AppText.get(c, R.string.dmAndroidSaveStashMerge));
                    else if (r.merge == SaveTransfer.MERGE_ALREADY) out.add(AppText.get(c, R.string.dmAndroidSaveStashMerged));
                    else if (r.merge == SaveTransfer.MERGE_SAME_PHONE) out.add(AppText.get(c, R.string.dmAndroidSaveStashSamePhone));
                    if (r.held > 0) out.add(AppText.get(c, R.string.dmAndroidSaveHeld));
                    List<String> own = new ArrayList<>();
                    if ((r.own & SaveTransfer.OWN_CUSTOM) != 0) own.add(AppText.get(c, R.string.dmAndroidSaveOwnCustom));
                    if ((r.own & SaveTransfer.OWN_BUILDS) != 0) own.add(AppText.get(c, R.string.dmAndroidSaveOwnBuilds));
                    if ((r.own & SaveTransfer.OWN_BACKUPS) != 0) own.add(AppText.get(c, R.string.dmAndroidSaveOwnBackups));
                    if (!own.isEmpty()) out.add(AppText.get(c, R.string.dmAndroidSaveOwn, String.join(", ", own)));
                }
                case SaveTransfer.IMPORT_REJECTED -> {
                    if (r.bad == SaveTransfer.BAD_NOTHING) { out.add(AppText.get(c, R.string.dmAndroidSaveImportRejected, r.path)); break; }
                    String why = r.bad == SaveTransfer.BAD_SIZE ? AppText.get(c, R.string.dmAndroidSaveBadSize, r.detail)
                        : r.bad == SaveTransfer.BAD_FILE ? AppText.get(c, R.string.dmAndroidSaveBadFile, r.detail) : AppText.get(c, R.string.dmAndroidSaveBadCount);
                    out.add(AppText.get(c, R.string.dmAndroidSaveImportBad, why, r.path));
                }
                default -> out.add(AppText.get(c, R.string.dmAndroidSaveTransferFailed, r.path));
            }
        }
        return out;
    }






    private static boolean driverReady, driverWorking;
    private static DriverStore.Config driverConfig;
    @Override protected void onNewIntent(Intent intent) { super.onNewIntent(intent); safeStart(intent); }
    private void safeStart(Intent intent) {
        if (intent == null || !intent.getBooleanExtra(DriverStore.SAFE_EXTRA, false)) return;
        intent.removeExtra(DriverStore.SAFE_EXTRA);
        DriverStore.requestSafeStart(this);
        if (hostStarted) AppArt.log("driver: safe start requested while running, applies to the next cold start");
    }




    private void openDriverPicker() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*");
        i.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{"application/zip", "application/x-zip-compressed", "application/octet-stream"});
        try { startActivityForResult(i, REQ_DRIVER); }
        catch (ActivityNotFoundException | SecurityException e) { AppArt.log("driver: the file picker could not be opened (" + e + ")"); if (driverPage != null) driverPage.addFailed("picker"); }
    }

    private void startHost() {
        if (bootFailure != 0) { showBootFailure(); return; }
        if (started || importing || !resumed || !ensureFileAccess()) return;
        if (!hostStarted && keepAlivePrompt()) return;
        File downloads = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);


        if (!libLoaded && packPage == null && PACK_BUILD_OFFERED) {
            if (PackBuildService.importOpen(this) && !installSettled()) return;
            try { PackBuildService.session(this).finish(); }
            catch (IOException e) { AppArt.log("pack build: an ended session could not be cleared (" + e.getClass().getSimpleName() + ")"); }
            if (PackBuildPage.active(this)) { packPage().showBuild(); return; }
        }
        if (packPage != null && packPage.getParent() != null && packPage.screen() != PackBuildPage.Screen.GAME_FILES) return;


        File shared = shared(this, downloads);

        if (!libLoaded && !installSettled()) return;
        if (!libLoaded && !ensureImported(SharedStorage.pack(shared, BuildConfig.DM_PRODUCT))) return;



        if (art == null) art = new AppArt(this);
        if (!artChecked) {
            String lack = art.verify();
            if (lack != null) { AppArt.log("app art: import pack incomplete, missing or unreadable " + lack); showStartError(getString(R.string.dmAndroidImportArt)); return; }
            artChecked = true;
        }
        if (gameFrame.getParent() == null) setContentView(gameFrame);
        if (transferNotice == null) {
            if (gameStyle == null) gameStyle = new GameStyle(this, art);
            transferNotice = new TransferNotice(this, art, gameStyle, this::freeInput, () -> topPage() == null && inputDialog == null);
            gameFrame.addView(transferNotice, new FrameLayout.LayoutParams(-1, -1));
            padPages.add(0, transferNotice);
        }

        if (!libLoaded && !transferSaves(shared)) { transferNotice.busy(true); return; }
        transferNotice.busy(false);
        if (!libraryLoaded) {
            if (!libLoaded) {


                File ext = getExternalFilesDir(null), internal = new File(getFilesDir(), "dm");
                File runtime = new File(internal, "out/main.patched.bin").canRead() || ext == null ? internal : new File(ext, "dm");
                try { Os.setenv("DM_DERIVED", new File(runtime, "derived.tsv").getAbsolutePath(), true); } catch (ErrnoException e) { showStartError(getString(R.string.dmAndroidStorage)); return; }
                try { System.loadLibrary("dm"); libLoaded = true; }
                catch (UnsatisfiedLinkError error) { showStartError(getString(R.string.dmAndroidLibrary)); return; }
                PackHost.loaded(PackBuildService.dir(this), android.os.Process.myPid());
            }
            libraryLoaded = true;
        }
        if (touchOverlay == null) {
            int[] gameIds = {R.string.dmAndroidAttack, R.string.dmAndroidSkill1, R.string.dmAndroidSkill2, R.string.dmAndroidSkill3, R.string.dmAndroidSkill4,
                R.string.dmAndroidPortal, R.string.dmAndroidPotion1, R.string.dmAndroidPotion2, R.string.dmAndroidPotion3, R.string.dmAndroidPotion4,
                R.string.dmAndroidMenu, R.string.dmAndroidMap, R.string.dmAndroidMove, R.string.dmAndroidSwap, R.string.dmAndroidDifficulty};
            int[] navIds = {R.string.dmConfirm, R.string.dmAndroidBack, R.string.dmAndroidAction, R.string.dmAndroidBind, R.string.dmAndroidNext};
            String[] gameLabels = new String[gameIds.length], navLabels = new String[navIds.length];
            for (int i = 0; i < gameIds.length; i++) gameLabels[i] = getString(gameIds[i]);
            for (int i = 0; i < navIds.length; i++) navLabels[i] = getString(navIds[i]);
            touchOverlay = new TouchOverlay(this, gameLabels, navLabels);
            touchOverlay.labelIds(gameIds, navIds);
            touchOverlay.setOnApplyWindowInsetsListener((v, insets) -> {
                android.graphics.Insets safe = insets.getInsets(WindowInsets.Type.displayCutout());
                touchOverlay.setSafeInsets(safe.left, safe.right, safe.bottom);
                return insets;
            });
            gameFrame.addView(touchOverlay, new FrameLayout.LayoutParams(-1, -1));
            touchOverlay.requestApplyInsets();
        }
        if (!view.getHolder().getSurface().isValid() || view.getWidth() <= 0 || view.getHeight() <= 0) return;
        File ext = getExternalFilesDir(null);
        if (ext == null) { showStartError(getString(R.string.dmAndroidStorage)); return; }
        nativeSurface(view.getHolder().getSurface(), view.getWidth(), view.getHeight());
        updateRefresh();





        File data = new File(shared, "data"), earlier = new File(downloads, "data");
        if (!data.isDirectory() && earlier.isDirectory() && !BuildConfig.DM_OWN_SHARED_ROOT && !TestHooks.sharedRootSet(this)) data = earlier;
        String src = data.getAbsolutePath();

        boolean reattach = hostStarted;
        if (!reattach) {
            if (!driverReady && DriverStore.needed(this)) {
                if (!driverWorking) {
                    driverWorking = true;
                    android.content.Context app = getApplicationContext();
                    new Thread(() -> {
                        DriverStore.Config dc = DriverStore.prepare(app);
                        new android.os.Handler(android.os.Looper.getMainLooper()).post(() -> {
                            driverConfig = dc; driverReady = true; driverWorking = false;
                            DmActivity a = current.get();
                            if (a != null && !a.isDestroyed()) a.startHost();
                        });
                    }, "dm-driver").start();
                }
                return;
            }
            DriverStore.Config dc = driverConfig;
            if (dc != null) nativeVkDriver(dc.dir, dc.lib, dc.hooks, dc.tuDebug, dc.attempt, dc.state, dc.label, dc.testFail);
            else if (driverReady) nativeVkDriver(null, null, null, null, null, null, null, 0);
        }
        if (!reattach) {
            nativeLegacySettings(ShoulderKeys.legacyFps(this), ShoulderKeys.legacyStatus(this));
            nativeLegacyShoulder(ShoulderKeys.legacyLeft(this), ShoulderKeys.legacyRight(this));
        }
        int result = reattach ? 0 : nativeStart(getFilesDir().getAbsolutePath(), ext.getAbsolutePath(), src, Locale.getDefault().toLanguageTag());
        if (!reattach) {
            DriverStore.started(this, result); DriverStore.flushLog();


            int assumed = PackLog.assumptions(PackBuildService.dir(this));
            if (assumed > 0) AppArt.log("ASSUME packbuild.log: " + assumed + " assumption lines in the builder log");
            if (result < 0) { driverReady = false; driverConfig = null; }
        }
        if (result < 0) {
            int resource = switch (result) {
                case -2 -> R.string.dmAndroidMissingData;
                case -3 -> R.string.dmAndroidMissingModules;
                case -4 -> R.string.dmAndroidMissingShaders;
                case -5 -> R.string.dmAndroidMissingConfig;
                case -9 -> R.string.dmAndroidMissingOverlay;
                case -10 -> R.string.dmAndroidInvalidOverlay;
                case -11 -> R.string.dmAndroidImportArt;
                default -> 0;
            };
            showStartError(resource == 0 ? getString(R.string.dmAndroidStartFailed, result) : getString(resource), phoneBuilt()); return;
        }
        started = true;
        hostStarted = true;
        syncLang();
        if (reattach) AppArt.log("keepalive: reattach (activity recreated, the host kept running)");
        else ExitLog.logOnce(this);
        KeepAliveService.start(this);
        pollLang();
        nativeSetHasTriggers(ShoulderKeys.hasTriggers(this) ? 1 : 0);
        applyPadMode(PadInput.isPadMode());
        if (camSensor != null) camSensor.start(nativeCamSensor());







        if (camButtons == null) {
            camThreats = new CamThreats(this); gameFrame.addView(camThreats, new FrameLayout.LayoutParams(-1, -1));
            camButtons = new CamButtons(this); gameFrame.addView(camButtons, new FrameLayout.LayoutParams(-1, -1)); }
        if (camButtons != null) camButtons.start();
        if (camThreats != null) camThreats.start();
        if (potionButtons == null) {


            if (gameStyle == null) gameStyle = new GameStyle(this, art);
            GameStyle style = gameStyle;
            if (BuildConfig.DEBUG) style.logCheck();
            recipeBook = new RecipeBook(this, art, style, this::freeInput);
            buildBook = new BuildBook(this, art, style, this::freeInput, recipeBook);
            glossaryPage = new GlossaryPage(this, art, style, this::freeInput);
            combatPage = new CombatPage(this, art, style, this::freeInput);
            featsPage = new FeatsPage(this, art, style, this::freeInput);
            bookPage = new BookPage(this, art, style, combatPage, this::freeInput);
            combatPage.setBook(bookPage);
            appMenu = new AppMenu(this, touchOverlay, art, style, recipeBook, buildBook, glossaryPage, combatPage, featsPage, bookPage, this::freeInput, () -> topPage() == null && inputDialog == null);
            holdRing = new HoldRing(this, art, style);

            driverPage = new DriverPage(this, art, style, this::freeInput, this::openDriverPicker);
            techPage = new TechPage(this, art, style, SharedStorage.app(shared(this, downloads)), this::freeInput, () -> driverPage.open(), () -> topPage() == null && inputDialog == null);
            padPages.add(driverPage);
            padPages.add(techPage);
            padPages.add(combatPage);
            padPages.add(featsPage);
            padPages.add(bookPage);
            padPages.add(glossaryPage);
            padPages.add(buildBook);
            padPages.add(recipeBook);
            padPages.add(appMenu);
            PadInput.setHost(padHost, padNav);
            potionButtons = new PotionButtons(this, touchOverlay, art, style, () -> openQuickMenu(false));
            damageNumbers = new DamageNumbers(this, touchOverlay, style);
            gameFrame.addView(damageNumbers, new FrameLayout.LayoutParams(-1, -1));
            killStreak = new KillStreak(this, touchOverlay, potionButtons, style);
            gameFrame.addView(killStreak, new FrameLayout.LayoutParams(-1, -1));
            featNotice = new FeatNotice(this, touchOverlay, potionButtons, style);
            gameFrame.addView(featNotice, new FrameLayout.LayoutParams(-1, -1));
            sortButton = new SortButton(this, art, () -> { if (combatPage != null && !combatPage.isOpen() && combatPage.available()) combatPage.openBest(); });
            gameFrame.addView(sortButton, new FrameLayout.LayoutParams(-1, -1));
            tipDots = new TipDots(this, style);
            gameFrame.addView(tipDots, new FrameLayout.LayoutParams(-1, -1));
            statusLine = new StatusLine(this, touchOverlay, style);
            gameFrame.addView(statusLine, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(potionButtons, new FrameLayout.LayoutParams(-1, -1));
            statusEffects = new StatusEffects(this, touchOverlay, art, style, statusLine);
            touchOverlay.setEffects(statusEffects);
            statusEffects.setGlossary(glossaryPage);
            touchOverlay.setStyle(style);
            gameFrame.addView(statusEffects, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(holdRing, new FrameLayout.LayoutParams(-1, -1));
            pageShade = new PageShade(this, appMenu, recipeBook, buildBook, glossaryPage, combatPage, featsPage, bookPage, driverPage, techPage, transferNotice);
            gameFrame.addView(pageShade, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(appMenu, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(recipeBook, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(buildBook, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(combatPage, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(featsPage, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(bookPage, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(glossaryPage, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(techPage, new FrameLayout.LayoutParams(-1, -1));
            gameFrame.addView(driverPage, new FrameLayout.LayoutParams(-1, -1));
            transferNotice.bringToFront(); }
        rectWatch(true);
        potionButtons.start();
        buildBook.start();
        techPage.start();
        appMenu.start();
        transferNotice.start();
        damageNumbers.start();
        killStreak.start();
        featNotice.start();
        sortButton.start();
        tipDots.start();
        statusLine.start();
        statusEffects.start();
        holdRing.watch();
        if (reattach) reattachPause();
        nativeFocus(3);
    }




    private void reattachPause() {
        int reasons = nativePauseReasons();
        nativePause(PAUSE_MENU | PAUSE_BOOK | PAUSE_BUILDS | PAUSE_TECH | PAUSE_GLOSSARY | PAUSE_COMBAT | PAUSE_FEATS | PAUSE_MONBOOK | PAUSE_DRIVER | PAUSE_RESUME, 0);
        if (nativeInGame() != 0) { appMenu.showLeftPause(); appMenu.keepStatus(KeepAliveService.status(this)); }
        else if ((reasons & PAUSE_LEFT) != 0) nativePause(PAUSE_LEFT, 0);
        AppArt.log("keepalive: reattach pause reasons=" + reasons + " in_game=" + nativeInGame());
    }



    private int keepLogged = -1;

    static void keepAliveChanged() {
        DmActivity a = current.get();
        if (a != null && a.appMenu != null && !a.isDestroyed()) a.appMenu.keepStatus(KeepAliveService.status(a));
    }
    private void keepAliveResume() {
        int s = KeepAliveService.status(this);
        if (appMenu != null) appMenu.keepStatus(s);
        if (KeepAliveService.starting()) return;
        if (s != keepLogged) {
            keepLogged = s;
            PowerManager pm = getSystemService(PowerManager.class);
            AppArt.log("keepalive: status " + KeepAliveStatus.name(s) + " (notifications " + (getSystemService(android.app.NotificationManager.class).areNotificationsEnabled() ? "on" : "off")
                + ", battery " + (pm != null && pm.isIgnoringBatteryOptimizations(getPackageName()) ? "exempt" : "optimised") + ")");
        }
        if (!KeepAliveService.foreground(this)) KeepAliveService.start(this);
        else KeepAliveService.update(this, KeepAliveService.FOREGROUND);
    }

    private void showStartError(String message) { showStartError(message, false); }



    private void showStartError(String message, boolean toGameFiles) {
        if (!toGameFiles) { startPage().show(null, message, null, null); return; }
        startPage().show(null, message, AppText.get(this, R.string.dmAndroidPbTitle), () -> {
            try { DriverLibrary.writeFile(new File(PackBuildService.dir(this), GAME_FILES_NEXT), "1", PackBuildService.SYNC); }
            catch (IOException e) { AppArt.log("pack build: the game files mark cannot be written (" + e.getClass().getSimpleName() + ")"); return; }
            Quit.restart(this, "game files after a failed start", null);
        });
    }


    static final String GAME_FILES_NEXT = "game-files-next";

    private boolean phoneBuilt() {
        if (!PACK_BUILD_OFFERED) return false;
        try {
            PackSource.Record r = PackSource.parse(readText(new File(new File(getFilesDir(), "dm"), PackSource.FILE)));
            return r != null && r.source == PackSource.Kind.PHONE;
        } catch (IllegalArgumentException e) { return false; }
    }

    private boolean settling, settled;


    private boolean installSettled() {
        if (settled) return true;
        if (!PackBuildService.installOpen(this)) { settled = true; return true; }
        if (settling) return false;
        settling = true;
        startPage().show(null, getString(R.string.dmAndroidImporting, 0), null, null);
        android.content.Context app = getApplicationContext();
        new Thread(() -> {
            PackInstall.Result r = PackBuildService.recover(app, false);
            runOnUiThread(() -> {
                settling = false;
                if (r.outcome == PackInstall.Outcome.STOPPED) { showStartError(getString(R.string.dmAndroidPbInstallStopped)); return; }
                settled = true;
                startHost();
            });
        }, "dm-install-recovery").start();
        return false;
    }



    private static void showNativeFailure(int code) {
        bootFailure = code;
        DmActivity activity = current.get();
        if (activity != null) activity.runOnUiThread(() -> {
            if (!activity.isDestroyed()) activity.showBootFailure();
        });
    }

    private void showBootFailure() {
        releaseTouches();
        showStartError(AppText.get(this, bootFailure == -6 ? R.string.dmAndroidGraphicsFailed : R.string.dmAndroidHostStopped));
    }








    private static void dropHaptic(int tier) {
        DmActivity activity = current.get();
        if (activity == null) return;
        android.os.VibratorManager manager = activity.getSystemService(android.os.VibratorManager.class);
        android.os.Vibrator vibrator = manager != null ? manager.getDefaultVibrator() : null;
        if (vibrator == null || !vibrator.hasVibrator()) { android.util.Log.i("dm", "drop haptic tier=" + tier + ": no vibrator"); return; }
        android.os.VibrationEffect effect = android.os.VibrationEffect.createPredefined(android.os.VibrationEffect.EFFECT_HEAVY_CLICK);
        if (android.os.Build.VERSION.SDK_INT >= 33)
            vibrator.vibrate(effect, android.os.VibrationAttributes.createForUsage(android.os.VibrationAttributes.USAGE_MEDIA));
        else
            vibrator.vibrate(effect, new android.media.AudioAttributes.Builder().setUsage(android.media.AudioAttributes.USAGE_GAME).build());
        android.util.Log.i("dm", "drop haptic tier=" + tier);
    }

    private static void showNativeInput(long id, String title, String sub, String guide, String initial,
                                        int maxChars, int nameRule, int numericMax) {
        DmActivity activity = current.get();
        if (activity == null) { nativeTextResult(id, false, ""); return; }
        activity.runOnUiThread(() -> activity.openInput(id, title, sub, guide, initial, maxChars, nameRule != 0, numericMax));
    }

    private void openInput(long id, String title, String sub, String guide, String initial,
                           int maxChars, boolean nameRule, int numericMax) {
        if (!resumed || isFinishing() || isDestroyed() || inputDialog != null) { nativeTextResult(id, false, ""); return; }
        releaseTouches();
        syncLang();
        EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setInputType(numericMax >= 0 ? InputType.TYPE_CLASS_NUMBER : InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        input.setHint(nameRule ? AppText.get(this, R.string.dmNameBlankRandom) : guide);
        input.setText(TextInputRules.normalize(initial, maxChars, nameRule, numericMax));
        input.setSelection(input.length());
        String message = sub + (sub.isEmpty() || guide.isEmpty() ? "" : "\n") + guide;
        if (nameRule) message += (message.isEmpty() ? "" : "\n") + AppText.get(this, R.string.dmNameRule);
        if (numericMax >= 0) message = AppText.get(this, R.string.dmAndroidQuantityRange, numericMax);
        if (title.isEmpty()) title = AppText.get(this, nameRule ? R.string.dmNameTitle : numericMax >= 0 ? R.string.dmQtyTitle : R.string.dmInputTitle);
        final boolean[] finished = {false};
        AlertDialog dialog = new AlertDialog.Builder(this).setTitle(title).setMessage(message).setView(input)
            .setPositiveButton(AppText.get(this, R.string.dmConfirm), null).setNegativeButton(AppText.get(this, R.string.dmCancel), (d, which) -> {})
            .create();
        inputDialog = dialog;
        dialog.setOnDismissListener(d -> {
            if (inputDialog == dialog) inputDialog = null;
            if (!finished[0]) { finished[0] = true; nativeTextResult(id, false, ""); }
            if (resumed) immersive();
        });
        dialog.setOnShowListener(d -> {
            Runnable check = () -> {
                boolean composing = BaseInputConnection.getComposingSpanStart(input.getText()) >= 0;
                int error = nameRule ? TextInputRules.nameError(input.getText().toString()) : 0;
                input.setError(composing ? AppText.get(this, R.string.dmNamePickIme) : error == 1 ? AppText.get(this, R.string.dmNameTooShort) : error == 2 ? AppText.get(this, R.string.dmNameEdge) : null);
                dialog.getButton(AlertDialog.BUTTON_POSITIVE).setEnabled(!composing && error == 0);
            };
            Runnable settle = () -> {
                Editable text = input.getText();
                if (BaseInputConnection.getComposingSpanStart(text) < 0) {
                    String normalized = TextInputRules.normalize(text.toString(), maxChars, nameRule, numericMax);
                    if (!normalized.contentEquals(text)) { input.setText(normalized); input.setSelection(input.length()); return; }
                }
                check.run();
            };





            SpanWatcher composingWatch = new SpanWatcher() {
                boolean composing = BaseInputConnection.getComposingSpanStart(input.getText()) >= 0;
                private void changed(Spannable text) {
                    boolean now = BaseInputConnection.getComposingSpanStart(text) >= 0;
                    if (now != composing) { composing = now; input.post(settle); }
                }
                public void onSpanAdded(Spannable text, Object what, int start, int end) { changed(text); }
                public void onSpanRemoved(Spannable text, Object what, int start, int end) { changed(text); }
                public void onSpanChanged(Spannable text, Object what, int ostart, int oend, int nstart, int nend) { changed(text); }
            };
            input.setEditableFactory(new Editable.Factory() {
                @Override public Editable newEditable(CharSequence source) {
                    Editable e = super.newEditable(source);
                    e.setSpan(composingWatch, 0, e.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE);
                    return e;
                }
            });
            input.getText().setSpan(composingWatch, 0, input.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE);
            input.addTextChangedListener(new TextWatcher() {
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
                public void onTextChanged(CharSequence s, int start, int before, int count) {}
                public void afterTextChanged(Editable text) { settle.run(); }
            });
            dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(v -> {
                finished[0] = true;
                nativeTextResult(id, true, input.getText().toString());
                dialog.dismiss();
            });
            check.run();
            input.requestFocus();
            if (dialog.getWindow() != null) dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        });
        dialog.show();
    }









    private final int[] rectIn = new int[8], rectBox = new int[4];
    private PageShade pageShade;
    private boolean rectWhole = true;
    private boolean rectWatching;
    private final Choreographer.FrameCallback rectPoll = new Choreographer.FrameCallback() {
        @Override public void doFrame(long ns) {
            if (!rectWatching) return;
            syncRect();
            Choreographer.getInstance().postFrameCallback(this);
        }
    };
    private boolean rectPlaced;
    private void rectWatch(boolean on) {
        if (on && !rectPlaced) { rectPlaced = true; GameRect.ui().forget(); }
        if (on == rectWatching) return;
        rectWatching = on;
        Choreographer.getInstance().removeFrameCallback(rectPoll);
        if (on) { syncRect(); Choreographer.getInstance().postFrameCallback(rectPoll); }
    }
    private void syncRect() {
        int w = gameFrame.getWidth(), h = gameFrame.getHeight();
        if (w <= 0 || h <= 0) return;
        boolean native_ = libLoaded && hostStarted;
        if (native_) nativeViewRect(w, h, rectIn);
        if (!GameRect.ui().update(w, h, native_ ? rectIn : null)) return;
        GameRect g = GameRect.ui();
        boolean wasWhole = rectWhole; rectWhole = g.whole();
        if (!rectWhole || !wasWhole) AppArt.log("fullscreen: window " + w + "x" + h + " block " + g.x() + "," + g.y() + " " + g.w() + "x" + g.h() + " picture " + g.pictureX() + "," + g.pictureY() + " " + g.pictureW() + "x" + g.pictureH());
        View[] pages = {appMenu, recipeBook, buildBook, glossaryPage, combatPage, featsPage, bookPage, driverPage, techPage, transferNotice};
        for (View v : pages) place(v, GameRect.PAGE);
        place(potionButtons, GameRect.BLOCK); place(camButtons, GameRect.BLOCK); place(killStreak, GameRect.BLOCK); place(featNotice, GameRect.BLOCK);
        place(statusLine, GameRect.TOP); place(statusEffects, GameRect.TOP);
        if (touchOverlay != null) touchOverlay.viewChanged();
        View[] whole = {camThreats, damageNumbers, sortButton, tipDots, holdRing, pageShade};
        for (View v : whole) if (v != null) v.invalidate();
    }
    private void place(View v, int kind) {
        if (v == null || !(v.getLayoutParams() instanceof FrameLayout.LayoutParams)) return;
        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) v.getLayoutParams();
        GameRect g = GameRect.ui();
        int x = 0, y = 0, w = -1, h = -1;
        if (!g.whole()) { g.bounds(kind, rectBox); x = rectBox[0]; y = rectBox[1]; w = rectBox[2]; h = rectBox[3]; }
        if (lp.width == w && lp.height == h && lp.leftMargin == x && lp.topMargin == y) return;
        lp.width = w; lp.height = h; lp.leftMargin = x; lp.topMargin = y;
        v.setLayoutParams(lp);
    }


    private View pageGesture;
    private boolean pageGesture(MotionEvent e) {
        int action = e.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) {
            pageGesture = null;
            PadPage p = libraryLoaded ? topPage() : null;
            if (p instanceof View && !GameRect.ui().whole()) {
                View v = (View) p; float x = e.getX(), y = e.getY();
                if (x < v.getLeft() || x >= v.getRight() || y < v.getTop() || y >= v.getBottom()) pageGesture = v;
            }
        }
        if (pageGesture == null) return false;
        View v = pageGesture;
        MotionEvent c = MotionEvent.obtain(e); c.offsetLocation(-v.getLeft(), -v.getTop());
        v.dispatchTouchEvent(c); c.recycle();
        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) pageGesture = null;
        return true;
    }

    private void freeInput() {
        ShoulderKeys.reset(touchOverlay);
        PadInput.pagesChanged();
        long now = android.os.SystemClock.uptimeMillis();
        MotionEvent cancel = MotionEvent.obtain(now, now, MotionEvent.ACTION_CANCEL, 0, 0, 0);
        super.dispatchTouchEvent(cancel);
        cancel.recycle();
        releaseTouches();
    }

    private void releaseTouches() {
        if (touchOverlay != null) touchOverlay.cancelTouches();
        if (started) nativeTouch(0, new int[0], new float[0], new float[0]);
    }

    private void updateRefresh() {
        if (!libraryLoaded) return;
        android.view.Display d = view.getDisplay();
        nativeRefresh(d == null ? 0 : d.getRefreshRate());
        if (d == null) return;
        android.view.Display.Mode now = d.getMode();
        android.view.Display.Mode[] all = d.getSupportedModes();
        float[] hz = new float[all.length]; int n = 0;
        for (android.view.Display.Mode m : all)
            if (m.getPhysicalWidth() == now.getPhysicalWidth() && m.getPhysicalHeight() == now.getPhysicalHeight()) hz[n++] = m.getRefreshRate();
        nativeDisplayModes(java.util.Arrays.copyOf(hz, n), now.getPhysicalWidth(), now.getPhysicalHeight());
    }
    @Override public void onDisplayAdded(int id) { updateRefresh(); }
    @Override public void onDisplayRemoved(int id) { updateRefresh(); }
    @Override public void onDisplayChanged(int id) { updateRefresh(); }
    @Override protected void onResume() { super.onResume(); if (Quit.committed()) { finish(); return; } Quit.cancelIfWaiting("resumed"); resumed = true; immersive(); if (packPage != null && packPage.screen() == PackBuildPage.Screen.BUILD) packPage.showBuild(); startHost(); updateRefresh(); if (started) { rectWatch(true); keepAliveResume(); nativeFocus(3); pollLang(); if (touchOverlay != null && bootFailure == 0) applyPadMode(PadInput.isPadMode()); if (camSensor != null) camSensor.start(nativeCamSensor()); if (camButtons != null) camButtons.start(); if (camThreats != null) camThreats.start(); if (potionButtons != null) potionButtons.start(); if (buildBook != null) buildBook.start(); if (techPage != null) techPage.start(); if (appMenu != null) appMenu.start(); if (transferNotice != null) transferNotice.start(); if (statusLine != null) statusLine.start(); if (sortButton != null) sortButton.start(); if (tipDots != null) tipDots.start(); if (damageNumbers != null) damageNumbers.start(); if (killStreak != null) killStreak.start(); if (featNotice != null) featNotice.start(); if (statusEffects != null) statusEffects.start(); if (holdRing != null) holdRing.watch(); } }



    @Override protected void onPause() { resumed = false; if (packPage != null) packPage.watch(false); rectWatch(false); navHandler.removeCallbacks(langPoll); ShoulderKeys.reset(touchOverlay); boolean overlayOpen = (techPage != null && techPage.isOpen()) || (driverPage != null && driverPage.isOpen()); if (techPage != null) techPage.stop(); if (appMenu != null) appMenu.stop(); if (transferNotice != null) transferNotice.stop(); if (recipeBook != null) recipeBook.pause(); if (buildBook != null) { buildBook.pause(); buildBook.stop(); } if (glossaryPage != null) glossaryPage.stop(); if (combatPage != null) combatPage.stop(); if (featsPage != null) featsPage.stop(); if (bookPage != null) bookPage.stop(); if (appMenu != null && !overlayOpen) appMenu.background(); if (potionButtons != null) potionButtons.stop(); if (statusLine != null) statusLine.stop(); if (sortButton != null) sortButton.stop(); if (tipDots != null) tipDots.stop(); if (damageNumbers != null) damageNumbers.stop(); if (killStreak != null) killStreak.stop(); if (featNotice != null) featNotice.stop(); if (statusEffects != null) statusEffects.stop(); if (touchOverlay != null) touchOverlay.resetDisplays(); if (holdRing != null) holdRing.stop(); if (camSensor != null) camSensor.stop(); if (camButtons != null) camButtons.stop(); if (camThreats != null) camThreats.stop(); if (inputDialog != null) inputDialog.dismiss(); releaseTouches(); PadInput.reset(); if (touchOverlay != null) touchOverlay.setInputEnabled(false); if (started) { nativeResumeLeft(); nativeLeave(); nativeFocus(0); KeepAliveService.update(this, nativeInGame() != 0 ? KeepAliveService.BACKGROUND_GAME : KeepAliveService.BACKGROUND_MENU); } super.onPause(); }
    @Override protected void onStart() { super.onStart(); if (touchOverlay != null) touchOverlay.setStopped(false); if (started) nativeFocus(2); }
    @Override protected void onStop() { if (touchOverlay != null) touchOverlay.setStopped(true); if (started) nativeFocus(1); com.xiaoboluo.d2r.widget.HeroWidgetProvider.requestUpdate(this); super.onStop(); }
    @Override protected void onDestroy() { if (combatPage != null) combatPage.dispose(); if (bookPage != null) bookPage.dispose(); navHandler.removeCallbacks(langPoll); if (displays != null) displays.unregisterDisplayListener(this); if (inputDialog != null) inputDialog.dismiss(); if (current.get() == this) current.clear(); super.onDestroy(); }
    @Override public void onWindowFocusChanged(boolean f) { super.onWindowFocusChanged(f); if (f) immersive(); else releaseTouches(); }

    @Override public void surfaceCreated(SurfaceHolder h) {}
    @Override public void surfaceChanged(SurfaceHolder h, int format, int w, int ht) {
        if (libraryLoaded) nativeSurface(h.getSurface(), w, ht);
        updateRefresh();
        startHost();
    }
    @Override public void surfaceDestroyed(SurfaceHolder h) { releaseTouches(); if (libraryLoaded) nativeSurface(null, 0, 0); }

}
