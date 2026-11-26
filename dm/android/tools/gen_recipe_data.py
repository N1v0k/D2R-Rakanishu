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

VERSION = '3.4.95035'
DATA_VERSION = 3

TABLES = tuple('global/excel/%s' % f for f in ('cubemain.txt', 'cubemain.bin', 'Runes.txt', 'runes.bin', 'armor.txt', 'weapons.txt', 'misc.txt', 'itemtypes.txt', 'uniqueitems.txt', 'setitems.txt', 'gems.txt', 'properties.txt', 'itemstatcost.txt', 'skills.txt', 'skilldesc.txt')) + \
    tuple('local/lng/strings/%s.json' % f for f in ('item-names', 'item-runes', 'item-gems', 'item-modifiers', 'item-nameaffixes', 'levels', 'ui', 'skills')) + ('hd/items/items.json',)
TABLES_SHA = '5066a5c0ac9df221a7eaf815af7c9141ccd9c8f407e626a342171469495694bc'

RESULT_SHA = '85d9d749e4e6c7043bd6c7f1d1cc80a8ded80a852143fe2cf77d6341f947848a'


TYPE_KEYS = {'any': 'AdvancedItems', 'hpot': 'hp3', 'mpot': 'mp3', 'gemz': 'sku', 'gemr': 'gsr', 'gemd': 'gsw', 'rod': 'stave', 'blun': 'maces', 'hamm': 'maces', 'shld': 'shlds', 'axe': 'axes', 'wand': 'wands',
             'grim': 'wa5', 'h2h': 'clw', 'head': 'necro', 'amul': 'amu', 'ring': 'rin'}
ANY_COLOUR = ('gem0', 'gem1', 'gem2', 'gem3', 'gem4')
CONDS = {'low': 'LootFilterInferiorRarity', 'nor': 'NormalQuality', 'hiq': 'LootFilterSuperiorRarity', 'mag': 'LootFilterMagicRarity', 'rar': 'LootFilterRareRarity', 'set': 'LootFilterSetRarity',
         'uni': 'LootFilterUniqueRarity', 'bas': 'NormalQuality', 'exc': 'ExceptionalQuality', 'eli': 'EliteQuality'}
OWN_CONDS = {'nos': ('nicht gesockelt', 'not socketed'), 'noe': ('nicht ätherisch', 'not ethereal'), 'upg': ('auch höhere Stufe', 'also higher tier'),
             'fil': ('Sockel gefüllt', 'sockets filled')}
QUALITIES = ('low', 'nor', 'hiq', 'mag', 'rar', 'set', 'uni')
CAT_NAMES = {'runes': ('Runen', 'Runes'), 'gems': ('Edelsteine', 'Gems'), 'craft': ('Handwerk', 'Crafting'), 'socket': ('Sockelfassungen', 'Sockets'), 'improve': ('Verbessern', 'Improve'),
             'quest': ('Quest', 'Quest'), 'ext': ('Erweiterung', 'Expansion')}
FAMILIES = (('gpr', 'Blut', 'Blood'), ('gpv', 'Zauberer', 'Caster'), ('gpb', 'Treffer', 'Hit Power'), ('gpg', 'Sicherheit', 'Safety'))
SLOTS = (('helm', 'hlm'), ('boot', 'lbt'), ('glov', 'glove'), ('belt', 'mbl'), ('shld', 'shield'), ('tors', 'armor'), ('amul', 'amu'), ('ring', 'rin'), ('weap', 'weap'))
GEM_COLOURS = ('gema', 'gemt', 'gems', 'geme', 'gemr', 'gemd', 'gemz')
PORTALS = {'Cow Portal': ('To The Moo Moo Farm',), 'Pandemonium Portal': ('strPandemoniumEventTitle', 'Pandemonium Run 1', 'Pandemonium Run 2', 'Pandemonium Run 3'),
           'Pandemonium Finale Portal': ('strPandemoniumEventTitle', 'Tristram'), 'Red Portal': ('ToTheColossalSummit',)}
