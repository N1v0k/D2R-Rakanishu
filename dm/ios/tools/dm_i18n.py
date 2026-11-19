#!/usr/bin/env python3




import csv, json, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
TABLE = os.path.join(HERE, '..', '..', 'i18n', 'ui_text.tsv')
LANGS = ('enUS', 'zhTW', 'deDE', 'esES', 'frFR', 'itIT', 'koKR', 'plPL', 'esMX', 'jaJP', 'ptBR', 'ruRU', 'zhCN')
ID0 = 300000

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

def rows():

    out = []
    with open(TABLE, encoding='utf-8') as f:
        for r in csv.DictReader(f, delimiter='\t'):
            k = r['键']; zh, en, es, mx, ru, ko, de, pt = (r[c].replace('\\n', '\n') for c in ('中文', 'English', 'Español', 'Español MX', 'Русский', '한국어', 'Deutsch', 'Português (Brasil)'))
            if not re.fullmatch(r'dm[A-Za-z0-9]+', k) or not zh or not en or not es or not ru or not ko or not de: raise SystemExit('dm_i18n：表里这一行不对：%r' % r)
            mx = mx or es
            for nm, t in (('英文', en), ('西语', es), ('拉美西语', mx), ('俄语', ru), ('韩语', ko), ('德语', de), ('葡语', pt or en)):
                if k not in EN_HAS_ZH and re.search(r'[　-鿿＀-￯]', re.sub(PROPER, '', t)): raise SystemExit('dm_i18n：%s 的%s里有中文字符：%r' % (k, nm, t))
                if FMT.findall(zh) != FMT.findall(t): raise SystemExit('dm_i18n：%s 的中文与%s占位符不一致：%s / %s' % (k, nm, FMT.findall(zh), FMT.findall(t)))
                if dict(POS.findall(zh)) != dict(POS.findall(t)) or len(POS.findall(t)) != len(POS.findall(zh)): raise SystemExit('dm_i18n：%s 的中文与%s位置占位符不一致：%s / %s' % (k, nm, POS.findall(zh), POS.findall(t)))
                if t.count('\n') != en.count('\n'): raise SystemExit('dm_i18n：%s 的%s换行数与英文不同（行高按行数排）：%r' % (k, nm, t))
            lim = next((n for pat, n in EN_LIMITS if re.fullmatch(pat, k)), None)
            for nm, t in (('英文', en), ('西语', es), ('拉美西语', mx), ('俄语', ru), ('韩语', ko), ('德语', de), ('葡语', pt or en)):
                if lim is not None and len(FMT.sub('000', t).split('\n')[0]) > lim: raise SystemExit('dm_i18n：%s 的%s「%s」%d 个字符，超过这一栏的上限 %d（会顶框 / 被截断；压短，或给这一栏做专用的小字号控件）' % (k, nm, t, len(FMT.sub('000', t).split('\n')[0]), lim))
            out.append((k, zh, en, es, mx, ru, ko, de, pt))
    ks = [r[0] for r in out]
    if len(set(ks)) != len(ks): raise SystemExit('dm_i18n：键重复：%s' % sorted({k for k in ks if ks.count(k) > 1}))
    return out

def build(romfs, outdir, repl):

    import touch_layout
    path, text = touch_layout.read_layout(romfs, None, suffix='/lng/strings/ui.json')
    prev = repl.get(path.lower())
    if prev: text = open(prev, encoding='utf-8-sig').read()
    have = set(re.findall(r'"Key":\s*"((?:[^"\\]|\\.)*)"', text))
    rs = rows()
    clash = [r[0] for r in rs if r[0] in have]
    if clash: raise SystemExit('dm_i18n：键和原版重名：%s' % clash)
    col = lambda l, zh, en, es, mx, ru, ko, de, pt: zh if l in ('zhCN', 'zhTW') else es if l == 'esES' else mx if l == 'esMX' else ru if l == 'ruRU' else ko if l == 'koKR' else de if l == 'deDE' else (pt or en) if l == 'ptBR' else en
    add = [{'id': ID0 + i, 'Key': k, **{l: col(l, zh, en, es, mx, ru, ko, de, pt) for l in LANGS}} for i, (k, zh, en, es, mx, ru, ko, de, pt) in enumerate(rs)]
    end = text.rstrip().rindex(']')
    body = ',\n'.join('  ' + json.dumps(e, ensure_ascii=False) for e in add)
    text = text[:end].rstrip() + ',\n' + body + '\n' + text[end:]
    json.loads(text)
    o = os.path.join(outdir, 'i18n_ui.json'); open(o, 'w', encoding='utf-8-sig', newline='').write(text)
    return {path.lower(): o}, len(add)

def header():

    c = lambda s: json.dumps(s, ensure_ascii=False)
    out = ['// 生成文件，勿手改：python3 dm/ios/tools/dm_i18n.py --header > dm/host/dm_i18n_tab.h（表 dm/i18n/ui_text.tsv）', '#pragma once',
           'static const struct { const char *k, *zh, *en, *es, *mx, *ru, *ko, *de, *pt; } k_dm_i18n[] = {']
    out += ['    { %s, %s, %s, %s, %s, %s, %s, %s, %s },' % (c(k), c(zh), c(en), c(es), c(mx), c(ru), c(ko), c(de), c(pt)) for k, zh, en, es, mx, ru, ko, de, pt in rows()]
    return '\n'.join(out + ['};', ''])

if __name__ == '__main__':
    import sys
    if sys.argv[1:] == ['--header']: sys.stdout.write(header())
    elif sys.argv[1:] == ['--check']:
        h = os.path.join(HERE, '..', '..', 'host', 'dm_i18n_tab.h')
        if open(h, encoding='utf-8').read() != header(): raise SystemExit('dm_i18n：dm/host/dm_i18n_tab.h 过期，重跑 python3 dm/ios/tools/dm_i18n.py --header > dm/host/dm_i18n_tab.h')
        print('dm_i18n OK %d 条' % len(rows()))
    else: raise SystemExit('用法：dm_i18n.py --header | --check')
