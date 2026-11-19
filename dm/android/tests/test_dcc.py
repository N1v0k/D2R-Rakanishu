




import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
import dcc

WIDTH_CODE = 5


class Writer:
    def __init__(self): self.bits = []

    def put(self, v, n):
        for i in range(n): self.bits.append(v >> i & 1)

    def extend(self, other): self.bits.extend(other.bits)

    def data(self):
        out = bytearray((len(self.bits) + 7) // 8)
        for i, b in enumerate(self.bits): out[i >> 3] |= b << (i & 7)
        return bytes(out)


def _cells(n):

    out, at = [], 0
    for size in dcc._split(0, n): out.append((at, size)); at += size
    return out


def encode_direction(frames, w, h, used):


    return encode_frames([(0, 0, w, h, f) for f in frames], used)


def encode_frames(frames, used):


    eq, pm, pc, pixels = Writer(), Writer(), Writer(), Writer()
    left, top = min(f[0] for f in frames), min(f[1] for f in frames)
    seen = {}
    for fx, fy, w, h, frame in frames:
        gx, gy = (fx - left) // 4, (fy - top) // 4
        rows, cols = _cells_at(fy - top, h), _cells_at(fx - left, w)
        for r, (cy, ch) in enumerate(rows):
            for c, (cx, cw) in enumerate(cols):
                block = [frame[(cy + y) * w + cx + x] for y in range(ch) for x in range(cw)]
                key = (gx + c, gy + r)
                if key in seen:
                    if seen[key] == ((cw, ch), block): eq.put(1, 1); continue
                    eq.put(0, 1); pm.put(0xF, 4)
                seen[key] = ((cw, ch), block)
                colours = sorted(set(block) - {0})
                prev = 0
                for col in colours:
                    d = col - prev
                    while d >= 15: pc.put(15, 4); d -= 15
                    pc.put(d, 4); prev = col
                if len(colours) < 4: pc.put(0, 4)
                v = list(reversed(colours)) + [0] * (4 - len(colours))
                if v[0] == v[1]: continue
                bits = 2 if v[1] != v[2] else 1
                for p in block: pixels.put(v.index(p), bits)
    out = Writer()
    out.put(0, 32); out.put(2, 2)
    for code in (0, WIDTH_CODE, WIDTH_CODE, WIDTH_CODE, WIDTH_CODE, 0, 0): out.put(code, 4)
    for fx, fy, w, h, _ in frames:
        out.put(w, 8); out.put(h, 8); out.put(fx & 255, 8); out.put((fy + h - 1) & 255, 8); out.put(0, 1)
    out.put(len(eq.bits), 20); out.put(len(pm.bits), 20)
    for i in range(256): out.put(1 if i in used else 0, 1)
    out.extend(eq); out.extend(pm); out.extend(pc); out.extend(pixels)
    return out.data()


def _cells_at(start, n):

    out, at = [], 0
    for size in dcc._split(start, n): out.append((at, size)); at += size
    return out


def encode_moving(frames, used):

    body = encode_frames(frames, used)
    head = Writer()
    head.put(0x74, 8); head.put(6, 8); head.put(1, 8); head.put(len(frames), 32); head.put(1, 32); head.put(0, 32); head.put(19, 32)
    return head.data() + body


def encode(directions, w, h, used):

    bodies = [encode_direction(f, w, h, used) for f in directions]
    head = Writer()
    head.put(0x74, 8); head.put(6, 8); head.put(len(directions), 8); head.put(len(directions[0]), 32); head.put(1, 32); head.put(0, 32)
    at = 15 + 4 * len(directions)
    for b in bodies: head.put(at, 32); at += len(b)
    return head.data() + b''.join(bodies)


def frame(w, h, f):
    return bytes(f(x, y) for y in range(h) for x in range(w))


W, H = 6, 5
FRAMES = [
    [frame(W, H, lambda x, y: 1 if x < 4 and y < 4 else 0),
     frame(W, H, lambda x, y: 1 if x < 4 and y < 4 else 2 if x >= 4 else 0),
     frame(W, H, lambda x, y: (x + y) % 3)],
    [frame(W, H, lambda x, y: 2 if y == 4 else 0),
     frame(W, H, lambda x, y: 2 if y == 4 else 0),
     frame(W, H, lambda x, y: 1 if x == y else 0)],
]
USED = (0, 7, 9)


def main():
    checks = fails = 0

    def check(ok, what):
        nonlocal checks, fails
        checks += 1
        if not ok: fails += 1; print('FAIL', what)

    data = encode(FRAMES, W, H, USED)
    dirs = dcc.decode(data)
    check(len(dirs) == 2 and all(len(d['frames']) == 3 for d in dirs), 'two directions of three frames')
    check(all((d['w'], d['h'], d['x'], d['y']) == (W, H, 0, 0) for d in dirs), 'canvas 6x5 at 0, 0')
    for di, d in enumerate(dirs):
        for fi, got in enumerate(d['frames']):
            want = bytes(USED[p] for p in FRAMES[di][fi])
            check(got == want, 'direction %d frame %d decodes pixel for pixel' % (di, fi))


    moved = dcc.decode(encode_moving([(0, 0, 3, 1, bytes([1, 2, 0])), (1, 0, 3, 1, bytes([1, 2, 0]))], USED))[0]
    check(moved['w'] == 4 and moved['frames'][1] == bytes([0, 7, 9, 0]), 'a cell moved right by one copies [7, 9, 0] (%s)' % list(moved['frames'][1]))
    try: dcc.decode(b'\x00' + data[1:]); check(False, 'a bad signature fails')
    except ValueError: check(True, '')
    try: dcc.decode(data[:40]); check(False, 'a cut file fails')
    except ValueError: check(True, '')
    print('test_dcc: %d checks, %d failed' % (checks, fails))
    sys.exit(1 if fails else 0)


if __name__ == '__main__':
    main()
