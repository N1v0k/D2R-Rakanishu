package com.xiaoboluo.d2r;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.InvalidPathException;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.Map;























final class DriverLibrary {
    static final int MAX_DRIVERS = 8;
    static final long MAX_INCOMING = DriverZip.MAX_ZIP;
    static final String INCOMING = "incoming.tmp", SELECTED = "selected.txt", MARKER = "lib-migrated", META_SUFFIX = ".meta", DIR_SUFFIX = ".dir";






    static final String VALIDATION = "2";




    static boolean recordCurrent(String[] cache) { return cache.length >= 7 && VALIDATION.equals(cache[6]); }







    static boolean recordTrusted(String[] cache) {
        return recordCurrent(cache) && validSha(cache[1]) && validSha(cache[2]) && DriverZip.safeLibrary(cache[3]);
    }








    static String placedHash(File dir) throws IOException {
        if (!realDir(dir)) throw new IOException("not a real directory: " + dir);
        File[] all = dir.listFiles();
        if (all == null) throw new IOException("list " + dir);
        List<String> lines = new ArrayList<>();
        for (File f : all) {


            if (!plainFile(f)) throw new IOException("not a plain file: " + f.getName());
            lines.add(f.getName() + "\0" + sha256(f) + "\n");
        }
        lines.sort(Comparator.naturalOrder());
        StringBuilder b = new StringBuilder();
        for (String l : lines) b.append(l);
        return sha256Bytes(b.toString().getBytes(StandardCharsets.UTF_8));
    }





    private static boolean realDir(File f) {
        try { return Files.isDirectory(f.toPath(), LinkOption.NOFOLLOW_LINKS); } catch (InvalidPathException e) { return false; }
    }
    private static boolean plainFile(File f) {
        try { return Files.isRegularFile(f.toPath(), LinkOption.NOFOLLOW_LINKS); } catch (InvalidPathException e) { return false; }
    }







    static String verifyPlaced(File base, String key, String library) throws IOException {
        if (!validSha(key)) return null;
        File dir = new File(base, key);
        if (!realDir(dir) || !plainFile(new File(dir, library))) return null;
        String now;
        try { now = placedHash(dir); } catch (IOException e) { return null; }
        if (now.equals(key)) return now;
        File[] kids = dir.listFiles();
        if (kids == null || kids.length != 1 || !plainFile(kids[0]) || !kids[0].getName().equals(library)) return null;
        if (!sha256(kids[0]).equals(key)) return null;
        File moved = new File(base, now);
        if (moved.exists()) return null;
        if (!dir.renameTo(moved)) throw new IOException("rename " + dir);
        return now;
    }








    static String migrateLegacySet(File base, File zip, int api, String oldKey, String library) throws IOException {
        if (!validSha(oldKey) || !DriverZip.safeLibrary(library)) return null;
        DriverZip.Meta m = DriverZip.inspect(zip, api);
        if (m.error != null || m.libraries.size() != 1 || !m.library.equals(library)) return null;
        return verifyPlaced(base, oldKey, library);
    }

    static File dirNote(File zip) { return new File(zip.getPath() + DIR_SUFFIX); }



    static void noteDir(File zip, String key) {
        if (!validSha(key)) return;
        try (FileOutputStream o = new FileOutputStream(dirNote(zip))) { o.write((key + "\n").getBytes(StandardCharsets.UTF_8)); }
        catch (IOException e) { }
    }



