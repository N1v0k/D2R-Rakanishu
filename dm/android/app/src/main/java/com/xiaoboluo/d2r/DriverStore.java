package com.xiaoboluo.d2r;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.ShortcutInfo;
import android.content.pm.ShortcutManager;
import android.graphics.drawable.Icon;
import android.os.Build;
import android.os.Process;
import android.system.Os;
import android.system.OsConstants;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;





























final class DriverStore {
    static final String SAFE_EXTRA = "dmSafeStart", SHORTCUT = "safe-start";
    static final String[] HOOKS = {"libhook_impl.so", "libmain_hook.so"};
    static final String LOADER = "libdm_driverload.so";
    static final String[] TU_DEBUG = {"sysmem", "gmem", "nolrz", "noubwc", "flushall", "syncdraw"};


    static final class Config { String dir, lib, hooks, tuDebug = "", attempt, state, label, zipSha; String setKey = ""; int testFail; }

    static final class Info { boolean selected; String label = "", sha = "", state = "", notice = "", noticeArg = ""; }

    private static final List<String> pending = java.util.Collections.synchronizedList(new ArrayList<>());
    private static volatile Config active;
    private static volatile boolean selected;
    private static volatile String selectedLabel = "", selectedSha = "", lastState = "";

    private DriverStore() {}

    private static void log(String line) { pending.add("driver: " + line); }

    static void flushLog() { synchronized (pending) { for (String l : pending) AppArt.log(l); pending.clear(); } }

    static File base(Context c) { return new File(c.getFilesDir(), "drivers"); }
    static File folder(Context c) { File ext = c.getExternalFilesDir(null); return ext == null ? null : new File(ext, "drivers"); }


    static void requestSafeStart(Context c) {
        File b = base(c);
        if (!b.isDirectory() && !b.mkdirs()) return;
        try { write(new File(b, "safe-once"), "1\n"); log("safe start requested"); } catch (IOException e) { log("safe start not recorded: " + e); }
    }



    static boolean needed(Context c) {
        if (base(c).isDirectory()) return true;
        return !DriverLibrary.zips(folder(c)).isEmpty();
    }



