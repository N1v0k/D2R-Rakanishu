package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;







final class BookModel {

    static final int GREEN = 0, ORANGE = 1, RED = 2, OPEN = -1;
    static final int SORT_PLACE = 0, SORT_KILL = 1, SORT_DEATH = 2, SORT_RATING = 3, SORTS = 4;

    static final class Kind {
        final String key;
        String nameKey = "", look = "";
        int type = CombatModel.REGULAR;
        final List<CombatModel.Enemy> encounters = new ArrayList<>();
        final List<String> parents = new ArrayList<>();
        Kind(String key) { this.key = key; }
    }

    static final class Listed {
        final String key, look, nameKey;
        final List<String> parents;
        Listed(String key, String look, String nameKey) { this(key, look, nameKey, new ArrayList<>()); }
        Listed(String key, String look, String nameKey, List<String> parents) { this.key = key; this.look = look; this.nameKey = nameKey; this.parents = parents; }
    }


    interface Drops { double chance(Kind k, CombatModel.Enemy e); }

    static final class Filter {
        int act, area, type = -1, sort = SORT_PLACE;
        boolean descending;
        String query = "";
    }


    static final class Tile {
        final Kind kind; final List<CombatModel.Enemy> shown; final CombatModel.Enemy hardest; final int badge;
        double chance;
        boolean named;
        Tile(Kind kind, List<CombatModel.Enemy> shown) {
            this.kind = kind; this.shown = shown; this.hardest = hardest(shown); this.badge = hardest == null ? OPEN : badge(hardest.light);
        }
    }



    static final int KILLS_OWN = 0, KILLS_RESTING = 1;
    static final class Kills {
        final int mode; final Map<String, int[]> counts = new HashMap<>();
        Kills(int mode) { this.mode = mode; }
        int count(String key, int diff) { int[] c = counts.get(key); return c == null || diff < 0 || diff > 2 ? 0 : c[diff]; }
        boolean known() { return mode == KILLS_OWN; }
    }
    static Kills kills(String text) {
        Kills k = null;
        for (String line : text.split("\n")) {
            String[] v = line.split("\t", -1);
            try {
                if (v[0].equals("book")) {
                    if (v.length != 3 || k != null) throw new IllegalArgumentException("book line");
                    int mode = Integer.parseInt(v[1]);
                    k = new Kills(mode == 0 ? KILLS_OWN : KILLS_RESTING);
                } else if (v[0].equals("kill")) {
                    if (v.length != 5 || k == null || !v[1].matches("[MS][0-9]{1,5}")) throw new IllegalArgumentException("kill line");
                    int[] c = new int[3];
                    for (int d = 0; d < 3; d++) { c[d] = Integer.parseInt(v[2 + d]); if (c[d] < 0) throw new IllegalArgumentException("kill count"); }
                    k.counts.put(v[1], c);
                }
            } catch (NumberFormatException e) { throw new IllegalArgumentException("kill number", e); }
        }
        if (k == null) throw new IllegalArgumentException("no book line");
        return k;
    }

    final List<Kind> kinds = new ArrayList<>();
    private final Map<String, Kind> byKey = new HashMap<>();

    static BookModel of(CombatModel report, List<Listed> listed) {
        BookModel b = new BookModel();
        if (report != null) for (CombatModel.Enemy e : report.enemies) {
            if (e.kindKey == null || e.kindKey.isEmpty() || e.kind == CombatModel.EXCLUDED) continue;
            Kind k = b.byKey.get(e.kindKey);
            if (k == null) { k = new Kind(e.kindKey); k.nameKey = e.key; k.look = e.look; k.type = e.kind; b.byKey.put(k.key, k); b.kinds.add(k); }
            k.encounters.add(e);
        }
        if (listed != null) for (Listed l : listed) {
            if (b.byKey.containsKey(l.key)) continue;
            Kind k = new Kind(l.key); k.nameKey = l.nameKey; k.look = l.look; k.parents.addAll(l.parents); b.byKey.put(k.key, k); b.kinds.add(k);
        }
        return b;
    }

