#!/usr/bin/env python3


















import csv
import hashlib
import io
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TABLES = tuple('global/excel/%s' % f for f in ('itemstatcost.txt', 'charstats.txt', 'difficultylevels.txt', 'skills.txt', 'skilldesc.txt', 'playerclass.txt'))

TABLES_SHA = 'b028f0c8741b628253fe3b4c51ded7af0408dc324bf116217878c6afffdbca00'

METRIC_STATS = ('strength', 'energy', 'dexterity', 'vitality', 'maxhp', 'maxmana', 'fireresist', 'maxfireresist', 'lightresist', 'maxlightresist', 'coldresist',
                'maxcoldresist', 'poisonresist', 'maxpoisonresist', 'lifedrainmindam', 'manadrainmindam', 'item_goldbonus', 'item_magicbonus',
                'item_fasterattackrate', 'item_fastermovevelocity', 'item_fastergethitrate', 'item_fasterblockrate', 'item_fastercastrate', 'item_allskills',
                'item_addclassskills',


                'armorclass', 'item_armor_percent', 'item_maxdamage_percent', 'item_mindamage_percent', 'tohit', 'toblock', 'mindamage', 'maxdamage',
                'secondary_mindamage', 'secondary_maxdamage', 'damagepercent', 'firemindam', 'firemaxdam', 'lightmindam', 'lightmaxdam', 'magicmindam', 'magicmaxdam',
                'coldmindam', 'coldmaxdam', 'coldlength', 'poisonmindam', 'poisonmaxdam', 'poisonlength', 'skill_poison_override_length', 'item_normaldamage',
                'item_tohit_percent', 'skill_armor_percent', 'armor_override_percent', 'bonus_mindamage', 'bonus_maxdamage', 'progressive_tohit', 'poison_count',
                'passive_fire_mastery', 'passive_ltng_mastery', 'passive_cold_mastery', 'passive_pois_mastery')


DESC_STATS = ('strength', 'energy', 'dexterity', 'vitality', 'maxhp', 'maxmana', 'toblock', 'fireresist', 'maxfireresist', 'lightresist', 'maxlightresist',
              'coldresist', 'maxcoldresist', 'poisonresist', 'maxpoisonresist', 'item_goldbonus', 'item_magicbonus', 'item_fasterattackrate',
              'item_fastermovevelocity', 'item_fastergethitrate', 'item_fasterblockrate', 'item_fastercastrate')
DESC_COLS = ('descfunc', 'descstrpos', 'descstrneg', 'dgrp', 'dgrpfunc', 'dgrpstrpos', 'dgrpstrneg', 'ValShift')
FORMAT = 1


class Refuse(Exception):
    """这份 RomFS 不出数据 | This RomFS gets no data."""


def table(text):
    rows = list(csv.reader(io.StringIO(text), delimiter='\t', quoting=csv.QUOTE_NONE))
    if not rows: raise Refuse('empty table')
    return rows[0], [dict(zip(rows[0], r)) for r in rows[1:] if r and any(r)]


def need(head, cols, name):
    miss = [c for c in cols if c not in head]
    if miss: raise Refuse('%s lacks %s' % (name, miss))


