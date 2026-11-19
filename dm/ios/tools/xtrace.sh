#!/bin/bash
















export DM_DEVLOCK_CLASS=measure
export XT_TABLES="ca-client-buffer-wait-interval ca-client-present-request display-vsyncs-interval display-surface-swap displayed-surfaces-interval metal-gpu-intervals metal-gpu-state-intervals metal-application-encoders-list metal-application-intervals metal-driver-event-intervals metal-command-buffer-error metal-shader-profiler-intervals metal-shader-profiler-shader-list gpu-performance-state-intervals gpu-performance-device-state-intervals gpu-shader-profiler-interval gpu-shader-profiler-sample gpu-counter-info gpu-counter-value device-thermal-state-intervals context-switch cpu-state graphics-compiler-activity-intervals graphics-compiler-spill-events hang-risks system-load thread-info"
set -u
t(){ echo "[$(date +%T)] $*"; }


kt_mark(){ KT0=$1/.kt0; : > "$KT0"; }
kt_clean(){ local T; T=$(getconf DARWIN_USER_TEMP_DIR); find "$T" -maxdepth 1 -name 'instruments*.ktrace' \( -newer "$KT0" -o -mmin +30 \) -delete 2>/dev/null; }
xexport(){
  local tr=$1 d; d=$(dirname "$tr"); mkdir -p "$d/tables"


  xcrun xctrace export --input "$tr" --toc --output "$d/toc.xml" 2> "$d/export.log" || { t "目录页导不出，按内置表名逐张导"; for s in $XT_TABLES; do echo "<table schema=\"$s\"/>"; done > "$d/toc.xml"; }
  python3 - "$tr" "$d" <<'PY'
import sys, re, os, subprocess, collections
tr, d = sys.argv[1:3]; toc = open(d + '/toc.xml').read(); seen = collections.Counter()
skip = re.compile(os.environ.get('XT_SKIP', r'^(kdebug|os-log|os-signpost|time-sample|time-profile|counters-profile)'))   # 原始内核事件表单张可上 GB，缺省不导；XT_SKIP='^$' 全导
only = re.compile(os.environ.get('XT_ONLY', '^(' + '|'.join(os.environ['XT_TABLES'].split()) + ')$'))   # 缺省只导读数用得上的表（全导约 6 分钟）；XT_ONLY=. 全导
for s in re.findall(r'<table [^>]*schema="([^"]+)"', toc):
    seen[s] += 1
    if skip.search(s) or not only.search(s): continue
    out = f'{d}/tables/{s}' + (f'.{seen[s]}' if seen[s] > 1 else '') + '.xml'
    xp = f'/trace-toc/run[@number="1"]/data/table[@schema="{s}"][{seen[s]}]'
    r = subprocess.run(['xcrun', 'xctrace', 'export', '--input', tr, '--xpath', xp, '--output', out], capture_output=True, text=True)
    print(f'{s}\t{out if r.returncode == 0 else "失败：" + r.stderr.strip()[:80]}')
PY
}
[ "${1:-}" = export ] && { xexport "${2:?缺 .trace 路径}"; exit; }
if [ "${1:-}" = mac ]; then
  SC=${2:?camp|wild}; TPL=${3:?模板名}; SEC=${4:-30}; WARM=${5:-60}; W=${XT_TREE:?XT_TREE = 含 dm/host/dmhost 的树}; OUT=${XT_OUT:?XT_OUT = 产物目录}
  source "$(dirname "$0")/mlock.sh"; mlock_acquire "${XT_OWNER:-未署名} xctrace $SC $(basename "$TPL" .tracetemplate) 约 $((WARM + SEC + 40)) s"; trap mlock_release EXIT
  SV=${XT_SAVE:-${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref_warlock/0000000000000001}; R=${XT_DIR:-${XT_RUN:-${DM_ROOT:-$HOME/dm-work}/run/xtrace}/$(date +%m%d_%H%M%S)_mac_${SC}_$(basename "$TPL" .tracetemplate | tr ' ' '_')}; mkdir -p "$R/temp"
  [ "$(df -m "$R" | awk 'NR==2{print $4}')" -ge 20480 ] || { echo "🔴 磁盘剩余不足 20 GB，不跑（清 ${DM_ROOT:-$HOME/dm-work}/run/xtrace 里旧的 t.trace）"; exit 1; }
  cp -R "$SV/0" "$R/save" && chmod -R u+w "$R/save" || { echo "🔴 拷存档失败"; exit 1; }
  K='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,MSG:CharacterSelect:LoadCharacter:0@OpenPanel:CharacterSelectPanel%1>FrontEndNavigation:ToGame'
  [ "$SC" = camp ] || { echo "🔴 mac 模式目前只接了营地（野外要回放前缀，后补）"; exit 2; }
  ( cd "$W/dm/host" && bash "${XT_GUIRUN:-$W/dm/tools/guirun.sh}" -o "$R/host.log" -e "$R/host.err" -p "$R/pid" DM_WINDOW=1 DM_GAME_RES=${XT_RES:-2868x1320} DM_3D_SCALE=${XT_3D:-0.75} \
      DM_KEYS="$K" DM_SAVE="$R/save" DM_TEMP="$R/temp" DM_NVN_SHMAP="$W/dm/gfx/nvn/shader_map.tsv" DM_WATCH=$((WARM + SEC + 60)) ${XT_ENV:-} -- "$W/dm/host/dmhost" "$OUT" main sdk ) &
  t "已起带窗口宿主，预热 $WARM s"; sleep "$WARM"; p=$(cat "$R/pid" 2>/dev/null); kill -0 "$p" 2>/dev/null || { t "🔴 宿主不在了"; tail -3 "$R/host.log" "$R/host.err"; exit 1; }
  t "录制 $TPL $SEC s（pid ${p}）"; kt_mark "$R"; timeout -k 5 $((SEC + 60)) xcrun xctrace record --template "$TPL" --attach "$p" --time-limit "${SEC}s" --output "$R/t.trace" --no-prompt > "$R/record.log" 2>&1; kill "$p" 2>/dev/null; kt_clean
  [ -d "$R/t.trace" ] || { t "🔴 没有录到 trace"; tail -5 "$R/record.log"; exit 1; }
  t "录完 → $R/t.trace"; [ "${XT_EXPORT:-1}" = 0 ] || xexport "$R/t.trace" | tee "$R/tables.tsv" | wc -l | xargs -I{} echo "导出 {} 张表 → $R/tables/"


  [ "${XT_EXPORT:-1}" = 0 ] || [ "${XT_KEEP_TRACE:-0}" = 1 ] || for sub in t.trace save save.work temp; do [ -e "$R/$sub" ] && find "$R/$sub" -delete; done
  echo "RUN $R PID $p"; exit
fi
SC=${1:?camp|wild|menu|blank}; TPL=${2:?模板名}; SEC=${3:-30}; WARM=${4:-30}
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/local_env.sh" && local_env_need DM_DEVICE DM_IOS_UDID || exit 1
D=$DM_DEVICE; XD=${XT_UDID:-$DM_IOS_UDID}; B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}; W=${XT_TREE:?XT_TREE 必填}
APP=$W/.iosdd/Build/Products/Release-iphoneos/DmIOS.app; [ -d "$APP" ] || { echo "🔴 $APP 不存在（先构建）"; exit 1; }
R=${DM_ROOT:-$HOME/dm-work}/run/ios/xtrace/$(date +%m%d_%H%M%S)_${SC}_$(basename "$TPL" .tracetemplate | tr ' ' '_'); mkdir -p "$R"
K_CAMP='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,MSG:CharacterSelect:LoadCharacter:0@OpenPanel:CharacterSelectPanel%1>FrontEndNavigation:ToGame'
K_WILD='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,C0.84:0.208@OpenPanel:CharacterSelectPanel%1>UpdateCharacterInfo,MSG:CharacterSelect:LoadCharacter:0@UpdateCharacterInfo%0.3>FrontEndNavigation:ToGame'
A='A@DM:ReplayDone+2'; for i in $(seq 1 40); do A="$A,$(echo LSL LSU LSR LSD | cut -d' ' -f$(( (i - 1) % 4 + 1 )))@+1,A@+2"; done
envs=(DEVICECTL_CHILD_DM_BRIGHTNESS=0.5 DEVICECTL_CHILD_DM_SAVE_FRESH=warlock DEVICECTL_CHILD_DM_UIMSG=1)
case "$SC" in camp|wild|menu|blank) ;; *) echo "🔴 未知场景 $SC"; exit 2;; esac
MODE=boot; [ "$SC" = blank ] && MODE=blank
if [ "$SC" = camp ]; then envs+=(DEVICECTL_CHILD_DM_KEYS="$K_CAMP")
elif [ "$SC" = menu ] || [ "$SC" = blank ]; then :
else envs+=(DEVICECTL_CHILD_DM_KEYS="$K_WILD,$A" DEVICECTL_CHILD_DM_C2S_REPLAY=m1_wild.jsonl DEVICECTL_CHILD_DM_GAME_SEED=305419896); fi
for e in ${XT_ENV:-}; do envs+=(DEVICECTL_CHILD_$e); done
pid(){ xcrun devicectl device info processes --device $D 2>/dev/null | grep "DmIOS.app/DmIOS" | awk '{print $1}'; }
source "$W/dm/ios/tools/devlock.sh"; devlock_acquire "${XT_OWNER:-未署名} xctrace $SC $TPL"; trap devlock_release EXIT; trap 'exit 143' INT TERM
[ "${XT_INSTALL:-0}" = 1 ] && { timeout -k 5 120 xcrun devicectl device install app --device $D "$APP" > "$R/install.log" 2>&1 || { t "🔴 装机失败"; exit 1; }; }
[ "$SC" = wild ] && { timeout -k 5 60 xcrun devicectl device copy to --device $D --domain-type appDataContainer --domain-identifier $B --source ${XT_WILD_C2S:-${DM_ROOT:-$HOME/dm-work}/run/verify/dm/run_sta_contract_1002_090004/wild_prefix_original_warlock/c2s.jsonl} --destination Documents/m1_wild.jsonl > /dev/null 2>&1 || t "🔴 推野外回放前缀失败"; }
env "${envs[@]}" timeout -k 5 30 xcrun devicectl device process launch --device $D --terminate-existing $B -mode $MODE > "$R/launch.log" 2>&1 || { t "🔴 拉起失败"; tail -3 "$R/launch.log"; exit 1; }
t "已拉起（${SC}），预热 $WARM s"; sleep "$WARM"
p=$(pid); [ -n "$p" ] || { t "🔴 预热后进程不在（App 已退出）"; exit 1; }
t "录制 $TPL $SEC s（pid ${p}）"; kt_mark "$R"; timeout -k 5 $((SEC + 40)) xcrun xctrace record --device "$XD" --template "$TPL" --attach "$p" --time-limit "${SEC}s" --output "$R/t.trace" --no-prompt > "$R/record.log" 2>&1; rc=$?; kt_clean
p=$(pid); [ -n "$p" ] && xcrun devicectl device process terminate --device $D --pid $p > /dev/null 2>&1
xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B --source Documents/perf.txt --destination "$R/perf.txt" > /dev/null 2>&1
devlock_release; trap - EXIT
tail -4 "$R/perf.txt" 2>/dev/null | cut -f1-5,9-11; tail -4 "$R/perf.txt" 2>/dev/null | grep -q 局内 || t "🔴 录制期间 App 不在局内（看上面 perf.txt 末几行的场景列）：这条不算数"
[ -d "$R/t.trace" ] || { t "🔴 没有录到 trace（xctrace 退出码 ${rc}）"; tail -5 "$R/record.log"; exit 1; }
t "录完 → $R/t.trace（$(du -sh "$R/t.trace" | cut -f1)）"; [ "${XT_EXPORT:-1}" = 0 ] || xexport "$R/t.trace" | tee "$R/tables.tsv" | wc -l | xargs -I{} echo "导出 {} 张表 → $R/tables/"
