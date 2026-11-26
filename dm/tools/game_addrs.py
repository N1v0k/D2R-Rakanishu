#!/usr/bin/env python3

















import fnmatch
import os
import re
import sys
import types

HERE = os.path.dirname(os.path.abspath(__file__))
DEF = os.path.normpath(os.path.join(HERE, '..', 'host', 'game_addrs.def'))
ALLOW = os.path.join(HERE, 'game_addrs_allow.tsv')
KINDS = ('func', 'site', 'vtable', 'data', 'bss', 'rodata')
MODULES = ('main', 'sdk')
_NAME = r'[A-Z][A-Z0-9_]*'
_LINE = {
    'GA': re.compile(r'GA\((\w+),\s*(' + _NAME + r'),\s*(0x[0-9a-f]+),\s*(\w+),\s*"([^"\\\t]*)"\)$'),
    'GL': re.compile(r'GL\((\w+),\s*(' + _NAME + r'),\s*(0x[0-9a-f]+)\)$'),
    'GB': re.compile(r'GB\((\w+),\s*"([^"\\]+)",\s*"([0-9a-f]{32})"\)$'),
    'GF': re.compile(r'GF\((\w+),\s*(' + _NAME + r'),\s*(0x[0-9a-f]+),\s*(0x[0-9a-f]+),\s*"([^"\\\t]*)"\)$'),
}


def insn_imm(word):


    if word & 0x3b000000 == 0x39000000:
        size = word >> 30
        if word >> 26 & 1 and word >> 23 & 1:
            size = 4
        return (word >> 10 & 0xfff) << size
    if word & 0x3a000000 == 0x28000000 and word >> 23 & 3:
        opc, simd = word >> 30, word >> 26 & 1
        scale = 2 + opc if simd else (3 if opc == 2 else 2)
        imm = word >> 15 & 0x7f
        return (imm - 0x80 if imm & 0x40 else imm) << scale
    if word & 0x3b200c00 == 0x38000000:
        imm = word >> 12 & 0x1ff
        return imm - 0x200 if imm & 0x100 else imm
    if word & 0x1f800000 == 0x11000000:
        return (word >> 10 & 0xfff) << (12 if word >> 22 & 1 else 0)
    if word & 0x7f800000 == 0x52800000:
        return (word >> 5 & 0xffff) << (16 * (word >> 21 & 3))
    return None


def insn_shape(word):


    for mask, value, imm in ((0x3b000000, 0x39000000, 0x003ffc00), (0x3a000000, 0x28000000, 0x003f8000),
                             (0x3b200c00, 0x38000000, 0x001ff000), (0x1f800000, 0x11000000, 0x007ffc00),
                             (0x7f800000, 0x52800000, 0x007fffe0)):
        if word & mask == value and (value != 0x28000000 or word >> 23 & 3):
            return word & ~imm & 0xffffffff
    return None


class TableError(ValueError):
    pass


class Table:


    def __init__(self, path=DEF):
        self.path = path
        self.entries, self.layout, self.build, self.fields = [], {}, {}, []
        names, addrs = set(), {}
        with open(path, encoding='utf-8') as f:
            for n, raw in enumerate(f, 1):
                line = raw.strip()
                if not line or line.startswith('//'):
                    continue
                m = _LINE.get(line[:2]) and _LINE[line[:2]].match(line)
                if not m:
                    raise TableError(f'{path}:{n}: not a table line: {line[:80]}')
                if m.group(1) not in MODULES:
                    raise TableError(f'{path}:{n}: unknown module {m.group(1)}')
                if line[:2] == 'GB':
                    self.build[m.group(1)] = (m.group(2), m.group(3))
                    continue
                name, value = m.group(2), int(m.group(3), 16)
                if name in names:
                    raise TableError(f'{path}:{n}: name {name} twice')
                names.add(name)
                if line[:2] == 'GL':
                    self.layout[name] = value
                    continue
                if line[:2] == 'GF':
                    self.fields.append((m.group(1), name, value, int(m.group(4), 16), m.group(5)))
                    continue
                if m.group(4) not in KINDS:
                    raise TableError(f'{path}:{n}: unknown kind {m.group(4)}')
                if value >= 1 << 31:
                    raise TableError(f'{path}:{n}: {name} does not fit a C int')
                if (m.group(1), value) in addrs:
                    raise TableError(f'{path}:{n}: address {value:#x} is already {addrs[(m.group(1), value)]}')
                addrs[(m.group(1), value)] = name
                self.entries.append((m.group(1), name, value, m.group(4), m.group(5)))
        if 'main' not in self.build:
            raise TableError(f'{path}: no GB(main, ...) line')
        self.ga = types.SimpleNamespace(**{e[1]: e[2] for e in self.entries})
        self.gl = types.SimpleNamespace(**self.layout)
        self.gf = types.SimpleNamespace(**{f[1]: f[2] for f in self.fields})
        self.game = types.SimpleNamespace(version=self.build['main'][0], build_id=self.build['main'][1])
        self.by_addr = {}
        for e in self.entries:
            self.by_addr.setdefault(e[2], e[1])


