
















package com.xiaoboluo.d2r;

import android.os.SystemClock;
import android.view.Choreographer;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

final class PadInput {

    static final long BTN_A = 1, BTN_B = 1 << 1, BTN_X = 1 << 2, BTN_Y = 1 << 3,
            BTN_LS = 1 << 4, BTN_RS = 1 << 5, BTN_L = 1 << 6, BTN_R = 1 << 7,
            BTN_ZL = 1 << 8, BTN_ZR = 1 << 9, BTN_PLUS = 1 << 10, BTN_MINUS = 1 << 11,
            BTN_LEFT = 1 << 12, BTN_UP = 1 << 13, BTN_RIGHT = 1 << 14, BTN_DOWN = 1 << 15;


    static final int GLYPH_PAD = 0, GLYPH_XBOX = 2, GLYPH_PS = 3;

    private static final double OUT = 0.25, IN = 0.15;
    private static final float TRIGGER = 0.5f;



    interface Host {
        boolean captured();
        boolean holdAllowed();
        void holdOpen();
        void holdRing(float progress);
    }

    private static final class State {
        long buttons;
        float lx, ly, rx, ry;
        boolean lt, rt;
        boolean hatL, hatR, hatU, hatD;
        int glyph = GLYPH_PAD;
    }

    private static final Map<Integer, State> pads = new HashMap<>();
    private static final PadHold hold = new PadHold();
    private static final PadGate gate = new PadGate();
    private static final long[] reads = new long[2];
    private static Host host;
    private static PadNav nav;
    private static boolean padMode;
    private static boolean needCenter;
    private static boolean onL, onR;
    private static boolean ticking;
    private static int glyph = GLYPH_PAD;
    private static long prevButtons;
    private static long extra;
    private static long lastButtons = -1;
    private static int lastLx, lastLy, lastRx, lastRy;

    private PadInput() {}

    static void setHost(Host h, PadNav n) { host = h; nav = n; }
    static boolean isPadMode() { return padMode; }
    static int glyph() { return glyph; }

    static void setExtra(long buttons) { extra = buttons; publish(); }







    static void pagesChanged() {
        boolean captured = host != null && host.captured();
        if (captured && hold.active()) log("手柄：我们的页开了，菜单键按住 / 短按作废 | our page opened, menu button hold / short press dropped");
        if (captured) hold.cancel();
        if (nav != null) nav.begin();
        publish();
        if (captured) DmActivity.nativePadUnlatch(-1L);
        ring();
    }


    static boolean isPadKey(KeyEvent e) { return isPadSource(e.getSource()) && (keyBit(e.getKeyCode()) != 0 || navCmd(e.getKeyCode()) != PadNav.NONE); }

    static boolean onKey(KeyEvent e) {
        if (!isPadKey(e)) return false;
        int code = e.getKeyCode(), dev = e.getDeviceId();
        State s = state(dev);
        long bit = keyBit(code), now = e.getEventTime();
        s.glyph = glyph = glyphOf(e.getDevice());
        if (e.getAction() == KeyEvent.ACTION_DOWN) {
            if (e.getRepeatCount() > 0) return true;
            if (bit == BTN_PLUS) hold.down(dev, now, holdAllowed());
            s.buttons |= bit;
            if (nav != null) nav.key(dev, navCmd(code), true, now);
        } else if (e.getAction() == KeyEvent.ACTION_UP) {
            if (bit == BTN_PLUS) hold.up(dev, now, holdAllowed(), (e.getFlags() & KeyEvent.FLAG_CANCELED) != 0);
            s.buttons &= ~bit;
            if (nav != null) nav.key(dev, navCmd(code), false, now);
        } else {
            return true;
        }
        publish();
        tickLater();
        return true;
    }

