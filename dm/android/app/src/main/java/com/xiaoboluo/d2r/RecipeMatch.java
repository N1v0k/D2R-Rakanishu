package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.IdentityHashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;



final class RecipeMatch {
    static final int READY = 0, IN_STASH = 1, ALMOST = 2, CHAIN = 3, PROBABLE = 4, NONE = 5;
    static final int HAVE = 0, LACKS = 1, CHAINED = 2;
    static final String CUBE_CODE = "box";
    static final String[] POUCH_TYPES = {"rune", "gem"};
    private static final Map<String, Integer> QUALITY = new HashMap<>();
    static { String[] q = {"low", "nor", "hiq", "mag", "set", "rar", "uni", "crf", "tmp"}; for (int i = 0; i < q.length; i++) QUALITY.put(q[i], i + 1); }
    private static final int[] PLACE_ORDER = {RecipeStock.INVENTORY, RecipeStock.CUBE, RecipeStock.STASH, RecipeStock.STACKS, RecipeStock.SHARED, RecipeStock.POUCH, RecipeStock.OTHER_PAGE, RecipeStock.VAULT};
    private static final int MAX_DEPTH = 3;

    static final class Result {
        int status = NONE;
        boolean held;
        String text = "", where = "";
        int[] state;
        String[] img;
    }

    private final RecipeModel.Catalog data;
    private final RecipeStock stock;
    private final boolean de;
    private final List<RecipeStock.Entry> order = new ArrayList<>();
    private final Map<String, RecipeModel.Card> byOutput = new HashMap<>();
    private int[] left;

    RecipeMatch(RecipeModel.Catalog data, RecipeStock stock, boolean de) {
        this.data = data;
        this.stock = stock;
        this.de = de;
        for (int place : PLACE_ORDER) for (RecipeStock.Entry e : stock.entries) if (e.place == place && !CUBE_CODE.equals(e.code)) order.add(resolved(e));
        for (RecipeModel.Card c : data.cube) if (chainable(c)) byOutput.putIfAbsent(c.out, c);
    }


    private boolean mayLieUnread(RecipeModel.Input in) {
        if ((stock.stale & 4) != 0) return true;
        if ((stock.stale & 2) != 0 && ("u".equals(in.kind) || in.flags.contains("uni") || in.flags.contains("set"))) return true;
        if ((stock.stale & 1) == 0 || "u".equals(in.kind)) return false;
        RecipeModel.Item item = data.items.get(in.id);
        for (String t : POUCH_TYPES) {
            if (item != null ? data.isType(item, t) : t.equals(in.id)) return true;
            java.util.List<String> up = data.typeUp.get(in.id);
            if (up != null && up.contains(t)) return true;
        }
        return false;
    }


    private RecipeStock.Entry resolved(RecipeStock.Entry e) {
        if (e.place == RecipeStock.VAULT && e.code.isEmpty()) { String code = data.vault.get((e.quality == 7 ? "u" : "s") + e.row); if (code != null) e.code = code; }
        return e;
    }


    private static boolean chainable(RecipeModel.Card c) {
        if (c.out == null || c.out.isEmpty()) return false;
        for (RecipeModel.Input in : c.in) if (!"i".equals(in.kind) || in.id.equals(c.out) || !in.flags.isEmpty()) return false;
        return true;
    }

    private String name(String code) {
        RecipeModel.Item i = data.items.get(code);
        if (i == null) return code;
        String s = de ? i.shortDe : i.shortEn;
        return s != null ? s : de ? i.de : i.en;
    }

    boolean fits(RecipeModel.Input in, RecipeStock.Entry e) {
        if ("u".equals(in.kind)) return e.row == in.row && e.quality == 7;
        RecipeModel.Item item = data.items.get(e.code);
        if (item == null) return false;
        if ("i".equals(in.kind)) {
            if (!in.id.equals(e.code)) {
                RecipeModel.Item want = data.items.get(in.id);
                if (want == null || !in.flags.contains("upg") || want.norm.isEmpty() || !want.norm.equals(item.norm) || item.tier < want.tier) return false;
            }
        } else if (!"any".equals(in.id) && !data.isType(item, in.id)) return false;
        for (String f : in.flags) {
            Integer q = QUALITY.get(f);
            if (q != null) { if (e.quality != q) return false; continue; }
            switch (f) {
                case "bas": if (item.tier != 0) return false; break;
                case "exc": if (item.tier != 1) return false; break;
                case "eli": if (item.tier != 2) return false; break;
                case "nos": if (e.sockets != 0) return false; break;
                case "sock": if (e.sockets == 0) return false; break;
                case "eth": if ((e.flags & RecipeStock.ETHEREAL) == 0) return false; break;
                case "noe": if ((e.flags & RecipeStock.ETHEREAL) != 0) return false; break;
                default:
            }
        }
        return true;
    }


