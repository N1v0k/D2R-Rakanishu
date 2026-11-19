package com.xiaoboluo.d2r;










final class DamageMotionTest {
    private static int checks, fails;
    private static void check(boolean ok, String what) { checks++; if (!ok) { fails++; System.out.println("FAIL: " + what); } }

    static final float H = 1216, W = 2688, SIZE = H * DamageMotion.TEXT;
    static final int F = DamageMotion.FIELDS;

    static final class Hit { int seq, unit, flags, value; float born, x = 0.5f, y = 0.4f, drift; }


    static int frame(java.util.List<Hit> hits, float now, float life, float[] d) {
        int n = 0;
        for (Hit h : hits) {
            float age = (now - h.born) / life;
            if (age < 0 || age >= 1 || n == DamageMotion.MAX) continue;
            int o = n++ * F;
            d[o] = h.x + h.drift * (now - h.born) / life; d[o + 1] = h.y; d[o + 2] = age; d[o + 3] = (h.unit & 31) | h.flags << 5; d[o + 4] = h.seq; d[o + 5] = 1; d[o + 6] = h.value; d[o + 7] = h.unit;
        }
        return n;
    }
    static float width(float[] d, int i) {
        int digits = String.valueOf((int) d[i * F + 6]).length();
        return 0.62f * digits * SIZE * DamageMotion.scale(DamageMotion.flags(d, i));
    }
    static void boxes(DamageMotion m, float[] d, int n, float[][] b) {
        float[] p = new float[3];
        for (int i = 0; i < n; i++) {
            m.place(d, i, SIZE, W, H, p);
            float w = width(d, i) * p[2] / (SIZE * DamageMotion.scale(DamageMotion.flags(d, i))), cx = p[0], base = p[1];
            b[i][0] = cx - w / 2; b[i][1] = base - 0.72f * p[2]; b[i][2] = cx + w / 2; b[i][3] = base;
        }
    }
    static float overlap(float[] a, float[] b) {
        float w = Math.min(a[2], b[2]) - Math.max(a[0], b[0]), h = Math.min(a[3], b[3]) - Math.max(a[1], b[1]);
        if (w <= 0 || h <= 0) return 0;
        float sa = (a[2] - a[0]) * (a[3] - a[1]), sb = (b[2] - b[0]) * (b[3] - b[1]);
        return w * h / Math.min(sa, sb);
    }



    static void boss(float every, int count, int burst, String what) {
        java.util.List<Hit> hits = new java.util.ArrayList<>();
        java.util.Random rnd = new java.util.Random(7);
        int seq = 100;
        for (int k = 0; k < count; k++) for (int b = 0; b < burst; b++) {
            Hit h = new Hit(); h.seq = seq++; h.unit = 24; h.born = k * every;
            h.value = 1 + rnd.nextInt(rnd.nextInt(3) == 0 ? 999 : 30);
            h.flags = rnd.nextInt(6) == 0 ? DamageMotion.F_CRIT : rnd.nextInt(8) == 0 ? DamageMotion.F_DOT : 0;
            hits.add(h);
        }
        DamageMotion m = new DamageMotion();
        float[] d = new float[DamageMotion.MAX * F], w = new float[DamageMotion.MAX];
        float[][] b = new float[DamageMotion.MAX][4];
        float worst = 0, minX = 1e9f, maxX = -1e9f;
        java.util.Map<Integer, float[]> last = new java.util.HashMap<>();
        float jump = 0;
        float step = 1f / 60 / 1.1f;
        for (float now = 0; now < count * every + 1; now += step) {
            int n = frame(hits, now, 1, d);
            for (int i = 0; i < n; i++) w[i] = width(d, i);
            m.track(d, n, w, SIZE, W, H);
            boxes(m, d, n, b);
            for (int i = 0; i < n; i++) {
                float cx = (b[i][0] + b[i][2]) / 2;
                minX = Math.min(minX, cx); maxX = Math.max(maxX, cx);
                float[] prev = last.get(DamageMotion.seq(d, i));
                if (prev != null) jump = Math.max(jump, Math.max(Math.abs(cx - prev[0]), Math.abs(b[i][3] - prev[1])));
                last.put(DamageMotion.seq(d, i), new float[]{cx, b[i][3]});
                for (int j = 0; j < i; j++) {
                    if (DamageMotion.age(d, i) >= DamageMotion.FADE_FROM || DamageMotion.age(d, j) >= DamageMotion.FADE_FROM) continue;
                    worst = Math.max(worst, overlap(b[i], b[j]));
                }
            }
        }
        check(worst < 0.25f, what + ": live numbers overlap at most " + worst);
        check(maxX - minX > 2.5f * SIZE, what + ": numbers fan out sideways (" + (maxX - minX) / SIZE + " text heights)");
        check(jump < 0.12f * SIZE, what + ": no jump between frames (" + jump + " px)");
    }





