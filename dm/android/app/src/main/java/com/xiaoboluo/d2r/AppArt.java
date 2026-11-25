package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import org.json.JSONException;
import org.json.JSONObject;












final class AppArt {
    static final int EXOCET = 0, FORMAL = 1;
    private final File root;
    private final Context context;
    private final HashMap<String, Bitmap> images = new HashMap<>();
    private final HashMap<String, Typeface> fonts = new HashMap<>();
    private final Rect src = new Rect();
    private final RectF dst = new RectF();

    AppArt(Context context) { this(context, new File(context.getFilesDir(), "dm/appart")); }


    AppArt(Context context, File root) { this.context = context; this.root = root; }


    String text(String name) {
        try { return new String(Files.readAllBytes(new File(root, name).toPath()), StandardCharsets.UTF_8); } catch (IOException e) { return null; }
    }



    private InvPictures inv;
    private boolean invRead;
    InvPictures invPictures() {
        if (!invRead) {
            invRead = true;
            try {
                JSONObject o = new JSONObject(text("inv.json"));
                Map<String, Integer> v = new HashMap<>();
                JSONObject vo = o.getJSONObject("v");
                for (Iterator<String> k = vo.keys(); k.hasNext(); ) { String key = k.next(); v.put(key, vo.getInt(key)); }
                inv = new InvPictures(strings(o.getJSONObject("i")), strings(o.getJSONObject("u")), strings(o.getJSONObject("s")), v);
            } catch (JSONException | NullPointerException e) { inv = null; }
        }
        return inv;
    }
    private static Map<String, String> strings(JSONObject o) throws JSONException {
        Map<String, String> m = new HashMap<>();
        for (Iterator<String> k = o.keys(); k.hasNext(); ) { String key = k.next(); m.put(key, o.getString(key)); }
        return m;
    }



    private MonPictures mon;
    private boolean monRead;
    MonPictures monPictures() {
        if (!monRead) {
            monRead = true;
            try {
                JSONObject o = new JSONObject(text("mon.json"));
                if (o.length() != 2) throw new JSONException("mon.json has " + o.length() + " members");
                java.util.Set<String> none = new java.util.HashSet<>();
                JSONObject no = o.getJSONObject("none");
                for (Iterator<String> k = no.keys(); k.hasNext(); ) { String key = k.next(); if (no.getString(key).isEmpty()) throw new JSONException(key); none.add(key); }
                mon = new MonPictures(strings(o.getJSONObject("pictures")), none);
            } catch (JSONException | NullPointerException | IllegalArgumentException e) { mon = null; }
        }
        return mon;
    }