def build(read):
    raw = {p: read(p) for p in TABLES}
    sha = hashlib.sha256(b''.join(hashlib.sha256(raw[p]).digest() for p in TABLES)).hexdigest()
    if sha != TABLES_SHA: raise Refuse('tables fingerprint %s is not the validated %s' % (sha, TABLES_SHA))
    t = {p.rsplit('/', 1)[1]: table(raw[p].decode('utf-8', 'replace')) for p in TABLES}
    h, isc = t['itemstatcost.txt']; need(h, ('Stat', '*ID', 'op', 'op stat1', 'op stat2', 'op stat3'), 'itemstatcost')
    sid = {r['Stat']: int(r['*ID']) for r in isc if r.get('Stat') and r.get('*ID', '').strip().lstrip('-').isdigit()}
    metric = {sid[n] for n in METRIC_STATS if n in sid}
    if len(metric) != len(METRIC_STATS): raise Refuse('itemstatcost lacks metric stats %s' % [n for n in METRIC_STATS if n not in sid])
    out = ['DM_ITEMCMP\t%d\t%s' % (FORMAT, sha)]
    h, pc = t['playerclass.txt']; need(h, ('Player Class', 'Code'), 'playerclass')
    codes = [r['Code'] for r in pc if r.get('Code')]
    h, cs = t['charstats.txt']; need(h, ('class', 'LifePerVitality', 'ManaPerMagic', 'StaminaPerVitality'), 'charstats')
    cls = [r for r in cs if r.get('class') and r['class'] != 'Expansion']
    if len(cls) != len(codes): raise Refuse('charstats has %d classes, playerclass %d' % (len(cls), len(codes)))
    for i, r in enumerate(cls): out.append('class\t%d\t%d\t%d\t%d' % (i, int(r['LifePerVitality']), int(r['ManaPerMagic']), int(r['StaminaPerVitality'])))
    h, dl = t['difficultylevels.txt']; need(h, ('Name', 'ResistPenalty', 'ResistPenaltyNonExpansion'), 'difficultylevels')
    if len(dl) != 3: raise Refuse('difficultylevels has %d rows' % len(dl))
    for i, r in enumerate(dl): out.append('diff\t%d\t%d\t%d' % (i, int(r['ResistPenalty']), int(r['ResistPenaltyNonExpansion'])))
    n_op = 0
    for r in isc:
        op = r.get('op', '').strip()
        if not op or op == '0' or r.get('Stat') not in sid: continue
        tg = [sid[r[k]] for k in ('op stat1', 'op stat2', 'op stat3') if r.get(k) in sid]
        if any(x in metric for x in tg): out.append('opstat\t%d\t%d\t%s' % (sid[r['Stat']], int(op), ','.join(map(str, tg)))); n_op += 1
    h, sk = t['skills.txt']; need(h, ('*Id', 'charclass', 'skilldesc') + tuple('passivestat%d' % k for k in range(1, 15)), 'skills')
    h, sd = t['skilldesc.txt']; need(h, ('skilldesc', 'SkillPage', 'str name'), 'skilldesc')
    desc = {r['skilldesc']: r for r in sd if r.get('skilldesc')}
    n_sk = 0
    for r in sk:
        if not r.get('*Id', '').isdigit(): continue
        c = codes.index(r['charclass']) if r.get('charclass') in codes else -1
        d = desc.get(r.get('skilldesc', ''), {})
        page = int(d['SkillPage']) if d.get('SkillPage', '').isdigit() else 0
        key = d.get('str name', '') or '-'
        if any(ch in key for ch in '\t\n\r'): raise Refuse('bad string key %r' % key)
        ps = sorted({sid[r['passivestat%d' % k]] for k in range(1, 15) if r.get('passivestat%d' % k) in sid})
        if c < 0 and not ps and int(r['*Id']) != 0: continue
        out.append('skill\t%d\t%d\t%d\t%s\t%s\t%d\t%d' % (int(r['*Id']), c, page, key, ','.join(map(str, ps)) or '-', int(d.get('SkillRow') or 0), int(d.get('SkillColumn') or 0))); n_sk += 1
    need(t['itemstatcost.txt'][0], DESC_COLS, 'itemstatcost')
    for r in isc:
        if r.get('Stat') not in DESC_STATS: continue
        v = [(r.get(c) or '').strip() for c in DESC_COLS]
        numbers = (v[0], v[3], v[4], v[7])
        if any(x and not x.lstrip('-').isdigit() for x in numbers) or any(ch in x for x in v for ch in ' \t\n\r'): raise Refuse('bad description of %s' % r['Stat'])
        out.append('desc\t%d\t%s' % (sid[r['Stat']], '\t'.join(x or '-' for x in v)))
    out.append('end\t%d' % (len(out) + 1))
    return '\n'.join(out) + '\n', n_op, n_sk


def write(romfs, dst):

    p = os.path.join(dst, 'itemcmp.tsv')
    try:
        sys.path.insert(0, HERE); import romfs_art
        text, n_op, n_sk = build(lambda path: romfs_art.read(romfs, path))
        with open(p, 'w', encoding='utf-8', newline='\n') as f: f.write(text)
    except (Exception, SystemExit) as e:
        if os.path.exists(p): os.remove(p)
        print('Item-Vergleich: keine Daten (%r), Tooltips bleiben original' % (e,))
        return False
    print('Item-Vergleich: itemcmp.tsv, %d Op-Werte, %d Fertigkeiten' % (n_op, n_sk))
    return True

















