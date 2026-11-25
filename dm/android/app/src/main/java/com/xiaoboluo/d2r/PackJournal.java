package com.xiaoboluo.d2r;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Consumer;














final class PackJournal {
    static final String FILE = "journal.tsv";


    interface Stamp { long of(File f) throws IOException; }

    static final class Record {
        final String unit, identity, fingerprint, digest;
        final String stamps;
        Record(String unit, String identity, String fingerprint, String digest, String stamps) {
            for (String f : new String[]{unit, identity, fingerprint, digest})
                if (f == null || f.isEmpty() || f.indexOf('\t') >= 0 || f.indexOf('\n') >= 0) throw new IllegalArgumentException("journal field is empty or holds a separator");
            if (stamps != null && !stamps.matches("\\d+(,\\d+)*")) throw new IllegalArgumentException("journal stamps malformed: " + stamps);
            this.unit = unit; this.identity = identity; this.fingerprint = fingerprint; this.digest = digest; this.stamps = stamps;
        }
        String line() { return unit + "\t" + identity + "\t" + fingerprint + "\t" + digest + (stamps == null ? "" : "\t" + stamps) + "\n"; }
    }


    static final class Output {
        final File file; final long size; final String sha256;
        Output(File file, long size, String sha256) {
            if (size < 0 || !sha256.matches("[0-9a-f]{64}")) throw new IllegalArgumentException("bad output " + file.getName());
            this.file = file; this.size = size; this.sha256 = sha256;
        }
        File part() { return new File(file.getPath() + ".part"); }
    }


    static final class Unit {
        final String id, fingerprint; final List<Output> outputs;
        Unit(String id, String fingerprint, List<Output> outputs) {
            if (outputs.isEmpty()) throw new IllegalArgumentException("unit " + id + " has no outputs");
            this.id = id; this.fingerprint = fingerprint; this.outputs = outputs;
        }
        long bytes() { long n = 0; for (Output o : outputs) n += o.size; return n; }

        String digest() {
            StringBuilder s = new StringBuilder();
            for (Output o : outputs) s.append(o.sha256).append('\n');
            return DriverLibrary.sha256Bytes(s.toString().getBytes(StandardCharsets.UTF_8));
        }
    }

    private final File file;
    private final DriverLibrary.Sync sync;
    private final Stamp stamp;

    PackJournal(File file, DriverLibrary.Sync sync, Stamp stamp) { this.file = file; this.sync = sync; this.stamp = stamp; }


    Map<String, Record> load() throws IOException {
        Map<String, Record> out = new LinkedHashMap<>();
        if (!file.exists()) return out;
        String text = new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8);
        int end = text.lastIndexOf('\n') + 1;
        for (String line : text.substring(0, end).split("\n")) {
            if (line.isEmpty()) continue;
            String[] f = line.split("\t", -1);
            if (f.length != 4 && f.length != 5) throw new IOException("journal line has " + f.length + " fields");
            out.put(f[0], new Record(f[0], f[1], f[2], f[3], f.length == 5 ? f[4] : null));
        }
        return out;
    }

    void append(Record r) throws IOException {
        boolean created = !file.exists();
        if (!created) cutTornTail();
        try (FileOutputStream o = new FileOutputStream(file, true)) {
            o.write(r.line().getBytes(StandardCharsets.UTF_8));
            o.getFD().sync();
        }
        if (created) sync.sync(file.getParentFile());
    }

    private void cutTornTail() throws IOException {
        try (RandomAccessFile f = new RandomAccessFile(file, "rw")) {
            long n = f.length(), keep = n;
            while (keep > 0) { f.seek(keep - 1); if (f.read() == '\n') break; keep--; }
            if (keep != n) { f.setLength(keep); f.getFD().sync(); }
        }
    }


    void done(Unit u, String identity) throws IOException { append(new Record(u.id, identity, u.fingerprint, u.digest(), stamps(u))); }



    List<Unit> reconcile(List<Unit> units, String identity, Consumer<String> checking) throws IOException {
        Map<String, Record> records = load();
        List<Unit> pending = new ArrayList<>();
        for (Unit u : units) {
            for (Output o : u.outputs) if (o.part().exists() && !o.part().delete()) throw new IOException("cannot remove " + o.part().getName());
            Record r = records.get(u.id);
            boolean stale = r != null && !(r.fingerprint.equals(u.fingerprint) && r.digest.equals(u.digest()));
            if (stale || !present(u)) { pending.add(u); continue; }
            if (r != null && stamps(u).equals(r.stamps)) continue;
            if (hashesMatch(u, checking)) { adopt(u, identity); continue; }
            pending.add(u);
        }
        return pending;
    }



    private void adopt(Unit u, String identity) throws IOException {
        Set<File> dirs = new LinkedHashSet<>();
        for (Output o : u.outputs) dirs.add(o.file.getParentFile());
        for (File d : dirs) sync.sync(d);
        done(u, identity);
    }

    private String stamps(Unit u) throws IOException {
        StringBuilder s = new StringBuilder();
        for (Output o : u.outputs) s.append(s.length() == 0 ? "" : ",").append(stamp.of(o.file));
        return s.toString();
    }

    private static boolean present(Unit u) {
        for (Output o : u.outputs) if (!o.file.isFile() || o.file.length() != o.size) return false;
        return true;
    }

    private static boolean hashesMatch(Unit u, Consumer<String> checking) throws IOException {
        for (Output o : u.outputs) {
            checking.accept(o.file.getName());
            if (!o.sha256.equals(DriverLibrary.sha256(o.file))) return false;
        }
        return true;
    }
}