    static synchronized Config prepare(Context c) {
        active = null; selected = false; selectedLabel = ""; selectedSha = ""; lastState = "";
        File dir = folder(c), b = base(c);
        if (!b.isDirectory() && DriverLibrary.zips(dir).isEmpty()) { updateShortcut(c, false); return null; }
        if (!b.isDirectory() && !b.mkdirs()) { log("cannot create " + b); return null; }
        try {
            DriverLibrary.Migration m = DriverLibrary.migrate(b, dir, DriverStore::syncDir);
            if (m != null) log("migrated " + m.imported + " driver ZIP(s) from " + dir + (m.sha.isEmpty() ? "" : ", selected " + shortSha(m.sha)));
        } catch (IOException e) { log("driver migration failed: " + e); }
        new File(b, "active.txt").delete();
        String sel = DriverLibrary.selected(b);
        File zip = sel.isEmpty() ? null : DriverLibrary.zipFor(b, sel);
        if (zip != null && !zip.isFile()) { log("selected driver is not in the library: " + shortSha(sel)); zip = null; sel = ""; }
        selected = zip != null;
        updateShortcut(c, selected);
        boolean safe = new File(b, "safe-once").delete();
        if (safe) try { syncDir(b); } catch (IOException e) { log("safe start: directory not synced: " + e); }
        DriverGuard.Input in = new DriverGuard.Input();
        in.safe = safe;
        String[] ok = lines(new File(b, "attempt.ok")), fb = lines(new File(b, "attempt.fallback"));
        in.okId = ok.length > 0 ? ok[0] : ""; in.fallbackId = fb.length > 0 ? fb[0] : ""; in.fallbackReason = fb.length > 1 ? fb[1] : "";
        in.attemptId = attemptId();
        Imported imp = null;
        if (zip != null) {
            if (safe) { in.zip = sel; log("safe start: system driver this time, the selected driver stays closed"); }
            else {
                String unfit = unfit(c);
                imp = unfit != null ? null : importZip(c, b, zip);
                if (unfit != null) { in.zip = sel; in.problem = unfit; }
                else { in.zip = imp.zipSha; in.problem = imp.error; selectedLabel = imp.label; selectedSha = imp.setSha; }
            }
        }
        gcPlaced(b, imp);
        DriverGuard.State before = readState(b);
        in.pid = Process.myPid();
        in.endedOutside = endedOutside(c, before, in);
        DriverGuard.Decision d = DriverGuard.decide(before, in);
        if (!before.equals(d.state) || d.custom) {
            try { write(new File(b, "guard.txt"), DriverGuard.text(d.state)); }
            catch (IOException e) { log("guard state not written (" + e + "), system driver"); return null; }
        }


        for (String f : new String[]{"attempt.ok", "attempt.fallback"}) new File(b, f).delete();
        log("guard " + (d.custom ? "own driver" : "system driver") + " fails=" + d.state.fails + (d.state.blocked.isEmpty() ? "" : " blocked=" + shortSha(d.state.blocked))
            + (d.state.notice.isEmpty() ? "" : " notice=" + d.state.notice + (d.state.noticeArg.isEmpty() ? "" : "(" + d.state.noticeArg + ")")));
        if (!d.custom || imp == null) return null;
        Config cfg = new Config();
        cfg.dir = imp.soDir.getAbsolutePath() + "/"; cfg.lib = imp.library; cfg.label = imp.label; cfg.attempt = in.attemptId; cfg.state = b.getAbsolutePath();
        cfg.hooks = imp.hooks.getAbsolutePath(); cfg.zipSha = imp.zipSha; cfg.setKey = imp.setSha;
        cfg.tuDebug = tuDebug(dir);
        if (BuildConfig.DM_QA_BUILD && BuildConfig.DEBUG) {
            String[] t = lines(new File(b, "test-fail"));
            String v = t.length > 0 ? t[0].trim() : "";
            cfg.testFail = v.equals("open") ? 1 : v.equals("verify") ? 2 : 0;
            if (v.equals("hooks")) cfg.hooks = new File(b, "hooks/missing").getAbsolutePath();
            if (!v.isEmpty()) log("test switch test-fail=" + v);
        }
        log("own driver " + imp.label + " (" + cfg.lib + ", set " + shortSha(imp.setSha) + ", zip " + shortSha(imp.zipSha) + ")" + (cfg.tuDebug.isEmpty() ? "" : " TU_DEBUG=" + cfg.tuDebug)
            + " attempt " + cfg.attempt);
        active = cfg;
        return cfg;
    }



    static boolean endedOutside(Context c, DriverGuard.State before, DriverGuard.Input in) {
        if (before.pending.isEmpty() || before.pending.equals(in.okId) || before.pending.equals(in.fallbackId)) return false;
        if (before.pendingPid <= 0 || before.pendingPid == in.pid) { log("attempt " + before.pending + " ended without a record of its process"); return false; }
        ApplicationExitInfo e = null;
        try {
            ActivityManager am = c.getSystemService(ActivityManager.class);
            List<ApplicationExitInfo> list = am == null ? null : am.getHistoricalProcessExitReasons(c.getPackageName(), before.pendingPid, 1);
            if (list != null && !list.isEmpty()) e = list.get(0);
        } catch (RuntimeException x) { log("exit record of attempt " + before.pending + " unreadable: " + x); }
        if (e == null) { log("attempt " + before.pending + " (pid " + before.pendingPid + ") ended without an exit record"); return false; }
        boolean outside = DriverGuard.endedOutsideDriver(e.getReason(), e.getStatus());
        log("attempt " + before.pending + " (pid " + before.pendingPid + ") ended before its OK frames: exit reason " + e.getReason() + " status " + e.getStatus()
            + (outside ? ", ended from outside the driver, not counted" : ", counted"));
        return outside;
    }


