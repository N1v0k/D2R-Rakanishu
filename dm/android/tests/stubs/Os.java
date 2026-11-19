



package android.system;

import java.io.FileDescriptor;

public final class Os {
    public static long sysconf(int name) { return 4096; }
    public static FileDescriptor open(String path, int flags, int mode) throws ErrnoException { return new FileDescriptor(); }
    public static void fsync(FileDescriptor fd) throws ErrnoException { }
    public static void close(FileDescriptor fd) throws ErrnoException { }

    private Os() {}
}
