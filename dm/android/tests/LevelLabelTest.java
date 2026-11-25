




package com.xiaoboluo.d2r;

import java.nio.charset.StandardCharsets;

public final class LevelLabelTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL " + what); } }
    private static void same(String want, String got, String what) { check(want.equals(got), what + ": want \"" + want + "\", got \"" + got + "\""); }
    private static byte[] u(String s) { return s.getBytes(StandardCharsets.UTF_8); }

    private static final String DE = "%1$s %2$d · %3$d %%", EN = "%1$s %2$d · %3$d%%";

    public static void main(String[] a) {

        same("Stufe 7 · 45 %", LevelLabel.text(LevelLabel.word(u("Stufe")), "Stufe", DE, 7, 45), "de: spelled out as the original");
        same("Level 7 · 45%", LevelLabel.text(LevelLabel.word(u("Level")), "Level", EN, 7, 45), "en");
        same("Nivel 12 · 0%", LevelLabel.text(LevelLabel.word(u("Nivel")), "Nivel", EN, 12, 0), "es");
        same("Уровень 99 · 100%", LevelLabel.text(LevelLabel.word(u("Уровень")), "Уровень", EN, 99, 100), "ru");
        same("레벨 1 · 5%", LevelLabel.text(LevelLabel.word(u("레벨")), "레벨", EN, 1, 5), "ko");
        same("等级 3 · 50%", LevelLabel.text(LevelLabel.word(u("等级")), "等级", EN, 3, 50), "zh");
        same("Nível 7 · 45 %", LevelLabel.text(LevelLabel.word(u("Nível")), "Level", DE, 7, 45), "pt: same spaced format as de");
        same("Stufe", LevelLabel.word(u(" Stufe ")), "trimmed");

        same("Level 7 · 45 %", LevelLabel.text("Level", "Stufe", DE, 7, 45), "game word wins over the copy");

        same("Stufe 7 · 45 %", LevelLabel.text(LevelLabel.word(null), "Stufe", DE, 7, 45), "null → copied word");
        same("Level 7 · 45%", LevelLabel.text(LevelLabel.word(new byte[0]), "Level", EN, 7, 45), "empty → copied word");

        same("Stufe 7 · 45%", LevelLabel.text(null, "Stufe", "%1$s %2$s %4$d", 7, 45), "broken format → plain line");
        same("Stufe 7 · 45%", LevelLabel.text(null, "Stufe", null, 7, 45), "no format → plain line");
        same("7 · 45%", LevelLabel.text(null, null, null, 7, 45), "no word at all → numbers only");
        check(LevelLabel.word(u("   ")) == null, "blank → null");
        check(LevelLabel.word(u("Lv\nl")) == null, "control character → null");
        check(LevelLabel.word(new byte[] { (byte) 0xc3, (byte) 0x28 }) == null, "invalid UTF-8 → null");
        check(LevelLabel.word(u("abcdefghijklmnopqrstuvwxyz")) == null, "too long → null");
        check(LevelLabel.word(new byte[65]) == null, "too many bytes → null");
        System.out.println("LevelLabelTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
