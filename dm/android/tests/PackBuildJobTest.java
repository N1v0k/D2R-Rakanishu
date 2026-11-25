package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;



public class PackBuildJobTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }


    static final class Recorded implements PackBuildJob.Opened {
        final List<String> events;
        final List<PackJournal.Unit> units;
        final PackJournalTest.Writer writer = new PackJournalTest.Writer();
        final CountDownLatch inUnit = new CountDownLatch(1), cancelled = new CountDownLatch(1), released = new CountDownLatch(1);
        final String blockAt;
        Recorded(List<String> events, List<PackJournal.Unit> units, String blockAt) { this.events = events; this.units = units; this.blockAt = blockAt; }
        public List<PackJournal.Unit> units() { return units; }
        public String identity() { return "id"; }
        public String extract(PackJournal.Unit u, PackReader.Sink sink) {
            events.add("extract " + u.id);
            if (!u.id.equals(blockAt)) return writer.extract(u, sink);
            sink.wrote(1, u.outputs.get(0).file.getName());
            inUnit.countDown();
            try {
                if (!cancelled.await(10, TimeUnit.SECONDS)) return "PB-IO\tnever cancelled";
                if (!released.await(10, TimeUnit.SECONDS)) return "PB-IO\tnever released";
            } catch (InterruptedException e) { return "PB-IO\tinterrupted"; }
            return "PB-CANCEL\t";
        }
        public void cancel() { events.add("cancel"); cancelled.countDown(); }
        public void close() { events.add("close"); }
        public long missing(List<PackJournal.Unit> todo) { return 0; }
        public boolean cool(Runnable paused) { return true; }
    }

    static List<PackJournal.Unit> units(File root) { return PackBuildRun.units(PackJournalTest.listing(), new File(root, "exefs"), new File(root, "data")); }
    static PackJournal journal(File root) { return new PackJournal(new File(root, "journal.tsv"), dir -> {}, PackJournalTest.STAMP); }

    public static void main(String[] args) throws Exception {
        File tmp = Files.createTempDirectory("packjob").toFile();



        File r1 = new File(tmp, "opening");
        List<String> ev = Collections.synchronizedList(new ArrayList<>());
        List<PackBuildState> states = Collections.synchronizedList(new ArrayList<>());
        CountDownLatch opening = new CountDownLatch(1), letOpen = new CountDownLatch(1);
        Recorded rec = new Recorded(ev, units(r1), null);
        rec.released.countDown();
        PackBuildJob job = new PackBuildJob(() -> 37, () -> {
            opening.countDown();
            try { letOpen.await(10, TimeUnit.SECONDS); } catch (InterruptedException e) { throw new IOException(e); }
            return rec;
        }, journal(r1), states::add, u -> {}, () -> 0);
        Thread t = new Thread(job::run);
        t.start();
        check(opening.await(10, TimeUnit.SECONDS), "the job reaches its setup");
        job.cancel();
        letOpen.countDown();
        t.join(10_000);
        check(ev.stream().noneMatch(e -> e.startsWith("extract")), "nothing is extracted after a cancel during setup: " + ev);
        check(ev.contains("close"), "the opened reader is closed");
        check(job.end() != null && job.end().phase == PackBuildState.Phase.STOPPED && job.end().percent == 37, "the job ends stopped at the shown percentage: " + (job.end() == null ? null : job.end().format()));



        File r2 = new File(tmp, "recreate");
        List<String> order = Collections.synchronizedList(new ArrayList<>());
        Recorded first = new Recorded(order, units(r2), "00-00000001.data");
        PackBuildJob firstJob = new PackBuildJob(() -> 0, () -> first, journal(r2), s -> {}, u -> {}, () -> 0);
        CountDownLatch firstEnded = new CountDownLatch(1), secondEnded = new CountDownLatch(1);
        PackBuildControl service1 = new PackBuildControl(platform(order, "service 1", firstEnded));
        service1.start(firstJob);
        check(first.inUnit.await(10, TimeUnit.SECONDS), "the first job extracts");
        PackBuildControl service2 = new PackBuildControl(platform(order, "service 2", secondEnded));
        Recorded second = new Recorded(order, units(r2), null);
        CountDownLatch secondSetup = new CountDownLatch(1);
        service1.timeout();
        service2.start(new PackBuildJob(() -> 0, () -> { order.add("second setup"); secondSetup.countDown(); return second; }, journal(r2), s -> {}, u -> {}, () -> 0));
        second.released.countDown();
        check(!secondSetup.await(1, TimeUnit.SECONDS), "while the first job still stops, the second does not set up");
        first.released.countDown();
        check(secondSetup.await(10, TimeUnit.SECONDS) && firstEnded.await(10, TimeUnit.SECONDS) && secondEnded.await(10, TimeUnit.SECONDS), "both jobs end");
        synchronized (order) {
            int closed = order.indexOf("close"), setup = order.indexOf("second setup");
            check(closed >= 0 && setup > closed, "the second job's setup starts only after the first job's cleanup: " + order);
        }


        File r3 = new File(tmp, "journalfail");
        List<PackBuildState> s3 = Collections.synchronizedList(new ArrayList<>());
        Recorded ok = new Recorded(new ArrayList<>(), units(r3), null);
        PackJournal broken = new PackJournal(new File(r3, "journal.tsv"), dir -> { throw new IOException("sync refused"); }, PackJournalTest.STAMP);
        long[] now = {0};
        PackBuildJob failing = new PackBuildJob(() -> 12, () -> ok, broken, s3::add, u -> {}, () -> now[0] += 300);
        failing.run();
        int highest = 0;
        for (PackBuildState s : s3) highest = Math.max(highest, s.percent);
        PackBuildState last = s3.get(s3.size() - 1);
        check(last.phase == PackBuildState.Phase.ERROR && last.error.equals("PB-IO"), "a journal failure ends in PB-IO: " + last.format());
        check(last.percent == highest && highest > 12, "the failure keeps the highest percentage published (" + highest + "), got " + last.percent);
        int prev = -1; boolean even = true;
        for (PackBuildState s : s3) { if (s.percent < prev) even = false; prev = s.percent; }
        check(even, "no state of the failing run goes back");


        File r4 = new File(tmp, "retry");
        List<PackBuildState> s4 = Collections.synchronizedList(new ArrayList<>());
        CountDownLatch probing = new CountDownLatch(1), probed = new CountDownLatch(1);
        Recorded again = new Recorded(new ArrayList<>(), units(r4), null);
        PackBuildJob retry = new PackBuildJob(() -> 40, () -> {
            probing.countDown();
            try { probed.await(10, TimeUnit.SECONDS); } catch (InterruptedException e) { throw new IOException(e); }
            return again;
        }, journal(r4), s4::add, u -> {}, () -> 0);
        Thread rt = new Thread(retry::run);
        rt.start();
        check(probing.await(10, TimeUnit.SECONDS), "the retry probes");
        check(s4.isEmpty(), "nothing is published while it probes (the page keeps the last value)");
        probed.countDown();
        rt.join(10_000);
        boolean atLeast = !s4.isEmpty();
        for (PackBuildState s : s4) if (s.percent < 40) atLeast = false;
        check(atLeast && s4.get(s4.size() - 1).phase == PackBuildState.Phase.DONE, "after probing every state is at 40 or above and the build finishes");



        File r6 = new File(tmp, "stale");
        java.util.concurrent.atomic.AtomicInteger stateFile = new java.util.concurrent.atomic.AtomicInteger(40);
        CountDownLatch oldInSetup = new CountDownLatch(1), oldMayEnd = new CountDownLatch(1);
        PackBuildJob old = new PackBuildJob(stateFile::get, () -> {
            oldInSetup.countDown();
            try { oldMayEnd.await(10, TimeUnit.SECONDS); } catch (InterruptedException e) { throw new IOException(e); }
            throw new PackBuildJob.Refused("PB-SOURCE\tbase");
        }, journal(r6), st -> {}, u -> {}, () -> 0);
        Thread ot = new Thread(old::run);
        ot.start();
        check(oldInSetup.await(10, TimeUnit.SECONDS), "the old job owns the build");
        List<PackBuildState> s6 = Collections.synchronizedList(new ArrayList<>());
        Recorded fresh = new Recorded(new ArrayList<>(), units(r6), null);
        PackBuildJob next = new PackBuildJob(stateFile::get, () -> fresh, journal(r6), s6::add, u -> {}, () -> 0);
        Thread nt = new Thread(next::run);
        nt.start();
        stateFile.set(60);
        oldMayEnd.countDown();
        ot.join(10_000);
        nt.join(10_000);
        check(!s6.isEmpty() && s6.get(0).percent >= 60, "the new job starts from the old job's last state: " + (s6.isEmpty() ? "nothing" : s6.get(0).percent));


        List<PackBuildState> s5 = new ArrayList<>();
        new PackBuildJob(() -> 55, () -> { throw new PackBuildJob.Refused("PB-INPLACE\tkeys"); }, journal(new File(tmp, "refused")), s5::add, u -> {}, () -> 0).run();
        check(s5.size() == 1 && s5.get(0).error.equals("PB-INPLACE") && s5.get(0).detail.equals("keys") && s5.get(0).percent == 55, "a refused setup: its error at the shown percentage");


        List<PackBuildState> s7 = new ArrayList<>();
        new PackBuildJob(() -> 55, () -> { throw new PackBuildJob.Ended(); }, journal(new File(tmp, "ended")), s7::add, u -> {}, () -> 0).run();
        check(s7.isEmpty(), "an ended session: nothing published");

        PackJournalTest.clean(tmp);
        System.out.println("PackBuildJobTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }

    static PackBuildControl.Platform platform(List<String> order, String name, CountDownLatch ended) {
        return new PackBuildControl.Platform() {
            public void hold() { order.add(name + " hold"); }
            public void release() { order.add(name + " release"); }
            public void stop() { order.add(name + " stop"); }
            public void spawn(Runnable r) { new Thread(() -> { r.run(); ended.countDown(); }).start(); }
        };
    }
}