PORTAL_ART = 'tsc'
POISON = ('pois-min', 'pois-max', 'pois-len')
MOD_KEYS = {'dmg%': 'strModEnhancedDamage', 'res-all': 'strModAllResistances', 'dmg-min': 'ModStr1g', 'dmg-max': 'ModStr1f'}
IMPROVE_TEXT = {'reroll': [('Würfelt den Gegenstand neu, gleicher Grundtyp', 'Rerolls the item, same base type')], 'normal': [('Macht den Gegenstand normal', 'Makes the item normal')],
                'upgrade': [('Hebt den Grundtyp eine Stufe an, Stufe +5 auf die Anforderung', 'Raises the base type one tier, +5 to the level requirement')],
                'repair': [('Stellt die Haltbarkeit her', 'Restores durability')], 'unsocket': [('Leert die Sockelfassungen, die Einsätze gehen verloren', 'Empties the sockets, what was in them is lost')]}
KEEP_TEXT = [('Die Einsätze kommen einzeln zurück', 'The contents come back one by one'), ('Haltbarkeit fällt auf 0, danach reparieren', 'Durability drops to 0, repair it afterwards')]
HELL_KEY = 'strCreateGameHellText'
BASE_GROUPS = ('weap', 'tors', 'helm', 'shld')
CHRONICLE = 'hd/global/ui/PANEL/Chronicle/'


TERMS = {'runewords': 'Runewords', 'cat.runes': 'runes', 'cat.gems': 'gems', 'cat.socket': 'ModStre8c', 'cat.ext': 'Expansion', 'all': 'ChronicleFilterSelect',
         'group.weap': 'strBSWeapons', 'group.tors': 'strBSArmor', 'group.helm': 'hlm', 'group.shld': 'shield',
         'place.inventory': 'strpanel4', 'place.shared': 'SharedHeader', 'place.chronicle': 'Chronicle', 'page': 'SharedStashPage'}
UI = {'tab_off': ('Chronicle_Tabs', 0), 'tab_on': ('Chronicle_Tabs', 1), 'input_l': ('Chronicle_InputBox_Left', 0), 'input_c': ('Chronicle_InputBox_Center', 0), 'input_r': ('Chronicle_InputBox_Right', 0)}


class Refuse(Exception):
    """This RomFS gets no book."""


def record_size(data, lines, name):

    n = struct.unpack_from('<I', data, 0)[0]
    size, rest = divmod(len(data) - 4, n) if n else (0, 1)
    if n != lines or rest: raise Refuse('%s holds %d lines in %d bytes' % (name, n, len(data)))
    return size


def game_word(pair):

    return tuple(re.sub(r'\s*\(%[a-z]\)', '', x).strip() for x in pair)


def more(n):
    return ('Dazu 1 weitere Eigenschaft', 'Plus 1 more property') if n == 1 else ('Dazu %d weitere Eigenschaften' % n, 'Plus %d more properties' % n)


def fold(s):

    s = s.lower().replace('ß', 'ss')
    for a, b in (('ä', 'a'), ('ö', 'o'), ('ü', 'u'), ('ae', 'a'), ('oe', 'o'), ('ue', 'u')): s = s.replace(a, b)
    return re.sub(r'[^0-9a-z\u0080-￿]+', ' ', s).strip()


def clean(s):

    m = re.match(r'\[\w\w\]([^\[]*)', s)
    return (m.group(1) if m else s).strip()


def table(text):
    rows = list(csv.reader(io.StringIO(text), delimiter='\t', quoting=csv.QUOTE_NONE))
    return [dict(zip(rows[0], r)) for r in rows[1:]]


def word_form(s):

    forms = dict(re.findall(r'\[(\w\w)\]([^\[]*)', s))
    return forms['fs'].strip().capitalize() if len(forms) > 1 and 'fs' in forms else clean(s)


def strings(read):
    out, raw = {}, {}
    for f in ('item-names', 'item-runes', 'item-gems', 'item-modifiers', 'item-nameaffixes', 'levels', 'ui', 'skills'):
        for e in json.loads(read('local/lng/strings/%s.json' % f).decode('utf-8-sig')):
            if 'Key' in e: out.setdefault(e['Key'], (clean(e['deDE']), clean(e['enUS']))); raw.setdefault(e['Key'], e['deDE'])
    return out, raw


def token(raw):

    p = raw.strip().strip('"').split(',')
    flags, values = [], {}
    for x in p[1:]:
        if '=' in x: k, v = x.split('=', 1); values[k] = v
        else: flags.append(x)
    return p[0], flags, values


