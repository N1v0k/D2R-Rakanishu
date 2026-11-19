













import hashlib
import io
import os
import re
import subprocess
import sys
import zipfile

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
PATHS = [re.compile(rb'/Users/[A-Za-z0-9_.-]+/'), re.compile(rb'/home/[A-Za-z0-9_.-]+/'), re.compile(rb'[A-Za-z]:[\\/]+Users[\\/]+', re.I)]
TABLES = ('dm_vault_tab.h', 'dm_hud_exp.h', 'dm_touch_native_art.c')


def entries(zf, prefix=''):

    for name in zf.namelist():
        data = zf.read(name)
        if name.lower().endswith('.hap'):
            with zipfile.ZipFile(io.BytesIO(data)) as inner:
                yield from entries(inner, prefix + name + '!')
        else:
            yield prefix + name, data


def runtime_pack(name):
    low = name.lower().split('!')[-1]
    return (low.startswith('resources/resfile/dm/') or low.endswith('.xgs') or low.endswith('.patched.bin')
            or '/overlay/' in '/' + low or low.endswith('derived.tsv'))


def table_strings(gen):

    found = set()
    for t in TABLES:
        p = os.path.join(gen, t)
        if os.path.isfile(p):
            found |= {m.encode() for m in re.findall(r'"((?:[^"\\\n]|\\.){6,})"', open(p, encoding='utf-8', errors='replace').read()) if '\\' not in m}


    tracked = subprocess.check_output(['git', 'ls-files', '-z', '--', 'dm'], cwd=REPO).split(b'\0')
    source = []
    for name in filter(None, tracked):
        path = os.path.join(REPO, os.fsdecode(name)); f = os.path.basename(path)
        if f.endswith(('.c', '.h', '.inc', '.m', '.cpp', '.py', '.ets', '.ts', '.json', '.tsv', '.md')) and f not in TABLES:
            source.append(open(path, 'rb').read())


    for name in ('dm/ios/Sources/TouchArt/native_art_manifest.json', 'dm/ios/docs/native_art_v1_geometry.json',
                 'dm/ios/docs/touch_layout_spec_native_binding_v5.json'):
        source.append(hashlib.sha256(open(os.path.join(REPO, name), 'rb').read()).hexdigest().encode())
    blob = b'\0'.join(source)
    return sorted(s for s in found if s not in blob)


def main():
    args = sys.argv[1:]
    gen = None
    if '--gen' in args:
        i = args.index('--gen'); gen = args[i + 1]; del args[i:i + 2]
    with zipfile.ZipFile(args[0]) as z:
        items = list(entries(z))
    fail = 0
    bad = [n for n, _ in items if runtime_pack(n)]
    if bad:
        print('NOT CLEAN (1): %d runtime pack files, e.g. %s' % (len(bad), bad[0])); fail = 1
    else:
        print('ok (1): no runtime pack')
    hits = [(n, m.group().decode('latin-1')) for n, d in items for rx in PATHS for m in [rx.search(d)] if m]
    if hits:
        print('NOT CLEAN (2): machine path in %d files, e.g. %s: %s' % (len(hits), hits[0][0], hits[0][1])); fail = 1
    else:
        print('ok (2): no machine paths')
    if gen:
        keys = table_strings(gen)
        if len(keys) < 100:
            print('NOT CLEAN (3): only %d table-only strings found in %s (need >= 100 to judge)' % (len(keys), gen)); fail = 1
        else:
            leaks = [(n, k) for n, d in items if n.endswith('.so') for k in keys if k in d]
            if leaks:
                print('NOT CLEAN (3): %d real-table strings in %s, e.g. %r' % (len(leaks), leaks[0][0], leaks[0][1])); fail = 1
            else:
                print('ok (3): none of %d real-table strings in any .so' % len(keys))
    else:
        print('skipped (3): pass --gen <generated dir> to check the real tables')
    sys.exit(fail)


main()
