package com.xiaoboluo.d2r;

import java.util.HashSet;
import java.util.Set;









final class PackSource {
    enum Kind { PHONE, PC }
    enum Action { START, IMPORT, VERSION_ERROR, GAME_FILES }

    static final String FILE = "pack_source.tsv";


    static final String CAP_QUICK = "quickmenu1";

    static final class Record {
        final Kind source;
        final String marker;
        Record(Kind source, String marker) {
            if (marker != null && !marker.matches("[0-9a-f]{64}")) throw new IllegalArgumentException("marker is not a SHA-256: " + marker);
            this.source = source; this.marker = marker;
        }
    }

    static final class Decision {
        final Action action;
        final String product;
        private Decision(Action action, String product) { this.action = action; this.product = product; }
        @Override public String toString() { return action + (product == null ? "" : " " + product); }
    }

    private PackSource() {}


    static Record parse(String text) {
        if (text == null) return null;
        Kind source = null; String marker = null; boolean markerSeen = false;
        for (String line : text.split("\n", -1)) {
            if (line.isEmpty()) continue;
            String[] f = line.split("\t", -1);
            if (f.length != 2) throw new IllegalArgumentException("pack source line has " + f.length + " fields: " + line);
            switch (f[0]) {
                case "source" -> {
                    if (source != null) throw new IllegalArgumentException("pack source names its source twice");
                    source = switch (f[1]) { case "phone" -> Kind.PHONE; case "pc" -> Kind.PC; default -> throw new IllegalArgumentException("unknown pack source " + f[1]); };
                }
                case "marker" -> {
                    if (markerSeen) throw new IllegalArgumentException("pack source names its marker twice");
                    markerSeen = true;
                    marker = f[1].equals("-") ? null : f[1];
                }
                default -> throw new IllegalArgumentException("unknown pack source line " + f[0]);
            }
        }
        if (source == null || !markerSeen) throw new IllegalArgumentException("pack source record is incomplete");
        return new Record(source, marker);
    }

    static String format(Record r) {
        return "source\t" + (r.source == Kind.PHONE ? "phone" : "pc") + "\nmarker\t" + (r.marker == null ? "-" : r.marker) + "\n";
    }

    static String product(String info) {
        if (info != null) for (String line : info.split("\n")) if (line.startsWith("PRODUCT=")) return line.substring(8).trim();
        return null;
    }

    static Set<String> caps(String info) {
        Set<String> caps = new HashSet<>();
        if (info != null) for (String line : info.split("\n")) if (line.startsWith("CAPS=")) for (String c : line.substring(5).split(",")) if (!c.trim().isEmpty()) caps.add(c.trim());
        return caps;
    }


    static boolean matches(String info, String product) {
        return info != null && product.equals(product(info)) && caps(info).contains(CAP_QUICK);
    }



    static Decision decide(String have, boolean packFiles, String want, Record record, String product) { return decide(have, packFiles, want, record, product, false); }



    static Decision decide(String have, boolean packFiles, String want, Record record, String product, boolean builder) {
        return decide(have, packFiles, want, record, product, builder, false);
    }



    static Decision decide(String have, boolean packFiles, String want, Record record, String product, boolean builder, boolean asked) {
        if (asked && matches(want, product)) return new Decision(Action.IMPORT, null);
        Decision d = decideAlone(have, packFiles, want, record, product);
        return builder && d.action == Action.VERSION_ERROR ? new Decision(Action.GAME_FILES, d.product) : d;
    }

    private static Decision decideAlone(String have, boolean packFiles, String want, Record record, String product) {
        if (record != null && record.source == Kind.PHONE) {
            if (matches(have, product)) return new Decision(Action.START, null);
            return new Decision(matches(want, product) ? Action.IMPORT : Action.GAME_FILES, null);
        }
        if (want != null && !want.equals(have)) return matches(want, product) ? new Decision(Action.IMPORT, null) : new Decision(Action.VERSION_ERROR, product(want));
        if (have != null) return matches(have, product) ? new Decision(Action.START, null) : new Decision(Action.VERSION_ERROR, product(have));
        return new Decision(packFiles ? Action.START : Action.GAME_FILES, null);
    }
}
