#!/usr/bin/env python3














import json
import os
import shutil
import sys

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
BASE, FILL = 'hd/global/ui/Controller/HUD/HUDBase.sprite', 'hd/global/ui/Controller/HUD/ExpBar.sprite'
BASE_SIZE, FILL_SIZE = (1582, 206), (938, 11)
ROW0, ROW1 = 84, 118
COL0, COL1 = 303, 1290
CAP_L, CAP_R = 323, 1270
STUD0, PITCH, STUD_W, STUDS = 409, 96, 8, 9
FILL_X, FILL_Y = 327.5, 99.5
VERSION = 1


def edges():

    return [COL0, CAP_L] + [x for k in range(STUDS) for x in (STUD0 + PITCH * k, STUD0 + PITCH * k + STUD_W)] + [CAP_R, COL1]


def runs(mask):

    out, start = [], None
    for x, v in enumerate(mask):
        if v and start is None: start = x
        if not v and start is not None: out.append((start, x)); start = None
    if start is not None: out.append((start, len(mask)))
    return out


def check(base, fill):

    import numpy as np
    if base.size != BASE_SIZE or fill.size != FILL_SIZE: raise ValueError('sizes %s %s' % (base.size, fill.size))
    b = np.asarray(base.convert('RGBA')).astype(float); a = b[:, :, 3]; luma = b[:, :, :3].mean(axis=2)
    for k in range(STUDS):
        c = STUD0 + PITCH * k + STUD_W / 2
        tip = a[85:89, int(c) - 8:int(c) + 8].sum(axis=0)
        if tip.sum() < 1000: raise ValueError('no spike at stud %d' % k)
        centre = (tip * np.arange(int(c) - 8, int(c) + 8)).sum() / tip.sum() + 0.5
        if abs(centre - c) > 1.5: raise ValueError('spike %d at %.1f, expected %.1f' % (k, centre, c))
        if a[84:88, int(c) + 40:int(c) + 56].max() > 32: raise ValueError('not transparent between spikes %d and %d' % (k, k + 1))
        if luma[102:108, int(c) - 3:int(c) + 3].mean() < 60: raise ValueError('stud %d not where expected' % k)
    e = edges()
    for i in range(1, len(e) - 1, 2):
        if luma[102:108, e[i] + 8:e[i + 1] - 8].mean() > 40: raise ValueError('gutter %d is not the dark channel' % (i // 2))
    if a[90:ROW1, COL0:COL1].min() < 192: raise ValueError('rail not opaque')
    fa = np.asarray(fill.convert('RGBA'))[:, :, 3]
    pieces = runs(fa.max(axis=0) > 0)
    if len(pieces) != 10: raise ValueError('fill has %d pieces, expected 10' % len(pieces))
    for k in range(STUDS):
        gap = (pieces[k][1] + pieces[k + 1][0]) / 2 + FILL_X
        if abs(gap - (STUD0 + PITCH * k + STUD_W / 2)) > 1.5: raise ValueError('fill gap %d at %.1f is not on its stud' % (k, gap))
    core = fa.mean(axis=1)
    centre = (core * np.arange(len(core))).sum() / core.sum() + 0.5 + FILL_Y
    if abs(centre - 105) > 1: raise ValueError('fill line centre at row %.1f, the channel is at 105' % centre)


def build(base, fill, out):

    os.makedirs(out)
    base.convert('RGBA').crop((COL0, ROW0, COL1, ROW1)).save(os.path.join(out, 'rail.png'))
    fill.convert('RGBA').save(os.path.join(out, 'fill.png'))
    meta = {'v': VERSION, 'rail': 'rail.png', 'fill': 'fill.png', 'edges': [x - COL0 for x in edges()], 'fill_x': FILL_X - COL0, 'fill_y': FILL_Y - ROW0}
    with open(os.path.join(out, 'rail.json'), 'w', encoding='utf-8', newline='\n') as f: json.dump(meta, f, separators=(',', ':'))


def remove(path):

    try:
        if os.path.isdir(path) and not os.path.islink(path): shutil.rmtree(path, ignore_errors=True)
        elif os.path.lexists(path): os.remove(path)
    except OSError: pass


def stage(romfs, out, read=None):


    tmp = out + '.part'
    try:
        if read is None:
            import romfs_art
            read = lambda path: romfs_art.sprite(romfs, path)
        base, fill = read(BASE), read(FILL)
        check(base, fill)
        remove(tmp); os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        build(base, fill, tmp)
        remove(out); os.rename(tmp, out)
    except (Exception, SystemExit) as e:
        remove(tmp); remove(out)
        print('appart/exp: rail not made (%r); the App draws the flat experience bar' % (e,))
        return False
    print('appart/exp: original experience rail made (10 gutters, 9 studs)')
    return True


if __name__ == '__main__':
    sys.exit(0 if stage(sys.argv[1], sys.argv[2]) else 1)
