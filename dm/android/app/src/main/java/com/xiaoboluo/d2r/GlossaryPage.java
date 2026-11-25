package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import java.io.File;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONObject;











final class GlossaryPage extends View implements PadPage, QuickPage, StatusEffects.GlossaryLink {
    private static final int HIT_CLOSE = 0, HIT_SEARCH = 1, HIT_TAB = 2, HIT_ROW = 3, HIT_LPREV = 4, HIT_LNEXT = 5, HIT_EPREV = 6, HIT_ENEXT = 7, HIT_CLEAR = 8;
    private static final long POLL_MS = 300;
    private static final float LINE = 1.25f;

    private static final int SEARCH_SLOT = 4;

    private static final float MARGIN = 20, LIST = 0.30f;

    private final AppArt art;
    private final GameStyle style;
    private final Runnable freeInput;
    private final Paint pic = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), shade = new Paint();
    private final GameText title, head, body, value, note, rowOn, legend, input, mark;
    private final RectF frame = new RectF(), r = new RectF(), drawn = new RectF();
    private final float[] box4 = new float[4];
    private final List<RectF> hitRects = new ArrayList<>(), focusRects = new ArrayList<>();
    private final List<int[]> hits = new ArrayList<>();
    private final GameLang.Watch lang = new GameLang.Watch();
    private volatile GlossaryModel model;
    private volatile boolean broken;
    private volatile JSONObject effects, monsterData;
    private GlossaryCalc.Monsters monsters;
    private GlossaryCalc.Snap snap = new GlossaryCalc.Snap();
    private final int[] buf = new int[GlossaryCalc.INTS];
    private GlossaryLive.Context ctx;
    private boolean de = GameLang.german(), paused, polling, revealSel;
    private int cat = -1, sel, listPage, listPages = 1, entryPage, entryPages = 1, focusKind = -1, focusWhich, downHit = -1;
    private boolean downOutside;
    private String query = "", selId;
    private List<GlossaryModel.Term> shown = new ArrayList<>();
    private final SearchTyping typing = new SearchTyping(this, this::searched, this::qaSearch);
    private final Runnable poll = this::pollTick;

    GlossaryPage(Context context, AppArt art, GameStyle style, Runnable freeInput) {
        super(context);
        this.art = art;
        this.style = style;
        this.freeInput = freeInput;
        title = style.text(GameStyleData.TITLE);
        head = style.text(GameStyleData.BUTTON_LABEL).colour("FontColorGoldYellow");
        body = style.text(GameStyleData.BODY);
        value = style.text(GameStyleData.BODY).colour("FontColorBlue");
        note = style.text(GameStyleData.NOTE);
        rowOn = style.text(GameStyleData.BODY).colour("FontColorLightGold");
        legend = style.text(GameStyleData.LEGEND);
        input = style.text(GameStyleData.BOOK_INPUT);
        mark = style.text(GameStyleData.BUTTON_LABEL);
        shade.setColor(0xA0000000);
        setVisibility(GONE);
        new Thread(this::load, "glossary").start();
    }


    private void load() {
        try (InputStream in = getContext().getAssets().open("glossary/terms.json")) {
            java.io.ByteArrayOutputStream out = new java.io.ByteArrayOutputStream();
            byte[] b = new byte[16384];
            for (int n; (n = in.read(b)) > 0; ) { out.write(b, 0, n); if (out.size() > (1 << 20)) throw new java.io.IOException("too big"); }
            GlossaryModel m = parse(new JSONObject(new String(out.toByteArray(), StandardCharsets.UTF_8)));
            if (m.cats.size() + 1 > SEARCH_SLOT) throw new java.io.IOException(m.cats.size() + " categories, the tab strip has room for " + (SEARCH_SLOT - 1));
            model = m;
        } catch (Exception e) { broken = true; AppArt.log("glossary: terms.json not read (" + e + ")"); }
        readPack();
    }



    private void readPack() {
        File dir = new File(getContext().getFilesDir(), "dm/appart");
        effects = readJson(new File(dir, "effects.json"));
        monsterData = readJson(new File(dir, "glossary.json"));
    }
    private static JSONObject readJson(File f) {
        try { return f.isFile() && f.length() < (4 << 20) ? new JSONObject(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8)) : null; }
        catch (Exception e) { return null; }
    }

    private static GlossaryModel.Text text(JSONObject o) { return o == null ? new GlossaryModel.Text("", "") : new GlossaryModel.Text(o.optString("de"), o.optString("en")); }
    private static String[] strings(JSONArray a) { String[] s = new String[a == null ? 0 : a.length()]; for (int i = 0; i < s.length; i++) s[i] = a.optString(i); return s; }
    private static int[][] steps(JSONArray a) {
        if (a == null) return null;
        int[][] t = new int[a.length()][];
        for (int i = 0; i < t.length; i++) { JSONArray p = a.optJSONArray(i); t[i] = new int[]{p == null ? 0 : p.optInt(0), p == null ? 0 : p.optInt(1)}; }
        return t;
    }

    static GlossaryModel parse(JSONObject d) {
        GlossaryModel m = new GlossaryModel();
        m.gameVersion = d.optString("gameVersion");
        JSONArray cats = d.optJSONArray("categories");
        for (int i = 0; cats != null && i < cats.length(); i++) { JSONObject c = cats.optJSONObject(i); m.catIds.add(c.optString("id")); m.cats.add(text(c)); }
        JSONObject src = d.optJSONObject("sources");
        for (Iterator<String> it = src == null ? null : src.keys(); it != null && it.hasNext(); ) {
            String k = it.next(); JSONObject o = src.optJSONObject(k);
            GlossaryModel.Source s = new GlossaryModel.Source(); s.id = k; s.site = o.optString("site"); s.title = o.optString("title"); s.url = o.optString("url"); s.seen = o.optString("seen");
            m.sources.put(k, s);
        }
        JSONObject ph = d.optJSONObject("phrases");
        for (Iterator<String> it = ph == null ? null : ph.keys(); it != null && it.hasNext(); ) { String k = it.next(); m.phrases.put(k, text(ph.optJSONObject(k))); }
        JSONObject bp = d.optJSONObject("breakpoints");
        for (String kind : new String[]{"fcr", "fhr"}) {
            JSONObject b = bp == null ? null : bp.optJSONObject(kind), cl = b == null ? null : b.optJSONObject("classes");
            for (int c = 0; c < GlossaryModel.CLASSES.length; c++) (kind.equals("fcr") ? m.fcr : m.fhr)[c] = cl == null ? null : steps(cl.optJSONArray(GlossaryModel.CLASSES[c]));
        }
        JSONArray terms = d.optJSONArray("terms");
        for (int i = 0; terms != null && i < terms.length(); i++) {
            JSONObject o = terms.optJSONObject(i);
            GlossaryModel.Term t = new GlossaryModel.Term();
            t.id = o.optString("id"); t.cat = o.optString("cat");
            JSONObject n = o.optJSONObject("name");
            if (n != null) { t.nameStat = n.optInt("stat", -1); t.nameGroup = n.optInt("group", -1); t.nameWord = n.has("word") ? n.optString("word") : null; }
            t.fallback = text(o.optJSONObject("fallback"));
            t.keywords = strings(o.optJSONArray("keywords")); t.sources = strings(o.optJSONArray("sources"));
            JSONArray st = o.optJSONArray("stats"); t.stats = new int[st == null ? 0 : st.length()]; for (int k = 0; k < t.stats.length; k++) t.stats[k] = st.optInt(k);
            JSONObject x = o.optJSONObject("text");
            if (x != null) { t.what = text(x.optJSONObject("what")); t.how = text(x.optJSONObject("how")); t.helps = text(x.optJSONObject("helps")); t.not = text(x.optJSONObject("not")); t.rule = text(x.optJSONObject("rule")); }
            t.limits = text(o.optJSONObject("limits"));
            m.terms.add(t);
        }
        return m;
    }


    private final GlossaryModel.Names names = new GlossaryModel.Names() {
        public String stat(int id, boolean german) {
            JSONObject e = effects, s = e == null ? null : e.optJSONObject("stats"), f = s == null ? null : s.optJSONObject(Integer.toString(id));
            return f == null ? null : f.optString(german ? "de" : "en", null);
        }
        public String group(int stat, boolean german) {
            JSONObject e = effects; JSONArray g = e == null ? null : e.optJSONArray("groups");
            for (int i = 0; g != null && i < g.length(); i++) {
                JSONObject f = g.optJSONObject(i); JSONArray ids = f == null ? null : f.optJSONArray("ids");
                for (int k = 0; ids != null && k < ids.length(); k++) if (ids.optInt(k, -1) == stat) return f.optString(german ? "de" : "en", null);
            }
            return null;
        }
        public String word(String key, boolean german) {
            JSONObject e = effects, w = e == null ? null : e.optJSONObject("words"); JSONArray a = w == null ? null : w.optJSONArray(key);
            return a == null ? null : a.optString(german ? 0 : 1, null);
        }
    };

    private final GlossaryLive.Game game = new GlossaryLive.Game() {
        public int penalty(int diff, boolean classic) {
            JSONObject e = effects; JSONArray p = e == null ? null : e.optJSONArray("penalty"), row = p == null ? null : p.optJSONArray(diff);
            return row == null || row.length() < 2 ? Integer.MIN_VALUE : row.optInt(classic ? 1 : 0, Integer.MIN_VALUE);
        }
        public String resistName(int stat, boolean german) { String w = names.word("res" + stat, german); return w == null ? null : GlossaryModel.clean(w); }
    };

    private GlossaryCalc.Monsters monsters() {
        if (monsters != null) return monsters;
        JSONObject d = monsterData;
        if (d == null) return null;
        GlossaryCalc.Monsters m = new GlossaryCalc.Monsters();
        String[] keys = {"classic", "expansion"};
        for (int e = 0; e < 2; e++) {
            JSONObject o = d.optJSONObject(keys[e]);
            m.ar[e] = table(o == null ? null : o.optJSONArray("ar"));
            m.def[e] = table(o == null ? null : o.optJSONArray("def"));
        }
        m.leech = table(d.optJSONArray("leech"));
        return monsters = m;
    }
    private static int[][] table(JSONArray a) {
        if (a == null) return null;
        int[][] t = new int[a.length()][];
        for (int i = 0; i < t.length; i++) { JSONArray r = a.optJSONArray(i); t[i] = new int[r == null ? 0 : r.length()]; for (int k = 0; k < t[i].length; k++) t[i][k] = r.optInt(k); }
        return t;
    }


    @Override public String linkLabel(int stat) {
        GlossaryModel m = model;
        GlossaryModel.Term t = m == null ? null : m.byStat(stat);
        if (t == null) return null;
        boolean g = GameLang.german();
        return AppText.get(getContext(), R.string.dmAndroidGlossLink, GlossaryModel.name(t, names, g));
    }
    @Override public String linkTerm(int stat) { GlossaryModel m = model; GlossaryModel.Term t = m == null ? null : m.byStat(stat); return t == null ? null : t.id; }
    @Override public void openFromCard(int state, int stat, int amount) {
        GlossaryModel m = model;
        GlossaryModel.Term t = m == null ? null : m.byStat(stat);
        if (t == null) return;
        ctx = new GlossaryLive.Context(state, stat, amount);
        openAt(t.id);
    }


    @Override public boolean available() { return model != null && !broken; }
    @Override public void takePause() { if (!paused) { paused = true; DmActivity.nativePause(DmActivity.PAUSE_GLOSSARY, 1); } }
    @Override public void open() { ctx = null; openAt(null); }

    private void openAt(String termId) {
        DmActivity.syncLang();
        de = GameLang.german();
        takePause();
        if (effects == null || monsterData == null) readPack();
        monsters = null;
        typing.end(); typing.setText("");
        cat = -1; query = ""; listPage = 0; entryPage = 0; focusKind = -1;
        refilter();
        if (termId != null) { for (int i = 0; i < shown.size(); i++) if (shown.get(i).id.equals(termId)) select(i); revealSel = true; }
        else select(0);
        DmActivity.nativeGlossaryWant(1);
        snap = new GlossaryCalc.Snap();
        polling = true; removeCallbacks(poll); post(poll);
        setVisibility(VISIBLE);
        freeInput.run();
        invalidate();
    }

    boolean isOpen() { return getVisibility() == VISIBLE; }

    void close() {
        typing.end();
        polling = false; removeCallbacks(poll);
        DmActivity.nativeGlossaryWant(0);
        if (paused) { paused = false; DmActivity.nativePause(DmActivity.PAUSE_GLOSSARY, 0); }
        ctx = null;
        setVisibility(GONE);
    }

    void stop() { if (isOpen()) close(); }

    private void pollTick() {
        if (!polling) return;
        int n = DmActivity.nativeGlossary(buf);
        GlossaryCalc.Snap s = GlossaryCalc.Snap.parse(buf, n);
        if (s.valid != snap.valid || s.gen != snap.gen) { snap = s; invalidate(); }
        postDelayed(poll, POLL_MS);
    }

    private void refilter() {
        GlossaryModel m = model;
        shown = m == null ? new ArrayList<>() : m.filter(cat < 0 || cat >= m.catIds.size() ? null : m.catIds.get(cat), query, names);
    }
    private void select(int i) {
        if (shown.isEmpty()) { sel = 0; selId = null; return; }
        sel = Math.max(0, Math.min(shown.size() - 1, i));
        String id = shown.get(sel).id;
        if (!id.equals(selId)) { entryPage = 0; if (ctx != null && !id.equals(model.byStat(ctx.stat) == null ? null : model.byStat(ctx.stat).id)) ctx = null; }
        selId = id;
    }
    private void setCat(int c) {
        GlossaryModel m = model;
        int n = m == null ? 0 : m.catIds.size();
        cat = c < -1 ? n - 1 : c >= n ? -1 : c;
        listPage = 0; refilter(); select(0); invalidate();
    }
    private void setQuery(String q) { typing.setText(q); searched(); }
    private void searched() { query = typing.text().trim(); listPage = 0; refilter(); select(0); invalidate(); }


    private void beginEditing() { if (isOpen() && model != null) typing.begin(); }

    @Override public WindowInsets onApplyWindowInsets(WindowInsets in) {
        typing.insets(in);
        return super.onApplyWindowInsets(in);
    }
    @Override public boolean onCheckIsTextEditor() { return typing.editing(); }
    @Override public InputConnection onCreateInputConnection(EditorInfo out) { return typing.connection(out); }
    @Override public boolean onKeyDown(int keyCode, KeyEvent e) { return typing.key(e) || super.onKeyDown(keyCode, e); }
    @Override public boolean onKeyUp(int keyCode, KeyEvent e) { return typing.key(e) || super.onKeyUp(keyCode, e); }
    @Override public boolean takesKey(KeyEvent e) { return typing.takesKey(e); }
    @Override public boolean dispatchKeyEventPreIme(KeyEvent e) { return typing.preIme(e) || super.dispatchKeyEventPreIme(e); }


    private float gamePx;
    private void hit(RectF at, int kind, int which) { hit(at, at, kind, which); }
    private void hit(RectF at, RectF drawn, int kind, int which) {
        hitRects.add(new RectF(at)); focusRects.add(new RectF(drawn)); hits.add(new int[]{kind, which});
        if (BuildConfig.DM_QA_BUILD) TestHooks.hit(at, kind, which);
    }
    private void image(Canvas c, String name, RectF at) { Bitmap b = art.image(name); if (b != null) c.drawBitmap(b, null, at, pic); }
    private float base(GameText t, float top, float bottom, float k) { Paint p = t.paint(k); return GameStyleGeometry.centreBaseline(top, bottom, p.ascent(), p.descent()); }
    private String s(int id) { return AppText.get(getContext(), id); }


    private void fitText(Canvas c, GameText t, String s, float x, float top, float bottom, float max, float k, Paint.Align a) { fitText(c, t, s, x, top, bottom, max, k, a, 255); }
    private void fitText(Canvas c, GameText t, String s, float x, float top, float bottom, float max, float k, Paint.Align a, int alpha) {
        float kk = t.fitTo(s, max, k);
        String out = s;
        while (out.length() > 1 && t.measure(out, kk) > max) out = out.substring(0, out.length() - 2) + "…";
        t.draw(c, out, x, base(t, top, bottom, kk), kk, a, alpha);
    }


    private static final class Row {
        final GameText kind; final String text; final GlossaryLive.Line live; final float gapBefore;
        Row(GameText kind, String text, GlossaryLive.Line live, float gapBefore) { this.kind = kind; this.text = text; this.live = live; this.gapBefore = gapBefore; }
    }

    @Override protected void onDraw(Canvas c) {
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "glossary");
        try {
        if (lang.changed()) { de = GameLang.german(); style.refresh(); }
        hitRects.clear(); focusRects.clear(); hits.clear();
        style.backdrop(c);
        GameStyleGeometry.Panel p = style.panel(getWidth(), getHeight());
        style.panelFrame(c, p);
        frame.set(p.frameL, p.frameT, p.frameR, p.frameB);
        gamePx = 2 * p.k;
        float g = gamePx, k = g, m = MARGIN * p.s;
        GlossaryModel md = model;
        if (md != null) drawStrip(c, md, p, m, k);
        style.closeX(c, p);
        r.set(p.hitCloseL, p.hitCloseT, p.hitCloseR, p.hitCloseB); drawn.set(p.closeL, p.closeT, p.closeR, p.closeB); hit(r, drawn, HIT_CLOSE, 0);
        if (md == null) {
            body.draw(c, broken ? s(R.string.dmAndroidGlossNoData) : "…", (p.contentL + p.contentR) / 2, (p.contentT + p.contentB) / 2, k, Paint.Align.CENTER, 255);
            drawFocus(c); return;
        }
        float top = p.contentT + m, bot = p.contentB - m, split = p.contentL + LIST * (p.contentR - p.contentL);
        drawLeft(c, md, p.contentL + m, split, top, bot, g, k);
        drawRight(c, md, split + 24 * g, p.contentR - m, top, bot, g, k);
        drawFocus(c);
        if (PadInput.isPadMode()) style.footLegend(c, p, getWidth(), legendParts());
        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }



    private void drawStrip(Canvas c, GlossaryModel md, GameStyleGeometry.Panel p, float m, float k) {
        for (int i = 0; i <= md.cats.size(); i++) {
            int id = i - 1;
            style.panelTab(c, p, 6, i, id < 0 ? s(R.string.dmAndroidGlossAll) : md.cats.get(id).get(de), id == cat);
            p.tab(6, i, box4); drawn.set(box4[0], box4[1], box4[2], box4[3]);
            p.tabHit(6, i, box4); r.set(box4[0], box4[1], box4[2], box4[3]);
            hit(r, drawn, HIT_TAB, id);
        }
        p.searchField(SEARCH_SLOT, m / 2, box4);
        float x0 = box4[0], top = box4[1], x1 = box4[2], bottom = box4[3], rowH = bottom - top, pad = rowH * 0.4f;
        drawn.set(x0, top, x1, bottom);
        style.inputField(c, drawn);
        r.set(x0, 0, x1, p.hitTabBottom); hit(r, drawn, HIT_SEARCH, 0);
        float ki = body.size(k) / input.units(), room = x1 - x0 - 2 * pad - rowH;
        String text = typing.text();
        if (text.isEmpty() && !typing.editing()) fitText(c, input, s(R.string.dmAndroidGlossSearch), x0 + pad, top, bottom, room, ki, Paint.Align.LEFT, GameStyleData.HINT_ALPHA);
        else {
            if (typing.caretOn()) text = text + "|";
            while (text.length() > 1 && input.measure(text, ki) > room) text = text.substring(1);
            fitText(c, input, text, x0 + pad, top, bottom, room, ki, Paint.Align.LEFT);
        }
        if (query.isEmpty() && typing.isEmpty()) return;
        fitText(c, mark, "×", x1 - pad - rowH * 0.25f, top, bottom, rowH, ki, Paint.Align.CENTER);
        drawn.set(x1 - pad - rowH * 0.5f, top, x1 - pad, bottom);
        r.set(x1 - pad - rowH * 0.8f, 0, x1, p.hitTabBottom); hit(r, drawn, HIT_CLEAR, 0);
    }

    private void column(Canvas c, float x0, float y0, float x1, float y1, float g) {
        r.set(x0, y0, x1, y1);
        c.drawRect(r, shade);
        art.rim(c, "build_inset", new RectF(r), 8, g, pic);
    }

    private void drawLeft(Canvas c, GlossaryModel md, float x0, float x1, float top, float bot, float g, float k) {
        column(c, x0, top, x1, bot, g);
        float in = 14 * g;


        float lineH = body.size(k) * LINE, rowsTop = top + in, pagerH = 44 * g, pitch = 2 * lineH + 18 * g;
        int per = Math.max(1, (int) ((bot - pagerH - rowsTop) / pitch));
        listPages = Math.max(1, (shown.size() + per - 1) / per);
        if (revealSel) { listPage = sel / per; revealSel = false; }
        listPage = Math.max(0, Math.min(listPages - 1, listPage));
        if (shown.isEmpty()) body.draw(c, s(R.string.dmAndroidGlossNone), (x0 + x1) / 2, rowsTop + pitch / 2, k, Paint.Align.CENTER, 255);
        for (int i = listPage * per; i < Math.min(shown.size(), (listPage + 1) * per); i++) {
            float y = rowsTop + (i - listPage * per) * pitch;
            r.set(x0 + in, y, x1 - in, y + pitch - 6 * g);
            boolean on = i == sel;
            if (on) art.row(c, "build_row", new RectF(r), 18, pic); else art.nine(c, "build_inset", new RectF(r), 8, g, pic);
            hit(r, HIT_ROW, i);
            nameLines(c, on ? rowOn : body, GlossaryModel.name(shown.get(i), names, de), r.left + 24 * g, r, r.width() - 40 * g, lineH, k);
        }
        pager(c, x0, x1, bot - pagerH, bot - 6 * g, listPage, listPages, HIT_LPREV, HIT_LNEXT, k);
    }


    private void nameLines(Canvas c, GameText t, String name, float x, RectF row, float max, float lineH, float k) {
        List<String> lines = EffectLayout.wrap(name, max, s -> t.measure(s, k));
        if (lines.size() > 2) lines = java.util.Arrays.asList(lines.get(0), String.join(" ", lines.subList(1, lines.size())));
        float y = row.centerY() - lines.size() * lineH / 2;
        for (String line : lines) { fitText(c, t, line, x, y, y + lineH, max, k, Paint.Align.LEFT); y += lineH; }
    }




    private void pager(Canvas c, float x0, float x1, float y0, float y1, int page, int pages, int prev, int next, float k) {
        if (pages <= 1) return;
        String s = pagerText(page, pages);
        legend.draw(c, s, (x0 + x1) / 2, base(legend, y0, y1, k), k, Paint.Align.CENTER, 255);
        GameStyleGeometry.pagerHits(x0, x1, y0, y1, legend.measure(s, k), legend.measure("‹", k), legend.measure("›", k), style.minTap(), y0, getWidth(), getHeight(), pagePrev, pageNext);
        float mid = (x0 + x1) / 2;
        r.set(pagePrev[0], pagePrev[1], pagePrev[2], pagePrev[3]); pageDrawn.set(x0, y0, mid, y1); hit(r, pageDrawn, prev, 0);
        r.set(pageNext[0], pageNext[1], pageNext[2], pageNext[3]); pageDrawn.set(mid, y0, x1, y1); hit(r, pageDrawn, next, 0);
    }
    private static String pagerText(int page, int pages) { return "‹   " + (page + 1) + " / " + pages + "   ›"; }
    private final float[] pagePrev = new float[4], pageNext = new float[4];
    private final RectF pageDrawn = new RectF();

    private List<Row> entryRows(GlossaryModel md, GlossaryModel.Term t) {
        List<Row> out = new ArrayList<>();
        float g = gamePx;
        String f = GlossaryModel.format(t, names, de);
        if (f != null) out.add(new Row(note, AppText.get(getContext(), R.string.dmAndroidGlossInGame, GlossaryModel.shown(f)), null, 0));
        out.add(new Row(head, s(R.string.dmAndroidGlossForYou), null, 18 * g));
        for (GlossaryLive.Line l : new GlossaryLive(md, de).lines(t, snap, monsters(), game, ctx)) out.add(new Row(body, null, l, 0));
        if (!t.limits.get(de).isEmpty()) out.add(new Row(note, t.limits.get(de), null, 4 * g));
        int[] heads = {R.string.dmAndroidGlossWhat, R.string.dmAndroidGlossHow, R.string.dmAndroidGlossHelps, R.string.dmAndroidGlossNot, R.string.dmAndroidGlossRule};
        GlossaryModel.Text[] texts = {t.what, t.how, t.helps, t.not, t.rule};
        for (int i = 0; i < heads.length; i++) {
            if (texts[i] == null || texts[i].get(de).isEmpty()) continue;
            out.add(new Row(head, s(heads[i]), null, 18 * g));
            out.add(new Row(body, texts[i].get(de), null, 0));
        }
        return out;
    }

    private String sourcesLine(GlossaryModel md, GlossaryModel.Term t) {
        StringBuilder b = new StringBuilder();
        List<String> sites = new ArrayList<>();
        for (String id : t.sources) { GlossaryModel.Source s = md.sources.get(id); if (s != null && !sites.contains(s.site)) sites.add(s.site); }
        b.append(s(R.string.dmAndroidGlossSources)).append(": ").append(String.join(", ", sites)).append(" · ").append(AppText.get(getContext(), R.string.dmAndroidGlossChecked, md.gameVersion));
        return b.toString();
    }

    private void drawRight(Canvas c, GlossaryModel md, float x0, float x1, float top, float bot, float g, float k) {
        column(c, x0, top, x1, bot, g);
        if (shown.isEmpty()) return;
        GlossaryModel.Term t = shown.get(Math.max(0, Math.min(shown.size() - 1, sel)));
        float x = x0 + 36 * g, max = x1 - x0 - 72 * g;
        fitText(c, title, GlossaryModel.name(t, names, de), x, top + 14 * g, top + 64 * g, max, k * 0.9f, Paint.Align.LEFT);
        r.set(x - 10 * g, top + 70 * g, x1 - 26 * g, top + 74 * g); image(c, "build_divider", r);


        float pagerR = x1 - 26 * g, pagerL = pagerR - legend.measure(pagerText(8, 9), k), noteH = note.size(k) * LINE;
        List<String> sources = EffectLayout.wrap(sourcesLine(md, t), pagerL - 16 * g - x, s -> note.measure(s, k));
        if (sources.size() > 2) sources = java.util.Arrays.asList(sources.get(0), String.join(" ", sources.subList(1, sources.size())));
        float y0 = top + 84 * g, footH = Math.max(44 * g, sources.size() * noteH + 6 * g), y1 = bot - footH;

        List<Object[]> lines = new ArrayList<>();
        for (Row row : entryRows(md, t)) {
            float lh = row.kind.size(k) * LINE;
            String whole = row.live != null ? row.live.text() : row.text;
            List<String> wrapped = EffectLayout.wrap(whole, max, s -> row.kind.measure(s, k));
            int offset = 0;
            for (int i = 0; i < wrapped.size(); i++) {
                String part = wrapped.get(i);
                int start = whole.indexOf(part, offset);
                Object what = row.live != null && start >= 0 ? row.live.slice(start, start + part.length()) : part;
                if (start >= 0) offset = start + part.length();
                lines.add(new Object[]{row.kind, what, i == 0 ? row.gapBefore : 0f, lh});
            }
        }
        List<Integer> starts = new ArrayList<>();
        float y = 0; starts.add(0);
        for (int i = 0; i < lines.size(); i++) {
            float need = (float) lines.get(i)[2] + (float) lines.get(i)[3];
            if (y + need > y1 - y0 && y > 0) {
                int at = i;
                if (at - 1 > starts.get(starts.size() - 1) && lines.get(at - 1)[0] == head) { at--; need = (float) lines.get(at)[3] + (float) lines.get(at + 1)[2] + (float) lines.get(at + 1)[3]; }
                else need = (float) lines.get(i)[3];
                starts.add(at); y = 0;
            }
            y += need;
        }
        entryPages = starts.size();
        entryPage = Math.max(0, Math.min(entryPages - 1, entryPage));
        int from = starts.get(entryPage), to = entryPage + 1 < starts.size() ? starts.get(entryPage + 1) : lines.size();
        y = y0;
        for (int i = from; i < to; i++) {
            Object[] l = lines.get(i);
            GameText kind = (GameText) l[0];
            if (i > from) y += (float) l[2];
            float lh = (float) l[3], bl = base(kind, y, y + lh, k);
            if (l[1] instanceof GlossaryLive.Line) {
                GlossaryLive.Line v = (GlossaryLive.Line) l[1];
                float xx = x;
                kind.draw(c, v.head, xx, bl, k, Paint.Align.LEFT, 255); xx += kind.measure(v.head, k);
                value.colour(v.colour).draw(c, v.value, xx, bl, k, Paint.Align.LEFT, 255); xx += value.measure(v.value, k);
                kind.draw(c, v.tail, xx, bl, k, Paint.Align.LEFT, 255);
            } else kind.draw(c, (String) l[1], x, bl, k, Paint.Align.LEFT, 255);
            y += lh;
        }
        float sy = bot - 6 * g - sources.size() * noteH;
        for (String line : sources) { fitText(c, note, line, x, sy, sy + noteH, pagerL - 16 * g - x, k, Paint.Align.LEFT); sy += noteH; }
        pager(c, pagerL, pagerR, bot - 44 * g, bot - 6 * g, entryPage, entryPages, HIT_EPREV, HIT_ENEXT, k);
    }

    private int focusIndex() {
        for (int i = 0; i < hits.size() && focusKind >= 0; i++) if (hits.get(i)[0] == focusKind && hits.get(i)[1] == focusWhich) return i;
        return -1;
    }

    private void drawFocus(Canvas c) {
        int f = focusIndex();
        if (f < 0) return;
        RectF at = new RectF(focusRects.get(f)); float gg = gamePx * 6;
        at.inset(-gg, -gg);
        art.focus(c, at, gamePx * 0.9f, pic);
    }


    private Object[] legendParts() {
        int gl = PadInput.glyph();
        return new Object[]{art.glyph(gl, "a"), s(R.string.dmAndroidQuickLegendSelect), null, art.glyph(gl, "b"), s(R.string.dmAndroidBack), null, art.glyph(gl, "start"), s(R.string.dmAndroidQuickClose)};
    }


    private int hitAt(float x, float y) { for (int i = hitRects.size() - 1; i >= 0; i--) if (hitRects.get(i).contains(x, y)) return i; return -1; }

    private void act(int[] h) {
        if (typing.editing() && h[0] != HIT_SEARCH && h[0] != HIT_CLEAR) typing.end();
        switch (h[0]) {
            case HIT_CLOSE: close(); return;
            case HIT_SEARCH: beginEditing(); return;
            case HIT_CLEAR: setQuery(""); return;
            case HIT_TAB: setCat(h[1]); return;
            case HIT_ROW: select(h[1]); break;
            case HIT_LPREV: listPage = Math.max(0, listPage - 1); break;
            case HIT_LNEXT: listPage = Math.min(listPages - 1, listPage + 1); break;
            case HIT_EPREV: entryPage = Math.max(0, entryPage - 1); break;
            case HIT_ENEXT: entryPage = Math.min(entryPages - 1, entryPage + 1); break;
            default: break;
        }
        invalidate();
    }



    @Override public boolean onTouchEvent(MotionEvent e) {
        int a = e.getActionMasked();
        float x = e.getX(), y = e.getY();
        if (a == MotionEvent.ACTION_DOWN) { downHit = hitAt(x, y); downOutside = downHit < 0 && !frame.contains(x, y); focusKind = -1; invalidate(); }
        else if (a == MotionEvent.ACTION_UP) {
            int at = hitAt(x, y);
            if (downHit >= 0 && downHit == at && at < hits.size()) act(hits.get(at));
            else if (downOutside && at < 0 && !frame.contains(x, y)) close();
            else if (downHit < 0) typing.end();
            downHit = -1; downOutside = false;
        } else if (a == MotionEvent.ACTION_CANCEL) { downHit = -1; downOutside = false; }
        return true;
    }


    @Override public boolean padOpen() { return isOpen(); }

    @Override public void back() {
        if (typing.editing()) typing.end();
        else if (!query.isEmpty()) setQuery("");
        else close();
    }

    private void move(int dx, int dy) {
        if (hitRects.isEmpty()) return;
        int f = focusIndex();
        if (f < 0) { int row = -1; for (int i = 0; i < hits.size(); i++) if (hits.get(i)[0] == HIT_ROW && hits.get(i)[1] == sel) row = i; setFocus(row >= 0 ? row : 0); return; }
        RectF at = focusRects.get(f);
        int best = -1; float bestScore = Float.MAX_VALUE;
        for (int i = 0; i < hitRects.size(); i++) {
            if (i == f) continue;
            RectF o = focusRects.get(i);
            float ox = o.centerX() - at.centerX(), oy = o.centerY() - at.centerY();
            float along = dx != 0 ? ox * dx : oy * dy, across = dx != 0 ? Math.abs(oy) : Math.abs(ox);
            if (along <= 1) continue;
            float score = along + across * 2.5f;
            if (score < bestScore) { bestScore = score; best = i; }
        }
        if (best >= 0) setFocus(best);
    }
    private void setFocus(int i) {
        focusKind = hits.get(i)[0]; focusWhich = hits.get(i)[1];
        if (focusKind == HIT_ROW) select(focusWhich);
        invalidate();
    }

    @Override public void onPad(int cmd) {
        switch (cmd) {
            case PadNav.B: back(); return;
            case PadNav.MENU: close(); return;
            case PadNav.LEFT: move(-1, 0); return;
            case PadNav.RIGHT: move(1, 0); return;
            case PadNav.UP: move(0, -1); return;
            case PadNav.DOWN: move(0, 1); return;
            case PadNav.A: { int f = focusIndex(); if (f >= 0) act(hits.get(f)); else move(0, 1); return; }
            case PadNav.L1: setCat(cat - 1); focusKind = -1; return;
            case PadNav.R1: setCat(cat + 1); focusKind = -1; return;
            case PadNav.L2: entryPage = Math.max(0, entryPage - 1); invalidate(); return;
            case PadNav.R2: entryPage = Math.min(entryPages - 1, entryPage + 1); invalidate(); return;
            case PadNav.Y: if (typing.editing()) typing.end(); else beginEditing(); return;
            default:
        }
    }

    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }
    @Override public void setVisibility(int v) { super.setVisibility(v); TestHooks.overlay(this, "glossary", isOpen() ? "open" : "closed"); }
    private void qaSearch() { TestHooks.overlay(this, "glossary_search", typing.editing() ? "editing" : "idle"); }
}
