#!/usr/bin/env python3














import os, re, sys
T = sys.argv[1]
RX = re.compile(rb'\b(pgrep|pkill)\b[^|;&\n"\']*?\s(-[A-Za-z0-9]*f[A-Za-z0-9]*|--full)(?=\s|$)')
OK = '只列不判'.encode()
def is_script(p):
    if p.endswith(('.sh', '.py', '.zsh')): return True
    if '.' in os.path.basename(p): return False
    try:
        with open(p, 'rb') as f: h = f.readline()
        return h.startswith(b'#!') and (b'sh' in h or b'python' in h)
    except OSError: return False
def code_part(l):

    q = None; i = 0
    while i < len(l):
        c = l[i:i+1]
        if q:
            if c == b'\\' and q == b'"': i += 2; continue
            if c == q: q = None
        elif c in (b'"', b"'"): q = c
        elif c == b'\\': i += 2; continue
        elif c == b'#' and (i == 0 or l[i-1:i] in (b' ', b'\t', b';')): return l[:i]
        i += 1
    return l
n = 0
for d, ds, fs in os.walk(T):
    ds[:] = [x for x in ds if x not in ('.git', 'third_party', 'Gen', 'build') and not x.startswith('.iosdd')]
    for f in sorted(fs):
        p = os.path.join(d, f)
        if os.path.islink(p) or not is_script(p) or os.path.abspath(p) == os.path.abspath(__file__): continue
        with open(p, 'rb') as fh: L = fh.read().split(b'\n')
        for i, l in enumerate(L):
            if OK in l: continue
            c = code_part(l)
            for m in RX.finditer(c):
                print('%s:%d:%s' % (os.path.relpath(p, T), i + 1, c[max(0, m.start() - 20):m.end() + 40].decode('utf-8', 'replace').strip())); n += 1
print('NAMEMATCH %s：按命令行文本 pgrep -f / pkill -f %d 处（改用 pid + 启动时刻，见 dm/verify/merge_running.sh、dm/ios/tools/devlock.sh）' % ('OK' if n == 0 else 'RED', n))
sys.exit(1 if n else 0)
