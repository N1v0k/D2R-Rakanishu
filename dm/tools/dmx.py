#!/usr/bin/env python3


























import fnmatch, hashlib, os, struct, sys, zlib

ROOT = os.environ.get("DMX_ROOT", "~/Diablo II Resurrected - 提取内容/Update/RomFS/data")
_IDX = None
_FH = {}


def index():
    global _IDX
    if _IDX is None:
        _IDX = {}

        with open(os.path.join(ROOT, "build_manifest.psv"), encoding="utf-8", errors="replace") as f:
            next(f)
            for line in f:
                if line.startswith("#"):
                    continue
                c = line.rstrip("\n").split("|")
                if len(c) < 11:
                    continue
                _IDX[c[9].lower()] = c
    return _IDX


def lookup(path):
    idx = index()
    k = path.lower().replace("\\", "/")
    for cand in (k, "data:" + k, "data:data/" + k):
        if cand in idx:
            return idx[cand]
    raise KeyError(path)


def _fh(chunk, arch):
    key = (chunk, arch)
    if key not in _FH:
        _FH[key] = open(os.path.join(ROOT, f"{chunk:02d}-{arch:08d}.data"), "rb")
    return _FH[key]


def _block(b):
    m = b[:1]
    if m == b"N":
        return b[1:]
    if m == b"Z":
        return zlib.decompress(b[1:])
    raise ValueError(f"未知 BLTE 块模式 {m!r}")


def read_row(c):
    chunk, arch, csize, esize, enc = int(c[2]), int(c[3]), int(c[5]), int(c[7]), c[8]
    ekey = bytes.fromhex(c[6])
    off = int.from_bytes(ekey[11:16], "big")
    assert ekey[9] == chunk and ekey[10] == arch, (c[9], ekey.hex())
    f = _fh(chunk, arch)
    f.seek(off)
    raw = f.read(esize)
    if raw[:4] == b"BLTE":
        hlen = struct.unpack(">I", raw[4:8])[0]
        n = int.from_bytes(raw[9:12], "big")
        pos, out = hlen, []
        for i in range(n):
            es, cs = struct.unpack(">II", raw[12 + 24 * i:20 + 24 * i])
            out.append(_block(raw[pos:pos + es]))
            pos += es
        data = b"".join(out)
    elif enc == "n":
        data = raw
    elif enc == "z":
        data = zlib.decompress(raw)
    else:
        raise ValueError(f"未知编码 {enc}")
    if len(data) != csize or hashlib.md5(data).hexdigest() != c[4]:
        raise ValueError(f"校验失败 {c[9]} len={len(data)} csize={csize}")
    return data


def get(path):
    return read_row(lookup(path))


if __name__ == "__main__":
    if sys.argv[1] == "get":
        d = get(sys.argv[2])
        open(sys.argv[3], "wb").write(d)
        print(len(d), "bytes, md5 ok")
    elif sys.argv[1] == "list":
        pat = sys.argv[2].lower() if len(sys.argv) > 2 else "*"
        for k, c in index().items():
            if fnmatch.fnmatch(k, pat):
                print(f"{c[9]}\t{c[5]}")
