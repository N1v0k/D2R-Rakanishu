#!/usr/bin/env python3








import numpy as np
from PIL import Image, ImageDraw
X0, X1 = 58, 594
W, H = 732, 649
EXW = W - (X1 - X0); HALF = EXW // 2
FR, FRR = 20, 26
SCALE = 0.85
TOP = dict(dx=43, dy=43, y=28, gap=18)
SHIFT = 36
CONS = dict(dx=44, y=278)
YM, YS, YB = 283, 462, 507
GEM_COLS = 'w g r y v b'.split(); GEM_ROWS = 'gc gf gs gl gp'.split()
GEM_ODD = {'glv': 'gzv'}; SKULL = 'skc skf sku skl skz'.split()
MAT_L = ['pk1', 'pk2', 'pk3', 'dhn', 'bey', 'mbr', 'rvs', 'rvl']
MAT_R = ['ua1', 'ua2', 'ua3', 'ua4', 'ua5', 'xa1', 'xa2', 'xa3', 'xa4', 'xa5', 'toa', 'tes', 'ceh', 'bet', 'fed']
PED_X = 205 - X0 + HALF - SHIFT

def geometry():

    dm = 45 * SCALE; tw = 13 * TOP['dx'] + dm + TOP['gap']; x0 = FR + (W - FR - FRR - tw) / 2; g = {}
    for r in range(5):
        for k in range(14):
            cx, cy = x0 + k * TOP['dx'] + dm / 2 + (TOP['gap'] if k >= 7 else 0), TOP['y'] + r * TOP['dy'] + dm / 2
            if k < 6: c = GEM_ROWS[r] + GEM_COLS[k]; g[GEM_ODD.get(c, c)] = (cx, cy, 1)
            elif k == 6: g[SKULL[r]] = (cx, cy, 1)
            elif r < 4: g['r%02d' % (r * 7 + k - 6)] = (cx, cy, 1)
            elif k - 7 < 5: g['r%02d' % (29 + k - 7)] = (cx + TOP['dx'], cy, 1)
    def group(codes, counts, gx):
        i = 0
        for row, n in enumerate(counts):
            for k in range(n):
                cx = gx + k * CONS['dx'] + dm / 2
                cy = CONS['y'] + 95 * SCALE / 2 if row == 0 else CONS['y'] + 95 * SCALE + 6 + (row - 1) * CONS['dx'] + dm / 2
                g[codes[i]] = (cx, cy, 2 if row == 0 else 1); i += 1
    pl, pr = PED_X, PED_X + 234
    group(MAT_L, (3, 3, 2), FR + (pl - FR - (2 * CONS['dx'] + dm)) / 2); group(MAT_R, (5, 5, 5), pr + (W - FRR - pr - (4 * CONS['dx'] + dm)) / 2)
    return g

def slots():

    out = {}
    for code, (cx, cy, hgt) in geometry().items():
        w = round(98 * SCALE); h = w if hgt == 1 else round(196 * SCALE); wp, hp = 49 * SCALE, 49 * SCALE * hgt
        out[code] = (round((cx - wp / 2 - (HALF - X0)) * 2), round((cy - hp / 2) * 2) + 102, w, h)
    return out

def _stats(im, boxes):
    a = np.concatenate([np.asarray(im.crop(b).convert('RGB'), dtype=np.float64).reshape(-1, 3) for b in boxes]); return a.mean(0), a.std(0)
def _stone(csh, w, h, target, ox=90, oy=190):
    src = np.asarray(csh.crop((ox, oy, ox + w, oy + h)).convert('RGB'), dtype=np.float64); m, s = src.reshape(-1, 3).mean(0), src.reshape(-1, 3).std(0)
    return Image.fromarray(np.clip((src - m) / s * target[1] + target[0], 0, 255).astype(np.uint8)).convert('RGBA')
def _feather(sz, n=2):
    w, h = sz; a = np.ones((h, w))
    for i in range(n): v = (i + 1) / (n + 1); a[i, :] = np.minimum(a[i, :], v); a[h - 1 - i, :] = np.minimum(a[h - 1 - i, :], v); a[:, i] = np.minimum(a[:, i], v); a[:, w - 1 - i] = np.minimum(a[:, w - 1 - i], v)
    return Image.fromarray((a * 255).astype(np.uint8))
