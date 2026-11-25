package com.xiaoboluo.d2r;








final class StatusText {
    private StatusText() {}


    static void split(String template, String formatted, String[] out) {
        String line = formatted == null ? "" : formatted;
        int at = template == null ? -1 : template.indexOf("%d");
        String label = at > 0 ? template.substring(0, at) : "";
        if (label.trim().isEmpty() || !line.startsWith(label)) { out[0] = ""; out[1] = line; return; }
        out[0] = label; out[1] = line.substring(label.length());
    }
}