class Catalog:
    def __init__(self, read):
        self.names, self.raw = strings(read)
        self.items, self.uniques = {}, {}
        for f in ('armor', 'weapons', 'misc'):
            for r in table(read('global/excel/%s.txt' % f).decode('utf-8', 'replace')):
                if r.get('code') and r['code'] not in self.items: self.items[r['code']] = r
        self.types = {r['Code']: r for r in table(read('global/excel/itemtypes.txt').decode('utf-8', 'replace')) if r['Code']}
        for r in table(read('global/excel/uniqueitems.txt').decode('utf-8', 'replace')):
            if r.get('index'): self.uniques[r['index']] = r
        self.sets = table(read('global/excel/setitems.txt').decode('utf-8', 'replace'))
        self.gems = {r['code']: r for r in table(read('global/excel/gems.txt').decode('utf-8', 'replace')) if r.get('code')}
        self.props = {r['code']: r for r in table(read('global/excel/properties.txt').decode('utf-8', 'replace')) if r.get('code')}
        self.stats = {r['Stat']: r for r in table(read('global/excel/itemstatcost.txt').decode('utf-8', 'replace'))}
        desc = {r['skilldesc']: r for r in table(read('global/excel/skilldesc.txt').decode('utf-8', 'replace'))}
        self.skills = {r['*Id']: desc.get(r['skilldesc'], {}).get('str name', '') for r in table(read('global/excel/skills.txt').decode('utf-8', 'replace')) if r.get('*Id')}
        self.used_items, self.used_types = {}, {}
        self.sockets, self.socketed = game_word(self.names['ModStre8c']), game_word(self.names['Socketable'])
        self.cat_names = dict(CAT_NAMES, gems=game_word(self.names['gems']), socket=self.sockets)

    def lineage(self, t, seen=None):

        seen = seen if seen is not None else []
        if t and t not in seen and t in self.types:
            seen.append(t)
            for k in ('Equiv1', 'Equiv2'): self.lineage(self.types[t][k], seen)
        return seen

    def is_type(self, code, want):
        r = self.items[code]
        return want in self.lineage(r['type']) or want in self.lineage(r.get('type2', ''))

    def stand_in(self, t):

        if t == 'any': return 'amu'
        fits = [(r['type'] != t, int(r['level']) or 999, code) for code, r in self.items.items()
                if code in self.names and self.is_type(code, t) and r.get('level', '').isdigit() and not self.is_type(code, 'ques')]
        if not fits: raise Refuse('no item of type %s' % t)
        return min(fits, key=lambda x: x[:2])[2]

    def item_name(self, code):
        key = code if code in self.names else self.items.get(code, {}).get('namestr')
        if key not in self.names: raise Refuse('no name for item %s' % code)
        return self.names[key]

    def use_item(self, code):
        if code in self.used_items: return
        r = self.items[code]
        e = {'de': self.item_name(code)[0], 'en': self.item_name(code)[1], 'w': int(r['invwidth'] or 1), 'h': int(r['invheight'] or 1), 'type': r['type']}
        if r.get('type2'): e['type2'] = r['type2']
        for k, name in (('normcode', 'norm'), ('ubercode', 'uber'), ('ultracode', 'ultra')):
            if r.get(k) and r[k] != 'xxx': e[name] = r[k]
        if code + 'L' in self.names: e['short'] = self.names[code + 'L']
        self.used_items[code] = e

    def type_name(self, t, img):
        if t in ANY_COLOUR: return self.item_name(img)
        r = self.types.get(t, {})
        for key in (TYPE_KEYS.get(t), t, r.get('UICategory'), r.get('RunewordCategory1')):
            if key in self.names: return self.names[key]
        raise Refuse('no name for item type %s' % t)

    def use_type(self, t):
        if t not in self.used_types:
            img = self.stand_in(t); self.use_item(img)
            name = self.type_name(t, img)
            self.used_types[t] = {'de': name[0], 'en': name[1], 'img': img, 'up': self.lineage(t)[1:]}
        return self.used_types[t]

    def fill(self, key, values):

        out = []
        for lang in (0, 1):
            s, left = self.names[key][lang], list(values)
            def sub(m):
                if m.group(0) == '%%': return '%'
                v = left.pop(0) if left else ''
                if isinstance(v, tuple): v = v[lang]
                if m.group(0) == '%+d' and not str(v).startswith(('-', '+')): return '+' + str(v)
                return str(v)
            if len(re.findall(r'%\+?[di]|%s', s)) != len(values) or None in values: return None
            out.append(re.sub(r'%%|%\+?[di]|%s', sub, s).strip())
            if left or '' in values: return None
        return tuple(out)

    @staticmethod
    def lines(*rows): return None if None in rows else list(rows)

    def mod_line(self, code, param, lo, hi):

        amount = lo if lo == hi else '%s-%s' % (lo, hi)
        if code in MOD_KEYS: return self.lines(self.fill(MOD_KEYS[code], [amount]))
        p = self.props.get(code)
        stat = self.stats.get(p.get('stat1', '')) if p else None
        if p and p.get('stat3'): return None
        if p and p.get('func1') == '15' and self.stats.get(p.get('stat2', '')):
            return self.lines(self.fill(self.stats[p['stat1']]['descstrpos'], [lo]), self.fill(self.stats[p['stat2']]['descstrpos'], [hi]))
        if p and p.get('stat2'): return None
        if not stat or not stat.get('descstrpos') or stat['descstrpos'] not in self.names: return None
        key = stat['descstrpos']
        if stat.get('descfunc') == '15':
            skill = self.names.get(self.skills.get(param or '', ''))
            return self.lines(self.fill(key, [lo, hi, skill])) if skill else None
        if param not in ('', '0', None): return None
        if '%' in self.names[key][1]: return self.lines(self.fill(key, [amount]))
        side = stat.get('descval')
        return [tuple('%s %s' % (amount, n) if side == '1' else '%s %s' % (n, amount) if side == '2' else n for n in self.names[key])]

    def mod_lines(self, mods):

        lines, unknown, poison = [], 0, False
        for code, param, lo, hi in mods:
            if code in POISON: poison = True; continue
            got = self.mod_line(code, param, lo, hi)
            if got: lines += got
            else: unknown += 1
        return lines, unknown + poison

    def preview(self, r, got, code, inputs):

        cat, fam = got[0], got[1]
        rows = []
        if code in self.items and self.items[code].get('levelreq', '0') not in ('', '0'): rows.append(self.fill('ItemStats1p', [self.items[code]['levelreq']]))
        if code in self.gems:
            for group, key in (('weaponMod', 'GemXp3'), ('helmMod', 'GemXp1'), ('shieldMod', 'GemXp2')):
                g = self.gems[code]
                lines, unknown = self.mod_lines([(g['%s%dCode' % (group, i)], g['%s%dParam' % (group, i)], g['%s%dMin' % (group, i)], g['%s%dMax' % (group, i)]) for i in range(1, 4) if g.get('%s%dCode' % (group, i))])
                if lines or unknown: rows.append(self.names[key]); rows += lines
                if unknown: rows.append(more(unknown))
        if cat == 'craft':
            lines, unknown = self.mod_lines([(r['mod %d' % i], r['mod %d param' % i], r['mod %d min' % i], r['mod %d max' % i]) for i in range(1, 6) if r['mod %d' % i]])
            rows += lines
            rows.append(('Dazu 1 bis 4 zufällige Eigenschaften', 'Plus 1 to 4 random properties'))
            rows.append(('Gegenstandsstufe: %s %% der Figurenstufe, %s %% der Stufe der Zutat' % (r['plvl'] or 0, r['ilvl'] or 0), 'Item level: %s %% of the character, %s %% of the ingredient' % (r['plvl'] or 0, r['ilvl'] or 0)))
            if unknown: rows.append(more(unknown))
        if cat == 'socket':
            sock = next((i for i in range(1, 6) if r['mod %d' % i] == 'sock'), None)
            rows.append(('%s: %s bis %s, höchstens was der Gegenstand erlaubt' % (self.sockets[0], r['mod %d min' % sock], r['mod %d max' % sock]), '%s: %s to %s, at most what the item allows' % (self.sockets[1], r['mod %d min' % sock], r['mod %d max' % sock])) if sock else ('Eine Sockelfassung dazu', 'One more socket'))
        rows += IMPROVE_TEXT.get(fam, [])
        if fam == 'sunder':
            u = self.uniques[code]
            lines, unknown = self.mod_lines([(u['prop%d' % i], u['par%d' % i], u['min%d' % i], u['max%d' % i]) for i in range(1, 13) if u.get('prop%d' % i)])
            rows += lines
            if unknown: rows.append(more(unknown))
        return [list(x) for x in rows]

    def vault_rows(self):

        rows = {'u' + r['*ID']: r['code'] for r in self.uniques.values() if r.get('*ID') and r.get('code')}
        rows.update({'s' + r['*ID']: r['item'] for r in self.sets if r.get('*ID') and r.get('item')})
        return rows

    def all_items(self):

        out = {}
        for code, r in self.items.items():
            key = code if code in self.names else r.get('namestr')
            de, en = self.names.get(key, ('', ''))
            tier = 2 if r.get('ultracode') == code else 1 if r.get('ubercode') == code else 0
            out[code] = [de, en, r['type'], r.get('type2', ''), tier, r.get('normcode', '') if r.get('normcode') != 'xxx' else '', int(r['invwidth'] or 1), int(r['invheight'] or 1)]
        return out

    def cond(self, f):
        if f == 'sock': return self.socketed
        if f in OWN_CONDS: return OWN_CONDS[f]
        return self.names[CONDS[f]]

    def titled_key(self, keys):

        names = [self.names[k] for k in keys]
        return tuple(names[0][i] + (': ' + ', '.join(n[i] for n in names[1:]) if names[1:] else '') for i in (0, 1))

    def describe(self, raw):

        code, flags, values = token(raw)
        e = {'id': code, 'n': int(values.get('qty', 1))}
        if code in self.items: self.use_item(code); e['k'] = 'i'; e['img'] = code; name = self.item_name(code)
        elif code in self.uniques: base = self.uniques[code]['code']; self.use_item(base); e['k'] = 'u'; e['img'] = base; e['row'] = int(self.uniques[code]['*ID']); name = self.item_name(code)
        else: t = self.use_type(code); e['k'] = 't'; e['img'] = t['img']; name = (t['de'], t['en'])
        e['de'], e['en'] = name
        conds = [c for c in (self.cond(f) for f in flags)]
        if code in ANY_COLOUR: conds.append(('jede Farbe', 'any colour'))
        if flags: e['f'] = flags
        if conds: e['cde'], e['cen'] = (', '.join(x[i] for x in conds) for i in (0, 1))
        return e


