package com.xiaoboluo.d2r;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.HashSet;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;




public class DriverZipTest {
    static int fails;
    static File dir;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }

    static final String META = "{\"schemaVersion\": 1, \"name\": \"Test Turnip\", \"description\": \"x\", \"author\": \"t\", \"packageVersion\": \"1\", "
        + "\"vendor\": \"Mesa\", \"driverVersion\": \"Vulkan 1.3.296\", \"minApi\": 27, \"libraryName\": \"vulkan.test.so\"}";
    static final byte[] LIB = "\u007fELF pretend library bytes".getBytes(StandardCharsets.ISO_8859_1);

    static File zip(String name, Object... entries) throws Exception {
        File f = new File(dir, name);
        try (ZipOutputStream z = new ZipOutputStream(new FileOutputStream(f))) {
            for (int i = 0; i < entries.length; i += 2) {
                z.putNextEntry(new ZipEntry((String) entries[i]));
                Object v = entries[i + 1];
                if (v instanceof String) z.write(((String) v).getBytes(StandardCharsets.UTF_8));
                else if (v instanceof byte[]) z.write((byte[]) v);
                else if (v instanceof Long) { byte[] zero = new byte[1 << 20]; for (long k = 0; k < (Long) v; k += zero.length) z.write(zero, 0, (int) Math.min(zero.length, (Long) v - k)); }
                z.closeEntry();
            }
        }
        return f;
    }

    static String err(File f) { return DriverZip.inspect(f, 34).error; }

    public static void main(String[] a) throws Exception {
        dir = Files.createTempDirectory("dz").toFile();
        File ok = zip("ok.zip", "meta.json", META, "vulkan.test.so", LIB);
        DriverZip.Meta m = DriverZip.inspect(ok, 34);
        check(m.error == null && m.name.equals("Test Turnip") && m.library.equals("vulkan.test.so") && m.minApi == 27 && m.vendor.equals("Mesa")
            && m.libraries.equals(Arrays.asList("vulkan.test.so")), "valid package: " + m.error);
        File out = new File(dir, "out");
        out.mkdir();
        check(DriverZip.extract(ok, out) && Arrays.equals(Files.readAllBytes(new File(out, "vulkan.test.so").toPath()), LIB)
            && Arrays.equals(out.list(), new String[]{"vulkan.test.so"}), "unpacks exactly the library");



        String metaMulti = META.replace("vulkan.test.so", "vulkan.adr891.so");
        File multi = zip("multi.zip", "meta.json", metaMulti, "vulkan.adr891.so", LIB, "vkbgsl.so", LIB, "vkbadreno_utils.so", LIB,
            "vkb891.qti.hardware.hexlp-V2-ndk.so", LIB, "LICENSE.txt", "note", "licenses/mit.txt", "note");
        DriverZip.Meta mm = DriverZip.inspect(multi, 34);
        check(mm.error == null && mm.library.equals("vulkan.adr891.so") && mm.libraries.equals(Arrays.asList(
            "vkb891.qti.hardware.hexlp-V2-ndk.so", "vkbadreno_utils.so", "vkbgsl.so", "vulkan.adr891.so")), "multi-library package accepted, helpers listed: " + mm.error);
        File mdir = new File(dir, "mout");
        mdir.mkdir();
        check(DriverZip.extract(multi, mdir) && new HashSet<>(Arrays.asList(mdir.list())).equals(new HashSet<>(mm.libraries))
            && Arrays.equals(Files.readAllBytes(new File(mdir, "vkbgsl.so").toPath()), LIB), "extraction writes exactly the .so files");
        check("library-name".equals(err(zip("badhelper.zip", "meta.json", META, "vulkan.test.so", LIB, "bad lib.so", LIB))), "helper with an unsafe name rejected");
        check("library-name".equals(err(zip("nestedhelper.zip", "meta.json", META, "vulkan.test.so", LIB, "lib/vkbgsl.so", LIB))), "helper in a subdirectory rejected");
        File hdup = zip("hdup.zip", "meta.json", META, "vulkan.test.so", LIB, "vkbgsl.so", LIB, "vkbgs1.so", LIB);
        String hs = new String(Files.readAllBytes(hdup.toPath()), StandardCharsets.ISO_8859_1).replace("vkbgs1.so", "vkbgsl.so");
        Files.write(hdup.toPath(), hs.getBytes(StandardCharsets.ISO_8859_1));
        check("zip-duplicate".equals(err(hdup)), "duplicate helper name rejected");
        check("zip-unpacked".equals(err(zip("mbomb.zip", "meta.json", metaMulti, "vulkan.adr891.so", (Long) (150L << 20), "vkbgsl.so", (Long) (50L << 20)))),
            "helpers pushed over 192 MB rejected");
        check(err(zip("onlyhelper.zip", "meta.json", META, "vulkan.test.so", LIB, "vkbgsl.so", LIB)) == null, "a helper next to the library is fine");

        check(err(zip("libfirst.zip", "vulkan.test.so", LIB, "meta.json", META)) == null, "meta.json after the library is fine");
        check(err(zip("dir.zip", "sub/", "", "meta.json", META, "vulkan.test.so", LIB)) == null, "directory entries are fine");

        check("zip-path".equals(err(zip("dotdot.zip", "meta.json", META, "../evil.so", LIB))), "../ rejected");
        check("zip-path".equals(err(zip("abs.zip", "meta.json", META, "/data/evil.so", LIB))), "absolute path rejected");
        check("zip-path".equals(err(zip("bs.zip", "meta.json", META, "a\\b.so", LIB))), "backslash rejected");
        check("zip-path".equals(err(zip("mid.zip", "meta.json", META, "a/../../b.so", LIB))), ".. inside a path rejected");
        Object[] many = new Object[2 * 33]; many[0] = "meta.json"; many[1] = META; many[2] = "vulkan.test.so"; many[3] = LIB;
        for (int i = 2; i < 33; i++) { many[2 * i] = "f" + i; many[2 * i + 1] = "x"; }
        check("zip-entries".equals(err(zip("many.zip", many))), "33 entries rejected");
        check("zip-unpacked".equals(err(zip("bomb.zip", "meta.json", META, "vulkan.test.so", (Long) (193L << 20)))), "193 MB unpacked (zip bomb) rejected");

        File dup = zip("dup.zip", "meta.json", META, "vulkan.test.so", LIB, "vulkan.tesu.so", LIB);
        byte[] raw = Files.readAllBytes(dup.toPath()); String s = new String(raw, StandardCharsets.ISO_8859_1).replace("vulkan.tesu.so", "vulkan.test.so");
        Files.write(dup.toPath(), s.getBytes(StandardCharsets.ISO_8859_1));
        check("zip-duplicate".equals(err(dup)), "duplicate names rejected");
        check("meta-missing".equals(err(zip("nometa.zip", "vulkan.test.so", LIB))), "missing meta.json rejected");
        check("meta-missing".equals(err(zip("submeta.zip", "x/meta.json", META, "vulkan.test.so", LIB))), "meta.json outside the root does not count");
        check("meta-json".equals(err(zip("badjson.zip", "meta.json", "{\"name\": \"x\",", "vulkan.test.so", LIB))), "broken JSON rejected");
        check("meta-json".equals(err(zip("dupkey.zip", "meta.json", "{\"name\":\"a\",\"name\":\"b\",\"libraryName\":\"vulkan.test.so\"}", "vulkan.test.so", LIB))), "duplicate JSON key rejected");
        check("meta-json".equals(err(zip("trail.zip", "meta.json", META + " x", "vulkan.test.so", LIB))), "trailing garbage after the JSON rejected");
        check("meta-size".equals(err(zip("bigmeta.zip", "meta.json", "{\"name\":\"" + "x".repeat(20000) + "\"}", "vulkan.test.so", LIB))), "meta.json over 16 KB rejected");
        check("meta-library".equals(err(zip("slash.zip", "meta.json", META.replace("vulkan.test.so", "lib/vulkan.test.so"), "lib/vulkan.test.zip", LIB))), "libraryName with / rejected");
        check("meta-library".equals(err(zip("noso.zip", "meta.json", META.replace("vulkan.test.so", "vulkan.test.dll"), "vulkan.test.dll", LIB))), "libraryName without .so rejected");
        check("library-missing".equals(err(zip("nolib.zip", "meta.json", META))), "library named but missing rejected");
        check("meta-minapi".equals(DriverZip.inspect(ok, 26).error), "minApi above the device rejected");
        check("meta-minapi".equals(err(zip("apistr.zip", "meta.json", META.replace("\"minApi\": 27", "\"minApi\": \"27\""), "vulkan.test.so", LIB))), "minApi as text rejected");
        check(err(zip("noapi.zip", "meta.json", META.replace("\"minApi\": 27, ", ""), "vulkan.test.so", LIB)) == null, "missing minApi = 0");
        check("meta-name".equals(err(zip("noname.zip", "meta.json", META.replace("\"name\": \"Test Turnip\", ", ""), "vulkan.test.so", LIB))), "missing name rejected");
        check(err(zip("nested.zip", "meta.json", "{\"name\":\"n\",\"x\":{\"a\":[1,2,{\"b\":null}]},\"libraryName\":\"vulkan.test.so\",\"t\":true}", "vulkan.test.so", LIB)) == null, "nested values are read past");
        check(err(zip("esc.zip", "meta.json", "﻿{\"name\":\"T\\u00fcrnip \\\"q\\\"\",\"libraryName\":\"vulkan.test.so\"}", "vulkan.test.so", LIB)) == null, "BOM and escapes accepted");
        check("meta-json".equals(err(zip("deep.zip", "meta.json", "{\"name\":\"n\",\"libraryName\":\"vulkan.test.so\",\"x\":" + "[".repeat(5000) + "]".repeat(5000) + "}", "vulkan.test.so", LIB))), "deeply nested JSON rejected without exhausting the stack");
        check(err(zip("depth.zip", "meta.json", "{\"name\":\"n\",\"libraryName\":\"vulkan.test.so\",\"x\":" + "[".repeat(10) + "]".repeat(10) + "}", "vulkan.test.so", LIB)) == null, "moderate nesting accepted");
        File junk = new File(dir, "junk.zip"); Files.write(junk.toPath(), "this is not a zip at all".getBytes(StandardCharsets.UTF_8));
        check("zip-broken".equals(err(junk)), "not a ZIP rejected");
        File cut = new File(dir, "cut.zip"); byte[] okb = Files.readAllBytes(ok.toPath()); Files.write(cut.toPath(), Arrays.copyOf(okb, okb.length / 2));
        String ce = err(cut); check(ce != null, "truncated ZIP rejected (" + ce + ")");
        File huge = new File(dir, "huge.zip"); try (RandomAccessFile r = new RandomAccessFile(huge, "rw")) { r.setLength(DriverZip.MAX_ZIP + 1); }
        check("zip-size".equals(err(huge)), "ZIP file over 96 MB rejected before reading");
        File bdir = new File(dir, "b"); bdir.mkdir();
        check(!DriverZip.extract(zip("bomb2.zip", "vulkan.test.so", (Long) (193L << 20)), bdir), "unpacking also stops at 192 MB");
        check(DriverZip.safeLibrary("vulkan.ad07xx.so") && DriverZip.safeLibrary("vendor.qti.hardware.display.mapper@4.0.so") && !DriverZip.safeLibrary(".so") && !DriverZip.safeLibrary("a b.so") && !DriverZip.safeLibrary("../x.so"), "library name rule");
        java.util.Random rnd = new java.util.Random(25); int crashes = 0;
        for (int i = 0; i < 300; i++) {
            byte[] t = okb.clone(); for (int k = 0; k < 3; k++) t[rnd.nextInt(t.length)] = (byte) rnd.nextInt(256);
            File f = new File(dir, "fz.zip"); Files.write(f.toPath(), t);
            try { DriverZip.inspect(f, 34); } catch (RuntimeException e) { crashes++; System.out.println("  " + e); }
        }
        check(crashes == 0, "300 randomly damaged ZIPs: no exception (" + crashes + ")");
        if (a.length > 0) {
            DriverZip.Meta r = DriverZip.inspect(new File(a[0]), 34);
            check(r.error == null, "real package " + a[0] + ": " + r.name + " / " + r.library + " (" + r.error + ")");
        }
        for (File f : dir.listFiles()) f.delete();
        dir.delete();
        if (fails > 0) { System.out.println(fails + " failure(s)"); System.exit(1); }
        System.out.println("DriverZipTest: all passed");
    }
}
