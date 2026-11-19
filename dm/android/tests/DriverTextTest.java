package com.xiaoboluo.d2r;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Set;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;






public class DriverTextTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    static String read(File f) throws Exception { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8); }

    static Set<String> keys(File f, StringBuilder args) throws Exception {
        Set<String> s = new TreeSet<>();
        Matcher m = Pattern.compile("<string name=\"([A-Za-z0-9_]+)\">([^<]*)</string>").matcher(read(f));
        while (m.find()) { check(s.add(m.group(1)), f.getParentFile().getName() + ": " + m.group(1) + " only once"); args.append(m.group(1)).append(m.group(2).split("%1\\$s", -1).length - 1).append(' '); }
        return s;
    }

    public static void main(String[] a) throws Exception {
        rootPath = a[0];
        File root = new File(a[0]), res = new File(root, "dm/android/app/src/main/res");
        StringBuilder argsEn = new StringBuilder();
        Set<String> en = keys(new File(res, "values/driver.xml"), argsEn);
        check(en.size() >= 10, "values/driver.xml has the keys (" + en.size() + ")");
        for (String v : new String[]{"values-zh", "values-es", "values-b+es+419", "values-ru", "values-ko", "values-de"}) {
            StringBuilder args = new StringBuilder();
            check(keys(new File(res, v + "/driver.xml"), args).equals(en), v + " has the same keys as values");
            check(args.toString().equals(argsEn.toString()), v + " has the same placeholders as values");
        }
        String table = read(new File(root, "dm/i18n/ui_text.tsv")), keep = read(new File(res, "values/keepalive.xml"));
        for (String k : en) check(!table.contains(k + "\t") && !keep.contains("\"" + k + "\""), k + " is neither in ui_text.tsv nor in keepalive.xml");
        Matcher m = Pattern.compile("allowed\\[\\] = \\{([^}]*)\\}").matcher(read(new File(root, "dm/android/vk_driver.c")));
        check(m.find(), "vk_driver.c has the TU_DEBUG list");
        Set<String> nat = new TreeSet<>(); for (String p : m.group(1).split(",")) nat.add(p.trim().replace("\"", ""));
        check(nat.equals(new TreeSet<>(java.util.List.of(DriverStore_TU_DEBUG()))), "TU_DEBUG list: Java " + java.util.List.of(DriverStore_TU_DEBUG()) + " = native " + nat);
        if (fails > 0) { System.out.println(fails + " failure(s)"); System.exit(1); }
        System.out.println("DriverTextTest: all passed");
    }


    static String[] DriverStore_TU_DEBUG() throws Exception {
        Matcher m = Pattern.compile("TU_DEBUG = \\{([^}]*)\\}").matcher(read(new File(root(), "dm/android/app/src/main/java/com/xiaoboluo/d2r/DriverStore.java")));
        if (!m.find()) return new String[0];
        String[] p = m.group(1).split(","); for (int i = 0; i < p.length; i++) p[i] = p[i].trim().replace("\"", ""); return p;
    }
    static String rootPath = ".";
    static File root() { return new File(rootPath); }
}