    static void started(Context c, int result) {
        if (active == null || result >= 0) return;
        File b = base(c);
        DriverGuard.State s = readState(b);
        try { write(new File(b, "guard.txt"), DriverGuard.text(DriverGuard.cancel(s, active.attempt))); } catch (IOException e) { log("guard cancel not written: " + e); }
        log("start failed before the driver was loaded (" + result + "), attempt " + active.attempt + " does not count");
        active = null;
    }




    static void poll(Context c) {
        if (active == null) return;
        String st = DmActivity.nativeDriverState();
        if (st == null || st.equals(lastState)) return;
        lastState = st;
        if (st.startsWith("fallback:")) {
            File b = base(c);
            DriverGuard.State s = DriverGuard.fellBackNow(readState(b), active.attempt, st.substring(9));
            try { write(new File(b, "guard.txt"), DriverGuard.text(s)); } catch (IOException e) { AppArt.log("driver: fallback not recorded: " + e); }
            AppArt.log("driver: own driver not used in this process (" + st.substring(9) + "), blocked until another driver is chosen");
        } else if (st.equals("custom-ok") && active.zipSha != null && active.zipSha.equals(DriverLibrary.selected(base(c)))) {
            File b = base(c);
            DriverGuard.State s = readState(b), f = DriverGuard.okReached(s, active.attempt);
            if (f.equals(s)) return;
            try { write(new File(b, "guard.txt"), DriverGuard.text(f)); AppArt.log("driver: chosen driver running, older notice cleared"); }
            catch (IOException e) { AppArt.log("driver: notice not cleared: " + e); }
        }
    }

    static boolean noticeDue(Context c) { File g = new File(base(c), "guard.txt"); return g.isFile() && !readState(base(c)).notice.isEmpty(); }

    static void noticeShown(Context c) {
        File b = base(c);
        if (!new File(b, "guard.txt").isFile()) return;
        DriverGuard.State s = readState(b);
        if (s.notice.isEmpty()) return;
        try { write(new File(b, "guard.txt"), DriverGuard.text(DriverGuard.noticeShown(s))); } catch (IOException e) { AppArt.log("driver: notice state not written: " + e); }
    }

    static Info info(Context c) {
        Info i = new Info();
        i.selected = selected; i.label = selectedLabel; i.sha = shortSha(selectedSha);
        String st = active == null ? "system" : DmActivity.nativeDriverState();
        i.state = st == null ? "" : st;
        if (new File(base(c), "guard.txt").isFile()) { DriverGuard.State s = readState(base(c)); i.notice = s.notice; i.noticeArg = s.noticeArg; }
        return i;
    }


    static String noticeText(Context c, String notice, String arg) {
        return switch (notice) {
            case DriverGuard.FELL_BACK -> AppText.get(c, R.string.dmAndroidDriverFellBack);
            case DriverGuard.LOAD_FAILED -> AppText.get(c, R.string.dmAndroidDriverLoadFailed, arg);
            case DriverGuard.REJECTED -> AppText.get(c, R.string.dmAndroidDriverRejected, arg);
            case DriverGuard.SAFE -> AppText.get(c, R.string.dmAndroidDriverSafe);
            default -> "";
        };
    }



    static List<DriverLibrary.Entry> drivers(Context c) { return DriverLibrary.list(base(c)); }
    static String selectedDriver(Context c) { return DriverLibrary.selected(base(c)); }
    static String blockedDriver(Context c) { return readState(base(c)).blocked; }




    static String runningDriver() {
        Config a = active;
        if (a == null || a.zipSha == null) return "";
        String st = DmActivity.nativeDriverState();
        return st == null || st.startsWith("fallback:") ? "" : a.zipSha;
    }




    static boolean selectDriver(Context c, String sha) {
        File b = base(c);
        try { DriverLibrary.select(b, sha, DriverStore::syncDir); } catch (IOException e) { log("selection not written: " + e); return false; }
        DriverGuard.State s = readState(b), f = DriverGuard.chosenAgain(s, sha);
        if (!f.equals(s)) { try { write(new File(b, "guard.txt"), DriverGuard.text(f)); } catch (IOException e) { log("guard state not written: " + e); } }
        updateShortcut(c, !sha.isEmpty());
        return true;
    }






