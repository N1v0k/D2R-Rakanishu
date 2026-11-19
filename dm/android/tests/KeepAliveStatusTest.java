package com.xiaoboluo.d2r;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;





public class KeepAliveStatusTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    static Set<String> keys(File f) throws Exception {
        Set<String> s = new TreeSet<>();
        Matcher m = Pattern.compile("<string name=\"([A-Za-z0-9_]+)\"").matcher(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8));
        while (m.find()) check(s.add(m.group(1)), f.getParentFile().getName() + ": " + m.group(1) + " only once");
        return s;
    }

    public static void main(String[] a) throws Exception {
        int S = KeepAliveStatus.OK;
        check(KeepAliveStatus.of(true, true, true, false, false) == S, "everything fine -> ok");
        check(KeepAliveStatus.of(true, false, true, false, false) == KeepAliveStatus.NO_NOTIFY, "notifications hidden -> notifications off");
        check(KeepAliveStatus.of(true, false, false, false, false) == KeepAliveStatus.BATTERY, "battery beats notifications");
        check(KeepAliveStatus.of(true, false, false, true, false) == KeepAliveStatus.RESTRICTED, "restricted beats battery");
        check(KeepAliveStatus.of(false, true, true, true, true) == KeepAliveStatus.RESTRICTED, "restricted explains a refusal (Android stops the service in the background)");
        check(KeepAliveStatus.of(true, true, false, false, true) == KeepAliveStatus.FAILED, "refused beats battery");
        check(KeepAliveStatus.of(false, true, true, false, false) == KeepAliveStatus.FAILED, "not a foreground service -> failed");
        check(KeepAliveStatus.colour(KeepAliveStatus.OK) == 0xFFADA894 && KeepAliveStatus.colour(KeepAliveStatus.NO_NOTIFY) == 0xFFADA894, "ok and notifications off in the note colour");
        check(KeepAliveStatus.colour(KeepAliveStatus.BATTERY) == 0xFFFC4646 && KeepAliveStatus.colour(KeepAliveStatus.RESTRICTED) == 0xFFFC4646
            && KeepAliveStatus.colour(KeepAliveStatus.FAILED) == 0xFFFC4646, "warnings in red");
        Set<String> names = new TreeSet<>();
        for (int s = KeepAliveStatus.OK; s <= KeepAliveStatus.FAILED; s++) check(names.add(KeepAliveStatus.name(s)) && KeepAliveStatus.name(s).matches("[a-z_]+"), "name " + KeepAliveStatus.name(s));

        if (a.length > 0) {
            File res = new File(a[0], "dm/android/app/src/main/res");
            String[] sets = {"values", "values-de", "values-zh", "values-es", "values-b+es+419", "values-ru", "values-ko"};
            Set<String> first = null;
            for (String set : sets) {
                File f = new File(res, set + "/keepalive.xml");
                check(f.isFile(), set + "/keepalive.xml exists");
                if (!f.isFile()) continue;
                Set<String> k = keys(f);
                if (first == null) { first = k; check(k.size() == 14, "14 keys in " + set); }
                else check(k.equals(first), set + " has the same keys as values");
            }
            List<String> table = new ArrayList<>();
            for (String line : Files.readAllLines(new File(a[0], "dm/i18n/ui_text.tsv").toPath(), StandardCharsets.UTF_8)) table.add(line.split("\t", 2)[0]);
            if (first != null) for (String k : first) check(!table.contains(k), k + " not in ui_text.tsv");
            File[] other = res.listFiles();
            if (other != null && first != null) for (File dir : other) {
                File[] files = dir.listFiles();
                if (files == null || !dir.getName().startsWith("values")) continue;
                for (File f : files) if (!f.getName().equals("keepalive.xml") && f.getName().endsWith(".xml")) for (String k : keys(f)) check(!first.contains(k), k + " not also in " + dir.getName() + "/" + f.getName());
            }
        }
        System.out.println(fails == 0 ? "KeepAliveStatusTest: all passed" : "KeepAliveStatusTest: " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
