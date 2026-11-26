#!/usr/bin/env python3













import argparse
import csv
import json
import os
import re
import sys
from datetime import date
from urllib.parse import urlparse

DM = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
DEFAULT = os.path.join(DM, 'android', 'glossary', 'terms.json')
VERSION = '3.4.95035'
CLASSES = ('ama', 'sor', 'nec', 'pal', 'bar', 'dru', 'ass', 'war')
SECTIONS = ('what', 'how', 'helps', 'not', 'rule')
CATEGORIES = ('def', 'off', 'speed')
LANGS = ('de', 'en')
PHRASES = ('noLive', 'def', 'pct', 'defWith', 'defWould', 'noModel', 'hitYou', 'hitYouWith', 'hitYouWould', 'block', 'noBlock', 'diff', 'res', 'dr', 'mdr', 'plr',
           'ar', 'arNone', 'hitIt', 'value', 'leech', 'leechDiff', 'cast', 'recover', 'next', 'top', 'noClass', 'diff0', 'diff1', 'diff2')
DATE = re.compile(r'^\d{4}-\d{2}-\d{2}$')


class Errors(list):
    def add(self, where, what): self.append('%s: %s' % (where, what))


def text_pair(e, where, v, allow_empty=False):
    if not isinstance(v, dict) or set(v) != set(LANGS): e.add(where, 'needs exactly de and en'); return
    for l in LANGS:
        if not isinstance(v[l], str): e.add(where, '%s is not text' % l)
    if not allow_empty and any(not v[l].strip() for l in LANGS if isinstance(v[l], str)): e.add(where, 'de and en must both be filled')
    if allow_empty and isinstance(v.get('de'), str) and isinstance(v.get('en'), str) and bool(v['de'].strip()) != bool(v['en'].strip()):
        e.add(where, 'de and en must both be filled or both be empty')


def check_date(e, where, s):
    if not isinstance(s, str) or not DATE.match(s): e.add(where, 'date must be YYYY-MM-DD'); return
    try:
        if date.fromisoformat(s) > date.today(): e.add(where, 'date lies in the future')
    except ValueError: e.add(where, 'not a real date')


def check_sources(e, where, refs, sources, need=2):
    if not isinstance(refs, list) or not refs: e.add(where, 'no sources'); return
    sites = set()
    for r in refs:
        if r not in sources: e.add(where, 'unknown source %r' % r)
        else: sites.add(sources[r]['site'])
    if len(sites) < need: e.add(where, 'needs sources from at least %d different sites' % need)


def check_steps(e, where, table):
    if not isinstance(table, list) or len(table) < 2: e.add(where, 'needs at least two steps'); return
    prev = None
    for i, s in enumerate(table):
        if not (isinstance(s, list) and len(s) == 2 and all(isinstance(x, int) for x in s)): e.add(where, 'step %d is not [percent, frames]' % i); return
        if i == 0 and s[0] != 0: e.add(where, 'the first step must start at 0 %')
        if prev and not (s[0] > prev[0] and s[1] < prev[1]): e.add(where, 'step %d must need more percent for fewer frames' % i)
        if s[1] <= 0: e.add(where, 'step %d has no frames' % i)
        prev = s


