#!/bin/bash














set -u
HERE=$(cd "$(dirname "$0")" && pwd)
PY=${DM_PY:-${DM_ROOT:-$HOME/dm-work}/build/cache/dm/venv/bin/python}
NSO_DIR=${DM_NSO_DIR:-${DM_ROOT:-$HOME/dm-work}/data/switch/$(sed -n '1s/-.*//p' "$HERE/../PRODUCT")/Update/ExeFS}
RUN=${DM_ROOT:-$HOME/dm-work}/run/verify/dm
FRESH=0; STRUCT_ONLY=0
[ "${1:-}" = --fresh ] && { FRESH=1; shift; }
[ "${1:-}" = --struct-only ] && { STRUCT_ONLY=1; shift; }
OUT=${1:-$(bash "$(dirname "$0")/../tools/product.sh" path keypage)}; [ $# -gt 0 ] && shift
MODS=${*:-main sdk subsdk0}
mkdir -p "$RUN"
TS=$(date +%m%d_%H%M%S)_$$
LOG=$RUN/regress_$TS.log
echo "# regress $TS out=$OUT mods=$MODS fresh=$FRESH struct_only=$STRUCT_ONLY seed=${SEED:-1} git=$(git -C "$HERE" rev-parse --short HEAD) $(git -C "$HERE" status --porcelain -- . ../tools | wc -l | tr -d ' ')处未提交" | tee "$LOG"

if [ $FRESH = 1 ]; then
  F=$RUN/fresh; rm -rf "$F"; mkdir -p "$F"
  for m in $MODS; do
    "$PY" "$HERE/../tools/patch_nso.py" "$NSO_DIR/$m" "$F" "$m" >> "$LOG" 2>&1 || { echo "FAIL patch_nso $m" | tee -a "$LOG"; exit 1; }
    for e in patched.bin fixups.tsv gates.tsv sites.tsv; do
      if [ ! -e "$OUT/$m.$e" ]; then r=缺; elif cmp -s "$F/$m.$e" "$OUT/$m.$e"; then r=同; else r=不同; fi
      echo "FRESH $m.$e 与 $OUT $r" | tee -a "$LOG"
    done
  done
  OUT=$F
fi

rc=0
for m in $MODS; do
  "$PY" "$HERE/structck.py" "$OUT" "$m" "$NSO_DIR/$m" >> "$LOG" 2>&1 || rc=1
  [ $STRUCT_ONLY = 1 ] || "$PY" "$HERE/equiv.py" "$OUT" "$m" "$NSO_DIR/$m" --seed "${SEED:-1}" ${JOBS:+--jobs $JOBS} --tsv "$RUN/eq_${m}_$TS.tsv" >> "$LOG" 2>&1 || rc=1
done


GT=$RUN/gate_test_$TS
if cc -O1 -o "$GT" "$HERE/gatefn/gate_test.c" "$HERE/gatefn/gate_tramp.S" "$HERE/../host/entry.S" >> "$LOG" 2>&1; then
  G=(); for m in $MODS; do [ -s "$OUT/$m.gates.tsv" ] && G+=("$OUT/$m.gates.tsv"); done
  "$GT" "${G[@]}" >> "$LOG" 2>&1 || rc=1; rm -f "$GT"
else echo "FAIL 门功能测试台编译失败" >> "$LOG"; rc=1; fi
echo "---- 汇总（${LOG}）"
grep -E '^(FRESH|STRUCT|EQUIV|GATEFN|FAIL|DIFF|NOEFF|MUTMISS|BELOWSP)' "$LOG" | grep -v '^FRESH.* 同$'
[ $rc = 0 ] && echo "REGRESS OK $(basename "$OUT")" || echo "REGRESS FAIL $(basename "$OUT")"
exit $rc