TABLE = Table()
GA, GF, GL, GAME = TABLE.ga, TABLE.gf, TABLE.gl, TABLE.game



def _c_ctx(t):
    out = bytearray(b'k' * len(t))
    i, n = 0, len(t)
    while i < n:
        ch = t[i]
        if t.startswith('//', i):
            j = t.find('\n', i)
            j = n if j < 0 else j
            out[i:j] = b'c' * (j - i)
            i = j
        elif t.startswith('/*', i):
            j = t.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out[i:j] = b'c' * (j - i)
            i = j
        elif ch == '"':
            j = i + 1
            while j < n and t[j] != '"' and t[j] != '\n':
                j += 2 if t[j] == '\\' else 1
            j = min(j + 1, n)
            out[i:j] = b's' * (j - i)
            i = j
        elif ch == "'":
            j = i + 1
            while j < n and t[j] != "'" and t[j] != '\n' and j - i < 6:
                j += 2 if t[j] == '\\' else 1
            j = min(j + 1, n)
            out[i:j] = b's' * (j - i)
            i = j
        else:
            i += 1
    return out


def _py_ctx(t):
    import io
    import tokenize
    out = bytearray(b'k' * len(t))
    offs = [0]
    for line in t.split('\n'):
        offs.append(offs[-1] + len(line) + 1)
    for tok in tokenize.generate_tokens(io.StringIO(t).readline):
        if tok.type in (tokenize.COMMENT, tokenize.STRING) or tok.type == getattr(tokenize, 'FSTRING_MIDDLE', -1):
            a = offs[tok.start[0] - 1] + tok.start[1]
            b = offs[tok.end[0] - 1] + tok.end[1]
            out[a:b] = (b'c' if tok.type == tokenize.COMMENT else b's') * (b - a)
    return out


def _sh_ctx(t):
    out = bytearray(b'k' * len(t))
    pos = 0
    for line in t.split('\n'):
        j = line.find('#')
        if j >= 0 and (j == 0 or line[j - 1] in ' \t'):
            out[pos + j:pos + len(line)] = b'c' * (len(line) - j)
        pos += len(line) + 1
    return out


C_EXT = {'.c', '.h', '.inc', '.m', '.mm', '.cpp', '.hpp', '.S', '.java', '.kt', '.glsl', '.metal', '.def'}
SH_EXT = {'.sh', '.mk', '.cmake'}
DOC_EXT = {'.md', '.txt'}


def lang(path):

    ext = os.path.splitext(path)[1]
    base = os.path.basename(path)
    if ext in C_EXT:
        return 'c'
    if ext == '.py':
        return 'py'
    if ext in SH_EXT or base in ('Makefile', 'CMakeLists.txt'):
        return 'sh'
    if ext in DOC_EXT:
        return 'doc'
    return 'other'


def context(path, text):
    k = lang(path)
    if k == 'c':
        return _c_ctx(text)
    if k == 'py':
        return _py_ctx(text)
    if k == 'sh':
        return _sh_ctx(text)
    return bytearray((b'd' if k == 'doc' else b'o') * len(text))


HEX = re.compile(r'(?<![0-9A-Za-z_.])0[xX]([0-9a-fA-F]+)([uU]?[lL]{0,2})(?![0-9A-Za-z_])')


def hex_tokens(path, text):

    ctx = context(path, text)
    for m in HEX.finditer(text):
        if len(m.group(1)) >= 5:
            yield m.start(), m.end(), int(m.group(1), 16), m.group(2), chr(ctx[m.start()])


