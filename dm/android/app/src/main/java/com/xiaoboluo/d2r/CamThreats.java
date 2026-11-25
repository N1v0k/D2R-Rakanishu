package com.xiaoboluo.d2r;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;
import java.io.File;












final class CamThreats extends View implements Choreographer.FrameCallback {
    private static final int[] TIER_COLOUR = { 0xFFE1E1E1, 0xFF6E6EFF, 0xFFC7B377, 0xFFFF5040 };
    private final float[] data = new float[16 * 5], head = new float[4], drawn = new float[4], aim = new float[50], mesh = new float[50];
    private final Paint fill = new Paint(Paint.ANTI_ALIAS_FLAG), edge = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path arrow = new Path();
    private int count;
    private boolean running, headOn, aimOn, ringTried;


    private Bitmap ring;
    private final Paint ringPaint = new Paint(Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG);

    CamThreats(Context context) {
        super(context);
        fill.setStyle(Paint.Style.FILL);
        edge.setStyle(Paint.Style.STROKE);
        edge.setColor(0xC0000000);
    }

    void start() { if (!running) { running = true; Choreographer.getInstance().postFrameCallback(this); } }
    void stop() { running = false; count = 0; headOn = false; aimOn = false; invalidate(); }

    @Override public void doFrame(long ns) {
        if (!running) return;
        int n = DmActivity.nativeCamThreats(data);
        int kind = DmActivity.nativeCamHeading(head);
        boolean h = kind != 0;



        boolean headMoved = h != headOn || h && (Math.abs(head[2] - drawn[2]) > 0.009f || Math.abs(head[0] - drawn[0]) > 0.002f
            || Math.abs(head[1] - drawn[1]) > 0.002f || Math.abs(head[3] - drawn[3]) > 0.002f);


        boolean a = DmActivity.nativeCamAim(aim) == 25;
        if (n != count || n > 0 || headMoved || a || a != aimOn || kind != headKind) { count = n; headOn = h; headKind = kind; aimOn = a; System.arraycopy(head, 0, drawn, 0, 4); invalidate(); }
        Choreographer.getInstance().postFrameCallback(this);
    }

    @Override public boolean onTouchEvent(MotionEvent e) { return false; }

    private int headKind;

    @Override protected void onDraw(Canvas canvas) {

        GameRect g = GameRect.ui(); boolean whole = g.whole();
        float ox = whole ? 0 : g.pictureX(), oy = whole ? 0 : g.pictureY();
        float w = whole ? getWidth() : g.pictureW(), h = whole ? getHeight() : g.pictureH(), uh = whole ? getHeight() : g.uiH(), blink = (System.nanoTime() / 1_000_000L % 400) < 200 ? 1f : 0.55f;
        if (aimOn) {


            if (!ringTried) {
                ringTried = true;
                File f = new File(getContext().getFilesDir(), "dm/overlay/touchart/dm_native_circle.png");
                if (f.isFile() && f.length() <= 4 * 1024 * 1024) ring = BitmapFactory.decodeFile(f.getAbsolutePath());
            }
            for (int i = 0; i < 25; i++) { mesh[2 * i] = ox + aim[2 * i] * w; mesh[2 * i + 1] = oy + aim[2 * i + 1] * h; }
            if (ring != null) {
                ringPaint.setAlpha(230);
                canvas.drawBitmapMesh(ring, 4, 4, mesh, 0, null, 0, ringPaint);
            } else {
                arrow.reset();
                final int[] rim = { 0, 1, 2, 3, 4, 9, 14, 19, 24, 23, 22, 21, 20, 15, 10, 5 };
                arrow.moveTo(mesh[0], mesh[1]);
                for (int k = 1; k < rim.length; k++) arrow.lineTo(mesh[2 * rim[k]], mesh[2 * rim[k] + 1]);
                arrow.close();
                edge.setColor(0xE0D4B060);
                edge.setStrokeWidth(Math.max(1.5f, uh * 0.004f));
                canvas.drawPath(arrow, edge);
                edge.setColor(0xC0000000);
            }
        }
        for (int i = 0; i < count; i++) {
            float x = ox + data[i * 5] * w, y = oy + data[i * 5 + 1] * h, dir = data[i * 5 + 2], urgency = data[i * 5 + 3];
            int tier = Math.max(0, Math.min(3, (int) data[i * 5 + 4]));


            float len = uh * (0.035f + 0.03f * urgency), half = len * 0.6f;
            float alpha = (0.45f + 0.55f * urgency) * (urgency > 0.75f ? blink : 1f);
            fill.setColor(TIER_COLOUR[tier]);
            fill.setAlpha((int) (255 * alpha));
            edge.setStrokeWidth(Math.max(1f, len * 0.08f));
            edge.setAlpha((int) (200 * alpha));
            canvas.save();
            canvas.translate(x, y);
            canvas.rotate((float) Math.toDegrees(dir));
            arrow.reset();
            arrow.moveTo(0, -len);
            arrow.lineTo(half, len * 0.35f);
            arrow.lineTo(0, len * 0.05f);
            arrow.lineTo(-half, len * 0.35f);
            arrow.close();
            canvas.drawPath(arrow, fill);
            canvas.drawPath(arrow, edge);
            canvas.restore();
        }
        if (headOn) {



            float s = drawn[3] * uh, tip = s * 0.115f, base = s * 0.045f, half = s * 0.042f;
            float hx = whole ? drawn[0] * getWidth() : headKind == 1 ? g.x() + drawn[0] * g.w() : ox + drawn[0] * w;
            float hy = whole ? drawn[1] * getHeight() : headKind == 1 ? g.y() + drawn[1] * g.h() : oy + drawn[1] * h;
            fill.setColor(0xFFFFFFFF);
            fill.setAlpha(235);
            edge.setStrokeWidth(Math.max(1f, s * 0.006f));
            edge.setAlpha(210);
            canvas.save();
            canvas.translate(hx, hy);
            canvas.rotate((float) Math.toDegrees(drawn[2]));
            arrow.reset();
            arrow.moveTo(0, -tip);
            arrow.lineTo(half, -base);
            arrow.lineTo(0, -base - (tip - base) * 0.3f);
            arrow.lineTo(-half, -base);
            arrow.close();
            canvas.drawPath(arrow, fill);
            canvas.drawPath(arrow, edge);
            canvas.restore();
        }
    }
}
