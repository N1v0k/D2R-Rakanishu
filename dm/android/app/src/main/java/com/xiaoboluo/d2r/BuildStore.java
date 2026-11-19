








package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.SharedPreferences;

final class BuildStore {
    static final String PREFS = "dm_builds";
    static final int OK = 0, NAMES_MISSING = 1, INCOMPATIBLE = 2;


    static final class ActiveBuild {
        String guideId = "", phaseId = "", profile = "{}";
        int version, compat;
    }

    private BuildStore() {}

    private static SharedPreferences prefs(Context c) { return c.getSharedPreferences(PREFS, Context.MODE_PRIVATE); }
    static BuildModel.Follow get(Context c, String name, int classIndex) { return name == null || name.isEmpty() ? null : BuildModel.Follow.decode(prefs(c).getString(BuildModel.Follow.key(name, classIndex), null)); }
    static void put(Context c, String name, int classIndex, BuildModel.Follow e) { if (name != null && !name.isEmpty()) prefs(c).edit().putString(BuildModel.Follow.key(name, classIndex), BuildModel.Follow.encode(e)).apply(); }
    static void remove(Context c, String name, int classIndex) { prefs(c).edit().remove(BuildModel.Follow.key(name, classIndex)).apply(); }



    static SaveTransfer.Builds transfer(Context c) {
        return new SaveTransfer.Builds() {
            @Override public java.util.Properties all() {
                java.util.Properties p = new java.util.Properties();
                for (java.util.Map.Entry<String, ?> e : prefs(c).getAll().entrySet()) if (e.getValue() instanceof String) p.setProperty(e.getKey(), (String) e.getValue());
                return p;
            }
            @Override public int addMissing(java.util.Properties q) throws java.io.IOException {
                java.util.Map<String, ?> have = prefs(c).getAll();
                SharedPreferences.Editor ed = prefs(c).edit();
                java.util.List<String> added = new java.util.ArrayList<>();
                for (String k : q.stringPropertyNames()) if (!have.containsKey(k)) { ed.putString(k, q.getProperty(k)); added.add(k); }
                if (added.isEmpty() || ed.commit()) return added.size();

                SharedPreferences.Editor undo = prefs(c).edit();
                for (String k : added) undo.remove(k);
                undo.apply();
                throw new java.io.IOException("build guides not stored");
            }
        };
    }

    static String profile(BuildModel.Guide g, int phase) {
        try {
            org.json.JSONObject p = new org.json.JSONObject(g.profile);
            String over = phase >= 0 && phase < g.phases.size() ? g.phaseProfiles.get(g.phases.get(phase).id) : null;
            if (over != null) {
                org.json.JSONObject o = new org.json.JSONObject(over);
                for (java.util.Iterator<String> it = o.keys(); it.hasNext(); ) { String k = it.next(); p.put(k, o.get(k)); }
            }
            return p.toString();
        } catch (org.json.JSONException e) { return "{}"; }
    }



    static int[] keySkills(String profile, int max) {
        try {
            org.json.JSONArray a = new org.json.JSONObject(profile).optJSONArray("keySkills");
            if (a == null) return new int[0];
            java.util.List<double[]> l = new java.util.ArrayList<>();
            for (int i = 0; i < a.length(); i++) { org.json.JSONObject o = a.optJSONObject(i); if (o != null && o.has("skill")) l.add(new double[] { o.getInt("skill"), o.optDouble("w", 0) }); }
            java.util.Collections.sort(l, (x, y) -> Double.compare(y[1], x[1]));
            int[] r = new int[Math.min(l.size(), max)];
            for (int i = 0; i < r.length; i++) r[i] = (int) l.get(i)[0];
            return r;
        } catch (org.json.JSONException e) { return new int[0]; }
    }


    static ActiveBuild active(Context c, BuildData data, String name, int classIndex, int level) {
        BuildModel.Follow e = get(c, name, classIndex);
        if (e == null) return null;
        ActiveBuild a = new ActiveBuild();
        a.guideId = e.guide;
        a.version = e.version;
        BuildModel.Guide g = data == null ? null : data.guide(e.guide);
        if (g == null || g.classIndex != classIndex) { a.compat = INCOMPATIBLE; return a; }
        int ph = BuildPlan.phase(g, level, e.phase);
        a.phaseId = g.phases.get(ph).id;
        a.version = g.version;
        a.profile = profile(g, ph);
        a.compat = data.names == null ? NAMES_MISSING : OK;
        return a;
    }
}
