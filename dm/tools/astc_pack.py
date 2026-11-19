#!/usr/bin/env python3


















































import argparse, os, re, struct, subprocess, sys, tempfile, time
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'ios', 'tools'))
import touch_layout

CODE_ASTC = {0x3d: 0x5f, 0x3e: 0x51}
BATCH_BLOCKS = 1 << 22
ROW_BLOCKS = 1024


def bc3_decode(raw):

    b = np.frombuffer(raw, np.uint8).reshape(-1, 16)
    n = len(b)
    a0 = b[:, 0].astype(np.int32); a1 = b[:, 1].astype(np.int32)
    abits = np.zeros(n, np.uint64)
    for i in range(6): abits |= b[:, 2 + i].astype(np.uint64) << np.uint64(8 * i)
    k = np.arange(8, dtype=np.int32)
    p8 = np.where(k == 0, a0[:, None], np.where(k == 1, a1[:, None], ((8 - k) * a0[:, None] + (k - 1) * a1[:, None] + 3) // 7))
    p6 = np.where(k == 0, a0[:, None], np.where(k == 1, a1[:, None], np.where(k == 6, 0, np.where(k == 7, 255, ((6 - k) * a0[:, None] + (k - 1) * a1[:, None] + 2) // 5))))
    apal = np.where((a0 > a1)[:, None], p8, p6)
    aidx = ((abits[:, None] >> (np.uint64(3) * np.arange(16, dtype=np.uint64))) & np.uint64(7)).astype(np.int64)
    alpha = np.take_along_axis(apal, aidx, 1)
    c0 = b[:, 8].astype(np.int32) | (b[:, 9].astype(np.int32) << 8); c1 = b[:, 10].astype(np.int32) | (b[:, 11].astype(np.int32) << 8)
    def rgb(c): r, g, bl = (c >> 11) & 31, (c >> 5) & 63, c & 31; return np.stack([(r << 3) | (r >> 2), (g << 2) | (g >> 4), (bl << 3) | (bl >> 2)], -1)
    e0, e1 = rgb(c0), rgb(c1)
    cpal = np.stack([e0, e1, (2 * e0 + e1 + 1) // 3, (e0 + 2 * e1 + 1) // 3], 1)
    cbits = b[:, 12].astype(np.uint32) | (b[:, 13].astype(np.uint32) << 8) | (b[:, 14].astype(np.uint32) << 16) | (b[:, 15].astype(np.uint32) << 24)
    cidx = ((cbits[:, None] >> (np.uint32(2) * np.arange(16, dtype=np.uint32))) & np.uint32(3)).astype(np.int64)
    col = np.take_along_axis(cpal, cidx[:, :, None].repeat(3, 2), 1)
    return np.concatenate([col, alpha[:, :, None]], -1).astype(np.uint8)


def blocks_to_image(px):
    n = len(px); rows = (n + ROW_BLOCKS - 1) // ROW_BLOCKS
    pad = np.zeros((rows * ROW_BLOCKS - n, 16, 4), np.uint8)
    a = np.concatenate([px, pad]).reshape(rows, ROW_BLOCKS, 4, 4, 4).transpose(0, 2, 1, 3, 4).reshape(rows * 4, ROW_BLOCKS * 4, 4)
    return a


def image_to_blocks(img, n):
    rows = img.shape[0] // 4
    return img.reshape(rows, 4, ROW_BLOCKS, 4, 4).transpose(0, 2, 1, 3, 4).reshape(-1, 16, 4)[:n]


def encode_batch(enc, preset, srgb, px, tmp):

    n = len(px); src, out, dec = (os.path.join(tmp, f) for f in ('in.png', 'out.astc', 'dec.png'))
    Image.fromarray(blocks_to_image(px), 'RGBA').save(src, compress_level=1)
    prof = '-cs' if srgb else '-cl'
    subprocess.run([enc, prof, src, out, '4x4', '-' + preset, '-silent'], check=True)
    a = open(out, 'rb').read()
    assert a[:4] == bytes.fromhex('13aba15c') and a[4:7] == bytes((4, 4, 1)), 'astcenc 输出头不对'
    w = int.from_bytes(a[7:10], 'little'); h = int.from_bytes(a[10:13], 'little')
    assert w == ROW_BLOCKS * 4 and h % 4 == 0
    blocks = np.frombuffer(a[16:], np.uint8).reshape(h // 4, ROW_BLOCKS, 16).reshape(-1, 16)[:n].tobytes()
    subprocess.run([enc, '-ds' if srgb else '-dl', out, dec, '-silent'], check=True)
    back = image_to_blocks(np.asarray(Image.open(dec).convert('RGBA')), n)
    return blocks, back


def psnr(a, b):
    mse = float(np.mean((a.astype(np.float64) - b.astype(np.float64)) ** 2))
    return 99.0 if mse == 0 else 10 * np.log10(255.0 ** 2 / mse)


def finalize(packdir):
    import hashlib, json
    idx = open(os.path.join(packdir, 'pack_index.tsv'), encoding='utf-8').readline().rstrip('\n').split('\t')
    assert idx[0] == '# pack', 'pack_index.tsv 首行不对'
    name = idx[1]; src = os.path.join(packdir, name); h = hashlib.sha256()
    with open(src, 'rb') as f:
        for b in iter(lambda: f.read(64 << 20), b''): h.update(b)
    info = {'name': name, 'size': os.path.getsize(src), 'sha256': h.hexdigest()}
    json.dump(info, open(os.path.join(packdir, 'pack.json'), 'w'), ensure_ascii=False, indent=1)
    root = os.path.join(packdir, 'Legacy Device Pack'); os.makedirs(os.path.join(root, 'data'), exist_ok=True)
    dst = os.path.join(root, 'data', name)
    if os.path.exists(dst): os.remove(dst)
    os.link(src, dst)
    json.dump({'files': [info]}, open(os.path.join(root, 'manifest.json'), 'w'), ensure_ascii=False, indent=1)
    print(f'astc_pack finalize：{name} {info["size"]} 字节 sha256 {info["sha256"]}')


def desc_cache(o, converted, tmp, repl):

    import base64, json
    logical = 'data/hd/texture_desc_cache.json'
    row = next(f for f in (l.rstrip('\n').split('|') for l in open(os.path.join(o.romfs, 'data', 'build_manifest.psv'), encoding='utf-8')) if len(f) >= 11 and f[9].split(':', 1)[-1].lower() == logical)
    J = json.loads(touch_layout._read_entry(o.romfs, row))
    jobs = {False: [], True: []}
    nconv = 0
    for e in J:
        p = e['filename'].lower()
        if p not in converted or e['format'] not in (61, 62): continue
        srgb = converted[p]
        if (e['format'] == 62) != srgb: sys.exit(f'astc_pack：{p} 缓存里的格式 {e["format"]} 与文件头不一致')
        e['format'] = 81 if srgb else 95; nconv += 1
        t = base64.b64decode(e['thumbnailBase64']) if e.get('thumbnailBase64') else b''
        if not t: continue
        n = struct.unpack_from('<I', t, 0)[0]; tab = 4 + struct.unpack_from('<i', t, 4)[0]; regs = []; px = []
        for i in range(n):
            size = struct.unpack_from('<I', t, tab + 8 * i)[0]; rf = tab + 8 * i + 4; start = rf + struct.unpack_from('<i', t, rf)[0]
            if size % 16 or start < tab + 8 * n or start + size > len(t): sys.exit(f'astc_pack：{p} 缩略图第 {i} 级（大小 {size}，起点 {start}）不是表后的整块')
            regs.append((start, size // 16)); px.append(bc3_decode(t[start:start + size]))
        jobs[srgb].append((e, t, regs, np.concatenate(px)))
    for srgb, items in jobs.items():
        if not items: continue
        blocks, _ = encode_batch(o.astcenc, o.preset, srgb, np.concatenate([it[3] for it in items]), tmp)
        at = 0
        for e, t, regs, px in items:
            nt = bytearray(t)
            for start, nb in regs: nt[start:start + nb * 16] = blocks[at * 16:(at + nb) * 16]; at += nb
            e['thumbnailBase64'] = base64.b64encode(bytes(nt)).decode()
    dst = os.path.join(o.out, 'files', logical); os.makedirs(os.path.dirname(dst), exist_ok=True)
    open(dst, 'w', encoding='utf-8').write(json.dumps(J, ensure_ascii=False, separators=(',', ':')))
    repl.write(f'{logical}\t{dst}\n')
    print(f'astc_pack：贴图描述缓存改了 {nconv} 条（缩略图 {sum(len(v) for v in jobs.values())} 张转 ASTC）')


def main():
    if len(sys.argv) == 3 and sys.argv[1] == 'finalize': finalize(sys.argv[2]); return
    ap = argparse.ArgumentParser(); ap.add_argument('romfs'); ap.add_argument('out')
    ap.add_argument('--astcenc', default=os.environ.get('ASTCENC')); ap.add_argument('--preset', default='thorough')
    ap.add_argument('--limit', type=int, default=0); ap.add_argument('--only', default='')
    o = ap.parse_args()
    if not o.astcenc or not os.access(o.astcenc, os.X_OK): sys.exit('astc_pack：找不到 astcenc（--astcenc 或环境变量 ASTCENC）')
    rows = [l.rstrip('\n').split('|') for l in open(os.path.join(o.romfs, 'data', 'build_manifest.psv'), encoding='utf-8')]
    rows = [f for f in rows if len(f) >= 11 and f[9].lower().endswith('.texture') and o.only in f[9].lower()]
    seen = set(); todo = []
    for f in rows:
        p = f[9].split(':', 1)[-1].lower()
        if p not in seen: seen.add(p); todo.append((p, f))
    if o.limit: todo = todo[:o.limit]
    os.makedirs(os.path.join(o.out, 'files'), exist_ok=True)
    t0 = time.time(); stat = {'跳过非 BC3': 0, '转码': 0, '块数': 0}
    pend = {False: [], True: []}
    qual = open(os.path.join(o.out, 'quality.tsv'), 'w'); qual.write('逻辑路径\t类\t宽\t高\t块数\tPSNR\n')
    repl = open(os.path.join(o.out, 'repl.tsv'), 'w')
    tmp = tempfile.mkdtemp(prefix='astc_pack_'); converted = {}

    def flush(srgb):
        items = pend[srgb]
        if not items: return
        px = np.concatenate([it[3] for it in items])
        blocks, back = encode_batch(o.astcenc, o.preset, srgb, px, tmp)
        at = 0
        for p, b, start, dec in items:
            n = len(dec); nb = bytearray(b); nb[4] = CODE_ASTC[b[4]]; nb[start:] = blocks[at * 16:(at + n) * 16]
            dst = os.path.join(o.out, 'files', p); os.makedirs(os.path.dirname(dst), exist_ok=True); open(dst, 'wb').write(nb)
            repl.write(f'{p}\t{dst}\n'); converted[p] = srgb
            w, h = struct.unpack_from('<II', b, 8)
            cls = 'sRGB 颜色' if srgb else ('法线' if p.endswith('_nrm.texture') else '线性')
            qual.write(f'{p}\t{cls}\t{w}\t{h}\t{n}\t{psnr(dec, back[at:at + n]):.2f}\n')
            at += n
        pend[srgb] = []
        print(f'astc_pack：已转 {stat["转码"]} 张、{stat["块数"]} 块，用时 {time.time() - t0:.0f} 秒', flush=True)

    for i, (p, f) in enumerate(todo):
        b = touch_layout._read_entry(o.romfs, f)
        if len(b) < 48 or b[4] not in CODE_ASTC: stat['跳过非 BC3'] += 1; continue
        start = 40 + struct.unpack_from('<I', b, 40)[0]
        if start >= len(b) or (len(b) - start) % 16: sys.exit(f'astc_pack：{p} 数据区不是 16 字节整块（起点 {start}，文件 {len(b)} 字节）')
        dec = bc3_decode(b[start:]); srgb = b[4] == 0x3e
        pend[srgb].append((p, b, start, dec)); stat['转码'] += 1; stat['块数'] += len(dec)
        if sum(len(x[3]) for x in pend[srgb]) >= BATCH_BLOCKS: flush(srgb)
    flush(False); flush(True); qual.close()
    desc_cache(o, converted, tmp, repl)
    repl.close()
    s = f'astc_pack：贴图 {len(todo)} 张，转码 {stat["转码"]} 张（{stat["块数"]} 块），跳过非 BC3 {stat["跳过非 BC3"]} 张；档 {o.preset}；墙钟 {time.time() - t0:.0f} 秒'
    open(os.path.join(o.out, 'summary.txt'), 'w').write(s + '\n'); print(s)


if __name__ == '__main__':
    main()
