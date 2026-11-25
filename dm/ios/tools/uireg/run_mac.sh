#!/bin/bash










set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../local_env.sh"
B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
TAG=$1; CAP=$2; HERE=$(cd "$(dirname "$0")" && pwd)
O=${UIREG_OUT:-${DM_ROOT:-$HOME/dm-work}/run/uireg}/pw/$TAG; mkdir -p "$O"; cat > "$O/script.txt"; : > "$O/launch.log"
fail(){ echo "$*" | tee "$O/fail.txt"; echo "计时：${1}" > "$O/timing.txt"; exit 2; }
case ${UIREG_MODE:-iphone} in
  iphone) W=${UIREG_APP:-$(git -C "$HERE" rev-parse --show-toplevel 2>/dev/null)/.iosdd_mac/DmMacSmoke.app};;
  *) W=${UIREG_APP_IPAD:-};;
esac
[ -x "$W/Wrapper/DmIOS.app/DmIOS" ] || fail "没有包（画面档 ${UIREG_MODE:-iphone}）：${W:-未设 UIREG_APP_IPAD}"
C=""; for m in "$HOME"/Library/Containers/*/.com.apple.containermanagerd.metadata.plist; do
  [ "$(/usr/libexec/PlistBuddy -c 'Print :MCMMetadataIdentifier' "$m" 2>/dev/null)" = "$B" ] && C="$(dirname "$m")/Data"; done
D="$C/Documents"; [ -n "$C" ] && [ -d "$D" ] || fail "本机没有 $B 的容器（先跑一次 smoke_mac.sh 1）"


T_B=$(date +%s); export DM_DEVLOCK_DIR=${DM_ROOT:-$HOME/dm-work}/run/ios/macsmoke.lock; source "$HERE/../devlock.sh"; devlock_acquire "${SMOKE_OWNER:-未署名} 界面回归 $TAG"; T_C=$(date +%s)




pids() { local p b; for p in $(ps -axo pid=,comm= | awk '{ pid=$1; $1=""; sub(/^ /,""); if ($0 ~ /\/Wrapper\/DmIOS\.app\/DmIOS$/) print pid }'); do
  b=$(lsappinfo info -only bundleID -app "$p" 2>/dev/null | sed -n 's/.*="\(.*\)"$/\1/p'); [ -z "$b" ] || [ "$b" = "$B" ] && echo "$p"; done; }
stop_all(){ local p; for p in $(pids); do kill -TERM "$p" 2>/dev/null; done; sleep 1; for p in $(pids); do kill -KILL "$p" 2>/dev/null; done; }
pre=$(pids | tr '\n' ' '); [ -z "$pre" ] || { devlock_release; fail "本机已有 $B 的实例在跑（pid ${pre}，非本次启动）"; }








SV="$D/save.test"; SWAP=""; BAD=""; restore(){ [ -n "$SWAP" ] || return 0; chmod -R u+w "$SV" 2>/dev/null; rm -rf "$C/save.uireg_ran" "$SV.work"
  if mv "$SV" "$C/save.uireg_ran" && mv "$SWAP" "$SV"; then SWAP=""; else BAD=1; echo "【红】换存档后挪不回：原测试存档在 ${SWAP}，Documents/save.test 不是原来那份，手工核对后挪回" | tee "$O/fail.txt"; [ -f "$O/timing.txt" ] && sed -i '' '1s/$/ 【红】换存档后挪不回，见 fail.txt/' "$O/timing.txt"; fi; }
trap 'stop_all; restore; savekey_out "$C"; devlock_release; [ -z "$BAD" ] || exit 4' EXIT; trap 'exit 143' INT TERM


source "$HERE/../launch_guard.sh"; launch_guard_pre "$C"; source "$HERE/../save_key.sh"; [ -n "${PW_SAVE:-}" ] || savekey_in "$C" "${PW_ENV:-}" || fail "存档钥匙不许起这一局（见上一行红字）"
if [ -n "${PW_SAVE:-}" ]; then
  [ -d "$C/$PW_SAVE" ] || fail "容器 Data 下没有存档 ${PW_SAVE}（d 局专用，备法见usage.md）"




  S1="$C/save.uireg_swap_$$"; [ -d "$SV" ] || mkdir -p "$SV"; mv "$SV" "$S1" || fail "换存档失败：原测试存档挪不走"; SWAP="$S1"; rm -rf "$SV.work"
  cp -R "$C/$PW_SAVE" "$SV" && chmod -R u+w "$SV" || fail "换存档失败：副本拷不进去"
fi
caffeinate -u -t 3 >/dev/null 2>&1
RID=u$(date +%H%M%S); awk -v r="$RID" '$2=="shot"{$3=r"_"$3} {print}' "$O/script.txt" > "$D/touch_script.txt"
EX=(); for kv in ${PW_ENV:-}; do EX+=(--env "$kv"); done; [ -z "${PW_SAVE:-}" ] || EX+=(--env DM_SAVE_TEST=1)


source "$HERE/../cloud_guard.sh"; cloud_guard_env "$D" "${PW_ENV:-}" || fail "总闸不许起这一局（见上一行红字）"; [ -z "$CG_ENV" ] || EX+=(--env "$CG_ENV")




launch(){ mv "$D/boot.txt" "$D/boot_prev.txt" 2>/dev/null
  open -n "$W" ${EX[@]+"${EX[@]}"} --env DM_TOUCH_SCRIPT=touch_script.txt --env DM_TOUCH_SCRIPT_START=menu --env DM_UIMSG=1 --env "DM_WATCH=$((CAP + 60))" --args -mode boot "${LG_ARGS[@]}" >> "$O/launch.log" 2>&1
  local i; for i in $(seq 20); do [ -s "$D/boot.txt" ] && return 0; sleep 0.5; done; return 1; }
TRY=1
if ! launch; then stop_all; TRY=2; launch || { launch_guard_blocked "$B" "两次 open 各等 10 秒" 2>&1 | tee "$O/fail.txt"; echo "计时：拷包 0 排锁 $((T_C - T_B)) 启动失败（两次都没有 boot.txt）" | tee "$O/timing.txt"; exit 3; }; fi
T_D=$(date +%s)


END=$((T_D + CAP)); while [ "$(date +%s)" -lt "$END" ]; do LC_ALL=C grep -a -q '\[script\] note 结束' "$D/boot.txt" 2>/dev/null && break; [ -n "$(pids)" ] || break; sleep 0.5; done; T_E=$(date +%s)
stop_all
cp "$D/boot.txt" "$O/boot.txt" 2>/dev/null; [ -f "$D/pwc2s.jsonl" ] && mv "$D/pwc2s.jsonl" "$O/c2s.jsonl"


cloud_guard_check "$O/boot.txt" || { BAD=1; echo "【红】这一局出现了 iCloud 同步动作，结果作废，立刻核云端目录" | tee "$O/fail.txt"; }; cloud_guard_clean "$D"


if [ -n "${PW_SAVE:-}" ]; then WHO=$(LC_ALL=C sed -n 's/.*刷怪积分牌：「\([^」]*\)」.*/\1/p' "$O/boot.txt" 2>/dev/null | tail -1)
  if [ -n "$WHO" ] && [ ! -f "$C/$PW_SAVE/$WHO/$WHO.d2s" ]; then BAD=1; echo "【红】换存档没生效：进局的角色「${WHO}」不在 ${PW_SAVE} 里，这一局不算" | tee "$O/fail.txt"; fi; fi
for f in "$D/${RID}"_*; do [ -f "$f" ] && mv "$f" "$O/$(basename "$f" | sed "s/^${RID}_//")"; done
echo "计时：拷包 0 排锁 $((T_C - T_B)) 起包 $((T_D - T_C)) 运行 $((T_E - T_D)) 收尾 $(( $(date +%s) - T_E )) 启动次数 $TRY" | tee "$O/timing.txt"
