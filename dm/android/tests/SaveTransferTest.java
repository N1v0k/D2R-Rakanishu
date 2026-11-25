package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;




public final class SaveTransferTest {
    private static int checks;
    private static void check(boolean value, String label) { ++checks; if (!value) throw new AssertionError(label); }
    private static void write(File f, byte[] data) throws IOException { f.getParentFile().mkdirs(); Files.write(f.toPath(), data); }
    private static void write(File f, String text) throws IOException { write(f, text.getBytes(StandardCharsets.UTF_8)); }
    private static String read(File f) throws IOException { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8); }
    private static byte[] bytes(File f) throws IOException { return Files.readAllBytes(f.toPath()); }


    static byte[] d2s(String body) {
        byte[] b = body.getBytes(StandardCharsets.UTF_8);
        ByteBuffer d = ByteBuffer.allocate(16 + b.length).order(ByteOrder.LITTLE_ENDIAN);
        d.putInt(0xaa55aa55).putInt(0x69).putInt(16 + b.length).putInt(0).put(b);
        return d.array();
    }

    static byte[] d2i(int gold, String body) {
        byte[] b = body.getBytes(StandardCharsets.UTF_8);
        ByteBuffer d = ByteBuffer.allocate(68 * 2 + b.length).order(ByteOrder.LITTLE_ENDIAN);
        d.putInt(0, 0xaa55aa55).putInt(12, gold).putInt(16, 68 + b.length); d.put(64, (byte) 'J').put(65, (byte) 'M');
        d.position(68); d.put(b);
        d.putInt(68 + b.length, 0xaa55aa55).putInt(68 + b.length + 16, 68);
        return d.array();
    }
    static byte[] vault(String... items) {
        int n = 8; for (String s : items) n += 6 + s.length();
        ByteBuffer d = ByteBuffer.allocate(n).order(ByteOrder.LITTLE_ENDIAN);
        d.put(new byte[] { 'S', 'W', 'V', 'T', 1, 0 }).putShort((short) items.length);
        for (int i = 0; i < items.length; i++) { d.put((byte) 1).put((byte) 0).putShort((short) i).putShort((short) items[i].length()).put(items[i].getBytes(StandardCharsets.US_ASCII)); }
        return d.array();
    }
    static final String SC = "ModernSharedStashSoftCoreV2", HC = "ModernSharedStashHardCoreV2";

    private static void character(File dir, String name, String body) throws IOException {
        write(new File(dir, name + "/" + name + ".d2s"), d2s(body));
        write(new File(dir, name + "/" + name + ".key"), "key " + name);
    }
    private static void stash(File dir, String name, int gold, String body) throws IOException { write(new File(dir, name + "/" + name + ".d2i"), d2i(gold, body)); }
    private static void stash(File dir, String body) throws IOException { stash(dir, SC, 100, body); }

    static final class Env {
        final File d, game, save, files, state;
        final FakeBuilds builds = new FakeBuilds();
        Env(File base, String name) {
            d = new File(base, name); game = new File(d, "D2R Mobile"); files = new File(d, "files"); save = new File(files, "save"); state = new File(files, "save-transfer");
            game.mkdirs(); files.mkdirs();
        }
        List<SaveTransfer.Result> run(String stamp) { return SaveTransfer.run(game, save, stamp, builds); }
        File in() { return new File(game, "save-import"); }
        File[] inbox() { File[] l = new File(save, "_cloudinbox").listFiles(); if (l == null) return new File[0]; Arrays.sort(l); return l; }
        String ledger() throws IOException { File f = new File(state, "ledger"); return f.isFile() ? read(f) : ""; }
    }
    static final class FakeBuilds implements SaveTransfer.Builds {
        final Properties p = new Properties();
        boolean fail;
        @Override public Properties all() { Properties c = new Properties(); c.putAll(p); return c; }
        @Override public int addMissing(Properties q) throws IOException {
            if (fail) throw new IOException("build guides not stored"); int n = 0; for (String k : q.stringPropertyNames()) if (!p.containsKey(k)) { p.setProperty(k, q.getProperty(k)); n++; } return n; }
    }

    private static void copyDir(File from, File to) throws IOException {
        if (!from.isDirectory()) { to.getParentFile().mkdirs(); Files.copy(from.toPath(), to.toPath()); return; }
        to.mkdirs(); for (File c : from.listFiles()) copyDir(c, new File(to, c.getName()));
    }
    private static void deleteDir(File f) { File[] l = f.listFiles(); if (l != null) for (File c : l) deleteDir(c); f.delete(); }


    static List<String> packed(File entry) throws IOException { List<String> l = SaveTransfer.packPaths(bytes(new File(entry, "mine.pack"))); java.util.Collections.sort(l, (a, b) -> a.replace('/', '\0').compareTo(b.replace('/', '\0'))); return l; }


    static String tree(File dir) throws IOException {
        List<String> out = new ArrayList<>(); walk(dir, "", out); java.util.Collections.sort(out); return String.join("\n", out);
    }
    private static void walk(File f, String rel, List<String> out) throws IOException {
        File[] l = f.listFiles();
        if (l == null) { if (f.isFile()) out.add(rel + "=" + (f.getName().equals("mine.pack") ? "pack" : String.valueOf(Arrays.hashCode(bytes(f))))); return; }
        for (File c : l) walk(c, rel + "/" + c.getName(), out);
    }

    static String inboxShape(Env e) throws IOException {
        List<String> out = new ArrayList<>();
        for (File x : e.inbox()) out.add(String.join(",", packed(x)) + "|" + read(new File(x, "import.sha256")).trim());
        java.util.Collections.sort(out); return String.join("\n", out);
    }

    public static void main(String[] args) throws IOException {
        File base = Files.createTempDirectory("dm-save-transfer").toFile();
        upstreamRules(base);
        components(base);
        idempotence(base);
        journal(base);
        inboxTakeOver(base);
        carriedTwice(base);
        carriedPartly(base);
        carriedThenAlone(base);
        skippedLedger(base);
        limits(base);
        ownData(base);
        buildsFail(base);
        oddImports(base);
        looks(base);
        looksFromHost(base, new File(args[0]));
        looksFromHost(base, new File(args[1]));
        looksSkipped(base, new File(args[1]));
        feats(base);
        book(base);
        settingsAndElysium(base);
        System.out.println("SaveTransferTest OK, checks=" + checks);
    }



    static byte[] featsFile(String who, int row, long kills) {
        ByteBuffer d = ByteBuffer.allocate(20 + 64 + 2 * 16).order(ByteOrder.LITTLE_ENDIAN);
        d.putInt(0x5446444d).putInt(1).putInt(1).putInt(2).putInt(0);
        byte[] name = who.getBytes(StandardCharsets.UTF_8); d.put(name).position(20 + 64);
        d.putShort((short) row).put((byte) 1).put((byte) 85).putInt(1763500000).putLong(0);
        d.putShort((short) 0).put((byte) 2).put((byte) 0).putInt(0).putLong(kills);
        java.util.zip.CRC32 crc = new java.util.zip.CRC32(); crc.update(d.array(), 20, d.capacity() - 20);
        d.putInt(16, (int) crc.getValue());
        return d.array();
    }



    static void feats(File base) throws IOException {
        byte[] ann = featsFile("Ann", 13, 3000), theirBob = featsFile("Bob", 4, 70000), myBob = featsFile("Bob", 1, 600);
        Env src = new Env(base, "feats-src");
        character(src.save, "Ann", "a"); write(new File(src.save, "Ann/Ann.sw_feats"), ann);
        character(src.save, "Bob", "their bob"); write(new File(src.save, "Bob/Bob.sw_feats"), theirBob);
        write(new File(src.game, "export"), "");
        List<SaveTransfer.Result> r = src.run("20261119-120000");
        File out = new File(src.game, "save-export/20261119-120000");
        check(r.size() == 1 && r.get(0).kind == SaveTransfer.EXPORTED && Arrays.equals(bytes(new File(out, "Ann/Ann.sw_feats")), ann)
              && Arrays.equals(bytes(new File(out, "Bob/Bob.sw_feats")), theirBob), "export carries the achievements of each character byte for byte");
        check(Arrays.equals(bytes(new File(src.save, "Ann/Ann.sw_feats")), ann), "export leaves the achievements as they are");
        Env dst = new Env(base, "feats-dst");
        character(dst.save, "Bob", "my bob"); write(new File(dst.save, "Bob/Bob.sw_feats"), myBob);
        copyDir(out, dst.in());
        r = dst.run("20261119-130000");
        check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && "Ann".equals(r.get(0).path) && "Bob".equals(r.get(0).skipped), "Ann imported, Bob skipped");
        check(Arrays.equals(bytes(new File(dst.save, "Ann/Ann.sw_feats")), ann), "the achievements arrive with their character byte for byte");
        check(Arrays.equals(bytes(new File(dst.save, "Bob/Bob.sw_feats")), myBob) && Arrays.equals(bytes(new File(dst.save, "Bob/Bob.d2s")), d2s("my bob")),
              "a skipped character keeps its own achievements");
    }



    static void book(File base) throws IOException {
        byte[] ann = "MDBK ann".getBytes(StandardCharsets.US_ASCII), theirBob = "MDBK their bob".getBytes(StandardCharsets.US_ASCII), myBob = "MDBK my bob".getBytes(StandardCharsets.US_ASCII);
        Env src = new Env(base, "book-src");
        character(src.save, "Ann", "a"); write(new File(src.save, "Ann/Ann.sw_book"), ann);
        character(src.save, "Bob", "their bob"); write(new File(src.save, "Bob/Bob.sw_book"), theirBob);
        write(new File(src.game, "export"), "");
        List<SaveTransfer.Result> r = src.run("20261124-120000");
        File out = new File(src.game, "save-export/20261124-120000");
        check(r.size() == 1 && r.get(0).kind == SaveTransfer.EXPORTED && Arrays.equals(bytes(new File(out, "Ann/Ann.sw_book")), ann)
              && Arrays.equals(bytes(new File(out, "Bob/Bob.sw_book")), theirBob), "export carries the monster book of each character byte for byte");
        Env dst = new Env(base, "book-dst");
        character(dst.save, "Bob", "my bob"); write(new File(dst.save, "Bob/Bob.sw_book"), myBob);
        copyDir(out, dst.in());
        r = dst.run("20261124-130000");
        check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && "Ann".equals(r.get(0).path) && "Bob".equals(r.get(0).skipped), "Ann imported, Bob skipped");
        check(Arrays.equals(bytes(new File(dst.save, "Ann/Ann.sw_book")), ann), "the monster book arrives with its character byte for byte");
        check(Arrays.equals(bytes(new File(dst.save, "Bob/Bob.sw_book")), myBob), "a skipped character keeps its own monster book");
    }


    static byte[] looksFile(int version, String body) {
        byte[] b = body.getBytes(StandardCharsets.US_ASCII), d = new byte[8 + b.length + 8];
        d[0] = 'D'; d[1] = 'M'; d[2] = 'L'; d[3] = 'K'; d[4] = (byte) version;
        System.arraycopy(b, 0, d, 8, b.length);
        long h = SaveTransfer.LOOKS_SEED;
        for (int i = 0; i < d.length - 8; i++) { h ^= d[i] & 0xff; h *= 0x100000001b3L; }
        for (int i = 0; i < 8; i++) d[d.length - 8 + i] = (byte) (h >>> (8 * i));
        return d;
    }



    static void looksFromHost(File base, File host) throws IOException {
        byte[] d = bytes(host);
        check(d.length > 16 && SaveTransfer.validLooks(d), "the host's looks file passes the import check");
        Env e = new Env(base, "looksHost" + d[4]); File in = e.in(); character(in, "Bob", "new"); write(new File(in, SaveTransfer.LOOKS), d);
        List<SaveTransfer.Result> r = e.run("20261101-120002");
        check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED, "a backup with the host's looks file is imported");
        File[] staged = e.save.listFiles((x, n) -> n.startsWith(SaveTransfer.LOOKS_IN) && n.endsWith(".swlooks"));
        check(staged != null && staged.length == 1 && Arrays.equals(bytes(staged[0]), d), "the host's looks file is staged byte for byte");
    }




    static void looksSkipped(File base, File host) throws IOException {
        byte[] d = bytes(host);
        Env e = new Env(base, "looksSkipped");
        long[] bits = { 0x2b5L, 10, 0xdeadL, 32, 30, 7, 4, 4, 0x1555L, 13 };
        byte[] body = new byte[16]; int at = 0;
        for (int i = 0; i < bits.length; i += 2) for (int k = 0; k < bits[i + 1]; k++, at++) if ((bits[i] >>> k & 1) != 0) body[at >> 3] |= (byte) (1 << (at & 7));
        byte[] old = d2s("old helm gem"), local = Arrays.copyOf(old, old.length + body.length);
        System.arraycopy(body, 0, local, old.length, body.length);
        write(new File(e.save, "Hero/Hero.d2s"), local);
        File in = e.in(); character(in, "Hero", "newer, helm coloured"); character(in, "Bob", "new"); write(new File(in, SaveTransfer.LOOKS), d);
        List<SaveTransfer.Result> r = e.run("20261101-120003");
        check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && "Hero".equals(r.get(0).skipped), "the phone's Hero is skipped");
        File[] staged = e.save.listFiles((x, n) -> n.startsWith(SaveTransfer.LOOKS_IN) && n.endsWith(".swlooks"));
        byte[] st = staged != null && staged.length == 1 ? bytes(staged[0]) : new byte[0];
        check(SaveTransfer.validLooks(st) && st[4] == 2 && st.length == d.length - 19, "the staged looks file is whole and one mapping shorter");
        ByteBuffer b = ByteBuffer.wrap(st).order(ByteOrder.LITTLE_ENDIAN); int map = -1;
        for (int i = 8; i + 4 <= st.length; i++) if (st[i] == 'M' && st[i + 1] == 'A' && st[i + 2] == 'P' && st[i + 3] == ' ') { map = i; break; }
        check(map > 0 && b.getInt(map + 8) == 1 && b.getInt(map + 12) == 0xbeef, "the phone's helm takes nothing from the backup, the buckler that is not here keeps its mapping");
    }



    static void looks(File base) throws IOException {
        { Env e = new Env(base, "looks"); character(e.save, "yyc", "phone"); write(new File(e.save, SaveTransfer.LOOKS), looksFile(1, "phone"));
          File in = e.in(); character(in, "yyc", "backup"); write(new File(in, SaveTransfer.LOOKS), looksFile(1, "backup"));
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED, "import with a looks file");
          check(Arrays.equals(bytes(new File(e.save, SaveTransfer.LOOKS)), looksFile(1, "phone")), "the phone's looks file stays");
          File[] staged = e.save.listFiles((d, n) -> n.startsWith(SaveTransfer.LOOKS_IN) && n.endsWith(".swlooks"));
          check(staged != null && staged.length == 1 && Arrays.equals(bytes(staged[0]), looksFile(1, "backup")), "the backup's looks file is staged for the host's merge");
          check(Arrays.equals(bytes(new File(e.save, "yyc/yyc.d2s")), d2s("phone")), "the existing character stays"); }
        { Env e = new Env(base, "looksbad"); character(e.save, "yyc", "keep");
          File in = e.in(); character(in, "Bob", "new"); byte[] bad = looksFile(1, "x"); bad[9] ^= 1; write(new File(in, SaveTransfer.LOOKS), bad);
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_FILE, "a damaged looks file rejects the import");
          check(!new File(e.save, "Bob").exists(), "nothing imported"); }
        { Env e = new Env(base, "looksnew"); File in = e.in(); character(in, "Bob", "new"); write(new File(in, SaveTransfer.LOOKS), looksFile(3, "later"));
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED, "a newer looks file passes to the host"); }
    }


    static void upstreamRules(File base) throws IOException {
        { Env e = new Env(base, "none"); character(e.save, "yyc", "v1");
          check(e.run("20261101-120000").isEmpty(), "no markers, no results");
          check(!new File(e.game, "save-export").exists() && Arrays.equals(bytes(new File(e.save, "yyc/yyc.d2s")), d2s("v1")), "nothing written"); }

        { Env e = new Env(base, "export");
          character(e.save, "yyc", "v1"); stash(e.save, "stash"); write(new File(e.save, "Settings/Settings.json"), "{}");
          write(new File(e.save, SC + ".swvault"), vault("a")); write(new File(e.save, "yyc/yyc.swpage"), "page"); write(new File(e.save, "_cloudinbox/1-import/mine.pack"), "p");
          write(new File(e.save, "yyc/yyc.sw_fights"), "fights");
          write(new File(e.save, "yyc/yyc.sw_streaks"), "streaks");
          write(new File(e.files, "sw_custom.txt"), "fps=60\n"); write(new File(e.files, "stash_backup/" + SC + ".20261031_105730.d2i"), "bk");
          e.builds.p.setProperty("yyc#1", "guide,3,auto");
          write(new File(e.game, "export"), "");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.EXPORTED && r.get(0).path.equals("save-export/20261101-120000"), "export result");
          File out = new File(e.game, "save-export/20261101-120000");
          check(Arrays.equals(bytes(new File(out, "yyc/yyc.d2s")), d2s("v1")) && Arrays.equals(bytes(new File(out, SC + "/" + SC + ".d2i")), d2i(100, "stash"))
                && read(new File(out, "Settings/Settings.json")).equals("{}"), "export copies the saves");
          check(new File(out, SC + ".swvault").isFile() && read(new File(out, "yyc/yyc.swpage")).equals("page") && new File(out, "_cloudinbox/1-import/mine.pack").isFile(), "export carries vault, page files and the inbox");
          check(read(new File(out, "yyc/yyc.sw_fights")).equals("fights"), "export carries the fight log of the character");
          check(read(new File(out, "yyc/yyc.sw_streaks")).equals("streaks"), "export carries the kill streaks of the character");
          check(read(new File(out, "_app/sw_custom.txt")).equals("fps=60\n") && read(new File(out, "_app/stash_backup/" + SC + ".20261031_105730.d2i")).equals("bk"), "export carries our own data");
          Properties p = new Properties(); p.load(new java.io.StringReader(read(new File(out, "_app/builds.properties"))));
          check("guide,3,auto".equals(p.getProperty("yyc#1")), "export carries the followed build guides");
          check(!new File(e.game, "export").exists() && !new File(out.getPath() + ".part").exists() && !new File(e.state, "export").exists(), "marker, part and pending record gone");
          check(Arrays.equals(bytes(new File(e.save, "yyc/yyc.d2s")), d2s("v1")), "saves untouched by export");
          new File(e.game, "export").mkdirs();
          r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).path.equals("save-export/20261101-120000-2") && new File(e.game, "save-export/20261101-120000-2/yyc/yyc.d2s").isFile(), "a folder marker works, the name gets -2"); }
        { Env e = new Env(base, "empty"); new File(e.save, "yyc").mkdirs();
          write(new File(e.game, "export"), "");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.EXPORT_EMPTY && !new File(e.game, "export").exists() && !new File(e.game, "save-export").exists(), "empty export"); }

        { Env e = new Env(base, "import");
          character(e.save, "yyc", "old"); stash(e.save, "phone stash"); write(new File(e.save, "Settings/Settings.json"), "phone");
          File in = e.in(); character(in, "Bob", "new"); character(in, "YYC", "other phone"); stash(in, "other stash");
          write(new File(in, "Settings/Settings.json"), "other"); write(new File(in, "readme.txt"), "x"); write(new File(in, "junk/notes.txt"), "x");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && "Bob".equals(r.get(0).path), "only Bob imported");
          String sk = r.get(0).skipped;
          check(sk != null && sk.contains("YYC") && !sk.contains(SC) && sk.contains("Settings") && !sk.contains("junk"), "skipped list (names without case; the stash is merged, not skipped)");
          check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && e.inbox().length == 1 && e.inbox()[0].getName().endsWith("-import"), "stash merge queued as one -import entry");
          byte[] pk = bytes(new File(e.inbox()[0], "mine.pack"));
          ByteBuffer bb = ByteBuffer.wrap(pk).order(ByteOrder.LITTLE_ENDIAN);
          check(new String(pk, 0, 5, StandardCharsets.US_ASCII).equals("SWPK1") && pk[5] == '\n' && pk[6] == 0 && pk[7] == 0, "pack magic");
          bb.position(8); long stamp = bb.getLong(); int flags = bb.getInt(), nf = bb.getInt();
          check(stamp > 0 && flags == 0 && nf == 1, "pack header");
          int pl = bb.getShort() & 0xffff; byte[] pth = new byte[pl]; bb.get(pth); long mt = bb.getLong(); int sz = bb.getInt(); byte[] body = new byte[sz]; bb.get(body);
          check(new String(pth, StandardCharsets.UTF_8).equals(SC + "/" + SC + ".d2i") && mt > 0 && Arrays.equals(body, d2i(100, "other stash")) && !bb.hasRemaining(), "pack file entry");
          check(Arrays.equals(bytes(new File(e.save, "Bob/Bob.d2s")), d2s("new")) && read(new File(e.save, "Bob/Bob.key")).equals("key Bob"), "Bob added");
          check(Arrays.equals(bytes(new File(e.save, "yyc/yyc.d2s")), d2s("old")) && Arrays.equals(bytes(new File(e.save, SC + "/" + SC + ".d2i")), d2i(100, "phone stash"))
                && read(new File(e.save, "Settings/Settings.json")).equals("phone") && !new File(e.save, "YYC").exists(), "nothing on the phone replaced");
          check(!new File(e.save, "readme.txt").exists() && !new File(e.save, "junk").exists() && !new File(e.state, "stage").exists() && !new File(e.state, "journal").exists(), "no extras, stage and journal gone");
          check(!in.exists() && new File(e.game, "save-import.done-20261101-120000/Bob/Bob.d2s").isFile(), "save-import renamed done");
          stash(in, "other stash"); character(in, "Cid", "c");
          List<SaveTransfer.Result> r2 = e.run("20261101-130000");
          check(r2.size() == 1 && r2.get(0).merge == SaveTransfer.MERGE_ALREADY && "Cid".equals(r2.get(0).path) && e.inbox().length == 1, "same stash not merged twice"); }

        { Env e = new Env(base, "nested");
          File in = new File(e.in(), "20261031-090000"); character(in, "Bob", "nested"); character(in, "Ann", "a"); stash(in, "s"); write(new File(in, "Settings/Settings.json"), "x");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && r.get(0).skipped == null && r.get(0).path.contains("Bob") && r.get(0).path.contains("Ann"), "fresh import result");
          check(Arrays.equals(bytes(new File(e.save, "Bob/Bob.d2s")), d2s("nested")) && Arrays.equals(bytes(new File(e.save, SC + "/" + SC + ".d2i")), d2i(100, "s"))
                && new File(e.save, "Settings/Settings.json").isFile() && e.inbox().length == 0, "everything in on a fresh install, nothing to merge"); }
        { Env e = new Env(base, "allskipped"); character(e.save, "yyc", "phone"); character(e.in(), "yyc", "import");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && r.get(0).path == null && "yyc".equals(r.get(0).skipped) && Arrays.equals(bytes(new File(e.save, "yyc/yyc.d2s")), d2s("phone")), "all skipped"); }
        { Env e = new Env(base, "reject"); character(e.save, "yyc", "keep");
          write(new File(e.in(), "readme.txt"), "x"); write(new File(e.in(), "Bob/notes.txt"), "x");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_NOTHING && r.get(0).path.equals("save-import.rejected-20261101-120000"), "rejected result");
          check(Arrays.equals(bytes(new File(e.save, "yyc/yyc.d2s")), d2s("keep")) && !new File(e.save, "Bob").exists() && new File(e.game, "save-import.rejected-20261101-120000/readme.txt").isFile(), "nothing touched"); }
        { Env e = new Env(base, "both"); character(e.save, "yyc", "old");
          write(new File(e.game, "export"), ""); character(e.in(), "Bob", "new");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 2 && r.get(0).kind == SaveTransfer.EXPORTED && r.get(1).kind == SaveTransfer.IMPORTED, "export then import");
          check(new File(e.game, "save-export/20261101-120000/yyc/yyc.d2s").isFile() && !new File(e.game, "save-export/20261101-120000/Bob").exists()
                && new File(e.save, "Bob/Bob.d2s").isFile(), "both done, the export holds the state before the import"); }

        { Env e = new Env(base, "same"); character(e.save, "yyc", "v"); stash(e.save, "phone stash");
          write(new File(e.game, "export"), "");
          check(e.run("20261101-120000").get(0).kind == SaveTransfer.EXPORTED, "same-phone export");
          File exp = new File(e.game, "save-export/20261101-120000"), id = new File(e.files, "save-transfer-id");
          check(id.isFile() && read(new File(exp, SaveTransfer.SOURCE_FILE)).trim().equals(read(id).trim()) && read(id).trim().length() == 36, "export carries this install's id");
          String first = read(id);
          copyDir(exp, new File(e.in(), "20261101-120000"));
          List<SaveTransfer.Result> r = e.run("20261101-130000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && r.get(0).merge == SaveTransfer.MERGE_SAME_PHONE && e.inbox().length == 0, "same phone: stash not merged");
          check(Arrays.equals(bytes(new File(e.save, SC + "/" + SC + ".d2i")), d2i(100, "phone stash")) && !new File(e.save, SaveTransfer.SOURCE_FILE).exists() && read(id).equals(first), "stash, saves and id untouched");
          Env o = new Env(base, "other"); character(o.save, "zed", "z"); stash(o.save, "other stash");
          copyDir(exp, o.in());
          r = o.run("20261101-140000");
          check(r.size() == 1 && r.get(0).merge == SaveTransfer.MERGE_QUEUED && o.inbox().length == 1, "another phone: stash merged");
          check(!new File(o.files, "save-transfer-id").exists() && Arrays.equals(bytes(new File(o.save, "yyc/yyc.d2s")), d2s("v")), "importing writes no id; the character is added"); }
    }


    static void components(File base) throws IOException {

        { Env e = new Env(base, "clean");
          character(e.in(), "Ann", "a"); write(new File(e.in(), "Ann/Ann.swpage"), "pg"); stash(e.in(), SC, 500, "sc"); stash(e.in(), HC, 700, "hc");
          write(new File(e.in(), "Ann/Ann.sw_fights"), "fl");
          write(new File(e.in(), "Ann/Ann.sw_streaks"), "sk");
          write(new File(e.in(), SC + ".swvault"), vault("v1")); write(new File(e.in(), HC + ".swvault"), vault("h1"));
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).merge == SaveTransfer.MERGE_NONE && e.inbox().length == 0, "clean restore: nothing merged");
          check(Arrays.equals(bytes(new File(e.save, SC + ".swvault")), vault("v1")) && Arrays.equals(bytes(new File(e.save, HC + ".swvault")), vault("h1"))
                && Arrays.equals(bytes(new File(e.save, HC + "/" + HC + ".d2i")), d2i(700, "hc")) && read(new File(e.save, "Ann/Ann.swpage")).equals("pg"), "clean restore: vaults, stashes and page files copied");
          check(read(new File(e.save, "Ann/Ann.sw_fights")).equals("fl"), "the fight log arrives with its character");
          check(read(new File(e.save, "Ann/Ann.sw_streaks")).equals("sk"), "the kill streaks arrive with their character");
          String l = e.ledger();
          check(l.contains("stash " + SC + " ") && l.contains("stash " + HC + " ") && l.contains("vault " + SC + " ") && l.contains("vault " + HC + " "), "every copied component recorded");
          r = e.run("2");
          check(r.isEmpty(), "save-import is done");
          copyDir(new File(e.game, "save-import.done-1"), e.in());
          r = e.run("3");
          check(r.get(0).merge == SaveTransfer.MERGE_ALREADY && e.inbox().length == 0 && "Ann".equals(r.get(0).skipped), "a direct copy imported again: nothing merged, no gold twice"); }

        { Env e = new Env(base, "mixed"); stash(e.save, SC, 100, "phone");
          stash(e.in(), SC, 500, "sc"); stash(e.in(), HC, 700, "hc"); write(new File(e.in(), SC + ".swvault"), vault("v1")); write(new File(e.in(), HC + ".swvault"), vault("h1"));
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && e.inbox().length == 1, "one entry");
          List<String> p = packed(e.inbox()[0]);
          check(p.equals(Arrays.asList(SC + "/" + SC + ".d2i", SC + ".swvault")), "the pack holds the SC stash and the SC vault at its root");
          check(Arrays.equals(bytes(new File(e.save, HC + "/" + HC + ".d2i")), d2i(700, "hc")) && Arrays.equals(bytes(new File(e.save, HC + ".swvault")), vault("h1"))
                && !new File(e.save, SC + ".swvault").exists(), "HC copied whole, the SC vault is not copied");

          File in = e.in(); stash(in, SC, 500, "sc"); stash(in, HC, 900, "hc changed"); write(new File(in, SC + ".swvault"), vault("v1")); write(new File(in, HC + ".swvault"), vault("h1"));
          r = e.run("2");
          check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && e.inbox().length == 2, "a second entry");
          check(packed(e.inbox()[1]).equals(Arrays.asList(HC + "/" + HC + ".d2i")), "a changed component never brings the unchanged ones back"); }

        { Env e = new Env(base, "vaultpages"); character(e.save, "Zed", "z"); write(new File(e.save, "Zed/Zed.swpouch"), "pouch");
          write(new File(e.in(), SC + ".swvault"), vault("v1")); character(e.in(), "Ann", "a");
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && !new File(e.save, SC + ".swvault").exists() && packed(e.inbox()[0]).equals(Arrays.asList(SC + ".swvault")), "vault merged, not copied"); }

        { Env e = new Env(base, "vaultstash"); stash(e.save, SC, 100, "phone"); stash(e.in(), SC, 100, "phone"); write(new File(e.in(), SC + ".swvault"), vault("v1"));
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && packed(e.inbox()[0]).equals(Arrays.asList(SC + "/" + SC + ".d2i", SC + ".swvault")), "vault merged when the phone has the stash"); }

        { Env e = new Env(base, "samephone"); stash(e.save, SC, 100, "phone"); write(new File(e.save, SC + ".swvault"), vault("v0"));
          write(new File(e.game, "export"), ""); e.run("1");
          File exp = new File(e.game, "save-export/1"); copyDir(exp, e.in()); stash(e.in(), HC, 50, "hc");
          List<SaveTransfer.Result> r = e.run("2");
          check(r.get(0).merge == SaveTransfer.MERGE_SAME_PHONE && e.inbox().length == 0 && new File(e.save, HC + "/" + HC + ".d2i").isFile(), "same phone: no merge, missing parts restored"); }

        { Env e = new Env(base, "lod"); write(new File(e.save, "SharedStashSoftCoreV2.d2i"), d2i(10, "phone"));
          write(new File(e.in(), "SharedStashSoftCoreV2.d2i"), d2i(30, "lod")); character(e.in(), "Ann", "a");
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && packed(e.inbox()[0]).equals(Arrays.asList("SharedStashSoftCoreV2.d2i")), "the root stash file goes into the pack"); }
    }


    static void idempotence(File base) throws IOException {
        Env src = new Env(base, "idem-src");
        character(src.save, "Ann", "a"); stash(src.save, SC, 800, "sc"); stash(src.save, HC, 300, "hc"); write(new File(src.save, SC + ".swvault"), vault("v1", "v2"));
        write(new File(src.save, "sw_elysium.txt"), "Ann\n");
        write(new File(src.game, "export"), ""); src.run("x");
        File backup = new File(src.game, "save-export/x");
        Env e = new Env(base, "idem"); character(e.save, "Zed", "z"); stash(e.save, SC, 100, "mine"); stash(e.save, HC, 100, "mine hc");
        copyDir(backup, e.in()); e.run("1");
        check(e.inbox().length == 1 && packed(e.inbox()[0]).size() == 3, "first import queues SC, HC and the SC vault");
        check(read(new File(e.save, "sw_elysium.txt")).equals("Ann\n"), "the imported character keeps its mode marker");
        String after = tree(e.save), ledger = e.ledger();
        for (int i = 2; i <= 4; i++) {
            copyDir(backup, e.in());
            List<SaveTransfer.Result> r = e.run(String.valueOf(i));
            check(r.get(0).merge == SaveTransfer.MERGE_ALREADY && r.get(0).path == null && "Ann".equals(r.get(0).skipped), "import " + i + ": already merged");
            check(tree(e.save).equals(after) && e.ledger().equals(ledger), "import " + i + ": the phone is unchanged");
        }

        deleteDir(new File(e.save, "_cloudinbox"));
        copyDir(backup, e.in());
        check(e.run("5").get(0).merge == SaveTransfer.MERGE_ALREADY && e.inbox().length == 0, "the ledger holds even when the inbox is gone");

        write(new File(e.game, "export"), ""); e.run("6");
        Env t = new Env(base, "idem-third"); stash(t.save, SC, 100, "third"); stash(t.save, HC, 100, "third hc");
        copyDir(new File(e.game, "save-export/6"), t.in()); t.run("1");
        int entries = t.inbox().length;
        copyDir(backup, t.in());
        List<SaveTransfer.Result> r = t.run("2");
        check(r.get(0).merge == SaveTransfer.MERGE_ALREADY && t.inbox().length == entries, "components recorded on another phone are not merged again");
    }


    static void journal(File base) throws IOException {
        Env src = new Env(base, "j-src"); character(src.save, "Ann", "a"); character(src.save, "Bea", "b"); stash(src.save, SC, 800, "sc"); stash(src.save, HC, 300, "hc");
        write(new File(src.save, SC + ".swvault"), vault("v1")); write(new File(src.save, "sw_elysium.txt"), "Bea\n");
        write(new File(src.files, "sw_custom.txt"), "fps=30\n"); src.builds.p.setProperty("Ann#2", "g,1,auto");
        write(new File(src.game, "export"), ""); src.run("x");
        File backup = new File(src.game, "save-export/x");
        Env ref = new Env(base, "j-ref"); stash(ref.save, SC, 100, "mine"); copyDir(backup, ref.in()); ref.run("1");
        String want = tree(ref.save).replaceAll("/_cloudinbox/[0-9]+-import", "/_cloudinbox/E");

        List<String> points = new ArrayList<>(Arrays.asList("journal"));
        for (int i = 1; i <= 5; i++) points.add("move" + i);
        points.add("ledger");
        for (String at : points) {
            Env e = new Env(base, "j-" + at); stash(e.save, SC, 100, "mine"); copyDir(backup, e.in());
            System.setProperty("dm.transfer.kill", at);
            boolean died = false;
            try { e.run("1"); } catch (SaveTransfer.Killed k) { died = true; }
            System.clearProperty("dm.transfer.kill");
            check(died && new File(e.state, "journal").isFile(), at + ": died with the journal written");
            if (at.equals("journal")) check(read(new File(e.state, "journal")).split("\nmove\t", -1).length - 1 == 5, "five moves in the journal");
            List<SaveTransfer.Result> r = e.run("2");
            check(r.size() == 1 && r.get(0).resumed && r.get(0).kind == SaveTransfer.IMPORTED && r.get(0).merge == SaveTransfer.MERGE_QUEUED, at + ": the next start finishes it");
            check(tree(e.save).replaceAll("/_cloudinbox/[0-9]+-import", "/_cloudinbox/E").equals(want) && e.inbox().length == 1 && inboxShape(e).equals(inboxShape(ref)), at + ": same end state as without a crash");
            check(!new File(e.state, "journal").exists() && !new File(e.state, "stage").exists() && !e.in().exists() && new File(e.game, "save-import.done-1").isDirectory(), at + ": journal, stage and save-import done");
            check(read(new File(e.files, "sw_custom.txt")).equals("fps=30\n") && "g,1,auto".equals(e.builds.p.getProperty("Ann#2")), at + ": own data in");
            copyDir(backup, e.in());
            check(e.run("3").get(0).merge == SaveTransfer.MERGE_ALREADY && e.inbox().length == 1, at + ": imported again, nothing doubled");
        }

        { Env e = new Env(base, "j-stage"); copyDir(backup, e.in());
          new File(e.state, "stage/save/Ann").mkdirs(); write(new File(e.state, "stage/save/Ann/Ann.d2s"), "half");
          List<SaveTransfer.Result> r = e.run("1");
          check(r.size() == 1 && !r.get(0).resumed && Arrays.equals(bytes(new File(e.save, "Ann/Ann.d2s")), d2s("a")), "a stage without a journal is discarded"); }

        { Env e = new Env(base, "j-export"); character(e.save, "Ann", "a"); write(new File(e.game, "export"), "");
          File part = new File(e.game, "save-export/old.part"); write(new File(part, "Ann/Ann.d2s"), "half");
          e.state.mkdirs(); write(new File(e.state, "export"), part.getAbsolutePath() + "\n");
          List<SaveTransfer.Result> r = e.run("1");
          check(!part.exists() && r.size() == 1 && r.get(0).kind == SaveTransfer.EXPORTED && new File(e.game, "save-export/1/Ann/Ann.d2s").isFile(), "an unfinished export is removed and done again"); }

        { Env e = new Env(base, "j-fail"); copyDir(backup, e.in()); write(new File(e.files, "stash_backup"), "in the way");
          write(new File(e.in(), "_app/stash_backup/" + SC + ".1.d2i"), "bk");
          List<SaveTransfer.Result> r = e.run("1");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.FAILED && new File(e.state, "journal").isFile() && new File(e.state, "stage/stash_backup/" + SC + ".1.d2i").isFile(), "a failed publish keeps the transaction");
          new File(e.files, "stash_backup").delete();
          r = e.run("2");
          check(r.size() == 1 && r.get(0).resumed && read(new File(e.files, "stash_backup/" + SC + ".1.d2i")).equals("bk") && !new File(e.state, "journal").exists(), "the next start finishes it"); }

        { Env e = new Env(base, "j-lost"); copyDir(backup, e.in());
          System.setProperty("dm.transfer.kill", "journal");
          try { e.run("1"); } catch (SaveTransfer.Killed k) { }
          System.clearProperty("dm.transfer.kill");
          deleteDir(new File(e.state, "stage/save/Ann"));
          List<SaveTransfer.Result> r = e.run("2");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.FAILED && new File(e.state, "journal").isFile() && e.ledger().isEmpty(), "a lost move fails, nothing recorded"); }

        { Env e = new Env(base, "j-renamed"); copyDir(backup, e.in());
          System.setProperty("dm.transfer.kill", "ledger");
          try { e.run("1"); } catch (SaveTransfer.Killed k) { }
          System.clearProperty("dm.transfer.kill");
          check(new File(e.state, "journal").isFile(), "died before the rename");
          String j = read(new File(e.state, "journal")); String dest = j.substring(j.indexOf("\nrename\t") + 1).split("\n")[0].split("\t")[2];
          e.in().renameTo(new File(dest));
          character(e.in(), "Neu", "n");
          List<SaveTransfer.Result> r = e.run("2");
          check(r.size() == 2 && r.get(0).resumed && r.get(1).kind == SaveTransfer.IMPORTED && "Neu".equals(r.get(1).path) && new File(e.save, "Neu/Neu.d2s").isFile(), "the new backup is imported"); }

        { Env e = new Env(base, "j-bad"); e.state.mkdirs(); write(new File(e.state, "journal"), "SWXJ1\nmove\tsave/X\tsave/X\n");
          List<SaveTransfer.Result> r = e.run("1");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.FAILED && !new File(e.state, "journal").exists(), "a journal without its end is dropped"); }
    }


    static byte[] plan(int items, long gold) {
        ByteBuffer b = ByteBuffer.allocate(16 + items * 4).order(ByteOrder.LITTLE_ENDIAN);
        b.put(new byte[] { 'S', 'W', 'C', 'M', 1, 0 }).putShort((short) items).putLong(gold);
        for (int i = 0; i < items; i++) b.putShort((short) 2).put((byte) 1).put((byte) 2);
        return b.array();
    }

    static byte[] pack(int tag, String... paths) {
        List<Object[]> files = new ArrayList<>();
        for (String p : paths) files.add(new Object[] { p, p.endsWith(".swvault") ? vault("v" + tag) : d2i(tag, "t" + tag), 1L });
        return SaveTransfer.packOf(files);
    }
    static void inboxTakeOver(File base) throws IOException {
        Env e = new Env(base, "inbox"); character(e.in(), "Ann", "a");
        File ib = new File(e.in(), "_cloudinbox");
        write(new File(ib, "100-import/mine.pack"), pack(1, SC + "/" + SC + ".d2i"));
        write(new File(ib, "200-import/mine.pack"), pack(2, SC + "/" + SC + ".d2i", HC + "/" + HC + ".d2i")); write(new File(ib, "200-import/plan-" + SC + ".bin"), plan(0, 0));
        write(new File(ib, "300-import/mine.pack"), pack(3, SC + ".swvault")); write(new File(ib, "300-import/plan-" + SC + ".bin"), plan(0, 0));
        write(new File(ib, "400-import/mine.pack"), pack(4, SC + ".d2i")); write(new File(ib, "400-import/plan-" + SC + ".bin"), plan(0, 5000));
        write(new File(ib, "450-import/mine.pack"), pack(1, SC + "/" + SC + ".d2i"));
        write(new File(ib, "500-import.part/mine.pack"), pack(5, SC + "/" + SC + ".d2i"));
        List<SaveTransfer.Result> r = e.run("1");
        check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && e.inbox().length == 3, "three open entries taken over");
        List<String> plans = new ArrayList<>();
        for (File x : e.inbox()) { check(x.getName().matches("[0-9]+-import"), "renamed as an import entry"); File pf = new File(x, "plan-" + SC + ".bin"); plans.add(pf.isFile() ? String.valueOf(bytes(pf).length) : "-"); }
        check(plans.contains("-") && plans.contains("16"), "plans come along with their entries");
        copyDir(new File(e.game, "save-import.done-1"), e.in());
        e.run("2");
        check(e.inbox().length == 3, "not taken over twice");

        Env f = new Env(base, "inbox-own"); stash(f.save, SC, 100, "phone"); stash(f.in(), SC, 300, "other");
        f.run("1");
        check(f.inbox().length == 1, "queued here");
        write(new File(f.game, "export"), ""); f.run("2");
        File exp = new File(f.game, "save-export/2");

        Env g = new Env(base, "inbox-fresh"); copyDir(exp, g.in());
        List<SaveTransfer.Result> rg = g.run("1");
        check(rg.get(0).merge == SaveTransfer.MERGE_QUEUED && g.inbox().length == 1 && new File(g.save, SC + "/" + SC + ".d2i").isFile(), "an open merge moves to a fresh install");
        new File(exp, SaveTransfer.SOURCE_FILE).delete();
        deleteDir(new File(exp, SC)); character(exp, "Ann", "a");
        copyDir(exp, f.in());
        f.run("3");
        check(f.inbox().length == 1, "the phone's own entry from its backup is not added again");
    }


    static void carriedTwice(File base) throws IOException {
        Env b = new Env(base, "two-b"); stash(b.save, SC, 900, "backup b"); character(b.save, "Bea", "b"); write(new File(b.game, "export"), ""); b.run("x");
        File backup = new File(b.game, "save-export/x");
        Env pa = new Env(base, "two-a"), pc = new Env(base, "two-c");
        stash(pa.save, SC, 100, "phone a"); stash(pc.save, SC, 100, "phone c");
        copyDir(backup, pa.in()); pa.run("1"); copyDir(backup, pc.in()); pc.run("1");
        check(pa.inbox().length == 1 && pc.inbox().length == 1, "both phones queued backup b");
        String bKey = SaveTransfer.packComponents(bytes(new File(pc.inbox()[0], "mine.pack"))).iterator().next();
        write(new File(pa.game, "export"), ""); pa.run("2");
        File exp = new File(pa.game, "save-export/2"); character(exp, "Ann", "a");
        copyDir(exp, pc.in()); pc.run("2");
        int carryB = 0; for (File x : pc.inbox()) carryB += SaveTransfer.packComponents(bytes(new File(x, "mine.pack"))).contains(bKey) ? 1 : 0;
        check(carryB == 1, "phone a's unfinished merge of backup b is not carried to phone c, which merged b itself");
    }


    static void carriedPartly(File base) throws IOException {
        Env e = new Env(base, "partly"); character(e.in(), "Ann", "a");
        e.state.mkdirs();
        Env probe = new Env(base, "partly-probe"); stash(probe.save, SC, 1, "other"); stash(probe.in(), SC, 10, "sc part"); probe.run("1");
        String scKey = null; for (String l : probe.ledger().split("\n")) if (l.startsWith("stash " + SC + " ")) scKey = l;
        write(new File(e.state, "ledger"), scKey + "\n");
        byte[] scBytes = d2i(10, "sc part"), hcBytes = d2i(20, "hc part");
        List<Object[]> files = new ArrayList<>();
        files.add(new Object[] { SC + "/" + SC + ".d2i", scBytes, 1L }); files.add(new Object[] { HC + "/" + HC + ".d2i", hcBytes, 1L });
        files.add(new Object[] { SC + ".swvault", vault("neW in the vault"), 1L });
        File ib = new File(e.in(), "_cloudinbox/100-import");
        write(new File(ib, "mine.pack"), SaveTransfer.packOf(files)); write(new File(ib, "plan-" + HC + ".bin"), plan(1, 20));
        List<SaveTransfer.Result> r = e.run("1");
        check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && e.inbox().length == 1, "the entry comes along");
        File x = e.inbox()[0];
        check(packed(x).equals(Arrays.asList(HC + "/" + HC + ".d2i", SC + ".swvault")) && new File(x, "plan-" + HC + ".bin").isFile() && !new File(x, "plan-" + SC + ".bin").exists(),
              "the unknown parts come along (hard core with its plan, the soft core vault without the soft core plan, which is computed again)");
        check(e.ledger().contains("stash " + HC + " ") && e.ledger().contains("vault " + SC + " "), "the carried parts are recorded");

        Env m = new Env(base, "partly-started"); character(m.in(), "Ann", "a"); m.state.mkdirs(); write(new File(m.state, "ledger"), scKey + "\n");
        File ib2 = new File(m.in(), "_cloudinbox/100-import");
        byte[] vaultOwed = moved("neW in the vault"), stashOwed = "a Stash item".getBytes(StandardCharsets.US_ASCII);
        write(new File(ib2, "mine.pack"), SaveTransfer.packOf(files)); write(new File(ib2, "plan-" + SC + ".bin"), planOf(30, stashOwed, vaultOwed)); write(new File(ib2, "plan-" + HC + ".bin"), plan(1, 20));
        m.run("1");
        File mx = m.inbox()[0];
        check(m.inbox().length == 1 && packed(mx).equals(Arrays.asList(HC + "/" + HC + ".d2i", SC + ".swvault")), "a started soft core merge brings its unknown vault along");
        List<byte[]> owed = SaveTransfer.planItems(bytes(new File(mx, "plan-" + SC + ".bin")));
        check(owed.size() == 1 && Arrays.equals(owed.get(0), vaultOwed) && planGold(new File(mx, "plan-" + SC + ".bin")) == 0, "its plan keeps the vault item still owed, not the stash item, and no gold");
        check(Arrays.equals(bytes(new File(mx, "plan-" + HC + ".bin")), plan(1, 20)) && m.ledger().contains("vault " + SC + " "), "the untouched hard core plan comes along unchanged, the vault is recorded");

        Env n = new Env(base, "partly-vault-known"); character(n.in(), "Ann", "a"); n.state.mkdirs();
        String vKey = null; for (String c : SaveTransfer.packComponents(SaveTransfer.packOf(files))) if (c.startsWith("vault " + SC + " ")) vKey = c;
        write(new File(n.state, "ledger"), vKey + "\n");
        File ib3 = new File(n.in(), "_cloudinbox/100-import");
        write(new File(ib3, "mine.pack"), SaveTransfer.packOf(files)); write(new File(ib3, "plan-" + SC + ".bin"), planOf(30, stashOwed, vaultOwed));
        n.run("1");
        File nx = n.inbox()[0]; owed = SaveTransfer.planItems(bytes(new File(nx, "plan-" + SC + ".bin")));
        check(packed(nx).equals(Arrays.asList(HC + "/" + HC + ".d2i", SC + "/" + SC + ".d2i")) && owed.size() == 1 && Arrays.equals(owed.get(0), stashOwed)
              && planGold(new File(nx, "plan-" + SC + ".bin")) == 30, "a known vault: the stash part of the plan with its gold comes along");

        Env o = new Env(base, "partly-both-known"); character(o.in(), "Ann", "a"); o.state.mkdirs(); write(new File(o.state, "ledger"), scKey + "\n" + vKey + "\n");
        File ib4 = new File(o.in(), "_cloudinbox/100-import");
        write(new File(ib4, "mine.pack"), SaveTransfer.packOf(files)); write(new File(ib4, "plan-" + SC + ".bin"), planOf(30, stashOwed, vaultOwed));
        o.run("1");
        check(packed(o.inbox()[0]).equals(Arrays.asList(HC + "/" + HC + ".d2i")) && !new File(o.inbox()[0], "plan-" + SC + ".bin").exists(), "both soft core parts known: soft core stays out");

        Env q = new Env(base, "partly-paid"); character(q.in(), "Ann", "a"); q.state.mkdirs(); write(new File(q.state, "ledger"), "paid " + scKey + "\n");
        File ib5 = new File(q.in(), "_cloudinbox/100-import");
        write(new File(ib5, "mine.pack"), SaveTransfer.packOf(files)); write(new File(ib5, "plan-" + SC + ".bin"), planOf(30, stashOwed, vaultOwed));
        q.run("1");
        File qa = q.inbox()[0]; owed = SaveTransfer.planItems(bytes(new File(qa, "plan-" + SC + ".bin")));
        check(owed.size() == 2 && planGold(new File(qa, "plan-" + SC + ".bin")) == 0 && packed(qa).size() == 3 && !new File(qa, "nogold-" + SC).exists(), "paid stash with a plan: all items still owed, no gold");
        Env q2 = new Env(base, "partly-paid-noplan"); character(q2.in(), "Ann", "a"); q2.state.mkdirs(); write(new File(q2.state, "ledger"), "paid " + scKey + "\n");
        List<Object[]> only = new ArrayList<>(); only.add(files.get(0)); write(new File(q2.in(), "_cloudinbox/200-import/mine.pack"), SaveTransfer.packOf(only));
        q2.run("1");
        File qb = q2.inbox()[0];
        check(new File(qb, "nogold-" + SC).isFile() && q2.ledger().contains("\n" + scKey + "\n"), "paid stash without a plan: marked nogold, then recorded as known");
        check(q.inbox().length == 1, "one entry for the paid stash");

        Env w2 = new Env(base, "partly-cut-whole"); character(w2.in(), "Ann", "a"); w2.state.mkdirs(); write(new File(w2.state, "ledger"), vKey + "\n");
        File ibw = new File(w2.in(), "_cloudinbox/100-import");
        write(new File(ibw, "mine.pack"), SaveTransfer.packOf(files)); write(new File(ibw, "plan-" + SC + ".bin"), planOf(0, vaultOwed));
        w2.run("1");
        check(Arrays.asList(w2.ledger().split("\n")).contains(scKey), "the cut away stash is recorded");

        byte[] rune = "same rune bits".getBytes(StandardCharsets.US_ASCII);
        List<Object[]> runes = new ArrayList<>(); runes.add(new Object[] { SC + "/" + SC + ".d2i", scBytes, 1L }); runes.add(new Object[] { SC + ".swvault", vault("same rune bits"), 1L });
        String rsKey = null, rvKey = null; for (String c : SaveTransfer.packComponents(SaveTransfer.packOf(runes))) { if (c.startsWith("stash ")) rsKey = c; else rvKey = c; }
        for (int known = 0; known < 2; known++) {
            Env u2 = new Env(base, "partly-runes-" + known); character(u2.in(), "Ann", "a"); u2.state.mkdirs(); write(new File(u2.state, "ledger"), (known == 0 ? rsKey : rvKey) + "\n");
            File ibr = new File(u2.in(), "_cloudinbox/100-import");
            write(new File(ibr, "mine.pack"), SaveTransfer.packOf(runes)); write(new File(ibr, "plan-" + SC + ".bin"), planOf(0, rune, moved("same rune bits")));
            u2.run("1");
            int left = SaveTransfer.planItems(bytes(new File(u2.inbox()[0], "plan-" + SC + ".bin"))).size();
            check(left == (known == 0 ? 1 : 2), (known == 0 ? "a known stash keeps only the vault's count" : "a known vault keeps the stash items, the larger count") + " (" + left + ")");
        }

        Env t = new Env(base, "truncated"); character(t.in(), "Ann", "a");
        byte[] whole = SaveTransfer.packOf(files);
        write(new File(t.in(), "_cloudinbox/100-import/mine.pack"), Arrays.copyOf(whole, whole.length - 5));
        r = t.run("1");
        check(r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_FILE && "_cloudinbox/100-import/mine.pack".equals(r.get(0).detail)
              && t.ledger().isEmpty() && !new File(t.save, "Ann").exists(), "a damaged inbox pack rejects the import");

        Env w = new Env(base, "badinside"); character(w.in(), "Ann", "a");
        List<Object[]> bad = new ArrayList<>(); byte[] d = d2i(10, "x"); bad.add(new Object[] { SC + "/" + SC + ".d2i", Arrays.copyOf(d, d.length - 3), 1L });
        write(new File(w.in(), "_cloudinbox/100-import/mine.pack"), SaveTransfer.packOf(bad));
        r = w.run("1");
        check(r.get(0).bad == SaveTransfer.BAD_FILE && r.get(0).detail.endsWith(":" + SC + "/" + SC + ".d2i") && w.ledger().isEmpty(), "a damaged stash inside an inbox pack rejects the import");

        Env u = new Env(base, "badplan"); character(u.in(), "Ann", "a");
        write(new File(u.in(), "_cloudinbox/100-import/mine.pack"), whole); byte[] pl = plan(2, 5); write(new File(u.in(), "_cloudinbox/100-import/plan-" + HC + ".bin"), Arrays.copyOf(pl, pl.length - 1));
        r = u.run("1");
        check(r.get(0).bad == SaveTransfer.BAD_FILE && ("_cloudinbox/100-import/plan-" + HC + ".bin").equals(r.get(0).detail) && u.ledger().isEmpty(), "a damaged plan rejects the import");
        Env v = new Env(base, "hugepack"); character(v.in(), "Ann", "a");
        new File(v.in(), "_cloudinbox/100-import").mkdirs();
        try (RandomAccessFile f = new RandomAccessFile(new File(v.in(), "_cloudinbox/100-import/mine.pack"), "rw")) { f.setLength(SaveTransfer.MAX_PACK + 1); }
        r = v.run("1");
        check(r.get(0).bad == SaveTransfer.BAD_SIZE && "_cloudinbox/100-import/mine.pack".equals(r.get(0).detail), "a pack over the pack limit rejects the import");
    }


    static byte[] planOf(long gold, byte[]... items) { return SaveTransfer.planOf(Arrays.asList(items), gold); }
    static long planGold(File f) throws IOException { return ByteBuffer.wrap(bytes(f)).order(ByteOrder.LITTLE_ENDIAN).getLong(8); }

    static byte[] moved(String item) { byte[] b = item.getBytes(StandardCharsets.US_ASCII).clone(); b[5] ^= 0x7f; b[4] ^= 0x18; return b; }



    static void skippedLedger(File base) throws IOException {
        Env z = new Env(base, "held-b0"); stash(z.save, SC, 900, "backup b0"); write(new File(z.save, SC + ".swvault"), vault("b0 vault item"));
        write(new File(z.game, "export"), ""); z.run("x");
        File b0 = new File(z.game, "save-export/x");
        Env a = new Env(base, "held-a"); character(a.save, "Ann", "a"); stash(a.save, SC, 100, "phone a");
        copyDir(b0, a.in()); a.run("1");
        check(a.inbox().length == 1 && a.ledger().contains("stash " + SC + " ") && a.ledger().contains("vault " + SC + " "), "phone a queued backup b0");
        write(new File(a.inbox()[0], "plan-" + SC + ".bin"), plan(0, 0));
        write(new File(a.game, "export"), ""); a.run("2");
        File b1 = new File(a.game, "save-export/2");

        Env c = new Env(base, "held-c"); character(c.save, "Ann", "c's own"); stash(c.save, SC, 100, "phone c");
        copyDir(b1, c.in()); List<SaveTransfer.Result> r = c.run("1");
        check(r.get(0).held >= 2 && "Ann".equals(r.get(0).skipped), "the import reports the records it could not take over (" + r.get(0).held + ")");
        String l = c.ledger();
        check(l.contains("paid stash " + SC + " ") && !l.contains("\nvault " + SC + " ") && !l.startsWith("vault "), "the b0 stash record only as paid, its vault record not at all");
        int before = c.inbox().length;
        copyDir(b0, c.in()); r = c.run("2");
        File last = c.inbox()[c.inbox().length - 1];
        check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && c.inbox().length == before + 1 && packed(last).equals(Arrays.asList(SC + "/" + SC + ".d2i", SC + ".swvault"))
              && new File(last, "nogold-" + SC).isFile(), "the earlier backup merges again, marked nogold");
        copyDir(b0, c.in()); r = c.run("3");
        check(r.get(0).merge == SaveTransfer.MERGE_ALREADY && c.inbox().length == before + 1, "and only once");


        Env a2 = new Env(base, "held-a2"); character(a2.save, "Ann", "a"); stash(a2.save, SC, 100, "phone a2");
        copyDir(b0, a2.in()); a2.run("1");
        write(new File(a2.inbox()[0], "plan-" + SC + ".bin"), planOf(0, "an item still owed".getBytes(StandardCharsets.US_ASCII)));
        write(new File(a2.game, "export"), ""); a2.run("2");
        Env c2 = new Env(base, "held-c2"); character(c2.save, "Ann", "c2's own"); stash(c2.save, SC, 100, "phone c2");
        copyDir(new File(a2.game, "save-export/2"), c2.in()); r = c2.run("1");
        check(c2.inbox().length == 2 && r.get(0).held >= 2, "the half done merge comes along and is reported");
        l = c2.ledger();
        String b0s = null, b0v = null; for (String x : a2.ledger().split("\n")) { if (x.startsWith("stash " + SC + " ")) b0s = x; if (x.startsWith("vault " + SC + " ")) b0v = x; }
        List<String> lines = Arrays.asList(l.split("\n"));
        check(lines.contains("paid " + b0s) && !lines.contains(b0s) && !lines.contains(b0v), "its b0 parts only as paid");
        copyDir(b0, c2.in()); r = c2.run("2");
        File last2 = c2.inbox()[c2.inbox().length - 1];
        check(r.get(0).merge == SaveTransfer.MERGE_QUEUED && new File(last2, "nogold-" + SC).isFile(), "the earlier backup merges again later, without gold");

        int n2 = c2.inbox().length;
        copyDir(new File(a2.game, "save-export/2"), c2.in()); c2.run("3");
        check(c2.inbox().length == n2, "a held entry comes along once per pack and plan");

        Env g2 = new Env(base, "held-gold-twice"); character(g2.save, "Ann", "own"); character(g2.in(), "Ann", "a");
        List<Object[]> one = new ArrayList<>(); one.add(new Object[] { SC + "/" + SC + ".d2i", d2i(70, "twice"), 1L });
        for (String ent : new String[] { "100-import", "200-import" }) {
            write(new File(g2.in(), "_cloudinbox/" + ent + "/mine.pack"), SaveTransfer.packOf(one)); write(new File(g2.in(), "_cloudinbox/" + ent + "/plan-" + SC + ".bin"), plan(ent.startsWith("1") ? 1 : 2, 100)); }
        g2.run("1");
        long gold = 0; for (File x : g2.inbox()) { File pf = new File(x, "plan-" + SC + ".bin"); if (pf.isFile()) gold += planGold(pf); }
        check(gold == 100, "gold left in two entries of one stash is queued once (" + gold + ")");

        Env f2 = new Env(base, "held-fuller"); character(f2.save, "Ann", "own");
        List<Object[]> fp = new ArrayList<>(); fp.add(new Object[] { SC + "/" + SC + ".d2i", d2i(8, "fuller"), 1L }); byte[] fpack = SaveTransfer.packOf(fp);
        for (int round = 0; round < 3; round++) {
            character(f2.in(), "Ann", "a"); File en = new File(f2.in(), "_cloudinbox/100-import");
            write(new File(en, "mine.pack"), fpack); write(new File(en, "plan-" + SC + ".bin"), plan(round < 2 ? 1 : 3, 0)); f2.run("f" + round);
        }
        check(f2.inbox().length == 2, "the same plan twice comes along once, a fuller plan of the same pack again (" + f2.inbox().length + ")");

        Env c3 = new Env(base, "held-c3"); character(c3.save, "Ann", "c3's own"); stash(c3.save, SC, 100, "phone c3");
        copyDir(new File(a2.game, "save-export/2"), c3.in()); c3.run("1");
        write(new File(c3.game, "export"), ""); c3.run("x");
        Env f3 = new Env(base, "held-export"); stash(f3.save, SC, 100, "phone f3");
        copyDir(new File(c3.game, "save-export/x"), f3.in()); f3.run("1");
        List<String> f3l = Arrays.asList(f3.ledger().split("\n"));
        check(!f3l.contains(b0s) && !f3l.contains(b0v) && f3l.contains("paid " + b0s), "a part held on the exporting phone stays held");

        Env h2 = new Env(base, "held-plan-changed"); character(h2.save, "Ann", "own");
        List<Object[]> two = new ArrayList<>(); two.add(new Object[] { SC + "/" + SC + ".d2i", d2i(5, "sc two"), 1L }); two.add(new Object[] { HC + "/" + HC + ".d2i", d2i(6, "hc two"), 1L });
        byte[] twoPack = SaveTransfer.packOf(two);
        for (int round = 0; round < 2; round++) {
            character(h2.in(), "Ann", "a"); File en = new File(h2.in(), "_cloudinbox/100-import");
            write(new File(en, "mine.pack"), twoPack); write(new File(en, "plan-" + SC + ".bin"), plan(1, 0)); write(new File(en, "plan-" + HC + ".bin"), plan(round + 1, 0));
            h2.run(String.valueOf(round));
        }
        int scCarried = 0; for (File x : h2.inbox()) scCarried += new File(x, "plan-" + SC + ".bin").isFile() ? 1 : 0;
        check(h2.inbox().length == 2 && scCarried == 1, "the unchanged soft core plan comes along once (" + scCarried + " of " + h2.inbox().length + ")");

        Env p2 = new Env(base, "held-paid-alone"); character(p2.save, "Ann", "own"); character(p2.in(), "Ann", "a");
        List<Object[]> lone = new ArrayList<>(); lone.add(new Object[] { SC + "/" + SC + ".d2i", d2i(100, "lone"), 1L });
        write(new File(p2.in(), "_cloudinbox/100-import/mine.pack"), SaveTransfer.packOf(lone)); write(new File(p2.in(), "_cloudinbox/100-import/plan-" + SC + ".bin"), plan(0, 100));
        p2.run("1");
        stash(p2.in(), SC, 100, "lone"); character(p2.in(), "Bea", "b");
        r = p2.run("2");
        File pl = p2.inbox()[p2.inbox().length - 1];
        check(!new File(p2.save, SC).exists() && r.get(0).merge == SaveTransfer.MERGE_QUEUED && new File(pl, "nogold-" + SC).isFile(), "a paid stash is not copied whole with its gold");

        Env d = new Env(base, "held-d"); stash(d.save, SC, 100, "phone d");
        copyDir(b1, d.in()); r = d.run("1");
        check(r.get(0).held == 0 && d.ledger().contains("vault " + SC + " ") && !d.ledger().contains("paid "), "nothing held without a skipped character");
        copyDir(b0, d.in()); r = d.run("2");
        check(r.get(0).merge == SaveTransfer.MERGE_ALREADY, "the earlier backup is done there");

        Env j = new Env(base, "held-journal"); character(j.save, "Ann", "j's own"); stash(j.save, SC, 100, "phone j");
        copyDir(b1, j.in());
        System.setProperty("dm.transfer.kill", "journal");
        try { j.run("1"); check(false, "killed at the journal"); } catch (SaveTransfer.Killed k) { }
        System.clearProperty("dm.transfer.kill");
        r = j.run("2");
        check(r.get(0).resumed && r.get(0).held >= 2, "held survives the journal");
    }


    static void carriedThenAlone(File base) throws IOException {
        Env e = new Env(base, "carried-alone"); character(e.in(), "Ann", "a");
        List<Object[]> files = new ArrayList<>(); files.add(new Object[] { "SharedStashSoftCoreV2.d2i", d2i(40, "lod"), 1L });
        write(new File(e.in(), "_cloudinbox/100-import/mine.pack"), SaveTransfer.packOf(files));
        e.run("1");
        check(e.inbox().length == 1, "carried");
        write(new File(e.in(), "SharedStashSoftCoreV2.d2i"), d2i(40, "lod")); character(e.in(), "Bea", "b");
        List<SaveTransfer.Result> r = e.run("2");
        check(r.get(0).merge == SaveTransfer.MERGE_ALREADY && !new File(e.save, "SharedStashSoftCoreV2.d2i").exists() && e.inbox().length == 1, "not copied while its merge is pending");
    }


    static void limits(File base) throws IOException {
        { Env e = new Env(base, "big"); character(e.save, "Zed", "z"); character(e.in(), "Ann", "a");
          try (RandomAccessFile f = new RandomAccessFile(new File(e.in(), "Ann/Ann.ma0"), "rw")) { f.setLength(SaveTransfer.MAX_FILE + 1); }
          String before = tree(e.save);
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_SIZE && "Ann/Ann.ma0".equals(r.get(0).detail) && tree(e.save).equals(before)
                && new File(e.game, "save-import.rejected-1").isDirectory(), "a file over 16 MiB rejects the import"); }

        { Env e = new Env(base, "bigpack"); character(e.in(), "Ann", "a");
          byte[] nine = new byte[9 << 20]; ByteBuffer nb = ByteBuffer.wrap(nine).order(ByteOrder.LITTLE_ENDIAN); nb.putInt(0, 0xaa55aa55).putInt(16, nine.length);
          List<Object[]> two = new ArrayList<>();
          two.add(new Object[] { SC + "/a.d2i", nine, 1L }); two.add(new Object[] { SC + "/b.d2i", nine, 1L });
          write(new File(e.in(), "_cloudinbox/100-import/mine.pack"), SaveTransfer.packOf(two));
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).kind == SaveTransfer.IMPORTED && e.inbox().length == 1, "a pack of two 9 MiB files is carried"); }

        { Env e = new Env(base, "bigmerge"); stash(e.save, SC, 1, "phone");
          for (int i = 0; i < 5; i++) { byte[] f = new byte[15 << 20]; ByteBuffer fb = ByteBuffer.wrap(f).order(ByteOrder.LITTLE_ENDIAN); fb.putInt(0, 0xaa55aa55).putInt(16, f.length); f[20] = (byte) i; write(new File(e.in(), SC + "/p" + i + ".d2i"), f); }
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_SIZE && e.inbox().length == 0 && !new File(e.state, "stage").exists(), "a merge entry over the pack limit rejects the import"); }
        { Env e = new Env(base, "many"); character(e.in(), "Ann", "a");
          for (int i = 0; i < SaveTransfer.MAX_FILES; i++) write(new File(e.in(), "Ann/x" + i), "");
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_COUNT && !new File(e.save, "Ann").exists(), "too many files reject the import"); }
        { Env e = new Env(base, "packmany"); stash(e.save, SC, 1, "p");
          for (int i = 0; i < SaveTransfer.MAX_PACK_FILES; i++) write(new File(e.in(), SC + "/p" + i + ".d2i"), d2i(1, "x" + i));
          write(new File(e.in(), SC + ".swvault"), vault("v"));
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_COUNT && e.inbox().length == 0 && !new File(e.state, "stage").exists(), "more than 64 files for one pack reject the import"); }
        String[][] damaged = { { "Ann/Ann.d2s", "d2s" }, { SC + "/" + SC + ".d2i", "d2i" }, { SC + ".swvault", "vault" } };
        for (String[] d : damaged) {
          Env e = new Env(base, "bad-" + d[1]); character(e.in(), "Ann", "a"); stash(e.in(), SC, 1, "s"); write(new File(e.in(), SC + ".swvault"), vault("v"));
          byte[] b = bytes(new File(e.in(), d[0])); write(new File(e.in(), d[0]), Arrays.copyOf(b, b.length - 1));
          List<SaveTransfer.Result> r = e.run("1");
          check(r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_FILE && d[0].equals(r.get(0).detail) && !new File(e.save, "Ann").exists() && e.ledger().isEmpty(), "damaged " + d[1] + " rejects the import");
        }
        check(SaveTransfer.validD2i(new File(base, "bad-d2s/D2R Mobile/save-import.rejected-1/" + SC + "/" + SC + ".d2i")), "the test stash is valid");
    }


    static void buildsFail(File base) throws IOException {
        Env e = new Env(base, "builds-fail"); character(e.in(), "Ann", "a"); write(new File(e.in(), "_app/builds.properties"), "Ann#2=g,1,auto\n");
        e.builds.fail = true;
        List<SaveTransfer.Result> r = e.run("1");
        check(r.get(0).kind == SaveTransfer.FAILED && SaveTransfer.unfinished(e.save) && e.in().isDirectory(), "not stored: unfinished, save-import stays");
        e.builds.fail = false;
        r = e.run("2");
        check(r.get(0).resumed && "g,1,auto".equals(e.builds.p.getProperty("Ann#2")) && !SaveTransfer.unfinished(e.save), "the next start stores them");
    }


    static void oddImports(File base) throws IOException {
        Env e = new Env(base, "vault-only"); write(new File(e.in(), "x/" + SC + ".swvault"), vault("v"));
        List<SaveTransfer.Result> r = e.run("1");
        check(r.get(0).kind == SaveTransfer.IMPORTED && Arrays.equals(bytes(new File(e.save, SC + ".swvault")), vault("v")), "a vault alone is imported (also one level down)");
        Env t = new Env(base, "tab-name"); character(t.in(), "Ann", "a"); write(new File(t.in(), "_app/stash_backup/bad\tname.d2i"), "x");
        r = t.run("1");
        check(r.get(0).kind == SaveTransfer.IMPORT_REJECTED && r.get(0).bad == SaveTransfer.BAD_FILE && !SaveTransfer.unfinished(t.save) && !new File(t.save, "Ann").exists(), "a tab in a name rejects the import");
    }


    static void ownData(File base) throws IOException {
        Env e = new Env(base, "own");
        write(new File(e.files, "sw_custom.txt"), "mine\n"); write(new File(e.files, "stash_backup/a.d2i"), "phone a");
        e.builds.p.setProperty("Zed#1", "phone"); e.builds.p.setProperty("张三#3", "guide,2,auto");
        File app = new File(e.in(), "_app"); character(e.in(), "Ann", "a");
        write(new File(app, "sw_custom.txt"), "theirs\n"); write(new File(app, "stash_backup/a.d2i"), "their a"); write(new File(app, "stash_backup/b.d2i"), "their b");
        write(new File(app, "builds.properties"), "Zed#1=theirs\nAnn#2=g,1,auto\n");
        List<SaveTransfer.Result> r = e.run("1");
        check(r.get(0).own == (SaveTransfer.OWN_BUILDS | SaveTransfer.OWN_BACKUPS), "backups and builds taken, settings kept");
        check(read(new File(e.files, "sw_custom.txt")).equals("mine\n") && read(new File(e.files, "stash_backup/a.d2i")).equals("phone a") && read(new File(e.files, "stash_backup/b.d2i")).equals("their b"), "only missing files");
        check("phone".equals(e.builds.p.getProperty("Zed#1")) && "g,1,auto".equals(e.builds.p.getProperty("Ann#2")), "only missing build keys");
        Env f = new Env(base, "own-fresh"); copyDir(e.game, f.game); copyDir(new File(f.game, "save-import.done-1"), f.in());
        r = f.run("1");
        check(r.get(0).own == (SaveTransfer.OWN_CUSTOM | SaveTransfer.OWN_BUILDS | SaveTransfer.OWN_BACKUPS) && read(new File(f.files, "sw_custom.txt")).equals("theirs\n"), "a fresh phone takes everything");

        write(new File(e.game, "export"), ""); e.run("2");
        Env g = new Env(base, "own-script"); copyDir(new File(e.game, "save-export/2"), g.in()); g.run("1");
        check("guide,2,auto".equals(g.builds.p.getProperty("张三#3")) && "phone".equals(g.builds.p.getProperty("Zed#1")), "a Chinese name keeps its guide");
    }


    static void settingsAndElysium(File base) throws IOException {
        { Env e = new Env(base, "settings-only"); character(e.save, "yyc", "keep");
          write(new File(e.in(), "Settings/Settings.json"), "backup");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && "Settings".equals(r.get(0).path) && r.get(0).skipped == null
                && read(new File(e.save, "Settings/Settings.json")).equals("backup") && new File(e.game, "save-import.done-20261101-120000").isDirectory(),
                "a settings-only folder is imported while the phone has no settings"); }
        { Env e = new Env(base, "settings-only-have"); write(new File(e.save, "Settings/Settings.json"), "phone");
          write(new File(e.in(), "Settings/Settings.json"), "backup");
          List<SaveTransfer.Result> r = e.run("20261101-120000");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && r.get(0).path == null && "Settings".equals(r.get(0).skipped)
                && read(new File(e.save, "Settings/Settings.json")).equals("phone") && new File(e.game, "save-import.done-20261101-120000").isDirectory(),
                "a settings-only folder is skipped, not rejected, when the phone has settings"); }
        { Env e = new Env(base, "settings-only-nested"); write(new File(e.in(), "20261031-090000/Settings/Settings.json"), "nested");
          List<SaveTransfer.Result> r = e.run("1");
          check(r.size() == 1 && r.get(0).kind == SaveTransfer.IMPORTED && read(new File(e.save, "Settings/Settings.json")).equals("nested"), "settings-only at the <stamp>/ level of an export"); }
        { Env e = new Env(base, "ely-stale"); character(e.save, "Zed", "z"); write(new File(e.save, "sw_elysium.txt"), "Ann\nZed\n");
          character(e.in(), "Ann", "normal"); character(e.in(), "Bea", "elysium"); write(new File(e.in(), "sw_elysium.txt"), "Bea\n");
          e.run("1");
          check(read(new File(e.save, "sw_elysium.txt")).equals("Zed\nBea\n"), "the stale marker of an earlier Ann goes, Bea's comes, Zed's stays"); }
        { Env e = new Env(base, "ely-last"); write(new File(e.save, "sw_elysium.txt"), "Ann\n"); character(e.in(), "Ann", "normal");
          e.run("1");
          check(!new File(e.save, "sw_elysium.txt").exists() && new File(e.save, "Ann/Ann.d2s").isFile(), "a list left empty is deleted"); }
        { Env e = new Env(base, "ely-skipped"); character(e.save, "Ann", "phone"); write(new File(e.save, "sw_elysium.txt"), "Ann\n");
          character(e.in(), "Ann", "backup");
          e.run("1");
          check(read(new File(e.save, "sw_elysium.txt")).equals("Ann\n"), "a skipped character keeps the phone's marker"); }
        { Env e = new Env(base, "ely-crash"); write(new File(e.save, "sw_elysium.txt"), "Ann\nZed\n"); character(e.in(), "Ann", "normal");
          System.setProperty("dm.transfer.kill", "journal");
          boolean died = false;
          try { e.run("1"); } catch (SaveTransfer.Killed k) { died = true; }
          System.clearProperty("dm.transfer.kill");
          check(died && read(new File(e.save, "sw_elysium.txt")).equals("Ann\nZed\n") && SaveTransfer.unfinished(e.save), "died with the journal written, the list untouched");
          List<SaveTransfer.Result> r = e.run("2");
          check(r.size() == 1 && r.get(0).resumed && read(new File(e.save, "sw_elysium.txt")).equals("Zed\n") && !SaveTransfer.unfinished(e.save),
                "the next start finishes the import and drops the stale marker"); }
    }
}