GEAR_TABLES = tuple('global/excel/%s' % f for f in ('itemstatcost.txt', 'properties.txt', 'sets.txt', 'setitems.txt'))
GEAR_TABLES_SHA = '935c315820cb0abc0e6cf0df7508415e20ccdebed2ae762e7a02f12ff3729c5f'
GEAR_FORMAT = 1
ROLL_FUNCS = (1, 3, 8)


def prop_stats(prop, par, lo, hi, props, sid, dmg=((), ())):


    r = props.get(prop)
    if r is None: return None, -1
    out, last, first = [], None, -1
    for k in range(1, 8):
        f = r.get('func%d' % k, '').strip()
        if not f: continue
        f, st = int(f), r.get('stat%d' % k, '')
        stat = sid.get(st, -1)
        if first < 0: first = stat
        if f in ROLL_FUNCS:
            if stat < 0 or par not in ('', '0') or lo != hi: return None, first
            v = last if f == 3 and last is not None else lo
            if f != 3: last = lo
            out.append((stat, 0, v))
        elif f == 15 and stat >= 0: out.append((stat, 0, lo))
        elif f == 16 and stat >= 0: out.append((stat, 0, hi))
        elif f == 17 and stat >= 0 and par.lstrip('-').isdigit(): out.append((stat, 0, int(par)))
        elif f in (5, 6) and dmg[f - 5] and lo == hi:
            if first < 0: first = dmg[f - 5][0]
            out.extend((x, 0, lo) for x in dmg[f - 5])
        elif f == 21 and stat >= 0 and r.get('val%d' % k, '').isdigit() and lo == hi: out.append((stat, int(r['val%d' % k]), lo))
        elif f == 10 and stat >= 0 and par.isdigit() and lo == hi: out.append((stat, int(par), lo))
        else: return None, first
    return (out, first) if out else (None, first)


def num(v):
    v = (v or '').strip()
    return int(v) if v.lstrip('-').isdigit() else None


