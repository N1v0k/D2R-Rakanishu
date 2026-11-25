package com.xiaoboluo.d2r;

import java.util.HashMap;
import java.util.Map;




public class EffectCalcTest {
    private static int checks, fails;
    private static void equal(Object want, Object actual, String what) {
        checks++;
        if (want == null ? actual != null : !want.equals(actual)) { fails++; System.err.println("FAIL " + what + ": want=" + want + " actual=" + actual); }
    }

    static final class Env implements EffectCalc.Env {
        int[] par = new int[12]; String[] calc = {"", "", "", ""}, ast = {"", "", "", "", "", ""}; String len = ""; final Map<String, Integer> base = new HashMap<>(); int emin = EffectCalc.NONE; final int[] eminlev = new int[5];
        public int param(int i) { return i >= 1 && i <= 12 ? par[i - 1] : 0; }
        public String calc(int i) { return calc[i - 1]; }
        public String ast(int i) { return ast[i - 1]; }
        public String len() { return len; }
        public int baseLevel(String skill) { Integer v = base.get(skill); return v == null ? EffectCalc.NONE : v; }
        public int emin() { return emin; }
        public int eminLev(int i) { return eminlev[i]; }
    }

    public static void main(String[] args) {
        Env e = new Env();
        e.par[0] = 10; e.par[1] = 2; e.par[2] = 35; e.par[3] = 150; e.par[4] = 30; e.par[5] = 1; e.par[8] = 5; e.par[9] = 100; e.par[10] = 7;
        equal(7, EffectCalc.eval("3+4", 1, e), "sum");
        equal(14, EffectCalc.eval("2*(3+4)", 1, e), "parentheses");
        equal(-3, EffectCalc.eval("-3", 1, e), "unary minus");
        equal(2, EffectCalc.eval("7/3", 1, e), "integer division");
        equal(-2, EffectCalc.eval("-7/3", 1, e), "integer division towards 0");
        equal(5, EffectCalc.eval("lvl", 5, e), "lvl");
        equal(5, EffectCalc.eval("par9", 1, e), "par9");
        equal(100, EffectCalc.eval("pa10", 1, e), "pa10");
        equal(7, EffectCalc.eval("pa11", 1, e), "pa11");
        equal(30, EffectCalc.eval("ln56", 1, e), "ln56 at level 1 = par5");
        equal(33, EffectCalc.eval("ln56", 4, e), "ln56 at level 4 = par5 + 3 par6");
        equal(18, EffectCalc.eval("ln12", 5, e), "ln12 = par1 + (lvl - 1) par2");
        equal(52, EffectCalc.eval("dm34", 1, e), "dm34 at level 1: 35 + (110 x 1 / 7 = 15) x 115 / 100 = 52, the simulation's level_dm");
        equal(35 + (110 * 20 / 26) * 115 / 100, EffectCalc.eval("dm34", 20, e), "dm34 at level 20: the quotient truncated first");
        equal(-30, EffectCalc.eval("min(ln56,50)*-1", 1, e), "min and a negative factor");
        equal(-50, EffectCalc.eval("min(ln56,50)*-1", 30, e), "min caps at 50");
        equal(-50, EffectCalc.eval("\"-min(ln56,50)\"", 30, e), "quotes around a formula");
        equal(31, EffectCalc.eval("max(ln56, 31)", 1, e), "max");
        e.base.put("Blast", 0);
        e.calc[0] = "150+(((skill('Blast'.blvl)) *par9))";
        equal(150, EffectCalc.field("clc1", 1, e), "clc1 with the referred skill at base 0");
        e.base.put("Blast", 3);
        equal(165, EffectCalc.field("clc1", 1, e), "clc1 with the referred skill at base 3");
        e.base.put("Ward", 0);
        String cond = "(skill('Ward'.blvl) < 1) ? 1 : skill('Ward'.blvl)";
        equal(1, EffectCalc.eval(cond, 1, e), "?: false branch");
        e.base.put("Ward", 4);
        equal(4, EffectCalc.eval(cond, 1, e), "?: true branch");
        equal(1, EffectCalc.eval("3 >= 3", 1, e), "comparison");
        e.ast[0] = "min(ln56,50)*-1"; e.ast[1] = "-25"; e.len = "ln34";
        equal(-30, EffectCalc.field("ast1", 1, e), "ast1");
        equal(-25, EffectCalc.field("ast2", 9, e), "ast2, a constant");
        equal(35 + 150 * 2, EffectCalc.field("len", 3, e), "len");
        e.emin = 2; int[] plev = {1, 1, 2, 2, 3}; for (int i = 0; i < plev.length; i++) e.eminlev[i] = plev[i];
        equal(2 * 256, EffectCalc.eval("edns", 1, e), "edns at level 1 = EMin in 1/256 points");
        equal(4 * 256, EffectCalc.eval("edns", 3, e), "edns at level 3: 2 + 2 x 1, times 256");
        equal(21, EffectCalc.eval("edmn", 18, e), "edmn at level 18: 2 + 7 x 1 + 8 x 1 + 2 x 2, unscaled");
        equal(2 + 7 * 1 + 8 * 1 + 6 * 2 + 6 * 2 + 228 * 3, EffectCalc.eval("edmn", 256, e), "the level steps 7 / 8 / 6 / 6 then the last one, as the simulator");
        equal(EffectCalc.NONE, EffectCalc.eval("edns", 3, new Env()), "no EMin on the row: cannot be evaluated");
        equal(25, EffectCalc.field("25", 3, e), "a plain number as a field");

        equal(EffectCalc.NONE, EffectCalc.eval("enma", 1, e), "an unknown token");
        equal(EffectCalc.NONE, EffectCalc.eval("ln34+toht", 1, e), "an unknown token inside a sum");
        equal(EffectCalc.NONE, EffectCalc.eval("skill('Gone'.blvl)*2", 1, e), "a skill the block does not know");
        equal(EffectCalc.NONE, EffectCalc.eval("skill('Blast'.ln12)", 1, e), "a property other than blvl");
        equal(EffectCalc.NONE, EffectCalc.eval("4/(lvl-1)", 1, e), "division by zero");
        equal(EffectCalc.NONE, EffectCalc.eval("3+", 1, e), "a formula cut short");
        equal(EffectCalc.NONE, EffectCalc.eval("3 4", 1, e), "trailing text");
        equal(EffectCalc.NONE, EffectCalc.eval("", 1, e), "empty");
        e.calc[1] = "clc2+1";
        equal(EffectCalc.NONE, EffectCalc.field("clc2", 1, e), "a formula referring to itself stops");
        equal(EffectCalc.NONE, EffectCalc.field("clc3", 1, e), "an empty calc");
        equal(EffectCalc.NONE, EffectCalc.eval("1234567890", 1, e), "a number too long");

        equal(false, EffectCalc.verified(385, "ast9"), "a field off the list is not verified");
        System.out.println("EffectCalcTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