    static float[] run(java.util.List<Hit> hits) {
        DamageMotion m = new DamageMotion();
        float[] d = new float[DamageMotion.MAX * F], w = new float[DamageMotion.MAX];
        float[][] b = new float[DamageMotion.MAX][4];
        float worst = 0, out = 0, jump = 0, end = 0;
        for (Hit h : hits) end = Math.max(end, h.born + 1);
        java.util.Map<Integer, float[]> last = new java.util.HashMap<>();
        for (float now = 0; now < end; now += 1f / 66) {
            int n = frame(hits, now, 1, d);
            for (int i = 0; i < n; i++) w[i] = width(d, i);
            m.track(d, n, w, SIZE, W, H);
            boxes(m, d, n, b);
            for (int i = 0; i < n; i++) {
                out = Math.max(out, Math.max(Math.max(-b[i][0], b[i][2] - W), Math.max(-b[i][1], b[i][3] - H)));
                float[] prev = last.get(DamageMotion.seq(d, i));
                float cx = (b[i][0] + b[i][2]) / 2;
                if (prev != null) jump = Math.max(jump, Math.max(Math.abs(cx - prev[0]), Math.abs(b[i][3] - prev[1])));
                last.put(DamageMotion.seq(d, i), new float[]{cx, b[i][3]});
                for (int j = 0; j < i; j++) {
                    if (DamageMotion.age(d, i) >= DamageMotion.FADE_FROM || DamageMotion.age(d, j) >= DamageMotion.FADE_FROM) continue;
                    worst = Math.max(worst, overlap(b[i], b[j]));
                }
            }
        }
        return new float[]{worst, out, jump};
    }
    static Hit hit(int seq, int value, int flags, float x, float born) { Hit h = new Hit(); h.seq = seq; h.unit = 24; h.value = value; h.flags = flags; h.x = x; h.born = born; return h; }
    static java.util.List<Hit> list(Hit... h) { return new java.util.ArrayList<>(java.util.Arrays.asList(h)); }

