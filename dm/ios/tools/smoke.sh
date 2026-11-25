#!/bin/bash


set -u

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/local_env.sh" && local_env_need DM_DEVICE || exit 1
D=$DM_DEVICE
B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
N=${1:-1}
ROOT=${DM_ROOT:-$HOME/dm-work}/run/ios/smoke
RUN="$ROOT/$(date +%Y%m%d_%H%M%S)"
R=${DM_ROOT:-$HOME/dm-work}/run/ios


TREE=$(git rev-parse --show-toplevel 2>/dev/null || pwd); DD=$TREE/.iosdd
APP=$DD/Build/Products/Release-iphoneos/DmIOS.app
KEYS_DEFAULT='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,C0.84:0.208@OpenPanel:CharacterSelectPanel%1>UpdateCharacterInfo,MSG:CharacterSelect:LoadCharacter:0@UpdateCharacterInfo%0.3>FrontEndNavigation:ToGame'
KEYS=${SMOKE_KEYS:-$KEYS_DEFAULT}
TIMEOUT_BIN=${TIMEOUT_BIN:-$(command -v timeout || command -v gtimeout || true)}

if [ -z "$TIMEOUT_BIN" ]; then
  echo "SMOKE RED 0/$N 用时 0 秒"
  echo "【红】缺少 timeout 命令"
  exit 2
fi
case "$N" in
  ''|*[!0-9]*|0) echo "SMOKE RED 0/$N 用时 0 秒"; echo "【红】次数必须是正整数"; exit 2;;
esac

mkdir -p "$RUN"
TOOLS=$(cd "$(dirname "$0")" && pwd)



DEV_PORT=${SMOKE_DEV_PORT:-27100}; LOCAL_PORT=${SMOKE_LOCAL_PORT:-$(python3 -c "import socket;s=socket.socket();s.bind(('127.0.0.1',0));print(s.getsockname()[1])")}
command -v iproxy >/dev/null || { echo "SMOKE RED 0/$N 用时 0 秒"; echo "【红】缺少 iproxy（brew install libimobiledevice）"; exit 2; }




source "$(dirname "$0")/devbattery.sh"; devbattery_gate || { echo "SMOKE RED 0/$N 用时 0 秒"; echo "【红】电量闸拒绝：${DEVBATTERY_LINE:-}"; exit 2; }; echo "冒烟开跑前 $DEVBATTERY_LINE"
source "$(dirname "$0")/devlock.sh"
devlock_acquire "${SMOKE_OWNER:-未署名} 冒烟 $(basename "$(git rev-parse --show-toplevel 2>/dev/null || pwd)")"






LEDGER=${SMOKE_LEDGER:-${DM_ROOT:-$HOME/dm-work}/run/ios/smoke_ledger.tsv}; LEDGER_RES=中断; LEDGER_SEGS=""; ok=0
ledger_write() {
  local top head dirty
  top=$(git rev-parse --show-toplevel 2>/dev/null || pwd); head=$(git -C "$top" rev-parse HEAD 2>/dev/null || echo 无)
  dirty=$(git -C "$top" status --porcelain --untracked-files=no 2>/dev/null | wc -l | tr -d ' ')
  mkdir -p "$(dirname "$LEDGER")"
  [ -s "$LEDGER" ] || printf '时刻\t调用方\t被测树\tHEAD\t未提交改动\t结果\t成功/次数\t用时秒\t各次分段\t结果目录\n' >> "$LEDGER"
  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s/%s\t%s\t%s\t%s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "${SMOKE_OWNER:-未署名}" "$top" "$head" "$dirty" \
    "${SMOKE_KEYS:+反例:}${SMOKE_TEST_STALE:+反例:}$LEDGER_RES" "$ok" "$N" "$(( $(date +%s) - ${T0:-$(date +%s)} ))" "${LEDGER_SEGS:-无}" "$RUN" >> "$LEDGER"
}
trap 'ledger_write; [ -n "${IPROXY_PID:-}" ] && kill "$IPROXY_PID" 2>/dev/null; devlock_release' EXIT
trap 'exit 143' INT TERM

T0=$(date +%s)
echo "SMOKE 开始 $N 次，结果目录 $RUN"