    static synchronized boolean deleteDriver(Context c, String sha) {
        File b = base(c);
        Config a = active;
        List<String> keep = a != null && !a.setKey.isEmpty() ? List.of(a.setKey) : List.of();
        try {
            if (!DriverLibrary.delete(b, sha, keep, DriverStore::syncDir)) return false;
            DriverGuard.State s = readState(b), f = DriverGuard.forgotten(s, sha);
            if (!f.equals(s)) write(new File(b, "guard.txt"), DriverGuard.text(f));
            return true;
        } catch (IOException e) { log("driver not deleted: " + e); return false; }
    }











    static DriverLibrary.Add addZip(Context c, InputStream in) {
        File b = base(c), incoming = new File(b, DriverLibrary.INCOMING);
        DriverLibrary.Add a = new DriverLibrary.Add();
        try {
            if (!b.isDirectory() && !b.mkdirs()) { a.error = "storage"; return a; }
            try (FileOutputStream out = new FileOutputStream(incoming)) {
                if (!DriverZip.copyBounded(in, out, DriverLibrary.MAX_INCOMING)) { a.error = "zip-size"; return a; }
                out.getFD().sync();
            }
            String unfit = unfit(c);
            if (unfit != null) { a.error = unfit; return a; }


            if (!new File(DriverLibrary.libDir(b), DriverLibrary.sha256(incoming) + ".zip").isFile()) {
                String bad = checkZip(c, b, incoming);
                if (bad != null) { a.error = bad; log("driver rejected: " + bad); return a; }
            }
            a = DriverLibrary.add(b, incoming, DriverStore::syncDir);
            return a;
        } catch (IOException e) { a.error = "zip-read"; log("driver add failed: " + e); return a; }
        finally { incoming.delete(); try { in.close(); } catch (IOException e) { } }
    }




    private static String checkZip(Context c, File b, File zip) {
        DriverZip.Meta m = DriverZip.inspect(zip, Build.VERSION.SDK_INT);
        if (m.error != null) return m.error;
        File tmp = new File(b, ".chk-" + attemptId());
        try {
            if (!tmp.mkdirs()) return "storage";
            return checkLibraries(zip, m, tmp, Os.sysconf(OsConstants._SC_PAGESIZE)).error;
        } catch (IOException e) { return "zip-read"; }
        finally { deleteTree(tmp); }
    }




    private static DriverElf.Result checkLibraries(File zip, DriverZip.Meta m, File out, long pageSize) throws IOException {
        DriverElf.Result res = new DriverElf.Result();
        if (!DriverZip.extract(zip, out)) { res.error = "zip-unpacked"; return res; }
        for (String n : m.libraries) {
            DriverElf.Result one = DriverElf.check(new File(out, n), pageSize, n.equals(m.library));
            if (one.error != null) { res.error = one.error; return res; }
            if (n.equals(m.library)) res.needed.addAll(one.needed);
        }
        return res;
    }




    private static String unfit(Context c) {
        String[] abis = Build.SUPPORTED_ABIS;
        if (abis.length == 0 || !abis[0].equals("arm64-v8a") || !new File("/dev/kgsl-3d0").exists()) return "device";
        try (ZipFile apk = new ZipFile(c.getApplicationInfo().sourceDir)) {
            for (String n : new String[]{LOADER, HOOKS[0], HOOKS[1]}) if (apk.getEntry("lib/arm64-v8a/" + n) == null) return "build";
        } catch (IOException e) { return "build"; }
        return null;
    }

    static final class Imported { String error, zipSha = "", setSha = "", library = "", label = ""; File soDir, hooks; }











