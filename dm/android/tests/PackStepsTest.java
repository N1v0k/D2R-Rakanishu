package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Deque;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;




public class PackStepsTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    static final String EXPECT = "# step\tprogress files\toutput files\toutput path digest\n"
        + "rewrite\t3\t2\t" + "a".repeat(64) + "\nart\t5\t1\t" + "b".repeat(64) + "\noverlay\t2\t2\t" + "c".repeat(64) + "\napp\t4\t4\t" + "d".repeat(64) + "\n"
        + "pictures\t3\t3\t" + "f".repeat(64) + "\nshaders\t6\t6\t" + "e".repeat(64) + "\n";
    static final List<String> PY = List.of("rewrite", "art", "overlay", "app");




    static final class Fake implements PackSteps.Launcher {
        final File stage;
        final Map<String, String> content = new HashMap<>();
        final List<List<String>> started = new ArrayList<>();
        final List<String> ends = Collections.synchronizedList(new ArrayList<>());
        Deque<String> script = new ArrayDeque<>();
        final List<String> keeps = new ArrayList<>();
        final List<String> prints = new ArrayList<>();
        int exit;
        boolean drains;
        CountDownLatch reached, proceed;
        Fake(File stage) { this.stage = stage; }

        String writeStep(String step) {
            try {
                String text = content.getOrDefault(step, "out of " + step);
                StringBuilder m = new StringBuilder();
                for (String name : new String[]{step + "/a", step + "/b"}) {
                    File f = new File(stage, name);
                    f.getParentFile().mkdirs();
                    Files.write(f.toPath(), (text + " " + name).getBytes(StandardCharsets.UTF_8));
                    m.append(name).append('\t').append(f.length()).append('\t').append(DriverLibrary.sha256(f)).append('\n');
                }
                File man = new File(stage, ".units/" + step + ".tsv");
                man.getParentFile().mkdirs();
                Files.write(man.toPath(), m.toString().getBytes(StandardCharsets.UTF_8));
                return "done\t" + step + "\t" + DriverLibrary.sha256Bytes(m.toString().getBytes(StandardCharsets.UTF_8));
            } catch (IOException e) { throw new IllegalStateException(e); }
        }

        String writePart(String step, String part) {
            try {
                File f = new File(stage, step + "/" + part + ".xgs");
                f.getParentFile().mkdirs();
                Files.write(f.toPath(), ("bundle " + part).getBytes(StandardCharsets.UTF_8));
                String m = step + "/" + part + ".xgs\t" + f.length() + "\t" + DriverLibrary.sha256(f) + "\n";
                File man = new File(stage, ".units/" + step + "/" + part + ".tsv");
                man.getParentFile().mkdirs();
                Files.write(man.toPath(), m.getBytes(StandardCharsets.UTF_8));
                return "part\t" + step + "\t" + part + "\t" + DriverLibrary.sha256Bytes(m.getBytes(StandardCharsets.UTF_8));
            } catch (IOException e) { throw new IllegalStateException(e); }
        }

        @Override public PackSteps.Session start(List<String> steps) {
            started.add(List.copyOf(steps));
            File keep = new File(stage, ".units/shaders.keep");
            try {
                String text = keep.isFile() ? new String(Files.readAllBytes(keep.toPath()), StandardCharsets.UTF_8) : null;
                String print = text != null && text.startsWith("fingerprint\t") ? text.substring(12, text.indexOf('\n')) : null;
                keeps.add(print == null ? text : text.substring(text.indexOf('\n') + 1));
                prints.add(print);
                if (print != null) {
                    File f = new File(stage, ".units/shaders/.fingerprint");
                    f.getParentFile().mkdirs();
                    Files.write(f.toPath(), (print + "\n").getBytes(StandardCharsets.UTF_8));
                }
            } catch (IOException e) { throw new IllegalStateException(e); }
            Deque<String> lines = script;
            script = new ArrayDeque<>();
            return new PackSteps.Session() {
                volatile boolean cancelled;
                public String readLine() {
                    while ((drains || !cancelled) && !lines.isEmpty()) {
                        String l = lines.poll();
                        if (l.equals("wait")) {
                            reached.countDown();
                            try { proceed.await(10, TimeUnit.SECONDS); } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
                            continue;
                        }
                        if (l.endsWith("+")) { String[] sp = l.substring(0, l.length() - 1).split("/"); return writePart(sp[0], sp[1]); }
                        if (l.endsWith("~")) { String[] sp = l.substring(0, l.length() - 1).split("/"); writePart(sp[0], sp[1]); continue; }
                        return l.endsWith("!") ? writeStep(l.substring(0, l.length() - 1)) : l;
                    }
                    return null;
                }
                public int finish() { ends.add("finish"); return cancelled && !drains ? 137 : exit; }
                public void cancel() { ends.add("cancel"); cancelled = true; }
            };
        }
    }


    static final class Sweep implements PackSteps.Sweeper {
        final Deque<Boolean> answers = new ArrayDeque<>();
        final List<String> calls = Collections.synchronizedList(new ArrayList<>());
        @Override public boolean sweep(String why) { calls.add(why); Boolean a = answers.poll(); return a == null || a; }
    }
    static final Sweep SWEPT = new Sweep();

    static PackSteps steps(File stage, String base, Fake fake) { return steps(stage, base, fake, SWEPT); }
    static PackSteps steps(File stage, String base, Fake fake, Sweep sweep) { return new PackSteps(PackSteps.units(PY, EXPECT), stage, base, fake, PackJournalTest.STAMP, sweep); }
    static PackSteps shaders(File stage, String deps, Fake fake) {
        return new PackSteps(PackSteps.units(List.of("shaders"), EXPECT, Map.of("shaders", deps)), stage, "b", fake, PackJournalTest.STAMP, SWEPT);
    }
    static PackJournal journal(File root) { return new PackJournal(new File(root, "journal.tsv"), dir -> {}, PackJournalTest.STAMP); }
    static List<String> ids(List<PackSteps.Unit> units) { List<String> out = new ArrayList<>(); for (PackSteps.Unit u : units) out.add(u.id); return out; }
    static final PackSteps.Listener QUIET = new PackSteps.Listener() { public void wrote(PackSteps.Unit u, long n, String f) {} public void finished(PackSteps.Unit u) {} };

    static void script(Fake f, String... lines) { f.script = new ArrayDeque<>(List.of(lines)); }

    public static void main(String[] args) throws Exception {
        File tmp = Files.createTempDirectory("packsteps").toFile();


        List<PackSteps.Unit> all = PackSteps.units(List.of("app", "overlay", "art", "rewrite"), EXPECT);
        check(ids(all).equals(List.of("rewrite", "art", "overlay", "app")), "units come in the runner's order: " + ids(all));
        check(all.get(1).files == 5 && all.get(1).progress == PackProgress.Step.ART && "art".equals(all.get(2).after), "totals, progress steps and the art -> overlay link");
        check(refused(() -> PackSteps.units(List.of("overlay"), EXPECT)), "overlay without art is refused");
        check(refused(() -> PackSteps.units(List.of("rewrite"), "rewrite\t3\t2\n")), "a malformed table line is refused");
        check(refused(() -> PackSteps.units(List.of("art"), "rewrite\t3\t2\t" + "a".repeat(64) + "\n")), "a step without a row is refused");
        check(refused(() -> PackSteps.units(List.of("icons"), EXPECT)), "an unknown step is refused");
        check(PackSteps.all().equals(List.of("rewrite", "art", "overlay", "app", "pictures", "shaders")), "every step, the shader step last: " + PackSteps.all());
        PackSteps.Unit pic = PackSteps.units(List.of("pictures"), EXPECT, Map.of("pictures", "monpic=1")).get(0);
        check(pic.progress == PackProgress.Step.PICTURES && pic.files == 3 && pic.after == null && !pic.resumable && pic.deps.equals("monpic=1"),
            "the pictures step fills the pictures share, with its library among its dependencies");
        PackSteps.Unit sh = PackSteps.units(List.of("shaders"), EXPECT).get(0);
        check(sh.resumable && sh.progress == PackProgress.Step.SHADERS && sh.files == 6 && !all.get(0).resumable, "the shader step finishes parts and fills the shader share");
        check(refused(() -> PackSteps.units(List.of("rewrite"), "rewrite\t0\t0\t" + "a".repeat(64) + "\n")), "a step counting no files is refused");


        File r1 = new File(tmp, "first"), st1 = new File(r1, "stage");
        Fake f1 = new Fake(st1);
        PackSteps s1 = steps(st1, "base1", f1);
        Map<String, String> done = new HashMap<>();
        List<PackSteps.Unit> pending = s1.reconcile(journal(r1), done);
        check(ids(pending).equals(List.of("rewrite", "art", "overlay", "app")) && done.isEmpty(), "a new build runs every step");
        List<String> heard = Collections.synchronizedList(new ArrayList<>());
        PackSteps.Listener rec = new PackSteps.Listener() {
            public void wrote(PackSteps.Unit u, long n, String f) { heard.add(u.id + " " + n + " " + f); }
            public void finished(PackSteps.Unit u) { heard.add(u.id + " done"); }
        };
        script(f1, "start\trewrite\t11", "start\tart\t12", "file\tart\t2\tbase.png", "rewrite!", "file\tart\t5\tlast.png", "art!", "start\toverlay\t13", "overlay!", "start\tapp\t14", "app!");
        String r = s1.run(pending, journal(r1), done, "id", rec, () -> 0);
        check(r.isEmpty(), "the run finishes: " + r);
        check(f1.started.equals(List.of(List.of("rewrite", "art", "overlay", "app"))), "one session for the pending steps: " + f1.started);
        check(heard.equals(List.of("art 2 base.png", "rewrite done", "art 5 last.png", "art done", "overlay done", "app done")), "the listener hears files and ends: " + heard);
        check(journal(r1).load().keySet().containsAll(List.of("rewrite", "art", "overlay", "app")), "every step is journaled");
        check(journal(r1).load().get("art").digest.equals(done.get("art")), "the journal holds the manifest digest");


        Map<String, String> again = new HashMap<>();
        check(steps(st1, "base1", f1).reconcile(journal(r1), again).isEmpty() && again.size() == 4, "a finished build has nothing pending");
        Files.write(new File(st1, "art/a").toPath(), "changed length".getBytes(StandardCharsets.UTF_8));
        check(ids(steps(st1, "base1", f1).reconcile(journal(r1), new HashMap<>())).equals(List.of("art", "overlay")), "a changed art output repeats art and overlay");
        new File(st1, "app/b").delete();
        check(ids(steps(st1, "base1", f1).reconcile(journal(r1), new HashMap<>())).equals(List.of("art", "overlay", "app")), "a missing output repeats its step");
        check(ids(steps(st1, "base2", f1).reconcile(journal(r1), new HashMap<>())).size() == 4, "other tools or inputs repeat every step");
        Files.write(s1.manifest("rewrite").toPath(), "rewrite/a\t1\tx\n".getBytes(StandardCharsets.UTF_8));
        check(ids(steps(st1, "base1", f1).reconcile(journal(r1), new HashMap<>())).get(0).equals("rewrite"), "a manifest that is not the journaled one repeats its step");


        File r2 = new File(tmp, "repeat"), st2 = new File(r2, "stage");
        Fake f2 = new Fake(st2);
        script(f2, "rewrite!", "art!", "overlay!", "app!");
        Map<String, String> d2 = new HashMap<>();
        check(steps(st2, "b", f2).run(steps(st2, "b", f2).reconcile(journal(r2), d2), journal(r2), d2, "id", QUIET, () -> 0).isEmpty(), "second fixture builds");
        Files.write(new File(st2, "art/a").toPath(), "x".getBytes(StandardCharsets.UTF_8));
        Map<String, String> d2b = new HashMap<>();
        PackSteps s2 = steps(st2, "b", f2);
        List<PackSteps.Unit> p2 = s2.reconcile(journal(r2), d2b);
        check(ids(p2).equals(List.of("art", "overlay")), "art and overlay pending after the change");
        script(f2, "art!");
        String partial = s2.run(List.of(p2.get(0)), journal(r2), d2b, "id", QUIET, () -> 0);
        check(partial.isEmpty() && ids(steps(st2, "b", f2).reconcile(journal(r2), new HashMap<>())).isEmpty(), "art rebuilt with equal outputs: overlay's record holds again");



        File r3 = new File(tmp, "fail"), st3 = new File(r3, "stage");
        Fake f3 = new Fake(st3);
        String[][] cases = {
            {"PB-STEP-overlay\t", "start\trewrite\t5", "rewrite!", "start\toverlay\t6", "error\toverlay\tPB-STEP-overlay\texit 1: missing file"},
            {"PB-STEP-runner\t", "hello"},
            {"PB-STEP-runner\t", "file\tart\t6\tx.png"},
            {"PB-STEP-runner\t", "done\tart\tnot-a-digest"},
            {"PB-STEP-runner\t", "rewrite!"},
            {"PB-STEP-runner\t", "start\tshaders\t3"},
        };
        for (String[] c : cases) {
            File rc = new File(r3, Integer.toString(c.length) + c[1].hashCode());
            Fake fc = new Fake(new File(rc, "stage"));
            PackSteps sc = steps(new File(rc, "stage"), "b", fc);
            script(fc, java.util.Arrays.copyOfRange(c, 1, c.length));
            String got = sc.run(sc.reconcile(journal(rc), new HashMap<>()), journal(rc), new HashMap<>(), "id", QUIET, () -> 0);
            check(got.equals(c[0]), "failure case " + c[1] + " gives " + c[0] + ": " + got);
        }
        Fake exitFake = new Fake(st3);
        exitFake.exit = 1;
        script(exitFake, "rewrite!", "art!", "overlay!", "app!");
        PackSteps se = steps(st3, "b", exitFake);
        check(se.run(se.reconcile(journal(r3), new HashMap<>()), journal(r3), new HashMap<>(), "id", QUIET, () -> 0).equals("PB-STEP-runner\t"), "a runner exiting with an error fails even after its lines");
        check(PackSteps.none().run(List.of(), journal(r3), new HashMap<>(), "id", QUIET, () -> 0).isEmpty(), "no steps: nothing to run");


        File r4 = new File(tmp, "cancel"), st4 = new File(r4, "stage");
        Fake f4 = new Fake(st4);
        f4.reached = new CountDownLatch(1); f4.proceed = new CountDownLatch(1);
        script(f4, "start\trewrite\t9", "rewrite!", "start\tart\t10", "wait", "art!");
        PackSteps s4 = steps(st4, "b", f4);
        String[] out = new String[1];
        Thread t = new Thread(() -> { try { out[0] = s4.run(s4.reconcile(journal(r4), new HashMap<>()), journal(r4), new HashMap<>(), "id", QUIET, () -> 0); } catch (IOException e) { out[0] = e.toString(); } });
        t.start();
        check(f4.reached.await(10, TimeUnit.SECONDS), "the run reaches the waiting step");
        s4.cancel();
        f4.proceed.countDown();
        t.join(10_000);
        check(PackBuildRun.CANCEL.equals(out[0]), "a cancelled run reports the cancel: " + out[0]);
        check(journal(r4).load().containsKey("rewrite") && !journal(r4).load().containsKey("art"), "the finished step stays journaled, the cancelled one not");


        File r5 = new File(tmp, "run");
        Fake f5 = new Fake(new File(r5, "stage"));
        List<PackJournal.Unit> units = PackBuildRun.units(PackJournalTest.listing(), new File(r5, "exefs"), new File(r5, "data"));
        script(f5, "start\trewrite\t1", "file\trewrite\t1\tmain.patched.bin", "rewrite!", "start\tart\t2", "file\tart\t3\ticon.png", "art!",
               "start\toverlay\t3", "overlay!", "start\tapp\t4", "file\tapp\t2\tfont.otf", "app!");
        List<PackBuildState> seen = Collections.synchronizedList(new ArrayList<>());
        PackBuildState end = new PackBuildRun(units, steps(new File(r5, "stage"), "b", f5), journal(r5), "id", 0, new PackJournalTest.Writer(), PackJournalTest.FINE, seen::add, u -> {}, () -> 0).run();
        check(end.phase == PackBuildState.Phase.DONE && end.percent == 100, "extraction and steps finish at 100: " + end.format());
        boolean monotonic = true;
        for (int i = 1; i < seen.size(); i++) if (seen.get(i).percent < seen.get(i - 1).percent) monotonic = false;
        check(monotonic, "the bar never goes back");
        int before = -1;
        for (PackBuildState s : seen) if ("icon.png".equals(s.file)) before = s.percent;
        check(before > 0 && before < 100, "a step's file shows on the page with its share: " + before);
        PackProgress.Step artStep = null, fontStep = null;
        for (PackBuildState s : seen) { if ("icon.png".equals(s.file)) artStep = s.step; if ("font.otf".equals(s.file)) fontStep = s.step; }
        check(artStep == PackProgress.Step.ART && fontStep == PackProgress.Step.APP_ART, "a step's file shows with that step's label: " + artStep + " / " + fontStep);
        PackBuildState second = new PackBuildRun(units, steps(new File(r5, "stage"), "b", f5), journal(r5), "id", 100, new PackJournalTest.Writer(), PackJournalTest.FINE, s -> {}, u -> {}, () -> 0).run();
        check(second.phase == PackBuildState.Phase.DONE && f5.started.size() == 1, "a finished build starts no runner again: " + f5.started);


        File r6 = new File(tmp, "alone");
        Fake f6 = new Fake(new File(r6, "stage"));
        script(f6, "start\trewrite\t1", "rewrite!", "start\tart\t2", "error\tart\tPB-STEP-art\texit 1");
        PackBuildState failed = new PackBuildRun(List.of(), steps(new File(r6, "stage"), "b", f6), journal(r6), "id", 0, new PackJournalTest.Writer(), PackJournalTest.FINE, s -> {}, u -> {}, () -> 0).run();
        check(failed.phase == PackBuildState.Phase.ERROR && "PB-STEP-art".equals(failed.error) && failed.percent > 0 && failed.percent < 100, "a failed step: " + failed.format());
        script(f6, "art!", "overlay!", "app!");
        PackBuildState retried = new PackBuildRun(List.of(), steps(new File(r6, "stage"), "b", f6), journal(r6), "id", failed.percent, new PackJournalTest.Writer(), PackJournalTest.FINE, s -> {}, u -> {}, () -> 0).run();
        check(retried.phase == PackBuildState.Phase.DONE && f6.started.get(1).equals(List.of("art", "overlay", "app")), "the retry runs only the unfinished steps: " + f6.started);



        File r8 = new File(tmp, "damage"), st8 = new File(r8, "stage");
        Fake f8 = new Fake(st8);
        script(f8, "rewrite!", "art!", "overlay!", "app!");
        Map<String, String> d8 = new HashMap<>();
        check(steps(st8, "b", f8).run(steps(st8, "b", f8).reconcile(journal(r8), d8), journal(r8), d8, "id", QUIET, () -> 0).isEmpty(), "damage fixture builds");
        File a8 = new File(st8, "art/a");
        byte[] good = Files.readAllBytes(a8.toPath()), bad = good.clone();
        bad[0] ^= 1;
        long t8 = a8.lastModified();
        Files.write(a8.toPath(), bad);
        a8.setLastModified(t8 + 5000);
        check(ids(steps(st8, "b", f8).reconcile(journal(r8), new HashMap<>())).equals(List.of("art", "overlay")), "same-length damage repeats art and overlay");
        Files.write(a8.toPath(), good);
        a8.setLastModified(t8 + 9000);
        int lines = Files.readAllLines(new File(r8, "journal.tsv").toPath()).size();
        check(steps(st8, "b", f8).reconcile(journal(r8), new HashMap<>()).isEmpty(), "a touched output with its bytes intact keeps every step done");
        check(Files.readAllLines(new File(r8, "journal.tsv").toPath()).size() == lines + 1, "and its step is journaled again with the new times");
        check(steps(st8, "b", f8).reconcile(journal(r8), new HashMap<>()).isEmpty() && Files.readAllLines(new File(r8, "journal.tsv").toPath()).size() == lines + 1,
            "the next start trusts the new times without hashing again");


        File r9 = new File(tmp, "throws"), st9 = new File(r9, "stage");
        Fake f9 = new Fake(st9);
        script(f9, "start\trewrite\t1", "rewrite!", "art!");
        new File(r9, "journal.tsv").mkdirs();
        PackSteps s9 = steps(st9, "b", f9);
        boolean threw = false;
        try { s9.run(List.of(s9.units.get(0), s9.units.get(1)), new PackJournal(new File(r9, "journal.tsv"), dir -> {}, PackJournalTest.STAMP), new HashMap<>(), "id", QUIET, () -> 0); }
        catch (IOException e) { threw = true; }
        check(threw && f9.ends.equals(List.of("cancel", "finish")), "a journal that cannot be written: the run fails and its session is cancelled, then waited for: " + f9.ends);
        File r10 = new File(tmp, "listener"), st10 = new File(r10, "stage");
        Fake f10 = new Fake(st10);
        script(f10, "start\tart\t1", "file\tart\t1\tx.png", "art!");
        PackSteps s10 = steps(st10, "b", f10);
        boolean threw10 = false;
        try {
            s10.run(List.of(s10.units.get(1)), journal(r10), new HashMap<>(), "id", new PackSteps.Listener() {
                public void wrote(PackSteps.Unit u, long n, String f) throws IOException { throw new IOException("page state cannot be written"); }
                public void finished(PackSteps.Unit u) {}
            }, () -> 0);
        } catch (IOException e) { threw10 = true; }
        check(threw10 && f10.ends.equals(List.of("cancel", "finish")), "a failing publish: the session is cancelled and waited for: " + f10.ends);
        check(!journal(r10).load().containsKey("art"), "and the step is not journaled");



        File r11 = new File(tmp, "parts"), st11 = new File(r11, "stage");
        Fake f11 = new Fake(st11);
        f11.reached = new CountDownLatch(1); f11.proceed = new CountDownLatch(1);
        script(f11, "start\tshaders\t1", "shaders/p0+", "shaders/p1+", "wait", "shaders!");
        PackSteps s11 = shaders(st11, "libs1", f11);
        String[] out11 = new String[1];
        Thread t11 = new Thread(() -> { try { out11[0] = s11.run(s11.reconcile(journal(r11), new HashMap<>()), journal(r11), new HashMap<>(), "id", QUIET, () -> 0); } catch (IOException e) { out11[0] = e.toString(); } });
        t11.start();
        check(f11.reached.await(10, TimeUnit.SECONDS), "the shader run reaches its third part");
        s11.cancel();
        f11.proceed.countDown();
        t11.join(10_000);
        check(PackBuildRun.CANCEL.equals(out11[0]) && "".equals(f11.keeps.get(0)), "a stopped shader run; its first start kept nothing: " + out11[0] + " " + f11.keeps);
        Map<String, PackJournal.Record> j11 = journal(r11).load();
        check(j11.containsKey("shaders/p0") && j11.containsKey("shaders/p1") && !j11.containsKey("shaders"), "both finished parts are journaled, the step is not");
        String keep01 = "p0\t" + j11.get("shaders/p0").digest + "\np1\t" + j11.get("shaders/p1").digest + "\n";
        script(f11, "start\tshaders\t2", "shaders/p2+", "shaders!");
        PackSteps s11b = shaders(st11, "libs1", f11);
        check(s11b.run(s11b.reconcile(journal(r11), new HashMap<>()), journal(r11), new HashMap<>(), "id", QUIET, () -> 0).isEmpty(), "the resumed shader run finishes");
        check(keep01.equals(f11.keeps.get(1)), "the resumed run keeps exactly the journaled parts: " + f11.keeps.get(1));
        check(shaders(st11, "libs1", f11).reconcile(journal(r11), new HashMap<>()).isEmpty(), "a finished shader step has nothing pending");



        new File(st11, ".units/shaders.tsv").delete();
        File b1 = new File(st11, "shaders/p1.xgs");
        byte[] g1 = Files.readAllBytes(b1.toPath()), x1 = g1.clone();
        x1[0] ^= 1;
        long tb = b1.lastModified();
        Files.write(b1.toPath(), x1);
        b1.setLastModified(tb + 5000);
        script(f11, "shaders!");
        PackSteps s11c = shaders(st11, "libs1", f11);
        s11c.run(s11c.reconcile(journal(r11), new HashMap<>()), journal(r11), new HashMap<>(), "id", QUIET, () -> 0);
        String kept11 = f11.keeps.get(2);
        check(kept11.startsWith("p0\t") && kept11.contains("p2\t") && !kept11.contains("p1\t"), "a damaged part is not kept: " + kept11);
        Files.write(b1.toPath(), g1);
        b1.setLastModified(tb + 9000);
        new File(st11, ".units/shaders.tsv").delete();
        int l11 = Files.readAllLines(new File(r11, "journal.tsv").toPath()).size();
        script(f11, "shaders!");
        PackSteps s11d = shaders(st11, "libs1", f11);
        s11d.run(s11d.reconcile(journal(r11), new HashMap<>()), journal(r11), new HashMap<>(), "id", QUIET, () -> 0);
        check(f11.keeps.get(3).contains("p1\t"), "a touched part with intact bytes is kept: " + f11.keeps.get(3));
        check(Files.readAllLines(new File(r11, "journal.tsv").toPath()).size() >= l11 + 2, "and journaled again with its new time");
        check(ids(shaders(st11, "libs2", f11).reconcile(journal(r11), new HashMap<>())).equals(List.of("shaders")), "other shader libraries repeat the step");
        new File(st11, ".units/shaders.tsv").delete();
        script(f11, "shaders!");
        PackSteps s11e = shaders(st11, "libs2", f11);
        s11e.run(s11e.reconcile(journal(r11), new HashMap<>()), journal(r11), new HashMap<>(), "id", QUIET, () -> 0);
        check("".equals(f11.keeps.get(4)), "and keep none of the parts made with the old ones: " + f11.keeps.get(4));
        check(steps(st11, "b", f11).fingerprint(steps(st11, "b", f11).units.get(0), "").equals(steps(st11, "b", f11).fingerprint(steps(st11, "b", f11).units.get(0), "")),
            "the fingerprint is stable");



        File r12 = new File(tmp, "unreported"), st12 = new File(r12, "stage");
        Fake f12 = new Fake(st12);
        f12.reached = new CountDownLatch(1); f12.proceed = new CountDownLatch(1);
        script(f12, "start\tshaders\t1", "shaders/p0+", "shaders/p1~", "shaders/p2~", "wait", "shaders!");
        PackSteps s12 = shaders(st12, "libs1", f12);
        Thread t12 = new Thread(() -> { try { s12.run(s12.reconcile(journal(r12), new HashMap<>()), journal(r12), new HashMap<>(), "id", QUIET, () -> 0); } catch (IOException e) { throw new IllegalStateException(e); } });
        t12.start();
        check(f12.reached.await(10, TimeUnit.SECONDS), "the run reaches the wait after two unreported parts");
        s12.cancel();
        f12.proceed.countDown();
        t12.join(10_000);
        check(!journal(r12).load().containsKey("shaders/p1") && f12.prints.get(0) != null, "unreported parts have no line; the keep file names the fingerprint");
        File b2 = new File(st12, "shaders/p2.xgs");
        byte[] x2 = Files.readAllBytes(b2.toPath());
        x2[0] ^= 1;
        Files.write(b2.toPath(), x2);
        script(f12, "shaders!");
        PackSteps s12b = shaders(st12, "libs1", f12);
        s12b.run(s12b.reconcile(journal(r12), new HashMap<>()), journal(r12), new HashMap<>(), "id", QUIET, () -> 0);
        String k12 = f12.keeps.get(1);
        check(k12.contains("p0\t") && k12.contains("p1\t") && !k12.contains("p2\t"), "the intact unreported part is kept, the damaged one not: " + k12);
        PackJournal.Record a12 = journal(r12).load().get("shaders/p1");
        check(a12 != null && "id".equals(a12.identity) && a12.fingerprint.equals(f12.prints.get(1)), "the kept unreported part is journaled with the run's identity");
        File r13 = new File(tmp, "unreported-other"), st13 = new File(r13, "stage");
        Fake f13 = new Fake(st13);
        f13.reached = new CountDownLatch(1); f13.proceed = new CountDownLatch(1);
        script(f13, "start\tshaders\t1", "shaders/p1~", "wait");
        PackSteps s13 = shaders(st13, "libs1", f13);
        Thread t13 = new Thread(() -> { try { s13.run(s13.reconcile(journal(r13), new HashMap<>()), journal(r13), new HashMap<>(), "id", QUIET, () -> 0); } catch (IOException e) { throw new IllegalStateException(e); } });
        t13.start();
        check(f13.reached.await(10, TimeUnit.SECONDS), "second unreported fixture waits");
        s13.cancel();
        f13.proceed.countDown();
        t13.join(10_000);
        script(f13, "shaders!");
        PackSteps s13b = shaders(st13, "libs2", f13);
        s13b.run(s13b.reconcile(journal(r13), new HashMap<>()), journal(r13), new HashMap<>(), "id", QUIET, () -> 0);
        check("".equals(f13.keeps.get(1)), "an unreported part made with other libraries is not kept: " + f13.keeps.get(1));


        String[][] partCases = {
            {"start\trewrite\t1", "part\trewrite\tp0\t" + "a".repeat(64)},
            {"shaders/p0+", "part\tshaders\t../x\t" + "a".repeat(64)},
            {"part\tshaders\tp9\t" + "a".repeat(64)},
        };
        int pc = 0;
        for (String[] c : partCases) {
            File rc = new File(tmp, "partcase" + pc++), sc = new File(rc, "stage");
            Fake fc = new Fake(sc);
            script(fc, c);
            PackSteps both = new PackSteps(PackSteps.units(List.of("rewrite", "shaders"), EXPECT), sc, "b", fc, PackJournalTest.STAMP, SWEPT);
            String got = both.run(both.reconcile(journal(rc), new HashMap<>()), journal(rc), new HashMap<>(), "id", QUIET, () -> 0);
            check("PB-STEP-runner\t".equals(got), "a part line that does not fit fails as the runner: " + String.join(" | ", c) + " -> " + got);
        }


        File r7 = new File(tmp, "noruntime");
        List<PackJournal.Unit> u7 = PackBuildRun.units(PackJournalTest.listing(), new File(r7, "exefs"), new File(r7, "data"));
        PackBuildState nort = new PackBuildRun(u7, PackSteps.unavailable(PackSteps.all()), journal(r7), "id", 0, new PackJournalTest.Writer(), PackJournalTest.FINE, s -> {}, u -> {}, () -> 0).run();
        check(nort.phase == PackBuildState.Phase.ERROR && "PB-STEP-runtime".equals(nort.error) && nort.percent > 0 && nort.percent < 100, "no runtime: the steps fail visibly: " + nort.format());
        check(journal(r7).load().keySet().containsAll(List.of("exefs", "meta")) && !journal(r7).load().containsKey("rewrite"), "no runtime: the extraction is journaled, no step");



        File r14 = new File(tmp, "late"), st14 = new File(r14, "stage");
        Fake f14 = new Fake(st14);
        f14.drains = true; f14.exit = 1;
        script(f14, "start\trewrite\t1", "start\tart\t2", "start\tapp\t3", "error\tart\tPB-STEP-art\texit 1", "rewrite!", "app!",
            "error\tapp\tPB-STEP-app\tstopped", "file\tapp\t1\tx.png");
        PackSteps s14 = steps(st14, "b", f14);
        String late = s14.run(s14.reconcile(journal(r14), new HashMap<>()), journal(r14), new HashMap<>(), "id", QUIET, () -> 0);
        Map<String, PackJournal.Record> j14 = journal(r14).load();
        check("PB-STEP-art\t".equals(late), "the first failure is shown: " + late);
        check(j14.containsKey("rewrite") && j14.containsKey("app") && !j14.containsKey("art"), "steps that finished after the error are journaled: " + j14.keySet());
        check(ids(steps(st14, "b", f14).reconcile(journal(r14), new HashMap<>())).equals(List.of("art", "overlay")), "the retry runs only the rest");



        File r15 = new File(tmp, "sweep"), st15 = new File(r15, "stage");
        Fake f15 = new Fake(st15);
        Sweep w15 = new Sweep();
        script(f15, "rewrite!", "art!", "overlay!", "app!");
        PackSteps s15 = steps(st15, "b", f15, w15);
        check(s15.run(s15.reconcile(journal(r15), new HashMap<>()), journal(r15), new HashMap<>(), "id", QUIET, () -> 0).isEmpty()
            && w15.calls.equals(List.of("before the run", "after the run")), "a clean run sweeps before and after: " + w15.calls);
        File r16 = new File(tmp, "sweepdead"), st16 = new File(r16, "stage");
        Fake f16 = new Fake(st16);
        f16.exit = 137;
        Sweep w16 = new Sweep();
        w16.answers.add(true); w16.answers.add(false);
        script(f16, "start\trewrite\t1", "rewrite!", "start\tart\t2");
        PackSteps s16 = steps(st16, "b", f16, w16);
        String dead = s16.run(s16.reconcile(journal(r16), new HashMap<>()), journal(r16), new HashMap<>(), "id", QUIET, () -> 0);
        check("PB-STEP-runner\t".equals(dead) && w16.calls.equals(List.of("before the run", "after the run")) && f16.ends.equals(List.of("finish")),
            "a runner that died: swept before run() returns: " + dead + " " + w16.calls + " " + f16.ends);
        w16.answers.add(false);
        String blocked = s16.run(s16.reconcile(journal(r16), new HashMap<>()), journal(r16), new HashMap<>(), "id", QUIET, () -> 0);
        check("PB-STEP-runner\t".equals(blocked) && f16.started.size() == 1, "a retry starts nothing while the sweep still finds a process: " + f16.started.size());
        File r17 = new File(tmp, "sweepkeep"), st17 = new File(r17, "stage");
        Fake f17 = new Fake(st17);
        Sweep w17 = new Sweep();
        w17.answers.add(false);
        PackSteps s17 = shaders(st17, "libs", f17);
        s17 = new PackSteps(s17.units, st17, "b", f17, PackJournalTest.STAMP, w17);
        check("PB-STEP-runner\t".equals(s17.run(s17.reconcile(journal(r17), new HashMap<>()), journal(r17), new HashMap<>(), "id", QUIET, () -> 0))
            && !new File(st17, ".units/shaders.keep").exists(), "a blocked run writes no keep file");
        File r18 = new File(tmp, "sweepend"), st18 = new File(r18, "stage");
        Fake f18 = new Fake(st18);
        Sweep w18 = new Sweep();
        w18.answers.add(true); w18.answers.add(false);
        script(f18, "rewrite!", "art!", "overlay!", "app!");
        PackSteps s18 = steps(st18, "b", f18, w18);
        check("PB-STEP-runner\t".equals(s18.run(s18.reconcile(journal(r18), new HashMap<>()), journal(r18), new HashMap<>(), "id", QUIET, () -> 0)),
            "every step done, but a process is still left after the runner ended: the run fails as the runner");


        File r19 = new File(tmp, "stop"), st19 = new File(r19, "stage");
        Fake f19 = new Fake(st19);
        f19.reached = new CountDownLatch(1); f19.proceed = new CountDownLatch(1);
        script(f19, "rewrite!", "wait", "art!");
        PackSteps s19 = steps(st19, "b", f19);
        List<PackSteps.Unit> p19 = s19.reconcile(journal(r19), new HashMap<>());
        String[] out19 = new String[1];
        Thread t19 = new Thread(() -> { try { out19[0] = s19.run(p19, journal(r19), new HashMap<>(), "id", QUIET, () -> 0); } catch (IOException e) { out19[0] = e.toString(); } });
        t19.start();
        check(f19.reached.await(10, TimeUnit.SECONDS), "the run reaches the wait");
        s19.stop();
        f19.proceed.countDown();
        t19.join(10_000);
        check(PackBuildRun.CANCEL.equals(out19[0]), "a stopped run ends like a cancel: " + out19[0]);
        s19.resume();
        script(f19, "art!", "overlay!", "app!");
        check(s19.run(s19.reconcile(journal(r19), new HashMap<>()), journal(r19), new HashMap<>(), "id", QUIET, () -> 0).isEmpty()
            && f19.started.get(1).equals(List.of("art", "overlay", "app")), "after resume() the rest runs: " + f19.started);


        File r20 = new File(tmp, "warm");
        Fake f20 = new Fake(new File(r20, "stage"));
        f20.reached = new CountDownLatch(1); f20.proceed = new CountDownLatch(1);
        script(f20, "rewrite!", "wait", "art!");
        java.util.concurrent.atomic.AtomicBoolean hot = new java.util.concurrent.atomic.AtomicBoolean();
        PackBuildRun.Conditions warm = new PackBuildRun.Conditions() {
            public long missing(List<PackJournal.Unit> u) { return 0; }
            public boolean cool(Runnable paused) { if (hot.get()) { paused.run(); hot.set(false); } return true; }
            public boolean hot() { return hot.get(); }
        };
        List<PackBuildState> seen20 = Collections.synchronizedList(new ArrayList<>());
        PackBuildRun run20 = new PackBuildRun(List.of(), steps(new File(r20, "stage"), "b", f20), journal(r20), "id", 0, new PackJournalTest.Writer(), warm, seen20::add, u -> {}, () -> 0);
        run20.thermalPollMs = 10;
        PackBuildState[] end20 = new PackBuildState[1];
        Thread t20 = new Thread(() -> { try { end20[0] = run20.run(); } catch (IOException e) { throw new IllegalStateException(e); } });
        t20.start();
        check(f20.reached.await(10, TimeUnit.SECONDS), "the warm run reaches its wait");
        script(f20, "art!", "overlay!", "app!");
        hot.set(true);
        for (int i = 0; i < 500 && !f20.ends.contains("cancel"); i++) Thread.sleep(10);
        f20.proceed.countDown();
        t20.join(10_000);
        boolean pausedShown = false;
        for (PackBuildState st : seen20) pausedShown |= st.phase == PackBuildState.Phase.PAUSED;
        check(end20[0] != null && end20[0].phase == PackBuildState.Phase.DONE && pausedShown, "too warm: paused, then finished: " + (end20[0] == null ? null : end20[0].format()));
        check(f20.started.size() == 2 && f20.started.get(1).equals(List.of("art", "overlay", "app")), "the stopped steps repeat after the pause: " + f20.started);



        File r21 = new File(tmp, "sweepretry");
        Fake f21 = new Fake(new File(r21, "stage"));
        Sweep w21 = new Sweep();
        w21.answers.add(true); w21.answers.add(false);
        script(f21, "rewrite!", "art!", "overlay!", "app!");
        PackSteps s21 = steps(new File(r21, "stage"), "b", f21, w21);
        PackBuildState first = new PackBuildRun(List.of(), s21, journal(r21), "id", 0, new PackJournalTest.Writer(), PackJournalTest.FINE, st -> {}, u -> {}, () -> 0).run();
        check(first.phase == PackBuildState.Phase.ERROR && "PB-STEP-runner".equals(first.error), "a process left after the last step: " + first.format());
        w21.answers.add(false);
        PackBuildState still = new PackBuildRun(List.of(), s21, journal(r21), "id", first.percent, new PackJournalTest.Writer(), PackJournalTest.FINE, st -> {}, u -> {}, () -> 0).run();
        check(still.phase == PackBuildState.Phase.ERROR && "PB-STEP-runner".equals(still.error) && f21.started.size() == 1,
            "the retry sweeps again and fails while the process is still there: " + still.format() + " " + w21.calls);
        PackBuildState end21 = new PackBuildRun(List.of(), s21, journal(r21), "id", still.percent, new PackJournalTest.Writer(), PackJournalTest.FINE, st -> {}, u -> {}, () -> 0).run();
        check(end21.phase == PackBuildState.Phase.DONE && f21.started.size() == 1 && w21.calls.equals(List.of("before the run", "after the run", "before the end", "before the end")),
            "then it finishes without starting a runner: " + end21.format() + " " + w21.calls);



        List<String> every = PackSteps.all();
        File r22 = new File(tmp, "fingerprints"), st22 = new File(r22, "stage");
        Fake f22 = new Fake(st22);
        String base0 = "runtime=r1\ntools=t1\nhelper=h1\ninputs=i1\ntarget=android";
        Map<String, String> deps0 = Map.of("shaders", "xlate=x1\nworkers=w1\nqueries=q1", "pictures", "monpic=m1");
        script(f22, "rewrite!", "art!", "overlay!", "app!", "pictures!", "shaders!");
        PackSteps s22 = new PackSteps(PackSteps.units(every, EXPECT, deps0), st22, base0, f22, PackJournalTest.STAMP, SWEPT);
        check(s22.run(s22.reconcile(journal(r22), new HashMap<>()), journal(r22), new HashMap<>(), "id", QUIET, () -> 0).isEmpty(), "the fingerprint fixture builds");
        String[][] categories = {
            {"runtime", base0.replace("r1", "r2"), null, "rewrite art overlay app pictures shaders"},
            {"tools", base0.replace("t1", "t2"), null, "rewrite art overlay app pictures shaders"},
            {"helper", base0.replace("h1", "h2"), null, "rewrite art overlay app pictures shaders"},
            {"inputs", base0.replace("i1", "i2"), null, "rewrite art overlay app pictures shaders"},
            {"picture library", base0, "pictures=monpic=m2", "pictures"},
            {"translator", base0, "shaders=xlate=x2\nworkers=w1\nqueries=q1", "shaders"},
            {"worker library", base0, "shaders=xlate=x1\nworkers=w2\nqueries=q1", "shaders"},
            {"query kernels", base0, "shaders=xlate=x1\nworkers=w1\nqueries=q2", "shaders"},
        };
        for (String[] c : categories) {
            Map<String, String> deps = new HashMap<>(deps0);
            if (c[2] != null) deps.put(c[2].substring(0, c[2].indexOf('=')), c[2].substring(c[2].indexOf('=') + 1));
            PackSteps sc = new PackSteps(PackSteps.units(every, EXPECT, deps), st22, c[1], f22, PackJournalTest.STAMP, SWEPT);
            String stale = String.join(" ", ids(sc.reconcile(journal(r22), new HashMap<>())));
            check(stale.equals(c[3]), "a changed " + c[0] + " makes exactly these steps stale: " + stale);
        }
        check(new PackSteps(PackSteps.units(every, EXPECT, deps0), st22, base0, f22, PackJournalTest.STAMP, SWEPT).reconcile(journal(r22), new HashMap<>()).isEmpty(),
            "nothing changed: nothing stale");


        File root = new File(System.getProperty("dm.root", "."));
        String page = new String(Files.readAllBytes(new File(root, "dm/android/app/src/main/java/com/xiaoboluo/d2r/PackBuildPage.java").toPath()), StandardCharsets.UTF_8);
        String table = new String(Files.readAllBytes(new File(root, "dm/i18n/ui_text.tsv").toPath()), StandardCharsets.UTF_8);
        List<String> idList = new ArrayList<>();
        for (String step : every) idList.add("PB-STEP-" + step);
        idList.addAll(List.of("PB-STEP-runner", "PB-STEP-runtime", "PB-STEP-install", "PB-STEP-check", "PB-SPACE", "PB-IO", "PB-SOURCE", "PB-COMPACT", "PB-INPLACE"));
        for (String id : idList) {
            java.util.regex.Matcher m = java.util.regex.Pattern.compile("case \"" + id + "\" -> [\\s\\S]{0,300}?AppText\\.get\\(c, R\\.string\\.(\\w+)").matcher(page);
            boolean found = m.find();
            check(found && table.contains("\n" + m.group(1) + "\t"), id + " has its own sentence in the string table");
        }


        File r23 = new File(tmp, "lowspace");
        Fake f23 = new Fake(new File(r23, "stage"));
        script(f23, "start\trewrite\t1", "error\trewrite\tPB-STEP-rewrite\twrite failed");
        PackBuildRun.Conditions full = new PackBuildRun.Conditions() {
            public long missing(List<PackJournal.Unit> u) { return 0; }
            public boolean cool(Runnable paused) { return true; }
            public long free() { return 50L << 20; }
        };
        PackBuildState low = new PackBuildRun(List.of(), steps(new File(r23, "stage"), "b", f23), journal(r23), "id", 0, new PackJournalTest.Writer(), full, st -> {}, u -> {}, () -> 0).run();
        check(low.phase == PackBuildState.Phase.ERROR && "PB-SPACE".equals(low.error) && Long.parseLong(low.detail) == PackSpace.STEPS - (50L << 20), "a step failing with 50 MB free: " + low.format());



        long rt = PackSpace.RUNTIME, wk = 40L << 20;
        check(PackSpace.steps(0, 0, 0) == PackSpace.STEPS && PackSpace.steps(rt, wk, 0) == PackSpace.STEPS - rt - wk
            && PackSpace.steps(2 * rt, 2 * PackSpace.WORK, 2 * PackSpace.STAGE) == 0, "what the steps still need");
        long need = 3_000_000_000L, free0 = need + PackSpace.STEPS;
        PackSpace.Check onPage = PackSpace.check(List.of(List.of(new PackSpace.Write(1, need, 0)), List.of(new PackSpace.Write(1, PackSpace.steps(0, 0, 0), 0))), Map.of(1L, free0));
        PackSpace.Check builder = PackSpace.check(List.of(List.of(new PackSpace.Write(1, need, 0)), List.of(new PackSpace.Write(1, PackSpace.steps(rt, wk, 0), 0))),
            Map.of(1L, free0 - rt - wk));
        check(onPage.fits() && builder.fits(), "the check page's fit holds once the runtime is unpacked: " + onPage.missing + " / " + builder.missing);



        File work = new File(tmp, "workcredit/work");
        File oldCache = new File(work, PackSpace.REWRITE_CACHES + "/3.3.94012-old-android"), cur = new File(work, PackSpace.REWRITE_CACHES + "/3.3.94012-cur-android");
        oldCache.mkdirs(); cur.mkdirs(); new File(work, "tree").mkdirs();
        Files.write(new File(oldCache, "main.patched.bin").toPath(), new byte[200_000]);
        Files.write(new File(cur, "main.patched.bin").toPath(), new byte[30_000]);
        Files.write(new File(work, "tree/tool.py").toPath(), new byte[5_000]);
        check(PackSpace.workCredit(work, "3.3.94012-cur") == 35_000, "only the current product's cache and the rest of the work folder count: "
            + PackSpace.workCredit(work, "3.3.94012-cur"));
        check(PackSpace.steps(0, PackSpace.workCredit(work, "3.3.94012-cur"), 0) == PackSpace.STEPS - 35_000, "the old cache lowers no need");
        PackSpace.pruneWork(work, "3.3.94012-cur");
        check(!oldCache.exists() && new File(cur, "main.patched.bin").isFile() && new File(work, "tree/tool.py").isFile(),
            "the start of a build removes the old product's cache and keeps the current one");
        check(PackSpace.workCredit(work, "3.3.94012-cur") == 35_000, "and the figure stays the same afterwards");


        File lw = new File(tmp, "linked/work"), outside = new File(tmp, "linked/outside");
        new File(lw, "build/cache/dm").mkdirs();
        new File(outside, "3.3.94012-old-android").mkdirs();
        Files.write(new File(outside, "3.3.94012-old-android/keep.bin").toPath(), new byte[1000]);
        Files.createSymbolicLink(new File(lw, PackSpace.REWRITE_CACHES).toPath(), outside.toPath());
        check(PackSpace.otherCaches(lw, "3.3.94012-cur").isEmpty() && PackSpace.reclaimable(lw, "3.3.94012-cur") == 0, "a linked out folder offers no caches");
        PackSpace.pruneWork(lw, "3.3.94012-cur");
        check(new File(outside, "3.3.94012-old-android/keep.bin").isFile(), "the prune does not delete through the link");


        File rw = new File(tmp, "reclaim/work"), oldBig = new File(rw, PackSpace.REWRITE_CACHES + "/3.3.94012-old-android");
        oldBig.mkdirs();
        try (java.io.RandomAccessFile f = new java.io.RandomAccessFile(new File(oldBig, "main.patched.bin"), "rw")) { f.setLength(200L << 20); }
        long reclaim = PackSpace.reclaimable(rw, "3.3.94012-cur");
        List<List<PackSpace.Write>> units100 = List.of(List.of(new PackSpace.Write(7, 1000L << 20, 0)));
        Map<Long, Long> free900 = Map.of(7L, 900L << 20);
        check(reclaim == 200L << 20 && !PackSpace.check(units100, free900).fits() && PackSpace.check(units100, free900, 7, reclaim).fits(),
            "100 MiB missing, 200 MiB of an old cache to free: Start is allowed (reclaim " + reclaim + ")");
        check(PackSpace.check(units100, free900, 7, 0).missing == PackSpace.check(units100, free900).missing, "nothing to free: the same check");

        System.out.println("PackStepsTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }

    interface Body { void run() throws Exception; }
    static boolean refused(Body b) {
        try { b.run(); return false; } catch (IllegalArgumentException e) { return true; } catch (Exception e) { return false; }
    }
}
