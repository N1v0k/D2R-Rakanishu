package com.xiaoboluo.d2r;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.regex.Matcher;
import java.util.regex.Pattern;



public class BestGearStoreTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }
    static boolean is(int[] g, int hits, int boss) { return g != null && g.length == 2 && g[0] == hits && g[1] == boss; }

    public static void main(String[] a) throws Exception {
        check(is(BestGearModel.decode(BestGearModel.encode(20, 30)), 20, 30), "codec round trip");
        check(BestGearModel.decode(null) == null && BestGearModel.decode("") == null && BestGearModel.decode("15") == null && BestGearModel.decode("15,50,1") == null
              && BestGearModel.decode("x,50") == null && BestGearModel.decode("15,") == null, "broken stored values are not goals");
        check(BestGearModel.decode("-1,50") == null && BestGearModel.decode("201,50") == null && BestGearModel.decode("15,101") == null && is(BestGearModel.decode("0,100"), 0, 100),
              "stored ranges: hits 0..200, boss 0..100");
        check(is(BestGearModel.fromGuide(12, 0.6), 12, 60) && is(BestGearModel.fromGuide(0, 0.0), 0, 0) && is(BestGearModel.fromGuide(20, 0.355), 20, 36), "guide values in percent");
        check(BestGearModel.fromGuide(null, 0.5) == null && BestGearModel.fromGuide(15, null) == null && BestGearModel.fromGuide(15, 1.5) == null
              && BestGearModel.fromGuide(-3, 0.5) == null && BestGearModel.fromGuide(15, Double.NaN) == null, "a guide without valid values gives none");
        check(is(BestGearModel.pick(null, null), 15, 50), "no guide: 15 hits, 50 %");
        check(is(BestGearModel.pick(null, new int[] { 12, 60 }), 12, 60), "the guide's values without an own one");
        check(is(BestGearModel.pick(new int[] { 30, 10 }, new int[] { 12, 60 }), 30, 10), "the character's own value first");
        check(BestGearModel.key("Hero", 7).equals("Hero#7"), "key name#class like the build guides");

        File dir = new File(a[0], "dm/android/builds");
        Pattern p = Pattern.compile("\\n  \"profile\": \\{\\n    \"bestgear\": \\{\"survivalHits\": (\\d+), \"bossShare\": ([0-9.]+)\\}");
        int guides = 0;
        for (File f : dir.listFiles()) {
            if (!f.getName().endsWith(".json") || f.getName().equals("schema.json")) continue;
            Matcher m = p.matcher(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8));
            check(m.find() && BestGearModel.fromGuide(Integer.parseInt(m.group(1)), Double.parseDouble(m.group(2))) != null, f.getName() + " has a valid goal");
            guides++;
        }
        check(guides > 20, "build guides found: " + guides);
        if (fails > 0) { System.out.println(fails + " failed"); System.exit(1); }
    }
}