if ! "$TIMEOUT_BIN" 600s env DM_NOINSTALL=1 DM_IOS_DD="$DD" bash dm/ios/run.sh build >"$RUN/build.log" 2>&1; then
  LEDGER_RES=构建失败
  echo "【红】构建失败：$RUN/build.log"
  echo "SMOKE RED 0/$N 用时 $(( $(date +%s) - T0 )) 秒"
  exit 2
fi
if ! "$TIMEOUT_BIN" 60s xcrun devicectl device install app --device "$D" "$APP" >"$RUN/install.log" 2>&1; then
  LEDGER_RES=装机失败
  echo "【红】装机失败：$RUN/install.log"
  echo "SMOKE RED 0/$N 用时 $(( $(date +%s) - T0 )) 秒"
  exit 2
fi







"$TIMEOUT_BIN" 20s xcrun devicectl list devices --json-output "$RUN/devices.json" >/dev/null 2>&1
UDID=$(python3 -c "import json,sys;print(next((d.get('hardwareProperties',{}).get('udid','') for d in json.load(open(sys.argv[1]))['result']['devices'] if sys.argv[2] in (d.get('identifier'), d.get('hardwareProperties',{}).get('udid'))), ''))" "$RUN/devices.json" "$D" 2>/dev/null)
if [ -z "$UDID" ]; then
  LEDGER_RES=查不到设备
  echo "【红】按 DM_DEVICE 查不到这台设备的 UDID（$RUN/devices.json）：端口转发没法点名设备"
  echo "SMOKE RED 0/$N 用时 $(( $(date +%s) - T0 )) 秒"
  exit 2
fi
iproxy -u "$UDID" "$LOCAL_PORT:$DEV_PORT" >"$RUN/iproxy.log" 2>&1 & IPROXY_PID=$!
sleep 1; kill -0 "$IPROXY_PID" 2>/dev/null || { LEDGER_RES=转发没起来; echo "【红】端口转发没起来（$RUN/iproxy.log：$(tail -1 "$RUN/iproxy.log" | cut -c1-120)）"; echo "SMOKE RED 0/$N 用时 $(( $(date +%s) - T0 )) 秒"; exit 2; }



cds_restart() {
  local p; p=$(pgrep -u "$(id -u)" -f 'CoreDeviceService.xpc/Contents/MacOS/CoreDeviceService' | head -1)
  [ -n "$p" ] && { kill -TERM "$p"; for _ in 1 2 3 4 5 6 7 8 9 10; do kill -0 "$p" 2>/dev/null || break; sleep 0.5; done; kill -0 "$p" 2>/dev/null && kill -KILL "$p"; }
  echo "健康检查：$1 → 重启 CoreDeviceService（原 pid ${p:-无}）"; LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}重启服务:$1"
}



pids() {
  local json="$RUN/processes.json"
  rm -f "$json"




  local _s=$(python3 -c 'import time;print(time.time())') _rc
  "$TIMEOUT_BIN" 15s xcrun devicectl device info processes --device "$D" --json-output "$json" >/dev/null 2>"$RUN/pids_last.err"; _rc=$?
  echo "$(date +%T) 查进程表 rc=$_rc 用时 $(python3 -c "import time;print(round(time.time()-$_s,2))") 秒；调用方 ${FUNCNAME[1]:-?}；$(head -c 300 "$RUN/pids_last.err" | tr '\n' ' ')" >> "$RUN/pids_trace.log"
  [ $_rc = 0 ] || return 2
  python3 - "$json" <<'PY'
import json
import sys

try:
    processes = json.load(open(sys.argv[1]))["result"]["runningProcesses"]
except (OSError, KeyError, TypeError, ValueError):
    raise SystemExit(2)
for process in processes:
    executable = process.get("executable", "")
    if executable.endswith("DmIOS.app/DmIOS"):
        print(process["processIdentifier"])
PY
}

stop_pid() {
  local pid=$1
  "$TIMEOUT_BIN" 15s xcrun devicectl device process terminate --device "$D" --pid "$pid" >/dev/null 2>&1 || true
}

