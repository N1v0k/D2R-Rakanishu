#!/bin/bash










set -u; MIN=$1; OUTROOT=$2
HERE=$(cd "$(dirname "$0")" && pwd); ROOT=${DM_ROOT:-$HOME/dm-work}
V=$(sed -n 's/.*DMSH_CHAIN "\(.*\)".*/\1/p' "$HERE/../nvn/dmsh_chain.h"); S=$ROOT/build/cache/metal_shader_cache/b-$V; CAS=$ROOT/build/cache/metal_shader_cache/cas/variant
[ -d "$S" ] || { echo "🔴 没有着色器库 ${S}（先跑 dmsh_build.py）"; exit 2; }
D=$OUTROOT/b-$V; rm -rf "$D"; mkdir -p "$D"




rsync -a --include='*.json' --exclude='*' "$S/" "$D/" || { echo "🔴 拷反射 json 失败"; exit 3; }
one() { src=$1; dst=$2; t=$(mktemp -d); if xcrun -sdk macosx metal -c -mmacosx-version-min="$MIN" "$src" -o "$t/a.air" 2>"$t/err" && xcrun -sdk macosx metallib "$t/a.air" -o "$t/a.metallib" 2>>"$t/err"; then mv "$t/a.metallib" "$dst"; else echo "失败 $(basename "$dst")：$(head -1 "$t/err" | cut -c1-120)"; fi; rm -rf "$t"; }
export -f one; export MIN D S
BAD=$(ls "$S" | grep '_b\.metal$' | sed 's/\.metal$//' | xargs -P 10 -I{} bash -c 'one "$S/{}.metal" "$D/{}.metallib"')
T=$(mktemp); for r in "$CAS"/*/; do [ -f "$r/mac.metallib" ] && [ -f "$r/metal" ] && echo "$(shasum -a 256 "$r/mac.metallib" | cut -c1-64) $r"; done > "$T"
miss=0
for f in "$S"/*_bf.metallib; do [ -f "$f" ] || continue; h=$(shasum -a 256 "$f" | cut -c1-64); r=$(grep -m1 "^$h " "$T" | cut -d' ' -f2-)
  if [ -n "$r" ]; then m=$D/$(basename "${f%.metallib}").metal; cp "$r/metal" "$m"; BAD="$BAD$(one "$m" "$D/$(basename "$f")")"; rm -f "$m"; else miss=$((miss+1)); fi; done
rm -f "$T"
have=$(ls "$D" | grep -c '\.metallib$'); want=$(ls "$S" | grep -c '\.metallib$')
nojson=$(ls "$D" | sed -n 's/_b\.metallib$//p' | while read -r k; do [ -f "$D/${k}_b.json" ] || echo "$k"; done | wc -l | tr -d ' ')
[ -z "$BAD" ] && [ "$miss" = 0 ] && [ "$have" = "$want" ] && [ "$nojson" = 0 ] || { echo "🔴 着色器库重编不全：产出 ${have} / ${want}，_bf 源码没对上 ${miss} 个，缺反射 json ${nojson} 个；${BAD:0:300}"; exit 3; }
echo "MSL_RETARGET ${D}（${have} 个库，目标 macOS ${MIN}）"
