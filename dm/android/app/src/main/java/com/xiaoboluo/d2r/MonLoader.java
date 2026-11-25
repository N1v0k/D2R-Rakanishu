package com.xiaoboluo.d2r;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Executor;
import java.util.function.ToLongFunction;







final class MonLoader<V> {
    interface Decoder<V> { V decode(String name, boolean small); }
    interface Listener { void loaded(); void failed(String name, boolean small); }

    private final MonCache<V> cache;
    private final Executor worker, ui;
    private final Decoder<V> decoder;
    private final Listener listener;
    private final Set<String> pending = new HashSet<>(), failed = new HashSet<>();
    private final List<String> shown = new ArrayList<>();
    private volatile Set<String> wanted = Collections.emptySet();
    private int request;

    MonLoader(long limit, ToLongFunction<V> size, Executor worker, Executor ui, Decoder<V> decoder, Listener listener) {
        this.cache = new MonCache<>(limit, size); this.worker = worker; this.ui = ui; this.decoder = decoder; this.listener = listener;
    }


    static String key(String name, boolean small) { return small ? "s/" + name : name; }

    V get(String name, boolean small) { return name == null ? null : cache.get(key(name, small)); }
    boolean failed(String name, boolean small) { return name != null && failed.contains(key(name, small)); }
    int request() { return request; }
    int pending() { return pending.size(); }
    long bytes() { return cache.bytes(); }


    void show(List<String> keys) {
        if (keys.equals(shown)) return;
        shown.clear(); shown.addAll(keys); request++;
        wanted = Collections.unmodifiableSet(new HashSet<>(keys));
        for (String k : keys) ask(k);
    }


    void reset() { shown.clear(); request++; wanted = Collections.emptySet(); pending.clear(); }

    private void ask(String key) {
        if (cache.get(key) != null || failed.contains(key) || !pending.add(key)) return;
        final int number = request;
        final boolean small = key.startsWith("s/");
        final String name = small ? key.substring(2) : key;
        worker.execute(() -> {
            if (!wanted.contains(key)) { ui.execute(() -> dropped(key)); return; }
            V v = decoder.decode(name, small);
            ui.execute(() -> arrived(number, key, name, small, v));
        });
    }

    private void dropped(String key) {
        pending.remove(key);
        if (wanted.contains(key)) ask(key);
    }

    private void arrived(int number, String key, String name, boolean small, V v) {
        pending.remove(key);

        if (v == null) { if (failed.add(key)) listener.failed(name, small); return; }
        if (number != request && !wanted.contains(key)) return;
        cache.put(key, v);
        listener.loaded();
    }
}
