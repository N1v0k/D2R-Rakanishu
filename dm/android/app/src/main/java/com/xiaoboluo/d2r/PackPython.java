package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.res.AssetManager;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import org.json.JSONException;
import org.json.JSONObject;












final class PackPython {
    private static final String ASSETS = "py";
    private static final long CANCEL_GRACE_MS = 3000;

    private PackPython() {}




    static PackSteps steps(Context c, List<String> names, File exefs, File romfs, String inputs) throws IOException {
        return steps(c, names, exefs, romfs, inputs, plan(c));
    }


    static PackPlan plan(Context c) {
        android.app.ActivityManager.MemoryInfo m = new android.app.ActivityManager.MemoryInfo();
        c.getSystemService(android.app.ActivityManager.class).getMemoryInfo(m);
        PackPlan p = TestHooks.packPlan(c, PackPlan.decide(Runtime.getRuntime().availableProcessors(), m.availMem));
        AppArt.log("pack build: " + Runtime.getRuntime().availableProcessors() + " cores, " + (m.availMem >> 20) + " MiB available: " + p);
        return p;
    }

    static PackSteps steps(Context c, List<String> names, File exefs, File romfs, String inputs, PackPlan plan) throws IOException {
        if (names.isEmpty()) return PackSteps.none();
        AssetManager assets = c.getAssets();
        File lib = new File(c.getApplicationInfo().nativeLibraryDir);
        File launcher = new File(lib, "libpylauncher.so"), helper = new File(lib, "libdeswizzle.so");
        File xlate = new File(lib, "libdmsh_xlate.so"), workers = new File(lib, "libdmsh_shaders.so"), monpic = new File(lib, "libmonpic.so");
        boolean shaders = names.contains(PackSteps.SHADERS), pictures = names.contains(PackSteps.PICTURES);
        String version;
        try (InputStream in = assets.open(ASSETS + "/VERSION")) { version = new String(in.readAllBytes(), StandardCharsets.UTF_8); }
        catch (FileNotFoundException e) { version = null; }
        if (version == null || !launcher.isFile() || !helper.isFile() || (shaders && !(xlate.isFile() && workers.isFile())) || (pictures && !monpic.isFile())) {
            AppArt.log("pack build: this APK carries no Python runtime, shader or picture libraries; the steps after the extraction cannot run");
            return PackSteps.unavailable(names);
        }
        String runtimeId = field(version, "runtime"), toolsId = field(version, "tools");
        File home = new File(c.getFilesDir(), "py/" + runtimeId);
        unpackOnce(assets, ASSETS + "/runtime.zip", home);
        File work = new File(PackBuildService.dir(c), "work"), stage = new File(PackBuildService.dir(c), "stage"), tree = new File(work, "tree");
        File tmp = new File(work, "tmp");
        for (File d : new File[]{work, stage, tmp}) if (!d.isDirectory() && !d.mkdirs()) throw new IOException("cannot create " + d.getName());
        unpackTree(assets, ASSETS + "/tools.zip", tree, toolsId);
        File queries = new File(work, "queries");
        Map<String, String> deps = new java.util.HashMap<>();
        if (shaders) {
            String queriesId = field(version, "queries");
            unpackTree(assets, ASSETS + "/queries.zip", queries, queriesId);
            deps.put(PackSteps.SHADERS, "xlate=" + DriverLibrary.sha256(xlate) + "\nworkers=" + DriverLibrary.sha256(workers) + "\nqueries=" + queriesId);
        }
        if (pictures) deps.put(PackSteps.PICTURES, "monpic=" + DriverLibrary.sha256(monpic));
        String expect = new String(Files.readAllBytes(new File(tree, "dm/tools/pack_steps.tsv").toPath()), StandardCharsets.UTF_8);
        String base = "runtime=" + runtimeId + "\ntools=" + toolsId + "\nhelper=" + DriverLibrary.sha256(helper) + "\ninputs=" + inputs + "\ntarget=android";
        String build;
        try {
            build = new JSONObject().put("dm", new File(tree, "dm").getPath()).put("exefs", exefs.getPath()).put("romfs", romfs.getPath())
                .put("stage", stage.getPath()).put("work", work.getPath()).put("python", launcher.getPath()).put("target", "android")
                .put("deswizzle", helper.getPath()).put("qa_combat", false)
                .put("xlate", shaders ? xlate.getPath() : JSONObject.NULL).put("shader_lib", shaders ? workers.getPath() : JSONObject.NULL)
                .put("queries", shaders ? queries.getPath() : JSONObject.NULL).put("monpic", pictures ? monpic.getPath() : JSONObject.NULL)
                .put("tokens", plan.tokens).put("shader_workers", plan.shaderWorkers).put("picture_threads", plan.pictureThreads).put("mode", plan.mode).toString();
        } catch (JSONException e) { throw new IllegalStateException(e); }
        File runner = new File(tree, "dm/tools/pack_steps.py");
        return new PackSteps(PackSteps.units(names, expect, deps), stage, base, steps -> {
            List<String> cmd = new ArrayList<>(List.of(launcher.getPath(), "-X", "utf8", runner.getPath(), "run", build));
            cmd.addAll(steps);
            File logs = new File(work, "build/pack_steps");
            if (!logs.isDirectory() && !logs.mkdirs()) throw new IOException("cannot create the step log folder");
            ProcessBuilder pb = new ProcessBuilder(cmd).directory(work).redirectError(new File(logs, "runner.log"));
            Map<String, String> env = pb.environment();
            env.put("DM_PY_HOME", home.getPath());
            env.put("LD_LIBRARY_PATH", lib.getPath());
            env.put("TMPDIR", tmp.getPath());
            env.put("HOME", work.getPath());
            return new Runner(pb.start());
        }, PackBuildService.STAMP, why -> PackSweep.sweep(processes(android.os.Process.myUid()), why));
    }


