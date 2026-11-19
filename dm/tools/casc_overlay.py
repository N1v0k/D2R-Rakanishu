#!/usr/bin/env python3
















































import hashlib
from pathlib import Path
import struct
import sys
import zlib


def die(message):
    raise SystemExit(f"casc_overlay.py: {message}")


def norm_path(value):
    value = value.strip().replace("\\", "/")
    while value.lower().startswith("data:"):
        value = value[5:]
    return value.lstrip("/").lower()


def read_replacements(filename):
    result = {}
    with open(filename, encoding="utf-8") as stream:
        for number, line in enumerate(stream, 1):
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            fields = line.rstrip("\n").split("\t")
            if len(fields) != 2:
                die(f"{filename}:{number}: expected path<TAB>file")
            add = fields[0].startswith("+")
            logical, replacement = ("+" if add else "") + norm_path(fields[0].lstrip("+")), Path(fields[1])
            if not logical or logical in result:
                die(f"{filename}:{number}: duplicate or empty logical path")
            if not replacement.is_file():
                die(f"{filename}:{number}: replacement is not a file: {replacement}")
            result[logical] = replacement
    return result


def link_tree(source, output, skip=()):
    output.mkdir(parents=True, exist_ok=True)
    for entry in source.iterdir():
        if entry.name in skip:
            continue
        target = output / entry.name
        if entry.is_dir() and not entry.is_symlink():
            link_tree(entry, target)
        else:
            target.symlink_to(entry.resolve())


def read_manifest(source):
    result = {}
    filename = source / "data" / "build_manifest.psv"
    if not filename.is_file():
        die(f"missing {filename}; TVFS lookup needs the extraction manifest")
    with filename.open(encoding="utf-8") as stream:
        for line in stream:
            fields = line.rstrip("\n").split("|")
            if len(fields) < 11 or fields[0].startswith("#"):
                continue
            path = norm_path(fields[9])
            if path and path not in result:
                result[path] = fields
    return result


def read_data_file(data_dir, ekey, encoded_size):
    chunk, archive = ekey[9], ekey[10]
    filename = data_dir / f"{chunk:02d}-{archive:08d}.data"
    with filename.open("rb") as stream:
        stream.seek(int.from_bytes(ekey[11:16], "big"))
        raw = stream.read(encoded_size)
    if raw[:4] != b"BLTE":
        die(f"expected BLTE at {filename}:{int.from_bytes(ekey[11:16], 'big'):#x}")
    header_size = struct.unpack(">I", raw[4:8])[0]
    block_count = int.from_bytes(raw[9:12], "big")
    position, output = header_size, bytearray()
    for index in range(block_count):
        encoded, decoded = struct.unpack(">II", raw[12 + index * 24:20 + index * 24])
        block = raw[position:position + encoded]
        if block[:1] == b"N":
            content = block[1:]
        elif block[:1] == b"Z":
            content = zlib.decompress(block[1:])
        else:
            die(f"unsupported BLTE block mode {block[:1]!r}")
        if len(content) != decoded:
            die("BLTE decoded length mismatch")
        output.extend(content)
        position += encoded
    return bytes(output)


def blte_encode(content, block_size=256 * 1024):
    blocks = []
    for start in range(0, len(content), block_size):
        block = content[start:start + block_size]
        blocks.append((b"Z" + zlib.compress(block, 9), block))
    if not blocks:
        blocks.append((b"N", b""))
    header_size = 12 + 24 * len(blocks)
    header = bytearray(b"BLTE" + struct.pack(">I", header_size) + b"\x0f" + len(blocks).to_bytes(3, "big"))
    for encoded, block in blocks:
        header.extend(struct.pack(">II", len(encoded), len(block)))
        header.extend(hashlib.md5(encoded).digest())
    return bytes(header) + b"".join(encoded for encoded, _ in blocks)


