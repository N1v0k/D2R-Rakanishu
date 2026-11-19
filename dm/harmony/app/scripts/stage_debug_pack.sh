#!/usr/bin/env bash









set -euo pipefail
src=${1:?usage: stage_debug_pack.sh <pack dir>}
here=$(cd "$(dirname "$0")/.." && pwd)
dst="$here/entry/src/debugpack/resources/resfile/dm"
for d in out shaders overlay ui-art; do [ -d "$src/$d" ] || { echo "missing $src/$d" >&2; exit 1; }; done
rm -rf "$dst"; mkdir -p "$dst"


(cd "$src" && find out shaders overlay ui-art -type f | LC_ALL=C sort | xargs -d '\n' sha256sum | sha256sum | cut -c1-16) > "$dst/pack-version.txt"
python -I - "$src" "$dst/dm-pack.zip" <<'PY'
import os, sys, zipfile
src, out = sys.argv[1], sys.argv[2]
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    for d in ('out', 'shaders', 'overlay', 'ui-art'):
        for root, _, files in os.walk(os.path.join(src, d)):
            for f in sorted(files):
                p = os.path.join(root, f)
                z.write(p, '/'.join(os.path.relpath(p, src).split(os.sep)))
PY
echo "staged $(du -h "$dst/dm-pack.zip" | cut -f1) dm-pack.zip, version $(cat "$dst/pack-version.txt")"
