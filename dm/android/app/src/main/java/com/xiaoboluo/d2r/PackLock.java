package com.xiaoboluo.d2r;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.util.concurrent.locks.ReentrantLock;



final class PackLock implements AutoCloseable {
    static final String FILE = "install.lock";
    private static final ReentrantLock HERE = new ReentrantLock();

    private final RandomAccessFile file;
    private final FileLock lock;

    private PackLock(RandomAccessFile file, FileLock lock) { this.file = file; this.lock = lock; }


    static PackLock take(File build) throws IOException {
        if (!build.isDirectory() && !build.mkdirs()) throw new IOException("cannot create " + build.getName());
        HERE.lock();
        RandomAccessFile f = null;
        try {
            f = new RandomAccessFile(new File(build, FILE), "rw");
            FileChannel ch = f.getChannel();
            return new PackLock(f, ch.lock());
        } catch (IOException | RuntimeException e) {
            if (f != null) try { f.close(); } catch (IOException ignored) {                              }
            HERE.unlock();
            throw e;
        }
    }

    @Override public void close() throws IOException {
        try { lock.release(); file.close(); }
        finally { HERE.unlock(); }
    }
}
