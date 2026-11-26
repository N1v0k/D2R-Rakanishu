#!/bin/bash











set -u
P=$(head -1 "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/PRODUCT") || exit 2
[ -n "$P" ] || { echo "dm/PRODUCT 为空" >&2; exit 2; }
case "${1:-}" in
  name) echo "$P-${2:?种类}";;
  path) echo "${DM_ROOT:-$HOME/dm-work}/build/cache/dm/out/$P-${2:?种类}";;
  rev)  echo "${P##*-}";;
  game) echo "${P%%-*}";;
  build)
    HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd); PY=${DM_PY:-${DM_ROOT:-$HOME/dm-work}/build/cache/dm/venv/bin/python3}; NSO=${DM_NSO_DIR:-${DM_ROOT:-$HOME/dm-work}/data/switch/${P%%-*}/Update/ExeFS}
    for k in keypage rules touch; do O=${DM_ROOT:-$HOME/dm-work}/build/cache/dm/out/$P-$k; mkdir -p "$O"
      case $k in keypage) E=(DM_RULESET=native);; rules) E=(DM_RULESET=touch-run-v4 DM_PRIVATE_GATES=0);; touch) E=(DM_RULESET=touch-run-v4);; esac
      for m in main sdk subsdk0; do env "${E[@]}" "$PY" "$HERE/patch_nso.py" "$NSO/$m" "$O" "$m" > "$O/$m.patch.log" 2>&1 || { echo "🔴 $k $m 改写失败：$(tail -2 "$O/$m.patch.log")" >&2; exit 1; }; done
      echo "$P-$k 已出：$(ls "$O" | wc -l | tr -d ' ') 个文件"; done;;
  *) echo "用法：product.sh name|path <种类> | rev | game | build" >&2; exit 2;;
esac
