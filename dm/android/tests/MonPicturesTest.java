package com.xiaoboluo.d2r;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;



public class MonPicturesTest {
    static int failed;
    static void check(String what, Object got, Object want) {
        if (String.valueOf(got).equals(String.valueOf(want))) return;
        failed++; System.out.println("FAIL " + what + ": got " + got + ", want " + want);
    }
    static void rejects(String what, Map<String, String> pictures, Set<String> none) {
        try { new MonPictures(pictures, none); failed++; System.out.println("FAIL " + what + ": accepted"); } catch (IllegalArgumentException expected) { }
    }

    public static void main(String[] args) {
        Map<String, String> pictures = new HashMap<>();
        pictures.put("boss1", "boss1"); pictures.put("su:7", "boss1"); pictures.put("other", "other_2");
        Set<String> none = new HashSet<>(); none.add("tinted"); none.add("su:8");
        MonPictures p = new MonPictures(pictures, none);
        check("monster's own picture", p.name("boss1"), "boss1");
        check("superunique shares its class's picture", p.name("su:7"), "boss1");
        check("no picture on purpose", p.name("tinted"), null);
        check("standard enemy: empty key, no picture", p.name(""), null);
        check("standard enemy known", p.known(""), true);
        check("look without picture known", p.known("su:8"), true);
        check("look the pack does not know", p.known("new"), false);
        check("no look", p.known(null), false);

        Set<String> files = new HashSet<>();
        for (String n : new String[]{"boss1", "other_2"}) { files.add("mon/" + n + ".webp"); files.add("mon/s/" + n + ".webp"); }
        check("complete", p.missing(files::contains), null);
        files.remove("mon/s/other_2.webp");
        check("small picture missing", p.missing(files::contains), "mon/s/other_2.webp");
        files.remove("mon/boss1.webp");
        check("first missing in name order", p.missing(files::contains), "mon/boss1.webp");
        check("no picture at all is no pack of pictures", new MonPictures(new HashMap<>(), none).missing(f -> true), "mon.json");

        for (String bad : new String[]{"../x", "a/b", "a.webp", "Boss", "", "x".repeat(41)}) {
            Map<String, String> m = new HashMap<>(); m.put("k", bad); rejects("picture name " + bad, m, new HashSet<>());
        }
        Map<String, String> both = new HashMap<>(); both.put("tinted", "t"); rejects("key with and without picture", both, none);

        check("big fits", MonPictures.fits(256, 256, false), true);
        check("big at its limit", MonPictures.fits(512, 512, false), true);
        check("big beyond", MonPictures.fits(513, 200, false), false);
        check("small beyond", MonPictures.fits(256, 256, true), false);
        check("small fits", MonPictures.fits(96, 96, true), true);
        check("empty picture", MonPictures.fits(0, 96, true), false);


        MonCache<int[]> c = new MonCache<>(1000, v -> v[0]);
        c.put("a", new int[]{400}); c.put("b", new int[]{400});
        check("two fit", c.count() + " " + c.bytes(), "2 800");
        c.get("a");
        c.put("c", new int[]{400});
        check("least recently used dropped", (c.get("a") != null) + " " + (c.get("b") != null) + " " + (c.get("c") != null), "true false true");
        check("bytes after dropping", c.bytes(), 800);
        c.put("a", new int[]{100});
        check("replacing a value counts its new size", c.bytes(), 500);
        c.put("huge", new int[]{1001});
        check("a value beyond the limit is not kept", (c.get("huge") == null) + " " + c.bytes(), "true 500");

        for (int i = 0; i < 200; i++) { c.put("p" + i, new int[]{90 + i % 7}); if (c.bytes() > 1000) { failed++; System.out.println("FAIL over the limit at " + i); break; } }
        c.clear();
        check("cleared", c.count() + " " + c.bytes(), "0 0");
        try { new MonCache<int[]>(0, v -> v[0]); failed++; System.out.println("FAIL zero limit accepted"); } catch (IllegalArgumentException expected) { }

        loader();

        if (failed > 0) { System.out.println(failed + " failed"); System.exit(1); }
        System.out.println("MonPicturesTest: look keys, missing files, names, sizes, the byte-limited cache and the loader passed");
    }


    static final class Queue implements java.util.concurrent.Executor {
        final ArrayDeque<Runnable> tasks = new ArrayDeque<>();
        public void execute(Runnable r) { tasks.add(r); }
        void run() { while (!tasks.isEmpty()) tasks.poll().run(); }
    }
    static final class Page implements MonLoader.Listener {
        int loaded; final List<String> failures = new ArrayList<>();
        public void loaded() { loaded++; }
        public void failed(String name, boolean small) { failures.add(MonLoader.key(name, small)); }
    }
    static List<String> keys(String... k) { return Arrays.asList(k); }