    static Imported importZip(Context c, File b, File zip) {
        Imported r = new Imported();
        String key = zip.getName() + "|" + zip.length() + "|" + zip.lastModified();
        String[] cache = lines(new File(b, "import.txt"));
        boolean sameZip = cache.length >= 6 && cache[0].equals(key);
        boolean settled = false;


        if (sameZip && DriverLibrary.recordTrusted(cache)) {
            r.zipSha = cache[1]; r.setSha = cache[2]; r.library = cache[3]; r.label = cache[4]; r.error = cache[5].isEmpty() ? null : cache[5];
            if (r.error != null) settled = true;
            else {
                String now;
                try { now = DriverLibrary.verifyPlaced(b, r.setSha, r.library); } catch (IOException e) { now = null; }
                if (now == null) { r = new Imported(); }
                else {
                    settled = true;
                    if (!now.equals(r.setSha)) {
                        r.setSha = now; r.soDir = new File(b, now);
                        DriverLibrary.noteDir(zip, now);
                        try { writeImport(b, key, r); } catch (IOException e) { log("import cache not written: " + e); }
                    } else r.soDir = new File(b, now);
                }
            }
        } else if (sameZip && cache[5].isEmpty() && DriverLibrary.validSha(cache[1]) && DriverLibrary.validSha(cache[2])) {


            String now;
            try { now = DriverLibrary.migrateLegacySet(b, zip, Build.VERSION.SDK_INT, cache[2], cache[3]); } catch (IOException e) { now = null; }
            if (now != null) {
                r.zipSha = cache[1]; r.setSha = now; r.library = cache[3]; r.label = cache[4]; r.soDir = new File(b, now);
                DriverLibrary.noteDir(zip, now);
                settled = true;
                try { writeImport(b, key, r); } catch (IOException e) { log("import cache not written: " + e); }
            }
        }
        if (!settled) {
            r = fresh(c, b, zip);


            if (r.error != null && (r.error.equals("storage") || r.error.equals("so-read") || r.error.equals("zip-read"))) return r;
            try { writeImport(b, key, r); } catch (IOException e) { log("import cache not written: " + e); }
        }
        if (r.error != null) { log("package " + zip.getName() + " rejected: " + r.error); return r; }

        try {
            if (!r.setSha.equals(DriverLibrary.verifyPlaced(b, r.setSha, r.library))) {
                r.error = "so-changed"; deleteTree(r.soDir); new File(b, "import.txt").delete();
                log("package " + zip.getName() + " rejected: so-changed");
                return r;
            }
        } catch (IOException e) { r.error = "so-read"; return r; }
        try { r.hooks = hooks(c, b); } catch (IOException e) { r.error = "hooks"; log("hook libraries not copied: " + e); }
        return r;
    }



    private static void writeImport(File b, String key, Imported r) throws IOException {
        write(new File(b, "import.txt"), key + "\n" + r.zipSha + "\n" + r.setSha + "\n" + r.library + "\n" + r.label.replace('\n', ' ') + "\n"
            + (r.error == null ? "" : r.error) + "\n" + DriverLibrary.VALIDATION + "\n");
    }







    private static void gcPlaced(File b, Imported imp) {
        List<String> keep = new ArrayList<>();
        if (imp != null && imp.error == null) keep.add(imp.setSha);
        String[] cache = lines(new File(b, "import.txt"));
        if (DriverLibrary.recordTrusted(cache) && cache[5].isEmpty() && DriverLibrary.zipFor(b, cache[1]).isFile()) keep.add(cache[2]);
        try {
            int dropped = DriverLibrary.cleanPlaced(b, keep);
            if (dropped > 0) log("dropped " + dropped + " unused unpacked driver director" + (dropped == 1 ? "y" : "ies"));
        } catch (IOException e) { log("driver directories not cleaned: " + e); }
    }

