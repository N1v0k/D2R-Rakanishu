

















package com.xiaoboluo.d2r;

import java.util.HashMap;
import java.util.Map;

final class PadNav {
    static final int NONE = 0, UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4, A = 5, B = 6, X = 7, Y = 8, L1 = 9, R1 = 10, L2 = 11, R2 = 12,
            MENU = 13, SELECT = 14, LS = 15, RS = 16;
    static final long REPEAT_FIRST_MS = 400, REPEAT_NEXT_MS = 150;
    static final float STICK_ON = 0.5f, STICK_OFF = 0.4f, HAT_ON = 0.5f, TRIGGER_ON = 0.6f, TRIGGER_OFF = 0.4f;

    interface Sink { void onPad(int cmd); }



    private static final class Pad { long buttons, blocked; int keyDirs, hat, stick; boolean lt, rt, ltKey, rtKey; }

    private final Sink sink;
    private final Map<Integer, Pad> pads = new HashMap<>();
    private int dirs;
    private int blockedDirs;
    private int repeating = NONE;
    private long nextRepeat;

    PadNav(Sink sink) { this.sink = sink; }


    void begin() { for (Pad p : pads.values()) p.blocked = p.buttons; blockedDirs = dirs; repeating = NONE; }

    private Pad pad(int dev) { Pad p = pads.get(dev); if (p == null) { p = new Pad(); pads.put(dev, p); } return p; }


    void key(int dev, int cmd, boolean down, long now) {
        if (cmd == NONE) return;
        Pad p = pad(dev);
        if (isDir(cmd)) {
            int bit = 1 << cmd;
            p.keyDirs = down ? p.keyDirs | bit : p.keyDirs & ~bit;
            update(now);
            return;
        }
        if (cmd == L2) { p.ltKey = down; button(p, L2, p.ltKey || p.lt); return; }
        if (cmd == R2) { p.rtKey = down; button(p, R2, p.rtKey || p.rt); return; }
        button(p, cmd, down);
    }

    private void button(Pad p, int cmd, boolean down) {
        long bit = 1L << cmd;
        if (down) {
            if ((p.buttons & bit) != 0) return;
            p.buttons |= bit;
            if ((p.blocked & bit) == 0) sink.onPad(cmd);
        } else { p.buttons &= ~bit; p.blocked &= ~bit; }
    }


    void motion(int dev, float sx, float sy, float hx, float hy, float lt, float rt, long now) {
        Pad a = pad(dev);
        a.hat = (hx < -HAT_ON ? 1 << LEFT : 0) | (hx > HAT_ON ? 1 << RIGHT : 0) | (hy < -HAT_ON ? 1 << UP : 0) | (hy > HAT_ON ? 1 << DOWN : 0);
        a.stick = stick(a.stick, sx, sy);
        boolean l = lt > (a.lt ? TRIGGER_OFF : TRIGGER_ON), r = rt > (a.rt ? TRIGGER_OFF : TRIGGER_ON);
        if (l != a.lt) { a.lt = l; button(a, L2, a.ltKey || a.lt); }
        if (r != a.rt) { a.rt = r; button(a, R2, a.rtKey || a.rt); }
        update(now);
    }


    void gone(int dev, long now) {
        if (pads.remove(dev) != null) update(now);
    }



    long poll(long now) {
        if (repeating == NONE || (dirs & 1 << repeating) == 0) { repeating = NONE; return -1; }
        if (now >= nextRepeat) { sink.onPad(repeating); nextRepeat = now + REPEAT_NEXT_MS; }
        return nextRepeat;
    }

    private static boolean isDir(int cmd) { return cmd >= UP && cmd <= RIGHT; }


    private static int stick(int was, float x, float y) {
        float ax = Math.abs(x), ay = Math.abs(y);
        int held = was != 0 ? Integer.numberOfTrailingZeros(was) : NONE;
        if (held != NONE) {
            float v = held == LEFT ? -x : held == RIGHT ? x : held == UP ? -y : y;
            if (v >= STICK_OFF && (held == LEFT || held == RIGHT ? ax >= ay : ay >= ax)) return was;
        }
        if (ax < STICK_ON && ay < STICK_ON) return 0;
        if (ax >= ay) return 1 << (x < 0 ? LEFT : RIGHT);
        return 1 << (y < 0 ? UP : DOWN);
    }

    private void update(long now) {
        int all = 0;
        for (Pad a : pads.values()) all |= a.keyDirs | a.hat | a.stick;
        int pressed = all & ~dirs;
        blockedDirs &= all;
        dirs = all;
        for (int d = UP; d <= RIGHT; d++) {
            int bit = 1 << d;
            if ((pressed & bit) == 0 || (blockedDirs & bit) != 0) continue;
            repeating = d;
            nextRepeat = now + REPEAT_FIRST_MS;
            sink.onPad(d);
        }
        if (repeating != NONE && (dirs & 1 << repeating) == 0) repeating = NONE;
    }
}
