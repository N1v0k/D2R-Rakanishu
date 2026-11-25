package com.xiaoboluo.d2r;




final class PackReader {
    static { System.loadLibrary("packbuild"); }


    interface Sink { void wrote(long unitBytes, String file); }


    static native String nativeProbe(int fd, long length, byte[] keys, String contract);

    static native String nativeUnits(String contract);

    static native long nativeOpen(int baseFd, int updateFd, byte[] keys, String contract, String[] error);

    static native String nativeExtract(long handle, String unit, String exefsDir, String dataDir, Sink sink);
    static native void nativeCancel(long handle);
    static native void nativeClose(long handle);

    private PackReader() {}
}