def available_location(data_dir):
    used = set()
    for entry in data_dir.glob("??-????????.data"):
        try:
            used.add((int(entry.name[:2]), int(entry.name[3:11])))
        except ValueError:
            continue
    for chunk in range(256):
        for archive in range(1, 0x100000000):
            if (chunk, archive) not in used:
                return chunk, archive
    die("no unused data-file location")


def config_values(filename):
    lines = filename.read_text(encoding="utf-8").splitlines(keepends=True)
    values = {}
    for line in lines:
        if " = " in line and not line.startswith("#"):
            key, value = line.rstrip("\n").split(" = ", 1)
            values[key] = value
    return lines, values


def replace_config(lines, key, value):
    prefix = key + " = "
    return [prefix + value + "\n" if line.startswith(prefix) else line for line in lines]


def path_entry(tvfs, pos):

    pre = post = 0
    name = b""
    value = None
    if tvfs[pos] == 0:
        pre, pos = 1, pos + 1
    if tvfs[pos] != 0xFF:
        length = tvfs[pos]
        name, pos = bytes(tvfs[pos + 1:pos + 1 + length]), pos + 1 + length
    if tvfs[pos] == 0:
        post, pos = 1, pos + 1
    if tvfs[pos] == 0xFF:
        value, pos = struct.unpack(">I", tvfs[pos + 1:pos + 5])[0], pos + 5
    else:
        post = 2
    return pos, pre, name, post, value


def walk_paths(tvfs, start, end, prefix=b"", parents=()):

    pos, base = start, prefix
    while pos < end:
        entry = pos
        pos, pre, name, post, value = path_entry(tvfs, pos)
        prefix = prefix + (b"/" if pre else b"") + name + (b"/" if post else b"")
        if value is None:
            continue
        if value & 0x80000000:
            folder_end = pos + (value & 0x7FFFFFFF) - 4
            yield entry, prefix, value, (pos, folder_end), parents
            yield from walk_paths(tvfs, pos, folder_end, prefix, parents + (entry,))
            pos = folder_end
        else:
            yield entry, prefix, value, None, parents
        prefix = base


def trie_bytes(names):

    out, depth = bytearray(), 0
    groups = {}
    for name in sorted(names):
        groups.setdefault(name[:1], []).append(name)
    for _, group in sorted(groups.items()):
        if len(group) == 1:
            name = group[0]
            out += bytes((len(name),)) + name + b"\xff" + struct.pack(">I", names[name])
            continue
        common = group[0]
        for name in group[1:]:
            while not name.startswith(common):
                common = common[:-1]
        if any(name == common for name in group):
            die("新增路径：一个文件名是另一个的前缀，前缀树编码不了: " + common.decode())
        sub, sub_depth = trie_bytes({name[len(common):]: names[name] for name in group})
        out += bytes((len(common),)) + common + b"\xff" + struct.pack(">I", 0x80000000 | (len(sub) + 4)) + sub
        depth = max(depth, sub_depth + 1)
    if any(len(name) > 0xFE for name in names):
        die("新增路径：名字片段超过 254 字节")
    return out, depth


def offset_field_size(size):
    return 1 if size <= 0xFF else 2 if size <= 0xFFFF else 3 if size <= 0xFFFFFF else 4


