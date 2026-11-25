package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;



public class PackSessionTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    static final class Killed extends RuntimeException { private static final long serialVersionUID = 1; Killed(String at) { super(at); } }


    static final class Sync implements DriverLibrary.Sync {
        int n, calls;
        Sync(int n) { this.n = n; }
        public void sync(File dir) { calls++; if (calls == n) throw new Killed("sync " + calls); }
    }

    static String kind(File dir) {
        PackSession s = new PackSession(dir, d -> {}, u -> {});
        return !s.active() ? "none" : s.kept() ? "kept" : "sources";
    }

    static void write(File f, String text) throws IOException { f.getParentFile().mkdirs(); Files.write(f.toPath(), text.getBytes(StandardCharsets.UTF_8)); }

    static final String SOURCES = "base\tcontent://a/base\tid1\nupdate\tcontent://a/update\tid2\nkeys\tcontent://a/keys\n";

    public static void main(String[] args) throws Exception {
        File tmp = Files.createTempDirectory("packsession").toFile();


        for (int n = 0; n <= 3; n++) {
            File d = new File(tmp, "start" + n);
            write(new File(d, PackSession.STATE), "phase\terror\npercent\t40\nerror\tPB-IO\ndetail\t\n");
            try { new PackSession(d, new Sync(n), u -> {}).startKept(); } catch (Killed k) {                }
            String k = kind(d);
            check(k.equals(n == 0 ? "kept" : k) && (k.equals("none") || k.equals("kept")), "start kept, killed at sync " + n + ": " + k);
            if (k.equals("kept")) check(!new File(d, PackSession.STATE).exists(), "a rebuild never shows an old state (kill " + n + ")");
        }



        for (int n = 0; n <= 3; n++) {
            File d = new File(tmp, "over" + n);
            write(new File(d, PackSession.SOURCES), SOURCES);
            write(new File(d, PackSession.STATE), "phase\terror\npercent\t20\nerror\tPB-SOURCE\ndetail\tkeys\n");
            List<String> released = new ArrayList<>();
            try { new PackSession(d, new Sync(n), released::add).startKept(); } catch (Killed k) {                }
            String k = kind(d);
            check(k.equals("none") || k.equals("kept"), "start kept over sources, killed at sync " + n + ": " + k);
            check(n != 0 || k.equals("kept") && released.size() == 3, "start kept over sources: the rebuild, the source grants let go: " + released);
        }


        File ks = new File(tmp, "convert");
        new PackSession(ks, d -> {}, u -> {}).startKept();
        DriverLibrary.writeFile(new File(ks, PackSession.SOURCES), SOURCES, d -> {});
        check(kind(ks).equals("sources"), "killed between the two writes: the sources win");
        new PackSession(ks, d -> {}, u -> {}).keptDone();
        check(kind(ks).equals("sources") && !new File(ks, PackSession.KEPT).exists(), "then the marker is gone");
        File kk = new File(tmp, "convert-killed");
        new PackSession(kk, d -> {}, u -> {}).startKept();
        DriverLibrary.writeFile(new File(kk, PackSession.SOURCES), SOURCES, d -> {});
        try { new PackSession(kk, new Sync(1), u -> {}).keptDone(); } catch (Killed k) {                                           }
        check(kind(kk).equals("sources"), "killed after the marker's removal: sources");


        for (String start : new String[]{"sources", "kept", "both"}) {
            for (int n = 0; n <= 6; n++) {
                File d = new File(tmp, "end-" + start + n);
                write(new File(d, PackSession.STATE), "phase\tdone\npercent\t100\n");
                if (!start.equals("kept")) write(new File(d, PackSession.SOURCES), SOURCES);
                if (!start.equals("sources")) write(new File(d, PackSession.KEPT), "1\n");
                List<String> released = new ArrayList<>();
                boolean killed = false;
                try { new PackSession(d, new Sync(n), released::add).end(); } catch (Killed k) { killed = true; }
                if (killed) check(new File(d, PackSession.ENDING).exists() || !new File(d, PackSession.SOURCES).exists() && !new File(d, PackSession.KEPT).exists(),
                    "an end killed at sync " + n + " (" + start + ") leaves its mark or nothing");
                check(kind(d).equals("none"), "an end killed at sync " + n + " (" + start + ") never reopens: " + kind(d));
                PackSession next = new PackSession(d, x -> {}, released::add);
                next.finish();
                check(!new File(d, PackSession.STATE).exists() && !new File(d, PackSession.SOURCES).exists() && !new File(d, PackSession.KEPT).exists()
                    && !new File(d, PackSession.ENDING).exists(), "the next start completes the end (" + start + ", kill " + n + ")");
                if (!start.equals("kept")) check(released.containsAll(List.of("content://a/base", "content://a/update", "content://a/keys")), "the source grants are let go (" + start + ", " + n + "): " + released);
                next.end();
                check(kind(d).equals("none") && !new File(d, PackSession.ENDING).exists(), "a second end changes nothing");
            }
        }


        File none = new File(tmp, "none");
        none.mkdirs();
        new PackSession(none, d -> {}, u -> {}).markEnding();
        check(!new File(none, PackSession.ENDING).exists(), "no session: no mark");


        File live = new File(tmp, "live");
        write(new File(live, PackSession.SOURCES), SOURCES);
        PackSession s = new PackSession(live, d -> {}, u -> {});
        check(!s.finish() && s.active() && !s.kept(), "a sources session without a state file goes on");


        File gone = new File(tmp, "gone");
        write(new File(gone, PackSession.ENDING), "1\n");
        check(new PackSession(gone, d -> {}, u -> {}).finish() && !new File(gone, PackSession.ENDING).exists(), "a mark alone is completed");

        PackJournalTest.clean(tmp);
        System.out.println("PackSessionTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