    Kind find(String key) { return key == null ? null : byKey.get(key); }


    static int rank(int light) { return light == 0 ? 0 : light < 0 || light > 2 ? 1 : light == 1 ? 2 : 3; }
    static int badge(int light) { return light >= 0 && light <= 2 ? light : OPEN; }
    static CombatModel.Enemy hardest(List<CombatModel.Enemy> list) {
        CombatModel.Enemy out = null;
        for (CombatModel.Enemy e : list) if (out == null || rank(e.light) > rank(out.light)) out = e;
        return out;
    }

    static boolean inFilter(CombatModel.Enemy e, Filter f) {
        return (f.act == 0 || e.act == f.act) && (f.area == 0 || e.area == f.area && !e.estimated);
    }

    static boolean named(Kind k, String folded, CombatModel.Names names) {
        if (folded.isEmpty() || CombatModel.fold(k.nameKey).contains(folded)) return true;
        for (int l = 0; l < GameLang.LANGS; l++) if (CombatModel.fold(names.get(k.nameKey, l)).contains(folded)) return true;
        return false;
    }

    List<Tile> tiles(Filter f, CombatModel.Names names) { return tiles(f, names, null); }


    List<CombatModel.Enemy> parentPlaces(Kind k, Filter f) {
        List<CombatModel.Enemy> out = new ArrayList<>();
        places(k, f, 0, out);
        return out;
    }
    private void places(Kind k, Filter f, int depth, List<CombatModel.Enemy> out) {
        if (depth > 4) return;
        for (String p : k.parents) {
            Kind pk = byKey.get(p);
            if (pk == null) continue;
            for (CombatModel.Enemy e : pk.encounters) if ((f == null || inFilter(e, f)) && !out.contains(e)) out.add(e);
            if (pk.encounters.isEmpty()) places(pk, f, depth + 1, out);
        }
    }


    List<Tile> tiles(Filter filter, CombatModel.Names names, Drops drops) {
        String q = CombatModel.fold(filter.query.trim());
        Filter f = filter;
        if (!q.isEmpty() && (f.act != 0 || f.area != 0)) {
            f = new Filter(); f.type = filter.type; f.sort = filter.sort; f.descending = filter.descending; f.query = filter.query;
        }
        List<Tile> out = new ArrayList<>();
        for (Kind k : kinds) {
            if (f.type >= 0 && !CombatModel.ofType(k.type, f.type)) continue;
            List<CombatModel.Enemy> shown = new ArrayList<>();
            for (CombatModel.Enemy e : k.encounters) if (inFilter(e, f)) shown.add(e);

            List<CombatModel.Enemy> places = k.encounters.isEmpty() ? parentPlaces(k, f) : shown;
            if (shown.isEmpty() && (!k.encounters.isEmpty() || (f.act != 0 || f.area != 0) && places.isEmpty())) continue;
            double chance = 0;
            if (drops != null) {
                if (k.encounters.isEmpty() && places.isEmpty()) chance = Math.max(0, drops.chance(k, null));
                else for (CombatModel.Enemy e : places) chance = Math.max(chance, drops.chance(k, e));
            }
            boolean byName = named(k, q, names);
            if (!(chance > 0) && !byName) continue;
            Tile t = new Tile(k, shown);
            t.chance = chance; t.named = byName;
            out.add(t);
        }
        if (drops != null) {
            out.sort(Comparator.comparingInt((Tile t) -> t.named ? 0 : 1).thenComparing((a, b) -> Double.compare(b.chance, a.chance)));
            return out;
        }
        Comparator<Tile> order;
        if (f.sort == SORT_KILL) order = Comparator.comparingDouble(t -> t.hardest == null || t.hardest.kill < 0 ? Double.MAX_VALUE : t.hardest.kill);
        else if (f.sort == SORT_DEATH) order = Comparator.comparingDouble(t -> t.hardest == null || t.hardest.death < 0 ? Double.MAX_VALUE : t.hardest.death);
        else if (f.sort == SORT_RATING) order = Comparator.comparingInt(t -> t.badge == OPEN ? 3 : t.badge);
        else order = Comparator.comparingInt((Tile t) -> firstAct(t)).thenComparingInt(BookModel::firstArea);

        order = order.thenComparingInt(t -> t.shown.isEmpty() ? 1 : 0);
        out.sort(f.descending ? order.reversed() : order);
        return out;
    }
    private static int firstAct(Tile t) { int a = Integer.MAX_VALUE; for (CombatModel.Enemy e : t.shown) if (e.act > 0) a = Math.min(a, e.act); return a; }
    private static int firstArea(Tile t) {
        int act = firstAct(t), a = Integer.MAX_VALUE;
        for (CombatModel.Enemy e : t.shown) if (e.act == act && e.area > 0) a = Math.min(a, e.area);
        return a;
    }



