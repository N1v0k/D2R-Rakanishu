package com.xiaoboluo.d2r;





final class PackBuildState {
    enum Phase { RUNNING, PAUSED, DONE, ERROR, STOPPED }

    static final String FILE = "state";

    final Phase phase;
    final int percent;
    final PackProgress.Step step;
    final String file;
    final String error;
    final String detail;

    PackBuildState(Phase phase, int percent, String file, String error, String detail) { this(phase, percent, null, file, error, detail); }

    PackBuildState(Phase phase, int percent, PackProgress.Step step, String file, String error, String detail) {
        if (percent < 0 || percent > 100) throw new IllegalArgumentException("percent " + percent);
        if ((phase == Phase.ERROR) != (error != null)) throw new IllegalArgumentException("an error state needs exactly an error id");
        if (phase == Phase.DONE && percent != 100) throw new IllegalArgumentException("a finished build shows 100");
        for (String s : new String[]{file, error, detail}) if (s != null && (s.indexOf('\t') >= 0 || s.indexOf('\n') >= 0)) throw new IllegalArgumentException("separator in a state field");
        this.phase = phase; this.percent = percent; this.step = step; this.file = file; this.error = error; this.detail = error == null ? null : detail == null ? "" : detail;
    }


    static PackBuildState failed(int percent, String file, String reader) {
        int tab = reader.indexOf('\t');
        return new PackBuildState(Phase.ERROR, percent, file, tab < 0 ? reader : reader.substring(0, tab), tab < 0 ? "" : reader.substring(tab + 1));
    }

    String format() {
        StringBuilder s = new StringBuilder();
        s.append("phase\t").append(phase.name().toLowerCase(java.util.Locale.ROOT)).append('\n').append("percent\t").append(percent).append('\n');
        if (step != null) s.append("step\t").append(step.name().toLowerCase(java.util.Locale.ROOT)).append('\n');
        if (file != null) s.append("file\t").append(file).append('\n');
        if (error != null) s.append("error\t").append(error).append('\n').append("detail\t").append(detail).append('\n');
        return s.toString();
    }

    static PackBuildState parse(String text) {
        Phase phase = null; int percent = -1; PackProgress.Step step = null; String file = null, error = null, detail = null;
        for (String line : text.split("\n")) {
            if (line.isEmpty()) continue;
            int tab = line.indexOf('\t');
            if (tab < 0) throw new IllegalArgumentException("state line without a value: " + line);
            String k = line.substring(0, tab), v = line.substring(tab + 1);
            switch (k) {
                case "phase" -> phase = Phase.valueOf(v.toUpperCase(java.util.Locale.ROOT));
                case "percent" -> percent = Integer.parseInt(v);
                case "step" -> step = PackProgress.Step.valueOf(v.toUpperCase(java.util.Locale.ROOT));
                case "file" -> file = v;
                case "error" -> error = v;
                case "detail" -> detail = v;
                default -> throw new IllegalArgumentException("unknown state line " + k);
            }
        }
        if (phase == null || percent < 0) throw new IllegalArgumentException("state without phase or percent");
        return new PackBuildState(phase, percent, step, file, error, detail);
    }
}
