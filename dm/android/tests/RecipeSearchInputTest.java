



package com.xiaoboluo.d2r;

public final class RecipeSearchInputTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }

    public static void main(String[] a) {
        RecipeSearchInput s = new RecipeSearchInput();
        check(s.type('A') && s.type('m') && s.type('e') && "Ame".equals(s.text()), "typing Ame");
        check(s.composingStart() == -1 && s.composingEnd() == -1, "nothing composing");
        check(s.backspace() && "Am".equals(s.text()), "backspace");
        check(s.commit("ethyst") && "Amethyst".equals(s.text()), "commit");
        check(!s.type(0) && !s.type('\n') && !s.type(0x7f) && "Amethyst".equals(s.text()), "no control characters, no line breaks");
        check(s.clear() && s.isEmpty() && !s.clear(), "clear");

        s.commit("Sol ");
        check(s.setComposing("ㅎ") && "Sol ㅎ".equals(s.text()) && s.composingStart() == 4 && s.composingEnd() == 5, "composing ㅎ");
        check(s.setComposing("하") && s.setComposing("한") && "Sol 한".equals(s.text()), "composing replaces itself");
        check(s.deleteBefore(1) && "Sol ".equals(s.text()) && s.composingStart() == -1, "backspace takes the composing part first");
        s.setComposing("한");
        check(s.commit("한") == false && "Sol 한".equals(s.text()) && s.composingStart() == -1, "commit of the composed text changes nothing visible");
        s.setComposing("ㄱ");
        check(!s.finishComposing() && "Sol 한ㄱ".equals(s.text()) && s.composingStart() == -1, "finish keeps the composed text");
        check(s.deleteBefore(3) && "Sol".equals(s.text()), "delete several");
        check(!s.deleteBefore(0) && s.deleteBefore(99) && s.isEmpty() && !s.deleteBefore(1), "delete more than there is");

        StringBuilder x = new StringBuilder();
        for (int i = 0; i < 40; i++) x.append('x');
        s.commit(x);
        check(s.length() == RecipeSearchInput.MAX && !s.type('y'), "at most 32 characters");
        s.clear();
        for (int i = 0; i < 31; i++) s.type('a');
        check(!s.commit("😀") && s.length() == 31, "a pair that does not fit is not cut in half");
        check(s.backspace() && s.commit("😀") && s.length() == 32 && Character.isHighSurrogate(s.text().charAt(30)), "a pair that fits");
        check(s.backspace() && s.length() == 30, "backspace takes the whole pair");
        s.clear();
        s.commit("a😀");
        check(s.deleteUnits(2) && "a".equals(s.text()), "2 UTF-16 units = the emoji only (deleteSurroundingText)");
        s.commit("😀");
        check(s.deleteUnits(1) && "a".equals(s.text()), "half a pair takes the whole pair");
        s.commit("bc"); s.setComposing("d");
        check(s.deleteUnits(2) && "ad".equals(s.text()) && s.composingStart() == 1 && s.composingEnd() == 2, "units: the composing part is left out, the text before it goes");
        check(!s.deleteUnits(0) && s.deleteUnits(9) && "d".equals(s.text()) && s.composingStart() == 0, "units: more than there is, the composing part stays");
        check(!s.deleteUnits(1) && "d".equals(s.text()), "units: nothing committed before the composing part");
        s.clear();
        s.commit("Sol ");
        s.setComposing("Hel");
        check(s.deleteCodePoints(1) && "SolHel".equals(s.text()) && s.composingStart() == 3, "code points: the space before the composing part goes, not the l");
        s.clear();
        s.commit("a😀b");
        check(s.deleteCodePoints(2) && "a".equals(s.text()), "code points: a pair counts as one");
        check(!s.deleteCodePoints(0) && s.deleteCodePoints(5) && s.isEmpty() && !s.deleteCodePoints(1), "code points: more than there is");
        s.clear();
        check(s.commit("a\nb\tc") && "abc".equals(s.text()), "line breaks and tabs in committed text are dropped");
        check(!s.setComposing(null) && !s.commit(null), "null is nothing");

        s.clear();
        s.commit("Hel");
        check(s.markComposing(0, 3) && "Hel".equals(s.text()) && s.composingStart() == 0 && s.composingEnd() == 3, "the whole text marked as composing");
        check(s.setComposing("Helm") && "Helm".equals(s.text()), "recomposing the marked text does not double it");
        s.clear();
        s.commit("Sol Hel");
        check(s.markComposing(7, 4) && s.composingStart() == 4 && s.composingEnd() == 7 && "Sol Hel".equals(s.text()), "a suffix marked (start and end swapped)");
        check(s.setComposing("Helm") && "Sol Helm".equals(s.text()), "the suffix recomposed");
        check(s.commit("Helm") == false && "Sol Helm".equals(s.text()) && s.composingStart() == -1, "the suffix committed again");
        check(!s.markComposing(0, 3) && "Sol Helm".equals(s.text()) && s.composingStart() == -1, "a range not reaching the end is refused");
        s.setComposing("x");
        check(s.markComposing(2, 2) && "Sol Helmx".equals(s.text()) && s.composingStart() == -1, "an empty range finishes composing");
        check(s.markComposing(-5, 99) && s.composingStart() == 0 && s.composingEnd() == 9, "out-of-range values clamped");
        s.clear();
        s.commit("a😀");
        check(s.markComposing(2, 3) && s.composingStart() == 1 && "a😀".equals(s.text()), "a start inside a pair takes the whole pair");

        s.clear();
        s.setComposing("rune");
        check(s.replaceFrom(3, "es") && "runes".equals(s.text()) && s.composingStart() == -1, "replacing the suffix keeps the composing prefix");
        s.clear();
        s.commit("Sol ");
        s.setComposing("Hel");
        check(s.replaceFrom(4, "Ist") && "Sol Ist".equals(s.text()), "replacing exactly the composing part");
        check(s.replaceFrom(7, "!") && "Sol Ist!".equals(s.text()), "an empty stretch at the end = insert");
        check(!s.replaceFrom(9, "x") && !s.replaceFrom(-1, "x") && "Sol Ist!".equals(s.text()), "a start out of range does nothing");
        s.clear();
        s.commit("a😀");
        check(s.replaceFrom(2, "b") && "ab".equals(s.text()), "a start inside a pair replaces the whole pair");


        java.util.function.IntBinaryOperator dc = (acc, ch) -> acc == 0xB4 && ch == 'e' ? 0xE9 : acc == '^' && ch == 'a' ? 0xE2 : 0;
        s.clear();
        check(!s.dead(0xB4) && s.isEmpty(), "a dead key alone writes nothing");
        check(s.type('e', dc) && "é".equals(s.text()), "dead key + e = é");
        check(s.type('e', dc) && "ée".equals(s.text()), "the accent is used only once");
        s.dead(0xB4);
        check(s.type('x', dc) && "ée´x".equals(s.text()), "no combination: the accent and the letter separately");
        s.clear();
        s.dead(0xB4);
        check(s.dead(0xB4) && "´".equals(s.text()) && s.type('e', dc) && "´e".equals(s.text()), "the same dead key twice = the accent itself, nothing left pending");
        s.clear();
        s.dead(0xB4);
        check(s.dead('^') && "´".equals(s.text()) && s.type('a', dc) && "´â".equals(s.text()), "another dead key writes the previous accent, the new one pends");
        s.clear();
        s.commit("ab");
        s.dead(0xB4);
        check(!s.backspace() && "ab".equals(s.text()) && s.type('e', dc) && "abe".equals(s.text()), "backspace drops the pending accent only");
        s.dead(0xB4);
        s.finishTyping();
        check(s.type('e', dc) && "abee".equals(s.text()), "typing ends: the pending accent is dropped");
        s.dead(0xB4);
        check(!s.type('\n', dc) && s.type('e', dc) && "abeeé".equals(s.text()), "a control character neither types nor uses up the accent");
        System.out.println("RecipeSearchInputTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
