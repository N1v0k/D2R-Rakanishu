package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.fonts.Font;
import android.graphics.fonts.FontFamily;
import android.graphics.text.PositionedGlyphs;
import android.graphics.text.TextRunShaper;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.HashMap;
import java.util.HashSet;






















final class GameStyle {
    static final int EXOCET = GameStyleData.EXOCET, FORMAL = GameStyleData.FORMAL, GLOBAL = GameStyleData.GLOBAL;
    static final int BUTTON_NORMAL = 0, BUTTON_DOWN = 1, BUTTON_DISABLED = 2, BUTTON_HOVER = 3;

    private static final String[] DMG_KEYS = {"physical", "FontColorWhite", "fire", "FontColorRed", "cold", "FontColorBlue", "lightning", "FontColorYellow",
        "poison", "FontColorGreen", "magic", "FontColorOrange"};
    private static final String[] BUTTONS = {"style/button_0", "style/button_1", "style/button_2", "style/button_3"};
    private final Context context;
    private final AppArt art;
    private final File root;
    private final GameStyleData data;


    private static final class Role {
        Typeface face; boolean loaded;
        final HashSet<String> allowed = new HashSet<>();
        final HashMap<String, Boolean> coverage = new HashMap<>();
    }
    private final Role[] roles = {new Role(), new Role(), new Role()};
    private final HashMap<String, Font> fonts = new HashMap<>();
    private final HashSet<String> logged = new HashSet<>();
    private final Paint probe = new Paint(), fill = new Paint(Paint.ANTI_ALIAS_FLAG), picture = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG),
        held = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final Rect src = new Rect();
    private final RectF dst = new RectF();
    private final float[] size2 = new float[2], corner = new float[3];
    private final float iconUnits, smallUnits;
    private GameText iconText;
    private GameText panelTab, panelSub, panelButton;
    private GameText panelLegend;
    private final GameStyleGeometry.Panel panel = new GameStyleGeometry.Panel();
    private final float[] box4 = new float[4];
    private final RectF pr = new RectF(), pa = new RectF();
    private String lang;
    private int generation, unknownLogged;

    GameStyle(Context context, AppArt art) {
        this.context = context;
        this.art = art;
        root = new File(context.getFilesDir(), "dm/appart");
        String tsv = read(new File(root, "style/style.tsv"));
        data = GameStyleData.parse(tsv);
        if (tsv == null) AppArt.log("game style: no style/style.tsv in the import pack (older pack): built-in style used");
        else if (data.bad() > 0) AppArt.log("game style: " + data.bad() + " broken lines in style/style.tsv: built-in values used for those");
        String dmg = read(new File(root, "dmg_colors.txt"));
        if (dmg != null) for (String line : dmg.split("\n")) {
            String[] f = line.trim().split("\\s+");
            if (f.length != 4) continue;
            for (int i = 0; i < DMG_KEYS.length; i += 2) {
                if (!DMG_KEYS[i].equals(f[0])) continue;
                try {
                    int r = Integer.parseInt(f[1]), g = Integer.parseInt(f[2]), b = Integer.parseInt(f[3]);
                    if ((r | g | b) >= 0 && r < 256 && g < 256 && b < 256) data.legacy(DMG_KEYS[i + 1], 0xFF000000 | r << 16 | g << 8 | b);
                } catch (NumberFormatException e) {                                              }
            }
        }
        lang = lang(context);
        GameStyleData.Counter c = data.counter();
        iconUnits = data.size(c.size);
        smallUnits = data.size("SmallFontSize");
        float[] t = data.tint("hover");
        ColorMatrix m = new ColorMatrix();
        if (t != null) m.setScale(t[0], t[1], t[2], t[3]);
        held.setColorFilter(new ColorMatrixColorFilter(m));
        prepare();
    }



    private void prepare() {
        for (int r = 0; r < roles.length; r++) face(r);
        art.image("style/slot");
        for (String b : BUTTONS) art.image(b);
        iconText = text(GameStyleData.ICON_NUMBER);
        iconText.prepare();
        panelTab = text(GameStyleData.PANEL_TAB); panelSub = text(GameStyleData.PANEL_SUBTAB); panelButton = text(GameStyleData.PANEL_BUTTON);
        panelLegend = text(GameStyleData.LEGEND);
        panelTab.prepare(); panelSub.prepare(); panelButton.prepare(); panelLegend.prepare();
        for (String n : PANEL_ART) art.image(n);
    }


    void prepareTabLabel(String label, float w, float h) {
        if (!(w > 0) || !(h > 0)) return;
        GameStyleGeometry.Panel p = panel(w, h);
        float k = p.s * GameStyleData.PANEL_EM;
        panelTab.prepareTabInk(label, (int) Math.floor(panelTab.size(k * p.minTabScale())), (int) Math.ceil(panelTab.size(k)));
        panelSub.prepareTabInk(label, (int) Math.floor(panelSub.size(k * MIN_TAB_K)), (int) Math.ceil(panelSub.size(k)));
    }

    private static String read(File f) {
        try { return f.isFile() && f.length() < (1 << 20) ? new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8) : null; }
        catch (IOException e) { return null; }
    }



    static String lang(Context context) { return AppText.get(context, R.string.dmAndroidStringsLang); }
    String lang() { return lang; }
    int generation() { return generation; }


    void refresh() {
        String l = lang(context);
        if (l.equals(lang)) return;
        lang = l;
        for (Role r : roles) { r.face = null; r.loaded = false; r.allowed.clear(); r.coverage.clear(); }
        generation++;
        prepare();
    }

    GameStyleData data() { return data; }


    int colour(String name) {
        int c = data.colour(name);
        if (data.unknown().size() != unknownLogged) { unknownLogged = data.unknown().size(); AppArt.log("game style: unknown colour names " + data.unknown() + ": FontColorWhite used"); }
        return c;
    }

    GameText text(GameStyleData.Preset p) {
        if (p == GameStyleData.ICON_NUMBER) {
            GameStyleData.Counter c = data.counter();
            p = new GameStyleData.Preset(p.name, p.face, c.colour, c.size, 0, p.shadow, c.spacing);
        }
        return new GameText(this, p);
    }



    private DotRows.Glyphs rowGlyphs;
    DotRows.Glyphs rowGlyphs() {
        if (rowGlyphs == null) rowGlyphs = new RowGlyphs();
        return rowGlyphs;
    }
    private final class RowGlyphs implements DotRows.Glyphs {
        private final GameText label = text(GameStyleData.TOOLTIP), dot = text(GameStyleData.TOOLTIP);
        RowGlyphs() { label.prepare(); dot.prepare(); }
        @Override public boolean ready() { return dot.ready(); }
        @Override public boolean covers(String s) { return GameStyle.this.covers(EXOCET, s); }
        @Override public float units() { return label.units(); }
        @Override public float measure(String s, float k) { return label.measure(s, k); }
        @Override public float baseline(float top, float bottom, float k) {
            Paint p = label.paint(k);
            return GameStyleGeometry.centreBaseline(top, bottom, p.ascent(), p.descent());
        }
        @Override public int generation() { return GameStyle.this.generation(); }
        @Override public int colour(String name) { return GameStyle.this.colour(name); }
        @Override public void drawLabel(Canvas c, String s, float x, float base, float k) { label.draw(c, s, x, base, k, Paint.Align.LEFT, 255); }
        @Override public void drawDot(Canvas c, String s, float x, float base, float k, int argb) { dot.colourArgb(argb); dot.draw(c, s, x, base, k, Paint.Align.LEFT, 255); }
        @Override public void backing(Canvas c, RectF box) { tooltip(c, box); }
    }


    Typeface face(int role) {
        if (!roles[role].loaded) load(role);
        return roles[role].face;
    }

    private void load(int role) {
        Role r = roles[role];
        r.loaded = true;
        Font global = font(data.font(lang, GLOBAL));
        if (global == null) global = font("BlizzardGlobal-V5_81.ttf");
        if (global == null) global = legacy("text.ttf");
        Font own = role == GLOBAL ? global : font(data.font(lang, role));
        if (own == null && role != GLOBAL) own = legacy(role == EXOCET ? "title.otf" : "text.ttf");
        if (own == null) own = global;
        if (own == null) { r.face = null; AppArt.log("game style: no readable game font for " + GameStyleData.ROLES[role] + " (" + lang + "): text of that kind is not drawn"); return; }
        Typeface.CustomFallbackBuilder b = new Typeface.CustomFallbackBuilder(new FontFamily.Builder(own).build());
        if (global != null && global != own) b.addCustomFallback(new FontFamily.Builder(global).build());
        r.face = b.build();
        r.allowed.add(path(own));
        if (global != null) r.allowed.add(path(global));
    }

    private Font font(String file) { return file == null ? null : load(new File(root, "fonts/" + file)); }
    private Font legacy(String file) { return load(new File(root, file)); }

    private Font load(File f) {
        String key = f.getAbsolutePath();
        if (fonts.containsKey(key)) return fonts.get(key);
        Font out = null;
        if (f.isFile()) {
            try { out = new Font.Builder(f).build(); }
            catch (IOException | RuntimeException e) { AppArt.log("game style: game font " + f.getName() + " unusable: " + e); }
        }
        fonts.put(key, out);
        return out;
    }

    private static String path(Font f) { File file = f.getFile(); return file == null ? "" : file.getAbsolutePath(); }



    boolean covers(int role, String s) {
        Typeface t = face(role);
        if (t == null || s == null) return false;
        if (s.isEmpty()) return true;
        HashMap<String, Boolean> coverage = roles[role].coverage;
        Boolean hit = coverage.get(s);
        if (hit != null) return hit;
        boolean ok = shaped(role, t, s);
        if (!ok && BuildConfig.DEBUG) {
            for (int i = 0; i < s.length(); ) {
                int cp = s.codePointAt(i), n = Character.charCount(cp);
                String one = s.substring(i, i + n);
                if (!shaped(role, t, one) && logged.add(role + ":" + cp)) AppArt.log(String.format("game style: %s has no game glyph for U+%04X '%s'", GameStyleData.ROLES[role], cp, one));
                i += n;
            }
        }
        if (coverage.size() >= 1024) coverage.clear();
        coverage.put(s, ok);
        return ok;
    }

    private boolean shaped(int role, Typeface t, String s) {
        probe.setTypeface(t);
        probe.setTextSize(32);
        try {
            PositionedGlyphs g = TextRunShaper.shapeTextRun(s, 0, s.length(), 0, s.length(), 0, 0, false, probe);
            for (int i = 0; i < g.glyphCount(); i++) {
                if (g.getGlyphId(i) == 0 || !roles[role].allowed.contains(path(g.getFont(i)))) return false;
            }
            return true;
        } catch (RuntimeException e) { return false; }
    }


    String pick(int role, String want, String instead) {
        if (covers(role, want)) return want;
        return covers(role, instead) ? instead : null;
    }



    void tooltip(Canvas c, RectF r) {
        fill.setColor(colour("Tooltip.backgroundColor"));
        c.drawRect(r, fill);
    }


    void tooltipBox(float textW, float textH, float k, RectF out) {
        GameStyleGeometry.tooltipBox(textW, textH, data.pad("tooltip"), k, size2);
        out.set(0, 0, size2[0], size2[1]);
    }






    void cornerNumber(Canvas c, char[] buf, int off, int len, float left, float top, float right, float bottom, float px, String colourName) {
        if (len <= 0 || iconText == null || !iconText.ready() || !(px > 0)) return;
        float w = iconText.measurePx(buf, off, len, px);
        GameStyleGeometry.corner(left, top, right, bottom, px, w, iconText.shadeRatio(), corner);
        if (!(corner[0] > 0)) return;
        iconText.colourArgb(colour(colourName));
        iconText.drawPx(c, buf, off, len, corner[1], corner[2], corner[0], Paint.Align.RIGHT, 255);
    }



    float counterPx(float field, float k) { return GameStyleGeometry.counterText(data.counter(), iconUnits, field, smallUnits, k); }



    void slot(Canvas c, RectF r, boolean pressed) {
        fill.setColor(colour("Tooltip.inGameBackgroundColor"));
        c.drawRect(r, fill);
        Bitmap b = art.image("style/slot");
        if (b != null) c.drawBitmap(b, null, r, pressed ? held : picture);
    }


    void slotPicture(Canvas c, Bitmap b, RectF r, boolean pressed) { if (b != null) c.drawBitmap(b, null, r, pressed ? held : picture); }




    boolean button(Canvas c, RectF r, int state) {
        Bitmap b = art.image(BUTTONS[Math.max(0, Math.min(3, state))]);
        if (b == null) return false;
        return art.row(c, BUTTONS[Math.max(0, Math.min(3, state))], r, b.getHeight() / 2, picture);
    }






    static final int ROW_LEFT = 90, ROW_RIGHT = 12;
    void listRow(Canvas c, RectF r) {
        Bitmap b = art.image("build_row_icon");
        if (b == null || b.getWidth() <= ROW_LEFT + ROW_RIGHT + 1) { tooltip(c, r); return; }
        capsAndTiles(c, b, r, ROW_LEFT, ROW_RIGHT);
    }






    static final int ROW_RIM_TOP = 5, ROW_RIM_BOTTOM = 4, ROW_RIM_LEFT = 1;
    void listRowTall(Canvas c, RectF r, float rowH) {
        Bitmap b = art.image("build_row_icon");
        int bw = b == null ? 0 : b.getWidth(), bh = b == null ? 0 : b.getHeight();
        if (b == null || bw <= ROW_LEFT + ROW_RIGHT + 1 || bh <= ROW_RIM_TOP + ROW_RIM_BOTTOM + 1 || rowH <= 0) { tooltip(c, r); return; }
        float s = rowH / bh;
        int[] sx = {0, ROW_RIM_LEFT, ROW_LEFT, bw - ROW_RIGHT, bw}, sy = {0, ROW_RIM_TOP, bh - ROW_RIM_BOTTOM, bh};
        float[] dx = {r.left, r.left + ROW_RIM_LEFT * s, r.right - ROW_RIGHT * s, r.right}, dy = {r.top, r.top + ROW_RIM_TOP * s, r.bottom - ROW_RIM_BOTTOM * s, r.bottom};
        for (int i = 0; i < 3; i++) {
            int x0 = i == 0 ? sx[0] : i == 1 ? sx[2] : sx[3], x1 = i == 0 ? sx[1] : i == 1 ? sx[3] : sx[4];
            for (int j = 0; j < 3; j++) tiles(c, b, x0, x1, sy[j], sy[j + 1], dx[i], dx[i + 1], dy[j], dy[j + 1], s);
        }
        float top = r.top + (r.height() - rowH) / 2 + ROW_RIM_TOP * s;
        src.set(ROW_RIM_LEFT, ROW_RIM_TOP, ROW_LEFT, bh - ROW_RIM_BOTTOM);
        dst.set(r.left + ROW_RIM_LEFT * s, top, r.left + ROW_LEFT * s, top + (bh - ROW_RIM_TOP - ROW_RIM_BOTTOM) * s);
        c.drawBitmap(b, src, dst, picture);
    }



    private void tiles(Canvas c, Bitmap b, int sx0, int sx1, int sy0, int sy1, float x0, float x1, float y0, float y1, float s) {
        float w = (sx1 - sx0) * s, h = (sy1 - sy0) * s;
        int nx = GameStyleGeometry.tiles(x0, x1, w), ny = GameStyleGeometry.tiles(y0, y1, h);
        for (int j = 0; j < ny; j++) {
            float a = y0 + j * h, e = Math.min(y1, a + h);
            int sb = Math.min(sy1, sy0 + Math.max(1, Math.round((e - a) / s)));
            for (int i = 0; i < nx; i++) {
                float l = x0 + i * w, rr = Math.min(x1, l + w);
                src.set(sx0, sy0, Math.min(sx1, sx0 + Math.max(1, Math.round((rr - l) / s))), sb);
                dst.set(l, a, rr, e);
                c.drawBitmap(b, src, dst, picture);
            }
        }
    }




    void inputField(Canvas c, RectF r) {
        Bitmap l = art.image("book_input_l"), m = art.image("book_input_c"), rt = art.image("book_input_r");
        if (l == null || m == null || rt == null) { tooltip(c, r); return; }
        float lw = l.getWidth() * r.height() / l.getHeight(), rw = rt.getWidth() * r.height() / rt.getHeight();
        dst.set(r.left, r.top, r.left + lw, r.bottom); c.drawBitmap(l, null, dst, picture);
        dst.set(r.right - rw, r.top, r.right, r.bottom); c.drawBitmap(rt, null, dst, picture);
        tile(c, m, 0, m.getWidth(), r.left + lw, r.right - rw, r.top, r.bottom);
    }



    private void capsAndTiles(Canvas c, Bitmap b, RectF r, int left, int right) {
        float s = r.height() / b.getHeight(), lw = Math.min(left * s, r.width() / 2), rw = Math.min(right * s, r.width() / 2);
        src.set(0, 0, left, b.getHeight()); dst.set(r.left, r.top, r.left + lw, r.bottom); c.drawBitmap(b, src, dst, picture);
        src.set(b.getWidth() - right, 0, b.getWidth(), b.getHeight()); dst.set(r.right - rw, r.top, r.right, r.bottom); c.drawBitmap(b, src, dst, picture);
        tile(c, b, left, b.getWidth() - right, r.left + lw, r.right - rw, r.top, r.bottom);
    }



    private void tile(Canvas c, Bitmap b, int sx0, int sx1, float x0, float x1, float top, float bottom) {
        float s = (bottom - top) / b.getHeight(), w = (sx1 - sx0) * s;
        int n = GameStyleGeometry.tiles(x0, x1, w);
        for (int i = 0; i < n; i++) {
            float a = x0 + i * w, e = Math.min(x1, a + w);
            src.set(sx0, 0, Math.min(sx1, sx0 + Math.max(1, Math.round((e - a) / s))), b.getHeight());
            dst.set(a, top, e, bottom);
            c.drawBitmap(b, src, dst, picture);
        }
    }



    private static final String[] PANEL_ART = {"panel/frame", "panel/endcap_l", "panel/endcap_r", "panel/close", "panel/tab5_0", "panel/tab5_1", "panel/tab6_0",
        "panel/tab6_1", "panel/tab7_0", "panel/tab7_1", "panel/foot_0", "panel/foot_1", "panel/foot_2", "panel/foot_3", "build_tab_on", "build_tab_off", "build_inset"};


    GameStyleGeometry.Panel panel(float w, float h) {
        panel.compute(w, h, data.scale(GameStyleData.CONSOLE), minTap());
        return panel;
    }

    float panelSmallPx(GameStyleGeometry.Panel p) { return p.smallPx(smallUnits); }

    float minTap() { return 7f / 25.4f * context.getResources().getDisplayMetrics().ydpi; }


    void backdrop(Canvas c) { fill.setColor(GameStyleGeometry.Panel.BACKDROP); c.drawPaint(fill); }




    void panelFrame(Canvas c, GameStyleGeometry.Panel p) {
        pr.set(p.frameL, p.frameT, p.frameR, p.frameB);
        picture(c, "panel/frame", pr);
        pr.set(p.endLl, p.endT, p.endLr, p.endB); picture(c, "panel/endcap_l", pr);
        pr.set(p.endRl, p.endT, p.endRr, p.endB); picture(c, "panel/endcap_r", pr);
        pr.set(p.contentL, p.contentT, p.contentR, p.contentB);
        fill.setColor(colour("Tooltip.backgroundColor"));
        c.drawRect(pr, fill);
        art.rim(c, "build_inset", pr, 8, 2 * p.s, picture);
    }


    void panelTab(Canvas c, GameStyleGeometry.Panel p, int n, int i, String text, boolean active) { panelTab(c, p, n, i, text, active, panelTabK(p, n, i, text)); }


    void panelTab(Canvas c, GameStyleGeometry.Panel p, int n, int i, String text, boolean active, float k) {
        p.tab(n, i, box4);
        pr.set(box4[0], box4[1], box4[2], box4[3]);
        picture(c, "panel/tab" + (n == 4 || n == 5 ? 5 : n <= 6 ? 6 : 7) + "_" + (active ? 1 : 0), pr);
        panelTab.colour(active ? GameStyleData.PANEL_TAB_ON : GameStyleData.PANEL_TAB_OFF);
        float k0 = p.s * GameStyleData.PANEL_EM;
        textIn(c, panelTab, text, pr.centerX(), p.tabBase - (panelTab.size(k0) - panelTab.size(k)) * GameStyleData.CAPS_EM / 2, (box4[2] - box4[0]) * TAB_ROOM, k, true);
    }





    float panelTabK(GameStyleGeometry.Panel p, int n, int i, String text) {
        float k = p.s * GameStyleData.PANEL_EM;
        if (!panelTab.ready()) return k;
        p.tab(n, i, box4);
        tabRoom[0] = (box4[2] - box4[0]) * TAB_ROOM; tabWidth[0] = panelTab.measure(text, k);
        return GameStyleGeometry.commonFit(tabWidth, tabRoom, 1, k, k * p.minTabScale());
    }


    boolean panelTabFits(GameStyleGeometry.Panel p, int n, int i, String text) {
        if (!panelTab.ready()) return true;
        p.tab(n, i, box4);
        return panelTab.measure(text, p.s * GameStyleData.PANEL_EM * p.minTabScale()) <= (box4[2] - box4[0]) * TAB_ROOM;
    }



    float subTabK(GameStyleGeometry.Panel p, float[] box, String text) {
        if (!panelSub.ready()) return p.s * GameStyleData.PANEL_EM;
        subText = text;
        return p.subTabK(box[2] - box[0], subWidth);
    }
    private String subText = "";
    private final GameStyleGeometry.Width subWidth = kk -> panelSub.measure(subText, kk);
    private static final float TAB_ROOM = 0.92f, SUB_ROOM = GameStyleGeometry.Panel.SUB_ROOM, MIN_TAB_K = GameStyleGeometry.Panel.MIN_TAB_K;
    private final float[] tabRoom = new float[1], tabWidth = new float[1];


    void closeX(Canvas c, GameStyleGeometry.Panel p) { pr.set(p.closeL, p.closeT, p.closeR, p.closeB); picture(c, "panel/close", pr); }


    void subTab(Canvas c, GameStyleGeometry.Panel p, float[] box, String text, boolean active) { subTab(c, p, box, text, active, subTabK(p, box, text)); }



    void subTab(Canvas c, GameStyleGeometry.Panel p, float[] box, String text, boolean active, float k) {
        pr.set(box[0], box[1], box[2], box[3]);
        if (!art.tab(c, active ? "build_tab_on" : "build_tab_off", pr, 6, 12, 16, 2 * p.s, picture)) tooltip(c, pr);
        panelSub.colour(active ? "TabsActiveTextColor" : "TabsInactiveTextColor");
        textIn(c, panelSub, text, pr.centerX(), p.subTabBase(box[1], panelSub.size(k) * GameStyleData.CAPS_EM), (box[2] - box[0]) * SUB_ROOM, k, true);
    }


    static final int PANEL_BUTTON_CAP = 34;




    void panelButton(Canvas c, RectF r, int state, String text) {
        art.row(c, "quick/button_" + Math.max(0, Math.min(3, state)), r, PANEL_BUTTON_CAP, picture);
        float k = r.height() / GameStyleGeometry.Panel.BUTTON_H * GameStyleData.PANEL_EM;
        panelButton.colour(state == BUTTON_DISABLED ? "FontColorGray" : "LightButtonTextColor");
        textIn(c, panelButton, text, r.centerX(), r.top + r.height() * GameStyleGeometry.Panel.BUTTON_BASE / GameStyleGeometry.Panel.BUTTON_H, r.width() * 0.86f, k);
    }


    void modalButton(Canvas c, RectF r, int state, String text) {
        String name = BUTTONS[Math.max(0, Math.min(3, state))];
        if (art.image(name) == null) name = state == BUTTON_DOWN ? "button_down" : "button";
        picture(c, name, r);
        float k = r.height() / GameStyleGeometry.Panel.MODAL_H * GameStyleData.PANEL_EM;
        panelButton.colour(state == BUTTON_DISABLED ? "FontColorGray" : "LightButtonTextColor");
        textIn(c, panelButton, text, r.centerX(), r.centerY() + panelButton.size(k) * GameStyleData.CAPS_EM / 2, r.width() * 0.86f, k);
    }



    void footButton(Canvas c, GameStyleGeometry.Panel p, float w, int col, int state, String text, int arrow) {
        p.foot(w, col, box4);
        pr.set(box4[0], box4[1], box4[2], box4[3]);
        int f = Math.max(0, Math.min(3, state));
        picture(c, "panel/foot_" + f, pr);
        if (arrow != 0) {
            float aw = 54 * p.k, ah = 44 * p.k, cx = pr.centerX(), cy = pr.centerY();
            pa.set(cx - aw / 2, cy - ah / 2, cx + aw / 2, cy + ah / 2);
            picture(c, "quick/arrow_" + (arrow < 0 ? "l" : "r") + "_" + (state == BUTTON_DISABLED ? 2 : state == BUTTON_DOWN ? 1 : 0), pa);
        }
        if (text != null) footText(c, p, w, col, text, state == BUTTON_DISABLED ? "FontColorGray" : "LightButtonTextColor");
    }


    void footText(Canvas c, GameStyleGeometry.Panel p, float w, int col, String text, String colourName) {
        p.foot(w, col, box4);
        panelButton.colour(colourName);
        textIn(c, panelButton, text, (box4[0] + box4[2]) / 2, p.footBase, (box4[2] - box4[0]) * 0.9f, p.k * GameStyleData.PANEL_EM);
    }


    void footLegend(Canvas c, GameStyleGeometry.Panel p, float w, Object... parts) {
        float h = p.footB - p.footT;
        Paint text = panelLegend.solid(panelLegend.units() * p.k, 255);
        float width = PadLegend.width(h, text, parts) + h * 2;
        pr.set(w / 2 - width / 2, p.footT, w / 2 + width / 2, p.footB);
        PadLegend.draw(c, pr, art.image("pad/footer"), picture, text, parts);
    }

    private void picture(Canvas c, String name, RectF r) { Bitmap b = art.image(name); if (b != null) c.drawBitmap(b, null, r, picture); }



    private void textIn(Canvas c, GameText t, String s, float x, float base, float max, float k) {
        textIn(c, t, s, x, base, max, k, false);
    }

    private void textIn(Canvas c, GameText t, String s, float x, float base, float max, float k, boolean tab) {
        if (s == null || s.isEmpty() || !t.ready() || !(max > 0)) return;
        float kk = tab ? (float) Math.floor(t.size(k)) / t.units() : t.fitTo(s, max, k);
        String fit = s;
        if (!tab && t.measure(fit, kk) > max) {
            int n = s.length();
            while (n > 0 && t.measure(s.substring(0, n).trim() + "...", kk) > max) n--;
            fit = n == 0 ? "" : s.substring(0, n).trim() + "...";
        }
        if (tab) base = GameStyleGeometry.fittedTabBaseline(base, t.size(k), t.size(kk)) + t.tabInkOffset(fit, kk);
        t.draw(c, fit, x, base, kk, Paint.Align.CENTER, 255);
    }



    void logCheck() {
        StringBuilder s = new StringBuilder("game style: ").append(data.empty() ? "built-in" : data.rows() + " rows").append(", lang ").append(lang);
        for (int r = 0; r < 3; r++) s.append(", ").append(GameStyleData.ROLES[r]).append(' ').append(face(r) == null ? "none" : data.font(lang, r));


        s.append(", exocet covers › ").append(covers(EXOCET, "›")).append(" → ").append(covers(EXOCET, "→")).append(" ⟲ ").append(covers(EXOCET, "⟲"));
        s.append(", slot ").append(art.image("style/slot") != null);
        AppArt.log(s.toString());
    }
}
