package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;







final class CombatGraph {
    interface Measure { float width(String text); }
    interface Words {
        String skill(int id);
        boolean icon(int id);
        String shortage(String skill);
        String potion(int group);
    }
    static final int LINE_ENEMY = 0, LINE_LIFE = 1, TICK = 2, SHADE = 3, POTION = 4, LINE_SIM = 5, STAR = 6, LINE_GROUP = 7;
    static final int ATTACK_TICK = 0xFFE1E1E1;

    static final int[] TICKS = {0xFFFFFF64, 0xFFB48CFF, 0xFF50DCDC, 0xFFFFA03C};
    static final String[] POTION_ICON = {"hp1", "hp2", "hp3", "hp4", "hp5", "mp1", "mp2", "mp3", "mp4", "mp5", "rv1", "rv2"};

    static final class Entry { int kind, colour, skill = -1; String text, icon; float x, y, textX; }

    static final class Extra {
        int[] enemy, own;
        String ownLane, enemyLane, enemyLaneShort;
        String hit, miss, dodge, block, crit;
        int hitColour, missColour, dodgeColour, blockColour, simColour, starColour, bandColour;
        final List<String> bandText = new ArrayList<>();
        final List<int[]> bandSpan = new ArrayList<>();
        float[] group; String groupLabel;
        CombatModel.Fight sim; int simOffset; String simLabel;
        boolean simLife = true;
        int zero;
    }
    static final class Band { float[] box; String text; float textX, baseline; }
    static final class Label { String text; float x, baseline; boolean right; }
    static final class Marker { String icon; float x, y; }

    final List<Entry> legend = new ArrayList<>();
    final List<Label> labels = new ArrayList<>();
    final List<Marker> markers = new ArrayList<>();
    final List<float[]> shades = new ArrayList<>();
    final List<float[]> ticks = new ArrayList<>();
    final List<Integer> tickColours = new ArrayList<>();
    final List<Band> bands = new ArrayList<>();
    final List<float[]> stars = new ArrayList<>();
    float[] enemy, life, mana, grid, faint, axes, dash, group, simEnemy, simLife;
    float plotL, plotT, plotR, plotB, manaT, manaB, laneT, laneB, enemyT, enemyB, icon, row, endX, endY;
    float[] endBox;
    String endText;
    int endKind;
    boolean manaShown;
    final List<Integer> skills = new ArrayList<>();

    static String seconds(double t, boolean de) {
        String s = String.format(Locale.ROOT, "%.2f", t).replaceAll("0+$", "").replaceAll("\\.$", "");
        return (de ? s.replace('.', ',') : s) + " s";
    }
    static double step(double span) {
        for (double s : new double[]{0.25, 0.5, 1, 2, 5, 10, 15, 20, 30, 60, 120}) if (span / s <= 6) return s;
        return 120;
    }

    static int group(int kind) { return kind < 5 ? 0 : kind < 10 ? 1 : 2; }


    static String belt(int[] belt, String[] names) {
        int[] count = new int[3];
        for (int k = 0; belt != null && k < belt.length; k++) count[group(k)] += belt[k];
        StringBuilder s = new StringBuilder();
        for (int g = 0; g < 3; g++) {
            if (count[g] == 0) continue;
            if (s.length() > 0) s.append(" · ");
            s.append(names[g]).append(" ×").append(count[g]);
        }
        return s.toString();
    }



    static int[] tickColours(int[] actSkill) {
        java.util.TreeSet<Integer> ids = new java.util.TreeSet<>();
        for (int s : actSkill) if (s > 0) ids.add(s);
        java.util.Map<Integer, Integer> colour = new java.util.HashMap<>();
        boolean[] taken = new boolean[TICKS.length];
        for (int id : ids) {
            int c = id % TICKS.length;
            for (int k = 0; k < TICKS.length && taken[c]; k++) c = (c + 1) % TICKS.length;
            taken[c] = true;
            if (colour.size() + 1 >= TICKS.length) java.util.Arrays.fill(taken, false);
            colour.put(id, TICKS[c]);
        }
        int[] out = new int[actSkill.length];
        for (int i = 0; i < out.length; i++) out[i] = actSkill[i] > 0 ? colour.get(actSkill[i]) : ATTACK_TICK;
        return out;
    }


