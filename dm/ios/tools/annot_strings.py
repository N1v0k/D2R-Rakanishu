#!/usr/bin/env python3










import csv, hashlib, io, os, re, sys
import touch_layout

HERE = os.path.dirname(os.path.abspath(__file__))
STR = '/lng/strings/'


LANGS = ('zhCN', 'esES', 'esMX')


GENDER_TAG = re.compile(r'(?:\[[fm][sp]\])+')


class Strings:

    def __init__(self, romfs, repl):
        self.romfs, self.repl, self.text, self.path, self.dirty = romfs, repl, {}, {}, set()
        self.col = 'zhCN'

    def _skip(self, name):

        return self.col != 'zhCN' and name == 'chinese-overlay.json'

    def _split(self, v):

        m = GENDER_TAG.match(v) if self.col != 'zhCN' else None
        return (m.group(), v[m.end():]) if m else ('', v)

    def _load(self, name):
        if name in self.text: return
        path, text = touch_layout.read_layout(self.romfs, None, suffix=STR + name)
        prev = self.repl.get(path.lower())
        if prev: text = open(prev, encoding='utf-8-sig').read()
        self.text[name], self.path[name] = text, path

    def _iter(self, name, key):
        if self._skip(name): return []
        self._load(name)
        return list(re.finditer(r'"Key":\s*"%s",.*?"%s":\s*("(?:[^"\\]|\\.)*")' % (re.escape(key.replace('"', '\\"')), self.col), self.text[name], re.S))

    def has(self, name, key): return bool(self._iter(name, key))

    def keys(self, name):
        self._load(name)
        return re.findall(r'"Key":\s*"((?:[^"\\]|\\.)*)"', self.text[name])

    def get(self, name, key):
        import json
        m = self._iter(name, key)
        return self._split(json.loads(m[0].group(1)))[1] if m else None

    def get_lang(self, name, key, lang):

        import json
        self._load(name)
        m = re.search(r'"Key":\s*"%s",.*?"%s":\s*("(?:[^"\\]|\\.)*")' % (re.escape(key), lang), self.text[name], re.S)
        return json.loads(m.group(1)) if m else None

    def set(self, name, key, fn):

        import json
        ms = self._iter(name, key)
        t = self.text[name]
        for m in reversed(ms):
            tag, v = self._split(json.loads(m.group(1)))
            nv = fn(v)




            if self.col != 'zhCN': nv = nv.replace('[', '(').replace(']', ')')
            t = t[:m.start(1)] + json.dumps(tag + nv, ensure_ascii=False) + t[m.end(1):]
        if ms: self.text[name] = t; self.dirty.add(name)
        return len(ms)

    def write(self, outdir):
        out = {}
        for name in sorted(self.dirty):
            o = os.path.join(outdir, 'annot_' + name)
            open(o, 'w', encoding='utf-8-sig', newline='').write(self.text[name]); out[self.path[name].lower()] = o
        return out


def excel(romfs, name):
    _, text = touch_layout.read_layout(romfs, None, suffix='/global/excel/' + name)
    return list(csv.DictReader(io.StringIO(text), delimiter='\t'))




ITEM_FILES = ('item-names.json', 'chinese-overlay.json')


def text_hash(t):

    return hashlib.sha1(t.encode('utf-8')).hexdigest()[:8]


OLDNAMES = os.path.join(HERE, 'annot', 'oldnames.tsv')
def old_rows():




    if not os.path.isfile(OLDNAMES):
        if not getattr(old_rows, 'said', 0): old_rows.said = 1; print('annot_strings：没有旧译名对照表 %s，不换旧译名（其余注释照常）' % OLDNAMES, file=sys.stderr)
        return []
    return list(csv.DictReader(open(OLDNAMES, encoding='utf-8'), delimiter='\t'))


def old_names(s):
    n = 0
    for row in old_rows():
        f, k = row['文件'] + '.json', row['键']
        new = row['旧译名'].replace('\\n', '\n')
        cur = s.get(f, k)
        if cur is None: raise SystemExit('annot_strings：%s 里没有键 %s' % (f, k))
        if text_hash(cur) != row['原文校验']: raise SystemExit('annot_strings：%s / %s 的原文变了（校验 %s，现行「%s」）——原包换了版本，核过再改表' % (f, k, row['原文校验'], cur))
        n += s.set(f, k, lambda _: new)
    return n


