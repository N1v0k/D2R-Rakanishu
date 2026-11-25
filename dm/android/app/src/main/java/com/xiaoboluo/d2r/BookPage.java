package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Shader;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import org.json.JSONArray;
import org.json.JSONObject;


















final class BookPage extends View implements PadPage, QuickPage {


    static final int HIT_CLOSE = 0, HIT_TILE = 1, HIT_TILE_PICTURE = 2, HIT_BADGE = 3, HIT_ACT = 4, HIT_AREA = 5, HIT_ACT_PREV = 6, HIT_ACT_NEXT = 7,
        HIT_DIFF = 8, HIT_PLAYERS = 9, HIT_TYPE = 10, HIT_SORT = 11, HIT_ORDER = 12, HIT_SEARCH = 13, HIT_CLEAR = 14, HIT_PREV = 15, HIT_NEXT = 16,
        HIT_BACK = 17, HIT_SECTION = 18, HIT_FIGHT = 19, HIT_PICTURE = 20, HIT_PLACE = 21, HIT_SECTION_PREV = 22, HIT_SECTION_NEXT = 23,
        HIT_LOOT_GROUP = 24, HIT_LOOT_ROW = 25, HIT_LOOT_PREV = 26, HIT_LOOT_NEXT = 27, HIT_PLACE_RIBBON = 28;

    static final int SECTION_FIGHT = 0, SECTION_STATS = 1, SECTION_PLACES = 2, SECTION_LOOT = 3;

    static final int MAX_FOUND = 12;
    private static final long MON_BYTES = 8L << 20;
    private static final int SEARCH_SLOT = 4;

