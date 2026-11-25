#!/usr/bin/env python3




import os
from PIL import Image, ImageDraw, ImageFilter, ImageChops
W, H, S = 24, 33, 16
def strokes(r, wd, off=(0, 0)):
    m = Image.new('L', (W * S, H * S), 0); d = ImageDraw.Draw(m); cx, cy = W * S / 2 + off[0] * S, H * S / 2 + off[1] * S
    hw, hh = 8.3 * r / 6.2, 13.2 * r / 6.2; cx += 0.1 * S; cy += 0.4 * S
    for sx in (1, -1): d.line((cx - sx * hw * S, cy - hh * S, cx + sx * hw * S, cy + hh * S), fill=255, width=int(wd * S))
    return m
im = Image.new('RGBA', (W * S, H * S), (0, 0, 0, 0))
def lay(mask, col, a): 
    global im; l = Image.new('RGBA', im.size, col + (0,)); l.putalpha(mask.point(lambda v: int(v * a))); im = Image.alpha_composite(im, l)


lay(strokes(6.3, 5.0, (0.35, 0.45)), (8, 7, 6), 0.75); lay(strokes(6.2, 3.7), (150, 118, 62), 1.0); lay(strokes(6.0, 1.3, (-0.35, -0.35)), (226, 198, 132), 0.9)
lay(strokes(6.0, 0.9, (0.5, 0.5)), (92, 68, 34), 0.8)
im = im.resize((W, H), Image.LANCZOS); im.save(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'NativeHUD', 'touch', 'close.png'))
