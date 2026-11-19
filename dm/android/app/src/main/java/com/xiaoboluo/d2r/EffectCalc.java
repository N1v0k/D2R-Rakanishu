package com.xiaoboluo.d2r;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;












final class EffectCalc {
    static final int NONE = Integer.MIN_VALUE;
    private static final int DEPTH = 8;





    static final Set<String> VERIFIED = new HashSet<>(Arrays.asList("385:ast1", "385:ast2"));


    static final Set<String> VERIFIED_BASE = new HashSet<>(Arrays.asList("385:clc1"));



    interface Env {
        int param(int i);
        String calc(int i);
        String ast(int i);
        String len();
        int baseLevel(String skill);
        default int emin() { return NONE; }
        default int eminLev(int i) { return 0; }
    }

    private EffectCalc() {}

    static boolean verified(int skill, String field) { return VERIFIED.contains(skill + ":" + field); }




    static Env gate(int skill, String field, Env env) {
        if (verified(skill, field)) return env;
        if (!VERIFIED_BASE.contains(skill + ":" + field)) return null;
        return new Env() {
            public int param(int i) { return env.param(i); }
            public String calc(int i) { return env.calc(i); }
            public String ast(int i) { return env.ast(i); }
            public String len() { return env.len(); }
            public int baseLevel(String skill) { int v = env.baseLevel(skill); return v == 0 ? 0 : NONE; }
        };
    }


    static int field(String f, int lvl, Env env) { return eval(f, lvl, env, 0); }

    static int eval(String f, int lvl, Env env) { return eval(f, lvl, env, 0); }

    private static int eval(String f, int lvl, Env env, int depth) {
        if (f == null || depth > DEPTH) return NONE;
        String s = f.trim();
        if (s.length() >= 2 && s.charAt(0) == '"' && s.charAt(s.length() - 1) == '"') s = s.substring(1, s.length() - 1).trim();
        if (s.isEmpty()) return NONE;
        Parser p = new Parser(s, lvl, env, depth);
        long v = p.expr();
        p.space();
        return p.bad || p.at != s.length() || v == NONE || v > Integer.MAX_VALUE || v < -Integer.MAX_VALUE ? NONE : (int) v;
    }

    private static final class Parser {
        final String s; final int lvl, depth; final Env env; int at; boolean bad;
        Parser(String s, int lvl, Env env, int depth) { this.s = s; this.lvl = lvl; this.env = env; this.depth = depth; }

        void space() { while (at < s.length() && s.charAt(at) == ' ') at++; }
        boolean eat(String t) { space(); if (s.startsWith(t, at)) { at += t.length(); return true; } return false; }
        long fail() { bad = true; return NONE; }

        long expr() {
            long c = compare();
            if (bad) return NONE;
            if (eat("?")) {
                long a = expr(); if (!eat(":")) return fail();
                long b = expr();
                return bad ? NONE : c != 0 ? a : b;
            }
            return c;
        }

        long compare() {
            long a = add();
            for (String op : new String[]{"<=", ">=", "==", "!=", "<", ">"}) {
                if (bad || !eat(op)) continue;
                long b = add();
                if (bad) return NONE;
                switch (op) {
                    case "<=": return a <= b ? 1 : 0;
                    case ">=": return a >= b ? 1 : 0;
                    case "==": return a == b ? 1 : 0;
                    case "!=": return a != b ? 1 : 0;
                    case "<": return a < b ? 1 : 0;
                    default: return a > b ? 1 : 0;
                }
            }
            return a;
        }

        long add() {
            long a = mul();
            while (!bad) {
                if (eat("+")) a = a + mul();
                else if (eat("-")) a = a - mul();
                else break;
            }
            return bad ? NONE : a;
        }

        long mul() {
            long a = unary();
            while (!bad) {
                if (eat("*")) a = a * unary();
                else if (eat("/")) { long b = unary(); if (bad || b == 0) return fail(); a = a / b; }
                else break;
            }
            return bad ? NONE : a;
        }

        long unary() { return eat("-") ? -unary() : primary(); }

        long primary() {
            space();
            if (at >= s.length()) return fail();
            char c = s.charAt(at);
            if (c == '(') { at++; long v = expr(); return eat(")") ? v : fail(); }
            if (Character.isDigit(c)) { int st = at; while (at < s.length() && Character.isDigit(s.charAt(at))) at++; return at - st > 9 ? fail() : Long.parseLong(s.substring(st, at)); }
            if (!Character.isLetter(c)) return fail();
            int st = at;
            while (at < s.length() && Character.isLetterOrDigit(s.charAt(at))) at++;
            String id = s.substring(st, at);
            if (id.equals("min") || id.equals("max")) {
                if (!eat("(")) return fail();
                long a = expr(); if (!eat(",")) return fail();
                long b = expr(); if (!eat(")")) return fail();
                return bad ? NONE : id.equals("min") ? Math.min(a, b) : Math.max(a, b);
            }
            if (id.equals("skill")) return skill();
            return name(id);
        }


        long skill() {
            if (!eat("(") || !eat("'")) return fail();
            int end = s.indexOf('\'', at);
            if (end < 0) return fail();
            String name = s.substring(at, end);
            at = end + 1;
            if (!eat(".blvl") || !eat(")")) return fail();
            int v = env.baseLevel(name);
            return v == NONE ? fail() : v;
        }

        long name(String id) {
            if (id.equals("lvl")) return lvl;
            if (id.matches("par[1-9]")) return env.param(id.charAt(3) - '0');
            if (id.matches("pa1[0-2]")) return env.param(Integer.parseInt(id.substring(2)));
            if (id.matches("ln[1-9][1-9]")) return (long) env.param(id.charAt(2) - '0') + (long) (lvl - 1) * env.param(id.charAt(3) - '0');
            if (id.matches("dm[1-9][1-9]")) {
                long a = env.param(id.charAt(2) - '0'), b = env.param(id.charAt(3) - '0');
                if (lvl < 1) return a;
                long v = (110L * lvl / (lvl + 6)) * (b - a) / 100 + a;
                return b >= a ? Math.min(v, b) : Math.max(v, b);
            }
            if (id.equals("edns") || id.equals("edmn")) {
                int base = env.emin();
                if (base == NONE) return fail();
                long v = base;
                int left = Math.min(lvl, 256) - 1;
                int[] steps = {7, 8, 6, 6, 228};
                for (int i = 0; i < steps.length && left > 0; i++) { int n = Math.min(left, steps[i]); v += (long) n * env.eminLev(i); left -= n; }
                return id.equals("edns") ? v * 256 : v;
            }
            if (id.matches("clc[1-4]")) return sub(env.calc(id.charAt(3) - '0'));
            if (id.matches("ast[1-6]")) return sub(env.ast(id.charAt(3) - '0'));
            if (id.equals("len")) return sub(env.len());
            return fail();
        }

        long sub(String f) { int v = eval(f, lvl, env, depth + 1); return v == NONE ? fail() : v; }
    }
}
