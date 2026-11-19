#!/usr/bin/env python3






import hashlib, json, os, sys
src, out = os.path.realpath(os.path.join(sys.argv[1], "data")), sys.argv[2]
os.makedirs(out, exist_ok=True)
cache_p = os.path.join(os.path.dirname(os.path.abspath(out)), ".asset_sha.json")
try: cache = json.load(open(cache_p))
except Exception: cache = {}
files, total = [], 0
for nm in sorted(os.listdir(src)):
    p = os.path.join(src, nm)
    if nm.startswith(".") or not os.path.isfile(p): continue
    st = os.stat(p); key = f"{p}|{st.st_size}|{st.st_mtime_ns}"
    if key not in cache:
        h = hashlib.sha256()
        with open(p, "rb") as f:
            while b := f.read(1 << 22): h.update(b)
        cache[key] = h.hexdigest()
    files.append({"name": nm, "size": st.st_size, "sha256": cache[key]}); total += st.st_size
json.dump({k: v for k, v in cache.items() if k.split("|")[0].startswith(src)}, open(cache_p, "w"))
json.dump({"version": 1, "total": total, "files": files}, open(os.path.join(out, "manifest.json"), "w"), indent=0)
link = os.path.join(out, "data")
if os.path.islink(link): os.unlink(link)
os.symlink(src, link)
print(f"清单 {len(files)} 个文件共 {total} 字节 → {out}/manifest.json；data → {src}")
