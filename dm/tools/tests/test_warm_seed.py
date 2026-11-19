#!/usr/bin/env python3





import hashlib, os, struct, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, os.path.dirname(HERE))
import d2rm_android as tool

A, B, C = 'a' * 16, 'b' * 16, 'c' * 16


def record(keys, words):

    r = struct.pack('<II', 0x57504c58, len(keys))
    for k in keys: r += struct.pack('<I', len(k)) + k.encode()
    return r + struct.pack('<I', len(words)) + struct.pack('<%dI' % len(words), *words)


def expect_exit(fn):
    try: fn()
    except SystemExit: return
    raise AssertionError('应当退出 | should have exited')


def main():
    seed = record([A, B], [1, 2, 3]) + record([C], [7])
    assert tool.warm_seed_records(seed) == (2, {A, B, C})
    assert tool.warm_seed_records(b'') == (0, set())
    for bad in (b'XLP', seed[:-2], struct.pack('<II', 0x57504c58, 6), struct.pack('<II', 0x12345678, 1), record([A], [])[:-4] + struct.pack('<I', 5000)):
        try: tool.warm_seed_records(bad)
        except ValueError: continue
        raise AssertionError('坏种子应当被拒 | a bad seed must be rejected: %r' % bad[:12])
    with tempfile.TemporaryDirectory() as d:
        sh = os.path.join(d, 'shaders'); os.makedirs(sh)
        for k in (A, B, C): open(os.path.join(sh, k + '.xgs'), 'wb').close()
        p = os.path.join(d, 'seed.bin'); open(p, 'wb').write(seed)

        assert tool.warm_seed_load(None) is None and tool.warm_seed(None, sh) is None and not os.path.exists(os.path.join(sh, 'vk-pipeline-list.seed'))
        loaded = tool.warm_seed_load(p); assert loaded[2] == 2 and loaded[3] == {A, B, C}
        assert tool.warm_seed(loaded, sh) == hashlib.sha256(seed).hexdigest()[:16]
        assert open(os.path.join(sh, 'vk-pipeline-list.seed'), 'rb').read() == seed


        os.remove(os.path.join(sh, C + '.xgs')); assert tool.warm_seed(loaded, sh) is not None
        for k in (A, B): os.remove(os.path.join(sh, k + '.xgs'))
        expect_exit(lambda: tool.warm_seed(loaded, sh))


        expect_exit(lambda: tool.warm_seed_load(os.path.join(d, 'none.bin')))
        open(p, 'wb').write(seed[:-2]); expect_exit(lambda: tool.warm_seed_load(p))
        open(p, 'wb').close(); expect_exit(lambda: tool.warm_seed_load(p))
    print('warm seed OK')


if __name__ == '__main__': main()
