#!/usr/bin/env python3






























import csv
import hashlib
import io
import json
import os
import re
import sys

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
sys.path.insert(0, os.path.join(DM, 'ios', 'tools'))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

VERSION = '3.4.95035'
SCHEMA = 1
MAX_BYTES = 4 * 1024 * 1024
SETS = (('rotw', 'global/excel/'), ('lod', 'global/excel/base/'))
NAMES = ('treasureclassex', 'monstats', 'superuniques', 'levels', 'itemratio', 'itemtypes', 'weapons', 'armor', 'misc', 'uniqueitems', 'setitems',
         'monpreset', 'skills', 'sets')
STRINGS = ('item-names', 'item-runes', 'item-gems', 'item-nameaffixes', 'item-modifiers', 'monsters')
TABLES = tuple(p + n + '.txt' for _, p in SETS for n in NAMES) + tuple('local/lng/strings/%s.json' % s for s in STRINGS)
TABLES_SHA = 'cbf8886db2bdc58be6f267eec4996f684ae65ddf39eeb4be190e08600c2199f3'
GAME_LANGUAGES = ('enUS', 'deDE', 'esES', 'frFR', 'itIT', 'koKR', 'plPL', 'ruRU', 'zhCN', 'zhTW', 'esMX', 'jaJP', 'ptBR')
DIFF = ('', '(N)', '(H)')
QUALITY, UBER, CLASS = 1, 2, 4


CODE_SPAWNED = (258, 259, 260, 261, 262, 263, 283, 462, 463, 464, 465, 467, 468, 234)

BOVINE_SET, BOVINE = 29, 'hellbovine'
UPGRADES, BOVINE_DROP = 1, 2
AUTO_TC = re.compile(r'^([a-z]+)(\d+)$')
CATEGORY = (('rune', 'r'), ('gem', 'e'), ('jewl', 'j'), ('char', 'c'), ('ring', 'y'), ('amul', 'y'), ('gold', 'g'), ('poti', 'p'), ('scro', 'p'),
            ('weap', 'w'), ('armo', 'a'))


class Refuse(Exception):
    """These tables give no book data."""


def table(raw, name):
    rows = list(csv.DictReader(io.StringIO(raw.decode('utf-8-sig')), delimiter='\t'))
    if not rows:
        raise Refuse('%s is empty' % name)
    return rows


def need(rows, cols, name):
    missing = [c for c in cols if c not in rows[0]]
    if missing:
        raise Refuse('%s lacks the columns %s' % (name, missing))


def num(v, what):
    v = (v or '').strip()
    if not v:
        return 0
    if not re.fullmatch(r'-?\d+', v):
        raise Refuse('%s is not a whole number: %r' % (what, v))
    return int(v)




COND_TOKEN = re.compile(r"\s*(?:(cond\(\s*'(\w+)'\s*(?:,\s*(\w+)\s*)?\))|(stat\(\s*'(\w+)'\.(\w+)\s*\))|(\d+)|(==|<|>|\*|\(|\)))")
DIFFICULTY_WORDS = {'normal': 0, 'nightmare': 1, 'hell': 2}


