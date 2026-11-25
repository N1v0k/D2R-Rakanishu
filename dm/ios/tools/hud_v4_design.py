











import os, struct
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import numpy as np
import romfs_art
DM_ROOT = os.environ.get('DM_ROOT') or os.path.expanduser('~/dm-work')



ART = {'front_panel': ('hd/global/ui/PANEL/HUD_02/front_panel.sprite', (2822, 415)),
       'levelupbutton': ('hd/global/ui/Controller/HUD/LevelUpButton.sprite', (248, 121)),
       'greater_healing_potion': ('hd/global/ui/items/misc/potion/greater_healing_potion.sprite', (98, 98)),
       'healing_potion': ('hd/global/ui/items/misc/potion/healing_potion.sprite', (98, 98)),
       'greater_mana_potion': ('hd/global/ui/items/misc/potion/greater_mana_potion.sprite', (98, 98)),
       'rejuv_potion': ('hd/global/ui/items/misc/potion/rejuv_potion.sprite', (98, 98)),
       'perfect_skull': ('hd/global/ui/items/misc/gem/perfect_skull.sprite', (98, 98))}


class Source:

    def __init__(self, romfs, pc=None):
        self.romfs = romfs
        pc = pc or os.environ.get('DM_PC_DATA') or os.path.join(DM_ROOT, 'data/assets-src/raw')
        self.pc = pc if all(os.path.isfile(os.path.join(pc, p)) for p, _ in ART.values()) else None
        if not self.pc and os.environ.get('DM_REQUIRE_PC_ART') == '1':
            raise SystemExit('🔴 DM_REQUIRE_PC_ART=1 但没找到 PC 版 D2R 文件（DM_PC_DATA=%s，要有 hd/global/ui/... 下的原始 .sprite）：正式包的底栏会变糊，不出' % pc)
        if not self.pc: print('没找到 PC 版文件（DM_PC_DATA=%s），底栏用 Switch 素材放大，会略糊' % pc)
        else: print('底栏素材：PC 版 2 倍原图（%s）' % self.pc)

    def img(self, name):
        path, size = ART[name]
        if self.pc:
            b = open(os.path.join(self.pc, path), 'rb').read(); ver, fw, w, h = struct.unpack_from('<HHII', b, 4)
            assert b[:4] == b'SpA1' and ver == 31 and (w, h) == size, (path, ver, w, h)
            return Image.frombytes('RGBA', (w, h), b[40:40 + w * h * 4])
        return romfs_art.sprite(self.romfs, path).resize(size, Image.LANCZOS)

    def font(self, name, size): return ImageFont.truetype(romfs_art.font(self.romfs, name), size)