    static int cleanPlaced(File base, List<String> keep) throws IOException {
        List<String> used = new ArrayList<>(keep);
        File lib = libDir(base);
        File[] zips = lib.listFiles();
        List<File> stale = new ArrayList<>();
        if (zips != null) for (File z : zips) {
            String n = z.getName();
            if (z.isFile() && n.length() == 68 && n.endsWith(".zip") && validSha(n.substring(0, 64))) {
                String k = firstLine(dirNote(z));
                if (validSha(k)) used.add(k);
                continue;
            }




            if (n.endsWith(DIR_SUFFIX) || n.endsWith(META_SUFFIX)) {
                String stem = n.substring(0, n.length() - (n.endsWith(DIR_SUFFIX) ? DIR_SUFFIX.length() : META_SUFFIX.length()));
                if (stem.length() == 68 && stem.endsWith(".zip") && validSha(stem.substring(0, 64)) && !new File(lib, stem).isFile()) stale.add(z);
            }
        }
        for (File s : stale) s.delete();
        int dropped = 0;
        File[] all = base.listFiles();
        if (all == null) return 0;
        for (File f : all) {
            if (!validSha(f.getName()) || used.contains(f.getName())) continue;




            Path p = f.toPath();
            if (!Files.isDirectory(p, LinkOption.NOFOLLOW_LINKS) && !Files.isSymbolicLink(p)) continue;
            deleteTree(f);
            dropped++;
        }
        return dropped;
    }

    private static void deleteTree(File f) {




        if (!realDir(f)) { f.delete(); return; }
        f.setWritable(true);
        File[] kids = f.listFiles();
        if (kids != null) for (File k : kids) deleteTree(k);
        f.delete();
    }

    interface Sync { void sync(File dir) throws IOException; }

    static final class Entry {
        final String sha, name, version; final File zip;
        Entry(String sha, String name, String version, File zip) { this.sha = sha; this.name = name; this.version = version; this.zip = zip; }
    }



    static final class Migration { int imported; String sha = ""; }



    static final class Add { String error; String sha = "", name = "", version = ""; }

    private DriverLibrary() {}

    static File libDir(File base) { return new File(base, "lib"); }
    static File zipFor(File base, String sha) { return new File(libDir(base), sha + ".zip"); }
    static boolean validSha(String s) { return s != null && s.matches("[0-9a-f]{64}"); }

    static List<Entry> list(File base) {
        List<Entry> out = new ArrayList<>();
        File[] all = libDir(base).listFiles();
        if (all != null) for (File f : all) {
            String n = f.getName();
            if (f.isFile() && n.length() == 68 && n.endsWith(".zip") && validSha(n.substring(0, 64))) {
                String[] mv = readMeta(f);
                out.add(new Entry(n.substring(0, 64), mv[0], mv[1], f));
            }
        }
        out.sort(Comparator.comparing((Entry e) -> e.name.toLowerCase(Locale.ROOT)).thenComparing(e -> e.sha));
        return out;
    }




    private static String[] readMeta(File zip) {
        String[] cache = readCache(zip);
        if (cache != null) return cache;
        String[] mv = unpackMeta(zip);
        writeCache(zip, mv);
        return mv;
    }

    private static File metaFile(File zip) { return new File(zip.getPath() + META_SUFFIX); }



