package com.xiaoboluo.d2r;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.StringReader;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Properties;
import java.util.Set;
import java.util.TreeMap;
import java.util.UUID;
























final class SaveTransfer {
    private SaveTransfer() {}

    static final int EXPORTED = 1, EXPORT_EMPTY = 2, IMPORTED = 3, IMPORT_REJECTED = 4, FAILED = 5;
    static final String EXPORT_MARKER = "export", EXPORT_DIR = "save-export", IMPORT_DIR = "save-import", INBOX = "_cloudinbox", APP_DIR = "_app";
    static final String SOURCE_FILE = "export-source.txt", ID_FILE = "save-transfer-id", STATE_DIR = "save-transfer";
    static final String CUSTOM = "sw_custom.txt", BACKUPS = "stash_backup", BUILDS = "builds.properties", LEDGER = "ledger", ELYSIUM = "sw_elysium.txt";


    static final String PAID = "paid ", NOGOLD = "nogold-";

    static final String LOOKS = "Looks.swlooks", LOOKS_IN = "Looks.import-";
    static final int MERGE_NONE = 0, MERGE_QUEUED = 1, MERGE_ALREADY = 2, MERGE_SAME_PHONE = 3;
    static final int OWN_CUSTOM = 1, OWN_BUILDS = 2, OWN_BACKUPS = 4;
    static final int BAD_NONE = 0, BAD_SIZE = 1, BAD_COUNT = 2, BAD_FILE = 3, BAD_NOTHING = 4;
    static final long MAX_FILE = 16L << 20;
    static final long MAX_PACK = 64L << 20;
    static final int MAX_FILES = 4096, MAX_PACK_FILES = 64;





    static final class Result {
        final int kind;
        String path, skipped, detail;
        int merge, own, bad, held;
        boolean resumed;
        Result(int kind, String path) { this.kind = kind; this.path = path; }
    }


    interface Builds {
        Properties all();
        int addMissing(Properties p) throws IOException;
    }

    static List<Result> run(File game, File save, String stamp) { return run(game, save, stamp, null); }


    static List<Result> run(File game, File save, String stamp, Builds builds) {
        List<Result> out = new ArrayList<>();
        File state = stateDir(save);
        try {
            Result r = resume(game, save, builds);
            if (r != null) out.add(r);
        } catch (IOException e) { out.add(new Result(FAILED, reason(e))); return out; }
        File marker = new File(game, EXPORT_MARKER);
        if (marker.exists()) {
            try {
                if (!hasFiles(save)) out.add(new Result(EXPORT_EMPTY, null));
                else out.add(new Result(EXPORTED, exportTo(game, save, stamp, builds)));
                deleteTree(marker);
            } catch (IOException e) { out.add(new Result(FAILED, reason(e))); }
        }
        File in = new File(game, IMPORT_DIR);
        if (in.isDirectory()) {
            try { out.add(importFrom(game, in, save, stamp, builds)); }
            catch (IOException e) {

                try { if (!new File(state, "journal").isFile()) deleteTree(new File(state, "stage")); } catch (IOException ignored) { }
                out.add(new Result(FAILED, reason(e)));
            }
        }
        return out;
    }


    static boolean unfinished(File save) { return new File(stateDir(save), "journal").isFile(); }

    static File stateDir(File save) { return new File(save.getAbsoluteFile().getParentFile(), STATE_DIR); }
    private static File filesDir(File save) { return save.getAbsoluteFile().getParentFile(); }



    private static String exportTo(File game, File save, String stamp, Builds builds) throws IOException {
        File target = unused(new File(new File(game, EXPORT_DIR), stamp)), part = new File(target.getPath() + ".part");
        File state = stateDir(save), pending = new File(state, "export");
        mkdirs(state);
        writeAtomic(pending, (part.getAbsolutePath() + "\n").getBytes(StandardCharsets.UTF_8));
        deleteTree(part);
        try {
            copyTree(save, part, new byte[1 << 16]);
            Files.write(new File(part, SOURCE_FILE).toPath(), (installId(save) + "\n").getBytes(StandardCharsets.US_ASCII));
            File app = new File(part, APP_DIR), files = filesDir(save);
            mkdirs(app);
            if (new File(files, CUSTOM).isFile()) copyTree(new File(files, CUSTOM), new File(app, CUSTOM), new byte[1 << 16]);
            if (new File(files, BACKUPS).isDirectory()) copyTree(new File(files, BACKUPS), new File(app, BACKUPS), new byte[1 << 16]);
            File ledger = new File(state, LEDGER);
            if (ledger.isFile()) copyTree(ledger, new File(app, LEDGER), new byte[1 << 16]);
            Properties p = builds == null ? null : builds.all();
            if (p != null && !p.isEmpty()) Files.write(new File(app, BUILDS).toPath(), props(p));
            mkdirs(target.getParentFile());
            if (!part.renameTo(target)) throw new IOException("rename " + target.getName());
        } catch (IOException e) { deleteTree(part); deleteTree(pending); throw e; }
        deleteTree(pending);
        return EXPORT_DIR + "/" + target.getName();
    }



    private static final class Plan {
        final List<String[]> moves = new ArrayList<>();
        final Set<String> ledger = new LinkedHashSet<>();
        final List<String> elysium = new ArrayList<>();
        final StringBuilder added = new StringBuilder(), skipped = new StringBuilder();
        final Set<String> nogold = new LinkedHashSet<>();
        boolean queued, already, samePhone, builds, charSkipped;
        int own, files, held;
        long bytes;
    }