    private static PackSweep.Table processes(int uid) {
        return new PackSweep.Table() {
            @Override public List<PackSweep.Proc> launchers() throws IOException {
                File[] dirs = new File("/proc").listFiles();
                if (dirs == null) throw new IOException("/proc cannot be listed");
                List<PackSweep.Proc> out = new ArrayList<>();
                for (File d : dirs) {
                    String n = d.getName();
                    if (n.isEmpty() || !Character.isDigit(n.charAt(0))) continue;
                    try {
                        PackSweep.Proc p = PackSweep.launcher(Integer.parseInt(n), PackSweep.read(new File(d, "status")), PackSweep.read(new File(d, "cmdline")),
                            PackSweep.read(new File(d, "stat")), uid);
                        if (p != null) out.add(p);
                    } catch (IOException | RuntimeException gone) {                              }
                }
                return out;
            }
            @Override public int ownGroup() throws IOException { return PackSweep.group(PackSweep.read(new File("/proc/self/stat"))); }
            @Override public void kill(int pid) { signal(pid); }
            @Override public void killGroup(int group) { signal(-group); }
            @Override public void pause(long ms) { android.os.SystemClock.sleep(ms); }
            @Override public long now() { return android.os.SystemClock.uptimeMillis(); }
        };
    }

    private static void signal(int target) {
        try { Os.kill(target, OsConstants.SIGKILL); } catch (ErrnoException gone) {                     }
    }

    private static String field(String version, String key) {
        for (String line : version.split("\n")) if (line.startsWith(key + "=")) return line.substring(key.length() + 1).trim();
        throw new IllegalStateException("assets/py/VERSION has no " + key);
    }



    private static final class Runner implements PackSteps.Session {
        private final Process p;
        private final BufferedReader out;
        private final Set<Integer> groups = ConcurrentHashMap.newKeySet();
        private final Map<String, Integer> byStep = new ConcurrentHashMap<>();

        Runner(Process p) { this.p = p; this.out = new BufferedReader(new InputStreamReader(p.getInputStream(), StandardCharsets.UTF_8)); }