def check(data, errors=None):
    e = errors if errors is not None else Errors()
    if not isinstance(data, dict): e.add('file', 'not an object'); return e
    for k in ('schema', 'version', 'gameVersion', 'categories', 'sources', 'breakpoints', 'phrases', 'terms'):
        if k not in data: e.add('file', 'missing %s' % k)
    if e: return e
    if data['schema'] != 1: e.add('schema', 'must be 1')
    if not isinstance(data['version'], int) or data['version'] < 1: e.add('version', 'must be a positive number')
    if data['gameVersion'] != VERSION: e.add('gameVersion', 'must be %s' % VERSION)
    cats = [c.get('id') for c in data['categories'] if isinstance(c, dict)]
    if tuple(cats) != CATEGORIES: e.add('categories', 'must be %s in this order' % (CATEGORIES,))
    for c in data['categories']: text_pair(e, 'categories.%s' % c.get('id'), {l: c.get(l) for l in LANGS})
    sources = data['sources']
    for sid, s in sources.items():
        w = 'sources.%s' % sid
        for k in ('site', 'title', 'url', 'seen', 'lang', 'note'):
            if k not in s: e.add(w, 'missing %s' % k)
        if 'url' in s:
            u = urlparse(s['url'])
            if u.scheme != 'https' or not u.netloc: e.add(w, 'url must be https')
        if 'seen' in s: check_date(e, w + '.seen', s['seen'])
        if 'note' in s: text_pair(e, w + '.note', s['note'])
    if e: return e
    for kind in ('fcr', 'fhr'):
        b = data['breakpoints'].get(kind)
        w = 'breakpoints.%s' % kind
        if not isinstance(b, dict): e.add(w, 'missing'); continue
        if set(b.get('classes', {})) != set(CLASSES): e.add(w, 'needs all eight classes %s' % (CLASSES,))
        for cls, t in b.get('classes', {}).items(): check_steps(e, '%s.%s' % (w, cls), t)
        srcs = b.get('sources', {})
        check_sources(e, w + '.sources.classic', srcs.get('classic'), sources)
        check_sources(e, w + '.sources.war', srcs.get('war'), sources)
        text_pair(e, w + '.note', b.get('note'))

    for k in PHRASES:
        if k not in data['phrases']: e.add('phrases', 'missing %s' % k)
    for k, v in data['phrases'].items():
        w = 'phrases.%s' % k
        text_pair(e, w, v)
        if isinstance(v, dict) and all(isinstance(v.get(l), str) for l in LANGS):
            if sorted(re.findall(r'\{\d\}', v['de'])) != sorted(re.findall(r'\{\d\}', v['en'])): e.add(w, 'de and en use different placeholders')
            for l in LANGS:
                if v[l].count('[') > 1 or v[l].count('[') != v[l].count(']') or v[l].find('[') > v[l].find(']'): e.add(w, '%s: at most one [value] pair' % l)
    seen = set()
    for i, t in enumerate(data['terms']):
        w = 'terms[%d]' % i
        tid = t.get('id')
        if not isinstance(tid, str) or not re.match(r'^[a-z]+$', tid): e.add(w, 'id must be lower-case letters'); continue
        w = 'terms.%s' % tid
        if tid in seen: e.add(w, 'id used twice')
        seen.add(tid)
        if t.get('cat') not in CATEGORIES: e.add(w, 'unknown category')
        n = t.get('name')
        if not (isinstance(n, dict) and len(n) == 1 and (isinstance(n.get('stat'), int) or isinstance(n.get('group'), int) or isinstance(n.get('word'), str))):
            e.add(w, 'name must be one of {stat: id}, {group: stat id}, {word: key}')
        text_pair(e, w + '.fallback', t.get('fallback'))
        kw = t.get('keywords')
        if not (isinstance(kw, list) and all(isinstance(k, str) and k == k.lower() and k.strip() for k in kw)): e.add(w, 'keywords must be lower-case words')
        st = t.get('stats')
        if not (isinstance(st, list) and st and all(isinstance(x, int) and x >= 0 for x in st)): e.add(w, 'stats must list stat ids')
        tx = t.get('text')
        if not isinstance(tx, dict) or set(tx) != set(SECTIONS): e.add(w, 'text needs exactly %s' % (SECTIONS,))
        else:
            for s in SECTIONS: text_pair(e, '%s.text.%s' % (w, s), tx[s])
        text_pair(e, w + '.limits', t.get('limits'), allow_empty=True)
        check_sources(e, w + '.sources', t.get('sources'), sources)
        if not (isinstance(t.get('checked'), list) and t['checked']): e.add(w, 'checked must name the tables the term was checked against')
    if not 1 <= len(seen) <= 40: e.add('terms', 'between 1 and 40 terms')
    return e


def read_table(path):
    with open(path, encoding='utf-8-sig', newline='') as fh: return list(csv.DictReader(fh, delimiter='\t'))


