package com.xiaoboluo.d2r;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;











final class DriverElf {
    static final class Result {
        String error;
        final List<String> needed = new ArrayList<>();
    }

    private static final int EM_AARCH64 = 183, ET_DYN = 3, PT_LOAD = 1, SHT_STRTAB = 3, SHT_DYNSYM = 11, SHT_DYNAMIC = 6, DT_NEEDED = 1;
    private static final long MAX_SIZE = 64L << 20;

    private DriverElf() {}

    static Result check(java.io.File file, long pageSize, boolean mainLibrary) {
        Result r = new Result();
        try (RandomAccessFile f = new RandomAccessFile(file, "r")) {
            long size = f.length();
            if (size < 64 || size > MAX_SIZE) { r.error = "elf-size"; return r; }
            byte[] all = new byte[(int) size];
            f.readFully(all);
            r.error = check(all, pageSize, r.needed, mainLibrary);
        } catch (IOException e) { r.error = "elf-read"; }
        return r;
    }


    static boolean inside(long off, long len, long size) { return off >= 0 && len >= 0 && off <= size && len <= size - off; }

    static String check(byte[] b, long pageSize, List<String> needed, boolean mainLibrary) {
        ByteBuffer m = ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN);
        long size = b.length;
        if (size < 64 || b[0] != 0x7f || b[1] != 'E' || b[2] != 'L' || b[3] != 'F') return "elf-header";
        if (b[4] != 2 || b[5] != 1 || b[6] != 1) return "elf-class";
        if (m.getShort(16) != ET_DYN) return "elf-type";
        if ((m.getShort(18) & 0xffff) != EM_AARCH64) return "elf-arch";
        if (m.getInt(20) != 1 || (m.getShort(52) & 0xffff) != 64) return "elf-header";
        long phoff = m.getLong(32), shoff = m.getLong(40);
        int phentsize = m.getShort(54) & 0xffff, phnum = m.getShort(56) & 0xffff, shentsize = m.getShort(58) & 0xffff, shnum = m.getShort(60) & 0xffff, shstrndx = m.getShort(62) & 0xffff;
        if (phentsize != 56 || phnum == 0 || phnum > 64 || !inside(phoff, (long) phnum * 56, size)) return "elf-bounds";
        if (shentsize != 64 || shnum == 0 || shnum > 4096 || !inside(shoff, (long) shnum * 64, size) || shstrndx >= shnum) return "elf-bounds";
        if (pageSize <= 0 || Long.bitCount(pageSize) != 1) return "elf-pagesize";
        int loads = 0;
        for (int i = 0; i < phnum; i++) {
            int p = (int) (phoff + 56L * i);
            if (m.getInt(p) != PT_LOAD) continue;
            loads++;
            long off = m.getLong(p + 8), vaddr = m.getLong(p + 16), filesz = m.getLong(p + 32), memsz = m.getLong(p + 40), align = m.getLong(p + 48);
            if (align <= 0 || Long.bitCount(align) != 1) return "elf-align";
            if (align < pageSize) return "elf-pagesize";
            if (Long.remainderUnsigned(off, align) != Long.remainderUnsigned(vaddr, align)) return "elf-align";
            if (Long.compareUnsigned(filesz, memsz) > 0 || !inside(off, filesz, size)) return "elf-bounds";
        }
        if (loads == 0) return "elf-bounds";
        boolean hmi = false;
        for (int i = 0; i < shnum; i++) {
            int sh = (int) (shoff + 64L * i);
            int type = m.getInt(sh + 4);
            if (type != SHT_DYNSYM && type != SHT_DYNAMIC) continue;
            long off = m.getLong(sh + 24), len = m.getLong(sh + 32), entsize = m.getLong(sh + 56);
            int link = m.getInt(sh + 40);
            if (!inside(off, len, size) || link <= 0 || link >= shnum) return "elf-bounds";
            int ls = (int) (shoff + 64L * link);
            if (m.getInt(ls + 4) != SHT_STRTAB) return "elf-bounds";
            long stroff = m.getLong(ls + 24), strlen = m.getLong(ls + 32);
            if (!inside(stroff, strlen, size)) return "elf-bounds";
            if (type == SHT_DYNSYM) {
                if (entsize != 24 || len % 24 != 0) return "elf-bounds";
                for (long s = off; s < off + len; s += 24) {
                    String name = string(b, stroff, strlen, m.getInt((int) s) & 0xffffffffL);
                    if (name == null) return "elf-bounds";
                    int info = b[(int) s + 4] & 0xff, shndx = m.getShort((int) s + 6) & 0xffff;
                    if (name.equals("HMI") && (info >> 4) == 1 && (info & 0xf) == 1 && shndx != 0) hmi = true;
                }
            } else {
                if (entsize != 16 || len % 16 != 0) return "elf-bounds";
                for (long d = off; d < off + len; d += 16) {
                    long tag = m.getLong((int) d);
                    if (tag == 0) break;
                    if (tag != DT_NEEDED) continue;
                    String name = string(b, stroff, strlen, m.getLong((int) d + 8));
                    if (name == null) return "elf-bounds";
                    if (needed.size() < 64) needed.add(name);
                }
            }
        }
        return !mainLibrary || hmi ? null : "elf-nohmi";
    }



    private static String string(byte[] b, long tab, long len, long at) {
        if (at < 0 || at >= len) return null;
        int start = (int) (tab + at), end = start, limit = (int) (tab + len);
        while (end < limit && b[end] != 0) end++;
        if (end >= limit || end - start > 4096) return null;
        return new String(b, start, end - start, java.nio.charset.StandardCharsets.ISO_8859_1);
    }
}