    private static Result importFrom(File game, File in, File save, String stamp, Builds builds) throws IOException {
        File root = savesRoot(in);
        if (root == null) return reject(game, in, stamp, BAD_NOTHING, null);
        File files = filesDir(save), state = stateDir(save), stage = new File(state, "stage");
        Result bad = check(root);
        if (bad != null) return reject(game, in, stamp, bad.bad, bad.detail);
        mkdirs(save);
        mkdirs(state);
        deleteTree(stage);
        Set<String> ledger = readLedger(new File(state, LEDGER));
        boolean samePhone = fromThisPhone(root, save);
        Plan plan = new Plan();
        File[] list = sorted(root.listFiles());

        boolean pageFiles = hasPageFiles(save);
        List<File> packFiles = new ArrayList<>(); List<String> packRels = new ArrayList<>();
        byte[] buffer = new byte[1 << 16];
        for (File c : list) {
            String name = c.getName();
            if (c.isDirectory() && isCharacter(c)) {
                if (has(save, name)) { append(plan.skipped, name); plan.charSkipped = true; continue; }
                stageCopy(c, stage, "save/" + name, plan, buffer);
                append(plan.added, name);
                if (listed(new File(root, ELYSIUM), name)) plan.elysium.add(name);
            } else if (c.isFile() && name.equals(LOOKS)) {


                String rel = "save/" + LOOKS_IN + System.currentTimeMillis() + ".swlooks";
                File to = new File(stage, rel); mkdirs(to.getParentFile());
                Files.write(to.toPath(), looksForImport(Files.readAllBytes(c.toPath()), save));
                plan.moves.add(new String[] { rel, rel });
            } else if (c.isDirectory() && name.equals("Settings")) {
                if (has(save, name)) { append(plan.skipped, name); continue; }
                stageCopy(c, stage, "save/" + name, plan, buffer);
                append(plan.added, name);
            } else if (stashName(c) != null) {
                String key = "stash " + name + " " + digestOf(c);

                if (ledger.contains(key)) plan.already = true;

                else if (ledger.contains(PAID + key) && !samePhone) { collect(c, name, packFiles, packRels); plan.ledger.add(key); plan.nogold.add(stashName(c)); }
                else if (!has(save, name)) {
                    stageCopy(c, stage, "save/" + name, plan, buffer);
                    plan.ledger.add(key);
                    append(plan.added, name);
                }
                else if (samePhone) plan.samePhone = true;
                else { collect(c, name, packFiles, packRels); plan.ledger.add(key); }
            }
        }
        for (File c : list) {
            String stash = vaultStash(c);
            if (stash == null) continue;
            String key = "vault " + stash + " " + digestOf(c);
            boolean clean = !has(save, c.getName()) && !has(save, stash) && !has(save, stash + ".d2i") && !pageFiles;
            if (ledger.contains(key)) plan.already = true;
            else if (clean) { stageCopy(c, stage, "save/" + c.getName(), plan, buffer); plan.ledger.add(key); append(plan.added, c.getName()); }
            else if (samePhone) plan.samePhone = true;
            else { packFiles.add(c); packRels.add(c.getName()); plan.ledger.add(key); }
        }
        if (!packFiles.isEmpty()) {
            if (packFiles.size() > MAX_PACK_FILES) { deleteTree(stage); return reject(game, in, stamp, BAD_COUNT, null); }
            long size = 24;
            for (int i = 0; i < packFiles.size(); i++) size += 14 + packRels.get(i).getBytes(StandardCharsets.UTF_8).length + packFiles.get(i).length();
            if (size > MAX_PACK) { deleteTree(stage); return reject(game, in, stamp, BAD_SIZE, packRels.get(0)); }
            String entry = INBOX + "/" + uniqueEntry(save, stage, System.currentTimeMillis());
            String digest = writePack(packFiles, packRels, new File(stage, "save/" + entry));
            Files.write(new File(stage, "save/" + entry + "/import.sha256").toPath(), (digest + "\n").getBytes(StandardCharsets.US_ASCII));
            for (String n : plan.nogold) Files.write(new File(stage, "save/" + entry + "/" + NOGOLD + n).toPath(), new byte[0]);
            plan.moves.add(new String[] { "save/" + entry, "save/" + entry });
            plan.ledger.add("inbox " + hex(sha256(Files.readAllBytes(new File(stage, "save/" + entry + "/mine.pack").toPath()))));
            plan.queued = true;
        }
        if (!samePhone) takeOverInbox(root, save, stage, ledger, readLedger(new File(root, APP_DIR + "/" + LEDGER)), plan, buffer);
        ownData(root, files, stage, builds, plan, buffer);
        inheritLedger(new File(root, APP_DIR + "/" + LEDGER), ledger, plan);
        Result r = new Result(IMPORTED, plan.added.length() == 0 ? null : plan.added.toString());
        r.skipped = plan.skipped.length() == 0 ? null : plan.skipped.toString();
        r.merge = plan.queued ? MERGE_QUEUED : plan.already ? MERGE_ALREADY : plan.samePhone ? MERGE_SAME_PHONE : MERGE_NONE;
        r.own = plan.own;
        r.held = plan.held;
        File done = unused(new File(game, IMPORT_DIR + ".done-" + stamp));
        writeJournal(state, plan, in, done, r);
        killAt("journal");
        return replay(game, save, builds, false);
    }





    private static void inheritLedger(File f, Set<String> ledger, Plan plan) throws IOException {
        if (!f.isFile()) return;
        for (String line : readLedger(f)) {
            if (ledger.contains(line) || plan.ledger.contains(line) || plan.ledger.contains(PAID + line)) continue;
            if (!plan.charSkipped || line.startsWith(PAID)) { plan.ledger.add(line); continue; }
            plan.held++;
            if (line.startsWith("stash ") && !ledger.contains(PAID + line)) plan.ledger.add(PAID + line);
        }
    }

    private static Result reject(File game, File in, String stamp, int why, String detail) throws IOException {
        File rejected = unused(new File(game, IMPORT_DIR + ".rejected-" + stamp));
        if (!in.renameTo(rejected)) throw new IOException("rename " + IMPORT_DIR);
        Result r = new Result(IMPORT_REJECTED, rejected.getName());
        r.bad = why; r.detail = detail;
        return r;
    }