def load_allow(path=ALLOW):

    rows = []
    if not os.path.exists(path):
        return rows
    with open(path, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            if not line.strip() or line.startswith('#'):
                continue
            cols = line.rstrip('\n').split('\t')
            if len(cols) != 3 or not cols[2].strip():
                raise TableError(f'{path}:{n}: want value<TAB>path pattern<TAB>reason')
            rows.append((None if cols[0] == '*' else int(cols[0], 16), cols[1], cols[2]))
    return rows


def allowed(allow, value, path):
    return any((v is None or v == value) and fnmatch.fnmatchcase(path, p) for v, p, _ in allow)


def spell(name, path):
    return f'GA.{name}' if lang(path) == 'py' else f'GA_{name}'


_INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+[<"]', re.M)


def include_line(path, table_path=DEF):

    d = os.path.dirname(os.path.abspath(path))
    rel = os.path.relpath(os.path.join(os.path.dirname(os.path.abspath(table_path)), 'game_addrs.h'), d)
    return f'#include "{rel}"\n'


def add_include(path, text, table_path=DEF):
    if re.search(r'#\s*include\s+"[^"]*game_addrs\.h"', text):
        return text
    line = include_line(path, table_path)
    m = _INCLUDE.search(text)
    if m:
        j = text.index('\n', m.start()) + 1
        return text[:j] + line + text[j:]
    m = re.search(r'^#define \w+_H\b.*\n', text, re.M)
    j = m.end() if m else 0
    return text[:j] + line + text[j:]


def rewrite(path, text, table, allow, comments, rel=None):

    rel = rel or path
    k = lang(path)
    edits = []
    for a, b, v, suf, cx in hex_tokens(path, text):
        name = table.by_addr.get(v)
        if not name or allowed(allow, v, rel):
            continue
        if cx == 's' or (cx in 'cdo' and not comments):
            continue
        if cx in 'ko' and k in ('sh', 'other'):
            continue
        rep = spell(name, path)
        if suf:
            if cx != 'k' or k != 'c' or suf.lower() != 'u':
                raise TableError(f'{rel}: {text[a:b]}: suffix {suf!r} not handled')
            rep = f'(uint32_t){rep}'
        edits.append((a, b, rep))
    for a, b, rep in reversed(edits):
        text = text[:a] + rep + text[b:]
    if edits and k == 'c' and any(rep.find('GA_') >= 0 for _, _, rep in edits) and path.endswith(('.c', '.m', '.mm', '.cpp', '.h')):
        text = add_include(path, text, table.path)
    return text, len(edits)


def codemod(paths, table, comments, root):
    allow = load_allow()
    total = 0
    for p in paths:
        rel = os.path.relpath(os.path.abspath(p), root)
        try:
            text = open(p, encoding='utf-8').read()
        except (UnicodeDecodeError, IsADirectoryError):
            continue
        new, n = rewrite(p, text, table, allow, comments, rel)
        if new != text:
            with open(p, 'w', encoding='utf-8') as f:
                f.write(new)
            total += n
            note = ''
            if lang(p) == 'py' and not re.search(r'^\s*from game_addrs import|^\s*import game_addrs', new, re.M) and 'GA.' in new:
                note = '  (add: from game_addrs import GA)'
            print(f'{rel}: {n}{note}')
    print(f'{total} replaced')


def main(argv):
    if not argv or argv[0] in ('-h', '--help'):
        print(__doc__)
        return 0
    cmd, rest = argv[0], argv[1:]
    table = TABLE
    if '--table' in rest:
        i = rest.index('--table')
        table = Table(rest[i + 1])
        del rest[i:i + 2]
    if cmd == 'codemod':
        comments = '--comments' in rest
        rest = [r for r in rest if r != '--comments']
        root = os.path.normpath(os.path.join(HERE, '..', '..'))
        codemod(rest, table, comments, root)
        return 0
    if cmd == 'value' and len(rest) == 1:
        name = rest[0].removeprefix('GA_').removeprefix('GL_')
        value = getattr(table.ga, name, None)
        value = table.layout.get(name) if value is None else value
        if value is None:
            print(f'game_addrs: no entry {rest[0]}', file=sys.stderr)
            return 1
        print(f'{value:#x}')
        return 0
    if cmd == 'show':
        for q in rest:
            v = int(q, 16) if q.lower().startswith('0x') else None
            for e in table.entries:
                if e[1] == q.upper().removeprefix('GA_').removeprefix('GA.') or e[2] == v:
                    print(f'{e[0]}\t{e[1]}\t{e[2]:#x}\t{e[3]}\t{e[4]}')
        return 0
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
