package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

















final class PackInstall {
    static final String RECORD = "install.tsv", INSTALLED = "installed.tsv", PREV = "dm-prev", UNDO = "undo";
    static final String[] ITEMS = {"out", "shaders", "overlay", "overlay_variants", "appart", "derived.tsv", PackSource.FILE, "pack_info.txt"};
    static final String[] MARKERS = {PackSource.FILE, "pack_info.txt"};

    enum Phase { SWAP, INSTALLED, CONFIRMED, ROLLBACK, ROLLEDBACK }


    enum Outcome { NOTHING, CONFIRMED, ROLLEDBACK, STOPPED, REFUSED }

    static final class Result {
        final Outcome outcome;
        final String reason;
        Result(Outcome outcome, String reason) { this.outcome = outcome; this.reason = reason; }
        @Override public String toString() { return outcome + (reason == null ? "" : " (" + reason + ")"); }
    }

    interface Fs {
        long number(File f) throws IOException;
        void sync(File dir) throws IOException;
    }

    interface Checks { String check(File dm, Manifest m); }
    interface Progress { void at(long done, long total); }

    interface Hold { void at(String point); }

    interface Confirmed { void at(File dm) throws IOException; }



    static final class Manifest {
        final TreeMap<String, String[]> files = new TreeMap<>();
        final TreeMap<String, String[]> steps = new TreeMap<>();

        void add(String path, long size, String sha, String step) {
            if (path.isEmpty() || path.startsWith("/") || path.contains("..") || path.contains("\t") || path.contains("\n")) throw new IllegalArgumentException("bad pack path " + path);
            String item = path.contains("/") ? path.substring(0, path.indexOf('/')) : path;
            if (!java.util.Arrays.asList(ITEMS).contains(item)) throw new IllegalArgumentException("not a pack item: " + path);
            if (files.put(path, new String[]{Long.toString(size), sha, step}) != null) throw new IllegalArgumentException("two files for the pack path " + path);
        }

        String format() {
            StringBuilder s = new StringBuilder();
            for (Map.Entry<String, String[]> e : files.entrySet()) s.append("file\t").append(e.getKey()).append('\t').append(String.join("\t", e.getValue())).append('\n');
            for (Map.Entry<String, String[]> e : steps.entrySet()) s.append("step\t").append(e.getKey()).append('\t').append(String.join("\t", e.getValue())).append('\n');
            return s.toString();
        }

        static Manifest parse(String text) {
            Manifest m = new Manifest();
            for (String line : text.split("\n")) {
                if (line.isEmpty()) continue;
                String[] f = line.split("\t", -1);
                if (f[0].equals("file") && f.length == 5) m.add(f[1], Long.parseLong(f[2]), f[3], f[4]);
                else if (f[0].equals("step") && f.length == 4) m.steps.put(f[1], new String[]{f[2], f[3]});
                else throw new IllegalArgumentException("bad manifest line " + line);
            }
            return m;
        }


        List<String> items() {
            List<String> out = new ArrayList<>();
            for (String item : ITEMS) {
                for (String p : files.keySet()) if (p.equals(item) || p.startsWith(item + "/")) { out.add(item); break; }
            }
            return out;
        }
    }


    static final class Record {
        final String generation;
        final String manifest;
        final String digest;
        Phase phase;
        final LinkedHashMap<String, String[]> items = new LinkedHashMap<>();

        Record(String generation, String manifest, String digest, Phase phase) {
            this.generation = generation; this.manifest = manifest; this.digest = digest; this.phase = phase;
        }

        String format() {
            StringBuilder s = new StringBuilder("generation\t" + generation + "\nmanifest\t" + manifest + "\ndigest\t" + digest + "\nphase\t"
                + phase.name().toLowerCase(java.util.Locale.ROOT) + "\n");
            for (Map.Entry<String, String[]> e : items.entrySet()) s.append("item\t").append(e.getKey()).append('\t').append(e.getValue()[0]).append('\t').append(e.getValue()[1]).append('\n');
            return s.toString();
        }

