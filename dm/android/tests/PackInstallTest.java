package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.LinkOption;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;








public class PackInstallTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    static final class Kill extends Error {
        private static final long serialVersionUID = 1;
        Kill() { super("killed"); }
    }

    static final PackInstall.Fs FS = new PackInstall.Fs() {
        @Override public long number(File f) throws IOException { return (Long) Files.getAttribute(f.toPath(), "unix:ino", LinkOption.NOFOLLOW_LINKS); }
        @Override public void sync(File dir) {}
    };


    static final class Killer implements PackInstall.Hold {
        final int at;
        int seen;
        Killer(int at) { this.at = at; }
        @Override public void at(String point) { if (++seen == at) throw new Kill(); }
    }

    enum Old { ABSENT, NONE, PC_RECORD, PC_BARE, PHONE }
    enum New { PHONE, PC_NO_VARIANTS }

    static void write(File f, String text) throws IOException {
        f.getParentFile().mkdirs();
        Files.write(f.toPath(), text.getBytes(StandardCharsets.UTF_8));
    }

    static String read(File f) throws IOException { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8); }


    static void pack(File dir, String tag, boolean variants, String source) throws IOException {
        write(new File(dir, "out/main.patched.bin"), tag + " main");
        write(new File(dir, "out/sdk.patched.bin"), tag + " sdk");
        write(new File(dir, "shaders/a.xgs"), tag + " a");
        write(new File(dir, "overlay/x.json"), tag + " x");
        if (variants) write(new File(dir, "overlay_variants/v1/x.json"), tag + " v1");
        write(new File(dir, "appart/font.otf"), tag + " font");
        write(new File(dir, "appart/mon/a.webp"), tag + " pic");
        write(new File(dir, "derived.tsv"), tag + " derived");
        if (source != null) write(new File(dir, PackSource.FILE), source);
        write(new File(dir, "pack_info.txt"), "PRODUCT=" + tag + "\n");
    }

    static Map<String, String> snapshot(File dir) throws IOException {
        Map<String, String> out = new TreeMap<>();
        walk(dir, "", out);
        return out;
    }
    static void walk(File f, String rel, Map<String, String> out) throws IOException {
        if (f.isFile()) { out.put(rel, read(f)); return; }
        File[] kids = f.listFiles();
        if (kids != null) for (File k : kids) walk(k, rel.isEmpty() ? k.getName() : rel + "/" + k.getName(), out);
    }

    static final class Setup {
        final File files, dm, gen, manifest;
        final Map<String, String> before, after;
        Setup(File files, Old old, New now) throws IOException {
            this.files = files;
            dm = new File(files, "dm");
            if (old != Old.ABSENT) {
                write(new File(dm, "env"), "DM_QA_STATE=1\n");
                write(new File(dm, "settings/x.txt"), "keep");
            }
            write(new File(files, "save/hero.d2s"), "hero");
            if (old != Old.NONE && old != Old.ABSENT) pack(dm, "old", old != Old.PC_BARE,
                old == Old.PC_RECORD ? "source\tpc\nmarker\t" + "a".repeat(64) + "\n" : old == Old.PHONE ? "source\tphone\nmarker\t-\n" : null);
            before = snapshot(files);
            boolean phone = now == New.PHONE;
            gen = phone || old == Old.ABSENT ? new File(files, "dm-build/gen-1") : new File(files, "dm/" + PackGeneration.IMPORT);
            String source = phone ? "source\tphone\nmarker\t-\n" : "source\tpc\nmarker\t" + "b".repeat(64) + "\n";
            pack(gen, "new", phone, source);
            PackInstall.Manifest m = new PackInstall.Manifest();
            Map<String, String> g = snapshot(gen);
            for (Map.Entry<String, String> e : g.entrySet()) m.add(e.getKey(), e.getValue().getBytes(StandardCharsets.UTF_8).length,
                DriverLibrary.sha256Bytes(e.getValue().getBytes(StandardCharsets.UTF_8)), phone ? "s" : "-");
            manifest = new File(files, phone || old == Old.ABSENT ? "dm-build/gen-1.tsv" : "dm-build/gen-import.tsv");
            write(manifest, m.format());
            Map<String, String> a = new TreeMap<>();
            for (Map.Entry<String, String> e : before.entrySet()) if (!packPath(e.getKey())) a.put(e.getKey(), e.getValue());
            for (Map.Entry<String, String> e : g.entrySet()) a.put("dm/" + e.getKey(), e.getValue());
            after = a;
        }
        Map<String, String> beforeWithGen() throws IOException { return before; }
    }

    static boolean packPath(String rel) {
        for (String item : PackInstall.ITEMS) if (rel.equals("dm/" + item) || rel.startsWith("dm/" + item + "/")) return true;
        return false;
    }


    static Map<String, String> state(File files) throws IOException {
        Map<String, String> s = snapshot(files);
        s.keySet().removeIf(k -> k.startsWith("dm-build/") || k.startsWith("dm/" + PackGeneration.IMPORT + "/"));
        return s;
    }

    static boolean clean(File files) {
        if (new File(files, PackInstall.PREV).exists() || new File(files, "dm-build/" + PackInstall.UNDO).exists() || new File(files, "dm-build/" + PackInstall.RECORD).exists()) return false;
        if (new File(files, "dm/" + PackGeneration.IMPORT).exists()) return false;
        String[] names = new File(files, "dm-build").list();
        if (names != null) for (String n : names) if (n.startsWith(PackGeneration.GEN)) return false;
        return true;
    }

    static PackInstall.Checks checker(boolean fail) { return (dm, m) -> fail ? "check: made to fail" : null; }

    static PackInstall in(File files, boolean fail, PackInstall.Hold hold) { return new PackInstall(files, FS, checker(fail), hold); }


    static String session(File files, String kind) throws IOException {
        String st = "phase\terror\npercent\t12\nerror\tPB-KEPT\ndetail\t\n";
        write(new File(files, "dm-build/" + PackSession.STATE), st);
        if (kind.equals("kept")) write(new File(files, "dm-build/" + PackSession.KEPT), "1\n");
        else write(new File(files, "dm-build/" + PackSession.SOURCES), "base\tcontent://x/b\tb\nupdate\tcontent://x/u\tu\nkeys\tcontent://x/k\n");
        return st;
    }

    static PackInstall withSession(File files, boolean fail, PackInstall.Hold hold) {
        PackSession s = new PackSession(new File(files, "dm-build"), d -> {}, u -> {});
        return new PackInstall(files, FS, checker(fail), hold, s::confirmed);
    }

    static int counter = 0;
    static File fresh(File tmp) { return new File(tmp, "c" + (counter++)); }

    public static void main(String[] args) throws Exception {

        File shm = new File("/dev/shm");
        File tmp = (shm.isDirectory() && shm.canWrite() ? Files.createTempDirectory(shm.toPath(), "packinstall") : Files.createTempDirectory("packinstall")).toFile();


        for (Old old : Old.values()) for (New now : New.values()) for (boolean fail : new boolean[]{false, true}) {
            File files = fresh(tmp);
            Setup s = new Setup(files, old, now);
            PackInstall.Result r = in(files, fail, p -> {}).commit(s.gen, s.manifest, (d, t) -> {});
            check(r.outcome == (fail ? PackInstall.Outcome.ROLLEDBACK : PackInstall.Outcome.CONFIRMED), old + " -> " + now + " fail " + fail + ": " + r);
            check(state(files).equals(fail ? s.before : s.after), old + " -> " + now + " fail " + fail + ": the pack is the " + (fail ? "old" : "new") + " one");
            check(clean(files), old + " -> " + now + ": nothing left over");
            check(fail || new File(files, "dm-build/" + PackInstall.INSTALLED).isFile(), "the generation manifest stays as installed.tsv");
        }


        File fa = fresh(tmp);
        Setup sa = new Setup(fa, Old.ABSENT, New.PHONE);
        check(!new File(fa, "dm").exists(), "the fresh setup has no files/dm");
        PackInstall.Result ra = in(fa, false, p -> {}).commit(sa.gen, sa.manifest, (d, t) -> {});
        check(ra.outcome == PackInstall.Outcome.CONFIRMED && state(fa).equals(sa.after) && clean(fa), "a phone build installs on a fresh app: " + ra);


        int runs = 0;
        for (Old old : Old.values()) for (New now : New.values()) for (boolean fail : new boolean[]{false, true}) {
            File probe = fresh(tmp);
            Setup ps = new Setup(probe, old, now);
            Killer count = new Killer(0);
            in(probe, fail, count).commit(ps.gen, ps.manifest, (d, t) -> {});
            for (int k = 1; k <= count.seen; k++) {

                File f0 = fresh(tmp);
                Setup s0 = new Setup(f0, old, now);
                try { in(f0, fail, new Killer(k)).commit(s0.gen, s0.manifest, (d, t) -> {}); check(false, "kill " + k + " did not happen"); } catch (Kill expected) {              }
                Killer rc = new Killer(0);
                in(f0, fail, rc).recover(false);
                verify(f0, s0, fail, old + " -> " + now + " fail " + fail + " kill " + k);
                for (int j = 1; j <= rc.seen; j++) {
                    File f = fresh(tmp);
                    Setup s = new Setup(f, old, now);
                    try { in(f, fail, new Killer(k)).commit(s.gen, s.manifest, (d, t) -> {}); } catch (Kill expected) {              }
                    try { in(f, fail, new Killer(j)).recover(false); } catch (Kill expected) {                    }
                    PackInstall.Result r = in(f, fail, p -> {}).recover(false);
                    check(r.outcome != PackInstall.Outcome.STOPPED, "recovery after kills " + k + "/" + j + " stopped: " + r);
                    verify(f, s, fail, old + " -> " + now + " fail " + fail + " kills " + k + "/" + j);
                    runs++;
                    PackInstall.deleteTree(f);
                }
                PackInstall.deleteTree(f0);
            }
            PackInstall.deleteTree(probe);
        }
        check(runs > 500, "the matrix ran " + runs + " double faults");




        int imports = 0;
        for (String open : new String[]{"kept", "sources"}) for (boolean fail : new boolean[]{false, true}) {
            File probe = fresh(tmp);
            Setup ps = new Setup(probe, Old.PHONE, New.PC_NO_VARIANTS);
            Killer count = new Killer(0);
            session(probe, open);
            withSession(probe, fail, count).commit(ps.gen, ps.manifest, (d, t) -> {});
            for (int k = 1; k <= count.seen; k++) for (int j = 0; j <= 3; j++) {
                File f = fresh(tmp);
                Setup s = new Setup(f, Old.PHONE, New.PC_NO_VARIANTS);
                String stateBefore = session(f, open);
                try { withSession(f, fail, new Killer(k)).commit(s.gen, s.manifest, (d, t) -> {}); } catch (Kill expected) {              }
                if (j > 0) try { withSession(f, fail, new Killer(j)).recover(false); } catch (Kill expected) {                    }
                withSession(f, fail, p -> {}).recover(false);
                PackSession next = new PackSession(new File(f, "dm-build"), d -> {}, u -> {});
                next.finish();
                boolean isNew = state(f).equals(s.after);
                String what = "import with a " + open + " session, fail " + fail + ", kills " + k + "/" + j;
                verify(f, s, fail, what);
                check(isNew ? !next.active() : next.active() && (open.equals("kept") ? next.kept() : !next.kept()), what + ": the session " + (isNew ? "ended" : "stays"));
                if (!isNew) check(read(new File(f, "dm-build/" + PackSession.STATE)).equals(stateBefore), what + ": its state as it was");
                imports++;
                PackInstall.deleteTree(f);
            }
            PackInstall.deleteTree(probe);
        }
        check(imports > 50, "the import matrix ran " + imports + " cases");

        File pb = fresh(tmp);
        Setup pbs = new Setup(pb, Old.PC_RECORD, New.PHONE);
        session(pb, "sources");
        check(withSession(pb, false, p -> {}).commit(pbs.gen, pbs.manifest, (d, t) -> {}).outcome == PackInstall.Outcome.CONFIRMED
            && new PackSession(new File(pb, "dm-build"), d -> {}, u -> {}).active() && !new File(pb, "dm-build/" + PackSession.ENDING).exists(), "a phone build keeps its session");


        File ff = fresh(tmp);
        Setup fs = new Setup(ff, Old.PC_RECORD, New.PHONE);
        try { in(ff, false, new Killer(3)).commit(fs.gen, fs.manifest, (d, t) -> {}); } catch (Kill expected) {                       }
        File out = new File(ff, "dm/out"), foreign = new File(ff, "foreign");
        write(new File(foreign, "main.patched.bin"), "foreign");
        if (out.exists()) PackInstall.deleteTree(out);
        check(foreign.renameTo(out), "a foreign out in place");
        Map<String, String> was = snapshot(ff);
        PackInstall.Result stop = in(ff, false, p -> {}).recover(false);
        check(stop.outcome == PackInstall.Outcome.STOPPED && snapshot(ff).equals(was), "a foreign item stops the recovery and nothing moves: " + stop);
        check(new File(ff, "dm-build/" + PackInstall.RECORD).isFile(), "the record stays for the next start");


        File fl = fresh(tmp);
        Setup ls = new Setup(fl, Old.PHONE, New.PHONE);
        try { in(fl, false, new Killer(4)).commit(ls.gen, ls.manifest, (d, t) -> {}); } catch (Kill expected) {              }
        Map<String, String> held = snapshot(fl);
        check(in(fl, false, p -> {}).recover(true).outcome == PackInstall.Outcome.REFUSED && snapshot(fl).equals(held), "the recovery refuses under a loaded library");
        in(fl, false, p -> {}).recover(false);
        verify(fl, ls, false, "and finishes once the library is not loaded");


        File fo = fresh(tmp);
        new Setup(fo, Old.PHONE, New.PHONE);
        write(new File(fo, "dm-prev/out/x"), "x");
        write(new File(fo, "dm-build/undo/out/x"), "x");
        check(in(fo, false, p -> {}).recover(false).outcome == PackInstall.Outcome.NOTHING && clean(fo), "leftovers of a finished install are removed");

        generation(tmp);

        PackInstall.deleteTree(tmp);
        System.out.println("PackInstallTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }

    static void verify(File files, Setup s, boolean fail, String what) throws IOException {
        Map<String, String> now = state(files);
        boolean isOld = now.equals(s.before), isNew = now.equals(s.after);
        check(fail ? isOld : isOld || isNew, what + ": the pack is " + (isOld ? "old" : isNew ? "new" : "MIXED " + now.keySet()));
        check(clean(files), what + ": nothing left over");
    }


    static void generation(File tmp) throws Exception {
        File build = new File(tmp, "gen/dm-build"), stage = new File(build, "stage");
        Map<String, String> outputs = new java.util.LinkedHashMap<>();
        outputs.put("rewrite", "out/main.patched.bin");
        outputs.put("art", "gen/dm_vault_tab.h derived.tsv");
        outputs.put("overlay", "overlay/x.json overlay_variants/v/x.json");
        outputs.put("app", "appart/font.otf");
        outputs.put("pictures", "monpics/mon.json monpics/mon/a.webp monpics/mon/s/a.webp");
        outputs.put("shaders", "shaders/k.xgs");
        List<PackGeneration.Step> steps = new ArrayList<>();
        Map<String, Long> times = new HashMap<>();
        for (Map.Entry<String, String> e : outputs.entrySet()) {
            StringBuilder m = new StringBuilder();
            List<String> paths = new ArrayList<>();
            StringBuilder stamps = new StringBuilder();
            String[] ps = e.getValue().split(" ");
            java.util.Arrays.sort(ps);
            for (String p : ps) {
                File f = new File(stage, p);
                write(f, "made " + p);
                m.append(p).append('\t').append(f.length()).append('\t').append(DriverLibrary.sha256(f)).append('\n');
                paths.add(p);
                stamps.append(stamps.length() == 0 ? "" : ",").append(f.lastModified());
            }
            write(new File(stage, ".units/" + e.getKey() + ".tsv"), m.toString());
            steps.add(new PackGeneration.Step(e.getKey(), "fp-" + e.getKey(), DriverLibrary.sha256Bytes(m.toString().getBytes(StandardCharsets.UTF_8)),
                stamps.toString(), PackGeneration.pathDigest(paths)));
        }
        PackJournal.Stamp mtime = File::lastModified;
        PackGeneration.Made g = PackGeneration.fromStage(build, stage, steps, "PRODUCT=p\n", "source\tphone\nmarker\t-\n", d -> {}, mtime, (d, t) -> {});
        check(g.folder.getName().equals("gen-1") && new File(g.folder, "appart/mon/s/a.webp").isFile() && new File(g.folder, "appart/mon.json").isFile()
            && new File(g.folder, "appart/font.otf").isFile(), "the pictures go into appart/");
        check(!new File(g.folder, "gen").exists() && new File(stage, "gen/dm_vault_tab.h").isFile(), "the art step's intermediates stay in the stage");
        PackInstall.Manifest m = PackInstall.Manifest.parse(read(g.manifest));
        check(m.files.keySet().equals(new java.util.TreeSet<>(List.of("out/main.patched.bin", "derived.tsv", "overlay/x.json", "overlay_variants/v/x.json",
            "appart/font.otf", "appart/mon.json", "appart/mon/a.webp", "appart/mon/s/a.webp", "shaders/k.xgs", "pack_info.txt", PackSource.FILE))),
            "the manifest lists the pack paths: " + m.files.keySet());
        check(m.files.get("appart/mon/a.webp")[2].equals("pictures") && m.steps.get("shaders")[0].equals("fp-shaders"), "each file names its step; the steps keep their fingerprints");
        check(PackGeneration.next(build) == 2, "the next generation number");

        File b2 = new File(tmp, "gen2/dm-build"), st2 = new File(b2, "stage");
        write(new File(st2, "appart/mon.json"), "x");
        write(new File(st2, "monpics/mon.json"), "y");
        List<PackGeneration.Step> two = new ArrayList<>();
        for (String[] e : new String[][]{{"app", "appart/mon.json"}, {"pictures", "monpics/mon.json"}}) {
            File f = new File(st2, e[1]);
            String line = e[1] + "\t" + f.length() + "\t" + DriverLibrary.sha256(f) + "\n";
            write(new File(st2, ".units/" + e[0] + ".tsv"), line);
            two.add(new PackGeneration.Step(e[0], "fp", DriverLibrary.sha256Bytes(line.getBytes(StandardCharsets.UTF_8)), Long.toString(f.lastModified()), PackGeneration.pathDigest(List.of(e[1]))));
        }
        check(refused(() -> PackGeneration.fromStage(b2, st2, two, "i", "s", d -> {}, mtime, (d, t) -> {})), "two stage files on one pack path are refused");
        List<PackGeneration.Step> wrongSet = List.of(new PackGeneration.Step("app", "fp", two.get(0).digest, two.get(0).stamps, "0".repeat(64)));
        check(refused(() -> PackGeneration.fromStage(b2, st2, wrongSet, "i", "s", d -> {}, mtime, (d, t) -> {})), "another path set than expected is refused");

        PackInstall.Manifest dm = PackInstall.Manifest.parse(read(g.manifest));
        File victim = new File(g.folder, "out/main.patched.bin");
        String body = read(victim);
        Files.write(victim.toPath(), body.replace('m', 'M').getBytes(StandardCharsets.UTF_8));
        victim.setLastModified(victim.lastModified() + 5000);
        check(refused(() -> PackGeneration.check(g.folder, dm, Map.of(), mtime, (d, t) -> {})), "a damaged file of the same size is caught");
        check(PackGeneration.installed("monpics/mon/s/a.webp").equals("appart/mon/s/a.webp") && PackGeneration.installed("gen/x") == null
            && PackGeneration.installed("shaders/k.xgs").equals("shaders/k.xgs"), "the path table");

        File py = new File(System.getProperty("dm.root", "."), "dm/tools/pack_steps.py");
        if (py.isFile()) check(read(py).contains("INSTALLED = (('monpics/', 'appart/'), ('gen/', None))"), "the path table equals pack_steps.INSTALLED");
        else check(false, "pack_steps.py not found (set -Ddm.root)");
    }

    interface Body { void run() throws Exception; }
    static boolean refused(Body b) { try { b.run(); return false; } catch (IOException | IllegalArgumentException e) { return true; } catch (Exception e) { return false; } }
}
