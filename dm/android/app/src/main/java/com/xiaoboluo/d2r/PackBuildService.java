package com.xiaoboluo.d2r;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.net.Uri;
import android.os.Environment;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.PowerManager;
import android.os.StatFs;
import android.os.SystemClock;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.system.StructStat;
import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Predicate;







public class PackBuildService extends Service {
    static final String CHANNEL = "dm_pack";
    private static final int ID = 9;

    private static final long COOL_RECHECK_MS = 10_000;
    static final DriverLibrary.Sync SYNC = dir -> {
        try { FileDescriptor fd = Os.open(dir.getPath(), OsConstants.O_RDONLY | OsConstants.O_CLOEXEC, 0); try { Os.fsync(fd); } finally { Os.close(fd); } }
        catch (ErrnoException e) { throw new IOException("sync " + dir.getName(), e); }
    };

    static final PackJournal.Stamp STAMP = f -> {
        try { StructStat st = Os.stat(f.getPath()); return st.st_mtim.tv_sec * 1_000_000_000L + st.st_mtim.tv_nsec; }
        catch (ErrnoException e) { throw new IOException("stat " + f.getName(), e); }
    };

    private PackBuildControl control;
    private PowerManager.WakeLock wake;
    private int lastStart;
    private volatile boolean shown;

