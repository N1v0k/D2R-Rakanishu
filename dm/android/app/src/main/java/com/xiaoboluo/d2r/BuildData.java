





package com.xiaoboluo.d2r;

import android.content.Context;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

final class BuildData {
    static final String VERSION = "3.3.94012";
    static final int DATA_VERSION = 3, NAMES_SCHEMA = 1, GUIDE_SCHEMA = 1;
    private static final int MAX_GUIDE_BYTES = 256 << 10, MAX_GUIDES = 64, MAX_NAMES_BYTES = 2 << 20;
    private static final int MAX_TEXT = 400, MAX_LIST = 120, MAX_KEY = 60;
    static final String[] CLASS_CODES = {"ama", "sor", "nec", "pal", "bar", "dru", "ass", "war"};

    final List<BuildModel.Guide> guides = new ArrayList<>();
    BuildModel.Names names;
    String problem = "";

    BuildModel.Guide guide(String id) { for (BuildModel.Guide g : guides) if (g.id.equals(id)) return g; return null; }

    List<BuildModel.Guide> ofClass(int classIndex) {
        List<BuildModel.Guide> out = new ArrayList<>();
        for (BuildModel.Guide g : guides) if (g.classIndex == classIndex) out.add(g);
        return out;
    }

    static BuildData load(Context context) {
        BuildData d = new BuildData();
        try {
            String[] files = context.getAssets().list("builds");
            if (files != null) {
                Arrays.sort(files);
                for (String f : files) {
                    if (!f.matches("[a-z0-9-]{3,40}\\.json") || d.guides.size() >= MAX_GUIDES) continue;
                    try (InputStream in = context.getAssets().open("builds/" + f)) {
                        BuildModel.Guide g = guide(new JSONObject(read(in, MAX_GUIDE_BYTES)));
                        if ((g.id + ".json").equals(f)) d.guides.add(g);
                    } catch (IOException | JSONException | RuntimeException e) { android.util.Log.w("dm", "Build-Guide " + f + ": " + e); }
                }
            }
        } catch (IOException e) { android.util.Log.w("dm", "Build-Guides: " + e); }
        File names = new File(context.getFilesDir(), "dm/appart/builds/gamenames.json");
        try {
            if (!names.isFile()) d.problem = "no gamenames.json";
            else if (names.length() > MAX_NAMES_BYTES) d.problem = "gamenames.json too large";
            else d.names = names(new JSONObject(new String(Files.readAllBytes(names.toPath()), StandardCharsets.UTF_8)));
        } catch (IOException | JSONException | RuntimeException e) { d.problem = e.toString(); d.names = null; }
        return d;
    }

