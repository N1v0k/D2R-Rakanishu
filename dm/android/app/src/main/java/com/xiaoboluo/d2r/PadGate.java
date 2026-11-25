






package com.xiaoboluo.d2r;

final class PadGate {
    private long quarantine;
    private boolean captured, quarL, quarR;




    long apply(boolean nowCaptured, long bits, boolean outL, boolean outR) {
        if (nowCaptured) { captured = true; return 0; }
        if (captured) { captured = false; quarantine = bits; quarL = outL; quarR = outR; }
        quarantine &= bits;
        if (!outL) quarL = false;
        if (!outR) quarR = false;
        return bits & ~quarantine;
    }

    boolean captured() { return captured; }
    boolean stickL() { return !captured && !quarL; }
    boolean stickR() { return !captured && !quarR; }
    long quarantined() { return quarantine; }


    void reset() { quarantine = 0; quarL = quarR = false; }
}