def load(src, VAR='C'):

    fp = src.img('front_panel'); lvl = src.img('levelupbutton').crop((0, 0, 124, 121))
    W, H = 2868, 1320


    K = 156 / 348
    P, DM = 120, 114
    TILE = (1673, 277, 1792, 391)
    SH = round(DM * (TILE[3] - TILE[1]) / (TILE[2] - TILE[0]))
    B = 12; EXT = 60; BH = 156
    XC = 1390
    N = 6; SLOTS_W = N * DM + (N - 1) * (P - DM)
    SX0 = XC - SLOTS_W // 2; SY0 = H - BH + (BH - SH) // 2
    BAR_X0 = SX0 - B; BAR_X1 = SX0 + SLOTS_W + B
    SAFE = 0.09
    BADGE = 26
    out = Image.new('RGBA', (W, H))


    STONE = (690, 330, 1170, 388); EDGE = (690, 276, 1170, 288)
    BW_ = BAR_X1 - BAR_X0 + 2 * EXT; bar = Image.new('RGBA', (BW_, BH))
    tile = fp.crop(STONE); edge = fp.crop(EDGE)
    for yy in range(0, BH, tile.height):
        for xx in range(0, BW_, tile.width): bar.alpha_composite(tile if (xx // tile.width) % 2 == 0 else tile.transpose(Image.FLIP_LEFT_RIGHT), (xx, yy))
    for xx in range(0, BW_, edge.width):
        bar.alpha_composite(edge, (xx, 0)); bar.alpha_composite(edge.transpose(Image.FLIP_TOP_BOTTOM), (xx, BH - edge.height))
    ImageDraw.Draw(bar).rectangle((0, 0, bar.width - 1, bar.height - 1), outline=(40, 33, 24, 255), width=3)
    slot = fp.crop(TILE).resize((DM, SH), Image.LANCZOS)
    ta = np.array(slot).astype(int); gold = (ta[..., 0] > 90) & (ta[..., 0] - ta[..., 2] > 40)
    def inner(line):
        def scan(seq):
            i = 0
            while i < len(seq) and not seq[i]: i += 1
            while i < len(seq) and seq[i]: i += 1
            while i + 3 < len(seq) and any(seq[i:i + 3]): i += 1
            return i
        return scan(line), len(line) - 1 - scan(line[::-1])
    INNER = (2, 107, 7, 101)
    if src.pc:
        IX0, IX1 = inner(list(gold[SH // 2])); IY0, IY1 = inner(list(gold[:, DM // 2]))
        assert (IX0, IX1, IY0, IY1) == INNER, ('金框内沿和记录的不一致，改 INNER', IX0, IX1, IY0, IY1)
    else: IX0, IX1, IY0, IY1 = INNER
    IW, IH = IX1 - IX0 + 1, IY1 - IY0 + 1; BOX = round(min(IW, IH) * (1 - 2 * SAFE))
    slots = [(SX0 + i * P, SY0, SX0 + i * P + DM, SY0 + SH) for i in range(N)]


    ns = {}
    def prep(im):
        a = np.array(im)[..., 3]; ys, xs = np.where(a > 24); return im.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    def fit(im, i, name, text=False):


        im = prep(im); w, h = im.size
        k = BOX * 0.85 / h if text else min(BOX / max(w, h), ns['REF_G'] / ((w * h) ** 0.5))
        im = im.resize((max(1, round(w * k)), max(1, round(h * k))), Image.LANCZOS)
        cx = slots[i][0] + IX0 + IW / 2; cy = slots[i][1] + IY0 + IH / 2
        ns['layer'].alpha_composite(im, (round(cx - im.width / 2), round(cy - im.height / 2)))
    pc = src.img
    exo = lambda s: src.font('exocetblizzardot-medium.otf', s)
    num = lambda s: src.font('blizzardglobal-v5_81.ttf', s)
    def metal(t, size):
        f = exo(size); pad = size // 4; W0, H0 = size * len(t) + 2 * pad, size * 2
        m = Image.new('L', (W0, H0)); ImageDraw.Draw(m).text((pad, pad), t, font=f, fill=255)
        a = np.array(m).astype(float) / 255; ys = np.where(a.max(1) > 0)[0]; y0, y1 = ys.min(), ys.max()
        g = np.zeros((H0, W0, 3)); top = np.array([236, 206, 140]); bot = np.array([120, 84, 36])
        for y in range(H0): t_ = min(1, max(0, (y - y0) / max(1, y1 - y0))); g[y, :] = top * (1 - t_) + bot * t_
        hi = np.roll(np.roll(a, -2, 0), -2, 1); sh = np.roll(np.roll(a, 3, 0), 3, 1)
        rgb = g * a[..., None]; rgb += np.clip(a - hi, 0, 1)[..., None] * np.array([255, 240, 200]) * 0.6; rgb *= (1 - 0.45 * np.clip(a - sh, 0, 1))[..., None]
        ol = np.array(m.filter(ImageFilter.MaxFilter(5))).astype(float) / 255
        out_ = np.zeros((H0, W0, 4)); out_[..., :3] = rgb + (1 - a)[..., None] * np.array([28, 18, 8]); out_[..., 3] = np.maximum(a, ol) * 255
        return Image.fromarray(np.clip(out_, 0, 255).astype('uint8'), 'RGBA')
    _r = prep(pc('greater_healing_potion')); _k = BOX / max(_r.size); ns['REF_G'] = ((_r.width * _k) * (_r.height * _k)) ** 0.5


    L = fp.crop((0, 0, 610, 415)); R = fp.crop((2210, 0, 2822, 415))
    L = L.resize((round(L.width * K), round(L.height * K)), Image.LANCZOS); R = R.resize((round(R.width * K), round(R.height * K)), Image.LANCZOS)
    BR = round(174 * K)
    LX = BAR_X0 + 6 - round(608 * K); RX = BAR_X1 - 6 - round(24 * K)
    LBC = (LX + round(434 * K), H - L.height + round(239 * K)); RBC = (RX + round(198 * K), H - R.height + round(239 * K))


    EX0, EX1 = slots[0][0], slots[-1][2]; EY = H - BH - 12


    d = ImageDraw.Draw(out); fz = src.font('blizzardglobal-v5_81.ttf', 30)
    statue = np.zeros((H, W), bool)
    for im_, x_ in ((L, LX), (R, RX)): statue[H - im_.height:H, x_:x_ + im_.width] |= np.array(im_)[..., 3] > 24
    TXT = [(LBC, '生命: 55 / 55'), (RBC, '法力: 20 / 20')]; GAP = 6
    def tbox(c, t, yb): x0, y0, x1, y1 = d.textbbox((c[0], yb), t, font=fz, anchor='mb', stroke_width=1); return x0, y0, x1 + 2, y1 + 2
    def ok_at(yb):
        for c, t in TXT:
            x0, y0, x1, y1 = tbox(c, t, yb)
            if statue[y0:y1, x0:x1].any(): return False
            if x1 > EX0 - GAP and x0 < EX1 + GAP and y1 > EY - GAP: return False
        return True
    YB = LBC[1] - BR - 4
    while not ok_at(YB): YB -= 1
    ns.update(W=W, H=H, DM=DM, SH=SH, EXT=EXT, BH=BH, BAR_X0=BAR_X0, BADGE=BADGE, BOX=BOX, IX0=IX0, IX1=IX1, IY0=IY0, IY1=IY1,
              bar=bar, slot=slot, slots=slots, L=L, R=R, LX=LX, RX=RX, LBC=LBC, RBC=RBC, BR=BR, EX0=EX0, EX1=EX1, EY=EY, YB=YB,
              lvl=lvl, prep=prep, fit=fit, pc=pc, num=num, metal=metal, layer=Image.new('RGBA', (W, H)))
    return ns
