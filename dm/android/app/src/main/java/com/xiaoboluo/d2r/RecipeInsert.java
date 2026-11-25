package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;




final class RecipeInsert {
    static final int OK = 0, RUNEWORD = 1, MISSING = 2, CHOOSE = 3, CUBE_FULL = 4, INVENTORY_FULL = 5, FAR = 6, ON_PAGE = 7, NO_CUBE = 8, UNSURE = 9, ELSEWHERE = 10;
    static final int CUBE_W = 3, CUBE_H = 4, INV_W = 10, INV_H = 4, MAX_STEPS = 24, STEP_INTS = 13;
    private static final int PAGE_INV = 0, PAGE_CUBE = 3, PAGE_STASH = 4;
    private static final int SOURCE_GRID = 0, SOURCE_STACK = 1, SOURCE_SHARED = 2, CHECK_ONLY = 3;
    private static final int Q_NORMAL = 2;

    static final class Plan {
        final int outcome, cubeId;
        final String what;
        final int[] steps;
        final int moves;
        Plan(int outcome, String what, int[] steps, int cubeId, int moves) { this.outcome = outcome; this.what = what; this.steps = steps; this.cubeId = cubeId; this.moves = moves; }
    }

    private static final class Unit {
        final RecipeStock.Entry e;
        final int w, h;
        Unit(RecipeStock.Entry e, int w, int h) { this.e = e; this.w = w; this.h = h; }
    }

    private RecipeInsert() {}

    private static Plan fail(int outcome, String what) { return new Plan(outcome, what, null, 0, 0); }

    private static boolean reachable(int place, boolean near) {
        return place == RecipeStock.INVENTORY || place == RecipeStock.CUBE || (near && (place == RecipeStock.STASH || place == RecipeStock.SHARED || place == RecipeStock.STACKS));
    }

    private static int rank(int place) { return place == RecipeStock.CUBE ? 0 : place == RecipeStock.INVENTORY ? 1 : place == RecipeStock.STASH ? 2 : place == RecipeStock.SHARED ? 3 : 4; }

    private static boolean smallGoods(Unit u) { return u.w <= 1 && u.h <= 1 && u.e.quality == Q_NORMAL && u.e.sockets == 0; }