def target(c, raw):

    code, flags, _ = token(raw)
    t = c.use_type(code)
    q = [c.cond(f) for f in flags if f in QUALITIES]
    return tuple(t[k] + (' (%s)' % q[0][i] if q else '') for i, k in enumerate(('de', 'en')))


def titled(c, verb, raw): return tuple('%s: %s' % (v, n) for v, n in zip(verb, target(c, raw)))


def classify(c, r):

    inputs = [r['input %d' % i] for i in range(1, 8) if r['input %d' % i]]
    first, out = inputs[0], r['output']
    code, flags, values = token(out)
    mods = [r['mod %d' % i] for i in range(1, 6)]
    first_code, first_flags, _ = token(first)
    if first_code.startswith('PreCrafted') or code == 'Red Portal':
        return ('ext', 'portal' if code == 'Red Portal' else 'sunder', c.titled_key(PORTALS[code]) if code in PORTALS else c.item_name(code), 'unique')
    if code in PORTALS: return ('quest', 'portal', c.titled_key(PORTALS[code]), 'unique')
    if 'pre' in values or 'suf' in values: return None
    if code == 'usetype' and 'crf' in flags:
        for gem, de, en in FAMILIES:
            if any(token(x)[0] == gem for x in inputs):
                for slot, key in SLOTS:
                    if slot in c.lineage(first_code) or (first_code in c.items and c.is_type(first_code, slot)):
                        sde, sen = c.names[key]
                        return ('craft', de, ('%s-%s' % (de, sde), '%s %s' % (en, sen)), 'crafted', FAMILIES.index((gem, de, en)) * 10 + SLOTS.index((slot, key)))
        return None
    if 'sock' in mods or 'sock' in values:
        if 'sock' in values: return ('socket', 'one', titled(c, ('Sockelfassung dazu', 'Add socket'), first), 'white')
        return ('socket', 'add', titled(c, c.sockets, first), 'magic' if 'mag' in flags else 'white')
    if code in ('usetype', 'useitem'):
        if 'uns' in flags: return ('improve', 'unsocket', ('%s leeren' % c.sockets[0], 'Empty sockets'), 'white')
        if 'rep' in flags: return ('improve', 'repair', titled(c, ('Reparieren und aufladen', 'Repair and recharge') if 'rch' in flags else ('Reparieren', 'Repair'), first), 'white')
        if 'exc' in flags or 'eli' in flags:
            tier = c.cond('exc' if 'exc' in flags else 'eli')
            return ('improve', 'upgrade', titled(c, ('Aufwerten zu ' + tier[0], 'Upgrade to ' + tier[1]), first), 'unique' if 'uni' in first_flags else 'set' if 'set' in first_flags else 'rare')
        if 'nor' in flags and 'low' in first_flags: return ('improve', 'normal', titled(c, ('Normal machen', 'Make normal'), first), 'white')
        if 'mag' in flags or 'rar' in flags: return ('improve', 'reroll', titled(c, ('Neu würfeln', 'Reroll'), first), 'magic' if 'mag' in flags else 'rare')
        return None
    if code not in c.items or flags: return None
    if r['op']: return ('quest', 'quest', c.item_name(code), 'unique')
    t = c.items[code]['type']
    if c.is_type(code, 'rune'): return ('runes', 'rune', c.item_name(code), 'crafted', int(code[1:]))
    if t in GEM_COLOURS: return ('gems', t, c.item_name(code), 'white', GEM_COLOURS.index(t) * 10 + [x for x in c.items if c.items[x]['type'] == t].index(code))
    if t == 'rpot': return ('gems', 'potion', c.item_name(code), 'white', 99)
    if t == 'ques' or c.is_type(code, 'ques') or token(first)[0] == 'tes': return ('quest', 'quest', c.item_name(code), 'unique')
    return None


