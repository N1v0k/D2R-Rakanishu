#!/usr/bin/env python3
























import collections
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(HERE, '..', 'app', 'src', 'main', 'java', 'com', 'xiaoboluo', 'd2r'))
BASELINE = os.path.join(HERE, 'check_style_baseline.txt')
EXEMPT = {'GameStyleData.java', 'GameStyle.java', 'GameText.java'}




STRICT = {'StatusEffects.java', 'EffectLayout.java', 'EffectFormat.java', 'EffectDetail.java', 'TouchOverlay.java', 'HudLevelText.java', 'PotionButtons.java',
          'DamageNumbers.java', 'DamageMotion.java', 'StatusLine.java', 'StatusText.java', 'AppMenu.java', 'HoldRing.java', 'PadLegend.java', 'TransferNotice.java'}
STRICT |= {'RecipeBook.java'}
STRICT |= {'BookPage.java'}

STRICT |= {'PackBuildPage.java', 'StartPage.java', 'PageLayout.java'}
STRICT |= {'BuildBook.java'}








PANEL_FILES = {'RecipeBook.java', 'BuildBook.java', 'TechPage.java', 'AppMenu.java', 'GlossaryPage.java', 'BookPage.java'}
PANEL_RULES = (('panel-backdrop', r'\.drawColor\s*\(\s*0[xX]'), ('panel-frame', r'\bimage\s*\(\s*\w+\s*,\s*"panel"'),
               ('panel-metric', r'\b(?:rowH|subH|footH|tabW|closeW)\s*=\s*[^;,]*?(?:\bh\b|getHeight\s*\(\s*\))\s*\*'), ('panel-metric', r'getHeight\s*\(\s*\)\s*/\s*1080'))
TEXT_DRAWS = ('drawText', 'drawTextRun', 'drawTextOnPath', 'drawPosText')
TEXT_METHODS = ('setTypeface', 'setTextSize', 'setTextAlign', 'measureText', 'getTextBounds', 'breakText', 'getFontMetrics', 'getFontMetricsInt', 'ascent',
                'descent', 'setLetterSpacing', 'getTextSize', 'getTextWidths')
HEADER = ('# check_style.py 的基线：现有的文字样式偏差，每处一行（文件、种类、语句）；#51–#55 改好一处就删一行，空了 = #50 验收\n'
          '# Baseline of check_style.py: the existing text style deviations, one line per place (file, kind, statement); #51-#55 delete a line per fix, empty = #50 accepted\n')


def strip(text):

    out, i, n = list(text), 0, len(text)
    while i < n:
        c = text[i]
        if c in '"\'':
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n': j += 2 if text[j] == '\\' else 1
            i = j + 1
        elif text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            for k in range(i, j): out[k] = ' '
            i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != '\n': out[k] = ' '
            i = j
        else: i += 1
    return ''.join(out)


def skip_string(code, i):
    q, j = code[i], i + 1
    while j < len(code) and code[j] != q and code[j] != '\n': j += 2 if code[j] == '\\' else 1
    return j + 1


def close(code, i):

    depth, j = 0, i
    while j < len(code):
        c = code[j]
        if c in '"\'': j = skip_string(code, j); continue
        if c in '([{': depth += 1
        elif c in ')]}':
            depth -= 1
            if depth == 0: return j
        j += 1
    return len(code)


def split_args(code, i):

    end, out, start, j, depth = close(code, i), [], i + 1, i + 1, 0
    while j < end:
        c = code[j]
        if c in '"\'': j = skip_string(code, j); continue
        if c in '([{': depth += 1
        elif c in ')]}': depth -= 1
        elif c == ',' and depth == 0: out.append(code[start:j].strip()); start = j + 1
        j += 1
    out.append(code[start:end].strip())
    return [a for a in out if a]


def blocks(code, cls):


    out, depth, j = [], 0, 0
    while j < len(code):
        c = code[j]
        if c in '"\'': j = skip_string(code, j); continue
        if c == '{':
            if depth == 1:
                end = close(code, j)
                head = code[max(0, j - 300):j]
                ctor = bool(re.search(r'\b%s\s*\([^()]*\)\s*(?:throws[\w\s.,]+)?$' % re.escape(cls), head)) or bool(re.search(r'[;{}]\s*(?:static\s*)?$', head))
                out.append((j, end, ctor))
                j = end + 1
                continue
            depth += 1
        elif c == '}': depth -= 1
        j += 1
    return out


