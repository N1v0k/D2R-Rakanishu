package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.List;

final class SharedStorage {
    static final String APP_FOLDER = "r4k4n1shu";

    static File app(File game) { return new File(game, APP_FOLDER); }

    static File pack(File game, String product) {
        File target = new File(app(game), "android");
        if (target.exists()) return target;
        File legacy = new File(game, "android");
        try {
            if (!Files.readAllLines(new File(legacy, "pack_info.txt").toPath()).contains("PRODUCT=" + product)) return target;
        } catch (IOException e) { return target; }

        if ((app(game).isDirectory() || app(game).mkdirs()) && legacy.renameTo(target)) return target;
        return legacy;
    }

    static List<SaveTransfer.Result> transfer(File game, File save, String stamp, SaveTransfer.Builds builds) {
        return SaveTransfer.run(app(game), save, stamp, builds);
    }

    private SharedStorage() {}
}
