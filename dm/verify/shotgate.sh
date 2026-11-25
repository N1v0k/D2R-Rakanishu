#!/bin/bash










set -u
HERE=$(cd "$(dirname "$0")" && pwd); PY=${DM_PY:-${DM_ROOT:-$HOME/dm-work}/build/cache/dm/venv/bin/python}
OUT=${1:-$(bash "$(dirname "$0")/../tools/product.sh" path keypage)}; [ $# -gt 0 ] && shift
PAGES=${*:-$(cd "$HERE/shotgate_pages" && ls *.json | sed 's/\.json$//')}
BIN=${DM_HOST:-$HERE/../host/dmhost}
SV=${DM_GATE_SAVE:-${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref_warlock/0000000000000001}; HSHA=$("$HERE/savesha.sh" "$SV")
TREE=$(basename "$(git -C "$HERE" rev-parse --show-toplevel 2>/dev/null || printf '%s' "$HERE")")
mkdir -p "${DM_ROOT:-$HOME/dm-work}/run/verify/dm/shotgate"
R=$(mktemp -d "${DM_ROOT:-$HOME/dm-work}/run/verify/dm/shotgate/$(date +%m%d_%H%M%S)_${TREE}_$$.XXXXXX") || { echo "🔴 无法创建 shotgate 结果目录"; exit 1; }
touch "$R/index.start"
make -s -C "$HERE/../host" || { echo "🔴 宿主编译失败"; exit 1; }


ROUTE=${DM_NVN_ROUTE:-B}; [ -n "${DM_NVN_B:-}" ] && [ "${DM_NVN_B}" != all ] && ROUTE="A/B 混（DM_NVN_B=${DM_NVN_B}）"
echo "着色器路线 ${ROUTE}；产物 $(basename "$OUT")；宿主 $BIN"
rc=0; rows=""
page() {
  local p=$1 S=$HERE/shotgate_pages/$1.json T TRIG KEYS TSHA KE HL ERRS line prc=0
  read -r T TRIG KEYS < <($PY -c "import json,os; d=json.load(open('$S')); h=d['host']; print(os.path.expanduser(d['truth']), h['trigger'], h.get('keys') or '-')")
  local -a HE=(); while IFS= read -r kv; do [ -n "$kv" ] && HE+=("$kv"); done < <($PY -c "import json,os; [print(f'{k}={os.path.expanduser(v)}') for k,v in json.load(open('$S'))['host'].get('env',{}).items()]")
  [ -f "$T" ] || { printf '1\n🔴 %s 缺真值参照图 %s\n' "$p" "$T" > "$R/$p.res"; return; }
  TSHA=$($PY -c "import json; print(json.load(open('${T%.png}.meta.json'))['save'][0])" 2>/dev/null)
  [ "$TSHA" = "$HSHA" ] || { printf '1\n🔴 %s 存档指纹不符：真值 %s / 宿主 %s\n' "$p" "${TSHA:0:12}" "${HSHA:0:12}" > "$R/$p.res"; return; }
  if [ "$KEYS" = - ]; then KE=(); else KE=(DM_KEYS="$KEYS"); fi


  env DM_BRIGHT_K=1 ${KE[@]+"${KE[@]}"} ${HE[@]+"${HE[@]}"} DM_GATE_SAVE="$SV" "$HERE/hostshot.sh" "$BIN" "$OUT" "$TRIG" "$R/${p}_host.png" > "$R/${p}_shot.log" 2>&1 || {


       PR=$(grep -h '【红】DM_POSCHK' "$R/${p}_host.log" 2>/dev/null | head -1)
       [ -n "$PR" ] && { printf '1\n🔴 %s 站位不一致：%s\n' "$p" "${PR#*【红】}" > "$R/$p.res"; return; }
       printf '1\n🔴 %s 宿主截图失败：%s（现场保留）\n' "$p" "$(tail -1 "$R/${p}_shot.log")" > "$R/$p.res"
       echo "$p 宿主截图失败 / 看门狗：本目录是现场，不许删、不许重跑覆盖" >> "$R/KEEP"
       pid=$(cat "$R/${p}_host.png.pid" 2>/dev/null)
       if [ -n "$pid" ]; then
         find ~/Library/Logs/DiagnosticReports -name 'dmhost-*' -newer "$R/index.start" -exec sh -c 'grep -qE "(^|[^0-9])PID[^0-9]*$1([^0-9]|$)|dmhost[^0-9]*\[$1\]" "$2" 2>/dev/null && cp "$2" "$3/"' sh "$pid" {} "$R" \; 2>/dev/null
       fi
       return; }


  HL=$R/${p}_host.log; ERRS=$("$HERE/logred.sh" "$HL" | cut -f1)
  line=$($PY "$HERE/shotgate.py" "$S" "$T" "$R/${p}_host.png" "$R/$p") || prc=1


  local miss=$($PY -c "import json,sys; L=open('$HL',errors='replace').read(); [print(x) for x in json.load(open('$S'))['host'].get('require_log',[]) if x not in L]")


  grep -qE 'SHOT 前置：(读取画面已撤|本局无读取画面|不适用)' "$HL" || miss="${miss}${miss:+$'\n'}SHOT 前置结论行（宿主未写）"
  [ -n "$miss" ] && { prc=1; line="${line/ OK / RED（宿主日志缺：$(echo "$miss" | head -1 | cut -c1-80)）}"; }
  if [ "$ERRS" -gt 0 ]; then prc=1; line="${line/ OK / RED（宿主日志 $ERRS 行错误：$("$HERE/logred.sh" "$HL" | cut -f2 | cut -c1-120)）}"; line="${line/ RED / RED（宿主日志 $ERRS 行错误）}"; fi
  printf '%s\n%s\n' "$prc" "$line" > "$R/$p.res"
}




mkfifo "$R/sem" && exec 3<>"$R/sem" && rm "$R/sem"
for i in $(seq 1 ${SHOTGATE_JOBS:-6}); do echo >&3; done
for p in $PAGES; do read -u 3; { page "$p"; echo >&3; } & done; wait; exec 3>&-
for p in $PAGES; do
  { read -r prc; read -r line; } < "$R/$p.res"; [ "$prc" = 0 ] || rc=1
  echo "$line"; rows="$rows<li><a href=\"$p/report.html\">$p</a>：${line#SHOTGATE }</li>"
done
echo "<!doctype html><meta charset=utf-8><title>画面闸门</title><h1>画面闸门 $(date '+%m-%d %H:%M')</h1><p>产物 ${OUT}；宿主 ${BIN}；着色器路线 ${ROUTE}；存档指纹 ${HSHA:0:12}；每页宿主完整日志 *_host.log</p><ul>${rows}</ul>" > "$R/index.html"
[ -f "$R/KEEP" ] && echo "⚠️ 宿主截图失败 / 看门狗：现场 $R 已标 KEEP（含崩溃报告副本），保留，勿重跑抹掉"






ROOT=$(dirname "$R"); REP="$R/index.html"
if [ $rc = 0 ] && [ ! -f "$R/KEEP" ] && [ "${SHOTGATE_KEEP_IMG:-0}" != 1 ] && [ "$(basename "$ROOT")" = shotgate ] && [ -n "$(basename "$R")" ]; then
  mkdir -p "$ROOT/green"; REP="$ROOT/green/$(basename "$R").html"
  { sed -e 's#<a href="[^"]*">##g' -e 's#</a>##g' "$R/index.html"; echo "<p>全绿：现场目录已删，只留这份小结（SHOTGATE_KEEP_IMG=1 可保留整份现场）</p>"; } > "$REP" && rm -rf -- "$R"
fi
bash "$HERE/shotgate_gc.sh" "$ROOT"
echo "SHOTGATE_ALL $([ $rc = 0 ] && echo OK || echo RED) → $REP"; exit $rc
