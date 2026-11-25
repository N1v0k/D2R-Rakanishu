package com.xiaoboluo.d2r;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.Process;
import android.os.SystemClock;
import android.util.AtomicFile;
import android.util.Base64;
import android.view.View;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import org.json.JSONArray;
import org.json.JSONObject;

final class UiObservation {
    private static final class Row {
        String text;
        Paint paint;
        Matrix matrix;
        float x, y;
        int[] bounds;
    }
    private static final class Hit {
        int kind, index;
        float[] bounds;
    }
    private static final class Snapshot {
        String page;
        long revision, stamp;
        File directory;
        ArrayList<Row> rows = new ArrayList<>();
        ArrayList<Hit> hits = new ArrayList<>();
    }
    private static final ConcurrentHashMap<String, Snapshot> pending = new ConcurrentHashMap<>();
    private static Snapshot collecting;
    private static long sequence;
    private static final RectF bounds = new RectF();
    private static final Matrix matrix = new Matrix();

    static {
        Executors.newSingleThreadScheduledExecutor().scheduleWithFixedDelay(UiObservation::write, 0, 50, TimeUnit.MILLISECONDS);
    }

    static void begin(String name) {
        collecting = new Snapshot();
        collecting.page = name;
        collecting.revision = ++sequence;
    }

    static void text(Canvas canvas, CharSequence value, float x, float y, Paint paint) {
        if (collecting == null || value.length() == 0) return;
        String text = value.toString();
        float width = paint.measureText(text);
        float left = x - (paint.getTextAlign() == Paint.Align.CENTER ? width / 2 : paint.getTextAlign() == Paint.Align.RIGHT ? width : 0);
        bounds.set(left, y + paint.ascent(), left + width, y + paint.descent());
        canvas.getMatrix(matrix);
        matrix.mapRect(bounds);
        Row row = new Row();
        row.text = text;
        row.paint = new Paint(paint);
        row.matrix = new Matrix(matrix);
        row.x = x; row.y = y;
        row.bounds = new int[]{(int)Math.floor(bounds.left), (int)Math.floor(bounds.top), (int)Math.ceil(bounds.right), (int)Math.ceil(bounds.bottom)};
        collecting.rows.add(row);
    }

    static void hit(RectF rect, int kind, int index) {
        if (collecting == null) return;
        Hit hit = new Hit();
        hit.kind = kind;
        hit.index = index;
        hit.bounds = new float[]{rect.left, rect.top, rect.right, rect.bottom};
        collecting.hits.add(hit);
    }

    static void foreground() {
        if (collecting != null) { collecting.rows.clear(); collecting.hits.clear(); }
    }

    static void end(View view) {
        Snapshot snapshot = collecting;
        collecting = null;
        if (snapshot == null) return;
        snapshot.directory = view.getContext().getFilesDir();
        snapshot.stamp = SystemClock.elapsedRealtime();

        pending.put(snapshot.page, snapshot);
    }

    private static JSONObject rowJson(Row row) throws Exception {
        int w = row.bounds[2] - row.bounds[0], h = row.bounds[3] - row.bounds[1];
        if (w <= 0 || h <= 0 || w > 8192 || h > 2048) throw new IllegalArgumentException("Invalid text bounds");
        Bitmap mask = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(mask);
        canvas.translate(-row.bounds[0], -row.bounds[1]);
        canvas.concat(row.matrix);
        int color = row.paint.getColor();
        row.paint.setColor(0xFFFFFFFF);
        canvas.drawText(row.text, row.x, row.y, row.paint);
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        mask.compress(Bitmap.CompressFormat.PNG, 100, bytes);
        mask.recycle();
        JSONArray characters = new JSONArray();
        float width = row.paint.measureText(row.text);
        float left = row.x - (row.paint.getTextAlign() == Paint.Align.CENTER ? width / 2 : row.paint.getTextAlign() == Paint.Align.RIGHT ? width : 0);
        for (int i = 0; i < row.text.length();) {
            int next = i + Character.charCount(row.text.codePointAt(i));
            RectF cell = new RectF(left + row.paint.measureText(row.text, 0, i), row.y + row.paint.ascent(),
                left + row.paint.measureText(row.text, 0, next), row.y + row.paint.descent());
            row.matrix.mapRect(cell);
            characters.put(new JSONArray(new int[]{(int)Math.floor(cell.left), (int)Math.floor(cell.top),
                (int)Math.ceil(cell.right), (int)Math.ceil(cell.bottom)}));
            i = next;
        }
        return new JSONObject().put("text", row.text).put("color", String.format("#%08X", color))
            .put("bounds", new JSONArray(row.bounds)).put("characters", characters)
            .put("glyphs", Base64.encodeToString(bytes.toByteArray(), Base64.NO_WRAP));
    }

    private static void write() {
        for (String page : pending.keySet()) {
            Snapshot snapshot = pending.remove(page);
            if (snapshot == null) continue;
            AtomicFile file = new AtomicFile(new File(snapshot.directory, "dm/ui." + page + ".json"));
            FileOutputStream stream = null;
            try {
                JSONArray rows = new JSONArray(), hits = new JSONArray();
                for (Row row : snapshot.rows) rows.put(rowJson(row));
                for (Hit hit : snapshot.hits) hits.put(new JSONObject().put("kind", hit.kind)
                    .put("index", hit.index).put("bounds", new JSONArray(hit.bounds)));
                JSONObject root = new JSONObject().put("pid", Process.myPid()).put("mono_ms", snapshot.stamp)
                    .put("revision", snapshot.revision).put("page", snapshot.page).put("rows", rows).put("hits", hits);
                stream = file.startWrite();
                stream.write(root.toString().getBytes(StandardCharsets.UTF_8));
                file.finishWrite(stream);
            } catch (Exception e) {
                if (stream != null) file.failWrite(stream);
                AppArt.log("qa.ui.write: " + e);
            }
        }
    }
}
