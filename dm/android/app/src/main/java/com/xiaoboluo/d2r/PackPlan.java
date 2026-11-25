package com.xiaoboluo.d2r;






final class PackPlan {
    static final String LANES = "lanes", SERIAL = "serial";
    static final long LANES_MIN = 2560L << 20;
    static final int MAX_SHADER_WORKERS = 4, MAX_PICTURE_THREADS = 8;

    final int tokens, shaderWorkers, pictureThreads;
    final String mode;

    PackPlan(int tokens, int shaderWorkers, int pictureThreads, String mode) {
        if (tokens < 1 || shaderWorkers < 1 || pictureThreads < 1) throw new IllegalArgumentException("a plan needs at least one of each: " + tokens + " " + shaderWorkers + " " + pictureThreads);
        if (!mode.equals(LANES) && !mode.equals(SERIAL)) throw new IllegalArgumentException("unknown mode " + mode);
        this.tokens = tokens; this.shaderWorkers = shaderWorkers; this.pictureThreads = pictureThreads; this.mode = mode;
    }


    static PackPlan decide(int cores, long available) {
        int n = Math.max(1, cores);
        if (available < LANES_MIN) return new PackPlan(n, clamp(n - 1, MAX_SHADER_WORKERS), clamp(n - 1, MAX_PICTURE_THREADS), SERIAL);
        return new PackPlan(n, clamp(n / 2 - 1, MAX_SHADER_WORKERS), clamp(n / 4, MAX_PICTURE_THREADS), LANES);
    }

    private static int clamp(int v, int max) { return Math.max(1, Math.min(max, v)); }

    @Override public String toString() {
        return mode + ", " + tokens + " tokens, shaders " + shaderWorkers + " + 1, pictures " + pictureThreads;
    }
}
