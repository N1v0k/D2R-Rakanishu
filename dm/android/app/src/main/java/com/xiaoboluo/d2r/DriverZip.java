package com.xiaoboluo.d2r;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipInputStream;
















final class DriverZip {
    static final int MAX_ENTRIES = 32, MAX_META = 16 << 10;
    static final long MAX_UNPACKED = 192L << 20, MAX_ZIP = 96L << 20;

    static final class Meta {
        String error;
        String name = "", library = "", vendor = "", driverVersion = "", author = "";
        int minApi;
        final List<String> libraries = new ArrayList<>();
    }

    private DriverZip() {}

    static boolean safeName(String n) {
        if (n == null || n.isEmpty() || n.length() > 200 || n.startsWith("/") || n.indexOf('\\') >= 0) return false;
        for (int i = 0; i < n.length(); i++) if (n.charAt(i) < 0x20 || n.charAt(i) == 0x7f) return false;
        for (String part : n.split("/", -1)) if (part.equals("..") || part.equals(".")) return false;
        return true;
    }

    static boolean safeLibrary(String n) { return n != null && n.matches("[A-Za-z0-9._+@-]{1,64}\\.so") && !n.startsWith("."); }


    static Meta inspect(File zip, int sdk) {
        Meta meta = new Meta();
        if (zip.length() > MAX_ZIP) { meta.error = "zip-size"; return meta; }
        byte[] json = null;
        HashSet<String> names = new HashSet<>();
        HashSet<String> rootFiles = new HashSet<>();
        ArrayList<String> libs = new ArrayList<>();
        long total = 0; int count = 0;
        byte[] buf = new byte[64 << 10];
        try (ZipInputStream in = new ZipInputStream(new BufferedInputStream(new FileInputStream(zip)), StandardCharsets.UTF_8)) {
            ZipEntry e;
            while ((e = in.getNextEntry()) != null) {
                String n = e.getName();
                if (++count > MAX_ENTRIES) { meta.error = "zip-entries"; return meta; }
                if (!safeName(n)) { meta.error = "zip-path"; return meta; }
                if (!names.add(n)) { meta.error = "zip-duplicate"; return meta; }
                boolean root = !e.isDirectory() && n.indexOf('/') < 0;
                if (root) rootFiles.add(n);
                if (n.endsWith(".so")) {
                    if (!root || !safeLibrary(n)) { meta.error = "library-name"; return meta; }
                    libs.add(n);
                }
                ByteArrayOutputStream keep = root && n.equals("meta.json") ? new ByteArrayOutputStream() : null;
                int k;
                while ((k = in.read(buf)) > 0) {
                    total += k;
                    if (total > MAX_UNPACKED) { meta.error = "zip-unpacked"; return meta; }
                    if (keep != null) { if (keep.size() + k > MAX_META) { meta.error = "meta-size"; return meta; } keep.write(buf, 0, k); }
                }
                if (keep != null) json = keep.toByteArray();
            }
        } catch (ZipException e) { meta.error = "zip-broken"; return meta; }
        catch (IOException | IllegalArgumentException e) { meta.error = "zip-broken"; return meta; }
        if (count == 0) { meta.error = "zip-broken"; return meta; }
        if (json == null) { meta.error = "meta-missing"; return meta; }
        Map<String, Object> m = MiniJson.object(new String(json, StandardCharsets.UTF_8));
        if (m == null) { meta.error = "meta-json"; return meta; }
        meta.name = str(m.get("name")); meta.library = str(m.get("libraryName")); meta.vendor = str(m.get("vendor"));
        meta.driverVersion = str(m.get("driverVersion")); meta.author = str(m.get("author"));
        Object api = m.get("minApi");
        if (api instanceof Double) { double d = (Double) api; if (d != Math.rint(d) || d < 0 || d > 1000) { meta.error = "meta-minapi"; return meta; } meta.minApi = (int) d; }
        else if (api != null) { meta.error = "meta-minapi"; return meta; }
        if (meta.name.isEmpty() || meta.name.length() > 96 || !printable(meta.name)) { meta.error = "meta-name"; return meta; }
        if (!safeLibrary(meta.library)) { meta.error = "meta-library"; return meta; }
        if (!rootFiles.contains(meta.library)) { meta.error = "library-missing"; return meta; }
        if (meta.minApi > sdk) { meta.error = "meta-minapi"; return meta; }
        for (String f : new String[]{meta.vendor, meta.driverVersion, meta.author}) if (f.length() > 96 || !printable(f)) { meta.error = "meta-text"; return meta; }
        Collections.sort(libs);
        meta.libraries.addAll(libs);
        return meta;
    }