    static boolean onMotion(MotionEvent e) {
        if (!isPadSource(e.getSource())) return false;
        State s = state(e.getDeviceId());
        s.glyph = glyph = glyphOf(e.getDevice());
        float ax = e.getAxisValue(MotionEvent.AXIS_X), ay = e.getAxisValue(MotionEvent.AXIS_Y);
        s.lx = axis(ax);
        s.ly = -axis(ay);
        s.rx = axis(e.getAxisValue(MotionEvent.AXIS_Z));
        s.ry = -axis(e.getAxisValue(MotionEvent.AXIS_RZ));
        float lt = Math.max(e.getAxisValue(MotionEvent.AXIS_LTRIGGER), e.getAxisValue(MotionEvent.AXIS_BRAKE));
        float rt = Math.max(e.getAxisValue(MotionEvent.AXIS_RTRIGGER), e.getAxisValue(MotionEvent.AXIS_GAS));
        s.lt = lt > TRIGGER;
        s.rt = rt > TRIGGER;

        float hx = e.getAxisValue(MotionEvent.AXIS_HAT_X), hy = e.getAxisValue(MotionEvent.AXIS_HAT_Y);
        boolean hl = hx < -TRIGGER, hr = hx > TRIGGER, hu = hy < -TRIGGER, hd = hy > TRIGGER;
        if (hl != s.hatL) { s.hatL = hl; toggle(s, BTN_LEFT, hl); }
        if (hr != s.hatR) { s.hatR = hr; toggle(s, BTN_RIGHT, hr); }
        if (hu != s.hatU) { s.hatU = hu; toggle(s, BTN_UP, hu); }
        if (hd != s.hatD) { s.hatD = hd; toggle(s, BTN_DOWN, hd); }
        if (nav != null) nav.motion(e.getDeviceId(), ax, ay, hx, hy, lt, rt, e.getEventTime());
        publish();
        return true;
    }



    static void onTouch() {
        if (hold.active()) log("手柄：碰屏，菜单键按住 / 短按作废 | touch, menu button hold / short press dropped");
        if (hold.cancel()) { publish(); unlatch(); }
        ring();
        if (!padMode) return;
        needCenter = true;
        setMode(false);
        publish();
    }



    static void reset() {
        if (hold.active()) log("手柄：进后台，菜单键按住 / 短按作废 | background, menu button hold / short press dropped");
        boolean any = hold.reset();
        boolean latch = any;
        gate.reset();
        for (State s : pads.values()) {
            any |= s.buttons != 0 || s.lx != 0 || s.ly != 0 || s.rx != 0 || s.ry != 0 || s.lt || s.rt;
            s.buttons = 0; s.lx = s.ly = s.rx = s.ry = 0; s.lt = s.rt = false; s.hatL = s.hatR = s.hatU = s.hatD = false;
        }
        if (nav != null) { long now = SystemClock.uptimeMillis(); for (Integer id : pads.keySet()) nav.gone(id, now); }
        if (any) publish();
        if (latch) unlatch();
        ring();
    }



    static void sweep() {
        if (pads.isEmpty()) return;
        boolean removed = false, latch = false;
        long now = SystemClock.uptimeMillis();
        for (Iterator<Map.Entry<Integer, State>> it = pads.entrySet().iterator(); it.hasNext(); ) {
            Map.Entry<Integer, State> e = it.next();
            if (InputDevice.getDevice(e.getKey()) != null) continue;
            it.remove(); removed = true;
            if (hold.active()) log("手柄：断开（设备 " + e.getKey() + "），菜单键按住 / 短按作废 | pad gone, menu button hold / short press dropped");
            latch |= hold.deviceGone(e.getKey());
            if (nav != null) nav.gone(e.getKey(), now);
        }
        if (!removed) return;
        if (pads.isEmpty()) { setMode(false); }
        publish();
        if (latch) unlatch();
        ring();
    }

    private static void setMode(boolean pad) {
        if (padMode == pad) return;
        padMode = pad;
        TestHooks.overlay("pad_layout", pad ? "pad" : "touch");
        DmActivity.nativePadMode(pad ? 1 : 0);
        DmActivity.onPadMode(pad);
        android.util.Log.i("dm", pad ? "手柄：有明确输入 → 手柄布局" : "手柄：碰屏 / 断开 → 触屏布局");
    }

