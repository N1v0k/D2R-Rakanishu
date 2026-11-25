package com.xiaoboluo.d2r;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;










final class PackGeneration {
    static final String GEN = "gen-", IMPORT = "import.part";
    static final String[][] INSTALLED = {{"monpics/", "appart/"}, {"gen/", null}};

    private static final String[] MOVED = {"out", "shaders", "overlay", "overlay_variants", "appart", "derived.tsv", "monpics"};


    static final class Step {
        final String name, fingerprint, digest, stamps, paths;
        Step(String name, String fingerprint, String digest, String stamps, String paths) {
            this.name = name; this.fingerprint = fingerprint; this.digest = digest; this.stamps = stamps; this.paths = paths;
        }
    }


    static final class Made {
        final File folder, manifest;
        Made(File folder, File manifest) { this.folder = folder; this.manifest = manifest; }
    }

    private PackGeneration() {}


    static String installed(String stagePath) {
        for (String[] t : INSTALLED) if (stagePath.startsWith(t[0])) return t[1] == null ? null : t[1] + stagePath.substring(t[0].length());
        return stagePath;
    }


    static String pathDigest(List<String> paths) {
        List<String> sorted = new ArrayList<>(paths);
        java.util.Collections.sort(sorted);
        StringBuilder s = new StringBuilder();
        for (String p : sorted) s.append(p).append('\n');
        return DriverLibrary.sha256Bytes(s.toString().getBytes(StandardCharsets.UTF_8));
    }


