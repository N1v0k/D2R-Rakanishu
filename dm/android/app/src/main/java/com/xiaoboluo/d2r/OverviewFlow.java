package com.xiaoboluo.d2r;









final class OverviewFlow {
    private OverviewFlow() {}






    static int[] place(float[] height, int[] section, boolean[] keepNext, float first, float rest) {
        int n = height.length; int[] col = new int[n];
        int c = -1; float used = 0;
        for (int i = 0; i < n; i++) {
            boolean withNext = keepNext[i] && i + 1 < n && section[i + 1] == section[i];
            float need = height[i] + (withNext ? height[i + 1] : 0);
            if (i == 0 || section[i] != section[i - 1]) { c++; used = 0; }
            else if (used > 0 && used + need > (c < 2 ? first : rest)) { c++; used = 0; }
            col[i] = c; used += height[i];
        }
        return col;
    }


    static int pages(int[] col) { return col.length == 0 ? 1 : col[col.length - 1] / 2 + 1; }
}
