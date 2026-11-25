package com.xiaoboluo.d2r;

import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import com.xiaoboluo.d2r.RecipeModel.Card;
import com.xiaoboluo.d2r.RecipeModel.Input;



final class RecipeData {
    static final int DATA_VERSION = 3;
    private static final long MAX_BYTES = 1 << 20;

    private static final int MAX_CARDS = 300, MAX_ITEMS = 1000, MAX_TYPES = 200, MAX_INPUTS = 7, MAX_RUNES = 6, MAX_GROUPS = 4, MAX_PREVIEW = 40, MAX_LIST = 12, MAX_CODE = 8, MAX_KEY = 16, MAX_TEXT = 120, MAX_SEARCH = 1024;
    private static final java.util.regex.Pattern PICTURE = java.util.regex.Pattern.compile("[a-z0-9]{1,8}");

    final RecipeModel.Catalog catalog = new RecipeModel.Catalog();
    final List<RecipeModel.Card> cube = catalog.cube, words = catalog.words;

    private static void strings(JSONArray a, List<String> into) throws JSONException {
        if (a == null) return;
        if (a.length() > MAX_LIST) throw new JSONException("list");
        for (int i = 0; i < a.length(); i++) into.add(bounded(a.getString(i), MAX_CODE));
    }

    private static String bounded(String s, int max) throws JSONException {
        if (s.length() > max) throw new JSONException("too long");
        return s;
    }

    private static String text(JSONObject o, String de, String en, boolean german) throws JSONException { return bounded(o.optString(german ? de : en), MAX_TEXT); }

    private static String picture(String s) throws JSONException {
        if (!PICTURE.matcher(s).matches()) throw new JSONException("picture");
        return s;
    }

    private static JSONArray array(JSONObject o, String key, int max) throws JSONException {
        JSONArray a = o.getJSONArray(key);
        if (a.length() > max) throw new JSONException("too many " + key);
        return a;
    }