    CombatModel.Enemy place(Kind k, int area, Filter f) {
        if (k == null || !k.encounters.isEmpty()) return null;
        List<CombatModel.Enemy> all = new ArrayList<>();
        for (String p : k.parents) { Kind pk = byKey.get(p); if (pk != null) all.addAll(pk.encounters); }
        if (all.isEmpty()) return null;
        if (area > 0) for (CombatModel.Enemy e : all) if (e.area == area) return e;
        List<CombatModel.Enemy> shown = new ArrayList<>();
        for (CombatModel.Enemy e : all) if (inFilter(e, f)) shown.add(e);
        CombatModel.Enemy h = hardest(shown);
        return h != null ? h : all.get(0);
    }


    static List<CombatModel.Enemy> places(Kind k) {
        List<CombatModel.Enemy> out = new ArrayList<>();
        if (k == null) return out;
        for (CombatModel.Enemy e : k.encounters) if (e.area > 0) out.add(e);
        out.sort(Comparator.comparingInt((CombatModel.Enemy e) -> e.act).thenComparingInt(e -> e.area));
        return out;
    }


    static CombatModel.Enemy encounter(Kind k, int area, Filter f) {
        if (k == null || k.encounters.isEmpty()) return null;
        if (area > 0) for (CombatModel.Enemy e : k.encounters) if (e.area == area) return e;
        List<CombatModel.Enemy> shown = new ArrayList<>();
        for (CombatModel.Enemy e : k.encounters) if (inFilter(e, f)) shown.add(e);
        CombatModel.Enemy h = hardest(shown);
        return h != null ? h : k.encounters.get(0);
    }



    static final class Where {
        static final int ALL = 0, AREA = 6;

        static int[] choices(boolean area) { return area ? new int[]{AREA, 1, 2, 3, 4, 5, ALL} : new int[]{1, 2, 3, 4, 5, ALL}; }
        static int index(int at, boolean area) {
            int[] c = choices(area);
            for (int i = 0; i < c.length; i++) if (c[i] == at) return i;
            return -1;
        }

        static int step(int at, int d, boolean area) {
            int[] c = choices(area);
            int i = index(at, area);
            return c[Math.max(0, Math.min(c.length - 1, (i < 0 ? 0 : i) + d))];
        }

        static int first(boolean area, int heroAct) { return area ? AREA : heroAct >= 1 && heroAct <= 5 ? heroAct : ALL; }

        static int kept(int at, boolean area, int heroAct) { return at == AREA && !area ? first(false, heroAct) : at; }
        static void apply(int at, int heroArea, Filter f) {
            f.act = at >= 1 && at <= 5 ? at : 0;
            f.area = at == AREA ? heroArea : 0;
        }

        static boolean areaHas(CombatModel report, int area) {
            if (report == null || area <= 0) return false;
            for (CombatModel.Enemy e : report.enemies)
                if (e.area == area && !e.estimated && e.kindKey != null && !e.kindKey.isEmpty() && e.kind != CombatModel.EXCLUDED) return true;
            return false;
        }
    }
}
