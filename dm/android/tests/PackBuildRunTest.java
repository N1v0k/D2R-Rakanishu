package com.xiaoboluo.d2r;

import java.io.File;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;



public class PackBuildRunTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    public static void main(String[] args) throws Exception {
        File tmp = Files.createTempDirectory("packrun").toFile();



        File root = new File(tmp, "timeout");
        CountDownLatch inUnit = new CountDownLatch(1), cancelled = new CountDownLatch(1);
        List<String> platform = new ArrayList<>();
        PackJournalTest.Writer done = new PackJournalTest.Writer();
        AtomicReference<PackBuildState> end = new AtomicReference<>();
        PackBuildRun.Extractor slow = new PackBuildRun.Extractor() {
            public String extract(PackJournal.Unit u, PackReader.Sink sink) {
                if (!u.id.equals("00-00000001.data")) return done.extract(u, sink);
                sink.wrote(10, u.outputs.get(0).file.getName());
                inUnit.countDown();
                try { if (!cancelled.await(10, TimeUnit.SECONDS)) return "PB-IO\tnever cancelled"; } catch (InterruptedException e) { return "PB-IO\tinterrupted"; }
                return "PB-CANCEL\t";
            }
            public void cancel() { cancelled.countDown(); }
        };
        List<PackJournal.Unit> units = PackBuildRun.units(PackJournalTest.listing(), new File(root, "exefs"), new File(root, "data"));
        PackJournal journal = new PackJournal(new File(root, "journal.tsv"), dir -> {}, PackJournalTest.STAMP);
        PackBuildRun run = new PackBuildRun(units, journal, "id", 0, slow, PackJournalTest.FINE, s -> {}, u -> {}, System::currentTimeMillis);
        CountDownLatch workerEnded = new CountDownLatch(1);
        PackBuildControl control = new PackBuildControl(new PackBuildControl.Platform() {
            public synchronized void hold() { platform.add("hold"); }
            public synchronized void release() { platform.add("release"); }
            public synchronized void stop() { platform.add("stop"); }
            public void spawn(Runnable r) { new Thread(() -> { r.run(); workerEnded.countDown(); }).start(); }
        });
        control.start(new PackBuildControl.Build() {
            public void run() { try { end.set(run.run()); } catch (Exception e) { end.set(new PackBuildState(PackBuildState.Phase.ERROR, 0, null, "PB-IO", e.toString().replace('\n', ' '))); } }
            public void cancel() { run.cancel(); }
        });
        check(inUnit.await(10, TimeUnit.SECONDS), "the run reaches the long unit");
        check(!control.start(new PackBuildControl.Build() { public void run() {} public void cancel() {} }), "a second start while running is ignored");
        control.timeout();
        synchronized (platform) { check(platform.equals(List.of("hold", "release", "stop")), "the time limit releases the wake lock and stops at once: " + platform); }
        check(workerEnded.await(10, TimeUnit.SECONDS), "the abandoned run ends by itself");
        check(end.get() != null && end.get().phase == PackBuildState.Phase.STOPPED, "the run ends stopped: " + (end.get() == null ? null : end.get().format()));
        synchronized (platform) { check(platform.size() == 3, "the worker's end does not release or stop a second time: " + platform); }
        check(!journal.load().containsKey("00-00000001.data") && journal.load().containsKey("meta"), "the journal keeps the finished units, not the abandoned one");
        PackJournalTest.Writer next = new PackJournalTest.Writer();
        PackBuildState after = new PackBuildRun(units, journal, "id", end.get().percent, next, PackJournalTest.FINE, s -> {}, u -> {}, () -> 0).run();
        check(after.phase == PackBuildState.Phase.DONE, "the next start finishes");
        check(next.runs.keySet().equals(java.util.Set.of("00-00000001.data", "00-00000002.data")), "the next start repeats only the abandoned unit and runs the rest: " + next.runs);