    private static Result check(File root) throws IOException {
        List<File> all = new ArrayList<>(); List<String> rels = new ArrayList<>();
        File[] list = sorted(root.listFiles());
        for (File c : list) {
            String n = c.getName();
            if ((c.isDirectory() && (isCharacter(c) || n.equals("Settings") || n.equals(INBOX) || n.equals(APP_DIR))) || stashName(c) != null || vaultStash(c) != null
                || (c.isFile() && n.equals(LOOKS))) {
                collect(c, n, all, rels);
                if (all.size() > MAX_FILES) return bad(BAD_COUNT, null);
            }
        }


        for (String rel : rels) if (rel.indexOf('\t') >= 0 || rel.indexOf('\n') >= 0 || rel.indexOf('\r') >= 0) return bad(BAD_FILE, rel.replaceAll("[\t\r\n]", "?"));
        for (int i = 0; i < all.size(); i++) {
            boolean pack = rels.get(i).matches(INBOX + "/[^/]+/(mine|base)\\.pack");
            if (all.get(i).length() > (pack ? MAX_PACK : MAX_FILE)) return bad(BAD_SIZE, rels.get(i));
        }
        for (File c : list) {
            if (c.isDirectory() && isCharacter(c)) { File f = new File(c, c.getName() + ".d2s"); if (!validD2s(f)) return bad(BAD_FILE, c.getName() + "/" + f.getName()); }
            String stash = stashName(c);
            if (stash != null) for (File f : stashFiles(c)) if (!validD2i(f)) return bad(BAD_FILE, c.isDirectory() ? c.getName() + "/" + f.getName() : f.getName());
            if (vaultStash(c) != null && !validVault(c)) return bad(BAD_FILE, c.getName());
            if (c.isFile() && c.getName().equals(LOOKS) && !validLooks(Files.readAllBytes(c.toPath()))) return bad(BAD_FILE, c.getName());
        }
        for (File e : sorted(new File(root, INBOX).listFiles())) {
            if (!e.isDirectory() || e.getName().endsWith(".part")) continue;
            for (String pk : new String[] { "mine.pack", "base.pack" }) {
                File f = new File(e, pk);
                if (!f.isFile()) continue;
                List<Object[]> in = validPack(Files.readAllBytes(f.toPath()));
                if (in == null) return bad(BAD_FILE, INBOX + "/" + e.getName() + "/" + pk);
                for (Object[] x : in) {
                    String rel = (String) x[0]; byte[] d = (byte[]) x[1];
                    if ((rel.endsWith(".d2i") && !validD2i(d)) || (rel.endsWith(".swvault") && !validVault(d))) return bad(BAD_FILE, INBOX + "/" + e.getName() + "/" + pk + ":" + rel);
                }
            }
            for (File f : sorted(e.listFiles()))
                if (f.getName().startsWith("plan-") && f.getName().endsWith(".bin") && !validPlan(Files.readAllBytes(f.toPath()))) return bad(BAD_FILE, INBOX + "/" + e.getName() + "/" + f.getName());
        }
        return null;
    }
    private static Result bad(int why, String detail) { Result r = new Result(IMPORT_REJECTED, null); r.bad = why; r.detail = detail; return r; }












    private static void takeOverInbox(File root, File save, File stage, Set<String> ledger, Set<String> source, Plan plan, byte[] buffer) throws IOException {
        File[] entries = sorted(new File(root, INBOX).listFiles());
        long t = System.currentTimeMillis();
        for (File e : entries) {
            File mine = new File(e, "mine.pack");
            if (!e.isDirectory() || e.getName().endsWith(".part") || !mine.isFile()) continue;
            byte[] pack = Files.readAllBytes(mine.toPath());
            String key = "inbox " + hex(sha256(pack));
            if (ledger.contains(key) || plan.ledger.contains(key) || !openEntry(e)) continue;
            List<Object[]> files = packFiles(pack), keep = new ArrayList<>();
            java.util.Map<String, Integer> state = new java.util.HashMap<>();
            for (String c : packComponents(pack)) state.put(c.substring(0, c.lastIndexOf(' ')), carriedState(ledger, plan, c, e));
            java.util.Map<String, String> stashOf = new java.util.TreeMap<>();
            for (Object[] f : files) { String rel = (String) f[0], n = packStash(rel); if (n != null && !rel.endsWith(".swvault")) stashOf.put(n, componentOf(rel)); }
            Set<String> names = new java.util.TreeSet<>(stashOf.keySet());
            for (Object[] f : files) { String n = packStash((String) f[0]); if (n != null) names.add(n); }
            java.util.Map<String, byte[]> plans = new java.util.HashMap<>();
            Set<String> out = new LinkedHashSet<>(), nogold = new LinkedHashSet<>();
            for (String n : names) {
                Integer st = stashOf.containsKey(n) ? state.get(stashOf.get(n)) : null, vt = state.get("vault " + n);
                boolean touched = (st != null && st != NEW) || (vt != null && vt != NEW);
                File pf = new File(e, "plan-" + n + ".bin");
                if (!pf.isFile()) { if (st != null && st == PAID_ONLY) nogold.add(n); continue; }
                byte[] pd = Files.readAllBytes(pf.toPath());
                if (!touched) { plans.put(n, pd); continue; }
                List<byte[]> records = new ArrayList<>();
                for (Object[] f : files) if (((String) f[0]).equals(n + ".swvault")) records = vaultItems((byte[]) f[1]);
                boolean keepStash = st != null && st != KNOWN, keepVault = vt != null && vt != KNOWN;
                List<byte[]> items = planItems(pd), owed = new ArrayList<>();
                java.util.Map<String, Integer> room = new java.util.HashMap<>();
                for (byte[] r : records) room.merge(keyString(r), 1, Integer::sum);
                for (byte[] it : items) {
                    if (st == null) { if (keepVault) owed.add(it); continue; }
                    String k = keyString(it); int v = room.getOrDefault(k, 0);
                    if (!simpleItem(it)) { if (v > 0 ? keepVault : keepStash) owed.add(it); continue; }
                    if (keepStash) owed.add(it);
                    else if (keepVault && v > 0) { owed.add(it); room.put(k, v - 1); }
                }
                long gold = st != null && st == NEW ? ByteBuffer.wrap(pd).order(ByteOrder.LITTLE_ENDIAN).getLong(8) : 0;
                if (owed.isEmpty() && gold <= 0) out.add(n);
                else plans.put(n, planOf(owed, gold));
            }
            for (Object[] f : files) {
                String rel = (String) f[0], name = packStash(rel);
                if (name == null || out.contains(name)) continue;
                Integer s = state.get(componentOf(rel));
                if (s == null || s != KNOWN) keep.add(f);
            }



            boolean held = false;
            for (String c : packComponents(pack)) {
                if (carriedState(ledger, plan, c, e) == KNOWN) { held |= known(ledger, plan, c) != KNOWN; continue; }
                String ck = carriedKey(c, e);
                if (ck != null) plan.ledger.add(ck);
                if (ck == null || !(plan.charSkipped || heldAt(source, c))) { plan.ledger.add(c); continue; }
                held = true; plan.held++;
                if (c.startsWith("stash ")) plan.ledger.add(PAID + c);
            }
            if (!held) plan.ledger.add(key);
            if (keep.isEmpty()) continue;
            byte[] kept = keep.size() == files.size() ? pack : packOf(keep);
            String entry = INBOX + "/" + uniqueEntry(save, stage, ++t);
            File dir = new File(stage, "save/" + entry);
            for (File x : sorted(e.listFiles())) {
                String n = x.getName();
                if (n.equals("mine.pack") || (n.startsWith("plan-") && n.endsWith(".bin") && names.contains(n.substring(5, n.length() - 4)))) continue;
                if (n.startsWith(NOGOLD) && out.contains(n.substring(NOGOLD.length()))) continue;
                copyTree(x, new File(dir, n), buffer);
            }
            mkdirs(dir);
            Files.write(new File(dir, "mine.pack").toPath(), kept);
            for (java.util.Map.Entry<String, byte[]> p : plans.entrySet()) if (!out.contains(p.getKey())) Files.write(new File(dir, "plan-" + p.getKey() + ".bin").toPath(), p.getValue());
            for (String n : nogold) Files.write(new File(dir, NOGOLD + n).toPath(), new byte[0]);
            plan.moves.add(new String[] { "save/" + entry, "save/" + entry });
            plan.queued = true;
        }
    }

