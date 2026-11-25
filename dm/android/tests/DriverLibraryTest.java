package com.xiaoboluo.d2r;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;




public class DriverLibraryTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    static final List<File> synced = new ArrayList<>();
    static final DriverLibrary.Sync SYNC = dir -> synced.add(dir);

    public static void main(String[] a) throws Exception {
        File base = Files.createTempDirectory("drv-lib").toFile(), ext = Files.createTempDirectory("drv-ext").toFile();


        check(DriverLibrary.list(base).isEmpty(), "fresh library is empty");
        check(DriverLibrary.selected(base).isEmpty(), "fresh base has no selection");
        check(DriverLibrary.migrate(base, ext, SYNC) == null, "an empty external folder migrates nothing");
        check(!new File(base, DriverLibrary.MARKER).isFile(), "an empty external folder writes no marker");


        byte[] zip1 = makeZip("First Driver", "1.0");
        DriverLibrary.Add r1 = add(base, zip1);
        check(r1.error == null && DriverLibrary.validSha(r1.sha), "add: no error, the sha is valid");
        check("First Driver".equals(r1.name) && "1.0".equals(r1.version), "add: name and version come back");
        check(DriverLibrary.zipFor(base, r1.sha).isFile(), "add: the ZIP sits in lib/<sha>.zip");
        check(!new File(base, DriverLibrary.INCOMING).exists(), "add: no incoming.tmp left behind");
        List<DriverLibrary.Entry> ls = DriverLibrary.list(base);
        check(ls.size() == 1 && ls.get(0).sha.equals(r1.sha) && "First Driver".equals(ls.get(0).name), "list: the one entry with its meta");


        DriverLibrary.Add r2 = add(base, zip1);
        check(r2.error == null && r2.sha.equals(r1.sha) && DriverLibrary.list(base).size() == 1, "the same ZIP is not added twice");
        check(r2.name.equals("First Driver"), "a duplicate answers with the existing entry's name");


        synced.clear();
        DriverLibrary.select(base, r1.sha, SYNC);
        check(DriverLibrary.selected(base).equals(r1.sha), "selection: the sha reads back");
        String raw = new String(Files.readAllBytes(new File(base, DriverLibrary.SELECTED).toPath()), StandardCharsets.UTF_8);
        check(raw.equals(r1.sha + "\n"), "selection: the file holds exactly the sha");
        check(!new File(base, DriverLibrary.SELECTED + ".tmp").exists(), "selection: the atomic write leaves no temp file");
        check(synced.contains(base), "selection: the directory is synced");
        try { DriverLibrary.select(base, "nonsense", SYNC); check(false, "selection: a bad sha is refused"); }
        catch (IOException e) { check(DriverLibrary.selected(base).equals(r1.sha), "selection: a bad sha is refused, the old one stays"); }


        Files.write(new File(base, DriverLibrary.SELECTED).toPath(), "这不是 sha!\n".getBytes(StandardCharsets.UTF_8));
        check(DriverLibrary.selected(base).isEmpty(), "a corrupt selected.txt reads as the system driver");
        DriverLibrary.select(base, r1.sha, SYNC);
        check(DriverLibrary.selected(base).equals(r1.sha), "a good write heals the corrupt file");


        for (int i = 2; i <= 8; i++) {
            byte[] z = makeZip("Driver " + i, "1." + i);
            check(add(base, z).error == null, "add driver " + i + " of 8");
        }
        check(DriverLibrary.list(base).size() == 8, "the library holds eight");
        DriverLibrary.Add r9 = add(base, makeZip("Driver 9", "1.9"));
        check("driver-full".equals(r9.error) && DriverLibrary.list(base).size() == 8, "the ninth entry is refused, the library stays at eight");
        check(!new File(base, DriverLibrary.INCOMING).exists(), "a refused incoming file is deleted");


        check(DriverLibrary.delete(base, r1.sha, List.of(), SYNC), "delete: the selected entry goes");
        check(DriverLibrary.selected(base).isEmpty(), "delete of the selected entry: the selection falls back to the system driver");
        check(!DriverLibrary.zipFor(base, r1.sha).isFile(), "delete: the ZIP is gone");
        check(!DriverLibrary.delete(base, r1.sha, List.of(), SYNC), "deleting an entry that is not there says so");


        File base2 = Files.createTempDirectory("drv-lib2").toFile(), ext2 = Files.createTempDirectory("drv-ext2").toFile();
        byte[] one = makeZip("Migrated One", "9.0");
        File extZip = new File(ext2, "driver.zip");
        Files.write(extZip.toPath(), one);
        DriverLibrary.Migration m1 = DriverLibrary.migrate(base2, ext2, SYNC);
        check(m1 != null && m1.imported == 1 && m1.sha.equals(DriverLibrary.sha256(extZip)), "migration: one ZIP in, named by its sha");
        check(DriverLibrary.selected(base2).equals(m1.sha), "migration of one ZIP selects it");
        check(extZip.isFile(), "migration leaves the external folder alone");
        check(new File(base2, DriverLibrary.MARKER).isFile(), "migration writes its marker once");
        check(DriverLibrary.migrate(base2, ext2, SYNC) == null, "the second look migrates nothing");


        File base3 = Files.createTempDirectory("drv-lib3").toFile(), ext3 = Files.createTempDirectory("drv-ext3").toFile();
        Files.write(new File(ext3, "a.zip").toPath(), makeZip("A", "1"));
        Files.write(new File(ext3, "b.zip").toPath(), makeZip("B", "2"));
        DriverLibrary.Migration m3 = DriverLibrary.migrate(base3, ext3, SYNC);
        check(m3 != null && m3.imported == 2 && m3.sha.isEmpty(), "migration of several ZIPs imports all, selects none");
        check(DriverLibrary.selected(base3).isEmpty(), "after migrating several: the system driver stays chosen");


        File base4 = Files.createTempDirectory("drv-lib4").toFile(), ext4 = Files.createTempDirectory("drv-ext4").toFile();
        check(add(base4, makeZip("Picked Earlier", "0.1")).error == null, "a ZIP added through the picker sits in the library");
        Files.write(new File(ext4, "old.zip").toPath(), makeZip("Old Folder Zip", "0"));
        check(DriverLibrary.migrate(base4, ext4, SYNC) == null, "a filled library is not migrated into");
        check(DriverLibrary.list(base4).size() == 1 && new File(base4, DriverLibrary.MARKER).isFile(), "the library keeps the picked entry and the marker is set");



        File base5 = Files.createTempDirectory("drv-lib5").toFile(), ext5 = Files.createTempDirectory("drv-ext5").toFile();
        byte[] goodZip = makeZip("Good One", "7.7");
        Files.write(new File(ext5, "one.zip").toPath(), goodZip);
        String goodSha = DriverLibrary.sha256(new File(ext5, "one.zip"));
        DriverLibrary.libDir(base5).mkdirs();
        File partial = new File(DriverLibrary.libDir(base5), goodSha + ".zip");
        Files.write(partial.toPath(), Arrays.copyOf(goodZip, 40));
        DriverLibrary.Migration m5 = DriverLibrary.migrate(base5, ext5, SYNC);
        check(m5 != null && m5.imported == 1 && m5.sha.equals(goodSha), "the half entry is dropped and the migration runs again");
        check(!partial.exists() || DriverLibrary.zipFor(base5, goodSha).length() == goodZip.length, "the library holds the complete ZIP, not the half one");
        List<DriverLibrary.Entry> ls5 = DriverLibrary.list(base5);
        check(ls5.size() == 1 && "Good One".equals(ls5.get(0).name), "only the complete entry stands in the library");
        check(DriverLibrary.selected(base5).equals(goodSha), "the re-run migration of one ZIP selects it");
        boolean leftTmp = false;
        for (File f : DriverLibrary.libDir(base5).listFiles()) if (f.getName().endsWith(".tmp")) leftTmp = true;
        check(!leftTmp, "no temp file is left in the library after the migration");



        File base6 = Files.createTempDirectory("drv-lib6").toFile(), ext6 = Files.createTempDirectory("drv-ext6").toFile();
        Files.write(new File(ext6, "one.zip").toPath(), makeZip("Sixth", "6.0"));
        String sha6 = DriverLibrary.sha256(new File(ext6, "one.zip"));
        DriverLibrary.Sync crashOnce = dir -> { synced.add(dir); if (dir.equals(base6)) throw new IOException("the phone died here"); };
        try { DriverLibrary.migrate(base6, ext6, crashOnce); check(false, "a failing directory sync is reported"); }
        catch (IOException e) { check(true, "a failing directory sync is reported"); }
        check(DriverLibrary.selected(base6).equals(sha6), "the selection is on disk before the marker: the crash keeps it");
        check(!new File(base6, DriverLibrary.MARKER).isFile(), "the crash stopped before the marker");
        check(DriverLibrary.migrate(base6, ext6, SYNC) == null && new File(base6, DriverLibrary.MARKER).isFile()
            && DriverLibrary.selected(base6).equals(sha6), "the next start completes the migration and keeps the selection");



        File base7 = Files.createTempDirectory("drv-lib7").toFile();
        DriverLibrary.Add r7 = add(base7, zipWithOversizedMeta("Late"));
        check(!"Late".equals(r7.name), "an oversized meta.json stops the read: no name from a later entry");
        DriverLibrary.Entry e7 = DriverLibrary.list(base7).get(0);
        check(e7.name.equals(r7.sha) && e7.version.isEmpty(), "the oversized-meta ZIP shows the file name, nothing more is read");



        File base8 = Files.createTempDirectory("drv-lib8").toFile();
        DriverLibrary.Add r8 = add(base8, makeZip("Cached Driver", "3.1"));
        File cache8 = new File(DriverLibrary.libDir(base8), r8.sha + ".zip" + DriverLibrary.META_SUFFIX);
        check(cache8.isFile(), "adding writes the name and version side file");
        Files.write(DriverLibrary.zipFor(base8, r8.sha).toPath(), "not a zip at all".getBytes(StandardCharsets.UTF_8));
        List<DriverLibrary.Entry> ls8 = DriverLibrary.list(base8);
        check(ls8.size() == 1 && "Cached Driver".equals(ls8.get(0).name) && "3.1".equals(ls8.get(0).version), "the list reads the side file, not the ZIP");
        cache8.delete();
        check(DriverLibrary.list(base8).get(0).name.equals(r8.sha), "without the side file a broken ZIP falls back to the file name");
        check(cache8.isFile(), "the side file is written back after the fallback");
        check(DriverLibrary.delete(base8, r8.sha, List.of(), SYNC) && !cache8.exists(), "deleting takes the side file with the entry");


        File pBase = Files.createTempDirectory("drv-placed").toFile();
        File pDir = new File(pBase, "work"); pDir.mkdir();
        File pMain = new File(pDir, "vulkan.adr891.so"); Files.write(pMain.toPath(), "main library".getBytes(StandardCharsets.UTF_8));
        File pHlp = new File(pDir, "vkbgsl.so"); Files.write(pHlp.toPath(), "helper one".getBytes(StandardCharsets.UTF_8));
        File pHlp2 = new File(pDir, "vkbcxx.so"); Files.write(pHlp2.toPath(), "helper two".getBytes(StandardCharsets.UTF_8));
        String key = DriverLibrary.placedHash(pDir);
        check(DriverLibrary.validSha(key) && !key.equals(DriverLibrary.sha256(pMain)), "the set key covers every file, it is not the main library's sha");
        byte[] hlp = Files.readAllBytes(pHlp.toPath());
        Files.write(pHlp.toPath(), "tampered".getBytes(StandardCharsets.UTF_8));
        check(!key.equals(DriverLibrary.placedHash(pDir)), "a changed helper is another key");
        Files.write(pHlp.toPath(), hlp);
        pHlp2.delete();
        check(!key.equals(DriverLibrary.placedHash(pDir)), "a missing helper is another key");
        Files.write(pHlp2.toPath(), "helper two".getBytes(StandardCharsets.UTF_8));
        check(key.equals(DriverLibrary.placedHash(pDir)), "the key is back once every file stands again");
        Files.write(new File(pDir, "junk.bin").toPath(), "x".getBytes(StandardCharsets.UTF_8));
        check(!key.equals(DriverLibrary.placedHash(pDir)), "an extra file is another key");
        new File(pDir, "junk.bin").delete();
        pDir.renameTo(new File(pBase, key));
        File placed = new File(pBase, key);
        check(key.equals(DriverLibrary.verifyPlaced(pBase, key, "vulkan.adr891.so")), "a placed directory verifies against its key");
        Files.write(new File(placed, "junk.bin").toPath(), "x".getBytes(StandardCharsets.UTF_8));
        check(DriverLibrary.verifyPlaced(pBase, key, "vulkan.adr891.so") == null, "an extra file reads as so-changed");
        new File(placed, "junk.bin").delete();
        File placedHlp = new File(placed, "vkbgsl.so");
        placedHlp.delete();
        check(DriverLibrary.verifyPlaced(pBase, key, "vulkan.adr891.so") == null, "a missing helper reads as so-changed");
        Files.write(placedHlp.toPath(), hlp);
        check(key.equals(DriverLibrary.verifyPlaced(pBase, key, "vulkan.adr891.so")), "complete again, it verifies again");



        File oBase = Files.createTempDirectory("drv-old").toFile();
        File oDir = new File(oBase, "b".repeat(64)); oDir.mkdir();
        File oLib = new File(oDir, "vulkan.turnip.so"); Files.write(oLib.toPath(), "one library bytes".getBytes(StandardCharsets.UTF_8));
        String oKey = DriverLibrary.sha256(oLib);
        File oReal = new File(oBase, oKey); oDir.renameTo(oReal);
        String movedKey = DriverLibrary.verifyPlaced(oBase, oKey, "vulkan.turnip.so");
        check(movedKey != null && !oReal.exists() && new File(oBase, movedKey).isDirectory()
            && movedKey.equals(DriverLibrary.placedHash(new File(oBase, movedKey))), "the old single-library directory migrates to its set key");
        check(movedKey.equals(DriverLibrary.verifyPlaced(oBase, movedKey, "vulkan.turnip.so")), "the migrated key verifies every load after");
        File o2 = Files.createTempDirectory("drv-old2").toFile();
        File o2Dir = new File(o2, "c".repeat(64)); o2Dir.mkdir();
        Files.write(new File(o2Dir, "vulkan.turnip.so").toPath(), "different bytes".getBytes(StandardCharsets.UTF_8));
        check(DriverLibrary.verifyPlaced(o2, "c".repeat(64), "vulkan.turnip.so") == null, "old-named directory with changed bytes reads as so-changed");




        File dB = Files.createTempDirectory("drv-del").toFile();
        DriverLibrary.Add dAdd = add(dB, makeZip("Big Pack", "2.0"));
        String dKey = "d".repeat(64);
        Files.write(DriverLibrary.dirNote(DriverLibrary.zipFor(dB, dAdd.sha)).toPath(), (dKey + "\n").getBytes(StandardCharsets.UTF_8));
        File dDir = new File(dB, dKey); dDir.mkdir();
        Files.write(new File(dDir, "vulkan.big.so").toPath(), "library".getBytes(StandardCharsets.UTF_8));
        check(DriverLibrary.delete(dB, dAdd.sha, List.of(), SYNC) && !dDir.exists() && !DriverLibrary.dirNote(DriverLibrary.zipFor(dB, dAdd.sha)).exists(),
            "deleting an entry removes the ZIP and the unpacked directory");


        File gB = Files.createTempDirectory("drv-gc").toFile();
        DriverLibrary.Add gAdd = add(gB, makeZip("Still In", "3.0"));
        String gKey = "e".repeat(64), gGone = "f".repeat(64);
        Files.write(DriverLibrary.dirNote(DriverLibrary.zipFor(gB, gAdd.sha)).toPath(), (gKey + "\n").getBytes(StandardCharsets.UTF_8));
        new File(gB, gKey).mkdir(); new File(gB, gGone).mkdir();
        check(DriverLibrary.cleanPlaced(gB, List.of()) == 1 && new File(gB, gKey).isDirectory() && !new File(gB, gGone).isDirectory(),
            "cleanPlaced drops only the directory no entry uses");
        check(DriverLibrary.cleanPlaced(gB, List.of(gKey)) == 0, "cleanPlaced keeps what the notes and the start keep");




        File shB = Files.createTempDirectory("drv-share").toFile();
        DriverLibrary.Add sh1 = add(shB, makeZip("Share A", "1")), sh2 = add(shB, makeZip("Share B", "2"));
        String shKey = "1".repeat(64);
        Files.write(DriverLibrary.dirNote(DriverLibrary.zipFor(shB, sh1.sha)).toPath(), (shKey + "\n").getBytes(StandardCharsets.UTF_8));
        Files.write(DriverLibrary.dirNote(DriverLibrary.zipFor(shB, sh2.sha)).toPath(), (shKey + "\n").getBytes(StandardCharsets.UTF_8));
        File shDir = new File(shB, shKey); shDir.mkdir();
        Files.write(new File(shDir, "vulkan.shared.so").toPath(), "shared library".getBytes(StandardCharsets.UTF_8));
        check(DriverLibrary.delete(shB, sh1.sha, List.of(), SYNC) && shDir.isDirectory() && !DriverLibrary.zipFor(shB, sh1.sha).isFile(),
            "delete keeps a set the remaining entry still notes");
        check(DriverLibrary.delete(shB, sh2.sha, List.of(), SYNC) && !shDir.exists(), "the last entry with the set takes the directory with it");



        DriverLibrary.Add sh3 = add(shB, makeZip("Share C", "3"));
        String runKey = "6".repeat(64);
        Files.write(DriverLibrary.dirNote(DriverLibrary.zipFor(shB, sh3.sha)).toPath(), (runKey + "\n").getBytes(StandardCharsets.UTF_8));
        File runDir = new File(shB, runKey); runDir.mkdir();
        check(DriverLibrary.delete(shB, sh3.sha, List.of(runKey), SYNC) && runDir.isDirectory(), "delete keeps the set this process runs");
        check(DriverLibrary.cleanPlaced(shB, List.of()) == 1 && !runDir.exists(), "the next start's sweep drops the set the process left");




        File tB = Files.createTempDirectory("drv-link").toFile();
        File outside = Files.createTempDirectory("drv-outside").toFile();
        File sentinel = new File(outside, "sentinel.bin");
        Files.write(sentinel.toPath(), "helper one".getBytes(StandardCharsets.UTF_8));
        File outDir = new File(outside, "outside-dir"); outDir.mkdir();
        Files.write(new File(outDir, "keep.me").toPath(), "outside".getBytes(StandardCharsets.UTF_8));
        File tDir = new File(tB, "work"); tDir.mkdir();
        Files.write(new File(tDir, "vulkan.adr891.so").toPath(), "main library".getBytes(StandardCharsets.UTF_8));
        Files.write(new File(tDir, "vkbgsl.so").toPath(), "helper one".getBytes(StandardCharsets.UTF_8));
        String tKey = DriverLibrary.placedHash(tDir);
        tDir.renameTo(new File(tB, tKey));
        new File(tB, tKey + "/vkbgsl.so").delete();
        Files.createSymbolicLink(new File(tB, tKey + "/vkbgsl.so").toPath(), sentinel.toPath());
        check(DriverLibrary.verifyPlaced(tB, tKey, "vulkan.adr891.so") == null, "a symlinked helper reads as so-changed");
        check(DriverLibrary.cleanPlaced(tB, List.of()) == 1 && sentinel.isFile() && new File(outDir, "keep.me").isFile(),
            "cleaning a set with links leaves the outside file and directory intact");
        File l2B = Files.createTempDirectory("drv-link2").toFile(), l2Real = new File(outside, "real-set"); l2Real.mkdir();
        Files.write(new File(l2Real, "vulkan.solo.so").toPath(), "one library".getBytes(StandardCharsets.UTF_8));
        String l2Key = DriverLibrary.placedHash(l2Real);
        Files.createSymbolicLink(new File(l2B, l2Key).toPath(), l2Real.toPath());
        check(DriverLibrary.verifyPlaced(l2B, l2Key, "vulkan.solo.so") == null, "a symlinked set directory reads as so-changed");
        Files.write(new File(l2B, l2Key + "/extra.so").toPath(), "through the link".getBytes(StandardCharsets.UTF_8));
        check(DriverLibrary.verifyPlaced(l2B, l2Key, "vulkan.solo.so") == null && new File(l2Real, "extra.so").isFile() && l2Real.isDirectory(),
            "the sweep of a symlinked set directory only unlinks it, the target stays");
        DriverLibrary.libDir(l2B).mkdirs();
        check(DriverLibrary.cleanPlaced(l2B, List.of()) == 1 && !new File(l2B, l2Key).exists() && l2Real.isDirectory() && new File(l2Real, "vulkan.solo.so").isFile(),
            "cleanPlaced drops the symlink without touching what it points at");




        File dlB = Files.createTempDirectory("drv-dangling").toFile();
        DriverLibrary.libDir(dlB).mkdirs();
        File dlLink = new File(dlB, "a".repeat(64));
        Files.createSymbolicLink(dlLink.toPath(), new File(dlB, "target-that-is-gone").toPath());
        File dlPlain = new File(dlB, "b".repeat(64)); Files.write(dlPlain.toPath(), "not a set".getBytes(StandardCharsets.UTF_8));
        check(!dlLink.isDirectory() && DriverLibrary.cleanPlaced(dlB, List.of()) == 1 && !dlLink.exists() && dlPlain.isFile(),
            "cleanPlaced unlinks a dangling set link and leaves a plain key-named file alone");
        new File(dlB, "c".repeat(64)).mkdir();
        check(DriverLibrary.cleanPlaced(dlB, List.of()) == 1 && !new File(dlB, "c".repeat(64)).exists(), "a real unclaimed set directory still goes");



        File nB = Files.createTempDirectory("drv-note").toFile();
        DriverLibrary.libDir(nB).mkdirs();
        File ghost = new File(DriverLibrary.libDir(nB), "3".repeat(64) + ".zip");
        String goneKey = "2".repeat(64);
        Files.write(DriverLibrary.dirNote(ghost).toPath(), (goneKey + "\n").getBytes(StandardCharsets.UTF_8));
        Files.write(new File(ghost.getPath() + DriverLibrary.META_SUFFIX).toPath(), "Gone\n1\n".getBytes(StandardCharsets.UTF_8));
        File goneDir = new File(nB, goneKey); goneDir.mkdir();
        check(DriverLibrary.cleanPlaced(nB, List.of()) == 1 && !goneDir.exists()
            && !DriverLibrary.dirNote(ghost).exists() && !new File(ghost.getPath() + DriverLibrary.META_SUFFIX).exists(),
            "notes of ZIPs that are gone go, and their unclaimed set directory with them");
        File n2B = Files.createTempDirectory("drv-note2").toFile();
        DriverLibrary.Add nLive = add(n2B, makeZip("Live", "1"));
        String liveKey = "5".repeat(64);
        DriverLibrary.noteDir(DriverLibrary.zipFor(n2B, nLive.sha), liveKey);
        File liveDir = new File(n2B, liveKey); liveDir.mkdir();
        File ghost2 = new File(DriverLibrary.libDir(n2B), "4".repeat(64) + ".zip");
        Files.write(DriverLibrary.dirNote(ghost2).toPath(), (liveKey + "\n").getBytes(StandardCharsets.UTF_8));
        check(DriverLibrary.cleanPlaced(n2B, List.of()) == 0 && liveDir.isDirectory() && !DriverLibrary.dirNote(ghost2).exists(),
            "a stale note for a still-shared set goes while the directory stays with the live entry");



        String legacyLine = "turnip.zip|34181579|1763000000000\n" + "9".repeat(64) + "\n" + "8".repeat(64) + "\nvulkan.turnip.so\nTurnip 39\n\n";
        check(!DriverLibrary.recordCurrent(legacyLine.split("\n", -1)), "an old import.txt record without a version line is not trusted");
        check(DriverLibrary.recordCurrent((legacyLine + DriverLibrary.VALIDATION + "\n").split("\n", -1)), "a record with the current version is trusted");
        check(!DriverLibrary.recordCurrent(new String[0]), "a missing record is not trusted");
        String curLine = legacyLine + DriverLibrary.VALIDATION + "\n";
        check(DriverLibrary.recordTrusted(curLine.split("\n", -1)), "a well-formed current record can be used field by field");
        check(!DriverLibrary.recordTrusted(curLine.replace("vulkan.turnip.so", "vulkan\0turnip.so").split("\n", -1)),
            "a record whose library name holds a NUL is never handed out (its toPath would throw)");
        check(!DriverLibrary.recordTrusted(curLine.replace("9".repeat(64), "9".repeat(63) + "x").split("\n", -1)),
            "a record whose zip key is not 64 hex characters is never handed out");
        check(!DriverLibrary.recordTrusted(curLine.replace("8".repeat(64), "8".repeat(63) + "F").split("\n", -1)),
            "a record whose set key is uppercase is not hex - never handed out");



        File vB = Files.createTempDirectory("drv-version").toFile();
        byte[] solo = driverZip("Solo", "vulkan.solo.so");
        File soloZip = new File(vB, "solo.zip"); Files.write(soloZip.toPath(), solo);
        File oldDir = new File(vB, "7".repeat(64)); oldDir.mkdir();
        File oldLib = new File(oldDir, "vulkan.solo.so"); Files.write(oldLib.toPath(), "bytes of vulkan.solo.so".getBytes(StandardCharsets.UTF_8));
        String oldKey = DriverLibrary.sha256(oldLib);
        oldDir.renameTo(new File(vB, oldKey));
        String got = DriverLibrary.migrateLegacySet(vB, soloZip, 34, oldKey, "vulkan.solo.so");
        check(got != null && !new File(vB, oldKey).exists() && got.equals(DriverLibrary.placedHash(new File(vB, got)))
            && Arrays.equals(new File(vB, got).list(), new String[]{"vulkan.solo.so"}),
            "a legacy one-library package migrates by rename only");
        check(got != null && got.equals(DriverLibrary.verifyPlaced(vB, got, "vulkan.solo.so")), "the migrated directory verifies under its new key on every later look");



        File wB = Files.createTempDirectory("drv-version2").toFile();
        byte[] duo = driverZip("Duo", "vulkan.duo.so", "vkbduo_helper.so");
        File duoZip = new File(wB, "duo.zip"); Files.write(duoZip.toPath(), duo);
        File wDir = new File(wB, "8".repeat(64)); wDir.mkdir();
        File wLib = new File(wDir, "vulkan.duo.so"); Files.write(wLib.toPath(), "bytes of vulkan.duo.so".getBytes(StandardCharsets.UTF_8));
        String wKey = DriverLibrary.sha256(wLib);
        wDir.renameTo(new File(wB, wKey));
        check(DriverLibrary.migrateLegacySet(wB, duoZip, 34, wKey, "vulkan.duo.so") == null && new File(wB, wKey).isDirectory(),
            "a legacy one-file directory of a multi-library ZIP is not migrated");
        DriverZip.Meta dm = DriverZip.inspect(duoZip, 34);
        File wOut = new File(wB, "out"); wOut.mkdir();
        check(dm.error == null && dm.libraries.size() == 2 && DriverZip.extract(duoZip, wOut) && wOut.list().length == 2,
            "the same ZIP now yields its whole set");

        if (fails > 0) { System.out.println(fails + " failure(s)"); System.exit(1); }
        System.out.println("DriverLibraryTest: all passed");
    }


    static DriverLibrary.Add add(File base, byte[] data) throws IOException {
        File in = new File(base, DriverLibrary.INCOMING);
        try (FileOutputStream o = new FileOutputStream(in)) { o.write(data); }
        return DriverLibrary.add(base, in, SYNC);
    }






    static byte[] zipWithOversizedMeta(String lateName) throws IOException {
        byte[] big = new byte[DriverZip.MAX_META + 4096];
        byte[] second = ("{\"name\":\"" + lateName + "\",\"driverVersion\":\"9\"}").getBytes(StandardCharsets.UTF_8);
        byte[] name = "meta.json".getBytes(StandardCharsets.UTF_8);
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        localEntry(buf, name, big);
        localEntry(buf, name, second);
        return buf.toByteArray();
    }



    private static void localEntry(ByteArrayOutputStream buf, byte[] name, byte[] data) throws IOException {
        java.util.zip.CRC32 crc = new java.util.zip.CRC32(); crc.update(data);
        le(buf, 0x04034b50); le16(buf, 20); le16(buf, 0); le16(buf, 0); le16(buf, 0); le16(buf, 0);
        le(buf, crc.getValue()); le(buf, data.length); le(buf, data.length); le16(buf, name.length); le16(buf, 0);
        buf.write(name); buf.write(data);
    }
    private static void le(ByteArrayOutputStream buf, long v) { for (int i = 0; i < 4; i++) buf.write((int)(v >>> (8 * i)) & 0xff); }
    private static void le16(ByteArrayOutputStream buf, int v) { buf.write(v & 0xff); buf.write((v >>> 8) & 0xff); }



    static byte[] driverZip(String name, String... libs) throws IOException {
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        try (ZipOutputStream z = new ZipOutputStream(buf)) {
            z.putNextEntry(new ZipEntry("meta.json"));
            z.write(("{\"name\":\"" + name + "\",\"libraryName\":\"" + libs[0] + "\"}").getBytes(StandardCharsets.UTF_8));
            z.closeEntry();
            for (String l : libs) {
                z.putNextEntry(new ZipEntry(l));
                z.write(("bytes of " + l).getBytes(StandardCharsets.UTF_8));
                z.closeEntry();
            }
        }
        return buf.toByteArray();
    }

    static byte[] makeZip(String name, String version) throws IOException {
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        try (ZipOutputStream z = new ZipOutputStream(buf)) {
            z.putNextEntry(new ZipEntry("meta.json"));
            z.write(("{\"name\":\"" + name + "\",\"libraryName\":\"libtest_" + name + ".so\",\"driverVersion\":\"" + version + "\"}")
                .getBytes(StandardCharsets.UTF_8));
            z.closeEntry();
            z.putNextEntry(new ZipEntry("libtest_" + name + ".so"));
            z.write(("driver bytes of " + name + " " + version).getBytes(StandardCharsets.UTF_8));
            z.closeEntry();
        }
        return buf.toByteArray();
    }
}