def card_terms(c, title, cat, fam, inputs):

    cat_name = c.cat_names[cat]
    terms = [title[0], title[1], cat_name[0], cat_name[1]] + (list(fam) + ['Handwerk Crafting'] if fam else [])
    for x in inputs: terms += [x['de'], x['en'], x.get('cde', ''), x.get('cen', '')]; terms += list(c.used_items.get(x['id'], {}).get('short', ()))
    return fold(' '.join(terms))


def keep_contents(c, clear):


    e = json.loads(json.dumps(clear))
    rune = [i for i, x in enumerate(e['in']) if x['id'] == 'r15']
    socketed = [i for i, x in enumerate(e['in']) if 'sock' in x.get('f', ())]
    if len(rune) != 1 or len(socketed) != 1 or len(e['in']) != 3: raise Refuse('the socket-clearing recipe changed its inputs')
    e['in'][rune[0]] = c.describe('r08')
    base = e['in'][socketed[0]]
    base['f'] = ['fil' if f == 'sock' else f for f in base['f']]
    conds = [c.cond(f) for f in base['f']]
    base['cde'], base['cen'] = (', '.join(x[i] for x in conds) for i in (0, 1))
    e['fam'], e['app'] = 'keep', 'unsocket'
    e['de'], e['en'] = '%s leeren, Einsätze behalten' % c.sockets[0], 'Empty sockets, keep contents'
    e['pv'] = [list(x) for x in KEEP_TEXT]
    e['s'] = card_terms(c, (e['de'], e['en']), e['cat'], None, e['in'])
    return e


