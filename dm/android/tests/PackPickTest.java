package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;


public class PackPickTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.err.println("FAIL " + what); } }

    static PackRows rows(String... picks) {
        List<PackRows.Pick> l = new ArrayList<>();
        for (int i = 0; i < picks.length; i += 2) l.add(new PackRows.Pick("content://x/" + i, picks[i], picks[i + 1]));
        return PackRows.of(l);
    }

    static PackRows.Pick p(String name, String result, PackRows.Row row) { return new PackRows.Pick("content://x/" + name, name, result, row); }

    static PackRows picks(PackRows.Pick... picks) { return PackRows.of(List.of(picks)); }

    public static void main(String[] args) {
        String qa = "io.example.qadocs";
        check(PackDocs.local(PackDocs.EXTERNAL_STORAGE, "primary:Download/base.nsp", null, null), "the system's external storage is local");
        check(PackDocs.local(PackDocs.DOWNLOADS, "msf:31", null, null), "the system's downloads are local");
        check(!PackDocs.local("com.google.android.apps.docs.storage", "doc=123", null, null), "a cloud provider is not local, whatever its descriptor looks like");
        check(!PackDocs.local("com.android.providers.media.documents", "document:12", null, null), "other system providers are not on the list");
        check(PackDocs.local(qa, "local:base.nsp", qa, "local"), "the qa provider's local root");
        check(!PackDocs.local(qa, "remote:base.nsp", qa, "local"), "the qa provider's remote root (seekable proxy descriptors) is refused");
        check(!PackDocs.local(qa, "local:base.nsp", null, null), "the qa provider is unknown to the player variant");
        check(!PackDocs.local(null, "x", qa, "local") && !PackDocs.local(PackDocs.DOWNLOADS, null, null, null), "no authority or no document id: refused");

        check(PackDocs.descriptor(true, 8_287_363_140L, 8_287_363_140L, 8_287_363_140L), "a regular file whose end is its reported length");
        check(!PackDocs.descriptor(false, 100, 100, 100), "a pipe or socket is refused");
        check(!PackDocs.descriptor(true, -1, 100, 100), "a descriptor that cannot seek is refused");
        check(!PackDocs.descriptor(true, 100, 100, -1), "an unknown length is refused");
        check(!PackDocs.descriptor(true, 50, 100, 100), "an end before the reported length is refused");
        check(!PackDocs.descriptor(true, 100, 100, 120), "a reported length the file does not have is refused");

        PackRows r = rows();
        check(r.base.state == PackRows.State.EMPTY && r.update.state == PackRows.State.EMPTY && r.keys.state == PackRows.State.EMPTY && !r.ready(), "nothing picked");
        r = rows("a.nsp", "package\tid1", "b.nsp", "package\tid2");
        check(r.base.state == PackRows.State.WAITING && r.update.state == PackRows.State.WAITING && r.keys.state == PackRows.State.EMPTY, "packages wait for the key file");
        r = rows("prod.keys", "keys", "b.nsp", "update\tid2", "a.nsp", "base\tid1");
        check(r.ready() && r.base.pick.name.equals("a.nsp") && r.update.pick.name.equals("b.nsp") && r.keys.pick.name.equals("prod.keys"), "classified by content, not by order");
        r = rows("prod.keys", "keys", "a.nsp", "PB-KEYMISSING\ttitlekek_0a", "b.nsp", "update\tid2");
        check(r.keys.state == PackRows.State.ERROR && r.keys.error.equals("PB-KEYMISSING") && r.keys.detail.equals("titlekek_0a") && !r.ready(), "a missing key entry shows on the key row with its name");
        r = rows("short.keys", "keys", "a.nsp", "PB-KEYMISSING\ttitlekek_0a", "b.nsp", "PB-KEYMISSING\ttitlekek_0a");
        check(r.base.state == PackRows.State.WAITING && r.update.state == PackRows.State.WAITING, "packages the key file cannot open wait instead of reading 'not chosen'");
        r = rows("bad.keys", "PB-KEYMISSING\theader_key");
        check(r.keys.state == PackRows.State.ERROR && r.base.state == PackRows.State.EMPTY, "a key file without header_key: only the key row");
        r = rows("old.keys", "keys", "new.keys", "keys");
        check(r.keys.pick.name.equals("new.keys"), "a later key file replaces the earlier one");
        r = rows("film.mp4", "PB-NOTGAME\t", "prod.keys", "keys");
        check(r.refused != null && r.refused.name.equals("film.mp4") && r.keys.state == PackRows.State.OK, "a file of no row shows as the message line");
        r = rows("cloud.nsp", "PB-NOTLOCAL\t");
        check(r.refused.kind().equals("PB-NOTLOCAL"), "a file that is not local is refused with PB-NOTLOCAL");


        PackRows.Row B = PackRows.Row.BASE, U = PackRows.Row.UPDATE, K = PackRows.Row.KEYS;
        r = picks(p("a.nsp", "package\tid1", U));
        check(r.update.state == PackRows.State.WAITING && r.update.pick.name.equals("a.nsp") && r.base.state == PackRows.State.EMPTY,
            "a package picked from the update row waits there, the base row stays empty");
        r = picks(p("a.nsp", "package\tid1", B), p("b.nsp", "package\tid2", U), p("prod.keys", "keys", K));
        check(r.base.state == PackRows.State.WAITING && r.update.state == PackRows.State.WAITING && r.keys.state == PackRows.State.OK && !r.ready(),
            "each package waits in its own row before the packages are probed with the key file");
        r = picks(p("prod.keys", "keys", K), p("b.nsp", "base\tid1", U), p("a.nsp", "update\tid2", B));
        check(r.ready() && r.base.pick.name.equals("b.nsp") && r.update.pick.name.equals("a.nsp"), "a package picked from the wrong row lands in its own row");
        r = picks(p("prod.keys", "keys", B), p("a.nsp", "base\tid1", B), p("b.nsp", "update\tid2", B), p("dlc.nsp", "addon\tid3", B));
        check(r.ready() && r.keys.pick.name.equals("prod.keys") && r.refused == null, "several files picked at once from one row are sorted into their rows, the add-on quietly left out");
        r = picks(p("prod.keys", "keys", K), p("a.nsp", "base\tid1", B), p("b.nsp", "update\tid2", U), p("a2.nsp", "base\tid9", B));
        check(r.ready() && r.base.pick.name.equals("a2.nsp") && r.update.pick.name.equals("b.nsp") && r.keys.pick.name.equals("prod.keys"), "picking a row again replaces only that row");
        r = picks(p("prod.keys", "keys", K), p("a.nsp", "base\tid1", B), p("b.nsp", "update\tid2", U), p("new.keys", "keys", K));
        check(r.ready() && r.keys.pick.name.equals("new.keys") && r.base.pick.name.equals("a.nsp"), "a new key file on the key row leaves the packages");
        r = picks(p("a.nsp", "package\tid1", K));
        check(r.base.state == PackRows.State.WAITING && r.update.state == PackRows.State.EMPTY, "a package picked from the key row waits in the first free package row");

        PackRows.Pick bundle = p("bundle.nsp", "package\tid1", B);
        r = picks(bundle);
        check(bundle.kept() && r.base.state == PackRows.State.WAITING && r.base.pick.name.equals("bundle.nsp") && r.refused == null,
            "a package without keys (even one holding an add-on) waits in its row and stays picked");
        r = picks(p("prod.keys", "keys", K), p("bundle.nsp", "base\tid1", B));
        check(r.base.state == PackRows.State.OK && r.base.pick.name.equals("bundle.nsp"), "probed again with the key file it ends in the base row");
        r = picks(p("prod.keys", "keys", K), p("dlc.nsp", "addon\tid3", B));
        check(r.base.state == PackRows.State.EMPTY && r.refused == null, "only the probe with keys drops an add-on, without a word");
        r = picks(p("dlc.nsp", "addon\tid3", B));
        check(r.base.state == PackRows.State.EMPTY && r.update.state == PackRows.State.EMPTY && r.keys.state == PackRows.State.EMPTY && r.refused == null,
            "an add-on package alone: no message, no row");
        check(!p("dlc.nsp", "addon\tid3", B).kept() && !p("film.mp4", "PB-NOTGAME\tfile", B).kept() && p("a.nsp", "package\tid1", B).kept()
            && p("a.nsp", "PB-KEYWRONG\ttitlekek_0a", B).kept() && p("prod.keys", "keys", K).kept(), "add-ons and refused files are not carried into the next pick");
        r = picks(p("prod.keys", "keys", K), p("a.nsp", "base\tid1", B), p("film.mp4", "PB-NOTGAME\tfile", U));
        check(r.refused != null && r.refused.name.equals("film.mp4") && r.update.state == PackRows.State.EMPTY && r.base.state == PackRows.State.OK,
            "a file of another game: the message line, the row it was picked for stays as it was");

        List<PackRows.Pick> restored = List.of(p("a.nsp", "base\tid1", B), p("prod.keys", "keys", K));
        PackRows.Missing lost = new PackRows.Missing(U, "PB-SOURCE", "update");
        r = PackRows.of(restored, lost);
        check(r.update.state == PackRows.State.ERROR && r.base.state == PackRows.State.OK && r.keys.state == PackRows.State.OK, "after a restart only the lost row asks");
        List<PackRows.Pick> refilled = new ArrayList<>(restored);
        refilled.add(p("b.nsp", "update\tid2", U));
        check(PackRows.of(refilled, lost).ready(), "the lost row picked again from its own button");
        refilled.set(2, p("dlc.nsp", "addon\tid3", U));
        r = PackRows.of(refilled, lost);
        check(r.update.state == PackRows.State.ERROR && r.refused == null, "an add-on picked for the lost row: it keeps asking, without a message");


        PackBuildState st = PackBuildState.parse(new PackBuildState(PackBuildState.Phase.RUNNING, 12, PackProgress.Step.EXTRACT, "00-00000001.data", null, null).format());
        check(st.step == PackProgress.Step.EXTRACT && st.file.equals("00-00000001.data") && st.percent == 12, "the step survives the state file");
        check(PackBuildState.parse("phase\trunning\npercent\t3\nfile\tmain\n").step == null, "a state without a step (written before steps were named)");
        check(PackBuildState.parse("phase\trunning\npercent\t3\nstep\tinstall\n").step == PackProgress.Step.INSTALL, "every step name reads back");



        long gib = 1L << 30;
        PackSpace.Write rewrite = new PackSpace.Write(1, gib, gib), fresh = new PackSpace.Write(1, gib, 0);
        PackSpace.Check c = PackSpace.check(List.of(List.of(rewrite), List.of(rewrite)), Map.of(1L, gib + gib / 2));
        check(c.fits() && c.need == gib, "two 1 GiB replacements fit into 1.5 GiB: need " + c.need);
        c = PackSpace.check(List.of(List.of(fresh), List.of(fresh)), Map.of(1L, gib + gib / 2));
        check(!c.fits() && c.need == 2 * gib && c.missing == gib / 2, "two new 1 GiB files do not: missing " + c.missing);
        c = PackSpace.check(List.of(List.of(rewrite, rewrite)), Map.of(1L, gib + gib / 2));
        check(!c.fits() && c.need == 2 * gib, "the files of one unit count in full");
        c = PackSpace.check(List.of(List.of(new PackSpace.Write(1, 100, 300)), List.of(new PackSpace.Write(1, 150, 0))), Map.of(1L, 100L));
        check(c.fits() && c.need == 100, "space freed by a smaller rewrite is used by the next unit: need " + c.need);
        c = PackSpace.check(List.of(List.of(new PackSpace.Write(1, 500, 0)), List.of(new PackSpace.Write(2, 700, 0)), List.of()), Map.of(1L, 600L, 2L, 600L));
        check(!c.fits() && c.need == 1200 && c.missing == 100 && c.free == 600, "each volume on its own: " + c.need + " / " + c.missing);
        c = PackSpace.check(List.of(List.of()), Map.of(1L, 5L));
        check(c.fits() && c.need == 0, "nothing to write needs nothing");


        check(PackSpace.missing("1048576") == 1048576L && PackSpace.missing("1") == 1L, "the missing bytes of a PB-SPACE detail");
        PackBuildState saved = PackBuildState.parse("phase\terror\npercent\t41\nfile\t00-00000002.data\nerror\tPB-SPACE\ndetail\t00-00000002.data\n");
        check(saved.error.equals("PB-SPACE") && PackSpace.missing(saved.detail) == null, "an older saved space error naming a file has no amount (generic line, no crash)");
        check(PackSpace.missing("") == null && PackSpace.missing(null) == null && PackSpace.missing("-5") == null && PackSpace.missing("99999999999999999999") == null, "no amount without a plain byte count");


        check(PackRows.repick("PB-SOURCE", "update") == PackRows.Row.UPDATE && PackRows.repick("PB-SOURCE", "keys") == PackRows.Row.KEYS
            && PackRows.repick("PB-SOURCE", "base") == PackRows.Row.BASE, "PB-SOURCE names the row to pick again");
        check(PackRows.repick("PB-KEYWRONG", "titlekek_0a") == PackRows.Row.KEYS && PackRows.repick("PB-KEYMISSING", "header_key") == PackRows.Row.KEYS, "keys that no longer open the packages: the key file again");
        check(PackRows.repick("PB-SOURCE", "read") == null && PackRows.repick("PB-DAMAGED", "00-00000001.data") == null && PackRows.repick("PB-SPACE", "5") == null, "other errors are no reason to pick again");


        check(PackRows.recheck(PackRows.Row.BASE, "base\tid1", "id1") == null && PackRows.recheck(PackRows.Row.UPDATE, "update\tid2", "id2") == null, "the same packages pass");
        String cut = PackRows.recheck(PackRows.Row.BASE, "PB-NOTGAME\tcontainer", "id1");
        check("PB-SOURCE\tbase".equals(cut) && PackRows.repick("PB-SOURCE", cut.substring(cut.indexOf('\t') + 1)) == PackRows.Row.BASE, "a base cut short (PB-NOTGAME) is picked again on its row: " + cut);
        check("PB-SOURCE\tupdate".equals(PackRows.recheck(PackRows.Row.UPDATE, "PB-DAMAGED\tpackage", "id2")), "an update that no longer reads is picked again on its row");
        check("PB-SOURCE\tbase".equals(PackRows.recheck(PackRows.Row.BASE, "base\tid9", "id1")) && "PB-SOURCE\tbase".equals(PackRows.recheck(PackRows.Row.BASE, "update\tid1", "id1")),
            "another package at the recorded place is picked again");
        String keyed = PackRows.recheck(PackRows.Row.UPDATE, "PB-KEYWRONG\ttitlekek_0a", "id2");
        check("PB-KEYWRONG\ttitlekek_0a".equals(keyed) && PackRows.repick("PB-KEYWRONG", "titlekek_0a") == PackRows.Row.KEYS, "keys that no longer open a package go to the key row");

        List<PackRows.Pick> kept = List.of(new PackRows.Pick("content://x/a", "a.nsp", "package\tid1"), new PackRows.Pick("content://x/b", "b.nsp", "package\tid2"));
        r = PackRows.of(kept, new PackRows.Missing(PackRows.Row.KEYS, "PB-SOURCE", "keys"));
        check(r.keys.state == PackRows.State.ERROR && r.keys.error.equals("PB-SOURCE") && r.keys.detail.equals("keys") && r.base.state == PackRows.State.WAITING && !r.ready(),
            "the key file gone: its row asks again, the packages wait for it");
        List<PackRows.Pick> again = new ArrayList<>(List.of(new PackRows.Pick("content://x/k", "prod.keys", "keys"), new PackRows.Pick("content://x/a", "a.nsp", "base\tid1"), new PackRows.Pick("content://x/b", "b.nsp", "update\tid2")));
        check(PackRows.of(again, new PackRows.Missing(PackRows.Row.KEYS, "PB-SOURCE", "keys")).ready(), "picking a key file again fills the row");
        r = PackRows.of(List.of(new PackRows.Pick("content://x/k", "prod.keys", "keys"), new PackRows.Pick("content://x/a", "a.nsp", "base\tid1")), new PackRows.Missing(PackRows.Row.UPDATE, "PB-SOURCE", "update"));
        check(r.update.state == PackRows.State.ERROR && r.base.state == PackRows.State.OK && r.keys.state == PackRows.State.OK, "only the missing row asks");


        Map<String, PackInPlace.Where> inputs = new java.util.LinkedHashMap<>();
        inputs.put("base", new PackInPlace.Where(1, 100, "/data/media/0/Download/base.nsp"));
        inputs.put("keys", new PackInPlace.Where(1, 200, "/storage/emulated/0/Download/prod.keys"));
        List<PackInPlace.Where> outs = new ArrayList<>();
        outs.add(new PackInPlace.Where(2, 300, "/data/user/0/app/files/dm-src/ExeFS/main"));
        check(PackInPlace.collision(inputs, outs) == null, "inputs elsewhere: no collision");
        outs.add(new PackInPlace.Where(1, 200, "/storage/emulated/0/Download/S2X/D2R Mobile/data/build_key.txt"));
        check("keys".equals(PackInPlace.collision(inputs, outs)), "a hard link of the key file at an output: same device and inode");
        outs.remove(outs.size() - 1);
        outs.add(new PackInPlace.Where(9, 999, "/storage/emulated/0/Download/base.nsp"));
        check("base".equals(PackInPlace.collision(inputs, outs)), "the same file under the lower and the user-visible path");
        check(PackInPlace.canonical("/sdcard/Download/x").equals("/storage/emulated/0/Download/x") && PackInPlace.canonical("/mnt/user/0/emulated/0/Download/x").equals("/storage/emulated/0/Download/x")
            && PackInPlace.canonical("/storage/self/primary/x").equals("/storage/emulated/0/x") && PackInPlace.canonical("/data/media/10/x").equals("/storage/emulated/10/x"), "shared storage aliases");

        Map<Long, long[]> one = new HashMap<>();
        one.put(1L, new long[]{5_679_052_956L, 12_300_000_000L});
        c = PackSpace.check(one);
        check(c.fits() && c.need == 5_679_052_956L && c.free == 12_300_000_000L, "fits on one volume");
        Map<Long, long[]> two = new HashMap<>();
        two.put(1L, new long[]{5_000_000_000L, 4_000_000_000L});
        two.put(2L, new long[]{30_000_000L, 9_000_000_000L});
        c = PackSpace.check(two);
        check(!c.fits() && c.missing == 1_000_000_000L && c.need == 5_030_000_000L && c.free == 4_000_000_000L, "the tightest volume decides and is shown");
        check(PackSpace.size(5_679_052_956L, Locale.GERMANY).equals("5,7 GB") && PackSpace.size(5_679_052_956L, Locale.US).equals("5.7 GB"), "gigabytes with one decimal in the phone's locale");
        check(PackSpace.size(740_100_000L, Locale.US).equals("741 MB") && PackSpace.size(12, Locale.US).equals("1 MB"), "megabytes rounded up, never 0");

        System.out.println("PackPickTest: " + checks + " checks, " + fails + " failed");
        if (fails > 0) System.exit(1);
    }
}
