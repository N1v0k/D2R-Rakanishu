package com.xiaoboluo.d2r;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.system.StructStat;
import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;



final class PackPick {
    static final String SOURCES = PackSession.SOURCES;
    static final long KEYS_MAX = 1 << 20;

    private PackPick() {}



    static Intent intent() {
        return new Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*")
            .putExtra(Intent.EXTRA_LOCAL_ONLY, true).putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
    }


    static List<Uri> uris(Intent data) {
        List<Uri> out = new ArrayList<>();
        if (data == null) return out;
        if (data.getClipData() != null) for (int i = 0; i < data.getClipData().getItemCount(); i++) out.add(data.getClipData().getItemAt(i).getUri());
        else if (data.getData() != null) out.add(data.getData());
        return out;
    }


    static PackRows.Pick probe(Context c, Uri uri, byte[] keys, String contract) {
        ContentResolver cr = c.getContentResolver();
        String name = uri.getLastPathSegment();
        long reported = -1;
        try (Cursor q = cr.query(uri, new String[]{OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE}, null, null, null)) {
            if (q != null && q.moveToFirst()) {
                if (!q.isNull(0)) name = q.getString(0);
                if (!q.isNull(1)) reported = q.getLong(1);
            }
        } catch (RuntimeException e) { return new PackRows.Pick(uri.toString(), name, "PB-NOTLOCAL\t"); }
        String id = DocumentsContract.isDocumentUri(c, uri) ? DocumentsContract.getDocumentId(uri) : null;
        if (!PackDocs.local(uri.getAuthority(), id, TestHooks.packDocsAuthority(c), "local")) return new PackRows.Pick(uri.toString(), name, "PB-NOTLOCAL\t");
        try (ParcelFileDescriptor pfd = cr.openFileDescriptor(uri, "r")) {
            if (pfd == null) return new PackRows.Pick(uri.toString(), name, "PB-NOTLOCAL\t");
            if (!localDescriptor(pfd.getFileDescriptor(), reported)) return new PackRows.Pick(uri.toString(), name, "PB-NOTLOCAL\t");
            List<PackJournal.Unit> units = PackBuildRun.units(PackReader.nativeUnits(contract), PackBuildService.exefsDir(c), PackBuildService.dataDir(c));
            Map<String, ParcelFileDescriptor> self = new LinkedHashMap<>();
            self.put("picked", pfd);
            if (inPlace(self, units) != null) return new PackRows.Pick(uri.toString(), name, "PB-INPLACE\t");
            String result = PackReader.nativeProbe(pfd.getFd(), reported, keys, contract);

            return new PackRows.Pick(uri.toString(), name, result.startsWith("PB-SOURCE") ? "PB-NOTLOCAL\t" : result);
        } catch (IOException | RuntimeException e) {
            AppArt.log("pack build: a picked document could not be opened (" + e.getClass().getSimpleName() + ")");
            return new PackRows.Pick(uri.toString(), name, "PB-NOTLOCAL\t");
        }
    }

    private static boolean localDescriptor(FileDescriptor fd, long reported) {
        try {
            StructStat st = Os.fstat(fd);
            long end = Os.lseek(fd, 0, OsConstants.SEEK_END);
            Os.lseek(fd, 0, OsConstants.SEEK_SET);
            return PackDocs.descriptor(OsConstants.S_ISREG(st.st_mode), end, st.st_size, reported);
        } catch (ErrnoException e) { return false; }
    }


    static byte[] readKeys(Context c, Uri uri) {
        ParcelFileDescriptor pfd;
        try { pfd = c.getContentResolver().openFileDescriptor(uri, "r"); } catch (IOException | RuntimeException e) { return null; }
        if (pfd == null) return null;
        byte[] keys = null;
        boolean ok = false;
        try {
            keys = readKeys(pfd);
            pfd.close();
            ok = true;
            return keys;
        } catch (IOException | RuntimeException e) {
            return null;
        } finally {
            if (!ok && keys != null) Arrays.fill(keys, (byte) 0);
        }
    }


    static byte[] readKeys(ParcelFileDescriptor pfd) {
        long size = pfd.getStatSize();
        if (size <= 0 || size > KEYS_MAX) return null;
        byte[] b = new byte[(int) size];
        boolean ok = false;
        try {
            FileDescriptor fd = pfd.getFileDescriptor();
            Os.lseek(fd, 0, OsConstants.SEEK_SET);
            int n = 0;
            while (n < b.length) { int k = Os.read(fd, b, n, b.length - n); if (k <= 0) break; n += k; }
            ok = n == b.length;
            return ok ? b : null;
        } catch (ErrnoException | java.io.InterruptedIOException | RuntimeException e) {
            return null;
        } finally {
            if (!ok) Arrays.fill(b, (byte) 0);
        }
    }


