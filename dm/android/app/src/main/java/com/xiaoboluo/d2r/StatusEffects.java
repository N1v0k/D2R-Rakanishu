package com.xiaoboluo.d2r;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import java.io.File;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONObject;




























final class StatusEffects extends View implements Choreographer.FrameCallback {

    static final int SYMBOL = 1 << 16, ROW = 2 << 16, MORE = 3 << 16, CARD = 4 << 16, PREV = 5 << 16, NEXT = 6 << 16, TERM = 7 << 16, KIND = 0xffff0000, STATE = 0xffff;


    interface GlossaryLink { String linkLabel(int stat); String linkTerm(int stat); void openFromCard(int state, int stat, int amount); }
    private static final int MAX_LINKS = 2;
    private GlossaryLink glossary;
    private final List<int[]> links = new ArrayList<>();
    private final List<RectF> linkBoxes = new ArrayList<>();
    void setGlossary(GlossaryLink g) { glossary = g; }
    private static final int POLL_FRAMES = 6, MAX_BYTES = 1 << 20, MAX_EXTRAPOLATE = 10;


    private static final float FILL = 0.86f, FIT = 0.80f;
    private static final long CLOSE_NS = 6_000_000_000L, TICK_NS = 40_000_000L;

    private static final float LINE = 1.3f, CARD_WIDTH = 0.40f;


    private static final int DEBUFF = 0, SHRINE = 1, BUFF = 2, POTION = 3, AURA = 4;

    private static final String WORSE = "FontColorRed";
    private static final class Effect { int state, kind; boolean curse, poison, fill; String name; Bitmap picture; final EffectDetail.Entry entry = new EffectDetail.Entry(); }


    private static final class Line {
        final String text, colour, accentColour; final GameText kind; final float k; final int accent;
        String head, tail; float headW, wholeW;
        Line(String t, GameText g, String c) { this(t, g, c, 0, -1, null); }
        Line(String t, GameText g, String c, float k, int accent, String accentColour) { text = t; kind = g; colour = c; this.k = k; this.accent = accent; this.accentColour = accentColour; }
    }

    private static final Object[][] OWN = {{"Mixed", R.string.dmAndroidFxMixed}, {"Aura", R.string.dmAndroidFxAura}, {"Potion", R.string.dmAndroidFxPotion}, {"Curse", R.string.dmAndroidFxCurse},
        {"Damage", R.string.dmAndroidFxDamage}, {"OnYou", R.string.dmAndroidFxOnYou}, {"OnEnemies", R.string.dmAndroidFxOnEnemies}, {"Party", R.string.dmAndroidFxParty},
        {"Cap", R.string.dmAndroidFxCap}, {"Effective", R.string.dmAndroidFxEffective}, {"CastAt", R.string.dmAndroidFxCastAt}, {"Recast", R.string.dmAndroidFxRecast},
        {"More", R.string.dmAndroidFxMore}, {"NextLevel", R.string.dmAndroidFxNextLevel}, {"Sheet", R.string.dmAndroidFxSheet}, {"NoNumbers", R.string.dmAndroidFxNoNumbers},
        {"EnemyTree", R.string.dmAndroidFxEnemyTree}, {"PerSec", R.string.dmAndroidFxPerSec}, {"Stand", R.string.dmAndroidFxStand}, {"Run", R.string.dmAndroidFxRun},
        {"FullIn", R.string.dmAndroidFxFullIn}, {"ExpBonus", R.string.dmAndroidFxExpBonus}, {"ExpKill", R.string.dmAndroidFxExpKill}, {"PoisonRate", R.string.dmAndroidFxPoisonRate},
        {"PoisonLeft", R.string.dmAndroidFxPoisonLeft}, {"Heals", R.string.dmAndroidFxHeals}, {"SlowEnemies", R.string.dmAndroidFxSlowEnemies},
        {"Thorns", R.string.dmAndroidFxThorns}, {"TakesMore", R.string.dmAndroidFxTakesMore}};

    private final TouchOverlay overlay;
    private final StatusLine statusLine;
    private final AppArt art;
    private final GameStyle style;
    private final GameText plus;
    private final GameText title, sub, body;
    private final RectF cardFill = new RectF();
    private final char[] plusText = new char[8], boxText = new char[32];
    private String minutes;



    private boolean de = GameLang.german();
    private final GameLang.Watch lang = new GameLang.Watch();
    private final int[] words = new int[EffectFormat.WORDS];
    private final float slop;
    private final RectF box = new RectF();


    private final EffectLayout.Cells cells = new EffectLayout.Cells();
    private final float[] cardRect = new float[4];
    private float layoutTop, layoutLeft, reach, below, reachField = -1;


    private final float[] stick = new float[3], keep = new float[3];
    private boolean keepOn;
    private volatile JSONObject data;
    private final HashMap<Integer, Effect> now = new HashMap<>();



