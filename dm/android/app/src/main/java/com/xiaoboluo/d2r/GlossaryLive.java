package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.List;






final class GlossaryLive {
    static final class Line {
        final String head, value, tail, colour;
        Line(String head, String value, String tail) { this(head, value, tail, "FontColorBlue"); }
        Line(String head, String value, String tail, String colour) { this.head = head; this.value = value; this.tail = tail; this.colour = colour; }
        Line compared(int before, int after, boolean lowerBetter) {
            String c = before == after ? EffectDetail.GREY : (after > before) != lowerBetter ? EffectDetail.BETTER : EffectDetail.WORSE;
            return new Line(head, value, tail, c);
        }
        Line slice(int start, int end) {
            String text = text();
            int a = Math.max(start, Math.min(end, head.length()));
            int b = Math.max(a, Math.min(end, head.length() + value.length()));
            return new Line(text.substring(start, a), text.substring(a, b), text.substring(b, end), colour);
        }
        String text() { return head + value + tail; }
    }



    static final class Context {
        final int state, stat, value;
        Context(int state, int stat, int value) { this.state = state; this.stat = stat; this.value = value; }
    }



    interface Game {
        int penalty(int diff, boolean classic);
        String resistName(int stat, boolean german);
    }

    static final int ARMOR_SHRINE = 100, ARMOR_SHRINE_STATE = 128;
    static final int UNUSED_ = 0;

    private final GlossaryModel model;
    private final boolean de;
    GlossaryLive(GlossaryModel model, boolean german) { this.model = model; this.de = german; }

    Line line(String key, Object... args) {
        String f = model.phrase(key, de);
        for (int i = 0; i < args.length; i++) f = f.replace("{" + i + "}", String.valueOf(args[i]));
        int a = f.indexOf('['), b = f.indexOf(']');
        if (a < 0 || b < a) return new Line(f, "", "");
        return new Line(f.substring(0, a), f.substring(a + 1, b), f.substring(b + 1));
    }

    List<Line> lines(GlossaryModel.Term t, GlossaryCalc.Snap s, GlossaryCalc.Monsters m, Game g, Context ctx) {
        List<Line> out = new ArrayList<>();
        if (s == null || !s.valid) { out.add(line("noLive")); return out; }
        int mlvl = GlossaryCalc.monsterLevel(s.level);
        switch (t.id) {
            case "defense": {
                out.add(line("def", s.def));
                int hit = monsterHits(s, m, s.def, mlvl);
                if (hit >= 0) out.add(line("hitYou", mlvl, hit));
                break;
            }
            case "enhdef": enhdef(out, s, m, ctx, mlvl); break;
            case "block":
                if (s.block <= 0) out.add(line("noBlock"));
                else out.add(line("block", s.block, s.block / 3));
                break;
            case "resist":
                difficulty(out, s);
                for (int r = 0; r < EffectDetail.RESISTS.length - 1; r++) resist(out, s, g, r);
                break;
            case "poison":
                difficulty(out, s);
                resist(out, s, g, 3);
                out.add(line("plr", s.stat(110)));
                break;
            case "dr": out.add(line("dr", s.stat(34), s.stat(36))); break;
            case "mdr": out.add(line("mdr", s.stat(35))); break;
            case "ar": {
                if (s.ar < 0) { out.add(line("arNone")); break; }
                out.add(line("ar", s.ar));
                int mdef = m == null ? -1 : m.def(s.expansion, s.diff, mlvl);
                int hit = mdef < 0 ? -1 : GlossaryCalc.hit(s.ar, mdef, s.level, mlvl);
                if (hit >= 0) out.add(line("hitIt", mlvl, hit));
                break;
            }
            case "deadly": out.add(line("value", s.stat(141))); break;
            case "openwounds": out.add(line("value", s.stat(135))); break;
            case "crushing": out.add(line("value", s.stat(136))); break;
            case "ias": out.add(line("value", s.stat(93))); break;
            case "leech": {
                int life = s.stat(60), mana = s.stat(62);
                out.add(line("leech", life, mana));
                int dl = m == null ? -1 : m.leech(s.diff, 0), dm = m == null ? -1 : m.leech(s.diff, 1);
                if (dl > 1 && dm > 1) out.add(line("leechDiff", diffName(s.diff), GlossaryCalc.tenths(life * 10 / dl, de), GlossaryCalc.tenths(mana * 10 / dm, de)));
                break;
            }
            case "fcr": speed(out, s, model.fcr, s.stat(105), "cast"); break;
            case "fhr": speed(out, s, model.fhr, s.stat(99), "recover"); break;
            default: break;
        }
        return out;
    }

