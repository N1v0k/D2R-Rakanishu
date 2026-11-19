package com.xiaoboluo.d2r;

import java.util.function.IntBinaryOperator;








final class RecipeSearchInput {
    static final int MAX = 32;
    private final StringBuilder committed = new StringBuilder(), composing = new StringBuilder();
    private int accent;

    String text() { return committed.toString() + composing; }
    int length() { return committed.length() + composing.length(); }
    boolean isEmpty() { return length() == 0; }

    int composingStart() { return composing.length() == 0 ? -1 : committed.length(); }
    int composingEnd() { return composing.length() == 0 ? -1 : length(); }


    boolean commit(CharSequence s) {
        String before = text();
        composing.setLength(0);
        append(committed, clean(s));
        return !before.equals(text());
    }


    boolean setComposing(CharSequence s) {
        String before = text();
        composing.setLength(0);
        append(composing, clean(s));
        return !before.equals(text());
    }


    boolean finishComposing() {
        if (composing.length() == 0) return false;
        committed.append(composing);
        composing.setLength(0);
        return false;
    }






    boolean markComposing(int start, int end) {
        finishComposing();
        int n = committed.length(), a = Math.max(0, Math.min(Math.min(start, end), n)), b = Math.max(0, Math.min(Math.max(start, end), n));
        if (a == b) return true;
        if (b != n) return false;
        if (a > 0 && Character.isLowSurrogate(committed.charAt(a)) && Character.isHighSurrogate(committed.charAt(a - 1))) a--;
        composing.append(committed, a, n);
        committed.setLength(a);
        return true;
    }




    boolean replaceFrom(int start, CharSequence s) {
        if (start < 0 || start > length()) return false;
        String before = text();
        finishComposing();
        if (start > 0 && start < committed.length() && Character.isLowSurrogate(committed.charAt(start)) && Character.isHighSurrogate(committed.charAt(start - 1))) start--;
        committed.setLength(start);
        append(committed, clean(s));
        return !before.equals(text());
    }



    boolean deleteBefore(int n) {
        if (n <= 0) return false;
        String before = text();
        for (int i = 0; i < n; i++) {
            StringBuilder b = composing.length() > 0 ? composing : committed;
            if (b.length() == 0) break;
            int end = b.length(), start = end - 1;
            if (start > 0 && Character.isLowSurrogate(b.charAt(start)) && Character.isHighSurrogate(b.charAt(start - 1))) start--;
            b.delete(start, end);
        }
        return !before.equals(text());
    }




    boolean deleteUnits(int n) {
        if (n <= 0 || committed.length() == 0) return false;
        int start = Math.max(0, committed.length() - n);
        if (start > 0 && Character.isLowSurrogate(committed.charAt(start)) && Character.isHighSurrogate(committed.charAt(start - 1))) start--;
        committed.setLength(start);
        return true;
    }


    boolean deleteCodePoints(int n) {
        if (n <= 0 || committed.length() == 0) return false;
        int start = committed.length();
        for (int i = 0; i < n && start > 0; i++) start = committed.offsetByCodePoints(start, -1);
        committed.setLength(start);
        return true;
    }



    boolean backspace() {
        if (accent != 0) { accent = 0; return false; }
        return deleteBefore(1);
    }


    void finishTyping() { finishComposing(); accent = 0; }

    boolean clear() {
        accent = 0;
        boolean had = length() > 0;
        committed.setLength(0);
        composing.setLength(0);
        return had;
    }


    boolean type(int codePoint) {
        if (codePoint <= 0 || Character.isISOControl(codePoint) || !Character.isValidCodePoint(codePoint)) return false;
        finishComposing();
        return commit(new String(Character.toChars(codePoint)));
    }





    boolean dead(int a) {
        if (a <= 0 || Character.isISOControl(a) || !Character.isValidCodePoint(a)) return false;
        int prev = accent;
        accent = 0;
        if (prev == 0) { accent = a; return false; }
        boolean t = type(prev);
        if (prev != a) accent = a;
        return t;
    }



    boolean type(int codePoint, IntBinaryOperator combine) {
        int a = accent;
        if (a == 0) return type(codePoint);
        if (codePoint <= 0 || Character.isISOControl(codePoint) || !Character.isValidCodePoint(codePoint)) return false;
        accent = 0;
        int c = combine.applyAsInt(a, codePoint);
        if (c > 0) return type(c);
        boolean x = type(a), y = type(codePoint);
        return x || y;
    }



    private static String clean(CharSequence s) {
        if (s == null) return "";
        StringBuilder out = new StringBuilder(s.length());
        for (int i = 0; i < s.length(); i++) { char c = s.charAt(i); if (!Character.isISOControl(c)) out.append(c); }
        return out.toString();
    }



    private void append(StringBuilder into, String s) {
        int room = MAX - length();
        if (room <= 0 || s.isEmpty()) return;
        int end = Math.min(room, s.length());
        if (end < s.length() && end > 0 && Character.isHighSurrogate(s.charAt(end - 1)) && Character.isLowSurrogate(s.charAt(end))) end--;
        into.append(s, 0, end);
    }
}
