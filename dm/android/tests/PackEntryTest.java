package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.List;
import java.util.Map;



public class PackEntryTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    static final String P = "3.3.94012-r45.n90";
    static String info(String product) { return "PRODUCT=" + product + "\nTARGET=android\nMADE=x\nCAPS=quickmenu1\n"; }

    static void expect(PackEntry e, PackEntry.Head head, String version, boolean rebuild, PackEntry.Target target, boolean missing, String what) {
        check(e.head == head && (version == null ? e.version == null : version.equals(e.version)) && e.rebuild == rebuild && e.target == target && e.missing == missing,
            what + ": " + e.head + " " + e.version + " rebuild=" + e.rebuild + " " + e.target + " missing=" + e.missing);
    }

    public static void main(String[] args) throws Exception {
        PackSource.Record phone = new PackSource.Record(PackSource.Kind.PHONE, null), pc = new PackSource.Record(PackSource.Kind.PC, null);
        String old = info("3.3.94012-r45.n89"), cur = info(P);
        PackEntry.Kept none = PackEntry.Kept.NONE, part = PackEntry.Kept.PARTIAL, full = PackEntry.Kept.COMPLETE;
        PackEntry.Head INTRO = PackEntry.Head.INTRO, UPDATED = PackEntry.Head.UPDATED, VERSION = PackEntry.Head.VERSION, NONE = PackEntry.Head.NONE;
        PackEntry.Target PICK = PackEntry.Target.PICK, KEPT = PackEntry.Target.KEPT;


        for (PackEntry.Kept k : PackEntry.Kept.values()) expect(PackEntry.decide(null, null, P, null, k), INTRO, null, false, PICK, false, "no pack, kept " + k);
        expect(PackEntry.decide(null, phone, P, null, none), INTRO, null, false, PICK, false, "no pack but a phone record (removed by hand)");

        expect(PackEntry.decide(old, phone, P, null, full), UPDATED, null, true, KEPT, false, "phone pack, other version, kept complete");
        expect(PackEntry.decide(old, phone, P, null, part), UPDATED, null, true, PICK, true, "phone pack, other version, kept partial");
        expect(PackEntry.decide(old, phone, P, null, none), UPDATED, null, true, PICK, true, "phone pack, other version, nothing kept");

        expect(PackEntry.decide(old, pc, P, "3.3.94012-r45.n89", full), VERSION, "3.3.94012-r45.n89", false, KEPT, false, "computer pack, other version, kept complete");
        expect(PackEntry.decide(old, pc, P, null, none), VERSION, "3.3.94012-r45.n89", false, PICK, false, "computer pack, other version, nothing kept (version from the pack)");
        expect(PackEntry.decide(old, null, P, null, part), VERSION, "3.3.94012-r45.n89", false, PICK, true, "no record (old computer pack), kept partial");
        expect(PackEntry.decide(cur, pc, P, "3.3.94012-r45.n70", none), NONE, null, false, PICK, false, "a current pack ignores a version found elsewhere");

        expect(PackEntry.decide(cur, phone, P, null, full), NONE, null, true, KEPT, false, "phone pack, this version, kept complete");
        expect(PackEntry.decide(cur, phone, P, null, part), NONE, null, true, PICK, true, "phone pack, this version, kept partial");
        expect(PackEntry.decide(cur, pc, P, null, full), NONE, null, true, KEPT, false, "computer pack, this version, kept complete");
        expect(PackEntry.decide(cur, pc, P, null, none), NONE, null, false, PICK, false, "computer pack, this version, nothing kept: Build on this phone");

        expect(PackEntry.decide("PRODUCT=" + P + "\n", phone, P, null, full), UPDATED, null, true, KEPT, false, "a phone pack without the Quick Menu mark");


        File tmp = Files.createTempDirectory("packentry").toFile();
        File d = new File(tmp, "k");
        List<PackJournal.Unit> units = PackBuildRun.units(PackJournalTest.listing(), new File(d, "exefs"), new File(d, "data"));
        PackJournal j = new PackJournal(new File(d, "journal.tsv"), dir -> {}, PackJournalTest.STAMP);
        check(PackEntry.kept(j.load(), units, PackEntry::present) == none, "no journal: nothing kept");
        PackJournalTest.run(d, new PackJournalTest.Writer());
        check(PackEntry.kept(j.load(), units, PackEntry::present) == full, "after a build: complete");
        File a = new File(new File(d, "data"), "00-00000002.data");
        byte[] b = Files.readAllBytes(a.toPath());
        b[0] ^= 1;
        Files.write(a.toPath(), b);
        check(PackEntry.kept(j.load(), units, PackEntry::present) == full, "changed bytes at the same size are not seen by the cheap look (the reconcile finds them)");
        Files.write(a.toPath(), new byte[3]);
        check(PackEntry.kept(j.load(), units, PackEntry::present) == part, "another size: partial");
        check(a.delete() && PackEntry.kept(j.load(), units, PackEntry::present) == part, "a missing file: partial");
        PackJournalTest.run(d, new PackJournalTest.Writer());
        byte[] c = PackJournalTest.content("new.data");
        List<PackJournal.Unit> wider = PackBuildRun.units(PackJournalTest.listing() + "new\tdata\tnew.data\t" + c.length + "\t" + DriverLibrary.sha256Bytes(c) + "\n", new File(d, "exefs"), new File(d, "data"));
        check(PackEntry.kept(j.load(), wider, PackEntry::present) == part, "a unit new in this app version: partial");
        Map<String, PackJournal.Record> records = j.load();
        PackJournal.Record r = records.get("meta");
        records.put("meta", new PackJournal.Record(r.unit, r.identity, "0".repeat(64), r.digest, r.stamps));
        check(PackEntry.kept(records, units, PackEntry::present) == part, "a stale fingerprint: partial");

        File d2 = new File(tmp, "torn");
        List<PackJournal.Unit> units2 = PackBuildRun.units(PackJournalTest.listing(), new File(d2, "exefs"), new File(d2, "data"));
        PackJournalTest.run(d2, new PackJournalTest.Writer());
        File torn = new File(d2, "journal.tsv");
        PackJournal j2 = new PackJournal(torn, dir -> {}, PackJournalTest.STAMP);
        String text = new String(Files.readAllBytes(torn.toPath()));
        int cut = text.lastIndexOf('\n', text.length() - 2);
        Files.write(torn.toPath(), text.substring(0, cut + 10).getBytes());
        check(PackEntry.kept(j2.load(), units2, PackEntry::present) == part, "a torn last line: that unit is not kept");
        Files.write(torn.toPath(), "broken\n".getBytes());
        boolean refused = false;
        try { j2.load(); } catch (IOException e) { refused = true; }
        check(refused, "a broken journal is refused (the page counts it as nothing kept)");

        PackJournalTest.clean(tmp);
        System.out.println("PackEntryTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
