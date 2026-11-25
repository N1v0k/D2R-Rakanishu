#!/usr/bin/env python3


import os
import sys
import zipfile

TIME = (1980, 1, 1, 0, 0, 0)


def write(entries, archive):

    part = archive + '.part'
    with zipfile.ZipFile(part, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for rel in sorted(entries):
            info = zipfile.ZipInfo(rel, TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            with open(entries[rel], 'rb') as f:
                z.writestr(info, f.read(), compresslevel=9)
    os.replace(part, archive)


def pack(folder, archive):
    entries = {}
    for top, dirs, files in os.walk(folder):
        for f in files:
            full = os.path.join(top, f)
            if os.path.islink(full) or not os.path.isfile(full):
                raise SystemExit('not a regular file: %s' % os.path.relpath(full, folder))
            entries[os.path.relpath(full, folder).replace(os.sep, '/')] = full
    write(entries, archive)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit('usage: pack_zip.py <folder> <archive>')
    pack(sys.argv[1], sys.argv[2])
