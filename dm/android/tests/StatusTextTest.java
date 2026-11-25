package com.xiaoboluo.d2r;



final class StatusTextTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL: " + what); } }

    static void one(String template, int percent, String label, String value) {
        String[] out = new String[2];
        String line = String.format(template, percent);
        StatusText.split(template, line, out);
        check(out[0].equals(label) && out[1].equals(value), template + " -> [" + out[0] + "|" + out[1] + "]");
        check((out[0] + out[1]).equals(line), template + ": nothing lost");
    }

    public static void main(String[] a) {
        one("Akku %d %%", 74, "Akku ", "74 %");
        one("Akku %d %% (lädt)", 74, "Akku ", "74 % (lädt)");
        one("Battery %d%% (charging)", 5, "Battery ", "5% (charging)");
        one("Batería %d %% (cargando)", 100, "Batería ", "100 % (cargando)");
        one("Батарея %d %% (заряжается)", 9, "Батарея ", "9 % (заряжается)");
        one("电量 %d%%（充电中）", 74, "电量 ", "74%（充电中）");
        one("배터리 %d%% (충전 중)", 74, "배터리 ", "74% (충전 중)");
        one("%d%% battery", 74, "", "74% battery");
        String[] out = new String[2];
        StatusText.split("Akku %d %%", "Battery 74%", out); check(out[0].isEmpty() && out[1].equals("Battery 74%"), "mismatching line: all value");
        StatusText.split(null, null, out); check(out[0].isEmpty() && out[1].isEmpty(), "nothing: empty");
        System.out.println("StatusTextTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
