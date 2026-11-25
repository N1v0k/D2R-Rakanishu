package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;




final class PackHost {
    static final String FILE = "host.pid";

    private PackHost() {}


    static void loaded(File build, int pid) {
        try { DriverLibrary.writeFile(new File(build, FILE), Integer.toString(pid), PackBuildService.SYNC); }
        catch (IOException e) { AppArt.log("pack build: the host marker cannot be written (" + e.getClass().getSimpleName() + ")"); }
    }


    static void ended(File build, int pid) {
        File f = new File(build, FILE);
        if (Integer.toString(pid).equals(read(f)) && !f.delete()) AppArt.log("pack build: the host marker cannot be removed");
    }


    static boolean running(File build, String pkg, int self, File proc) {
        String text = read(new File(build, FILE));
        if (text == null || !text.trim().matches("[1-9][0-9]{0,9}")) return false;
        int pid = Integer.parseInt(text.trim());
        if (pid == self) return true;
        String cmd = read(new File(proc, pid + "/cmdline"));
        if (cmd == null) return false;
        int zero = cmd.indexOf('\0');
        return (zero < 0 ? cmd : cmd.substring(0, zero)).equals(pkg);
    }

    private static String read(File f) {
        try { return new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8); } catch (IOException e) { return null; }
    }
}
