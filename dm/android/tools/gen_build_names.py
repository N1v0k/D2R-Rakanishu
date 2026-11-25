#!/usr/bin/env python3
















import csv
import hashlib
import io
import json
import os
import re
import shutil
import struct
import sys

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

VERSION = '3.3.94012'
DATA_VERSION = 3
SCHEMA = 1
EXCEL = ('skills.txt', 'skilldesc.txt', 'uniqueitems.txt', 'setitems.txt', 'Runes.txt', 'armor.txt', 'weapons.txt', 'misc.txt', 'itemtypes.txt', 'hireling.txt', 'gems.txt',
         'properties.txt', 'itemstatcost.txt', 'cubemain.txt')
STRINGS = ('item-names', 'item-runes', 'item-gems', 'item-modifiers', 'item-nameaffixes', 'levels', 'ui', 'skills')


BINS = {'uniqueitems.bin': 0x15c, 'setitems.bin': 0x1cc, 'runes.bin': 0xe8}
TABLES = tuple('global/excel/' + f for f in EXCEL) + tuple('local/lng/strings/%s.json' % f for f in STRINGS) + ('hd/items/items.json', 'hd/items/uniques.json', 'hd/items/sets.json') + \
    tuple('global/excel/' + f for f in BINS)
TABLES_SHA = 'b2e74d9dff8f85ded64cab3dfdab71fcb6b39a7ff68a162cdb0f2fbacf6097d7'
RESULT_SHA = 'ab87179ce3acd10ca6b013a51621653faf805d0371956223b4aaad360bf78d7e'
CLASSES = (('ama', 'Amazon', 'Am'), ('sor', 'Sorceress', 'So'), ('nec', 'Necromancer', 'Ne'), ('pal', 'Paladin', 'Pa'), ('bar', 'Barbarian', 'Ba'),
           ('dru', 'Druid', 'Dr'), ('ass', 'Assassin', 'As'), ('war', 'Warlock', 'Wa'))
STAT_KEYS = {'str': 'strchrstr', 'dex': 'strchrdex', 'vit': 'strchrvit', 'ene': 'strchreng'}
DIFF_KEYS = ('strCreateGameNormalText', 'strCreateGameNightmareText', 'strCreateGameHellText')
QUALITY_KEYS = {'normal': 'NormalQuality', 'superior': 'LootFilterSuperiorRarity', 'magic': 'LootFilterMagicRarity', 'rare': 'LootFilterRareRarity',
                'set': 'LootFilterSetRarity', 'unique': 'LootFilterUniqueRarity'}

LABEL_KEYS = {'lvl': 'ItemStats1p', 'str': 'ItemStats1e', 'dex': 'ItemStats1f', 'def': 'ItemStats1h', 'dmg1': 'ItemStats1l', 'dmg2': 'ItemStats1m',
              'sock': 'Socketable', 'eth': 'strethereal'}
SYNERGY_COLUMNS = ('EDmgSymPerCalc', 'DmgSymPerCalc', 'ELenSymPerCalc')
OUT_ITEMS = ('amu', 'rin', 'cm1', 'cm2', 'cm3', 'jew')


class Refuse(Exception):
    """这个 RomFS 不出指南名字 | this RomFS gets no guide names"""