    private List<Integer> stable = new ArrayList<>(), order = new ArrayList<>();
    private final HashMap<Integer, Integer> kinds = new HashMap<>();
    private int arranged = -1;
    private int manaBase, frames, gen = -1, tick = Integer.MIN_VALUE, open = -1, page, holds, held = -1, padPointer = -1, padTarget;


    private final EffectDetail.Player player = new EffectDetail.Player();
    private final EffectDetail.Skills skills = new EffectDetail.Skills();


    private final EffectDetail.Combat combatArmor = new EffectDetail.Combat(), combatShrine = new EffectDetail.Combat();
    private EffectDetail.Combat slotCombat(int state) {
        return state == EffectDetail.ARMOR_SHRINE ? combatArmor : state == EffectDetail.COMBAT_SHRINE ? combatShrine : null;
    }
    private final HashMap<Integer, EffectDetail.Info> infos = new HashMap<>();
    private final HashMap<Integer, EffectDetail.Entry> entries = new HashMap<>();
    private final HashMap<String, String> own = new HashMap<>();
    private final EffectDetail.Texts texts = new CardTexts();
    private String arrow = "\u203a";
    private long anchorNs, touchedNs, frameNs;
    private boolean shown, running, listOpen, padArmed, padMoved, styled;
    private float padX, padY;
    private String signature = "";

    private final RectF cardBox = new RectF(), prevBox = new RectF(), nextBox = new RectF();
    private final List<RectF> rowBoxes = new ArrayList<>(), rawSpans = new ArrayList<>();
    private final List<Integer> rowStates = new ArrayList<>(), rowHits = new ArrayList<>();

    private boolean cardDrawn;


    private final List<Line> cardRaw = new ArrayList<>(), cardLines = new ArrayList<>();
    private final List<Float> cardY = new ArrayList<>();
    private final List<Integer> cardOwner = new ArrayList<>();
    private final EffectLayout.CardKey laid = new EffectLayout.CardKey();
    private String cardText = "";
    private boolean cardDirty, cardForce, cardReady;
    private int pages = 1;
    private long builtNs;
    private static final long REBUILD_NS = 200_000_000L;

