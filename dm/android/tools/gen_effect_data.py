#!/usr/bin/env python3





import csv
import hashlib
import io
import json
import os
import re
import shutil
import sys

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))

VERSION = '3.3.94012'
STATES = 232
STRINGS = ('skills', 'shrines', 'item-modifiers', 'ui')
PALETTE = 'global/palette/ACT1/pal.dat'
TABLES = tuple('global/excel/%s.txt' % f for f in ('states', 'itemstatcost', 'skills', 'skilldesc', 'misc', 'shrines', 'overlay', 'difficultylevels', 'experience')) + tuple('local/lng/strings/%s.json' % f for f in STRINGS + ('item-names',)) + ('hd/items/items.json', PALETTE)
TABLES_SHA = '457e5dec1a0e3c34370bfd58108a8be00b8e4c284e991ca0681663ba8ca7d394'

ART_SHA = '96b6a3c56b79dc4c54fcb086e34a6577d5bd6eb2b655b56a117a5052c0658307'
OVERLAYS = 'global/overlays/%s.dcc'


ATLASES = {'ama': 'amazon/AmSkillIcon', 'sor': 'Sorceress/SoSkillIcon', 'nec': 'Necromancer/NeSkillIcon', 'pal': 'paladin/PaSkillIcon', 'bar': 'Barbarian/BaSkillIcon',
           'dru': 'Druid/DrSkillIcon', 'ass': 'Assassin/AsSkillIcon', 'war': 'Warlock/WaSkillIcon'}
PITCH, WIDTH, TRIM = 68, 65, 0.08


FILL = ('skill',)
EFFECT_ICONS = 'hd/global/ui/Controller/HUD/EffectIcons/%s.sprite'



VISIBLE, BRIGHT, SMALLEST = 24, 170, 3


CURED = {True: ('StrSklTree17', 'Immune_Poison'), False: ('StrSklTree23', 'Immune_Ice')}

SAME_AS = {'skill_armor_percent': 'item_armor_percent', 'velocitypercent': 'item_fastermovevelocity', 'attackrate': 'item_fasterattackrate', 'other_animrate': 'item_fastercastrate'}
BINDING = '19'
TEMPLATE = {'19', '29'}


WORDS = {'remaining': 'SimpleTimeRemainingFormatTooltipString', 'minutes': 'timeMinutesFormatString', 'level': 'strchrlvl', 'life': 'strchrlif', 'mana': 'strchrman',
         'shrine': 'Shrine3wilderness', 'skill': 'StrSklTree1', 'poison': 'StrSklTree17'}



CARD_WORDS = {'allskills': 'ModStr3k', 'stamina': 'strchrstm', 'experience': 'strchrexp', 'res39': 'strchrfir', 'res41': 'strchrlit', 'res43': 'strchrcol', 'res45': 'strchrpos',
              'attr0': 'strchrstr', 'attr1': 'strchreng', 'attr2': 'strchrdex', 'attr3': 'strchrvit',
              'range21': 'strModMinDamageRange', 'range48': 'strModFireDamageRange', 'range50': 'strModLightningDamageRange', 'range52': 'strModMagicDamageRange',
              'range54': 'strModColdDamageRange', 'range57': 'strModPoisonDamageRange',



              'defense': 'strchrdef', 'attackrating': 'strchratr', 'damage': 'strchrskm', 'attack': 'skillname0'}

CLASSES = ('Amazon', 'Sorceress', 'Necromancer', 'Paladin', 'Barbarian', 'Druid', 'Assassin', 'Warlock')
DIFFICULTIES = ('Normal', 'Nightmare', 'Hell')


DESC_LINES = ('36', '74', '75')
PARAMS, CALCS = 12, 4
SHEET_BLUE = ('attblue', 'damblue', 'armblue')


PROFILE = 'global/ui/layouts/_profilehd.json'
STYLE = {'title': 'UniqueColor', 'magic': 'MagicColor', 'sub': 'substringColor', 'text': 'DefaultColor'}
STYLE_FONT = {'red': 'FontColorRed'}


class Refuse(Exception):
    """这个 RomFS 不出状态效果。 | This RomFS gets no effects."""