    private static final int NEW = 0, KNOWN = 1, PAID_ONLY = 2;

    private static int known(Set<String> ledger, Plan plan, String component) {
        if (ledger.contains(component) || plan.ledger.contains(component)) return KNOWN;
        return ledger.contains(PAID + component) || plan.ledger.contains(PAID + component) ? PAID_ONLY : NEW;
    }


    private static String carriedKey(String component, File e) throws IOException {
        String what = component.substring(component.indexOf(' ') + 1, component.lastIndexOf(' ')), n = what.endsWith(".d2i") ? what.substring(0, what.length() - 4) : what;
        File pf = new File(e, "plan-" + n + ".bin");
        return pf.isFile() ? "carried " + component + " " + hex(sha256(Files.readAllBytes(pf.toPath()))) : null;
    }
    private static boolean heldAt(Set<String> source, String component) {
        if (source.contains(component)) return false;
        if (source.contains(PAID + component)) return true;
        for (String l : source) if (l.startsWith("carried " + component + " ")) return true;
        return false;
    }
    private static int carriedState(Set<String> ledger, Plan plan, String component, File e) throws IOException {
        String ck = carriedKey(component, e);
        return ck != null && (ledger.contains(ck) || plan.ledger.contains(ck)) ? KNOWN : known(ledger, plan, component);
    }


    static List<byte[]> planItems(byte[] d) {
        List<byte[]> out = new ArrayList<>();
        ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
        int n = b.getShort(6) & 0xffff, o = 16;
        for (int i = 0; i < n; i++) { int len = b.getShort(o) & 0xffff; out.add(Arrays.copyOfRange(d, o + 2, o + 2 + len)); o += 2 + len; }
        return out;
    }
    static byte[] planOf(List<byte[]> items, long gold) {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        b.write('S'); b.write('W'); b.write('C'); b.write('M'); b.write(1); b.write(0); le(b, items.size(), 2); le(b, gold, 8);
        for (byte[] it : items) { le(b, it.length, 2); b.write(it, 0, it.length); }
        return b.toByteArray();
    }

    static List<byte[]> vaultItems(byte[] d) {
        List<byte[]> out = new ArrayList<>();
        if (!validVault(d)) return out;
        ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
        int n = b.getShort(6) & 0xffff, o = 8;
        for (int i = 0; i < n; i++) { int len = b.getShort(o + 4) & 0xffff; out.add(Arrays.copyOfRange(d, o + 6, o + 6 + len)); o += 6 + len; }
        return out;
    }

    static byte[] itemKey(byte[] bits) {
        byte[] k = bits.clone();
        if (k.length >= 7) { k[1] &= (byte) ~0x20; k[4] &= 0x07; k[5] = 0; k[6] &= (byte) ~0x1f; }
        return k;
    }
    private static String keyString(byte[] item) { return hex(itemKey(item)); }

    static boolean simpleItem(byte[] bits) { return bits.length >= 4 && (bits[2] & 0x20) != 0; }


    static boolean validPlan(byte[] d) {
        if (d.length < 16 || d[0] != 'S' || d[1] != 'W' || d[2] != 'C' || d[3] != 'M' || d[4] != 1) return false;
        ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
        int n = b.getShort(6) & 0xffff, o = 16;
        for (int i = 0; i < n; i++) {
            if (o + 2 > d.length) return false;
            int len = b.getShort(o) & 0xffff;
            if (len == 0 || len > 1024 || o + 2 + len > d.length) return false;
            o += 2 + len;
        }
        return o == d.length;
    }


    static boolean openEntry(File e) throws IOException {
        Set<String> names = new LinkedHashSet<>();
        for (String pack : new String[] { "mine.pack", "base.pack" }) {
            File p = new File(e, pack);
            if (p.isFile()) for (String rel : packPaths(Files.readAllBytes(p.toPath()))) {
                String n = rel.endsWith(".swvault") ? rel.substring(0, rel.length() - 8) : rel.endsWith(".d2i") ? rel.substring(0, rel.indexOf('/') > 0 ? rel.indexOf('/') : rel.length() - 4) : null;
                if (n != null && pack.equals("mine.pack")) names.add(n);
            }
        }
        for (String n : names) {
            File plan = new File(e, "plan-" + n + ".bin");
            if (!plan.isFile()) return true;
            byte[] d = Files.readAllBytes(plan.toPath());
            if (d.length < 16 || d[0] != 'S' || d[1] != 'W' || d[2] != 'C' || d[3] != 'M') return true;
            ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
            if ((b.getShort(6) & 0xffff) > 0 || b.getLong(8) > 0) return true;
        }
        return false;
    }

