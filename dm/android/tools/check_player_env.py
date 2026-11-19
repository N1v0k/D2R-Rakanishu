#!/usr/bin/env python3
"""Check that player builds can read exactly the environment values set by the app."""
import argparse
import pathlib
import re
import sys

KEEP = re.compile(r'^\s*X\("(DM_[A-Z0-9_]+)"\)\s*\\?\s*$')
C_SET = re.compile(r'\bsetenv\(\s*"(DM_[A-Z0-9_]+)"')
JAVA_SET = re.compile(r'\bOs\.setenv\(\s*"(DM_[A-Z0-9_]+)"')
DEBUG_KEYS = re.compile(r'dm_android_debug_keys\[\]\s*=\s*\{([^}]*)\}')


def allowlist(root):
    names = []
    for line in (root / 'dm/android/compat/include/dm_player_env.h').read_text(encoding='utf-8').splitlines():
        m = KEEP.match(line)
        if m:
            names.append(m.group(1))
    return names


def app_names(root):
    found = {}
    android = root / 'dm/android'
    sources = [p for pattern in ('*.c', '*.h', '*.cpp') for p in android.glob(pattern)]
    sources += [p for p in (android / 'compat').rglob('*') if p.suffix in ('.c', '.h', '.cpp') and p.name != 'dm_player_env.h']
    for p in sorted(sources):
        for name in C_SET.findall(p.read_text(encoding='utf-8')):
            found.setdefault(name, p.relative_to(root).as_posix())
    for p in sorted((android / 'app/src').rglob('*.java')):
        for name in JAVA_SET.findall(p.read_text(encoding='utf-8')):
            found.setdefault(name, p.relative_to(root).as_posix())


    debug = android / 'debug_env.h'
    if debug.exists():
        m = DEBUG_KEYS.search(debug.read_text(encoding='utf-8'))
        if not m:
            raise SystemExit('check_player_env: dm_android_debug_keys not found in dm/android/debug_env.h')
        for name in re.findall(r'"(DM_[A-Z0-9_]+)"', m.group(1)):
            found.setdefault(name, 'dm/android/debug_env.h')
    return found


def host_names(root):
    found = {}
    for top in ('dm/host', 'dm/gfx'):
        for p in sorted((root / top).rglob('*')):
            if p.suffix not in ('.c', '.h', '.cpp') or '/tests/' in '/' + p.relative_to(root).as_posix():
                continue
            for name in C_SET.findall(p.read_text(encoding='utf-8', errors='replace')):
                if 'TEST' not in name:
                    found.setdefault(name, p.relative_to(root).as_posix())
    return found


def check(root):
    errors = []
    keep = allowlist(root)
    if not keep:
        errors.append('dm_player_env.h: no X("DM_...") lines found')
    if keep != sorted(set(keep)):
        errors.append('dm_player_env.h: the list must be sorted and free of duplicates')
    app = app_names(root)
    for name in sorted(set(app) - set(keep)):
        errors.append(f'{name} is set by the App ({app[name]}) but missing from dm_player_env.h')
    for name in sorted(set(keep) - set(app)):
        errors.append(f'{name} is in dm_player_env.h but the App never sets it')
    host = host_names(root)
    for name in sorted(set(host) - set(keep)):
        errors.append(f'{name} is set by {host[name]} but missing from dm_player_env.h (player could not read it back)')
    return keep, errors


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[-1], formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--root', type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[3])
    args = ap.parse_args()
    keep, errors = check(args.root)
    for e in errors:
        print('check_player_env: ' + e)
    if errors:
        return 1
    print(f'check_player_env: {len(keep)} names, list matches the source')
    return 0


if __name__ == '__main__':
    sys.exit(main())
