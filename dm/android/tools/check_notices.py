#!/usr/bin/env python3






import argparse
import io
import os
import re
import sys
import zipfile

UPSTREAM = 'd2r-mobile (upstream project)'
DEKO = 'deko3d (block-linear layout algorithm)'
WEBP = 'libwebp'
NOT_INSIDE = '## Components not inside the app'


LIBS = {
    'libdm.so': [UPSTREAM, 'SPIRV-Tools', 'SPIR-V Headers', 'libdeflate', 'bcdec', 'nvn.h', DEKO, 'LLVM libc++'],
    'libdm_driverload.so': ['libadrenotools', 'linkernsbypass', 'LLVM libc++'],
    'libhook_impl.so': ['libadrenotools', 'LLVM libc++'],
    'libmain_hook.so': ['libadrenotools'],
    'libpackbuild.so': [UPSTREAM],
    'libdeswizzle.so': [DEKO],
    'libmonpic.so': [WEBP, 'libdeflate', DEKO, 'BitKnit 2 format description (libbg3)', 'LLVM libc++'],
    'libdmsh_shaders.so': ['Shader translator of the pack build', 'SPIRV-Tools', 'SPIR-V Headers', 'LLVM libc++'],
    'libdmsh_xlate.so': ['Shader translator of the pack build', 'Ryujinx shader translator', '.NET runtime (NativeAOT)'],
    'libpylauncher.so': ['Python runtime of the pack build', 'CPython'],
    'libfreetype.so': ['FreeType', 'zlib', 'libpng', 'Brotli'],
    'libharfbuzz.so': ['HarfBuzz'],
    'libfribidi.so': ['FriBiDi'],
    'libwebp.so': [WEBP], 'libwebpmux.so': [WEBP], 'libwebpdemux.so': [WEBP], 'libsharpyuv.so': [WEBP],
}
LIB_PATTERNS = [(re.compile(r'libpython3\.\d+\.so'), ['Python runtime of the pack build', 'CPython'])]


RUNTIME = [
    (re.compile(r'lib/python3\.\d+/lib-dynload/zlib\.'), ['CPython', 'zlib']),
    (re.compile(r'lib/python3\.\d+/'), ['Python runtime of the pack build', 'CPython']),
    (re.compile(r'site-packages/numpy\.libs/'), ['LLVM libc++ (shared copy)']),
    (re.compile(r'site-packages/numpy/'), ['NumPy']),
    (re.compile(r'site-packages/PIL/'), ['Pillow', 'FreeType', 'HarfBuzz', 'FriBiDi', 'libpng', 'zlib', 'Brotli', 'libjpeg-turbo', WEBP]),
    (re.compile(r'site-packages/lz4/'), ['lz4']),
    (re.compile(r'site-packages/capstone/'), ['Capstone']),
]


TOOLS = {
    'dm/tools/patch_nso.py': [UPSTREAM, 'Offline patches of unknown origin'],
    'dm/gfx/nvn/nvn_tex.c': [DEKO],
    'dm/gfx/nvn/nvn_tex.h': [DEKO],
}
THIRD_PARTY_PATH = re.compile(r'(^|/)third_party/')


LICENCE_NAME = re.compile(r'(LICEN[SC]E|COPYING|FTL\.TXT|THIRD-PARTY-NOTICES)', re.I)


def sections(text):

    out = {}
    inside = True
    for line in text.splitlines():
        if line.rstrip() == NOT_INSIDE:
            inside = False
        elif line.startswith('## '):
            out.setdefault(line[3:].strip(), inside)
    return out


def norm(text):
    return '\n'.join(l.rstrip() for l in text.replace('\r\n', '\n').strip('\n﻿').splitlines())


def check(apk, notices, runtime=None, translator=None):

    text = open(notices, encoding='utf-8').read()
    have = sections(text)
    bad = []
    if re.search(r'\bqa (app|variant)\b', text):
        bad.append('the notices speak of the qa app')
    need = {}

    def want(names, what):
        for n in names:
            need.setdefault(n, what)

    with zipfile.ZipFile(apk) as z:
        names = z.namelist()
        for n in names:
            if not n.startswith('lib/'):
                continue
            base = n.rsplit('/', 1)[1]
            got = LIBS.get(base) or next((s for p, s in LIB_PATTERNS if p.fullmatch(base)), None)
            if got is None:
                bad.append('%s is in no notices mapping (check_notices.py LIBS)' % n)
            else:
                want(got, n)
        inner = {}
        for part in ('runtime', 'tools', 'queries'):
            entry = 'assets/py/%s.zip' % part
            if entry in names:
                inner[part] = zipfile.ZipFile(io.BytesIO(z.read(entry)))
    if 'runtime' in inner:
        for n in inner['runtime'].namelist():
            got = next((s for p, s in RUNTIME if p.match(n)), None)
            if got is None:
                bad.append('runtime.zip: %s is in no notices mapping (check_notices.py RUNTIME)' % n)
            else:
                want(got, 'runtime.zip ' + n)
    if 'tools' in inner:
        for n in inner['tools'].namelist():
            if n in TOOLS:
                want(TOOLS[n], 'tools.zip ' + n)
            elif THIRD_PARTY_PATH.search(n):
                bad.append('tools.zip: %s is third-party code without a notices mapping (check_notices.py TOOLS)' % n)
            else:
                want([UPSTREAM], 'tools.zip ' + n)
    if 'queries' in inner:
        want([UPSTREAM], 'queries.zip')
    for name, what in sorted(need.items()):
        if name not in have:
            bad.append('no section "## %s" for %s' % (name, what))
        elif not have[name]:
            bad.append('"## %s" (needed by %s) stands under "Components not inside the app"' % (name, what))
    flat = norm(text)
    for root in (runtime, translator):
        if not root:
            continue
        lic = os.path.join(root, 'licenses')
        if not os.path.isdir(lic):
            bad.append('%s has no licenses/ folder' % root)
            continue
        for top, _, files in os.walk(lic):
            for f in sorted(files):
                if not LICENCE_NAME.search(f):
                    continue
                p = os.path.join(top, f)
                body = norm(open(p, encoding='utf-8', errors='strict').read())
                if body not in flat:
                    bad.append('licence text %s is not in the notices word for word' % os.path.relpath(p, root))
    return bad


def main():
    a = argparse.ArgumentParser(description='Third-party notices inventory of an APK')
    a.add_argument('apk')
    a.add_argument('notices')
    a.add_argument('--runtime')
    a.add_argument('--translator')
    o = a.parse_args()
    bad = check(o.apk, o.notices, o.runtime, o.translator)
    for b in bad:
        print('check_notices: ' + b)
    print('check_notices: %s' % ('FAILED' if bad else 'OK'))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
