package com.xiaoboluo.d2r;




public class EffectFormatTest {
    private static void equal(Object want, Object actual) {
        if (!want.equals(actual)) throw new AssertionError("want=" + want + " actual=" + actual);
    }
    private static String box(int ticks, String minutes) { char[] b = new char[16]; return new String(b, 0, EffectFormat.boxText(ticks, minutes, b)); }

    public static void main(String[] args) {
        equal(1000, EffectFormat.manaPerTick(1000, 0));
        equal(5000, EffectFormat.manaPerTick(1000, 400));
        equal(0, EffectFormat.manaPerTick(0, 400));
        equal(0x200000 / 100 * 200, EffectFormat.manaPerTick(0x200000, 100));
        equal(0x10000 * 0x10000 / 100, EffectFormat.manaPerTick(0x10000, 0x10000 - 100));
        equal(25.0, EffectFormat.perSecond(256));
        equal("-2,5", EffectFormat.signed(-2.5, true));
        equal("+2.5", EffectFormat.signed(2.5, false));
        equal("3", EffectFormat.number(3.0, true));
        equal(1, EffectFormat.seconds(1));
        equal(1, EffectFormat.seconds(25));
        equal(2, EffectFormat.seconds(26));
        equal("+40 x", EffectFormat.apply("%+d x", 40));
        equal("-4 x", EffectFormat.apply("%+d x", -4));
        equal("7% y", EffectFormat.apply("%d%% y", 7));
        equal(null == EffectFormat.apply("%s", 1), true);
        equal(0, EffectFormat.seconds(0));
        equal("Noch 72 Sek.", EffectFormat.apply("Noch %d Sek.", 72));
        equal(10032, EffectFormat.WORDS);
        equal(30, EffectFormat.PLAYER_STATS.length);
        equal(true, EffectFormat.PLAYER + EffectFormat.P_STATS + 30 <= EffectFormat.SKILLS && EffectFormat.SKILLS + EffectFormat.S_ENTRIES + 3 * EffectFormat.S_MAX <= EffectFormat.HEAD);

        int[] w = new int[EffectFormat.WORDS];
        EffectDetail.Player p = new EffectDetail.Player(); EffectDetail.Skills s = new EffectDetail.Skills();
        w[EffectFormat.VER] = 1; w[EffectFormat.PLAYER] = 1;
        equal(false, EffectFormat.blocks(w, w.length, p, s));
        equal(0, p.flags);
        w[EffectFormat.VER] = EffectFormat.VERSION; w[EffectFormat.PLAYER + EffectFormat.P_UNIT] = 42; w[EffectFormat.PLAYER + EffectFormat.P_STATS + 17] = 75;
        w[EffectFormat.SKILLS + EffectFormat.S_FLAGS] = EffectFormat.SF_VALID; w[EffectFormat.SKILLS + EffectFormat.S_COUNT] = 99;
        w[EffectFormat.SKILLS + EffectFormat.S_ENTRIES + 3] = 394; w[EffectFormat.SKILLS + EffectFormat.S_ENTRIES + 4] = 6; w[EffectFormat.SKILLS + EffectFormat.S_ENTRIES + 5] = 2;
        equal(true, EffectFormat.blocks(w, w.length, p, s));
        equal(true, p.valid() && p.unit == 42 && p.stat(39) == 75 && p.stat(85) == 0);
        equal(64, s.count);
        equal(8, s.effective(394));
        equal(false, EffectFormat.blocks(w, EffectFormat.HEAD - 1, p, s));

        equal(true, EffectFormat.SKILLS + EffectFormat.S_ENTRIES + 3 * EffectFormat.S_MAX <= EffectFormat.COMBAT && EffectFormat.COMBAT + EffectFormat.C_SLOTS * EffectFormat.C_LEN <= EffectFormat.HEAD);
        EffectDetail.Combat c = new EffectDetail.Combat(), c2 = new EffectDetail.Combat();
        w[EffectFormat.COMBAT + EffectFormat.C_FLAGS] = EffectFormat.CF_VALID | EffectFormat.CF_DEF; w[EffectFormat.COMBAT + EffectFormat.C_STATE] = 128; w[EffectFormat.COMBAT + EffectFormat.C_V171] = 100;
        w[EffectFormat.COMBAT + EffectFormat.C_VALUES] = 34; w[EffectFormat.COMBAT + EffectFormat.C_VALUES + 1] = 68; w[EffectFormat.SKILLS + EffectFormat.S_DROPPED] = 3;
        int b2 = EffectFormat.COMBAT + EffectFormat.C_LEN;
        w[b2 + EffectFormat.C_FLAGS] = EffectFormat.CF_VALID | EffectFormat.CF_AR | EffectFormat.CF_DMG; w[b2 + EffectFormat.C_STATE] = 129; w[b2 + EffectFormat.C_V19] = 276; w[b2 + EffectFormat.C_V25] = 200;
        w[b2 + EffectFormat.C_VALUES + 2] = 133; w[b2 + EffectFormat.C_VALUES + 3] = 533;
        equal(true, EffectFormat.blocks(w, w.length, p, s, c, c2));
        equal(true, c.state == 128 && c.v171 == 100 && c.values[0] == 34 && c.values[1] == 68 && (c.flags & EffectFormat.CF_DEF) != 0 && s.dropped == 3);
        equal(true, c2.state == 129 && c2.v19 == 276 && c2.v25 == 200 && c2.v171 == 0 && c2.values[2] == 133 && c2.values[3] == 533 && c2.values[0] == 0);
        equal(true, c.slot == 0 && c2.slot == 1);
        w[EffectFormat.VER] = 2; equal(false, EffectFormat.blocks(w, w.length, p, s, c, c2)); equal(0, c.flags & c2.flags);
        int[] z = new int[EffectFormat.HEAD]; z[EffectFormat.VER] = EffectFormat.VERSION;
        equal(true, EffectFormat.blocks(z, z.length, p, s, c, null));
        char[] buf = new char[8];
        equal("+5", new String(buf, 0, EffectFormat.plus(5, buf)));
        equal("+12", new String(buf, 0, EffectFormat.plus(12, buf)));
        equal(4, EffectFormat.digits(0, buf, 3));
        equal('0', buf[3]);
        equal("+1907", new String(buf, 0, EffectFormat.plus(1907, buf)));


        String de = "%d Min.";
        equal("99", box(99 * 25, de));
        equal("2 Min.", box(100 * 25, de));
        equal("2 Min.", box(101 * 25, de));
        equal("2 Min.", box(120 * 25, de));
        equal("3 Min.", box(121 * 25, de));
        equal("86", box(86 * 25 - 3, de));
        equal("99 Min.", box(99 * 60 * 25, de));
        equal("", box(99 * 60 * 25 + 1, de));
        equal("", box(Integer.MAX_VALUE, de));
        equal("", box(-1, de));
        equal("2 minutes", box(100 * 25, "%d minutes"));
        equal("", box(100 * 25, ""));
        equal("", box(100 * 25, "%d %d"));
        equal("5", box(5 * 25, de));
        equal(true, EffectFormat.warn(5 * 25));
        equal(false, EffectFormat.warn(5 * 25 + 1));
        equal(false, EffectFormat.warn(-1));
        equal(11, EffectFormat.boxCode(5 * 25));
        equal(12, EffectFormat.boxCode(6 * 25));
        equal(1002, EffectFormat.boxCode(100 * 25));
        equal(-1, EffectFormat.boxCode(-1));
        equal(-1, EffectFormat.boxCode(Integer.MAX_VALUE));
        equal(85899346, EffectFormat.seconds(Integer.MAX_VALUE));
        System.out.println("EffectFormat OK (50 cases)");
    }
}
