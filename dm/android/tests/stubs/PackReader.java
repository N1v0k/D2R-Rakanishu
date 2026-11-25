package com.xiaoboluo.d2r;


final class PackReader {
    interface Sink { void wrote(long unitBytes, String file); }
    private PackReader() {}
}
