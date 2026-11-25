

























import math, re

FILES = ('item-names.json', 'item-runes.json', 'chinese-overlay.json')
NAME_FILES = FILES + ('monsters.json', 'npcs.json')
RED, BLUE, BLUE2, PURPLE, ORANGE, GREEN = 'ÿc1', 'ÿc3', 'ÿc5', 'ÿc;', 'ÿc8', 'ÿc2'
SIZE = ('微', '小', '中', '大', '超')
SHORT = {'tsc': '回城', 'aqv': '箭', 'cqv': '弩矢', 'rvs': PURPLE + '小紫', 'rvl': PURPLE + '大紫'}
STRIP = {'isc': '卷轴', 'yps': '药水'}
GEM_COLOR = {'gema': '紫', 'gemt': '黄', 'gems': '蓝', 'geme': '绿', 'gemr': '红', 'gemd': '钻', 'gemz': '骷髅'}
GEM_GRADE = (('碎裂', ''), ('裂开', ''), ('', ''), ('无瑕', RED), ('完美', RED))
GEM_GRADE_CH = {'c': 0, 'f': 1, 's': 2, 'l': 3, 'z': 3, 'p': 4}
SKULL_GRADE_CH = {'c': 0, 'f': 1, 'u': 2, 'l': 3, 'z': 4}
HIGH_RUNE = 24
SOURCE_NOTE = {'pk1': '火炬钥匙', 'pk2': '火炬钥匙', 'pk3': '火炬钥匙', 'dhn': '地狱火炬材料', 'bey': '地狱火炬材料', 'mbr': '地狱火炬材料',
               'tes': '合成赦免勋章', 'ceh': '合成赦免勋章', 'bet': '合成赦免勋章', 'fed': '合成赦免勋章'}
SOURCE_COLOR = {'pk1': RED, 'pk2': ORANGE, 'pk3': GREEN}
BOSS_ACT = {'The Countess': 1, 'The Summoner': 2, 'Nihlathak Boss': 5}








VALUE_BASES = {
    **{c: {} for c in ('ob1', 'ob2', 'ob3', 'ob4', 'ob5', 'ob6', 'ob7', 'ob8', 'ob9', 'oba', 'obb', 'obc', 'obd', 'obe', 'obf')},
    **{c: {} for c in ('9lw', '9tw', '7lw', '7tw', 'ci1', 'ci2', 'ci3', 'uhm', 'usk', 'utu', 'upl', 'uth', 'uul', 'uar', 'utp', 'amd', 'paa')},
    'xtp': {'value': 'range'},
    'urn': {'value': 'max', 'eth': True},
}


TOP_RUNEWORDS = {'7wa': ('Oath', 'Death', 'Breath of the Dying')}
WEAPON_VALUE = {'7wa': {'value': 'range', 'eth': True}}
FRAMED = ('ci3',)
CLASS_MAGIC = ('ama', 'amf')
CRAFT_GLOVE_MODS = {'crush': '压碎', 'mana-kill': '杀敌回蓝'}
_ROMAN = ('', 'I', 'II', 'III', 'IV', 'V')




_ES = {
    'pot': '{c} {s}', 'pot_color': {'hpot': 'Roja', 'mpot': 'Azul'},
    'short': {'tsc': 'Portal', 'rvs': PURPLE + 'Morada', 'rvl': PURPLE + 'Morada total'},
    'strip': {'isc': r'^Pergamino de ', 'yps': r'^Poción '},
    'gem': '{c} {w}', 'gem_color': {'gema': 'Amat.', 'gemt': 'Topa.', 'gems': 'Zaf.', 'geme': 'Esm.', 'gemr': 'Rubí', 'gemd': 'Diam.', 'gemz': 'Crán.'},
    'gem_rune': ' (Runa #%d)',
    'source': dict(**{c: 'Llave para {torch}' for c in ('pk1', 'pk2', 'pk3')}, **{c: 'Material para {torch}' for c in ('dhn', 'bey', 'mbr')},
                   **{c: 'Para crear {token}' for c in ('tes', 'ceh', 'bet', 'fed')}),
    'or': ' o ',
    'class_magic': 'Mágico: hab. +%d, vel. ataque +%d', 'weapon_value': 'Vel. ataque %s · Daño %d-%d', 'def_range': 'Defensa %d-%d', 'def': 'Defensa %d',
    'can_make': 'Para ', 'star': '*', 'craft': ' «Artesanal: %s»', 'craft_mods': {'crush': 'golpe aplastante', 'mana-kill': 'maná al matar'},
}
WORDS = {
    'zhCN': {'pot': '{s}{c}', 'size': SIZE, 'pot_color': {'hpot': '红', 'mpot': '蓝'}, 'short': SHORT, 'strip': {k: re.escape(v) + '$' for k, v in STRIP.items()},
             'gem': '{w}-{c}', 'gem_color': GEM_COLOR, 'grade': tuple(w for w, _ in GEM_GRADE), 'gem_rune': ' (合%d)',
             'source': SOURCE_NOTE, 'or': '或', 'act': lambda n: ' 第%s幕' % n,
             'class_magic': '魔法 技能+%d 攻速+%d', 'weapon_value': '攻速 %s · 伤害 %d-%d', 'def_range': '防御 %d-%d', 'def': '防御 %d',
             'can_make': '可做 ', 'star': '★', 'craft': ' 〈合成·%s〉', 'craft_mods': CRAFT_GLOVE_MODS},
    'esES': dict(_ES, size=('menor', 'ligera', '', 'superior', 'súper'), grade=('frag.', 'estrop.', '', 'sin def.', 'perf.'),
                 act=lambda n: ' Acto %s' % _ROMAN[int(n)]),
    'esMX': dict(_ES, size=('menor', 'leve', '', 'mayor', 'especial'), grade=('mell.', 'imperf.', '', 'impec.', 'perf.'),
                 act=lambda n: ' Acto %s' % n),
}


