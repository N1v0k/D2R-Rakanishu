









WIDTHS = (0, 1, 2, 4, 6, 8, 10, 12, 14, 16, 20, 24, 26, 28, 30, 32)
COLOURS = (0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4)


class Bits:

    def __init__(self, data, offset=0):
        self.data, self.pos = data, offset

    def copy(self):
        return Bits(self.data, self.pos)

    def get(self, n):
        v = 0
        for i in range(n):
            byte = self.pos >> 3
            if byte >= len(self.data): raise ValueError('dcc: read past the end')
            v |= (self.data[byte] >> (self.pos & 7) & 1) << i
            self.pos += 1
        return v

    def signed(self, n):
        v = self.get(n)
        if n == 0: return 0
        if n == 1: return -v
        return v - (1 << n) if v & (1 << (n - 1)) else v

    def skip(self, n):
        self.pos += n


def decode(data):


    b = Bits(data)
    if b.get(8) != 0x74: raise ValueError('dcc: not a dcc file')
    b.get(8)
    directions, frames = b.get(8), b.get(32)
    b.get(32); b.get(32)
    if not directions or not frames or frames > 256: raise ValueError('dcc: %d directions, %d frames' % (directions, frames))
    offsets = [b.get(32) for _ in range(directions)]
    return [direction(Bits(data, o * 8), frames) for o in offsets]


class _Cell:
    def __init__(self, x, y, w, h):
        self.x, self.y, self.w, self.h = x, y, w, h
        self.lw = self.lh = -1
        self.lx = self.ly = 0


def _split(start, size):

    first = 4 - start % 4
    if size - first <= 1: return [size]
    rest = size - first - 1
    n = 2 + rest // 4 - (1 if rest % 4 == 0 else 0)
    if n == 1: return [size]
    return [first] + [4] * (n - 2) + [size - first - 4 * (n - 2)]


def direction(b, count):
    b.get(32)
    comp = b.get(2)
    v0, wb, hb, xb, yb, ob, cb = (WIDTHS[b.get(4)] for _ in range(7))
    heads = []
    for _ in range(count):
        b.get(v0)
        w, h, x, y = b.get(wb), b.get(hb), b.signed(xb), b.signed(yb)
        b.get(ob); b.get(cb)
        if b.get(1): raise ValueError('dcc: bottom-up frames are not supported')
        heads.append((w, h, x, y - h + 1))
    if ob: raise ValueError('dcc: optional frame bytes are not supported')
    equal = b.get(20) if comp & 2 else 0
    mask = b.get(20)
    encoding = raw = 0
    if comp & 1: encoding, raw = b.get(20), b.get(20)
    palette = [i for i in range(256) if b.get(1)]
    eq = b.copy(); b.skip(equal)
    pm = b.copy(); b.skip(mask)
    et = b.copy(); b.skip(encoding)
    rp = b.copy(); b.skip(raw)
    pc = b.copy()
    left = min(h[2] for h in heads); top = min(h[3] for h in heads)
    W = max(h[2] + h[0] for h in heads) - left; H = max(h[3] + h[1] for h in heads) - top
    if W <= 0 or H <= 0 or W * H > 1 << 22: raise ValueError('dcc: canvas %dx%d' % (W, H))
    across, down = 1 + (W - 1) // 4, 1 + (H - 1) // 4
    widths = [4] * (across - 1) + [W - 4 * (across - 1)]
    heights = [4] * (down - 1) + [H - 4 * (down - 1)]
    grid = [_Cell(x * 4, y * 4, widths[x], heights[y]) for y in range(down) for x in range(across)]
    cells = []
    for w, h, x, y in heads:
        cw, ch = _split(x - left, w), _split(y - top, h)
        oy, mine = y - top, []
        for r in range(len(ch)):
            ox = x - left
            for c in range(len(cw)):
                mine.append(_Cell(ox, oy, cw[c], ch[r])); ox += cw[c]
            oy += ch[r]
        cells.append((len(cw), len(ch), mine))

    stacks, last = [], [None] * (across * down)
    for f, (w, h, x, y) in enumerate(heads):
        n_across, n_down, _ = cells[f]
        gx, gy = (x - left) // 4, (y - top) // 4
        for r in range(n_down):
            for c in range(n_across):
                at = gx + c + (gy + r) * across
                if last[at] is not None:
                    if equal and eq.get(1): continue
                    m = pm.get(4)
                else: m = 0xF
                stack, prev, got = [0, 0, 0, 0], 0, 0
                coded = et.get(1) if COLOURS[m] and encoding else 0
                for i in range(COLOURS[m]):
                    if coded: stack[i] = rp.get(8)
                    else:
                        stack[i] = prev; d = pc.get(4); stack[i] += d
                        while d == 15: d = pc.get(4); stack[i] += d
                    if stack[i] == prev: stack[i] = 0; break
                    prev = stack[i]; got += 1
                value, k = [0, 0, 0, 0], got - 1
                for i in range(4):
                    if m & 1 << i:
                        value[i] = stack[k] if k >= 0 else 0; k -= 1
                    else: value[i] = last[at]['v'][i]
                e = {'v': value, 'f': f, 'c': c + r * n_across}
                stacks.append(e); last[at] = e
    for e in stacks: e['v'] = [palette[v] if v < len(palette) else 0 for v in e['v']]

    canvas, out, k = bytearray(W * H), [], 0
    for f in range(len(heads)):
        frame = bytearray(W * H)
        for i, cell in enumerate(cells[f][2]):
            g = grid[cell.x // 4 + cell.y // 4 * across]
            e = stacks[k] if k < len(stacks) else None
            if e is None or e['f'] != f or e['c'] != i:

                same = (cell.w, cell.h) == (g.lw, g.lh)
                old = [canvas[g.lx + xx + (g.ly + yy) * W] for yy in range(cell.h) for xx in range(cell.w)] if same else None
                for yy in range(cell.h):
                    row = (cell.y + yy) * W
                    for xx in range(cell.w):
                        v = old[yy * cell.w + xx] if same else 0
                        canvas[cell.x + xx + row] = v; frame[cell.x + xx + row] = v
            else:
                v = e['v']; bits = 0 if v[0] == v[1] else 2 if v[1] != v[2] else 1
                for yy in range(cell.h):
                    row = (cell.y + yy) * W
                    for xx in range(cell.w):
                        p = v[pc.get(bits)] if bits else v[0]
                        canvas[cell.x + xx + row] = p; frame[cell.x + xx + row] = p
                k += 1
            g.lw, g.lh, g.lx, g.ly = cell.w, cell.h, cell.x, cell.y
        out.append(bytes(frame))
    return {'w': W, 'h': H, 'x': left, 'y': top, 'frames': out}
