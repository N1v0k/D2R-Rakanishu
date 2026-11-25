package com.xiaoboluo.d2r;


public class PackSourceTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }
    private static void expect(PackSource.Decision d, PackSource.Action a, String product, String what) {
        check(d.action == a && (product == null ? d.product == null : product.equals(d.product)), what + ": got " + d);
    }

    static final String P = "3.3.94012-r45.n79";
    static String info(String product, boolean caps, String made) { return "PRODUCT=" + product + "\nTARGET=android\nMADE=" + made + "\n" + (caps ? "CAPS=quickmenu1\n" : ""); }

    public static void main(String[] args) {
        String good = info(P, true, "a"), goodNew = info(P, true, "b"), old = info("3.3.94012-r45.n70", true, "c"), noCaps = info(P, false, "d");
        PackSource.Record phone = new PackSource.Record(PackSource.Kind.PHONE, null), pc = new PackSource.Record(PackSource.Kind.PC, "ab".repeat(32));

        expect(PackSource.decide(good, true, null, phone, P), PackSource.Action.START, null, "phone pack, no external");
        expect(PackSource.decide(good, true, goodNew, phone, P), PackSource.Action.START, null, "phone pack survives a newer matching computer pack");
        expect(PackSource.decide(good, true, old, phone, P), PackSource.Action.START, null, "phone pack survives an old computer pack");
        expect(PackSource.decide(good, true, noCaps, phone, P), PackSource.Action.START, null, "phone pack survives a computer pack without the Quick Menu art");

        expect(PackSource.decide(old, true, goodNew, phone, P), PackSource.Action.IMPORT, null, "phone record, old pack, matching external: import");
        expect(PackSource.decide(old, true, old, phone, P), PackSource.Action.GAME_FILES, null, "phone record, old pack, old external: game files page");
        expect(PackSource.decide(null, false, null, phone, P), PackSource.Action.GAME_FILES, null, "phone record, nothing at all: game files page");

        for (PackSource.Record r : new PackSource.Record[]{pc, null}) {
            String n = r == null ? "no record" : "pc record";
            expect(PackSource.decide(good, true, null, r, P), PackSource.Action.START, null, n + ", valid pack, no external");
            expect(PackSource.decide(good, true, good, r, P), PackSource.Action.START, null, n + ", external equals installed");
            expect(PackSource.decide(good, true, goodNew, r, P), PackSource.Action.IMPORT, null, n + ", changed matching external: import");
            expect(PackSource.decide(good, true, old, r, P), PackSource.Action.VERSION_ERROR, "3.3.94012-r45.n70", n + ", mismatched external: version message");
            expect(PackSource.decide(good, true, noCaps, r, P), PackSource.Action.VERSION_ERROR, P, n + ", external without Quick Menu art: version message");
            expect(PackSource.decide(old, true, null, r, P), PackSource.Action.VERSION_ERROR, "3.3.94012-r45.n70", n + ", installed pack of another version");
            expect(PackSource.decide(null, false, goodNew, r, P), PackSource.Action.IMPORT, null, n + ", nothing installed, matching external: import");
            expect(PackSource.decide(null, true, null, r, P), PackSource.Action.START, null, n + ", hand-pushed development pack without pack_info.txt");
            expect(PackSource.decide(null, false, null, r, P), PackSource.Action.GAME_FILES, null, n + ", nothing installed, nothing external: game files page");
        }

        PackSource.Record back = PackSource.parse(PackSource.format(pc));
        check(back.source == PackSource.Kind.PC && back.marker.equals(pc.marker), "record round trip (pc + marker)");
        back = PackSource.parse(PackSource.format(phone));
        check(back.source == PackSource.Kind.PHONE && back.marker == null, "record round trip (phone, no marker)");
        check(PackSource.parse(null) == null, "no record file: null");
        for (String bad : new String[]{"", "source\tphone\n", "source\tcloud\nmarker\t-\n", "source\tpc\nsource\tpc\nmarker\t-\n", "source\tpc\nmarker\tzz\n", "source\tpc\nmarker\t-\nextra\t1\n", "source pc\nmarker -\n"}) {
            boolean refused = false;
            try { PackSource.parse(bad); } catch (IllegalArgumentException e) { refused = true; }
            check(refused, "broken record refused: " + bad.replace("\n", "|"));
        }

        PackSource.Decision vb = PackSource.decide(null, false, "PRODUCT=old\nCAPS=quickmenu1\n", null, "new", true);
        check(vb.action == PackSource.Action.GAME_FILES && "old".equals(vb.product), "external pack of another version, builder offered: " + vb);
        PackSource.Decision ib = PackSource.decide("PRODUCT=old\nCAPS=quickmenu1\n", true, null, new PackSource.Record(PackSource.Kind.PC, null), "new", true);
        check(ib.action == PackSource.Action.GAME_FILES && "old".equals(ib.product), "installed computer pack of another version, builder offered: " + ib);
        check(PackSource.decide("PRODUCT=new\nCAPS=quickmenu1\n", true, "PRODUCT=old\n", new PackSource.Record(PackSource.Kind.PHONE, null), "new", true).action
            == PackSource.Action.START, "a phone pack of this version still starts");

        PackSource.Record phoneRec = new PackSource.Record(PackSource.Kind.PHONE, null);
        String mine = "PRODUCT=new\nCAPS=quickmenu1\n", pcInfo = "PRODUCT=new\nCAPS=quickmenu1\nMADE=x\n";
        check(PackSource.decide(mine, true, pcInfo, phoneRec, "new", true, false).action == PackSource.Action.START, "without the tap the phone pack starts");
        check(PackSource.decide(mine, true, pcInfo, phoneRec, "new", true, true).action == PackSource.Action.IMPORT, "the tap imports the computer pack");
        check(PackSource.decide(mine, true, "PRODUCT=old\nCAPS=quickmenu1\n", phoneRec, "new", true, true).action == PackSource.Action.START,
            "a computer pack of another version is not imported by the tap");
        System.out.println("PackSourceTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
