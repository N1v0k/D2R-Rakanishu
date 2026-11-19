#!/usr/bin/env python3
























import os, runpy, shutil, subprocess, sys

_real = subprocess.run


def _done(args, out=''):
    return subprocess.CompletedProcess(args, 0, stdout=out, stderr='')


def run(args, *a, **k):
    text = k.get('text') or k.get('universal_newlines')
    enc = (lambda s: s) if text else (lambda s: s.encode())
    if isinstance(args, (list, tuple)) and args:
        c = [str(x) for x in args]
        if c[0] == 'xcrun' and 'metal' in c and '--version' in c: return _done(args, enc('none\n'))
        if c[0] == 'xcrun' and 'metal' in c and '-c' in c and '-o' in c:
            o = c.index('-o'); src = [x for i, x in enumerate(c[c.index('metal') + 1:], c.index('metal') + 1) if not x.startswith('-') and i != o + 1]
            if len(src) != 1: raise RuntimeError('nometal_run：认不出源码参数 %r' % c)
            with open(src[0], 'rb') as f: b = f.read()
            with open(c[o + 1], 'wb') as f: f.write(b.replace(b'\r\n', b'\n'))
            return _done(args, enc(''))
        if c[0] == 'xcrun' and 'metallib' in c and '-o' in c: shutil.copyfile(c[c.index('metallib') + 1], c[c.index('-o') + 1]); return _done(args, enc(''))
        if c[0] == 'xcrun': raise RuntimeError('nometal_run：没认出的 xcrun 命令 %r' % c)
        if c[0] == 'which' and len(c) == 2: return _done(args, enc((shutil.which(c[1]) or '') + '\n'))
        if c[0] == 'cp' and len(c) == 4 and c[1] == '-c': shutil.copyfile(c[2], c[3]); return _done(args, enc(''))
    return _real(args, *a, **k)


if __name__ == '__main__':
    if len(sys.argv) < 2: sys.exit(__doc__ or '用法：nometal_run.py <脚本.py> [参数…]')
    subprocess.run = run
    script = os.path.abspath(sys.argv[1]); sys.argv = sys.argv[1:]; sys.path.insert(0, os.path.dirname(script))
    runpy.run_path(script, run_name='__main__')