    private final AppArt art;
    private final GameStyle style;
    private final CombatPage combat;
    private final Runnable freeInput;
    private final GameText body, dim, gold, legend, input, mark;
    private final Paint pic = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), grey = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), band = new Paint(),
        badgeRim = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint ribbonCloth = new Paint(Paint.ANTI_ALIAS_FLAG), ribbonShadow = new Paint(Paint.ANTI_ALIAS_FLAG), ribbonShine = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Ribbon tileRibbon = new Ribbon(), rowRibbon = new Ribbon();
    private float ribbonSide;
    private static final float ROW_RIBBON = 1.8f;
    private final ExecutorService worker = Executors.newSingleThreadExecutor(r -> new Thread(r, "book-page"));
    private final ExecutorService monWorker = Executors.newSingleThreadExecutor(r -> new Thread(r, "book-pictures"));
    private final MonLoader<Bitmap> mon, inv;
    private final Set<String> monUnknown = new HashSet<>();
    private final List<String> monShown = new ArrayList<>(), monAsked = new ArrayList<>(), invShown = new ArrayList<>(), invAsked = new ArrayList<>();
    private int monDrawn;
    private final Runnable monAsk, invAsk;
    private final SearchTyping typing = new SearchTyping(this, this::searched, this::qaSearch);
    private final Runnable retry = () -> request(false);


    private final List<RectF> boxes = new ArrayList<>();
    private final List<Runnable> actions = new ArrayList<>();
    private final List<int[]> ids = new ArrayList<>();
    private final List<Boolean> focusable = new ArrayList<>();
    private final RectF frame = new RectF(), r = new RectF(), cell = new RectF();
    private final float[] box = new float[4], tileBox = new float[4], picBox = new float[4];

    private boolean paused, busy, failed, stale, padUsed, arrows, downOutside;
    private int epoch, diff = -1, players = -1, page, pages = 1, downHit = -1, focusKind = -1, focusWhich, nameLang, fontLogged = -1, section;
    private CombatModel report;
    private int[] reportOpts;
    private BookModel book;
    private final BookModel.Filter filter = new BookModel.Filter();
    private List<BookModel.Tile> tiles = new ArrayList<>();

    private List<BookModel.Listed> listed = Collections.emptyList();

    private BookData drops;


    private List<BookLoot.Row> lootRows;
    private String lootFor = "", lootWant = "";
    private boolean[] lootOpen = BookLoot.defaultOpen();
    private int lootPage, lootPages = 1;
    private final List<BookLoot.Line> lootLines = new ArrayList<>();
    private final List<String> lootTexts = new ArrayList<>(), lootRatios = new ArrayList<>(), lootPics = new ArrayList<>();
    private String lootNote = "";


    private java.util.Set<String> foundItems = Collections.emptySet();
    private java.util.Map<String, Double> foundChance;
    private String foundFor = "";
    private boolean foundTooMany;
    private int searchEpoch;
    private JSONObject names = new JSONObject();
    private BookModel.Kills kills;
    private int heroAct, heroArea;

    private int where;
    private boolean whereOpen;
    private int[] whereChoices = BookModel.Where.choices(false);
    private String[] whereTexts = new String[0];
    private String whereText = "";
    private final RectF whereBox = new RectF();
    private float tabBottom;
    private static final float WHERE_SHARE = 0.4f;
    private final GameText buttonText;
    private String kindKey;
    private int kindArea, gridPage, gridFocus = -1;

    private String heroLine = "", kindTitle = "", ratingText = "", defeated = "";
    private final List<String> fightLines = new ArrayList<>();


    private final java.util.LinkedHashMap<String, CombatModel> details = new java.util.LinkedHashMap<String, CombatModel>(16, .75f, true) {
        @Override protected boolean removeEldestEntry(java.util.Map.Entry<String, CombatModel> eldest) { return size() > 8; }
    };
    private String detailAsked, detailFailed;
    private final List<String> sectionLines = new ArrayList<>(), sectionColours = new ArrayList<>();
    private final List<CombatModel.Enemy> places = new ArrayList<>();
    private final List<String> placeTexts = new ArrayList<>();
    private int sectionPage, sectionPages = 1;
    private final List<String> fightColours = new ArrayList<>();
    private final List<String> tileRaw = new ArrayList<>(), tileRatios = new ArrayList<>(), tileNames = new ArrayList<>(), tilePictures = new ArrayList<>();
    private final List<Boolean> tileGrey = new ArrayList<>();
    private final List<Float> tileK = new ArrayList<>();
    private String kindPicture;
    private int laidW, laidH, laidTexts = -1, texts;
    private float laidB;

    BookPage(Context context, AppArt art, GameStyle style, CombatPage combat, Runnable freeInput) {
        super(context);
        this.art = art; this.style = style; this.combat = combat; this.freeInput = freeInput;
        body = style.text(GameStyleData.BODY);
        dim = style.text(GameStyleData.BODY).colour("FontColorLightGray");
        gold = style.text(GameStyleData.BUTTON_LABEL).colour("FontColorGoldYellow");
        legend = style.text(GameStyleData.LEGEND);
        input = style.text(GameStyleData.BOOK_INPUT);
        mark = style.text(GameStyleData.BOOK_INPUT).colour("FontColorLightGray");
        band.setColor(0xC0000000);
        badgeRim.setStyle(Paint.Style.STROKE); badgeRim.setStrokeCap(Paint.Cap.ROUND);
        ribbonShine.setStyle(Paint.Style.STROKE); ribbonShadow.setColor(0x8C000000);
        ColorMatrix desaturate = new ColorMatrix(); desaturate.setSaturation(0);
        grey.setColorFilter(new ColorMatrixColorFilter(desaturate)); grey.setAlpha(120);
        buttonText = style.text(GameStyleData.PANEL_BUTTON);
        mon = new MonLoader<>(MON_BYTES, Bitmap::getAllocationByteCount, monWorker, this::post, art::monPicture, new MonLoader.Listener() {
            public void loaded() { qa(); invalidate(); }
            public void failed(String name, boolean small) { AppArt.log("ASSUME book.monpic: the enemy picture " + MonLoader.key(name, small) + " is unreadable or beyond its size"); prepare(); invalidate(); }
        });
        monAsk = () -> mon.show(monAsked);
        inv = new MonLoader<>(2L << 20, Bitmap::getAllocationByteCount, monWorker, this::post, (name, small) -> art.invPicture(name), new MonLoader.Listener() {
            public void loaded() { invalidate(); }
            public void failed(String name, boolean small) { AppArt.log("ASSUME book.invpic: the item picture " + name + " is unreadable or beyond its size"); invalidate(); }
        });
        invAsk = () -> inv.show(invAsked);
        setFocusableInTouchMode(true);
        setVisibility(GONE);
    }


    private int lang() { return GameLang.german() ? CombatModel.L_DE : GameLang.PT.equals(GameLang.code()) ? CombatModel.L_PT : CombatModel.L_EN; }

    private final java.util.HashMap<String, Integer> textIds = new java.util.HashMap<>();
    private final BookText w = new BookText(key -> {
        Integer id = textIds.get(key);
        if (id == null) { id = getResources().getIdentifier(key, "string", getContext().getPackageName()); textIds.put(key, id); }
        return id == 0 ? null : AppText.res(getContext()).getString(id);
    });
    private String t(String key, Object... args) { return w.t("dmAndroidBook" + key, args); }
    private String name(String key, int language) { JSONArray a = names.optJSONArray(key); return a == null ? key : a.optString(language, key); }
    private String shownName(String key) { return name(key, nameLang); }

    private void chooseNameLanguage() {
        int language = GameLang.index();
        if (language < 0) language = GameLang.german() ? 1 : GameLang.PT.equals(GameLang.code()) ? 12 : 0;
        StringBuilder sample = new StringBuilder();
        for (int g = 0; g < 3; g++) { JSONArray a = names.optJSONArray("@potion:" + g); if (a != null) sample.append(a.optString(language, "")); }
        String text = sample.toString();
        nameLang = text.isEmpty() || style.covers(GameStyle.GLOBAL, text) || style.covers(GameStyle.FORMAL, text) ? language : 0;
        if (nameLang != language && language != fontLogged) { fontLogged = language; AppArt.log("ASSUME book.name_fonts: game names of language " + language + " not drawable, English names shown"); }
    }
    private String location(CombatModel.Enemy e) {
        return e.area > 0 ? (e.act > 0 ? t("ActArea", e.act, shownName("@area:" + e.area)) : shownName("@area:" + e.area)) : t("Event");
    }
    private String immunity(CombatModel.Enemy e) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < 6; i++) if (e.resistance[i] >= 100) { if (s.length() > 0) s.append(", "); s.append(BookStats.element(i, w)); }
        return s.toString();
    }
    private String typeLabel() {
        int t = filter.type;
        return t(t < 0 ? "AllTypes" : t == CombatModel.REGULAR ? "Regular" : t == CombatModel.SUPERUNIQUE ? "MiniBosses" : "Bosses");
    }
    private String sortLabel() {
        int s = filter.sort;
        return t(s == BookModel.SORT_KILL ? "KillTime" : s == BookModel.SORT_DEATH ? "DeathTime" : s == BookModel.SORT_RATING ? "OrderRating" : "OrderPlace");
    }

    private String diffLabel = ""; private int diffLabelFor = -2, diffLabelLang = -1;
    private String difficulty() {
        if (diff != diffLabelFor || lang() != diffLabelLang) { diffLabelFor = diff; diffLabelLang = lang(); diffLabel = diff < 0 ? "…" : CombatModel.difficultyLabel(diff, lang()).replaceFirst("^[^:]*: ", ""); }
        return diffLabel;
    }
    private String playersShort() { return players < 1 ? "…" : t("PlayersShort", players); }
    private String orderLabel() { return arrows ? (filter.descending ? "↓" : "↑") : t(filter.descending ? "Down" : "Up"); }


    @Override public boolean available() { return DmActivity.nativeInGame() != 0; }
    @Override public void takePause() { if (!paused) { paused = true; DmActivity.nativePause(DmActivity.PAUSE_MONBOOK, 1); } }
    @Override public void open() {
        DmActivity.syncLang();
        style.refresh();
        takePause();
        arrows = style.covers(GameStyle.FORMAL, "↑↓");
        typing.end(); typing.setText("");
        filter.query = ""; filter.type = -1; filter.sort = BookModel.SORT_PLACE; filter.descending = false;
        epoch++; diff = -1; players = -1; report = null; reportOpts = null; book = null; tiles = new ArrayList<>(); failed = false; stale = false;
        page = 0; gridPage = 0; gridFocus = -1; focusKind = -1; kindKey = null; section = SECTION_FIGHT; details.clear(); detailAsked = null; detailFailed = null; sectionPage = 0; heroAct = 0; heroArea = 0; where = BookModel.Where.ALL; whereOpen = false;
        padUsed = PadInput.isPadMode();
        mon.reset(); monAsked.clear(); inv.reset(); invAsked.clear();
        lootRows = null; lootFor = ""; lootWant = ""; lootOpen = BookLoot.defaultOpen(); lootPage = 0;
        foundItems = Collections.emptySet(); foundChance = null; foundFor = ""; foundTooMany = false; searchEpoch++;
        setVisibility(VISIBLE);
        freeInput.run();
        kills = null;
        worker.execute(() -> {
            JSONObject n = read("overlay/sim-names.json");
            BookModel.Kills k = readKills();
            BookData d = art.book();
            post(() -> { names = n; kills = k; drops = d; if (report != null) arrived(report, reportOpts); prepare(); qa(); invalidate(); });
        });
        request(true);
    }
    private JSONObject read(String path) {
        try {
            File f = new File(getContext().getFilesDir(), "dm/" + path);
            if (f.length() > 4 * 1024 * 1024) return new JSONObject();
            return new JSONObject(new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8));
        } catch (Exception e) { AppArt.log("ASSUME book.names: name catalog unavailable"); return new JSONObject(); }
    }
    boolean isOpen() { return getVisibility() == VISIBLE; }
    void close() {
        typing.end();
        epoch++; searchEpoch++; removeCallbacks(retry); mon.reset(); monAsked.clear(); inv.reset(); invAsked.clear();
        setVisibility(GONE);
        if (paused) { paused = false; DmActivity.nativePause(DmActivity.PAUSE_MONBOOK, 0); }
    }
    void stop() { if (isOpen()) close(); }
    void dispose() { stop(); worker.shutdownNow(); monWorker.shutdownNow(); }



    private void showFight(CombatModel.Enemy e) {
        if (e == null || reportOpts == null) return;
        typing.end();
        combat.openFight(reportOpts[0], reportOpts[1], e.id, this::comeBack);
        mon.reset(); monAsked.clear(); inv.reset(); invAsked.clear();
        setVisibility(GONE);
        if (paused) { paused = false; DmActivity.nativePause(DmActivity.PAUSE_MONBOOK, 0); }
    }

    private static BookModel.Kills readKills() {
        byte[] b = DmActivity.nativeBookKills();
        if (b == null) { AppArt.log("book: no kills from the host"); return null; }
        try { return BookModel.kills(new String(b, StandardCharsets.UTF_8)); }
        catch (IllegalArgumentException e) { AppArt.log("book.kills: " + e); return null; }
    }

    private boolean unbeaten(BookModel.Kind k) { return kills != null && kills.known() && k != null && kills.count(k.key, diff) == 0; }
    private void comeBack() {
        takePause();
        setVisibility(VISIBLE);
        freeInput.run();
        prepare(); qa(); invalidate();

        if (report == null || !java.util.Arrays.equals(options(), reportOpts) && !(diff < 0 || players < 0)) request(false);
    }


    private int[] options() { return CombatModel.options(diff, players, false, false, 0, -1, false, Collections.emptyList()); }
    private void change() { epoch++; request(true); }
    private void request(boolean changed) {
        removeCallbacks(retry);
        if (!isOpen()) return;
        if (changed) failed = false;
        int[] opts = options();
        stale = report != null && !java.util.Arrays.equals(opts, reportOpts) && !(diff < 0 || players < 0);
        qa(); invalidate();
        if (busy) { postDelayed(retry, 300); return; }
        int token = epoch;
        busy = true;
        worker.execute(() -> {
            CombatModel result = null;
            try { byte[] bytes = DmActivity.nativeCombat(opts); if (bytes != null) result = CombatModel.parse(new String(bytes, StandardCharsets.UTF_8)); }
            catch (RuntimeException e) { AppArt.log("book.response: " + e); result = null; }
            final CombatModel ready = result;
            post(() -> {
                busy = false;
                if (token != epoch) { if (isOpen()) request(false); return; }
                failed = ready == null;
                if (ready != null) arrived(ready, opts);
                else if (isOpen()) postDelayed(retry, 1500);
                stale = false;
                qa(); invalidate();
            });
        });
    }
    private void arrived(CombatModel m, int[] opts) {
        boolean first = report == null;
        report = m;
        reportOpts = opts.clone();
        if (reportOpts[0] < 0) reportOpts[0] = m.difficulty;
        if (reportOpts[1] < 0) reportOpts[1] = m.players;
        if (diff < 0) diff = m.difficulty;
        if (players < 0) players = m.players;
        if (first) { heroAct = m.act; heroArea = m.refArea > 0 ? m.refArea : m.area; }

        where = first ? BookModel.Where.first(areaAvailable(), heroAct) : BookModel.Where.kept(where, areaAvailable(), heroAct);
        BookModel.Where.apply(where, heroArea, filter);
        listed = listedKinds();
        book = BookModel.of(m, listed);

        if (kindKey != null && book.find(kindKey) == null) leaveKind();
        refilter();
        if (kindKey != null) requestDetail();
        searchDrops();
        requestLoot();
    }

    private BookData.Set dropSet() { return drops == null || report == null ? null : drops.sets.get(report.rotw ? "rotw" : "lod"); }

    private List<BookModel.Listed> listedKinds() {
        BookData.Set s = dropSet();
        if (s == null) return Collections.emptyList();
        List<Integer> keys = new ArrayList<>(s.extra.keySet());
        Collections.sort(keys);
        List<BookModel.Listed> out = new ArrayList<>();
        for (int k : keys) {
            BookData.Extra x = s.extra.get(k);
            List<String> parents = new ArrayList<>();
            for (int p : x.parents) parents.add("M" + p);
            out.add(new BookModel.Listed("M" + k, x.look, x.name, parents));
        }
        return out;
    }
    private double foundDrop(BookModel.Kind k, CombatModel.Enemy e) {
        Double c = foundChance == null ? null : foundChance.get(k.key + "|" + (e == null ? "-" : Integer.toString(e.id)));
        return c == null ? 0 : c;
    }
    private void refilter() {
        tiles = book == null ? new ArrayList<>() : book.tiles(filter, this::name, foundChance == null ? null : this::foundDrop);
        pages = BookLayout.pages(tiles.size());
        page = BookLayout.clamp(page, pages);
        prepare();
        qa();
    }



    private boolean areaAvailable() { return BookModel.Where.areaHas(report, heroArea); }
    private void pickWhere(int to) {
        if (BookModel.Where.index(to, areaAvailable()) < 0) return;
        boolean was = whereOpen;
        whereOpen = false;
        if (was && padUsed) { focusKind = HIT_ACT; focusWhich = 0; }
        if (to == where) { qa(); invalidate(); return; }
        where = to;
        BookModel.Where.apply(where, heroArea, filter);
        page = 0; refilter(); invalidate();
    }
    private void stepWhere(int d) { pickWhere(BookModel.Where.step(where, d, areaAvailable())); }
    private void openWhere(boolean open) {
        whereOpen = open;
        if (open && padUsed) { focusKind = HIT_AREA; focusWhich = where; }
        else if (!open && padUsed && focusKind == HIT_AREA) { focusKind = HIT_ACT; focusWhich = 0; }
        qa(); invalidate();
    }
    private String whereLabel(int at) { return at == BookModel.Where.AREA ? shownName("@area:" + heroArea) : at == BookModel.Where.ALL ? t("AllActs") : t("Act", at); }
    private void searched() { filter.query = typing.text().trim(); page = 0; if (kindKey != null) leaveKind(); refilter(); searchDrops(); invalidate(); }


    private void searchDrops() {
        String q = CombatModel.fold(filter.query.trim());
        BookData d = drops;
        BookData.Set s = dropSet();
        BookModel b = book;
        if (q.length() < 2 || d == null || s == null || b == null || reportOpts == null || report == null) {
            boolean had = foundChance != null || foundTooMany || !foundItems.isEmpty();
            foundItems = Collections.emptySet(); foundChance = null; foundFor = ""; foundTooMany = false; searchEpoch++;
            if (had) { refilter(); invalidate(); }
            return;
        }
        int dff = reportOpts[0], pl = reportOpts[1], mf = report.magicFind;
        String want = q + "|" + dff + "|" + pl + "|" + mf + "|" + System.identityHashCode(b);
        if (want.equals(foundFor)) return;
        foundFor = want;
        int token = ++searchEpoch;
        worker.execute(() -> {
            List<int[]> m = BookLoot.match(d, s, q);
            boolean many = m.size() > MAX_FOUND;
            java.util.Set<String> keys = new HashSet<>();
            java.util.Map<String, Double> chance = null;
            if (!many && !m.isEmpty()) {
                long t0 = android.os.SystemClock.uptimeMillis();
                for (int[] t : m) keys.add(t[0] + ":" + t[1]);
                chance = BookLoot.chances(b, s, dff, pl, mf, m);
                AppArt.log("book.search: " + m.size() + " items, " + chance.size() + " droppers in " + (android.os.SystemClock.uptimeMillis() - t0) + " ms");
            }
            final java.util.Map<String, Double> ready = chance;
            final java.util.Set<String> found = many ? Collections.emptySet() : keys;
            post(() -> {
                if (token != searchEpoch) return;
                foundItems = found; foundChance = ready; foundTooMany = many;
                prepareLoot();
                refilter(); invalidate();
            });
        });
    }
    private void setQuery(String q) { typing.setText(q); searched(); }
    private void beginEditing() { if (!isOpen()) return; if (whereOpen) openWhere(false); typing.begin(); }


    private void openKind(BookModel.Tile t, int place) {
        gridPage = page; gridFocus = place; whereOpen = false;
        kindKey = t.kind.key;
        CombatModel.Enemy e = BookModel.encounter(t.kind, heroArea, filter);
        if (e == null && book != null) e = book.place(t.kind, heroArea, filter);
        kindArea = e == null ? 0 : e.area;

        section = t.chance > 0 ? SECTION_LOOT : SECTION_FIGHT; sectionPage = 0; focusKind = -1;
        lootOpen = BookLoot.defaultOpen(); lootPage = 0;
        requestLoot();
        prepare(); qa(); invalidate();
        requestDetail();
    }

    private void pickPlace(int area) { kindArea = area; sectionPage = 0; prepare(); qa(); invalidate(); requestDetail(); }
    private void pickSection(int s) { section = s; sectionPage = 0; qa(); invalidate(); }
    private void leaveKind() {
        kindKey = null; page = BookLayout.clamp(gridPage, pages);
        if (gridFocus >= 0) { focusKind = HIT_TILE; focusWhich = gridFocus; }
        prepare(); qa(); invalidate();
    }
    private BookModel.Kind kind() { return book == null || kindKey == null ? null : book.find(kindKey); }
    private CombatModel.Enemy shownEncounter() { return BookModel.encounter(kind(), kindArea, filter); }
    private List<Integer> sections() {
        List<Integer> out = new ArrayList<>();
        if (shownEncounter() != null) { out.add(SECTION_FIGHT); out.add(SECTION_STATS); }
        if (!BookModel.places(kind()).isEmpty()) out.add(SECTION_PLACES);
        if (lootClass() >= 0) out.add(SECTION_LOOT);
        return out;
    }
    private String sectionName(int s) {
        return t(s == SECTION_STATS ? "Values" : s == SECTION_PLACES ? "Places" : s == SECTION_LOOT ? "Loot" : "Fight");
    }

    private String detailWanted() {
        CombatModel.Enemy e = shownEncounter();
        return e == null || reportOpts == null ? null : kindKey + "/" + e.area + "/" + reportOpts[0] + "/" + reportOpts[1];
    }
    private void requestDetail() {
        String want = detailWanted();
        CombatModel.Enemy e = shownEncounter();
        if (want == null || details.containsKey(want) || want.equals(detailAsked) || want.equals(detailFailed) || !isOpen()) return;
        detailAsked = want;
        int[] opts = CombatModel.options(reportOpts[0], reportOpts[1], false, false, 0, e.id, false, Collections.emptyList());
        worker.execute(() -> {
            CombatModel result = null;
            try { byte[] bytes = DmActivity.nativeCombat(opts); if (bytes != null) result = CombatModel.parse(new String(bytes, StandardCharsets.UTF_8)); }
            catch (RuntimeException x) { AppArt.log("book.detail: " + x); }
            final CombatModel ready = result;
            post(() -> {
                if (!want.equals(detailAsked)) return;
                detailAsked = null;
                if (ready == null || ready.stats == null) { AppArt.log("book.detail: no values for " + want); detailFailed = want; }
                else details.put(want, ready);
                if (!want.equals(detailWanted())) { requestDetail(); return; }
                prepare(); qa(); invalidate();
            });
        });
    }


    private int lootClass() {
        BookData.Set s = dropSet();
        return s == null || kindKey == null || reportOpts == null ? -1 : BookLoot.dropClass(s, kindKey, reportOpts[0]);
    }


    private void requestLoot() {
        BookData d = drops;
        BookData.Set s = dropSet();
        BookModel.Kind k = kind();
        if (s == null || lootClass() < 0 || k == null || report == null) { lootRows = null; lootWant = ""; lootFor = ""; prepareLoot(); return; }
        CombatModel.Enemy e = shownEncounter();
        int dff = reportOpts[0], pl = reportOpts[1], mf = report.magicFind, flags = BookLoot.flags(s, kindKey);
        int level = BookLoot.level(s, kindKey, dff, e, e == null && book != null ? book.place(k, kindArea, filter) : null);
        int root = level < 0 ? -1 : BookLoot.rootClass(s, kindKey, dff, level), lang = nameLang;
        String want = kindKey + "|" + dff + "|" + pl + "|" + mf + "|" + level + "|" + lang + "|" + (report.rotw ? 1 : 0);
        if (want.equals(lootWant) || want.equals(lootFor) && lootRows != null) return;

        lootFor = ""; lootRows = null;
        if (root < 0) { lootRows = new ArrayList<>(); lootFor = want; lootWant = ""; prepareLoot(); return; }
        lootWant = want;
        worker.execute(() -> {
            long t0 = android.os.SystemClock.uptimeMillis();
            List<BookLoot.Row> rows = BookLoot.rows(d, s, new DropCalc(s, dff, level, pl, mf, flags).all(root), lang);
            AppArt.log("book.loot: " + rows.size() + " rows in " + (android.os.SystemClock.uptimeMillis() - t0) + " ms");
            post(() -> {
                if (!want.equals(lootWant)) return;
                lootRows = rows; lootFor = want; lootWant = ""; lootPage = 0;
                prepareLoot(); qa(); invalidate();
            });
        });
        prepareLoot();
    }

    private void prepareLoot() {
        lootLines.clear(); lootTexts.clear(); lootRatios.clear(); lootPics.clear();
        CombatModel m = report;
        lootNote = m == null ? "" : m.elysium ? t("Standard", t("PerKill", m.magicFind)) : t("PerKill", m.magicFind);
        if (lootRows == null) return;
        lootLines.addAll(BookLoot.lines(lootRows, lootOpen, foundItems));
        InvPictures ip = art.invPictures();
        for (BookLoot.Line l : lootLines) {
            if (l.row == null) {
                lootTexts.add(t("Group", groupName(l.group), l.count));
                lootRatios.add("");
                lootPics.add(null);
            } else {
                lootTexts.add(l.row.name);
                lootRatios.add(BookLoot.ratio(l.row.chance, lang()));
                lootPics.add(ip == null ? null : ip.name(l.row.code, l.row.kind == DropCalc.UNIQUE ? 7 : l.row.kind == DropCalc.SET ? 5 : -1, l.row.id, -1));
            }
        }
        fits.clear();
    }
    private String groupName(int g) {
        switch (g) {
            case BookLoot.UNIQUES: return t("GroupUniques");
            case BookLoot.SETS: return t("GroupSets");
            case BookLoot.RUNES: return t("GroupRunes");
            case BookLoot.GEMS: return t("GroupGems");
            case BookLoot.JEWELLERY: return t("GroupJewellery");
            case BookLoot.WEAPONS: return t("GroupWeapons");
            case BookLoot.ARMOUR: return t("GroupArmour");
            default: return t("GroupOther");
        }
    }
    private void toggleGroup(int g) {
        if (g < 0 || g >= BookLoot.GROUPS) return;
        lootOpen[g] = !lootOpen[g];

        int at = 0;
        prepareLoot();
        for (int i = 0; i < lootLines.size(); i++) if (lootLines.get(i).row == null && lootLines.get(i).group == g) { at = i; break; }
        lootPage = lootPer > 0 ? at / lootPer : 0;
        qa(); invalidate();
    }
    private int lootPer = 1;
    private void lootFlip(int d) { int to = Math.max(0, Math.min(lootPages - 1, lootPage + d)); if (to != lootPage) { lootPage = to; qa(); } invalidate(); }


    private void prepare() {
        texts++;
        chooseNameLanguage();
        CombatModel m = report;
        heroLine = m == null ? "" : t("Hero", m.name, m.level, m.act, m.life, m.mana);
        whereChoices = BookModel.Where.choices(areaAvailable());
        if (whereOpen && focusKind == HIT_AREA && BookModel.Where.index(focusWhich, areaAvailable()) < 0) focusWhich = where;
        whereTexts = new String[whereChoices.length];
        for (int i = 0; i < whereChoices.length; i++) whereTexts[i] = whereLabel(whereChoices[i]);
        whereText = report == null ? "" : whereLabel(where);
        BookModel.Kind k = kind();
        kindTitle = k == null ? "" : shownName(k.nameKey);
        defeated = "";
        if (k != null && kills != null && kills.known()) {
            StringBuilder d = new StringBuilder();
            for (int x = 0; x < 3; x++) if (kills.count(k.key, x) > 0)
                d.append(d.length() == 0 ? "" : " · ").append(CombatModel.difficultyLabel(x, lang()).replaceFirst("^[^:]*: ", "")).append(' ').append(kills.count(k.key, x));
            if (d.length() > 0) defeated = t("Label", t("Defeated"), d);
        }
        kindPicture = k == null ? null : picture(k.look);
        fightLines.clear(); fightColours.clear(); ratingText = "";
        CombatModel.Enemy e = shownEncounter();
        if (e != null) {
            ratingText = CombatModel.light(e.light, lang());
            line(location(e), "FontColorLightGray");
            line(t("Label", t("KillTime"), CombatModel.time(e.kill, lang())), "FontColorWhite");
            line(t("Label", t("DeathTime"), CombatModel.time(e.death, lang())), "FontColorWhite");
            if (e.death != -2 && e.maxHit > 0) line(t("LargestHit", CombatModel.number(e.maxHit * 100, lang())), "FontColorWhite");
            if (!immunity(e).isEmpty()) line(t("Immune", immunity(e)), "FontColorWhite");
            if (e.underestimated && e.light >= 0 && e.light < 2) line(t("Underestimated"), "FontColorLightGray");
        }
        sectionLines.clear(); sectionColours.clear();
        String want = detailWanted();
        CombatModel detail = want == null ? null : details.get(want);
        if (e != null && detail != null) {
            for (BookStats.Line l : BookStats.values(detail.stats, e.resistance, lang(), w)) { sectionLines.add(l.text); sectionColours.add(l.colour); }
            List<String> abilities = BookStats.abilities(detail.stats, w, new BookStats.Names() {
                public String get(String key) { return shownName(key); }
                public boolean has(String key) { return names.has(key); }
            });
            if (!abilities.isEmpty()) { sectionLines.add(t("Abilities")); sectionColours.add("FontColorGoldYellow"); }
            for (String a : abilities) { sectionLines.add(a); sectionColours.add("FontColorWhite"); }
        }
        places.clear(); placeTexts.clear();
        for (CombatModel.Enemy x : BookModel.places(k)) {
            places.add(x);
            placeTexts.add((x.estimated ? t("Probably", location(x)) : location(x)) + " · " + t("Level", x.level));
        }
        List<Integer> secs = sections();
        if (!secs.contains(section)) section = secs.isEmpty() ? SECTION_FIGHT : secs.get(0);
        tileRaw.clear(); tileRatios.clear(); tilePictures.clear(); tileGrey.clear();
        for (int i = 0; i < BookLayout.onPage(tiles.size(), page); i++) {
            BookModel.Tile t = tiles.get(page * BookLayout.PER_PAGE + i);
            tileRaw.add(shownName(t.kind.nameKey));
            tileRatios.add(t.chance > 0 ? " · " + BookLoot.ratio(t.chance, lang()) : "");
            tilePictures.add(picture(t.kind.look));
            tileGrey.add(unbeaten(t.kind));
        }
        fits.clear();
        laidTexts = -1;
        if (kindKey != null) requestLoot();
        prepareLoot();
    }
    private void line(String text, String colour) { fightLines.add(text); fightColours.add(colour); }

    private String picture(String look) {
        MonPictures p = art.monPictures();
        if (p == null || look == null || look.isEmpty()) return null;
        if (!p.known(look)) { if (monUnknown.add(look)) AppArt.log("ASSUME book.listed: the import pack has no entry for the enemy look " + look); return null; }
        String n = p.name(look);
        return n == null || mon.failed(n, false) ? null : n;
    }

    private void layout(GameStyleGeometry.Panel p, float gridL, float gridT, float gridR, float gridB, float g, float k) {
        if (laidTexts == texts && laidW == getWidth() && laidH == getHeight() && laidB == gridB) return;
        laidTexts = texts; laidW = getWidth(); laidH = getHeight(); laidB = gridB;
        tileNames.clear(); tileK.clear();
        for (int i = 0; i < tileRaw.size(); i++) {
            BookLayout.tile(gridL, gridT, gridR, gridB, 10 * g, i, tileBox);

            float room = tileBox[2] - tileBox[0] - 16 * g, ratioW = tileRatios.get(i).isEmpty() ? 0 : body.measure(tileRatios.get(i), k);
            Fit f = fit(body, tileRaw.get(i), Math.max(room * 0.3f, room - ratioW), k);
            tileNames.add(f.text + tileRatios.get(i)); tileK.add(f.k);
        }
    }


    private void hit(RectF at, int kind, int index, boolean focus, Runnable action) {
        boxes.add(new RectF(at)); actions.add(action); ids.add(new int[]{kind, index}); focusable.add(focus);
        if (BuildConfig.DM_QA_BUILD) TestHooks.hit(at, kind, index);
    }
    private float base(GameText t, float top, float bottom, float k) { Paint q = t.paint(k); return GameStyleGeometry.centreBaseline(top, bottom, q.ascent(), q.descent()); }


    private static final class Fit { final String text; final float k, width; Fit(String text, float k, float width) { this.text = text; this.k = k; this.width = width; } }
    private final java.util.HashMap<String, Fit> fits = new java.util.HashMap<>();
    private Fit fit(GameText t, String s, float max, float k) {
        String key = System.identityHashCode(t) + "\n" + s + "\n" + max + "\n" + k;
        Fit f = fits.get(key);
        if (f != null) return f;
        float kk = t.fitTo(s, max, k);
        String out = s;
        if (t.measure(out, kk) > max) { while (out.length() > 1 && t.measure(out + "…", kk) > max) out = out.substring(0, out.length() - 1); out = out + "…"; }
        f = new Fit(out, kk, t.measure(out, kk));
        fits.put(key, f);
        return f;
    }
    private float fitted(Canvas c, GameText t, String s, float x, float top, float bottom, float max, float k, Paint.Align a) {
        if (s.isEmpty() || !(max > 0)) return 0;
        Fit f = fit(t, s, max, k);
        t.draw(c, f.text, x, base(t, top, bottom, f.k), f.k, a, 255);
        return f.width;
    }
    private void button(Canvas c, RectF at, String label, int kind, Runnable action) {
        style.panelButton(c, at, GameStyle.BUTTON_NORMAL, label);
        tall(at); hit(r, kind, 0, true, action);
    }
    @Override protected void onDraw(Canvas c) {
        if (!isOpen()) return;
        if (BuildConfig.DM_QA_BUILD) { TestHooks.begin(this, "monster_book"); monDrawn = 0; }
        monShown.clear(); invShown.clear();
        try {
            boxes.clear(); actions.clear(); ids.clear(); focusable.clear();
            style.backdrop(c);
            GameStyleGeometry.Panel p = style.panel(getWidth(), getHeight());
            style.panelFrame(c, p);
            frame.set(p.frameL, p.frameT, p.frameR, p.frameB);
            style.panelTab(c, p, 6, 0, t("Menu"), true);
            drawSearch(c, p);
            style.closeX(c, p);
            r.set(p.hitCloseL, p.hitCloseT, p.hitCloseR, p.hitCloseB); hit(r, HIT_CLOSE, 0, false, this::close);
            float g = 2 * p.k, k = g, m = 20 * p.s, x = p.contentL + m, right = p.contentR - m, y = p.contentT + m, bottom = p.contentB - m;
            float lineH = body.size(k) * 1.25f;
            float bh = p.buttonH();

            boolean grid = report != null && (kindKey == null || kind() == null);
            float headH = grid ? Math.max(lineH, bh) : lineH, heroRight = grid ? right - (right - x) * WHERE_SHARE - 16 * g : right;
            tabBottom = p.hitTabBottom;
            if (grid) y = Math.max(y, tabBottom + style.minTap() - bh);
            if (grid) whereBox.set(heroRight + 16 * g, y, right, y + bh);
            float calcW = stale || busy && report != null ? fit(dim, t("Calculating"), right - x, k).width : 0;
            body.colour("FontColorWhite");
            if (!heroLine.isEmpty()) fitted(c, body, heroLine, x, y, y + headH, heroRight - x - (calcW > 0 ? calcW + 16 * g : 0), k, Paint.Align.LEFT);
            if (calcW > 0) { dim.colour("FontColorLightGray"); dim.draw(c, t("Calculating"), heroRight, base(dim, y, y + headH, k), k, Paint.Align.RIGHT, 255); }
            y += headH + (grid ? Math.max(10 * g, (style.minTap() - bh) / 2 + 4 * g) : 10 * g);
            if (report == null) {
                dim.colour("FontColorLightGray");
                String s = failed ? t("Unavailable") : t("Calculating");
                fitted(c, dim, s, x, y + 20 * g, y + 20 * g + lineH, right - x, k, Paint.Align.LEFT);
            } else {
                float inner = padUsed ? bottom - lineH - 8 * g : bottom;
                if (kindKey != null && kind() != null) drawKind(c, p, x, right, y, inner, bh, lineH, g, k);
                else drawGrid(c, p, x, right, y, inner, bh, lineH, g, k);
            }
            if (padUsed) { gold.colour("FontColorGoldYellow"); fitted(c, gold, legendLine(), x, bottom - lineH, bottom, right - x, k * 0.8f, Paint.Align.LEFT); }
            drawFocus(c, g);
        } finally {
            if (!monShown.equals(monAsked)) { monAsked.clear(); monAsked.addAll(monShown); post(monAsk); }
            if (!invShown.equals(invAsked)) { invAsked.clear(); invAsked.addAll(invShown); post(invAsk); }
            if (BuildConfig.DM_QA_BUILD) {
                int waiting = 0;
                for (String key : monShown) if (mon.get(key, false) == null) waiting++;
                TestHooks.overlay("monster_book_pics", "w" + waiting + "_" + monDrawn);
                qa();
                TestHooks.end(this);
            }
        }
    }



    private void drawSearch(Canvas c, GameStyleGeometry.Panel p) {
        p.searchField(SEARCH_SLOT, 10 * p.s, box);
        float x0 = box[0], top = box[1], x1 = box[2], bottom = box[3], rowH = bottom - top, pad = rowH * 0.4f;
        cell.set(x0, top, x1, bottom);
        style.inputField(c, cell);
        r.set(x0, 0, x1, p.hitTabBottom); hit(r, HIT_SEARCH, 0, false, this::beginEditing);
        float k = 2 * p.k, ki = body.size(k) / input.units(), room = x1 - x0 - 2 * pad - rowH;
        String text = typing.text();
        if (text.isEmpty() && !typing.editing()) {
            Fit f = fit(input, t("Search"), room, ki);
            input.draw(c, f.text, x0 + pad, base(input, top, bottom, f.k), f.k, Paint.Align.LEFT, GameStyleData.HINT_ALPHA);
        } else {
            if (typing.caretOn()) text = text + "|";
            input.draw(c, tail(text, room, ki), x0 + pad, base(input, top, bottom, ki), ki, Paint.Align.LEFT, 255);
        }
        if (filter.query.isEmpty() && typing.isEmpty()) return;
        mark.draw(c, "×", x1 - pad - rowH * 0.25f, base(mark, top, bottom, ki), ki, Paint.Align.CENTER, 255);
        r.set(x1 - pad - rowH * 0.8f, 0, x1, p.hitTabBottom); hit(r, HIT_CLEAR, 0, false, () -> setQuery(""));
    }

    private void drawGrid(Canvas c, GameStyleGeometry.Panel p, float x, float right, float y, float bottom, float bh, float lineH, float g, float k) {
        drawWhere(c, whereBox, g);


        float gap = 10 * g, unit = (right - x - 4 * gap) / 5f;
        float[] widths = {unit * 0.9f, unit * 0.85f, unit * 1.2f, unit * 1.55f, unit * 0.5f};
        String[] labels = {difficulty(), playersShort(), typeLabel(), sortLabel(), orderLabel()};
        int[] kinds = {HIT_DIFF, HIT_PLAYERS, HIT_TYPE, HIT_SORT, HIT_ORDER};
        Runnable[] acts = {
            () -> { if (diff < 0) return; diff = (diff + 1) % 3; change(); },
            () -> { if (players < 1) return; players = CombatModel.nextPlayers(players); change(); },
            () -> { int t = filter.type; filter.type = t < 0 ? CombatModel.REGULAR : t == CombatModel.REGULAR ? CombatModel.SUPERUNIQUE : t == CombatModel.SUPERUNIQUE ? CombatModel.ACT_BOSS : -1; page = 0; refilter(); invalidate(); },
            () -> { filter.sort = (filter.sort + 1) % BookModel.SORTS; page = 0; refilter(); invalidate(); },
            () -> { filter.descending = !filter.descending; page = 0; refilter(); invalidate(); } };
        float bx = x;
        for (int i = 0; i < 5; i++) { r.set(bx, y, bx + widths[i], y + bh); button(c, r, labels[i], kinds[i], acts[i]); bx += widths[i] + gap; }
        y += bh + Math.max(14 * g, (style.minTap() - bh) / 2 + 4 * g);
        if (foundTooMany) {
            dim.colour("FontColorLightGray");
            fitted(c, dim, t("TooMany"), x, y, y + lineH, right - x, k, Paint.Align.LEFT);
            y += lineH + 8 * g;
        }

        float gridB = bottom;
        layout(p, x, y, right, gridB, g, k);
        int n = BookLayout.onPage(tiles.size(), page);
        if (n == 0) { dim.colour("FontColorLightGray"); fitted(c, dim, t("NoResults"), x, y + 20 * g, y + 20 * g + lineH, right - x, k, Paint.Align.LEFT); }
        BookLayout.tile(x, y, right, gridB, 10 * g, 0, tileBox);
        ribbonSide = Math.min(tileBox[2] - tileBox[0], tileBox[3] - tileBox[1]) - 6 * g;
        for (int i = 0; i < n; i++) drawTile(c, tiles.get(page * BookLayout.PER_PAGE + i), i, x, y, right, gridB, g, k);
        if (pages > 1) {
            style.footButton(c, p, getWidth(), 0, GameStyle.BUTTON_NORMAL, "‹", -1); p.footHit(getWidth(), 0, box);
            r.set(box[0], box[1], box[2], box[3]); hit(r, HIT_PREV, 0, false, () -> flip(-1));
            style.footText(c, p, getWidth(), 1, (page + 1) + " / " + pages, "FontColorLightGray");
            style.footButton(c, p, getWidth(), 2, GameStyle.BUTTON_NORMAL, "›", 1); p.footHit(getWidth(), 2, box);
            r.set(box[0], box[1], box[2], box[3]); hit(r, HIT_NEXT, 0, false, () -> flip(1));
        }
        if (whereOpen) drawWhereList(c, whereBox.bottom + 6 * g, bottom, g);
    }


    private void drawWhere(Canvas c, RectF at, float g) {
        style.panelButton(c, at, whereOpen ? GameStyle.BUTTON_DOWN : GameStyle.BUTTON_NORMAL, null);
        up(at); hit(r, HIT_ACT, 0, true, () -> openWhere(!whereOpen));
        float u = at.height() / 60f, aw = 27 * u, ah = 22 * u, inset = 26 * u, cy = at.centerY();
        int i = whereIndex();
        for (int side = 0; side < 2; side++) {
            boolean end = side == 0 ? i <= 0 : i >= whereChoices.length - 1;
            int kind = side == 0 ? HIT_ACT_PREV : HIT_ACT_NEXT;
            boolean focused = padUsed && focusKind == kind;
            float x0 = side == 0 ? at.left + inset : at.right - inset - aw;
            cell.set(x0, cy - ah / 2, x0 + aw, cy + ah / 2);
            Bitmap b = art.image("quick/arrow_" + (side == 0 ? "l" : "r") + "_" + (end ? 2 : focused ? 3 : 0));
            if (b != null) c.drawBitmap(b, null, cell, pic);
            float reach = Math.max(inset + aw + 8 * u, style.minTap() * 0.7f);
            cell.set(side == 0 ? at.left - 4 * g : at.right - reach, at.top, side == 0 ? at.left + reach : at.right + 4 * g, at.bottom);
            up(cell); hit(r, kind, 0, true, end ? () -> { } : side == 0 ? () -> stepWhere(-1) : () -> stepWhere(1));
        }
        float kk = at.height() / GameStyleGeometry.Panel.BUTTON_H * GameStyleData.PANEL_EM, room = at.width() - 2 * (inset + aw + 8 * u);
        Fit f = fit(buttonText, whereText, room, kk);
        buttonText.colour("LightButtonTextColor");
        buttonText.draw(c, f.text, at.centerX(), at.top + at.height() * GameStyleGeometry.Panel.BUTTON_BASE / GameStyleGeometry.Panel.BUTTON_H, f.k, Paint.Align.CENTER, 255);
    }
    private int whereIndex() { for (int i = 0; i < whereChoices.length; i++) if (whereChoices[i] == where) return i; return -1; }


    private void drawWhereList(Canvas c, float top, float bottom, float g) {
        int n = whereChoices.length;
        float pad = 10 * g, eh = whereBox.height(), want = Math.max(eh + 8 * g, style.minTap()), room = bottom - top - 2 * pad;

        int cols = n * want <= room ? 1 : 2, rows = (n + cols - 1) / cols;
        float pitch = Math.min(want, room / Math.max(1, rows)), colW = whereBox.width();
        eh = Math.min(eh, pitch - 4 * g);
        RectF back = new RectF(whereBox.right - cols * colW, top, whereBox.right, top + 2 * pad + rows * pitch);
        style.tooltip(c, back);
        art.nine(c, "build_inset", new RectF(back), 8, g, pic);
        for (int i = 0; i < n; i++) {
            float rt = top + pad + (i % rows) * pitch, cl = back.left + (i / rows) * colW;
            r.set(cl + pad, rt + (pitch - eh) / 2, cl + colW - pad, rt + (pitch - eh) / 2 + eh);
            boolean focused = padUsed && focusKind == HIT_AREA && focusWhich == whereChoices[i];
            style.panelButton(c, r, whereChoices[i] == where ? GameStyle.BUTTON_DOWN : focused ? GameStyle.BUTTON_HOVER : GameStyle.BUTTON_NORMAL, whereTexts[i]);
            final int to = whereChoices[i];
            r.set(cl, rt, cl + colW, rt + pitch);
            hit(r, HIT_AREA, to, true, () -> pickWhere(to));
        }
    }

    private void up(RectF at) { r.set(at.left, Math.max(tabBottom, Math.min(at.top, at.bottom - style.minTap())), at.right, at.bottom); }
    private void tall(RectF at) { float ext = Math.max(0, (style.minTap() - at.height()) / 2); r.set(at.left, at.top - ext, at.right, at.bottom + ext); }
    private void drawTile(Canvas c, BookModel.Tile t, int i, float left, float top, float right, float bottom, float g, float k) {
        BookLayout.tile(left, top, right, bottom, 10 * g, i, tileBox);
        r.set(tileBox[0], tileBox[1], tileBox[2], tileBox[3]);
        art.nine(c, "build_inset", new RectF(r), 8, g, pic);
        hit(r, HIT_TILE, i, true, () -> openKind(t, i));
        BookLayout.picture(tileBox, 8 * g, picBox);
        String name = i < tilePictures.size() ? tilePictures.get(i) : null;
        boolean dull = i < tileGrey.size() && tileGrey.get(i);
        if (name != null) drawPicture(c, name, picBox, HIT_TILE_PICTURE, i, dull);
        float bt = BookLayout.bandTop(tileBox);
        c.drawRect(tileBox[0] + 4 * g, bt, tileBox[2] - 4 * g, tileBox[3] - 4 * g, band);
        if (i < tileNames.size()) {
            body.colour(dull ? "FontColorLightGray" : "FontColorWhite");
            float kk = i < tileK.size() ? tileK.get(i) : k;
            body.draw(c, tileNames.get(i), (tileBox[0] + tileBox[2]) / 2, base(body, bt, tileBox[3] - 4 * g, kk), kk, Paint.Align.CENTER, 255);
        }
        if (t.badge != BookModel.OPEN) {
            float in = 3 * g;
            ribbon(c, tileRibbon, tileBox[0] + in, tileBox[1] + in, tileBox[2] - in, tileBox[3] - in, ribbonSide, false, t.badge, g);
            if (BuildConfig.DM_QA_BUILD) TestHooks.hit(r, HIT_BADGE, i);
        }
    }

    private void drawPicture(Canvas c, String name, float[] at, int kind, int index) { drawPicture(c, name, at, kind, index, false); }
    private void drawPicture(Canvas c, String name, float[] at, int kind, int index, boolean dull) {
        if (mon.failed(name, false)) return;
        monShown.add(MonLoader.key(name, false));
        Bitmap b = mon.get(name, false);
        if (b == null) return;
        cell.set(at[0], at[1], at[2], at[3]);
        c.drawBitmap(b, null, cell, dull ? grey : pic);
        if (BuildConfig.DM_QA_BUILD) { TestHooks.hit(cell, kind, index); monDrawn++; }
    }



    private final String[] tailOf = {"", ""}, tailIs = {"", ""}; private final float[] tailRoom = new float[2];
    private String tail(String text, float room, float k) {
        int i = text.endsWith("|") ? 1 : 0;
        if (text.equals(tailOf[i]) && room == tailRoom[i]) return tailIs[i];
        String t = text;
        while (t.length() > 1 && input.measure(t, k) > room) t = t.substring(1);
        tailOf[i] = text; tailRoom[i] = room; tailIs[i] = t;
        return t;
    }

    private void drawKind(Canvas c, GameStyleGeometry.Panel p, float x, float right, float y, float bottom, float bh, float lineH, float g, float k) {
        BookModel.Kind kd = kind();
        CombatModel.Enemy e = shownEncounter();
        float gap = 12 * g, bw = (right - x - 2 * gap) / 3;
        r.set(x, y, x + bw * 0.6f, y + bh); button(c, r, t("Back"), HIT_BACK, this::leaveKind);
        r.set(x + bw * 0.6f + gap, y, x + bw * 1.75f + gap, y + bh);
        button(c, r, CombatModel.difficultyLabel(diff, lang()), HIT_DIFF, () -> { if (diff < 0) return; diff = (diff + 1) % 3; change(); });
        r.set(x + bw * 1.75f + 2 * gap, y, x + bw * 2.75f + 2 * gap, y + bh);
        button(c, r, CombatModel.playersLabel(players, lang()), HIT_PLAYERS, () -> { if (players < 1) return; players = CombatModel.nextPlayers(players); change(); });
        y += bh + 18 * g;

        float titleH = gold.size(k) * 1.6f;
        gold.colour("FontColorGoldYellow");
        String rating = ratingText.isEmpty() ? "" : "  ·  " + ratingText;
        float ratingW = rating.isEmpty() ? 0 : fit(body, rating, right - x, k).width;
        float nameW = fitted(c, gold, kindTitle, x, y, y + titleH, right - x - ratingW, k * 1.25f, Paint.Align.LEFT);
        if (!rating.isEmpty() && e != null) { body.colour(CombatModel.colour(e.light)); fitted(c, body, rating, x + nameW, y, y + titleH, right - x - nameW, k, Paint.Align.LEFT); }
        y += titleH + 4 * g;
        if (!defeated.isEmpty()) { dim.colour("FontColorLightGray"); fitted(c, dim, defeated, x, y, y + lineH, right - x, k, Paint.Align.LEFT); y += lineH; }
        y += 8 * g;

        float side = Math.min(bottom - y, (right - x) * 0.36f);
        r.set(x, y, x + side, y + side);
        art.nine(c, "build_inset", new RectF(r), 8, g, pic);
        picBox[0] = x + 8 * g; picBox[1] = y + 8 * g; picBox[2] = x + side - 8 * g; picBox[3] = y + side - 8 * g;
        if (kindPicture != null) drawPicture(c, kindPicture, picBox, HIT_PICTURE, 0, unbeaten(kd));
        float sx = x + side + 28 * g;
        List<Integer> secs = sections();
        if (secs.isEmpty() || kd == null) return;
        float sw = p.subTabWidth(sx, right, Math.max(secs.size(), 3));
        for (int i = 0; i < secs.size(); i++) {
            int s = secs.get(i);
            p.subTab(sx, y, sw, i, box);
            style.subTab(c, p, box, sectionName(s), s == section);
            r.set(box[0], box[1], box[2], box[3]); hit(r, HIT_SECTION, s, true, () -> pickSection(s));
        }
        float ty = y + p.subTabH() + 16 * g;
        if (section == SECTION_STATS) {
            if (sectionLines.isEmpty()) {
                boolean none = detailFailed != null && detailFailed.equals(detailWanted());
                dim.colour("FontColorLightGray");
                fitted(c, dim, none ? t("NotAvailable") : t("Calculating"), sx, ty, ty + lineH, right - sx, k, Paint.Align.LEFT);
            }
            else drawLines(c, sectionLines, sectionColours, sx, right, ty, bottom, lineH, g, k);
            return;
        }
        if (section == SECTION_PLACES) { drawPlaces(c, sx, right, ty, bottom, lineH, g, k); return; }
        if (section == SECTION_FIGHT && e != null) {
            for (int i = 0; i < fightLines.size() && ty + lineH < bottom - bh - 12 * g; i++) {
                body.colour(fightColours.get(i));
                fitted(c, body, fightLines.get(i), sx, ty, ty + lineH, right - sx, k, Paint.Align.LEFT);
                ty += lineH + 4 * g;
            }
            r.set(sx, bottom - bh, sx + Math.min(right - sx, 520 * g), bottom);
            button(c, r, t("ViewFight"), HIT_FIGHT, () -> showFight(e));
        }
        if (section == SECTION_LOOT) drawLoot(c, p, sx, right, ty, bottom, bh, lineH, g, k);
    }



    private void drawLoot(Canvas c, GameStyleGeometry.Panel p, float x, float right, float y, float bottom, float bh, float lineH, float g, float k) {
        dim.colour("FontColorLightGray");
        fitted(c, dim, lootNote, x, y, y + lineH, right - x, k * 0.9f, Paint.Align.LEFT);
        y += lineH + 8 * g;
        if (lootRows == null) { fitted(c, dim, t("Calculating"), x, y, y + lineH, right - x, k, Paint.Align.LEFT); return; }
        if (lootLines.isEmpty()) { fitted(c, dim, t("NoItems"), x, y, y + lineH, right - x, k, Paint.Align.LEFT); return; }
        float rowH = Math.max(lineH * 1.25f, style.minTap() * 0.45f), pagerH = lineH + 8 * g;
        int per = Math.max(1, (int) ((bottom - y - pagerH - 8 * g) / rowH));
        if (lootLines.size() <= (int) ((bottom - y) / rowH)) per = Math.max(1, (int) ((bottom - y) / rowH));
        lootPer = per;
        lootPages = Math.max(1, (lootLines.size() + per - 1) / per);
        lootPage = Math.max(0, Math.min(lootPages - 1, lootPage));
        String open = arrows ? "▾ " : "− ", closed = arrows ? "▸ " : "+ ";
        float ratioW = fit(body, "1 : 1.234.567", right - x, k).width;
        for (int i = 0; i < per && lootPage * per + i < lootLines.size(); i++) {
            int at = lootPage * per + i;
            BookLoot.Line l = lootLines.get(at);
            float top = y + i * rowH, bot = top + rowH;
            if (l.row == null) {
                gold.colour("FontColorGoldYellow");
                fitted(c, gold, (lootOpen[l.group] ? open : closed) + lootTexts.get(at), x, top, bot, right - x, k, Paint.Align.LEFT);
                r.set(x - 6 * g, top, right, bot);
                int grp = l.group;
                hit(r, HIT_LOOT_GROUP, grp, true, () -> toggleGroup(grp));
                continue;
            }
            float side = rowH - 6 * g;
            String pn = lootPics.get(at);
            if (pn != null && !inv.failed(pn, false)) {
                invShown.add(pn);
                Bitmap b = inv.get(pn, false);
                if (b != null && b.getWidth() > 0 && b.getHeight() > 0) {
                    float scale = Math.min(side / b.getWidth(), side / b.getHeight()), w = b.getWidth() * scale, h = b.getHeight() * scale;
                    cell.set(x + (side - w) / 2, top + (rowH - h) / 2, x + (side + w) / 2, top + (rowH + h) / 2);
                    c.drawBitmap(b, null, cell, pic);
                }
            }
            GameText t = body;
            t.colour(l.group == BookLoot.FOUND ? "FontColorGoldYellow" : "FontColorWhite");
            float nx = x + side + 10 * g;
            fitted(c, t, lootTexts.get(at), nx, top, bot, right - nx - ratioW - 14 * g, k, Paint.Align.LEFT);
            dim.colour("FontColorLightGray");
            fitted(c, dim, lootRatios.get(at), right, top, bot, ratioW, k, Paint.Align.RIGHT);
            if (BuildConfig.DM_QA_BUILD) { r.set(x, top, right, bot); TestHooks.hit(r, HIT_LOOT_ROW, i); }
        }
        if (lootPages > 1) {
            String t = "‹   " + (lootPage + 1) + " / " + lootPages + "   ›";
            float top = bottom - lineH, w = fit(legend, t, right - x, k).width, cx = (x + right) / 2, side = Math.max(style.minTap(), lineH);
            fitted(c, legend, t, cx - w / 2, top, bottom, right - x, k, Paint.Align.LEFT);
            r.set(cx - w / 2 - side / 2, top - (side - lineH) / 2, cx - w / 2 + side / 2, bottom + (side - lineH) / 2);
            hit(r, HIT_LOOT_PREV, 0, false, () -> lootFlip(-1));
            r.set(cx + w / 2 - side / 2, r.top, cx + w / 2 + side / 2, r.bottom);
            hit(r, HIT_LOOT_NEXT, 0, false, () -> lootFlip(1));
        }
    }


    private void drawLines(Canvas c, List<String> lines, List<String> colours, float sx, float right, float ty, float bottom, float lineH, float g, float k) {
        float pitch = lineH + 4 * g;
        int per = Math.max(1, (int) ((bottom - ty) / pitch));
        if (lines.size() > per) per = Math.max(1, (int) ((bottom - ty - pitch) / pitch));
        sectionPages = Math.max(1, (lines.size() + per - 1) / per); sectionPage = BookLayout.clamp(sectionPage, sectionPages);
        for (int i = sectionPage * per; i < Math.min(lines.size(), (sectionPage + 1) * per); i++) {
            GameText t = colours.get(i).equals("FontColorGoldYellow") ? gold : body;
            t.colour(colours.get(i));
            fitted(c, t, lines.get(i), sx, ty, ty + lineH, right - sx, k, Paint.Align.LEFT);
            ty += pitch;
        }
        if (sectionPages > 1) sectionPager(c, sx, right, bottom - lineH, bottom, g, k);
    }
    private void sectionPager(Canvas c, float sx, float right, float top, float bottom, float g, float k) {
        String t = "‹   " + (sectionPage + 1) + " / " + sectionPages + "   ›";
        float w = fit(legend, t, right - sx, k).width, cx = (sx + right) / 2, side = Math.max(style.minTap(), bottom - top);
        fitted(c, legend, t, cx - w / 2, top, bottom, right - sx, k, Paint.Align.LEFT);
        r.set(cx - w / 2 - side / 2, top - (side - (bottom - top)) / 2, cx - w / 2 + side / 2, bottom + (side - (bottom - top)) / 2);
        hit(r, HIT_SECTION_PREV, 0, false, () -> { sectionPage = BookLayout.clamp(sectionPage - 1, sectionPages); invalidate(); });
        r.set(cx + w / 2 - side / 2, r.top, cx + w / 2 + side / 2, r.bottom);
        hit(r, HIT_SECTION_NEXT, 0, false, () -> { sectionPage = BookLayout.clamp(sectionPage + 1, sectionPages); invalidate(); });
    }


    private void drawPlaces(Canvas c, float sx, float right, float ty, float bottom, float lineH, float g, float k) {
        float rowH = Math.max(lineH * 1.5f, style.minTap() * 0.8f), pitch = rowH + 6 * g;
        int per = Math.max(1, (int) ((bottom - ty) / pitch));
        if (places.size() > per) per = Math.max(1, (int) ((bottom - ty - lineH - 6 * g) / pitch));
        sectionPages = Math.max(1, (places.size() + per - 1) / per); sectionPage = BookLayout.clamp(sectionPage, sectionPages);
        for (int i = sectionPage * per; i < Math.min(places.size(), (sectionPage + 1) * per); i++) {
            CombatModel.Enemy x = places.get(i);
            r.set(sx, ty, right, ty + rowH);
            if (x.area == kindArea) art.row(c, "build_row", new RectF(r), 18, pic); else art.nine(c, "build_inset", new RectF(r), 8, g, pic);
            final int area = x.area;
            hit(r, HIT_PLACE, area, true, () -> pickPlace(area));
            float rad = rowH * 0.16f, cx = sx + 18 * g + rad;
            int rating = BookModel.badge(x.light);
            if (rating != BookModel.OPEN) {
                ribbon(c, rowRibbon, sx + 3 * g, ty + 3 * g, right - 3 * g, ty + rowH - 3 * g, rowH * ROW_RIBBON, true, rating, g);
                if (BuildConfig.DM_QA_BUILD) TestHooks.hit(r, HIT_PLACE_RIBBON, area);
            }
            body.colour(x.estimated ? "FontColorLightGray" : "FontColorWhite");
            fitted(c, body, placeTexts.get(i), cx + rad + 14 * g, ty, ty + rowH, right - (cx + rad + 14 * g) - 12 * g, k, Paint.Align.LEFT);
            ty += pitch;
        }
        if (sectionPages > 1) sectionPager(c, sx, right, bottom - lineH, bottom, g, k);
    }




    private void ribbon(Canvas c, Ribbon s, float left, float top, float right, float bottom, float side, boolean mirror, int rating, float g) {
        s.make(side, g);
        c.save();
        c.clipRect(left, top, right, bottom);
        if (mirror) { c.translate(left, top); c.scale(-1, 1); } else c.translate(right, top);
        c.drawPath(s.shadow, ribbonShadow);
        ribbonCloth.setShader(s.shade[rating]);
        c.drawPath(s.cloth, ribbonCloth);
        badgeRim.setStrokeWidth(Math.max(1, 2 * g)); c.drawPath(s.edges, badgeRim);
        ribbonShine.setStrokeWidth(Math.max(1, 1.2f * g)); c.drawPath(s.shine, ribbonShine);
        c.restore();
        if (mirror) r.set(left - s.bounds.right, top + s.bounds.top, left - s.bounds.left, top + s.bounds.bottom);
        else r.set(right + s.bounds.left, top + s.bounds.top, right + s.bounds.right, top + s.bounds.bottom);
    }


    private final class Ribbon {
        final Path cloth = new Path(), shadow = new Path(), edges = new Path(), shine = new Path();
        final Shader[] shade = new Shader[3];
        final RectF bounds = new RectF();
        private float side = -1, scale = -1; private int generation = -1;
        void make(float side, float g) {
            if (side == this.side && g == scale && style.generation() == generation) return;
            this.side = side; scale = g; generation = style.generation();
            badgeRim.setColor(style.colour(BookRibbon.EDGE));
            ribbonShine.setColor(style.colour(BookRibbon.SHINE)); ribbonShine.setAlpha(110);
            cloth.reset(); edges.reset(); shine.reset();
            float[] a = new float[2]; BookRibbon.sash(side, a);
            cloth.moveTo(-a[1], 0); cloth.lineTo(-a[0], 0); cloth.lineTo(0, a[0]); cloth.lineTo(0, a[1]); cloth.close();
            edges.moveTo(-a[1], 0); edges.lineTo(0, a[1]); edges.moveTo(-a[0], 0); edges.lineTo(0, a[0]);
            float in = 2.6f * g;
            shine.moveTo(-a[0] - in, 0); shine.lineTo(0, a[0] + in);
            bounds.set(-a[1], 0, 0, a[1]);
            shadow.set(cloth); shadow.offset(0, 3 * g);
            for (int i = 0; i < shade.length; i++)
                shade[i] = new LinearGradient(-a[1] / 2, a[1] / 2, -a[0] / 2, a[0] / 2, BookRibbon.shades(style.colour(CombatModel.colour(i))), BookRibbon.AT, Shader.TileMode.CLAMP);
        }
    }

    private void drawFocus(Canvas c, float g) {
        if (!padUsed) return;
        int f = focusIndex();
        if (f < 0) return;
        RectF at = new RectF(boxes.get(f));
        at.inset(-6 * g, -6 * g);
        art.rim(c, "build_row", at, 8, g, pic);
    }
    private String legendLine() {
        return t(kindKey != null ? "LegendKindPages" : whereOpen ? "LegendWhere" : "LegendGrid");
    }
    private void flip(int d) { int to = BookLayout.clamp(page + d, pages); if (to != page) { page = to; focusKind = -1; prepare(); qa(); } invalidate(); }


    private int[] downId;
    private int hitAt(float x, float y) { for (int i = boxes.size() - 1; i >= 0; i--) if (boxes.get(i).contains(x, y)) return i; return -1; }
    private void act(int i) {
        int kind = ids.get(i)[0];
        if (whereOpen && kind != HIT_AREA && kind != HIT_ACT) { openWhere(false); return; }
        if (typing.editing() && kind != HIT_SEARCH && kind != HIT_CLEAR) typing.end();
        actions.get(i).run();
        invalidate();
    }
    @Override public boolean onTouchEvent(MotionEvent e) {
        int a = e.getActionMasked();
        float x = e.getX(), y = e.getY();

        if (a == MotionEvent.ACTION_DOWN) { downHit = hitAt(x, y); downId = downHit >= 0 ? ids.get(downHit).clone() : null; downOutside = downHit < 0 && !frame.contains(x, y); if (padUsed) { padUsed = false; invalidate(); } }
        else if (a == MotionEvent.ACTION_UP) {
            int at = hitAt(x, y);
            if (downHit >= 0 && at >= 0 && java.util.Arrays.equals(downId, ids.get(at))) act(at);
            else if (whereOpen && at < 0) openWhere(false);
            else if (downOutside && at < 0 && !frame.contains(x, y)) close();
            else if (downHit < 0) typing.end();
            downHit = -1; downOutside = false;
        } else if (a == MotionEvent.ACTION_CANCEL) { downHit = -1; downOutside = false; }
        return true;
    }


    @Override public WindowInsets onApplyWindowInsets(WindowInsets in) { typing.insets(in); return super.onApplyWindowInsets(in); }
    @Override public boolean onCheckIsTextEditor() { return typing.editing(); }
    @Override public InputConnection onCreateInputConnection(EditorInfo out) { return typing.connection(out); }
    @Override public boolean onKeyDown(int keyCode, KeyEvent e) { return typing.key(e) || super.onKeyDown(keyCode, e); }
    @Override public boolean onKeyUp(int keyCode, KeyEvent e) { return typing.key(e) || super.onKeyUp(keyCode, e); }
    @Override public boolean takesKey(KeyEvent e) { return typing.takesKey(e); }
    @Override public boolean dispatchKeyEventPreIme(KeyEvent e) { return typing.preIme(e) || super.dispatchKeyEventPreIme(e); }


    @Override public boolean padOpen() { return isOpen(); }

    @Override public void back() {
        if (whereOpen) openWhere(false);
        else if (typing.editing()) typing.end();
        else if (kindKey != null) leaveKind();
        else if (!filter.query.isEmpty()) setQuery("");
        else close();
    }
    private int focusIndex() {
        for (int i = 0; i < boxes.size(); i++) if (focusable.get(i) && ids.get(i)[0] == focusKind && ids.get(i)[1] == focusWhich) return i;
        return -1;
    }
    private void move(int dx, int dy) {
        List<Integer> map = new ArrayList<>();
        for (int i = 0; i < boxes.size(); i++) if (focusable.get(i) && (!whereOpen || ids.get(i)[0] == HIT_AREA)) map.add(i);
        if (map.isEmpty()) return;
        int f = focusIndex(), from = map.indexOf(f);
        int to;
        if (from < 0) {
            to = 0;
            for (int i = 0; i < map.size(); i++) if (ids.get(map.get(i))[0] == HIT_TILE) { to = i; break; }
        } else {
            float[][] rects = new float[map.size()][];
            for (int i = 0; i < map.size(); i++) { RectF b = boxes.get(map.get(i)); rects[i] = new float[]{b.left, b.top, b.right, b.bottom}; }
            to = BookLayout.near(rects, from, dx, dy);
        }
        int[] id = ids.get(map.get(to));
        focusKind = id[0]; focusWhich = id[1];
        qa(); invalidate();
    }
    @Override public void onPad(int cmd) {
        padUsed = true;
        switch (cmd) {
            case PadNav.B: back(); break;
            case PadNav.MENU: close(); return;
            case PadNav.LEFT: move(-1, 0); break;
            case PadNav.RIGHT: move(1, 0); break;
            case PadNav.UP: move(0, -1); break;
            case PadNav.DOWN: move(0, 1); break;
            case PadNav.A: { int f = focusIndex(); if (f >= 0) act(f); else move(0, 1); break; }
            case PadNav.L2: if (whereOpen) break; if (kindKey != null && section == SECTION_LOOT) lootFlip(-1); else if (kindKey != null) sectionPage = BookLayout.clamp(sectionPage - 1, sectionPages); else flip(-1); break;
            case PadNav.R2: if (whereOpen) break; if (kindKey != null && section == SECTION_LOOT) lootFlip(1); else if (kindKey != null) sectionPage = BookLayout.clamp(sectionPage + 1, sectionPages); else flip(1); break;
            case PadNav.L1: case PadNav.R1: {
                List<Integer> secs = sections();
                if (kindKey != null && secs.size() > 1) { int at = secs.indexOf(section); pickSection(secs.get(Math.floorMod(at + (cmd == PadNav.L1 ? -1 : 1), secs.size()))); }
                break;
            }
            case PadNav.Y: if (typing.editing()) typing.end(); else beginEditing(); break;
            default: break;
        }
        invalidate();
    }

    @Override protected void onSizeChanged(int w, int h, int ow, int oh) { super.onSizeChanged(w, h, ow, oh); prepare(); }
    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) { qa(); DmActivity.padPagesChanged(); }
    }
    private void qaSearch() { TestHooks.overlay(this, "monster_book_search", typing.editing() ? "editing" : "idle"); invalidate(); }



    private void qa() {
        if (!BuildConfig.DM_QA_BUILD) return;
        String state = !isOpen() ? "closed" : report == null ? (failed ? "failed" : "loading") : stale || busy ? "loading" : "open";
        TestHooks.overlay("monster_book_focus", focusKind < 0 ? "none" : focusKind + "_" + focusWhich);
        TestHooks.overlay("monster_book_view", !isOpen() || report == null ? "none" : kindKey != null ? "kind_" + kindKey.toLowerCase(java.util.Locale.ROOT) + "_" + kindArea : "grid_" + (page + 1) + "_" + pages);
        StringBuilder t = new StringBuilder();
        for (int i = 0; isOpen() && kindKey == null && i < BookLayout.onPage(tiles.size(), page); i++) {
            BookModel.Tile x = tiles.get(page * BookLayout.PER_PAGE + i);
            t.append(t.length() == 0 ? "" : "_").append(x.kind.key.toLowerCase(java.util.Locale.ROOT)).append('b').append(x.badge < 0 ? "o" : Integer.toString(x.badge));
        }
        TestHooks.overlay("monster_book_tiles", t.length() == 0 ? "none" : t.toString());

        String loot = !isOpen() || kindKey == null || lootClass() < 0 ? "none" : lootRows == null ? "loading" : lootLines.size() + "_" + (lootPage + 1) + "_" + lootPages;
        TestHooks.overlay("monster_book_loot", loot);
        StringBuilder lr = new StringBuilder();
        for (int i = 0; lootRows != null && kindKey != null && i < lootPer && lootPage * lootPer + i < lootLines.size(); i++) {
            BookLoot.Line l = lootLines.get(lootPage * lootPer + i);
            lr.append(lr.length() == 0 ? "" : "_").append(l.row == null ? "g" + l.group : l.row.kind + "x" + l.row.index);
        }
        TestHooks.overlay("monster_book_loot_rows", lr.length() == 0 ? "none" : lr.toString());
        TestHooks.overlay("monster_book_found", foundTooMany ? "many" : foundChance == null ? "none" : Integer.toString(foundItems.size()));
        TestHooks.overlay("monster_book_filter", where == BookModel.Where.AREA ? "area" + filter.area : "act" + filter.act);
        TestHooks.overlay("monster_book_where", whereOpen ? "open" : "closed");
        StringBuilder dull = new StringBuilder();
        for (int i = 0; isOpen() && kindKey == null && i < tileGrey.size(); i++) if (tileGrey.get(i)) dull.append(dull.length() == 0 ? "" : "_").append(i);
        TestHooks.overlay("monster_book_grey", kills == null ? "unknown" : dull.length() == 0 ? "none" : dull.toString());
        TestHooks.overlay(this, "monster_book", state);
    }
}
