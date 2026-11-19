#!/usr/bin/env python3









import struct
import sys
import zipfile

LIB = 'lib/arm64-v8a/'
EXTRA = ('libdm_driverload.so', 'libhook_impl.so', 'libmain_hook.so')


def sections(elf):
    shoff, = struct.unpack_from('<Q', elf, 40)
    shentsize, shnum = struct.unpack_from('<HH', elf, 58)
    return [struct.unpack_from('<IIQQQQIIQQ', elf, shoff + i * shentsize) for i in range(shnum)]


def cstr(elf, off):
    return elf[off:elf.index(b'\0', off)].decode('latin-1')


def needed_and_undefined(elf):
    if elf[:4] != b'\x7fELF' or elf[4] != 2:
        raise ValueError('not a 64-bit ELF')
    sh = sections(elf)
    needed, undefined = [], []
    for name, typ, flags, addr, off, size, link, info, align, entsize in sh:
        strtab = sh[link][4] if typ in (6, 11) else 0
        if typ == 6:
            for d in range(off, off + size, 16):
                tag, val = struct.unpack_from('<qQ', elf, d)
                if tag == 0:
                    break
                if tag == 1:
                    needed.append(cstr(elf, strtab + val))
        elif typ == 11:
            for s in range(off, off + size, 24):
                st_name, st_info, st_other, st_shndx = struct.unpack_from('<IBBH', elf, s)
                if st_shndx == 0 and st_name:
                    undefined.append(cstr(elf, strtab + st_name))
    return needed, undefined


def main(argv):
    if len(argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    bad = []
    with zipfile.ZipFile(argv[1]) as apk:
        names = set(apk.namelist())
        for n in EXTRA:
            if LIB + n not in names:
                bad.append(f'{n} missing from the APK')
        needed, undefined = needed_and_undefined(apk.read(LIB + 'libdm.so'))
    for n in ('libvulkan.so',) + EXTRA:
        if n in needed:
            bad.append(f'libdm.so has NEEDED {n}')
    vk = sorted(u for u in undefined if u.startswith('vk'))
    if vk:
        bad.append('libdm.so imports ' + ', '.join(vk[:8]) + (' ...' if len(vk) > 8 else ''))
    for b in bad:
        print('check_driver_libs: ' + b)
    if not bad:
        print(f'check_driver_libs: ok (libdm.so NEEDED {", ".join(needed)}; {", ".join(EXTRA)} packaged)')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
