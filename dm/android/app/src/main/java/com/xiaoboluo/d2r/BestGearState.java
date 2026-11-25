package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONObject;




final class BestGearState {
    static final int WORN = 0, INV = 1, STASH = 2, SHARED = 3;
    static final class Row {





        final String slot, name, colour, code; final int quality, where, page, row, gfx, id, charm, place; final boolean first;
        Row(String slot, String name, String colour, String code, int quality, int where, int page, int row, int gfx, int id, int charm, int place, boolean first) {
            this.slot = slot; this.name = name; this.colour = colour; this.code = code; this.quality = quality; this.where = where; this.page = page;
            this.row = row; this.gfx = gfx; this.id = id; this.charm = charm; this.place = place; this.first = first;
        }
    }


    static final class Tip {
        final int id, place, dots; final String text, block;
        Tip(int id, int place, int dots, String text, String block) { this.id = id; this.place = place; this.dots = dots; this.text = text; this.block = block; }
    }
    static final class Line { final String colour, text; Line(String colour, String text) { this.colour = colour; this.text = text; } }



    final String eqkey;
    final int stash;
    final int[] eq;
    final boolean eqall;
    final BestGearEquip.Job job;
    final String rev, status, state;
    final long runs, estimate;
    final int share;
    final int dots, pages, shared, skipped, unrated;
    final boolean result;
    final boolean charms;
    final List<Row> rows;
    final List<Line> totals;

    final List<int[]> hands;
    final Tip tip;

    private BestGearState(JSONObject o) {
        rev = o.optString("rev", ""); status = o.optString("status", "none"); state = o.optString("state", "none");
        runs = o.optLong("runs", 0); estimate = o.optLong("estimate", 0); share = o.optInt("share", 0);
        dots = o.optInt("dots", 0); pages = o.optInt("pages", 0); shared = o.optInt("shared", 0); skipped = o.optInt("skipped", 0); unrated = o.optInt("unrated", 0);
        charms = o.optInt("charms", 0) != 0;
        JSONArray r = o.optJSONArray("rows"), t = o.optJSONArray("totals");
        result = r != null;
        List<Row> rs = new ArrayList<>();
        for (int i = 0; r != null && i < r.length(); i++) {
            JSONObject x = r.optJSONObject(i); if (x == null) continue;
            rs.add(new Row(x.optString("slot", ""), x.optString("name", ""), x.optString("c", ""), x.optString("code", ""), x.optInt("q", 0), x.optInt("where", -1),
                x.optInt("page", 0), x.optInt("row", -1), x.optInt("gfx", -1), x.optInt("id", 0), x.optInt("charm", 0), x.optInt("p", -1), x.optInt("first", 0) != 0));
        }
        List<Line> ts = new ArrayList<>();
        for (int i = 0; t != null && i < t.length(); i++) { JSONObject x = t.optJSONObject(i); if (x != null) ts.add(new Line(colour(x.optString("c", "5")), x.optString("t", ""))); }
        rows = Collections.unmodifiableList(rs); totals = Collections.unmodifiableList(ts);
        JSONArray h = o.optJSONArray("hands");
        List<int[]> hs = new ArrayList<>();
        for (int i = 0; h != null && i < h.length(); i++) { JSONArray x = h.optJSONArray(i); if (x != null && x.length() == 2) hs.add(new int[] { x.optInt(0), x.optInt(1) }); }
        hands = Collections.unmodifiableList(hs);
        eqkey = o.optString("eqkey", ""); stash = o.optInt("stash", -1); eqall = o.optInt("eqall", 0) != 0;
        JSONArray e = o.optJSONArray("eq");
        eq = new int[rs.size()];
        for (int i = 0; i < eq.length; i++) eq[i] = e != null && i < e.length() ? e.optInt(i, BestGearEquip.EQ_NONE) : BestGearEquip.EQ_NONE;
        JSONObject j = o.optJSONObject("job");
        job = j == null ? null : new BestGearEquip.Job(j.optInt("seq", 0), j.optString("st", ""), j.optInt("k", 0), j.optInt("n", 0), j.optInt("off", 0), j.optString("why", ""));
        JSONObject p = o.optJSONObject("tip");
        tip = p == null ? null : new Tip(p.optInt("id", 0), p.optInt("p", -1), p.optInt("dots", 0), p.optString("t", ""), p.optString("b", ""));
    }
    static BestGearState parse(String json) { try { return new BestGearState(new JSONObject(json)); } catch (Exception e) { return null; } }

    boolean done() { return "done".equals(status); }
    boolean failed() { return "failed".equals(status); }
    boolean usable() { return "ok".equals(state); }


    float fill(float before) {
        if (done()) return 1f;
        float now = share / 1e6f;
        if (estimate > 0) now = Math.max(now, runs / (float) estimate);
        return Math.max(before, Math.min(0.99f, now));
    }

    static String colour(String digit) { return "3".equals(digit) ? "FontColorPartyGreen" : "1".equals(digit) ? "FontColorRed" : "FontColorLightGray"; }

    static String quality(int q) { return q == 5 ? "FontColorGreen" : q == 7 || q == 9 ? "FontColorGold" : q == 6 ? "FontColorYellow" : q == 4 ? "FontColorBlue" : q == 8 ? "FontColorOrange" : "FontColorWhite"; }
}