    public static void main(String[] a) {

        float[] d = new float[DamageMotion.MAX * F], w = {60}, p1 = new float[3], p2 = new float[3], p3 = new float[3];
        d[0] = 0.5f; d[1] = 0.4f; d[2] = 0.3f; d[3] = 24; d[4] = 77; d[6] = 12; d[7] = 24;
        DamageMotion m = new DamageMotion(), m2 = new DamageMotion();
        m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p1); m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p2);
        m2.track(d, 1, w, SIZE, W, H); m2.place(d, 0, SIZE, W, H, p3);
        check(p1[0] == p2[0] && p1[1] == p2[1] && p1[0] == p3[0] && p1[1] == p3[1], "same number, same place every frame");

        float top = 0.4f * H, prev = 1, prevStep = 1e9f; boolean mono = true, slows = true;
        for (float t = 0; t <= 1; t += 0.05f) { d[2] = t; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p1); float y = p1[1] - top;
            if (y > prev) mono = false; if (prev != 1 && prev - y > prevStep + 1e-3f) slows = false; if (prev != 1) prevStep = prev - y; prev = y; }
        check(mono && slows, "rise monotonic and slowing");

        d[2] = 0.5f; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p1);
        d[3] = 24 | DamageMotion.F_DOT << 5; d[4] = 78; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p2);
        check(top - p2[1] < (top - p1[1]) * 0.75f && p2[2] < p1[2], "damage over time slower and smaller");

        d[3] = 24 | DamageMotion.F_CRIT << 5; d[4] = 79; d[2] = 0.09f; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p1);
        d[2] = 0.5f; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p2);
        check(Math.abs(p1[2] - SIZE * 1.3f * 1.35f) < 0.01f * SIZE && Math.abs(p2[2] - SIZE * 1.3f) < 1e-3f, "critical pop at the start, then steady 1.3");

        d[3] = 24 | DamageMotion.F_SPLASH << 5; d[4] = 80; d[2] = 0.5f; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p1);
        d[3] = 24 | (DamageMotion.F_SPLASH | DamageMotion.F_CRIT) << 5; d[4] = 81; d[2] = 0.09f; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p2);
        d[2] = 0.5f; m.track(d, 1, w, SIZE, W, H); m.place(d, 0, SIZE, W, H, p3);
        check(p1[2] < SIZE && p3[2] == p1[2] && Math.abs(p2[2] - p1[2] * 1.35f) < 0.01f * SIZE, "splash small; a critical splash small with its pop");
        check(DamageMotion.pop(0, 0.09f) == 1f, "no pop without the crit flag");

        int[] lanes = new int[8]; float[] ages = new float[8]; boolean distinct = true;
        for (int k = 0; k < 4; k++) { int l = DamageMotion.lane(200 + k, false, lanes, ages, k); for (int j = 0; j < k; j++) if (lanes[j] == l) distinct = false;
            for (int j = 0; j < k; j++) ages[j] += 0.1f; lanes[k] = l; ages[k] = 0; }
        check(distinct, "four hits in a row use four lanes");
        check(DamageMotion.lane(204, false, lanes, ages, 4) == lanes[0], "the fifth takes the lane of the oldest");
        check(DamageMotion.lane(205, true, lanes, ages, 4) >= DamageMotion.LANES, "damage over time takes an outer lane of its own");

        boss(0.25f, 24, 1, "boss, a hit every 0.27 s");
        boss(0.12f, 40, 1, "boss, a fast hit every 0.13 s");
        boss(0.4f, 12, 3, "three numbers per frame (burst)");


        float[] r = run(list(hit(8, 8, 0, 0.5f, 0), hit(9, 8888, 0, 0.5f, 0)));
        check(r[0] == 0, "8 and 8888 together do not touch (" + r[0] + ")");
        r = run(list(hit(9, 8, 0, 0.5f, 0), hit(8, 8888, 0, 0.5f, 0)));
        check(r[0] == 0, "8888 and 8 together do not touch (" + r[0] + ")");


        float mixed = 0, mixedJump = 0;
        for (int seed = 1; seed <= 200; seed++) {
            java.util.Random rnd = new java.util.Random(seed);
            java.util.List<Hit> hits = new java.util.ArrayList<>();
            float born = 0; int sq = rnd.nextInt(1000);
            for (int k = 0; k < 6; k++, born += 0.08f + 0.27f * rnd.nextFloat())
                for (int c = 1 + rnd.nextInt(4); c > 0; c--) {
                    int digits = 1 + rnd.nextInt(5), v = (int) Math.pow(10, digits - 1) + rnd.nextInt((int) (9 * Math.pow(10, digits - 1)));
                    hits.add(hit(sq++, v, rnd.nextInt(5) == 0 ? DamageMotion.F_CRIT : rnd.nextInt(6) == 0 ? DamageMotion.F_DOT : 0, 0.5f, born));
                }
            r = run(hits); mixed = Math.max(mixed, r[0]); mixedJump = Math.max(mixedJump, r[2]);
        }
        check(mixed < 0.25f, "mixed-width bursts: live numbers overlap at most " + mixed);
        check(mixedJump < 0.12f * SIZE, "mixed-width bursts: no jump between frames (" + mixedJump + " px)");


        for (float x : new float[]{0.0f, 0.01f, 0.03f, 0.97f, 0.99f, 1.0f}) {
            r = run(list(hit(0, 24, 0, x, 0), hit(1, 24, 0, x, 0)));
            check(r[0] == 0 && r[1] <= 0, "two hits at x=" + x + ": apart (" + r[0] + ") and inside (" + r[1] + " px out)");
            r = run(list(hit(0, 24, 0, x, 0), hit(1, 8888, DamageMotion.F_CRIT, x, 0), hit(2, 7, 0, x, 0), hit(3, 512, 0, x, 0), hit(4, 33, DamageMotion.F_DOT, x, 0)));
            check(r[0] == 0 && r[1] <= 0, "a burst of five at x=" + x + ": apart (" + r[0] + ") and inside (" + r[1] + " px out)");
        }


        java.util.List<Hit> walk = list(hit(0, 24, 0, 0.06f, 0), hit(1, 24, 0, 0.06f, 0), hit(2, 130, 0, 0.06f, 0.1f), hit(3, 9, 0, 0.06f, 0.1f));
        for (Hit h : walk) { h.drift = -0.1f; h.x -= 0.1f * h.born; }
        r = run(walk);
        check(r[0] == 0 && r[1] <= 0.5f && r[2] < 0.12f * SIZE + 0.1f * W / 66, "monster walking to the edge: apart (" + r[0] + "), inside (" + r[1] + " px out), no jump (" + r[2] + " px)");


        for (float y : new float[]{0.95f, 0.99f, 1.0f}) {
            Hit dot = hit(0, 37, DamageMotion.F_DOT, 0.5f, 0), direct = hit(1, 512, 0, 0.5f, 0); dot.y = y; direct.y = y;
            r = run(list(dot));
            check(r[1] <= 0, "damage over time at y=" + y + ": inside (" + r[1] + " px out)");
            r = run(list(dot, direct));
            check(r[0] == 0 && r[1] <= 0, "damage over time and a hit at y=" + y + ": apart (" + r[0] + ") and inside (" + r[1] + " px out)");
        }
        System.out.println("DamageMotionTest: " + checks + " checks, " + fails + " failed");
        if (fails != 0) System.exit(1);
    }
}