def rune_numbers(s):






    n = 0
    if s.col != 'zhCN':
        for i in range(1, 34):
            nm, short = s.get('item-runes.json', 'r%02d' % i), s.get('item-runes.json', 'r%02dL' % i)
            if not nm or not short or not re.fullmatch(r"\S+ %s" % re.escape(short), nm) or not re.fullmatch(r"[A-Za-z']+", short): raise SystemExit('annot_strings：%s 的 r%02d / r%02dL 不是「Runa 短名」/「短名」：%r %r' % (s.col, i, i, nm, short))
            n += s.set('item-runes.json', 'r%02d' % i, lambda _, i=i, nm=nm: '%s #%d' % (nm, i))
            n += s.set('item-runes.json', 'r%02dL' % i, lambda _, i=i, short=short: '%s#%d ' % (short, i))
        return n
    for i in range(1, 34):
        zh, en = s.get('item-runes.json', 'r%02dL' % i), s.get_lang('item-runes.json', 'r%02dL' % i, 'enUS')
        if not zh or not en or not re.fullmatch(r"[A-Za-z']+", en): raise SystemExit('annot_strings：r%02dL 的中 / 英短名不对：%r %r' % (i, zh, en))
        n += s.set('item-runes.json', 'r%02d' % i, lambda _, i=i, zh=zh, en=en: '%s %s #%d' % (zh, en, i))




        n += s.set('item-runes.json', 'r%02dL' % i, lambda _, i=i, zh=zh: '%s#%d ' % (zh, i))
    return n


def is_shield(code, types):
    seen = set()
    while code and code not in seen:
        if code == 'shld': return True
        seen.add(code); code = types.get(code, {}).get('Equiv1', '')
    return False




TIER_WORDS = {'zhCN': (('普', '扩', '精'), {'0': '轻', '5': '中', '10': '重'}, '%s[%s]'),
              'esES': (('Nor', 'Exc', 'Éli'), {'0': 'lig', '5': 'med', '10': 'pes'}, '%s [%s]'),
              'esMX': (('Nor', 'Exc', 'Éli'), {'0': 'lig', '5': 'med', '10': 'pes'}, '%s [%s]')}


def base_tiers(s, romfs):




    (wn, wu, wl), wsp, fmt = TIER_WORDS[s.col]
    types = {r['Code']: r for r in excel(romfs, 'itemtypes.txt') if r.get('Code')}
    n = 0
    for tab in ('weapons.txt', 'armor.txt'):
        for r in excel(romfs, tab):
            c = r.get('code')
            if not c or r.get('spawnable') != '1' or r.get('quest') not in ('', '0') or r.get('type') == 'tpot': continue
            tier = wn if c == r['normcode'] else wu if c == r['ubercode'] else wl if c == r['ultracode'] else None
            if not tier: continue
            if tab == 'armor.txt' and (r['type'] == 'tors' or is_shield(r['type'], types)):
                tier += '·' + wsp[r['speed']]
            key = r['namestr'] or c
            for f in ITEM_FILES:
                n += s.set(f, key, lambda t, tier=tier: fmt % (t, tier))
    return n


def comments(s):








    renamed, orig = {}, Strings(s.romfs, s.repl)
    for row in old_rows():
        if row['文件'] == 'skills' and row['键'].lower().startswith('skillname'):
            cur = orig.get('skills.json', row['键'])
            if cur and cur != row['旧译名']: renamed[cur] = row['旧译名']
    kept = {s.get('skills.json', k) for k in s.keys('skills.json') if k.lower().startswith('skillname')}
    renamed = {a: b for a, b in renamed.items() if a not in kept}
    n = 0
    cp = os.path.join(HERE, 'annot', 'comments.tsv')
    if not os.path.isfile(cp):
        print('annot_strings：没有点评表 %s，不加点评（其余注释照常）' % cp, file=sys.stderr); return 0
    for row in csv.DictReader(open(cp, encoding='utf-8'), delimiter='\t'):
        c, color = row['点评'], '\u00ffc2' if row['名字颜色'] == 'set' else '\u00ffc4'
        bad = [a for a in renamed if a in c and renamed[a] not in c]
        if bad: raise SystemExit('annot_strings：%s 的点评「%s」里有 3.3 现行技能名 %s（应写旧译名 %s）' % (row['键'], c, bad, [renamed[a] for a in bad]))
        def add(t, c=c, color=color):
            if '\n' in t: head, name = t.rsplit('\n', 1); return '%s\n\u00ffc2%s\n%s' % (head, c, name)
            return '\u00ffc2%s\n%s%s' % (c, color, t)
        files = ('item-runes.json', 'chinese-overlay.json') if row['文件'] == 'item-runes' else ITEM_FILES
        k = sum(s.set(f, row['键'], add) for f in files)
        if not k: raise SystemExit('annot_strings：点评的键 %s 在字符串表里没有' % row['键'])
        n += k
    return n


