#!/usr/bin/env python3







import hashlib
import struct
import sys

DT_NULL, DT_STRTAB, DT_RPATH, DT_RUNPATH = 0, 5, 15, 29
PT_LOAD, PT_DYNAMIC, PT_NOTE = 1, 2, 4
NT_GNU_BUILD_ID = 3


def _offset(phdrs, vaddr):
    for p_type, p_offset, p_vaddr, p_filesz in phdrs:
        if p_type == PT_LOAD and p_vaddr <= vaddr < p_vaddr + p_filesz:
            return p_offset + vaddr - p_vaddr
    raise ValueError('address 0x%x is in no loaded segment' % vaddr)


def clean(path):

    with open(path, 'r+b') as f:
        data = bytearray(f.read())
        if data[:4] != b'\x7fELF' or data[4] != 2 or data[5] != 1:
            raise ValueError('%s is not a 64-bit little-endian ELF file' % path)
        phoff, = struct.unpack_from('<Q', data, 0x20)
        phentsize, phnum = struct.unpack_from('<HH', data, 0x36)
        phdrs = []
        dynamic = None
        for i in range(phnum):
            p_type, _, p_offset, p_vaddr, _, p_filesz, _, _ = struct.unpack_from('<IIQQQQQQ', data, phoff + i * phentsize)
            phdrs.append((p_type, p_offset, p_vaddr, p_filesz))
            if p_type == PT_DYNAMIC:
                dynamic = (p_offset, p_filesz)
        notes = [(o, n) for t, o, _, n in phdrs if t == PT_NOTE]
        if dynamic is None:
            return _write_id(f, data, notes)
        entries = [struct.unpack_from('<qQ', data, dynamic[0] + 16 * i) for i in range(dynamic[1] // 16)]
        end = next(i for i, (tag, _) in enumerate(entries) if tag == DT_NULL)
        live = entries[:end]
        strtab = _offset(phdrs, next(v for t, v in live if t == DT_STRTAB))
        drop, changed = [], False
        for t, v in live:
            if t not in (DT_RPATH, DT_RUNPATH):
                continue
            start = strtab + v
            stop = data.index(0, start)
            parts = data[start:stop].split(b':')
            kept = b':'.join(x for x in parts if x.startswith(b'$ORIGIN'))
            if kept == data[start:stop]:
                continue
            changed = True
            data[start:stop] = kept + b'\0' * (stop - start - len(kept))
            if not kept:
                drop.append((t, v))
        if not changed:
            return _write_id(f, data, notes)
        keep = [(t, v) for t, v in live if (t, v) not in drop] + [(DT_NULL, 0)] * (len(entries) - len(live) + len(drop))
        for i, (t, v) in enumerate(keep):
            struct.pack_into('<qQ', data, dynamic[0] + 16 * i, t, v)
        _write_id(f, data, notes, True)
        return 1


def _write_id(f, data, notes, changed=False):
    old = bytes(data)
    _rebuild_id(data, notes)
    if changed or data != old:
        f.seek(0)
        f.write(data)
    return 0


def _rebuild_id(data, notes):
    for start, size in notes:
        i = start
        while i + 12 <= start + size:
            namesz, descsz, kind = struct.unpack_from('<III', data, i)
            name = i + 12
            desc = name + ((namesz + 3) & ~3)
            if kind == NT_GNU_BUILD_ID and data[name:name + namesz] == b'GNU\0':
                data[desc:desc + descsz] = b'\0' * descsz
                data[desc:desc + descsz] = hashlib.sha1(data).digest()[:descsz].ljust(descsz, b'\0')
                return
            i = desc + ((descsz + 3) & ~3)


if __name__ == '__main__':
    for p in sys.argv[1:]:
        clean(p)
