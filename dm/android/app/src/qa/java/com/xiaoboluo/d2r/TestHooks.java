package com.xiaoboluo.d2r;

import android.content.Context;
import android.net.Uri;
import android.view.View;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

final class TestHooks {
    private TestHooks() {}
    static native int nativeObservationEnabled();
    static native int nativeEnabled();
    static native void nativeOverlay(String name, String state, boolean drawn);

    private static int on = -1;
    private static final HashMap<String, String> sent = new HashMap<>();
    private static final HashMap<String, Integer> generation = new HashMap<>();

    private static boolean enabled() {
        if (on >= 0) return on == 1;
        if (!BuildConfig.DEBUG) { on = 0; return false; }

        if (!DmActivity.hostStarted()) return false;
        int v;
        try { v = nativeEnabled(); } catch (UnsatisfiedLinkError e) { on = 0; return false; }
        if (v < 0) return false;
        on = v;
        if (on == 1) overlay("pad_layout", PadInput.isPadMode() ? "pad" : "touch");
        return on == 1;
    }

    private static boolean send(String name, String state, boolean drawn) {
        try { nativeOverlay(name, state, drawn); return true; } catch (UnsatisfiedLinkError e) { on = 0; return false; }
    }

    static void overlay(String name, String state) { overlay(null, name, state); }




    static void overlay(View view, String name, String state) {
        if (!enabled() || state.equals(sent.get(name)) || !send(name, state, false)) return;
        sent.put(name, state);
        final int gen = generation.merge(name, 1, Integer::sum);
        if (view == null || view.getVisibility() != View.VISIBLE) return;
        view.getViewTreeObserver().registerFrameCommitCallback(() -> {
            if (on == 1 && gen == generation.getOrDefault(name, 0)) send(name, state, true);
        });
    }
    private static int observe = -1;
    private static boolean observing() {
        if (!enabled()) return false;
        if (observe < 0) {
            try { observe = nativeObservationEnabled(); }
            catch (UnsatisfiedLinkError e) { observe = 0; }
        }
        return observe == 1;
    }
    static void text(android.graphics.Canvas c, char[] s, int off, int len, float x, float y, android.graphics.Paint p) { if (observing()) UiObservation.text(c, new String(s, off, len), x, y, p); }
    static void begin(View view, String page) { if (observing()) UiObservation.begin(page); }
    static void text(android.graphics.Canvas c, CharSequence s, float x, float y, android.graphics.Paint p) { if (observing()) UiObservation.text(c, s, x, y, p); }
    static void end(View view) { if (observing()) UiObservation.end(view); }
    static void foreground() { if (observing()) UiObservation.foreground(); }
    static void hit(android.graphics.RectF rect, int kind, int index) { if (observing()) UiObservation.hit(rect, kind, index); }


    private static int preHost = -1;
    private static boolean pageObserving(Context c) {
        if (preHost < 0) {
            String env = read(new File(c.getFilesDir(), "dm/env"));
            preHost = env != null && ("\n" + env).contains("\nDM_QA_STATE=1\n") ? 1 : 0;
        }
        return preHost == 1;
    }
    static void pageBegin(View view, String page) { if (pageObserving(view.getContext())) UiObservation.begin(page); }
    static void pageText(android.graphics.Canvas c, CharSequence s, float x, float y, android.graphics.Paint p) { if (preHost == 1) UiObservation.text(c, s, x, y, p); }
    static void pageHit(android.graphics.RectF rect, int kind, int index) { if (preHost == 1) UiObservation.hit(rect, kind, index); }
    static void pageEnd(View view) { if (preHost == 1) UiObservation.end(view); }





    static String packContract(Context c) { return read(new File(PackBuildService.dir(c), "qa-contract.tsv")); }
    static File packDataDir(Context c, File standard) { return packContract(c) == null ? standard : new File(standard.getParentFile(), "qa-data"); }
    static List<Uri> packPicks(Context c) {
        File f = new File(PackBuildService.dir(c), "qa-pick");
        String text = read(f);
        if (text == null) return null;
        if (!f.delete()) throw new IllegalStateException("qa-pick cannot be removed");
        List<Uri> out = new ArrayList<>();
        for (String line : text.split("\n")) if (!line.trim().isEmpty()) out.add(Uri.parse(line.trim()));
        return out;
    }
    static void packHold(Context c, String unit) {
        File f = new File(PackBuildService.dir(c), "qa-hold");
        long until = android.os.SystemClock.uptimeMillis() + 600_000;
        boolean logged = false;
        while (unit.equals(trimmed(read(f))) && android.os.SystemClock.uptimeMillis() < until) {
            if (!logged) { AppArt.log("qa: pack build holds before " + unit); logged = true; }
            android.os.SystemClock.sleep(100);
        }
    }
    static String packDocsAuthority(Context c) { return c.getPackageName() + ".qadocs"; }