def add_paths(tvfs, additions):

    path_off, path_size, vfs_off, vfs_size, cft_off, cft_size = struct.unpack(">IIIIII", tvfs[12:36])
    est_off, est_size = struct.unpack(">II", tvfs[38:46])
    if not (path_off + path_size == est_off and est_off + est_size == cft_off and cft_off + cft_size == vfs_off and vfs_off + vfs_size == len(tvfs)):
        die("TVFS 各表不是首尾相接的（头 / 路径树 / 规格表 / 容器文件表 / 文件表），不认识这种布局")
    before = {path: value for _, path, value, folder, _ in walk_paths(tvfs, path_off, path_off + path_size) if folder is None}
    nodes = {path: (entry, folder, parents) for entry, path, value, folder, parents in walk_paths(tvfs, path_off, path_off + path_size) if folder}
    cft_field = offset_field_size(cft_size)
    if offset_field_size(cft_size + 25 * len(additions)) != cft_field:
        die("新增路径会让容器文件表偏移字段变宽，要重写全部文件表条目（没实现）")
    vfs_entry = 1 + 4 + 4 + cft_field
    new_cft, new_vfs = bytearray(), bytearray()
    groups = {}
    for number, (logical, (ekey, encoded, espec, content)) in enumerate(sorted(additions.items())):
        parts = logical.encode().rsplit(b"/", 2)
        if len(parts) != 3:
            die("新增路径须是「已有目录/新子目录/文件」: " + logical)
        parent, folder, name = parts[0] + b"/", parts[1], parts[2]
        groups.setdefault((parent, folder), {})[name] = vfs_size + number * vfs_entry
        new_cft += ekey + struct.pack(">I", encoded) + bytes((espec,)) + struct.pack(">I", content)
        new_vfs += b"\x01" + struct.pack(">II", 0, content) + (cft_size + number * 25).to_bytes(cft_field, "big")
    path = bytearray(tvfs[path_off:path_off + path_size])
    grown = {}
    inserts = []
    max_depth = struct.unpack(">H", tvfs[36:38])[0]
    for (parent, folder), names in sorted(groups.items()):
        if parent.lower() not in {key.lower() for key in nodes}:
            die("新增路径：父目录在路径树里不是一个目录项: " + parent.decode())
        entry, (start, end), parents = next(nodes[key] for key in nodes if key.lower() == parent.lower())
        if any(key.lower().startswith(parent.lower() + folder.lower()) for key in list(before) + list(nodes)):
            die("新增路径：子目录已存在或与已有名字同前缀: " + (parent + folder).decode())
        sub, depth = trie_bytes(names)
        block = bytes((len(folder),)) + folder + b"\x00\xff" + struct.pack(">I", 0x80000000 | (len(sub) + 4)) + sub
        at, pos, cont = end, start, False
        while pos < end:
            child = pos
            pos, pre, name, post, value = path_entry(tvfs, pos)
            first, was_cont, cont = (b"/" if pre else name[:1]), cont, value is None
            if was_cont:
                if value is not None and value & 0x80000000:
                    pos = pos + (value & 0x7FFFFFFF) - 4
                continue
            if first == folder[:1]:
                die("新增路径：新子目录首字母与已有子项冲突: " + (parent + folder).decode())
            if first > folder[:1] and at == end:
                at = child
            if value is not None and value & 0x80000000:
                pos = pos + (value & 0x7FFFFFFF) - 4
        inserts.append((at - path_off, block))
        for ancestor in parents + (entry,):
            grown[ancestor - path_off] = grown.get(ancestor - path_off, 0) + len(block)
        max_depth = max(max_depth, len(parents) + 2 + depth)
    for rel, delta in grown.items():
        value_at = path_entry(path, rel)[0] - 4
        value = struct.unpack(">I", path[value_at:value_at + 4])[0]
        path[value_at:value_at + 4] = struct.pack(">I", 0x80000000 | ((value & 0x7FFFFFFF) + delta))
    for rel, block in sorted(inserts, reverse=True):
        path[rel:rel] = block
    head = bytearray(tvfs[:path_off])
    est = tvfs[est_off:est_off + est_size]
    cft = tvfs[cft_off:cft_off + cft_size] + new_cft
    vfs = tvfs[vfs_off:vfs_off + vfs_size] + new_vfs
    n_path_off = path_off
    n_est_off = n_path_off + len(path)
    n_cft_off = n_est_off + len(est)
    n_vfs_off = n_cft_off + len(cft)
    head[12:36] = struct.pack(">IIIIII", n_path_off, len(path), n_vfs_off, len(vfs), n_cft_off, len(cft))
    head[36:38] = struct.pack(">H", max_depth)
    head[38:42] = struct.pack(">I", n_est_off)
    result = head + path + est + cft + vfs
    after = {p: v for _, p, v, folder, _ in walk_paths(result, n_path_off, n_path_off + len(path)) if folder is None}
    added = {p: v for p, v in after.items() if p not in before}
    if any(after.get(p) != v for p, v in before.items()):
        die("新增路径自检：原有路径或其文件表位置变了")
    if sorted(p.decode().lower() for p in added) != sorted(additions):
        die(f"新增路径自检：解析出来的新路径与要加的不一致（{len(added)} / {len(additions)}）")
    return result


