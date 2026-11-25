#!/usr/bin/env python3




import csv, json, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
TABLE = os.path.join(HERE, '..', '..', 'i18n', 'ui_text.tsv')
LANGS = ('enUS', 'zhTW', 'deDE', 'esES', 'frFR', 'itIT', 'koKR', 'plPL', 'esMX', 'jaJP', 'ptBR', 'ruRU', 'zhCN')
ID0 = 300000
GAME_COLUMN = {'zhCN': 'zh', 'zhTW': 'zh', 'esES': 'es', 'esMX': 'mx', 'ruRU': 'ru', 'koKR': 'ko', 'deDE': 'de', 'ptBR': 'pt'}

PROPER = '老机型补充包|D2R Mobile Resources|D2R Mobile'
FMT = re.compile(r'%[-+ #0-9.]*(?:ll|l|h|z)?[a-zA-Z@%]')
POS = re.compile(r'%(\d+)\$[-+ #0-9.]*(?:ll|l|h|z)?([a-zA-Z@])')










EN_LIMITS = (
    (r'dmRiftSay(?!Prefix).*', 31),
    (r'dmRiftPtsGot', 62), (r'dmRiftPtsHeld', 33),
    (r'dmRift(Range|Best|Str|Lvl|Time)', 13),
    (r'dmRiftGam(Uni|Set)', 8), (r'dmRiftGamAfterEn', 17),
    (r'dmRift(SelT|InfT|GamT|TitleRun|TitleDone|TitleLate)', 16),
    (r'dmRift(GuardDown|NextOpen|Saved|RetryOpen|NotCounted)', 16),
    (r'dmRiftCap.*', 20),
    (r'dmRiftBtn.*', 12),
    (r'dmRiftMsItem\d+', 15), (r'dmRiftMsSub\d+', 19), (r'dmRiftMsTier\d+', 4), (r'dmRiftMs(Got|Next|No)', 7), (r'dmRiftMsT', 12), (r'dmRiftMsNEn', 38),
    (r'dmRiftHud(Title)', 5), (r'dmRiftHud(Done|Late|Guard)', 18), (r'dmRiftHud(Next|Same|Kill|Goal)', 26),
    (r'dmKbRiftUnlock', 9), (r'dmKbRift(Best|Total)', 16),
)




EN_HAS_ZH = {'dmCustomLangZh', 'dmCustomLangHintZh'}




GRAMMAR_MARK = re.compile(r'\[(?:ms|fs|ns|mp|fp|pl|nl|p)\]')
GRAMMAR_LEAD = re.compile(r'(?:\[(?:ms|fs|ns|mp|fp|pl|nl|p)\])+')

def untag(text):


    lead = GRAMMAR_LEAD.match(text)
    if not lead: return text
    rest = text[lead.end():]
    nxt = GRAMMAR_MARK.search(rest)
    return (rest[:nxt.start()] if nxt else rest).strip()




KINDS = ('one', 'few', 'many', 'other')
RULES = {'en': 'en', 'es': 'es', 'mx': 'es', 'ru': 'ru', 'de': 'de', 'pt': 'pt'}
ALLOWED = {'en': {'one', 'other'}, 'es': {'one', 'other'}, 'mx': {'one', 'other'}, 'de': {'one', 'other'}, 'pt': {'one', 'other'}, 'ru': set(KINDS)}

COMPOSED = {'dmRiftPtsGot': ('dmRiftPtsSkill', 'dmRiftPtsStat')}

def forms(cell):

    if not re.match(r'(?:%s)=' % '|'.join(KINDS), cell):
        if '|' in cell: raise ValueError('a plain text holds "|" (reserved for plural forms)')
        return None
    out = {}
    for part in cell.split('|'):
        m = re.fullmatch(r'(%s)=(.*)' % '|'.join(KINDS), part, re.S)
        if not m: raise ValueError('a plural part is not "<one|few|many|other>=<text>": %r' % part)
        if m.group(1) in out: raise ValueError('plural form %s twice' % m.group(1))
        out[m.group(1)] = m.group(2)
    if 'other' not in out: raise ValueError('plural forms without other=')
    return out

