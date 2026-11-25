package com.xiaoboluo.d2r;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Comparator;

public final class SharedStorageTest {
    static void check(boolean ok) { if (!ok) throw new AssertionError(); }
    public static void main(String[] args) throws Exception {
        Path temp = Files.createTempDirectory("shared-storage");
        try {
            File game = temp.toFile(), old = new File(game, "android");
            File target = new File(SharedStorage.app(game), "android");
            check(SharedStorage.pack(game, "pack").equals(target));
            Files.createDirectories(old.toPath());
            Files.writeString(new File(old, "pack_info.txt").toPath(), "PRODUCT=other\n");
            check(SharedStorage.pack(game, "pack").equals(target) && old.isDirectory());
            Files.writeString(new File(old, "pack_info.txt").toPath(), "PRODUCT=pack\n");
            Files.createDirectories(new File(game, "data").toPath());
            Path data = new File(game, "data/shared").toPath();
            Files.writeString(data, "shared data");
            Files.writeString(new File(game, "export").toPath(), "upstream marker");
            check(SharedStorage.pack(game, "pack").equals(target));
            check(target.isDirectory() && !old.exists());
            check(Files.readString(data).equals("shared data"));
            Files.createDirectories(old.toPath());
            Files.writeString(new File(old, "pack_info.txt").toPath(), "PRODUCT=pack\n");
            check(SharedStorage.pack(game, "pack").equals(target) && old.isDirectory());
            Files.delete(new File(target, "pack_info.txt").toPath());
            Files.delete(target.toPath());
            Files.delete(SharedStorage.app(game).toPath());
            Files.writeString(SharedStorage.app(game).toPath(), "blocked directory");
            check(SharedStorage.pack(game, "pack").equals(old));
            Files.delete(SharedStorage.app(game).toPath());
            Files.createDirectories(SharedStorage.app(game).toPath());
            Path save = temp.resolve("internal/save"), foreignImport = temp.resolve("save-import/Foreign/Foreign.d2s");
            Files.createDirectories(save.resolve("Local"));
            byte[] character = java.nio.ByteBuffer.allocate(16).order(java.nio.ByteOrder.LITTLE_ENDIAN)
                .putInt(0xaa55aa55).putInt(0x69).putInt(16).putInt(0).array();
            Files.write(save.resolve("Local/Local.d2s"), character);
            Files.createDirectories(foreignImport.getParent());
            Files.write(foreignImport, character);
            check(SharedStorage.transfer(game, save.toFile(), "idle", null).isEmpty());
            check(Files.readString(temp.resolve("export")).equals("upstream marker"));
            check(Files.exists(foreignImport) && !Files.exists(save.resolve("Foreign")));
            Path own = SharedStorage.app(game).toPath();
            Files.writeString(own.resolve("export"), "");
            Path ownImport = own.resolve("save-import/Own/Own.d2s");
            Files.createDirectories(ownImport.getParent());
            Files.write(ownImport, character);
            var results = SharedStorage.transfer(game, save.toFile(), "active", null);
            check(results.size() == 2 && results.get(0).kind == SaveTransfer.EXPORTED && results.get(1).kind == SaveTransfer.IMPORTED);
            check(Files.exists(own.resolve("save-export/active/Local/Local.d2s")));
            check(Files.exists(save.resolve("Own/Own.d2s")));
            check(!Files.exists(own.resolve("export")) && !Files.exists(own.resolve("save-import")));
            check(Files.readString(temp.resolve("export")).equals("upstream marker"));
            check(Files.exists(foreignImport) && !Files.exists(temp.resolve("save-export")));
            check(!Files.exists(save.resolve("Foreign")));
            check(SharedStorage.transfer(game, save.toFile(), "again", null).isEmpty());
            System.out.println("SharedStorageTest: OK");
        } finally {
            try (var paths = Files.walk(temp)) {
                for (Path p : paths.sorted(Comparator.reverseOrder()).toList()) Files.delete(p);
            }
        }
    }
}
