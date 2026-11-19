#!/usr/bin/env python3








import os, sys
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import romfs_art, gen_widestash_art as G
from gen_paths import gen_dir

SRC = 'hd/global/ui/FrontEnd/HD/Final/FrontEnd_RightPanel.sprite'
Y0, Y1 = 980, 1040
COPY = ((112, 52, 58), (170, 112, 60), (230, 53, 1), (231, 52, 1), (232, 52, 58))

def main(romfs):
    b = romfs_art.read(romfs, SRC); o = np.array(romfs_art.sprite(romfs, SRC)); w, h = o.shape[1], o.shape[0]
    assert (w, h) == (401, 1080), (w, h)
    m = o.copy()
    for d, s, n in COPY: m[Y0:Y1, d:d + n] = o[Y0:Y1, s:s + n]
    p = os.path.join(gen_dir('NativeHUD', 'touch'), 'fe_rightpanel.sprite')
    open(p, 'wb').write(G.pack(G.lib(), b, G.bc3(Image.fromarray(m, 'RGBA')), w, h, w))
    print('fe_rightpanel：%d×%d → %s' % (w, h, p))

if __name__ == '__main__':
    main(sys.argv[1])
