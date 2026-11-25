











import argparse, sys
import numpy as np
from PIL import Image

ap = argparse.ArgumentParser()
ap.add_argument('mac_a'); ap.add_argument('mac_b'); ap.add_argument('ios')
ap.add_argument('--rect', action='append', default=[], help='x0,y0,x1,y1（含端点）')
ap.add_argument('--mask-out')
o = ap.parse_args()
paths, mask_out = (o.mac_a, o.mac_b, o.ios), o.mask_out
rects = [tuple(map(int, r.split(','))) for r in o.rect]
a, b, c = (np.asarray(Image.open(p).convert('RGBA')).astype(np.int16) for p in paths)
if not (a.shape == b.shape == c.shape): sys.exit(f'【红】尺寸不一：{a.shape} {b.shape} {c.shape}')
H, W = a.shape[:2]
T = 16
self_px = np.abs(a - b).max(axis=2) > 0
th, tw = (H + T - 1) // T, (W + T - 1) // T
tiles = np.zeros((th, tw), bool)
ys, xs = np.nonzero(self_px); tiles[ys // T, xs // T] = True
grown = tiles.copy()
for dy in (-1, 0, 1):
    for dx in (-1, 0, 1):
        grown |= np.pad(tiles, 1)[1 + dy:1 + dy + th, 1 + dx:1 + dx + tw]
mask = np.kron(grown, np.ones((T, T), bool))[:H, :W]
auto_n = int(mask.sum())
for x0, y0, x1, y1 in rects: mask[y0:y1 + 1, x0:x1 + 1] = True
diff = np.abs(c - a).max(axis=2)
bad = (diff > 0) & ~mask
inside = int(((diff > 0) & mask).sum())
print(f'尺寸 {W}×{H}；Mac 自身差异 {int(self_px.sum())} 像素 → 自动遮罩 {int(grown.sum())} 块 {auto_n} 像素；矩形 {len(rects)} 个；遮罩合计占 {mask.sum() / mask.size:.2%}')
if not self_px.any(): print('【警告】Mac 两次无自身差异：按墙钟变化的区域没暴露出来，遮罩可能不全，下方的红可能是误报')
print(f'遮罩外：比对 {int((~mask).sum())} 像素，不同 {int(bad.sum())}；遮罩内不同 {inside}（不计）')
if mask_out: Image.fromarray((mask * 255).astype(np.uint8)).save(mask_out)
if bad.any():
    ys, xs = np.nonzero(bad)
    print(f'【红】遮罩外有差：x {xs.min()}–{xs.max()} y {ys.min()}–{ys.max()}，最大分量差 {int(diff[bad].max())}')
    sys.exit(1)
print('全过：遮罩外逐位全等')
