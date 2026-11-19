package com.xiaoboluo.d2r;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;





public class DriverElfTest {
    static int fails;
    static void check(boolean ok, String what) { System.out.println((ok ? "ok   " : "FAIL ") + what); if (!ok) fails++; }


    static class P {
        int cls = 2, data = 1, type = 3, machine = 183, phentsize = 56, shentsize = 64;
        long align = 0x4000, vaddrDelta = 0, filesz = -1, memszDelta = 0, phoff = 64;
        String sym = "HMI"; int bind = 1, stype = 1, shndx = 5;
        boolean dynsymEntsizeBad, strtabOutside;
    }

    static byte[] elf(P p) {
        byte[] dynstr = ("\0libc.so\0libhardware.so\0" + p.sym + "\0").getBytes(StandardCharsets.ISO_8859_1);
        int nameOff = ("\0libc.so\0libhardware.so\0").length();
        String shs = "\0.dynstr\0.dynsym\0.dynamic\0.shstrtab\0.data\0";
        byte[] shstr = shs.getBytes(StandardCharsets.ISO_8859_1);
        int phoff = 64, ph = 56, off = phoff + ph;
        int oDynstr = off; off += dynstr.length; off = (off + 7) & ~7;
        int oDynsym = off; off += 48;
        int oDynamic = off; off += 48;
        int oShstr = off; off += shstr.length; off = (off + 7) & ~7;
        int oData = off; off += 16;
        int shoff = (off + 7) & ~7; int shnum = 6; int size = shoff + shnum * 64;
        ByteBuffer b = ByteBuffer.allocate(size).order(ByteOrder.LITTLE_ENDIAN);
        b.put(0, (byte) 0x7f).put(1, (byte) 'E').put(2, (byte) 'L').put(3, (byte) 'F').put(4, (byte) p.cls).put(5, (byte) p.data).put(6, (byte) 1);
        b.putShort(16, (short) p.type).putShort(18, (short) p.machine).putInt(20, 1).putLong(32, p.phoff).putLong(40, shoff);
        b.putShort(52, (short) 64).putShort(54, (short) p.phentsize).putShort(56, (short) 1).putShort(58, (short) p.shentsize).putShort(60, (short) shnum).putShort(62, (short) 4);

        long filesz = p.filesz >= 0 ? p.filesz : size;
        b.putInt(phoff, 1).putInt(phoff + 4, 5).putLong(phoff + 8, 0).putLong(phoff + 16, p.vaddrDelta).putLong(phoff + 24, p.vaddrDelta)
            .putLong(phoff + 32, filesz).putLong(phoff + 40, filesz + p.memszDelta).putLong(phoff + 48, p.align);
        for (int i = 0; i < dynstr.length; i++) b.put(oDynstr + i, dynstr[i]);
        for (int i = 0; i < shstr.length; i++) b.put(oShstr + i, shstr[i]);

        b.putInt(oDynsym + 24, nameOff).put(oDynsym + 24 + 4, (byte) ((p.bind << 4) | p.stype)).putShort(oDynsym + 24 + 6, (short) p.shndx).putLong(oDynsym + 24 + 8, oData).putLong(oDynsym + 24 + 16, 8);

        b.putLong(oDynamic, 1).putLong(oDynamic + 8, 1).putLong(oDynamic + 16, 1).putLong(oDynamic + 24, 9).putLong(oDynamic + 32, 0);
        int[][] sh = {
            {0, 0, 0, 0, 0, 0},
            {shs.indexOf(".dynstr"), 3, p.strtabOutside ? size - 4 : oDynstr, dynstr.length, 0, 0},
            {shs.indexOf(".dynsym"), 11, oDynsym, 48, 1, p.dynsymEntsizeBad ? 16 : 24},
            {shs.indexOf(".dynamic"), 6, oDynamic, 48, 1, 16},
            {shs.indexOf(".shstrtab"), 3, oShstr, shstr.length, 0, 0},
            {shs.indexOf(".data"), 1, oData, 16, 0, 0}};
        for (int i = 0; i < shnum; i++) {
            int s = shoff + i * 64;
            b.putInt(s, sh[i][0]).putInt(s + 4, sh[i][1]).putLong(s + 24, sh[i][2]).putLong(s + 32, sh[i][3]).putInt(s + 40, sh[i][4]).putLong(s + 56, sh[i][5]);
        }
        return b.array();
    }

    static String run(P p, long page) { return DriverElf.check(elf(p), page, new ArrayList<>(), true); }
    static String runHelper(P p, long page) { return DriverElf.check(elf(p), page, new ArrayList<>(), false); }