count_lines() {
  local pattern=$1
  [ -f "$2" ] || return 0
  grep -c -- "$pattern" "$2" 2>/dev/null || true
}







gone_timeout=${SMOKE_GONE_TIMEOUT:-5}; ENV_RELAUNCH=0
wait_gone() {
  local t0 p q rc lim=$gone_timeout; t0=$(date +%s)
  while :; do
    p=$(pids); rc=$?; [ -n "$p" ] && p="$(echo $p) "
    if [ "$rc" -ne 0 ]; then
      if [ $(( $(date +%s) - t0 )) -ge "$lim" ]; then
        echo "$1：拉起前查设备进程表连续失败 ${lim} 秒，不能确认上一局已退出；最后一次报错：$(head -c 200 "$RUN/pids_last.err" 2>/dev/null | tr '\n' ' ')（逐次记录在 $RUN/pids_trace.log）"; LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}进程表查询失败:$1"; return
      fi
      sleep 0.3; continue
    fi
    [ -n "$p" ] || break
    if [ $(( $(date +%s) - t0 )) -ge "$lim" ]; then
      echo "$1：拉起前本包进程 pid ${p}等了 ${lim} 秒仍在进程表里，按 pid 结束"; LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}拉起前强制结束:$1"
      for q in $p; do stop_pid "$q"; done; t0=$(date +%s); lim=$((lim + 5))
    fi
    sleep 0.3
  done
}


noenv_instance() {
  local p q; p=$(pids | tr '\n' ' '); [ -n "$p" ] || return 1
  ENV_RELAUNCH=$((ENV_RELAUNCH + 1)); echo "$1：launch 成功但日志端口零行，设备上有本包进程 pid ${p}= 起来了但没拿到环境变量；按 pid 结束，重起一次（环境重起累计 ${ENV_RELAUNCH} 次）"
  for q in $p; do stop_pid "$q"; done; return 0
}


stale_inject() { ( for v in $(env | sed -n 's/^\(DEVICECTL_CHILD_[A-Z_]*\)=.*/\1/p'); do unset "$v"; done; "$TIMEOUT_BIN" -k 5s 30s xcrun devicectl device process launch --device "$D" "$B" -mode boot >"$RUN/stale_launch.txt" 2>&1 ); sleep 3; echo "故障注入（$1）：已拉起一个不带环境变量的实例 pid $(pids | tr '\n' ' ')"; }

find_pid() {
  pids | awk 'NR == 1 { print; exit }'
}

ok=0
known_pk=0
main_menu_timeout=${SMOKE_MAIN_MENU_TIMEOUT:-40}
game_screen_timeout=${SMOKE_GAME_SCREEN_TIMEOUT:-30}
case "$main_menu_timeout:$game_screen_timeout" in
  ''|*[!0-9:]*|*:*:*) echo "SMOKE RED 0/$N 用时 0 秒"; echo "【红】阶段上限必须是非负整数"; exit 2;;