    private RecipeData(JSONObject root, boolean de) throws JSONException {
        if (root.getInt("dataVersion") != DATA_VERSION) throw new JSONException("data version");
        JSONObject items = root.getJSONObject("items");
        JSONArray a = array(root, "cube", MAX_CARDS);
        for (int i = 0; i < a.length(); i++) {
            JSONObject o = a.getJSONObject(i);
            Card c = new Card();
            c.cat = o.getString("cat");
            c.title = text(o, "de", "en", de);
            c.colour = o.getString("colour");
            c.img = picture(o.getString("img"));
            c.search = bounded(o.getString("s"), MAX_SEARCH);
            c.order = o.getInt("ord");
            c.out = bounded(o.optString("out"), MAX_CODE);
            c.diff = o.optInt("diff");
            c.num = o.getInt("num");
            c.locked = o.has("season");
            c.quest = o.optInt("quest") != 0;
            c.row = o.getInt("row");
            c.app = bounded(o.optString("app"), MAX_CODE);
            JSONArray pv = array(o, "pv", MAX_PREVIEW);
            for (int k = 0; k < pv.length(); k++) c.preview.add(bounded(pv.getJSONArray(k).getString(de ? 0 : 1), MAX_TEXT));
            c.sub = text(o, "nde", "nen", de);
            JSONArray in = array(o, "in", MAX_INPUTS);
            c.in = new Input[in.length()];
            for (int k = 0; k < c.in.length; k++) {
                JSONObject x = in.getJSONObject(k);
                Input e = new Input();
                e.id = bounded(x.getString("id"), MAX_TEXT);
                e.kind = bounded(x.getString("k"), 1);
                e.img = picture(x.getString("img"));
                e.name = text(x, "de", "en", de);
                e.cond = text(x, "cde", "cen", de);
                e.count = x.getInt("n");
                e.row = x.optInt("row", -1);
                JSONArray f = x.optJSONArray("f");
                if (f != null) { if (f.length() > MAX_INPUTS) throw new JSONException("flags"); for (int n = 0; n < f.length(); n++) e.flags.add(f.getString(n)); }
                c.in[k] = e;
            }
            cube.add(c);
        }
        a = array(root, "words", MAX_CARDS);
        for (int i = 0; i < a.length(); i++) {
            JSONObject o = a.getJSONObject(i);
            Card c = new Card();
            c.title = text(o, "de", "en", de);
            c.key = bounded(o.optString("key"), MAX_KEY);
            c.colour = "unique";
            c.img = picture(o.getString("img"));
            c.search = bounded(o.getString("s"), MAX_SEARCH);
            c.order = o.getInt("lvl");
            JSONArray runes = array(o, "runes", MAX_RUNES);
            c.sockets = runes.length();
            c.sub = (de ? "Stufe " : "Level ") + c.order + ": " + text(o, "bde", "ben", de);
            c.in = new Input[c.sockets];
            for (int k = 0; k < c.sockets; k++) {
                String code = picture(runes.getString(k));
                JSONObject item = items.getJSONObject(code);
                JSONArray shortName = item.optJSONArray("short");
                Input in = new Input();
                in.id = code;
                in.kind = "i";
                in.img = code;
                in.name = shortName != null ? bounded(shortName.getString(de ? 0 : 1), MAX_TEXT) : text(item, "de", "en", de);
                c.in[k] = in;
            }
            JSONArray g = array(o, "groups", MAX_GROUPS);
            c.locked = o.has("season") || o.has("lock");
            c.row = o.getInt("row");
            c.preview.add(text(o, "bde", "ben", de));
            strings(o.getJSONArray("itype"), c.itype);
            strings(o.optJSONArray("etype"), c.etype);
            for (int k = 0; k < g.length(); k++) c.groups.add(g.getString(k));
            words.add(c);
        }
        JSONObject all = root.getJSONObject("all");
        if (all.length() > MAX_ITEMS) throw new JSONException("too many items");
        for (java.util.Iterator<String> i = all.keys(); i.hasNext();) {
            String code = i.next();
            JSONArray x = all.getJSONArray(code);
            RecipeModel.Item item = new RecipeModel.Item(bounded(x.getString(0), MAX_TEXT), bounded(x.getString(1), MAX_TEXT), bounded(x.getString(2), MAX_CODE), bounded(x.getString(3), MAX_CODE), x.getInt(4), bounded(x.getString(5), MAX_CODE));
            item.w = x.getInt(6);
            item.h = x.getInt(7);
            JSONObject used = items.optJSONObject(code);
            JSONArray s = used == null ? null : used.optJSONArray("short");
            if (s != null) { item.shortDe = bounded(s.getString(0), MAX_TEXT); item.shortEn = bounded(s.getString(1), MAX_TEXT); }
            catalog.items.put(code, item);
        }
        JSONObject rows = root.getJSONObject("vault");
        if (rows.length() > MAX_ITEMS) throw new JSONException("too many rows");
        for (java.util.Iterator<String> i = rows.keys(); i.hasNext();) { String row = i.next(); catalog.vault.put(row, bounded(rows.getString(row), MAX_CODE)); }


        JSONObject terms = root.optJSONObject("terms");
        if (terms != null) {
            if (terms.length() > MAX_TYPES) throw new JSONException("too many terms");
            for (java.util.Iterator<String> i = terms.keys(); i.hasNext();) {
                String key = i.next();
                JSONArray w = terms.optJSONArray(key);
                if (!RecipeTerms.known(key) || w == null || w.length() != 2) continue;
                String d = w.optString(0), e = w.optString(1);
                if (d.length() <= MAX_TEXT && e.length() <= MAX_TEXT) catalog.terms.put(key, new String[]{d, e});
            }
        }
        JSONObject up = root.getJSONObject("typeUp");
        if (up.length() > MAX_TYPES) throw new JSONException("too many types");
        for (java.util.Iterator<String> i = up.keys(); i.hasNext();) {
            String type = i.next();
            List<String> list = new ArrayList<>();
            strings(up.getJSONArray(type), list);
            catalog.typeUp.put(type, list);
        }
        Comparator<Card> byOrder = Comparator.comparingInt(x -> x.order);
        cube.sort(byOrder);
        words.sort(byOrder.thenComparing(x -> x.title));
    }

    private static File file(File filesDir) { return new File(filesDir, "dm/appart/recipes.json"); }

    static boolean usable(File filesDir) { File f = file(filesDir); return f.isFile() && f.length() > 0 && f.length() <= MAX_BYTES; }


    static RecipeData load(File filesDir, boolean de) {
        if (!usable(filesDir)) return null;
        try { return new RecipeData(new JSONObject(new String(Files.readAllBytes(file(filesDir).toPath()), StandardCharsets.UTF_8)), de); }
        catch (java.io.IOException | JSONException | RuntimeException | VirtualMachineError e) { return null; }
    }


    static String fold(String s) {
        s = s.toLowerCase(Locale.ROOT).replace("ß", "ss").replace('ä', 'a').replace('ö', 'o').replace('ü', 'u').replace("ae", "a").replace("oe", "o").replace("ue", "u");
        return s.replaceAll("[^0-9a-z\\u0080-\\uffff]+", " ").trim();
    }


    static boolean matches(String search, String[] query) {
        String words = " " + search;
        for (String q : query) if (!words.contains(q.length() <= 3 ? " " + q : q)) return false;
        return true;
    }

    static String[] query(String text) {
        String f = fold(text);
        return f.isEmpty() ? new String[0] : f.split(" ");
    }


    List<Card> search(String text, List<String> categories) {
        String[] q = query(text);
        List<Card> out = new ArrayList<>();
        if (q.length == 0) return out;
        for (String cat : categories) for (Card c : cube) if (cat.equals(c.cat) && matches(c.search, q)) out.add(c);
        for (Card c : words) if (matches(c.search, q)) out.add(c);
        return out;
    }

    static List<String> list(String... s) { return Arrays.asList(s); }
}