def check_tables(data, tables, errors=None):
    e = errors if errors is not None else Errors()
    path = lambda f: os.path.join(tables, f)
    for f in ('itemstatcost.txt', 'difficultylevels.txt', 'shrines.txt'):
        if not os.path.isfile(path(f)): e.add('tables', 'missing %s' % f); return e
    stats = {int(r['*ID']): r['Stat'] for r in read_table(path('itemstatcost.txt')) if r.get('*ID', '').isdigit()}
    diff = {r['Name']: r for r in read_table(path('difficultylevels.txt'))}
    for t in data['terms']:
        w = 'terms.%s' % t['id']
        ids = list(t['stats']) + [v for k, v in t['name'].items() if k in ('stat', 'group')]
        for s in ids:
            if s not in stats: e.add(w, 'stat %d is not in itemstatcost.txt' % s)
        for c in t['checked']:
            f, _, col = c.partition(':')
            if not os.path.isfile(path(f)): e.add(w, 'checked table %s is missing' % f); continue
            if not col or f == 'shrines.txt' or f == 'states.txt': continue
            if re.match(r'^\d+(-\d+)?$', col):
                a, _, b = col.partition('-')
                for s in range(int(a), int(b or a) + 1):
                    if s not in stats: e.add(w, 'checked stat %d is not in itemstatcost.txt' % s)
            else:
                with open(path(f), encoding='utf-8-sig') as fh: head = fh.readline().rstrip('\r\n').split('\t')
                if col not in head: e.add(w, 'column %s is not in %s' % (col, f))

    want = {'Normal': (0, 0, 1), 'Nightmare': (-40, -20, 2), 'Hell': (-100, -50, 3)}
    for name, (p, pc, div) in want.items():
        r = diff.get(name)
        if not r: e.add('difficultylevels.txt', 'no row %s' % name); continue
        if int(r['ResistPenalty']) != p or int(r['ResistPenaltyNonExpansion']) != pc: e.add('terms.resist', '%s penalty in the table is %s/%s, the text says %d/%d' % (name, r['ResistPenalty'], r['ResistPenaltyNonExpansion'], p, pc))
        if int(r['LifeStealDivisor']) != div or int(r['ManaStealDivisor']) != div: e.add('terms.leech', '%s divisor in the table is %s/%s, the text says %d' % (name, r['LifeStealDivisor'], r['ManaStealDivisor'], div))

    shrine = [r for r in read_table(path('shrines.txt')) if r.get('Name') == 'Armor Shrine']
    if not shrine: e.add('shrines.txt', 'no Armor Shrine')
    elif shrine[0].get('Arg0') != '100' or shrine[0].get('Duration in frames') != '2400': e.add('terms.enhdef', 'the Armor Shrine is not +100 % for 2400 frames in shrines.txt')

    if os.path.isfile(path('skills.txt')):
        amp = [r for r in read_table(path('skills.txt')) if r.get('skill') == 'Amplify Damage']
        if not amp or amp[0].get('aurastat1') != 'damageresist' or amp[0].get('aurastatcalc1') != '-par5' or amp[0].get('Param5') != '100':
            e.add('terms.dr', 'Amplify Damage does not subtract 100 points of damageresist in skills.txt')

    if os.path.isfile(path('monstats.txt')):
        dia = [r for r in read_table(path('monstats.txt')) if r.get('Id') == 'diablo']
        if not dia or dia[0].get('ResDm(H)') != '45': e.add('terms.crushing', 'Hell Diablo is not 45 % physical resistance in monstats.txt')
    return e


def main(argv=None):
    ap = argparse.ArgumentParser(description='check the glossary data (#59)')
    ap.add_argument('--tables', help='directory of the extracted 3.4.95035 .txt tables')
    ap.add_argument('file', nargs='?', default=DEFAULT)
    a = ap.parse_args(argv)
    with open(a.file, encoding='utf-8') as fh: data = json.load(fh)
    e = check(data)
    if not e and a.tables: check_tables(data, a.tables, e)
    for x in e: print('check_glossary: ' + x)
    if e: return 1
    print('check_glossary: %s ok (%d terms%s)' % (os.path.basename(a.file), len(data['terms']), ', tables checked' if a.tables else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
