package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.LongSupplier;












final class PackBuildRun {
    static final long PUBLISH_MS = 250;
    static final String CANCEL = "PB-CANCEL", SPACE = "PB-SPACE", KEPT = "PB-KEPT";

    interface Extractor {

        String extract(PackJournal.Unit unit, PackReader.Sink sink);
        void cancel();
    }
    interface Publisher { void publish(PackBuildState state) throws IOException; }


    interface Installer { String install(Map<String, String> done, PackInstall.Progress progress) throws IOException; }
    static final long INSTALL_UNITS = 1000;
    interface Hold { void before(String unit); }

    interface Conditions {

        long missing(List<PackJournal.Unit> units);


        boolean cool(Runnable paused);

        default boolean hot() { return false; }

        default long free() { return Long.MAX_VALUE; }
    }

    private final List<PackJournal.Unit> units;
    private final PackSteps steps;
    private final PackJournal journal;
    private final String identity;
    private final int shownBefore;
    private final Extractor extractor;
    private final Conditions conditions;
    private final Publisher publisher;
    private final Hold hold;
    private final LongSupplier clock;
    private volatile boolean cancelled;
    private PackProgress progress;
    private PackProgress.Step step;
    private String file;
    private long lastPublish;
    private boolean heldBack;
    long thermalPollMs = 2000;
    Installer installer;


    boolean extracts = true;

    PackBuildRun(List<PackJournal.Unit> units, PackSteps steps, PackJournal journal, String identity, int shownBefore, Extractor extractor, Conditions conditions,
                 Publisher publisher, Hold hold, LongSupplier clock) {
        this.units = units; this.steps = steps; this.journal = journal; this.identity = identity; this.shownBefore = shownBefore;
        this.extractor = extractor; this.conditions = conditions; this.publisher = publisher; this.hold = hold; this.clock = clock;
    }


    PackBuildRun(List<PackJournal.Unit> units, PackJournal journal, String identity, int shownBefore, Extractor extractor, Conditions conditions, Publisher publisher,
                 Hold hold, LongSupplier clock) {
        this(units, PackSteps.none(), journal, identity, shownBefore, extractor, conditions, publisher, hold, clock);
    }



    static List<PackJournal.Unit> units(String listing, File exefsDir, File dataDir) {
        if (listing.startsWith("PB-")) throw new IllegalArgumentException("the reader refused the contract: " + listing);
        Map<String, List<PackJournal.Output>> outputs = new LinkedHashMap<>();
        Map<String, StringBuilder> lines = new LinkedHashMap<>();
        String last = null;
        for (String line : listing.split("\n")) {
            if (line.isEmpty()) continue;
            String[] f = line.split("\t", -1);
            if (f.length != 5) throw new IllegalArgumentException("unit line has " + f.length + " fields");
            File dir = switch (f[1]) { case "exefs" -> exefsDir; case "data" -> dataDir; default -> throw new IllegalArgumentException("unknown target " + f[1]); };
            if (f[2].isEmpty() || f[2].contains("/") || f[2].startsWith("..")) throw new IllegalArgumentException("bad file name " + f[2]);
            if (!f[0].equals(last) && outputs.containsKey(f[0])) throw new IllegalArgumentException("unit " + f[0] + " is split");
            last = f[0];
            outputs.computeIfAbsent(f[0], k -> new ArrayList<>()).add(new PackJournal.Output(new File(dir, f[2]), Long.parseLong(f[3]), f[4]));
            lines.computeIfAbsent(f[0], k -> new StringBuilder()).append(line).append('\n');
        }
        if (outputs.isEmpty()) throw new IllegalArgumentException("the reader listed no units");
        List<PackJournal.Unit> out = new ArrayList<>();
        for (Map.Entry<String, List<PackJournal.Output>> e : outputs.entrySet())
            out.add(new PackJournal.Unit(e.getKey(), DriverLibrary.sha256Bytes(lines.get(e.getKey()).toString().getBytes(StandardCharsets.UTF_8)), e.getValue()));
        return out;
    }

    void cancel() { cancelled = true; extractor.cancel(); steps.cancel(); }