    private static Imported fresh(Context c, File b, File zip) {
        Imported r = new Imported();
        File copy = new File(b, ".in-" + attemptId() + ".zip"), tmp = null;
        try {
            if (zip.length() > DriverZip.MAX_ZIP) { r.error = "zip-size"; return r; }
            try (InputStream in = new FileInputStream(zip); FileOutputStream out = new FileOutputStream(copy)) {
                if (!DriverZip.copyBounded(in, out, DriverZip.MAX_ZIP)) { r.error = "zip-size"; return r; }
                out.getFD().sync();
            }
            r.zipSha = sha256(copy);
            DriverZip.Meta m = DriverZip.inspect(copy, Build.VERSION.SDK_INT);
            if (m.error != null) { r.error = m.error; return r; }
            r.library = m.library; r.label = m.name;
            tmp = new File(b, ".tmp-" + attemptId());
            if (!tmp.mkdirs()) { r.error = "storage"; return r; }
            DriverElf.Result elf = checkLibraries(copy, m, tmp, Os.sysconf(OsConstants._SC_PAGESIZE));
            if (elf.error != null) { r.error = elf.error; return r; }
            r.setSha = DriverLibrary.placedHash(tmp);
            log("package " + zip.getName() + " ok: " + m.name + " / " + m.library + " vendor=" + m.vendor + " driverVersion=" + m.driverVersion + " minApi=" + m.minApi
                + " needs " + String.join(",", elf.needed) + " libraries " + String.join(",", m.libraries));
            File dest = new File(b, r.setSha);
            String standing = null;
            try { if (new File(dest, m.library).isFile()) standing = DriverLibrary.placedHash(dest); } catch (IOException e) { }
            if (r.setSha.equals(standing)) log("package already placed: " + dest.getName());
            else {
                deleteTree(dest);
                File[] kids = tmp.listFiles();
                boolean ro = kids != null && kids.length > 0;
                if (ro) for (File k : kids) ro &= k.isFile() && k.setReadOnly();
                if (!ro || !tmp.renameTo(dest)) { r.error = "storage"; return r; }
                tmp = null;
            }
            r.soDir = dest; dest.setWritable(false, false);
            DriverLibrary.noteDir(DriverLibrary.zipFor(b, r.zipSha), r.setSha);
            return r;
        } catch (IOException e) { r.error = r.zipSha.isEmpty() ? "zip-read" : "storage"; log("package check failed: " + e); return r; }
        finally { copy.delete(); if (tmp != null) deleteTree(tmp); }
    }



    private static File hooks(Context c, File b) throws IOException {
        long stamp; String version;
        try { PackageInfo pi = c.getPackageManager().getPackageInfo(c.getPackageName(), 0); stamp = pi.lastUpdateTime; version = Long.toString(pi.getLongVersionCode()); }
        catch (Exception e) { throw new IOException(e); }
        File root = new File(b, "hooks"), dir = new File(root, version + "-" + stamp);
        boolean ok = true;
        for (String n : HOOKS) ok &= new File(dir, n).isFile();
        if (ok) return dir;
        File[] old = root.listFiles();
        if (old != null) for (File o : old) deleteTree(o);
        File tmp = new File(root, ".tmp-" + attemptId());
        if (!tmp.mkdirs()) throw new IOException("mkdir " + tmp);
        ApplicationInfo ai = c.getApplicationInfo();
        try (ZipFile apk = new ZipFile(ai.sourceDir)) {
            for (String n : HOOKS) {
                ZipEntry e = apk.getEntry("lib/arm64-v8a/" + n);
                if (e == null) throw new IOException("no " + n + " in the APK");
                File out = new File(tmp, n);
                try (InputStream in = apk.getInputStream(e); FileOutputStream o = new FileOutputStream(out)) { if (!DriverZip.copyBounded(in, o, 16L << 20)) throw new IOException(n + " too large"); o.getFD().sync(); }
                out.setReadOnly();
            }
        } catch (IOException e) { deleteTree(tmp); throw e; }
        if (!tmp.renameTo(dir)) { deleteTree(tmp); throw new IOException("rename " + dir); }
        return dir;
    }

    private static String tuDebug(File dir) {
        String[] t = lines(new File(dir, "tu_debug.txt"));
        String v = t.length > 0 ? t[0].trim() : "";
        if (v.isEmpty()) return "";
        if (tuDebugOk(v)) return v;
        log("tu_debug.txt ignored: only " + String.join(", ", TU_DEBUG));
        return "";
    }