    private static String[] readCache(File zip) {
        File f = metaFile(zip);
        if (!f.isFile() || f.length() > 4096) return null;
        String[] l;
        try { l = new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8).split("\n", -1); } catch (IOException e) { return null; }
        if (l.length < 2) return null;
        String name = printable(l[0].trim(), 96), version = printable(l[1].trim(), 96);
        return name.isEmpty() ? null : new String[]{name, version};
    }

    private static void writeCache(File zip, String[] mv) {
        try (FileOutputStream o = new FileOutputStream(metaFile(zip))) { o.write((mv[0] + "\n" + mv[1] + "\n").getBytes(StandardCharsets.UTF_8)); }
        catch (IOException e) { }
    }





    private static String[] unpackMeta(File zip) {
        String fallback = zip.getName().toLowerCase(Locale.ROOT).replaceAll("\\.zip$", "");
        byte[] json = null;
        int count = 0; long total = 0;
        byte[] buf = new byte[64 << 10];
        try (java.util.zip.ZipInputStream in = new java.util.zip.ZipInputStream(new java.io.BufferedInputStream(new java.io.FileInputStream(zip)), StandardCharsets.UTF_8)) {
            java.util.zip.ZipEntry e;
            outer:
            while (json == null && (e = in.getNextEntry()) != null) {
                if (++count > DriverZip.MAX_ENTRIES) break;
                int k; boolean meta = !e.isDirectory() && e.getName().equals("meta.json");
                java.io.ByteArrayOutputStream keep = meta ? new java.io.ByteArrayOutputStream() : null;
                while ((k = in.read(buf)) > 0) {
                    total += k;
                    if (total > DriverZip.MAX_UNPACKED) break outer;
                    if (keep != null) {
                        if (keep.size() + k > DriverZip.MAX_META) break outer;
                        keep.write(buf, 0, k);
                    }
                }
                if (keep != null) json = keep.toByteArray();
            }
        } catch (IOException | RuntimeException e) { json = null; }
        String name = fallback, version = "";
        if (json != null) {
            try {
                Map<String, Object> m = DriverZip.MiniJson.object(new String(json, StandardCharsets.UTF_8));
                if (m != null) {
                    Object n = m.get("name"), v = m.get("driverVersion");
                    if (n instanceof String && !((String) n).isEmpty()) name = (String) n;
                    if (v instanceof String) version = (String) v;
                }
            } catch (RuntimeException e) { name = fallback; version = ""; }
        }
        name = printable(name, 96); version = printable(version, 96);
        return new String[]{name, version};
    }

    private static String printable(String s, int max) {
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < s.length() && b.length() < max; i++) { char ch = s.charAt(i); if (ch >= 0x20 && ch != 0x7f) b.append(ch); }
        return b.toString();
    }



    static String selected(File base) {
        String s = firstLine(new File(base, SELECTED));
        return validSha(s) ? s : "";
    }

    static void select(File base, String sha, Sync sync) throws IOException {
        if (!sha.isEmpty() && !validSha(sha)) throw new IOException("bad driver sha");
        writeFile(new File(base, SELECTED), sha.isEmpty() ? "" : sha + "\n", sync);
    }




    static Add add(File base, File incoming, Sync sync) throws IOException {
        Add a = new Add();
        if (!incoming.isFile() || incoming.length() > MAX_INCOMING) { a.error = "zip-size"; incoming.delete(); return a; }
        a.sha = sha256(incoming);
        File dest = zipFor(base, a.sha);
        if (dest.isFile()) {
            incoming.delete();
            String[] mv = readMeta(dest); a.name = mv[0]; a.version = mv[1];
            return a;
        }
        List<Entry> have = list(base);
        if (have.size() >= MAX_DRIVERS) { a.error = "driver-full"; incoming.delete(); return a; }
        File lib = libDir(base);
        if (!lib.isDirectory() && !lib.mkdirs()) throw new IOException("mkdir " + lib);
        if (!incoming.renameTo(dest)) { incoming.delete(); throw new IOException("rename " + dest); }
        sync.sync(lib); sync.sync(base);
        String[] mv = readMeta(dest); a.name = mv[0]; a.version = mv[1];
        return a;
    }









    static boolean delete(File base, String sha, List<String> keep, Sync sync) throws IOException {
        File zip = zipFor(base, sha);
        if (!zip.isFile()) return false;
        String placed = firstLine(dirNote(zip));
        if (!zip.delete()) return false;
        metaFile(zip).delete();
        dirNote(zip).delete();
        sync.sync(libDir(base));
        if (validSha(placed) && !keep.contains(placed) && !setInUse(base, placed)) deleteTree(new File(base, placed));
        if (selected(base).equals(sha)) select(base, "", sync);
        return true;
    }



    private static boolean setInUse(File base, String key) {
        File[] zips = libDir(base).listFiles();
        if (zips == null) return true;
        for (File z : zips) {
            String n = z.getName();
            if (z.isFile() && n.length() == 68 && n.endsWith(".zip") && validSha(n.substring(0, 64)) && key.equals(firstLine(dirNote(z)))) return true;
        }
        return false;
    }









    static Migration migrate(File base, File extDir, Sync sync) throws IOException {
        if (extDir == null) return null;
        List<File> zips = zips(extDir);
        if (zips.isEmpty() || new File(base, MARKER).isFile()) return null;
        dropBroken(base, sync);
        if (!list(base).isEmpty()) { writeFile(new File(base, MARKER), "1\n", sync); return null; }
        Migration m = new Migration();
        File lib = libDir(base);
        if (!lib.isDirectory() && !lib.mkdirs()) return null;
        for (File z : zips) {
            if (m.imported >= MAX_DRIVERS) break;
            String sha;
            try { sha = sha256(z); } catch (IOException e) { continue; }
            File dest = zipFor(base, sha);
            if (!dest.isFile()) {
                File tmp = new File(dest.getPath() + ".tmp");
                try (InputStream in = new java.io.FileInputStream(z); FileOutputStream out = new FileOutputStream(tmp)) {
                    if (!DriverZip.copyBounded(in, out, DriverZip.MAX_ZIP)) { tmp.delete(); continue; }
                    out.getFD().sync();
                } catch (IOException e) { tmp.delete(); continue; }
                if (!tmp.renameTo(dest)) { tmp.delete(); continue; }
            }
            m.sha = sha; m.imported++;
            if (m.imported > 1) m.sha = "";
            readMeta(dest);
        }
        if (zips.size() == 1 && m.imported == 1 && !m.sha.isEmpty() && selected(base).isEmpty()) select(base, m.sha, sync);
        writeFile(new File(base, MARKER), "1\n", sync);
        sync.sync(lib); sync.sync(base);
        return m;
    }




    private static void dropBroken(File base, Sync sync) throws IOException {
        File lib = libDir(base);
        File[] all = lib.listFiles();
        if (all == null) return;
        boolean changed = false;
        for (File f : all) {
            if (!f.isFile()) continue;
            String n = f.getName();
            boolean entry = n.length() == 68 && n.endsWith(".zip") && validSha(n.substring(0, 64));
            try {
                if (entry) {
                    if (sha256(f).equals(n.substring(0, 64))) continue;
                    metaFile(f).delete();
                } else if (!n.endsWith(".tmp")) continue;
            } catch (IOException e) { continue; }
            f.delete(); changed = true;
        }
        if (changed) sync.sync(lib);
    }

    static List<File> zips(File dir) {
        List<File> out = new ArrayList<>();
        File[] all = dir == null ? null : dir.listFiles();
        if (all != null) for (File f : all) if (f.isFile() && f.getName().toLowerCase(Locale.ROOT).endsWith(".zip")) out.add(f);
        out.sort(File::compareTo);
        return out;
    }



    static void writeFile(File f, String text, Sync sync) throws IOException {
        File tmp = new File(f.getPath() + ".tmp");
        try (FileOutputStream o = new FileOutputStream(tmp)) { o.write(text.getBytes(StandardCharsets.UTF_8)); o.getFD().sync(); }
        if (!tmp.renameTo(f)) { tmp.delete(); throw new IOException("rename " + f); }
        sync.sync(f.getParentFile());
    }

    private static String firstLine(File f) {
        if (!f.isFile() || f.length() > 4096) return "";
        try { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8).trim(); } catch (IOException e) { return ""; }
    }

    static String sha256(File f) throws IOException {
        try (InputStream in = new java.io.FileInputStream(f)) {
            MessageDigest d = MessageDigest.getInstance("SHA-256");
            byte[] buf = new byte[1 << 16]; int k;
            while ((k = in.read(buf)) > 0) d.update(buf, 0, k);
            return hex(d.digest());
        } catch (NoSuchAlgorithmException e) { throw new IOException(e); }
    }

    static String sha256Bytes(byte[] data) {
        try {
            return hex(MessageDigest.getInstance("SHA-256").digest(data));
        } catch (NoSuchAlgorithmException e) { throw new IllegalStateException(e); }
    }

    private static String hex(byte[] digest) {
        StringBuilder s = new StringBuilder();
        for (byte x : digest) s.append(String.format(Locale.ROOT, "%02x", x));
        return s.toString();
    }
}