    StatusEffects(Context context, TouchOverlay overlay, AppArt art, GameStyle style, StatusLine statusLine) {
        super(context);
        this.overlay = overlay;
        this.art = art;
        this.style = style;
        this.statusLine = statusLine;
        slop = ViewConfiguration.get(context).getScaledTouchSlop();
        title = style.text(GameStyleData.TOOLTIP_TITLE); sub = style.text(GameStyleData.TOOLTIP_SUB); body = style.text(GameStyleData.TOOLTIP);
        title.prepare(); sub.prepare(); body.prepare();
        plus = style.text(GameStyleData.ICON_NUMBER).colour("FontColorGoldYellow");
        plus.prepare();
        ownWords();
        File file = new File(context.getFilesDir(), "dm/appart/effects.json");
        new Thread(() -> {
            try { if (file.isFile() && file.length() < MAX_BYTES) data = new JSONObject(new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8)); }
            catch (Exception e) { data = null; }
        }, "effects").start();
    }



    private void ownWords() {
        Resources res = AppText.res(getContext(), de ? GameLang.DE : GameLang.EN);
        for (Object[] o : OWN) own.put((String) o[0], res.getString((Integer) o[1]));
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; Choreographer.getInstance().removeFrameCallback(this); hide(); invalidate(); }
    @Override protected void onDetachedFromWindow() { stop(); super.onDetachedFromWindow(); }


    private void hide() {
        shown = false; close();
        overlay.voidClaims();
        holds = 0; held = -1; padPointer = -1; padArmed = false;
    }

    private void close() { if (open >= 0 || listOpen) { open = -1; listOpen = false; cardDrawn = false; cardReady = false; laid.reset(); prevBox.setEmpty(); nextBox.setEmpty(); invalidate(); } }

    @Override public void doFrame(long ns) {
        if (!running) return;
        frameNs = ns;
        if (lang.changed()) {
            de = GameLang.german();
            style.refresh();
            ownWords(); styled = false; gen = -1; frames = POLL_FRAMES;
            if (open >= 0 || listOpen) cardForce = true;
            invalidate();
        }
        JSONObject d = data;

        boolean active = d != null && overlay.displaysShown(ns) && DmActivity.nativeStatusEffectsOn();
        if (active && !styled) words(d);
        if (active != shown) { if (active) { shown = true; frames = POLL_FRAMES; } else hide(); invalidate(); }
        if (active) {
            layout();

            if (holds == 0 && !stable.isEmpty()) { int cap = capacity(unit()); if (cap != arranged) arrange(cap); }
            if (++frames >= POLL_FRAMES) { frames = 0; poll(d, ns); }
            if ((open >= 0 || listOpen) && ns - touchedNs > CLOSE_NS) close();
            if (open >= 0 || listOpen) prepareCard(ns);
            String sign = signature(ns);
            if (!sign.equals(signature)) { signature = sign; invalidate(); }
        }
        Choreographer.getInstance().postFrameCallback(this);
    }

    private static int kind(String k) { return "debuff".equals(k) ? DEBUFF : "shrine".equals(k) ? SHRINE : "potion".equals(k) ? POTION : "aura".equals(k) ? AURA : BUFF; }


    private void words(JSONObject d) {
        String f = word("minutes");
        minutes = f.contains("%d") && style.covers(GameStyle.EXOCET, f.replace("%d", "99")) ? f : null;
        String a = style.pick(GameStyle.EXOCET, "\u203a", ">");
        if (a != null) arrow = a;
        infos.clear();
        JSONObject states = d.optJSONObject("states");
        for (Iterator<String> it = states == null ? Collections.<String>emptyIterator() : states.keys(); it.hasNext(); ) {
            String key = it.next();
            JSONObject o = states.optJSONObject(key);
            try { if (o != null) infos.put(Integer.parseInt(key), info(o)); } catch (NumberFormatException e) {                                        }
        }
        styled = true;
    }


    private EffectDetail.Info info(JSONObject o) {
        EffectDetail.Info i = new EffectDetail.Info();
        int lang = de ? 0 : 1;
        i.kind = o.optString("kind"); i.name = o.optString(de ? "de" : "en");
        i.curse = o.optInt("curse") != 0; i.poison = o.optInt("poison") != 0; i.party = o.optInt("party") != 0; i.sheet = o.optInt("sheet") != 0;
        JSONArray sh = o.optJSONArray("short");
        if (sh != null) i.shortText = sh.optString(lang);
        JSONArray en = o.optJSONArray("enemy");
        for (int k = 0; en != null && k < en.length(); k++) {
            JSONObject l = en.optJSONObject(k);
            if (l == null) continue;
            EffectDetail.EnemyLine e = new EffectDetail.EnemyLine();
            e.f = l.optInt("f"); e.ca = l.optString("ca"); e.cb = l.optString("cb");
            JSONArray a = l.optJSONArray("a"), b = l.optJSONArray("b");
            e.a = a == null ? null : a.optString(lang); e.b = b == null ? null : b.optString(lang);
            if (e.a != null) i.enemy.add(e);
        }
        JSONArray ow = o.optJSONArray("own");
        for (int k = 0; ow != null && k < ow.length() && i.ownN < i.ownSt.length; k++) {
            JSONObject w = ow.optJSONObject(k);
            if (w == null) continue;
            i.ownSt[i.ownN] = w.optInt("st", -1); i.ownCa[i.ownN] = w.optString("ca"); i.ownN++;
        }
        i.slow = o.optString("slow"); i.ownSkill = o.optInt("skill", -1);
        JSONObject c = o.optJSONObject("calc");
        if (c != null) {
            JSONArray par = c.optJSONArray("par"), calc = c.optJSONArray("calc"), ast = c.optJSONArray("ast"), eminLev = c.optJSONArray("eminlev");
            for (int k = 0; par != null && k < Math.min(par.length(), i.par.length); k++) i.par[k] = par.optInt(k);
            for (int k = 0; calc != null && k < Math.min(calc.length(), i.calc.length); k++) i.calc[k] = calc.optString(k);
            for (int k = 0; ast != null && k < Math.min(ast.length(), i.ast.length); k++) i.ast[k] = ast.optString(k);
            for (int k = 0; eminLev != null && k < Math.min(eminLev.length(), i.eminLev.length); k++) i.eminLev[k] = eminLev.optInt(k);
            if (c.has("emin")) i.emin = c.optInt("emin");
            i.len = c.optString("len"); i.bad = c.optInt("bad") != 0;
            JSONObject refs = c.optJSONObject("refs");
            HashMap<String, Integer> r = new HashMap<>();
            for (Iterator<String> it = refs == null ? Collections.<String>emptyIterator() : refs.keys(); it.hasNext(); ) { String n = it.next(); r.put(n, refs.optInt(n)); }
            i.refs = r;
        } else if (!i.enemy.isEmpty() || i.ownN > 0 || !i.slow.isEmpty()) i.bad = true;
        return i;
    }

    private void poll(JSONObject d, long ns) {
        int w = DmActivity.nativeEffects(words);
        if (w < EffectFormat.HEAD || words[EffectFormat.COUNT] < 0) return;
        if (!EffectFormat.blocks(words, w, player, skills, combatArmor, combatShrine)) return;
        if (open >= 0 || listOpen) cardDirty = true;
        if (words[EffectFormat.TICK] != tick) { tick = words[EffectFormat.TICK]; anchorNs = ns; }
        if (words[EffectFormat.GEN] == gen) return;
        gen = words[EffectFormat.GEN];
        manaBase = words[EffectFormat.MANA];
        now.clear(); entries.clear();
        int p = EffectFormat.HEAD;
        for (int i = 0; i < words[EffectFormat.COUNT] && p + EffectFormat.ENTRY <= w; i++) {
            Effect e = new Effect();
            EffectDetail.Entry x = e.entry;
            e.state = x.state = words[p]; x.skill = words[p + 1]; x.level = words[p + 2]; x.ticks = words[p + 3]; x.lists = words[p + 4];
            int n = words[p + 5]; p += EffectFormat.ENTRY;
            if (n < 0 || p + 3 * n > w) break;
            for (int k = 0; k < n; k++, p += 3) if (k < EffectFormat.STATS) { x.id[k] = words[p]; x.layer[k] = words[p + 1]; x.value[k] = words[p + 2]; x.n = k + 1; }
            EffectDetail.Info o = infos.get(e.state);
            if (o == null) continue;
            e.kind = kind(o.kind);
            e.name = o.name;
            e.curse = o.curse; e.poison = o.poison;
            JSONObject states = d.optJSONObject("states"), so = states == null ? null : states.optJSONObject(Integer.toString(e.state));
            e.fill = so != null && so.optInt("fill") != 0;
            e.picture = art.image("fx/" + e.state);
            now.put(e.state, e); entries.put(e.state, x);
        }
        if (holds == 0) reorder();
        if (open >= 0 && !now.containsKey(open)) close();


        if (open >= 0 || listOpen) invalidate();
    }




    private void reorder() {
        kinds.clear();
        for (Effect e : now.values()) kinds.put(e.state, e.kind);
        stable = EffectOrder.next(stable, kinds);
        arrange(capacity(unit()));
    }
    private void arrange(int capacity) { order = EffectOrder.shown(stable, kinds, DEBUFF, capacity); arranged = capacity; }




    private int ticksNow(Effect e, long ns) {
        if (e.entry.ticks < 0 || e.kind == AURA) return -1;
        long elapsed = Math.min(Math.max(0, (ns - anchorNs) / TICK_NS), MAX_EXTRAPOLATE);
        return (int) Math.max(0, e.entry.ticks - elapsed);
    }



    private String signature(long ns) {
        StringBuilder b = new StringBuilder();
        b.append(Math.round(layoutTop)).append(',').append(Math.round(layoutLeft)).append(',').append(Math.round(reach)).append(',').append(keepOn ? Math.round(keep[0]) : -1).append(',').append(keepOn ? Math.round(keep[1]) : -1).append(',').append(keepOn ? Math.round(keep[2]) : -1).append('|');
        for (Integer s : order) { Effect e = now.get(s); b.append(s).append(':').append(e == null ? -2 : EffectFormat.boxCode(ticksNow(e, ns))).append(' '); }
        return b.toString();
    }


    private float unit() {
        return EffectLayout.unit(getWidth(), getHeight(), getResources().getDisplayMetrics().density, getResources().getConfiguration().smallestScreenWidthDp >= 600);
    }

    private float kTouch() { return GameStyleGeometry.kTouch(getHeight(), unit() / getResources().getDisplayMetrics().density); }
    private float bottom() { return getHeight() * EffectLayout.BOTTOM; }



    private void layout() {
        layoutTop = StatusLine.effectsTop(this);
        layoutLeft = StatusLine.effectsLeft(this);
        float u = unit(), field = 2 * EffectLayout.RADIUS * u;
        if (!overlay.stickThumb(stick) || !(stick[2] > 0)) keepOn = false;
        else if (!overlay.stickActive()) {
            GameRect g = GameRect.ui(); float dx = g.whole() ? 0 : g.x() - getLeft(), dy = g.whole() ? 0 : g.y() - getTop();
            keep[0] = stick[0] + dx; keep[1] = stick[1] + dy; keep[2] = stick[2] + EffectLayout.CARD_GAP * u; keepOn = true; }
        if (field == reachField) return;
        reachField = field;


        reach = EffectLayout.reach(field, 0); below = EffectLayout.below(field, 0);
    }
    private int capacity(float u) { return EffectLayout.capacity(layoutTop, bottom(), u, layoutLeft, EffectLayout.step(reach, u), reach, below, keepOn ? keep : null); }
    private EffectLayout.Cells place(float u) { return EffectLayout.place(cells, order.size(), layoutTop, bottom(), u, layoutLeft, EffectLayout.step(reach, u), reach, below, keepOn ? keep : null); }




    int claim(float x, float y) {
        if (!shown || order.isEmpty()) return 0;
        if (cardDrawn && cardBox.contains(x, y)) {
            if (listOpen) for (int i = 0; i < rowBoxes.size(); i++) if (rowBoxes.get(i).contains(x, y)) return ROW | rowHits.get(i);
            for (int i = 0; open >= 0 && i < linkBoxes.size(); i++) if (linkBoxes.get(i).contains(x, y)) return TERM | i;
            if (prevBox.contains(x, y)) return PREV;
            if (nextBox.contains(x, y)) return NEXT;
            return CARD;
        }
        float u = unit();
        EffectLayout.Cells c = place(u);
        float half = EffectLayout.PITCH * u / 2, r = EffectLayout.RADIUS * u, gap = EffectLayout.COLUMN_GAP * u / 2;
        for (int i = 0; i < c.cells(); i++) {

            float field = c.x[i] - r, left = c.x[i] <= c.x[0] + 1 ? 0 : field - gap;
            if (x >= left && x <= field + reach + gap && y >= c.y[i] - half && y <= Math.max(c.y[i] + half, c.y[i] + r + below)) return i < c.shown ? SYMBOL | order.get(i) : MORE;
        }
        return 0;
    }


    void hold(int target) { holds++; held = (target & KIND) == SYMBOL ? target & STATE : (target & KIND) == MORE ? MORE : -1; touchedNs = frameNs; invalidate(); }
    void release(int target) { if (holds > 0 && --holds == 0) { held = -1; reorder(); } invalidate(); }


    void tap(int target) {
        touchedNs = frameNs;
        int s = target & STATE;
        switch (target & KIND) {
            case SYMBOL: if (!now.containsKey(s)) break; if (open == s && !listOpen) close(); else { open = s; listOpen = false; page = 0; } break;
            case MORE: if (listOpen) close(); else { listOpen = true; open = -1; page = 0; } break;
            case ROW: if (now.containsKey(s)) { open = s; listOpen = false; page = 0; } break;
            case PREV: page = Math.max(0, page - 1); break;
            case NEXT: page++; break;
            case TERM: if (s < links.size() && glossary != null) { int[] l = links.get(s); close(); glossary.openFromCard(l[1], l[2], l[3]); return; } break;
            default: break;
        }
        cardForce = true;
        invalidate();
    }


    void gameTouch() { close(); }


    boolean claimPad(float x, float y) { padArmed = claim(x, y) != 0; return padArmed; }


    void cancelPad() { if (padPointer >= 0) { int t = padTarget; padPointer = -1; release(t); } padArmed = false; }

    @Override public boolean onTouchEvent(MotionEvent e) {
        int a = e.getActionMasked();
        if (a == MotionEvent.ACTION_DOWN) {
            if (padPointer >= 0) release(padTarget);
            padPointer = -1;
            if (!padArmed) return false;
            padArmed = false;
            int t = claim(e.getX(), e.getY());
            if (t == 0) return false;
            padPointer = e.getPointerId(0); padTarget = t; padX = e.getX(); padY = e.getY(); padMoved = false; hold(t);
            return true;
        }
        if (padPointer < 0) return false;
        int index = e.findPointerIndex(padPointer);
        if (index >= 0) { float dx = e.getX(index) - padX, dy = e.getY(index) - padY; if (dx * dx + dy * dy > slop * slop) padMoved = true; }
        if (a == MotionEvent.ACTION_UP || a == MotionEvent.ACTION_POINTER_UP && e.getPointerId(e.getActionIndex()) == padPointer) {
            int t = padTarget; padPointer = -1; release(t);
            if (!padMoved) tap(t);
            return true;
        }
        if (a == MotionEvent.ACTION_CANCEL) { int t = padTarget; padPointer = -1; release(t); }
        return true;
    }



    private String word(String key) {
        JSONObject d = data, w = d == null ? null : d.optJSONObject("words");
        JSONArray a = w == null ? null : w.optJSONArray(key);
        return a == null ? "" : a.optString(de ? 0 : 1);
    }


    private final class CardTexts implements EffectDetail.Texts {
        public String stat(int id, int value) {
            JSONObject d = data, stats = d == null ? null : d.optJSONObject("stats"), f = stats == null ? null : stats.optJSONObject(Integer.toString(id));
            if (f == null) return null;
            int shifted = value >> Math.max(0, Math.min(16, f.optInt("shift")));
            return EffectFormat.apply(f.optString(value < 0 ? (de ? "nde" : "nen") : (de ? "de" : "en")), shifted);
        }
        public String word(String key) { return StatusEffects.this.word(key); }
        public String own(String key) { String w = own.get(key); return w == null ? "" : w; }
        public String skillName(int id) {
            JSONObject d = data, sk = d == null ? null : d.optJSONObject("skills");
            JSONArray a = sk == null ? null : sk.optJSONArray(Integer.toString(id));
            return a == null ? null : a.optString(de ? 0 : 1, null);
        }
        public int penalty(int diff, boolean classic) {
            JSONObject d = data;
            JSONArray p = d == null ? null : d.optJSONArray("penalty"), row = p == null ? null : p.optJSONArray(diff);
            return row == null || row.length() < 2 ? Integer.MIN_VALUE : row.optInt(classic ? 1 : 0, Integer.MIN_VALUE);
        }
        public long threshold(int cls, int level) {
            JSONObject d = data, e = d == null ? null : d.optJSONObject("experience");
            JSONArray a = e == null ? null : e.optJSONArray(Integer.toString(cls));
            return a == null || level < 0 || level >= a.length() ? -1 : a.optLong(level, -1);
        }
        public String arrow() { return arrow; }
        public boolean german() { return de; }
        public int[] group(int stat) {
            JSONObject f = groupOf(stat);
            JSONArray ids = f == null ? null : f.optJSONArray("ids");
            int[] out = new int[ids == null ? 0 : ids.length()];
            for (int i = 0; i < out.length; i++) out[i] = ids.optInt(i, -1);
            return out;
        }
        public String groupText(int stat, int value) {
            JSONObject f = groupOf(stat);
            if (f == null) return null;
            String s = EffectFormat.apply(f.optString(value < 0 ? (de ? "nde" : "nen") : (de ? "de" : "en")), value >> Math.max(0, Math.min(16, f.optInt("shift"))));
            return s == null || s.isEmpty() ? null : s;
        }

        private JSONObject groupOf(int stat) {
            JSONObject d = data;
            JSONArray all = d == null ? null : d.optJSONArray("groups");
            for (int i = 0; all != null && i < all.length(); i++) {
                JSONObject f = all.optJSONObject(i);
                JSONArray ids = f == null ? null : f.optJSONArray("ids");
                for (int k = 0; ids != null && k < ids.length(); k++) if (ids.optInt(k, -1) == stat) return f;
            }
            return null;
        }
        public int[] opSources(int stat) {
            JSONObject d = data, ops = d == null ? null : d.optJSONObject("ops");
            JSONArray a = ops == null ? null : ops.optJSONArray(Integer.toString(stat));
            int[] out = new int[a == null ? 0 : a.length()];
            for (int i = 0; i < out.length; i++) out[i] = a.optInt(i, -1);
            return out;
        }
    }


    private String sub(Effect e, int ticks) {
        List<String> parts = new ArrayList<>();
        String k = e.kind == SHRINE ? word("shrine") : e.kind == BUFF ? word("skill") : e.kind == AURA ? texts.own("Aura") : e.kind == POTION ? texts.own("Potion")
            : e.curse ? texts.own("Curse") : e.poison ? texts.own("Damage") : "";
        if (!k.isEmpty()) parts.add(k);
        if (ticks >= 0) { String left = EffectFormat.apply(word("remaining"), EffectFormat.seconds(ticks)); if (left != null && !left.isEmpty()) parts.add(left); }
        if (e.entry.level > 0 && (e.kind == BUFF || e.kind == AURA || e.kind == DEBUFF)) parts.add(word("level") + " " + e.entry.level);
        return String.join(" · ", parts);
    }




    private void addLinks(Effect e) {
        if (glossary == null) return;
        List<String> seen = new ArrayList<>();
        EffectDetail.Entry x = e.entry;
        for (int k = 0; k < x.n && links.size() < MAX_LINKS; k++) {
            String term = glossary.linkTerm(x.id[k]), label = term == null || seen.contains(term) ? null : glossary.linkLabel(x.id[k]);
            if (label == null) continue;
            seen.add(term);
            links.add(new int[]{cardRaw.size(), e.state, x.id[k], x.value[k]});
            cardRaw.add(new Line(arrow + " " + label, sub, "FontColorGoldYellow"));
        }
    }

    private void buildRaw(long ns) {
        cardRaw.clear(); rowStates.clear(); links.clear();
        Effect e = open >= 0 ? now.get(open) : null;
        if (e != null) {
            int t = ticksNow(e, ns);
            cardRaw.add(new Line(e.name, title, null));
            String s = sub(e, t);
            if (!s.isEmpty()) cardRaw.add(new Line(s, sub, null));
            EffectDetail.Info info = infos.get(e.state);
            e.entry.shown = t;
            if (info != null) for (EffectDetail.Line l : EffectDetail.lines(e.entry, info, entries, infos, player, skills, slotCombat(e.state), manaBase, texts))
                cardRaw.add(new Line(l.text, l.kind == EffectDetail.SUB ? sub : body, l.colour, 0, l.accent, l.accentColour));
            addLinks(e);
        } else if (listOpen) {
            int from = place(unit()).shown;
            List<Integer> rest = order.subList(Math.min(from, order.size()), order.size());
            for (int i = 0; i < rest.size(); i++) {
                Effect x = now.get(rest.get(i));
                if (x == null) continue;
                int t = ticksNow(x, ns);
                cardRaw.add(new Line(x.name + (t >= 0 ? " · " + EffectFormat.seconds(t) : ""), x.kind == SHRINE ? title : body, x.kind == DEBUFF ? WORSE : null));
                rowStates.add(x.state);
            }
        }
        StringBuilder b = new StringBuilder();
        for (Line l : cardRaw) b.append(l.text).append('\u0001').append(l.colour).append('\u0001');
        cardText = b.toString();
    }


    private void prepareCard(long ns) {
        float u = unit();
        if (!(u > 0) || order.isEmpty()) { cardReady = false; return; }
        EffectLayout.Cells cells = place(u);
        float r = EffectLayout.RADIUS * u;
        int anchor = -1;
        for (int i = 0; i < Math.min(cells.cells(), cells.shown); i++) if (order.get(i) == open) anchor = i;
        if (anchor < 0 && cells.more > 0 && (open >= 0 && now.containsKey(open) || listOpen)) anchor = cells.cells() - 1;
        if (anchor < 0 || listOpen && cells.more == 0) { cardReady = false; return; }
        if (cardForce || cardDirty && ns - builtNs >= REBUILD_NS) { cardForce = cardDirty = false; builtNs = ns; buildRaw(ns); }
        float right = 0;
        for (int i = 0; i < cells.cells(); i++) right = Math.max(right, cells.x[i] - r + reach + EffectLayout.CARD_GAP * u);
        float cy = cells.y[anchor], top = layoutTop, bottom = bottom(), keepKey = keepOn ? keep[0] * 31 + keep[1] * 7 + keep[2] : -1;
        if (!laid.changed(cardText, getWidth(), getHeight(), top, bottom, right, cy, style.generation(), page, keepKey) && cardReady) return;
        layoutCard(right, cy, top, bottom);
        laid.page(page);
        cardReady = true;
        invalidate();
    }


    private float cardK() { return GameStyleGeometry.k(getHeight()) * style.data().scale(GameStyleData.PROFILE); }







    private void layoutCard(float left, float cy, float top, float bottom) {
        float k = cardK(), max = getWidth() * CARD_WIDTH;
        style.tooltipBox(0, 0, k, cardFill);
        float padX = cardFill.width() / 2, padY = cardFill.height() / 2;
        max -= 2 * padX;
        List<Line> all = new ArrayList<>();
        List<Integer> owner = new ArrayList<>();
        for (int i = 0; i < cardRaw.size(); i++) {
            Line l = cardRaw.get(i);
            if (!style.covers(GameStyle.EXOCET, l.text)) continue;
            if (l.accent > 0 && l.accent < l.text.length() && l.kind.measure(l.text, l.kind.fitTo(l.text, max, k)) <= max) {

                Line a = new Line(l.text, l.kind, l.colour, l.kind.fitTo(l.text, max, k), l.accent, l.accentColour);
                a.head = l.text.substring(0, l.accent); a.tail = l.text.substring(l.accent); a.headW = l.kind.measure(a.head, a.k); a.wholeW = l.kind.measure(l.text, a.k);
                all.add(a); owner.add(i); continue;
            }
            for (String w : EffectLayout.wrap(l.text, max, x -> l.kind.measure(x, k))) {
                float kk = l.kind.fitTo(w, max, k);
                for (String piece : EffectLayout.breakWord(w, max, x -> l.kind.measure(x, kk))) { all.add(new Line(piece, l.kind, l.colour, kk, -1, null)); owner.add(i); }
            }
        }

        float room = EffectLayout.bottomFor(left, left + max + 2 * padX, bottom, keepOn ? keep : null) - top - 2 * padY, paging = sub.size(k) * LINE;
        int from = 0, to = all.size();
        int[] cut = EffectLayout.cardPages(heights(all, k), room, paging, page);
        pages = cut[0]; page = cut[1]; from = cut[2]; to = cut[3];
        cardLines.clear(); cardY.clear(); cardOwner.clear();
        float width = 0, height = 0;
        for (int i = from; i < to; i++) { Line l = all.get(i); cardLines.add(l); cardOwner.add(owner.get(i)); width = Math.max(width, l.kind.measure(l.text, l.k)); height += l.kind.size(k) * LINE; }
        if (pages > 1) {
            Line nav = new Line("<   " + (page + 1) + " / " + pages + "   >", sub, null, k, -1, null);
            cardLines.add(nav); cardOwner.add(-1); width = Math.max(width, sub.measure(nav.text, k)); height += paging;
        }
        width = Math.min(width, max);
        float[] r = cardRect;
        EffectLayout.card(left, cy, width + 2 * padX, height + 2 * padY, top, EffectLayout.bottomFor(left, left + width + 2 * padX, bottom, keepOn ? keep : null), r);
        cardBox.set(r[0], r[1], r[2], r[3]);
        float y = r[1] + padY;
        rawSpans.clear();
        for (int i = 0; i < cardRaw.size(); i++) rawSpans.add(new RectF(r[0], 0, r[2], 0));
        prevBox.setEmpty(); nextBox.setEmpty();
        for (int i = 0; i < cardLines.size(); i++) {
            Line l = cardLines.get(i);
            float h = l.kind.size(k) * LINE;
            cardY.add(y);
            int o = cardOwner.get(i);
            if (o >= 0) { RectF span = rawSpans.get(o); if (span.height() == 0) span.top = y; span.bottom = y + h; }
            else { float mid = (r[0] + r[2]) / 2; prevBox.set(r[0], y, mid, r[3]); nextBox.set(mid, y, r[2], r[3]); }
            y += h;
        }
        rowBoxes.clear(); rowHits.clear(); linkBoxes.clear();
        for (int[] l : links) linkBoxes.add(l[0] < rawSpans.size() && rawSpans.get(l[0]).height() > 0 ? new RectF(rawSpans.get(l[0])) : new RectF());
        for (int i = 0; listOpen && i < rowStates.size() && i < rawSpans.size(); i++) if (rawSpans.get(i).height() > 0) { rowBoxes.add(new RectF(rawSpans.get(i))); rowHits.add(rowStates.get(i)); }
    }


    private float[] heights(List<Line> lines, float k) { float[] h = new float[lines.size()]; for (int i = 0; i < h.length; i++) h[i] = lines.get(i).kind.size(k) * LINE; return h; }


    private void drawCard(Canvas c) {
        float k = cardK();
        cardFill.set(cardBox);
        style.tooltip(c, cardFill);
        float mid = (cardBox.left + cardBox.right) / 2;
        for (int i = 0; i < cardLines.size(); i++) {
            Line l = cardLines.get(i);
            float y = cardY.get(i), h = l.kind.size(k) * LINE;
            Paint p = l.kind.paint(l.k);
            float base = GameStyleGeometry.centreBaseline(y, y + h, p.ascent(), p.descent());
            if (l.colour != null) l.kind.colour(l.colour); else l.kind.colourArgb(style.colour(colourOf(l.kind)));
            if (l.head == null) { l.kind.draw(c, l.text, mid, base, l.k, Paint.Align.CENTER, 255); continue; }

            float x = mid - l.wholeW / 2;
            l.kind.draw(c, l.head, x, base, l.k, Paint.Align.LEFT, 255);
            l.kind.colour(l.accentColour);
            l.kind.draw(c, l.tail, x + l.headW, base, l.k, Paint.Align.LEFT, 255);
        }
    }


    private String colourOf(GameText kind) { return kind == title ? GameStyleData.TOOLTIP_TITLE.colour : kind == sub ? GameStyleData.TOOLTIP_SUB.colour : GameStyleData.TOOLTIP.colour; }




    private void picture(Canvas c, Effect e, float x, float y, float r, boolean pressed) {
        Bitmap b = e.picture;
        if (b == null) return;
        float half = r * (e.fill ? FILL : FIT), k = 2 * half / Math.max(b.getWidth(), b.getHeight()), w = b.getWidth() * k / 2, h = b.getHeight() * k / 2;
        box.set(x - w, y - h, x + w, y + h);
        style.slotPicture(c, b, box, pressed);
    }

    private void symbol(Canvas c, Effect e, float x, float y, float r, boolean pressed) {
        box.set(x - r, y - r, x + r, y + r);
        style.slot(c, box, pressed);
        picture(c, e, x, y, r, pressed);
        int t = ticksNow(e, frameNs), len = EffectFormat.boxText(t, minutes, boxText);
        if (len > 0) style.cornerNumber(c, boxText, 0, len, x - r, y - r, x + r, y + r, style.counterPx(2 * r, kTouch()), EffectFormat.warn(t) ? "FontColorRed" : "FontColorWhite");
    }


    private void more(Canvas c, int n, float x, float y, float r, boolean pressed) {
        box.set(x - r, y - r, x + r, y + r);
        style.slot(c, box, pressed);
        int len = EffectFormat.plus(n, plusText);
        Paint p = plus.paintPx(r * 0.72f);
        plus.drawPx(c, plusText, 0, len, x, GameStyleGeometry.centreBaseline(y - r, y + r, p.ascent(), p.descent()), r * 0.72f, Paint.Align.CENTER, 255);
    }

    @Override protected void onDraw(Canvas canvas) {
        cardDrawn = false;
        if (!shown || order.isEmpty()) return;
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "status");
        try {
            float u = unit();
            if (!(u > 0)) return;
            if (BuildConfig.DM_QA_BUILD) TestHooks.overlay(this, "status", "visible");
            float r = EffectLayout.RADIUS * u;
            EffectLayout.Cells cells = place(u);
            for (int i = 0; i < cells.cells(); i++) {
                if (i < cells.shown) {
                    int s = order.get(i);
                    Effect e = now.get(s);
                    if (e != null) {
                        symbol(canvas, e, cells.x[i], cells.y[i], r, held == s);
                        if (BuildConfig.DM_QA_BUILD) TestHooks.hit(box, SYMBOL | s, 0);
                    }
                } else more(canvas, cells.more, cells.x[i], cells.y[i], r, held == MORE);
            }
            if (cardReady && (open >= 0 || listOpen)) { drawCard(canvas); cardDrawn = true; }
            if (BuildConfig.DM_QA_BUILD) TestHooks.overlay(this, "status", cardDrawn ? "open" : "visible");
        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }
}