    static void loader() {
        Queue worker = new Queue(), ui = new Queue();
        List<String> decoded = new ArrayList<>();
        Page page = new Page();

        MonLoader<int[]> l = new MonLoader<>(1000, v -> v[0], worker, ui, (n, small) -> { decoded.add(MonLoader.key(n, small)); return n.equals("bad") ? null : new int[]{100}; }, page);
        check("keys of the two sizes", MonLoader.key("a", true) + " " + MonLoader.key("a", false), "s/a a");

        l.show(keys("s/a", "s/b", "big"));
        check("asked, nothing loaded yet", l.pending() + " " + (l.get("a", true) == null), "3 true");
        worker.run(); ui.run();
        check("shown pictures loaded", (l.get("a", true) != null) + " " + (l.get("b", true) != null) + " " + (l.get("big", false) != null) + " " + l.pending(), "true true true 0");
        check("the page hears of each", page.loaded, 3);
        l.show(keys("s/a", "s/b", "big"));
        worker.run(); ui.run();
        check("the same pictures again: nothing decoded again", decoded.size(), 3);



        decoded.clear();
        l.show(keys("s/c"));
        worker.run();
        l.show(keys("s/d"));
        ui.run();
        check("stale picture dropped", (l.get("c", true) == null) + " " + l.bytes(), "true 300");
        worker.run(); ui.run();
        check("the page's own picture loaded", l.get("d", true) != null, true);

        l.show(keys("s/e", "s/f"));
        worker.run();
        l.show(keys("s/f", "s/g"));
        ui.run(); worker.run(); ui.run();
        check("a picture the newer page shows too is kept", (l.get("e", true) == null) + " " + (l.get("f", true) != null) + " " + (l.get("g", true) != null), "true true true");


        decoded.clear();
        for (int p = 0; p < 50; p++) l.show(keys("s/p" + p + "a", "s/p" + p + "b", "s/p" + p + "c"));
        worker.run(); ui.run();
        check("only the last page decoded", decoded.toString(), "[s/p49a, s/p49b, s/p49c]");
        check("nothing pending after paging", l.pending(), 0);
        check("the last page shown", (l.get("p49a", true) != null) + " " + (l.get("p48a", true) == null), "true true");
        check("cache within its limit", l.bytes() <= 1000, true);

        decoded.clear();
        l.show(keys("s/x"));
        l.show(keys("s/y"));
        worker.run();
        l.show(keys("s/x"));
        ui.run(); worker.run(); ui.run();
        check("a page shown again is loaded again", (l.get("x", true) != null) + " " + decoded, "true [s/y, s/x]");


        for (int p = 0; p < 30; p++) {
            l.show(keys("s/q" + p, "q" + p));
            worker.run(); ui.run();
            if (l.get("q" + p, true) == null || l.get("q" + p, false) == null || l.bytes() > 1000) { failed++; System.out.println("FAIL eviction at " + p); break; }
        }


        l.show(keys("one"));
        worker.run(); ui.run();
        decoded.clear();
        l.show(keys("two"));
        worker.run(); ui.run();
        check("switched enemy's picture loaded", (l.get("two", false) != null) + " " + decoded, "true [two]");


        decoded.clear();
        l.show(keys("bad"));
        worker.run(); ui.run();
        l.show(keys());
        l.show(keys("bad"));
        worker.run(); ui.run();
        check("unreadable picture reported once, not asked again", page.failures + " " + decoded + " " + l.failed("bad", false), "[bad] [bad] true");

        decoded.clear();
        l.show(keys("s/bad"));
        worker.run();
        l.show(keys("s/other"));
        ui.run(); worker.run(); ui.run();
        l.show(keys("s/bad"));
        worker.run(); ui.run();
        check("failure of a left page reported once, not decoded again", page.failures + " " + decoded + " " + l.failed("bad", true),
            "[bad, s/bad] [s/bad, s/other] true");
        l.show(keys());
        l.show(keys("s/after"));
        worker.run(); ui.run();
        check("after a page without pictures the next picture shows", l.get("after", true) != null, true);


        l.show(keys("s/r1"));
        worker.run();
        l.reset();
        ui.run();
        check("closed page: arrival dropped", (l.get("r1", true) == null) + " " + l.pending(), "true 0");
        l.show(keys("s/r1"));
        worker.run(); ui.run();
        check("opened again: loaded", l.get("r1", true) != null, true);
    }
}