def set_cft_sizes(tvfs, pos, encoded_size, content_size):




    est_size = struct.unpack(">I", tvfs[42:46])[0]
    idx = 1 if est_size <= 0xff else 2 if est_size <= 0xffff else 3 if est_size <= 0xffffff else 4
    tvfs[pos + 16:pos + 20] = struct.pack(">I", encoded_size)
    tvfs[pos + 20 + idx:pos + 24 + idx] = struct.pack(">I", content_size)


def cft_position(vfs_content, cft_offset, cft_end, manifest, logical):

    row = manifest.get(logical)
    if row is None:
        die("path not found in build_manifest.psv: " + logical)
    old_ekey = bytes.fromhex(row[6])
    matches = []
    search_at = cft_offset
    while True:
        position = vfs_content.find(old_ekey, search_at, cft_end)
        if position < 0:
            break
        matches.append(position)
        search_at = position + 1
    if len(matches) != 1:
        die(f"CFT key match count for {logical}: {len(matches)}")
    return matches[0]


def load_vfs1(source):
    data_dir = source / "data"
    lines, values = config_values(data_dir / ".build.config")
    vfs_key, vfs_ekey = values["vfs-1"].split()
    vfs_decoded_size, vfs_encoded_size = map(int, values["vfs-1-size"].split())
    vfs_content = bytearray(read_data_file(data_dir, bytes.fromhex(vfs_ekey), vfs_encoded_size))
    if len(vfs_content) != vfs_decoded_size or vfs_content[:4] != b"TVFS":
        die("vfs-1 is not a valid TVFS manifest")
    return vfs_content


def encode_by_espec(content, espec):


    return zlib.compress(content, 9) if espec == "z" else content if espec == "n" else blte_encode(content)


def ekey_for(encoded, chunk, archive, offset):
    header_size = struct.unpack(">I", encoded[4:8])[0] if encoded[:4] == b"BLTE" else len(encoded)
    return hashlib.md5(encoded[:header_size]).digest()[:9] + bytes((chunk, archive)) + offset.to_bytes(5, "big")


def write_pack(source, output, replacements, location):




    import concurrent.futures as cf
    chunk, archive = location
    data_dir = source / "data"
    name = f"{chunk:02d}-{archive:08d}.data"
    if (data_dir / name).exists():
        die(f"pack location {name} already exists in source RomFS")
    vfs_content = load_vfs1(source)
    manifest = read_manifest(source)
    cft_offset, cft_size = struct.unpack(">II", vfs_content[28:36])
    est_off, est_size = struct.unpack(">II", vfs_content[38:46])
    especs = bytes(vfs_content[est_off:est_off + est_size]).split(b"\0")
    items = []
    for logical, replacement in replacements.items():
        if logical.startswith("+"):
            die("pack 不支持新增路径: " + logical)
        pos = cft_position(vfs_content, cft_offset, cft_offset + cft_size, manifest, logical)
        items.append((logical, replacement, especs[vfs_content[pos + 20]].decode()))
    output.mkdir(parents=True, exist_ok=True)
    offset = 0
    with (output / name).open("wb") as data, (output / "pack_index.tsv").open("w", encoding="utf-8") as index, cf.ProcessPoolExecutor() as pool:
        index.write(f"# pack\t{name}\n")
        for (logical, _, _), (encoded, decoded_size) in zip(items, pool.map(_encode_one, [(str(r), e) for _, r, e in items], chunksize=16)):
            ekey = ekey_for(encoded, chunk, archive, offset)
            data.write(encoded)
            index.write(f"{logical}\t{ekey.hex()}\t{len(encoded)}\t{decoded_size}\n")
            offset += len(encoded)
    print(f"pack: {len(items)} file(s) → {name}, {offset} bytes")