    private static boolean holdAllowed() { return host != null && host.holdAllowed(); }

    private static void unlatch() { DmActivity.nativePadUnlatch(BTN_PLUS); }


    private static void log(String line) { AppArt.log(line); }

    private static void ring() { if (host != null) host.holdRing(hold.ring(SystemClock.uptimeMillis())); }


    private static final Choreographer.FrameCallback ticker = t -> tick();
    private static void tickLater() {
        if (ticking || !hold.active()) { ring(); return; }
        ticking = true;
        Choreographer.getInstance().postFrameCallback(ticker);
        ring();
    }

    private static void tick() {
        ticking = false;
        long now = SystemClock.uptimeMillis();
        boolean busy = DmActivity.nativePadBusy() != 0;
        DmActivity.nativePlusReads(reads);
        int ev = hold.tick(now, busy, reads[0], reads[1], holdAllowed());
        publish();
        if ((ev & PadHold.EV_UNLATCH) != 0) unlatch();
        if ((ev & PadHold.EV_DROPPED) != 0) log("手柄：菜单键短按没送到游戏，作废 | menu button short press not delivered, dropped");
        if ((ev & PadHold.EV_SENT) != 0) log("手柄：菜单键短按交给游戏（第 " + hold.attempts() + " 次投） | menu button short press delivered (attempt " + hold.attempts() + ")");
        if ((ev & PadHold.EV_OPEN) != 0) { log("手柄：按住菜单键 0.5 s → 快捷菜单 | menu button held 0.5 s -> Quick Menu"); if (host != null) host.holdOpen(); }
        tickLater();
    }

    private static void toggle(State s, long bit, boolean on) {
        if (on) s.buttons |= bit; else s.buttons &= ~bit;
    }

    private static State state(int id) {
        State s = pads.get(id);
        if (s == null) { s = new State(); pads.put(id, s); }
        return s;
    }

