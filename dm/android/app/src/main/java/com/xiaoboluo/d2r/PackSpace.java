package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;




final class PackSpace {
    static final class Check {
        final long need, free, missing;
        Check(long need, long free, long missing) { this.need = need; this.free = free; this.missing = missing; }
        boolean fits() { return missing == 0; }
    }


    static final class Write {
        final long volume, size, replaced;
        Write(long volume, long size, long replaced) {
            if (size < 0 || replaced < 0) throw new IllegalArgumentException("negative size");
            this.volume = volume; this.size = size; this.replaced = replaced;
        }
    }



    static final long STAGE = 750L << 20, WORK = 250L << 20, RUNTIME = 80L << 20, STEPS = STAGE + WORK + RUNTIME;



    static long steps(long runtimePresent, long workPresent, long stagePresent) {
        return Math.max(0, STAGE - stagePresent) + Math.max(0, WORK - workPresent) + Math.max(0, RUNTIME - runtimePresent);
    }

    static final String REWRITE_CACHES = "build/cache/dm/out";



    static List<File> otherCaches(File work, String product) {
        List<File> out = new ArrayList<>();
        File dir = new File(work, REWRITE_CACHES);
        if (!dir.isDirectory()) return out;
        try {
            String root = work.getCanonicalPath() + File.separator;
            for (File f = dir; !f.equals(work); f = f.getParentFile()) {
                if (Files.isSymbolicLink(f.toPath()) || !f.getCanonicalPath().startsWith(root)) {
                    AppArt.log("pack build: the work caches lie behind a link; nothing of them is counted or removed");
                    return out;
                }
            }
        } catch (IOException e) {
            AppArt.log("pack build: the work caches cannot be resolved (" + e.getClass().getSimpleName() + "); nothing of them is counted or removed");
            return out;
        }
        File[] caches = dir.listFiles();
        if (caches != null) for (File c : caches) if (!c.getName().startsWith(product + "-")) out.add(c);
        return out;
    }


    static long reclaimable(File work, String product) {
        long n = 0;
        for (File c : otherCaches(work, product)) n += size(c);
        return n;
    }


    static Check check(List<List<Write>> units, Map<Long, Long> free, long volume, long reclaim) {
        if (reclaim <= 0 || !free.containsKey(volume)) return check(units, free);
        Map<Long, Long> more = new HashMap<>(free);
        more.merge(volume, reclaim, Long::sum);
        return check(units, more);
    }


    static long workCredit(File work, String product) {
        long n = size(work);
        for (File c : otherCaches(work, product)) n -= size(c);
        return Math.max(0, n);
    }


    static void pruneWork(File work, String product) throws IOException {
        for (File c : otherCaches(work, product)) PackInstall.deleteTree(c);
    }

    static long size(File f) {
        if (Files.isSymbolicLink(f.toPath())) return 0;
        if (f.isFile()) return f.length();
        long n = 0;
        File[] kids = f.listFiles();
        if (kids != null) for (File k : kids) n += size(k);
        return n;
    }


    static final long LOW = 100L << 20;

    private PackSpace() {}




    static Check check(List<List<Write>> units, Map<Long, Long> free) {
        Map<Long, long[]> volumes = new HashMap<>();
        for (Map.Entry<Long, Long> f : free.entrySet()) volumes.put(f.getKey(), new long[]{0, 0});
        for (List<Write> unit : units) {
            Map<Long, Long> current = new HashMap<>();
            for (Write w : unit) current.merge(w.volume, w.size, Long::sum);
            for (Map.Entry<Long, Long> c : current.entrySet()) {
                long[] v = volumes.get(c.getKey());
                if (v == null) throw new IllegalArgumentException("no free space known for volume " + c.getKey());
                v[0] = Math.max(v[0], v[1] + c.getValue());
            }
            for (Write w : unit) volumes.get(w.volume)[1] += w.size - w.replaced;
        }
        Map<Long, long[]> needs = new HashMap<>();
        for (Map.Entry<Long, long[]> v : volumes.entrySet()) needs.put(v.getKey(), new long[]{v.getValue()[0], free.get(v.getKey())});
        return check(needs);
    }


    static Check check(Map<Long, long[]> volumes) {
        if (volumes.isEmpty()) throw new IllegalArgumentException("no volume to check");
        long need = 0, tightestFree = Long.MAX_VALUE, tightestMargin = Long.MAX_VALUE, missing = 0;
        for (long[] v : volumes.values()) {
            if (v[0] < 0 || v[1] < 0) throw new IllegalArgumentException("negative space");
            need += v[0];
            long margin = v[1] - v[0];
            if (margin < tightestMargin) { tightestMargin = margin; tightestFree = v[1]; }
            if (margin < 0) missing = Math.max(missing, -margin);
        }
        return new Check(need, tightestFree, missing);
    }



    static Long missing(String detail) {
        return detail != null && detail.matches("\\d{1,18}") ? Long.valueOf(detail) : null;
    }


    static String size(long bytes, Locale locale) {
        if (bytes < 1_000_000_000L) return String.format(locale, "%d MB", Math.max(1, (bytes + 999_999) / 1_000_000));
        return String.format(locale, "%.1f GB", bytes / 1e9);
    }
}
