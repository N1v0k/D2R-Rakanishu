#!/bin/bash












unset LANG LC_CTYPE; export LC_ALL=C
TAG=$1; CAP=$2; HERE=$(cd "$(dirname "$0")" && pwd)
O=${UIREG_OUT:-${DM_ROOT:-$HOME/dm-work}/run/uireg}/pw/$TAG; mkdir -p "$O"; cat > "$O/in.txt"
[ "${UIREG_MODE:-iphone}" = iphone ] || { echo "计时：真机只有 iphone 档，没跑" > "$O/timing.txt"; exit 2; }
[ -z "${PW_SAVE:-}" ] || { echo "计时：真机上没有存档 ${PW_SAVE}，没跑" > "$O/timing.txt"; exit 2; }
rm -f "$O/boot.txt" "$O/boot_full.txt" "$O/c2s.jsonl" "$O/e2e_c2s.jsonl"




J=$(python3 -c "
import json,sys
d={'DM_TOUCH_SCRIPT_START':'menu','DM_TOUCH_REF':'667x375','DM_FE_AUTO':'1'}
for kv in sys.argv[1].split():
    k,v=kv.split('=',1)
    if k not in ('DM_C2S_LOG','DM_SAVE_FRESH'): d[k]=v
print(json.dumps(d))" "${PW_ENV:-}")
t0=$(date +%s)
E2E_KEYS='B@OpenPanel:NoSuchPanelEver' E2E_ENV="$J" E2E_PULL="boot.txt c2s.jsonl ${UIREG_PULL:-}" bash "$HERE/../touch_e2e.sh" "$O/in.txt" "$O" "$CAP" > "$O/e2e.log" 2>&1
[ -f "$O/e2e_c2s.jsonl" ] && cp "$O/e2e_c2s.jsonl" "$O/c2s.jsonl"







EXP=$(git -C "$HERE" log -1 --format=%ct ${UIREG_EXPECT_COMMIT:-} 2>/dev/null); GOT=$(LC_ALL=C grep -a -m1 -o 'boot: 版本 [^ ]* 构建 [0-9]*' "$O/boot.txt" 2>/dev/null | awk '{print $NF}')
if [ "${UIREG_BUILD_CHECK:-1}" = 1 ] && [ -f "$O/boot.txt" ] && [ "$GOT" != "$EXP" ]; then
  mv "$O/boot.txt" "$O/boot_wrongpkg.txt"; echo "[dm] 【红】包不对，整局作废：设备上这一局的构建号 ${GOT:-读不到}，期望 ${EXP:-读不到}（提交 $(git -C "$HERE" log -1 --format=%h ${UIREG_EXPECT_COMMIT:-} 2>/dev/null | cut -c1-9) 的提交时间戳）；原日志在 boot_wrongpkg.txt" > "$O/boot.txt"
fi


if LC_ALL=C grep -a -q '\[script\] note 结束' "$O/boot.txt" 2>/dev/null; then mv "$O/boot.txt" "$O/boot_full.txt"; LC_ALL=C awk '{ print } /\[script\] note 结束/ { exit }' "$O/boot_full.txt" > "$O/boot.txt"; fi
echo "计时：真机整局墙钟 $(( $(date +%s) - t0 )) 秒（含排锁、推脚本、拉日志）" > "$O/timing.txt"