        static Record parse(String text) {
            Map<String, String> v = new LinkedHashMap<>();
            LinkedHashMap<String, String[]> items = new LinkedHashMap<>();
            for (String line : text.split("\n")) {
                if (line.isEmpty()) continue;
                String[] f = line.split("\t", -1);
                if (f[0].equals("item") && f.length == 4) items.put(f[1], new String[]{f[2], f[3]});
                else if (f.length == 2 && !v.containsKey(f[0])) v.put(f[0], f[1]);
                else throw new IllegalArgumentException("bad install record line " + line);
            }
            for (String k : new String[]{"generation", "manifest", "digest", "phase"}) if (!v.containsKey(k)) throw new IllegalArgumentException("install record without " + k);
            if (!new ArrayList<>(items.keySet()).equals(java.util.Arrays.asList(ITEMS))) throw new IllegalArgumentException("install record items " + items.keySet());
            Record r = new Record(v.get("generation"), v.get("manifest"), v.get("digest"), Phase.valueOf(v.get("phase").toUpperCase(java.util.Locale.ROOT)));
            r.items.putAll(items);
            return r;
        }
    }

    private final File files, dm, prev, build;
    private final Fs fs;
    private final Checks checks;
    private final Hold hold;
    private final Confirmed confirmed;


    PackInstall(File files, Fs fs, Checks checks, Hold hold) { this(files, fs, checks, hold, dm -> { }); }

    PackInstall(File files, Fs fs, Checks checks, Hold hold, Confirmed confirmed) {
        this.files = files; this.fs = fs; this.checks = checks; this.hold = hold; this.confirmed = confirmed;
        dm = new File(files, "dm"); prev = new File(files, PREV); build = new File(files, "dm-build");
    }

    File record() { return new File(build, RECORD); }



    Result commit(File generation, File manifest, Progress progress) throws IOException {
        if (record().exists()) throw new IllegalStateException("an install is still open");
        String text = new String(Files.readAllBytes(manifest.toPath()), StandardCharsets.UTF_8);
        Manifest m = Manifest.parse(text);
        Record r = new Record(rel(generation), rel(manifest), DriverLibrary.sha256Bytes(text.getBytes(StandardCharsets.UTF_8)), Phase.SWAP);
        for (String item : ITEMS) {
            File old = new File(dm, item), now = new File(generation, item);
            r.items.put(item, new String[]{identity(old), identity(now)});
        }
        for (String item : m.items()) if (r.items.get(item)[1].equals("-")) throw new IOException("the generation lacks " + item);
        settleFolder(dm);
        write(r);
        return forward(r, m, progress);
    }



    Result recover(boolean libraryLoaded) {
        if (libraryLoaded) {
            AppArt.log("ASSUME packbuild.hostidle: the install recovery was asked to run under a loaded game library; nothing moved");
            return new Result(Outcome.REFUSED, "library loaded");
        }
        try {
            if (!record().exists()) { leftovers(); return new Result(Outcome.NOTHING, null); }
            Record r = Record.parse(new String(Files.readAllBytes(record().toPath()), StandardCharsets.UTF_8));
            AppArt.log("pack build: an install of " + r.generation + " was open in phase " + r.phase.name().toLowerCase(java.util.Locale.ROOT));
            switch (r.phase) {
                case SWAP, INSTALLED -> {
                    File mf = new File(files, r.manifest);
                    String text = mf.isFile() ? new String(Files.readAllBytes(mf.toPath()), StandardCharsets.UTF_8) : null;
                    if (text == null || !DriverLibrary.sha256Bytes(text.getBytes(StandardCharsets.UTF_8)).equals(r.digest)) return back(r, "the generation manifest is gone or changed");
                    return forward(r, Manifest.parse(text), (d, t) -> {});
                }
                case CONFIRMED -> { confirmed(r); return new Result(Outcome.CONFIRMED, null); }
                case ROLLBACK -> { return back(r, "a roll back was open"); }
                default -> { rolledback(r); return new Result(Outcome.ROLLEDBACK, "a roll back was open"); }
            }
        } catch (Foreign e) {
            AppArt.log("pack build: install recovery stopped: " + e.getMessage());
            return new Result(Outcome.STOPPED, e.getMessage());
        } catch (IOException | RuntimeException e) {
            AppArt.log("pack build: install recovery stopped: " + e);
            return new Result(Outcome.STOPPED, e.toString());
        }
    }