        List<String> order = new ArrayList<>();
        CountDownLatch release = new CountDownLatch(1), secondRan = new CountDownLatch(1);
        PackBuildControl c2 = new PackBuildControl(new PackBuildControl.Platform() {
            public synchronized void hold() { order.add("hold"); }
            public synchronized void release() { order.add("release"); }
            public synchronized void stop() { order.add("stop"); }
            public void spawn(Runnable r) { new Thread(r).start(); }
        });
        c2.start(new PackBuildControl.Build() {
            public void run() { try { release.await(10, TimeUnit.SECONDS); } catch (InterruptedException e) { Thread.currentThread().interrupt(); } synchronized (order) { order.add("first ended"); } }
            public void cancel() {}
        });
        c2.timeout();
        check(c2.start(new PackBuildControl.Build() { public void run() { synchronized (order) { order.add("second ran"); } secondRan.countDown(); } public void cancel() {} }), "a start after the time limit is taken");
        release.countDown();
        check(secondRan.await(10, TimeUnit.SECONDS), "the queued start runs once the cancelled run ended");
        synchronized (order) { check(order.indexOf("first ended") < order.indexOf("second ran") && order.indexOf("second ran") >= 0, "never two runs at once: " + order); }


        File f = new File(tmp, "fail");
        List<PackJournal.Unit> u2 = PackBuildRun.units(PackJournalTest.listing(), new File(f, "exefs"), new File(f, "data"));
        PackJournalTest.Writer ok = new PackJournalTest.Writer();
        PackBuildState failed = new PackBuildRun(u2, new PackJournal(new File(f, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", 0, new PackBuildRun.Extractor() {
            public String extract(PackJournal.Unit u, PackReader.Sink sink) { return u.id.equals("00-00000001.data") ? "PB-DAMAGED\t00-00000001.data" : ok.extract(u, sink); }
            public void cancel() {}
        }, PackJournalTest.FINE, s -> {}, u -> {}, () -> 0).run();
        check(failed.phase == PackBuildState.Phase.ERROR && failed.error.equals("PB-DAMAGED") && failed.detail.equals("00-00000001.data"), "a damaged archive: " + failed.format());
        check(failed.percent > 0 && failed.percent < 100, "the bar stays where the work was: " + failed.percent);


        File p = new File(tmp, "publish");
        List<PackJournal.Unit> u3 = PackBuildRun.units(PackJournalTest.listing(), new File(p, "exefs"), new File(p, "data"));
        AtomicLong clock = new AtomicLong();
        List<PackBuildState> states = new ArrayList<>();
        PackJournalTest.Writer w = new PackJournalTest.Writer();
        new PackBuildRun(u3, new PackJournal(new File(p, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", 0, new PackBuildRun.Extractor() {
            public String extract(PackJournal.Unit u, PackReader.Sink sink) {
                return w.extract(u, (bytes, name) -> { clock.addAndGet(100); sink.wrote(bytes, name); });
            }
            public void cancel() {}
        }, PackJournalTest.FINE, states::add, u -> {}, clock::get).run();
        check(states.get(0).phase == PackBuildState.Phase.RUNNING && states.get(0).percent == 0, "the first state: running at 0");
        check(states.get(states.size() - 1).phase == PackBuildState.Phase.DONE && states.get(states.size() - 1).percent == 100, "the last state: done at 100");
        int last = -1; boolean even = true;
        for (PackBuildState s : states) { if (s.percent < last) even = false; last = s.percent; }
        check(even, "the published percentage never moves backwards");
        check(states.size() < 8, "reports every 100 ms are thinned to at most four a second: " + states.size() + " states for " + clock.get() + " ms");
        boolean named = false;
        boolean stepped = true;
        for (PackBuildState s : states) if (s.phase == PackBuildState.Phase.RUNNING && s.file != null) { named = true; stepped &= s.step == PackProgress.Step.EXTRACT; }
        check(named, "running states name the file being written");
        check(stepped, "and the step writing it (the extraction)");



        File hb = new File(tmp, "heldback");
        List<PackJournal.Unit> u4 = PackBuildRun.units(PackJournalTest.listing(), new File(hb, "exefs"), new File(hb, "data"));
        List<PackBuildState> seen = new ArrayList<>();
        List<String> wrong = new ArrayList<>();
        long total = 0;
        for (PackJournal.Unit u : u4) total += u.bytes();
        final long all = total;
        new PackBuildRun(u4, new PackJournal(new File(hb, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", 0, new PackJournalTest.Writer(), PackJournalTest.FINE, seen::add, unit -> {
            long before = 0;
            for (PackJournal.Unit u : u4) { if (u.id.equals(unit)) break; before += u.bytes(); }
            int expected = (int) Math.floor(before * 100.0 / all);
            int shown = seen.get(seen.size() - 1).percent;
            if (shown != expected) wrong.add(unit + " shows " + shown + " instead of " + expected);
        }, () -> 0).run();
        check(wrong.isEmpty(), "every unit starts with the units before it on the page: " + wrong);



        File sp = new File(tmp, "space");
        List<PackJournal.Unit> u5 = PackBuildRun.units(PackJournalTest.listing(), new File(sp, "exefs"), new File(sp, "data"));
        long[] free = {Long.MAX_VALUE};
        PackBuildRun.Conditions room = new PackBuildRun.Conditions() {
            public long missing(List<PackJournal.Unit> todo) { long need = 0; for (PackJournal.Unit u : todo) need += u.bytes(); return Math.max(0, need - free[0]); }
            public boolean cool(Runnable paused) { return true; }
        };
        PackJournalTest.Writer w5 = new PackJournalTest.Writer();
        PackBuildState full = new PackBuildRun(u5, new PackJournal(new File(sp, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", 0, new PackBuildRun.Extractor() {
            public String extract(PackJournal.Unit u, PackReader.Sink sink) { String r = w5.extract(u, sink); if (u.id.equals("meta")) free[0] = 10; return r; }
            public void cancel() {}
        }, room, s -> {}, u -> {}, () -> 0).run();
        long lack = u5.get(2).bytes() + u5.get(3).bytes() - 10;
        check(full.phase == PackBuildState.Phase.ERROR && full.error.equals("PB-SPACE") && full.detail.equals(Long.toString(lack)), "PB-SPACE with the bytes still missing: " + full.format());
        check(!w5.runs.containsKey("00-00000001.data") && full.percent > 0, "nothing is written once the rest does not fit, the bar stays: " + w5.runs);
        free[0] = Long.MAX_VALUE;
        PackJournalTest.Writer w6 = new PackJournalTest.Writer();
        check(new PackBuildRun(u5, new PackJournal(new File(sp, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", full.percent, w6, room, s -> {}, u -> {}, () -> 0).run().phase == PackBuildState.Phase.DONE
            && w6.runs.keySet().equals(java.util.Set.of("00-00000001.data", "00-00000002.data")), "with space again Try again writes only the rest: " + w6.runs);


        File nospace = new File(tmp, "nospace");
        List<PackJournal.Unit> u7 = PackBuildRun.units(PackJournalTest.listing(), new File(nospace, "exefs"), new File(nospace, "data"));
        PackJournalTest.Writer w7 = new PackJournalTest.Writer();
        PackBuildState enospc = new PackBuildRun(u7, new PackJournal(new File(nospace, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", 0, new PackBuildRun.Extractor() {
            public String extract(PackJournal.Unit u, PackReader.Sink sink) { return u.id.equals("meta") ? "PB-SPACE\t.build.config" : w7.extract(u, sink); }
            public void cancel() {}
        }, PackJournalTest.FINE, s -> {}, u -> {}, () -> 0).run();
        check(enospc.phase == PackBuildState.Phase.ERROR && enospc.error.equals("PB-SPACE") && enospc.detail.equals("1"), "out of space mid-unit: PB-SPACE with a byte count: " + enospc.format());


        File heat = new File(tmp, "heat");
        List<PackJournal.Unit> u8 = PackBuildRun.units(PackJournalTest.listing(), new File(heat, "exefs"), new File(heat, "data"));
        List<PackBuildState> hs = new ArrayList<>();
        int[] cools = {0};
        PackBuildRun.Conditions warm = new PackBuildRun.Conditions() {
            public long missing(List<PackJournal.Unit> todo) { return 0; }
            public boolean cool(Runnable paused) { if (cools[0]++ == 2) paused.run(); return true; }
        };
        PackJournalTest.Writer w8 = new PackJournalTest.Writer();
        PackBuildState cooled = new PackBuildRun(u8, new PackJournal(new File(heat, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", 0, w8, warm, hs::add, u -> {}, () -> 0).run();
        int pausedAt = -1;
        for (int i = 0; i < hs.size(); i++) if (hs.get(i).phase == PackBuildState.Phase.PAUSED) pausedAt = i;
        check(cooled.phase == PackBuildState.Phase.DONE && pausedAt > 0 && hs.get(pausedAt + 1).phase == PackBuildState.Phase.RUNNING && w8.runs.size() == 4,
            "a pause for heat shows PAUSED, then the build goes on: " + hs.stream().map(x -> x.phase.name()).toList());
        check(hs.get(pausedAt).percent == hs.get(pausedAt - 1).percent, "the bar holds while paused");
        File heat2 = new File(tmp, "heat2");
        List<PackJournal.Unit> u9 = PackBuildRun.units(PackJournalTest.listing(), new File(heat2, "exefs"), new File(heat2, "data"));
        PackJournalTest.Writer w9 = new PackJournalTest.Writer();
        PackBuildState stopped = new PackBuildRun(u9, new PackJournal(new File(heat2, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), "id", 0, w9, new PackBuildRun.Conditions() {
            public long missing(List<PackJournal.Unit> todo) { return 0; }
            public boolean cool(Runnable paused) { paused.run(); return false; }
        }, s -> {}, u -> {}, () -> 0).run();
        check(stopped.phase == PackBuildState.Phase.STOPPED && w9.runs.isEmpty(), "cancelled while cooling: stopped, nothing written");


        File kd = new File(tmp, "kept");
        List<PackJournal.Unit> ku = PackBuildRun.units(PackJournalTest.listing(), new File(kd, "exefs"), new File(kd, "data"));
        check(PackJournalTest.run(kd, new PackJournalTest.Writer()).phase == PackBuildState.Phase.DONE, "the first build writes the data the phone keeps");
        PackJournal kj = new PackJournal(new File(kd, "journal.tsv"), dir -> {}, PackJournalTest.STAMP);
        PackBuildRun.Extractor never = new PackBuildRun.Extractor() {
            public String extract(PackJournal.Unit u, PackReader.Sink sink) { throw new IllegalStateException("a rebuild from the kept data extracted " + u.id); }
            public void cancel() {}
        };
        List<PackBuildState> ks = new ArrayList<>();
        int[] installed = {0};
        PackBuildRun kr = new PackBuildRun(ku, kj, "kept", 0, never, PackJournalTest.FINE, ks::add, u -> {}, () -> 0);
        kr.extracts = false;
        kr.installer = (doneSteps, progress) -> { installed[0]++; progress.at(1, 2); progress.at(2, 2); return ""; };
        PackBuildState kept = kr.run();
        check(kept.phase == PackBuildState.Phase.DONE && installed[0] == 1, "kept data that holds: built and installed without extracting: " + kept.format());
        check(ks.get(0).percent == 0 && ks.get(ks.size() - 1).percent == 100, "the bar runs from 0 to 100 without the extraction's share: " + ks.stream().map(x -> x.percent).toList());
        int prev = -1; boolean up = true;
        for (PackBuildState x : ks) { if (x.percent < prev) up = false; prev = x.percent; }
        check(up, "and never backwards");

        File archive = new File(new File(kd, "data"), "00-00000001.data");
        check(archive.setLastModified(archive.lastModified() - 5000), "the archive's time can be changed");
        kr = new PackBuildRun(ku, kj, "kept", 0, never, PackJournalTest.FINE, s0 -> {}, u -> {}, () -> 0);
        kr.extracts = false;
        kr.installer = (doneSteps, progress) -> "";
        check(kr.run().phase == PackBuildState.Phase.DONE, "a changed time with equal bytes is adopted, the rebuild goes on");

        String[] cases = {"bytes", "missing", "new unit"};
        for (String what : cases) {
            File d = new File(tmp, "kept-" + what.replace(' ', '-'));
            List<PackJournal.Unit> u = PackBuildRun.units(PackJournalTest.listing(), new File(d, "exefs"), new File(d, "data"));
            PackJournalTest.run(d, new PackJournalTest.Writer());
            File a = new File(new File(d, "data"), "00-00000002.data");
            List<PackJournal.Unit> wanted = u;
            if (what.equals("bytes")) { byte[] b = Files.readAllBytes(a.toPath()); b[3] ^= 1; Files.write(a.toPath(), b); }
            if (what.equals("missing")) check(a.delete(), "the archive can be removed");
            if (what.equals("new unit")) {
                byte[] c = PackJournalTest.content("new.data");
                wanted = PackBuildRun.units(PackJournalTest.listing() + "new\tdata\tnew.data\t" + c.length + "\t" + DriverLibrary.sha256Bytes(c) + "\n", new File(d, "exefs"), new File(d, "data"));
            }
            PackJournal j = new PackJournal(new File(d, "journal.tsv"), dir -> {}, PackJournalTest.STAMP);
            java.util.Set<String> before = j.load().keySet();
            java.util.Set<String> files = new java.util.TreeSet<>(List.of(new File(d, "data").list()));
            PackBuildRun r = new PackBuildRun(wanted, j, "kept", 0, never, PackJournalTest.FINE, s0 -> {}, x -> {}, () -> 0);
            r.extracts = false;
            int[] calls = {0};
            r.installer = (doneSteps, progress) -> { calls[0]++; return ""; };
            PackBuildState e = r.run();
            check(e.phase == PackBuildState.Phase.ERROR && e.error.equals("PB-KEPT") && calls[0] == 0, what + ": the rebuild asks for the game files (PB-KEPT), nothing installed: " + e.format());
            check(j.load().keySet().containsAll(before), what + ": the journal keeps its lines");
            check(new java.util.TreeSet<>(List.of(new File(d, "data").list())).equals(files), what + ": nothing in the data folder was removed");
        }


        for (String bad : new String[]{"", "PB-NOTGAME\tcontract", "u\tsomewhere\tx\t1\t" + "a".repeat(64), "u\tdata\t../x\t1\t" + "a".repeat(64),
                                       "a\tdata\tx\t1\t" + "a".repeat(64) + "\nb\tdata\ty\t1\t" + "a".repeat(64) + "\na\tdata\tz\t1\t" + "a".repeat(64), "u\tdata\tx\t1\tnothex"}) {
            boolean refused = false;
            try { PackBuildRun.units(bad, tmp, tmp); } catch (IllegalArgumentException e) { refused = true; }
            check(refused, "listing refused: " + bad.replace("\n", "|").replace("\t", " "));
        }
        List<PackJournal.Unit> parsed = PackBuildRun.units(PackJournalTest.listing(), new File("/e"), new File("/d"));
        check(parsed.size() == 4 && parsed.get(0).outputs.size() == 2 && parsed.get(0).outputs.get(0).file.getPath().equals("/e/main") && parsed.get(1).outputs.get(0).file.getPath().equals("/d/.build.config"),
            "listing parsed into units with their targets");


        PackBuildState s = new PackBuildState(PackBuildState.Phase.ERROR, 41, "00-00000003.data", "PB-KEYWRONG", "titlekek_0a");
        PackBuildState back = PackBuildState.parse(s.format());
        check(back.phase == s.phase && back.percent == 41 && back.file.equals(s.file) && back.error.equals("PB-KEYWRONG") && back.detail.equals("titlekek_0a"), "state round trip");
        back = PackBuildState.parse(new PackBuildState(PackBuildState.Phase.RUNNING, 3, null, null, null).format());
        check(back.file == null && back.error == null, "state without file or error");
        for (String bad : new String[]{"phase\tdone\npercent\t40\n", "phase\terror\npercent\t1\n", "percent\t5\n", "phase\trunning\npercent\t101\n", "phase\trunning\npercent\t1\nwhat\tx\n"}) {
            boolean refused = false;
            try { PackBuildState.parse(bad); } catch (IllegalArgumentException e) { refused = true; }
            check(refused, "state refused: " + bad.replace("\n", "|").replace("\t", " "));
        }

        PackJournalTest.clean(tmp);
        System.out.println("PackBuildRunTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
