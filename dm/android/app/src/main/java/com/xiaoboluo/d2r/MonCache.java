package com.xiaoboluo.d2r;

import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.function.ToLongFunction;


final class MonCache<V> {
    private final long limit;
    private final ToLongFunction<V> size;
    private final LinkedHashMap<String, V> values = new LinkedHashMap<>(16, .75f, true);
    private long bytes;

    MonCache(long limit, ToLongFunction<V> size) {
        if (limit <= 0) throw new IllegalArgumentException("limit " + limit);
        this.limit = limit; this.size = size;
    }

    V get(String key) { return values.get(key); }

    void put(String key, V value) {
        V old = values.remove(key);
        if (old != null) bytes -= size.applyAsLong(old);
        long n = size.applyAsLong(value);
        if (n > limit) return;
        values.put(key, value); bytes += n;
        for (Iterator<Map.Entry<String, V>> it = values.entrySet().iterator(); bytes > limit && it.hasNext(); ) {
            Map.Entry<String, V> e = it.next();
            bytes -= size.applyAsLong(e.getValue()); it.remove();
        }
    }

    void clear() { values.clear(); bytes = 0; }
    long bytes() { return bytes; }
    int count() { return values.size(); }
}