esac
for i in $(seq 1 "$N"); do
  OUT="$RUN/$i"
  mkdir -p "$OUT"
  START=$(date +%s)
  attempt=0
  result=1
  stage=启动
  pid=""
  envre=0
  CONSOLE="$RUN/console_${i}.txt"
  : > "$CONSOLE"
  while [ "$attempt" -lt 3 ]; do
    attempt=$((attempt + 1))
    attempt_log="$OUT/console_${attempt}.txt"
    : > "$attempt_log"
    export DEVICECTL_CHILD_DM_SAVE_FRESH=warlock
    export DEVICECTL_CHILD_DM_WATCH=$((main_menu_timeout + game_screen_timeout + 5))
    export DEVICECTL_CHILD_DM_UIMSG=1
    export DEVICECTL_CHILD_DM_LOG_PORT=$DEV_PORT
    export DEVICECTL_CHILD_DM_KEYS="$KEYS"




    python3 "$TOOLS/logport.py" quit "$LOCAL_PORT" 10 || { echo "第 $i/$N 次：上一局实例 quit 后仍在，改用 devicectl 结束"; for p in $(pids); do stop_pid "$p"; done; }
    [ "${CDS_DUE:-0}" = 1 ] && { cds_restart "上一局拉起超 10 s"; CDS_DUE=0; }
    [ "${SMOKE_TEST_STALE:-}" = pre ] && [ "$i" = 2 ] && [ "$envre" = 0 ] && stale_inject pre
    wait_gone "第 $i 次"
    [ "${SMOKE_TEST_STALE:-}" = post ] && [ "$i" = 2 ] && [ "$envre" = 0 ] && stale_inject post
    tA=$(date +%s)
    "$TIMEOUT_BIN" -k 5s 20s xcrun devicectl device process launch --device "$D" "$B" -mode boot >"$OUT/launch_${attempt}.txt" 2>&1; lrc=$?


    if [ "$lrc" -ne 0 ]; then
      cds_restart "第 $i 次拉起失败（rc=${lrc}）"; tA=$(date +%s)
      "$TIMEOUT_BIN" -k 5s 30s xcrun devicectl device process launch --device "$D" "$B" -mode boot >>"$OUT/launch_${attempt}.txt" 2>&1; lrc=$?
    elif [ $(( $(date +%s) - tA )) -gt 10 ]; then CDS_DUE=1; fi
    eval "$(python3 "$TOOLS/logport.py" read "$LOCAL_PORT" "$attempt_log" "$main_menu_timeout" "$game_screen_timeout")"
    cat "$attempt_log" >> "$CONSOLE"
    if [ "$result" -ne 0 ] && [ -z "$tL" ] && [ "$lrc" -eq 0 ] && [ "$envre" = 0 ] && noenv_instance "第 $i 次"; then envre=1; attempt=$((attempt - 1)); continue; fi
    [ "$pk" = 1 ] && { known_pk=1; stage=启动; }
    chan=""; [ "$lrc" -ne 0 ] && chan="（拉起失败：devicectl 退出码 ${lrc}）"
    [ "$result" -ne 0 ] && [ -z "$tL" ] && [ "$lrc" -eq 0 ] && chan="（日志端口零行：App 未监听或 iproxy 不通）"
    [ "$exited" = 1 ] || chan="${chan}（quit 后实例未退）"

    if [ "$result" -eq 0 ]; then
      break
    fi
    if [ "$pk" = 1 ] && [ "$attempt" -lt 3 ]; then
      echo "第 $i/$N 次出现已知PK，补跑 $((attempt))/2"
      continue
    fi
    break
  done





  seg=""; prev=$tA; for kv in "连接:$tL:9999" "主菜单:$tM:10" "进局:$tG:5" "撤屏:$tS:8"; do IFS=: read -r nm tt nv <<<"$kv"; [ -n "$tt" ] || break; d=$((tt - prev)); w=""; [ $d -gt $((nv * 2)) ] && w="⚠️慢"; seg="$seg $nm ${d}s$w"; prev=$tt; done
  # App 自己的启动 = 日志里「启动计时 … T6 dm_main 前 N ms」（零点 = 进程创建）；实测 148–160 ms，超过 500 ms 标慢
  # The App's own launch = 「启动计时 … T6 dm_main 前 N ms」(launch timing ... T6 before dm_main N ms) in the log (zero point = process creation); measured 148–160 ms, over 500 ms is marked slow
  t6=$(sed -n 's/.*T6 dm_main 前 \([0-9]*\)\.[0-9]* ms.*/\1/p' "$CONSOLE" 2>/dev/null | head -1)
  if [ -n "$t6" ]; then w=""; [ "$t6" -gt 500 ] && w="⚠️慢"; seg="$seg 启动(App) ${t6}ms$w"; fi
  if [ "$result" -eq 0 ]; then
    echo "第 $i/$N 次 OK：主菜单 $(count_lines MainMenuButtonRibbon "$CONSOLE") 进局 $(count_lines 'FrontEndNavigation ToGame' "$CONSOLE") 撤屏 1，用时 $(( $(date +%s) - START )) 秒；分段$seg"
    ok=$((ok + 1))
    LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}$i:OK$seg"
  else
    LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}$i:RED@$stage$seg"
    echo "第 $i/$N 次 RED：卡在：${stage}（主菜单 $(count_lines MainMenuButtonRibbon "$CONSOLE")/进局 $(count_lines 'FrontEndNavigation ToGame' "$CONSOLE")/撤屏 0）；分段$seg${chan}"
  fi