class Romfs:

    def __init__(self, romfs):
        import touch_layout
        self.romfs, self.entry, self.index = romfs, touch_layout._read_entry, {}
        with open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8') as f:
            for line in f:
                p = line.rstrip('\n').split('|')
                if len(p) >= 11: self.index.setdefault(p[9].lower().split(':', 1)[-1], p)

    def read(self, path):
        want = path.lower().lstrip('/')
        if not want.startswith('data/'): want = 'data/' + want
        if want not in self.index: raise Refuse('no %s in the game files' % path)
        return self.entry(self.romfs, self.index[want])

    def has(self, path):
        want = path.lower().lstrip('/')
        return ('data/' + want if not want.startswith('data/') else want) in self.index

    def frame(self, path, i):

        import romfs_art
        from PIL import Image
        b = self.read(path)
        if b[:4] != b'SpA1': raise Refuse('%s is not a sprite' % path)
        ver, fw, w, h = struct.unpack_from('<HHII', b, 4)
        if ver == 61: im = romfs_art.dxt5(b, w, h)
        elif ver == 31: im = Image.frombytes('RGBA', (w, h), b[40:40 + w * h * 4])
        else: raise Refuse('%s: sprite version %d' % (path, ver))


        pitch = fw if im.width % fw == 0 else (fw + 3) // 4 * 4
        if im.width % pitch or (i + 1) * pitch > im.width: raise Refuse('%s has no frame %d' % (path, i))
        return im.crop((i * pitch, 0, i * pitch + fw, im.height))


def table(raw):
    rows = list(csv.reader(io.StringIO(raw.decode('utf-8', 'replace')), delimiter='\t', quoting=csv.QUOTE_NONE))
    return [dict(zip(rows[0], r)) for r in rows[1:]]


def text(pair):

    return [re.sub(r'\s+', ' ', s.replace('-\n', '').replace('\n', ' ')).strip() for s in pair]


def bin_rows(raw, size, rows, name):


    n = struct.unpack_from('<I', raw, 0)[0] if len(raw) >= 4 else -1
    if n < rows or len(raw) != 4 + n * size: raise Refuse('%s: %d records in %d bytes, want at least %d of %d bytes' % (name, n, len(raw), rows, size))
    return lambda i: raw[4 + i * size:4 + (i + 1) * size]


def item_row(rec, row, code, name):


    if not str(row).isdigit(): raise Refuse('%s: no row for %s' % (name, code))
    got = rec(int(row))[0x28:0x2c]
    if got != code.encode('ascii').ljust(4, b' '): raise Refuse('%s: record %s has %r, the table says %s' % (name, row, got, code))
    return int(row)


def word_row(rec, row, r):


    b = rec(row)
    runes = sum(1 for k in range(6) if struct.unpack_from('<i', b, 0x5c + 4 * k)[0] >= 1)
    if b[0x44] != int(r.get('complete') or 0) or runes != sum(1 for k in range(1, 7) if r.get('Rune%d' % k)): raise Refuse('runes.bin record %d differs from Runes.txt' % row)
    return row


def num_pair(r, lo, hi):

    a, b = (r.get(lo) or '').strip(), (r.get(hi) or '').strip()
    if not (a.isdigit() and b.isdigit()) or int(b) == 0: return None
    return [int(a), int(b)]


