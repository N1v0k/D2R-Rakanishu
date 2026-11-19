#!/usr/bin/env python3









import json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import touch_layout

GROUPS = (('weap', 1), ('shld', 2), ('helm', 3), ('tors', 4), ('glov', 5), ('boot', 6), ('belt', 7))
CLASSES = {'ama': 0, 'sor': 1, 'nec': 2, 'pal': 3, 'bar': 4, 'dru': 5, 'ass': 6, 'war': 7}
INVISIBLE = {'helmet': 'data/hd/items/armor/helmet/cap_hat.json', 'shield': 'data/hd/items/armor/shield/buckler.json'}
KEEP = ('UnitRootComponent', 'SkeletonDefinitionComponent')


def excel(romfs, name):
    text = touch_layout.read_layout(romfs, None, suffix=f'/global/excel/{name}.txt')[1]
    lines = text.splitlines()
    head = lines[0].split('\t')
    return [dict(zip(head, line.split('\t'))) for line in lines[1:] if line.strip()]


def types(rows):
    by = {r['Code']: r for r in rows if r.get('Code')}

    def walk(code, seen=()):
        r = by.get(code)
        if not r or code in seen:
            return [code]
        out = [code]
        for e in (r.get('Equiv1', ''), r.get('Equiv2', '')):
            if e:
                out += walk(e, seen + (code,))
        return out

    def klass(code, seen=()):
        r = by.get(code)
        if not r or code in seen:
            return None
        if r.get('Class'):
            return r['Class']
        for e in (r.get('Equiv1', ''), r.get('Equiv2', '')):
            k = klass(e, seen + (code,)) if e else None
            if k:
                return k
        return None
    return walk, klass


def table_rows(weapons, armor, itemtypes, assets, sprites=None):


    walk, klass = types(itemtypes)
    stances, rows = {}, []
    for kind, src in (('w', weapons), ('a', armor)):
        for r in src:
            code = r.get('code', '')
            if not code or len(code) > 4 or r.get('quest', '0') not in ('', '0'):
                continue
            chain = set(walk(r['type'])) | (set(walk(r['type2'])) if r.get('type2') else set())
            group = next((g for t, g in GROUPS if t in chain), 0)
            if not group or (kind == 'w') != (group == 1):
                continue
            w, h = int(r['invwidth'] or 0), int(r['invheight'] or 0)
            if not (1 <= w <= 4 and 1 <= h <= 4):
                continue
            k = klass(r['type'])
            stance = 0
            if group == 1:
                key = (r.get('wclass', ''), r.get('2handed', '') or '0', r.get('2handedwclass', ''), r.get('1or2handed', '') or '0')
                stance = stances.setdefault(key, len(stances) + 1)
            rows.append((code, group, w, h, CLASSES.get(k, -1) if k else -1, stance))
    seen, first, out = set(), {}, []
    for row in rows:
        if row[0] not in seen:
            seen.add(row[0])
            asset = assets.get(row[0], row[0])
            pic = first.setdefault((asset,) + row[1:], row[0])
            icon = 'Items/%s/%s' % ('weapon' if row[1] == 1 else 'armor', asset)
            if sprites is not None and icon.lower() not in sprites:
                raise SystemExit('look_table: %s has no inventory picture %s' % (row[0], icon))
            out.append(row + (pic, icon))
    return out


def dye_rows(misc, gems, itemtypes, assets, sprites=None):


    walk, _ = types(itemtypes)
    rows = {r['code']: r for r in misc if r.get('code')}
    better = {c: r.get('BetterGem', '') for c, r in rows.items()}
    out = []
    for g in gems:
        code = g.get('code', '')
        r = rows.get(code)
        if not r or 'gem' not in walk(r.get('type', '')) or not g.get('transform', '').isdigit():
            continue
        colour = int(g['transform'])
        if not 0 <= colour <= 20:
            raise SystemExit('look_table: gem %s has the colour %d' % (code, colour))
        grade, c = 0, code
        while grade < 10:
            c = next((k for k, v in better.items() if v == c), None)
            if not c:
                break
            grade += 1
        icon = 'Items/misc/%s' % assets.get(code, code)
        if sprites is not None and icon.lower() not in sprites:
            raise SystemExit('look_table: gem %s has no inventory picture %s' % (code, icon))
        out.append((code, colour, grade, icon))
    return out


def sprite_paths(romfs):
    out = set()
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        p = line.split('|')[9].lower() if line.count('|') >= 10 else ''
        m = re.match(r'data:data/hd/global/ui/(items/.*)\.sprite$', p)
        if m:
            out.add(m.group(1))
    return out


def asset_map(romfs):
    text = touch_layout.read_layout(romfs, None, suffix='/hd/items/items.json')[1]
    out = {}
    for entry in json.loads(re.sub(r',(\s*[\]}])', r'\1', text)):
        for code, v in entry.items():
            if isinstance(v, dict) and v.get('asset'):
                out[code] = v['asset']
    return out


def invisible(text, name):
    d = json.loads(text); assert d.get('type') == 'UnitDefinition', name
    root = d['entities'][0]
    root['components'] = [c for c in root['components'] if c.get('type') in KEEP]
    d['entities'] = [root]
    for k in ('models', 'textures', 'particles', 'objecteffects'):
        d['dependencies'][k] = []
    d['name'] = name
    return json.dumps(d, separators=(',', ':'))


def build(romfs, outdir):

    rows = table_rows(excel(romfs, 'weapons'), excel(romfs, 'armor'), excel(romfs, 'itemtypes'), asset_map(romfs), sprite_paths(romfs))
    tsv = os.path.join(outdir, 'looks.tsv')
    with open(tsv, 'w') as f:
        f.write('# code\tgroup\tw\th\tclass\tstance\tpicture\ticon\n')
        for r in rows:
            f.write('\t'.join(str(x) for x in r) + '\n')
    misc = excel(romfs, 'misc')
    gems = dye_rows(misc, excel(romfs, 'gems'), excel(romfs, 'itemtypes'), asset_map(romfs), sprite_paths(romfs))
    with open(os.path.join(outdir, 'dyes.tsv'), 'w') as f:
        f.write('# gem code\tcolour\tgrade\ticon\n')
        for r in gems:
            f.write('\t'.join(str(x) for x in r) + '\n')
    out = {}
    for slot, src in INVISIBLE.items():
        text = touch_layout.read_layout(romfs, None, suffix='/' + src.split('/', 1)[1])[1]
        o = os.path.join(outdir, f'lookinv_{slot}.json')
        open(o, 'w').write(invisible(text, f'zinv_{slot}'))
        out[f'+data/hd/items/armor/zinv/{slot}.json'] = o
    return out, tsv, f'looks: {len(rows)} base types, {len(out)} invisible definitions, {len(gems)} gems that colour'


if __name__ == '__main__':
    print(build(*sys.argv[1:3])[2])