done

# 玩家路径：产品缺省下 30 s alarm / 15 s 无进展看门狗曾杀掉桌面启动的 App，而所有测试都显式设 DM_WATCH 从未测到。
# 每次冒烟 1 局（不带任何启动参数 = 玩家点图标，原先带 -mode boot，掩盖了缺省模式是探针的缺陷）：去掉看门狗与存档等测试变量，只带日志口与按键；进局 35 s 后开暂停菜单（PLUS → R → R）再静置 20 s——跨过两种看门狗门限；反例运行（设了 SMOKE_KEYS）不跑
# Player path: under product defaults the 30 s alarm / 15 s no-progress watchdog used to kill an App launched from the home screen, and since all tests explicitly set DM_WATCH this was never caught.
# 1 run per smoke test (with no launch arguments at all = the player tapping the icon; previously it carried -mode boot, masking the defect that the default mode was the probe): drop test variables such as watchdog and save, carrying only the log port and keys; 35 s after entering the game open the pause menu (PLUS → R → R) then idle 20 s -- crossing both watchdog thresholds; negative-case runs (SMOKE_KEYS set) do not run it
PLAYER_RES=跳过
if [ -z "${SMOKE_KEYS:-}" ] && [ "${SMOKE_PLAYER:-1}" = 1 ]; then
  python3 "$TOOLS/logport.py" quit "$LOCAL_PORT" 10 >/dev/null 2>&1 || for p in $(pids); do stop_pid "$p"; done
  KP="$KEYS_DEFAULT,PLUS@UnloadPanel:LoadScreenPanel%35>OpenPanel:PlayerInventoryExpansionLayout,R@OpenPanel:PlayerInventoryExpansionLayout%1>OpenPanel:SkillsTreePanel,R@OpenPanel:SkillsTreePanel%1>OpenPanel:PauseLayout"
  player_once() {
  wait_gone "玩家路径"
  env -u DEVICECTL_CHILD_DM_WATCH -u DEVICECTL_CHILD_DM_SAVE_FRESH DEVICECTL_CHILD_DM_SAVE_TEST=1 DEVICECTL_CHILD_DM_LOG_PORT=$DEV_PORT DEVICECTL_CHILD_DM_UIMSG=1 DEVICECTL_CHILD_DM_KEYS="$KP" \
    "$TIMEOUT_BIN" -k 5s 30s xcrun devicectl device process launch --device "$D" "$B" > "$RUN/player_launch.txt" 2>&1 || { cds_restart "玩家路径拉起失败"; env -u DEVICECTL_CHILD_DM_WATCH -u DEVICECTL_CHILD_DM_SAVE_FRESH DEVICECTL_CHILD_DM_SAVE_TEST=1 DEVICECTL_CHILD_DM_LOG_PORT=$DEV_PORT DEVICECTL_CHILD_DM_UIMSG=1 DEVICECTL_CHILD_DM_KEYS="$KP" "$TIMEOUT_BIN" -k 5s 30s xcrun devicectl device process launch --device "$D" "$B" >> "$RUN/player_launch.txt" 2>&1; }
  pr=$(python3 "$TOOLS/logport.py" player "$LOCAL_PORT" "$RUN/player.txt" "$main_menu_timeout" 35 20)
  }
  player_once
  # 同上一道防线：玩家路径日志零字节而设备上有本包进程 = 没拿到环境，结束后重起一次
  # Same line of defense as above: player path log zero bytes while the device has this package's process = the environment was not received; end it and relaunch once
  [ -s "$RUN/player.txt" ] || { noenv_instance "玩家路径" && player_once; }
  # 产品缺省必须真出声：玩家路径日志里要有真出声的 AudioOut 打开行，出现「静音接收端」即红
  # Product defaults must really produce sound: the player path log must contain an AudioOut open line with real sound output, and 「静音接收端」(muted sink) appearing means failure
  au_real=$(grep -c 'audio::OpenAudioOut .*真出声' "$RUN/player.txt" 2>/dev/null); au_null=$(grep -c 'audio::OpenAudioOut .*静音接收端' "$RUN/player.txt" 2>/dev/null)
  hud_ok=$(LC_ALL=C grep -a 'dm_hud：' "$RUN/player.txt" 2>/dev/null | LC_ALL=C grep -a -v -c '未读到玩家')   # 产品缺省必须读得到受控玩家（同 smoke_mac.sh） | Product defaults must be able to read the controlled player (same as smoke_mac.sh)
  red_n=$(LC_ALL=C grep -a -c '【红】' "$RUN/player.txt" 2>/dev/null)   # 玩家路径日志不许有红行（同 smoke_mac.sh） | The player path log must have no failure lines (same as smoke_mac.sh)
  if [[ "$pr" == result=0* ]] && [ "${au_real:-0}" -gt 0 ] && [ "${au_null:-0}" = 0 ] && [ "${hud_ok:-0}" -gt 0 ] && [ "${red_n:-0}" = 0 ]; then PLAYER_RES=OK; echo "玩家路径 OK：无看门狗变量，进局 35 s + 暂停菜单 20 s 仍活着；AudioOut 真出声 ${au_real} 路（${pr}）"
  else PLAYER_RES=RED; echo "玩家路径 RED：${pr}；AudioOut 真出声 ${au_real:-0} 路 / 静音接收端 ${au_null:-0} 路（须真出声 > 0、静音 0）；读到玩家状态 ${hud_ok:-0} 行（须 > 0）；红行 ${red_n:-0} 条（须 0）"; fi
  LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}玩家路径:$PLAYER_RES"