    private static void ownData(File root, File files, File stage, Builds builds, Plan plan, byte[] buffer) throws IOException {
        File app = new File(root, APP_DIR);
        if (!app.isDirectory()) return;
        File custom = new File(app, CUSTOM);
        if (custom.isFile() && !new File(files, CUSTOM).exists()) { stageCopy(custom, stage, CUSTOM, plan, buffer); plan.own |= OWN_CUSTOM; }
        for (File f : sorted(new File(app, BACKUPS).listFiles())) {
            if (!f.isFile() || new File(files, BACKUPS + "/" + f.getName()).exists()) continue;
            stageCopy(f, stage, BACKUPS + "/" + f.getName(), plan, buffer);
            plan.own |= OWN_BACKUPS;
        }
        File b = new File(app, BUILDS);
        if (b.isFile() && builds != null) {
            Properties have = builds.all(), want = new Properties();
            want.load(new StringReader(new String(Files.readAllBytes(b.toPath()), StandardCharsets.ISO_8859_1)));
            boolean any = false;
            for (String k : want.stringPropertyNames()) any |= have == null || !have.containsKey(k);
            if (any) { copyTree(b, new File(stage, BUILDS), buffer); plan.builds = true; plan.own |= OWN_BUILDS; }
        }
    }

    private static void stageCopy(File from, File stage, String rel, Plan plan, byte[] buffer) throws IOException {
        copyTree(from, new File(stage, rel), buffer);
        plan.moves.add(new String[] { rel, rel });
    }



    private static void writeJournal(File state, Plan plan, File in, File done, Result r) throws IOException {
        StringBuilder j = new StringBuilder("SWXJ1\n");
        for (String[] m : plan.moves) j.append("move\t").append(m[0]).append('\t').append(m[1]).append('\n');
        for (String n : plan.elysium) j.append("elysium\t").append(n).append('\n');
        for (String l : plan.ledger) j.append("ledger\t").append(l).append('\n');
        if (plan.builds) j.append("builds\n");
        j.append("rename\t").append(in.getAbsolutePath()).append('\t').append(done.getAbsolutePath()).append('\n');
        j.append("result\t").append(r.kind).append('\t').append(r.merge).append('\t').append(r.own).append('\t')
            .append(r.path == null ? "" : r.path).append('\t').append(r.skipped == null ? "" : r.skipped).append('\t').append(r.held).append('\n');
        j.append("end\n");
        writeAtomic(new File(state, "journal"), j.toString().getBytes(StandardCharsets.UTF_8));
    }


    private static Result resume(File game, File save, Builds builds) throws IOException {
        File state = stateDir(save);
        if (!state.isDirectory()) return null;
        deleteTree(new File(state, "journal.tmp"));
        File pending = new File(state, "export");
        if (pending.isFile()) {
            String part = new String(Files.readAllBytes(pending.toPath()), StandardCharsets.UTF_8).trim();
            if (part.endsWith(".part")) deleteTree(new File(part));
            deleteTree(pending);
        }
        if (new File(state, "journal").isFile()) return replay(game, save, builds, true);
        deleteTree(new File(state, "stage"));
        return null;
    }

    private static Result replay(File game, File save, Builds builds, boolean resumed) throws IOException {
        File files = filesDir(save), state = stateDir(save), stage = new File(state, "stage"), journal = new File(state, "journal");
        String[] lines = new String(Files.readAllBytes(journal.toPath()), StandardCharsets.UTF_8).split("\n");
        if (lines.length < 2 || !lines[0].equals("SWXJ1") || !lines[lines.length - 1].equals("end")) {
            deleteTree(journal); deleteTree(stage);
            throw new IOException("journal");
        }
        Result r = null;
        List<String> ely = new ArrayList<>(), led = new ArrayList<>();
        int step = 0;
        for (String line : lines) {
            String[] f = line.split("\t", -1);
            switch (f[0]) {
                case "move": {
                    File from = new File(stage, f[1]), to = new File(files, f[2]);
                    if (from.exists() && !to.exists()) {
                        mkdirs(to.getParentFile());
                        if (!from.renameTo(to)) throw new IOException("rename " + to.getName());
                    } else if (!to.exists()) throw new IOException("missing " + f[1]);
                    killAt("move" + (++step));
                    break;
                }
                case "elysium": ely.add(f[1]); break;
                case "ledger": led.add(f[1]); break;
                case "builds": {
                    File b = new File(stage, BUILDS);
                    if (b.isFile() && builds != null) {
                        Properties p = new Properties();
                        p.load(new StringReader(new String(Files.readAllBytes(b.toPath()), StandardCharsets.ISO_8859_1)));
                        builds.addMissing(p);
                    }
                    break;
                }
                case "rename": {
                    addLines(new File(save, ELYSIUM), ely);
                    addLines(new File(state, LEDGER), led);
                    killAt("ledger");
                    File from = new File(f[1]), to = new File(f[2]);


                    if (from.isDirectory() && !to.exists()) from.renameTo(to);
                    break;
                }
                case "result": {
                    r = new Result(Integer.parseInt(f[1]), f[4].isEmpty() ? null : f[4]);
                    r.merge = Integer.parseInt(f[2]); r.own = Integer.parseInt(f[3]); r.skipped = f[5].isEmpty() ? null : f[5]; r.resumed = resumed;
                    r.held = f.length > 6 && !f[6].isEmpty() ? Integer.parseInt(f[6]) : 0;
                    break;
                }
                default: break;
            }
        }
        deleteTree(journal);
        deleteTree(stage);
        return r;
    }


    static final class Killed extends RuntimeException { Killed(String at) { super(at); } }
    private static void killAt(String at) { if (at.equals(System.getProperty("dm.transfer.kill"))) throw new Killed(at); }


    static byte[] packOf(List<Object[]> files) {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        b.write('S'); b.write('W'); b.write('P'); b.write('K'); b.write('1'); b.write('\n'); b.write(0); b.write(0);
        le(b, System.currentTimeMillis(), 8); le(b, 0, 4); le(b, files.size(), 4);
        for (Object[] f : files) {
            byte[] p = ((String) f[0]).getBytes(StandardCharsets.UTF_8), data = (byte[]) f[1];
            le(b, p.length, 2); b.write(p, 0, p.length); le(b, (Long) f[2], 8); le(b, data.length, 4); b.write(data, 0, data.length);
        }
        return b.toByteArray();
    }



