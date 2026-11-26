#!/usr/bin/env python3








import csv
import io
import json
import os
import sys

VERSION = '3.4.95035'
SCHEMA = 1
MAX_LEVEL = 110
DIFFS = ('', '(N)', '(H)')


class Refuse(Exception):
    """这份表不出术语表的数 | these tables give no glossary numbers"""


def table(raw):
    return list(csv.DictReader(io.StringIO(raw.decode('utf-8-sig')), delimiter='\t'))


def number(row, col):
    v = (row.get(col) or '').strip()
    if not v.lstrip('-').isdigit(): raise Refuse('%s is not a number in level %s' % (col, row.get('Level')))
    return int(v)


def build(monlvl, difficulty):

    rows = {}
    for r in table(monlvl):
        lv = (r.get('Level') or '').strip()
        if lv.isdigit() and int(lv) <= MAX_LEVEL: rows[int(lv)] = r
    if sorted(rows) != list(range(MAX_LEVEL + 1)): raise Refuse('monlvl.txt does not have every level 0-%d' % MAX_LEVEL)
    out = {'version': VERSION, 'schema': SCHEMA}
    for key, prefix in (('expansion', 'L-'), ('classic', '')):
        out[key] = {'ar': [[number(rows[lv], prefix + 'TH' + d) for lv in range(MAX_LEVEL + 1)] for d in DIFFS],
                    'def': [[number(rows[lv], prefix + 'AC' + d) for lv in range(MAX_LEVEL + 1)] for d in DIFFS]}
    div = {r.get('Name'): r for r in table(difficulty)}
    leech = []
    for name in ('Normal', 'Nightmare', 'Hell'):
        if name not in div: raise Refuse('difficultylevels.txt has no %s' % name)
        leech.append([number(div[name], 'LifeStealDivisor'), number(div[name], 'ManaStealDivisor')])
    if any(x < 1 for p in leech for x in p): raise Refuse('a leech divisor below 1')
    out['leech'] = leech
    return out


def stage(romfs, appart):

    path = os.path.join(appart, 'glossary.json')
    try:
        os.remove(path)
    except OSError:
        pass
    try:
        import gen_build_names
        rf = gen_build_names.Romfs(romfs)
        data = build(rf.read('global/excel/monlvl.txt'), rf.read('global/excel/difficultylevels.txt'))
        os.makedirs(appart, exist_ok=True)
        with open(path, 'w', encoding='utf-8', newline='\n') as f: json.dump(data, f, separators=(',', ':'), sort_keys=True)
    except (Exception, SystemExit) as e:
        try:
            os.remove(path)
        except OSError:
            pass
        print('glossary: monster values not built (%r), the glossary shows no hit chances' % (e,))
        return False
    print('Glossar: Monsterwerte fuer %d Stufen' % (MAX_LEVEL + 1))
    return True


if __name__ == '__main__':
    if len(sys.argv) != 3: sys.exit('usage: gen_glossary_data.py <RomFS> <appart directory>')
    sys.exit(0 if stage(sys.argv[1], sys.argv[2]) else 1)
