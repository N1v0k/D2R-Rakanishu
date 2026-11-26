#!/usr/bin/env python3




















import os, re, sys

FIND = r'\b(?:ui_find|vis_find|find_w|ui_panel|ui_child|belt_find|belt_now|vu_find|kb_find|rw_find|find_widget|ui_top_panel)\s*\('
TREE = r'(?:\bch\s*\+\s*8\s*\*|\+\s*0x58\b|\bGA_UI_ROOT\b|\+\s*0x190\b|\+\s*0x178\b)'
PARAM = r'^(?:w|bw|pw|sp|tw|cw|vt|panel|\w+_w)$'
SRC = re.compile(FIND + '|' + TREE)

def strip(text):
    text = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group(0)), text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', text)

def decl_names(decl):
    decl = re.sub(r'\{[^{}]*\}', '', decl); decl = re.sub(r'\[[^\]]*\]', '', decl)
    while re.search(r'\([^()]*\)', decl): decl = re.sub(r'\([^()]*\)', '', decl)
    out = []
    for part in decl.split(','):
        n = re.match(r'\s*\**\s*(\w+)', part)
        if n: out.append(n.group(1))
    return out

def struct_types(code):
    return {m.group(2) for m in re.finditer(r'\btypedef\s+struct\s*\w*\s*\{([^{}]*)\}\s*(\w+)\s*;', code) if re.search(r'\buint64_t\b', m.group(1))}

def decls(code, types, static_only):
    pre = r'\bstatic\s+' if static_only else r'(?<![\w.>(,])'
    names, structs = set(), set()
    for m in re.finditer(pre + r'(?:const\s+)?(?:_Atomic\s+)?(?:volatile\s+)?uint64_t\s+([^;\n]*);', code): names.update(decl_names(m.group(1)))
    for m in re.finditer(pre + r'struct\s*\w*\s*\{([^{}]*)\}\s*([^;]*);', code):
        if re.search(r'\buint64_t\b', m.group(1)): structs.update(decl_names(m.group(2)))
    for t in types:
        for m in re.finditer(pre + re.escape(t) + r'\s+([^;(]*);', code): structs.update(decl_names(m.group(1)))
    return names, structs

def split_funcs(code):
    out, depth, start, isfn = [], 0, 0, False
    for i, ch in enumerate(code):
        if ch == '{':
            if depth == 0: isfn = code[:i].rstrip().endswith(')')
            if depth == 0 and isfn: out.append((start, code[start:i], False)); start = i
            depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0 and isfn: out.append((start, code[start:i + 1], True)); start = i + 1
    out.append((start, code[start:], False)); return out

ASSIGN = re.compile(r'(?<![=!<>+\-*/%&|^\w.\]])\s*(\w+)(\s*\[[^\]=]*\])?(\s*(?:\.|->)\s*\w+(?:\s*\[[^\]=]*\])?)?\s*=(?!=)\s*([^;]+)')

EXEMPT = {('dm_cursor.c', 'ui_gen_scan'): '界面代号本身：记上一帧的顶层面板，只比身份、不解引用'}

def func_name(head):
    m = re.findall(r'(\w+)\s*\([^()]*(?:\([^()]*\)[^()]*)*\)\s*$', head)
    return m[-1] if m else ''

def group_ok(code, var):
    decl = re.search(r'\bstatic\s+struct\s*\w*\s*\{([^{}]*)\}\s*[^;]*\b' + re.escape(var) + r'\b[^;]*;', code)
    return bool(decl and re.search(r'\buint32_t\s+(?:\w+\s*,\s*)*ui_gen\b', decl.group(1)) and re.search(re.escape(var) + r'\s*\.\s*ui_gen\s*[!=]=\s*dm_ui_gen\s*\(\s*\)', code))

def scan(text, where):
    code = strip(text); types = struct_types(code); bad = []
    top = ''.join(t for _, t, body in split_funcs(code) if not body)
    top = re.sub(r'\b_Atomic\s*\(\s*([^()]*?)\s*\)', r'_Atomic \1', top)
    while re.search(r'\([^()]*\)', top): top = re.sub(r'\([^()]*\)', ' ', top)
    _, fs = decls(top, types, False)
    while re.search(r'\{[^{}]*\}', top): top = re.sub(r'\{[^{}]*\}', ' ', top)
    fn, _ = decls(top, types, False)
    parts = split_funcs(code)
    for k, (off, chunk, body) in enumerate(parts):
        if not body: continue
        if (where, func_name(parts[k - 1][1] if k else '')) in EXEMPT: continue
        sn, ss = decls(chunk, types, True); nost = re.sub(r'\bstatic\s+[^;]*;', ';', chunk); ln, ls = decls(nost, types, False)
        ln |= set(re.findall(r'\b(?:const\s+)?(?:unsigned\s+|signed\s+)?(?:int|long|short|char|float|double|bool|BOOL|size_t|\w+_t)\b[\s*]+(\w+)\s*(?=[=;,\[)])', nost))
        names = (fn - ln) | sn; structs = (fs - ls) | ss
        for m in ASSIGN.finditer(chunk):
            var, idx, fld, rhs = m.group(1), m.group(2), m.group(3), re.split(r'(?<![=!<>])=(?!=)', m.group(4))[-1].strip()
            if not ((var in names and not fld) or (var in structs and (fld or re.match(r'^\(\s*\w+\s*\)\s*\{', rhs)))): continue
            if var in structs and group_ok(code, var): continue
            if re.match(r'(?:const\s+)?(?:_Atomic\s+)?uint64_t\b', chunk[max(0, m.start(1) - 40):m.start(1)].split(';')[-1].split('{')[-1].strip() + ' '): continue
            lhs = re.findall(r'\w+', fld)[-1] if fld else var
            byname = re.match(r'^(?:w|panel|\w+_w)$', lhs) and re.match(r'^[A-Za-z_]\w*\s*(?:\(.*\))?\s*\)?$', rhs) and rhs.strip(') ') not in ('NULL',)
            if byname or SRC.search(rhs) or re.match(PARAM, re.sub(r'[\s;)]+$', '', rhs)) or re.match(r'^\(\s*\w+\s*\)\s*\{\s*(?:w|bw|pw|sp|tw|cw|panel)\b', rhs):
                no = code.count('\n', 0, off + m.start(1)) + 1
                bad.append('%s:%d 静态变量 %s%s 存了控件指针，没带界面代号（改用 dm_wref）：%s' % (where, no, var, re.sub(r'\s', '', fld) if fld else '', text.split('\n')[no - 1].strip()[:140]))
        for m in re.finditer(r'atomic_(?:store|exchange)\s*\(\s*&\s*(\w+)\s*,\s*([^;]+)\)', chunk):
            var, rhs = m.group(1), m.group(2).strip()
            if var in names and (SRC.search(rhs) or re.match(PARAM, rhs)):
                no = code.count('\n', 0, off + m.start(1)) + 1
                bad.append('%s:%d 静态变量 %s 存了控件指针，没带界面代号（改用 dm_wref）：%s' % (where, no, var, text.split('\n')[no - 1].strip()[:140]))
    return bad