    private static String writePack(List<File> files, List<String> rels, File dir) throws IOException {
        Integer[] order = new Integer[files.size()];
        for (int i = 0; i < order.length; i++) order[i] = i;
        Arrays.sort(order, (a, b) -> rels.get(a).compareTo(rels.get(b)));
        MessageDigest md = md();
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        long now = System.currentTimeMillis();
        b.write(new byte[] { 'S', 'W', 'P', 'K', '1', '\n', 0, 0 }); le(b, now, 8); le(b, 0, 4); le(b, order.length, 4);
        for (int i : order) {
            byte[] data = Files.readAllBytes(files.get(i).toPath()), p = rels.get(i).getBytes(StandardCharsets.UTF_8);
            if (p.length >= 256) throw new IOException("path " + rels.get(i));
            md.update(p); md.update((byte) 0); md.update(data); md.update((byte) 0);
            le(b, p.length, 2); b.write(p); le(b, files.get(i).lastModified(), 8); le(b, data.length, 4); b.write(data);
        }
        mkdirs(dir);
        try (OutputStream out = new FileOutputStream(new File(dir, "mine.pack"))) { b.writeTo(out); }
        return hex(md.digest());
    }

    static List<String> packPaths(byte[] d) {
        List<String> out = new ArrayList<>();
        for (Object[] f : packFiles(d)) out.add((String) f[0]);
        return out;
    }


    static String componentOf(String rel) {
        int slash = rel.indexOf('/');
        if (slash > 0) return "stash " + rel.substring(0, slash);
        if (rel.endsWith(".swvault")) return "vault " + rel.substring(0, rel.length() - 8);
        return "stash " + rel;
    }


    static Set<String> packComponents(byte[] d) throws IOException {
        TreeMap<String, TreeMap<String, byte[]>> comps = new TreeMap<>();
        for (Object[] f : packFiles(d)) {
            String rel = (String) f[0], key, sub;
            int slash = rel.indexOf('/');
            if (slash > 0) { key = "stash " + rel.substring(0, slash); sub = rel.substring(slash + 1); }
            else if (rel.endsWith(".swvault")) { key = "vault " + rel.substring(0, rel.length() - 8); sub = ""; }
            else if (rel.endsWith(".d2i")) { key = "stash " + rel; sub = ""; }
            else continue;
            comps.computeIfAbsent(key, k -> new TreeMap<>()).put(sub, (byte[]) f[1]);
        }
        Set<String> out = new LinkedHashSet<>();
        for (String key : comps.keySet()) {
            MessageDigest md = md();
            for (java.util.Map.Entry<String, byte[]> x : comps.get(key).entrySet()) { md.update(x.getKey().getBytes(StandardCharsets.UTF_8)); md.update((byte) 0); md.update(x.getValue()); md.update((byte) 0); }
            out.add(key + " " + hex(md.digest()));
        }
        return out;
    }


    static List<Object[]> packFiles(byte[] d) {
        List<Object[]> out = validPack(d);
        return out == null ? new ArrayList<>() : out;
    }



    static List<Object[]> validPack(byte[] d) {
        byte[] magic = { 'S', 'W', 'P', 'K', '1', '\n', 0, 0 };
        if (d.length < 24 || !Arrays.equals(Arrays.copyOf(d, 8), magic)) return null;
        ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
        long n = b.getInt(20) & 0xffffffffL;
        if (n > MAX_PACK_FILES) return null;
        b.position(24);
        List<Object[]> out = new ArrayList<>();
        for (int i = 0; i < n; i++) {
            if (b.remaining() < 2) return null;
            int pl = b.getShort() & 0xffff;
            if (pl >= 256 || b.remaining() < pl + 12) return null;
            byte[] p = new byte[pl]; b.get(p);
            long mt = b.getLong(), len = b.getInt() & 0xffffffffL;
            String rel = new String(p, StandardCharsets.UTF_8);
            if (len > MAX_FILE || b.remaining() < len || rel.contains("..") || rel.startsWith("/")) return null;
            byte[] data = new byte[(int) len]; b.get(data);
            out.add(new Object[] { rel, data, mt });
        }
        return out;
    }


    static String packStash(String rel) {
        int slash = rel.indexOf('/');
        if (slash > 0) return rel.substring(0, slash);
        if (rel.endsWith(".swvault")) return rel.substring(0, rel.length() - 8);
        if (rel.endsWith(".d2i")) return rel.substring(0, rel.length() - 4);
        return null;
    }


    private static String digestOf(File c) throws IOException {
        List<File> files = new ArrayList<>(); List<String> rels = new ArrayList<>();
        collect(c, "", files, rels);
        TreeMap<String, File> byRel = new TreeMap<>();
        for (int i = 0; i < files.size(); i++) byRel.put(rels.get(i), files.get(i));
        MessageDigest md = md();
        for (String rel : byRel.keySet()) { md.update(rel.getBytes(StandardCharsets.UTF_8)); md.update((byte) 0); md.update(Files.readAllBytes(byRel.get(rel).toPath())); md.update((byte) 0); }
        return hex(md.digest());
    }




    static boolean validD2s(File f) throws IOException {
        if (!f.isFile() || f.length() < 16) return false;
        ByteBuffer b = ByteBuffer.wrap(Files.readAllBytes(f.toPath())).order(ByteOrder.LITTLE_ENDIAN);
        return b.getInt(0) == 0xaa55aa55 && (b.getInt(8) & 0xffffffffL) == f.length();
    }


    static boolean validD2i(File f) throws IOException { return validD2i(Files.readAllBytes(f.toPath())); }
    static boolean validD2i(byte[] d) {
        ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
        int o = 0;
        while (o < d.length) {
            if (o + 68 > d.length || b.getInt(o) != 0xaa55aa55) return false;
            long ln = b.getInt(o + 16) & 0xffffffffL;
            if (ln < 68 || o + ln > d.length) return false;
            o += (int) ln;
        }
        return d.length > 0;
    }