def condition(text, what):




    text = (text or '').strip()
    if not text:
        return 7
    tokens, pos = [], 0
    while pos < len(text):
        m = COND_TOKEN.match(text, pos)
        if not m or m.end() == pos:
            raise Refuse('%s: unknown condition %r' % (what, text))
        pos = m.end()
        if m.group(1):
            name, arg = m.group(2), m.group(3)
            if name == 'Difficulty' and arg in DIFFICULTY_WORDS:
                tokens.append(('diff', DIFFICULTY_WORDS[arg]))
            elif name == 'Desecrated' and arg is None:
                tokens.append(('num', 0))
            elif name == 'MonsterTestElite' and arg == 'herald':
                tokens.append(('num', 0))
            else:
                raise Refuse('%s: unknown condition %r' % (what, text))
        elif m.group(4):
            if (m.group(5), m.group(6)) != ('heraldtier', 'accr'):
                raise Refuse('%s: unknown condition %r' % (what, text))
            tokens.append(('num', 0))
        elif m.group(7):
            tokens.append(('num', int(m.group(7))))
        else:
            tokens.append(('op', m.group(8)))

    def value(d):
        stream = list(tokens)

        def atom():
            if not stream:
                raise Refuse('%s: condition ends early %r' % (what, text))
            kind, v = stream.pop(0)
            if kind == 'num':
                return v
            if kind == 'diff':
                return 1 if v == d else 0
            if v == '(':
                r = compare()
                if not stream or stream.pop(0) != ('op', ')'):
                    raise Refuse('%s: unbalanced condition %r' % (what, text))
                return r
            raise Refuse('%s: unexpected %r in %r' % (what, v, text))

        def product():
            r = atom()
            while stream and stream[0] == ('op', '*'):
                stream.pop(0)
                r *= atom()
            return r

        def compare():
            r = product()
            while stream and stream[0] in (('op', '=='), ('op', '<'), ('op', '>')):
                op = stream.pop(0)[1]
                b = product()
                r = 1 if (r == b if op == '==' else r < b if op == '<' else r > b) else 0
            return r
        r = compare()
        if stream:
            raise Refuse('%s: trailing %r in %r' % (what, stream, text))
        return r
    return sum(1 << d for d in range(3) if value(d))


def ladder(row):
    return bool((row.get('firstLadderSeason') or '').strip() or (row.get('lastLadderSeason') or '').strip())




def type_chains(itemtypes):

    rows = {r['Code']: r for r in itemtypes if r.get('Code')}
    chains = {}

    def chain(code, seen=()):
        if code in chains:
            return chains[code]
        if code in seen:
            raise Refuse('itemtypes: %s is its own ancestor' % code)
        out = {code}
        for e in ('Equiv1', 'Equiv2'):
            parent = rows[code].get(e)
            if parent:
                if parent not in rows:
                    raise Refuse('itemtypes: %s names the unknown type %s' % (code, parent))
                out |= chain(parent, seen + (code,))
        chains[code] = out
        return out
    for c in rows:
        chain(c)
    return chains, rows


def item_flags(kind, row, types, trows):


    flags = 0
    if not any(trows[t].get('Normal') == '1' for t in types) and (kind in ('weapons', 'armor') or any(
            trows[t].get('Magic') == '1' or trows[t].get('Rare') == '1' for t in types)):
        flags |= QUALITY
    if kind in ('weapons', 'armor') and row.get('normcode') and row['code'] != row['normcode']:
        flags |= UBER
    if any(trows[t].get('Class') for t in types):
        flags |= CLASS
    return flags


def category(types):
    for t, c in CATEGORY:
        if t in types:
            return c
    return 'q'


def items(t, chains, trows):

    out = []
    for kind in ('weapons', 'armor', 'misc'):
        rows = t[kind]
        need(rows, ('code', 'namestr', 'level', 'spawnable', 'type', 'type2', 'quest'), kind)
        if kind != 'misc':
            need(rows, ('normcode',), kind)
        for r in rows:
            code = r.get('code')
            if not code:
                continue
            types = set()
            for c in ('type', 'type2'):
                if r.get(c):
                    if r[c] not in chains:
                        raise Refuse('%s: %s has the unknown type %s' % (kind, code, r[c]))
                    types |= chains[r[c]]
            out.append((code, r['namestr'] or code, num(r['level'], code + ' level'), category(types), item_flags(kind, r, types, trows), types,
                        r.get('type') or '', r.get('spawnable') == '1' and (r.get('quest') or '0') == '0'))
    codes = [i[0] for i in out]
    if len(set(codes)) != len(codes):
        raise Refuse('an item code is listed twice')
    return out


def auto_name(ref, trows):

    m = AUTO_TC.match(ref)
    return bool(m) and m.group(1) in trows and trows[m.group(1)].get('TreasureClass') == '1' and int(m.group(2)) % 3 == 0 and int(m.group(2)) > 0


