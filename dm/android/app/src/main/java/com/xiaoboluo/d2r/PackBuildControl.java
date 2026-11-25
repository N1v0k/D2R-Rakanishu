package com.xiaoboluo.d2r;





final class PackBuildControl {
    interface Platform {
        void hold();
        void release();
        void stop();
        void spawn(Runnable r);
    }
    interface Build {
        void run();
        void cancel();
    }

    private final Platform platform;
    private Build running, draining, queued;

    PackBuildControl(Platform platform) { this.platform = platform; }


    synchronized boolean start(Build b) {
        if (running != null || queued != null) return false;
        if (draining != null) { queued = b; return true; }
        launch(b);
        return true;
    }

    private void launch(Build b) {
        running = b;
        platform.hold();
        platform.spawn(() -> {
            try { b.run(); }
            finally { finished(b); }
        });
    }

    synchronized boolean busy() { return running != null || queued != null; }


    synchronized void timeout() {
        if (running == null) return;
        running.cancel();
        draining = running;
        queued = null;
        end();
    }

    private synchronized void finished(Build b) {
        if (draining == b) {
            draining = null;
            Build next = queued;
            queued = null;
            if (next != null) launch(next);
            return;
        }
        if (running == b) end();
    }

    private void end() {
        running = null;
        platform.release();
        platform.stop();
    }
}