    static Plan plan(RecipeModel.Card card, RecipeModel.Catalog data, RecipeStock stock, RecipeMatch match) {
        if (card.cat == null) return fail(RUNEWORD, null);
        if (stock.truncated || stock.stale != 0 || card.quest) return fail(UNSURE, null);
        RecipeStock.Entry cube = null;
        List<Unit> units = new ArrayList<>(), away = new ArrayList<>();
        for (RecipeStock.Entry e : stock.entries) {
            if (RecipeMatch.CUBE_CODE.equals(e.code)) { if (e.place == RecipeStock.INVENTORY) cube = e; continue; }
            RecipeModel.Item item = data.items.get(e.code);
            List<Unit> into = reachable(e.place, stock.near) ? units : away;
            for (int i = 0; i < e.qty && i < 64; i++) into.add(new Unit(e, item == null ? 1 : item.w, item == null ? 1 : item.h));
        }
        if (cube == null) return fail(NO_CUBE, null);
        units.sort(Comparator.comparingInt(u -> rank(u.e.place)));
        boolean[] used = new boolean[units.size()];
        List<Unit> chosen = new ArrayList<>();
        RecipeModel.Input[] inputs = card.in.clone();
        Arrays.sort(inputs, Comparator.comparingInt(in -> "t".equals(in.kind) ? 1 : 0));
        for (RecipeModel.Input in : inputs) {
            List<Integer> fit = new ArrayList<>();
            int inCube = 0;
            boolean free = true, cubeFree = true;
            for (int i = 0; i < used.length; i++) {
                Unit u = units.get(i);
                if (used[i] || !match.fits(in, u.e)) continue;
                if ((u.e.flags & RecipeStock.UNKNOWN) != 0) return fail(UNSURE, in.name);
                fit.add(i);
                if (u.e.place == RecipeStock.CUBE) { inCube++; cubeFree &= smallGoods(u); } else free &= smallGoods(u);
            }
            if (in.flags.contains("eth") || in.flags.contains("noe")) return fail(UNSURE, in.name);
            if (fit.size() < in.count) {
                int outcome = MISSING;
                for (Unit u : away) if (match.fits(in, u.e)) {
                    int o = u.e.place == RecipeStock.OTHER_PAGE ? ON_PAGE : u.e.place == RecipeStock.POUCH || u.e.place == RecipeStock.VAULT ? ELSEWHERE : FAR;
                    if (outcome == MISSING || o == FAR) outcome = o;
                }
                return fail(outcome, in.name);
            }
            int rest = in.count - Math.min(inCube, in.count);
            if (rest > 0 && !free && fit.size() - inCube > rest) return fail(CHOOSE, in.name);
            if (!cubeFree && inCube > in.count) return fail(CHOOSE, in.name);
            for (int k = 0; k < in.count; k++) { used[fit.get(k)] = true; chosen.add(units.get(fit.get(k))); }
        }
        if (card.num > 0 && chosen.size() != card.num) return fail(UNSURE, null);
        boolean[] cubeCells = new boolean[CUBE_W * CUBE_H], invCells = new boolean[INV_W * INV_H];
        List<int[]> steps = new ArrayList<>();
        for (Unit u : chosen) if (u.e.place == RecipeStock.CUBE) {
            mark(cubeCells, CUBE_W, u.e.x, u.e.y, u.w, u.h);
            steps.add(new int[]{u.e.id, CHECK_ONLY, PAGE_CUBE, u.e.x, u.e.y, 0, PAGE_CUBE, u.e.x, u.e.y, code(u.e.code), u.e.quality, 0, 0});
        }
        int checks = steps.size();
        for (Unit u : units) if (u.e.place == RecipeStock.INVENTORY) mark(invCells, INV_W, u.e.x, u.e.y, u.w, u.h);
        for (RecipeStock.Entry e : stock.entries) if (RecipeMatch.CUBE_CODE.equals(e.code) && e.place == RecipeStock.INVENTORY) { RecipeModel.Item i = data.items.get(e.code); mark(invCells, INV_W, e.x, e.y, i == null ? 2 : i.w, i == null ? 2 : i.h); }
        for (int i = 0; i < used.length; i++) {
            Unit u = units.get(i);
            if (u.e.place != RecipeStock.CUBE || used[i]) continue;
            int[] cell = take(invCells, INV_W, INV_H, u.w, u.h);
            if (cell == null) return fail(INVENTORY_FULL, null);
            steps.add(new int[]{u.e.id, SOURCE_GRID, PAGE_CUBE, u.e.x, u.e.y, 0, PAGE_INV, cell[0], cell[1], code(u.e.code), u.e.quality, 0, 0});
        }
        List<Unit> toPlace = new ArrayList<>(chosen);
        toPlace.sort(Comparator.comparingInt((Unit u) -> -u.w * u.h));
        for (Unit u : toPlace) {
            if (u.e.place == RecipeStock.CUBE) continue;
            int[] cell = take(cubeCells, CUBE_W, CUBE_H, u.w, u.h);
            if (cell == null) return fail(CUBE_FULL, null);
            int source = u.e.place == RecipeStock.STACKS ? SOURCE_STACK : u.e.place == RecipeStock.SHARED ? SOURCE_SHARED : SOURCE_GRID;

            int[] back = source == SOURCE_GRID ? new int[]{0, 0} : take(invCells, INV_W, INV_H, u.w, u.h);
            if (back == null) return fail(INVENTORY_FULL, null);
            steps.add(new int[]{u.e.id, source, u.e.place == RecipeStock.STASH ? PAGE_STASH : PAGE_INV, u.e.x, u.e.y, u.e.unit, PAGE_CUBE, cell[0], cell[1], code(u.e.code), u.e.quality, back[0], back[1]});
        }
        if (steps.size() > MAX_STEPS) return fail(CUBE_FULL, null);
        int[] flat = new int[steps.size() * STEP_INTS];
        for (int i = 0; i < steps.size(); i++) System.arraycopy(steps.get(i), 0, flat, i * STEP_INTS, STEP_INTS);
        return new Plan(OK, null, flat, cube.id, steps.size() - checks);
    }


    private static int code(String s) {
        byte[] b = {' ', ' ', ' ', ' '};
        for (int i = 0; i < 4 && i < s.length(); i++) b[i] = (byte) s.charAt(i);
        return (b[0] & 255) | (b[1] & 255) << 8 | (b[2] & 255) << 16 | (b[3] & 255) << 24;
    }

    private static void mark(boolean[] cells, int w, int x, int y, int iw, int ih) {
        int h = cells.length / w;
        for (int j = y; j < y + Math.max(1, ih) && j < h; j++) for (int i = x; i < x + Math.max(1, iw) && i < w; i++) if (i >= 0 && j >= 0) cells[j * w + i] = true;
    }


    private static int[] take(boolean[] cells, int w, int h, int iw, int ih) {
        iw = Math.max(1, iw);
        ih = Math.max(1, ih);
        for (int y = 0; y + ih <= h; y++) {
            for (int x = 0; x + iw <= w; x++) {
                boolean free = true;
                for (int j = 0; j < ih && free; j++) for (int i = 0; i < iw && free; i++) free = !cells[(y + j) * w + x + i];
                if (free) { mark(cells, w, x, y, iw, ih); return new int[]{x, y}; }
            }
        }
        return null;
    }
}