        @Override public String readLine() throws IOException {
            String line = out.readLine();
            if (line == null) return null;
            String[] f = line.split("\t", -1);
            if (f.length == 3 && f[0].equals("start") && f[2].matches("[1-9][0-9]{0,9}")) {
                int g = Integer.parseInt(f[2]);
                groups.add(g);
                byStep.put(f[1], g);
            } else if (f.length >= 2 && (f[0].equals("done") || f[0].equals("error"))) {
                Integer g = byStep.remove(f[1]);
                if (g != null) groups.remove(g);
            }
            return line;
        }

        @Override public int finish() throws IOException {
            try { return p.waitFor(); }
            catch (InterruptedException e) { Thread.currentThread().interrupt(); throw new IOException("interrupted while the step runner ends", e); }
        }

        @Override public void cancel() {
            try { p.getOutputStream().close(); } catch (IOException e) { AppArt.log("pack build: the step runner's input did not close (" + e.getClass().getSimpleName() + ")"); }
            Thread t = new Thread(() -> {
                try { if (p.waitFor(CANCEL_GRACE_MS, TimeUnit.MILLISECONDS)) return; }
                catch (InterruptedException e) { Thread.currentThread().interrupt(); }
                AppArt.log("pack build: the step runner did not end after its input closed; killing it");
                for (int g : groups) {
                    try { Os.kill(-g, OsConstants.SIGKILL); } catch (ErrnoException gone) {                                   }
                }
                p.destroyForcibly();
            }, "dm-packbuild-cancel");
            t.setDaemon(true);
            t.start();
        }
    }


    private static void unpackOnce(AssetManager assets, String zip, File home) throws IOException {
        File parent = home.getParentFile();
        if (!parent.isDirectory() && !parent.mkdirs()) throw new IOException("cannot create the runtime folder");
        File[] old = parent.listFiles();
        if (old != null) for (File f : old) if (!f.getName().equals(home.getName())) deleteTree(f);
        if (home.isDirectory()) return;
        File part = new File(parent, home.getName() + ".part");
        unzip(assets, zip, part);
        if (!part.renameTo(home)) throw new IOException("cannot move the runtime into place");
        PackBuildService.SYNC.sync(parent);
    }



    private static void unpackTree(AssetManager assets, String zip, File tree, String toolsId) throws IOException {
        File marker = new File(tree.getParentFile(), tree.getName() + ".version");
        if (tree.isDirectory() && marker.isFile() && new String(Files.readAllBytes(marker.toPath()), StandardCharsets.UTF_8).equals(toolsId)) return;
        if (marker.exists() && !marker.delete()) throw new IOException("cannot remove " + marker.getName());
        deleteTree(tree);
        unzip(assets, zip, tree);
        DriverLibrary.writeFile(marker, toolsId, PackBuildService.SYNC);
    }

    private static void unzip(AssetManager assets, String zip, File dir) throws IOException {
        deleteTree(dir);
        String root = dir.getCanonicalPath() + File.separator;
        byte[] buf = new byte[1 << 16];
        try (ZipInputStream z = new ZipInputStream(assets.open(zip))) {
            for (ZipEntry e; (e = z.getNextEntry()) != null; ) {
                File f = new File(dir, e.getName());
                if (!f.getCanonicalPath().startsWith(root)) throw new IOException("entry outside the folder in " + zip + ": " + e.getName());
                if (e.isDirectory()) { if (!f.isDirectory() && !f.mkdirs()) throw new IOException("cannot create " + e.getName()); continue; }
                File parent = f.getParentFile();
                if (!parent.isDirectory() && !parent.mkdirs()) throw new IOException("cannot create the folder of " + e.getName());
                try (FileOutputStream o = new FileOutputStream(f)) {
                    for (int n; (n = z.read(buf)) > 0; ) o.write(buf, 0, n);
                    o.getFD().sync();
                }
            }
        }
    }

    private static void deleteTree(File f) throws IOException {
        if (!f.exists()) return;
        File[] kids = f.isDirectory() && !Files.isSymbolicLink(f.toPath()) ? f.listFiles() : null;
        if (kids != null) for (File k : kids) deleteTree(k);
        if (!f.delete()) throw new IOException("cannot remove " + f.getName());
    }
}