    static final long LOOKS_SEED = 1469598103934665603L;
    static boolean validLooks(byte[] d) {
        if (d.length < 16 || d[0] != 'D' || d[1] != 'M' || d[2] != 'L' || d[3] != 'K' || d[4] < 1) return false;
        long h = LOOKS_SEED;
        for (int i = 0; i < d.length - 8; i++) { h ^= d[i] & 0xff; h *= 0x100000001b3L; }
        return ByteBuffer.wrap(d, d.length - 8, 8).order(ByteOrder.LITTLE_ENDIAN).getLong() == h;
    }







    static byte[] looksForImport(byte[] d, File save) throws IOException {
        if (!validLooks(d) || d[4] > 2) return d;
        int entry = d[4] > 1 ? 19 : 18, end = d.length - 8, map = -1, count = 0;
        ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
        for (int at = 8; at + 8 <= end; ) {
            int len = b.getInt(at + 4);
            if (len < 4 || len > end - at - 8) return d;
            if (b.getInt(at) == 0x2050414d) { map = at; count = b.getInt(at + 8); if (count < 0 || 4 + (long) count * entry != len) return d; }
            at += 8 + len;
        }
        if (map < 0 || count == 0) return d;
        int[] fp = new int[count], quality = new int[count];
        for (int i = 0; i < count; i++) { fp[i] = b.getInt(map + 12 + i * entry); quality[i] = d[map + 12 + i * entry + 8] & 0xff; }
        boolean[] local = new boolean[count];
        markLocal(save, fp, quality, local);
        int kept = 0; for (boolean l : local) if (!l) kept++;
        if (kept == count) return d;
        int cut = (count - kept) * entry;
        byte[] out = new byte[d.length - cut];
        System.arraycopy(d, 0, out, 0, map + 12);
        int o = map + 12;
        for (int i = 0; i < count; i++) if (!local[i]) { System.arraycopy(d, map + 12 + i * entry, out, o, entry); o += entry; }
        System.arraycopy(d, map + 12 + count * entry, out, o, end - (map + 12 + count * entry));
        ByteBuffer w = ByteBuffer.wrap(out).order(ByteOrder.LITTLE_ENDIAN);
        w.putInt(map + 4, 4 + kept * entry); w.putInt(map + 8, kept);
        long h = LOOKS_SEED;
        for (int i = 0; i < out.length - 8; i++) { h ^= out[i] & 0xff; h *= 0x100000001b3L; }
        w.putLong(out.length - 8, h);
        return out;
    }
    private static void markLocal(File dir, int[] fp, int[] quality, boolean[] local) throws IOException {
        boolean[] maybe = new boolean[1 << 16];
        for (int v : fp) maybe[v & 0xffff] = true;
        markLocal(dir, fp, quality, local, maybe);
    }
    private static void markLocal(File dir, int[] fp, int[] quality, boolean[] local, boolean[] maybe) throws IOException {
        File[] list = dir.listFiles();
        if (list == null) return;
        for (File f : list) {
            if (f.isDirectory()) { markLocal(f, fp, quality, local, maybe); continue; }
            if (!f.isFile() || f.getName().startsWith("Looks.")) continue;
            byte[] d = Files.readAllBytes(f.toPath());
            for (int i = 0; i < d.length; i++) {
                long w = 0;
                for (int k = 0; k < 8 && i + k < d.length; k++) w |= (long) (d[i + k] & 0xff) << (8 * k);
                for (int k = 0; k < 8 && (long) i * 8 + k + 43 <= (long) d.length * 8; k++) {
                    int v = (int) (w >>> k);
                    if (!maybe[v & 0xffff]) continue;
                    int q = (int) (w >>> (k + 39)) & 0xf;
                    for (int e = 0; e < fp.length; e++) if (fp[e] == v && quality[e] == q) local[e] = true;
                }
            }
        }
    }


    static boolean validVault(File f) throws IOException { return validVault(Files.readAllBytes(f.toPath())); }
    static boolean validVault(byte[] d) {
        if (d.length < 8 || d[0] != 'S' || d[1] != 'W' || d[2] != 'V' || d[3] != 'T' || d[4] != 1 || d[5] != 0) return false;
        ByteBuffer b = ByteBuffer.wrap(d).order(ByteOrder.LITTLE_ENDIAN);
        int n = b.getShort(6) & 0xffff, o = 8;
        for (int i = 0; i < n; i++) {
            if (o + 6 > d.length) return false;
            int len = b.getShort(o + 4) & 0xffff;
            if (len == 0 || len > 1024 || o + 6 + len > d.length) return false;
            o += 6 + len;
        }
        return o == d.length;
    }



    private static boolean isCharacter(File dir) { return new File(dir, dir.getName() + ".d2s").isFile(); }


    static String stashName(File c) {
        String n = c.getName();
        if (c.isDirectory() && n.startsWith("ModernSharedStash")) { for (File f : sorted(c.listFiles())) if (f.isFile() && f.getName().endsWith(".d2i")) return n; return null; }
        if (c.isFile() && n.startsWith("SharedStash") && n.endsWith(".d2i")) return n.substring(0, n.length() - 4);
        return null;
    }
    private static List<File> stashFiles(File c) {
        List<File> out = new ArrayList<>();
        if (c.isFile()) out.add(c);
        else for (File f : sorted(c.listFiles())) if (f.isFile() && f.getName().endsWith(".d2i")) out.add(f);
        return out;
    }

    static String vaultStash(File c) {
        String n = c.getName();
        return c.isFile() && n.endsWith(".swvault") && (n.startsWith("ModernSharedStash") || n.startsWith("SharedStash")) ? n.substring(0, n.length() - 8) : null;
    }


    private static boolean has(File dir, String name) {
        for (File f : sorted(dir.listFiles())) if (f.getName().equalsIgnoreCase(name)) return true;
        return false;
    }

    private static boolean hasPageFiles(File save) {
        for (File c : sorted(save.listFiles())) if (c.isDirectory() && isCharacter(c))
            for (File f : sorted(c.listFiles())) if (f.getName().endsWith(".swpage") || f.getName().endsWith(".swpouch")) return true;
        return false;
    }

    private static boolean listed(File list, String name) throws IOException {
        if (!list.isFile()) return false;
        for (String l : new String(Files.readAllBytes(list.toPath()), StandardCharsets.UTF_8).split("\n")) if (l.trim().equals(name)) return true;
        return false;
    }