    private static String read(InputStream in, int max) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] b = new byte[16384];
        for (int n; (n = in.read(b)) > 0; ) { out.write(b, 0, n); if (out.size() > max) throw new IOException("too large"); }
        return new String(out.toByteArray(), StandardCharsets.UTF_8);
    }

    private static String str(JSONObject o, String k, int max) throws JSONException {
        String s = o.optString(k, "");
        if (s.length() > max) throw new JSONException(k + " too long");
        return s;
    }

    private static BuildModel.Text text(JSONObject o, String k) throws JSONException {
        JSONObject t = o.optJSONObject(k);
        if (t == null) return null;
        String de = str(t, "de", MAX_TEXT), en = str(t, "en", MAX_TEXT);
        if (de.isEmpty() || en.isEmpty()) throw new JSONException(k + " needs de and en");
        return new BuildModel.Text(de, en);
    }

    private static JSONArray array(JSONObject o, String k) throws JSONException {
        JSONArray a = o.optJSONArray(k);
        if (a == null) return new JSONArray();
        if (a.length() > MAX_LIST) throw new JSONException(k + " too long");
        return a;
    }

    private static int[] ints(JSONArray a) throws JSONException { int[] r = new int[a.length()]; for (int i = 0; i < r.length; i++) r[i] = a.getInt(i); return r; }

    private static void texts(JSONObject o, String k, List<BuildModel.Text> into) throws JSONException {
        JSONObject t = o.optJSONObject(k);
        if (t == null) return;
        JSONArray de = array(t, "de"), en = array(t, "en");
        if (de.length() != en.length()) throw new JSONException(k + ": de and en differ");
        for (int i = 0; i < de.length(); i++) into.add(new BuildModel.Text(de.getString(i), en.getString(i)));
    }

    private static BuildModel.Gear gear(JSONObject o) throws JSONException {
        BuildModel.Gear x = new BuildModel.Gear();
        x.phase = str(o, "phase", 8); x.slot = str(o, "slot", 10); x.tier = str(o, "tier", 8);
        JSONObject ref = o.getJSONObject("ref");
        for (String k : new String[]{"runeword", "unique", "set", "item", "type"}) if (ref.has(k)) { x.kind = k; x.key = str(ref, k, MAX_KEY); }
        if (x.kind.isEmpty()) throw new JSONException("gear without ref");
        x.quality = str(ref, "quality", 10);
        JSONObject b = o.optJSONObject("base");
        if (b != null) { x.baseItem = str(b, "item", 4); x.baseType = str(b, "type", 4); x.sockets = b.optInt("sockets"); x.eth = b.optBoolean("eth"); }
        x.note = text(o, "note");
        return x;
    }

    static BuildModel.Guide guide(JSONObject o) throws JSONException {
        if (o.getInt("schema") != GUIDE_SCHEMA) throw new JSONException("schema");
        if (!VERSION.equals(o.getString("gameVersion"))) throw new JSONException("game version");
        BuildModel.Guide g = new BuildModel.Guide();
        g.id = str(o, "id", 40); g.version = o.getInt("version"); g.cls = str(o, "class", 3); g.classIndex = o.getInt("classIndex");
        if (g.classIndex < 0 || g.classIndex > 7 || !CLASS_CODES[g.classIndex].equals(g.cls)) throw new JSONException("class");
        g.title = text(o, "title"); g.summary = text(o, "summary");
        if (g.title == null || g.summary == null) throw new JSONException("title");
        JSONObject ov = o.getJSONObject("overview");
        texts(ov, "strengths", g.strengths); texts(ov, "weaknesses", g.weaknesses); texts(ov, "notes", g.notes);
        JSONArray tags = array(o, "tags");
        for (int i = 0; i < tags.length(); i++) g.tags.add(tags.getString(i));
        g.difficulty = o.getInt("difficulty"); g.patch = str(o, "patch", 10);
        g.keySkills = ints(array(o, "keySkills")); g.trees = ints(array(o, "trees"));
        g.profile = o.getJSONObject("profile").toString();
        JSONArray ph = array(o, "phases");
        for (int i = 0; i < ph.length(); i++) {
            JSONObject p = ph.getJSONObject(i);
            BuildModel.Phase x = new BuildModel.Phase();
            x.id = str(p, "id", 8); x.name = text(p, "name"); x.note = text(p, "note");
            JSONArray lv = p.getJSONArray("levels"); x.lo = lv.getInt(0); x.hi = lv.getInt(1);
            x.respec = "respec".equals(p.getString("plan"));
            JSONArray sk = array(p, "skills");
            for (int k = 0; k < sk.length(); k++) {
                JSONObject s = sk.getJSONObject(k);
                BuildModel.Step st = new BuildModel.Step();
                st.lvl = s.getInt("lvl"); st.skill = s.getInt("skill"); st.to = s.getInt("to"); st.save = s.optBoolean("save"); st.key = str(s, "key", MAX_KEY); st.note = text(s, "note");
                x.skills.add(st);
            }
            JSONArray stats = array(p, "stats");
            for (int k = 0; k < stats.length(); k++) {
                JSONObject s = stats.getJSONObject(k);
                BuildModel.StatRule r = new BuildModel.StatRule();
                r.stat = str(s, "stat", 3); r.to = s.optInt("to", -1); r.rest = s.optBoolean("rest"); r.none = s.optBoolean("none"); r.why = text(s, "why");
                x.stats.add(r);
            }
            if (p.has("profile")) g.phaseProfiles.put(x.id, p.getJSONObject("profile").toString());
            g.phases.add(x);
        }
        if (g.phases.isEmpty()) throw new JSONException("no phases");
        JSONArray ge = array(o, "gear");
        for (int i = 0; i < ge.length(); i++) g.gear.add(gear(ge.getJSONObject(i)));
        JSONObject m = o.getJSONObject("merc");
        g.merc = new BuildModel.Merc();
        g.merc.act = m.getInt("act"); g.merc.difficulty = m.getInt("difficulty"); g.merc.aura = str(m, "aura", MAX_KEY); g.merc.why = text(m, "why");
        JSONArray mg = array(m, "gear");
        for (int i = 0; i < mg.length(); i++) g.merc.gear.add(gear(mg.getJSONObject(i)));
        JSONArray so = array(o, "sources");
        for (int i = 0; i < so.length(); i++) {
            JSONObject s = so.getJSONObject(i);
            BuildModel.Source x = new BuildModel.Source();
            x.site = str(s, "site", 40); x.title = str(s, "title", 120); x.url = str(s, "url", 300); x.seen = str(s, "seen", 10); x.patch = str(s, "patch", 20); x.kind = str(s, "kind", 10); x.note = text(s, "note");
            if (!x.url.startsWith("https://")) throw new JSONException("url");
            g.sources.add(x);
        }
        JSONObject rv = o.getJSONObject("review");
        JSONArray ch = array(rv, "checks");
        for (int i = 0; i < ch.length(); i++) g.checks.add(ch.getString(i));
        g.played = rv.optBoolean("played");
        return g;
    }

    private static BuildModel.Text pair(JSONArray a) throws JSONException {
        if (a == null || a.length() != 2) throw new JSONException("name pair");
        String de = a.getString(0), en = a.getString(1);
        if (de.length() > 80 || en.length() > 80) throw new JSONException("name too long");
        return new BuildModel.Text(de, en);
    }





    static void preview(JSONObject s, BuildModel.ItemInfo ii) throws JSONException {
        if (s.has("row")) ii.row = strictInt(s.get("row"), 0, 65535, "row");
        if (s.has("gat")) ii.gat = strictInt(s.get("gat"), 0, 2, "gat");
        ii.ac = range(s, "ac"); ii.d1 = range(s, "d1"); ii.d2 = range(s, "d2");
        if (s.has("short")) ii.shortName = pair(s.getJSONArray("short"));
        ii.ease = s.optInt("ease") == 1;
    }

    private static int strictInt(Object v, int lo, int hi, String what) throws JSONException {
        if (!(v instanceof Integer) || (Integer) v < lo || (Integer) v > hi) throw new JSONException(what + " out of range: " + v);
        return (Integer) v;
    }

    private static int[] range(JSONObject s, String k) throws JSONException {
        if (!s.has(k)) return null;
        JSONArray a = s.getJSONArray(k);
        if (a.length() != 2) throw new JSONException(k + " needs two numbers");
        int lo = strictInt(a.get(0), 0, 100000, k), hi = strictInt(a.get(1), 0, 100000, k);
        if (lo > hi) throw new JSONException(k + " reversed");
        return new int[]{lo, hi};
    }

    static BuildModel.Names names(JSONObject o) throws JSONException {
        if (!VERSION.equals(o.getString("version")) || o.getInt("dataVersion") != DATA_VERSION || o.getInt("schema") != NAMES_SCHEMA) throw new JSONException("names version");
        BuildModel.Names n = new BuildModel.Names();
        JSONObject cl = o.getJSONObject("classes");
        for (Iterator<String> it = cl.keys(); it.hasNext(); ) {
            String code = it.next(); JSONObject c = cl.getJSONObject(code);
            BuildModel.ClassInfo ci = new BuildModel.ClassInfo();
            ci.index = c.getInt("index"); BuildModel.Text t = pair(c.getJSONArray("name")); ci.de = t.de; ci.en = t.en;
            JSONObject tabs = c.getJSONObject("tabs");
            for (int p = 1; p <= 3; p++) { BuildModel.Text tt = pair(tabs.getJSONArray(String.valueOf(p))); ci.tabsDe[p] = tt.de; ci.tabsEn[p] = tt.en; }
            n.classes.put(code, ci);
        }
        JSONObject sk = o.getJSONObject("skills");
        if (sk.length() > 600) throw new JSONException("too many skills");
        for (Iterator<String> it = sk.keys(); it.hasNext(); ) {
            String id = it.next(); JSONObject s = sk.getJSONObject(id);
            BuildModel.SkillInfo si = new BuildModel.SkillInfo();
            si.id = Integer.parseInt(id); si.key = s.getString("key"); si.cls = s.getString("class"); si.page = s.getInt("page"); si.row = s.getInt("row"); si.col = s.getInt("col");
            si.req = s.getInt("req"); si.max = s.getInt("max"); si.icon = s.getInt("icon"); si.pre = ints(s.getJSONArray("pre")); si.syn = ints(s.getJSONArray("syn"));
            BuildModel.Text t = pair(s.getJSONArray("name")); si.de = t.de; si.en = t.en; si.nameKey = s.optString("nk", "");
            n.skills.put(si.id, si);
        }
        JSONObject items = o.getJSONObject("items");
        if (items.length() > 4000) throw new JSONException("too many items");
        for (Iterator<String> it = items.keys(); it.hasNext(); ) {
            String k = it.next(); JSONObject s = items.getJSONObject(k);
            BuildModel.ItemInfo ii = new BuildModel.ItemInfo();
            BuildModel.Text t = pair(s.getJSONArray("name")); ii.de = t.de; ii.en = t.en;
            ii.img = s.optString("img"); ii.code = s.optString("code"); ii.type = s.optString("type");


            if (!ii.img.isEmpty() && !ii.img.matches("[a-z0-9]{2,4}|[us]_[a-z0-9_]{1,60}")) throw new JSONException("picture");
            ii.lvl = s.optInt("lvl"); ii.str = s.optInt("str"); ii.dex = s.optInt("dex"); ii.sockets = s.optInt("sockets");
            JSONArray r = s.optJSONArray("runes");
            if (r != null) for (int i = 0; i < r.length() && i < 6; i++) ii.runes.add(r.getString(i));
            JSONArray up = s.optJSONArray("up");
            if (up != null) for (int i = 0; i < up.length() && i < 16; i++) ii.up.add(up.getString(i));
            preview(s, ii);
            n.items.put(k, ii);
        }
        for (String[] m : new String[][]{{"stats", "s"}, {"qualities", "q"}, {"auras", "a"}}) {
            JSONObject x = o.getJSONObject(m[0]);
            for (Iterator<String> it = x.keys(); it.hasNext(); ) {
                String k = it.next();
                (m[1].equals("s") ? n.stats : m[1].equals("q") ? n.qualities : n.auras).put(k, pair(x.getJSONArray(k)));
            }
        }
        JSONObject lb = o.optJSONObject("labels");
        if (lb != null) for (Iterator<String> it = lb.keys(); it.hasNext(); ) { String k = it.next(); n.labels.put(k, pair(lb.getJSONArray(k))); }
        JSONArray df = o.getJSONArray("difficulties");
        for (int i = 0; i < df.length() && i < 3; i++) n.difficulties.add(pair(df.getJSONArray(i)));
        return n;
    }
}