    PackBuildState run() throws IOException {
        long total = 0, done;
        for (PackJournal.Unit u : units) total += u.bytes();
        Map<String, String> stepDigests = new HashMap<>();
        List<PackSteps.Unit> pendingSteps = steps.reconcile(journal, stepDigests);
        Map<PackProgress.Step, Long> totals = new EnumMap<>(PackProgress.Step.class);
        if (total > 0 && extracts) totals.put(PackProgress.Step.EXTRACT, total);
        for (PackSteps.Unit u : steps.units) totals.put(u.progress, u.files);
        if (installer != null) totals.put(PackProgress.Step.INSTALL, INSTALL_UNITS);
        progress = new PackProgress(totals, shownBefore);
        for (PackSteps.Unit u : steps.units) if (!pendingSteps.contains(u)) progress.set(u.progress, u.files);
        List<PackJournal.Unit> pending = journal.reconcile(units, identity, name -> { step = PackProgress.Step.EXTRACT; file = name; publishUnchecked(PackBuildState.Phase.RUNNING, false); });
        if (!extracts && !pending.isEmpty()) {
            AppArt.log("pack build: " + pending.size() + " units of the kept data do not hold (first " + pending.get(0).id + "); the game files are needed");
            return failed(KEPT + "\t");
        }
        done = total;
        for (PackJournal.Unit u : pending) done -= u.bytes();
        if (total > 0 && extracts) progress.set(PackProgress.Step.EXTRACT, done);
        publish(PackBuildState.Phase.RUNNING, true);
        long started = clock.getAsLong(), written = 0;
        for (int i = 0; i < pending.size(); i++) {
            PackJournal.Unit u = pending.get(i);
            List<PackJournal.Unit> rest = pending.subList(i, pending.size());
            if (cancelled) return publish(PackBuildState.Phase.STOPPED, true);
            if (heldBack) publish(PackBuildState.Phase.RUNNING, true);
            hold.before(u.id);
            long missing = conditions.missing(rest);
            if (missing > 0) return failed("PB-SPACE\t" + missing);
            boolean[] paused = {false};
            if (!conditions.cool(() -> { paused[0] = true; publishUnchecked(PackBuildState.Phase.PAUSED, true); })) return publish(PackBuildState.Phase.STOPPED, true);
            if (paused[0]) publish(PackBuildState.Phase.RUNNING, true);
            final long before = done;
            String r = extractor.extract(u, (bytes, name) -> {
                if (bytes < 0 || bytes > u.bytes()) throw new IllegalStateException("unit " + u.id + " reported " + bytes + " of " + u.bytes() + " bytes");
                progress.set(PackProgress.Step.EXTRACT, before + bytes);
                step = PackProgress.Step.EXTRACT;
                file = name;
                publishUnchecked(PackBuildState.Phase.RUNNING, false);
            });
            if (r.equals(CANCEL) || r.startsWith(CANCEL + "\t") || (cancelled && !r.isEmpty())) return publish(PackBuildState.Phase.STOPPED, true);

            if (r.equals(SPACE) || r.startsWith(SPACE + "\t")) return failed(SPACE + "\t" + Math.max(1, conditions.missing(rest)));
            if (!r.isEmpty()) return failed(r);
            journal.done(u, identity);
            done += u.bytes();
            written += u.bytes();
            progress.set(PackProgress.Step.EXTRACT, done);
        }
        AppArt.log("pack build: wrote " + written + " bytes in " + (clock.getAsLong() - started) + " ms");
        PackSteps.Listener listener = new PackSteps.Listener() {
            @Override public void wrote(PackSteps.Unit u, long files, String name) throws IOException {
                progress.set(u.progress, files);
                step = u.progress;
                file = name;
                publish(PackBuildState.Phase.RUNNING, false);
            }
            @Override public void finished(PackSteps.Unit u) throws IOException {
                progress.set(u.progress, u.files);
                if (heldBack) publish(PackBuildState.Phase.RUNNING, true);
            }
        };
        String r;
        while (true) {
            if (cancelled) return publish(PackBuildState.Phase.STOPPED, true);
            if (heldBack) publish(PackBuildState.Phase.RUNNING, true);
            if (!pendingSteps.isEmpty()) {
                boolean[] paused = {false};
                if (!conditions.cool(() -> { paused[0] = true; publishUnchecked(PackBuildState.Phase.PAUSED, true); })) return publish(PackBuildState.Phase.STOPPED, true);
                if (paused[0]) publish(PackBuildState.Phase.RUNNING, true);
            }
            if (pendingSteps.isEmpty()) { r = steps.run(pendingSteps, journal, stepDigests, identity, listener, clock); break; }
            steps.resume();
            Thermal watch = new Thermal();
            try { r = steps.run(pendingSteps, journal, stepDigests, identity, listener, clock); }
            finally { watch.end(); }
            if (!watch.tripped || cancelled || r.isEmpty()) break;
            AppArt.log("pack build: the phone is too warm; the running steps stopped and repeat once it has cooled down");
            publish(PackBuildState.Phase.PAUSED, true);
            pendingSteps = steps.reconcile(journal, stepDigests);
        }
        if (r.equals(CANCEL) || (cancelled && !r.isEmpty())) return publish(PackBuildState.Phase.STOPPED, true);
        if (!r.isEmpty()) return failed(lowSpace(r));
        if (installer != null) {
            if (cancelled) return publish(PackBuildState.Phase.STOPPED, true);
            step = PackProgress.Step.INSTALL;
            file = null;
            publish(PackBuildState.Phase.RUNNING, true);
            long t0 = clock.getAsLong();
            r = installer.install(stepDigests, (d, t) -> {
                progress.set(PackProgress.Step.INSTALL, Math.min(INSTALL_UNITS - 1, d * INSTALL_UNITS / Math.max(1, t)));
                publishUnchecked(PackBuildState.Phase.RUNNING, false);
            });
            AppArt.log("pack build: install " + (r.isEmpty() ? "done" : "failed " + r.trim()) + " after " + (clock.getAsLong() - t0) + " ms");
            if (!r.isEmpty()) return failed(lowSpace(r));
            progress.set(PackProgress.Step.INSTALL, INSTALL_UNITS);
        }
        return publish(PackBuildState.Phase.DONE, true);
    }