def build_gear(read):
    raw = {p: read(p) for p in GEAR_TABLES}
    sha = hashlib.sha256(b''.join(hashlib.sha256(raw[p]).digest() for p in GEAR_TABLES)).hexdigest()
    if sha != GEAR_TABLES_SHA: raise Refuse('gear tables fingerprint %s is not the validated %s' % (sha, GEAR_TABLES_SHA))
    t = {p.rsplit('/', 1)[1]: table(raw[p].decode('utf-8', 'replace')) for p in GEAR_TABLES}
    h, isc = t['itemstatcost.txt']; need(h, ('Stat', '*ID', 'op', 'op param', 'op base', 'op stat1', 'op stat2', 'op stat3', 'ValShift'), 'itemstatcost')
    sid = {r['Stat']: int(r['*ID']) for r in isc if r.get('Stat') and r.get('*ID', '').strip().isdigit()}
    shift = {sid[r['Stat']]: int(r['ValShift']) for r in isc if r.get('Stat') in sid and (r.get('ValShift') or '0').strip() not in ('', '0')}
    dmg = tuple(tuple(sid[r[k]] for k in ('op stat1', 'op stat2', 'op stat3') if r.get(k) in sid)
                for n in ('item_mindamage_percent', 'item_maxdamage_percent') for r in isc if r.get('Stat') == n)
    if len(dmg) != 2: raise Refuse('itemstatcost lacks the item damage percent rows')
    h, pr = t['properties.txt']; need(h, ('code',) + tuple('%s%d' % (c, k) for k in range(1, 8) for c in ('func', 'stat', 'val')), 'properties')
    props = {r['code']: r for r in pr if r.get('code')}
    h, sets = t['sets.txt']; need(h, ('index',) + tuple('%s%d%s' % (c, n, s) for n in range(2, 6) for s in 'ab' for c in ('PCode', 'PParam', 'PMin', 'PMax'))
                                  + tuple('%s%d' % (c, n) for n in range(1, 9) for c in ('FCode', 'FParam', 'FMin', 'FMax')), 'sets')
    sets = [r for r in sets if r.get('index') != 'Expansion']
    h, items = t['setitems.txt']; need(h, ('set', 'add func') + tuple('%s%d%s' % (c, n, s) for n in range(1, 6) for s in 'ab' for c in ('aprop', 'apar', 'amin', 'amax')), 'setitems')
    items = [r for r in items if r.get('index') != 'Expansion']
    out = ['DM_GEAR\t%d\t%s' % (GEAR_FORMAT, sha)]
    for r in isc:
        op = num(r.get('op'))
        if not op or r.get('Stat') not in sid: continue
        tg = [sid[r[k]] for k in ('op stat1', 'op stat2', 'op stat3') if r.get(k) in sid]
        if not tg: continue
        out.append('op\t%d\t%d\t%d\t%d\t%s' % (sid[r['Stat']], op, num(r.get('op param')) or 0, 1 if (r.get('op base') or '').strip() == 'level' else 0, ','.join(map(str, tg))))
    for st in sorted(shift): out.append('shift\t%d\t%d' % (st, shift[st]))
    row = {r['index']: i for i, r in enumerate(sets)}
    pieces = [0] * len(sets)
    for r in items:
        if r.get('set') not in row: raise Refuse('set item names an unknown set')
        pieces[row[r['set']]] += 1

    def emit(kind, owner, level, prop, par, lo, hi):
        if not prop: return 0
        lo_, hi_ = num(lo), num(hi)
        if lo_ is None: lo_ = 0
        if hi_ is None: hi_ = lo_
        stats, first = prop_stats(prop, (par or '').strip(), lo_, hi_, props, sid, dmg)
        if stats is None: out.append('%sx\t%d\t%d\t%d' % (kind, owner, level, first)); return 1
        for stat, layer, v in stats: out.append('%sb\t%d\t%d\t%d\t%d\t%d' % (kind, owner, level, stat, layer, v << shift.get(stat, 0)))
        return 1
    nb = 0
    for i, r in enumerate(sets):
        out.append('set\t%d\t%d' % (i, pieces[i]))
        for n in range(2, 6):
            for s in 'ab': nb += emit('set', i, n, r['PCode%d%s' % (n, s)], r['PParam%d%s' % (n, s)], r['PMin%d%s' % (n, s)], r['PMax%d%s' % (n, s)])
        for n in range(1, 9): nb += emit('set', i, 0, r['FCode%d' % n], r['FParam%d' % n], r['FMin%d' % n], r['FMax%d' % n])
    for i, r in enumerate(items):
        out.append('item\t%d\t%d\t%d' % (i, row[r['set']], num(r.get('add func')) or 0))
        for n in range(1, 6):
            for s in 'ab': nb += emit('item', i, n, r['aprop%d%s' % (n, s)], r['apar%d%s' % (n, s)], r['amin%d%s' % (n, s)], r['amax%d%s' % (n, s)])
    out.append('end\t%d' % (len(out) + 1))
    return '\n'.join(out) + '\n', len(sets), len(items), nb


def write_gear(romfs, dst):

    p = os.path.join(dst, 'gear.tsv')
    try:
        sys.path.insert(0, HERE); import romfs_art
        text, ns, ni, nb = build_gear(lambda path: romfs_art.read(romfs, path))
        with open(p, 'w', encoding='utf-8', newline='\n') as f: f.write(text)
    except (Exception, SystemExit) as e:
        if os.path.exists(p): os.remove(p)
        print('gear: no set data (%r), set pieces stay unrated' % (e,))
        return False
    print('Ausrüstung: gear.tsv, %d Sets, %d Set-Teile, %d Boni' % (ns, ni, nb))
    return True


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--gear-sha':
        sys.path.insert(0, HERE); import romfs_art
        raw = {p: romfs_art.read(sys.argv[2], p) for p in GEAR_TABLES}
        print(hashlib.sha256(b''.join(hashlib.sha256(raw[p]).digest() for p in GEAR_TABLES)).hexdigest()); sys.exit(0)
    if len(sys.argv) == 3 and sys.argv[1] == '--sha':
        sys.path.insert(0, HERE); import romfs_art
        raw = {p: romfs_art.read(sys.argv[2], p) for p in TABLES}
        print(hashlib.sha256(b''.join(hashlib.sha256(raw[p]).digest() for p in TABLES)).hexdigest()); sys.exit(0)
    sys.exit(0 if write(sys.argv[1], sys.argv[2]) else 1)