def _encode_one(job):
    path, espec = job
    content = Path(path).read_bytes()
    return encode_by_espec(content, espec), len(content)


def read_prebuilt(filename):
    result = {}
    for line in Path(filename).read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        logical, ekey, encoded_size, decoded_size = line.split("\t")
        result[norm_path(logical)] = (bytes.fromhex(ekey), int(encoded_size), int(decoded_size))
    return result


def rewrite_tvfs(source, output, replacements, prebuilt=None, link_source=True):
    data_dir = source / "data"
    config = data_dir / ".build.config"
    lines, values = config_values(config)
    if "vfs-1" not in values or "vfs-1-size" not in values:
        die(f"missing vfs-1 in {config}")
    vfs_key, vfs_ekey = values["vfs-1"].split()
    vfs_decoded_size, vfs_encoded_size = map(int, values["vfs-1-size"].split())
    vfs_content = bytearray(read_data_file(data_dir, bytes.fromhex(vfs_ekey), vfs_encoded_size))
    if len(vfs_content) != vfs_decoded_size or vfs_content[:4] != b"TVFS":
        die("vfs-1 is not a valid TVFS manifest")

    manifest = read_manifest(source)
    cft_offset, cft_size = struct.unpack(">II", vfs_content[28:36])
    cft_end = cft_offset + cft_size
    if cft_end > len(vfs_content):
        die("unsupported TVFS CFT bounds")

    updates = []
    adds = {k[1:]: v for k, v in replacements.items() if k.startswith("+")}
    replacements = {k: v for k, v in replacements.items() if not k.startswith("+")}
    for logical, replacement in replacements.items():
        updates.append((cft_position(vfs_content, cft_offset, cft_end, manifest, logical), replacement.read_bytes()))
    prebuilt_pos = [(cft_position(vfs_content, cft_offset, cft_end, manifest, logical), entry) for logical, entry in (prebuilt or {}).items()]

    new_chunk, new_archive = available_location(data_dir)
    new_data = bytearray()

    def append(content, espec="b:256K*=z"):
        encoded = encode_by_espec(content, espec)
        offset = len(new_data)
        new_data.extend(encoded)
        return ekey_for(encoded, new_chunk, new_archive, offset), len(content), len(encoded)

    est_off, est_size = struct.unpack(">II", vfs_content[38:46])
    especs = bytes(vfs_content[est_off:est_off + est_size]).split(b"\0")
    for pos, content in updates:
        espec = especs[vfs_content[pos + 20]].decode()
        ekey, decoded_size, encoded_size = append(content, espec)
        vfs_content[pos:pos + 9] = ekey[:9]
        vfs_content[pos + 9:pos + 16] = ekey[9:]
        set_cft_sizes(vfs_content, pos, encoded_size, decoded_size)
    for pos, (ekey, encoded_size, decoded_size) in prebuilt_pos:
        vfs_content[pos:pos + 16] = ekey
        set_cft_sizes(vfs_content, pos, encoded_size, decoded_size)

    if adds:
        espec_index = next((i for i, e in enumerate(especs) if e == b"z"), 0)
        encoded_adds = {}
        for logical, replacement in adds.items():
            if logical in manifest:
                die("新增路径已存在（应作替换）: " + logical)
            ekey, decoded_size, encoded_size = append(replacement.read_bytes(), especs[espec_index].decode())
            encoded_adds[logical] = (ekey, encoded_size, espec_index, decoded_size)
        vfs_content = add_paths(vfs_content, encoded_adds)
    new_vfs_ekey, new_vfs_decoded, new_vfs_encoded = append(bytes(vfs_content))




    root_ckey, root_ekey = values["vfs-root"].split()
    root_decoded_size, root_encoded_size = map(int, values["vfs-root-size"].split())
    root = bytearray(read_data_file(data_dir, bytes.fromhex(root_ekey), root_encoded_size))
    if len(root) != root_decoded_size or root[:4] != b"TVFS":
        die("vfs-root is not a valid TVFS manifest")
    old_vfs = bytes.fromhex(vfs_ekey)
    hits = [i for i in range(len(root) - 15) if root[i:i + 16] == old_vfs]
    if len(hits) != 1:
        die(f"vfs-root 里 vfs-1 键命中 {len(hits)} 次（期望 1）")
    pos = hits[0]
    root[pos:pos + 16] = new_vfs_ekey
    set_cft_sizes(root, pos, new_vfs_encoded, new_vfs_decoded)
    new_root_ekey, new_root_decoded, new_root_encoded = append(bytes(root))
    data_name = f"{new_chunk:02d}-{new_archive:08d}.data"
    (output / "data" / data_name).write_bytes(new_data)
    new_config = replace_config(lines, "vfs-1", hashlib.md5(bytes(vfs_content)).hexdigest() + " " + new_vfs_ekey.hex())
    new_config = replace_config(new_config, "vfs-1-size", f"{new_vfs_decoded} {new_vfs_encoded}")
    root_value = hashlib.md5(bytes(root)).hexdigest() + " " + new_root_ekey.hex()
    for key in ("vfs-root", "vfs-2"):
        if values.get(key) == values["vfs-root"]:
            new_config = replace_config(new_config, key, root_value)
            new_config = replace_config(new_config, key + "-size", f"{new_root_decoded} {new_root_encoded}")
    (output / "data" / ".build.config").write_text("".join(new_config), encoding="utf-8")
    if link_source:
        for entry in data_dir.iterdir():
            if entry.name not in {".build.config", data_name}:
                (output / "data" / entry.name).symlink_to(entry.resolve())
    print(f"overlay: {len(updates)} replacement(s), {len(adds)} addition(s), {len(prebuilt_pos)} prebuilt, TVFS rewritten, data={data_name}")