def build(rf):
    import gen_recipe_data as grd
    digest = hashlib.sha256()
    for path in TABLES: digest.update(rf.read(path))
    if digest.hexdigest() != TABLES_SHA: raise Refuse('the tables are not those of %s (%s)' % (VERSION, digest.hexdigest()))
    read = rf.read
    c = grd.Catalog(read)
    name = lambda key: text(c.names[key]) if key in c.names else None
    t = {f: table(read('global/excel/' + f)) for f in ('skills.txt', 'skilldesc.txt', 'hireling.txt', 'setitems.txt', 'Runes.txt', 'uniqueitems.txt', 'armor.txt', 'weapons.txt')}
    ids = lambda f: [int(r['*ID']) for r in t[f] if r.get('*ID', '').isdigit()]
    uniq_rec = bin_rows(read('global/excel/uniqueitems.bin'), BINS['uniqueitems.bin'], max(ids('uniqueitems.txt')) + 1, 'uniqueitems.bin')
    set_rec = bin_rows(read('global/excel/setitems.bin'), BINS['setitems.bin'], max(ids('setitems.txt')) + 1, 'setitems.bin')
    word_rec = bin_rows(read('global/excel/runes.bin'), BINS['runes.bin'], len(t['Runes.txt']), 'runes.bin')
    desc = {r['skilldesc']: r for r in t['skilldesc.txt'] if r.get('skilldesc')}
    by_key = {r['skill']: r for r in t['skills.txt'] if r.get('*Id', '').isdigit()}
    out = {'version': VERSION, 'dataVersion': DATA_VERSION, 'schema': SCHEMA, 'classes': {}, 'skills': {}, 'items': {}, 'stats': {}, 'difficulties': [], 'qualities': {}, 'auras': {}}
    for i, (code, key, short) in enumerate(CLASSES):
        n = name('partychar' + code)
        if not n: raise Refuse('no name for class %s' % code)
        tabs = {}
        for page in (1, 2, 3):





            num = page if code == 'war' else 4 - page
            tn = name('SkillCategory%s%d' % (short, num))
            if not tn: raise Refuse('no tree name %s%d' % (short, num))
            tabs[str(page)] = tn
        out['classes'][code] = {'index': i, 'name': n, 'tabs': tabs, 'icons': 'hd/global/ui/Spells/%s/%sSkillIcon.sprite' % (key, short)}
    icons = {}
    for r in t['skills.txt']:
        if not r.get('*Id', '').isdigit() or r.get('charclass') not in out['classes']: continue
        d = desc.get(r['skilldesc'])
        if not d or not d.get('SkillPage', '').isdigit() or d['SkillPage'] == '0': continue
        sid = int(r['*Id'])
        n = name(d.get('str name', ''))
        if not n: raise Refuse('no name for skill %s' % r['skill'])
        pre = []
        for k in (1, 2, 3):
            q = r.get('reqskill%d' % k)
            if q:
                if q not in by_key: raise Refuse('skill %s: unknown prerequisite %s' % (r['skill'], q))
                pre.append(int(by_key[q]['*Id']))
        syn = []
        for col in SYNERGY_COLUMNS:
            for s in re.findall(r"skill\('([^']+)'\.blvl\)", r.get(col, '')):
                if s in by_key and int(by_key[s]['*Id']) not in syn: syn.append(int(by_key[s]['*Id']))
        out['skills'][str(sid)] = {'key': r['skill'], 'class': r['charclass'], 'page': int(d['SkillPage']), 'row': int(d['SkillRow']), 'col': int(d['SkillColumn']),
                                   'req': int(r['reqlevel'] or 0), 'pre': pre, 'max': int(r['maxlvl'] or 0), 'icon': int(d['IconCel'] or 0), 'syn': syn, 'name': n,
                                   'nk': d['str name']}
        icons[sid] = (out['classes'][r['charclass']]['icons'], int(d['IconCel'] or 0))
    for st, key in STAT_KEYS.items():
        if not name(key): raise Refuse('no stat name %s' % key)
        out['stats'][st] = name(key)
    out['difficulties'] = [name(k) for k in DIFF_KEYS]
    for q, key in QUALITY_KEYS.items():
        if not name(key): raise Refuse('no quality name %s' % key)
        out['qualities'][q] = name(key)
    if None in out['difficulties']: raise Refuse('no difficulty names')
    out['labels'] = {}
    for k, key in LABEL_KEYS.items():
        if not name(key): raise Refuse('no label %s' % key)
        out['labels'][k] = name(key)
    for h in t['hireling.txt']:
        for k in range(1, 7):
            s = h.get('Skill%d' % k)
            if s and s in by_key and s not in out['auras']:
                d = desc.get(by_key[s]['skilldesc'], {})
                n = name(d.get('str name', ''))
                if n: out['auras'][s] = n
    items = out['items']
    pics = set(OUT_ITEMS)
    armor, weapons = {r['code'] for r in t['armor.txt'] if r.get('code')}, {r['code'] for r in t['weapons.txt'] if r.get('code')}
    uniq_art = {}
    for code, r in c.items.items():
        if not c.items[code].get('type'): continue
        try: n = text(c.item_name(code))
        except grd.Refuse: continue
        e = {'name': n, 'lvl': int(r.get('levelreq') or 0), 'str': int(r.get('reqstr') or 0), 'dex': int(r.get('reqdex') or 0), 'sockets': int(r.get('gemsockets') or 0),
             'type': r['type'], 'img': code}
        if r.get('type2'): e['type2'] = r['type2']


        if code in armor and num_pair(r, 'minac', 'maxac'): e['ac'] = num_pair(r, 'minac', 'maxac')
        if code in weapons:
            if num_pair(r, 'mindam', 'maxdam'): e['d1'] = num_pair(r, 'mindam', 'maxdam')
            if num_pair(r, '2handmindam', '2handmaxdam'): e['d2'] = num_pair(r, '2handmindam', '2handmaxdam')
        if code + 'L' in c.names: e['short'] = text(c.names[code + 'L'])
        if (r.get('gemapplytype') or '').isdigit(): e['gat'] = int(r['gemapplytype'])
        items['i:' + code] = e
        if code in OUT_ITEMS or code.startswith('r') and code[1:].isdigit() and len(code) == 3: pics.add(code)
    for f in ('armor.txt', 'weapons.txt'):
        for r in table(read('global/excel/' + f)):
            if r.get('code') and 'i:' + r['code'] in items: pics.add(r['code'])





    assets = {k: v['asset'] for e in json.loads(read('hd/items/items.json').decode('utf-8-sig')) for k, v in e.items()}
    own, sprites = {}, grd.sprite_paths(rf.romfs)
    def own_art(prefix, fn, index, code):
        m = own.setdefault(fn, {k: v for e in json.loads(read(fn).decode('utf-8-sig')) for k, v in e.items()})
        key = re.sub(r'[^a-z0-9_]', '', index.lower().replace(' ', '_').replace('-', '_'))
        a = (m.get(key) or {}).get('normal', '')
        if not a or a.lower() == assets.get(code, '').lower() or a.lower() not in sprites: return code
        img = prefix + key; uniq_art[img] = a.lower(); return img
    for index, r in c.uniques.items():
        if r.get('disabled') == '1' or index not in c.names or r.get('code') not in c.items: continue
        items['u:' + index] = {'name': text(c.names[index]), 'lvl': int(r.get('lvl req') or 0), 'code': r['code'], 'img': own_art('u_', 'hd/items/uniques.json', index, r['code']),
                               'row': item_row(uniq_rec, r.get('*ID'), r['code'], 'uniqueitems.bin')}
        if any(r.get('prop%d' % k) == 'ease' for k in range(1, 13)): items['u:' + index]['ease'] = 1
    for r in t['setitems.txt']:
        index = r.get('index')
        if not index or r.get('disabled') == '1' or index not in c.names or r.get('item') not in c.items: continue
        items['s:' + index] = {'name': text(c.names[index]), 'lvl': int(r.get('lvl req') or 0), 'code': r['item'], 'img': own_art('s_', 'hd/items/sets.json', index, r['item']),
                               'row': item_row(set_rec, r.get('*ID'), r['item'], 'setitems.bin')}
        if any(r.get('prop%d' % k) == 'ease' for k in range(1, 10)): items['s:' + index]['ease'] = 1
    for row, r in enumerate(t['Runes.txt']):
        key = r.get('Name')
        if not key or r.get('complete') != '1' or key not in c.names or not c.raw.get(key): continue
        runes = [r['Rune%d' % k] for k in range(1, 7) if r.get('Rune%d' % k)]
        items['r:' + key] = {'name': text((grd.word_form(c.raw[key]), c.names[key][1])), 'runes': runes, 'lvl': max(int(c.items[x]['levelreq'] or 0) for x in runes),
                             'itypes': [r['itype%d' % k] for k in range(1, 7) if r.get('itype%d' % k)], 'etypes': [r['etype%d' % k] for k in range(1, 4) if r.get('etype%d' % k)],
                             'row': word_row(word_rec, row, r)}


        rune_ease = any(v == 'ease' for x in runes for kk, v in c.gems.get(x, {}).items() if kk.endswith('Code'))
        if rune_ease or any(r.get('T1Code%d' % k) == 'ease' for k in range(1, 8)): items['r:' + key]['ease'] = 1
    for tcode in c.types:
        try:
            ty = c.use_type(tcode)
        except grd.Refuse: continue
        items['t:' + tcode] = {'name': text((ty['de'], ty['en'])), 'img': ty['img'], 'up': ty['up']}


        gats = {items['i:' + x].get('gat') for x in c.items if 'i:' + x in items and c.is_type(x, tcode)}
        if len(gats) == 1 and None not in gats: items['t:' + tcode]['gat'] = gats.pop()
        pics.add(ty['img'])
    return out, icons, sorted(p for p in pics if 'i:' + p in items), uniq_art


