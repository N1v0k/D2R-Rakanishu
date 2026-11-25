#!/usr/bin/env python3
















import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
import gen_widestash_art as GW

ART = __import__('gen_paths').gen_dir('NativeHUD', 'mercslots')
DX, DY = 63, 11
INV_AT = (814, 95)
EQUIP_H, POTION = 392, (122, 345, 388)
EDGE = 8
BAND = (312, 422)
BAND_Y, ROW_H, ROW_TOPS = 420, 34, (448, 495, 543, 590, 638)

def main(romfs):
    L = GW.lib(); b = GW.orig(romfs, 'hirelinginventory/v2/hirelinginventorybg'); ver, fw, w, h = struct.unpack_from('<HHII', b, 4)
    assert b[:4] == b'SpA1' and ver == 61 and (fw, w, h) == (652, 652, 704), '佣兵面板底图不是 652×704（原版资源变了？）'
    src = GW.decode(L, b); inv = GW.decode(L, GW.orig(romfs, 'inventorypanel/v2/inventorybg_classic')); assert inv.size == (507, 700), '经典版格子图不是 507×700'
    mp = GW.decode(L, GW.orig(romfs, 'mainpanelbg')); assert mp.size == (2888, 881), '通用底板不是 2888×881'
    mp = mp.crop((mp.width // 2, 0, mp.width, mp.height))
    im = src.copy()
    sx, sy = INV_AT[0] - DX, INV_AT[1] - DY
    im.paste(mp.crop((sx + EDGE, sy + EDGE, sx + w - EDGE, sy + BAND_Y)).convert('RGBA'), (EDGE, EDGE))
    for x0, x1 in ((0, EDGE), (w - EDGE, w)): im.paste(src.crop((x0, 100, x1, 100 + BAND_Y - BAND[0])), (x0, BAND[0]))
    for box in ((0, 0, inv.width, POTION[1]), (0, POTION[1], POTION[0], EQUIP_H), (POTION[2], POTION[1], inv.width, EQUIP_H)):
        fr = inv.crop(box); im.paste(fr, (DX + box[0], DY + box[1]), fr)
    im.paste(src.crop((0, BAND[0], w, BAND[1])), (0, BAND_Y)); y = BAND_Y + BAND[1] - BAND[0]
    for t in ROW_TOPS: im.paste(src.crop((0, t, w, t + ROW_H)), (0, y)); y += ROW_H
    if y < h: im.paste(src.crop((0, h - (h - y), w, h)), (0, y))
    o = GW.pack(L, b, GW.bc3(im), w, h, w); open(os.path.join(ART, 'HirelingInventoryBG.sprite'), 'wb').write(o); im.save(os.path.join(ART, 'HirelingInventoryBG.png'))
    print('HirelingInventoryBG：%d×%d，%d 字节' % (w, h, len(o)))

if __name__ == '__main__': main(sys.argv[1])
