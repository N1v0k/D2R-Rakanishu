






































package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.net.Uri;
import android.text.TextPaint;
import android.text.TextUtils;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class BuildBook extends View implements PadPage, QuickPage {
    static final int TAB_OVERVIEW = 0, TAB_SKILLS = 1, TAB_STATS = 2, TAB_GEAR = 3, TAB_MERC = 4, TAB_SOURCES = 5;
    private static final long POLL_MS = 400;
    private static final int HIT_CLOSE = 0, HIT_CLASS = 1, HIT_CARD = 2, HIT_FOLLOW = 3, HIT_BACK = 4, HIT_PHASE = 5, HIT_TAB = 6, HIT_PREV = 7, HIT_NEXT = 8,
        HIT_URL = 9, HIT_TREE = 10, HIT_RESPEC = 11, HIT_RECIPE = 12, HIT_GEAR = 13;
    private static final String[] STAT_KEYS = {"str", "dex", "vit", "ene"};
    private static final String[] TAGS = {"leveling", "endgame", "budget", "bosses", "ssf", "magicfind", "fire", "cold", "lightning", "poison", "physical", "magic", "summon", "melee", "caster"};
    private static final int[] TAG_IDS = {R.string.dmAndroidBuildTagLeveling, R.string.dmAndroidBuildTagEndgame, R.string.dmAndroidBuildTagBudget, R.string.dmAndroidBuildTagBosses,
        R.string.dmAndroidBuildTagSsf, R.string.dmAndroidBuildTagMagicfind, R.string.dmAndroidBuildTagFire, R.string.dmAndroidBuildTagCold, R.string.dmAndroidBuildTagLightning,
        R.string.dmAndroidBuildTagPoison, R.string.dmAndroidBuildTagPhysical, R.string.dmAndroidBuildTagMagic, R.string.dmAndroidBuildTagSummon, R.string.dmAndroidBuildTagMelee,
        R.string.dmAndroidBuildTagCaster};
    private static final String[] SLOTS = {"weapon", "offhand", "helm", "armor", "gloves", "belt", "boots", "amulet", "ring", "charm", "switch"};
    private static final int[] SLOT_IDS = {R.string.dmAndroidBuildSlotWeapon, R.string.dmAndroidBuildSlotOffhand, R.string.dmAndroidBuildSlotHelm, R.string.dmAndroidBuildSlotArmor,
        R.string.dmAndroidBuildSlotGloves, R.string.dmAndroidBuildSlotBelt, R.string.dmAndroidBuildSlotBoots, R.string.dmAndroidBuildSlotAmulet, R.string.dmAndroidBuildSlotRing,
        R.string.dmAndroidBuildSlotCharm, R.string.dmAndroidBuildSlotSwitch};
    private static final int[] TAB_IDS = {R.string.dmAndroidBuildTabOverview, R.string.dmAndroidBuildTabSkills, R.string.dmAndroidBuildTabStats, R.string.dmAndroidBuildTabGear,
        R.string.dmAndroidBuildTabMerc, R.string.dmAndroidBuildTabSources};


    private final int inkWhite, inkGold, inkLightGold, inkBeige, inkNote, inkDim, inkRed, inkDarkGold, inkTabOn, inkTabOff, inkDescription;

    private final GameStyleData.Shadow headShadow;
    private boolean shaded;
    private float shadeDx, shadeDy;

    private static final float ROW_UNITS = 183.6f, MARGIN_UNITS = 43.2f;
    private final AppArt art;
    private final Runnable freeInput;
    private final RecipeBook recipes;
    private final List<String> recipeTargets = new ArrayList<>();










    private final List<BuildModel.Gear> gearHits = new ArrayList<>();
    private final ItemTooltip tooltip;
    private final RecipePreview previewText = new RecipePreview();
    private BuildModel.Gear preview;
    private GearPreview.Card previewCard;
    private List<GearPreview.Line> previewLines;
    private String previewRaw;
    private int previewDrawn = -1;
    private boolean previewDown;
    private float previewX, previewY;
    private final Runnable previewPoll = this::pollPreview;


    private boolean de;
    private final GameLang.Watch lang = new GameLang.Watch();
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), box = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final TextPaint head = new TextPaint(Paint.ANTI_ALIAS_FLAG), body = new TextPaint(Paint.ANTI_ALIAS_FLAG);
    private final RectF panel = new RectF(), r = new RectF();


    private final GameStyle style;
    private GameStyleGeometry.Panel win;
    private final float[] box4 = new float[4], statusW = new float[3], statusMax = new float[3];
    private final GameStyleGeometry.ItemGrid items = new GameStyleGeometry.ItemGrid();
    private final GameStyleGeometry.GearRow gearRow = new GameStyleGeometry.GearRow();
    private final List<RectF> hitRects = new ArrayList<>(), focusRects = new ArrayList<>();
    private final List<int[]> hits = new ArrayList<>();
    private final Map<String, Bitmap> pictures = new HashMap<>();
    private final File pictureRoot;
    private BuildData data;
    private boolean loading, paused, running;
    private BuildModel.Snap snap = new BuildModel.Snap();
    private int askedGen = -1, askedFor = -1;
    private final int[] snapInts = new int[BuildModel.SNAP_INTS];
    private final byte[] snapName = new byte[16];
    private int level, cls = 7, listPage, listPages = 1, tab, phase, page, pages = 1, treePage = 3;
    private BuildModel.Guide guide;
    private String note = "";
    private int focus = -1, downHit = -1, focusKind = -1, focusWhich;
    private float downX, downY;
    private int lastTarget = Integer.MIN_VALUE, lastStat = Integer.MIN_VALUE;
    private String lastCard, lastKeys;
    private boolean hints = true;
    private final Runnable poll = this::pollTick;

    BuildBook(Context context, AppArt art, GameStyle style, Runnable freeInput, RecipeBook recipes) {
        super(context);
        this.art = art;
        this.style = style;
        inkWhite = style.colour("FontColorWhite"); inkGold = style.colour("FontColorGoldYellow"); inkLightGold = style.colour("FontColorLightGold");
        inkBeige = style.colour("FontColorBeige"); inkNote = style.colour("PanelNoteTextColor"); inkDim = style.colour("FontColorLightGray");
        inkRed = style.colour("FontColorRed"); inkDarkGold = style.colour("FontColorDarkGold"); inkTabOn = style.colour("TabsActiveTextColor");
        inkTabOff = style.colour("TabsInactiveTextColor"); inkDescription = style.colour("DescriptionTextColor");
        headShadow = style.data().shadow("default");
        tooltip = new ItemTooltip(style);
        this.freeInput = freeInput;
        this.recipes = recipes;
        pictureRoot = new File(context.getFilesDir(), "dm/appart/builds");
        head.setTypeface(art.role(AppArt.EXOCET));
        body.setTypeface(art.role(AppArt.FORMAL));
        body.setLetterSpacing(-0.02f);
        setVisibility(GONE);
    }

    private String s(int id) { return AppText.get(getContext(), id); }
    private String s(int id, Object... args) { return AppText.get(getContext(), id, args); }
    private void refreshLang() {
        if (!lang.changed()) return;
        de = GameLang.german();
        head.setTypeface(art.role(AppArt.EXOCET));
        body.setTypeface(art.role(AppArt.FORMAL));
        style.refresh();
        removeCallbacks(prepareTabInk); post(prepareTabInk);
        if (preview != null) showPreview(preview);
    }

    @Override protected void onSizeChanged(int width, int height, int oldWidth, int oldHeight) {
        super.onSizeChanged(width, height, oldWidth, oldHeight);
        removeCallbacks(prepareTabInk);
        post(prepareTabInk);
    }

    private final Runnable prepareTabInk = () -> { prepareTabs(); invalidate(); };

    private void prepareTabs() {
        float w = getWidth(), h = getHeight();
        for (int id : TAB_IDS) style.prepareTabLabel(s(id), w, h);
        style.prepareTabLabel("‹ " + s(R.string.dmAndroidBack), w, h);
        style.prepareTabLabel("< " + s(R.string.dmAndroidBack), w, h);
        style.prepareTabLabel(s(R.string.dmAndroidBuildFollowing), w, h);
        style.prepareTabLabel(s(R.string.dmAndroidBuildFollow), w, h);
        for (int i = 0; i < 8; i++) {
            String name = className(i);
            style.prepareTabLabel(name, w, h);
            style.prepareTabLabel(name + " •", w, h);
            style.prepareTabLabel(name + " *", w, h);
        }
        if (guide != null) for (BuildModel.Phase phase : guide.phases) {
            style.prepareTabLabel(phaseLabel(phase), w, h);
            style.prepareTabLabel(phase.name.get(de), w, h);
        }
    }

    boolean isOpen() { return getVisibility() == VISIBLE; }

    @Override public void takePause() { paused = true; DmActivity.nativePause(DmActivity.PAUSE_BUILDS, 1); }

    @Override public boolean available() { return true; }


    void start() { if (!running) { running = true; ensureData(); removeCallbacks(poll); post(poll); } }
    void stop() {
        running = false; removeCallbacks(poll);
        DmActivity.nativeBuildTarget(0, 0, 0, 0, 0, 0); DmActivity.nativeBuildCard(0, null, 0, 0, -1); DmActivity.nativeBuildStat(0, 0, 0);
        DmActivity.nativeBuildKeys(0, new int[0]);
        lastTarget = Integer.MIN_VALUE; lastCard = null; lastStat = Integer.MIN_VALUE; lastKeys = null; lastGoal = null;
    }

    private void ensureData() {
        if (data != null || loading) return;
        loading = true;
        final Context context = getContext();
        new Thread(() -> { BuildData d = BuildData.load(context); post(() -> { data = d; loading = false; prepareTabs(); if (d.names == null) android.util.Log.i("dm", "Build-Guides: keine Namen (" + d.problem + ")"); invalidate(); }); }, "builds").start();
    }

    private void readSnap() {
        int got = DmActivity.nativeBuildState(snapInts, snapName);
        snap = BuildModel.Snap.parse(snapInts, got >> 8, snapName, got & 0xff);
        if (!snap.valid || data == null) return;
        if (askedFor != snap.cls) {
            List<Integer> ids = new ArrayList<>();
            if (data.names != null) { for (BuildModel.SkillInfo si : data.names.skills.values()) if (si.cls.equals(BuildData.CLASS_CODES[snap.cls])) ids.add(si.id); }
            else for (BuildModel.Guide g : data.ofClass(snap.cls)) for (BuildModel.Phase p : g.phases) for (BuildModel.Step st : p.skills) if (!ids.contains(st.skill)) ids.add(st.skill);
            int[] a = new int[Math.min(ids.size(), BuildModel.SNAP_MAX_SKILLS)];
            java.util.Collections.sort(ids);
            for (int i = 0; i < a.length; i++) a[i] = ids.get(i);
            askedGen = DmActivity.nativeBuildSkills(a);
            askedFor = snap.cls;
        }
        if (snap.request != askedGen) { snap.base.clear(); snap.reason.clear(); }
    }


    private BuildModel.Follow follow() { return snap.valid ? BuildStore.get(getContext(), snap.name, snap.cls) : null; }
    private BuildModel.Guide followedGuide() {
        BuildModel.Follow f = follow();
        BuildModel.Guide g = f == null || data == null ? null : data.guide(f.guide);
        return g != null && g.classIndex == snap.cls ? g : null;
    }

    private boolean isFollowed(BuildModel.Guide g) { BuildModel.Guide f = followedGuide(); return g != null && f != null && f.id.equals(g.id); }


    private BuildModel.Snap planSnap;
    private BuildModel.Guide planGuide;
    private int planPhase;
    private BuildPlan.Result planResult;

    private BuildPlan.Result planNow(BuildModel.Guide g, int ph) {
        if (!snap.valid || g == null || snap.base.isEmpty()) return null;
        if (planSnap == snap && planGuide == g && planPhase == ph) return planResult;
        Map<Integer, BuildModel.SkillInfo> sk = data.names == null ? new HashMap<>() : data.names.skills;
        planResult = BuildPlan.next(BuildPlan.chain(g, ph), snap.level, snap.base, BuildPlan.snapReason(snap, sk), snap.skillPoints, data.names == null ? null : BuildPlan.tableFuture(sk));
        planSnap = snap; planGuide = g; planPhase = ph;
        return planResult;
    }

    private void pollTick() {
        if (!running) return;
        readSnap();
        hints = DmActivity.nativeBuildHints() != 0;
        if (!isOpen()) {
            int open = DmActivity.nativeBuildOpenRequest();
            if (open != 0 && snap.valid && (open >> 8) == snap.session && followedGuide() != null) { openAt(followedGuide(), TAB_SKILLS); }
        }
        sendTarget();
        if (isOpen()) invalidate();
        postDelayed(poll, POLL_MS);
    }

    private void sendTarget() {
        BuildModel.Guide g = followedGuide();
        int sessionNow = snap.valid ? snap.session : 0, pg = 0, row = 0, col = 0, show = 0;
        if (g != null && data != null && data.names != null) {
            BuildModel.Follow f = follow();
            int ph = BuildPlan.phase(g, snap.level, f.phase);
            BuildPlan.Result res = planNow(g, ph);
            if (res != null && res.state == BuildPlan.NEXT) {
                BuildModel.SkillInfo si = data.names.skills.get(BuildPlan.chain(g, ph).get(res.index).skill);
                if (si != null) { pg = si.page; row = si.row; col = si.col; show = 1; }
            }
        }
        int key = ((((sessionNow * 4 + pg) * 8 + row) * 4 + col) * 2 + show) * 2 + (g != null ? 1 : 0);
        if (key != lastTarget) { lastTarget = key; DmActivity.nativeBuildTarget(sessionNow, pg, row, col, show, g != null ? 1 : 0); }
        sendCardAndStat(g, sessionNow);
        sendKeys(sessionNow);
        sendGoal(sessionNow);
    }



    private String lastGoal;
    private void sendGoal(int sessionNow) {
        if (sessionNow == 0 || !snap.valid) return;
        BuildModel.Follow f = data != null ? follow() : null;
        String k = sessionNow + "|" + snap.name + "#" + snap.cls + "|" + (f == null ? "-" : BuildModel.Follow.encode(f) + "|" + snap.level);
        if (k.equals(lastGoal)) return;
        BuildStore.ActiveBuild a = f == null ? null : BuildStore.active(getContext(), data, snap.name, snap.cls, snap.level);
        int[] goal = BestGearStore.goal(getContext(), snap.name, snap.cls, a != null && a.compat != BuildStore.INCOMPATIBLE ? a.profile : null);
        lastGoal = k; DmActivity.nativeGearGoal(sessionNow, goal[0], goal[1]);
    }



    private void sendKeys(int sessionNow) {
        BuildModel.Follow f = sessionNow != 0 && data != null ? follow() : null;
        String k = sessionNow + "|" + (f == null ? "-" : BuildModel.Follow.encode(f) + "|" + snap.level);
        if (k.equals(lastKeys)) return;
        int[] keys = new int[0];
        if (f != null) {
            BuildStore.ActiveBuild a = BuildStore.active(getContext(), data, snap.name, snap.cls, snap.level);
            if (a != null && a.compat != BuildStore.INCOMPATIBLE) keys = BuildStore.keySkills(a.profile, 8);
        }
        lastKeys = k; DmActivity.nativeBuildKeys(sessionNow, keys);
    }



    private void sendCardAndStat(BuildModel.Guide g, int sessionNow) {
        String card = ""; int to = 0, at = 0, state = -1, stat = 0, show = 0;
        if (g != null && data != null && sessionNow != 0) {
            BuildModel.Follow f = follow();
            int ph = BuildPlan.phase(g, snap.level, f.phase);
            BuildModel.Phase p = g.phases.get(ph);
            BuildPlan.Result res = planNow(g, ph);
            String nk = "", noteDe = "", noteEn = "";
            if (res != null) {
                state = res.state; at = res.at;
                if (res.index >= 0 && res.index < BuildPlan.chain(g, ph).size()) {
                    BuildModel.Step st = BuildPlan.chain(g, ph).get(res.index);
                    BuildModel.SkillInfo si = data.names == null ? null : data.names.skills.get(st.skill);
                    nk = si != null ? si.nameKey : ""; to = state == BuildPlan.SAVE ? snap.skillPoints : st.to;
                    BuildModel.Text why = state == BuildPlan.SAVE ? BuildPlan.saveNote(st) : null;
                    if (why != null) { noteDe = why.de; noteEn = why.en; }
                }
            }
            if (state >= 0) card = cut(cardTitle(g.title.de)) + "\0" + cut(cardTitle(g.title.en)) + "\0" + cut(phaseText(p, true)) + "\0" + cut(phaseText(p, false)) + "\0"
                + cut(nk) + "\0" + cut(noteDe) + "\0" + cut(noteEn) + "\0";
            int i = BuildPlan.statNext(p.stats, snap.baseMap());
            if (i >= 0) { String k = p.stats.get(i).stat; for (int j = 0; j < 4; j++) if (STAT_KEYS[j].equals(k)) stat = j + 1; show = stat > 0 && snap.statPoints > 0 ? 1 : 0; }
        }
        String ck = sessionNow + "|" + to + "|" + at + "|" + state + "|" + card;
        if (!ck.equals(lastCard)) { lastCard = ck; DmActivity.nativeBuildCard(sessionNow, state < 0 ? null : card.getBytes(java.nio.charset.StandardCharsets.UTF_8), to, at, state); }
        int sk = (sessionNow * 8 + stat) * 2 + show;
        if (sk != lastStat) { lastStat = sk; DmActivity.nativeBuildStat(sessionNow, stat, show); }
    }




    private static final float TITLE_PT = 26f, TITLE_UNITS_PER_PX = 1.281f, TITLE_LINE = 362f;
    private Paint titlePaint;
    private final Map<String, String> titles = new HashMap<>();

    private String cardTitle(String title) {
        android.graphics.Typeface face = art.role(AppArt.EXOCET);
        if (titlePaint == null || titlePaint.getTypeface() != face) { titlePaint = new Paint(); titlePaint.setTypeface(face); titlePaint.setTextSize(TITLE_PT); titles.clear(); }
        String t = titles.get(title);
        if (t != null) return t;
        t = BuildPlan.breakAtHyphens(title, x -> titlePaint.measureText(x) * TITLE_UNITS_PER_PX, TITLE_LINE);
        titles.put(title, t);
        return t;
    }

    private static String phaseText(BuildModel.Phase p, boolean german) { return p.name.get(german) + " · " + p.lo + "–" + p.hi; }


    private static String cut(String t) {
        if (t == null) return "";
        while (t.getBytes(java.nio.charset.StandardCharsets.UTF_8).length > 250) t = t.substring(0, t.offsetByCodePoints(0, t.codePointCount(0, t.length()) - 1));
        return t;
    }


    @Override public void open() { openAt(null, TAB_OVERVIEW); }

    private void openAt(BuildModel.Guide g, int atTab) {
        DmActivity.syncLang();
        refreshLang();
        ensureData();
        RecipeBook.warmText(getContext());
        readSnap();
        if (!paused) { paused = true; DmActivity.nativePause(DmActivity.PAUSE_BUILDS, 1); }
        level = g == null ? 0 : 1; guide = g; tab = atTab; page = 0; listPage = 0; focus = focusKind = -1; note = ""; treePage = mainTree(g);
        cls = snap.valid ? snap.cls : cls;
        if (g != null) phase = BuildPlan.phase(g, snap.level, follow() == null ? "auto" : follow().phase);
        prepareTabs();
        setVisibility(VISIBLE);
        post(freeInput);
        invalidate();
    }

    void close() {
        dropPreview();
        if (paused) { paused = false; DmActivity.nativePause(DmActivity.PAUSE_BUILDS, 0); }
        setVisibility(GONE);
    }

    void pause() { close(); }





    private void toRecipe(String target) {
        if (recipes == null || !recipes.has(target)) return;
        recipes.takePause();
        close();
        recipes.openAt(target, this::resumeFromBook);
    }

    private void resumeFromBook() {
        if (!paused) { paused = true; DmActivity.nativePause(DmActivity.PAUSE_BUILDS, 1); }
        setVisibility(VISIBLE);
        invalidate();
    }

    @Override public void back() {
        if (preview != null) { closePreview(); return; }
        if (level == 1) { level = 0; guide = null; focus = focusKind = -1; note = ""; invalidate(); }
        else close();
    }



    private static int mainTree(BuildModel.Guide g) { return g != null && g.trees.length > 0 && g.trees[0] >= 1 && g.trees[0] <= 3 ? g.trees[0] : 3; }

    private void hit(RectF rect, int kind, int which) { hitRects.add(new RectF(rect)); focusRects.add(new RectF(rect)); hits.add(new int[]{kind, which}); if (BuildConfig.DM_QA_BUILD) TestHooks.hit(rect, kind, which); }


    private void hit(RectF rect, int kind, int which, float l, float t, float r, float b) { hitRects.add(new RectF(rect)); focusRects.add(new RectF(l, t, r, b)); hits.add(new int[]{kind, which}); if (BuildConfig.DM_QA_BUILD) TestHooks.hit(rect, kind, which); }

    private void image(Canvas c, String name, RectF into) { Bitmap b = art.image(name); if (b != null) c.drawBitmap(b, null, into, paint); }

    private Bitmap picture(String path) {
        if (!pictures.containsKey(path)) {
            File f = new File(pictureRoot, path + ".png");
            pictures.put(path, f.isFile() ? BitmapFactory.decodeFile(f.getAbsolutePath()) : null);
        }
        return pictures.get(path);
    }

    private void fit(Canvas c, Bitmap b, float cx, float cy, float size, int alpha) {
        if (b == null) return;
        float scale = size / Math.max(b.getWidth(), b.getHeight()), w = b.getWidth() * scale, h = b.getHeight() * scale;
        r.set(cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2);
        paint.setAlpha(alpha);
        c.drawBitmap(b, null, r, paint);
        paint.setAlpha(255);
    }






    private void button(Canvas c, float x0, float y0, float x1, float y1, String text, boolean on, TextPaint p, int kind, int which) {
        r.set(x0, y0, x1, y1);
        float k = gamePx();
        boolean game = art.tab(c, on ? "build_tab_on" : "build_tab_off", new RectF(r), 6, 12, 16, k, paint);
        if (!game) image(c, on ? "book_tab_on" : "book_tab_off", r);
        hit(r, kind, which);
        p.setColor(on ? inkTabOn : inkTabOff);
        p.setTextAlign(Paint.Align.CENTER);
        float room = x1 - x0 - (y1 - y0) * 0.3f, size = p.getTextSize(), tw = p.measureText(text);
        if (tw > room) p.setTextSize(size * Math.max(0.75f, room / tw));
        textObserved(c, TextUtils.ellipsize(text, p, room, TextUtils.TruncateAt.END).toString(), (x0 + x1) / 2, (y0 + y1) / 2 + p.getTextSize() * 0.36f - (game ? 3 * k : 0), p);
        p.setTextSize(size);
    }

    private void bigButton(Canvas c, RectF at, String text, int kind, int which) {


        float bw = win.modalW(), bh = win.modalH(), f = Math.min(1, Math.min(at.width() / bw, at.height() / bh));
        bw *= f; bh *= f;
        at.set(at.centerX() - bw / 2, at.centerY() - bh / 2, at.centerX() + bw / 2, at.centerY() + bh / 2);
        head.setTextSize(Math.min(head.getTextSize(), bh * 0.36f));
        image(c, "button", at);
        hit(at, kind, which);
        head.setColor(inkLightGold);
        head.setTextAlign(Paint.Align.CENTER);
        boolean checked = text.endsWith(" ✓");
        String t = TextUtils.ellipsize(checked ? text.substring(0, text.length() - 2) : text, head, at.width() * (checked ? 0.62f : 0.8f), TextUtils.TruncateAt.END).toString();
        float tw = head.measureText(t), box = at.height() * 0.42f, x = at.centerX() - (checked ? box * 0.6f : 0);
        textObserved(c, t, x, at.centerY() + head.getTextSize() * 0.36f, head);
        if (checked) check(c, x + tw / 2 + box * 0.25f, at.centerY() - box / 2, box, true);
    }


    private void check(Canvas c, float x, float y, float size, boolean on) {
        r.set(x, y, x + size * 35 / 33f, y + size);
        image(c, on ? "build_check1" : "build_check0", r);
    }


    private static String glyph(Paint p, String want, String instead) { return p.hasGlyph(want) ? want : instead; }



    private float gamePx() { return 2 * win.k; }




    private RectF column(Canvas c, RectF at) {
        float k = gamePx();

        box.setStyle(Paint.Style.FILL); box.setColor(0xA0000000); c.drawRect(at, box);
        art.rim(c, "build_inset", at, 8, k, paint);
        return new RectF(at.left + 12 * k, at.top + 46 * k, at.right - 12 * k, at.bottom - 10 * k);
    }


    private void columnTitle(Canvas c, RectF at, String text) {
        float k = gamePx();
        head.setTextSize(24 * k); head.setColor(inkGold); shaded = true;
        left(c, head, text, at.left + 16 * k, at.top + 32 * k, at.width() - 32 * k);
        shaded = false;
        r.set(at.left + 12 * k, at.top + 40 * k, at.right - 12 * k, at.top + 43 * k); image(c, "build_divider", r);
    }


    private void field(Canvas c, RectF at, boolean lit) {
        if (lit) rowBg(c, at, "build_row");
        else art.nine(c, "build_inset", new RectF(at), 8, gamePx(), paint);
    }



    private String arrow() { String l = art.lang(); return l.equals("ko") || l.equals("zh") ? "→" : "›"; }


    private void rowBg(Canvas c, RectF at, String name) { art.row(c, name, at, 18, paint); }




    static final int LIT_ALPHA = 70;
    private void litRow(Canvas c, RectF at) {
        art.row(c, "build_long0", at, 18, paint);
        paint.setAlpha(LIT_ALPHA); art.row(c, "build_long1", at, 18, paint); paint.setAlpha(255);
    }

    private float left(Canvas c, TextPaint p, String text, float x, float y, float width) {
        p.setTextAlign(Paint.Align.LEFT);
        String t = TextUtils.ellipsize(text, p, width, TextUtils.TruncateAt.END).toString();
        textObserved(c, t, x, y, p);
        return p.measureText(t);
    }

    private void centre(Canvas c, TextPaint p, String text, float x, float y, float width) {
        p.setTextAlign(Paint.Align.CENTER);
        textObserved(c, TextUtils.ellipsize(text, p, width, TextUtils.TruncateAt.END).toString(), x, y, p);
    }

    private List<String> wrap(TextPaint p, String s, float width) {
        List<String> out = new ArrayList<>();
        while (!s.isEmpty()) {
            int n = p.breakText(s, true, width, null);
            if (n >= s.length()) { out.add(s); break; }
            int cut = s.lastIndexOf(' ', n);
            if (cut <= 0) cut = n;
            out.add(s.substring(0, cut));
            s = s.substring(cut).trim();
        }
        return out;
    }


    private static final float CHIP_TEXT = 0.5f;

    private static final float CARD_TITLE_MIN = 0.8f, DETAIL_TITLE_MIN = 0.72f;
    private void chip(Canvas c, String text, float x, float y, float h) {
        float w = chipWidth(text, h);
        r.set(x, y, x + w, y + h);
        RectF chipAt = new RectF(r);
        art.nine(c, "build_chip", chipAt, 6, h / 43f, paint);
        r.set(chipAt);
        head.setColor(inkNote);
        centre(c, head, text, r.centerX(), r.centerY() + head.getTextSize() * 0.36f, w);
    }

    private float chipWidth(String text, float h) { head.setTextSize(h * CHIP_TEXT); return head.measureText(text) + h * 0.8f; }


    private static void shrinkToFit(TextPaint p, String text, float width, float minScale) {
        float tw = p.measureText(text);
        if (tw > width) p.setTextSize(p.getTextSize() * Math.max(minScale, width / tw));
    }

    private void message(Canvas c, String text, float y) {
        body.setTextSize(panel.height() * 0.04f);
        body.setColor(inkWhite);
        centre(c, body, text, panel.centerX(), y, panel.width() * 0.85f);
    }


    private BuildModel.Names names() { return data == null ? null : data.names; }

    private String className(int index) {
        BuildModel.ClassInfo ci = names() == null ? null : names().byIndex(index);
        return ci != null ? ci.name(de) : BuildData.CLASS_CODES[index];
    }

    private String skillName(int id, String key) {
        BuildModel.SkillInfo si = names() == null ? null : names().skills.get(id);
        return si != null ? si.name(de) : key;
    }

    private String treeName(int cls, int page) {
        BuildModel.ClassInfo ci = names() == null ? null : names().byIndex(cls);
        return ci != null ? ci.tab(page, de) : String.valueOf(page);
    }

    private String tagName(String t) { for (int i = 0; i < TAGS.length; i++) if (TAGS[i].equals(t)) return s(TAG_IDS[i]); return t; }
    private String slotName(String t) { for (int i = 0; i < SLOTS.length; i++) if (SLOTS[i].equals(t)) return s(SLOT_IDS[i]); return t; }
    private String tierName(String t) { return t.equals("bis") ? s(R.string.dmAndroidBuildTierBis) : t.equals("alt") ? s(R.string.dmAndroidBuildTierAlt) : s(R.string.dmAndroidBuildTagBudget); }
    private String statName(String st) { BuildModel.Text t = names() == null ? null : names().stats.get(st); return t != null ? t.get(de) : st; }
    private String phaseLabel(BuildModel.Phase p) { return p.name.get(de) + " · " + p.lo + "–" + p.hi; }


    private String phaseTab(int n, int i, BuildModel.Phase p) {
        String label = phaseLabel(p);
        return style.panelTabFits(win, n, i, label) ? label : p.name.get(de);
    }


    private static String qualityColour(String kind) {
        switch (kind) {
            case "unique": case "runeword": return "Tooltip.UniqueColor";
            case "set": return "Tooltip.SetColor";
            case "magic": return "Tooltip.MagicColor";
            case "rare": return "Tooltip.RareColor";
            case "crafted": return "Tooltip.CraftedColor";
            default: return "Tooltip.DefaultColor";
        }
    }


    private String[] gearText(BuildModel.Gear x) {
        BuildModel.Names n = names();
        String prefix = x.kind.equals("runeword") ? "r:" : x.kind.equals("unique") ? "u:" : x.kind.equals("set") ? "s:" : x.kind.equals("item") ? "i:" : "t:";
        BuildModel.ItemInfo it = n == null ? null : n.items.get(prefix + x.key);
        String name = it != null ? it.name(de) : x.key, img = it != null ? it.img : "", sub = "", baseCode = x.baseItem;
        if (x.kind.equals("runeword")) {
            BuildModel.ItemInfo base = n == null ? null : n.items.get(!x.baseItem.isEmpty() ? "i:" + x.baseItem : "t:" + x.baseType);
            if (base != null) img = base.img;
            sub = s(R.string.dmAndroidBuildRuneword) + " · " + (base != null ? base.name(de) : x.baseItem + x.baseType) + " · " + s(R.string.dmAndroidBuildSockets, x.sockets);
            if (it != null) sub += " · " + s(R.string.dmAndroidBuildReqLevel, it.lvl);
        } else if (x.kind.equals("unique") || x.kind.equals("set")) {
            BuildModel.Text q = n == null ? null : n.qualities.get(x.kind);
            BuildModel.ItemInfo base = it == null || n == null ? null : n.items.get("i:" + it.code);
            sub = (q != null ? q.get(de) : x.kind) + (base != null ? " · " + base.name(de) : "") + (it != null ? " · " + s(R.string.dmAndroidBuildReqLevel, it.lvl) : "");
        } else if (x.kind.equals("type")) {
            BuildModel.Text q = n == null || x.quality.isEmpty() ? null : n.qualities.get(x.quality);
            if (q != null) sub = q.get(de);
        }
        return new String[]{name, img, sub, baseCode};
    }


    @Override protected void onDraw(Canvas c) {
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "build_book");
        try {
        refreshLang();
        hitRects.clear();
        focusRects.clear();
        hits.clear();
        recipeTargets.clear();
        gearHits.clear();
        style.backdrop(c);
        float h = getHeight(), w = getWidth();
        win = style.panel(w, h);
        style.panelFrame(c, win);
        panel.set(win.contentL, win.contentT, win.contentR, win.contentB);
        shaded = false;
        shadeDx = headShadow == null ? 0 : GameStyleGeometry.shadow(headShadow.dx, win.k);
        shadeDy = headShadow == null ? 0 : GameStyleGeometry.shadow(headShadow.dy, win.k);
        float m = MARGIN_UNITS * win.k, rowH = ROW_UNITS * win.k, top = win.contentT + m / 2, x = panel.left + m, right = panel.right - m;
        head.setTextSize(rowH * 0.36f);
        body.setTextSize(rowH * 0.4f);
        if (!artReady()) { message(c, s(R.string.dmAndroidBuildNoArt), panel.centerY()); drawClose(c); return; }
        if (data == null) { message(c, "…", panel.centerY()); drawClose(c); return; }
        if (level == 0) drawList(c, x, right, top, rowH, m);
        else drawGuide(c, x, right, top, rowH, m);
        focus = -1;
        for (int i = 0; i < hits.size() && focusKind >= 0; i++) if (hits.get(i)[0] == focusKind && hits.get(i)[1] == focusWhich) { focus = i; break; }
        if (focus >= 0 && focus < hitRects.size()) {
            RectF f = new RectF(focusRects.get(focus)); float g = win.s * 12;
            f.inset(-g, -g);
            art.focus(c, f, win.s * 1.8f, paint);
        }
        if (preview != null) drawPreview(c);

        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }




    private static final String[] ART = {"button", "button_down", "build_inset", "build_row", "build_long0", "build_long1", "build_chip", "build_check0",
        "build_check1", "build_divider", "build_focus_0", "build_tree"};
    private boolean artReady() {
        for (String a : ART) if (art.image(a) == null) return false;
        return art.hasRole(AppArt.EXOCET) && art.hasRole(AppArt.FORMAL);
    }


    private void drawClose(Canvas c) {
        style.closeX(c, win);
        r.set(win.hitCloseL, win.hitCloseT, win.hitCloseR, win.hitCloseB);
        hit(r, HIT_CLOSE, 0, win.closeL, win.closeT, win.closeR, win.closeB);
    }


    private void stripTab(Canvas c, int n, int i, String text, boolean on, int kind, int which) { stripTab(c, n, i, text, on, kind, which, style.panelTabK(win, n, i, text)); }

    private void stripTab(Canvas c, int n, int i, String text, boolean on, int kind, int which, float k) {
        style.panelTab(c, win, n, i, text, on, k);
        win.tab(n, i, box4); float l = box4[0], t = box4[1], rr = box4[2], b = box4[3];
        win.tabHit(n, i, box4); r.set(box4[0], box4[1], box4[2], box4[3]);
        hit(r, kind, which, l, t, rr, b);
    }


    private void subTab(Canvas c, String text, boolean on, int kind, int which) {
        style.subTab(c, win, box4, text, on);
        r.set(box4[0], box4[1], box4[2], box4[3]);
        hit(r, kind, which);
    }



    private void drawStatus(Canvas c, BuildModel.Guide g, float x, float y, float width, float size) {
        String[] names = {s(R.string.dmAndroidBuildChkTables), s(R.string.dmAndroidBuildChkReview), s(R.string.dmAndroidBuildChkPlay)};
        boolean[] on = {g.checks.contains("tables-3.3.94012"), g.checks.contains("two-sources") && g.checks.contains("review"), g.played};
        head.setTextSize(size);
        for (int i = 0; i < 3; i++) statusW[i] = head.measureText(names[i]);


        size = GameStyleGeometry.checkRow(statusW, size, style.panelSmallPx(win), width, statusMax);
        head.setTextSize(size); float at = x;
        for (int i = 0; i < 3; i++) {
            head.setColor(on[i] ? inkWhite : inkDim);
            at += left(c, head, names[i], at, y, statusMax[i]) + size * GameStyleGeometry.CHECK_GAP;
            check(c, at, y - size * 0.85f, size * 1.05f, on[i]);
            at += size * GameStyleGeometry.CHECK_STEP;
        }
    }


    private String headerName() { return snap.valid ? snap.name + " · " + className(snap.cls) + " · " + s(R.string.dmAndroidBuildLevel, snap.level) : s(R.string.dmAndroidBuildNotInGame); }

    private String headerLine() {
        if (!snap.valid) return "";
        StringBuilder b = new StringBuilder();
        BuildModel.Guide g = followedGuide();
        if (g == null) b.append(s(R.string.dmAndroidBuildNotFollowing));
        else {
            BuildModel.Follow f = follow();
            int ph = BuildPlan.phase(g, snap.level, f.phase);
            b.append(s(R.string.dmAndroidBuildFollowed, g.title.get(de))).append(" · ").append(s(R.string.dmAndroidBuildPhase, g.phases.get(ph).name.get(de)));
            if ("auto".equals(f.phase)) b.append(' ').append(s(R.string.dmAndroidBuildAuto));
            if (!hints) b.append("  ·  ").append(s(R.string.dmAndroidBuildHintsOff));
        }
        return b.toString();
    }

    private void drawList(Canvas c, float x, float right, float top, float rowH, float m) {
        head.setTextSize(rowH * 0.32f);


        for (int i = 0; i < 8; i++) {
            String name = className(i) + (snap.valid && snap.cls == i ? " " + glyph(head, "•", "*") : "");
            stripTab(c, 8, i, name, i == cls, HIT_CLASS, i, style.panelTabK(win, 8, i, name));
        }
        drawClose(c);
        float y = top, lineH = rowH * 0.62f;
        head.setTextSize(lineH * 0.5f); head.setColor(inkGold); shaded = true;
        left(c, head, headerName(), x + m * 0.5f, y + lineH * 0.6f, right - x - m);
        shaded = false;
        body.setTextSize(lineH * 0.52f); body.setColor(inkBeige);
        left(c, body, headerLine(), x + m * 0.5f, y + lineH * 1.3f, right - x - m);
        if (names() == null) { body.setColor(inkRed); left(c, body, s(R.string.dmAndroidBuildNoNames), x + m * 0.5f, y + lineH * 2.0f, right - x - m); y += lineH * 0.7f; }
        y += lineH * 1.55f + m * 0.3f;
        List<BuildModel.Guide> list = data.ofClass(cls);
        float footH = rowH * 0.75f, y1 = panel.bottom - m * 0.5f, gap = m * 0.5f;
        int per = 3;
        float cardH = Math.min((y1 - y - gap * (per - 1)) / per, getHeight() * 0.2f);
        listPages = Math.max(1, (list.size() + per - 1) / per);
        if (listPage >= listPages) listPage = listPages - 1;
        if (list.isEmpty()) message(c, s(R.string.dmAndroidBuildNone), (y + y1) / 2);
        for (int i = 0; i < per && listPage * per + i < list.size(); i++) {
            RectF at = new RectF(x, y + i * (cardH + gap), right, y + i * (cardH + gap) + cardH);
            drawCard(c, list.get(listPage * per + i), at, listPage * per + i);
        }
        if (!note.isEmpty()) { body.setTextSize(footH * 0.4f); body.setColor(inkRed); centre(c, body, note, panel.centerX(), y1 - m * 0.2f, panel.width() * 0.8f); }
        drawPager(c, listPage, listPages, footH, m);
    }



    private void drawPager(Canvas c, int at, int of, float footH, float m) {
        float w = getWidth();
        style.footText(c, win, w, 1, s(R.string.dmAndroidBuildPage) + " " + (at + 1) + "/" + of, "LightButtonTextColor");
        for (int side = 0; side < 2; side++) {
            int col = side == 0 ? 0 : 2;
            boolean can = side == 0 ? at > 0 : at < of - 1;
            style.footButton(c, win, w, col, can ? GameStyle.BUTTON_NORMAL : GameStyle.BUTTON_DISABLED, null, side == 0 ? -1 : 1);
            win.foot(w, col, box4); float l = box4[0], t = box4[1], rr = box4[2], b = box4[3];
            win.footHit(w, col, box4); r.set(box4[0], box4[1], box4[2], box4[3]);
            hit(r, side == 0 ? HIT_PREV : HIT_NEXT, 0, l, t, rr, b);
        }
    }

    private void drawCard(Canvas c, BuildModel.Guide g, RectF at, int index) {
        BuildModel.Guide followed = followedGuide();
        boolean mine = followed != null && followed.id.equals(g.id);
        field(c, at, mine);
        hit(at, HIT_CARD, index);
        float h = at.height(), pad = h * 0.1f, icon = h * 0.32f;
        for (int i = 0; i < g.keySkills.length && i < 3; i++) {
            float cx = at.left + pad + icon / 2 + i * (icon * 1.1f);
            Bitmap b = picture("skills/" + g.keySkills[i]);
            if (b != null) fit(c, b, cx, at.top + h * 0.38f, icon, 255);
            else { r.set(cx - icon / 2, at.top + h * 0.38f - icon / 2, cx + icon / 2, at.top + h * 0.38f + icon / 2); box.setStyle(Paint.Style.STROKE); box.setColor(inkDarkGold); c.drawRect(r, box); }
        }

        float tx = at.left + pad + icon * 3.4f, sideW = at.width() * 0.22f, midW = at.width() * 0.3f, mx = at.right - pad - sideW - midW;
        head.setTextSize(h * 0.17f);
        head.setColor(inkGold); shaded = true;
        shrinkToFit(head, g.title.get(de), mx - tx - pad, CARD_TITLE_MIN);
        left(c, head, g.title.get(de), tx, at.top + h * 0.3f, mx - tx - pad);
        shaded = false;
        body.setTextSize(h * 0.14f);
        body.setColor(inkBeige);
        StringBuilder sub = new StringBuilder();
        for (int t : g.trees) sub.append(sub.length() > 0 ? " · " : "").append(treeName(g.classIndex, t));
        left(c, body, sub.toString(), tx, at.top + h * 0.5f, mx - tx - pad);
        body.setTextSize(h * 0.12f); body.setColor(inkDescription);
        String sum = g.summary.get(de); int dot = sum.indexOf(". "); if (dot > 0) sum = sum.substring(0, dot + 1);
        List<String> sl = wrap(body, sum, mx - tx - pad);
        for (int i = 0; i < sl.size() && i < 2; i++) left(c, body, i == 1 && sl.size() > 2 ? sl.get(i) + " …" : sl.get(i), tx, at.top + h * (0.7f + 0.15f * i), mx - tx - pad);

        float cy = at.top + h * 0.14f, chipH = h * 0.2f, cx = mx;
        for (String t : g.tags) { String n = tagName(t); float cw = chipWidth(n, chipH); if (cx + cw > at.right - pad) break; chip(c, n, cx, cy, chipH); cx += cw + chipH * 0.3f; }
        body.setTextSize(h * 0.13f);
        body.setColor(inkBeige);
        String diff = g.difficulty == 1 ? s(R.string.dmAndroidBuildEasy) : g.difficulty == 2 ? s(R.string.dmAndroidBuildMedium) : s(R.string.dmAndroidBuildHard);
        left(c, body, s(R.string.dmAndroidBuildDifficulty, diff) + " · " + s(R.string.dmAndroidBuildPatch, g.patch), mx, at.top + h * 0.58f, midW);
        left(c, body, s(R.string.dmAndroidBuildSourcesN, g.sources.size()), mx, at.top + h * 0.78f, midW);

        float sx = at.right - pad - sideW;
        if (snap.valid && snap.cls == g.classIndex) {
            r.set(sx, at.top + h * 0.42f, at.right - pad, at.top + h * 0.82f);
            head.setTextSize(h * 0.14f);
            bigButton(c, new RectF(r), mine ? s(R.string.dmAndroidBuildFollowing) : s(R.string.dmAndroidBuildFollow), HIT_FOLLOW, index);
        }
    }

    private void drawGuide(Canvas c, float x, float right, float top, float rowH, float m) {
        BuildModel.Guide g = guide;

        boolean own = snap.valid && snap.cls == g.classIndex;
        BuildModel.Guide followed = followedGuide();
        boolean mine = followed != null && followed.id.equals(g.id);
        int n = 1 + g.phases.size() + (own ? 1 : 0);
        head.setTextSize(rowH * 0.32f);
        stripTab(c, n, 0, glyph(head, "‹", "<") + " " + s(R.string.dmAndroidBack), false, HIT_BACK, 0);
        for (int i = 0; i < g.phases.size(); i++) stripTab(c, n, 1 + i, phaseTab(n, 1 + i, g.phases.get(i)), i == phase, HIT_PHASE, i);
        if (own) stripTab(c, n, n - 1, mine ? s(R.string.dmAndroidBuildFollowing) : s(R.string.dmAndroidBuildFollow), mine, HIT_FOLLOW, -1);
        drawClose(c);

        float y = top, subH = win.subTabH();
        for (int i = 0; i < 6; i++) { win.subTab(x, y, 0, i, box4); subTab(c, s(TAB_IDS[i]), i == tab, HIT_TAB, i); }
        float tx = box4[2] + m;
        head.setTextSize(rowH * 0.38f); head.setColor(inkGold); shaded = true;
        float titleW = right - tx;
        shrinkToFit(head, g.title.get(de), titleW, DETAIL_TITLE_MIN);
        left(c, head, g.title.get(de), tx, y + subH / 2 + head.getTextSize() * 0.36f, titleW);
        shaded = false;
        float cy0 = y + subH + m / 2, cy1 = panel.bottom - m;
        RectF area = new RectF(x, cy0, right, cy1);
        if (!note.isEmpty()) { body.setTextSize(rowH * 0.3f); body.setColor(inkRed); centre(c, body, note, panel.centerX(), cy1 - m * 0.3f, panel.width() * 0.8f); area.bottom -= rowH * 0.45f; }
        switch (tab) {
            case TAB_OVERVIEW: drawOverview(c, g, area, m); break;
            case TAB_SKILLS: drawSkills(c, g, area, m); break;
            case TAB_STATS: drawStats(c, g, area, m); break;
            case TAB_GEAR: drawGear(c, g.gear, g.phases.get(phase).id, area, m, false); break;
            case TAB_MERC: drawMerc(c, g, area, m); break;
            default: drawSources(c, g, area, m);
        }
    }

    private float paragraph(Canvas c, String text, float x, float y, float width, float size, int colour, int maxLines) {
        body.setTextSize(size); body.setColor(colour);
        int n = 0;
        for (String line : wrap(body, text, width)) { if (n++ >= maxLines) break; left(c, body, line, x, y + size, width); y += size * 1.3f; }
        return y;
    }






    private static final int OV_SUMMARY = 0, OV_STRONG = 1, OV_WEAK_HEAD = 2, OV_WEAK = 3, OV_PHASE = 4, OV_PHASE_NOTE = 5, OV_NOTE = 6;
    private static final int OV_LINES = 99;
    private static final class OvEntry {
        final int kind, section; final String text; final float size; final int colour; final float before, after;
        OvEntry(int kind, int section, String text, float size, int colour, float before, float after) {
            this.kind = kind; this.section = section; this.text = text; this.size = size; this.colour = colour; this.before = before; this.after = after; }
    }

    private void drawOverview(Canvas c, BuildModel.Guide g, RectF a, float m) {
        float footH = a.height() * 0.075f, stripH = a.height() * 0.22f, half = a.width() / 2 - m / 2, k = gamePx();
        float bottom0 = a.bottom - footH - stripH - m * 0.6f, bottom1 = a.bottom - footH - m * 0.3f;
        float fs = a.height() * 0.043f, textW = half - 24 * k - m;
        List<OvEntry> list = new ArrayList<>();
        list.add(new OvEntry(OV_SUMMARY, 0, g.summary.get(de), fs * 1.05f, inkWhite, 0, 0));
        for (BuildModel.Text t : g.strengths) list.add(new OvEntry(OV_STRONG, 1, "· " + t.get(de), fs * 0.95f, inkBeige, 0, fs * 0.15f));
        if (!g.weaknesses.isEmpty()) {
            list.add(new OvEntry(OV_WEAK_HEAD, 1, s(R.string.dmAndroidBuildWeaknesses), fs, inkGold, fs * 0.2f, 0));
            for (BuildModel.Text t : g.weaknesses) list.add(new OvEntry(OV_WEAK, 1, "· " + t.get(de), fs * 0.95f, inkBeige, 0, fs * 0.15f));
        }
        for (BuildModel.Phase p : g.phases) {
            list.add(new OvEntry(OV_PHASE, 2, phaseLabel(p) + (p.respec ? " · " + s(R.string.dmAndroidBuildRespec) : ""), fs, inkGold, 0, 0));
            if (p.note != null) list.add(new OvEntry(OV_PHASE_NOTE, 2, p.note.get(de), fs * 0.95f, inkWhite, 0, fs * 0.3f));
        }
        for (BuildModel.Text t : g.notes) list.add(new OvEntry(OV_NOTE, 3, "· " + t.get(de), fs * 0.92f, inkNote, 0, fs * 0.2f));
        int n = list.size(); float[] hgt = new float[n]; int[] sec = new int[n]; boolean[] keep = new boolean[n];
        for (int i = 0; i < n; i++) { OvEntry e = list.get(i); sec[i] = e.section; keep[i] = e.kind == OV_WEAK_HEAD || e.kind == OV_PHASE;
            hgt[i] = e.before + entryH(e, textW, fs) + e.after; }


        int[] col = OverviewFlow.place(hgt, sec, keep, bottom0 - a.top - 56 * k - m * 0.3f, bottom1 - a.top - 56 * k - m * 0.3f);
        pages = OverviewFlow.pages(col);
        if (page >= pages) page = pages - 1;
        float bottom = page == 0 ? bottom0 : bottom1;
        for (int side = 0; side < 2; side++) {
            int want = page * 2 + side, from = -1;
            for (int i = 0; i < n && from < 0; i++) if (col[i] == want) from = i;
            if (from < 0) continue;
            RectF at = side == 0 ? new RectF(a.left, a.top, a.left + half, bottom) : new RectF(a.right - half, a.top, a.right, bottom), in = column(c, at);
            OvEntry first = list.get(from);
            columnTitle(c, at, s(first.kind == OV_SUMMARY ? R.string.dmAndroidBuildTabOverview : first.kind == OV_STRONG ? R.string.dmAndroidBuildStrengths
                : first.kind == OV_WEAK_HEAD || first.kind == OV_WEAK ? R.string.dmAndroidBuildWeaknesses : first.section == 2 ? R.string.dmAndroidBuildPhases : R.string.dmAndroidBuildNotes));
            float y = in.top + (first.kind == OV_SUMMARY ? m * 0.3f : m * 0.2f);
            for (int i = from; i < n && col[i] == want; i++) {
                OvEntry e = list.get(i);
                if (e.kind == OV_WEAK_HEAD && i == from) continue;
                y += e.before;
                if (e.kind == OV_WEAK_HEAD || e.kind == OV_PHASE) y = heading(c, e.text, in, y, fs, m);
                else y = paragraph(c, e.text, in.left + m * 0.5f, y, in.width() - m, e.size, e.colour, OV_LINES);
                y += e.after;
            }
        }
        if (page == 0) drawKeyItems(c, g, new RectF(a.left, bottom0 + m * 0.6f, a.right, a.bottom - footH), m);
        pageLine(c, a, footH);
    }



    private float entryH(OvEntry e, float width, float fs) {
        if (e.kind == OV_WEAK_HEAD || e.kind == OV_PHASE) return fs * 1.6f;
        body.setTextSize(e.size);
        return wrap(body, e.text, width).size() * e.size * 1.3f;
    }


    private void pageLine(Canvas c, RectF a, float footH) {
        if (pages <= 1) return;
        head.setTextSize(footH * 0.5f); head.setColor(inkLightGold);
        centre(c, head, s(R.string.dmAndroidBuildPage) + " " + (page + 1) + "/" + pages + "  ‹ ›", a.centerX(), a.bottom - footH * 0.28f, a.width());
        r.set(a.left, a.bottom - footH, a.centerX(), a.bottom); hit(r, HIT_PREV, 0);
        r.set(a.centerX(), a.bottom - footH, a.right, a.bottom); hit(r, HIT_NEXT, 0);
    }


    private void drawKeyItems(Canvas c, BuildModel.Guide g, RectF strip, float m) {
        field(c, strip, false);
        String last = g.phases.get(g.phases.size() - 1).id;
        List<BuildModel.Gear> keys = new ArrayList<>();
        for (BuildModel.Gear x : g.gear) if (x.phase.equals(last) && x.tier.equals("bis") && keys.size() < 6) keys.add(x);
        float sh = strip.height(), lw = sh * 1.5f;
        head.setTextSize(sh * 0.15f); head.setColor(inkGold);
        List<String> kl = wrap(head, s(R.string.dmAndroidBuildKeyItems), lw - m * 0.6f);
        for (int k = 0; k < kl.size() && k < 2; k++) left(c, head, kl.get(k), strip.left + m * 0.6f, strip.centerY() + head.getTextSize() * (kl.size() > 1 ? (k == 0 ? -0.3f : 0.95f) : 0.36f), lw);



        float size = sh * 0.15f, wordEm = 0, slotEm = 0, gx = strip.left + lw + m * 0.4f;
        head.setTextSize(size); body.setTextSize(size);
        for (BuildModel.Gear x : keys) {
            for (String word : gearText(x)[0].split(" ")) wordEm = Math.max(wordEm, head.measureText(word) / size);
            slotEm = Math.max(slotEm, body.measureText(slotName(x.slot)) / size);
        }
        items.compute(keys.size(), strip.right - m * 0.6f - gx, sh, wordEm, slotEm, size, style.panelSmallPx(win));
        for (int i = 0; i < keys.size(); i++) {
            BuildModel.Gear x = keys.get(i); String[] t = gearText(x);
            items.cell(gx, strip.top, i, box4); float ix = box4[0], top = box4[1], pic = items.pic, tx = ix + pic * GameStyleGeometry.ItemGrid.TEXT_AT;
            gearHits.add(x); hit(new RectF(ix, top, ix + items.cellW - m * 0.2f, top + items.cellH), HIT_GEAR, gearHits.size() - 1);
            fit(c, picture("items/" + t[1]), ix + pic / 2, top + items.cellH / 2, pic, 255);
            body.setTextSize(items.slotSize); body.setColor(inkNote);
            left(c, body, slotName(x.slot), tx, top + items.cellH * items.slotAt, items.textW);
            head.setTextSize(items.nameSize); head.setColor(style.colour(qualityColour(x.kind)));
            List<String> nl = wrap(head, t[0], items.textW);
            for (int k = 0; k < nl.size() && k < items.nameLines; k++) {
                String line = k == items.nameLines - 1 && nl.size() > items.nameLines ? String.join(" ", nl.subList(k, nl.size())) : nl.get(k);
                left(c, head, line, tx, top + items.cellH * (items.nameAt + items.nameStep * k), items.textW);
            }
        }
    }


    private float heading(Canvas c, String text, RectF col, float y, float fs, float m) {
        head.setTextSize(fs * 0.95f); head.setColor(inkGold); shaded = true;
        left(c, head, text, col.left + m * 0.5f, y + fs, col.width() - m);
        shaded = false;
        r.set(col.left + m * 0.5f, y + fs * 1.25f, col.right - m * 0.5f, y + fs * 1.25f + gamePx() * 4);
        image(c, "build_divider", r);
        return y + fs * 1.6f;
    }

    private void drawSkills(Canvas c, BuildModel.Guide g, RectF a, float m) {
        float lw = a.width() * 0.54f;
        RectF l0 = new RectF(a.left, a.top, a.left + lw, a.bottom), r0 = new RectF(a.left + lw + m, a.top, a.right, a.bottom);
        RectF l = column(c, l0), rr = column(c, r0);
        BuildModel.Phase p = g.phases.get(phase);
        columnTitle(c, r0, s(R.string.dmAndroidBuildEndDist, p.hi));
        List<BuildModel.Step> chain = BuildPlan.chain(g, phase);
        int own0 = chain.size() - p.skills.size();
        boolean live = snap.valid && snap.cls == g.classIndex && !snap.base.isEmpty();
        BuildPlan.Result res = live ? planNow(g, phase) : null;


        float u = l.height() / 12.5f, lineH = l.height() / 10.8f, fs = u * 0.52f, y = l.top;
        String arrow = arrow();
        head.setTextSize(fs * 0.8f); head.setColor(inkGold);
        left(c, head, s(R.string.dmAndroidBuildColLevel), l.left + m * 0.5f, y + lineH * 0.6f, u * 2);
        left(c, head, s(R.string.dmAndroidBuildColSkill).replace("→", arrow), l.left + m * 0.5f + u * 1.7f, y + lineH * 0.6f, l.width() * 0.4f);
        columnTitle(c, l0, phaseLabel(p));
        if (live) {
            String st = s(R.string.dmAndroidBuildLevel, snap.level) + " · " + s(R.string.dmAndroidBuildFreeSkill, snap.skillPoints);
            int out = BuildPlan.outsidePlan(chain, snap.base);
            if (out > 0) st += " · " + s(R.string.dmAndroidBuildOutside, out);
            body.setTextSize(gamePx() * 25); body.setColor(inkBeige);
            body.setTextAlign(Paint.Align.RIGHT); textObserved(c, TextUtils.ellipsize(st, body, l0.width() * 0.62f, TextUtils.TruncateAt.END).toString(), l0.right - gamePx() * 16, l0.top + gamePx() * 32, body);
        }
        boolean respecHere = live && isFollowed(g) && BuildPlan.pendingRespec(g, snap.level, follow().phase) == phase;
        if (respecHere) {
            RectF b = new RectF(l.left + m, y + lineH * 0.95f, l.left + l.width() * 0.6f, y + lineH * 1.85f);
            head.setTextSize(fs * 0.8f);
            bigButton(c, b, s(R.string.dmAndroidBuildRespecDone), HIT_RESPEC, phase);
            y += lineH;
        }
        y += lineH;
        int fits = respecHere ? 8 : 9, per = p.skills.size() > fits ? fits - 1 : fits;
        pages = Math.max(1, (p.skills.size() + per - 1) / per);
        if (page >= pages) page = pages - 1;
        for (int i = page * per; i < p.skills.size() && i < (page + 1) * per; i++) {
            BuildModel.Step st = p.skills.get(i);
            int ci = own0 + i;
            boolean done = live && BuildPlan.get(snap.base, st.skill) >= st.to, current = res != null && res.index == ci;
            r.set(l.left, y + lineH * 0.08f, l.right, y + lineH * 0.92f);
            if (current) litRow(c, new RectF(r));
            else art.nine(c, "build_inset", new RectF(r), 8, gamePx(), paint);
            if (done) check(c, l.left + m * 0.35f, y + lineH * 0.5f - fs * 0.5f, fs, true);
            body.setTextSize(fs);
            body.setColor(done ? inkDim : current ? inkLightGold : inkWhite);
            body.setTextAlign(Paint.Align.RIGHT);
            textObserved(c, String.valueOf(st.lvl), l.left + m * 0.5f + u * 1.3f, y + lineH * 0.5f + fs * 0.36f, body);
            Bitmap ic = picture("skills/" + st.skill);
            fit(c, ic, l.left + m * 0.5f + u * 1.95f, y + lineH * 0.5f, u * 0.82f, done ? 110 : 255);
            float nx = l.left + m * 0.5f + u * 2.55f;
            body.setColor(done ? inkDim : current ? inkLightGold : inkWhite);
            left(c, body, skillName(st.skill, st.key) + " " + arrow + " " + st.to, nx, y + lineH * 0.5f + fs * 0.36f, l.width() * 0.4f);
            String info = st.note != null ? st.note.get(de) : "";
            if (current) info = res.state == BuildPlan.NEXT ? s(R.string.dmAndroidBuildNextPoint) : res.state == BuildPlan.UPCOMING ? s(R.string.dmAndroidBuildAtLevelUp)
                : res.state == BuildPlan.SAVE ? (res.at > 0 ? s(snap.skillPoints == 1 ? R.string.dmAndroidBuildSaveUntil1 : R.string.dmAndroidBuildSaveUntil, res.at) : s(R.string.dmAndroidBuildSavePoint))
                : res.state == BuildPlan.BLOCKED ? s(R.string.dmAndroidBuildBlocked)
                : res.state == BuildPlan.WAIT ? (res.at > 0 ? s(R.string.dmAndroidBuildWaitUntil, res.at) : s(R.string.dmAndroidBuildWaiting)) : info;
            else if (st.save && !done) info = info.isEmpty() ? s(R.string.dmAndroidBuildSavePoint) : info;
            body.setTextSize(fs * 0.88f);
            body.setColor(current ? inkGold : inkNote);
            if (body.measureText(info) > l.width() * 0.37f) {
                body.setTextSize(fs * 0.72f); List<String> il = wrap(body, info, l.width() * 0.37f);
                for (int k = 0; k < il.size() && k < 2; k++) left(c, body, il.get(k), l.left + l.width() * 0.62f, y + lineH * (0.42f + 0.3f * k), l.width() * 0.37f);
            } else left(c, body, info, l.left + l.width() * 0.62f, y + lineH * 0.5f + body.getTextSize() * 0.36f, l.width() * 0.37f);
            y += lineH;
        }
        if (res != null && res.state == BuildPlan.DONE) { body.setTextSize(fs); body.setColor(style.colour("FontColorPartyGreen")); left(c, body, s(R.string.dmAndroidBuildDone), l.left + m * 0.5f, y + lineH * 0.55f, l.width()); }
        if (pages > 1) {
            head.setTextSize(fs * 0.8f); head.setColor(inkLightGold);
            centre(c, head, s(R.string.dmAndroidBuildPage) + " " + (page + 1) + "/" + pages + "  ‹ ›", l.centerX(), l.bottom - lineH * 0.25f, l.width());
            r.set(l.left, l.bottom - lineH, l.centerX(), l.bottom); hit(r, HIT_PREV, 0);
            r.set(l.centerX(), l.bottom - lineH, l.right, l.bottom); hit(r, HIT_NEXT, 0);
        }

        Map<Integer, Integer> dist = BuildPlan.distribution(chain);
        float tw = (rr.width() - m) / 3, ty = rr.top + u * 0.15f, th = u * 0.9f;
        head.setTextSize(th * 0.34f);
        for (int pg = 3; pg >= 1; pg--) {
            int sum = 0;
            if (names() != null) for (Map.Entry<Integer, Integer> e : dist.entrySet()) { BuildModel.SkillInfo si = names().skills.get(e.getKey()); if (si != null && si.page == pg) sum += e.getValue(); }
            float bx = rr.left + m * 0.5f + (3 - pg) * tw;
            button(c, bx, ty, bx + tw - m / 3, ty + th, treeName(g.classIndex, pg) + " " + sum, pg == treePage, head, HIT_TREE, pg);
        }
        if (names() != null) drawTree(c, g, dist, new RectF(rr.left, ty + th + m * 0.4f, rr.right, rr.bottom - m * 0.2f));
    }









    private static final int[] TREE_COL = {451, 789, 1125}, TREE_ROW = {179, 368, 560, 750, 939, 1129};
    private void drawTree(Canvas c, BuildModel.Guide g, Map<Integer, Integer> dist, RectF at) {
        Bitmap slab = art.image("build_tree"), tree = art.image("build_tree_" + g.cls + "_" + (3 - treePage));
        final int cut = 56;
        float sc = Math.min(at.width() / 629f, at.height() / (638f - cut)), w = 629 * sc, h = (638 - cut) * sc, ox = at.centerX() - w / 2, oy = at.top - cut * sc;
        r.set(ox, at.top, ox + w, at.top + h);
        if (slab != null) c.drawBitmap(slab, new android.graphics.Rect(134, 1 + cut, 763, 639), r, paint);
        if (tree != null) c.drawBitmap(tree, new android.graphics.Rect(0, cut, 629, 638), r, paint);
        for (BuildModel.SkillInfo si : names().skills.values()) {
            if (si.page != treePage || !si.cls.equals(g.cls) || si.col < 1 || si.col > 3 || si.row < 1 || si.row > 6) continue;
            float sx = ox + ((TREE_COL[si.col - 1] - 268) / 2f + 8) * sc, sy = oy + ((TREE_ROW[si.row - 1] - 3) / 2f + 1) * sc;
            int pts = BuildPlan.get(dist, si.id);



            fit(c, picture("skills/" + si.id), sx + 33 * sc, sy + 33 * sc, 72 * sc, pts > 0 ? 255 : 90);
            if (pts > 0) {
                head.setTextSize(22 * sc); head.setColor(inkWhite); head.setTextAlign(Paint.Align.CENTER);
                textObserved(c, String.valueOf(pts), sx + 33 * sc + 60 * sc, sy + 33 * sc + 21.5f * sc + 22 * sc * 0.36f, head);
            }
        }
    }

    private void drawStats(Canvas c, BuildModel.Guide g, RectF a, float m) {
        pages = 1; page = 0;
        float lw = a.width() * 0.58f;
        RectF l0 = new RectF(a.left, a.top, a.left + lw, a.bottom), r0 = new RectF(a.left + lw + m, a.top, a.right, a.bottom);
        RectF l = column(c, l0), rr = column(c, r0);
        BuildModel.Phase p = g.phases.get(phase);
        boolean live = snap.valid && snap.cls == g.classIndex;
        Map<String, Integer> baseNow = live ? snap.baseMap() : null;
        int next = BuildPlan.statNext(p.stats, baseNow);
        columnTitle(c, l0, phaseLabel(p)); columnTitle(c, r0, live ? (baseNow != null ? s(R.string.dmAndroidBuildStatNowBase) : s(R.string.dmAndroidBuildStatNow)) : s(R.string.dmAndroidBuildTabStats));
        float lineH = l.height() / 8.6f, fs = lineH * 0.36f, y = l.top + m * 0.2f;
        body.setTextSize(fs * 0.9f); body.setColor(inkNote);
        left(c, body, s(R.string.dmAndroidBuildStatHint), l.left + m * 0.5f, y + fs, l.width() - m);
        y += lineH * 0.55f;
        for (int i = 0; i < p.stats.size(); i++) {
            BuildModel.StatRule st = p.stats.get(i);
            r.set(l.left, y, l.right, y + lineH * 0.95f);
            if (i == next) litRow(c, new RectF(r)); else art.nine(c, "build_inset", new RectF(r), 8, gamePx(), paint);
            head.setTextSize(fs * 1.1f); head.setColor(inkGold);
            left(c, head, (i + 1) + ". " + statName(st.stat), l.left + m * 0.6f, y + lineH * 0.42f, l.width() * 0.34f);
            body.setTextSize(fs * 1.15f); body.setColor(i == next ? inkLightGold : inkWhite);
            String what = st.rest ? s(R.string.dmAndroidBuildStatRest) : st.none ? s(R.string.dmAndroidBuildStatNone) : s(R.string.dmAndroidBuildStatTo, st.to);
            left(c, body, what, l.left + l.width() * 0.38f, y + lineH * 0.42f, l.width() * 0.6f);
            if (st.why != null) { body.setTextSize(fs * 0.92f); body.setColor(inkNote); left(c, body, st.why.get(de), l.left + l.width() * 0.38f, y + lineH * 0.8f, l.width() * 0.6f); }
            y += lineH * 1.02f;
        }


        y = rr.top + m * 0.2f;
        body.setTextSize(fs); body.setColor(inkBeige);
        left(c, body, live ? snap.name + " · " + s(R.string.dmAndroidBuildLevel, snap.level) : s(R.string.dmAndroidBuildNotInGame), rr.left + m * 0.5f, y + fs, rr.width() - m);
        y += lineH * 0.6f;
        String nextKey = next >= 0 ? p.stats.get(next).stat : "";
        for (int i = 0; i < 4; i++) {
            boolean here = live && snap.statPoints > 0 && STAT_KEYS[i].equals(nextKey);
            r.set(rr.left, y, rr.right, y + lineH * 0.7f);
            if (here) litRow(c, new RectF(r)); else art.nine(c, "build_inset", new RectF(r), 8, gamePx(), paint);
            head.setTextSize(fs * 1.05f); head.setColor(inkGold);
            left(c, head, statName(STAT_KEYS[i]), rr.left + m * 0.6f, y + lineH * 0.46f, rr.width() * 0.45f);
            if (live) {
                String v = snap.baseAttr[i] >= 0 ? s(R.string.dmAndroidBuildStatBase, snap.baseAttr[i], snap.attr[i]) : String.valueOf(snap.attr[i]);
                body.setTextSize(fs * 1.1f); body.setColor(here ? inkLightGold : inkWhite); body.setTextAlign(Paint.Align.RIGHT);
                textObserved(c, TextUtils.ellipsize(v, body, rr.width() * 0.5f, TextUtils.TruncateAt.END).toString(), rr.right - m * 0.6f, y + lineH * 0.46f, body);
            }
            y += lineH * 0.78f;
        }
        if (live) {
            body.setTextSize(fs * 1.05f); body.setColor(inkGold);
            String free = s(R.string.dmAndroidBuildFreeStat, snap.statPoints);
            if (snap.statPoints > 0 && next >= 0) free += " · " + s(R.string.dmAndroidBuildStatNext) + ": " + statName(nextKey);
            left(c, body, free, rr.left + m * 0.5f, y + lineH * 0.45f, rr.width() - m); y += lineH * 0.7f;
        }
        int need = 0; String what = "";
        for (BuildModel.Gear x : g.gear) {
            if (!x.phase.equals(p.id) || names() == null) continue;
            String code = x.kind.equals("runeword") ? x.baseItem : x.kind.equals("item") ? x.key : "";
            if (x.kind.equals("unique") || x.kind.equals("set")) { BuildModel.ItemInfo it = names().items.get((x.kind.equals("unique") ? "u:" : "s:") + x.key); if (it != null) code = it.code; }
            BuildModel.ItemInfo base = code.isEmpty() ? null : names().items.get("i:" + code);
            if (base != null && base.str > need) { need = base.str; what = gearText(x)[0]; }
        }
        if (need > 0) paragraph(c, statName("str") + ": " + s(R.string.dmAndroidBuildReq, need) + " (" + what + ")", rr.left + m * 0.5f, y, rr.width() - m, fs * 0.95f, inkNote, 2);
    }




    private void drawGear(Canvas c, List<BuildModel.Gear> all, String phaseId, RectF a, float m, boolean merc) {
        List<BuildModel.Gear> list = new ArrayList<>();
        for (BuildModel.Gear x : all) if (merc || x.phase.equals(phaseId)) list.add(x);
        int per = 4;
        pages = Math.max(1, (list.size() + per - 1) / per);
        if (page >= pages) page = pages - 1;
        float footH = pages > 1 ? a.height() * 0.08f : 0, gap = m * 0.4f, rowH = (a.height() - footH - gap * (per - 1)) / per;


        float slotPx = rowH * 0.19f, nameEm = 0, wordEm = 0;
        body.setTextSize(slotPx);
        for (BuildModel.Gear x : list) {
            String sn = slotName(x.slot);
            nameEm = Math.max(nameEm, body.measureText(sn) / slotPx);
            for (String word : sn.split(" ")) wordEm = Math.max(wordEm, body.measureText(word) / slotPx);
        }
        gearRow.compute(a.left, a.width(), rowH, rowH * 0.16f, nameEm, wordEm, slotPx);
        for (int i = 0; i < per && page * per + i < list.size(); i++) {
            BuildModel.Gear x = list.get(page * per + i);
            RectF at = new RectF(a.left, a.top + i * (rowH + gap), a.right, a.top + i * (rowH + gap) + rowH);
            field(c, at, false);
            gearHits.add(x); hit(at, HIT_GEAR, gearHits.size() - 1);
            String[] t = gearText(x);
            float h = at.height(), pad = h * 0.16f, W = at.width();

            body.setTextSize(gearRow.slotSize); body.setColor(inkBeige);
            List<String> sl = wrap(body, slotName(x.slot), gearRow.slotW);
            if (sl.size() > 2) sl = sl.subList(0, 2);
            for (int k = 0; k < sl.size(); k++)
                left(c, body, sl.get(k), at.left + pad, at.centerY() + body.getTextSize() * (0.36f + 1.15f * (k - (sl.size() - 1) / 2f)), gearRow.slotW);
            fit(c, picture("items/" + t[1]), gearRow.picCx, at.centerY(), h * GameStyleGeometry.GearRow.PIC, 255);
            boolean runes = x.kind.equals("runeword") && names() != null && names().items.get("r:" + x.key) != null;
            float tx = gearRow.textX, tw = (runes ? at.left + W * 0.6f : at.left + W * 0.83f) - tx;


            body.setTextSize(h * 0.16f);
            List<String> dl = GameStyleGeometry.GearRow.details(t[2], tw, 2, body::measureText);
            int lines = dl.size() + (x.note != null ? 1 : 0);
            head.setTextSize(h * 0.2f); head.setColor(style.colour(qualityColour(x.kind.equals("type") ? x.quality : x.kind)));
            left(c, head, t[0], tx, at.top + h * GameStyleGeometry.GearRow.nameAt(lines), tw);
            body.setTextSize(Math.max(h * GameStyleGeometry.GearRow.bodyScale(lines), Math.min(h * 0.16f, style.panelSmallPx(win)))); body.setColor(inkNote);
            for (int k = 0; k < dl.size(); k++) left(c, body, dl.get(k), tx, at.top + h * GameStyleGeometry.GearRow.bodyAt(lines, k), tw);
            if (x.note != null) { body.setColor(inkBeige); left(c, body, x.note.get(de), tx, at.top + h * GameStyleGeometry.GearRow.bodyAt(lines, dl.size()), tw); }
            if (runes) {
                BuildModel.ItemInfo it = names().items.get("r:" + x.key);
                int n = it.runes.size(); float rx0 = at.left + W * 0.61f, step = Math.min(W * 0.05f, W * 0.24f / Math.max(1, n));
                for (int k = 0; k < n; k++) {
                    float cx = rx0 + step * (k + 0.5f);
                    fit(c, picture("items/" + it.runes.get(k)), cx, at.top + h * 0.4f, Math.min(h * 0.42f, step * 0.95f), 255);
                    BuildModel.ItemInfo rune = names().items.get("i:" + it.runes.get(k));
                    if (rune != null) { body.setTextSize(h * 0.15f); body.setColor(inkGold); String rn = rune.name(de); int sp = rn.indexOf(' '), dash = rn.indexOf('-'); String shortName = dash > 0 ? rn.substring(0, dash) : sp > 0 ? rn.substring(0, sp) : rn;
                        centre(c, body, shortName, cx, at.top + h * 0.82f, step * 0.98f); }
                }
            }
            float bx0 = at.left + W * 0.86f, bx1 = at.right - pad * 0.6f;
            String tier = tierName(x.tier); float chipH = h * 0.27f;
            chip(c, tier, bx1 - chipWidth(tier, chipH), at.top + h * 0.12f, chipH);
            String target = recipeTarget(x);
            if (target != null && recipes != null && recipes.has(target)) {
                RectF b = new RectF(bx0, at.top + h * 0.5f, bx1, at.bottom - h * 0.1f);
                head.setTextSize(b.height() * 0.38f);
                recipeTargets.add(target);
                bigButton(c, b, s(R.string.dmAndroidBuildRecipeLink), HIT_RECIPE, recipeTargets.size() - 1);
            }
        }
        if (pages > 1) pageLine(c, a, footH);
    }



    private static String recipeTarget(BuildModel.Gear x) {
        if (x.kind.equals("runeword")) return "w:" + x.key;
        if (x.kind.equals("item")) return "o:" + x.key;
        return null;
    }

    private void drawMerc(Canvas c, BuildModel.Guide g, RectF a, float m) {
        BuildModel.Merc mc = g.merc;
        float hh = a.height() * 0.17f;
        RectF top = new RectF(a.left, a.top, a.right, a.top + hh);
        field(c, top, false);
        String diff = names() != null && mc.difficulty >= 1 && mc.difficulty <= names().difficulties.size() ? names().difficulties.get(mc.difficulty - 1).get(de) : String.valueOf(mc.difficulty);
        BuildModel.Text aura = names() == null ? null : names().auras.get(mc.aura);
        head.setTextSize(hh * 0.28f); head.setColor(inkGold);
        left(c, head, s(R.string.dmAndroidBuildAct, mc.act) + " · " + diff + " · " + (aura != null ? aura.get(de) : mc.aura), top.left + m, top.top + hh * 0.42f, top.width() - 2 * m);
        if (mc.why != null) { body.setTextSize(hh * 0.22f); body.setColor(inkWhite); left(c, body, mc.why.get(de), top.left + m, top.top + hh * 0.8f, top.width() - 2 * m); }
        drawGear(c, mc.gear, "", new RectF(a.left, a.top + hh + m * 0.5f, a.right, a.bottom), m, true);
    }


    private void drawSources(Canvas c, BuildModel.Guide g, RectF a, float m) {
        int n = g.sources.size(), per = 4;
        pages = Math.max(1, (n + per - 1) / per);
        if (page >= pages) page = pages - 1;
        float foot = a.height() * 0.12f, pageH = pages > 1 ? a.height() * 0.07f : 0, gap = m * 0.3f, rowH = (a.height() - foot - pageH - gap * (per - 1)) / per;
        for (int k = 0; k < per && page * per + k < n; k++) {
            int i = page * per + k;
            BuildModel.Source src = g.sources.get(i);
            RectF at = new RectF(a.left, a.top + k * (rowH + gap), a.right, a.top + k * (rowH + gap) + rowH);
            field(c, at, false);
            float h = at.height(), pad = h * 0.16f, bw = at.width() * 0.18f;
            head.setTextSize(h * 0.2f); head.setColor(inkGold);
            left(c, head, src.site, at.left + pad, at.top + h * 0.34f, at.width() * 0.16f);
            body.setTextSize(h * 0.2f); body.setColor(inkWhite);
            left(c, body, src.title, at.left + at.width() * 0.18f, at.top + h * 0.34f, at.width() * 0.6f);
            body.setTextSize(h * 0.16f); body.setColor(inkBeige);
            left(c, body, s(R.string.dmAndroidBuildPatch, src.patch) + " · " + s(R.string.dmAndroidBuildSeen, src.seen), at.left + at.width() * 0.18f, at.top + h * 0.6f, at.width() * 0.6f);
            if (src.note != null) { body.setColor(inkNote); left(c, body, src.note.get(de), at.left + at.width() * 0.18f, at.top + h * 0.83f, at.width() * 0.6f); }
            RectF b = new RectF(at.right - pad - bw, at.top + h * 0.2f, at.right - pad, at.bottom - h * 0.2f);
            head.setTextSize(b.height() * 0.32f);
            bigButton(c, b, s(R.string.dmAndroidBuildOpenBrowser), HIT_URL, i);
        }
        RectF rest = new RectF(a.left, a.top, a.right, a.bottom - foot);
        if (pages > 1) pageLine(c, rest, pageH);
        body.setTextSize(foot * 0.3f); body.setColor(inkBeige);
        left(c, body, s(R.string.dmAndroidBuildOwnWords), a.left + m, a.bottom - foot * 0.55f, a.width() - 2 * m);
        drawStatus(c, g, a.left + m, a.bottom - foot * 0.1f, a.width() - 2 * m, foot * 0.28f);
    }


    private int hitAt(float x, float y) { for (int i = hitRects.size() - 1; i >= 0; i--) if (hitRects.get(i).contains(x, y)) return i; return -1; }

    private void toggleFollow(BuildModel.Guide g) {
        if (g == null) return;
        if (!snap.valid) { note = s(R.string.dmAndroidBuildNotInGame); return; }
        if (snap.cls != g.classIndex) { note = s(R.string.dmAndroidBuildOwnClass); return; }
        BuildModel.Guide followed = followedGuide();
        if (followed != null && followed.id.equals(g.id)) BuildStore.remove(getContext(), snap.name, snap.cls);
        else BuildStore.put(getContext(), snap.name, snap.cls, new BuildModel.Follow(g.id, g.version, "auto"));
        note = "";
        lastTarget = Integer.MIN_VALUE;
        sendTarget();
    }

    private void act(int[] h) {
        switch (h[0]) {
            case HIT_CLOSE: close(); return;
            case HIT_CLASS: cls = h[1]; listPage = 0; note = ""; break;
            case HIT_CARD: { List<BuildModel.Guide> l = data.ofClass(cls); if (h[1] >= 0 && h[1] < l.size()) { guide = l.get(h[1]); prepareTabs(); level = 1; tab = TAB_OVERVIEW; page = 0; note = ""; treePage = mainTree(guide);
                BuildModel.Follow f = follow(); phase = BuildPlan.phase(guide, snap.valid ? snap.level : 1, f != null && f.guide.equals(guide.id) ? f.phase : "auto"); focus = focusKind = -1; } break; }
            case HIT_FOLLOW: { List<BuildModel.Guide> l = data.ofClass(cls); toggleFollow(h[1] < 0 ? guide : h[1] < l.size() ? l.get(h[1]) : null); break; }
            case HIT_BACK: back(); return;
            case HIT_PHASE: phase = h[1]; page = 0; break;
            case HIT_TAB: tab = h[1]; page = 0; break;
            case HIT_PREV: flip(-1); return;
            case HIT_NEXT: flip(1); return;
            case HIT_TREE: treePage = h[1]; break;
            case HIT_RECIPE: if (h[1] >= 0 && h[1] < recipeTargets.size()) toRecipe(recipeTargets.get(h[1])); return;
            case HIT_GEAR: if (h[1] >= 0 && h[1] < gearHits.size()) showPreview(gearHits.get(h[1])); return;
            case HIT_RESPEC: if (snap.valid && guide != null && isFollowed(guide)) { BuildStore.put(getContext(), snap.name, snap.cls, new BuildModel.Follow(guide.id, guide.version, guide.phases.get(h[1]).id)); lastTarget = Integer.MIN_VALUE; } break;
            case HIT_URL: if (guide != null && h[1] < guide.sources.size()) {
                try { getContext().startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(guide.sources.get(h[1]).url))); } catch (RuntimeException e) { note = e.getClass().getSimpleName(); } }
                break;
            default:
        }
        invalidate();
    }

    private void flip(int step) {
        if (level == 0) { int to = Math.max(0, Math.min(listPages - 1, listPage + step)); if (to != listPage) { listPage = to; invalidate(); } }
        else { int to = Math.max(0, Math.min(pages - 1, page + step)); if (to != page) { page = to; invalidate(); } }
    }

    @Override public boolean onTouchEvent(MotionEvent e) {
        int action = e.getActionMasked();
        if (preview != null) {
            downHit = -1;
            if (action == MotionEvent.ACTION_DOWN) { previewDown = true; previewX = e.getX(); previewY = e.getY(); }
            else if (action == MotionEvent.ACTION_CANCEL) previewDown = false;
            else if (action == MotionEvent.ACTION_UP && previewDown) {
                previewDown = false;

                int step = GearPreview.touch(e.getX() - previewX, e.getY() - previewY, getWidth() * 0.06f);
                if (step == GearPreview.TOUCH_CLOSE) closePreview(); else if (tooltip.flip(step)) invalidate();
            }
            return true;
        }
        if (action == MotionEvent.ACTION_DOWN) { downX = e.getX(); downY = e.getY(); downHit = hitAt(downX, downY); focus = focusKind = -1; }
        else if (action == MotionEvent.ACTION_UP) {
            float dx = e.getX() - downX, dy = e.getY() - downY, swipe = getWidth() * 0.06f;
            if (Math.abs(dx) > swipe && Math.abs(dx) > Math.abs(dy) * 2) flip(dx < 0 ? 1 : -1);
            else if (downHit >= 0 && downHit == hitAt(e.getX(), e.getY()) && downHit < hits.size()) act(hits.get(downHit));
            downHit = -1;
        } else if (action == MotionEvent.ACTION_CANCEL) downHit = -1;
        return true;
    }


    private void move(int dx, int dy) {
        if (hitRects.isEmpty()) return;
        if (focus < 0 || focus >= hitRects.size()) { setFocus(0); return; }
        RectF f = focusRects.get(focus);
        float fx = f.centerX(), fy = f.centerY();
        int best = -1; float bestScore = Float.MAX_VALUE;
        for (int i = 0; i < hitRects.size(); i++) {
            if (i == focus) continue;
            RectF o = focusRects.get(i);
            float ox = o.centerX() - fx, oy = o.centerY() - fy;



            float along = dx != 0 ? ox * dx : oy * dy, across = dx != 0 ? Math.max(0, Math.max(o.top - fy, fy - o.bottom)) : Math.max(0, Math.max(o.left - fx, fx - o.right));
            if (along <= 1) continue;



            boolean beam = dx != 0 ? o.top < f.bottom && o.bottom > f.top : o.left < f.right && o.right > f.left;
            float score = along + across * 2.5f + (beam ? 0 : getWidth() + getHeight());
            if (score < bestScore) { bestScore = score; best = i; }
        }
        if (best >= 0) setFocus(best);
    }

    private void setFocus(int i) { focus = i; focusKind = hits.get(i)[0]; focusWhich = hits.get(i)[1]; invalidate(); }

    private void cycleTab(int step) {
        if (level == 0) { cls = (cls + step + 8) % 8; listPage = 0; }
        else { tab = (tab + step + 6) % 6; page = 0; }
        focus = focusKind = -1;
        invalidate();
    }






    @Override public boolean padOpen() { return isOpen(); }
    @Override public void onPad(int cmd) {
        if (preview != null) {
            if (cmd == PadNav.A || cmd == PadNav.B || cmd == PadNav.MENU) closePreview();
            else if ((cmd == PadNav.LEFT || cmd == PadNav.RIGHT) && tooltip.flip(cmd == PadNav.LEFT ? -1 : 1)) invalidate();
            return;
        }
        switch (cmd) {
            case PadNav.B: case PadNav.MENU: back(); return;
            case PadNav.LEFT: move(-1, 0); return;
            case PadNav.RIGHT: move(1, 0); return;
            case PadNav.UP: move(0, -1); return;
            case PadNav.DOWN: move(0, 1); return;
            case PadNav.A: if (focus >= 0 && focus < hits.size()) act(hits.get(focus)); else move(0, 1); return;
            case PadNav.L1: cycleTab(-1); return;
            case PadNav.R1: cycleTab(1); return;
            case PadNav.L2: flip(-1); return;
            case PadNav.R2: flip(1); return;
            case PadNav.Y:
                if (level == 1) toggleFollow(guide);
                else if (focus >= 0 && focus < hits.size() && (hits.get(focus)[0] == HIT_CARD || hits.get(focus)[0] == HIT_FOLLOW)) { List<BuildModel.Guide> l = data.ofClass(cls); int i = hits.get(focus)[1]; if (i >= 0 && i < l.size()) toggleFollow(l.get(i)); }
                invalidate(); return;
            default:
        }
    }



    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }
    @Override public void setVisibility(int v) { super.setVisibility(v); qa(); }
    private void qa() { TestHooks.overlay(this, "build_book", !isOpen() ? "closed" : preview != null ? "preview" : "open"); }


    private void showPreview(BuildModel.Gear x) {
        preview = x;
        previewCard = GearPreview.of(x, names(), de);
        previewLines = null; previewRaw = null; previewDrawn = -1; previewDown = false;
        previewText.reset();
        tooltip.reset();
        removeCallbacks(previewPoll);
        qa();
        invalidate();
    }

    private void dropPreview() { preview = null; previewCard = null; previewLines = null; previewRaw = null; previewText.reset(); removeCallbacks(previewPoll); }
    private void closePreview() { dropPreview(); qa(); invalidate(); }



    private byte[] askHost() {
        GearPreview.Card c = previewCard;
        return c == null || c.kind < 0 || c.row < 0 ? new byte[0] : DmActivity.nativeItemText(c.kind, c.row, c.grp);
    }

    private void pollPreview() {
        if (preview == null || !isOpen()) return;
        long now = android.os.SystemClock.uptimeMillis();
        int state = previewText.state(previewCard, now, this::askHost);
        if (state != previewDrawn) invalidate(); else schedulePreviewPoll(state, now);
    }

    private void schedulePreviewPoll(int state, long now) {
        removeCallbacks(previewPoll);
        long next = previewText.nextPoll(state, now);
        if (next == RecipePreview.NEXT_FRAME) postOnAnimation(previewPoll);
        else if (next > 0) postDelayed(previewPoll, next);
    }

    private void textObserved(Canvas c, String value, float x, float y, Paint textPaint) {
        if (shaded && headShadow != null) {
            int ink = textPaint.getColor();
            textPaint.setColor(headShadow.argb);
            c.drawText(value, x + shadeDx, y + shadeDy, textPaint);
            textPaint.setColor(ink);
        }
        c.drawText(value, x, y, textPaint);
        if (BuildConfig.DM_QA_BUILD) TestHooks.text(c, value, x, y, textPaint);
    }



    private void drawPreview(Canvas c) {
        long now = android.os.SystemClock.uptimeMillis();
        int state = previewText.state(previewCard, now, this::askHost);
        previewDrawn = state;
        schedulePreviewPoll(state, now);
        if (state == RecipePreview.WAIT) return;
        if (BuildConfig.DM_QA_BUILD) TestHooks.foreground();
        String raw = previewText.raw(previewCard);
        if (previewLines == null || raw != previewRaw) {
            previewRaw = raw;
            List<GearPreview.Line> game = GearPreview.gameLines(raw, previewCard);
            previewLines = GearPreview.lines(previewCard, game);


            if (BuildConfig.DEBUG) AppArt.log("build preview: " + preview.kind + ":" + preview.key + (preview.baseItem.isEmpty() ? "" : "/" + preview.baseItem) + (preview.baseType.isEmpty() ? "" : "/" + preview.baseType)
                + " state " + state + " lines " + previewLines.size() + " game " + game.size() + " lang " + GameLang.index() + " | " + (previewLines.size() > 1 ? previewLines.get(1).text : "") + " | " + (game.isEmpty() ? "" : game.get(0).text));
        }
        tooltip.draw(c, getWidth(), getHeight(), previewCard.img.isEmpty() ? null : picture("items/" + previewCard.img), previewLines);
    }
}
