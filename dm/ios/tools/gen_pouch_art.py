#!/usr/bin/env python3






import os, sys
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE); sys.path.insert(0, os.path.join(HERE, '..', '..', 'tools'))
import gen_widestash_art as G, dmx
from PIL import Image

W, CAP = 140, 24

def main():
    dmx.ROOT = os.path.join(sys.argv[1], 'data'); L = G.lib()
    src = dmx.get('data/hd/global/ui/controller/panel/inventorypanel/v2/inventorybg_classic.sprite'); im = G.decode(L, src)
    fr = im.crop((152, 628, 352, 692)); h = fr.height
    out = Image.new('RGBA', (W, h)); out.paste(fr.crop((0, 0, CAP, h)), (0, 0)); out.paste(fr.crop((fr.width - CAP, 0, fr.width, h)), (W - CAP, 0))
    out.paste(fr.crop((CAP, 0, fr.width - CAP, h)).resize((W - 2 * CAP, h), Image.LANCZOS), (CAP, 0))
    p = os.path.join(__import__('gen_paths').gen_dir('NativeHUD', 'pouch'), 'PouchBox.sprite'); open(p, 'wb').write(G.pack(L, src, G.bc3(out), W, h, W))
    print('PouchBox：%d×%d 像素（布局 %d×%d 单位）→ %s' % (W, h, 2 * W, 2 * h, p))

if __name__ == '__main__':
    main()