def art(rf, icons, pics, out, uniq_art=None):
    import gen_recipe_data as grd
    assets = {k: v['asset'] for e in json.loads(rf.read('hd/items/items.json').decode('utf-8-sig')) for k, v in e.items()}
    paths = grd.sprite_paths(rf.romfs)
    os.makedirs(os.path.join(out, 'skills')); os.makedirs(os.path.join(out, 'items'))
    sheets = {}
    for sid, (sheet, cel) in sorted(icons.items()):
        if sheet not in sheets: sheets[sheet] = rf.read(sheet)
        rf.frame(sheet, cel).save(os.path.join(out, 'skills', '%d.png' % sid))
    made = 0
    for code in pics:
        path = paths.get(assets.get(code, '').lower())
        if not path: continue
        rf.frame(path, 0).save(os.path.join(out, 'items', code + '.png')); made += 1
    for img, asset in sorted((uniq_art or {}).items()):
        path = paths.get(asset)
        if path: rf.frame(path, 0).save(os.path.join(out, 'items', img + '.png')); made += 1
    return made


def result(rf):


    data, icons, pics, uniq_art = build(rf)
    got = hashlib.sha256(json.dumps(data, sort_keys=True, ensure_ascii=False, separators=(',', ':')).encode('utf-8')).hexdigest()
    if got != RESULT_SHA: raise Refuse('the guide names are not those of %s (%s)' % (VERSION, got))
    return data, icons, pics, uniq_art


