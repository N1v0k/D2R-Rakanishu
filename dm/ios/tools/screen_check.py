#!/usr/bin/env python3














import argparse, re, sys
import numpy as np
from PIL import Image

ap = argparse.ArgumentParser()
ap.add_argument('png'); ap.add_argument('meta'); ap.add_argument('log')
ap.add_argument('--want', choices=['full', '16:9'], default='full')
a = ap.parse_args()
BLACK_MAX, BLACK_FRAC = 8, 0.85

img = np.asarray(Image.open(a.png).convert('RGB')).astype(np.int16)
H, W = img.shape[:2]
meta = dict(l.split('\t', 1) for l in open(a.meta).read().splitlines() if '\t' in l)
orient = int(meta['orientation'])
vps = re.findall(r'出图视口 (\d+),(\d+) (\d+)x(\d+) / 画布 (\d+)x(\d+)', open(a.log, errors='replace').read())
bad = 0
def res(ok, msg):
    global bad
    print(('过  ' if ok else '【红】') + msg); bad += not ok

res(orient in (3, 4) and W > H, f'① 方向：界面方向 {orient}（3/4 = 横屏），截图 {W}x{H}')
if not vps:
    res(False, '② 宽高比：日志里没有「出图视口」行（没出过图？）'); vx = vy = 0; vw, vh, cw, ch = W, H, W, H
else:
    vx, vy, vw, vh, cw, ch = map(int, vps[-1])
    same = abs(cw / ch - W / H) < 0.01
    if a.want == 'full': ok = same and vx == 0 and vy == 0 and abs(vw - cw) <= 1 and abs(vh - ch) <= 1; what = '占满画布'
    else: ok = same and abs(vw / vh - 16 / 9) < 0.01 and abs(2 * vx + vw - cw) <= 1 and abs(2 * vy + vh - ch) <= 1; what = '16:9 居中'
    res(ok, f'② 宽高比（要{what}）：视口 {vx},{vy} {vw}x{vh} / 画布 {cw}x{ch}（{cw / ch:.4f}）；截图比例 {W / H:.4f}')




band = img[int(H * 0.1):int(H * 0.9)].max(axis=(0, 2)); blk = band <= 1; nb = int(blk.sum())
lb = int(np.argmax(~blk)) if (~blk).any() else W; rb = int(np.argmax(~blk[::-1])) if (~blk).any() else W
if a.want == 'full': res(nb == 0, f'①b 内容铺满：整列纯黑 {nb} 列（左起连续 {lb}、右起连续 {rb}；判据 0）')
else: print(f'   ①b（16:9 口径只报数）整列纯黑 {nb} 列（左起连续 {lb}、右起连续 {rb}）')
sx, sy = W / cw, H / ch
reg = img[int(vy * sy):int((vy + vh) * sy), int(vx * sx):int((vx + vw) * sx)]
frac = float((reg.max(axis=2) < BLACK_MAX).mean()) if reg.size else 1.0
res(frac <= BLACK_FRAC, f'③ 不大面积全黑：视口区域近黑像素占 {frac * 100:.1f}%（阈值 ≤ {BLACK_FRAC * 100:.0f}%）')
sys.exit(1 if bad else 0)
