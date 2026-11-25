






























package com.xiaoboluo.d2r;

import java.util.HashSet;
import java.util.Set;

final class PadHold {
    static final long RING_MS = 150, OPEN_MS = 500, PULSE_TIMEOUT_MS = 2000, PULSE_DELAY_MS = 60;
    static final int SEND_READS = 3, RELEASE_READS = 2, STALL_READS = 4, ATTEMPTS = 3, MAX_QUEUED = 2;

    static final int EV_OPEN = 1, EV_UNLATCH = 2, EV_DROPPED = 4, EV_SENT = 8;

    private static final int IDLE = 0, HELD = 1, OPENING = 2, LATCHED = 3;
    private static final int P_IDLE = 0, P_WAIT = 1, P_SEND = 2, P_RELEASE = 3, P_RETRY = 4, P_VERIFY = 5;

    private final Set<Integer> owned = new HashSet<>();
    private int state = IDLE, device;
    private boolean keyDown;
    private long t0;
    private int pstate = P_IDLE, queued, attempts, pulseDev;
    private long on0, off0, onV, offV, pulseAt, readyAt, lastOn = -1, lastOff = -1;
    private boolean plus, armed;


    boolean down(int dev, long now, boolean eligible) {
        if (owned.contains(dev)) return true;
        if (state == IDLE && eligible) { state = HELD; device = dev; keyDown = true; t0 = now; owned.add(dev); return true; }
        if (state != IDLE) { owned.add(dev); return true; }
        return false;
    }




    void up(int dev, long now, boolean eligible, boolean cancelled) {
        owned.remove(dev);
        if (dev != device || state == IDLE) return;
        keyDown = false;
        if (cancelled) state = IDLE;
        else if (state == HELD && now - t0 < OPEN_MS) {
            if (eligible && queued < MAX_QUEUED) { queued++; pulseDev = dev; readyAt = now + PULSE_DELAY_MS; }
            state = IDLE;
        } else if (state == HELD) state = OPENING;
        else if (state == LATCHED) state = IDLE;
    }




    int tick(long now, boolean busy, long on, long off, boolean eligible) {
        int ev = 0;
        if (!eligible && (state == HELD || state == OPENING || pstate != P_IDLE || queued > 0)) ev |= cancelEv();
        if (state == HELD && now - t0 >= OPEN_MS) state = OPENING;
        if (state == OPENING && !busy) { state = keyDown ? LATCHED : IDLE; ev |= EV_OPEN; }
        if (pstate == P_IDLE && queued > 0 && now >= readyAt) { pstate = P_WAIT; pulseAt = now; attempts = 0; }
        if (busy || on != lastOn || off != lastOff) pulseAt = now;
        lastOn = on; lastOff = off;
        switch (pstate) {
            case P_WAIT:
                if (!busy) { pstate = P_SEND; plus = true; on0 = on; off0 = off; attempts++; }
                break;
            case P_SEND:
                if (on - on0 >= SEND_READS) { plus = false; pstate = P_RELEASE; off0 = off; }
                else if (on > on0 && busy) { plus = false; pstate = P_RELEASE; off0 = off; }
                else if (on == on0 && (busy || off - off0 >= STALL_READS)) {
                    plus = false; ev |= EV_UNLATCH; pstate = P_VERIFY; armed = false;
                }
                break;
            case P_VERIFY:
                if (!armed) { armed = true; onV = on; offV = off; break; }
                if (on == onV && off == offV) break;
                if (on > on0) { pstate = P_RELEASE; off0 = off; break; }
                off0 = off;
                pstate = attempts < ATTEMPTS ? P_RETRY : P_IDLE;
                if (pstate == P_IDLE) { queued--; ev |= EV_DROPPED; }
                break;
            case P_RELEASE:
                if (off - off0 >= RELEASE_READS) { pstate = P_IDLE; queued--; ev |= EV_SENT; if (queued > 0) { pstate = P_WAIT; pulseAt = now; attempts = 0; } }
                break;
            case P_RETRY:
                if (off - off0 >= RELEASE_READS && !busy) pstate = P_WAIT;
                break;
            default: break;
        }
        if (pstate != P_IDLE && now - pulseAt > PULSE_TIMEOUT_MS) {
            if (plus) ev |= EV_UNLATCH;
            plus = false; queued = 0; pstate = P_IDLE; ev |= EV_DROPPED;
        }
        return ev;
    }



    boolean cancel() { return cancelEv() != 0; }

    private int cancelEv() {
        int ev = plus ? EV_UNLATCH : 0;
        if (state == HELD || state == OPENING) state = keyDown ? LATCHED : IDLE;
        queued = 0; plus = false; pstate = P_IDLE;
        return ev;
    }


    boolean reset() { boolean had = cancelEv() != 0; owned.clear(); state = IDLE; keyDown = false; return had; }




    boolean deviceGone(int dev) {
        boolean had = false;
        if (state != IDLE && dev == device) { had = cancel(); keyDown = false; state = IDLE; }
        else if (queued > 0 && dev == pulseDev) had = cancel();
        owned.remove(dev);
        return had;
    }

    boolean masksPlus() { return !owned.isEmpty(); }
    boolean plusOut() { return plus; }
    boolean active() { return state == HELD || state == OPENING || pstate != P_IDLE || queued > 0; }
    int attempts() { return attempts; }
    boolean holding() { return state == HELD || state == OPENING; }


    float ring(long now) {
        if (state == OPENING) return 1f;
        if (state != HELD || now - t0 < RING_MS) return -1f;
        return Math.min(1f, (now - t0 - RING_MS) / (float) (OPEN_MS - RING_MS));
    }
}