    static Map<String, String> expectedPaths(String expect) {
        Map<String, String> out = new HashMap<>();
        for (String line : expect.split("\n")) {
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] f = line.split("\t", -1);
            if (f.length == 4) out.put(f[0], f[3]);
        }
        return out;
    }


    static int next(File build) {
        int n = 0;
        String[] names = build.list();
        if (names != null) for (String name : names) {
            if (!name.startsWith(GEN)) continue;
            String num = name.substring(GEN.length()).replace(".tsv", "");
            if (num.matches("[0-9]{1,9}")) n = Math.max(n, Integer.parseInt(num));
        }
        return n + 1;
    }



    static Made fromStage(File build, File stage, List<Step> steps, String info, String source, DriverLibrary.Sync sync, PackJournal.Stamp stamp,
                          PackInstall.Progress progress) throws IOException {
        int n = next(build);
        File folder = new File(build, GEN + n), mf = new File(build, GEN + n + ".tsv");
        PackInstall.Manifest m = new PackInstall.Manifest();
        Map<String, String> stamps = new HashMap<>();
        for (Step s : steps) {
            File sm = new File(stage, ".units/" + s.name + ".tsv");
            byte[] text = sm.isFile() ? Files.readAllBytes(sm.toPath()) : null;
            if (text == null || !DriverLibrary.sha256Bytes(text).equals(s.digest)) throw new IOException("the manifest of step " + s.name + " is not the journaled one");
            List<String[]> lines = new ArrayList<>();
            List<String> paths = new ArrayList<>();
            for (String line : new String(text, StandardCharsets.UTF_8).split("\n")) {
                if (line.isEmpty()) continue;
                String[] f = line.split("\t", -1);
                if (f.length != 3) throw new IOException("bad manifest line of step " + s.name);
                lines.add(f);
                paths.add(f[0]);
            }
            if (!pathDigest(paths).equals(s.paths)) throw new IOException("step " + s.name + " wrote another file set than expected");
            String[] st = s.stamps == null ? null : s.stamps.split(",");
            for (int i = 0; i < lines.size(); i++) {
                String[] f = lines.get(i);
                String p = installed(f[0]);
                if (p == null) continue;
                try { m.add(p, Long.parseLong(f[1]), f[2], s.name); }
                catch (IllegalArgumentException e) { throw new IOException(e.getMessage()); }
                if (st != null && st.length == lines.size()) stamps.put(p, st[i]);
            }
            m.steps.put(s.name, new String[]{s.fingerprint, s.digest});
        }
        byte[] infoBytes = info.getBytes(StandardCharsets.UTF_8), sourceBytes = source.getBytes(StandardCharsets.UTF_8);
        m.add("pack_info.txt", infoBytes.length, DriverLibrary.sha256Bytes(infoBytes), "-");
        m.add(PackSource.FILE, sourceBytes.length, DriverLibrary.sha256Bytes(sourceBytes), "-");
        DriverLibrary.writeFile(mf, m.format(), sync);
        if (!folder.isDirectory() && !folder.mkdirs()) throw new IOException("cannot create " + folder.getName());
        for (String top : MOVED) {
            File from = new File(stage, top);
            if (!from.exists()) continue;
            if (top.equals("monpics")) {
                File appart = new File(folder, "appart");
                String[] kids = from.list();
                if (kids == null) throw new IOException("cannot list the pictures");
                if (!appart.isDirectory()) throw new IOException("the pictures have no appart to go into");
                for (String k : kids) {
                    File to = new File(appart, k);
                    if (to.exists()) throw new IOException("the picture path appart/" + k + " is taken");
                    if (!new File(from, k).renameTo(to)) throw new IOException("cannot move monpics/" + k);
                }
                sync.sync(appart);
                sync.sync(from);
                continue;
            }
            if (!from.renameTo(new File(folder, top))) throw new IOException("cannot move " + top + " out of the stage");
        }
        DriverLibrary.writeFile(new File(folder, "pack_info.txt"), info, sync);
        DriverLibrary.writeFile(new File(folder, PackSource.FILE), source, sync);
        sync.sync(folder);
        sync.sync(stage);
        check(folder, m, stamps, stamp, progress);
        return new Made(folder, mf);
    }


    static void check(File folder, PackInstall.Manifest m, Map<String, String> stamps, PackJournal.Stamp stamp, PackInstall.Progress progress) throws IOException {
        Map<String, File> found = new java.util.TreeMap<>();
        for (String item : PackInstall.ITEMS) walk(new File(folder, item), item, found);
        String[] extra = folder.list();
        if (extra != null) for (String e : extra) if (!java.util.Arrays.asList(PackInstall.ITEMS).contains(e)) throw new IOException("the generation holds " + e);
        if (!found.keySet().equals(m.files.keySet())) throw new IOException("the generation holds " + found.size() + " files, the manifest lists " + m.files.size());
        long done = 0, total = found.size();
        for (Map.Entry<String, File> e : found.entrySet()) {
            String[] want = m.files.get(e.getKey());
            File f = e.getValue();
            if (f.length() != Long.parseLong(want[0])) throw new IOException(e.getKey() + " has another size than its manifest says");
            String t = stamps.get(e.getKey());
            if (t == null || !t.equals(Long.toString(stamp.of(f)))) {
                if (!DriverLibrary.sha256(f).equals(want[1])) throw new IOException(e.getKey() + " is not the file its manifest names");
            }
            progress.at(++done, total);
        }
    }

    private static void walk(File f, String rel, Map<String, File> out) throws IOException {
        if (Files.isSymbolicLink(f.toPath())) throw new IOException("link in the pack: " + rel);
        if (f.isFile()) { out.put(rel, f); return; }
        String[] names = f.list();
        if (names == null) return;
        for (String n : names) walk(new File(f, n), rel + "/" + n, out);
    }



    static Made fromComputer(File files, File source, String record, DriverLibrary.Sync sync, PackInstall.Progress progress) throws IOException {
        File folder = new File(files, "dm/" + IMPORT), mf = new File(files, "dm-build/" + GEN + "import.tsv");
        PackInstall.deleteTree(folder);
        List<String> items = new ArrayList<>();
        for (String item : PackInstall.ITEMS) if (!item.equals(PackSource.FILE) && new File(source, item).exists()) items.add(item);
        for (String required : new String[]{"out", "shaders", "overlay", "appart", "derived.tsv", "pack_info.txt"})
            if (!items.contains(required)) throw new IOException("missing " + required);
        long total = 0;
        for (String item : items) total += size(new File(source, item));
        PackInstall.Manifest m = new PackInstall.Manifest();
        long[] done = {0};
        byte[] buffer = new byte[1 << 20];
        for (String item : items) copy(new File(source, item), new File(folder, item), item, m, buffer, done, total, sync, progress);
        DriverLibrary.writeFile(new File(folder, PackSource.FILE), record, sync);
        byte[] r = record.getBytes(StandardCharsets.UTF_8);
        m.add(PackSource.FILE, r.length, DriverLibrary.sha256Bytes(r), "-");
        sync.sync(folder);
        File build = mf.getParentFile();
        if (!build.isDirectory() && !build.mkdirs()) throw new IOException("cannot create " + build.getName());
        DriverLibrary.writeFile(mf, m.format(), sync);
        return new Made(folder, mf);
    }

    private static long size(File f) {
        if (!f.isDirectory()) return f.length();
        long n = 0;
        File[] list = f.listFiles();
        if (list != null) for (File c : list) n += size(c);
        return n;
    }

    private static void copy(File from, File to, String rel, PackInstall.Manifest m, byte[] buffer, long[] done, long total, DriverLibrary.Sync sync,
                             PackInstall.Progress progress) throws IOException {
        if (from.isDirectory()) {
            if (!to.isDirectory() && !to.mkdirs()) throw new IOException("mkdir " + to.getName());
            File[] list = from.listFiles();
            if (list == null) throw new IOException("list " + from.getName());
            for (File c : list) copy(c, new File(to, c.getName()), rel + "/" + c.getName(), m, buffer, done, total, sync, progress);
            sync.sync(to);
            return;
        }
        MessageDigest h;
        try { h = MessageDigest.getInstance("SHA-256"); } catch (NoSuchAlgorithmException e) { throw new IllegalStateException(e); }
        long n = 0;
        try (InputStream in = new FileInputStream(from); FileOutputStream out = new FileOutputStream(to)) {
            for (int k; (k = in.read(buffer)) > 0; ) {
                out.write(buffer, 0, k);
                h.update(buffer, 0, k);
                n += k;
                done[0] += k;
                progress.at(done[0], Math.max(total, done[0]));
            }
            out.getFD().sync();
        }
        StringBuilder hex = new StringBuilder();
        for (byte b : h.digest()) hex.append(String.format("%02x", b));
        try { m.add(rel, n, hex.toString(), "-"); } catch (IllegalArgumentException e) { throw new IOException(e.getMessage()); }
    }
}