def check_bin(cube, words, cube_bin, rune_bin):

    rec = record_size(cube_bin, len(cube), 'cubemain.bin')
    for i, r in enumerate(cube):
        b = cube_bin[4 + i * rec:4 + (i + 1) * rec]
        num = lambda k: int(r[k] or 0)
        got = (b[0], b[1], b[3], struct.unpack_from('<H', b, 0xc)[0], struct.unpack_from('<H', b, 0xe)[0], struct.unpack_from('<h', b, 0x10)[0], struct.unpack_from('<h', b, 0x12)[0])
        want = (num('enabled'), num('min diff'), num('op'), num('numinputs'), num('version'), num('firstLadderSeason'), num('lastLadderSeason'))
        if got != want: raise Refuse('cube line %d: bin %s, txt %s' % (i, got, want))
        given = [r['input %d' % k] for k in range(1, 8) if r['input %d' % k]]
        inputs = [b[0x14 + 8 * k:0x1c + 8 * k] for k in range(7)]
        used = [x for x in inputs if x[0] or x[1]]
        if len(used) != len(given): raise Refuse('cube line %d: bin has %d inputs, txt %d' % (i, len(used), len(given)))
        for x, g in zip(used, given):
            if max(1, x[7]) != int(token(g)[2].get('qty', 1)): raise Refuse('cube line %d: bin quantity %d, txt %s' % (i, x[7], g))
        if want[0] and sum(max(1, x[7]) for x in used) != want[3]: raise Refuse('cube line %d: quantities do not make numinputs' % i)
    rec = record_size(rune_bin, len(words), 'runes.bin')
    for i, r in enumerate(words):
        b = rune_bin[4 + i * rec:4 + (i + 1) * rec]
        runes = sum(1 for k in range(6) if struct.unpack_from('<i', b, 0x5c + 4 * k)[0] >= 1)
        if b[0x44] != int(r['complete'] or 0) or runes != sum(1 for k in range(1, 7) if r['Rune%d' % k]): raise Refuse('rune word line %d differs' % i)


