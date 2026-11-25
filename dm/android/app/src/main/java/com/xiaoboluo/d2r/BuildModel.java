




package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

final class BuildModel {
    private BuildModel() {}


    static final class Text {
        final String de, en;
        Text(String de, String en) { this.de = de; this.en = en; }
        String get(boolean german) { return german ? de : en; }
    }

    static final class Step {
        int lvl, skill, to;
        boolean save;
        String key = "";
        Text note;
    }

    static final class StatRule {
        String stat = "";
        int to = -1;
        boolean rest, none;
        Text why;
    }

    static final class Phase {
        String id = "";
        Text name, note;
        int lo, hi;
        boolean respec;
        final List<Step> skills = new ArrayList<>();
        final List<StatRule> stats = new ArrayList<>();
    }


    static final class Gear {
        String phase = "", slot = "", tier = "", kind = "", key = "", quality = "", baseItem = "", baseType = "";
        int sockets;
        boolean eth;
        Text note;
    }

    static final class Merc {
        int act, difficulty;
        String aura = "";
        Text why;
        final List<Gear> gear = new ArrayList<>();
    }

    static final class Source {
        String site = "", title = "", url = "", seen = "", patch = "", kind = "";
        Text note;
    }

    static final class Guide {
        String id = "", cls = "", patch = "";
        int version, classIndex, difficulty;
        Text title, summary;
        final List<Text> strengths = new ArrayList<>(), weaknesses = new ArrayList<>(), notes = new ArrayList<>();
        final List<String> tags = new ArrayList<>();
        int[] keySkills = new int[0], trees = new int[0];
        final List<Phase> phases = new ArrayList<>();
        final List<Gear> gear = new ArrayList<>();
        Merc merc;
        final List<Source> sources = new ArrayList<>();
        final Set<String> checks = new HashSet<>();
        boolean played;
        String profile = "{}";
        final Map<String, String> phaseProfiles = new HashMap<>();

        int phaseIndex(String id) { for (int i = 0; i < phases.size(); i++) if (phases.get(i).id.equals(id)) return i; return -1; }
    }


    static final class SkillInfo {
        int id, page, row, col, req, max, icon;
        int[] pre = new int[0], syn = new int[0];
        String key = "", cls = "", de = "", en = "", nameKey = "";
        String name(boolean german) { return german ? de : en; }
    }

    static final class ItemInfo {
        String de = "", en = "", img = "", code = "", type = "";
        int lvl, str, dex, sockets;
        final List<String> runes = new ArrayList<>();
        final List<String> up = new ArrayList<>();




        int row = -1, gat = -1;
        int[] ac, d1, d2;
        Text shortName;
        boolean ease;
        String name(boolean german) { return german ? de : en; }
    }

    static final class ClassInfo {
        int index;
        String de = "", en = "";
        final String[] tabsDe = new String[4], tabsEn = new String[4];
        String name(boolean german) { return german ? de : en; }
        String tab(int page, boolean german) { return page >= 1 && page <= 3 ? (german ? tabsDe : tabsEn)[page] : ""; }
    }

    static final class Names {
        final Map<Integer, SkillInfo> skills = new HashMap<>();
        final Map<String, ItemInfo> items = new HashMap<>();
        final Map<String, ClassInfo> classes = new HashMap<>();
        final Map<String, Text> stats = new HashMap<>(), qualities = new HashMap<>(), auras = new HashMap<>();
        final Map<String, Text> labels = new HashMap<>();
        final List<Text> difficulties = new ArrayList<>();

        ClassInfo byIndex(int index) { for (ClassInfo c : classes.values()) if (c.index == index) return c; return null; }
    }







    static final int SNAP_HEAD = 17, SNAP_MAX_SKILLS = 40, SNAP_INTS = SNAP_HEAD + 3 * SNAP_MAX_SKILLS;

    static final class Snap {
        boolean valid;
        int session, cls = -1, level, dataVersion, statPoints, skillPoints, request;
        final int[] attr = new int[4], baseAttr = {-1, -1, -1, -1};
        String name = "";
        final Map<Integer, Integer> base = new HashMap<>(), reason = new HashMap<>();



        Map<String, Integer> baseMap() {
            Map<String, Integer> m = new HashMap<>();
            String[] keys = {"str", "dex", "vit", "ene"};
            for (int i = 0; i < 4; i++) { if (baseAttr[i] < 0) return null; m.put(keys[i], baseAttr[i]); }
            return m;
        }

        static Snap parse(int[] out, int count, byte[] name, int nameLength) {
            Snap s = new Snap();
            if (out == null || count < SNAP_HEAD || out[0] == 0) return s;
            s.valid = true;
            s.session = out[1]; s.cls = out[2]; s.level = out[3]; s.dataVersion = out[4]; s.statPoints = out[5]; s.skillPoints = out[6];
            for (int i = 0; i < 4; i++) { s.attr[i] = out[7 + i]; s.baseAttr[i] = out[13 + i]; }
            s.request = out[11];
            int n = Math.min(out[12], Math.min(SNAP_MAX_SKILLS, (count - SNAP_HEAD) / 3));
            for (int i = 0; i < n; i++) { s.base.put(out[SNAP_HEAD + 3 * i], out[SNAP_HEAD + 3 * i + 1]); s.reason.put(out[SNAP_HEAD + 3 * i], out[SNAP_HEAD + 3 * i + 2]); }
            if (name != null && nameLength > 0) s.name = new String(name, 0, Math.min(nameLength, name.length), java.nio.charset.StandardCharsets.UTF_8);
            if (s.name.isEmpty() || s.cls < 0 || s.cls > 7) s.valid = false;
            return s;
        }
    }


    static final class Follow {
        final String guide, phase;
        final int version;
        Follow(String guide, int version, String phase) { this.guide = guide; this.version = version; this.phase = phase == null || phase.isEmpty() ? "auto" : phase; }

        static String key(String name, int classIndex) { return name + "#" + classIndex; }
        static String encode(Follow e) { return e.guide + "\t" + e.version + "\t" + e.phase; }
        static Follow decode(String s) {
            if (s == null) return null;
            String[] f = s.split("\t", -1);
            if (f.length != 3 || f[0].isEmpty()) return null;
            try { return new Follow(f[0], Integer.parseInt(f[1]), f[2]); } catch (NumberFormatException e) { return null; }
        }
    }
}