def _strings_get(s, key):
    v = [s.get(f, key) for f in NAME_FILES if s.has(f, key)]
    return v[-1] if v else None


class Tables:

    def __init__(self, romfs):
        from annot_strings import excel
        self.misc = {r['code']: r for r in excel(romfs, 'misc.txt') if r.get('code')}
        self.weap = {r['code']: r for r in excel(romfs, 'weapons.txt') if r.get('code')}
        self.armor = {r['code']: r for r in excel(romfs, 'armor.txt') if r.get('code')}
        self.types = {r['Code']: r for r in excel(romfs, 'itemtypes.txt') if r.get('Code')}
        self.cube = excel(romfs, 'cubemain.txt')
        self.runes = {r['*Rune Name']: r for r in excel(romfs, 'runes.txt') if r.get('*Rune Name')}
        self.tc = excel(romfs, 'treasureclassex.txt')
        self.monstats = excel(romfs, 'monstats.txt'); self.superuniques = excel(romfs, 'superuniques.txt')
        self.automagic = excel(romfs, 'automagic.txt'); self.prefix = excel(romfs, 'magicprefix.txt'); self.suffix = excel(romfs, 'magicsuffix.txt')
        self.parents = {}
        for r in self.tc:
            for k in range(1, 11):
                i = r.get('Item%d' % k, '')
                if i: self.parents.setdefault(i, set()).add(r['Treasure Class'])

    def ancestors(self, t):
        out, st = set(), [t]
        while st:
            x = st.pop()
            if x and x not in out:
                out.add(x); st += [self.types.get(x, {}).get('Equiv1', ''), self.types.get(x, {}).get('Equiv2', '')]
        return out

    def gem_rune(self, code):

        for r in self.cube:
            ins = [(r.get('input %d' % k, '') or '').split(',')[0] for k in range(1, 8)]; out = r.get('output', '')
            if code in ins and re.fullmatch(r'r\d\d', out) and any(re.fullmatch(r'r\d\d', i) for i in ins): return int(out[1:])
        return None

    def sources(self, code):

        seen, st = set(), [code]
        while st:
            for p in self.parents.get(st.pop(), ()):
                if p not in seen: seen.add(p); st.append(p)
        out = []
        for r in self.superuniques:
            if any(r.get(c) in seen for c in r if c.startswith('TC') and '(H)' in c): out.append(('su', r['Superunique'], r.get('Name')))
        if out: return out
        for r in self.monstats:
            if any(r.get(c) in seen for c in r if c.startswith('TreasureClass') and '(H)' in c): out.append(('mon', r.get('Id'), r.get('NameStr')))
        return out

    def class_magic(self, code):

        a = self.ancestors(self.weap[code]['type'])
        def best(tab, mods, spawn):
            v = 0
            for r in tab:
                if spawn and r.get('spawnable') != '1': continue
                its = {r.get('itype%d' % k, '') for k in range(1, 8)}; ex = {r.get('etype%d' % k, '') for k in range(1, 6)}
                if not (its & a) or (ex & a): continue
                for k in range(1, 4):
                    if r.get('mod%dcode' % k) in mods: v = max(v, int(r.get('mod%dmax' % k) or 0))
            return v
        return best(self.automagic, ('skilltab',), False) + best(self.prefix, ('skilltab',), True), best(self.suffix, ('swing1', 'swing2', 'swing3'), True)

    def craft_glove(self, code):

        r = self.armor[code]; fam = {r.get('normcode'), r.get('ubercode'), r.get('ultracode')}
        for c in self.cube:
            if (c.get('input 1', '') or '').split(',')[0] in fam:
                for k in range(1, 6):
                    if c.get('mod %d' % k) in CRAFT_GLOVE_MODS: return c.get('mod %d' % k)
        return None