    static boolean extract(File zip, File out) throws IOException {
        long total = 0; int written = 0;
        byte[] buf = new byte[64 << 10];
        try (ZipInputStream in = new ZipInputStream(new BufferedInputStream(new FileInputStream(zip)), StandardCharsets.UTF_8)) {
            ZipEntry e;
            while ((e = in.getNextEntry()) != null) {
                String n = e.getName();
                boolean want = safeLibrary(n);
                try (OutputStream o = want ? new FileOutputStream(new File(out, n)) : null) {
                    int k;
                    while ((k = in.read(buf)) > 0) {
                        total += k;
                        if (total > MAX_UNPACKED) return false;
                        if (o != null) o.write(buf, 0, k);
                    }
                    if (o instanceof FileOutputStream) ((FileOutputStream) o).getFD().sync();
                }
                if (want) written++;
            }
        } catch (ZipException | IllegalArgumentException e) { return false; }
        return written > 0;
    }

    private static String str(Object o) { return o instanceof String ? (String) o : ""; }
    private static boolean printable(String s) { for (int i = 0; i < s.length(); i++) if (s.charAt(i) < 0x20 || s.charAt(i) == 0x7f) return false; return true; }




    static final class MiniJson {
        private final String s; private int i, depth;
        private MiniJson(String s) { this.s = s; }

        static Map<String, Object> object(String text) {
            if (text.startsWith("﻿")) text = text.substring(1);
            MiniJson p = new MiniJson(text);
            try {
                p.ws(); Map<String, Object> m = p.obj(true); p.ws();
                return p.i == p.s.length() ? m : null;
            } catch (RuntimeException e) { return null; }
        }
        private void ws() { while (i < s.length() && " \t\r\n".indexOf(s.charAt(i)) >= 0) i++; }
        private char peek() { if (i >= s.length()) throw new IllegalStateException(); return s.charAt(i); }
        private void expect(char c) { if (peek() != c) throw new IllegalStateException(); i++; }
        private Map<String, Object> obj(boolean keep) {
            expect('{'); ws(); Map<String, Object> m = keep ? new HashMap<>() : null;
            if (++depth > 16) throw new IllegalStateException();
            if (peek() == '}') { i++; depth--; return m; }
            for (;;) {
                ws(); String k = string(); ws(); expect(':'); ws(); Object v = value();
                if (m != null) { if (m.containsKey(k)) throw new IllegalStateException(); m.put(k, v); }
                ws(); char c = peek(); i++;
                if (c == '}') { depth--; return m; }
                if (c != ',') throw new IllegalStateException();
            }
        }
        private Object value() {
            char c = peek();
            if (c == '"') return string();
            if (c == '{') { obj(false); return null; }
            if (c == '[') {
                i++; if (++depth > 16) throw new IllegalStateException();
                ws(); if (peek() == ']') { i++; depth--; return null; }
                for (;;) { ws(); value(); ws(); char d = peek(); i++; if (d == ']') { depth--; return null; } if (d != ',') throw new IllegalStateException(); }
            }
            if (s.startsWith("true", i)) { i += 4; return Boolean.TRUE; }
            if (s.startsWith("false", i)) { i += 5; return Boolean.FALSE; }
            if (s.startsWith("null", i)) { i += 4; return null; }
            int st = i;
            while (i < s.length() && "+-0123456789.eE".indexOf(s.charAt(i)) >= 0) i++;
            if (st == i) throw new IllegalStateException();
            return Double.parseDouble(s.substring(st, i));
        }
        private String string() {
            expect('"'); StringBuilder b = new StringBuilder();
            for (;;) {
                char c = peek(); i++;
                if (c == '"') return b.toString();
                if (c < 0x20) throw new IllegalStateException();
                if (c != '\\') { b.append(c); continue; }
                char e = peek(); i++;
                switch (e) {
                    case '"', '\\', '/' -> b.append(e);
                    case 'b' -> b.append('\b'); case 'f' -> b.append('\f'); case 'n' -> b.append('\n'); case 'r' -> b.append('\r'); case 't' -> b.append('\t');
                    case 'u' -> { b.append((char) Integer.parseInt(s.substring(i, i + 4), 16)); i += 4; }
                    default -> throw new IllegalStateException();
                }
            }
        }
    }



    static boolean copyBounded(InputStream in, OutputStream out, long limit) throws IOException {
        byte[] buf = new byte[64 << 10]; long n = 0; int k;
        while ((k = in.read(buf)) > 0) { n += k; if (n > limit) return false; out.write(buf, 0, k); }
        return true;
    }
}