    private RecipeStock.Entry base(RecipeModel.Card c) {
        for (int i = 0; i < order.size(); i++) {
            RecipeStock.Entry e = order.get(i);
            RecipeModel.Item item = data.items.get(e.code);
            if (item == null || left[i] <= 0 || e.sockets != c.sockets || e.filled != 0 || e.quality >= 4 || e.quality == 0) continue;
            if ((e.flags & (RecipeStock.RUNEWORD | RecipeStock.NO_WORD)) != 0) continue;
            boolean ok = false;
            for (String t : c.itype) ok |= data.isType(item, t);
            for (String t : c.etype) ok &= !data.isType(item, t);
            if (ok) return e;
        }
        return null;
    }

    private int take(RecipeModel.Input in, int want, int[] places, boolean[] unknown) {
        int got = 0;
        for (int i = 0; i < order.size() && got < want; i++) {
            RecipeStock.Entry e = order.get(i);
            if (left[i] <= 0 || !fits(in, e)) continue;
            int n = Math.min(want - got, left[i]);
            left[i] -= n;
            got += n;
            places[e.place] += n;

            if ((e.flags & RecipeStock.UNKNOWN) != 0 && (e.place != RecipeStock.VAULT || !"u".equals(in.kind))) unknown[0] = true;
            if (e.place == RecipeStock.POUCH && (stock.stale & 1) != 0 || e.place == RecipeStock.VAULT && (stock.stale & 2) != 0) unknown[0] = true;
            if (in.flags.contains("eth") || in.flags.contains("noe")) unknown[0] = true;
        }
        return got;
    }


    private boolean make(String code, int n, int depth, Map<String, Integer> leaves) {
        RecipeModel.Card c = byOutput.get(code);
        if (c == null || depth <= 0) return false;
        int[] before = left.clone();
        Map<String, Integer> found = new LinkedHashMap<>();
        for (RecipeModel.Input in : c.in) {
            int want = in.count * n, got = 0;
            for (int i = 0; i < order.size() && got < want; i++) {
                if (left[i] <= 0 || !in.id.equals(order.get(i).code) || order.get(i).place == RecipeStock.OTHER_PAGE) continue;
                int k = Math.min(want - got, left[i]);
                left[i] -= k;
                got += k;
            }
            if (got > 0) found.merge(in.id, got, Integer::sum);
            if (got < want && !make(in.id, want - got, depth - 1, found)) { left = before; return false; }
        }
        for (Map.Entry<String, Integer> f : found.entrySet()) leaves.merge(f.getKey(), f.getValue(), Integer::sum);
        return true;
    }

    private String leavesText(Map<String, Integer> leaves) {
        StringBuilder s = new StringBuilder();
        for (Map.Entry<String, Integer> l : leaves.entrySet()) s.append(s.length() > 0 ? " + " : "").append(l.getValue()).append(' ').append(name(l.getKey()));
        return s.toString();
    }