    public static void main(String[] a) throws Exception {
        List<String> needed = new ArrayList<>();
        check(DriverElf.check(elf(new P()), 4096, needed, true) == null, "well-formed AArch64 driver with HMI passes");
        check(needed.equals(List.of("libc.so", "libhardware.so")), "DT_NEEDED collected: " + needed);
        check(run(new P(), 16384) == null, "16 KB alignment passes on a 16 KB device");
        P p;
        p = new P(); p.align = 0x1000; check("elf-pagesize".equals(run(p, 16384)), "4 KB alignment rejected on a 16 KB device");
        check(run(p, 4096) == null, "4 KB alignment fine on a 4 KB device");
        p = new P(); p.cls = 1; check("elf-class".equals(run(p, 4096)), "32-bit ELF rejected");
        p = new P(); p.data = 2; check("elf-class".equals(run(p, 4096)), "big endian rejected");
        p = new P(); p.machine = 62; check("elf-arch".equals(run(p, 4096)), "x86-64 rejected");
        p = new P(); p.type = 2; check("elf-type".equals(run(p, 4096)), "ET_EXEC rejected");
        p = new P(); p.phentsize = 32; check("elf-bounds".equals(run(p, 4096)), "wrong program header size rejected");
        p = new P(); p.phoff = 1L << 40; check("elf-bounds".equals(run(p, 4096)), "program headers outside the file rejected");
        p = new P(); p.phoff = -64; check("elf-bounds".equals(run(p, 4096)), "negative / overflowing program header offset rejected");
        p = new P(); p.align = 0x3000; check("elf-align".equals(run(p, 4096)), "alignment not a power of two rejected");
        p = new P(); p.vaddrDelta = 0x10; check("elf-align".equals(run(p, 4096)), "offset and address not congruent rejected");
        p = new P(); p.memszDelta = -8; check("elf-bounds".equals(run(p, 4096)), "p_filesz above p_memsz rejected");
        p = new P(); p.filesz = 1L << 30; p.memszDelta = 0; check("elf-bounds".equals(run(p, 4096)), "PT_LOAD beyond the end of the file rejected");
        p = new P(); p.dynsymEntsizeBad = true; check("elf-bounds".equals(run(p, 4096)), "wrong .dynsym entry size rejected");
        p = new P(); p.strtabOutside = true; check("elf-bounds".equals(run(p, 4096)), ".dynstr reaching outside the file rejected");
        p = new P(); p.sym = "HMX"; check("elf-nohmi".equals(run(p, 4096)), "no HMI symbol rejected");
        p = new P(); p.sym = "HMX"; check(runHelper(p, 4096) == null, "helper library without HMI passes the structure check");
        p = new P(); p.sym = "HMX"; p.machine = 62; check("elf-arch".equals(runHelper(p, 4096)), "helper library is still checked for architecture");
        p = new P(); p.sym = "HMX"; p.align = 0x3000; check("elf-align".equals(runHelper(p, 4096)), "helper library is still checked for alignment");
        p = new P(); p.shndx = 0; check("elf-nohmi".equals(run(p, 4096)), "undefined HMI (import) rejected");
        p = new P(); p.bind = 0; check("elf-nohmi".equals(run(p, 4096)), "local HMI rejected");
        p = new P(); p.stype = 2; check("elf-nohmi".equals(run(p, 4096)), "HMI as a function rejected");
        byte[] good = elf(new P());
        for (int cut : new int[]{10, 63, 200, good.length - 1}) {
            byte[] t = java.util.Arrays.copyOf(good, cut);
            String r = DriverElf.check(t, 4096, new ArrayList<>(), true);
            check(r != null, "truncated to " + cut + " bytes rejected (" + r + ")");
        }
        check(DriverElf.check(new byte[100], 4096, new ArrayList<>(), true) != null, "zeros rejected");
        java.util.Random rnd = new java.util.Random(25);
        int crashes = 0;
        for (int i = 0; i < 2000; i++) {
            byte[] t = good.clone();
            for (int k = 0; k < 4; k++) t[rnd.nextInt(t.length)] = (byte) rnd.nextInt(256);
            try { DriverElf.check(t, 4096, new ArrayList<>(), new java.util.Random(i).nextBoolean()); } catch (RuntimeException e) { crashes++; }
        }
        check(crashes == 0, "2000 randomly damaged files: no exception (" + crashes + ")");
        if (a.length > 0) {
            DriverElf.Result r = DriverElf.check(Paths.get(a[0]).toFile(), 4096, true);
            check(r.error == null, "real driver " + a[0] + " passes (" + r.error + "), needs " + r.needed);
            check(Files.size(Paths.get(a[0])) > 0, "real driver read");
        }
        if (fails > 0) { System.out.println(fails + " failure(s)"); System.exit(1); }
        System.out.println("DriverElfTest: all passed");
    }
}
