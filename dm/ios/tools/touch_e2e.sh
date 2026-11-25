#!/bin/bash










set -euo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/local_env.sh" && local_env_need DM_DEVICE || exit 1
D=$DM_DEVICE; B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
SCRIPT=$1; OUT=$2; CAP=${3:-60}
mkdir -p "$OUT"; cp "$SCRIPT" "$OUT/script.txt"




source "$(dirname "$0")/devbattery.sh"; devbattery_gate || { echo "【红】电量闸拒绝：${DEVBATTERY_LINE:-}" | tee "$OUT/battery.txt"; exit 3; }; echo "$DEVBATTERY_LINE" > "$OUT/battery.txt"
source "$(dirname "$0")/devlock.sh"; devlock_acquire "${SMOKE_OWNER:-未署名} 真机触屏 $(basename "$OUT")"; trap devlock_release EXIT




RID=r$(date +%H%M%S)
awk -v r="$RID" '($2=="shot"||$2=="tshot"){$3=r"_"$3} {print}' "$SCRIPT" > "$OUT/script_run.txt"
t(){ echo "[$(date +%T)] $*"; }
dc(){ timeout 60 xcrun devicectl device "$@"; }
pids(){ dc info processes --device $D --json-output "$OUT/.ps.json" >/dev/null 2>&1; python3 -c "import json,sys;print(' '.join(str(p['processIdentifier']) for p in json.load(open(sys.argv[1]))['result']['runningProcesses'] if 'DmIOS.app/DmIOS' in p.get('executable','')))" "$OUT/.ps.json"; }
for p in $(pids); do t "停残留 DmIOS pid $p"; dc process terminate --device $D --pid $p >/dev/null 2>&1 || true; done
dc copy to --device $D --domain-type appDataContainer --domain-identifier $B --source "$OUT/script_run.txt" --destination Documents/touch_script.txt >/dev/null
[ -n "${PREFIX:-}" ] && dc copy to --device $D --domain-type appDataContainer --domain-identifier $B --source "$PREFIX" --destination Documents/e2e_prefix.jsonl >/dev/null
ENV=$(CAP=$CAP RID=$RID PREFIX=${PREFIX:-} E2E_ENV=${E2E_ENV:-} E2E_KEYS=${E2E_KEYS:-} python3 - <<'EOF'
import json, os
r = os.environ["RID"]
env = {
  "DM_SAVE_FRESH": "warlock",   # 档位不设 = 产品缺省（主机档）。之前这里写死掌机档（DM_OPMODE=0），触屏脚本看到的界面参数与玩家看到的不是同一份；要测掌机档用 E2E_ENV 传
  "DM_TOUCH_SCRIPT": "touch_script.txt", "DM_TOUCH_CONTROL_LOG": "1",
  "DM_C2S_LOG": r + "_c2s.jsonl", "DM_UIMSG": r + "_uimsg.jsonl", "DM_STATE": r + "_state.jsonl",
  "DM_TICKS": r + "_ticks.jsonl", "DM_TICKS_FULL": "1", "DM_TICKS_N": "0", "DM_TARGET_LOG": r + "_target.jsonl",
  "DM_KEYS": os.environ.get("E2E_KEYS") or "X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,C0.84:0.208@OpenPanel:CharacterSelectPanel%1>UpdateCharacterInfo,MSG:CharacterSelect:LoadCharacter:0@UpdateCharacterInfo%0.3>FrontEndNavigation:ToGame",   # E2E_KEYS：换起局键序（同 touch_e2e_mac.sh）
  "DM_WATCH": str(int(os.environ["CAP"]) + 60),
}
if os.environ["PREFIX"]:
    env.update({"DM_C2S_REPLAY": "e2e_prefix.jsonl", "DM_GAME_SEED": "305419896", "DM_FIXED_TIME": "1790895128"})
if os.environ.get("E2E_ENV"): env.update(json.loads(os.environ["E2E_ENV"]))   # 额外环境（JSON），对照实验用，如 {"DM_GAME_SEED":"1"}
env = {k: v for k, v in env.items() if v is not None}   # E2E_ENV 里写 null = 去掉这个缺省（如 {"DM_SAVE_FRESH":null,"DM_SAVE_TEST":"1"}：沿用上一局的 save.test）
print(json.dumps(env))
EOF
)
dc process launch --device $D --terminate-existing --environment-variables "$ENV" --json-output "$OUT/launch.json" $B -mode boot > "$OUT/launch.log" 2>&1
PID=$(python3 -c "import json;print(json.load(open('$OUT/launch.json'))['result']['process']['processIdentifier'])")
t "启动 pid ${PID}，跑 ${CAP}s"; sleep "$CAP"
dc process terminate --device $D --pid "$PID" > "$OUT/stop.log" 2>&1 && t "已停 pid $PID" || t "pid $PID 已自行退出"




ALL="boot.txt ${RID}_c2s.jsonl ${RID}_uimsg.jsonl ${RID}_ticks.jsonl ${RID}_target.jsonl ${RID}_state.jsonl $(awk '$2=="shot"{print $3; print $3".txt"} $2=="tshot"{print $3; print $3".json"}' "$OUT/script_run.txt")"
[ -n "${E2E_PULL:-}" ] && ALL=$(for f in $E2E_PULL; do [ "$f" = boot.txt ] && echo boot.txt || echo "${RID}_$f"; done)
for f in $ALL; do
  g=${f#${RID}_}
  case $f in ${RID}_*.jsonl) g=e2e_$g;; esac
  dc copy from --device $D --domain-type appDataContainer --domain-identifier $B --source "Documents/$f" --destination "$OUT/$g" >> "$OUT/copy.log" 2>&1 || t "【红】拉不到 ${f}（本局没写）"
done
left=$(pids); [ -z "$left" ] && t "设备上 DmIOS 进程 0" || t "【红】仍有 DmIOS 进程 $left"
