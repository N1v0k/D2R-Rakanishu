package com.xiaoboluo.d2r;

import java.util.Locale;



final class EffectFormat {
    static final int STAT_MANA_RECOVERY = 26, STAT_MANA_BONUS = 27, STAT_LIFE_REGEN = 74;
    static final int TICKS = 25;




    static final int VERSION = 4, HEAD = 288, MAX = 232, STATS = 12, ENTRY = 6, WORDS = HEAD + MAX * (ENTRY + STATS * 3);
    static final int GEN = 0, COUNT = 1, TICK = 2, VER = 3, MANA = 4, PLAYER = 8, SKILLS = 56, COMBAT = 256;

    static final int P_FLAGS = 0, P_SESSION = 1, P_UNIT = 2, P_CLASS = 3, P_AGE = 4, P_DIFF = 5, P_CLASSIC = 6, P_MODE = 7, P_STAND = 8, P_WALK = 9, P_RUN = 10, P_STATS = 12;
    static final int PF_VALID = 1, PF_DIFF = 2, PF_CLASSIC = 4, PF_STAMINA = 8;
    static final int[] PLAYER_STATS = {0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12, 13, 26, 27, 28, 74, 36, 39, 40, 41, 42, 43, 44, 45, 46, 60, 62, 79, 80, 85};

    static final int S_FLAGS = 0, S_SESSION = 1, S_UNIT = 2, S_CLASS = 3, S_AGE = 4, S_COUNT = 5, S_DROPPED = 6, S_ENTRIES = 8, S_MAX = 64;
    static final int SF_VALID = 1, SF_SHRINE = 2, SF_PLUS2 = 4;



    static final int C_FLAGS = 0, C_SESSION = 1, C_UNIT = 2, C_AGE = 3, C_STATE = 4, C_V171 = 5, C_V19 = 6, C_V25 = 7, C_VALUES = 8, C_LEN = 16, C_SLOTS = 2;
    static final int CF_VALID = 1, CF_DEF = 2, CF_AR = 4, CF_DMG = 8;



    static boolean blocks(int[] words, int len, EffectDetail.Player p, EffectDetail.Skills s) { return blocks(words, len, p, s, (EffectDetail.Combat[]) null); }


    static boolean blocks(int[] words, int len, EffectDetail.Player p, EffectDetail.Skills s, EffectDetail.Combat... cs) {
        p.flags = 0; s.flags = 0; s.count = 0; s.dropped = 0;
        if (cs != null) for (EffectDetail.Combat c : cs) if (c != null) c.flags = 0;
        if (len < HEAD || words[VER] != VERSION) return false;
        int b = PLAYER;
        p.flags = words[b + P_FLAGS]; p.session = words[b + P_SESSION]; p.unit = words[b + P_UNIT]; p.cls = words[b + P_CLASS]; p.age = words[b + P_AGE];
        p.diff = words[b + P_DIFF]; p.classic = words[b + P_CLASSIC]; p.mode = words[b + P_MODE]; p.stand = words[b + P_STAND]; p.walk = words[b + P_WALK]; p.run = words[b + P_RUN];
        System.arraycopy(words, b + P_STATS, p.stats, 0, PLAYER_STATS.length);
        b = SKILLS;
        s.flags = words[b + S_FLAGS]; s.session = words[b + S_SESSION]; s.unit = words[b + S_UNIT]; s.cls = words[b + S_CLASS]; s.age = words[b + S_AGE];
        s.count = Math.max(0, Math.min(S_MAX, words[b + S_COUNT]));
        for (int i = 0; i < s.count; i++) { s.id[i] = words[b + S_ENTRIES + 3 * i]; s.base[i] = words[b + S_ENTRIES + 3 * i + 1]; s.bonus[i] = words[b + S_ENTRIES + 3 * i + 2]; }
        s.dropped = words[b + S_DROPPED];
        if (cs != null) for (int i = 0; i < cs.length && i < C_SLOTS; i++) {
            EffectDetail.Combat c = cs[i];
            if (c == null) continue;
            b = COMBAT + i * C_LEN;
            c.slot = i;
            c.flags = words[b + C_FLAGS]; c.session = words[b + C_SESSION]; c.unit = words[b + C_UNIT]; c.age = words[b + C_AGE]; c.state = words[b + C_STATE];
            c.v171 = words[b + C_V171]; c.v19 = words[b + C_V19]; c.v25 = words[b + C_V25];
            System.arraycopy(words, b + C_VALUES, c.values, 0, c.values.length);
        }
        return true;
    }
    private EffectFormat() {}



    static int manaPerTick(int base, int bonus) {
        int percent = bonus + 100;
        if (base > 0x100000) return base / 100 * percent;
        if (percent > 0x10000) return percent / 100 * base;
        return base * percent / 100;
    }

    static double perSecond(int perTick) { return perTick * (double) TICKS / 256; }


    static String number(double v, boolean german) {
        String s = String.format(Locale.ROOT, "%.1f", v);
        if (s.endsWith(".0")) s = s.substring(0, s.length() - 2);
        return german ? s.replace('.', ',') : s;
    }

    static String signed(double v, boolean german) { return (v > 0 ? "+" : "") + number(v, german); }



    static int seconds(int ticks) { return ticks <= 0 ? 0 : (ticks - 1) / TICKS + 1; }




    static final int BOX_SECONDS = 99, BOX_MINUTES = 99, WARN_SECONDS = 5;



    static int boxText(int ticks, String minutes, char[] out) {
        if (ticks < 0) return 0;
        int s = seconds(ticks);
        if (s <= BOX_SECONDS) return digits(s, out, 0);
        int m = (s - 1) / 60 + 1, at = minutes == null ? -1 : minutes.indexOf("%d");
        if (m > BOX_MINUTES || at < 0 || minutes.indexOf('%', at + 2) >= 0 || minutes.length() + 1 > out.length) return 0;
        minutes.getChars(0, at, out, 0);
        int end = digits(m, out, at);
        minutes.getChars(at + 2, minutes.length(), out, end);
        return end + minutes.length() - at - 2;
    }


    static int boxCode(int ticks) {
        if (ticks < 0) return -1;
        int s = seconds(ticks);
        if (s <= BOX_SECONDS) return s * 2 + (warn(ticks) ? 1 : 0);
        int m = (s - 1) / 60 + 1;
        return m > BOX_MINUTES ? -1 : 1000 + m;
    }

    static boolean warn(int ticks) { return ticks >= 0 && seconds(ticks) <= WARN_SECONDS; }


    static int digits(int v, char[] out, int at) {
        int n = Math.max(0, v), len = 1;
        for (int t = n; t >= 10; t /= 10) len++;
        for (int i = at + len - 1; i >= at; i--) { out[i] = (char) ('0' + n % 10); n /= 10; }
        return at + len;
    }


    static int plus(int n, char[] out) { out[0] = '+'; return digits(n, out, 1); }



    static String apply(String format, int value) {
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < format.length(); i++) {
            char c = format.charAt(i);
            if (c != '%') { out.append(c); continue; }
            if (format.startsWith("%%", i)) { out.append('%'); i++; }
            else if (format.startsWith("%+d", i)) { out.append(value >= 0 ? "+" : "").append(value); i += 2; }
            else if (format.startsWith("%d", i)) { out.append(value); i++; }
            else return null;
        }
        return out.toString();
    }
}
