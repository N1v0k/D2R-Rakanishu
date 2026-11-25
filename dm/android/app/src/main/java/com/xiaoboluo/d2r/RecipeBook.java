package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.SystemClock;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;














final class RecipeBook extends View implements PadPage, QuickPage {
    private static final int TAB_DOABLE = 0, TAB_CUBE = 1, TAB_WORDS = 2;
    private static final long STOCK_WAIT_MS = 500;
    private static final List<String> CATS = RecipeData.list("runes", "gems", "craft", "socket", "improve", "quest", "ext");
    private static final String[] GROUPS = {null, "weap", "tors", "helm", "shld"};
    private static final int[] SOCKETS = {0, 2, 3, 4, 5, 6};

    private static final int HIT_CARD = 10, HIT_INSERT = 9, HIT_CLOSE = 0, HIT_TAB = 1, HIT_CAT = 2, HIT_SOCKETS = 3, HIT_GROUP = 4, HIT_SEARCH = 5, HIT_CLEAR = 6, HIT_PREV = 7, HIT_NEXT = 8;
    private static final String ELLIPSIS = "...";

    private final AppArt art;
    private final GameStyle style;
    private final Runnable freeInput;



    private boolean de, dataDe, loadingDe;
    private int loadGen;
    private final GameLang.Watch lang = new GameLang.Watch();
    private final Paint pic = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), box = new Paint(Paint.ANTI_ALIAS_FLAG);

    private final GameText tName, tStatus, tItem, tInfo, tDesc, tHeader, tInput, tButton, tBody, tTip, tTipTitle, tTipSub;
    private final RectF panel = new RectF(), r = new RectF();
    private final float[] cols = new float[6], rows = new float[2 * RecipeStyle.NAME_LINES + 2], cell = new float[4];
    private final char[] num = new char[4];
    private final List<RectF> hitRects = new ArrayList<>();
    private final List<int[]> hits = new ArrayList<>();
    private RecipeData data;
    private boolean paused, loading, broken;
    private RecipeStock stock;
    private final java.util.Map<RecipeModel.Card, String> notes = new java.util.IdentityHashMap<>();
    private java.util.Map<RecipeModel.Card, RecipeMatch.Result> results;
    private boolean noCube, stockFailed;
    private long stockAsked;
    private final java.nio.ByteBuffer stockBytes = java.nio.ByteBuffer.allocateDirect(RecipeStock.BYTES);
    private int stockAskedId;
    private RecipeModel.Card preview;


    private final RecipePreview previewText = new RecipePreview();



    private final ItemTooltip tooltip;
    private List<GearPreview.Line> tipLines;
    private Object tipFor, tipSrc, tipRes;
    private int previewDrawn = -1;
    private final Runnable previewPoll = this::pollPreview;
    private final Runnable stockPoll = this::waitStock, insertPoll = this::pollInsert;
    private static final int INSERT_DONE = 1, INSERT_RUNNING = 4;
    private static final long INSERT_MAX_MS = 20000;
    private RecipeModel.Card inserting, pendingCard;
    private long insertSince;
    private int insertAborts;
    private String pendingNote;
    private int tab = TAB_DOABLE, cat, sockets, group, page, pages = 1, charData;




    private String pendingTarget;
    private RecipeModel.Card jumpTo;
    private Runnable returnTo;
    private String query = "";
    private List<RecipeModel.Card> shown = new ArrayList<>();
    private int downHit = -1;
    private float downX, downY;
    private boolean qaQuiet;
    private final SearchTyping typing = new SearchTyping(this, this::searched, this::qaSearch);

    RecipeBook(Context context, AppArt art, GameStyle style, Runnable freeInput) {
        super(context);
        this.art = art;
        this.style = style;
        this.freeInput = freeInput;
        tName = style.text(GameStyleData.BOOK_NAME); tStatus = style.text(GameStyleData.BOOK_STATUS); tItem = style.text(GameStyleData.BOOK_ITEM);
        tInfo = style.text(GameStyleData.BOOK_INFO); tDesc = style.text(GameStyleData.BOOK_DESC); tHeader = style.text(GameStyleData.BOOK_HEADER);
        tInput = style.text(GameStyleData.BOOK_INPUT); tButton = style.text(GameStyleData.BUTTON_LABEL);
        tBody = style.text(GameStyleData.BODY); tTip = style.text(GameStyleData.TOOLTIP); tTipTitle = style.text(GameStyleData.TOOLTIP_TITLE);
        tTipSub = style.text(GameStyleData.TOOLTIP_SUB);
        tooltip = new ItemTooltip(style);
        setVisibility(GONE);
    }

    @Override public boolean available() { return !broken && RecipeData.usable(getContext().getFilesDir()) && DmActivity.nativeCharData() != 0; }
    boolean isOpen() { return getVisibility() == VISIBLE; }




    @Override public boolean padOpen() { return isOpen(); }
    @Override public void onPad(int cmd) {
        if (inserting != null) return;
        if (cmd == PadNav.B || cmd == PadNav.MENU) { back(); return; }
        if (preview != null) {
            if (cmd == PadNav.A) setPreview(null);
            else if ((cmd == PadNav.LEFT || cmd == PadNav.RIGHT) && tooltip.flip(cmd == PadNav.LEFT ? -1 : 1)) invalidate();
            return;
        }
        switch (cmd) {
            case PadNav.Y: if (typing.editing()) endEditing(); else beginEditing(); return;
            case PadNav.X: if (!query.isEmpty()) setQuery(""); return;
            case PadNav.L1: cycleTab(-1); return;
            case PadNav.R1: cycleTab(1); return;
            case PadNav.LEFT: case PadNav.L2: flip(-1); return;
            case PadNav.RIGHT: case PadNav.R2: flip(1); return;
            case PadNav.UP: filter(-1); return;
            case PadNav.DOWN: filter(1); return;
            default:
        }
    }
    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }

    private void cycleTab(int step) {
        if (data == null) return;
        int to = query.isEmpty() ? Math.floorMod(tab + step, 3) : tab;
        if (!query.isEmpty()) setQueryQuiet("");
        tab = to; page = 0; refill();
    }


    private void filter(int step) {
        if (!query.isEmpty()) return;
        if (tab == TAB_CUBE) { cat = Math.floorMod(cat + step, CATS.size()); page = 0; refill(); }
        else if (tab == TAB_WORDS) {
            int i = 0;
            while (i < SOCKETS.length && SOCKETS[i] != sockets) i++;
            sockets = SOCKETS[Math.floorMod((i < SOCKETS.length ? i : 0) + step, SOCKETS.length)]; page = 0; refill();
        }
    }

    @Override public void takePause() { paused = true; DmActivity.nativePause(DmActivity.PAUSE_BOOK, 1); }

    @Override public void open() {
        qaQuiet = true;
        DmActivity.syncLang();
        charData = DmActivity.nativeCharData();
        refreshLang();
        prepareTabs();
        page = 0;
        setVisibility(VISIBLE);
        stock = null;
        results = null;
        notes.clear();
        if (pendingCard != null) { notes.put(pendingCard, pendingNote); pendingCard = null; }
        stockFailed = false;
        tab = TAB_DOABLE;
        stockAsked = System.currentTimeMillis();
        stockAskedId = DmActivity.nativeRecipeRequest();
        warmText(getContext());
        previewText.reset();
        removeCallbacks(stockPoll);
        post(stockPoll);
        post(freeInput);
        pendingTarget = null; jumpTo = null; returnTo = null;
        load();
        refill();
        qaQuiet = false;
        qa();
    }




    private void load() {
        if (data != null || broken || DmActivity.nativeCharData() != RecipeData.DATA_VERSION || (loading && loadingDe == de)) return;
        loading = true;
        loadingDe = de;
        final boolean want = de;
        final int gen = ++loadGen;
        final Context context = getContext();
        new Thread(() -> {
            RecipeData d = RecipeData.load(context.getFilesDir(), want);
            post(() -> {
                if (gen != loadGen) return;
                data = d; loading = false;
                if (d == null) { broken = true; pendingTarget = null; if (isOpen()) close(); return; }
                dataDe = want;
                prepareTabs();
                if (pendingTarget != null) resolveTarget();
                refill(); evaluate();
            });
        }, "recipes").start();
    }


    private RecipeModel.Card card(String target) {
        if (data == null || target == null || target.length() < 3) return null;
        String want = target.substring(2);
        if (target.startsWith("w:")) { for (RecipeModel.Card c : data.words) if (want.equals(c.key)) return c; }
        else if (target.startsWith("o:")) { for (RecipeModel.Card c : data.cube) if (want.equals(c.out)) return c; }
        return null;
    }



    boolean has(String target) {
        if (!available()) return false;
        refreshLang();
        load();
        return card(target) != null;
    }




    void openAt(String target, Runnable back) {
        open();
        pendingTarget = target;
        returnTo = back;
        if (data != null) resolveTarget();
    }

    private void resolveTarget() {
        RecipeModel.Card c = card(pendingTarget);
        pendingTarget = null;
        if (c == null) return;
        endEditing();
        typing.setText(""); query = ""; page = 0;
        if (c.cat == null) { tab = TAB_WORDS; sockets = group = 0; }
        else { tab = TAB_CUBE; cat = Math.max(0, CATS.indexOf(c.cat)); }
        refill();
        jumpTo = c;
        setPreview(c);
    }


    private void waitStock() {
        if (!isOpen()) return;
        int bytes = DmActivity.nativeRecipeStock(stockBytes);
        RecipeStock got = RecipeStock.parse(stockBytes, bytes);
        boolean fresh = stock == null && got != null && got.gen == stockAskedId;
        long waited = System.currentTimeMillis() - stockAsked;
        if (fresh) stock = got;
        if (!paused && (fresh || waited >= STOCK_WAIT_MS)) { paused = true; DmActivity.nativePause(DmActivity.PAUSE_BOOK, 1); }
        if (fresh) { evaluate(); return; }
        if (waited >= STOCK_WAIT_MS * 4) { stockFailed = true; if (tab == TAB_DOABLE) tab = TAB_CUBE; refill(); return; }
        postDelayed(stockPoll, 16);
    }


    private void insert(RecipeModel.Card card) {
        if (stock == null || data == null) return;
        RecipeInsert.Plan p = RecipeInsert.plan(card, data.catalog, stock, new RecipeMatch(data.catalog, stock, de));
        if (p.outcome == RecipeInsert.OK && p.moves == 0) { notes.put(card, de ? "liegt schon im Würfel" : "already in the cube"); invalidate(); return; }
        if (p.outcome != RecipeInsert.OK) { notes.put(card, reason(p)); invalidate(); return; }
        if (DmActivity.nativeRecipeInsert(p.steps, p.cubeId, stock.gen) == 0) { notes.put(card, de ? "gerade nicht möglich" : "not possible right now"); invalidate(); return; }

        returnTo = null;
        endEditing();
        inserting = card;
        insertAborts = 0;
        insertSince = System.currentTimeMillis();
        if (paused) { paused = false; DmActivity.nativePause(DmActivity.PAUSE_BOOK, 0); }
        postDelayed(insertPoll, 50);
        invalidate();
        qa();
    }

    private void pollInsert() {
        if (inserting == null) return;
        int state = DmActivity.nativeRecipeInsertState(), result = state & 255, why = state >> 8;
        if (result == INSERT_RUNNING && System.currentTimeMillis() - insertSince < INSERT_MAX_MS) { postDelayed(insertPoll, 50); return; }
        if (result == INSERT_RUNNING && insertAborts++ < 2) { DmActivity.nativeRecipeAbort(); insertSince = System.currentTimeMillis() - INSERT_MAX_MS / 2; postDelayed(insertPoll, 50); return; }
        if (result == INSERT_RUNNING) { inserting = null; hide(); return; }
        RecipeModel.Card card = inserting;
        inserting = null;
        if (result == INSERT_DONE) { hide(); return; }
        String[] whyDe = {"", "Bestand hat sich geändert", "Zutat hat sich geändert", "zu weit vom Schatz", "am Cursor liegt etwas", "Inventar nicht offen", "unterbrochen", "Figur oder Spiel gewechselt", "Stück kam nicht an", "Stück liegt am Cursor: selbst ablegen"};
        String[] whyEn = {"", "stock has changed", "ingredient has changed", "too far from the stash", "something is on the cursor", "inventory not open", "interrupted", "character or game changed", "item did not arrive", "item is on the cursor: put it down yourself"};
        String text = why >= 0 && why < whyDe.length ? (de ? whyDe : whyEn)[why] : "";
        if (!isShown() && text.isEmpty()) { qa(); return; }
        pendingNote = text;
        pendingCard = card;
        open();
    }


    void pause() { returnTo = null; endEditing(); if (inserting != null) DmActivity.nativeRecipeAbort(); else close(); }

    private void hide() { removeCallbacks(insertPoll); setVisibility(GONE); }

    private String reason(RecipeInsert.Plan p) {
        String what = p.what == null ? "" : p.what;
        switch (p.outcome) {
            case RecipeInsert.CHOOSE: return (de ? "selbst wählen: " : "choose yourself: ") + what;
            case RecipeInsert.CUBE_FULL: return de ? "passt nicht in den Würfel" : "does not fit the cube";
            case RecipeInsert.INVENTORY_FULL: return de ? "Inventar zu voll" : "inventory too full";
            case RecipeInsert.FAR: return de ? "im Schatz, zu weit weg" : "in the stash, too far away";
            case RecipeInsert.ON_PAGE: return de ? "auf anderer Schatzseite: " + what : "on another stash page: " + what;
            case RecipeInsert.ELSEWHERE: return de ? "im Beutel oder in der Chronik: " + what : "in the pouch or chronicle: " + what;
            case RecipeInsert.NO_CUBE: return de ? "Würfel ins Inventar legen" : "put the cube in the inventory";
            case RecipeInsert.UNSURE: return de ? "nicht sicher, selbst einlegen" : "unsure, put it in yourself";
            default: return (de ? "fehlt: " : "missing: ") + what;
        }
    }

    private void evaluate() {
        final RecipeData d = data;
        final RecipeStock s = stock;
        final boolean german = de;
        if (d == null || s == null) return;
        new Thread(() -> {
            java.util.Map<RecipeModel.Card, RecipeMatch.Result> r = new RecipeMatch(d.catalog, s, german).evaluateAll();
            post(() -> { if (stock == s && data == d) { results = r; noCube = !s.holds("box"); refill(); } });
        }, "recipes").start();
    }

    @Override protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        removeCallbacks(prepareTabInk);
        post(prepareTabInk);
    }

    private final Runnable prepareTabInk = () -> { prepareTabs(); invalidate(); };

    private String[] tabNames() { return new String[]{de ? "Machbar" : "Doable", de ? "Würfel" : "Cube", term("runewords")}; }

    private void prepareTabs() {
        float w = getWidth(), h = getHeight();
        for (String label : tabNames()) style.prepareTabLabel(label, w, h);
        for (String category : CATS) style.prepareTabLabel(catName(category), w, h);
        for (int count : SOCKETS) style.prepareTabLabel(count == 0 ? term("all") : String.valueOf(count), w, h);
        for (String group : GROUPS) style.prepareTabLabel(group == null ? term("all") : term("group." + group), w, h);
    }




    private void refreshLang() {
        if (!lang.changed()) return;
        de = GameLang.german();
        style.refresh();
        removeCallbacks(prepareTabInk); post(prepareTabInk);
        layouts.clear(); crowd.clear();
        tipLines = null;
        if (data != null && dataDe != de) { data = null; results = null; notes.clear(); preview = null; pendingCard = null; shown = new ArrayList<>(); }
        if (isOpen()) load();
    }

    void close() {
        if (inserting != null) return;
        endEditing();
        preview = null;
        pendingTarget = null; jumpTo = null;
        removeCallbacks(stockPoll);
        removeCallbacks(previewPoll);
        Runnable back = returnTo;
        returnTo = null;
        if (back != null) back.run();
        if (paused) { paused = false; DmActivity.nativePause(DmActivity.PAUSE_BOOK, 0); }
        stock = null;
        results = null;
        previewText.reset();
        removeCallbacks(previewPoll);
        setVisibility(GONE);
    }


    @Override public void back() {
        if (inserting != null) return;
        if (preview != null) { setPreview(null); return; }
        if (typing.editing()) { endEditing(); return; }
        if (!query.isEmpty()) setQuery("");
        else if (tab == TAB_WORDS && (sockets != 0 || group != 0)) { sockets = group = 0; page = 0; refill(); }
        else close();
    }


    private void setQuery(String q) { setQueryQuiet(q); page = 0; refill(); }
    private void setQueryQuiet(String q) {
        typing.setText(q);
        query = typing.text().trim();
    }





    static void warmText(Context context) {
        if (DmActivity.nativeCharData() == RecipeData.DATA_VERSION && RecipeData.usable(context.getFilesDir()))
            DmActivity.nativeRecipeTextStart(new java.io.File(context.getFilesDir(), "dm").getPath());
    }


    private void setPreview(RecipeModel.Card c) {
        preview = c;
        previewText.reset();
        tooltip.reset();
        previewDrawn = -1;
        removeCallbacks(previewPoll);
        invalidate();
    }



    private void pollPreview() {
        RecipeModel.Card p = preview;
        if (p == null || !isOpen() || inserting != null) return;
        long now = SystemClock.uptimeMillis();
        int state = previewText.state(p, now, DmActivity::nativeRecipeText);
        if (state != previewDrawn) invalidate();
        else schedulePreviewPoll(state, now);
    }

    private void schedulePreviewPoll(int state, long now) {
        removeCallbacks(previewPoll);
        long next = previewText.nextPoll(state, now);
        if (next == RecipePreview.NEXT_FRAME) postOnAnimation(previewPoll);
        else if (next > 0) postDelayed(previewPoll, next);
    }


    private boolean offered(RecipeModel.Card c) {
        if (c.app.isEmpty()) return true;
        return "unsocket".equals(c.app) && stock != null && stock.unsocket;
    }

    private void refill() {
        shown = new ArrayList<>();
        if (data != null) {
            if (!query.isEmpty()) shown = data.search(query, CATS);
            else if (tab == TAB_DOABLE) {
                if (results != null) { List<RecipeModel.Card> all = new ArrayList<>(data.cube); all.addAll(data.words); shown = RecipeMatch.doable(all, results, CATS); }
            } else if (tab == TAB_CUBE) { for (RecipeModel.Card c : data.cube) if (CATS.get(cat).equals(c.cat)) shown.add(c); }
            else for (RecipeModel.Card c : data.words) if ((sockets == 0 || c.sockets == sockets) && (GROUPS[group] == null || c.groups.contains(GROUPS[group]))) shown.add(c);
            shown.removeIf(c -> !offered(c));
        }
        invalidate();
        qa();
    }


    private void beginEditing() {
        if (!isOpen() || inserting != null || data == null) return;
        typing.begin();
    }
    private void endEditing() { typing.end(); }
    private void searched() { query = typing.text().trim(); page = 0; refill(); }

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


    private void hit(RectF rect, int kind, int which) { hitRects.add(new RectF(rect)); hits.add(new int[]{kind, which}); if (BuildConfig.DM_QA_BUILD) TestHooks.hit(rect, kind, which); }

    private void image(Canvas c, String name, RectF into) {
        Bitmap b = art.image(name);
        if (b != null) c.drawBitmap(b, null, into, pic);
    }


    private void item(Canvas c, String code, float cx, float cy, float size) {
        Bitmap b = art.image("items/" + code);
        if (b == null) return;
        float scale = size / Math.max(b.getWidth(), b.getHeight());
        float w = b.getWidth() * scale, h = b.getHeight() * scale;
        r.set(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2);
        c.drawBitmap(b, null, r, pic);
    }



    private void line(Canvas c, GameText t, String s, float x, float top, float bottom, float max, float k, Paint.Align align, int alpha) {
        if (s == null || s.isEmpty() || !t.ready() || max <= 0) return;
        float kk = t.fitTo(s, max, k);
        String fit = cut(t, s, max, kk);
        Paint p = t.paint(kk);
        t.draw(c, fit, x, GameStyleGeometry.centreBaseline(top, bottom, p.ascent(), p.descent()), kk, align, alpha);
    }
    private void line(Canvas c, GameText t, String s, float x, float top, float bottom, float max, float k, Paint.Align align) { line(c, t, s, x, top, bottom, max, k, align, 255); }

    private static String cut(GameText t, String s, float max, float k) {
        if (t.measure(s, k) <= max) return s;
        int n = s.length();
        while (n > 0 && t.measure(s.substring(0, n).trim() + ELLIPSIS, k) > max) n--;
        return n == 0 ? "" : s.substring(0, n).trim() + ELLIPSIS;
    }


    private static List<String> wrap(GameText t, String s, float width, float k) {
        List<String> out = new ArrayList<>();
        StringBuilder cur = new StringBuilder();
        for (String w : s.split(" ")) {
            String next = cur.length() == 0 ? w : cur + " " + w;
            if (t.measure(next, k) <= width || cur.length() == 0) { cur.setLength(0); cur.append(next); }
            else { out.add(cur.toString()); cur.setLength(0); cur.append(w); }
        }
        if (cur.length() > 0) out.add(cur.toString());
        for (int i = 0; i < out.size(); i++) out.set(i, cut(t, out.get(i), width, k));
        return out;
    }

    private String term(String key) { return RecipeTerms.get(data == null ? null : data.catalog.terms, key, de); }
    private String catName(String c) { return term("cat." + c); }






    private void drawCard(Canvas c, RecipeModel.Card card, RectF at, float k, float statusW, boolean tall) {
        hit(at, HIT_CARD, shown.indexOf(card));
        if (tall) style.listRowTall(c, at, RecipeStyle.ROW_UNITS * k); else style.listRow(c, at);
        float dy = (at.height() - RecipeStyle.ROW_UNITS * k) / 2;
        item(c, card.img, at.left + (RecipeStyle.BOX_X + RecipeStyle.BOX / 2) * k, at.top + dy + (RecipeStyle.BOX_Y + RecipeStyle.BOX / 2) * k, RecipeStyle.BOX * 0.86f * k);
        RecipeMatch.Result result = results == null ? null : results.get(card);
        final float kt = k * RecipeStyle.TEXT;

        String[] ts = titleSub(card);
        String title = ts[0], sub = ts[1];


        RecipeStyle.Measure mName = s -> tName.measure(s, kt), mDesc = s -> tDesc.measure(s, kt), mItem = s -> tItem.measure(s, kt);
        RecipeStyle.Layout lay = layoutOf(card, title, sub, at, k, statusW, tall, mName, mItem, mDesc);
        for (int i = 0; i < 6; i++) cols[i] = at.left + lay.cols[i];
        float nameW = cols[1] - cols[0];
        List<String> tl = RecipeStyle.wrap(title, nameW, mName), sl = sub.isEmpty() ? new ArrayList<>() : RecipeStyle.wrap(sub, nameW, mDesc);
        if (tl.size() > RecipeStyle.TITLE_LINES || tl.size() + sl.size() > RecipeStyle.NAME_LINES) {
            tl = Collections.singletonList(title); sl = sub.isEmpty() ? new ArrayList<>() : Collections.singletonList(sub);
        }
        int nl = tl.size() + sl.size();
        RecipeStyle.nameRows(tl.size(), sl.size(), rows);
        tName.colour(RecipeStyle.quality(card.colour));
        for (int i = 0; i < nl; i++) {
            boolean t = i < tl.size();
            line(c, t ? tName : tDesc, t ? tl.get(i) : sl.get(i - tl.size()), cols[0], at.top + dy + rows[i] * k, at.top + dy + rows[i + nl] * k, nameW + 2 * k, kt, Paint.Align.LEFT);
        }



        int n = card.in.length;
        float f = lay.f, kf = kt * f, area = cols[3] - cols[2], cx0 = cols[3] - Math.min(lay.used, area), band = at.top;
        float[] slots = lay.slots;
        RecipeStyle.Measure mi = RecipeStyle.scaled(mItem, f), md = RecipeStyle.scaled(mDesc, f);
        for (int i = 0; i < n; i++) {
            RecipeModel.Input in = card.in[i];
            if (i == lay.split && i > 0) { cx0 = cols[3] - Math.min(lay.used2, area); band = at.bottom - RecipeStyle.ROW_UNITS * k; }
            float slot = slots[i], cx = cx0 + slot / 2, room = RecipeStyle.cellRoom(slot, k);
            cx0 += slot;
            List<String> name = RecipeStyle.wrap(in.name, room, mi), cond = in.cond.isEmpty() ? new ArrayList<>() : RecipeStyle.wrap(in.cond, room, md);
            int lines = name.size() + cond.size();
            RecipeStyle.cellRows(lines, cell);
            float icon = cell[1] * k, iconY = band + cell[0] * k;
            int state = result == null ? RecipeMatch.HAVE : result.state[i];
            String img = result != null && result.img[i] != null ? result.img[i] : in.img;
            pic.setAlpha(state == RecipeMatch.HAVE ? 255 : 90);
            item(c, img, cx, iconY, icon);
            pic.setAlpha(255);
            if (in.count > 1) {
                int len = digits(in.count);


                style.cornerNumber(c, num, 0, len, cx - icon / 2, iconY - icon / 2, cx + icon / 2, iconY + icon / 2, tItem.size(k) * 1.2f, "FontColorWhite");
            }
            tItem.colour(RecipeStyle.input(state, in.kind != null && "i".equals(in.kind) ? in.id : img));
            float y = band + cell[2] * k, pitch = cell[3] * k;
            for (int j = 0; j < lines; j++, y += pitch) {
                boolean nm = j < name.size();
                line(c, nm ? tItem : tDesc, nm ? name.get(j) : cond.get(j - name.size()), cx, y, y + pitch, room + 2 * k, kf, Paint.Align.CENTER);
            }
        }

        if (result == null) return;
        float sx = cols[4], sw = cols[5] - cols[4];
        tStatus.colour(RecipeStyle.status(result.status));
        line(c, tStatus, result.text, sx, at.top + 10 * k, at.top + 64 * k, sw, kt, Paint.Align.LEFT);
        String note = notes.get(card);
        boolean canInsert = note == null && card.cat != null && (result.status == RecipeMatch.READY || (result.status == RecipeMatch.IN_STASH && card.diff <= stock.difficulty));
        if (note != null) { tInfo.colour("FontColorRed"); line(c, tInfo, note, sx, at.top + 64 * k, at.top + 108 * k, sw, kt, Paint.Align.LEFT); tInfo.colour("FontColorLightGray"); }
        else { tInfo.colour("FontColorLightGray"); line(c, tInfo, result.where, sx, at.top + 64 * k, at.top + 108 * k, sw, kt, Paint.Align.LEFT); }
        if (canInsert) {


            float bw = mw, bh = mh, room = at.bottom - (at.top + 108 * k) - 4 * k, fit = Math.min(1, Math.min(room / bh, sw / bw));
            bw *= fit; bh *= fit;
            r.set(sx, at.bottom - 4 * k - bh, sx + bw, at.bottom - 4 * k);
            style.modalButton(c, r, GameStyle.BUTTON_NORMAL, de ? "In den Würfel" : "Into the cube");
            hit(r, HIT_INSERT, shown.indexOf(card));
        }
    }



    private RecipeStyle.Layout layoutOf(RecipeModel.Card card, String title, String sub, RectF at, float k, float statusW, boolean tall,
                                        RecipeStyle.Measure name, RecipeStyle.Measure item, RecipeStyle.Measure desc) {
        CachedLayout cl = layouts.get(card);
        if (cl != null && cl.w == at.width() && cl.k == k && cl.statusW == statusW && cl.tall == tall && cl.gen == style.generation() && cl.sub.equals(sub) && cl.title.equals(title)) return cl.lay;
        if (layouts.size() > 64) layouts.clear();
        int n = card.in.length;
        String[] names = new String[n], conds = new String[n];
        for (int i = 0; i < n; i++) { names[i] = card.in[i].name; conds[i] = card.in[i].cond; }
        cl = new CachedLayout();
        cl.w = at.width(); cl.k = k; cl.statusW = statusW; cl.tall = tall; cl.sub = sub; cl.title = title; cl.gen = style.generation();
        RecipeStyle.layout(title, sub, names, conds, name, item, desc, 0, at.width(), k, statusW, tall, cl.lay);
        if (tName.ready() && tItem.ready() && tDesc.ready()) layouts.put(card, cl);
        return cl.lay;
    }
    private static final class CachedLayout { float w, k, statusW; boolean tall; int gen; String sub, title; final RecipeStyle.Layout lay = new RecipeStyle.Layout(); }
    private final java.util.HashMap<RecipeModel.Card, CachedLayout> layouts = new java.util.HashMap<>();



    private String[] titleSub(RecipeModel.Card card) {
        String title = card.title, sub = card.sub;
        int colon = title.indexOf(": ");
        if (colon > 0) { sub = title.substring(colon + 2); title = title.substring(0, colon); }
        if (!query.isEmpty()) { String where = card.cat == null ? term("runewords") : catName(card.cat); sub = sub.isEmpty() ? where : where + ", " + sub; }
        return new String[]{title, sub};
    }






    private boolean[] crowded(float w, float k, float statusW) {
        int n = shown.size(), gen = style.generation();
        if (crowdW != w || crowdK != k || crowdStatusW != statusW || crowdGen != gen || !crowdQuery.equals(query)) {
            crowd.clear(); crowdW = w; crowdK = k; crowdStatusW = statusW; crowdGen = gen; crowdQuery = query;
        }
        if (tallOf.length < n) tallOf = new boolean[n];
        final float kt = k * RecipeStyle.TEXT;
        RecipeStyle.Measure mName = s -> tName.measure(s, kt), mDesc = s -> tDesc.measure(s, kt), mItem = s -> tItem.measure(s, kt);
        boolean ready = tName.ready() && tItem.ready() && tDesc.ready();
        for (int i = 0; i < n; i++) {
            RecipeModel.Card card = shown.get(i);
            Boolean known = crowd.get(card);
            if (known == null) {
                int m = card.in.length;
                if (m < 2) known = false;
                else {
                    String[] ts = titleSub(card), names = new String[m], conds = new String[m];
                    for (int j = 0; j < m; j++) { names[j] = card.in[j].name; conds[j] = card.in[j].cond; }
                    RecipeStyle.layout(ts[0], ts[1], names, conds, mName, mItem, mDesc, 0, w, k, statusW, false, crowdLay);
                    known = RecipeStyle.crowded(crowdLay);
                }
                if (ready) crowd.put(card, known);
            }
            tallOf[i] = known;
        }
        return tallOf;
    }
    private final java.util.HashMap<RecipeModel.Card, Boolean> crowd = new java.util.HashMap<>();
    private final RecipeStyle.Layout crowdLay = new RecipeStyle.Layout();
    private boolean[] tallOf = new boolean[0];
    private float crowdW, crowdK, crowdStatusW;
    private int crowdGen;
    private String crowdQuery = "";


    private float statusNeed(RecipeModel.Card card, float kt) {
        RecipeMatch.Result result = results == null ? null : results.get(card);
        if (result == null) return 0;
        String note = notes.get(card);
        float w = result.text == null ? 0 : tStatus.measure(result.text, kt);
        String info = note != null ? note : result.where;
        return info == null ? w : Math.max(w, tInfo.measure(info, kt));
    }


    private int digits(int n) {
        String s = Integer.toString(Math.max(0, Math.min(9999, n)));
        s.getChars(0, s.length(), num, 0);
        return s.length();
    }









    private void drawPreview(Canvas c, RecipeModel.Card card, List<String> lines) {
        if (BuildConfig.DM_QA_BUILD) TestHooks.foreground();
        RecipeMatch.Result result = results == null ? null : results.get(card);
        if (tipFor != card || tipSrc != lines || tipRes != result || tipLines == null) {
            List<GearPreview.Line> out = new ArrayList<>();
            out.add(new GearPreview.Line(card.title.replace(": ", ", "), RecipeStyle.quality(card.colour)));
            for (String s : lines) out.add(new GearPreview.Line(s, s.endsWith(":") ? GearPreview.UNIQUE : GearPreview.MAGIC));
            if (result != null && result.text != null && !result.text.isEmpty()) out.add(new GearPreview.Line(result.text, RecipeStyle.status(result.status)));
            if (card.in.length > 0) {
                if (card.cat == null && !card.sub.isEmpty()) out.add(new GearPreview.Line(card.sub, GearPreview.GREY));
                StringBuilder s = new StringBuilder();
                for (RecipeModel.Input in : card.in) s.append(s.length() > 0 ? " + " : "").append(in.count > 1 ? in.count + "× " : "").append(in.name);
                out.add(new GearPreview.Line(s.toString(), GearPreview.GREY));
            }
            if (tipLines != null) tooltip.reset();
            tipLines = out; tipFor = card; tipSrc = lines; tipRes = result;
        }
        tooltip.draw(c, getWidth(), getHeight(), art.image("items/" + card.img), tipLines);
    }

    private String summary() {
        if (results == null || stock == null) return "";
        int ready = 0, almost = 0;
        for (RecipeMatch.Result r : results.values()) { if (r.status == RecipeMatch.READY) ready++; else if (r.status == RecipeMatch.ALMOST || r.status == RecipeMatch.CHAIN) almost++; }
        StringBuilder s = new StringBuilder(ready + (de ? " bereit, " : " ready, ") + almost + (de ? " fast" : " almost"));
        StringBuilder unseen = new StringBuilder();
        for (int i = 0; i < stock.pages; i++) if ((stock.seen >> i & 1) == 0 && i != stock.page) unseen.append(unseen.length() > 0 ? ", " : "").append(i + 1);
        if (unseen.length() > 0) s.append(de ? ". Schatzseiten noch nicht gesehen: " : ". Stash pages not seen yet: ").append(unseen);
        String[] sources = {term("place.pouch"), term("place.chronicle"), de ? "andere Schatzseiten" : "other stash pages"};
        StringBuilder old = new StringBuilder();
        for (int i = 0; i < sources.length; i++) if ((stock.stale >> i & 1) != 0) old.append(old.length() > 0 ? ", " : "").append(sources[i]);
        if (old.length() > 0) s.append(de ? ". Nicht aktuell: " : ". Not current: ").append(old);
        if (noCube) s.append(de ? ". Kein Würfel im Bestand" : ". No cube in stock");
        return s.toString();
    }

    private void message(Canvas c, String text, float k) {
        line(c, tBody, text, panel.centerX(), panel.centerY() - panel.height() * 0.05f, panel.centerY() + panel.height() * 0.05f, panel.width() * 0.8f, k, Paint.Align.CENTER);
    }

    @Override protected void onDraw(Canvas c) {
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "recipe_book");
        try {
        refreshLang();
        hitRects.clear();
        hits.clear();
        if (inserting != null) return;


        style.backdrop(c);
        float h = getHeight(), w = getWidth();
        GameStyleGeometry.Panel p = style.panel(w, h);
        style.panelFrame(c, p);
        panel.set(p.contentL, p.contentT, p.contentR, p.contentB);
        mw = p.modalW(); mh = p.modalH();
        float m = MARGIN * p.s, x = p.contentL + m, right = p.contentR - m;


        float y = p.contentT + m, subH = p.subTabH(), y0 = y + subH + m, y1 = p.contentB - m, gap = CARD_GAP * p.s;
        int full = RecipeStyle.perPage(y1 - y0, h * 0.13f, false);
        float cardH = (y1 - y0 - gap * (full - 1)) / full, k = RecipeStyle.k(cardH), kt = k * RecipeStyle.TEXT;

        if (charData != RecipeData.DATA_VERSION) {
            message(c, de ? "Das Rezeptbuch kennt nur Figuren mit Datenstand 3.x." : "The recipe book only knows characters with data version 3.x.", kt);
            drawClose(c, p);
            return;
        }

        String[] tabs = tabNames();
        for (int i = 0; i < tabs.length; i++) {
            style.panelTab(c, p, tabs.length, i, tabs[i], query.isEmpty() && tab == i);
            p.tabHit(tabs.length, i, box4); r.set(box4[0], box4[1], box4[2], box4[3]); hit(r, HIT_TAB, i);
        }




        p.searchField(3, m / 2, box4); float sx0 = box4[0], top = box4[1], sx1 = box4[2], bottom = box4[3], rowH = bottom - top, pad = rowH * 0.4f;
        r.set(sx0, top, sx1, bottom);
        style.inputField(c, r);
        r.set(sx0, 0, sx1, p.hitTabBottom); hit(r, HIT_SEARCH, 0);
        float textW = sx1 - sx0 - 2 * pad - rowH;
        String shownText = typing.text();
        if (shownText.isEmpty() && !typing.editing()) line(c, tInput, de ? "Suche, deutsch oder englisch" : "Search, German or English", sx0 + pad, top, bottom, textW, kt, Paint.Align.LEFT, GameStyleData.HINT_ALPHA);
        else {
            if (typing.caretOn()) shownText = shownText + "|";
            String tail = shownText;
            while (tail.length() > 1 && tInput.ready() && tInput.measure(tail, kt) > textW) tail = tail.substring(1);
            line(c, tInput, tail, sx0 + pad, top, bottom, textW, kt, Paint.Align.LEFT);
        }
        if (!query.isEmpty() || !typing.isEmpty()) {
            line(c, tButton, "×", sx1 - pad - rowH * 0.25f, top, bottom, rowH, kt, Paint.Align.CENTER);
            r.set(sx1 - pad - rowH * 0.8f, 0, sx1, p.hitTabBottom); hit(r, HIT_CLEAR, 0);
        }
        drawClose(c, p);


        if (!query.isEmpty()) line(c, tHeader, shown.size() + (de ? " Treffer" : shown.size() == 1 ? " hit" : " hits"), x, y, y + subH, right - x, kt, Paint.Align.LEFT);
        else if (tab == TAB_DOABLE) line(c, tHeader, summary(), x, y, y + subH, right - x, kt, Paint.Align.LEFT);
        else if (tab == TAB_CUBE) {

            float each = p.subTabWidth(x, right, CATS.size());
            int n = CATS.size();
            for (int i = 0; i < n; i++) { p.subTab(x, y, each, i, subBoxes[i]); subTexts[i] = catName(CATS.get(i)); }
            for (int i = 0; i < n; i++) subTab(c, p, subBoxes[i], subTexts[i], i == cat, HIT_CAT, i, style.subTabK(p, subBoxes[i], subTexts[i]));
        } else {

            int cells = SOCKETS.length + GROUPS.length * 2, items = SOCKETS.length + GROUPS.length, n = 0;
            float g = GameStyleGeometry.Panel.SUBTAB_GAP * p.s, each = (right - x - m - (items - 1) * g) / cells, at = x;
            for (int i = 0; i < SOCKETS.length; i++, n++) {
                float[] bx = subBoxes[n]; bx[0] = at; bx[1] = y; bx[2] = at + each; bx[3] = y + subH;
                subTexts[n] = SOCKETS[i] == 0 ? term("all") : String.valueOf(SOCKETS[i]);
                at += each + g;
            }
            at += m - g;
            for (int i = 0; i < GROUPS.length; i++, n++) {
                float[] bx = subBoxes[n]; bx[0] = at; bx[1] = y; bx[2] = at + each * 2; bx[3] = y + subH;
                subTexts[n] = GROUPS[i] == null ? term("all") : term("group." + GROUPS[i]);
                at += each * 2 + g;
            }
            for (int i = 0; i < SOCKETS.length; i++) subTab(c, p, subBoxes[i], subTexts[i], SOCKETS[i] == sockets, HIT_SOCKETS, SOCKETS[i], style.subTabK(p, subBoxes[i], subTexts[i]));
            for (int i = 0; i < GROUPS.length; i++) { int j = SOCKETS.length + i; subTab(c, p, subBoxes[j], subTexts[j], i == group, HIT_GROUP, i, style.subTabK(p, subBoxes[j], subTexts[j])); }
        }


        float imeTop = typing.imeTop();
        boolean keyboard = typing.editing() && imeTop < y1;
        int perPage = keyboard ? RecipeStyle.perPage(imeTop - y0 + gap, cardH + gap, true) : full;


        float statusAll = mw;
        for (int i = 0; i < shown.size(); i++) statusAll = Math.max(statusAll, statusNeed(shown.get(i), kt));
        statusAll = Math.min(statusAll + 2 * k, RecipeStyle.STATUS_W * k);
        boolean[] tall = crowded(right - x, k, statusAll);
        int[] starts = RecipeStyle.pages(tall, shown.size(), perPage);
        pages = Math.max(1, starts.length - 1);
        if (jumpTo != null && data != null) { int at = shown.indexOf(jumpTo); if (at >= 0) page = RecipeStyle.pageOf(starts, at); jumpTo = null; }
        if (page >= pages) page = pages - 1;
        if (data == null || (tab == TAB_DOABLE && query.isEmpty() && results == null && !stockFailed)) message(c, de ? "Rezepte werden geladen" : "Loading recipes", kt);
        else if (shown.isEmpty()) message(c, tab == TAB_DOABLE && query.isEmpty() ? (de ? "Mit dem Bestand ist gerade nichts machbar" : "Nothing doable with this stock") : de ? "Keine Treffer" : "No hits", kt);
        else {


            int from = starts[page], to = starts[page + 1];
            float statusW = mw;
            for (int i = from; i < to; i++) statusW = Math.max(statusW, statusNeed(shown.get(i), kt));
            statusW = Math.min(statusW + 2 * k, RecipeStyle.STATUS_W * k);
            for (int i = from, slot = 0; i < to; i++) {
                int span = tall[i] && perPage >= 2 ? 2 : 1;
                RectF at = new RectF(x, y0 + slot * (cardH + gap), right, y0 + (slot + span) * (cardH + gap) - gap);
                drawCard(c, shown.get(i), at, k, statusW, span == 2);
                slot += span;
            }
        }



        if (preview != null) {
            long now = SystemClock.uptimeMillis();
            int state = previewText.state(preview, now, DmActivity::nativeRecipeText);
            previewDrawn = state;
            schedulePreviewPoll(state, now);
            if (state != RecipePreview.WAIT) { drawPreview(c, preview, previewText.lines(preview)); return; }
        }
        if (keyboard) return;



        style.footText(c, p, w, 1, RecipeTerms.page(data == null ? null : data.catalog.terms, de, page + 1, pages), "LightButtonTextColor");
        for (int side = 0; side < 2; side++) {
            boolean can = side == 0 ? page > 0 : page < pages - 1;
            int col = side == 0 ? 0 : 2;
            style.footButton(c, p, w, col, can ? GameStyle.BUTTON_NORMAL : GameStyle.BUTTON_DISABLED, null, side == 0 ? -1 : 1);
            p.footHit(w, col, box4); r.set(box4[0], box4[1], box4[2], box4[3]);
            hit(r, side == 0 ? HIT_PREV : HIT_NEXT, 0);
        }

        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }


    private static final float MARGIN = 20, CARD_GAP = 14;
    private final float[] box4 = new float[4];
    private float mw, mh;


    private void subTab(Canvas c, GameStyleGeometry.Panel p, float[] box, String text, boolean on, int kind, int which, float k) {
        style.subTab(c, p, box, text, on, k);
        r.set(box[0], box[1], box[2], box[3]);
        hit(r, kind, which);
    }
    private final float[][] subBoxes = new float[16][4];
    private final String[] subTexts = new String[16];

    private void drawClose(Canvas c, GameStyleGeometry.Panel p) {
        style.closeX(c, p);
        r.set(p.hitCloseL, p.hitCloseT, p.hitCloseR, p.hitCloseB);
        hit(r, HIT_CLOSE, 0);
    }

    private int hitAt(float x, float y) {
        for (int i = hitRects.size() - 1; i >= 0; i--) if (hitRects.get(i).contains(x, y)) return i;
        return -1;
    }

    private void act(int[] h) {
        if (typing.editing() && h[0] != HIT_SEARCH && h[0] != HIT_CLEAR) endEditing();
        switch (h[0]) {
            case HIT_CLOSE: close(); return;
            case HIT_TAB: setQueryQuiet(""); tab = h[1]; page = 0; refill(); return;
            case HIT_CAT: cat = h[1]; page = 0; refill(); return;
            case HIT_SOCKETS: sockets = h[1]; page = 0; refill(); return;
            case HIT_GROUP: group = h[1]; page = 0; refill(); return;
            case HIT_SEARCH: beginEditing(); return;
            case HIT_CLEAR: setQuery(""); return;
            case HIT_CARD: if (h[1] >= 0 && h[1] < shown.size()) setPreview(shown.get(h[1])); return;
            case HIT_INSERT: if (h[1] >= 0 && h[1] < shown.size()) insert(shown.get(h[1])); return;
            case HIT_PREV: flip(-1); return;
            case HIT_NEXT: flip(1); return;
            default:
        }
    }

    private void flip(int step) {
        int to = Math.max(0, Math.min(pages - 1, page + step));
        if (to != page) { page = to; invalidate(); }
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        if (inserting != null) return true;
        int action = e.getActionMasked();
        if (preview != null) {
            if (action == MotionEvent.ACTION_UP) { if (tooltip.inside(e.getX(), e.getY()) && tooltip.flip(1)) invalidate(); else setPreview(null); }
            return true;
        }
        if (action == MotionEvent.ACTION_DOWN) { downX = e.getX(); downY = e.getY(); downHit = hitAt(downX, downY); }
        else if (action == MotionEvent.ACTION_UP) {
            float dx = e.getX() - downX, dy = e.getY() - downY, swipe = getWidth() * 0.06f;
            if (Math.abs(dx) > swipe && Math.abs(dx) > Math.abs(dy) * 2) flip(dx < 0 ? 1 : -1);
            else if (downHit >= 0 && downHit == hitAt(e.getX(), e.getY())) act(hits.get(downHit));
            else if (typing.editing() && downHit < 0) endEditing();
            downHit = -1;
        } else if (action == MotionEvent.ACTION_CANCEL) downHit = -1;
        return true;
    }






    private String qaState() {
        if (!isOpen()) return "closed";
        if (inserting != null) return "inserting";
        if (charData == RecipeData.DATA_VERSION && (data == null || (query.isEmpty() && tab == TAB_DOABLE && results == null && !stockFailed))) return "loading";
        return "open";
    }
    private void qa() { if (!qaQuiet) TestHooks.overlay(this, "recipe_book", qaState()); }
    private void qaSearch() { TestHooks.overlay(this, "recipe_search", typing.editing() ? "editing" : "idle"); }
    @Override public void setVisibility(int visibility) { super.setVisibility(visibility); qa(); }
}