fi

[ "$ENV_RELAUNCH" -gt 0 ] && { echo "环境重起 ${ENV_RELAUNCH} 次（起来的实例没拿到环境变量，已按 pid 结束并重起；不算红）"; LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}环境重起:${ENV_RELAUNCH}次"; }
left=""; left_rc=2
for _ in 1 2 3; do left=$(pids) && { left_rc=0; break; }; sleep 2; done   # devicectl 偶发失败重试；三次都失败 = 不能确认，按红报 | Retry on sporadic devicectl failures; all three failing = cannot confirm, report as a failure
left=$(echo $left)
if [ "$left_rc" -ne 0 ]; then
  echo "【红】结束后查设备进程表 3 次都失败（devicectl 出错 / 超时），不能确认本包已退出"
  LEDGER_SEGS="${LEDGER_SEGS:+${LEDGER_SEGS}；}收尾进程表查询失败"
elif [ -n "$left" ]; then
  echo "【红】结束后仍有本包 DmIOS pid：$left"
  # 残留实例的 App 端日志（Documents/boot.txt）在结束它之前拉回：下次拉起会清空重写，锁外别的任务一拉起证据就没了
  # The leftover instance's App-side log (Documents/boot.txt) is pulled back before ending it: the next launch clears and rewrites it, and as soon as another task outside the lock launches, the evidence is gone
  "$TIMEOUT_BIN" 60s xcrun devicectl device copy from --device "$D" --domain-type appDataContainer --domain-identifier "$B" --source Documents/boot.txt --destination "$RUN/boot_left.txt" >/dev/null 2>&1 \
    && echo "残留实例日志：$RUN/boot_left.txt（$(grep -c '' "$RUN/boot_left.txt") 行，末行：$(tail -1 "$RUN/boot_left.txt" | cut -c1-80)）"
  for pid in $left; do stop_pid "$pid"; done
fi

elapsed=$(( $(date +%s) - T0 ))
if [ "$ok" -eq "$N" ] && [ "$left_rc" -eq 0 ] && [ -z "$left" ] && [ "$PLAYER_RES" != RED ]; then
  [ "$known_pk" -eq 1 ] && echo "【已知PK】占不到 pthread 键 507：已按规则补跑，最终通过"
  LEDGER_RES=OK
  echo "SMOKE OK $ok/$N 用时 $elapsed 秒"
  exit 0
fi
[ "$known_pk" -eq 1 ] && echo "【已知PK】占不到 pthread 键 507：已按规则最多补跑 2 次"
LEDGER_RES=RED
echo "SMOKE RED $ok/$N 用时 $elapsed 秒"
exit 1
