#!/usr/bin/env python3


















import os, sys
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import gen_widestash_art as G

ART = __import__('gen_paths').gen_dir('NativeHUD', 'widestash')
BG_X, BG_Y = -2, 100
TOP = 516
BOX = (322, 576, 524, 644)

def main():
    import numpy as np
    from PIL import Image
    L = G.lib(); src = open(os.path.join(ART, 'StashPanelBGPersonal_wide.sprite'), 'rb').read(); im = G.decode(L, src)
    w, h = im.size; eh = h - TOP; eh += -eh % 4
    a = np.asarray(im).astype(np.float32).copy(); x0, y0, x1, y1 = BOX; mid = (x0 + x1) // 2; n = mid - x0


    g = a[..., :3].sum(axis=2)
    if not (g[608, x0 + 20:x1 - 20].mean() < 75 and g[608, x0 - 60:x0 - 10].mean() > 85): raise SystemExit('gen_vault_cover：个人页底图里金币框不在预期位置（原版资源变了？）')
    left = a[y0:y1, x0 - n:x0][:, ::-1]; right = a[y0:y1, x1:x1 + (x1 - mid)][:, ::-1]
    a[y0:y1, x0:mid] = left; a[y0:y1, mid:x1] = right
    for k in range(16):
        t = (k + .5) / 16; xl = mid - 8 + k; a[y0:y1, xl] = (1 - t) * a[y0:y1, x0 - 1 - (xl - x0)] + t * a[y0:y1, x1 + (x1 - 1 - xl)]
    im = Image.fromarray(np.clip(a, 0, 255).astype(np.uint8), 'RGBA')
    crop = im.crop((0, TOP, w, TOP + eh))
    open(os.path.join(ART, 'VaultBottom.sprite'), 'wb').write(G.pack(L, src, G.bc3(crop), w, eh, w)); crop.save(os.path.join(ART, 'VaultBottom.png'))
    print('VaultBottom：底图第 %d–%d 行，%d×%d，面板单位 (%d,%d)' % (TOP, TOP + eh, w, eh, BG_X, BG_Y + 2 * TOP))

if __name__ == '__main__':
    main()