def build(romfs, read):
    digest = hashlib.sha256()
    for path in TABLES: digest.update(read(path))
    if digest.hexdigest() != TABLES_SHA: raise Refuse('the tables are not those of %s (%s)' % (VERSION, digest.hexdigest()))
    cube_bin, rune_bin = read('global/excel/cubemain.bin'), read('global/excel/runes.bin')
    cube = table(read('global/excel/cubemain.txt').decode('utf-8', 'replace'))
    words = table(read('global/excel/Runes.txt').decode('utf-8', 'replace'))
    check_bin(cube, words, cube_bin, rune_bin)
    c = Catalog(read)
    result, out_cube = [], []
    for row, r in enumerate(cube):
        if r['enabled'] != '1': continue
        got = classify(c, r)
        result.append([row, got[0] if got else 'fun'])
        if not got: continue
        cat, title, colour = got[0], got[2], got[3]
        inputs = [c.describe(r['input %d' % i]) for i in range(1, 8) if r['input %d' % i]]
        code = token(r['output'])[0]
        img = PORTAL_ART if code in PORTALS else inputs[0]['img'] if code in ('usetype', 'useitem') else c.uniques[code]['code'] if code in c.uniques else code
        c.use_item(img)
        fam = next(((f[1], f[2]) for f in FAMILIES if f[1] == got[1]), None)
        e = {'row': row, 'cat': cat, 'fam': got[1], 'quest': 1 if r['op'] else 0, 'out': code if code in c.items else '', 'ord': got[4] if len(got) > 4 else row, 'de': title[0], 'en': title[1], 'colour': colour, 'in': inputs, 'img': img, 'num': int(r['numinputs'])}
        if r['min diff']:
            e['diff'] = int(r['min diff'])
            if e['diff'] == 2: e['nde'], e['nen'] = ('Nur ' + c.names[HELL_KEY][0], c.names[HELL_KEY][1] + ' only')
        if r['firstLadderSeason']: e['season'] = [int(r['firstLadderSeason']), int(r['lastLadderSeason'] or -1)]
        e['s'] = card_terms(c, title, cat, fam, inputs)
        e['pv'] = c.preview(r, got, code, inputs)
        out_cube.append(e)
        result[-1] += [r['output'], img, e['num'], e['quest'], e.get('diff'), e.get('season'), [[x['id'], x['k'], x.get('row'), x['n'], x.get('f')] for x in inputs], e['pv']]
    out_words = []
    for row, r in enumerate(words):
        if r['complete'] != '1': continue
        key, runes = r['Name'], [r['Rune%d' % k] for k in range(1, 7) if r['Rune%d' % k]]
        itype, etype = [r['itype%d' % k] for k in range(1, 7) if r['itype%d' % k]], [r['etype%d' % k] for k in range(1, 4) if r['etype%d' % k]]
        for code in runes: c.use_item(code)
        if key not in c.names or not c.raw[key]: raise Refuse('no name for rune word %s' % key)
        name = (word_form(c.raw[key]), c.names[key][1])
        bases = [c.use_type(t) for t in itype]
        groups = [g for g in BASE_GROUPS if any(g == t or g in c.lineage(t) for t in itype)]
        e = {'row': row, 'key': key, 'de': name[0], 'en': name[1], 'runes': runes, 'itype': itype, 'lvl': max(int(c.items[x]['levelreq'] or 0) for x in runes),
             'bde': ', '.join(dict.fromkeys(b['de'] for b in bases)), 'ben': ', '.join(dict.fromkeys(b['en'] for b in bases)), 'groups': groups, 'img': bases[0]['img']}
        if etype: e['etype'] = etype
        if r['firstLadderSeason']: e['season'] = [int(r['firstLadderSeason']), int(r['lastLadderSeason'] or -1)]
        if r['disallowCraftingInLadder'] == '1' or r['disallowCraftingInNonLadder'] == '1': e['lock'] = [int(r['disallowCraftingInLadder'] or 0), int(r['disallowCraftingInNonLadder'] or 0)]
        terms = [e['de'], e['en'], e['bde'], e['ben'], 'Runenwort Runeword'] + [x for code in runes for x in c.used_items[code].get('short', ())]
        e['s'] = fold(' '.join(terms))
        out_words.append(e)
        result.append([row, runes, itype, etype, e['lvl'], e.get('season'), e.get('lock')])
    extra = {'all': c.all_items(), 'typeUp': {t: c.lineage(t)[1:] for t in c.types}, 'vault': c.vault_rows()}
    result.append(extra)
    got = hashlib.sha256(json.dumps(result, separators=(',', ':')).encode()).hexdigest()
    if got != RESULT_SHA: raise Refuse('the recipes are not those of %s (%s)' % (VERSION, got))
    clear = [i for i, x in enumerate(out_cube) if x['fam'] == 'unsocket']
    if len(clear) != 1: raise Refuse('%d socket-clearing recipes' % len(clear))
    out_cube.insert(clear[0] + 1, keep_contents(c, out_cube[clear[0]]))
    for x in out_words + out_cube: c.use_item(x['img'])
    terms = {k: list(game_word(c.names[v])) for k, v in TERMS.items() if v in c.names and all(game_word(c.names[v]))}
    return {'version': VERSION, 'dataVersion': DATA_VERSION, 'cube': out_cube, 'words': out_words, 'types': c.used_types, 'items': c.used_items, 'terms': terms, **extra}


