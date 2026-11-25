package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.LongSupplier;
























final class PackSteps {


    static final class Unit {
        final String id; final PackProgress.Step progress; final long files; final String after, deps; final boolean resumable;
        final String paths;
        Unit(String id, PackProgress.Step progress, long files, String after, String deps, boolean resumable, String paths) {
            if (files <= 0) throw new IllegalArgumentException("step " + id + " counts no files");
            this.id = id; this.progress = progress; this.files = files; this.after = after; this.deps = deps; this.resumable = resumable; this.paths = paths;
        }
    }


    interface Session {
        String readLine() throws IOException;
        int finish() throws IOException;
        void cancel();
    }
    interface Launcher { Session start(List<String> steps) throws IOException; }

    interface Sweeper { boolean sweep(String why); }
    interface Listener {
        void wrote(Unit u, long files, String name) throws IOException;
        void finished(Unit u) throws IOException;
    }


    static PackSteps none() { return new PackSteps(List.of(), null, "", null, null, why -> true); }


    static PackSteps unavailable(List<String> names) {
        StringBuilder expect = new StringBuilder();
        for (String n : names) expect.append(n).append("\t1\t1\t-\n");
        return new PackSteps(units(names, expect.toString()), null, "", null, null, why -> true);
    }


    static List<String> all() {
        List<String> out = new ArrayList<>();
        for (String[] s : STEPS) out.add(s[0]);
        return out;
    }


    static String inputs(List<PackJournal.Unit> units) {
        StringBuilder s = new StringBuilder();
        for (PackJournal.Unit u : units) s.append(u.id).append('\t').append(u.digest()).append('\n');
        return DriverLibrary.sha256Bytes(s.toString().getBytes(StandardCharsets.UTF_8));
    }
    static final String RUNTIME = "PB-STEP-runtime", RUNNER = "PB-STEP-runner";


    private static final String[][] STEPS = {
        {"rewrite", "REWRITE", null, ""}, {"art", "ART", null, ""}, {"overlay", "OVERLAY", "art", ""}, {"app", "APP_ART", null, ""},
        {"pictures", "PICTURES", null, ""}, {"shaders", "SHADERS", null, "parts"}};
    static final String SHADERS = "shaders", PICTURES = "pictures";
    private static final java.util.regex.Pattern PART = java.util.regex.Pattern.compile("[A-Za-z0-9_][A-Za-z0-9_.-]{0,99}");

    final List<Unit> units;
    private final File stage;
    private final String base;
    private final Launcher launcher;
    private final PackJournal.Stamp stamp;
    private final Sweeper sweeper;
    private volatile Session session;
    private volatile boolean cancelled;
    private volatile boolean stopping;
    private final Map<String, String> keep = new HashMap<>();
    private final Map<String, List<PackJournal.Record>> adopted = new HashMap<>();



    PackSteps(List<Unit> units, File stage, String base, Launcher launcher, PackJournal.Stamp stamp, Sweeper sweeper) {
        this.units = units; this.stage = stage; this.base = base; this.launcher = launcher; this.stamp = stamp; this.sweeper = sweeper;
    }



    static List<Unit> units(List<String> names, String expect) { return units(names, expect, Map.of()); }