    private static final class Foreign extends IOException {
        private static final long serialVersionUID = 1;
        Foreign(String what) { super(what); }
    }
    private static final class Missing extends IOException {
        private static final long serialVersionUID = 1;
        Missing(String what) { super(what); }
    }


    private Result forward(Record r, Manifest m, Progress progress) throws IOException {
        long total = ITEMS.length + m.files.size(), done = 0;
        if (r.phase == Phase.SWAP) {
            File gen = new File(files, r.generation);
            settleFolder(dm);
            for (String item : ITEMS) {
                hold.at("install:" + item);
                try { swap(item, r.items.get(item), gen); }
                catch (Missing e) { return back(r, e.getMessage()); }
                progress.at(++done, total);
            }
            for (File d : new File[]{dm, prev, gen}) if (d.isDirectory()) fs.sync(d);
            r.phase = Phase.INSTALLED;
            write(r);
        }
        done = ITEMS.length;
        String failed = verify(r, m);
        progress.at(total, total);
        if (failed != null) return back(r, failed);
        r.phase = Phase.CONFIRMED;
        write(r);
        confirmed(r);
        return new Result(Outcome.CONFIRMED, null);
    }

    private void swap(String item, String[] ids, File gen) throws IOException {
        File at = new File(dm, item), old = new File(prev, item), now = new File(gen, item);
        String here = identity(at);
        if (!ids[1].equals("-") && here.equals(ids[1])) return;
        if (ids[1].equals("-") && here.equals("-")) return;
        if (!here.equals("-")) {
            if (!here.equals(ids[0])) throw new Foreign(item + " in the pack is neither the old nor the new one");
            if (old.exists()) throw new Foreign(item + " is in " + PREV + " already");
            mkdirs(prev);
            move(at, old);
        }
        if (ids[1].equals("-")) return;
        if (!identity(now).equals(ids[1])) throw new Missing("the new " + item + " is missing or changed");
        move(now, at);
    }



    private String verify(Record r, Manifest m) throws IOException {
        for (String item : ITEMS) if (!identity(new File(dm, item)).equals(r.items.get(item)[1])) return item + " is not the new one";
        Map<String, Long> found = new TreeMap<>();
        for (String item : ITEMS) walk(new File(dm, item), item, found);
        if (!found.keySet().equals(m.files.keySet())) return "the installed paths differ from the manifest (" + found.size() + " files, " + m.files.size() + " listed)";
        for (Map.Entry<String, Long> e : found.entrySet()) if (e.getValue() != Long.parseLong(m.files.get(e.getKey())[0])) return e.getKey() + " has another size";
        return checks.check(dm, m);
    }

    private static void walk(File f, String rel, Map<String, Long> out) throws IOException {
        if (Files.isSymbolicLink(f.toPath())) throw new IOException("link in the pack: " + rel);
        if (f.isFile()) { out.put(rel, f.length()); return; }
        String[] names = f.list();
        if (names == null) return;
        for (String n : names) walk(new File(f, n), rel + "/" + n, out);
    }


