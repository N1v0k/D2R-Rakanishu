package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.SharedPreferences;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;








final class ShoulderKeys {
    static final int NONE = 0, ATTACK = 1, SKILL1 = 2, SKILL4 = 5, HEAL = 6, MANA = 7, REJUV = 8, PORTAL = 9, SWAP = 10, MENU = 11, MAP = 12;
    static final int RECIPES = 13, QUICK = 14;

    private static final int T_PORTAL = 5, T_MENU = 10, T_SWAP = 13;
    private static final long BTN_MINUS = 1 << 11;
    private static final String PREFS = "dm_shoulder", LEFT = "left", RIGHT = "right", HAS = "has_triggers";

    private static final int[] held = new int[2];
    private static final boolean[] pressed = new boolean[2];
    private static final Set<Integer> triggerDevices = new HashSet<>();

    private ShoulderKeys() {}


    static boolean german() { return GameLang.german(); }

    private static SharedPreferences prefs(Context c) { return c.getSharedPreferences(PREFS, Context.MODE_PRIVATE); }

    static int action(int hand) { return DmActivity.nativeShoulderAction(hand); }


    static int legacyLeft(Context c) { return prefs(c).getInt(LEFT, -1); }
    static int legacyRight(Context c) { return prefs(c).getInt(RIGHT, -1); }


    static boolean hasTriggers(Context c) {
        if (prefs(c).getBoolean(HAS, false)) return true;
        String m = android.os.Build.MANUFACTURER == null ? "" : android.os.Build.MANUFACTURER.toLowerCase(Locale.ROOT);
        return m.contains("nubia") || m.contains("redmagic");
    }
    private static boolean seenTrigger;


    static void detect(KeyEvent e, Context c) {
        if (e.getKeyCode() != KeyEvent.KEYCODE_F7 && e.getKeyCode() != KeyEvent.KEYCODE_F8) return;
        triggerDevices.add(e.getDeviceId());
        if (seenTrigger) return;
        seenTrigger = true;
        if (!prefs(c).getBoolean(HAS, false)) prefs(c).edit().putBoolean(HAS, true).apply();
        DmActivity.nativeSetHasTriggers(1);
    }






    static final int STATUS_CLOCK = 1, STATUS_BATTERY = 2, STATUS_FPS = 4;
    static int legacyFps(Context c) { return prefs(c).getInt("fps_cap", -1); }
    static int legacyStatus(Context c) {
        SharedPreferences p = prefs(c);
        if (!p.contains("status_line") && !p.contains("status_clock") && !p.contains("status_battery") && !p.contains("status_fps")) return -1;
        if (!p.getBoolean("status_line", true)) return 0;
        return (p.getBoolean("status_clock", true) ? STATUS_CLOCK : 0) | (p.getBoolean("status_battery", true) ? STATUS_BATTERY : 0) | (p.getBoolean("status_fps", true) ? STATUS_FPS : 0);
    }
    static int statusMask() { return DmActivity.nativeStatusLines(); }

    static boolean onKey(KeyEvent e, TouchOverlay overlay) {
        int code = e.getKeyCode();
        if (code != KeyEvent.KEYCODE_F7 && code != KeyEvent.KEYCODE_F8) return false;
        int hand = code == KeyEvent.KEYCODE_F7 ? 0 : 1;
        triggerDevices.add(e.getDeviceId());
        if (e.getAction() == KeyEvent.ACTION_DOWN && e.getRepeatCount() == 0) {
            held[hand] = action(hand);
            pressed[hand] = press(hand, held[hand], overlay);
        } else if (e.getAction() == KeyEvent.ACTION_UP && pressed[hand]) {
            pressed[hand] = false;
            release(hand, held[hand], overlay);
        }
        return true;
    }


    private static boolean press(int hand, int action, TouchOverlay overlay) {
        if (action >= HEAL && action <= REJUV) { DmActivity.nativeBeltDrink(action - HEAL + 1); return false; }
        if (action == MAP) { PadInput.setExtra(BTN_MINUS); return true; }
        if (action == QUICK) { DmActivity.quickMenuFromTrigger(PadInput.isPadMode()); return false; }
        if (action == RECIPES) return false;
        int target = target(action);
        return target >= 0 && overlay != null && overlay.pressTarget(hand, target);
    }

    private static void release(int hand, int action, TouchOverlay overlay) {
        if (action == MAP) PadInput.setExtra(0);
        else if (overlay != null) overlay.releaseTarget(hand);
    }

    private static int target(int action) {
        if (action == ATTACK) return 0;
        if (action >= SKILL1 && action <= SKILL4) return action - SKILL1 + 1;
        return action == PORTAL ? T_PORTAL : action == SWAP ? T_SWAP : action == MENU ? T_MENU : -1;
    }


    static void reset(TouchOverlay overlay) {
        for (int hand = 0; hand < 2; hand++) if (pressed[hand]) { pressed[hand] = false; release(hand, held[hand], overlay); }
    }

    static boolean strayTouch(MotionEvent e) {
        return e.isFromSource(InputDevice.SOURCE_MOUSE) && triggerDevices.contains(e.getDeviceId());
    }

    static boolean fromTrigger(MotionEvent e) { return triggerDevices.contains(e.getDeviceId()); }
}
