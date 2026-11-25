#!/bin/bash








set -u
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/local_env.sh"
B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
SCRIPT=$1; OUT=$2; CAP=${3:-60}
TOOLS=$(cd "$(dirname "$0")" && pwd); TREE=$(git -C "$TOOLS" rev-parse --show-toplevel)
W=${E2E_MAC_APP:-$TREE/.iosdd_mac/DmMacSmoke.app}; exe="$W/Wrapper/DmIOS.app/DmIOS"
[ -x "$exe" ] || { echo "【红】没有包：${W}（先跑 dm/ios/tools/smoke_mac.sh 1）"; exit 2; }
mkdir -p "$OUT"; cp "$SCRIPT" "$OUT/script.txt"
t(){ echo "[$(date +%T)] $*"; }
C=""; for m in "$HOME"/Library/Containers/*/.com.apple.containermanagerd.metadata.plist; do
  [ "$(/usr/libexec/PlistBuddy -c 'Print :MCMMetadataIdentifier' "$m" 2>/dev/null)" = "$B" ] && C="$(dirname "$m")/Data"; done
[ -n "$C" ] || { echo "【红】本机没有 $B 的容器（先跑 smoke_mac.sh 1）"; exit 2; }
DOC="$C/Documents"
export DM_DEVLOCK_DIR=${DM_ROOT:-$HOME/dm-work}/run/ios/macsmoke.lock; source "$TOOLS/devlock.sh"; source "$TOOLS/cloud_guard.sh"; source "$TOOLS/save_key.sh"; source "$TOOLS/launch_guard.sh"; devlock_acquire "${SMOKE_OWNER:-未署名} Mac 触屏 $(basename "$OUT")"


pids(){ ps -axo pid=,comm= | awk '{ pid=$1; $1=""; sub(/^ /,""); if ($0 ~ /\/Wrapper\/DmIOS\.app\/DmIOS$/) print pid }'; }
stop_all(){ local p; for p in $(pids); do kill -TERM "$p" 2>/dev/null || true; done; sleep 1; for p in $(pids); do kill -KILL "$p" 2>/dev/null || true; done; }
pre=$(pids | tr '\n' ' '); [ -z "$pre" ] || { echo "【红】本机已有 $B 的实例在跑（pid ${pre}，非本次启动）：先结束它再跑"; devlock_release; exit 2; }
trap 'stop_all; savekey_out "$C"; devlock_release' EXIT; trap 'exit 143' INT TERM
RID=r$(date +%H%M%S)
awk -v r="$RID" '($2=="shot"||$2=="tshot"){$3=r"_"$3} {print}' "$SCRIPT" > "$OUT/script_run.txt"
cp "$OUT/script_run.txt" "$DOC/touch_script.txt"
[ -n "${PREFIX:-}" ] && cp "$PREFIX" "$DOC/e2e_prefix.jsonl"
KEYS=${E2E_KEYS:-'X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,C0.84:0.208@OpenPanel:CharacterSelectPanel%1>UpdateCharacterInfo,MSG:CharacterSelect:LoadCharacter:0@UpdateCharacterInfo%0.3>FrontEndNavigation:ToGame'}
ARGS=(--env DM_SAVE_FRESH=warlock --env DM_TOUCH_SCRIPT=touch_script.txt --env DM_TOUCH_CONTROL_LOG=1
  --env "DM_C2S_LOG=${RID}_c2s.jsonl" --env "DM_UIMSG=${RID}_uimsg.jsonl" --env "DM_STATE=${RID}_state.jsonl"
  --env "DM_TICKS=${RID}_ticks.jsonl" --env DM_TICKS_FULL=1 --env DM_TICKS_N=0 --env "DM_TARGET_LOG=${RID}_target.jsonl"
  --env "DM_KEYS=$KEYS" --env "DM_WATCH=$((CAP + 60))")


[ "${DM_MAC_DEVICE_PROFILE:-1}" != 0 ] && ARGS+=(--env DM_GAME_RES=2868x1320 --env DM_TEST_SAFE_L=62 --env DM_TEST_SAFE_R=62)



[ "${E2E_DESK:-0}" = 1 ] && ARGS+=(--env DM_PRESENT_LINK=0)
[ -n "${E2E_REF:-}" ] && ARGS+=(--env "DM_TOUCH_REF=$E2E_REF")
[ -n "${PREFIX:-}" ] && ARGS+=(--env DM_C2S_REPLAY=e2e_prefix.jsonl --env DM_GAME_SEED=305419896 --env DM_FIXED_TIME=1790895128)




ENVL=(); while IFS= read -r kv; do ENVL+=(--env "$kv"); done < <(python3 -c "
import json,sys
a=sys.argv[2:]; env={}
for i in range(0,len(a),2): k,v=a[i+1].split('=',1); env[k]=v   # a 是 --env K=V 成对 | a holds --env K=V in pairs
env.update(json.loads(sys.argv[1] or '{}'))
[print(f'{k}={v}') for k,v in env.items() if v is not None]" "${E2E_ENV:-}" "${ARGS[@]}")
ARGS=("${ENVL[@]}")


cloud_guard_env "$DOC" "$(python3 -c "import json,sys;print(' '.join(f'{k}={v}' for k,v in json.loads(sys.argv[1] or '{}').items()))" "${E2E_ENV:-}")" || exit 1
[ -z "$CG_ENV" ] || ARGS+=(--env "$CG_ENV")


savekey_in "$C" "$(python3 -c "import json,sys;print(' '.join(f'{k}={v}' for k,v in json.loads(sys.argv[1] or '{}').items() if v is not None))" "${E2E_ENV:-}")" || exit 1
launch_guard_pre "$C"; mv "$DOC/boot.txt" "$DOC/boot_prev.txt" 2>/dev/null
open -n "$W" "${ARGS[@]}" --args -mode boot "${LG_ARGS[@]}" > "$OUT/launch.log" 2>&1 || { echo "【红】open 启动失败：$(tail -1 "$OUT/launch.log")"; exit 1; }
for _ in $(seq 30); do [ -s "$DOC/boot.txt" ] && break; sleep 0.5; done; [ -s "$DOC/boot.txt" ] || { launch_guard_blocked "$B" "open 之后 15 秒"; exit 3; }
t "已启动，跑 ${CAP}s"; sleep "$CAP"; stop_all; t "已停"
CG_BAD=0; cloud_guard_check "$DOC/boot.txt" || CG_BAD=1; cloud_guard_clean "$DOC"
ALL="boot.txt ${RID}_c2s.jsonl ${RID}_uimsg.jsonl ${RID}_ticks.jsonl ${RID}_target.jsonl ${RID}_state.jsonl $(awk '$2=="shot"{print $3; print $3".txt"} $2=="tshot"{print $3; print $3".json"}' "$OUT/script_run.txt")"
[ -n "${E2E_PULL:-}" ] && ALL=$(for f in $E2E_PULL; do [ "$f" = boot.txt ] && echo boot.txt || echo "${RID}_$f"; done)
for f in $ALL; do
  g=${f#${RID}_}; case $f in ${RID}_*.jsonl) g=e2e_$g;; esac
  if [ -f "$DOC/$f" ]; then cp "$DOC/$f" "$OUT/$g"; [ "$f" = boot.txt ] || rm -f "$DOC/$f"; else t "【红】没有 ${f}（本局没写）"; fi
done
left=$(pids); [ -z "$left" ] && t "本机 DmIOS 进程 0" || t "【红】仍有 DmIOS 进程 $left"
[ "$CG_BAD" = 0 ] || exit 4