    private Result back(Record r, String why) throws IOException {
        AppArt.log("pack build: the install rolls back: " + why);
        if (r.phase != Phase.ROLLBACK) { r.phase = Phase.ROLLBACK; write(r); }
        File undo = new File(build, UNDO);
        for (int i = ITEMS.length - 1; i >= 0; i--) {
            String item = ITEMS[i];
            hold.at("rollback:" + item);
            String[] ids = r.items.get(item);
            File at = new File(dm, item), old = new File(prev, item);
            String here = identity(at);
            if (!ids[1].equals("-") && here.equals(ids[1])) {
                mkdirs(undo);
                File u = new File(undo, item);
                if (u.exists()) throw new Foreign(item + " is in " + UNDO + " already");
                move(at, u);
                here = "-";
            }
            if (here.equals("-")) {
                if (!ids[0].equals("-")) {
                    if (!identity(old).equals(ids[0])) throw new Foreign("the old " + item + " is not in " + PREV);
                    move(old, at);
                }
            } else if (!here.equals(ids[0])) throw new Foreign(item + " in the pack is neither the old nor the new one");
        }
        for (File d : new File[]{dm, prev, undo}) if (d.isDirectory()) fs.sync(d);
        r.phase = Phase.ROLLEDBACK;
        write(r);
        rolledback(r);
        return new Result(Outcome.ROLLEDBACK, why);
    }

    private void confirmed(Record r) throws IOException {
        delete(prev);
        delete(new File(files, r.generation));
        File mf = new File(files, r.manifest), kept = new File(build, INSTALLED);
        if (mf.isFile() && !mf.equals(kept)) {
            hold.at("move:" + rel(mf));
            if (!mf.renameTo(kept)) throw new IOException("cannot keep the generation manifest");
            fs.sync(build);
        }
        hold.at("confirmed");
        confirmed.at(dm);
        remove(record());
        AppArt.log("pack build: the new pack is installed (" + r.generation + ")");
    }

    private void rolledback(Record r) throws IOException {
        delete(new File(build, UNDO));
        delete(prev);
        delete(new File(files, r.generation));
        File mf = new File(files, r.manifest);
        if (!mf.equals(new File(build, INSTALLED))) remove(mf);
        remove(record());
        AppArt.log("pack build: rolled back to generation " + r.generation);
    }


    private void leftovers() throws IOException {
        delete(prev);
        delete(new File(build, UNDO));
        delete(new File(dm, PackGeneration.IMPORT));
        String[] names = build.list();
        if (names != null) for (String n : names) if (n.startsWith(PackGeneration.GEN)) delete(new File(build, n));
    }


    private String identity(File f) throws IOException {
        if (!f.exists() && !Files.isSymbolicLink(f.toPath())) return "-";
        String n = Long.toString(fs.number(f));
        for (String m : MARKERS) if (f.getName().equals(m) && f.isFile()) return n + ":" + DriverLibrary.sha256(f);
        return n;
    }

    private void write(Record r) throws IOException {
        mkdirs(build);
        hold.at("record:" + r.phase.name().toLowerCase(java.util.Locale.ROOT));
        DriverLibrary.writeFile(record(), r.format(), fs::sync);
    }

    private void move(File from, File to) throws IOException {
        hold.at("move:" + rel(from));
        if (!from.renameTo(to)) throw new IOException("cannot move " + rel(from) + " to " + rel(to));
    }

    private void delete(File f) throws IOException {
        if (!f.exists()) return;
        hold.at("delete:" + rel(f));
        deleteTree(f);
    }


    private void settleFolder(File d) throws IOException {
        if (d.isDirectory()) return;
        mkdirs(d);
        fs.sync(d.getParentFile());
    }

    private void mkdirs(File d) throws IOException { if (!d.isDirectory() && !d.mkdirs()) throw new IOException("cannot create " + rel(d)); }

    private void remove(File f) throws IOException {
        if (f.exists()) hold.at("remove:" + rel(f));
        if (f.exists() && !f.delete()) throw new IOException("cannot remove " + rel(f));
        fs.sync(f.getParentFile());
    }

    static void deleteTree(File f) throws IOException {
        if (!f.exists() && !Files.isSymbolicLink(f.toPath())) return;
        File[] kids = f.isDirectory() && !Files.isSymbolicLink(f.toPath()) ? f.listFiles() : null;
        if (kids != null) for (File k : kids) deleteTree(k);
        if (!f.delete()) throw new IOException("cannot remove " + f.getName());
    }

    private String rel(File f) {
        String base = files.getAbsolutePath() + File.separator, p = f.getAbsolutePath();
        return p.startsWith(base) ? p.substring(base.length()) : f.getName();
    }
}
