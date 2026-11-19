






import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'tools'))
sys.path.insert(0, HERE)
import gen_effect_data as g
import test_dcc

checks = fails = 0


def check(ok, what):
    global checks, fails
    checks += 1
    if not ok: fails += 1; print('FAIL', what)


def palette():

    cols = [(0, 0, 0)] * 256
    cols[1], cols[2], cols[3] = (100, 0, 0), (240, 200, 40), (60, 60, 60)
    return cols


def glow_file(frames, w, h):
    return test_dcc.encode([frames], w, h, (0, 1, 2, 3))


def main():

    files = {'global/overlays/CurseOld.dcc': b'A' * 40, 'global/overlays/HexNew.dcc': b'A' * 40, 'global/overlays/Own.dcc': b'B' * 40}
    rows = [{'overlay': 'curseold', 'Filename': 'CurseOld'}, {'overlay': 'shared', 'Filename': 'CurseOld'}, {'overlay': 'hexnew', 'Filename': 'HexNew'},
            {'overlay': 'own', 'Filename': 'Own'}, {'overlay': 'gone', 'Filename': 'Gone'}, {'overlay': '', 'Filename': 'x'}]
    idx = g.overlay_index(rows, files.get)
    check(not idx['curseold']['copy'] and idx['curseold']['data'] == b'A' * 40, 'the earlier of two equal files keeps its glow')
    check(idx['hexnew']['copy'], 'the later row with another name is a placeholder')
    check(not idx['shared']['copy'], 'the same file name in a second row is no copy')
    check(not idx['own']['copy'] and idx['gone']['data'] is None, 'an own file is no copy; a missing file has no data')
    check('' not in idx, 'a row without a name is skipped')
    rows.insert(0, rows.pop(2))
    idx2 = g.overlay_index(rows, files.get)
    check(not idx2['hexnew']['copy'] and idx2['curseold']['copy'], 'the original is whichever row comes first')


    skill, effect, item = ('skill', 'nec', 4), ('effect', 'Immune_Poison'), ('item', 'potion.sprite')
    own, copy = {'overlay1': 'Own'}, {'overlay1': 'hexnew'}
    check(g.sources('shrine', own, idx, None) == [('overlay', 'global/overlays/Own.dcc')], 'A: a shrine takes its glow')
    check(g.sources('shrine', copy, idx, None) == [] and g.sources('shrine', {'overlay1': 'gone'}, idx, None) == [], 'A: a placeholder or missing glow means no picture')
    check(g.sources('debuff', own, idx, skill) == [('overlay', 'global/overlays/Own.dcc'), skill], 'B: a debuff takes its glow, then its skill icon')
    check(g.sources('debuff', copy, idx, skill) == [skill] and g.sources('debuff', {'overlay1': ''}, idx, skill) == [skill], 'B: placeholder / no glow -> skill icon')
    check(g.sources('buff', own, idx, skill) == [skill] and g.sources('aura', own, idx, skill) == [skill], 'C: own buffs and auras take the skill icon even with a glow')
    check(g.sources('debuff', {}, idx, None, effect=effect) == [effect], 'D: poison / cold take the game icon')
    check(g.sources('potion', {}, idx, None, item=item) == [item], 'E: potions take the bottle')
    check(g.sources('buff', {}, idx, None) == [], 'no source: no picture')


    cols = palette()
    W, H = 8, 6
    dim = test_dcc.frame(W, H, lambda x, y: 1 if 2 <= x < 5 and 1 <= y < 5 else 0)
    bright = test_dcc.frame(W, H, lambda x, y: 2 if 3 <= x < 5 and 2 <= y < 4 else 3 if x == 6 and y == 1 else 0)
    im = g.glow(glow_file([dim, bright, dim], W, H), cols)

    check(im.size == (4, 4) and im.mode == 'RGBA', 'brightest frame cropped and padded square (%s)' % (im.size,))
    px = im.load()
    check(px[0, 1] == (255, 213, 43, 240), 'gold: alpha = max channel, colour scaled to full (%s)' % (px[0, 1],))
    check(px[3, 0] == (255, 255, 255, 60), 'grey: alpha 60, white (%s)' % (px[3, 0],))
    check(px[1, 3] == (0, 0, 0, 0), 'the padding row is transparent')
    empty = test_dcc.frame(2, 2, lambda x, y: 0)
    for name, data in (('empty 2x2', glow_file([empty], 2, 2)), ('too small', glow_file([test_dcc.frame(4, 4, lambda x, y: 2 if x == 1 and y < 2 else 0)], 4, 4)),
                       ('too dark', glow_file([dim], W, H))):
        try: g.glow(data, cols); check(False, name + ' is refused')
        except ValueError: check(True, '')
    check(len(g.palette(bytes(range(256)) * 3)) == 256 and g.palette(bytes([1, 2, 3]) * 256)[1] == (3, 2, 1) and g.palette(bytes(768))[0] == (0, 0, 0), 'palette: BGR, index 0 transparent')


    from PIL import Image
    def picture(spec):
        if spec[0] == 'overlay': raise ValueError('glow too dark')
        return Image.new('RGBA', (4, 4), (200, 10, 10, 255))
    with tempfile.TemporaryDirectory() as d:
        path = os.path.join(d, '9.png')
        check(g.save_picture([('overlay', 'x'), skill], path, picture) == skill and os.path.exists(path), 'B falls to the skill icon')
        path = os.path.join(d, '128.png')
        check(g.save_picture([('overlay', 'x')], path, picture) is None and not os.path.exists(path), 'A: no picture written')
        check(g.save_picture([skill], os.path.join(d, 'x.png'), lambda s: Image.new('RGBA', (4, 4))) is None, 'a fully transparent picture counts as none')
        def missing(spec):
            if spec[0] == 'item': raise SystemExit('no such file')
            return Image.new('RGBA', (4, 4), (9, 9, 9, 255))
        check(g.save_picture([('item', 'gone'), skill], os.path.join(d, 'y.png'), missing) == skill, 'a missing game file only moves on to the next source')


    import hashlib
    art = {128: [('overlay', 'global/overlays/Own.dcc')], 9: [('overlay', 'global/overlays/Own.dcc'), skill], 5: [skill], 6: [effect]}
    glows = {'global/overlays/Own.dcc': b'B' * 40}
    want = hashlib.sha256(b'global/overlays/own.dcc\0' + b'B' * 40).hexdigest()
    check(g.trusted_glows(art, glows, want) == art, 'the right glows are kept')
    check(g.trusted_glows(art, glows, 'f' * 64) == {128: [], 9: [skill], 5: [skill], 6: [effect]}, 'other glows: shrine without picture, debuff to its skill icon, the rest as it was')

    print('test_effect_art: %d checks, %d failed' % (checks, fails))
    sys.exit(1 if fails else 0)


if __name__ == '__main__':
    main()
