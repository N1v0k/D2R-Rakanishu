#!/usr/bin/env python3








import csv, glob, os, re, sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
CJK = re.compile(r'[　-〿一-鿿＀-￯]')


SINK = {'LOG', 'LOGV', 'plog', 'NSLog', 'fprintf', 'printf', 'die', 'HLE_FORCE', 'HLE_WEAK', 'HLE_STUB', 'dprintf', 'dm_prof_mark', 'perror', 'puts',
        'SystemExit', 'print', 'Exception', 'RuntimeError', 'ValueError', 'AssertionError', 'warn', 'log', 'dm_log', 'boot_log', 'SMOKE', '_Static_assert', 'static_assert',
        'KEYS_BAD'}
C_FILES = ['host/*.c', 'host/*.m', 'ios/Sources/*.m', 'ios/Sources/*.c']
PY_FILES = ['ios/tools/touch_layout.py', 'ios/tools/wide_stash.py', 'ios/tools/pouch_layout.py', 'ios/tools/gen_romfs_overlay.py', 'ios/tools/drop_light.py',
            'ios/tools/gen_hud_v4.py', 'ios/tools/gen_map_goal_art.py', 'ios/tools/touch_strings.py', 'ios/tools/gen_close_btn.py', 'ios/tools/gen_vault_cover.py']

def strip_c(s):

    out, lits, i, n = list(s), [], 0, len(s)
    while i < n:
        c = s[i]
        if s.startswith('//', i): j = s.find('\n', i); j = n if j < 0 else j; out[i:j] = ' ' * (j - i); i = j
        elif s.startswith('/*', i): j = s.find('*/', i + 2); j = n if j < 0 else j + 2; out[i:j] = [ch if ch == '\n' else ' ' for ch in s[i:j]]; i = j
        elif c == '"' or (c == '@' and i + 1 < n and s[i + 1] == '"'):
            k = i + (2 if c == '@' else 1); j = k
            while j < n and s[j] != '"':
                j += 2 if s[j] == '\\' else 1
            lits.append((i, s[k:j])); i = j + 1
        elif c == "'":
            j = i + 1
            while j < n and s[j] != "'": j += 2 if s[j] == '\\' else 1
            i = j + 1
        else: i += 1
    return ''.join(out), lits

def strip_py(s):
    import io, tokenize
    lits = []; lines = s.splitlines(True); off = [0]
    for l in lines: off.append(off[-1] + len(l))
    prev = None
    for t in tokenize.generate_tokens(io.StringIO(s).readline):
        if t.type == tokenize.STRING:
            doc = prev in (None, tokenize.NEWLINE, tokenize.INDENT, tokenize.DEDENT, tokenize.NL) and t.string.lstrip('rbuRBUfF')[:3] in ('"""', "'''")
            if not doc: lits.append((off[t.start[0] - 1] + t.start[1], t.string))
        if t.type not in (tokenize.COMMENT, tokenize.NL): prev = t.type
    return s, lits

def caller(text, pos):

    depth = 0
    for i in range(pos - 1, max(-1, pos - 4000), -1):
        c = text[i]
        if c in ')]}': depth += 1
        elif c in '([{':
            if depth: depth -= 1; continue
            if c != '(': return ''
            m = re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', text[max(0, i - 80):i]); return m.group(1) if m else ''
        elif c == ';' and not depth: return ''
    return ''

def hits():
    out = []
    for pat, kind in [(p, 'c') for p in C_FILES] + [(p, 'py') for p in PY_FILES]:
        for f in sorted(glob.glob(os.path.join(ROOT, pat))):
            s = open(f, encoding='utf-8', errors='replace').read(); text, lits = (strip_c if kind == 'c' else strip_py)(s)
            for pos, lit in lits:
                if not CJK.search(lit) or caller(text, pos) in SINK: continue
                out.append((os.path.relpath(f, ROOT).replace(os.sep, "/"), s.count('\n', 0, pos) + 1, lit, caller(text, pos)))
    return out

def main():
    wl = list(csv.reader(open(os.path.join(ROOT, 'i18n', 'literal_whitelist.tsv'), encoding='utf-8'), delimiter='\t'))[1:]
    used, bad = [0] * len(wl), []
    for f, ln, lit, fn in hits():
        ok = [i for i, r in enumerate(wl) if r[0] == f and (r[1] == '*' or (r[1][:1] == '@' and r[1][1:] == fn) or (r[1][:1] == '=' and r[1][1:] == lit) or (r[1][:1] not in '@*=' and r[1] in lit))]
        if ok: used[ok[0]] += 1
        else: bad.append('%s:%d  %s' % (f, ln, lit[:90]))
    stale = ['%s  「%s」' % (r[0], r[1]) for r, u in zip(wl, used) if not u]
    if bad or stale:
        if bad: print('I18N 红：%d 条上屏中文没走中英表、也不在白名单（dm/i18n/literal_whitelist.tsv）：' % len(bad)); [print('  ' + b) for b in bad]
        if stale: print('I18N 红：白名单里 %d 行已经匹配不到（删掉）：' % len(stale)); [print('  ' + b) for b in stale]
        sys.exit(1)
    print('I18N OK：中文字面量 %d 条全在白名单（%d 行），其余走中英表' % (sum(used), len(wl)))

if __name__ == '__main__':
    if sys.argv[1:] == ['--list']:
        for f, ln, lit, fn in hits(): print('%s:%d\t%s\t%s' % (f, ln, fn, lit[:120]))
    else: main()