def statement(code, pos):

    a = max(code.rfind(';', 0, pos), code.rfind('{', 0, pos), code.rfind('}', 0, pos)) + 1
    b = code.find(';', pos)
    return re.sub(r'\s+', ' ', code[a:len(code) if b < 0 else b]).strip()


def calls(code, names):

    for m in re.finditer(r'\b(%s)\s*\(' % '|'.join(map(re.escape, names)), code): yield m.group(1), m.end() - 1


def text_paints(code):



    made = set(re.findall(r'\b(\w+)\s*=\s*new\s+(?:Text)?Paint\s*\(', code))
    params = set(re.findall(r'\b(?:Text)?Paint\s+(\w+)\s*[,)]', code))
    declared = set(re.findall(r'\b(?:Text)?Paint\s+(\w+)\s*[=;]', code))
    known = made | params | declared
    text = set()
    for _, i in calls(code, TEXT_DRAWS):
        a = split_args(code, i)
        if a and re.match(r'\w+\Z', a[-1]): text.add(a[-1])
    text |= set(re.findall(r'\b(\w+)\.(?:%s)\s*\(' % '|'.join(TEXT_METHODS), code))
    text |= set(re.findall(r'\b(\w+)::measureText\b', code))
    for _, i in calls(code, ('ellipsize',)):
        text |= {a for a in split_args(code, i) if re.match(r'\w+\Z', a)}
    text &= known
    aliases = []
    for m in re.finditer(r'\b(\w+)\s*=\s*([^;=][^;]*);', code):
        if m.group(1) in known and not re.match(r'\s*new\b', m.group(2)): aliases.append((m.group(1), set(re.findall(r'\b\w+\b', m.group(2))) & known - {m.group(1)}))
    grew = True
    while grew:
        grew = False
        for name, parts in aliases:
            if name in text and not parts <= text: text |= parts; grew = True
    return text, made


def enclosing_call(code, at):

    depth, j = 0, at
    while j >= 0:
        c = code[j]
        if c in ')]}': depth += 1
        elif c in '([{':
            if depth == 0:
                m = re.search(r'(\w+)\s*$', code[:j]) if c == '(' else None
                return m.group(1) if m else ''
            depth -= 1
        j -= 1
    return ''


def text_use(code, name, until, pos, end):





    again = re.search(r'\b%s\.(?:%s)\s*\(' % (re.escape(name), until), code[pos:end])
    stop = pos + again.start() if again else end
    uses = [pos + m.start() for m in re.finditer(r'[(,]\s*%s\s*[,)]' % re.escape(name), code[pos:stop])]
    if not uses: return True
    for at in uses:
        call = enclosing_call(code, at)
        if not (call.startswith('draw') and call not in TEXT_DRAWS): return True
    return False


def colour_helpers(code):

    out = set()
    for m in re.finditer(r'\bint\s+(\w+)\s*\([^()]*\)\s*\{', code):
        body = code[m.end() - 1:close(code, m.end() - 1)]
        if re.search(r'\breturn\b[^;]*\b0[xX][0-9a-fA-F]{6,8}\b', body): out.add(m.group(1))
    return out


def colour_kind(expr, helpers=()):
    if re.search(r'\b0[xX][0-9a-fA-F]+', expr): return 'text-colour-literal'
    if any(re.search(r'(?<![\w.])(?:this\.)?%s\s*\(' % re.escape(h), expr) for h in helpers): return 'text-colour-literal'
    if re.search(r'\brgba\s*\(|\bColor\s*\.', expr): return 'text-colour-rgba'
    if re.search(r'\b[A-Z][A-Z0-9_]+\b', expr): return 'text-colour-constant'
    return None