SELF_BAD = [
    'static uint64_t g_x_w;\nvoid f(uint64_t w) { g_x_w = w; }\n',
    'void f(uint64_t root) { static uint64_t pw; pw = ui_find(root, "X", 0); }\n',
    'static struct { uint64_t panel, row[4]; int n; } W;\nvoid f(uint64_t p) { W.panel = vis_find(p, "A", 0); }\n',
    'static struct { uint64_t panel; uint32_t ui_gen; } W;\nvoid f(uint64_t p) { W.ui_gen = dm_ui_gen(); W.panel = vis_find(p, "A", 0); }\n',
    'static _Atomic uint64_t g_t;\nvoid f(uint64_t tw) { atomic_store(&g_t, tw); }\n',
    'void f(uint64_t root) { static uint64_t c[4]; uint64_t ch = 0; c[0] = *(uint64_t *)(ch + 8 * 1); }\n',
    'static struct { _Atomic(uint64_t) panel; } W;\nvoid f(uint64_t w) { W.panel = w; }\n',
    'static uint64_t g_d_w, g_s_w;\nvoid f(int r) { if (r == 1) g_d_w = g_s_w; }\n',
    'typedef struct { uint64_t w; int f; } ent;\nstatic ent g_e[8];\nvoid f(uint64_t w) { g_e[0] = (ent){ w, 1 }; g_e[1].w = w; }\n',
]
SELF_OK = [
    'static dm_wref g_x;\nvoid f(uint64_t w) { dm_wref_set(&g_x, w); }\n',
    'static uint64_t g_n;\nvoid f(uint64_t w) { g_n = g_n + 1; uint64_t pw = ui_find(w, "a", 0); (void)pw; }\n',
    'static uint64_t next;\nvoid f(uint64_t fr) { next = fr + 6; }\n',
    'void f(uint64_t w) { uint64_t x = w; static int k; k = 1; (void)x; }\n',
    'static uint64_t g_xy; // g_xy = w 只是注释\nvoid f(void) { g_xy = 3; }\n',
    'static uint64_t pw;\nvoid g(void) { pw = 1; }\nvoid f(uint64_t root) { uint64_t pw = ui_find(root, "a", 0); pw = ui_find(root, "b", 0); (void)pw; }\n',
    'typedef struct { uint64_t it; int n; } row;\nstatic int g(int max, row *out);\nstatic int g(int max, row *out) { uint64_t w = 1; if (max) out[0].it = w; return 1; }\n',
    'static struct { uint64_t panel, bg; uint32_t ui_gen; } W;\nvoid f(uint64_t p) { W.ui_gen = dm_ui_gen(); W.panel = p; W.bg = find_w(p, "B", 0); }\nint ok(void) { return W.panel && W.ui_gen == dm_ui_gen(); }\n',
]

def selftest():
    ok = True
    for i, t in enumerate(SELF_BAD):
        if not scan(t, 'bad%d' % i): print('【红】自检：违规样例 %d 没报出来' % i); ok = False
    for i, t in enumerate(SELF_OK):
        r = scan(t, 'ok%d' % i)
        if r: print('【红】自检：合规样例 %d 误报：%s' % (i, r)); ok = False
    return ok

if __name__ == '__main__':
    args = [a for a in sys.argv[1:] if a != '--selftest']
    if '--selftest' in sys.argv and not selftest(): sys.exit(1)
    d = args[0] if args else os.path.join(os.path.dirname(__file__), '..')
    bad, n = [], 0
    for f in sorted(os.listdir(d)):
        if f.endswith('.c') or f.endswith('.m'):
            n += 1; bad += scan(open(os.path.join(d, f), errors='replace').read(), f)
    for b in bad: print('【红】' + b)
    print('控件指针缓存扫描：%d 个文件，%d 处违规' % (n, len(bad)))
    sys.exit(1 if bad else 0)