    private int monsterHits(GlossaryCalc.Snap s, GlossaryCalc.Monsters m, int def, int mlvl) {
        int mar = m == null ? -1 : m.ar(s.expansion, s.diff, mlvl);
        return mar < 0 ? -1 : GlossaryCalc.hit(mar, def, mlvl, s.level);
    }

    private void enhdef(List<Line> out, GlossaryCalc.Snap s, GlossaryCalc.Monsters m, Context ctx, int mlvl) {
        out.add(line("def", s.def));
        if (s.skillPct != 0) out.add(line("pct", s.skillPct));
        if (!GlossaryCalc.defenseModelHolds(s)) { out.add(line("noModel")); return; }
        int now = monsterHits(s, m, s.def, mlvl);


        int bonus = ctx != null && ctx.state == ARMOR_SHRINE_STATE && ctx.stat == 171 && ctx.value > 0 ? ctx.value : ARMOR_SHRINE;
        if (s.armorShrine && s.skillPct >= bonus) {
            int without = GlossaryCalc.defense(s.ac, s.dex, s.acpct - bonus, s.acovr), before = monsterHits(s, m, without, mlvl);
            out.add(line("defWith", without).compared(s.def, without, false));
            if (now >= 0 && before >= 0) out.add(line("hitYouWith", mlvl, now, before).compared(before, now, true));
        } else if (!s.armorShrine) {
            int with = GlossaryCalc.defense(s.ac, s.dex, s.acpct + ARMOR_SHRINE, s.acovr), after = monsterHits(s, m, with, mlvl);
            out.add(line("defWould", ARMOR_SHRINE, with).compared(s.def, with, false));
            if (now >= 0 && after >= 0) out.add(line("hitYouWould", mlvl, after, now).compared(now, after, true));
        }
    }

    private String diffName(int diff) { return diff >= 0 && diff <= 2 ? model.phrase("diff" + diff, de) : "?"; }

    private void difficulty(List<Line> out, GlossaryCalc.Snap s) { if (s.diff >= 0) out.add(line("diff", diffName(s.diff))); }

    private void resist(List<Line> out, GlossaryCalc.Snap s, Game g, int r) {
        int total = s.stat(EffectDetail.RESISTS[r]), max = s.stat(EffectDetail.MAX_RESISTS[r]);
        int pen = g == null || s.diff < 0 || s.expansion < 0 ? Integer.MIN_VALUE : g.penalty(s.diff, s.expansion == 0);
        String name = g == null ? null : g.resistName(EffectDetail.RESISTS[r], de);
        if (name == null || name.isEmpty()) name = "#" + EffectDetail.RESISTS[r];
        if (pen == Integer.MIN_VALUE) { out.add(line("res", name, "?", total, "?", EffectDetail.resCap(max))); return; }
        out.add(line("res", name, EffectDetail.resShown(total, max, pen), total, pen, EffectDetail.resCap(max)));
    }

    private void speed(List<Line> out, GlossaryCalc.Snap s, int[][][] tables, int value, String key) {
        int[][] t = s.cls >= 0 && s.cls < tables.length ? tables[s.cls] : null;
        int[] st = GlossaryModel.step(t, value);
        if (st == null) { out.add(line("noClass")); return; }
        out.add(line(key, value, st[0], GlossaryCalc.seconds(st[0], de)));
        if (st[1] < 0) out.add(line("top"));
        else out.add(line("next", st[1], st[2], st[1] - value));
    }
}
