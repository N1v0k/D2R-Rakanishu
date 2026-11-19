package com.xiaoboluo.d2r;








final class QuitRule {
    static final int WAIT = 0, COMMIT = 1, RETRY = 2, ABORT = 3;
    private QuitRule() {}



    static int decide(long target, long done, int result, boolean retried) {
        if (done < target) return WAIT;
        if (result == 1 || result == 0) return COMMIT;
        if (result == -1 && !retried) return RETRY;
        return ABORT;
    }


    static boolean clean(int result) { return result == 1 || result == 0; }
}