def apply_all(s, romfs):






    from annot_ranges import ann_ranges
    from annot_world import ann_levels, ann_shrines
    from annot_drop import ann_drop
    stat = {}
    for col in LANGS:
        s.col = col; zh = col == 'zhCN'
        st = {'旧译名': old_names(s) if zh else 0, '可变范围': ann_ranges(s, romfs), '点评': comments(s) if zh else 0, '符文序号': rune_numbers(s), '底材档次': base_tiers(s, romfs),
              '区域等级': ann_levels(s, romfs), '神殿效果': ann_shrines(s, romfs),
              '掉落名': ann_drop(s, romfs)}
        if zh: stat.update(st)
        else: stat[col] = {k: v for k, v in st.items() if k not in ('旧译名', '点评')}
    s.col = 'zhCN'
    return stat












TEXTBOX_TABLE = 'dm_textbox_plain.tsv'
TEXTBOX_STEPS = ('旧译名', '符文序号', '底材档次', '区域等级')
_last_table = None


def apply_textbox(s, romfs):
    from annot_world import ann_levels
    steps = {'旧译名': lambda: old_names(s) if s.col == 'zhCN' else 0, '符文序号': lambda: rune_numbers(s), '底材档次': lambda: base_tiers(s, romfs), '区域等级': lambda: ann_levels(s, romfs)}
    out = {}
    for col in LANGS:
        s.col = col; out[col] = {k: steps[k]() for k in TEXTBOX_STEPS}
    s.col = 'zhCN'
    return out


def textbox_table(full, plain):

    rt = lambda t: t.replace('\u00ffc', '\ue07e')


    from annot_drop import FILES as LOAD_ORDER
    order = [n for n in LOAD_ORDER if n in full.dirty] + sorted(n for n in full.dirty if n not in LOAD_ORDER)
    tab, src = {}, {}
    for col in LANGS:
        full.col = plain.col = col; eff = {}
        for name in order:
            for key in full.keys(name):
                a, b = full.get(name, key), plain.get(name, key)
                if a is not None and b is not None: eff[(name if name not in LOAD_ORDER else '*', key)] = (a, b)
        for k, (a, b) in eff.items():
            if 'ÿc' not in a or a == b: continue
            ra, rb = rt(a), rt(b)
            if tab.get(ra, rb) != rb and src[ra][0] != col:
                tab[ra] = re.sub('\\ue07e.', '', ra); src[ra] = (src[ra][0] + '/' + col,) + k; continue
            if tab.get(ra, rb) != rb: raise SystemExit('annot_strings：注释版 %r 对到两种文字框版 %r（%s）/ %r（%s %s）' % (a, tab[ra], src[ra], rb, col, k))
            tab[ra] = rb; src[ra] = (col,) + k
    full.col = plain.col = 'zhCN'
    return tab


def write_textbox_table(dst):

    if _last_table is None: raise SystemExit('annot_strings：还没 build 就写文字框对照表')
    esc = lambda t: t.replace('\\', '\\\\').replace('\n', '\\n').replace('\t', '\\t')
    with open(os.path.join(dst, TEXTBOX_TABLE), 'w', encoding='utf-8', newline='\n') as f:
        f.write('# 注释版\t文字框版（annot_strings.py 生成，宿主 dm_textbox.c 读）\n')
        for a in sorted(_last_table): f.write('%s\t%s\n' % (esc(a), esc(_last_table[a])))
    return len(_last_table)


def build(romfs, outdir, repl):

    global _last_table
    s = Strings(romfs, repl)
    stat = apply_all(s, romfs)
    p = Strings(romfs, repl); apply_textbox(p, romfs)
    _last_table = textbox_table(s, p); stat['文字框对照'] = len(_last_table)
    return s.write(outdir), stat

if __name__ == '__main__':
    import sys
    os.makedirs(sys.argv[2], exist_ok=True)
    r, st = build(sys.argv[1], sys.argv[2], {})
    print(st); [print(k, '→', v) for k, v in r.items()]