def verified(romfs):


    return result(Romfs(romfs))[0]


def stage(romfs, appart):

    out = os.path.join(appart, 'builds')
    shutil.rmtree(out, ignore_errors=True)
    try:
        rf = Romfs(romfs)
        data, icons, pics, uniq_art = result(rf)
        os.makedirs(out)
        n = art(rf, icons, pics, out, uniq_art)
        with open(os.path.join(out, 'gamenames.json'), 'w', encoding='utf-8', newline='\n') as f: json.dump(data, f, ensure_ascii=False, separators=(',', ':'), sort_keys=True)
    except (Exception, SystemExit) as e:
        shutil.rmtree(out, ignore_errors=True)
        print('build guides: names not built (%r), the page shows English keys' % (e,))
        return False
    print('Build-Guides: %d Fertigkeiten, %d Gegenstaende/Typen, %d Bilder' % (len(data['skills']), len(data['items']), len(icons) + n))
    return True


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--measure':
        rf = Romfs(sys.argv[2])
        d = hashlib.sha256()
        for p in TABLES: d.update(rf.read(p))
        print('TABLES_SHA', d.hexdigest())
        TABLES_SHA = d.hexdigest()
        data, _, pics, _ = build(rf)
        print('RESULT_SHA', hashlib.sha256(json.dumps(data, sort_keys=True, ensure_ascii=False, separators=(',', ':')).encode('utf-8')).hexdigest(), len(data['skills']), len(data['items']), len(pics))
        sys.exit(0)
    sys.exit(0 if stage(sys.argv[1], sys.argv[2]) else 1)