def auto_classes(itemlist, trows):




    tc_types = sorted(c for c, r in trows.items() if r.get('TreasureClass') == '1')
    out = {}
    for i, it in enumerate(itemlist):
        code, _, level, _, _, types, primary, spawnable = it
        if not spawnable or level <= 0 or level > 96:
            continue
        n = -(-level // 3) * 3
        weight = max(1, num(trows[primary].get('Rarity'), primary + ' Rarity')) if primary in trows else 1
        for t in tc_types:
            if t in types and (t == 'tpot' or 'tpot' not in types):
                out.setdefault('%s%d' % (t, n), []).append((i, weight))
    return out




def classes(t, itemlist, uniques):

    rows = t['treasureclassex']
    need(rows, ('Treasure Class', 'group', 'level', 'Picks', 'Unique', 'Set', 'Rare', 'Magic', 'NoDrop', 'ConditionCalc', 'firstLadderSeason',
                'lastLadderSeason') + tuple(c for i in range(1, 11) for c in ('Item%d' % i, 'Prob%d' % i)), 'treasureclassex')
    named = [r for r in rows if r.get('Treasure Class')]
    index = {}
    for r in named:
        if r['Treasure Class'] in index:
            raise Refuse('treasureclassex: %s twice' % r['Treasure Class'])
        index[r['Treasure Class']] = len(index)
    codes = {it[0]: i for i, it in enumerate(itemlist)}
    unique_by_index = {u['index']: k for k, u in enumerate(uniques)}
    auto = auto_classes(itemlist, t['_types'])
    used_auto = []
    out = []
    for r in named:
        name = r['Treasure Class']
        entries = []
        for i in range(1, 11):
            ref = (r.get('Item%d' % i) or '').strip()
            if not ref:
                continue
            prob = num(r.get('Prob%d' % i), '%s Prob%d' % (name, i))
            if prob <= 0:
                raise Refuse('treasureclassex: %s Prob%d is %d' % (name, i, prob))
            base = ref.split(',', 1)[0] if ref.startswith('gld,mul=') else ref
            if ref in index:
                entries += [0, index[ref], prob]
            elif base in codes:
                entries += [1, codes[base], prob]
            elif ref in unique_by_index:
                entries += [2, unique_by_index[ref], prob]
            elif auto_name(ref, t['_types']):
                if ref not in used_auto:
                    used_auto.append(ref)
                entries += [0, -1 - used_auto.index(ref), prob]
            else:
                raise Refuse('treasureclassex: %s names %r, which is no class, item, unique or item class holding an item' % (name, ref))
        if not entries:
            raise Refuse('treasureclassex: %s drops nothing' % name)
        diffs = 0 if ladder(r) else condition(r.get('ConditionCalc'), name)
        out.append([num(r['Picks'], name + ' Picks'), num(r['NoDrop'], name + ' NoDrop'), num(r['Unique'], name), num(r['Set'], name),
                    num(r['Rare'], name), num(r['Magic'], name), num(r['group'], name + ' group'), num(r['level'], name + ' level'), diffs, entries])
        if out[-1][0] == 0:
            raise Refuse('treasureclassex: %s has no picks' % name)
    first_auto = len(out)
    names = list(index)
    order = sorted(range(len(used_auto)), key=lambda k: used_auto[k])
    place = {k: first_auto + n for n, k in enumerate(order)}
    for k in order:
        out.append([1, 0, 0, 0, 0, 0, 0, 0, 7, [x for i, w in auto.get(used_auto[k], []) for x in (1, i, w)]])
        names.append(used_auto[k])
    for row in out[:first_auto]:
        e = row[9]
        for j in range(0, len(e), 3):
            if e[j] == 0 and e[j + 1] < 0:
                e[j + 1] = place[-1 - e[j + 1]]
    check_acyclic(out, names)
    return out, {n: i for i, n in enumerate(names)}


def check_acyclic(tcs, names):
    state = [0] * len(tcs)

    def visit(i, path):
        if state[i] == 2:
            return
        if state[i] == 1:
            raise Refuse('treasureclassex: the classes %s call each other' % ' -> '.join(names[k] for k in path + [i]))
        state[i] = 1
        e = tcs[i][9]
        for j in range(0, len(e), 3):
            if e[j] == 0:
                visit(e[j + 1], path + [i])
        state[i] = 2
    for i in range(len(tcs)):
        visit(i, [])




def sim_listed(t, bosses):


    ids = {r['Id'].lower() for r in t['monstats'] if r.get('Id')}
    out = {r['Id'].lower() for r in t['monstats'] if r.get('*hcIdx', '').isdigit() and int(r['*hcIdx']) in bosses}
    for r in t['levels']:
        out.update((r.get('%s%d' % (c, i)) or '').lower() for c in ('mon', 'nmon') for i in range(1, 26))
    out.update((r.get('Class') or '').lower() for r in t['superuniques'])
    return out & ids


def extra_monsters(t, listed, monster_names):




    rows = {r['Id'].lower(): r for r in t['monstats'] if r.get('Id')}
    need(t['monstats'], ('minion1', 'minion2', 'spawn', 'Align', 'npc', 'inTown', 'enabled', 'killable', 'neverCount', 'NameStr', '*hcIdx')
         + tuple('Skill%d' % k for k in range(1, 9)), 'monstats')
    summons = {r['skill']: (r.get('summon') or '').lower() for r in t['skills'] if r.get('skill')}
    by_index = {num(r['*hcIdx'], 'monstats *hcIdx'): k for k, r in rows.items() if r.get('*hcIdx', '').strip()}
    seeds = set(listed)
    for r in t['levels']:
        seeds.update((r.get('umon%d' % i) or '').lower() for i in range(1, 26))
    seeds.update((r.get('Place') or '').lower() for r in t['monpreset'])
    seeds.update(by_index[i] for i in CODE_SPAWNED if i in by_index)
    seeds &= set(rows)
    parents = {k: set() for k in seeds}
    todo = sorted(seeds)
    while todo:
        k = todo.pop()
        r = rows[k]
        kids = {(r.get(c) or '').lower() for c in ('minion1', 'minion2', 'spawn')}
        kids |= {summons.get(r.get('Skill%d' % s) or '', '') for s in range(1, 9)}
        for c in sorted(kids & set(rows)):
            if c == k:
                continue
            if c not in parents:
                parents[c] = set()
                todo.append(c)
            parents[c].add(k)

    def counted(r):
        return (r.get('enabled') == '1' and r.get('killable') == '1' and r.get('Align') != '1' and r.get('npc') != '1' and r.get('inTown') != '1'
                and r.get('neverCount') != '1' and r.get('NameStr') in monster_names)
    out = {}
    for k in sorted(parents):
        if k in listed or not counted(rows[k]):
            continue
        out[k] = (sorted(num(rows[p]['*hcIdx'], 'monstats *hcIdx') for p in parents[k] if p in listed or p in parents and counted(rows[p])), rows[k])
    return out


def book_monsters(read):


    import gen_sim_data
    t = {n: table(read('global/excel/%s.txt' % n), n) for n in ('monstats', 'levels', 'superuniques', 'monpreset', 'skills')}
    names = {r['Key'] for r in json.loads(read('local/lng/strings/monsters.json').decode('utf-8-sig'))}
    listed = sim_listed(t, gen_sim_data.BOSSES)
    return listed | set(extra_monsters(t, listed, names))


def flags(r):

    return (UPGRADES if r.get('boss') != '1' and r.get('noRatio') != '1' else 0) | (BOVINE_DROP if (r.get('Id') or '').lower() == BOVINE else 0)


def monster_classes(t, index, listed, extra):

    def ref(name, what):
        name = (name or '').strip()
        if not name:
            return -1
        if name not in index:
            raise Refuse('%s names the treasure class %r, which the table lacks' % (what, name))
        return index[name]
    mon = {}
    for r in t['monstats']:
        k = (r.get('Id') or '').lower()
        if k in listed or k in extra:
            mon[str(num(r['*hcIdx'], 'monstats *hcIdx'))] = [ref(r.get('TreasureClass' + d), r['Id']) for d in DIFF] + [flags(r)]
    su = {}
    need(t['superuniques'], ('hcIdx', 'TC', 'TC(N)', 'TC(H)', 'Class'), 'superuniques')
    classes_ = {(r.get('Id') or ''): r for r in t['monstats']}
    for r in t['superuniques']:
        if (r.get('hcIdx') or '').strip().isdigit():
            cls = classes_.get(r.get('Class') or '')
            su[str(int(r['hcIdx']))] = [ref(r.get(c), r.get('Superunique')) for c in ('TC', 'TC(N)', 'TC(H)')] + [flags(cls) if cls else 0]
    return mon, su




def build_set(t, strings, monster_names, bosses):
    chains, trows = type_chains(t['itemtypes'])
    t['_types'] = trows
    itemlist = items(t, chains, trows)
    codes = {it[0]: i for i, it in enumerate(itemlist)}
    need(t['uniqueitems'], ('index', '*ID', 'disabled', 'spawnable', 'DropConditionCalc', 'rarity', 'lvl', 'code'), 'uniqueitems')
    need(t['setitems'], ('index', '*ID', 'disabled', 'spawnable', 'rarity', 'lvl', 'item', 'set'), 'setitems')
    uniques = [u for u in t['uniqueitems'] if (u.get('*ID') or '').strip().isdigit() and u.get('code')]
    tcs, index = classes(t, itemlist, uniques)
    keys = set()
    unique_out = []
    for u in uniques:
        if u['code'] not in codes:
            raise Refuse('uniqueitems: %s is made of the unknown item %s' % (u['index'], u['code']))
        rollable = u.get('spawnable') == '1' and u.get('disabled') != '1'
        unique_out.append([int(u['*ID']), codes[u['code']], num(u['lvl'], u['index']), num(u['rarity'], u['index']) if rollable else 0,
                           u['index'], condition(u.get('DropConditionCalc'), u['index']) if rollable else 0])
    need(t['sets'], ('index', 'name'), 'sets')
    set_records = [r['index'] for r in t['sets'] if r.get('name')]
    set_out = []
    for s in t['setitems']:
        if not (s.get('*ID') or '').strip().isdigit() or not s.get('item'):
            continue
        if s['item'] not in codes:
            raise Refuse('setitems: %s is made of the unknown item %s' % (s['index'], s['item']))
        if s.get('spawnable') != '1' or s.get('disabled') == '1':
            continue
        if s.get('set') not in set_records:
            raise Refuse('setitems: %s belongs to the unknown set %r' % (s['index'], s.get('set')))
        set_out.append([int(s['*ID']), codes[s['item']], num(s['lvl'], s['index']), num(s['rarity'], s['index']), s['index'],
                        1 if set_records.index(s['set']) == BOVINE_SET else 0])
    need(t['itemratio'], ('Version', 'Uber', 'Class Specific', 'Unique', 'UniqueDivisor', 'UniqueMin', 'Rare', 'RareDivisor', 'RareMin', 'Set',
                          'SetDivisor', 'SetMin', 'Magic', 'MagicDivisor', 'MagicMin'), 'itemratio')
    ratio = []
    for r in t['itemratio']:
        if r.get('Version') != '1':
            continue
        ratio.append([num(r[c], 'itemratio ' + c) for c in ('Uber', 'Class Specific', 'Unique', 'UniqueDivisor', 'UniqueMin', 'Rare', 'RareDivisor',
                                                            'RareMin', 'Set', 'SetDivisor', 'SetMin', 'Magic', 'MagicDivisor', 'MagicMin')])
    if sorted((r[0], r[1]) for r in ratio) != [(0, 0), (0, 1), (1, 0), (1, 1)] or any(r[c] <= 0 for r in ratio for c in (3, 6, 9, 12)):
        raise Refuse('itemratio: the expansion rows are not one per uber / class combination with divisors above 0')
    listed = sim_listed(t, bosses)
    extra = extra_monsters(t, listed, monster_names)
    mon, su = monster_classes(t, index, listed, extra)
    extra_out = {}
    for k, (parents, r) in extra.items():
        extra_out[str(num(r['*hcIdx'], 'monstats *hcIdx'))] = [r['NameStr'], k, [num(r.get('Level' + d), r['Id'] + ' level') for d in DIFF], parents]
    item_out = [[code, name, level, cat, flags] for code, name, level, cat, flags, _, _, _ in itemlist]
    for it in item_out:
        keys.add(it[1])
    for u in unique_out:
        keys.add(u[4])
    for s in set_out:
        keys.add(s[4])
    return {'tc': tcs, 'item': item_out, 'unique': unique_out, 'set': set_out, 'ratio': sorted(ratio), 'mon': mon, 'su': su, 'extra': extra_out}, keys


def game_names(row):
    from dm_i18n import untag
    names = [untag(row.get(lang)) if isinstance(row.get(lang), str) else '' for lang in GAME_LANGUAGES]
    return [n or names[0] for n in names]


def build(read):
    raw = {p: read(p) for p in TABLES}
    sha = hashlib.sha256(b''.join(hashlib.sha256(raw[p]).digest() for p in TABLES)).hexdigest()
    if sha != TABLES_SHA:
        raise Refuse('the tables are not those of %s (%s)' % (VERSION, sha))
    import gen_sim_data
    strings = {}
    for s in STRINGS:
        for row in json.loads(raw['local/lng/strings/%s.json' % s].decode('utf-8-sig')):
            strings.setdefault(row['Key'], row)
    monster_names = {row['Key'] for row in json.loads(raw['local/lng/strings/monsters.json'].decode('utf-8-sig'))}
    out = {'v': SCHEMA, 'game': VERSION, 'sets': {}, 'names': {}}
    keys = set()
    for set_name, prefix in SETS:
        t = {n: table(raw[prefix + n + '.txt'], prefix + n) for n in NAMES}
        out['sets'][set_name], k = build_set(t, strings, monster_names, gen_sim_data.BOSSES)
        keys |= k
    for key in sorted(keys):
        if key in strings:
            out['names'][key] = game_names(strings[key])
    return out


def write(data, path):
    text = json.dumps(data, ensure_ascii=False, sort_keys=True, separators=(',', ':'))
    blob = text.encode('utf-8')
    if len(blob) > MAX_BYTES:
        raise Refuse('book.json would take %d bytes, more than %d' % (len(blob), MAX_BYTES))
    with open(path + '.part', 'wb') as f:
        f.write(blob)
    os.replace(path + '.part', path)
    return len(blob)


def stage(romfs, appart):

    import gen_build_names
    rf = gen_build_names.Romfs(romfs)
    try:
        data = build(rf.read)
    except Refuse as e:
        raise SystemExit('appart: no book data: %s' % e)
    os.makedirs(appart, exist_ok=True)
    size = write(data, os.path.join(appart, 'book.json'))
    print('appart: book.json %d KiB, %s' % (size // 1024, ', '.join('%s %d classes / %d monsters / %d extra' % (
        k, len(v['tc']), len(v['mon']), len(v['extra'])) for k, v in sorted(data['sets'].items()))))


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--sha':
        import gen_build_names
        rf = gen_build_names.Romfs(sys.argv[2])
        print(hashlib.sha256(b''.join(hashlib.sha256(rf.read(p)).digest() for p in TABLES)).hexdigest())
        sys.exit(0)
    if len(sys.argv) != 3:
        sys.exit('usage: gen_book_data.py <RomFS> <appart directory> | --sha <RomFS>')
    try:
        stage(sys.argv[1], sys.argv[2])
    except Refuse as e:
        sys.exit('appart: no book data: %s' % e)
