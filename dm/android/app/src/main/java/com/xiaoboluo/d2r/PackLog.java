package com.xiaoboluo.d2r;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;






final class PackLog {
    static final String FILE = "build.log";
    static final long MAX = 1 << 20;
    static final String ASSUME = "ASSUME packbuild.";

    private static volatile PackLog current;

    private final File file;
    private final java.util.function.LongSupplier clock;
    private FileOutputStream out;

    PackLog(File dir, java.util.function.LongSupplier clock) { this.file = new File(dir, FILE); this.clock = clock; }


    static void open(File dir) {
        if (current == null) current = new PackLog(dir, System::currentTimeMillis);
    }


    static void tee(String line) {
        PackLog l = current;
        if (l != null) l.line(line);
    }

    static void flush() {
        PackLog l = current;
        if (l != null) l.sync();
    }

    synchronized void line(String text) {
        try {
            if (out == null || file.length() >= MAX) reopen();
            String time = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSS", Locale.ROOT).format(new Date(clock.getAsLong()));
            out.write((time + " " + text.replace('\n', ' ') + "\n").getBytes(StandardCharsets.UTF_8));
            if (text.startsWith(ASSUME)) out.getFD().sync();
        } catch (IOException e) {
            System.err.println("pack build: build.log cannot be written (" + e.getClass().getSimpleName() + ")");
            close();
        }
    }

    synchronized void sync() {
        try { if (out != null) out.getFD().sync(); }
        catch (IOException e) { System.err.println("pack build: build.log cannot be synced (" + e.getClass().getSimpleName() + ")"); }
    }

    private void reopen() throws IOException {
        close();
        File dir = file.getParentFile();
        if (!dir.isDirectory() && !dir.mkdirs()) throw new IOException("cannot create " + dir.getName());
        if (file.length() >= MAX) {
            File old = new File(dir, FILE + ".1");
            if (!file.renameTo(old)) throw new IOException("cannot rotate " + FILE);
        }
        out = new FileOutputStream(file, true);
    }

    private void close() {
        if (out == null) return;
        try { out.close(); } catch (IOException ignored) {                                    }
        out = null;
    }


    static int assumptions(File dir) {
        int n = 0;
        for (String name : new String[]{FILE + ".1", FILE}) {
            File f = new File(dir, name);
            if (!f.isFile()) continue;
            try (BufferedReader r = new BufferedReader(new InputStreamReader(new FileInputStream(f), StandardCharsets.UTF_8))) {
                for (String line; (line = r.readLine()) != null; ) if (line.contains(" " + ASSUME)) n++;
            } catch (IOException e) { n++; }
        }
        return n;
    }
}