    static CombatGraph build(CombatModel.Fight f, float left, float top, float right, float bottom, float k, float textHeight, Measure measure,
                             Words words, String enemyName, String lifeLabel, String end, boolean de, int[] colours) {
        return build(f, left, top, right, bottom, k, textHeight, measure, words, enemyName, lifeLabel, end, de, colours, null);
    }
    static CombatGraph build(CombatModel.Fight f, float left, float top, float right, float bottom, float k, float textHeight, Measure measure,
                             Words words, String enemyName, String lifeLabel, String end, boolean de, int[] colours, Extra x) {
        CombatGraph g = new CombatGraph();
        g.endKind = f.endKind;
        float pad = 14 * k;
        g.icon = 22 * k;
        g.row = Math.max(textHeight * 1.3f, 30 * k);

        long seen = 0;
        for (int i = 0; i < f.n; i++) {
            long fresh = f.acts[i] & ~seen;
            for (int b = 0; b < 64; b++) if ((fresh >> b & 1) != 0 && f.actSkill[b] >= 0 && !g.skills.contains(f.actSkill[b])) g.skills.add(f.actSkill[b]);
            seen |= f.acts[i];
        }
        int[] tickColour = tickColours(f.actSkill), skillColour = new int[g.skills.size()];
        for (int i = 0; i < g.skills.size(); i++) for (int b = 0; b < 64; b++) if (f.actSkill[b] == g.skills.get(i)) skillColour[i] = tickColour[b];

        boolean shortage = false;
        for (int i = 0; i < f.n; i++) shortage |= f.shortage[i];
        shortage &= f.best >= 0;
        float manaLo = 100, manaHi = 0;
        for (int i = 0; i < f.n; i++) { manaLo = Math.min(manaLo, f.lo[CombatModel.MANA][i]); manaHi = Math.max(manaHi, f.hi[CombatModel.MANA][i]); }
        g.manaShown = shortage || manaHi - manaLo > 0.05f;
        int[] drunk = new int[3], strongest = {-1, -1, -1};
        for (CombatModel.Drink d : f.drinks) { int gr = group(d.kind); drunk[gr]++; strongest[gr] = Math.max(strongest[gr], d.kind); }


        List<Entry> entries = new ArrayList<>();
        entries.add(entry(LINE_ENEMY, colours[0], enemyName, null));
        if (x != null && x.group != null && x.groupLabel != null) entries.add(entry(LINE_GROUP, colours[0], x.groupLabel, null));
        entries.add(entry(LINE_LIFE, colours[1], lifeLabel, null));
        if (x != null && x.sim != null) entries.add(entry(LINE_SIM, x.simColour, x.simLabel, null));
        for (int i = 0; i < g.skills.size(); i++) {
            int id = g.skills.get(i);
            Entry e = entry(TICK, skillColour[i], words.skill(id), id > 0 && words.icon(id) ? "builds/skills/" + id : null);
            e.skill = id;
            entries.add(e);
        }
        if (shortage) entries.add(entry(SHADE, colours[2], words.shortage(words.skill(f.best)), null));
        int[] outcomes = new int[5];
        String laneName = null;
        if (x != null) {
            for (int i = 0; i < f.n; i++) {
                if ((x.enemy[i] & FightLog.EN_HIT) != 0) outcomes[0]++;
                if ((x.enemy[i] & FightLog.EN_MISS) != 0) outcomes[1]++;
                if ((x.enemy[i] & FightLog.EN_DODGE) != 0) outcomes[2]++;
                if ((x.enemy[i] & FightLog.EN_BLOCK) != 0) outcomes[3]++;
                if ((x.own[i] & FightLog.ME_CRIT) != 0) outcomes[4]++;
            }
            if (outcomes[4] > 0) entries.add(entry(STAR, x.starColour, x.crit + " ×" + outcomes[4], null));
            laneName = measure.width(x.enemyLane) <= measure.width("100 %") * 2.2f ? x.enemyLane : x.enemyLaneShort;
            String[] words4 = {x.hit, x.miss, x.dodge, x.block}; int[] colours4 = {x.hitColour, x.missColour, x.dodgeColour, x.blockColour}; boolean first = true;
            for (int o = 0; o < 4; o++) if (outcomes[o] > 0) { entries.add(entry(TICK, colours4[o], (first ? laneName + ": " : "") + words4[o] + " ×" + outcomes[o], null)); first = false; }
        }
        for (int gr = 0; gr < 3; gr++) if (drunk[gr] > 0) entries.add(entry(POTION, colours[2], words.potion(gr) + " ×" + drunk[gr], POTION_ICON[strongest[gr]]));
        float lx = left + pad, y = top + pad * 0.7f, gap = 22 * k;
        for (Entry e : entries) {
            float lead = e.kind == LINE_ENEMY || e.kind == LINE_GROUP || e.kind == LINE_LIFE || e.kind == LINE_SIM ? 30 * k : e.kind == TICK ? (e.icon != null ? 26 * k : 0) + 9 * k : e.kind == SHADE ? 28 * k : 26 * k;
            float w = lead + measure.width(e.text);
            if (lx + w > right - pad && lx > left + pad) { lx = left + pad; y += g.row; }
            e.x = lx; e.y = y; e.textX = lx + lead;
            lx += w + gap;
            g.legend.add(e);
        }
        float legendBottom = y + g.row;


        float labelW = Math.max(measure.width("100 %"), g.manaShown ? measure.width("Mana") : 0);
        if (x != null) labelW = Math.max(labelW, Math.max(measure.width(x.ownLane), measure.width(laneName)));
        labelW += 10 * k;
        g.plotL = left + pad + labelW;
        g.plotR = right - pad - 4 * k;
        g.plotT = legendBottom + 12 * k;
        if (f.endKind != CombatModel.END_NONE) {
            float boxT = legendBottom + 2 * k, w = measure.width(end);
            g.endBox = new float[]{g.plotR - w - 12 * k, boxT, g.plotR, boxT + textHeight * 1.2f + 4 * k};
            g.plotT = g.endBox[3] + 8 * k;
        }
        float labelsH = textHeight * 1.4f, lane = 14 * k;
        g.laneB = bottom - pad * 0.7f - labelsH;
        if (x != null) { lane = Math.max(lane, textHeight * 0.9f); g.enemyB = g.laneB; g.enemyT = g.enemyB - lane; g.laneB = g.enemyT - 4 * k; }
        g.laneT = g.laneB - lane;
        float above = g.laneT;
        int nband = x == null ? 0 : x.bandText.size();
        float bandRow = Math.max(22 * k, textHeight * 1.15f);
        if (nband > 0) above -= 6 * k + nband * (bandRow + 4 * k);
        if (x != null && !g.manaShown) above -= textHeight * 0.8f;
        if (g.manaShown) {
            g.manaB = above - 6 * k;
            float strip = Math.max(50 * k, Math.min(80 * k, (g.manaB - g.plotT) * 0.25f));
            g.manaT = g.manaB - strip;
            g.plotB = g.manaT - 12 * k;
        } else g.plotB = g.manaT = g.manaB = above - 6 * k;
        int span = Math.max(1, f.end);
        if (x != null && x.sim != null) span = Math.max(span, x.simOffset + x.sim.end);
        float pl = g.plotL, pw = g.plotR - g.plotL;
        float ph = g.plotB - g.plotT, mh = g.manaB - g.manaT;

        g.enemy = series(f, CombatModel.ENEMY, 0, span, pl, pw, g.plotB, ph);
        g.life = series(f, CombatModel.LIFE, 0, span, pl, pw, g.plotB, ph);
        g.mana = g.manaShown ? series(f, CombatModel.MANA, 0, span, pl, pw, g.manaB, mh) : new float[0];


        for (int i = 0; shortage && i < f.n; i++) {
            if (!f.shortage[i]) continue;
            int j = i;
            while (j + 1 < f.n && f.shortage[j + 1]) j++;
            float a = xAt(i * f.width, span, pl, pw), b = xAt(Math.min((j + 1) * f.width, span), span, pl, pw);
            g.shades.add(new float[]{a, g.manaT, Math.max(b, a + k), g.manaB});
            i = j;
        }

        for (int i = 0; i < f.n; i++) for (int b = 0; b < 64; b++) {
            if ((f.acts[i] >> b & 1) == 0) continue;
            float tx = xAt(i * f.width, span, pl, pw);
            g.ticks.add(new float[]{tx - k, g.laneT, tx + k, g.laneB});
            g.tickColours.add(tickColour[b]);
        }
        if (x != null) extras(g, f, x, span, pl, pw, ph, k, textHeight, bandRow, measure, laneName);

        float half = g.icon / 2;
        for (CombatModel.Drink d : f.drinks) {
            float dx = xAt(d.frame, span, pl, pw);
            int gr = group(d.kind);
            if (gr != 1) g.markers.add(marker(POTION_ICON[d.kind], dx, clamp(g.plotB - ph * d.life / 100, g.plotT + half, g.plotB - half)));
            if (gr != 0 && g.manaShown) g.markers.add(marker(POTION_ICON[d.kind], dx, clamp(g.manaB - mh * d.mana / 100, g.manaT + half, g.manaB - half)));
        }

        int zero = x != null ? x.zero : 0;
        double seconds = span / 25.0, st = step(seconds), first = -Math.floor(zero / 25.0 / st + 1e-9) * st;
        List<Float> grid = new ArrayList<>(), faint = new ArrayList<>();
        for (int p = 25; p <= 100; p += 25) line(grid, pl, g.plotB - ph * p / 100, g.plotR, g.plotB - ph * p / 100);
        if (g.manaShown) {
            line(grid, pl, g.manaT, g.plotR, g.manaT);
            line(faint, pl, g.manaB - mh / 2, g.plotR, g.manaB - mh / 2);
        }
        for (double t = first; t * 25 + zero <= span + 1e-6; t += st) {
            if (Math.abs(t * 25 + zero) < 1e-6) continue;
            float tx = xAt((int) Math.round(t * 25) + zero, span, pl, pw);
            line(faint, tx, g.plotT, tx, g.plotB);
            if (g.manaShown) line(faint, tx, g.manaT, tx, g.manaB);
        }
        g.grid = array(grid);
        g.faint = array(faint);
        g.axes = g.manaShown ? new float[]{pl, g.plotT, pl, g.plotB, pl, g.plotB, g.plotR, g.plotB, pl, g.manaT, pl, g.manaB, pl, g.manaB, g.plotR, g.manaB}
                             : new float[]{pl, g.plotT, pl, g.plotB, pl, g.plotB, g.plotR, g.plotB};
        float lift = textHeight * 0.35f;
        for (int p = 0; p <= 100; p += 50) g.labels.add(label(p + " %", pl - 8 * k, g.plotB - ph * p / 100 + lift, true));
        if (g.manaShown) g.labels.add(label("Mana", pl - 8 * k, (g.manaT + g.manaB) / 2 + lift, true));
        float labelBase = (x != null ? g.enemyB : g.laneB) + textHeight * 1.15f, spanX = xAt(span, span, pl, pw);
        for (double t = first; t * 25 + zero <= span + 1e-6; t += st) {
            String s = seconds(Math.abs(t) < 1e-9 ? 0 : t, de);
            float tx = xAt((int) Math.round(t * 25) + zero, span, pl, pw), w = measure.width(s);
            if (t == first || spanX - (tx + w / 2) > 40 * k) g.labels.add(label(s, Math.max(tx - w / 2, pl - w / 2), labelBase, false));
        }

        if (g.endBox != null) {
            float endXp = xAt(f.end, span, pl, pw);
            if (endXp < g.endBox[2]) { float w = g.endBox[2] - g.endBox[0]; g.endBox[0] = Math.max(pl, endXp - w); g.endBox[2] = g.endBox[0] + w; }
            g.endX = endXp;
            g.endY = g.plotB;
            List<Float> dash = new ArrayList<>();
            dashes(dash, g.endX, g.endBox[3], g.plotB, k);
            if (g.manaShown) dashes(dash, g.endX, g.manaT, g.manaB, k);
            g.dash = array(dash);
            g.endText = end;
        }
        return g;
    }
    private static void dashes(List<Float> out, float x, float from, float to, float k) {
        for (float y = from; y < to; y += 8 * k) line(out, x, y, x, Math.min(y + 4 * k, to));
    }

