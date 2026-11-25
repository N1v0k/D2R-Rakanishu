package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.TimeZone;

















final class FeatsPage extends View implements PadPage, QuickPage {
    private static final int HIT_CLOSE = 0, HIT_GROUP = 1, HIT_PREV = 2, HIT_NEXT = 3, HIT_VIEW = 6;
    private static final int QA_ROW = 4, QA_BAR = 5;
    private static final int FOOT_LINES = 2;
    private static final float MARGIN = 20, LINE = 1.25f;
    private static final int[] GROUP_NAMES = { R.string.dmAndroidFeatsGroup1, R.string.dmAndroidFeatsGroup2, R.string.dmAndroidFeatsGroup3,
        R.string.dmAndroidFeatsGroup4, R.string.dmAndroidFeatsGroup5, R.string.dmAndroidFeatsGroup6, R.string.dmAndroidFeatsGroup7, R.string.dmAndroidFeatsGroup8 };

    private final AppArt art;
    private final GameStyle style;
    private final Runnable freeInput;
    private final Paint pic = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG), grey = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);
    private final Paint shade = new Paint(), barBack = new Paint(), barFill = new Paint(), barFrame = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final GameText body, bodyOn, dim, nameDone, nameOpen, day, count, total, legend;
    private final RectF frame = new RectF(), r = new RectF(), cell = new RectF();
    private final List<RectF> hitRects = new ArrayList<>();
    private final List<int[]> hits = new ArrayList<>();
    private final float[] pagePrev = new float[4], pageNext = new float[4];
    private final GameLang.Watch lang = new GameLang.Watch();
    private FeatsModel model;
    private boolean paused, padUsed, downOutside, everyone;
    private int group, page, pages = 1, downHit = -1;


    private static final class Line {
        final FeatsModel.Row row; final String name, cond, right; final float fill; final Bitmap picture;
        FeatsLayout.Fit nameFit = FeatsLayout.NONE, condFit = FeatsLayout.NONE, rightFit = FeatsLayout.NONE;
        Line(FeatsModel.Row row, String name, String cond, String right, float fill, Bitmap picture) {
            this.row = row; this.name = name; this.cond = cond; this.right = right; this.fill = fill; this.picture = picture;
        }
    }
    private List<Integer> groups = new ArrayList<>();
    private final List<List<Line>> lines = new ArrayList<>();
    private final List<String> groupNames = new ArrayList<>(), groupCounts = new ArrayList<>();
    private String title = "", note = "", hero = "", totalText = "", closing = "";
    private final String[] viewNames = { "", "" };
    private final List<String> foot = new ArrayList<>();
    private boolean noteOnly;
    private boolean noteDim;
    private float totalFill;

    private final FeatsLayout.Stamp laid = new FeatsLayout.Stamp();
    private int texts;
    private List<String> noteLines = new ArrayList<>();
    private final List<String> footLines = new ArrayList<>();
    private final FeatsLayout.Fit[] viewFits = { FeatsLayout.NONE, FeatsLayout.NONE };
    private FeatsLayout.Fit heroFit = FeatsLayout.NONE, closingFit = FeatsLayout.NONE;
    private final List<FeatsLayout.Fit> groupFits = new ArrayList<>();
    private int[] perOf = new int[0], pagesOf = new int[0];
    private String[][] pagerTexts = new String[0][];
    private float[][] pagerWidths = new float[0][];
    private final List<String> widest = new ArrayList<>();
    private float prevW, nextW, rowPitch, nameH, condH, pagerH, textMax, rightW, rowsBot;

    FeatsPage(Context context, AppArt art, GameStyle style, Runnable freeInput) {
        super(context);
        this.art = art;
        this.style = style;
        this.freeInput = freeInput;
        body = style.text(GameStyleData.BODY);
        bodyOn = style.text(GameStyleData.BODY).colour("FontColorLightGold");
        dim = style.text(GameStyleData.BODY).colour("FontColorLightGray");
        nameDone = style.text(GameStyleData.BUTTON_LABEL).colour("FontColorGoldYellow");
        nameOpen = style.text(GameStyleData.BUTTON_LABEL).colour("FontColorLightGray");
        day = style.text(GameStyleData.BUTTON_LABEL).colour("FontColorPartyGreen");
        count = style.text(GameStyleData.BUTTON_LABEL).colour("FontColorWhite");
        total = style.text(GameStyleData.BUTTON_LABEL).colour("FontColorGoldYellow");
        legend = style.text(GameStyleData.LEGEND);
        shade.setColor(0xA0000000);
        ColorMatrix m = new ColorMatrix(); m.setSaturation(0);
        grey.setColorFilter(new ColorMatrixColorFilter(m)); grey.setAlpha(150);
        barBack.setColor(0xBF000000);
        barFill.setColor(KillStreak.barColour(style.colour("FontColorRed")));
        barFrame.setColor(style.colour("FontColorDarkGold")); barFrame.setStyle(Paint.Style.STROKE);
        setVisibility(GONE);
    }


    @Override public boolean available() { return DmActivity.nativeInGame() != 0; }
    @Override public void takePause() { if (!paused) { paused = true; DmActivity.nativePause(DmActivity.PAUSE_FEATS, 1); } }
    @Override public void open() {
        DmActivity.syncLang();
        takePause();
        model = read();
        group = 0; page = 0; padUsed = false; everyone = false;
        prepare();
        setVisibility(VISIBLE);
        freeInput.run();
        invalidate();
    }
    boolean isOpen() { return getVisibility() == VISIBLE; }
    void close() {
        if (paused) { paused = false; DmActivity.nativePause(DmActivity.PAUSE_FEATS, 0); }
        setVisibility(GONE);
    }

    void stop() { if (isOpen()) close(); }


    private static FeatsModel read() {
        long t0 = System.nanoTime();
        byte[] b = DmActivity.nativeFeats();
        if (b == null) { AppArt.log("feats page: no list from the host"); return null; }
        AppArt.log("feats page: list of " + b.length + " bytes read in " + (System.nanoTime() - t0) / 1000 + " us");
        try { return FeatsModel.parse(new String(b, StandardCharsets.UTF_8)); }
        catch (IllegalArgumentException e) { AppArt.log("feats page: " + e + (e.getCause() != null ? " (" + e.getCause() + ")" : "")); return null; }
    }

    private String s(int id) { return AppText.get(getContext(), id); }


    private void view(boolean all) {
        if (model == null || all == everyone) return;
        List<Integer> before = groups; int at = group;
        everyone = all;
        prepare();
        group = FeatsLayout.sameGroup(before, at, groups); page = 0;
        invalidate();
    }


    private void prepare() {
        texts++;
        title = s(R.string.dmAndroidFeatsMenu);
        viewNames[0] = s(R.string.dmAndroidFeatsHero); viewNames[1] = s(R.string.dmAndroidFeatsAll);
        groups = new ArrayList<>(); lines.clear(); groupNames.clear(); groupCounts.clear(); foot.clear(); widest.clear();
        hero = ""; totalText = ""; closing = ""; totalFill = 0;
        FeatsModel m = model == null ? null : everyone ? model.all : model;
        noteOnly = m == null; noteDim = m != null && m.unavailable;
        note = m == null ? s(R.string.dmAndroidFeatsFailed) : m.unavailable ? s(R.string.dmAndroidFeatsOthersUnavailable)
            : m.mode == FeatsModel.MODE_NEWER ? s(R.string.dmAndroidFeatsNewer) : m.mode == FeatsModel.MODE_UNREAD ? s(R.string.dmAndroidFeatsUnread) : "";
        if (!note.isEmpty()) return;
        Context c = getContext();
        Locale language = AppText.res(c).getConfiguration().getLocales().get(0);
        InvPictures inv = art.invPictures();
        groups = m.groups();
        for (int g : groups) {
            List<Line> out = new ArrayList<>();
            for (FeatsModel.Row row : m.rows(g)) {
                String[] names = m.gameNames(row), t = FeatTexts.of(c, row.id, row.arg, names[0], names[1]);
                if (t == null) { AppArt.log("feats page: no text for row " + row.id); continue; }
                long have = m.progress(row);
                String most = FeatsModel.widestCount(row, language);
                if (most != null) widest.add(most);
                String right = row.entered ? FeatsModel.date(row.when, language, TimeZone.getDefault()) : have >= 0 ? FeatsModel.progressText(have, row.arg, language) : "";
                int unique = FeatTexts.pictureUnique(row.id);
                String code = FeatTexts.picture(row.id), pictureName = code == null || inv == null ? null : inv.name(code, unique >= 0 ? 7 : 0, unique, -1);
                out.add(new Line(row, t[0], t[1], right, have >= 0 ? FeatsModel.fill(have, row.arg) : -1, pictureName == null ? null : art.image("inv/" + pictureName)));
            }
            lines.add(out);
            groupNames.add(g >= 1 && g <= GROUP_NAMES.length ? s(GROUP_NAMES[g - 1]) : "");
            int[] n = m.count(g);
            groupCounts.add(n[0] > 0 ? n[0] + " / " + n[1] : "");
        }
        int[] all = m.count(0);
        hero = m.hero; totalText = all[0] + " / " + all[1]; totalFill = all[1] > 0 ? (float) all[0] / all[1] : 0;
        if (m.account) {
            if (m.complete()) closing = s(R.string.dmAndroidFeatsDone);
            if (m.incomplete) foot.add(s(R.string.dmAndroidFeatsIncomplete));
            if (!m.skipped.isEmpty()) foot.add(AppText.get(c, R.string.dmAndroidFeatsSkipped, String.join(", ", m.skipped)));
            if (!m.newer.isEmpty()) foot.add(AppText.get(c, R.string.dmAndroidFeatsSkippedNewer, String.join(", ", m.newer)));
            if (!m.waiting.isEmpty()) foot.add(AppText.get(c, R.string.dmAndroidFeatsWaiting, String.join(", ", m.waiting)));
        }
        group = FeatsLayout.clamp(group, groups.size());
    }


    private float gamePx;
    private void hit(RectF at, int kind, int which) {
        hitRects.add(new RectF(at)); hits.add(new int[]{kind, which});
        if (BuildConfig.DM_QA_BUILD) TestHooks.hit(at, kind, which);
    }
    private float base(GameText t, float top, float bottom, float k) { Paint p = t.paint(k); return GameStyleGeometry.centreBaseline(top, bottom, p.ascent(), p.descent()); }
    private static FeatsLayout.Fit fit(GameText t, String s, float max, float k) { return FeatsLayout.fit(s, max, k, t::fitTo, t::measure); }
    private void draw(Canvas c, GameText t, FeatsLayout.Fit f, float x, float top, float bottom, Paint.Align a) {
        if (!f.text.isEmpty()) t.draw(c, f.text, x, base(t, top, bottom, f.k), f.k, a, 255);
    }


    private void layout(GameStyleGeometry.Panel p) {
        float g = 2 * p.k, k = g, m = MARGIN * p.s, top = p.contentT + m, bot = p.contentB - m, in = 14 * g;
        float split = p.contentL + FeatsLayout.LIST * (p.contentR - p.contentL);
        GameText noteText = noteDim ? dim : body;
        noteLines = note.isEmpty() ? new ArrayList<>() : EffectLayout.wrap(note, noteOnly ? (p.contentR - p.contentL) * 0.7f : (p.contentR - split) * 0.8f, t -> noteText.measure(t, k));

        float x0 = p.contentL + m, x1 = split, left = x0 + in + 10 * g, right = x1 - in - 10 * g;
        float half = ((x1 - in) - (x0 + in) - 8 * g) / 2;
        for (int i = 0; i < 2; i++) viewFits[i] = fit(i == (everyone ? 1 : 0) ? bodyOn : body, viewNames[i], half - 24 * g, k);
        heroFit = fit(body, hero, right - left - total.measure(totalText, k) - 16 * g, k);
        closingFit = fit(total, closing, right - left, k);
        groupFits.clear();
        for (int i = 0; i < groups.size(); i++) {
            String n = groupCounts.get(i);
            float w = n.isEmpty() ? 0 : dim.measure(n, k) + 16 * g;
            groupFits.add(fit(body, groupNames.get(i), (x1 - in) - (x0 + in) - 48 * g - w, k));
        }

        x0 = split + 24 * g; x1 = p.contentR - m;
        nameH = nameDone.size(k) * LINE; condH = body.size(k) * LINE; rowPitch = nameH + condH + 30 * g; pagerH = 44 * g;
        float rowL = x0 + in, rowR = x1 - in, side = rowPitch - 6 * g - 16 * g, tx = rowL + 10 * g + side + 18 * g, rx = rowR - 20 * g;


        rightW = 0.28f * (rowR - rowL);
        for (String s : widest) rightW = Math.max(rightW, count.measure(s, k) + 4 * g);
        rightW = Math.min(rightW, 0.45f * (rowR - rowL)); textMax = rx - rightW - 16 * g - tx;
        prevW = legend.measure("‹", k); nextW = legend.measure("›", k);

        footLines.clear();
        List<String> wrapped = new ArrayList<>();
        float footW = rowR - rowL - 20 * g, footLineH = dim.size(k) * LINE;
        for (String f : foot) wrapped.addAll(FeatsLayout.cap(EffectLayout.wrap(f, footW, t -> dim.measure(t, k)), FOOT_LINES));
        for (String l : FeatsLayout.cap(wrapped, (int) ((bot - in - (top + in + rowPitch + pagerH)) / footLineH))) footLines.add(fit(dim, l, footW, k).text);
        rowsBot = footLines.isEmpty() ? bot : bot - in - footLines.size() * footLineH - 8 * g;
        int n = lines.size();
        perOf = new int[n]; pagesOf = new int[n]; pagerTexts = new String[n][]; pagerWidths = new float[n][];
        for (int gi = 0; gi < n; gi++) {
            List<Line> rows = lines.get(gi);
            int per = FeatsLayout.perPage(top + in, rowsBot - in, rowPitch);
            if (rows.size() > per) per = FeatsLayout.perPage(top + in, rowsBot - pagerH, rowPitch);
            perOf[gi] = per; pagesOf[gi] = FeatsLayout.pages(rows.size(), per);
            pagerTexts[gi] = new String[pagesOf[gi]]; pagerWidths[gi] = new float[pagesOf[gi]];
            for (int pg = 0; pg < pagesOf[gi]; pg++) {
                pagerTexts[gi][pg] = "‹   " + (pg + 1) + " / " + pagesOf[gi] + "   ›";
                pagerWidths[gi][pg] = legend.measure(pagerTexts[gi][pg], k);
            }
            for (Line l : rows) {
                boolean on = l.row.entered;
                l.nameFit = fit(on ? nameDone : nameOpen, l.name, textMax, k);
                l.condFit = fit(on ? body : dim, l.cond, textMax, k);
                l.rightFit = on ? fit(day, l.right, rightW, k) : l.fill >= 0 ? fit(count, l.right, rightW, k) : FeatsLayout.NONE;
            }
        }
    }

    @Override protected void onDraw(Canvas c) {
        if (BuildConfig.DM_QA_BUILD) TestHooks.begin(this, "feats");
        try {
            if (lang.changed()) { style.refresh(); prepare(); }
            hitRects.clear(); hits.clear();
            style.backdrop(c);
            GameStyleGeometry.Panel p = style.panel(getWidth(), getHeight());
            if (laid.stale(texts, style.generation(), getWidth(), getHeight())) layout(p);
            style.panelFrame(c, p);
            frame.set(p.frameL, p.frameT, p.frameR, p.frameB);
            style.panelTab(c, p, 1, 0, title, true);
            style.closeX(c, p);
            r.set(p.hitCloseL, p.hitCloseT, p.hitCloseR, p.hitCloseB); hit(r, HIT_CLOSE, 0);
            gamePx = 2 * p.k;
            float g = gamePx, k = g, m = MARGIN * p.s, top = p.contentT + m, bot = p.contentB - m;
            if (noteOnly) drawNote(c, p.contentL, p.contentR, p.contentT, p.contentB, k);
            else {
                float split = p.contentL + FeatsLayout.LIST * (p.contentR - p.contentL);
                drawGroups(c, p.contentL + m, split, top, bot, g, k);
                if (!note.isEmpty()) { column(c, split + 24 * g, top, p.contentR - m, bot, g); drawNote(c, split + 24 * g, p.contentR - m, top, bot, k); }
                else drawRows(c, split + 24 * g, p.contentR - m, top, bot, g, k);
            }
            if (PadInput.isPadMode()) style.footLegend(c, p, getWidth(), legendParts());
        } finally { if (BuildConfig.DM_QA_BUILD) TestHooks.end(this); }
    }


    private void drawNote(Canvas c, float x0, float x1, float y0, float y1, float k) {
        GameText t = noteDim ? dim : body;
        float lineH = t.size(k) * LINE, y = (y0 + y1) / 2 - noteLines.size() * lineH / 2;
        for (String line : noteLines) { t.draw(c, line, (x0 + x1) / 2, base(t, y, y + lineH, k), k, Paint.Align.CENTER, 255); y += lineH; }
    }

    private void column(Canvas c, float x0, float y0, float x1, float y1, float g) {
        r.set(x0, y0, x1, y1);
        c.drawRect(r, shade);
        art.rim(c, "build_inset", new RectF(r), 8, g, pic);
    }


    private void bar(Canvas c, RectF at, float fill, float g) {
        c.drawRect(at, barBack);
        if (fill > 0) c.drawRect(at.left, at.top, at.left + at.width() * fill, at.bottom, barFill);
        barFrame.setStrokeWidth(Math.max(1f, g));
        c.drawRect(at, barFrame);
    }



    private void drawGroups(Canvas c, float x0, float x1, float top, float bot, float g, float k) {
        column(c, x0, top, x1, bot, g);
        float in = 14 * g, lineH = body.size(k) * LINE, left = x0 + in + 10 * g, right = x1 - in - 10 * g, y = top + in;
        float switchH = Math.max(lineH + 24 * g, style.minTap()), half = ((x1 - in) - (x0 + in) - 8 * g) / 2;
        for (int i = 0; i < 2; i++) {
            float sx = x0 + in + i * (half + 8 * g);
            r.set(sx, y, sx + half, y + switchH);
            boolean on = i == (everyone ? 1 : 0);
            if (on) art.row(c, "build_row", new RectF(r), 18, pic); else art.nine(c, "build_inset", new RectF(r), 8, g, pic);
            hit(r, HIT_VIEW, i);
            draw(c, on ? bodyOn : body, viewFits[i], r.centerX(), r.top, r.bottom, Paint.Align.CENTER);
        }
        if (note.isEmpty()) {
            y += switchH + 18 * g;
            draw(c, body, heroFit, left, y, y + lineH, Paint.Align.LEFT);
            total.draw(c, totalText, right, base(total, y, y + lineH, k), k, Paint.Align.RIGHT, 255);
            y += lineH + 8 * g;
            r.set(left, y, right, y + 10 * g); bar(c, r, totalFill, g);
            y += 10 * g;
            if (!closingFit.text.isEmpty()) { y += 8 * g; draw(c, total, closingFit, (left + right) / 2, y, y + lineH, Paint.Align.CENTER); y += lineH; }
            y += 24 * g;
        }
        float pitch = FeatsLayout.groupPitch(Math.max(lineH + 40 * g, style.minTap()), bot - in - y, groups.size(), lineH + 22 * g);
        for (int i = 0; i < groups.size(); i++) {
            r.set(x0 + in, y, x1 - in, y + pitch - 6 * g);
            boolean on = i == group;
            if (on) art.row(c, "build_row", new RectF(r), 18, pic); else art.nine(c, "build_inset", new RectF(r), 8, g, pic);
            hit(r, HIT_GROUP, i);
            String n = groupCounts.get(i);
            draw(c, on ? bodyOn : body, groupFits.get(i), r.left + 24 * g, r.top, r.bottom, Paint.Align.LEFT);
            if (!n.isEmpty()) dim.draw(c, n, r.right - 24 * g, base(dim, r.top, r.bottom, k), k, Paint.Align.RIGHT, 255);
            if (on && padUsed) { RectF f = new RectF(r); f.inset(-6 * g, -6 * g); art.focus(c, f, g * 0.9f, pic); }
            y += pitch;
        }
    }


    private void drawRows(Canvas c, float x0, float x1, float top, float bot, float g, float k) {
        column(c, x0, top, x1, bot, g);
        if (groups.isEmpty()) return;
        List<Line> rows = lines.get(group);
        float in = 14 * g, pitch = rowPitch;
        int per = perOf[group];
        pages = pagesOf[group];
        page = FeatsLayout.clamp(page, pages);
        float y = top + in;
        for (int i = page * per; i < Math.min(rows.size(), (page + 1) * per); i++) {
            Line l = rows.get(i);
            r.set(x0 + in, y, x1 - in, y + pitch - 6 * g);
            art.nine(c, "build_inset", new RectF(r), 8, g, pic);
            if (BuildConfig.DM_QA_BUILD) TestHooks.hit(r, QA_ROW, l.row.id);
            float side = r.height() - 16 * g;
            cell.set(r.left + 10 * g, r.top + 8 * g, r.left + 10 * g + side, r.top + 8 * g + side);
            picture(c, l.picture, cell, l.row.entered);
            float tx = cell.right + 18 * g, rx = r.right - 20 * g;
            float mid = r.top + 8 * g + nameH;
            draw(c, l.row.entered ? nameDone : nameOpen, l.nameFit, tx, r.top + 8 * g, mid, Paint.Align.LEFT);
            draw(c, l.row.entered ? body : dim, l.condFit, tx, mid, mid + condH, Paint.Align.LEFT);
            if (l.row.entered) draw(c, day, l.rightFit, rx, r.top, r.bottom, Paint.Align.RIGHT);
            else if (l.fill >= 0) {
                draw(c, count, l.rightFit, rx, r.top + 8 * g, mid, Paint.Align.RIGHT);
                cell.set(rx - rightW, mid + condH * 0.3f, rx, mid + condH * 0.3f + 12 * g);
                bar(c, cell, l.fill, g);
                if (BuildConfig.DM_QA_BUILD) TestHooks.hit(cell, QA_BAR, l.row.id);
            }
            y += pitch;
        }
        if (pages > 1) pager(c, x0, x1, rowsBot - pagerH, rowsBot - 6 * g, k);
        float footH = dim.size(k) * LINE, fy = bot - in - footLines.size() * footH;
        for (String l : footLines) { dim.draw(c, l, x0 + in + 10 * g, base(dim, fy, fy + footH, k), k, Paint.Align.LEFT, 255); fy += footH; }
    }


    private void picture(Canvas c, Bitmap b, RectF in, boolean entered) {
        if (b == null || b.getWidth() <= 0 || b.getHeight() <= 0) return;
        float s = Math.min(in.width() / b.getWidth(), in.height() / b.getHeight()), w = b.getWidth() * s, h = b.getHeight() * s;
        RectF at = new RectF(in.centerX() - w / 2, in.centerY() - h / 2, in.centerX() + w / 2, in.centerY() + h / 2);
        c.drawBitmap(b, null, at, entered ? pic : grey);
    }


    private void pager(Canvas c, float x0, float x1, float y0, float y1, float k) {
        String t = pagerTexts[group][page];
        legend.draw(c, t, (x0 + x1) / 2, base(legend, y0, y1, k), k, Paint.Align.CENTER, 255);
        GameStyleGeometry.pagerHits(x0, x1, y0, y1, pagerWidths[group][page], prevW, nextW, style.minTap(), y0, getWidth(), getHeight(), pagePrev, pageNext);
        r.set(pagePrev[0], pagePrev[1], pagePrev[2], pagePrev[3]); hit(r, HIT_PREV, 0);
        r.set(pageNext[0], pageNext[1], pageNext[2], pageNext[3]); hit(r, HIT_NEXT, 0);
    }


    private Object[] legendParts() {
        int gl = PadInput.glyph();
        Object[] close = {art.glyph(gl, "b"), s(R.string.dmAndroidBack), null, art.glyph(gl, "start"), s(R.string.dmAndroidQuickClose)};
        if (model == null) return close;
        Object[] out = new Object[close.length + 3];
        out[0] = art.glyph(gl, "y"); out[1] = viewNames[everyone ? 0 : 1]; out[2] = null;
        System.arraycopy(close, 0, out, 3, close.length);
        return out;
    }


    private int hitAt(float x, float y) { for (int i = hitRects.size() - 1; i >= 0; i--) if (hitRects.get(i).contains(x, y)) return i; return -1; }

    private void act(int[] h) {
        switch (h[0]) {
            case HIT_CLOSE: close(); return;
            case HIT_GROUP: if (h[1] != group) { group = h[1]; page = 0; } break;
            case HIT_VIEW: view(h[1] == 1); break;
            case HIT_PREV: page = FeatsLayout.clamp(page - 1, pages); break;
            case HIT_NEXT: page = FeatsLayout.clamp(page + 1, pages); break;
            default: break;
        }
        invalidate();
    }


    @Override public boolean onTouchEvent(MotionEvent e) {
        int a = e.getActionMasked();
        float x = e.getX(), y = e.getY();
        if (a == MotionEvent.ACTION_DOWN) { downHit = hitAt(x, y); downOutside = downHit < 0 && !frame.contains(x, y); if (padUsed) { padUsed = false; invalidate(); } }
        else if (a == MotionEvent.ACTION_UP) {
            int at = hitAt(x, y);
            if (downHit >= 0 && downHit == at) act(hits.get(at));
            else if (downOutside && at < 0 && !frame.contains(x, y)) close();
            downHit = -1; downOutside = false;
        } else if (a == MotionEvent.ACTION_CANCEL) { downHit = -1; downOutside = false; }
        return true;
    }


    @Override public boolean padOpen() { return isOpen(); }
    @Override public void back() { close(); }
    @Override public void onPad(int cmd) {
        if (cmd == PadNav.B || cmd == PadNav.MENU) { close(); return; }
        if (cmd == PadNav.Y) { padUsed = true; view(!everyone); return; }
        int[] to = FeatsLayout.pad(cmd, group, groups.size(), page, pages);
        group = to[0]; page = to[1]; padUsed = true;
        invalidate();
    }

    @Override protected void onVisibilityChanged(View changed, int visibility) {
        super.onVisibilityChanged(changed, visibility);
        if (changed == this) DmActivity.padPagesChanged();
    }
    @Override public void setVisibility(int v) { super.setVisibility(v); TestHooks.overlay(this, "feats", isOpen() ? "open" : "closed"); }
}
