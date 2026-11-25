package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;















final class PackSession {
    static final String KEPT = "kept", ENDING = "ending", SOURCES = "sources.tsv", STATE = PackBuildState.FILE;

    interface Grants { void release(String uri); }

    private final File dir;
    private final DriverLibrary.Sync sync;
    private final Grants grants;

    PackSession(File dir, DriverLibrary.Sync sync, Grants grants) { this.dir = dir; this.sync = sync; this.grants = grants; }

    private File file(String name) { return new File(dir, name); }


    boolean active() { return !file(ENDING).exists() && (file(SOURCES).isFile() || file(KEPT).isFile()); }


    boolean kept() { return active() && !file(SOURCES).isFile(); }


    void startKept() throws IOException {
        if (!dir.isDirectory() && !dir.mkdirs()) throw new IOException("cannot create " + dir.getName());
        remove(file(STATE));
        File src = file(SOURCES);
        if (src.isFile()) {
            release(src);
            remove(src);
            sync.sync(dir);
        }
        DriverLibrary.writeFile(file(KEPT), "1\n", sync);
    }


    void keptDone() throws IOException {
        if (!file(KEPT).exists()) return;
        remove(file(KEPT));
        sync.sync(dir);
    }


    void markEnding() throws IOException {
        if (file(ENDING).exists()) return;
        if (!file(SOURCES).exists() && !file(KEPT).exists() && !file(STATE).exists()) return;
        DriverLibrary.writeFile(file(ENDING), "1\n", sync);
    }


    void confirmed(File dm) throws IOException {
        PackSource.Record r;
        try { r = PackSource.parse(read(new File(dm, PackSource.FILE))); } catch (IllegalArgumentException e) { r = null; }
        if (r == null || r.source != PackSource.Kind.PHONE) markEnding();
    }

    private static String read(File f) {
        try { return f.isFile() ? new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8) : null; } catch (IOException e) { return null; }
    }

    void end() throws IOException {
        markEnding();
        finish();
    }


    boolean finish() throws IOException {
        if (!file(ENDING).exists()) return false;
        remove(file(STATE));
        File src = file(SOURCES);
        if (src.isFile()) release(src);
        remove(src);
        remove(file(KEPT));
        sync.sync(dir);
        remove(file(ENDING));
        sync.sync(dir);
        return true;
    }


    private void release(File src) {
        String text;
        try { text = new String(Files.readAllBytes(src.toPath()), StandardCharsets.UTF_8); }
        catch (IOException e) { text = ""; }
        for (String line : text.split("\n")) {
            String[] f = line.split("\t", -1);
            if (f.length >= 2 && !f[1].isEmpty()) grants.release(f[1]);
        }
    }


    private static void remove(File f) throws IOException {
        if (f.exists() && !f.delete() && f.exists()) throw new IOException("cannot remove " + f.getName());
    }
}
