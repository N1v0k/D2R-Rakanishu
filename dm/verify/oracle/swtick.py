















import sys, os, struct, json, argparse, time
sys.path.insert(0, os.path.dirname(__file__))
from swgdb import Gdb
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
from game_addrs import GA

TICK_ENTRY = GA.SERVER_TICK
TBASE = (0x2230, 0x2630, 0x2a30, 0x3230, 0x2e30)
NB, NEXT, MAXS = 128, 0x158, 96

def fnv(h, b):
    for c in b: h = ((h ^ c) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return h

class Reader:
    def __init__(self, g): self.g = g; self.n = 0
    def rd(self, a, n):
        if not a: return None
        self.n += 1
        try: return self.g.read(a, n)
        except IOError: return None

def units(r, game):
    tab = r.rd(game + TBASE[0], 0x1400)
    out = []; bad = 0
    for t, base in enumerate(TBASE):
        for b in range(NB):
            u = struct.unpack_from('<Q', tab, base - TBASE[0] + 8 * b)[0]
            k = 0
            while u and k < 4096:
                hd = r.rd(u, NEXT + 8)
                if hd is None: bad += 1; break
                ty, cls, uid, mode = struct.unpack_from('<IIII', hd, 0)
                x = {'type': ty, 'cls': cls, 'id': uid, 'mode': mode, 'act': hd[0x18], 'seed': struct.unpack_from('<I', hd, 0x30)[0],
                     'rlo': struct.unpack_from('<I', hd, 0x28)[0], 'rhi': struct.unpack_from('<I', hd, 0x2c)[0],
                     'x': 0, 'y': 0, 'xf': 0, 'yf': 0, 'it': None, 'st': [], 'ptr': u, 'path': struct.unpack_from('<Q', hd, 0x38)[0]}
                if t == 4:
                    d = struct.unpack_from('<Q', hd, 0x10)[0]; idt = r.rd(d, 0x1c)
                    if idt: x['it'] = struct.unpack_from('<IIIII', idt, 0)[0:3] + (struct.unpack_from('<I', idt, 0x18)[0], struct.unpack_from('<I', idt, 0x10)[0])
                if t in (0, 1, 3):
                    p = r.rd(struct.unpack_from('<Q', hd, 0x38)[0], 8)
                    if p: x['xf'], x['x'], x['yf'], x['y'] = struct.unpack('<HHHH', p)
                sl = struct.unpack_from('<Q', hd, 0x88)[0]
                slh = r.rd(sl, 0x40)
                if slh:
                    sb, sc = struct.unpack_from('<Q', slh, 0x30)[0], struct.unpack_from('<I', slh, 0x38)[0]
                    arr = r.rd(sb, 8 * min(sc, MAXS)) if sb and sc else None
                    if arr: x['st'] = [struct.unpack_from('<II', arr, 8 * j) for j in range(len(arr) // 8)]
                if ty != t: bad += 1
                out.append(x); k += 1
                u = struct.unpack_from('<Q', hd, NEXT)[0]
    return out, bad

def tick_line(r, game):
    tick = struct.unpack('<I', r.rd(game + 0x170, 4))[0]
    us, bad = units(r, game)
    us.sort(key=lambda u: (u['type'], u['id']))
    h = [0xcbf29ce484222325] * 6
    gs = r.rd(game + 0x1a8, 8) or bytes(8); h[5] = fnv(h[5], gs)
    cnt = [0] * 5
    for u in us:
        t = u['type']
        if t >= 5: continue
        cnt[t] += 1
        hh = fnv(h[t], struct.pack('<9I', t, u['cls'], u['id'], u['mode'], u['act'], u['x'], u['y'], u['xf'], u['yf']))
        hh = fnv(hh, struct.pack('<II', u['rlo'], u['rhi']))
        if t == 4: hh = fnv(hh, struct.pack('<5I', *(u['it'] or (0,) * 5)))
        if t <= 1 or t == 4:
            hh = fnv(hh, struct.pack('<I', len(u['st'])))
            for a, b in u['st']: hh = fnv(hh, struct.pack('<II', a, b))
        h[t] = hh
    return {'tick': tick, 'n': cnt, 'h': ['%016x' % x for x in h], 'bad': bad, 'reads': r.n}, us

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--port', type=int, default=55571); ap.add_argument('--ticks', type=int, default=25)
    ap.add_argument('--out', default='-'); ap.add_argument('--full', action='store_true')
    A = ap.parse_args()
    g = Gdb(port=A.port)
    mods, info = g.modules()
    game_mod = next((v for k, v in mods.items() if k.startswith('Game')), None)
    if not game_mod: sys.exit('模块表里没有 Game.nss：\n' + info)
    bp = game_mod[0] + TICK_ENTRY
    g.interrupt()
    if not g.set_bp(bp): sys.exit(f'下断失败 {bp:#x}')
    f = sys.stdout if A.out == '-' else open(A.out, 'w')
    t0 = time.time()
    try:
        last = None; dup = 0; gaps = []
        while True:
            g.cont(); stop = g.wait_stop(timeout=60)
            k = 0
            while g.reg(32) != bp:
                k += 1
                if k > 50: sys.exit(f'连续 50 次停在非拍入口 {g.reg(32):#x}')
                g.cont(); stop = g.wait_stop(timeout=60)
            game = g.reg(0)
            tick = int.from_bytes(g.read(game + 0x170, 4), 'little')
            if tick == last:
                dup += 1; g.step_over(bp, stop); continue
            if last is not None and tick != last + 1: gaps.append((last, tick))
            r = Reader(g)
            line, us = tick_line(r, game)
            line['game'] = hex(game); line['dt'] = round(time.time() - t0, 3)
            f.write(json.dumps(line) + '\n'); f.flush()
            if A.full:
                for u in us: f.write(json.dumps({'t': line['tick'], 'u': u}) + '\n')
            last = tick
            g.step_over(bp, stop)
            A.ticks -= 1
            if A.ticks <= 0: break
        print(json.dumps({'重复报告': dup, '断档': gaps}), file=sys.stderr)
        if gaps: sys.exit(f'拍号不连续：{gaps[:5]}')
    finally:
        g.clear_bp(bp); g.cont(); g.close()