    static String inPlace(Map<String, ParcelFileDescriptor> inputs, List<PackJournal.Unit> units) {
        Map<String, PackInPlace.Where> in = new LinkedHashMap<>();
        for (Map.Entry<String, ParcelFileDescriptor> e : inputs.entrySet()) in.put(e.getKey(), where(e.getValue()));
        List<PackInPlace.Where> out = new ArrayList<>();
        for (PackJournal.Unit u : units) for (PackJournal.Output o : u.outputs) for (File f : new File[]{o.file, o.part()}) {
            try { StructStat st = Os.stat(f.getPath()); out.add(new PackInPlace.Where(st.st_dev, st.st_ino, f.getPath())); }
            catch (ErrnoException e) { if (e.errno != OsConstants.ENOENT) throw new IllegalStateException("stat " + f.getName(), e); }
        }
        return PackInPlace.collision(in, out);
    }



    static String compact(List<PackJournal.Unit> units) {
        for (PackJournal.Unit u : units) for (PackJournal.Output o : u.outputs) if (new File(o.file.getPath() + ".holes").exists()) return o.file.getName();
        return null;
    }

    private static PackInPlace.Where where(ParcelFileDescriptor pfd) {
        try {
            StructStat st = Os.fstat(pfd.getFileDescriptor());
            String path;
            try { path = Os.readlink("/proc/self/fd/" + pfd.getFd()); } catch (ErrnoException e) { path = null; }
            return new PackInPlace.Where(st.st_dev, st.st_ino, path);
        } catch (ErrnoException e) { throw new IllegalStateException("fstat of a picked document", e); }
    }


    static void keep(Context c, Uri uri) {
        if (uri.getAuthority() != null && uri.getAuthority().equals(TestHooks.packDocsAuthority(c))) return;
        c.getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
    }

    static void release(Context c, String uri) {
        Uri u = Uri.parse(uri);
        if (u.getAuthority() != null && u.getAuthority().equals(TestHooks.packDocsAuthority(c))) return;
        try { c.getContentResolver().releasePersistableUriPermission(u, Intent.FLAG_GRANT_READ_URI_PERMISSION); }
        catch (SecurityException e) { AppArt.log("pack build: a source grant was already gone"); }
    }


    static final class Sources {
        final String base, baseId, update, updateId, keys;
        Sources(String base, String baseId, String update, String updateId, String keys) {
            this.base = base; this.baseId = baseId; this.update = update; this.updateId = updateId; this.keys = keys;
        }
        String identity() { return baseId + "+" + updateId; }
        List<String> uris() { return List.of(base, update, keys); }
        String uri(PackRows.Row r) { return r == PackRows.Row.BASE ? base : r == PackRows.Row.UPDATE ? update : keys; }
        String format() { return "base\t" + base + "\t" + baseId + "\nupdate\t" + update + "\t" + updateId + "\nkeys\t" + keys + "\n"; }
        static Sources parse(String text) {
            String b = null, bi = null, u = null, ui = null, k = null;
            for (String line : text.split("\n")) {
                if (line.isEmpty()) continue;
                String[] f = line.split("\t", -1);
                switch (f[0]) {
                    case "base" -> { need(f, 3); b = f[1]; bi = f[2]; }
                    case "update" -> { need(f, 3); u = f[1]; ui = f[2]; }
                    case "keys" -> { need(f, 2); k = f[1]; }
                    default -> throw new IllegalArgumentException("unknown sources line " + f[0]);
                }
            }
            if (b == null || u == null || k == null) throw new IllegalArgumentException("sources incomplete");
            return new Sources(b, bi, u, ui, k);
        }
        private static void need(String[] f, int n) { if (f.length != n) throw new IllegalArgumentException("sources line " + f[0] + " has " + f.length + " fields"); }
    }

    static Sources sources(Context c) {
        File f = new File(PackBuildService.dir(c), SOURCES);
        if (!f.isFile()) return null;
        try { return Sources.parse(new String(java.nio.file.Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)); }
        catch (IOException e) { throw new IllegalStateException("pack build sources unreadable", e); }
    }

    static void saveSources(Context c, Sources s) throws IOException {
        File dir = PackBuildService.dir(c);
        if (!dir.isDirectory() && !dir.mkdirs()) throw new IOException("cannot create " + dir.getName());
        DriverLibrary.writeFile(new File(dir, SOURCES), s.format(), PackBuildService.SYNC);
    }
}
