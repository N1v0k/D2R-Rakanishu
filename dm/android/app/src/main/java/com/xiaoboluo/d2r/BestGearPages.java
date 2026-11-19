package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;




final class BestGearPages {
    static final class Page {
        final int rowFrom, rowTo, lineFrom, lineTo;
        final boolean heading;
        Page(int rowFrom, int rowTo, int lineFrom, int lineTo, boolean heading) {
            this.rowFrom = rowFrom; this.rowTo = rowTo; this.lineFrom = lineFrom; this.lineTo = lineTo; this.heading = heading;
        }
    }

    static List<Page> split(int rows, int lines, float room, float rowH, float headH, float lineH) {
        List<Page> pages = new ArrayList<>();
        int r = 0, l = 0;
        boolean headed = lines == 0;
        while (r < rows || l < lines) {
            float y = 0;
            int rowFrom = r, lineFrom = l;
            boolean heading = false, empty = true;
            while (r < rows && (empty || y + rowH <= room)) { y += rowH; r++; empty = false; }
            if (r == rows && lines > 0) {
                if (!headed && (empty || y + headH + lineH <= room)) { y += headH; heading = true; headed = true; empty = false; }
                if (headed) while (l < lines && (empty || heading && l == lineFrom || y + lineH <= room)) { y += lineH; l++; empty = false; }
            }
            pages.add(new Page(rowFrom, r, lineFrom, l, heading));
        }
        if (pages.isEmpty()) pages.add(new Page(0, 0, 0, 0, false));
        return Collections.unmodifiableList(pages);
    }
}
