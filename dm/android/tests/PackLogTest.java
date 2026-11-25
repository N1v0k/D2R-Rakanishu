package com.xiaoboluo.d2r;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.List;



public class PackLogTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    public static void main(String[] args) throws Exception {
        File dir = new File(Files.createTempDirectory("packlog").toFile(), "dm-build");
        check(PackLog.assumptions(dir) == 0, "no log, no assumption");
        PackLog log = new PackLog(dir, () -> 1_700_000_000_000L);
        log.line("pack build: step art done after 17.0 s");
        log.line("ASSUME packbuild.sweep: the process table cannot be read (IOException)");
        log.line("two\nlines");
        List<String> lines = Files.readAllLines(new File(dir, PackLog.FILE).toPath(), StandardCharsets.UTF_8);
        check(lines.size() == 3 && lines.get(0).matches("\\d{4}-\\d\\d-\\d\\dT\\d\\d:\\d\\d:\\d\\d\\.\\d{3} pack build: step art done after 17\\.0 s"), "a timed line: " + lines);
        check(lines.get(2).endsWith("two lines"), "a line break is not a new line: " + lines.get(2));
        check(PackLog.assumptions(dir) == 1, "the assumption line is counted");

        String filler = "x".repeat(1000);
        for (long n = 0; n * 1000 < PackLog.MAX + 2000; n++) log.line(filler);
        check(new File(dir, PackLog.FILE + ".1").isFile() && new File(dir, PackLog.FILE).length() < PackLog.MAX, "rotated at the limit");
        check(PackLog.assumptions(dir) == 1, "the assumption survives the rotation");
        log.line("ASSUME packbuild.shaderkeys: 3 bundles written, 4 expected");
        check(PackLog.assumptions(dir) == 2, "both files are counted");
        for (long n = 0; n * 1000 < 2 * PackLog.MAX + 4000; n++) log.line(filler);
        check(PackLog.assumptions(dir) == 0, "after two more rotations the old assumptions are gone with their file");
        log.sync();
        PackLog.tee("nothing happens without a log of this process");
        System.out.println("PackLogTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
