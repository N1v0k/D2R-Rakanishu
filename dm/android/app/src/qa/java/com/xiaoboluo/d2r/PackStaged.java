package com.xiaoboluo.d2r;

import android.content.Context;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;






final class PackStaged {
    private PackStaged() {}

    static File prepared(Context c) {
        File f = new File(PackBuildService.dir(c), "qa-stage-pack");
        if (!f.isFile()) return null;
        try { return new File(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8).trim()); }
        catch (IOException e) { throw new IllegalStateException("qa-stage-pack unreadable", e); }
    }


    static String[] stage(String path) {
        if (path.startsWith("out/")) return new String[]{"rewrite", path};
        if (path.equals("derived.tsv")) return new String[]{"art", path};
        if (path.startsWith("overlay/") || path.startsWith("overlay_variants/")) return new String[]{"overlay", path};
        if (path.equals("appart/mon.json") || path.startsWith("appart/mon/")) return new String[]{"pictures", "monpics/" + path.substring("appart/".length())};
        if (path.startsWith("appart/")) return new String[]{"app", path};
        if (path.startsWith("shaders/")) return new String[]{"shaders", path};
        return null;
    }

    static PackSteps steps(Context c, File pack, File stage) throws IOException {
        Map<String, TreeMap<String, File>> files = new TreeMap<>();
        for (String s : PackSteps.all()) files.put(s, new TreeMap<>());
        walk(pack, "", (rel, f) -> {
            String[] s = stage(rel);
            if (s != null) files.get(s[0]).put(s[1], f);
        });
        StringBuilder expect = new StringBuilder();
        for (Map.Entry<String, TreeMap<String, File>> e : files.entrySet()) {
            if (e.getValue().isEmpty()) throw new IOException("the prepared pack has nothing of step " + e.getKey());
            expect.append(e.getKey()).append('\t').append(e.getValue().size()).append('\t').append(e.getValue().size()).append('\t')
                .append(PackGeneration.pathDigest(new ArrayList<>(e.getValue().keySet()))).append('\n');
        }
        return new PackSteps(PackSteps.units(PackSteps.all(), expect.toString()), stage, "qa-staged", names -> session(c, names, files, stage),
            PackBuildService.STAMP, why -> true);
    }

    private interface Visit { void at(String rel, File f) throws IOException; }

    private static void walk(File f, String rel, Visit v) throws IOException {
        if (f.isFile()) { v.at(rel, f); return; }
        String[] names = f.list();
        if (names == null) return;
        java.util.Arrays.sort(names);
        for (String n : names) walk(new File(f, n), rel.isEmpty() ? n : rel + "/" + n, v);
    }

    private static PackSteps.Session session(Context c, List<String> names, Map<String, TreeMap<String, File>> files, File stage) {
        Deque<String> todo = new ArrayDeque<>(names);
        Deque<String> lines = new ArrayDeque<>();
        return new PackSteps.Session() {
            volatile boolean cancelled;
            @Override public String readLine() throws IOException {
                while (lines.isEmpty()) {
                    if (cancelled || todo.isEmpty()) return null;
                    String step = todo.poll();
                    TestHooks.packHold(c, "step:" + step);
                    lines.add("start\t" + step + "\t1");
                    File fail = new File(PackBuildService.dir(c), "qa-stage-fail");
                    if (fail.isFile() && new String(Files.readAllBytes(fail.toPath()), StandardCharsets.UTF_8).trim().equals(step)) {
                        if (!fail.delete()) throw new IOException("qa-stage-fail cannot be removed");
                        AppArt.log("ASSUME packbuild.staged: the qa step " + step + " was made to fail");
                        lines.add("error\t" + step + "\tPB-STEP-" + step + "\tqa: made to fail");
                        todo.clear();
                        continue;
                    }
                    lines.add("done\t" + step + "\t" + copy(step, files.get(step), stage));
                }
                return lines.poll();
            }
            @Override public int finish() { return 0; }
            @Override public void cancel() { cancelled = true; }
        };
    }


    private static String copy(String step, TreeMap<String, File> files, File stage) throws IOException {
        StringBuilder m = new StringBuilder();
        byte[] buf = new byte[1 << 16];
        for (Map.Entry<String, File> e : files.entrySet()) {
            File to = new File(stage, e.getKey());
            File dir = to.getParentFile();
            if (!dir.isDirectory() && !dir.mkdirs()) throw new IOException("cannot create " + dir.getName());
            try (InputStream in = new FileInputStream(e.getValue()); OutputStream out = new FileOutputStream(to)) {
                for (int n; (n = in.read(buf)) > 0; ) out.write(buf, 0, n);
            }
            m.append(e.getKey()).append('\t').append(to.length()).append('\t').append(DriverLibrary.sha256(to)).append('\n');
        }
        File units = new File(stage, ".units");
        if (!units.isDirectory() && !units.mkdirs()) throw new IOException("cannot create the manifest folder");
        DriverLibrary.writeFile(new File(units, step + ".tsv"), m.toString(), PackBuildService.SYNC);
        return DriverLibrary.sha256Bytes(m.toString().getBytes(StandardCharsets.UTF_8));
    }
}