def _slot(src, x, y, w=45, h=45, pad=4):
    t = src.crop((x - pad, y - pad, x + w + pad, y + h + pad)); return t.resize((round(t.width * SCALE), round(t.height * SCALE)), Image.LANCZOS)
def _paste(dst, t, cx, cy): dst.paste(t, (round(cx - t.width / 2), round(cy - t.height / 2)), _feather(t.size))
def _mirror_fill(dst, strip, x, y, w):
    k = 0; px = x
    while px < x + w:
        p = strip if k % 2 == 0 else strip.transpose(Image.FLIP_LEFT_RIGHT); p = p.crop((0, 0, min(p.width, x + w - px), p.height)); dst.paste(p, (px, y)); px += p.width; k += 1

def build(gems, runes, cons, csh):

    assert gems.size == runes.size == cons.size == (652, 649) and csh.size == (1369, 706), '原版底图尺寸不是原样（原版资源变了？）'
    out = Image.new('RGBA', (W, H)); g = gems.crop((X0, 0, X1, H)); c = cons.crop((X0, 0, X1, H)); gw = g.width
    tgt = _stats(gems, [(165, 31, 178, 276), (226, 31, 239, 276), (287, 31, 300, 276)])
    out.paste(g.crop((0, 0, FR, H)), (0, 0)); out.paste(g.crop((gw - FRR, 0, gw, H)), (W - FRR, 0))
    st = _stone(csh, W - 2 * FR + 6, H, tgt); out.paste(st.crop((0, 0, st.width, YS - 14)), (FR - 3, 14))
    out.paste(g.crop((0, 0, FR - 3, YS)), (0, 0)); out.paste(g.crop((gw - FRR + 3, 0, gw, YS)), (W - FRR + 3, 0)); _mirror_fill(out, g.crop((FR, 0, gw - FRR, 15)), FR, 0, W - FR - FRR)


    def widen(y0, y1, xl, xr, mode):
        band = c.crop((0, y0, gw, y1)); h = y1 - y0; nx = xl + HALF - SHIFT; left, mid, right = band.crop((0, 0, xl, h)), band.crop((xl, 0, xr, h)), band.crop((xr, 0, gw, h))
        def fill(x, w, edge, flip):
            if w <= 0: return
            if mode == 'tile':
                src = edge.crop((edge.width - 70, 0, edge.width - 6, h)) if not flip else edge.crop((6, 0, 70, h)); blk = Image.new('RGBA', (w, h)); px = 0
                while px < w: blk.paste(src, (px if not flip else w - px - src.width, 0)); px += src.width
            else:
                tg = _stats(band, [(30, h - 16, gw - 30, h - 4)]); blk = _stone(csh, w, h, tg, ox=300 + x, oy=420);    col = np.asarray(edge.convert('RGB'))[:, -3 if not flip else 2].astype(int); d = ImageDraw.Draw(blk)
                for yy in range(h):
                    r_, g_, b_ = col[yy]
                    if r_ - b_ > 22 and r_ > 60: d.line([(0, yy), (w, yy)], fill=(int(r_), int(g_), int(b_), 255))
            out.paste(blk, (x, y0))
        fill(xl, nx - xl, left, False); fill(nx + (xr - xl), W - (gw - xr) - nx - (xr - xl), right, True)
        out.paste(left, (0, y0)); out.paste(mid, (nx, y0)); out.paste(right, (W - (gw - xr), y0))
    ped = cons.crop((205, 264, 439, YS + 3)); out.paste(ped, (PED_X, 264), _feather(ped.size, 1))
    widen(YS, YB, 205 - X0, 439 - X0, 'tile'); widen(YB, H, 226 - X0, 421 - X0, 'dark')
    gs, rs, tall, one = _slot(gems, 117, 31), _slot(runes, 93, 51), _slot(cons, 87, 49, 45, 95), _slot(cons, 87, 154)
    for code, (cx, cy, hgt) in geometry().items():
        _paste(out, tall if hgt == 2 else gs if code[0] in 'gs' else rs if code[0] == 'r' and code[1:].isdigit() else one, cx, cy)
    return out