    static File dir(Context c) { return new File(c.getFilesDir(), "dm-build"); }
    static File exefsDir(Context c) { return new File(c.getFilesDir(), "dm-src/ExeFS"); }
    static File dataDir(Context c) {
        File shared = DmActivity.shared(c, Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS));
        return TestHooks.packDataDir(c, new File(shared, "data"));
    }


    static PackBuildState state(Context c) {
        File f = new File(dir(c), PackBuildState.FILE);
        if (!f.isFile()) return null;
        try { return PackBuildState.parse(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)); }
        catch (IOException e) { throw new IllegalStateException("pack build state unreadable", e); }
    }

    static void start(Context c) { c.startForegroundService(new Intent(c, PackBuildService.class)); }


    static void stop(Context c) { c.stopService(new Intent(c, PackBuildService.class)); }


    static PackSession session(Context c) { return new PackSession(dir(c), SYNC, uri -> PackPick.release(c, uri)); }





    static PackSpace.Check space(Context c, List<PackJournal.Unit> units, Predicate<PackJournal.Output> write, long reserve, long reclaim) {
        Map<Long, Long> free = new HashMap<>();
        List<List<PackSpace.Write>> writes = new ArrayList<>();
        for (PackJournal.Unit u : units) {
            List<PackSpace.Write> unit = new ArrayList<>();
            for (PackJournal.Output o : u.outputs) {
                File dir = existing(o.file.getParentFile());
                long vol = volume(dir);
                free.computeIfAbsent(vol, id -> TestHooks.packFree(c, new StatFs(dir.getPath()).getAvailableBytes()));
                if (write.test(o)) unit.add(new PackSpace.Write(vol, o.size, o.file.isFile() ? o.file.length() : 0));
            }
            writes.add(unit);
        }
        File own = c.getFilesDir();
        long vol = volume(own);
        if (reserve > 0 || reclaim > 0) free.computeIfAbsent(vol, id -> TestHooks.packFree(c, new StatFs(own.getPath()).getAvailableBytes()));
        if (reserve > 0) writes.add(List.of(new PackSpace.Write(vol, reserve, 0)));
        return PackSpace.check(writes, free, vol, reclaim);
    }



    static long stepsReserve(Context c) {
        return PackSpace.steps(PackSpace.size(new File(c.getFilesDir(), "py")), PackSpace.workCredit(new File(dir(c), "work"), BuildConfig.DM_PRODUCT),
            PackSpace.size(new File(dir(c), "stage")));
    }

    static long reclaimable(Context c) { return PackSpace.reclaimable(new File(dir(c), "work"), BuildConfig.DM_PRODUCT); }


    static void pruneWork(Context c) {
        try { PackSpace.pruneWork(new File(dir(c), "work"), BuildConfig.DM_PRODUCT); }
        catch (IOException e) { AppArt.log("pack build: the caches of other versions could not be removed (" + e.getClass().getSimpleName() + ")"); }
    }

    private static File existing(File f) { while (f != null && !f.exists()) f = f.getParentFile(); if (f == null) throw new IllegalStateException("no existing folder"); return f; }

    private static long volume(File dir) {
        try { return Os.stat(dir.getPath()).st_dev; } catch (ErrnoException e) { throw new IllegalStateException("stat " + dir.getName(), e); }
    }

    @Override public IBinder onBind(Intent intent) { return null; }

    @Override public void onCreate() {
        super.onCreate();
        PackLog.open(dir(this));
        PowerManager pm = getSystemService(PowerManager.class);
        wake = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "dm:packbuild");
        wake.setReferenceCounted(false);
        control = new PackBuildControl(new PackBuildControl.Platform() {
            @Override public void hold() { wake.acquire(); }
            @Override public void release() { if (wake.isHeld()) wake.release(); }
            @Override public void stop() { shown = false; stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(lastStart); }
            @Override public void spawn(Runnable r) { new Thread(r, "dm-packbuild").start(); }
        });
    }

    @Override public int onStartCommand(Intent intent, int flags, int startId) {
        lastStart = startId;
        PackBuildState s = state(this);
        startForeground(ID, notification(s == null || s.phase == PackBuildState.Phase.DONE ? 0 : s.percent), ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC);
        shown = true;
        if (!control.start(job())) AppArt.log("pack build: start ignored, a build runs");
        return START_NOT_STICKY;
    }

    @Override public void onTimeout(int startId, int fgsType) {
        AppArt.log("pack build: the platform's time limit ended the service; the running unit repeats at the next start");
        control.timeout();
    }

    @Override public void onDestroy() {
        control.timeout();
        super.onDestroy();
    }



    private PackBuildJob job() {
        Context c = this;
        PackBuildJob.Setup qa = TestHooks.packJob(c);
        return new PackBuildJob(() -> {
            PackBuildState before = state(c);
            return before == null || before.phase == PackBuildState.Phase.DONE ? 0 : before.percent;
        }, qa != null ? qa : () -> opener(c),
            new PackJournal(new File(dir(c), PackJournal.FILE), SYNC, STAMP), this::publish,
            unit -> TestHooks.packHold(c, unit), SystemClock::uptimeMillis);
    }



    private static PackBuildJob.Opened opener(Context c) throws PackBuildJob.Refused, PackBuildJob.Finished, PackBuildJob.Ended, IOException {
        PackSession session = session(c);
        if (session.finish() && !session.active()) throw new PackBuildJob.Ended();
        return session.kept() ? kept(c) : open(c);
    }



    private static PackBuildJob.Opened open(Context c) throws PackBuildJob.Refused, PackBuildJob.Finished, IOException {
        settle(c);
        pruneWork(c);
        PackPick.Sources src = PackPick.sources(c);
        if (src == null) throw new PackBuildJob.Refused("PB-SOURCE\tbase");
        String contract = TestHooks.packContract(c);
        File exefs = exefsDir(c), data = dataDir(c);
        for (File d : new File[]{dir(c), exefs, data}) if (!d.isDirectory() && !d.mkdirs()) throw new IOException("cannot create " + d.getName());
        List<PackJournal.Unit> units = PackBuildRun.units(PackReader.nativeUnits(contract), exefs, data);
        String compact = PackPick.compact(units);
        if (compact != null) throw new PackBuildJob.Refused("PB-COMPACT\t" + compact);
        PackSteps staged = TestHooks.packStaged(c, new File(dir(c), "stage"));
        PackSteps steps = staged != null ? staged : PackPython.steps(c, TestHooks.packSteps(c, PackSteps.all()), exefs, data.getParentFile(), PackSteps.inputs(units));
        ParcelFileDescriptor base = null, update = null;
        boolean ok = false;
        try {
            base = openSource(c, src.base, "base");
            update = openSource(c, src.update, "update");
            byte[] keys = null;
            boolean keysKept = false;
            ParcelFileDescriptor keyFile = openSource(c, src.keys, "keys");
            try {
                Map<String, ParcelFileDescriptor> inputs = new LinkedHashMap<>();
                inputs.put("base", base); inputs.put("update", update); inputs.put("keys", keyFile);
                String inPlace = PackPick.inPlace(inputs, units);
                if (inPlace != null) throw new PackBuildJob.Refused("PB-INPLACE\t" + inPlace);
                keys = PackPick.readKeys(keyFile);
                keyFile.close();
                keysKept = true;
            } finally {
                if (!keysKept) {
                    if (keys != null) Arrays.fill(keys, (byte) 0);
                    try { keyFile.close(); } catch (IOException closing) { AppArt.log("pack build: the key file did not close (" + closing.getClass().getSimpleName() + ")"); }
                }
            }
            if (keys == null) throw new PackBuildJob.Refused("PB-SOURCE\tkeys");
            long handle;
            try {
                String b = PackReader.nativeProbe(base.getFd(), base.getStatSize(), keys, contract), u = PackReader.nativeProbe(update.getFd(), update.getStatSize(), keys, contract);
                String refused = PackRows.recheck(PackRows.Row.BASE, b, src.baseId);
                if (refused == null) refused = PackRows.recheck(PackRows.Row.UPDATE, u, src.updateId);
                if (refused != null) throw new PackBuildJob.Refused(refused);
                String[] error = new String[1];
                handle = PackReader.nativeOpen(base.getFd(), update.getFd(), keys, contract, error);
                if (handle == 0) throw new PackBuildJob.Refused(error[0]);
            } finally { Arrays.fill(keys, (byte) 0); }
            PackBuildJob.Opened opened = new Reader(c, handle, base, update, units, steps, src.identity(), exefs, data, steps.units.isEmpty() ? 0 : stepsReserve(c));
            ok = true;
            return opened;
        } finally {
            if (!ok) for (ParcelFileDescriptor p : new ParcelFileDescriptor[]{base, update}) if (p != null) p.close();
        }
    }

    private static ParcelFileDescriptor openSource(Context c, String uri, String row) throws PackBuildJob.Refused {
        try {
            ParcelFileDescriptor p = c.getContentResolver().openFileDescriptor(Uri.parse(uri), "r");
            if (p != null) return p;
        } catch (IOException | SecurityException e) { AppArt.log("pack build: the " + row + " source could not be opened again (" + e.getClass().getSimpleName() + ")"); }
        throw new PackBuildJob.Refused("PB-SOURCE\t" + row);
    }


    private abstract static class Phone implements PackBuildJob.Opened {
        final Object lock = new Object();
        final Context context;
        private final PowerManager power;
        boolean cancelled;
        final List<PackJournal.Unit> units;
        final PackSteps steps;
        final long reserve;

        Phone(Context context, List<PackJournal.Unit> units, PackSteps steps, long reserve) {
            this.context = context; this.power = context.getSystemService(PowerManager.class); this.units = units; this.steps = steps; this.reserve = reserve;
        }
        @Override public long missing(List<PackJournal.Unit> todo) { return space(context, todo, o -> true, reserve, 0).missing; }
        @Override public long free() { return TestHooks.packFree(context, new StatFs(context.getFilesDir().getPath()).getAvailableBytes()); }
        @Override public boolean hot() { return TestHooks.packThermal(context, power.getCurrentThermalStatus()) >= PowerManager.THERMAL_STATUS_SEVERE; }
        @Override public boolean cool(Runnable paused) {
            if (!hot()) return true;
            paused.run();
            AppArt.log("pack build: paused until the phone has cooled down");
            PowerManager.OnThermalStatusChangedListener changed = status -> { synchronized (lock) { lock.notifyAll(); } };
            power.addThermalStatusListener(context.getMainExecutor(), changed);
            try {
                synchronized (lock) {
                    while (!cancelled && hot()) lock.wait(COOL_RECHECK_MS);
                    if (!cancelled) AppArt.log("pack build: cooled down, going on");
                    return !cancelled;
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return false;
            } finally { power.removeThermalStatusListener(changed); }
        }
        @Override public List<PackJournal.Unit> units() { return units; }
        @Override public PackSteps steps() { return steps; }
        @Override public PackBuildRun.Installer installer(PackJournal journal) { return complete(context, steps, journal); }
    }


    private static final class Reader extends Phone {
        private long handle;
        private final ParcelFileDescriptor base, update;
        private final String identity;
        private final File exefs, data;

        Reader(Context context, long handle, ParcelFileDescriptor base, ParcelFileDescriptor update, List<PackJournal.Unit> units, PackSteps steps, String identity, File exefs,
               File data, long reserve) {
            super(context, units, steps, reserve);
            this.handle = handle; this.base = base; this.update = update; this.identity = identity; this.exefs = exefs; this.data = data;
        }
        @Override public String identity() { return identity; }
        @Override public String extract(PackJournal.Unit unit, PackReader.Sink sink) {
            long h;
            synchronized (lock) { h = handle; }
            return h == 0 ? "PB-CANCEL\t" : PackReader.nativeExtract(h, unit.id, exefs.getPath(), data.getPath(), sink);
        }
        @Override public void cancel() { synchronized (lock) { cancelled = true; lock.notifyAll(); if (handle != 0) PackReader.nativeCancel(handle); } }
        @Override public void close() {
            synchronized (lock) { PackReader.nativeClose(handle); handle = 0; }
            for (ParcelFileDescriptor p : new ParcelFileDescriptor[]{base, update}) {
                try { p.close(); } catch (IOException e) { AppArt.log("pack build: a source did not close (" + e.getClass().getSimpleName() + ")"); }
            }
        }
    }




    private static PackBuildJob.Opened kept(Context c) throws PackBuildJob.Refused, PackBuildJob.Finished, IOException {
        settle(c);
        pruneWork(c);
        String contract = TestHooks.packContract(c);
        File exefs = exefsDir(c), data = dataDir(c);
        for (File d : new File[]{dir(c), exefs, data}) if (!d.isDirectory() && !d.mkdirs()) throw new IOException("cannot create " + d.getName());
        List<PackJournal.Unit> units = PackBuildRun.units(PackReader.nativeUnits(contract), exefs, data);
        String compact = PackPick.compact(units);
        if (compact != null) throw new PackBuildJob.Refused("PB-COMPACT\t" + compact);
        PackSteps staged = TestHooks.packStaged(c, new File(dir(c), "stage"));
        PackSteps steps = staged != null ? staged : PackPython.steps(c, TestHooks.packSteps(c, PackSteps.all()), exefs, data.getParentFile(), PackSteps.inputs(units));
        AppArt.log("pack build: a rebuild from the kept data");
        return new Kept(c, units, steps, exefs, data);
    }

    private static final class Kept extends Phone {
        private final File[] folders;
        private final String before;

        Kept(Context context, List<PackJournal.Unit> units, PackSteps steps, File exefs, File data) {
            super(context, units, steps, 0);
            folders = new File[]{exefs, data};
            before = listing(folders);
        }
        @Override public String identity() { return "kept"; }
        @Override public boolean extracts() { return false; }
        @Override public String extract(PackJournal.Unit unit, PackReader.Sink sink) { throw new IllegalStateException("a rebuild from the kept data extracts nothing"); }
        @Override public void cancel() { synchronized (lock) { cancelled = true; lock.notifyAll(); } }
        @Override public void close() {
            if (!listing(folders).equals(before)) AppArt.log("ASSUME packbuild.kept: a rebuild from the kept data changed the game data or ExeFS folder");
        }

        private static String listing(File[] folders) {
            StringBuilder s = new StringBuilder();
            for (File d : folders) {
                String[] names = d.list();
                if (names == null) continue;
                java.util.Arrays.sort(names);
                for (String n : names) {
                    if (n.endsWith(".part")) continue;
                    File f = new File(d, n);
                    s.append(d.getName()).append('/').append(n).append('\t').append(f.length()).append('\t').append(f.lastModified()).append('\n');
                }
            }
            return s.toString();
        }
    }



    static final PackInstall.Fs FS = new PackInstall.Fs() {
        @Override public long number(File f) throws IOException {
            try { return Os.lstat(f.getPath()).st_ino; } catch (ErrnoException e) { throw new IOException("stat " + f.getName(), e); }
        }
        @Override public void sync(File dir) throws IOException { SYNC.sync(dir); }
    };



    static PackInstall.Checks checks(Context c) {
        return (dm, m) -> {
            String info;
            try { info = new String(Files.readAllBytes(new File(dm, "pack_info.txt").toPath()), StandardCharsets.UTF_8); }
            catch (IOException e) { return "check: pack_info.txt unreadable"; }
            if (!PackSource.matches(info, BuildConfig.DM_PRODUCT)) return "check: pack_info.txt is not for this app";
            String lack = new AppArt(c, new File(dm, "appart")).verify();
            if (lack != null) return "check: art " + lack;
            return TestHooks.packCheckFail(c) ? "check: failed by the test" : null;
        };
    }


    static PackInstall installer(Context c) {
        return new PackInstall(c.getFilesDir(), FS, checks(c), point -> TestHooks.packHold(c, point), dm -> session(c).confirmed(dm));
    }


    static boolean importOpen(Context c) {
        File f = new File(dir(c), PackInstall.RECORD);
        if (!f.isFile()) return false;
        try { return PackInstall.Record.parse(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)).generation.equals("dm/" + PackGeneration.IMPORT); }
        catch (IOException | IllegalArgumentException e) { return false; }
    }


    static PackInstall.Result recover(Context c, boolean libraryLoaded) {
        PackLog.open(dir(c));
        try (PackLock lock = PackLock.take(dir(c))) { return installer(c).recover(libraryLoaded); }
        catch (IOException e) {
            AppArt.log("pack build: the install lock cannot be taken (" + e + ")");
            return new PackInstall.Result(PackInstall.Outcome.STOPPED, e.toString());
        } finally { PackLog.flush(); }
    }


    static boolean installOpen(Context c) {
        File b = dir(c);
        if (new File(b, PackInstall.RECORD).exists() || new File(c.getFilesDir(), PackInstall.PREV).exists() || new File(b, PackInstall.UNDO).exists()
            || new File(c.getFilesDir(), "dm/" + PackGeneration.IMPORT).exists()) return true;
        String[] names = b.list();
        if (names != null) for (String n : names) if (n.startsWith(PackGeneration.GEN)) return true;
        return false;
    }



    static void settle(Context c) throws PackBuildJob.Refused, PackBuildJob.Finished {
        boolean open = new File(dir(c), PackInstall.RECORD).exists();
        PackInstall.Result r = recover(c, false);
        if (r.outcome == PackInstall.Outcome.STOPPED) throw new PackBuildJob.Refused("PB-STEP-install\t");
        if (open && r.outcome == PackInstall.Outcome.CONFIRMED && phonePack(c)) throw new PackBuildJob.Finished();
    }

    private static boolean phonePack(Context c) { return phonePack(new File(c.getFilesDir(), "dm")); }

    static boolean phonePack(File dm) {
        try {
            PackSource.Record rec = PackSource.parse(new String(Files.readAllBytes(new File(dm, PackSource.FILE).toPath()), StandardCharsets.UTF_8));
            return rec != null && rec.source == PackSource.Kind.PHONE;
        } catch (IOException | IllegalArgumentException e) { return false; }
    }


    static PackBuildRun.Installer complete(Context c, PackSteps steps, PackJournal journal) {
        List<String> ids = new ArrayList<>();
        for (PackSteps.Unit u : steps.units) ids.add(u.id);
        if (!ids.equals(PackSteps.all())) return null;
        return (done, progress) -> installBuilt(c, steps, journal, done, progress);
    }

    static boolean hostRunning(Context c) {
        return PackHost.running(dir(c), c.getPackageName(), android.os.Process.myPid(), new File("/proc"));
    }


    static String installBuilt(Context c, PackSteps steps, PackJournal journal, Map<String, String> done, PackInstall.Progress progress) throws IOException {
        try (PackLock lock = PackLock.take(dir(c))) {
            if (hostRunning(c)) { AppArt.log("pack build: the game runs in another process; nothing is installed under it"); return "PB-STEP-install\t"; }
            PackInstall in = installer(c);
            PackInstall.Result open = in.recover(false);
            if (open.outcome == PackInstall.Outcome.STOPPED) return "PB-STEP-install\t";
            String info = "PRODUCT=" + BuildConfig.DM_PRODUCT + "\nTARGET=android\nMADE="
                + new java.text.SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss", java.util.Locale.ROOT).format(new java.util.Date()) + "\nCAPS=" + PackSource.CAP_QUICK + "\n";
            String source = PackSource.format(new PackSource.Record(PackSource.Kind.PHONE, null));
            List<PackGeneration.Step> finished = steps.finished(journal, done);
            PackGeneration.Made g;
            long[] half = {0};
            try {
                g = PackGeneration.fromStage(dir(c), steps.stage(), finished, info, source, SYNC, STAMP, (d, t) -> { half[0] = t; progress.at(d, 2 * t); });
            } catch (IOException e) {
                AppArt.log("pack build: the generation failed its check: " + e.getMessage());
                return "PB-STEP-install\t";
            }
            long base = half[0];
            PackInstall.Result r = in.commit(g.folder, g.manifest, (d, t) -> progress.at(base + d * base / Math.max(1, t), 2 * Math.max(1, base)));
            AppArt.log("pack build: install " + r);
            if (r.outcome == PackInstall.Outcome.CONFIRMED) return "";
            return r.reason != null && r.reason.startsWith("check:") ? "PB-STEP-check\t" : "PB-STEP-install\t";
        }
    }

    private void publish(PackBuildState s) throws IOException {
        DriverLibrary.writeFile(new File(dir(this), PackBuildState.FILE), s.format(), SYNC);
        if (shown && (s.phase == PackBuildState.Phase.RUNNING || s.phase == PackBuildState.Phase.PAUSED)) getSystemService(NotificationManager.class).notify(ID, notification(s.percent));
    }

    private Notification notification(int percent) {
        NotificationManager nm = getSystemService(NotificationManager.class);
        String title = AppText.get(this, R.string.dmAndroidPbNotify);
        nm.createNotificationChannel(new NotificationChannel(CHANNEL, title, NotificationManager.IMPORTANCE_LOW));
        Intent open = new Intent(this, DmActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return new Notification.Builder(this, CHANNEL).setSmallIcon(android.R.drawable.stat_sys_download).setContentTitle(title)
            .setContentText(percent + " %").setProgress(100, percent, false).setOngoing(true).setOnlyAlertOnce(true)
            .setContentIntent(PendingIntent.getActivity(this, 0, open, PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT)).build();
    }
}
