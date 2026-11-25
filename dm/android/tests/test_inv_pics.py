#!/usr/bin/env python3


import json
import os
import pathlib
import sys
import tempfile
import types

dm = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(dm / 'android' / 'tools'))
sys.path.insert(0, str(dm / 'ios' / 'tools'))
import gen_inv_pics as g

DATA = {'i': {'b1': 'pic_b', 'b2': 'pic_c'}, 'u': {'7': 'own_u'}, 's': {'3': 'own_s'}, 'v': {'pic_b': 2}}
PICS = ('pic_b', 'pic_b1', 'pic_b2', 'pic_c', 'own_u', 'own_s')


def pack(root, data=DATA, pics=PICS):
    os.makedirs(os.path.join(root, 'inv'), exist_ok=True)
    with open(os.path.join(root, 'inv.json'), 'w', encoding='utf-8') as f:
        f.write(data if isinstance(data, str) else json.dumps(data))
    for n in pics:
        with open(os.path.join(root, 'inv', n + '.png'), 'wb') as f:
            f.write(b'x')


def main():
    with tempfile.TemporaryDirectory() as t:
        a = os.path.join(t, 'a'); pack(a)
        assert g.missing(a) == [], g.missing(a)
        os.remove(os.path.join(a, 'inv', 'pic_b2.png'))
        assert g.missing(a) == ['inv/pic_b2.png'], g.missing(a)
        open(os.path.join(a, 'inv', 'pic_b2.png'), 'wb').close()
        assert g.missing(a) == ['inv/pic_b2.png'], 'an empty picture counts as missing'
        os.remove(os.path.join(a, 'inv.json'))
        assert g.missing(a) == ['inv.json']
        for bad in ('{', '[]', json.dumps({'i': {}, 'u': {}, 's': {}, 'v': {}}), json.dumps(dict(DATA, v={'pic_b': 0})),
                    json.dumps(dict(DATA, u={'7': ''})), json.dumps({k: v for k, v in DATA.items() if k != 's'})):
            b = os.path.join(t, 'b'); pack(b, bad)
            assert g.missing(b) == ['inv.json'], (bad, g.missing(b))


        romfs_art = types.ModuleType('romfs_art')
        romfs_art.read = lambda romfs, path: (_ for _ in ()).throw(SystemExit('no such file: ' + path))
        sys.modules['romfs_art'] = romfs_art
        try:
            g.stage('romfs', os.path.join(t, 'c'))
        except (Exception, SystemExit):
            pass
        else:
            raise AssertionError('stage returned although the export failed')

        class Pic:
            def save(self, path):
                with open(path, 'wb') as f:
                    f.write(b'x')
        romfs_art.frame = lambda romfs, path, i: Pic()
        g.build = lambda romfs, read: (dict(DATA), {'pic_b': 'p', 'pic_b1': 'p', 'pic_c': 'p', 'own_u': 'p', 'own_s': 'p'})
        try:
            g.stage('romfs', os.path.join(t, 'd'))
        except SystemExit as e:
            assert 'pic_b2' in str(e), e
        else:
            raise AssertionError('stage passed with a variant picture missing')
        g.build = lambda romfs, read: (dict(DATA), {n: 'p' for n in PICS})
        g.stage('romfs', os.path.join(t, 'e'))
        assert g.missing(os.path.join(t, 'e')) == []
    print('PASS inv pics: incomplete item pictures stop the pack build')


if __name__ == '__main__':
    main()
