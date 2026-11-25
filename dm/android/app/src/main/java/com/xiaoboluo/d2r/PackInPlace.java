package com.xiaoboluo.d2r;

import java.util.List;
import java.util.Map;





final class PackInPlace {
    static final class Where {
        final long dev, ino; final String path;
        Where(long dev, long ino, String path) { this.dev = dev; this.ino = ino; this.path = path == null ? null : canonical(path); }
        boolean same(Where o) { return (dev == o.dev && ino == o.ino) || (path != null && path.equals(o.path)); }
    }

    private PackInPlace() {}


    static String canonical(String p) {
        if (p.startsWith("/sdcard/")) return "/storage/emulated/0/" + p.substring(8);
        if (p.startsWith("/storage/self/primary/")) return "/storage/emulated/0/" + p.substring(22);
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("^/data/media/(\\d+)/(.*)$").matcher(p);
        if (m.matches()) return "/storage/emulated/" + m.group(1) + "/" + m.group(2);
        m = java.util.regex.Pattern.compile("^/mnt/(?:user|pass_through)/\\d+/emulated/(\\d+)/(.*)$").matcher(p);
        if (m.matches()) return "/storage/emulated/" + m.group(1) + "/" + m.group(2);
        return p;
    }


    static String collision(Map<String, Where> inputs, List<Where> outputs) {
        for (Map.Entry<String, Where> in : inputs.entrySet())
            for (Where out : outputs) if (in.getValue().same(out)) return in.getKey();
        return null;
    }
}