    static boolean tuDebugOk(String v) {
        if (v.isEmpty() || v.length() > 60 || v.startsWith(",") || v.endsWith(",") || v.contains(",,")) return false;
        outer:
        for (String p : v.split(",")) { for (String a : TU_DEBUG) if (a.equals(p)) continue outer; return false; }
        return true;
    }


    private static void updateShortcut(Context c, boolean want) {
        try {
            ShortcutManager sm = c.getSystemService(ShortcutManager.class);
            if (sm == null) return;
            boolean have = false;
            for (ShortcutInfo s : sm.getDynamicShortcuts()) have |= s.getId().equals(SHORTCUT);
            if (!want) { if (have) sm.removeDynamicShortcuts(List.of(SHORTCUT)); return; }
            Intent i = new Intent(c, DmActivity.class).setAction(Intent.ACTION_MAIN).putExtra(SAFE_EXTRA, true);
            ShortcutInfo s = new ShortcutInfo.Builder(c, SHORTCUT).setShortLabel(c.getString(R.string.dmAndroidDriverSafeShort))
                .setLongLabel(c.getString(R.string.dmAndroidDriverSafeLong)).setIcon(Icon.createWithResource(c, R.mipmap.ic_launcher)).setIntent(i).build();
            sm.setDynamicShortcuts(List.of(s));
        } catch (RuntimeException e) { log("shortcut not updated: " + e); }
    }


    static DriverGuard.State readState(File b) { String[] l = lines(new File(b, "guard.txt")); return DriverGuard.parse(l.length == 0 ? null : String.join("\n", l)); }

    private static String[] lines(File f) {
        if (!f.isFile() || f.length() > 4096) return new String[0];
        try { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8).split("\n", -1); } catch (IOException e) { return new String[0]; }
    }



    static void write(File f, String text) throws IOException {
        File tmp = new File(f.getPath() + ".tmp");
        try (FileOutputStream o = new FileOutputStream(tmp)) { o.write(text.getBytes(StandardCharsets.UTF_8)); o.getFD().sync(); }
        if (!tmp.renameTo(f)) { tmp.delete(); throw new IOException("rename " + f); }
        syncDir(f.getParentFile());
    }
    static void syncDir(File dir) throws IOException {
        try { java.io.FileDescriptor fd = Os.open(dir.getPath(), OsConstants.O_RDONLY | OsConstants.O_CLOEXEC, 0); try { Os.fsync(fd); } finally { Os.close(fd); } }
        catch (android.system.ErrnoException e) { throw new IOException(e); }
    }

    static String sha256(File f) throws IOException {
        try (InputStream in = new FileInputStream(f)) {
            MessageDigest d = MessageDigest.getInstance("SHA-256");
            byte[] buf = new byte[1 << 16]; int k;
            while ((k = in.read(buf)) > 0) d.update(buf, 0, k);
            StringBuilder s = new StringBuilder();
            for (byte x : d.digest()) s.append(String.format(Locale.ROOT, "%02x", x));
            return s.toString();
        } catch (NoSuchAlgorithmException e) { throw new IOException(e); }
    }

    static String shortSha(String s) { return s == null || s.length() < 12 || s.contains(":") ? (s == null ? "" : s) : s.substring(0, 12); }

    private static String attemptId() { byte[] r = new byte[6]; new SecureRandom().nextBytes(r); StringBuilder s = new StringBuilder(); for (byte x : r) s.append(String.format(Locale.ROOT, "%02x", x)); return s.toString(); }






    private static void deleteTree(File f) {
        if (f == null) return;
        Path p;
        try { p = f.toPath(); } catch (InvalidPathException e) { return; }
        if (!Files.exists(p, LinkOption.NOFOLLOW_LINKS)) return;
        if (!Files.isDirectory(p, LinkOption.NOFOLLOW_LINKS)) { f.delete(); return; }
        f.setWritable(true);
        File[] kids = f.listFiles();
        if (kids != null) for (File k : kids) deleteTree(k);
        f.delete();
    }
}
