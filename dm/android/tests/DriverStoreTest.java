package com.xiaoboluo.d2r;

import android.content.Context;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;








public class DriverStoreTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    static final String LIB = "vulkan.adr891.so", H1 = "vkbh1.so", H2 = "vkbh2.so";
    static final DriverLibrary.Sync NOOP = dir -> { };

    public static void main(String[] a) throws Exception {
        File apk = fakeApk();


        File b = Files.createTempDirectory("drv-store").toFile();
        Context c = ctx(b, apk);
        File zip = addTo(b, driverZip("Adr Multi", LIB, H1, H2));
        DriverStore.Imported r = DriverStore.importZip(c, b, zip);
        check(r.error == null, "a multi-library package imports: " + r.error);
        check(r.soDir != null && new File(r.soDir, LIB).isFile() && new File(r.soDir, H1).isFile() && new File(r.soDir, H2).isFile(),
            "the whole set stands in the directory keyed over it");
        check(r.hooks != null && new File(r.hooks, DriverStore.HOOKS[0]).isFile() && new File(r.hooks, DriverStore.HOOKS[1]).isFile(),
            "the hook libraries come out of the shell APK");
        String[] rec = record(b);
        check(DriverLibrary.recordTrusted(rec) && rec[1].equals(r.zipSha) && rec[2].equals(r.setSha) && rec[3].equals(LIB),
            "the published record carries the zip key, the set key and the library");





        String key = zip.getName() + "|" + zip.length() + "|" + zip.lastModified();
        Files.write(new File(b, "import.txt").toPath(),
            (key + "\n" + r.zipSha + "\n" + r.setSha + "\nvulkan\0adr891.so\nAdr Multi\n\n2\n").getBytes(StandardCharsets.UTF_8));
        DriverStore.Imported r2 = null;
        try { r2 = DriverStore.importZip(c, b, zip); check(true, "a cached library name holding a NUL does not throw"); }
        catch (RuntimeException e) { check(false, "a cached library name holding a NUL does not throw, got " + e); }
        check(r2 != null && r2.error == null && r2.setSha.equals(r.setSha) && new File(r2.soDir, LIB).isFile(),
            "the NUL record is a cache miss: re-inspected and loaded");
        String[] fixed = record(b);
        check(DriverLibrary.recordTrusted(fixed) && fixed[3].equals(LIB), "the re-import rewrote the record clean");
        Files.write(new File(b, "import.txt").toPath(),
            (key + "\n" + "z".repeat(64) + "\n" + r.setSha + "\n" + LIB + "\nAdr Multi\n\n2\n").getBytes(StandardCharsets.UTF_8));
        DriverStore.Imported rb1 = DriverStore.importZip(c, b, zip);
        check(rb1.error == null && rb1.setSha.equals(r.setSha), "a record whose zip key is not 64 hex is a miss, not a failure");
        Files.write(new File(b, "import.txt").toPath(),
            (key + "\n" + r.zipSha + "\n" + "0".repeat(63) + "g\n" + LIB + "\nAdr Multi\n\n2\n").getBytes(StandardCharsets.UTF_8));
        DriverStore.Imported rb2 = DriverStore.importZip(c, b, zip);
        check(rb2.error == null && rb2.setSha.equals(r.setSha), "a record whose set key is not 64 hex is a miss, not a failure");




        File b2 = Files.createTempDirectory("drv-store2").toFile();
        Context c2 = ctx(b2, apk);
        File zip2 = addTo(b2, driverZip("Turnip at", "vulkan.t@st.so"));
        String key2 = zip2.getName() + "|" + zip2.length() + "|" + zip2.lastModified();
        Files.write(new File(b2, "import.txt").toPath(),
            (key2 + "\n" + DriverLibrary.sha256(zip2) + "\n" + "8".repeat(64) + "\nvulkan.t@st.so\nTurnip old\nlibrary-name\n").getBytes(StandardCharsets.UTF_8));
        DriverStore.Imported r3 = DriverStore.importZip(c2, b2, zip2);
        check(r3.error == null, "a ZIP the old rules rejected (library-name) passes the re-inspection: " + r3.error);
        check(new File(b2, r3.setSha + "/vulkan.t@st.so").isFile() && record(b2)[6].equals(DriverLibrary.VALIDATION),
            "the accepted package is placed and the record now carries the version");







        File b3 = Files.createTempDirectory("drv-store3").toFile();
        Context c3 = ctx(b3, apk);
        File zip3 = addTo(b3, driverZip("Link Pack", LIB, H1, H2));
        DriverStore.Imported r4 = DriverStore.importZip(c3, b3, zip3);
        check(r4.error == null, "the to-be-damaged set imports first: " + r4.error);
        File set3 = new File(b3, r4.setSha);
        set3.setWritable(true);
        File helper = new File(set3, H1);
        helper.delete();
        Files.createSymbolicLink(helper.toPath(), new File(b3, "target-that-is-gone").toPath());
        check(!helper.exists() && Files.isSymbolicLink(helper.toPath()), "the dangling link stands there unseen by the following exists()");
        DriverStore.Imported r5 = DriverStore.importZip(c3, b3, zip3);
        check(r5.error == null, "the start after the damage re-places the set (" + r5.error + ")");
        check(r5.setSha.equals(r4.setSha) && Files.isRegularFile(new File(b3, r5.setSha + "/" + H1).toPath()),
            "the re-placed set is complete again");
        check(DriverStore.importZip(c3, b3, zip3).error == null, "and the start after that loads it normally");


        Context ce = ctx(b, apk);
        DriverGuard.State open = new DriverGuard.State(); open.pending = "e1"; open.pendingZip = "Z1"; open.pendingPid = 4242; open.fails = 1;
        DriverGuard.Input next = new DriverGuard.Input(); next.pid = 1000;
        check(!DriverStore.endedOutside(ce, open, next), "no exit records service: counted");
        ce.activityManager = new android.app.ActivityManager();
        check(!DriverStore.endedOutside(ce, open, next), "no exit record of that process: counted");
        ce.activityManager.records.add(new android.app.ApplicationExitInfo(4243, DriverGuard.USER_REQUESTED, 0));
        check(!DriverStore.endedOutside(ce, open, next), "a record of another process does not excuse it");
        ce.activityManager.records.add(0, new android.app.ApplicationExitInfo(4242, DriverGuard.USER_REQUESTED, 0));
        check(DriverStore.endedOutside(ce, open, next), "a force stop of that process excuses it");
        ce.activityManager.records.set(0, new android.app.ApplicationExitInfo(4242, 5, 11));
        check(!DriverStore.endedOutside(ce, open, next), "a native crash of that process counts");
        ce.activityManager.records.set(0, new android.app.ApplicationExitInfo(4242, DriverGuard.EXIT_SELF, 0));
        check(DriverStore.endedOutside(ce, open, next), "a clean own exit of that process excuses it");
        next.okId = "e1";
        check(!DriverStore.endedOutside(ce, open, next), "an attempt with its OK marker needs no excuse");
        next.okId = ""; open.pendingPid = 0;
        check(!DriverStore.endedOutside(ce, open, next), "an attempt from a guard file without a pid counts as before");
        open.pendingPid = 1000;
        check(!DriverStore.endedOutside(ce, open, next), "the own process has no exit record yet: counted");

        if (fails > 0) { System.out.println(fails + " failure(s)"); System.exit(1); }
        System.out.println("DriverStoreTest: all passed");
    }


    static Context ctx(File base, File apk) {
        Context c = new Context();
        c.filesDir = base;
        c.appInfo.sourceDir = apk.getAbsolutePath();
        return c;
    }


    static File addTo(File base, byte[] zipBytes) throws IOException {
        File incoming = new File(base, DriverLibrary.INCOMING);
        Files.write(incoming.toPath(), zipBytes);
        DriverLibrary.Add add = DriverLibrary.add(base, incoming, NOOP);
        if (add.error != null) throw new IOException("add failed: " + add.error);
        return DriverLibrary.zipFor(base, add.sha);
    }

    static String[] record(File base) throws IOException {
        return new String(Files.readAllBytes(new File(base, "import.txt").toPath()), StandardCharsets.UTF_8).split("\n", -1);
    }



    static File fakeApk() throws IOException {
        File apk = File.createTempFile("fake-shell", ".apk");
        try (ZipOutputStream z = new ZipOutputStream(new FileOutputStream(apk))) {
            for (String n : new String[]{DriverStore.LOADER, DriverStore.HOOKS[0], DriverStore.HOOKS[1]}) {
                z.putNextEntry(new ZipEntry("lib/arm64-v8a/" + n));
                z.write("fake library bytes".getBytes(StandardCharsets.UTF_8));
                z.closeEntry();
            }
        }
        return apk;
    }




    static byte[] driverZip(String name, String... libs) throws IOException {
        ByteArrayOutputStream buf = new ByteArrayOutputStream();
        byte[] elf = DriverElfTest.elf(new DriverElfTest.P());
        try (ZipOutputStream z = new ZipOutputStream(buf)) {
            z.putNextEntry(new ZipEntry("meta.json"));
            z.write(("{\"name\":\"" + name + "\",\"libraryName\":\"" + libs[0] + "\"}").getBytes(StandardCharsets.UTF_8));
            z.closeEntry();
            for (String l : libs) { z.putNextEntry(new ZipEntry(l)); z.write(elf); z.closeEntry(); }
        }
        return buf.toByteArray();
    }
}