    private static boolean isPadSource(int source) {
        return (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
                || (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD;
    }



    private static long keyBit(int code) {
        switch (code) {
            case KeyEvent.KEYCODE_BUTTON_A: return BTN_A;
            case KeyEvent.KEYCODE_BUTTON_B: return BTN_B;
            case KeyEvent.KEYCODE_BUTTON_X: return BTN_X;
            case KeyEvent.KEYCODE_BUTTON_Y: return BTN_Y;
            case KeyEvent.KEYCODE_BUTTON_L1: return BTN_L;
            case KeyEvent.KEYCODE_BUTTON_R1: return BTN_R;
            case KeyEvent.KEYCODE_BUTTON_L2: return BTN_ZL;
            case KeyEvent.KEYCODE_BUTTON_R2: return BTN_ZR;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: return BTN_LS;
            case KeyEvent.KEYCODE_BUTTON_THUMBR: return BTN_RS;
            case KeyEvent.KEYCODE_BUTTON_START: return BTN_PLUS;
            case KeyEvent.KEYCODE_BUTTON_SELECT:
            case KeyEvent.KEYCODE_BACK: return BTN_MINUS;
            case KeyEvent.KEYCODE_DPAD_UP: return BTN_UP;
            case KeyEvent.KEYCODE_DPAD_DOWN: return BTN_DOWN;
            case KeyEvent.KEYCODE_DPAD_LEFT: return BTN_LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return BTN_RIGHT;
            default: return 0;
        }
    }


    private static int navCmd(int code) {
        switch (code) {
            case KeyEvent.KEYCODE_BUTTON_A: case KeyEvent.KEYCODE_DPAD_CENTER: return PadNav.A;
            case KeyEvent.KEYCODE_BUTTON_B: case KeyEvent.KEYCODE_BACK: return PadNav.B;
            case KeyEvent.KEYCODE_BUTTON_X: return PadNav.X;
            case KeyEvent.KEYCODE_BUTTON_Y: return PadNav.Y;
            case KeyEvent.KEYCODE_BUTTON_L1: return PadNav.L1;
            case KeyEvent.KEYCODE_BUTTON_R1: return PadNav.R1;
            case KeyEvent.KEYCODE_BUTTON_L2: return PadNav.L2;
            case KeyEvent.KEYCODE_BUTTON_R2: return PadNav.R2;
            case KeyEvent.KEYCODE_BUTTON_THUMBL: return PadNav.LS;
            case KeyEvent.KEYCODE_BUTTON_THUMBR: return PadNav.RS;
            case KeyEvent.KEYCODE_BUTTON_START: return PadNav.MENU;
            case KeyEvent.KEYCODE_BUTTON_SELECT: return PadNav.SELECT;
            case KeyEvent.KEYCODE_DPAD_UP: return PadNav.UP;
            case KeyEvent.KEYCODE_DPAD_DOWN: return PadNav.DOWN;
            case KeyEvent.KEYCODE_DPAD_LEFT: return PadNav.LEFT;
            case KeyEvent.KEYCODE_DPAD_RIGHT: return PadNav.RIGHT;
            default: return PadNav.NONE;
        }
    }


    private static int glyphOf(InputDevice dev) {
        if (dev == null) return GLYPH_PAD;
        switch (dev.getVendorId()) {
            case 0x045e: return GLYPH_XBOX;
            case 0x054c: return GLYPH_PS;
            default: return GLYPH_PAD;
        }
    }

    private static float axis(float v) {
        if (v > 1f) v = 1f;
        if (v < -1f) v = -1f;
        return v * 32767f;
    }

    private static boolean stickOn(boolean was, float x, float y) {
        double m = Math.hypot(x, y) / 32767.0;
        return was ? m >= IN : m >= OUT;
    }

    private static void publish() {
        sweep();
        long bits = 0;
        float lx = 0, ly = 0, rx = 0, ry = 0;
        double bestL = -1, bestR = -1;
        for (State s : pads.values()) {
            bits |= s.buttons | (s.lt ? BTN_ZL : 0) | (s.rt ? BTN_ZR : 0);
            double ml = Math.hypot(s.lx, s.ly), mr = Math.hypot(s.rx, s.ry);
            if (ml > bestL) { bestL = ml; lx = s.lx; ly = s.ly; }
            if (mr > bestR) { bestR = mr; rx = s.rx; ry = s.ry; }
        }
        onL = stickOn(onL, lx, ly);
        onR = stickOn(onR, rx, ry);
        if (!onL && !onR) needCenter = false;



        boolean captured = host != null && host.captured();
        long out = gate.apply(captured, bits, onL, onR);
        boolean stickOut = onL && gate.stickL() || onR && gate.stickR();
        if (!captured && ((bits & ~prevButtons) != 0 || (stickOut && !needCenter))) {

            int g = GLYPH_PAD;
            for (State s : pads.values()) g = s.glyph;
            DmActivity.nativePadGlyph(g);
            setMode(true);
        }
        prevButtons = bits;

        if (hold.masksPlus()) out &= ~BTN_PLUS;
        if (!captured) out |= (hold.plusOut() ? BTN_PLUS : 0) | extra;


        if (!padMode || !onL || !gate.stickL()) { lx = 0; ly = 0; }
        if (!padMode || !onR || !gate.stickR()) { rx = 0; ry = 0; }

        int ilx = (int) lx, ily = (int) ly, irx = (int) rx, iry = (int) ry;
        if (out == lastButtons && ilx == lastLx && ily == lastLy && irx == lastRx && iry == lastRy) return;
        lastButtons = out; lastLx = ilx; lastLy = ily; lastRx = irx; lastRy = iry;
        DmActivity.nativePad(out, ilx, ily, irx, iry);
    }
}
