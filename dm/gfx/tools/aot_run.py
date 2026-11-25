#!/usr/bin/env python3










import builtins, os, runpy, sys

DLLS = ('dmsh_xlate.dll', 'Ryujinx.Graphics.Shader.dll', 'Ryujinx.Common.dll')
_open = builtins.open


def _aot_open(file, *a, **k):
    if isinstance(file, str) and os.path.basename(file) in DLLS and not os.path.exists(file):
        d = os.path.dirname(file)
        exe = next((os.path.join(d, e) for e in ('dmsh_xlate.exe', 'dmsh_xlate') if os.path.isfile(os.path.join(d, e))), None)
        if exe: return _open(exe, *a, **k)
    return _open(file, *a, **k)


if __name__ == '__main__':
    if len(sys.argv) < 2: sys.exit('用法：aot_run.py <脚本.py> [参数…] | usage: aot_run.py <script.py> [args...]')
    builtins.open = _aot_open
    script = os.path.abspath(sys.argv[1]); sys.argv = sys.argv[1:]; sys.path.insert(0, os.path.dirname(script))
    runpy.run_path(script, run_name='__main__')