    Bitmap monPicture(String name, boolean small) {
        if (!MonPictures.NAME.matcher(name).matches()) return null;
        String path = new File(root, small ? "mon/s/" + name + ".webp" : "mon/" + name + ".webp").getAbsolutePath();
        BitmapFactory.Options bounds = new BitmapFactory.Options();
        bounds.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, bounds);
        if (!MonPictures.fits(bounds.outWidth, bounds.outHeight, small)) return null;
        return BitmapFactory.decodeFile(path);
    }



    Bitmap invPicture(String name) {
        if (name == null || name.isEmpty() || name.contains("/") || name.contains("..")) return null;
        String path = new File(root, "inv/" + name + ".png").getAbsolutePath();
        BitmapFactory.Options bounds = new BitmapFactory.Options();
        bounds.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, bounds);
        if (bounds.outWidth <= 0 || bounds.outHeight <= 0 || bounds.outWidth > 512 || bounds.outHeight > 512) return null;
        return BitmapFactory.decodeFile(path);
    }

    Bitmap image(String name) {
        if (!images.containsKey(name)) images.put(name, BitmapFactory.decodeFile(new File(root, name + ".png").getAbsolutePath()));
        return images.get(name);
    }




    int[] colours(String name, String[] keys, int[] fallback) {
        int[] out = fallback.clone();
        boolean[] got = new boolean[keys.length];
        boolean noFile = false;
        try {
            for (String line : new String(Files.readAllBytes(new File(root, name).toPath()), StandardCharsets.UTF_8).split("\n")) {
                String[] f = line.trim().split("\\s+");
                if (f.length != 4) continue;
                for (int i = 0; i < keys.length; i++) {
                    if (!keys[i].equals(f[0])) continue;
                    try {
                        int r = Integer.parseInt(f[1]), g = Integer.parseInt(f[2]), b = Integer.parseInt(f[3]);
                        if ((r | g | b) >= 0 && r < 256 && g < 256 && b < 256) { out[i] = 0xFF000000 | r << 16 | g << 8 | b; got[i] = true; }
                    } catch (NumberFormatException e) {                                                     }
                }
            }
        } catch (IOException e) { noFile = true;                                                                          }
        StringBuilder missing = new StringBuilder();
        for (int i = 0; i < keys.length; i++) if (!got[i]) missing.append(missing.length() == 0 ? "" : ", ").append(keys[i]);
        if (noFile) log("app art: no " + name + " in the import pack (older pack): built-in colours used");
        else if (missing.length() > 0) log("app art: " + name + " has no valid colour for " + missing + ": built-in colours used for those");
        return out;
    }



    static final String[] UI_COLOURS = {"FontColorLightGold", "FontColorGoldYellow", "FontColorWhite", "FontColorGray", "FontColorLightGray", "FontColorDarkGrayGold"};
    private int[] ui;
    int uiColour(String name) {
        if (ui == null) ui = colours("ui_colors.txt", UI_COLOURS, new int[UI_COLOURS.length]);
        for (int i = 0; i < UI_COLOURS.length; i++) if (UI_COLOURS[i].equals(name)) return ui[i];
        throw new IllegalArgumentException(name);
    }








    static final String[] VERIFY_IMAGES = {
        "quick/panel", "quick/button_0", "quick/button_1", "quick/button_2", "quick/button_3", "quick/arrow_l_0", "quick/arrow_l_1", "quick/arrow_l_2", "quick/arrow_l_3",
        "quick/arrow_r_0", "quick/arrow_r_1", "quick/arrow_r_2", "quick/arrow_r_3", "quick/focus", "quick/divider", "quick/modal_bg",
        "pad/footer", "pad/focus", "pad/hold",
        "pad/switch/a", "pad/switch/b", "pad/switch/start", "pad/switch/left", "pad/switch/right", "pad/switch/y",
        "pad/xbox/a", "pad/xbox/b", "pad/xbox/start", "pad/xbox/left", "pad/xbox/right", "pad/xbox/y",
        "pad/ps5/a", "pad/ps5/b", "pad/ps5/start", "pad/ps5/left", "pad/ps5/right", "pad/ps5/y",
        "button", "button_down", "build_modal",

        "panel/frame", "panel/endcap_l", "panel/endcap_r", "panel/tab5_0", "panel/tab5_1", "panel/tab6_0", "panel/tab6_1", "panel/tab7_0", "panel/tab7_1",
        "panel/tabfocus5", "panel/tabfocus6", "panel/tabfocus7", "panel/foot_0", "panel/foot_1", "panel/foot_2", "panel/foot_3", "panel/close"};
    static final String[] VERIFY_FONTS = {"ExocetBlizzardOT-Medium.otf", "Formal436BT.ttf", "Philosopher-BoldItalic.ttf", "KoDia.ttf", "IrisL.ttf", "BlizzardGlobal-V5_81.ttf"};
    String verify() {
        for (String name : VERIFY_IMAGES) {
            Bitmap b = image(name);
            if (b == null || b.getWidth() <= 0 || b.getHeight() <= 0) return name + ".png";
        }
        for (int role : new int[]{EXOCET, FORMAL}) {
            File f = new File(root, "fonts/" + roleFile(role));
            if (!f.isFile()) return "fonts/" + roleFile(role);
            try {
                Typeface t = Typeface.createFromFile(f);
                if (t == null || t == Typeface.DEFAULT || t == Typeface.SERIF) return "fonts/" + roleFile(role);
                fonts.put("fonts/" + roleFile(role), t);
            } catch (RuntimeException e) { return "fonts/" + roleFile(role); }
        }
        for (String font : VERIFY_FONTS) if (!new File(root, "fonts/" + font).isFile()) return "fonts/" + font;
        try {
            String text = new String(Files.readAllBytes(new File(root, "ui_colors.txt").toPath()), StandardCharsets.UTF_8);
            for (String key : UI_COLOURS) {
                java.util.regex.Matcher m = java.util.regex.Pattern.compile("(?m)^" + key + " (\\d{1,3}) (\\d{1,3}) (\\d{1,3})\\s*$").matcher(text);
                if (!m.find() || Integer.parseInt(m.group(1)) > 255 || Integer.parseInt(m.group(2)) > 255 || Integer.parseInt(m.group(3)) > 255) return "ui_colors.txt " + key;
            }
        } catch (IOException e) { return "ui_colors.txt"; }

        InvPictures p = invPictures();
        if (p == null) return "inv.json";
        String inv = p.missing(n -> new File(root, "inv/" + n + ".png").length() > 0);
        if (inv != null) return inv;

        MonPictures m = monPictures();
        if (m == null) return "mon.json";
        String pics = m.missing(f -> new File(root, f).length() > 0);
        if (pics != null) return pics;


        return bookShape(new File(root, "book.json")) ? null : "book.json";
    }

    static final int BOOK_MAX = 4 * 1024 * 1024;
    static boolean bookShape(File f) {
        long n = f.length();
        if (n < 32 || n > BOOK_MAX) return false;
        try (java.io.RandomAccessFile r = new java.io.RandomAccessFile(f, "r")) {
            byte[] head = new byte[8], tail = new byte[8];
            r.readFully(head);
            r.seek(n - tail.length);
            r.readFully(tail);
            return new String(head, StandardCharsets.US_ASCII).equals("{\"game\":")
                    && new String(tail, StandardCharsets.US_ASCII).endsWith("\"v\":" + BookData.SCHEMA + "}");
        } catch (IOException e) { return false; }
    }



    private BookData book;
    private boolean bookRead;
    synchronized BookData book() {
        if (!bookRead) {
            bookRead = true;
            try {
                String t = text("book.json");
                if (t == null || t.length() > BOOK_MAX) throw new IllegalArgumentException("book.json missing or too large");
                long t0 = android.os.SystemClock.uptimeMillis();
                book = BookData.parse(t);
                log("book.json read in " + (android.os.SystemClock.uptimeMillis() - t0) + " ms");
            } catch (IllegalArgumentException e) {
                book = null;
                log("ASSUME book.tc: " + e.getMessage());
            }
        }
        return book;
    }


    private static native void nativeLog(String line);

    static void log(String line) { PackLog.tee(line); try { nativeLog(line); } catch (UnsatisfiedLinkError e) { android.util.Log.i("dm", line); } }



    Bitmap glyph(int style, String name) {
        return image("pad/" + (style == PadInput.GLYPH_XBOX ? "xbox" : style == PadInput.GLYPH_PS ? "ps5" : "switch") + "/" + name);
    }

    Typeface font(String name) {
        if (!fonts.containsKey(name)) {
            File f = new File(root, name);
            fonts.put(name, f.isFile() ? Typeface.createFromFile(f) : Typeface.SERIF);
        }
        return fonts.get(name);
    }




    String lang() { return GameStyle.lang(context); }


    Typeface role(int role) {
        String l = lang(), file = roleFile(role);
        if (new File(root, "fonts/" + file).isFile()) return font("fonts/" + file);


        return font(role == EXOCET && !l.equals("ko") && !l.equals("zh") ? "title.otf" : "text.ttf");
    }

    private String roleFile(int role) {
        switch (lang()) {
            case "ko": return role == EXOCET ? "KoDia.ttf" : "IrisL.ttf";
            case "zh": return "BlizzardGlobal-V5_81.ttf";
            case "ru": return role == EXOCET ? "ExocetBlizzardOT-Medium.otf" : "Philosopher-BoldItalic.ttf";
            default: return role == EXOCET ? "ExocetBlizzardOT-Medium.otf" : "Formal436BT.ttf";
        }
    }



    boolean hasRole(int role) { return new File(root, "fonts/" + roleFile(role)).isFile() || new File(root, role == EXOCET ? "title.otf" : "text.ttf").isFile(); }



    boolean nine(Canvas c, String name, RectF at, int inset, float scale, Paint p) {
        Bitmap b = image(name);
        if (b == null) return false;
        int w = b.getWidth(), h = b.getHeight(), ix = Math.min(inset, w / 2 - 1), iy = Math.min(inset, h / 2 - 1);
        float cx = Math.min(ix * scale, at.width() / 2), cy = Math.min(iy * scale, at.height() / 2);
        int[] sx = {0, ix, w - ix, w}, sy = {0, iy, h - iy, h};
        float[] dx = {at.left, at.left + cx, at.right - cx, at.right}, dy = {at.top, at.top + cy, at.bottom - cy, at.bottom};
        for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++) {
            if (dx[i + 1] <= dx[i] || dy[j + 1] <= dy[j]) continue;
            src.set(sx[i], sy[j], sx[i + 1], sy[j + 1]); dst.set(dx[i], dy[j], dx[i + 1], dy[j + 1]);
            c.drawBitmap(b, src, dst, p);
        }
        return true;
    }


    boolean rim(Canvas c, String name, RectF at, int inset, float scale, Paint p) {
        Bitmap b = image(name);
        if (b == null) return false;
        int w = b.getWidth(), h = b.getHeight(), ix = Math.min(inset, w / 2 - 1), iy = Math.min(inset, h / 2 - 1);
        float cx = Math.min(ix * scale, at.width() / 2), cy = Math.min(iy * scale, at.height() / 2);
        int[] sx = {0, ix, w - ix, w}, sy = {0, iy, h - iy, h};
        float[] dx = {at.left, at.left + cx, at.right - cx, at.right}, dy = {at.top, at.top + cy, at.bottom - cy, at.bottom};
        for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++) {
            if ((i == 1 && j == 1) || dx[i + 1] <= dx[i] || dy[j + 1] <= dy[j]) continue;
            src.set(sx[i], sy[j], sx[i + 1], sy[j + 1]); dst.set(dx[i], dy[j], dx[i + 1], dy[j + 1]);
            c.drawBitmap(b, src, dst, p);
        }
        return true;
    }






    boolean tab(Canvas c, String name, RectF at, int inset, int bottom, int mid, float scale, Paint p) {
        Bitmap b = image(name);
        if (b == null) return false;
        int w = b.getWidth(), h = b.getHeight();
        float ex = inset * scale, mx = mid * scale, ty = inset * scale, by = bottom * scale;
        if (at.width() < 2 * ex + mx + 2 || at.height() < ty + by + 2 || w < 2 * inset + mid + 2 || h < inset + bottom + 2) return nine(c, name, at, inset, scale, p);
        int m0 = (w - mid) / 2;
        int[] sx = {0, inset, m0, m0 + mid, w - inset, w}, sy = {0, inset, h - bottom, h};
        float cx = at.centerX();
        float[] dx = {at.left, at.left + ex, cx - mx / 2, cx + mx / 2, at.right - ex, at.right}, dy = {at.top, at.top + ty, at.bottom - by, at.bottom};
        for (int j = 0; j < 3; j++) for (int i = 0; i < 5; i++) {
            if (dx[i + 1] <= dx[i] || dy[j + 1] <= dy[j]) continue;
            src.set(sx[i], sy[j], sx[i + 1], sy[j + 1]); dst.set(dx[i], dy[j], dx[i + 1], dy[j + 1]);
            c.drawBitmap(b, src, dst, p);
        }
        return true;
    }


    boolean row(Canvas c, String name, RectF at, int end, Paint p) {
        Bitmap b = image(name);
        if (b == null) return false;
        int w = b.getWidth(), h = b.getHeight(), e = Math.min(end, w / 2 - 1);
        float k = at.height() / h, ex = Math.min(e * k, at.width() / 2);
        src.set(0, 0, e, h); dst.set(at.left, at.top, at.left + ex, at.bottom); c.drawBitmap(b, src, dst, p);
        src.set(e, 0, w - e, h); dst.set(at.left + ex, at.top, at.right - ex, at.bottom); c.drawBitmap(b, src, dst, p);
        src.set(w - e, 0, w, h); dst.set(at.right - ex, at.top, at.right, at.bottom); c.drawBitmap(b, src, dst, p);
        return true;
    }



    boolean focus(Canvas c, RectF at, float scale, Paint p) {
        Bitmap b = image("build_focus_0");
        if (b == null) return false;
        int n = b.getWidth();
        float s = Math.min(n * scale, Math.min(at.width(), at.height()) / 2);
        for (int k = 0; k < 4; k++) {
            boolean right = (k & 1) != 0, bottom = (k & 2) != 0;
            c.save();
            c.translate(right ? at.right : at.left, bottom ? at.bottom : at.top);
            c.scale(right ? -1 : 1, bottom ? -1 : 1);
            src.set(0, 0, n, n); dst.set(0, 0, s, s); c.drawBitmap(b, src, dst, p);
            if (!right) { src.set(n - 1, 0, n, n); dst.set(s, 0, at.width() - s, s); c.drawBitmap(b, src, dst, p); }
            if (!bottom) { src.set(0, n - 1, n, n); dst.set(0, s, s, at.height() - s); c.drawBitmap(b, src, dst, p); }
            c.restore();
        }
        return true;
    }
}