def make_overlay(source, output, replacements, prebuilt=None, link_source=True):
    if output.exists():
        die(f"output already exists: {output}")


    if link_source: link_tree(source, output, {"data"})
    else: output.mkdir(parents=True)
    (output / "data").mkdir()
    if not replacements and not prebuilt:
        if link_source:
            for entry in (source / "data").iterdir():
                (output / "data" / entry.name).symlink_to(entry.resolve())
        else:
            (output / "data" / ".build.config").write_bytes((source / "data" / ".build.config").read_bytes())
        print("empty replacement table: linked base" if link_source else "empty replacement table: thin base config")
        return
    rewrite_tvfs(source, output, replacements, prebuilt, link_source)


def main(argv):
    if len(argv) == 6 and argv[1] == "pack":
        chunk, archive = (int(x) for x in argv[5].split("-"))
        write_pack(Path(argv[2]).resolve(), Path(argv[3]).resolve(), read_replacements(argv[4]), (chunk, archive))
        return
    link_source = "--thin" not in argv
    argv = [a for a in argv if a != "--thin"]
    prebuilt = None
    if len(argv) == 6 and argv[4] == "--prebuilt":
        prebuilt = read_prebuilt(argv[5]); argv = argv[:4]
    if len(argv) != 4:
        die("usage: casc_overlay.py <RomFS> <output-dir> <replacements.tsv> [--prebuilt pack_index.tsv] [--thin] | pack <RomFS> <out> <replacements.tsv> <chunk-archive>")
    make_overlay(Path(argv[1]).resolve(), Path(argv[2]).resolve(), read_replacements(argv[3]), prebuilt, link_source)


if __name__ == "__main__":
    main(sys.argv)