    static List<Unit> units(List<String> names, String expect, Map<String, String> deps) {
        Map<String, Long> files = new HashMap<>();
        Map<String, String> paths = new HashMap<>();
        for (String line : expect.split("\n")) {
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] f = line.split("\t", -1);
            if (f.length != 4 || files.containsKey(f[0])) throw new IllegalArgumentException("bad expected-files line: " + line);
            files.put(f[0], Long.parseLong(f[1]));
            paths.put(f[0], f[3]);
        }
        List<Unit> out = new ArrayList<>();
        for (String[] s : STEPS) {
            if (!names.contains(s[0])) continue;
            Long n = files.get(s[0]);
            if (n == null) throw new IllegalArgumentException("no expected files for step " + s[0]);
            if (s[2] != null && !names.contains(s[2])) throw new IllegalArgumentException("step " + s[0] + " needs step " + s[2]);
            out.add(new Unit(s[0], PackProgress.Step.valueOf(s[1]), n, s[2], deps.getOrDefault(s[0], ""), s[3].equals("parts"), paths.get(s[0])));
        }
        if (out.size() != names.size()) throw new IllegalArgumentException("unknown step in " + names);
        return out;
    }



    List<Unit> reconcile(PackJournal journal, Map<String, String> done) throws IOException {
        if (launcher == null) return units;
        Map<String, PackJournal.Record> records = journal.load();
        List<Unit> pending = new ArrayList<>();
        keep.clear();
        adopted.clear();
        for (Unit u : units) {
            PackJournal.Record r = records.get(u.id);
            String upstream = u.after == null ? "" : done.get(u.after);
            if (r != null && upstream != null && r.fingerprint.equals(fingerprint(u, upstream)) && holds(u.id, manifest(u.id), r, journal)) done.put(u.id, r.digest);
            else {
                pending.add(u);
                if (u.resumable && upstream != null) keep.put(u.id, keptParts(u, fingerprint(u, upstream), records, journal));
            }
        }
        return pending;
    }



    List<PackGeneration.Step> finished(PackJournal journal, Map<String, String> done) throws IOException {
        Map<String, PackJournal.Record> records = journal.load();
        List<PackGeneration.Step> out = new ArrayList<>();
        for (Unit u : units) {
            String digest = done.get(u.id), upstream = u.after == null ? "" : done.get(u.after);
            PackJournal.Record r = records.get(u.id);
            if (digest == null || upstream == null || r == null || !r.digest.equals(digest)) throw new IOException("step " + u.id + " is not finished");
            out.add(new PackGeneration.Step(u.id, fingerprint(u, upstream), digest, r.stamps, u.paths));
        }
        return out;
    }

    String fingerprint(Unit u, String upstream) {
        return DriverLibrary.sha256Bytes((base + "\nstep=" + u.id + "\ndeps=" + u.deps + "\nafter=" + upstream + "\n").getBytes(StandardCharsets.UTF_8));
    }



    private String keptParts(Unit u, String fingerprint, Map<String, PackJournal.Record> records, PackJournal journal) throws IOException {
        java.util.TreeMap<String, String> kept = new java.util.TreeMap<>();
        for (Map.Entry<String, PackJournal.Record> e : records.entrySet()) {
            if (!e.getKey().startsWith(u.id + "/")) continue;
            String part = e.getKey().substring(u.id.length() + 1);
            PackJournal.Record r = e.getValue();
            if (!PART.matcher(part).matches() || !r.fingerprint.equals(fingerprint)) continue;
            if (holds(e.getKey(), partManifest(u.id, part), r, journal)) kept.put(part, r.digest);
        }
        File dir = partManifest(u.id, "x").getParentFile(), made = new File(dir, ".fingerprint");
        String[] names = dir.list();
        if (names != null && made.isFile() && new String(Files.readAllBytes(made.toPath()), StandardCharsets.UTF_8).trim().equals(fingerprint)) {
            java.util.Arrays.sort(names);
            List<PackJournal.Record> found = new ArrayList<>();
            for (String n : names) {
                String part = n.endsWith(".tsv") ? n.substring(0, n.length() - 4) : "";
                if (!PART.matcher(part).matches() || kept.containsKey(part)) continue;
                File m = new File(dir, n);
                String digest = DriverLibrary.sha256(m);
                List<String[]> outputs = manifestLines(m, digest);
                if (outputs == null || !intact(outputs)) continue;
                kept.put(part, digest);
                found.add(new PackJournal.Record(u.id + "/" + part, "-", fingerprint, digest, stamps(outputs)));
            }
            adopted.put(u.id, found);
        }
        StringBuilder s = new StringBuilder("fingerprint\t" + fingerprint + "\n");
        for (Map.Entry<String, String> e : kept.entrySet()) s.append(e.getKey()).append('\t').append(e.getValue()).append('\n');
        return s.toString();
    }


    private boolean intact(List<String[]> outputs) throws IOException {
        for (String[] f : outputs) {
            File out = new File(stage, f[0]);
            if (!out.isFile() || out.length() != Long.parseLong(f[1]) || !DriverLibrary.sha256(out).equals(f[2])) return false;
        }
        return true;
    }



    private boolean holds(String unit, File manifest, PackJournal.Record r, PackJournal journal) throws IOException {
        List<String[]> outputs = manifestLines(manifest, r.digest);
        if (outputs == null) return false;
        String[] times = r.stamps == null ? null : r.stamps.split(",");
        if (times != null && times.length != outputs.size()) times = null;
        boolean rehashed = false;
        for (int i = 0; i < outputs.size(); i++) {
            String[] f = outputs.get(i);
            File out = new File(stage, f[0]);
            if (!out.isFile() || out.length() != Long.parseLong(f[1])) return false;
            if (times != null && Long.toString(stamp.of(out)).equals(times[i])) continue;
            if (!DriverLibrary.sha256(out).equals(f[2])) return false;
            rehashed = true;
        }
        if (rehashed) journal.append(new PackJournal.Record(unit, r.identity, r.fingerprint, r.digest, stamps(outputs)));
        return true;
    }


    private List<String[]> manifestLines(File m, String digest) throws IOException {
        if (!m.isFile()) return null;
        byte[] text = Files.readAllBytes(m.toPath());
        if (!DriverLibrary.sha256Bytes(text).equals(digest)) return null;
        List<String[]> out = new ArrayList<>();
        for (String line : new String(text, StandardCharsets.UTF_8).split("\n")) {
            if (line.isEmpty()) continue;
            String[] f = line.split("\t", -1);
            if (f.length != 3) throw new IOException("manifest line of " + m.getName() + " has " + f.length + " fields");
            out.add(f);
        }
        return out;
    }


    private String stamps(List<String[]> outputs) throws IOException {
        if (outputs.isEmpty()) return null;
        StringBuilder s = new StringBuilder();
        for (String[] f : outputs) s.append(s.length() == 0 ? "" : ",").append(stamp.of(new File(stage, f[0])));
        return s.toString();
    }

    File stage() { return stage; }
    File manifest(String step) { return new File(new File(stage, ".units"), step + ".tsv"); }
    File partManifest(String step, String part) { return new File(new File(new File(stage, ".units"), step), part + ".tsv"); }
    File keepFile(String step) { return new File(new File(stage, ".units"), step + ".keep"); }

    void cancel() {
        cancelled = true;
        Session s = session;
        if (s != null) s.cancel();
    }


    void stop() {
        stopping = true;
        Session s = session;
        if (s != null) s.cancel();
    }

    void resume() { stopping = false; }




    String run(List<Unit> pending, PackJournal journal, Map<String, String> done, String identity, Listener listener, LongSupplier clock) throws IOException {
        if (launcher == null) return pending.isEmpty() ? "" : RUNTIME + "\t";

        if (pending.isEmpty()) return sweeper.sweep("before the end") ? "" : RUNNER + "\t";
        if (!sweeper.sweep("before the run")) return RUNNER + "\t";
        Map<String, Unit> byId = new LinkedHashMap<>();
        for (Unit u : pending) byId.put(u.id, u);
        List<String> names = new ArrayList<>(byId.keySet());
        for (Unit u : pending) {
            if (!u.resumable) continue;
            File k = keepFile(u.id);
            if (!k.getParentFile().isDirectory() && !k.getParentFile().mkdirs()) throw new IOException("cannot create the step manifest folder");
            Files.write(k.toPath(), keep.getOrDefault(u.id, "").getBytes(StandardCharsets.UTF_8));
            for (PackJournal.Record r : adopted.getOrDefault(u.id, List.of()))
                journal.append(new PackJournal.Record(r.unit, identity, r.fingerprint, r.digest, r.stamps));
        }
        Session s = launcher.start(names);
        session = s;
        boolean ended = false;
        try {
            if (cancelled || stopping) s.cancel();
            String failure = null;
            boolean believed = true;
            int finished = 0;
            Map<String, Long> began = new HashMap<>();
            for (String line; (line = s.readLine()) != null; ) {
                if (!believed) continue;
                String[] f = line.split("\t", -1);
                Unit u = f.length >= 2 ? byId.get(f[1]) : null;
                String bad = null;
                if (u == null) bad = protocol(line);
                else switch (f[0]) {
                    case "start" -> {
                        if (f.length != 3) bad = protocol(line);
                        else began.put(u.id, clock.getAsLong());
                    }
                    case "file" -> {
                        long n = f.length == 4 && f[2].matches("[0-9]{1,18}") ? Long.parseLong(f[2]) : -1;
                        if (n < 0 || n > u.files) bad = protocol(line);
                        else if (failure == null) listener.wrote(u, n, f[3]);
                    }
                    case "part" -> {
                        if (f.length != 4 || !u.resumable || !PART.matcher(f[2]).matches() || !f[3].matches("[0-9a-f]{64}")) { bad = protocol(line); break; }
                        String upstream = u.after == null ? "" : done.get(u.after);
                        List<String[]> outputs = upstream == null ? null : manifestLines(partManifest(u.id, f[2]), f[3]);
                        if (outputs == null) { bad = protocol(line); break; }
                        journal.append(new PackJournal.Record(u.id + "/" + f[2], identity, fingerprint(u, upstream), f[3], stamps(outputs)));
                    }
                    case "done" -> {
                        if (f.length != 3 || !f[2].matches("[0-9a-f]{64}")) { bad = protocol(line); break; }
                        String upstream = u.after == null ? "" : done.get(u.after);
                        if (upstream == null) { bad = protocol(line); break; }
                        List<String[]> outputs = manifestLines(manifest(u.id), f[2]);
                        if (outputs == null) { bad = protocol(line); break; }
                        journal.append(new PackJournal.Record(u.id, identity, fingerprint(u, upstream), f[2], stamps(outputs)));
                        done.put(u.id, f[2]);
                        finished++;
                        Long t0 = began.get(u.id);
                        if (t0 != null) AppArt.log(String.format(java.util.Locale.ROOT, "pack build: step %s done after %.1f s%s", u.id, (clock.getAsLong() - t0) / 1000.0,
                            failure == null ? "" : " (after the failure)"));
                        listener.finished(u);
                    }
                    case "error" -> {
                        if (f.length != 4) { bad = protocol(line); break; }
                        AppArt.log("pack build: step " + u.id + " failed: " + f[2] + " " + f[3]);
                        if (failure == null) {
                            failure = f[2].startsWith("PB-STEP-") ? f[2] + "\t" : "PB-STEP-" + u.id + "\t";
                            s.cancel();
                        }
                    }
                    default -> bad = protocol(line);
                }
                if (bad != null) {
                    believed = false;
                    if (failure == null) failure = bad;
                    s.cancel();
                }
            }
            int exit = s.finish();
            ended = true;
            boolean clean = sweeper.sweep("after the run");
            if (cancelled || stopping) return PackBuildRun.CANCEL;
            if (failure != null) return failure;
            if (!clean) return RUNNER + "\t";
            if (exit != 0 || finished != pending.size()) {
                AppArt.log("pack build: the step runner ended with " + exit + " after " + finished + " of " + pending.size() + " steps");
                return RUNNER + "\t";
            }
            return "";
        } finally {
            if (!ended) {
                end(s);
                sweeper.sweep("after the run broke off");
            }
            session = null;
        }
    }


    private static void end(Session s) {
        s.cancel();
        try { s.finish(); }
        catch (IOException | RuntimeException e) { AppArt.log("pack build: the step runner did not end cleanly (" + e.getClass().getSimpleName() + ")"); }
    }

    private static String protocol(String line) {
        AppArt.log("pack build: unexpected line from the step runner: " + line);
        return RUNNER + "\t";
    }
}
