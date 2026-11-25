package com.xiaoboluo.d2r;

import android.content.Context;
import java.util.List;



final class StepsOnly implements PackBuildJob.Opened {
    private final PackSteps steps;
    private final Context context;
    StepsOnly(Context context, PackSteps steps) { this.context = context; this.steps = steps; }
    @Override public PackBuildRun.Installer installer(PackJournal journal) { return PackBuildService.complete(context, steps, journal); }
    @Override public List<PackJournal.Unit> units() { return List.of(); }
    @Override public String identity() { return "qa-steps"; }
    @Override public String extract(PackJournal.Unit unit, PackReader.Sink sink) { throw new IllegalStateException("a steps-only build extracts nothing"); }
    @Override public void cancel() {}
    @Override public void close() {}
    @Override public PackSteps steps() { return steps; }
    @Override public long missing(List<PackJournal.Unit> todo) { return 0; }
    @Override public boolean cool(Runnable paused) { return true; }
}
