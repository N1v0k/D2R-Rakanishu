package com.xiaoboluo.d2r;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.Log;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import org.json.JSONArray;
import org.json.JSONObject;









final class ExpRail {
    static final float ABOVE_BAR = 2, REF_PER_SOURCE = .95f;
    private static final String TAG = "dm";
    private final File dir;
    private final Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG);
    private final Matrix matrix = new Matrix();

    private final RectF drawn = new RectF();
    private Bitmap rail, fill, bakedRail, bakedFill;
    private int[] edges, target;
    private float fillX, fillY;
    private int bakedW = -1, bakedH = -1;

    ExpRail(File dir) { this.dir = dir; }



    void reload() {
        rail = fill = null; edges = target = null; bakedRail = bakedFill = null; bakedW = bakedH = -1; drawn.setEmpty();
        File meta = new File(dir, "rail.json");
        if (!meta.isFile()) { Log.i(TAG, "exp rail: no appart/exp in the import pack, flat experience bar"); return; }
        try {
            if (meta.length() > 4096) throw new IOException("rail.json too large");
            JSONObject m = new JSONObject(new String(Files.readAllBytes(meta.toPath()), StandardCharsets.UTF_8));
            if (m.getInt("v") != 1) throw new IOException("version " + m.optInt("v"));
            JSONArray a = m.getJSONArray("edges");
            int[] e = new int[a.length()];
            for (int i = 0; i < e.length; i++) e[i] = a.getInt(i);
            float fx = (float) m.getDouble("fill_x"), fy = (float) m.getDouble("fill_y");
            Bitmap r = decode(m.getString("rail")), f = decode(m.getString("fill"));
            if (!ExpRailGeometry.validEdges(e, r.getWidth())) throw new IOException("edges");
            if (r.getHeight() < 8 || r.getHeight() > 256 || f.getHeight() > r.getHeight() || f.getWidth() > r.getWidth()) throw new IOException("sizes");
            if (!(fx >= 0 && fx + f.getWidth() <= r.getWidth() && fy >= 0 && fy + f.getHeight() <= r.getHeight())) throw new IOException("fill position");
            rail = r; fill = f; edges = e; fillX = fx; fillY = fy;
        } catch (Exception | OutOfMemoryError e) {
            Log.w(TAG, "exp rail: appart/exp unusable (" + e + "), flat experience bar");
        }
    }



    private Bitmap decode(String name) throws IOException {
        if (!name.matches("[a-z]{1,16}\\.png")) throw new IOException("name " + name);
        File f = new File(dir, name);
        if (!f.isFile() || f.length() > (1 << 20)) throw new IOException("missing " + name);
        BitmapFactory.Options o = new BitmapFactory.Options();
        o.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(f.getAbsolutePath(), o);
        if (o.outWidth <= 0 || o.outHeight <= 0 || o.outWidth > 2048 || o.outHeight > 256) throw new IOException("size " + o.outWidth + "x" + o.outHeight + " of " + name);
        o.inJustDecodeBounds = false; o.inPreferredConfig = Bitmap.Config.ARGB_8888; o.inScaled = false;
        Bitmap b = BitmapFactory.decodeFile(f.getAbsolutePath(), o);
        if (b == null) throw new IOException("undecodable " + name);
        return b;
    }



    boolean draw(Canvas c, float leftDp, float rightDp, float barTopDp, float u, float density, int permille) {
        hide();
        if (rail == null) return false;
        float topDp = barTopDp - ABOVE_BAR * u;
        int left = Math.round(leftDp * density), top = Math.round(topDp * density);
        int right = Math.round(rightDp * density), bottom = Math.round((topDp + rail.getHeight() * REF_PER_SOURCE * u) * density);
        int w = right - left, h = bottom - top;
        if (w != bakedW || h != bakedH) bake(w, h, REF_PER_SOURCE * u * density);
        if (bakedRail == null) return false;
        c.save();
        c.scale(1 / density, 1 / density);
        c.drawBitmap(bakedRail, left, top, paint);
        float clip = ExpRailGeometry.clip(target, permille);
        if (clip > 0) { c.clipRect(left, top, left + clip, bottom); c.drawBitmap(bakedFill, left, top, paint); }
        c.restore();
        drawn.set(left, top, right, bottom);
        return true;
    }


    boolean covers(float x, float y) { return drawn.contains(x, y); }
    void hide() { drawn.setEmpty(); }




    private void bake(int w, int h, float pxPerSource) {
        bakedW = w; bakedH = h; bakedRail = bakedFill = null;
        int[] t = h >= 4 && h <= 512 && w <= 8192 ? ExpRailGeometry.target(edges, w, pxPerSource) : null;
        if (t == null) { Log.w(TAG, "exp rail: does not fit " + w + "x" + h + ", flat experience bar"); return; }
        Bitmap br, bf;
        try { br = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888); bf = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888); }
        catch (OutOfMemoryError e) { Log.w(TAG, "exp rail: no memory for " + w + "x" + h + ", flat experience bar"); return; }
        Canvas cr = new Canvas(br), cf = new Canvas(bf);
        float ky = (float) h / rail.getHeight();
        for (int i = 0; i < ExpRailGeometry.PIECES; i++) {
            float kx = (float) (t[i + 1] - t[i]) / (edges[i + 1] - edges[i]);
            matrix.setScale(kx, ky); matrix.postTranslate(t[i] - edges[i] * kx, 0);
            cr.save(); cr.clipRect(t[i], 0, t[i + 1], h); cr.drawBitmap(rail, matrix, paint); cr.restore();
            matrix.setScale(kx, ky); matrix.postTranslate(t[i] + (fillX - edges[i]) * kx, fillY * ky);
            cf.save(); cf.clipRect(t[i], 0, t[i + 1], h); cf.drawBitmap(fill, matrix, paint); cf.restore();
        }
        target = t; bakedRail = br; bakedFill = bf;
    }
}
