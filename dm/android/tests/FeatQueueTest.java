package com.xiaoboluo.d2r;





final class FeatQueueTest {
    private static int checks;
    private static void check(boolean value, String what) { checks++; if (!value) throw new AssertionError(what); }
    private static final long S = 1_000_000_000L;


    private static int[] ring(int first, int last) {
        int[] h = new int[FeatQueue.N];
        h[FeatQueue.SEQ] = last;
        for (int q = Math.max(1, first); q <= last; q++) { int k = FeatQueue.NOTES + 3 * (q % FeatQueue.KEPT); h[k] = q; h[k + 1] = 100 + q; h[k + 2] = q * 10; }
        return h;
    }

    public static void main(String[] args) {
        FeatQueue q = new FeatQueue();
        q.take(ring(1, 2), FeatQueue.N);
        check(q.at(0, true) == null, "notices from before the first read are not shown");
        q.take(ring(1, 2), FeatQueue.N - 1);
        check(q.at(0, true) == null, "a short copy is ignored");


        q.take(ring(1, 3), FeatQueue.N);
        FeatQueue.Note n = q.at(10 * S, true);
        check(n != null && n.seq == 3 && n.id == 103 && n.arg == 30 && q.alpha() == 255, "the new notice");
        check(q.at(13 * S + S / 2, true) == n && q.alpha() == 255, "fully shown before 4 s");
        check(q.at(14 * S + S / 4, true) == n && q.alpha() > 0 && q.alpha() < 255, "fading after 4 s");
        check(q.at(14 * S + S / 2, true) == null && q.alpha() == 0, "gone after the fade");
        q.take(ring(1, 3), FeatQueue.N);
        check(q.at(15 * S, true) == null, "the same ring again shows nothing");


        q.take(ring(1, 6), FeatQueue.N);
        long t = 20 * S;
        for (int seq = 4; seq <= 6; seq++) {
            FeatQueue.Note m = q.at(t, true);
            check(m != null && m.seq == seq && q.alpha() == 255, "notice " + seq + " in turn");
            check(q.at(t + FeatQueue.SHOW_NS - 1, true) == m, "notice " + seq + " stays its time");
            t += FeatQueue.SHOW_NS + FeatQueue.FADE_NS;
        }
        check(q.at(t, true) == null, "all shown");

        q.take(ring(1, 7), FeatQueue.N);
        FeatQueue.Note a = q.at(t, true);
        q.take(ring(1, 8), FeatQueue.N);
        check(a.seq == 7 && q.at(t + S, true) == a, "the shown one keeps its place");
        check(q.at(t + FeatQueue.SHOW_NS + FeatQueue.FADE_NS, true).seq == 8, "the next one after it");

        q.take(ring(1, 30), FeatQueue.N);
        long u = t + 10 * S;
        FeatQueue.Note b = q.at(u, true);
        check(b != null && b.seq == 23, "the oldest kept one first (" + (b == null ? "none" : b.seq) + ")");
        int count = 1;
        for (u += FeatQueue.SHOW_NS + FeatQueue.FADE_NS; q.at(u, true) != null; u += FeatQueue.SHOW_NS + FeatQueue.FADE_NS) count++;
        check(count == FeatQueue.KEPT, "eight shown: " + count);

        FeatQueue v = new FeatQueue();
        v.take(ring(1, 1), FeatQueue.N);
        v.take(ring(1, 2), FeatQueue.N);
        check(v.at(0, false) == null && v.at(10 * S, false) == null, "a notice that comes while hidden is not shown and not used up");
        FeatQueue.Note w = v.at(11 * S, true);
        check(w != null && w.seq == 2 && v.alpha() == 255, "it shows once visible, from its start");
        check(v.at(14 * S, true) == w, "3 s visible");
        check(v.at(15 * S, false) == null && v.at(60 * S, false) == null, "hidden (a panel, the pause menu): not drawn");
        check(v.at(61 * S, true) == w && v.alpha() == 255, "back: the same notice with the second it had left, fully shown");
        check(v.at(61 * S + 900_000_000L, true) == w && v.alpha() == 255, "still within its 4 s");
        check(v.at(62 * S + S / 4, true) == w && v.alpha() < 255, "then it fades");
        check(v.at(62 * S + S / 2 + 1, true) == null, "and goes after 4.5 s of visible time");
        v.take(ring(1, 3), FeatQueue.N);
        FeatQueue.Note x = v.at(70 * S, true);
        v.hidden();
        check(v.at(500 * S, true) == x && v.alpha() == 255, "after the background it continues where it was");

        FeatQueue l = new FeatQueue();
        l.take(ring(1, 1), FeatQueue.N);
        l.take(ring(1, 2), FeatQueue.N);
        check(l.textsDue(l.at(0, false), false) == null, "nothing to build while hidden");
        FeatQueue.Note y = l.at(S, true);
        check(l.textsDue(y, false) == y, "built when it first shows");
        check(l.textsDue(l.at(2 * S, true), false) == null, "kept while it stays");
        check(l.textsDue(l.at(2 * S + 1, true), true) == y, "built again after a language change");
        check(l.textsDue(l.at(3 * S, false), true) == null, "the language changes while it is hidden (options)");
        check(l.textsDue(l.at(4 * S, false), false) == null, "still hidden");
        check(l.textsDue(l.at(5 * S, true), false) == y, "back: built again in the new language");
        check(l.textsDue(l.at(5 * S + 1, true), false) == null, "then kept");
        System.out.println("FeatQueueTest: " + checks + " checks");
    }
}