def clean(s):
    m = re.match(r'\[\w\w\]([^\[]*)', s)
    s = (m.group(1) if m else s).strip()
    return s if '%' in s else s.rstrip('.')


def table(data):
    rows = list(csv.reader(io.StringIO(data.decode('utf-8', 'replace')), delimiter='\t', quoting=csv.QUOTE_NONE))
    return [dict(zip(rows[0], r)) for r in rows[1:]]


def strings(read):
    out = {}
    for f in STRINGS + ('item-names',):
        for e in json.loads(read('local/lng/strings/%s.json' % f).decode('utf-8-sig')):
            if 'Key' in e: out.setdefault(e['Key'], (clean(e['deDE']), clean(e['enUS'])))
    return out


def op_sources(rows):



    ids = {r['Stat']: i for i, r in enumerate(rows)}
    out = {}
    for i, r in enumerate(rows):
        if not r.get('op', '').strip(): continue
        for c in ('op stat1', 'op stat2', 'op stat3'):
            t = r.get(c, '').strip()
            if t in ids: out.setdefault(str(ids[t]), []).append(i)
    return out


def stat_rows(names, rows):

    by_name = {r['Stat']: r for r in rows}
    out = {}
    for i, r in enumerate(rows):
        src = by_name.get(SAME_AS.get(r['Stat'], r['Stat']), r)
        if src['descfunc'] not in TEMPLATE or src['descstrpos'] not in names: continue
        pos, neg = names[src['descstrpos']], names.get(src['descstrneg'], names[src['descstrpos']])
        out[str(i)] = {'de': pos[0], 'en': pos[1], 'nde': neg[0], 'nen': neg[1], 'shift': int(r['ValShift'] or 0)}
    return out


def stat_groups(names, rows):



    members = {}
    for i, r in enumerate(rows):
        g = (r.get('dgrp') or '').strip()
        if g.isdigit() and int(g) > 0: members.setdefault(g, []).append((i, r))
    out = []
    for g in sorted(members, key=int):
        ids = members[g]
        first = ids[0][1]
        if len(ids) < 2 or first.get('dgrpfunc') not in TEMPLATE or first.get('dgrpstrpos') not in names: continue
        if any(r.get('dgrpfunc') != first['dgrpfunc'] or r.get('dgrpstrpos') != first['dgrpstrpos'] or (r.get('ValShift') or '0') != (first.get('ValShift') or '0') for _, r in ids): continue
        pos, neg = names[first['dgrpstrpos']], names.get(first.get('dgrpstrneg', ''), names[first['dgrpstrpos']])
        out.append({'ids': [i for i, _ in ids], 'de': pos[0], 'en': pos[1], 'nde': neg[0], 'nen': neg[1], 'shift': int(first.get('ValShift') or 0)})
    return out


def skills_of(skills):

    out = {}
    for col in ('auratargetstate', 'aurastate'):
        for r in skills:
            if r[col] and r['charclass'] and r[col] not in out: out[r[col]] = r
    return out


def mechanism(skill, state):

    return bool(skill['summon']) or state['nosend'] == '1' or state['setfunc'] == BINDING


def kind_of(name, state, skill):


    if state['curse'] == '1' or skill['auratargetstate'] == name and skill['aurastate'] != name: return 'debuff'
    if skill.get('aura') == '1' and skill['aurastate'] == name: return 'aura'
    return 'buff'


def shrine_rows(states, rows):

    boosters = [r for r in rows if r['Duration in frames'].strip('0')]
    out = {}
    for i, s in enumerate(states):
        if not s['state'].startswith('shrine_'): continue
        words = set(s['state'].split('_')[1:])
        score = sorted(((len(words & set(r['Name'].lower().split())), n) for n, r in enumerate(boosters)), reverse=True)
        if not score[0][0] or score[0][0] == score[1][0]: raise Refuse('no shrine row for %s' % s['state'])
        out[i] = boosters[score[0][1]]
    if len(out) != len(boosters) or len({r['Name'] for r in out.values()}) != len(boosters): raise Refuse('shrines and their states differ')
    return out


