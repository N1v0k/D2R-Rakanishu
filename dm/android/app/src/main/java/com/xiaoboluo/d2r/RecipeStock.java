package com.xiaoboluo.d2r;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;


final class RecipeStock {
    static final int INVENTORY = 0, CUBE = 1, STASH = 2, OTHER_PAGE = 3, SHARED = 4, STACKS = 5, POUCH = 6, VAULT = 7;
    static final int ETHEREAL = 1, RUNEWORD = 2, UNKNOWN = 4, NO_WORD = 8;
    static final int HEAD = 20, RECORD = 24, BYTES = HEAD + 1024 * RECORD;

    static final class Entry {
        String code;
        int qty, place, quality, sockets, filled, flags, row, id, x, y, unit;
        Entry(String code, int qty, int place, int quality, int sockets, int filled, int flags, int row) {
            this.code = code; this.qty = qty; this.place = place; this.quality = quality; this.sockets = sockets; this.filled = filled; this.flags = flags; this.row = row;
        }
    }

    final List<Entry> entries = new ArrayList<>();
    int dataVersion, difficulty, page, pages, seen, gen, stale;
    boolean ladderFree, truncated, near, stashOpen;
    int insert;


    static RecipeStock parse(ByteBuffer b, int bytes) {
        if (bytes < HEAD) return null;
        b.order(ByteOrder.LITTLE_ENDIAN);
        int gen = b.getInt(0), n = b.getInt(4);
        if (gen == 0 || n < 0 || bytes != HEAD + n * RECORD) return null;
        RecipeStock s = new RecipeStock();
        s.gen = gen;
        s.dataVersion = b.get(8) & 255;
        s.difficulty = b.get(9) & 255;
        s.ladderFree = b.get(10) != 0;
        s.truncated = b.get(11) != 0;
        s.page = b.get(12) & 255;
        s.pages = b.get(13) & 255;
        s.seen = b.get(14) & 255;
        s.stale = b.get(15) & 255;
        s.near = b.get(16) != 0;
        s.stashOpen = b.get(17) != 0;
        s.insert = b.get(18) & 255;
        byte[] code = new byte[4];
        for (int i = 0; i < n; i++) {
            int at = HEAD + i * RECORD;
            for (int k = 0; k < 4; k++) code[k] = b.get(at + k);
            Entry e = new Entry(new String(code, StandardCharsets.ISO_8859_1).trim(), b.getShort(at + 4) & 0xffff, b.get(at + 6) & 255, b.get(at + 7) & 255,
                b.get(at + 8) & 255, b.get(at + 9) & 255, b.get(at + 10) & 255, b.getShort(at + 12) & 0xffff);
            e.x = b.get(at + 14) & 255;
            e.y = b.get(at + 15) & 255;
            e.id = b.getInt(at + 16);
            e.unit = b.getInt(at + 20);
            s.entries.add(e);
        }
        return s;
    }

    boolean holds(String code) {
        for (Entry e : entries) if (code.equals(e.code)) return true;
        return false;
    }
}