    static File savesRoot(File dir) {
        if (looksLikeSaves(dir)) return dir;
        File only = null; int dirs = 0;
        for (File c : sorted(dir.listFiles())) if (c.isDirectory()) { dirs++; only = c; }
        return dirs == 1 && looksLikeSaves(only) ? only : null;
    }
    private static boolean looksLikeSaves(File dir) {
        for (File c : sorted(dir.listFiles())) if ((c.isDirectory() && isCharacter(c)) || stashName(c) != null || vaultStash(c) != null) return true;
        return false;
    }

    private static String uniqueEntry(File save, File stage, long millis) {
        for (long t = millis; ; t++) {
            String n = t + "-import";
            if (!new File(save, INBOX + "/" + n).exists() && !new File(stage, "save/" + INBOX + "/" + n).exists()) return n;
        }
    }



    private static File idFile(File save) { return new File(filesDir(save), ID_FILE); }

    private static String installId(File save) throws IOException {
        File f = idFile(save);
        if (f.isFile()) { String s = new String(Files.readAllBytes(f.toPath()), StandardCharsets.US_ASCII).trim(); if (!s.isEmpty()) return s; }
        String id = UUID.randomUUID().toString();
        writeAtomic(f, (id + "\n").getBytes(StandardCharsets.US_ASCII));
        return id;
    }


    private static boolean fromThisPhone(File root, File save) throws IOException {
        File src = new File(root, SOURCE_FILE), id = idFile(save);
        if (!src.isFile() || !id.isFile()) return false;
        String a = new String(Files.readAllBytes(src.toPath()), StandardCharsets.US_ASCII).trim(), b = new String(Files.readAllBytes(id.toPath()), StandardCharsets.US_ASCII).trim();
        return !a.isEmpty() && a.equals(b);
    }



    private static Set<String> readLedger(File f) throws IOException {
        Set<String> s = new LinkedHashSet<>();
        if (f.isFile()) for (String l : new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8).split("\n")) if (!l.trim().isEmpty()) s.add(l.trim());
        return s;
    }


    private static void addLines(File f, List<String> lines) throws IOException {
        Set<String> have = readLedger(f);
        StringBuilder add = new StringBuilder();
        for (String l : lines) if (have.add(l)) add.append(l).append('\n');
        if (add.length() == 0) return;
        String old = f.isFile() ? new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8) : "";
        if (!old.isEmpty() && !old.endsWith("\n")) old += "\n";
        mkdirs(f.getParentFile());
        writeAtomic(f, (old + add).getBytes(StandardCharsets.UTF_8));
    }


    private static byte[] props(Properties p) throws IOException {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        p.store(b, null);
        StringBuilder out = new StringBuilder();
        for (String l : new String(b.toByteArray(), StandardCharsets.ISO_8859_1).split("\n")) if (!l.startsWith("#")) out.append(l).append('\n');
        return out.toString().getBytes(StandardCharsets.ISO_8859_1);
    }

    private static void writeAtomic(File f, byte[] data) throws IOException {
        File tmp = new File(f.getPath() + ".tmp");
        try (FileOutputStream out = new FileOutputStream(tmp)) { out.write(data); out.getFD().sync(); }
        if (!tmp.renameTo(f)) { deleteTree(tmp); throw new IOException("rename " + f.getName()); }
    }

    private static MessageDigest md() throws IOException {
        try { return MessageDigest.getInstance("SHA-256"); } catch (NoSuchAlgorithmException e) { throw new IOException("sha256"); }
    }
    private static byte[] sha256(byte[] d) throws IOException { return md().digest(d); }

    private static void collect(File f, String rel, List<File> files, List<String> rels) throws IOException {
        if (f.isDirectory()) {
            File[] list = f.listFiles();
            if (list == null) throw new IOException("list " + f.getName());
            for (File c : sorted(list)) collect(c, rel.isEmpty() ? c.getName() : rel + "/" + c.getName(), files, rels);
        } else if (f.isFile()) { files.add(f); rels.add(rel); }
    }

    private static File[] sorted(File[] list) {
        if (list == null) return new File[0];
        Arrays.sort(list, (a, b) -> a.getName().compareTo(b.getName()));
        return list;
    }

    private static void le(ByteArrayOutputStream b, long v, int n) { for (int i = 0; i < n; i++) b.write((int) (v >>> (8 * i)) & 0xff); }

    private static String hex(byte[] d) { StringBuilder s = new StringBuilder(); for (byte x : d) s.append(String.format(Locale.ROOT, "%02x", x)); return s.toString(); }

    private static void append(StringBuilder b, String name) { if (b.length() > 0) b.append(", "); b.append(name); }

    private static boolean hasFiles(File dir) {
        for (File c : sorted(dir.listFiles())) if (c.isFile() || hasFiles(c)) return true;
        return false;
    }

    private static File unused(File f) {
        File t = f;
        for (int i = 2; t.exists() || new File(t.getPath() + ".part").exists(); i++) t = new File(f.getPath() + "-" + i);
        return t;
    }

    private static void mkdirs(File d) throws IOException { if (d != null && !d.isDirectory() && !d.mkdirs()) throw new IOException("mkdir " + d.getName()); }

    private static void copyTree(File from, File to, byte[] buffer) throws IOException {
        if (from.isDirectory()) {
            mkdirs(to);
            File[] list = from.listFiles();
            if (list == null) throw new IOException("list " + from.getName());
            for (File c : list) copyTree(c, new File(to, c.getName()), buffer);
            return;
        }
        mkdirs(to.getParentFile());
        try (InputStream in = new FileInputStream(from); OutputStream out = new FileOutputStream(to)) {
            for (int n; (n = in.read(buffer)) > 0; ) out.write(buffer, 0, n);
        }
        to.setLastModified(from.lastModified());
    }

    private static void deleteTree(File f) throws IOException {
        File[] list = f.isDirectory() ? f.listFiles() : null;
        if (list != null) for (File c : list) deleteTree(c);
        if (f.exists() && !f.delete()) throw new IOException("delete " + f.getName());
    }

    private static String reason(IOException e) { return e.getMessage() == null ? e.toString() : e.getMessage(); }
}