def texts(cell):

    f = forms(cell)
    return list(f.values()) if f else [cell]

def stamp(cell, rules):

    return rules + '|' + cell if forms(cell) else cell

def out(row, column):

    k, zh, en, es, mx, ru, ko, de, pt = row
    if column == 'pt' and not pt: return stamp(en, 'en')
    t = {'zh': zh, 'en': en, 'es': es, 'mx': mx, 'ru': ru, 'ko': ko, 'de': de, 'pt': pt}[column]
    return stamp(t, RULES[column]) if column in RULES else t

def plain(cell):

    f = forms(cell)
    return f['other'] if f else cell

def rows():

    out_ = []
    with open(TABLE, encoding='utf-8') as f:
        for r in csv.DictReader(f, delimiter='\t'):
            k = r['键']; zh, en, es, mx, ru, ko, de, pt = (r[c].replace('\\n', '\n') for c in ('中文', 'English', 'Español', 'Español MX', 'Русский', '한국어', 'Deutsch', 'Português (Brasil)'))
            if not re.fullmatch(r'dm[A-Za-z0-9]+', k) or not zh or not en or not es or not ru or not ko or not de: raise SystemExit('dm_i18n：表里这一行不对：%r' % r)
            mx = mx or es
            cols = (('英文', 'en', en), ('西语', 'es', es), ('拉美西语', 'mx', mx), ('俄语', 'ru', ru), ('韩语', 'ko', ko), ('德语', 'de', de), ('葡语', 'pt', pt or en), ('中文', 'zh', zh))
            fs = {}
            for nm, code, t in cols:
                try: fs[code] = forms(t)
                except ValueError as e: raise SystemExit('dm_i18n：%s 的%s：%s' % (k, nm, e))
                if fs[code] and not set(fs[code]) <= ALLOWED.get(code, set()):
                    raise SystemExit('dm_i18n：%s 的%s不许这些复数形式：%s（允许 %s）' % (k, nm, sorted(fs[code]), sorted(ALLOWED.get(code, ()))))
            if any(fs.values()) and k in EN_HAS_ZH: raise SystemExit('dm_i18n：%s 带复数形式又在 EN_HAS_ZH 里' % k)
            en_lines = texts(en)[0].count('\n')
            for nm, code, t in cols[:-1]:
                for x in texts(t):
                    if k not in EN_HAS_ZH and re.search(r'[　-鿿＀-￯]', re.sub(PROPER, '', x)): raise SystemExit('dm_i18n：%s 的%s里有中文字符：%r' % (k, nm, x))
                    if FMT.findall(zh) != FMT.findall(x): raise SystemExit('dm_i18n：%s 的中文与%s占位符不一致：%s / %s' % (k, nm, FMT.findall(zh), FMT.findall(x)))
                    if dict(POS.findall(zh)) != dict(POS.findall(x)) or len(POS.findall(x)) != len(POS.findall(zh)): raise SystemExit('dm_i18n：%s 的中文与%s位置占位符不一致：%s / %s' % (k, nm, POS.findall(zh), POS.findall(x)))
                    if x.count('\n') != en_lines: raise SystemExit('dm_i18n：%s 的%s换行数与英文不同（行高按行数排）：%r' % (k, nm, x))
            lim = next((n for pat, n in EN_LIMITS if re.fullmatch(pat, k)), None)
            for nm, code, t in cols[:-1]:
                for x in texts(t):
                    if lim is not None and k not in COMPOSED and len(FMT.sub('000', x).split('\n')[0]) > lim: raise SystemExit('dm_i18n：%s 的%s「%s」%d 个字符，超过这一栏的上限 %d（会顶框 / 被截断；压短，或给这一栏做专用的小字号控件）' % (k, nm, x, len(FMT.sub('000', x).split('\n')[0]), lim))
            out_.append((k, zh, en, es, mx, ru, ko, de, pt))
    ks = [r[0] for r in out_]
    if len(set(ks)) != len(ks): raise SystemExit('dm_i18n：键重复：%s' % sorted({k for k in ks if ks.count(k) > 1}))
    composed(out_)
    return out_