    private final class Thermal implements Runnable {
        private final Thread thread = new Thread(this, "dm-packbuild-thermal");
        private volatile boolean over;
        volatile boolean tripped;
        Thermal() { thread.setDaemon(true); thread.start(); }
        @Override public void run() {
            try {
                while (!over) {
                    if (conditions.hot()) { tripped = true; steps.stop(); return; }
                    Thread.sleep(thermalPollMs);
                }
            } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        }
        void end() {
            over = true;
            thread.interrupt();
            try { thread.join(); } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        }
    }


    private String lowSpace(String r) {
        long free = conditions.free();
        if (!r.startsWith("PB-STEP-") || free >= PackSpace.LOW) return r;
        AppArt.log("pack build: " + r.trim() + " with " + (free >> 20) + " MiB free: reported as space");
        return SPACE + "\t" + Math.max(1, PackSpace.STEPS - free);
    }

    private PackBuildState failed(String reader) throws IOException {
        PackBuildState s = PackBuildState.failed(progress.percent(), file, reader);
        publisher.publish(s);
        AppArt.log("pack build: " + progress.timeline() + " error " + s.error);
        return s;
    }

    private void publishUnchecked(PackBuildState.Phase phase, boolean force) {
        try { publish(phase, force); } catch (IOException e) { throw new IllegalStateException(e); }
    }

    private PackBuildState publish(PackBuildState.Phase phase, boolean force) throws IOException {
        long now = clock.getAsLong();
        if (!force && now - lastPublish < PUBLISH_MS) { heldBack = true; return null; }
        lastPublish = now;
        heldBack = false;
        PackBuildState s = new PackBuildState(phase, progress.percent(), step, file, null, null);
        publisher.publish(s);
        AppArt.log("pack build: " + progress.timeline() + (phase == PackBuildState.Phase.RUNNING ? "" : " " + phase.name().toLowerCase(java.util.Locale.ROOT)));
        return s;
    }
}