    private static Entry entry(int kind, int colour, String text, String icon) { Entry e = new Entry(); e.kind = kind; e.colour = colour; e.text = text; e.icon = icon; return e; }
    private static Marker marker(String icon, float x, float y) { Marker m = new Marker(); m.icon = icon; m.x = x; m.y = y; return m; }
    private static Label label(String text, float x, float baseline, boolean right) { Label l = new Label(); l.text = text; l.x = x; l.baseline = baseline; l.right = right; return l; }
    private static float clamp(float v, float a, float b) { return Math.max(a, Math.min(b, v)); }
    private static float xAt(int frame, int span, float left, float width) { return left + width * Math.min(frame, span) / (float) span; }


    private static void extras(CombatGraph g, CombatModel.Fight f, Extra x, int span, float pl, float pw, float ph, float k, float textHeight, float bandRow,
                               Measure measure, String laneName) {
        float lift = textHeight * 0.35f;
        g.labels.add(label(x.ownLane, pl - 8 * k, (g.laneT + g.laneB) / 2 + lift, true));
        g.labels.add(label(laneName, pl - 8 * k, (g.enemyT + g.enemyB) / 2 + lift, true));
        int[] bits = {FightLog.EN_HIT, FightLog.EN_BLOCK, FightLog.EN_MISS, FightLog.EN_DODGE}, colour = {x.hitColour, x.blockColour, x.missColour, x.dodgeColour};
        for (int i = 0; i < f.n; i++) {
            int n = 0;
            for (int o = 0; o < bits.length; o++) {
                if ((x.enemy[i] & bits[o]) == 0) continue;
                float tx = xAt(i * f.width, span, pl, pw) + n++ * 3 * k;
                g.ticks.add(new float[]{tx - k, g.enemyT, tx + k, g.enemyB});
                g.tickColours.add(colour[o]);
            }
            if ((x.own[i] & FightLog.ME_CRIT) != 0) g.stars.add(star(xAt(i * f.width, span, pl, pw), (g.laneT + g.laneB) / 2, (g.laneB - g.laneT) * 0.75f));
        }
        float top = g.laneT - 6 * k - x.bandText.size() * (bandRow + 4 * k);
        for (int i = 0; i < x.bandText.size(); i++) {
            int[] t = x.bandSpan.get(i);
            float a = xAt(t[0], span, pl, pw), b = Math.max(xAt(t[1], span, pl, pw), a + 2 * k), y = top + i * (bandRow + 4 * k);
            Band band = new Band(); band.box = new float[]{a, y, b, y + bandRow}; band.text = x.bandText.get(i);
            float w = measure.width(band.text);
            band.textX = b - a >= w + 12 * k ? a + 6 * k : b + 6 * k + w <= g.plotR ? b + 6 * k : Math.max(pl, a - 6 * k - w);
            band.baseline = y + bandRow * 0.5f + lift;
            g.bands.add(band);
        }
        if (x.group != null) {
            CombatModel.Fight one = new CombatModel.Fight(); one.n = f.n; one.width = f.width;
            one.lo = new float[][]{x.group}; one.hi = one.lo; one.last = one.lo;
            g.group = series(one, 0, 0, span, pl, pw, g.plotB, ph);
        }
        if (x.sim != null) {
            List<Float> e = new ArrayList<>(), l = new ArrayList<>();
            dashed(e, series(x.sim, CombatModel.ENEMY, x.simOffset, span, pl, pw, g.plotB, ph), 7 * k, 5 * k);
            if (x.simLife) dashed(l, series(x.sim, CombatModel.LIFE, x.simOffset, span, pl, pw, g.plotB, ph), 7 * k, 5 * k);
            g.simEnemy = array(e); g.simLife = array(l);
        }
        for (Entry e : g.legend) if (e.kind == STAR) g.stars.add(star(e.x + g.icon / 2, e.y + g.row / 2, g.icon * 0.45f));
    }