def composed(table):

    by = {r[0]: r for r in table}
    for frame, parts in COMPOSED.items():
        lim = next((n for pat, n in EN_LIMITS if re.fullmatch(pat, frame)), None)
        if lim is None or frame not in by or any(p not in by for p in parts): raise SystemExit('dm_i18n：COMPOSED %s 缺键或缺上限' % frame)
        for i, nm in ((2, '英文'), (3, '西语'), (4, '拉美西语'), (5, '俄语'), (6, '韩语'), (7, '德语'), (8, '葡语')):
            col = lambda r: (r[i] or r[2]) if i == 8 else r[i]
            longest = [max((FMT.sub('000', x) for x in texts(col(by[p]))), key=len) for p in parts]
            text = plain(col(by[frame]))
            for p in longest: text = text.replace('%s', p, 1)
            n = len(FMT.sub('000', text).split('\n')[0])
            if n > lim: raise SystemExit('dm_i18n：%s 的%s拼起来「%s」%d 个字符，超过上限 %d' % (frame, nm, text, n, lim))

def build(romfs, outdir, repl):

    import touch_layout
    path, text = touch_layout.read_layout(romfs, None, suffix='/lng/strings/ui.json')
    prev = repl.get(path.lower())
    if prev: text = open(prev, encoding='utf-8-sig').read()
    have = set(re.findall(r'"Key":\s*"((?:[^"\\]|\\.)*)"', text))
    rs = rows()
    clash = [r[0] for r in rs if r[0] in have]
    if clash: raise SystemExit('dm_i18n：键和原版重名：%s' % clash)
    add = [{'id': ID0 + i, 'Key': r[0], **{l: out(r, GAME_COLUMN.get(l, 'en')) for l in LANGS}} for i, r in enumerate(rs)]
    end = text.rstrip().rindex(']')
    body = ',\n'.join('  ' + json.dumps(e, ensure_ascii=False) for e in add)
    text = text[:end].rstrip() + ',\n' + body + '\n' + text[end:]
    json.loads(text)
    o = os.path.join(outdir, 'i18n_ui.json'); open(o, 'w', encoding='utf-8-sig', newline='').write(text)
    return {path.lower(): o}, len(add)

def layout_plurals():

    keys = {r[0] for r in rows() if any(forms(t) for t in r[1:] if t)}
    for name in sorted(os.listdir(HERE)):
        if not name.endswith('.py'): continue
        text = open(os.path.join(HERE, name), encoding='utf-8').read()
        hit = sorted(k for k in keys if re.search(r'@%s(?![A-Za-z0-9])' % k, text))
        if hit: raise SystemExit('dm_i18n：%s 在布局里写了带复数形式的键 %s（布局没有数量，只能在代码里按数量取）' % (name, hit))

def header():

    c = lambda s: json.dumps(s, ensure_ascii=False)
    lines = ['// 生成文件，勿手改：python3 dm/ios/tools/dm_i18n.py --header > dm/host/dm_i18n_tab.h（表 dm/i18n/ui_text.tsv）', '#pragma once',
           'static const struct { const char *k, *zh, *en, *es, *mx, *ru, *ko, *de, *pt; } k_dm_i18n[] = {']
    for r in rows():
        lines.append('    { %s, %s, %s, %s, %s, %s, %s, %s, %s },' % (c(r[0]), *(c(out(r, col)) for col in ('zh', 'en', 'es', 'mx', 'ru', 'ko', 'de')), c(out(r, 'pt') if r[8] else '')))
    return '\n'.join(lines + ['};', ''])

if __name__ == '__main__':
    import sys
    if sys.argv[1:] == ['--header']: sys.stdout.write(header())
    elif sys.argv[1:] == ['--check']:
        h = os.path.join(HERE, '..', '..', 'host', 'dm_i18n_tab.h')
        if open(h, encoding='utf-8').read() != header(): raise SystemExit('dm_i18n：dm/host/dm_i18n_tab.h 过期，重跑 python3 dm/ios/tools/dm_i18n.py --header > dm/host/dm_i18n_tab.h')
        layout_plurals()
        print('dm_i18n OK %d 条' % len(rows()))
    else: raise SystemExit('用法：dm_i18n.py --header | --check')
