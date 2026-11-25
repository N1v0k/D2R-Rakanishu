package com.xiaoboluo.d2r;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.function.IntSupplier;
import java.util.function.LongSupplier;








final class PackBuildJob implements PackBuildControl.Build {
    static final Semaphore OWNER = new Semaphore(1);


    static final class Refused extends Exception {
        private static final long serialVersionUID = 1;
        final String reader;
        Refused(String reader) { super(reader); this.reader = reader; }
    }



    interface Opened extends PackBuildRun.Conditions {
        List<PackJournal.Unit> units();
        String identity();
        String extract(PackJournal.Unit unit, PackReader.Sink sink);
        void cancel();
        void close();

        default PackSteps steps() { return PackSteps.none(); }

        default PackBuildRun.Installer installer(PackJournal journal) { return null; }

        default boolean extracts() { return true; }
    }

    static final class Finished extends Exception {
        private static final long serialVersionUID = 1;
        Finished() { super("the build's install was completed"); }
    }

    static final class Ended extends Exception {
        private static final long serialVersionUID = 1;
        Ended() { super("the build session was ended"); }
    }
    interface Setup { Opened open() throws Refused, Finished, Ended, IOException; }

    private final IntSupplier shownBefore;
    private final Setup setup;
    private final PackJournal journal;
    private final PackBuildRun.Publisher publisher;
    private final PackBuildRun.Hold hold;
    private final LongSupplier clock;
    private volatile boolean cancelled;
    private volatile Opened opened;
    private volatile PackBuildRun run;
    private volatile int last;
    private volatile String lastFile;
    private volatile PackProgress.Step lastStep;
    private volatile PackBuildState end;

    PackBuildJob(IntSupplier shownBefore, Setup setup, PackJournal journal, PackBuildRun.Publisher publisher, PackBuildRun.Hold hold, LongSupplier clock) {
        this.shownBefore = shownBefore; this.setup = setup; this.journal = journal; this.publisher = publisher; this.hold = hold; this.clock = clock;
    }

    PackBuildState end() { return end; }

    @Override public void cancel() {
        cancelled = true;
        PackBuildRun r = run;
        if (r != null) r.cancel();
        Opened o = opened;
        if (o != null) o.cancel();
    }

    @Override public void run() {
        OWNER.acquireUninterruptibly();
        try {
            last = shownBefore.getAsInt();
            if (cancelled) { publish(state(PackBuildState.Phase.STOPPED)); return; }
            Opened o;
            try { o = setup.open(); }
            catch (Refused r) { publish(PackBuildState.failed(last, lastFile, r.reader)); return; }
            catch (Finished f) { publish(new PackBuildState(PackBuildState.Phase.DONE, 100, null, null, null, null)); return; }
            catch (Ended e) { AppArt.log("pack build: no session to build (it was ended)"); return; }
            try {
                opened = o;
                if (cancelled) { publish(state(PackBuildState.Phase.STOPPED)); return; }
                PackBuildRun r = new PackBuildRun(o.units(), o.steps(), journal, o.identity(), last, new PackBuildRun.Extractor() {
                    @Override public String extract(PackJournal.Unit unit, PackReader.Sink sink) { return o.extract(unit, sink); }
                    @Override public void cancel() { o.cancel(); }
                }, o, this::publish, hold, clock);
                r.installer = o.installer(journal);
                r.extracts = o.extracts();
                run = r;
                if (cancelled) r.cancel();
                r.run();
            } finally {
                opened = null;
                o.close();
            }
        } catch (IOException | RuntimeException e) {
            AppArt.log("pack build: run failed " + e);
            try { publish(new PackBuildState(PackBuildState.Phase.ERROR, last, lastFile, "PB-IO", "")); }
            catch (IOException again) { AppArt.log("pack build: the failure could not be published " + again); }
        } finally {
            PackBuildState e = end;
            AppArt.log("pack build: run ended " + (e == null ? "without a state" : e.phase.name().toLowerCase(java.util.Locale.ROOT) + (e.error == null ? "" : " " + e.error + " " + e.detail)));
            PackLog.flush();
            OWNER.release();
        }
    }

    private PackBuildState state(PackBuildState.Phase phase) { return new PackBuildState(phase, last, lastStep, lastFile, null, null); }

    private void publish(PackBuildState s) throws IOException {
        last = Math.max(last, s.percent);
        if (s.file != null) { lastFile = s.file; lastStep = s.step; }
        end = s;
        publisher.publish(s);
    }
}