    private String where(int[] places, RecipeStock.Entry base) {
        String[] label = RecipeTerms.places(data.terms, de);
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < places.length; i++) if (places[i] > 0) s.append(s.length() > 0 ? ", " : "").append(label[i]).append(' ').append(places[i]);
        if (base != null) s.append(s.length() > 0 ? ", " : "").append(de ? "Basis: " : "Base: ").append(name(base.code)).append(" in ").append(label[base.place]);
        return s.toString();
    }

    Result evaluate(RecipeModel.Card c) {
        left = new int[order.size()];
        for (int i = 0; i < left.length; i++) left[i] = order.get(i).qty;
        Result r = new Result();
        r.state = new int[c.in.length];
        r.img = new String[c.in.length];
        int[] places = new int[8], got = new int[c.in.length];
        boolean[] unknown = {false};
        Integer[] byKind = new Integer[c.in.length];
        for (int i = 0; i < byKind.length; i++) byKind[i] = i;
        Arrays.sort(byKind, Comparator.comparingInt(i -> "t".equals(c.in[i].kind) ? 1 : 0));
        int any = 0;
        for (int i : byKind) {
            RecipeModel.Input in = c.in[i];
            got[i] = take(in, in.count, places, unknown);
            any += got[i];
            if (got[i] > 0 && "t".equals(in.kind)) for (RecipeStock.Entry e : order) if (fits(in, e)) { r.img[i] = e.code; break; }
        }
        RecipeStock.Entry base = c.cat == null ? base(c) : null;

        String chained = null;
        boolean unread = false;
        r.held = any > 0 || base != null;
        int missing = 0, firstMissing = -1;
        for (int i = 0; i < c.in.length; i++) {
            int lack = c.in[i].count - got[i];
            if (lack <= 0) continue;
            Map<String, Integer> leaves = new LinkedHashMap<>();
            if ("i".equals(c.in[i].kind) && make(c.in[i].id, lack, MAX_DEPTH, leaves)) {
                r.state[i] = CHAINED;
                if (chained == null) {
                    boolean own = !c.out.isEmpty();
                    String head = name(own ? c.out : c.in[i].id);
                    if (!own || got[i] == 0) chained = head + (de ? ": aus " : ": from ") + leavesText(leaves);
                    else chained = head + (de ? ": aus " : ": from ") + c.in[i].count + " " + name(c.in[i].id) + (de ? ", davon " : ", ") + lack + (de ? " aus " : " from ") + leavesText(leaves);
                }
            } else { r.state[i] = LACKS; missing++; if (firstMissing < 0) firstMissing = i; unread |= mayLieUnread(c.in[i]); }
        }
        boolean baseLacks = c.cat == null && base == null;
        if (baseLacks) missing++;
        r.where = where(places, base);
        boolean stored = base != null && base.place != RecipeStock.INVENTORY && base.place != RecipeStock.CUBE;
        for (int p = RecipeStock.STASH; p < places.length; p++) stored |= places[p] > 0;
        boolean unsure = unknown[0] || stock.truncated || (c.locked && !stock.ladderFree);
        if (unread) { r.status = PROBABLE; r.text = de ? "Bestand unvollständig" : "stock incomplete"; }
        else if (missing > 0) {
            String what = firstMissing >= 0 ? name(inputCode(c.in[firstMissing])) : de ? "Basis" : "Base";
            if (firstMissing >= 0 && !"i".equals(c.in[firstMissing].kind)) what = c.in[firstMissing].name;
            r.text = missing > 1 ? (de ? "fehlt: " : "missing: ") + missing + (de ? " Sorten" : " kinds") : (de ? "fehlt: " : "missing: ") + what;
            r.status = missing == 1 && chained == null && (any > 0 || base != null) ? ALMOST : NONE;
        } else if (chained != null) { r.status = CHAIN; r.text = chained; }
        else if (c.diff > stock.difficulty) { r.status = IN_STASH; r.text = de ? "nur in der Hölle" : "Hell only"; }
        else if (c.quest) { r.status = PROBABLE; r.text = de ? "wahrscheinlich, Quest nötig" : "probably, needs its quest"; }
        else if (unsure) { r.status = PROBABLE; r.text = de ? "wahrscheinlich" : "probably"; }
        else if (stored) { r.status = IN_STASH; r.text = de ? "im Lager" : "in stash"; }
        else { r.status = READY; r.text = de ? "bereit" : "ready"; }
        return r;
    }

    private static String inputCode(RecipeModel.Input in) { return in.id; }

    Map<RecipeModel.Card, Result> evaluateAll() {
        Map<RecipeModel.Card, Result> out = new IdentityHashMap<>();
        for (RecipeModel.Card c : data.cube) out.put(c, evaluate(c));
        for (RecipeModel.Card c : data.words) out.put(c, evaluate(c));
        return out;
    }


    static List<RecipeModel.Card> doable(List<RecipeModel.Card> cards, Map<RecipeModel.Card, Result> results, List<String> categories) {
        List<RecipeModel.Card> out = new ArrayList<>();
        for (RecipeModel.Card c : cards) if (results.get(c).status != NONE && (results.get(c).status != PROBABLE || results.get(c).held)) out.add(c);
        out.sort(Comparator.<RecipeModel.Card>comparingInt(c -> rank(results.get(c).status))
            .thenComparingInt(c -> c.cat == null ? categories.size() : categories.indexOf(c.cat)).thenComparingInt(c -> -c.order));
        return out;
    }

    private static int rank(int status) { return status == READY ? 0 : status == IN_STASH ? 1 : status == PROBABLE ? 2 : 3; }
}
