package com.xiaoboluo.d2r;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;



public class PackJournalTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }


    static final PackJournal.Stamp STAMP = f -> Files.getLastModifiedTime(f.toPath()).to(java.util.concurrent.TimeUnit.NANOSECONDS);


    static final PackBuildRun.Conditions FINE = new PackBuildRun.Conditions() {
        public long missing(List<PackJournal.Unit> units) { return 0; }
        public boolean cool(Runnable paused) { return true; }
    };

    static final class Killed extends RuntimeException { private static final long serialVersionUID = 1; Killed(String at) { super(at); } }


    static final String[] FILES = {"exefs\texefs\tmain", "exefs\texefs\tsdk", "meta\tdata\t.build.config", "00-00000001.data\tdata\t00-00000001.data", "00-00000002.data\tdata\t00-00000002.data"};
    static byte[] content(String name) { return ("content of " + name + "\n").repeat(50).getBytes(StandardCharsets.UTF_8); }
    static String listing() {
        StringBuilder s = new StringBuilder();
        for (String f : FILES) { String name = f.split("\t")[2]; byte[] c = content(name); s.append(f).append('\t').append(c.length).append('\t').append(DriverLibrary.sha256Bytes(c)).append('\n'); }
        return s.toString();
    }



    static final class Writer implements PackBuildRun.Extractor {
        final Map<String, Integer> runs = new HashMap<>();
        String kill;
        public String extract(PackJournal.Unit u, PackReader.Sink sink) {
            runs.merge(u.id, 1, Integer::sum);
            long n = 0;
            for (PackJournal.Output o : u.outputs) {
                try {
                    File part = o.part();
                    part.getParentFile().mkdirs();
                    try (FileOutputStream out = new FileOutputStream(part)) { out.write(content(o.file.getName())); }
                    n += o.size;
                    sink.wrote(n, o.file.getName());
                    if (("part:" + o.file.getName()).equals(kill)) throw new Killed(kill);
                    if (!part.renameTo(o.file)) return "PB-IO\trename";
                    if (("renamed:" + o.file.getName()).equals(kill)) throw new Killed(kill);
                } catch (IOException e) { return "PB-IO\t" + o.file.getName(); }
            }
            if (("done:" + u.id).equals(kill)) throw new Killed(kill);
            return "";
        }
        public void cancel() {}
    }

    static PackBuildState run(File root, Writer w) throws IOException {
        List<PackJournal.Unit> units = PackBuildRun.units(listing(), new File(root, "exefs"), new File(root, "data"));
        PackJournal j = new PackJournal(new File(root, "journal.tsv"), dir -> {}, PackJournalTest.STAMP);
        return new PackBuildRun(units, j, "base+update", 0, w, FINE, s -> {}, unit -> {}, () -> 0).run();
    }

    static void clean(File f) { File[] l = f.listFiles(); if (l != null) for (File c : l) clean(c); f.delete(); }

    public static void main(String[] args) throws Exception {
        File tmp = Files.createTempDirectory("packjournal").toFile();
        List<String> points = new ArrayList<>();
        for (String f : FILES) { String name = f.split("\t")[2]; points.add("part:" + name); points.add("renamed:" + name); }
        for (String f : FILES) if (!points.contains("done:" + f.split("\t")[0])) points.add("done:" + f.split("\t")[0]);
        for (String point : points) {
            File root = new File(tmp, "k" + points.indexOf(point));
            Writer w = new Writer();
            w.kill = point;
            boolean died = false;
            try { run(root, w); } catch (Killed k) { died = true; }
            check(died, "the writer died at " + point);
            w.kill = null;
            PackBuildState end = run(root, w);
            check(end.phase == PackBuildState.Phase.DONE && end.percent == 100, "the next start finishes after a kill at " + point + ": " + end.format().replace("\n", " "));
            int repeated = 0, adopted = 0;
            for (String unit : new String[]{"exefs", "meta", "00-00000001.data", "00-00000002.data"}) {
                int n = w.runs.getOrDefault(unit, 0);
                check(n >= 0 && n <= 2, unit + " ran at most twice after a kill at " + point);
                if (n == 2) repeated++;
                if (point.startsWith("done:" + unit) && n == 1) adopted++;
            }
            check(repeated <= 1, "at most one unit repeats after a kill at " + point + " (repeated " + repeated + ")");
            if (point.startsWith("done:")) check(adopted == 1 && repeated == 0, "outputs renamed but not journaled are adopted by their digests, not redone (" + point + ")");
            for (String f : FILES) {
                String[] p = f.split("\t");
                File out = new File(root, (p[1].equals("exefs") ? "exefs/" : "data/") + p[2]);
                check(out.isFile() && java.util.Arrays.equals(Files.readAllBytes(out.toPath()), content(p[2])), "output " + p[2] + " complete after " + point);
                check(!new File(out.getPath() + ".part").exists(), "no .part left of " + p[2] + " after " + point);
            }
            Writer again = new Writer();
            run(root, again);
            check(again.runs.isEmpty(), "a third start after " + point + " has nothing left to do");
        }


        File order = new File(tmp, "order");
        Writer stop = new Writer();
        stop.kill = "done:exefs";
        try { run(order, stop); } catch (Killed k) {                           }
        File journalFile = new File(order, "journal.tsv");
        List<String> seen = new ArrayList<>();
        List<PackJournal.Unit> all = PackBuildRun.units(listing(), new File(order, "exefs"), new File(order, "data"));
        new PackJournal(journalFile, dir -> {
            String text = journalFile.exists() ? new String(Files.readAllBytes(journalFile.toPath()), StandardCharsets.UTF_8) : "";
            seen.add(dir.getName() + (text.contains("exefs\t") ? " after" : " before"));
        }, STAMP).reconcile(all, "base+update", name -> {});
        check(seen.contains("exefs before") && !seen.contains("exefs after"), "the adopted unit's folder is synced before its line is appended: " + seen);
        check(new PackJournal(journalFile, dir -> {}, PackJournalTest.STAMP).load().containsKey("exefs"), "and then the unit is adopted");


        File root = new File(tmp, "torn");
        Writer w = new Writer();
        run(root, w);
        File journal = new File(root, "journal.tsv");
        Map<String, PackJournal.Record> before = new PackJournal(journal, dir -> {}, PackJournalTest.STAMP).load();
        check(before.size() == 4, "four records after a full run: " + before.size());
        try (FileOutputStream o = new FileOutputStream(journal, true)) { o.write("meta\tbase+upd".getBytes(StandardCharsets.UTF_8)); }
        check(new PackJournal(journal, dir -> {}, PackJournalTest.STAMP).load().size() == 4, "a torn tail is not a record");
        new PackJournal(journal, dir -> {}, PackJournalTest.STAMP).append(new PackJournal.Record("x", "i", "f", "d", "1"));
        String text = new String(Files.readAllBytes(journal.toPath()), StandardCharsets.UTF_8);
        check(!text.contains("base+upd\tx") && !text.contains("base+updx") && text.endsWith("x\ti\tf\td\t1\n"), "the next append cuts the torn tail off first");


        new File(root, "data/00-00000002.data").delete();
        Writer redo = new Writer();
        run(root, redo);
        check(redo.runs.size() == 1 && redo.runs.getOrDefault("00-00000002.data", 0) == 1, "a missing output is written again, nothing else: " + redo.runs);


        File other = new File(tmp, "other");
        Writer partial = new Writer();
        partial.kill = "done:meta";
        try { run(other, partial); } catch (Killed k) {                           }
        Files.write(new File(other, "data/.build.config").toPath(), new byte[content(".build.config").length]);
        Writer after = new Writer();
        run(other, after);
        check(after.runs.getOrDefault("meta", 0) == 1, "an unjournaled output with another digest is redone");


        File fp = new File(tmp, "fp");
        run(fp, new Writer());
        List<PackJournal.Unit> units = PackBuildRun.units(listing(), new File(fp, "exefs"), new File(fp, "data"));
        List<PackJournal.Unit> changed = new ArrayList<>();
        for (PackJournal.Unit u : units) changed.add(u.id.equals("meta") ? new PackJournal.Unit(u.id, "0".repeat(64), u.outputs) : u);
        List<PackJournal.Unit> pending = new PackJournal(new File(fp, "journal.tsv"), dir -> {}, PackJournalTest.STAMP).reconcile(changed, "base+update", name -> {});
        check(pending.size() == 1 && pending.get(0).id.equals("meta"), "a stale fingerprint makes exactly that unit pending");



        File same = new File(tmp, "same");
        run(same, new Writer());
        File arch = new File(same, "data/00-00000001.data");
        byte[] flipped = Files.readAllBytes(arch.toPath());
        flipped[0] ^= 1;
        long stamp = arch.lastModified();
        Files.write(arch.toPath(), flipped);
        check(arch.setLastModified(stamp + 2000), "the damaged output gets another modification time");
        Writer fix = new Writer();
        run(same, fix);
        check(fix.runs.equals(Map.of("00-00000001.data", 1)), "a same-size change of a recorded output is caught by its digest, only that unit is redone: " + fix.runs);
        check(java.util.Arrays.equals(Files.readAllBytes(arch.toPath()), content("00-00000001.data")), "and the output is whole again");


        byte[] again = Files.readAllBytes(arch.toPath());
        again[1] ^= 1;
        java.nio.file.attribute.FileTime written = Files.getLastModifiedTime(arch.toPath());
        Files.write(arch.toPath(), again);
        Files.setLastModifiedTime(arch.toPath(), java.nio.file.attribute.FileTime.from(written.to(java.util.concurrent.TimeUnit.NANOSECONDS) + 1000, java.util.concurrent.TimeUnit.NANOSECONDS));
        Writer quick = new Writer();
        run(same, quick);
        check(quick.runs.equals(Map.of("00-00000001.data", 1)), "a change within the same second is caught: " + quick.runs);


        File sameJournal = new File(same, "journal.tsv");
        check(arch.setLastModified(arch.lastModified() + 5000), "the output is touched");
        List<String> hashed = new ArrayList<>();
        List<PackJournal.Unit> sameUnits = PackBuildRun.units(listing(), new File(same, "exefs"), new File(same, "data"));
        List<PackJournal.Unit> sp = new PackJournal(sameJournal, dir -> {}, PackJournalTest.STAMP).reconcile(sameUnits, "base+update", hashed::add);
        check(sp.isEmpty() && hashed.equals(List.of("00-00000001.data")), "a touched output with its digest is hashed and kept: " + hashed);
        hashed.clear();
        sp = new PackJournal(sameJournal, dir -> {}, PackJournalTest.STAMP).reconcile(sameUnits, "base+update", hashed::add);
        check(sp.isEmpty() && hashed.isEmpty(), "and the next start trusts its new stamp: " + hashed);


        File legacy = new File(tmp, "legacy");
        run(legacy, new Writer());
        File legacyJournal = new File(legacy, "journal.tsv");
        StringBuilder old = new StringBuilder();
        for (String l : new String(Files.readAllBytes(legacyJournal.toPath()), StandardCharsets.UTF_8).split("\n")) old.append(l, 0, l.lastIndexOf('\t')).append('\n');
        Files.write(legacyJournal.toPath(), old.toString().getBytes(StandardCharsets.UTF_8));
        check(new PackJournal(legacyJournal, dir -> {}, PackJournalTest.STAMP).load().values().stream().allMatch(r -> r.stamps == null), "lines without stamps load");
        hashed.clear();
        List<PackJournal.Unit> legacyUnits = PackBuildRun.units(listing(), new File(legacy, "exefs"), new File(legacy, "data"));
        sp = new PackJournal(legacyJournal, dir -> {}, PackJournalTest.STAMP).reconcile(legacyUnits, "base+update", hashed::add);
        check(sp.isEmpty() && hashed.size() == FILES.length, "every output of a line without stamps is hashed once: " + hashed);
        check(new PackJournal(legacyJournal, dir -> {}, PackJournalTest.STAMP).load().values().stream().allMatch(r -> r.stamps != null), "and recorded again with stamps");
        for (String bad : new String[]{"u\ti\tf\td\t12,x\n", "u\ti\tf\td\t\n", "u\ti\tf\td\t1\textra\n"}) {
            File j = new File(tmp, "bad.tsv");
            Files.write(j.toPath(), bad.getBytes(StandardCharsets.UTF_8));
            boolean refused = false;
            try { new PackJournal(j, dir -> {}, PackJournalTest.STAMP).load(); } catch (IOException | IllegalArgumentException e) { refused = true; }
            check(refused, "a malformed journal line is refused: " + bad.replace("\t", " ").trim());
        }

        clean(tmp);
        System.out.println("PackJournalTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