def findings(name, text):

    code = strip(text)
    cls = name[:-5] if name.endswith('.java') else name
    paints, made = text_paints(code)
    helpers = colour_helpers(code)
    out = []
    member = blocks(code, cls)

    def block_end(pos):
        for a, b, _ in member:
            if a < pos < b: return b
        return len(code)

    for m in re.finditer(r'\b(\w+)\.(setColor|setARGB)\s*\(', code):
        x = m.group(1)
        if x not in paints: continue
        i = m.end() - 1
        expr = code[i + 1:close(code, i)]
        kind = 'text-colour-literal' if m.group(2) == 'setARGB' else colour_kind(expr, helpers)
        if kind and text_use(code, x, 'setColor|setARGB', close(code, i) + 1, block_end(m.start())): out.append((kind, statement(code, m.start())))
    for m in re.finditer(r'\b(\w+)\.setStyle\s*\(\s*(?:Paint\s*\.\s*)?Style\s*\.\s*(?:STROKE|FILL_AND_STROKE)\b', code):
        if m.group(1) in paints and text_use(code, m.group(1), 'setStyle', m.end(), block_end(m.start())): out.append(('text-stroke', statement(code, m.start())))
    for m in re.finditer(r'\bTypeface\s*\.\s*\w+', code):
        line = code[code.rfind('\n', 0, m.start()) + 1:m.start()]
        if not line.lstrip().startswith('import'): out.append(('typeface', statement(code, m.start())))
    for m in re.finditer(r'\bfont\s*\(\s*"', code): out.append(('font-file', statement(code, m.start())))
    for m in re.finditer(r'\.setShadowLayer\s*\(', code): out.append(('shadow-layer', statement(code, m.start())))
    if name in PANEL_FILES:
        for kind, rule in PANEL_RULES:
            for m in re.finditer(rule, code): out.append((kind, statement(code, m.start())))
    for p in sorted(paints & made):

        q = re.escape(p)
        font = [a for _, i in calls(code, ('%s.setTypeface' % p,)) for a in split_args(code, i)]
        copied = re.search(r'\b%s\.set\s*\(' % q, code) or re.search(r'\b%s\s*=\s*new\s+(?:Text)?Paint\s*\(\s*[a-z]\w*\s*\)' % q, code)
        if not copied and all(a == 'null' for a in font): out.append(('system-font', p))
    return out


def scan(src=SRC):
    found = collections.Counter()
    for root, _, files in os.walk(src):
        for f in sorted(files):
            if not f.endswith('.java') or f in EXEMPT: continue
            with open(os.path.join(root, f), encoding='utf-8') as h:
                for kind, stmt in findings(f, h.read()): found[(f, kind, stmt)] += 1
    return found


def load(path):
    c = collections.Counter()
    if not os.path.isfile(path): return c
    with open(path, encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n')
            if not line or line.startswith('#'): continue
            parts = line.split('\t', 2)
            if len(parts) == 3: c[tuple(parts)] += 1
    return c


def write(path, found):
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(HEADER)
        for key in sorted(found):
            for _ in range(found[key]): f.write('\t'.join(key) + '\n')


def check(src=SRC, baseline=BASELINE, update=False, out=sys.stdout):
    found = scan(src)
    strict = collections.Counter({k: n for k, n in found.items() if k[0] in STRICT})
    if update:
        if strict:
            for k in sorted(strict): print('STRICT %s\t%s\t%s' % k, file=out)
            return 1
        write(baseline, found)
        print('check_style: baseline rewritten, %d entries' % sum(found.values()), file=out)
        return 0
    base = load(baseline)
    new, gone = found - base, base - found
    for k in sorted(strict): print('STRICT %s\t%s\t%s' % k, file=out)
    for k in sorted(new - strict): print('NEW    %s\t%s\t%s' % k, file=out)
    for k in sorted(gone): print('GONE   %s\t%s\t%s   (fixed: delete it from %s)' % (k + (os.path.basename(baseline),)), file=out)
    bad = bool(strict or new or gone)
    files = sorted({k[0] for k in found})
    print('check_style: %d known deviations in %d files%s' % (sum(found.values()), len(files), ', FAILED' if bad else ', ok'), file=out)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(check(update='--update' in sys.argv[1:]))