def sprite_paths(romfs):

    out = {}
    for line in open(os.path.join(romfs, 'data', 'build_manifest.psv'), encoding='utf-8'):
        f = line.rstrip('\n').split('|')
        if len(f) >= 11:
            p = f[9].split(':', 1)[-1]
            m = re.match(r'(?i)data/(hd/global/ui/items/(armor|misc|weapon)/(.+)\.sprite)$', p)
            if m: out[m.group(3).lower()] = m.group(1)
    return out


def art(romfs, read, data, out):
    import romfs_art
    assets = {k: v['asset'] for e in json.loads(read('hd/items/items.json').decode('utf-8-sig')) for k, v in e.items()}
    paths = sprite_paths(romfs)
    os.makedirs(os.path.join(out, 'items'))
    for code in data['items']:
        path = paths.get(assets.get(code, '').lower())
        if not path: raise Refuse('no picture for item %s' % code)
        romfs_art.frame(romfs, path, 0).save(os.path.join(out, 'items', code + '.png'))
    for name, (asset, i) in UI.items(): romfs_art.frame(romfs, CHRONICLE + asset + '.sprite', i).save(os.path.join(out, 'book_%s.png' % name))


def stage(romfs, out):
    import romfs_art
    read = lambda path: romfs_art.read(romfs, path)
    os.makedirs(out, exist_ok=True)
    made = [os.path.join(out, 'recipes.json'), os.path.join(out, 'items')] + [os.path.join(out, 'book_%s.png' % n) for n in UI]
    try:
        data = build(romfs, read)
        art(romfs, read, data, out)
        with open(made[0], 'w', encoding='utf-8', newline='\n') as f: json.dump(data, f, ensure_ascii=False, separators=(',', ':'))
    except (Exception, SystemExit) as e:
        for p in made:
            if os.path.isdir(p): shutil.rmtree(p)
            elif os.path.exists(p): os.remove(p)
        print('recipe book: not built (%r), the game runs without the book' % (e,))
        return False
    print('Rezeptbuch: %d Rezepte, %d Runenwoerter, %d Bilder' % (len(data['cube']), len(data['words']), len(data['items'])))
    return True


if __name__ == '__main__':
    sys.exit(0 if stage(sys.argv[1], sys.argv[2]) else 1)
