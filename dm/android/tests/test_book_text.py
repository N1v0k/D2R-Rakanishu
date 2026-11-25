#!/usr/bin/env python3



import csv
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
SRC = os.path.join(ROOT, 'dm', 'android', 'app', 'src', 'main', 'java', 'com', 'xiaoboluo', 'd2r')


def used():
    page = open(os.path.join(SRC, 'BookPage.java'), encoding='utf-8').read()
    keys = set()
    for call in re.findall(r'\bt\(([^;]*?)\)', page):
        keys |= {'dmAndroidBook' + k for k in re.findall(r'"([A-Z][A-Za-z0-9]+)"', call)}
    keys |= {'dmAndroidBook' + k for k in re.findall(r'"(Group[A-Za-z]+)"', page)}
    for name in ('BookStats.java', 'AppMenu.java'):
        keys |= set(re.findall(r'dmAndroidBook[A-Za-z0-9]+', open(os.path.join(SRC, name), encoding='utf-8').read()))
    return keys


def calls(text):

    out = []
    for m in re.finditer(r'(?<![A-Za-z0-9_.])t\("([A-Z][A-Za-z0-9]+)"', text):
        depth, i, args, quoted = 1, m.end(), 0, False
        while depth:
            c = text[i]
            if c == '"' and text[i - 1] != '\\':
                quoted = not quoted
            elif not quoted:
                if c in '([{':
                    depth += 1
                elif c in ')]}':
                    depth -= 1
                elif c == ',' and depth == 1:
                    args += 1
            i += 1
        out.append(('dmAndroidBook' + m.group(1), args))
    return out


def placeholders(text):
    numbers = [int(n) for n in re.findall(r'%(\d+)\$', text)]
    return max(numbers) if numbers else 0


def main():
    with open(os.path.join(ROOT, 'dm', 'i18n', 'ui_text.tsv'), encoding='utf-8') as f:
        table = {r['键']: r for r in csv.DictReader(f, delimiter='\t') if r['键'].startswith('dmAndroidBook')}
    rows = set(table)
    page = open(os.path.join(SRC, 'BookPage.java'), encoding='utf-8').read()
    for key, args in calls(page):
        for column in ('中文', 'English', 'Español', 'Русский', '한국어', 'Deutsch', 'Português (Brasil)'):
            text = table.get(key, {}).get(column) or table.get(key, {}).get('English', '')
            if key in table and placeholders(text) != args:
                sys.exit('test_book_text: %s gets %d values, its %s text %r has %d placeholders' % (key, args, column, text, placeholders(text)))
    keys = used()
    missing, unused = sorted(keys - rows), sorted(rows - keys)
    if missing or unused:
        sys.exit('test_book_text: missing %s, unused %s' % (missing, unused))
    print('test_book_text: %d texts, all used and present' % len(keys))


if __name__ == '__main__':
    main()
