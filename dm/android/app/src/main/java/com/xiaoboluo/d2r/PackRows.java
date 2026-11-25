package com.xiaoboluo.d2r;

import java.util.List;








final class PackRows {
    enum Row { BASE, UPDATE, KEYS }
    enum State { EMPTY, WAITING, OK, ERROR }



    static final class Pick {
        final String uri, name, result;
        final Row row;
        Pick(String uri, String name, String result) { this(uri, name, result, null); }
        Pick(String uri, String name, String result, Row row) { this.uri = uri; this.name = name; this.result = result; this.row = row; }
        Pick in(Row r) { return new Pick(uri, name, result, r); }
        String kind() { int t = result.indexOf('\t'); return t < 0 ? result : result.substring(0, t); }
        String rest() { int t = result.indexOf('\t'); return t < 0 ? "" : result.substring(t + 1); }


        boolean kept() {
            return switch (kind()) { case "keys", "base", "update", "package", "PB-KEYMISSING", "PB-KEYWRONG" -> true; default -> false; };
        }
    }

    static final class Line {
        final State state; final Pick pick; final String error, detail;
        Line(State state, Pick pick, String error, String detail) { this.state = state; this.pick = pick; this.error = error; this.detail = detail; }
    }


    static final class Missing {
        final Row row; final String error, detail;
        Missing(Row row, String error, String detail) { this.row = row; this.error = error; this.detail = detail; }
    }



    static Row repick(String error, String detail) {
        return switch (error) {
            case "PB-SOURCE" -> switch (detail) { case "base" -> Row.BASE; case "update" -> Row.UPDATE; case "keys" -> Row.KEYS; default -> null; };
            case "PB-KEYMISSING", "PB-KEYWRONG" -> Row.KEYS;
            default -> null;
        };
    }




    static String recheck(Row row, String probe, String identity) {
        String name = row == Row.BASE ? "base" : "update";
        if (probe.equals(name + "\t" + identity)) return null;
        if (probe.startsWith("PB-KEYMISSING\t") || probe.startsWith("PB-KEYWRONG\t")) return probe;
        return "PB-SOURCE\t" + name;
    }

    final Line base, update, keys;
    final Pick refused;

    private PackRows(Line base, Line update, Line keys, Pick refused) { this.base = base; this.update = update; this.keys = keys; this.refused = refused; }

    Line line(Row r) { return r == Row.BASE ? base : r == Row.UPDATE ? update : keys; }
    boolean ready() { return base.state == State.OK && update.state == State.OK && keys.state == State.OK; }

    static PackRows of(List<Pick> picks) { return of(picks, null); }


    static PackRows of(List<Pick> picks, Missing missing) {
        Pick keys = null, refused = null, keyError = null;
        Line base = null, update = null;
        for (Pick p : picks) {
            switch (p.kind()) {
                case "keys" -> { keys = p; keyError = null; }
                case "base" -> base = new Line(State.OK, p, null, null);
                case "update" -> update = new Line(State.OK, p, null, null);
                case "addon" -> { }
                case "package" -> { if (waitsIn(p, base) == Row.BASE) base = waiting(p); else update = waiting(p); }
                case "PB-KEYMISSING", "PB-KEYWRONG" -> {
                    keyError = p;
                    if (!p.rest().equals("header_key")) { if (waitsIn(p, base) == Row.BASE) base = waiting(p); else update = waiting(p); }
                }
                default -> refused = p;
            }
        }
        Line k = keyError != null ? new Line(State.ERROR, keys, keyError.kind(), keyError.rest())
            : keys != null ? new Line(State.OK, keys, null, null) : missingLine(missing, Row.KEYS);
        return new PackRows(base != null ? base : missingLine(missing, Row.BASE), update != null ? update : missingLine(missing, Row.UPDATE), k, refused);
    }

    private static Line waiting(Pick p) { return new Line(State.WAITING, p, null, null); }



    private static Row waitsIn(Pick p, Line base) {
        if (p.row == Row.BASE || p.row == Row.UPDATE) return p.row;
        return base == null ? Row.BASE : Row.UPDATE;
    }

    private static Line missingLine(Missing missing, Row row) {
        return missing != null && missing.row == row ? new Line(State.ERROR, null, missing.error, missing.detail) : new Line(State.EMPTY, null, null, null);
    }
}
