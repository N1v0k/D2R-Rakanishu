#!/usr/bin/env python3
















import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import game_addrs as ga

SKIP = ('dm/third_party/',)
TABLE_FILES = ('dm/host/game_addrs.def', 'dm/tools/game_addrs_allow.tsv')


def tracked(root):
    try:
        out = subprocess.run(['git', '-C', root, 'ls-files', '-z'], check=True, capture_output=True).stdout
        return [p for p in out.decode('utf-8', 'surrogateescape').split('\0') if p]
    except (OSError, subprocess.CalledProcessError):
        files = []
        for d, dirs, fs in os.walk(root):
            dirs[:] = [x for x in dirs if not x.startswith('.') and x != 'build']
            files += [os.path.relpath(os.path.join(d, f), root) for f in fs]
        return files


def in_range(table, v):
    lay = table.layout
    if not (lay['ADDR_LOW'] <= v < lay['IMAGE_END']):
        return False
    if v < lay['TEXT_END']:
        return v % 4 == 0
    return v >= lay['RO_START']


def check(root, table):
    allow = ga.load_allow(os.path.join(root, 'dm', 'tools', 'game_addrs_allow.tsv'))
    findings = []
    used, used_fields = set(), set()
    name_rx = re.compile(r'\bGA[._]([A-Z][A-Z0-9_]*)')
    field_rx = re.compile(r'\bGF[._]([A-Z][A-Z0-9_]*)')
    for rel in tracked(root):
        if rel.startswith(SKIP) or rel in TABLE_FILES:
            continue
        path = os.path.join(root, rel)
        try:
            with open(path, encoding='utf-8') as f:
                text = f.read()
        except (UnicodeDecodeError, IsADirectoryError, FileNotFoundError):
            continue
        if 'GA' in text:
            used.update(name_rx.findall(text))
        if 'GF' in text:
            used_fields.update(field_rx.findall(text))
        if '0x' not in text and '0X' not in text:
            continue
        lines = None
        for a, b, v, suf, cx in ga.hex_tokens(rel, text):
            if cx == 's':
                if v in table.by_addr:
                    used.add(table.by_addr[v])
                continue
            name = table.by_addr.get(v)
            if name:
                why = f'table address {name}: write {ga.spell(name, rel)}'
            elif cx == 'k' and in_range(table, v):
                why = 'address-range literal in code: name it in dm/host/game_addrs.def or allow-list it'
            else:
                continue
            if ga.allowed(allow, v, rel):
                continue
            if lines is None:
                lines = [0] + [m.end() for m in re.finditer('\n', text)]
            import bisect
            ln = bisect.bisect_right(lines, a)
            findings.append(f'{rel}:{ln}: {text[a:b]} {why}')
    for e in table.entries:
        if e[1] not in used:
            findings.append(f'dm/host/game_addrs.def: {e[1]} is used nowhere')
    for f in table.fields:
        if f[1] not in used_fields:
            findings.append(f'dm/host/game_addrs.def: field {f[1]} is used nowhere')
    return findings


def main(argv):
    report = '--report' in argv
    rest = [a for a in argv if a != '--report']
    root = os.path.abspath(rest[0]) if rest else os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
    try:
        table = ga.Table(os.path.join(root, 'dm', 'host', 'game_addrs.def'))
        for k in ('ADDR_LOW', 'TEXT_END', 'RO_START', 'IMAGE_END'):
            if k not in table.layout:
                raise ga.TableError(f'layout value {k} missing')
    except (OSError, ga.TableError) as e:
        print(f'check_game_addrs: {e}', file=sys.stderr)
        return 1
    findings = check(root, table)
    with open(os.path.join(root, 'dm', 'PRODUCT'), encoding='utf-8') as f:
        product = f.readline().strip()
    if table.build.get('main', ('',))[0] != product.split('-')[0]:
        findings.append(f'dm/host/game_addrs.def: game version {table.build.get("main", ("none",))[0]} is not the one of '
                        f'dm/PRODUCT ({product})')
    for f in findings:
        print(f)
    print(f'check_game_addrs: {len(table.entries)} names, {len(findings)} findings' + (' (report mode)' if report else ''),
          file=sys.stderr)
    return 0 if report or not findings else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