    static List<String> packSteps(Context c, List<String> standard) {
        if (packContract(c) == null) return standard;
        String text = read(new File(PackBuildService.dir(c), "qa-steps"));
        List<String> out = new ArrayList<>();
        if (text != null) for (String line : text.split("\n")) if (!line.trim().isEmpty()) out.add(line.trim());
        return out;
    }
    static File[] packStepInputs(Context c) {
        String text = read(new File(PackBuildService.dir(c), "qa-step-inputs"));
        if (text == null) return null;
        File exefs = null, romfs = null;
        for (String line : text.split("\n")) {
            String[] f = line.split("\t");
            if (f.length == 2 && f[0].equals("exefs")) exefs = new File(f[1].trim());
            else if (f.length == 2 && f[0].equals("romfs")) romfs = new File(f[1].trim());
        }
        if (exefs == null || romfs == null) throw new IllegalStateException("qa-step-inputs needs an exefs and a romfs line");
        return new File[]{exefs, romfs};
    }

    static PackBuildJob.Setup packJob(Context c) {
        File[] inputs = packStepInputs(c);
        if (inputs == null) return null;
        return () -> {
            PackBuildService.settle(c);
            PackBuildService.pruneWork(c);
            return new StepsOnly(c, PackPython.steps(c, packStepInputSteps(c, PackSteps.all()), inputs[0], inputs[1], "qa-steps"));
        };
    }
    static List<String> packStepInputSteps(Context c, List<String> standard) {
        String text = read(new File(PackBuildService.dir(c), "qa-step-inputs"));
        if (text != null) for (String line : text.split("\n")) if (line.startsWith("steps\t")) return List.of(line.substring(6).trim().split(" +"));
        return standard;
    }

    static long packFree(Context c, long real) {
        String v = trimmed(read(new File(PackBuildService.dir(c), "qa-free")));
        return v == null ? real : Long.parseLong(v);
    }

    static int packThermal(Context c, int real) {
        String v = trimmed(read(new File(PackBuildService.dir(c), "qa-thermal")));
        return v == null ? real : Integer.parseInt(v);
    }


    static File sharedRoot(Context c, File standard) {
        String name = read(new File(PackBuildService.dir(c), "qa-shared-root"));
        if (name == null) return standard;
        name = name.trim().isEmpty() ? c.getPackageName() : name.trim();
        if (!name.matches("[A-Za-z0-9._-]{1,64}")) throw new IllegalStateException("qa-shared-root names no plain folder");
        return new File(standard, name);
    }
    static boolean sharedRootSet(Context c) { return new File(PackBuildService.dir(c), "qa-shared-root").exists(); }

    static PackSteps packStaged(Context c, File stage) throws IOException {
        File pack = PackStaged.prepared(c);
        return pack == null ? null : PackStaged.steps(c, pack, stage);
    }

    static boolean packCheckFail(Context c) { return new File(PackBuildService.dir(c), "qa-check-fail").exists(); }

    static PackPlan packPlan(Context c, PackPlan real) {
        String text = read(new File(PackBuildService.dir(c), "qa-plan"));
        if (text == null) return real;
        int tokens = real.tokens, shaders = real.shaderWorkers, pictures = real.pictureThreads;
        String mode = real.mode;
        for (String line : text.split("\n")) {
            String[] f = line.trim().split("\t");
            if (f.length != 2) continue;
            switch (f[0]) {
                case "tokens" -> tokens = Integer.parseInt(f[1]);
                case "shaders" -> shaders = Integer.parseInt(f[1]);
                case "pictures" -> pictures = Integer.parseInt(f[1]);
                case "mode" -> mode = f[1];
                default -> throw new IllegalStateException("unknown qa-plan line " + f[0]);
            }
        }
        return new PackPlan(tokens, shaders, pictures, mode);
    }

    static boolean packGameFiles(Context c) { return new File(PackBuildService.dir(c), "qa-game-files").exists(); }

    private static String trimmed(String s) { return s == null ? null : s.trim(); }
    private static String read(File f) {
        try { return f.isFile() ? new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8) : null; }
        catch (IOException e) { throw new IllegalStateException("qa input " + f.getName() + " unreadable", e); }
    }
}
