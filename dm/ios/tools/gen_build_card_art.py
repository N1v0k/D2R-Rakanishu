#!/usr/bin/env python3







import os
import sys
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'android', 'tools'))
import gen_widestash_art as G
import profile_hd
import romfs_art
import touch_layout
from gen_paths import gen_dir

INSET = 'hd/global/ui/Controller/Panel/CharacterSheet/V2/CharacterSheet_Inset.sprite'
CORNER = 8
SOLID = 128


def card(romfs):
    _, _, width = touch_layout.build_card_place(romfs)
    w, h = width // 2, touch_layout.BUILD_CARD_H // 2
    named = profile_hd.chain(lambda path: romfs_art.read(romfs, path))
    tip = named.get('TooltipStyle')
    fill = profile_hd.colour(tip.get('backgroundColor') if isinstance(tip, dict) else None, named)
    if fill is None: raise SystemExit('gen_build_card_art: the game profile has no TooltipStyle.backgroundColor')
    inset = romfs_art.frame(romfs, INSET, 0)
    rim = inset.crop(inset.split()[3].point(lambda v: 255 if v >= SOLID else 0).getbbox())
    rw, rh = rim.size
    if rw <= 2 * CORNER or rh <= 2 * CORNER: raise SystemExit('gen_build_card_art: inset rim %dx%d is smaller than its corners' % (rw, rh))
    edge = Image.new('RGBA', (w, h))
    cols = ((0, CORNER, 0, CORNER), (CORNER, rw - CORNER, CORNER, w - CORNER), (rw - CORNER, rw, w - CORNER, w))
    rows = ((0, CORNER, 0, CORNER), (CORNER, rh - CORNER, CORNER, h - CORNER), (rh - CORNER, rh, h - CORNER, h))
    for i, (sx0, sx1, dx0, dx1) in enumerate(cols):
        for j, (sy0, sy1, dy0, dy1) in enumerate(rows):
            if i == j == 1: continue
            edge.paste(rim.crop((sx0, sy0, sx1, sy1)).resize((dx1 - dx0, dy1 - dy0), Image.BILINEAR), (dx0, dy0))
    return Image.alpha_composite(Image.new('RGBA', (w, h), tuple(fill)), edge)


def main(romfs):
    img = card(romfs)
    p = os.path.join(gen_dir('NativeHUD', 'touch'), 'build_card.sprite')
    open(p, 'wb').write(G.pack(G.lib(), romfs_art.read(romfs, INSET), G.bc3(img), img.width, img.height, img.width))
    print('build_card: %dx%d -> %s' % (img.width, img.height, p))


if __name__ == '__main__':
    main(sys.argv[1])
