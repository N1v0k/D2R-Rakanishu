#!/bin/bash








set -u
BIN=$1; OUT=$2; N=$3; PNG=$4; TO=${5:-600}
SV=${DM_GATE_SAVE:-${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref_warlock/0000000000000001}
echo "SAVE_SHA $("$(dirname "$0")/savesha.sh" "$SV") $SV"
W=$(mktemp -d "${PNG%.png}.XXXX"); cp -R "$SV/0" "$W/save" && chmod -R u+w "$W/save" || { echo "拷存档失败" >&2; exit 2; }
case "$N" in *+*) SHOTENV="DM_NVN_SHOT_UI=$N:$PNG" ;; *) SHOTENV="DM_NVN_SHOT=$N:$PNG" ;; esac
mkdir -p "$W/temp"
${HOSTSHOT_WRAP:-} env DM_SAVE="$W/save" DM_TEMP="$W/temp" "$SHOTENV" DM_WATCH_STALL=15 DM_WATCH=$((TO * 5 / 4)) "$BIN" "$OUT" main sdk > "$W/host.log" 2>&1 & pid=$!
printf '%s\n' "$pid" > "${PNG%.png}.pid"
s=$SECONDS


until grep -q "→ $PNG 成功" "$W/host.log" 2>/dev/null; do
  grep -q "→ $PNG 失败" "$W/host.log" 2>/dev/null && { kill $pid; echo "截图写盘失败" >&2; rm -rf "$W"; exit 1; }
  grep -q "读取画面未撤（等待" "$W/host.log" 2>/dev/null && { kill $pid; wait $pid 2>/dev/null; cp "$W/host.log" "${PNG%.png}.log"; echo "读取画面未撤（等待 15 秒）" >&2; rm -rf "$W"; exit 1; }
  grep -q "坐标核对不一致" "$W/host.log" 2>/dev/null && { kill $pid; wait $pid 2>/dev/null; cp "$W/host.log" "${PNG%.png}.log"; echo "站位核对不一致，宿主已撤掉截图" >&2; rm -rf "$W"; exit 1; }
  kill -0 $pid 2>/dev/null || { echo "宿主在第 $N 帧前退出（日志 ${PNG%.png}.log）" >&2; cp "$W/host.log" "${PNG%.png}.log"; tail -3 "$W/host.log" >&2; rm -rf "$W"; exit 1; }
  [ $((SECONDS - s)) -gt "$TO" ] && { kill $pid; echo "$TO 秒未到第 $N 帧" >&2; grep '【红】看门狗' "$W/host.log" | tail -1 >&2 || true; rm -rf "$W"; exit 1; }
  sleep 1
done
kill $pid 2>/dev/null; wait $pid 2>/dev/null; cp "$W/host.log" "${PNG%.png}.log"; rm -rf "$W"
[ -s "$PNG" ] || { echo "日志说截到但 $PNG 不存在" >&2; exit 1; }
echo "截到 ${PNG}（$((SECONDS - s)) 秒）"
