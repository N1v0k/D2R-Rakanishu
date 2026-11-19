#!/usr/bin/env python3







import json
import os
import re
import sys

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def name_of(asset):
    return asset.lower().replace('/', '__')


def table(read, name):
    raw = read('global/excel/%s.txt' % name).decode('utf-8', 'replace').splitlines()
    head = raw[0].split('\t')
    return [dict(zip(head, line.split('\t'))) for line in raw[1:] if line.strip()]


def hd(read, name):
    out = {}
    for entry in json.loads(read('hd/items/%s.json' % name).decode('utf-8-sig')):
        out.update(entry)
    return out


def own_key(index):

    return re.sub(r'[^a-z0-9]+', '_', index.lower().replace("'", '')).strip('_')


def build(romfs, read):
    import gen_recipe_data
    paths = gen_recipe_data.sprite_paths(romfs)
    base = {code: v['asset'].lower() for code, v in hd(read, 'items').items() if v.get('asset')}
    data = {'i': {c: name_of(a) for c, a in base.items() if a in paths}, 'u': {}, 's': {}, 'v': {}}
    for kind, txt, js, column in (('u', 'UniqueItems', 'uniques', 'code'), ('s', 'SetItems', 'sets', 'item')):
        own = hd(read, js)
        for r in table(read, txt):
            if not r.get('*ID', '').strip().isdigit() or not r.get('index'):
                continue
            asset = ((own.get(own_key(r['index'])) or {}).get('normal') or '').lower()
            if asset in paths:
                data[kind][r['*ID'].strip()] = name_of(asset)
    for asset in paths:
        n = 0
        while asset + str(n + 1) in paths:
            n += 1
        if n:
            data['v'][name_of(asset)] = n
    return data, paths


def pictures(data):

    names = set(data['i'].values()) | set(data['u'].values()) | set(data['s'].values())
    for name, n in data['v'].items():
        names.update(name + str(k) for k in range(1, n + 1))
    return names


def missing(out):


    try:
        with open(os.path.join(out, 'inv.json'), encoding='utf-8') as f:
            data = json.load(f)
        if not isinstance(data, dict) or any(not isinstance(data.get(k), dict) for k in 'iusv') or not data['i'] \
                or any(not isinstance(v, str) or not v for k in 'ius' for v in data[k].values()) \
                or any(type(n) is not int or n < 1 for n in data['v'].values()):
            return ['inv.json']
    except (OSError, ValueError):
        return ['inv.json']
    return sorted('inv/%s.png' % n for n in pictures(data) if not os.path.isfile(os.path.join(out, 'inv', n + '.png'))
                  or os.path.getsize(os.path.join(out, 'inv', n + '.png')) == 0)


def stage(romfs, out):

    import romfs_art
    read = lambda path: romfs_art.read(romfs, path)
    pics = os.path.join(out, 'inv')
    data, paths = build(romfs, read)
    os.makedirs(pics, exist_ok=True)
    for asset, path in sorted(paths.items()):
        romfs_art.frame(romfs, path, 0).save(os.path.join(pics, name_of(asset) + '.png'))
    with open(os.path.join(out, 'inv.json'), 'w', encoding='utf-8', newline='\n') as f:
        json.dump(data, f, sort_keys=True, separators=(',', ':'))
    lack = missing(out)
    if lack:
        raise SystemExit('appart: item pictures incomplete, missing %s' % ', '.join(lack[:8]))
    print('appart: %d item pictures, %d bases, %d uniques, %d set items, %d with variants' % (len(paths), len(data['i']), len(data['u']), len(data['s']), len(data['v'])))


if __name__ == '__main__':
    stage(sys.argv[1], sys.argv[2])