    static float[] star(float cx, float cy, float r) {
        float[] p = new float[20];
        for (int i = 0; i < 10; i++) {
            double a = -Math.PI / 2 + i * Math.PI / 5; float rr = i % 2 == 0 ? r : r * 0.42f;
            p[2 * i] = cx + (float) (rr * Math.cos(a)); p[2 * i + 1] = cy + (float) (rr * Math.sin(a));
        }
        return p;
    }

    static void dashed(List<Float> out, float[] xy, float on, float off) {
        float phase = 0; boolean drawing = true;
        for (int i = 0; i + 3 < xy.length; i += 2) {
            float ax = xy[i], ay = xy[i + 1], bx = xy[i + 2], by = xy[i + 3], len = (float) Math.hypot(bx - ax, by - ay), at = 0;
            while (at < len) {
                float step = Math.min((drawing ? on : off) - phase, len - at);
                if (drawing) line(out, ax + (bx - ax) * at / len, ay + (by - ay) * at / len, ax + (bx - ax) * (at + step) / len, ay + (by - ay) * (at + step) / len);
                at += step; phase += step;
                if (phase >= (drawing ? on : off) - 1e-4f) { phase = 0; drawing = !drawing; }
            }
        }
    }
    private static void line(List<Float> out, float a, float b, float c, float d) { out.add(a); out.add(b); out.add(c); out.add(d); }
    private static float[] array(List<Float> l) { float[] a = new float[l.size()]; for (int i = 0; i < a.length; i++) a[i] = l.get(i); return a; }



    private static float[] series(CombatModel.Fight f, int s, int offset, int span, float left, float width, float base, float height) {
        List<Float> p = new ArrayList<>();
        float prev = f.hi[s][0];
        p.add(xAt(offset, span, left, width)); p.add(base - height * prev / 100);
        for (int i = 0; i < f.n; i++) {
            float x0 = xAt(offset + i * f.width, span, left, width), x1 = xAt(Math.min(offset + (i + 1) * f.width, span), span, left, width);
            float lo = f.lo[s][i], hi = f.hi[s][i], last = f.last[s][i];
            if (lo != prev || hi != prev || last != prev) {
                boolean down = last - lo < hi - last;
                for (float v : new float[]{down ? hi : lo, down ? lo : hi, last}) { p.add(x0); p.add(base - height * v / 100); }
            }
            p.add(x1); p.add(base - height * last / 100);
            prev = last;
        }
        return array(p);
    }
}