def _rune_line(code, name):
    m = re.fullmatch(r"(.+) ([A-Za-z']+) (#\d+)", name)
    if not m: return None
    i, txt = int(code[1:]), ORANGE + m.group(1) + ' ' + BLUE2 + m.group(2) + GREEN + m.group(3)
    if i < HIGH_RUNE: return txt
    bar, gap = ORANGE + '—' * (13 if i >= 28 else 9), '\n' * (4 if i >= 27 else 2)
    return bar + gap + txt + gap + bar


def misc_rule(T, s, code, name):

    row = T.misc[code]; t = row.get('type', ''); L = WORDS[s.col]
    if code in L['short']: return L['short'][code]
    m = re.search(L['strip'][code], name) if code in L['strip'] else None
    if m: r = name[:m.start()] + name[m.end():]; return r[:1].upper() + r[1:]
    if t in ('hpot', 'mpot') and code[-1:].isdigit():
        i = int(code[-1]) - 1; w = L['pot'].format(s=L['size'][i], c=L['pot_color'][t]).strip()
        return (RED if t == 'hpot' else BLUE) + w if i == 4 else w
    if t in GEM_COLOR:
        g = SKULL_GRADE_CH.get(code[2]) if t == 'gemz' else GEM_GRADE_CH.get(code[1])
        if g is None: return None
        word, color = L['grade'][g], GEM_GRADE[g][1]
        base = color + L['gem'].format(w=word, c=L['gem_color'][t]) if word else name
        n = T.gem_rune(code) if g <= 2 else None
        return base + (L['gem_rune'] % n if n else '')
    if t == 'rune' and re.fullmatch(r'r\d\d', code): return _rune_line(code, name)
    if code in SOURCE_NOTE:
        src = T.sources(code)
        if not src: raise SystemExit('annot_drop：%s 在掉落表里找不到来源首领' % code)
        who = L['or'].join(dict.fromkeys(_strings_get(s, key) or key for _, _, key in src))
        act = next((BOSS_ACT[x[1]] for x in src if x[1] in BOSS_ACT), None)
        color = SOURCE_COLOR.get(code, GREEN)
        nm = lambda f, k: re.sub(r'\u00ffc.', '', s.get(f, k).split('\n')[-1])
        note = L['source'][code].format(torch=nm('item-names.json', 'Hellfire Torch'), token=nm('item-modifiers.json', 'toa')) if s.col != 'zhCN' else L['source'][code]
        return color + '[' + who + ' · ' + note + ']\n' + color + name + (L['act'](act) if act else '')
    m = re.search(r'Act ?(\d)', row.get('name', ''))
    if m and t == 'ques' and code.startswith('ua'): return name + L['act'](m.group(1))
    return None


def base_rule(T, s, code, name):

    parts = []; L = WORDS[s.col]
    w, a = T.weap.get(code), T.armor.get(code)
    if code in CLASS_MAGIC:
        sk, ias = T.class_magic(code); parts.append(L['class_magic'] % (sk, ias))
    vb = VALUE_BASES.get(code) or WEAPON_VALUE.get(code)
    if vb and vb.get('value'):
        k = 1.5 if vb.get('eth') else 1.0
        if w:
            lo, hi = int(w['mindam'] or w['2handmindam']), int(w['maxdam'] or w['2handmaxdam'])
            parts.append(L['weapon_value'] % (w.get('speed') or '0', lo, math.floor(math.ceil(hi * k) * 1.15)))
        else:
            lo, hi = int(a['minac']), int(a['maxac']); top = math.floor(math.floor((hi + 1) * k) * 1.15)
            parts.append((L['def_range'] % (lo, top)) if vb['value'] == 'range' else L['def'] % top)
    if code in TOP_RUNEWORDS:
        names = []
        for rw in TOP_RUNEWORDS[code]:
            r = T.runes[rw]; its = {r.get('itype%d' % k, '') for k in range(1, 7)}
            if not (its & T.ancestors((w or a)['type'])): raise SystemExit('annot_drop：%s 做不了 %s' % (code, rw))
            nm = (_strings_get(s, r['Name']) or rw).split('\n')[-1]
            names.append(re.sub(r'\u00ffc.', '', nm))
        parts.append(L['can_make'] + ' / '.join(names))
    glove = T.craft_glove(code) if a and a.get('type') == 'glov' else None
    out = name
    if code in VALUE_BASES: out = L['star'] + out
    if glove: out += L['craft'] % L['craft_mods'][glove]
    if parts: out = '[' + ' · '.join(parts) + ']\n' + out
    if code in FRAMED: out = '—' * 7 + '\n' + out + '\n' + '—' * 7
    return out if out != name else None


def ann_drop(s, romfs):

    T = Tables(romfs); n = 0
    for tab, rule in ((T.misc, misc_rule), (T.weap, base_rule), (T.armor, base_rule)):
        for code, row in tab.items():
            key = row.get('namestr') or code
            cur = [s.get(f, key) for f in FILES if s.has(f, key)]
            if not cur: continue
            new = rule(T, s, code, cur[-1])
            if new is None or new == cur[-1]: continue
            n += sum(s.set(f, key, lambda _t, v=new: v) for f in FILES)
    return n