def potions(misc, of_state):

    rows = {}
    for r in misc:
        if r['state'] and r['state'] not in of_state: rows.setdefault(r['state'], []).append(r)
    return {k: v[len(v) // 2] for k, v in rows.items()}


def cured(misc, states):

    by_name = {s['state']: s for s in states}
    out = {r[c] for r in misc for c in ('cstate1', 'cstate2') if r[c]}
    out = {n for n in out if by_name[n]['hide'] != '1'}
    if len(out) != 2 or len({by_name[n]['green'] == '1' for n in out}) != 2: raise Refuse('cure potions lift %s' % sorted(out))
    return {n: CURED[by_name[n]['green'] == '1'] for n in out}


def style(text):


    import profile_hd
    named = profile_hd.parse(text)
    tip = named.get('TooltipStyle', {})
    out = {}
    bg = profile_hd.colour(tip.get('backgroundColor'), named)
    if bg: out['background'] = bg
    for key, field in STYLE.items():
        c = profile_hd.colour(tip.get(field), named)
        if c: out[key] = c
    for key, field in STYLE_FONT.items():
        c = profile_hd.colour(named.get(field), named)
        if c: out[key] = c
    return out


def overlay_index(rows, read_dcc):




    first, out = {}, {}
    for r in rows:
        name, fn = r.get('overlay', '').strip().lower(), r.get('Filename', '').strip()
        if not name or not fn: continue
        norm = fn.replace('\\', '/')
        data = read_dcc(OVERLAYS % norm)
        copy = False
        if data is not None:
            key = hashlib.sha256(data).digest()
            copy = key in first and first[key] != norm.lower()
            first.setdefault(key, norm.lower())
        out.setdefault(name, {'path': OVERLAYS % norm, 'data': data, 'copy': copy})
    return out


def sources(kind, state, overlays, skill, effect=None, item=None):










    glow = overlays.get(state.get('overlay1', '').strip().lower())
    own = [('overlay', glow['path'])] if glow and glow['data'] is not None and not glow['copy'] else []
    if kind == 'shrine': return own
    if effect: return [effect]
    if item: return [item]
    if kind == 'debuff': return own + ([skill] if skill else [])
    return [skill] if skill else []


def penalties(rows):


    by = {r.get('Name', ''): r for r in rows}
    try: return [[int(by[d]['ResistPenalty']), int(by[d]['ResistPenaltyNonExpansion'])] for d in DIFFICULTIES]
    except (KeyError, ValueError): return None


def thresholds(rows):


    levels = {r.get('Level', ''): r for r in rows}
    out = {}
    for i, c in enumerate(CLASSES):
        try: out[str(i)] = [int(levels[str(n)][c]) for n in range(100)]
        except (KeyError, ValueError): continue
    return out


def one_line(text):

    return ' '.join(t.strip() for t in text.replace('\\n', '\n').split('\n') if t.strip())






CARD_VALUE_SKILLS = ('Might', 'Prayer', 'Thorns', 'Defiance', 'Blessed Aim', 'Holy Freeze', 'Amplify Damage')


def skill_card(r, d, names, by_name, stat_ids):








    out = {}
    if d.get('str short') in names: out['short'] = [one_line(t) for t in names[d['str short']]]
    lines = []
    for i in range(1, 7):
        f, a = d.get('dsc2line%d' % i, ''), d.get('dsc2texta%d' % i, '')
        if f not in DESC_LINES or a not in names or d.get('dsc2calca%d' % i, '') == 'usmc': continue
        b = d.get('dsc2textb%d' % i, '')
        line = {'f': int(f), 'a': list(names[a]), 'ca': d.get('dsc2calca%d' % i, '').strip('"'), 'cb': d.get('dsc2calcb%d' % i, '').strip('"')}
        if b in names: line['b'] = list(names[b])
        lines.append(line)


    own = [{'st': stat_ids[r['aurastat%d' % i]], 'ca': r['aurastatcalc%d' % i].strip('"')}
           for i in range(1, 7) if r.get('aurastat%d' % i, '').strip() in stat_ids and r.get('aurastatcalc%d' % i, '').strip()
           and not re.search(r"skill\('[^']+'\.(?!blvl)", r['aurastatcalc%d' % i])]
    calc = {'par': [int(r.get('Param%d' % i) or 0) for i in range(1, PARAMS + 1)], 'calc': [r.get('calc%d' % i, '').strip('"') for i in range(1, CALCS + 1)],
            'ast': [r.get('aurastatcalc%d' % i, '').strip('"') for i in range(1, 7)], 'len': r.get('auralencalc', '').strip('"'),
            'emin': int(r.get('EMin') or 0), 'eminlev': [int(r.get('EMinLev%d' % i) or 0) for i in range(1, 6)]}
    refs = {}
    for f in calc['calc'] + calc['ast'] + [calc['len']] + [o['ca'] for o in own] + [y for l in lines for y in (l['ca'], l['cb'])]:
        for name in re.findall(r"skill\('([^']+)'\.", f):
            if name not in by_name: calc['bad'] = 1
            else: refs[name] = by_name[name]
    calc['refs'] = refs
    if lines: out['enemy'] = lines
    if own: out['own'] = own
    if lines or own: out['calc'] = calc
    return out


def build(romfs, read):
    from gen_recipe_data import sprite_paths
    digest = hashlib.sha256()
    for path in TABLES: digest.update(read(path))
    if digest.hexdigest() != TABLES_SHA: raise Refuse('the tables are not those of %s (%s)' % (VERSION, digest.hexdigest()))
    names = strings(read)
    states, stats = table(read('global/excel/states.txt')), table(read('global/excel/itemstatcost.txt'))
    skills, desc = table(read('global/excel/skills.txt')), {r['skilldesc']: r for r in table(read('global/excel/skilldesc.txt'))}
    misc = table(read('global/excel/misc.txt'))
    if len(states) != STATES: raise Refuse('%d states' % len(states))
    of_state = skills_of(skills)
    shrines, potion, lifted = shrine_rows(states, table(read('global/excel/shrines.txt'))), potions(misc, of_state), cured(misc, states)
    assets = {k: v['asset'] for e in json.loads(read('hd/items/items.json').decode('utf-8-sig')) for k, v in e.items()}
    paths = sprite_paths(romfs)

    def maybe(path):
        try: return read(path)
        except (SystemExit, Exception): return None
    overlays = overlay_index(table(read('global/excel/overlay.txt')), maybe)

    def skill_art(r):
        cel = desc.get(r['skilldesc'], {}).get('IconCel', '')
        if not cel.strip() or r['charclass'] not in ATLASES: return None
        return ('skill', r['charclass'], int(cel))

    def item_art(state_name):
        p = potion.get(state_name)
        path = p and paths.get(assets.get(p['code'], '').lower())
        return ('item', path) if path else None

    def named(key):
        if key not in names: raise Refuse('no text %s' % key)
        return names[key]

    by_name = {r['skill']: int(r['*Id']) for r in skills if r.get('*Id', '').isdigit()}
    stat_ids = {r['Stat']: i for i, r in enumerate(stats)}
    slow_group = {stat_ids[n] for n in ('velocitypercent', 'attackrate', 'other_animrate') if n in stat_ids}
    out, art = {}, {}
    for i, r in enumerate(states):
        name = r['state']
        extra, effect, item, skill = {}, None, None, None
        if i in shrines:
            kind, text = 'shrine', named(shrines[i]['StringName'])
            extra['frames'] = int(shrines[i]['Duration in frames'])
        elif name in lifted:
            kind, text, effect = 'debuff', named(lifted[name][0]), ('effect', lifted[name][1])
            if lifted[name][0] == CURED[True][0]: extra['poison'] = 1
        elif name in potion: kind, text, item = 'potion', named(potion[name]['namestr']), item_art(name)
        elif name in of_state and r['hide'] != '1' and of_state[name]['skilldesc'] in desc and not mechanism(of_state[name], r):
            s = of_state[name]
            kind, text, skill = kind_of(name, r, s), named(desc[s['skilldesc']]['str name']), skill_art(s)
            if s['aurastate'] == name and s['auratargetstate'] == name: extra['party'] = 1
            card = skill_card(s, desc[s['skilldesc']], names, by_name, stat_ids)
            if 'short' in card: extra['short'] = card['short']
            if s['skill'] not in CARD_VALUE_SKILLS: card.pop('own', None)
            elif card.get('own'): extra['skill'] = int(s['*Id'])
            if s['auratargetstate'] == name and card.get('own'):
                extra['own'], extra['calc'] = card['own'], card['calc']
            elif s['aurastate'] == name and s['auratargetstate'] not in ('', name) and card.get('own'):
                if len({o['ca'] for o in card['own']}) == 1 and all(o['st'] in slow_group for o in card['own']) and not card['calc'].get('bad'):
                    extra['slow'], extra['calc'] = card['own'][0]['ca'], card['calc']
            if s['aurastate'] == name and s['auratargetstate'] not in ('', name) and card.get('enemy'):
                extra['enemy'], extra['calc'] = card['enemy'], card['calc']
        else: continue
        if r['curse'] == '1' and kind == 'debuff': extra['curse'] = 1
        if any(r.get(c) == '1' for c in SHEET_BLUE): extra['sheet'] = 1
        out[str(i)] = dict({'kind': kind, 'de': text[0], 'en': text[1]}, **extra)
        art[i] = sources(kind, r, overlays, skill, effect, item)
    glows = {g['path']: g['data'] for g in overlays.values() if g['data'] is not None}
    art = trusted_glows(art, glows)
    words = {k: named(v) for k, v in WORDS.items()}
    words.update({k: names[v] for k, v in CARD_WORDS.items() if v in names})
    data = {'version': VERSION, 'states': out, 'stats': stat_rows(names, stats), 'groups': stat_groups(names, stats), 'words': words, 'ops': op_sources(stats)}
    pen, exp = penalties(table(read('global/excel/difficultylevels.txt'))), thresholds(table(read('global/excel/experience.txt')))
    if pen: data['penalty'] = pen
    if exp: data['experience'] = exp
    data['skills'] = {str(i): list(named(desc[r['skilldesc']]['str name'])) for i, r in ((int(r['*Id']), r) for r in skills if r.get('*Id', '').isdigit())
                      if r['charclass'] and r['skilldesc'] in desc and desc[r['skilldesc']].get('str name') in names}
    return data, art, glows


def trusted_glows(art, glows, want=None):




    digest = hashlib.sha256()
    for path in sorted({s[1] for a in art.values() for s in a if s[0] == 'overlay'}): digest.update(path.lower().encode() + b'\0' + glows[path])
    if digest.hexdigest() == (want or ART_SHA): return art
    print('status effects: glow art is not from %s (%s), states without a glow image' % (VERSION, digest.hexdigest()))
    return {i: [s for s in a if s[0] != 'overlay'] for i, a in art.items()}


def palette(raw):

    if len(raw) < 768: raise ValueError('palette of %d bytes' % len(raw))
    return [(0, 0, 0)] + [(raw[3 * i + 2], raw[3 * i + 1], raw[3 * i]) for i in range(1, 256)]


def glow(data, colours):




    import dcc
    from PIL import Image
    d = dcc.decode(data)[0]
    W, H, light = d['w'], d['h'], [max(c) for c in colours]
    luma = [(r * 19595 + g * 38470 + b * 7471 + 0x8000) >> 16 for r, g, b in colours]
    best = max(range(len(d['frames'])), key=lambda i: sum(luma[v] for v in d['frames'][i]))
    frame = d['frames'][best]
    xs = [k % W for k, v in enumerate(frame) if light[v] > VISIBLE]
    if not xs: raise ValueError('empty glow')
    ys = [k // W for k, v in enumerate(frame) if light[v] > VISIBLE]
    x0, x1, y0, y1 = min(xs), max(xs) + 1, min(ys), max(ys) + 1
    if x1 - x0 < SMALLEST or y1 - y0 < SMALLEST: raise ValueError('glow of %dx%d' % (x1 - x0, y1 - y0))
    if max(light[v] for v in frame) < BRIGHT: raise ValueError('glow too dark')
    side = max(x1 - x0, y1 - y0)
    left, top = x0 - (side - (x1 - x0)) // 2, y0 - (side - (y1 - y0)) // 2
    im = Image.new('RGBA', (side, side))
    px = im.load()
    for y in range(side):
        for x in range(side):
            sx, sy = left + x, top + y
            if not (0 <= sx < W and 0 <= sy < H): continue
            r, g, b = colours[frame[sx + sy * W]]
            a = max(r, g, b)
            if a: px[x, y] = ((r * 255 + a // 2) // a, (g * 255 + a // 2) // a, (b * 255 + a // 2) // a, a)
    return im


def picture(romfs, spec, glows=None, colours=None):
    import romfs_art
    if spec[0] == 'overlay': return glow(glows[spec[1]], colours)
    if spec[0] == 'effect': return romfs_art.sprite(romfs, EFFECT_ICONS % spec[1])
    if spec[0] == 'item': return romfs_art.frame(romfs, spec[1], 0)
    atlas = romfs_art.sprite(romfs, 'hd/global/ui/Spells/%s.sprite' % ATLASES[spec[1]])
    if spec[2] < 0 or spec[2] * PITCH + WIDTH > atlas.width: raise ValueError('icon %d outside the %s atlas' % (spec[2], spec[1]))
    t = round(WIDTH * TRIM)
    return atlas.crop((spec[2] * PITCH + t, t, spec[2] * PITCH + WIDTH - t, min(atlas.height, WIDTH) - t))


def save_picture(specs, path, picture):


    for s in specs:
        try:
            im = picture(s)
            if im.getbbox() is None or (im.mode == 'RGBA' and im.getchannel('A').getbbox() is None): raise ValueError('empty picture')
            im.save(path)
            return s
        except (Exception, SystemExit) as e:

            print('Statuseffekte: Bild %r nicht gebaut (%r)' % (s, e))
    return None


def stage(romfs, out):
    import romfs_art
    read = lambda path: romfs_art.read(romfs, path)
    os.makedirs(out, exist_ok=True)
    made = [os.path.join(out, 'effects.json'), os.path.join(out, 'fx')]
    for p in made:
        if os.path.isdir(p): shutil.rmtree(p)
        elif os.path.exists(p): os.remove(p)
    try:
        data, art, glows = build(romfs, indexed(romfs, read))
        try: data['style'] = style(read(PROFILE).decode('utf-8', 'replace'))
        except Exception as e: data['style'] = {}; print('Statuseffekte: Kartenfarben nicht gelesen (%r), die App nimmt die gleichen eingebauten' % (e,))
        colours = palette(read(PALETTE))
        os.makedirs(made[1])
        counts = {}
        for state, specs in art.items():
            used = save_picture(specs, os.path.join(made[1], '%d.png' % state), lambda s: picture(romfs, s, glows, colours))
            key = used[0] if used else 'none'
            counts[key] = counts.get(key, 0) + 1
            if used and used[0] in FILL: data['states'][str(state)]['fill'] = 1
        with open(made[0], 'w', encoding='utf-8', newline='\n') as f: json.dump(data, f, ensure_ascii=False, separators=(',', ':'))
    except (Exception, SystemExit) as e:
        for p in made:
            if os.path.isdir(p): shutil.rmtree(p)
            elif os.path.exists(p): os.remove(p)
        print('status effects: not built (%r), the game runs without icons' % (e,))
        return False
    print('Statuseffekte: %d Zustaende, %d Werte, %d Farben, Bilder %s' % (len(data['states']), len(data['stats']), len(data['style']), ', '.join('%s %d' % kv for kv in sorted(counts.items()))))
    return True


def indexed(romfs, read):


    try:
        import touch_layout
        index = {}
        with open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8') as f:
            for line in f:
                e = line.rstrip('\n').split('|')
                if len(e) >= 11: index.setdefault(e[9].lower().split(':', 1)[-1], e)
    except Exception: return read

    def get(path):
        want = path.lower().lstrip('/')
        if not want.startswith('data/'): want = 'data/' + want
        if want not in index: raise SystemExit('gen_effect_data: no %s in the game files' % path)
        return touch_layout._read_entry(romfs, index[want])
    return get


if __name__ == '__main__':
    sys.exit(0 if stage(sys.argv[1], sys.argv[2]) else 1)
