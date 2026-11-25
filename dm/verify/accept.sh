#!/bin/bash


















set -u
export LC_ALL=C
HERE=$(cd "$(dirname "$0")" && pwd)
MODE=full
case "${1:-}" in --fast) MODE=fast; shift;; --full) shift;; --*) echo "ACCEPT RED 未知档位 $1"; exit 2;; esac
OUT=${1:-$(bash "$(dirname "$0")/../tools/product.sh" path keypage)}
TR=${TRUTH_RUN:-${DM_ROOT:-$HOME/dm-work}/data/switch/truth_runs/cvd5}




STALE=$(git -C "$HERE" log --no-merges --oneline HEAD..main -- "$HERE" 2>/dev/null | wc -l | tr -d ' ')
[ "${STALE:-0}" = 0 ] || { echo "🔴 ACCEPT 拒跑：本树 dm/verify 落后 main $STALE 个验证提交（验收入口过期，读数无意义）——先合入 main 再跑"; git -C "$HERE" log --no-merges --oneline HEAD..main -- "$HERE" | head -5; exit 1; }




TREE=$(basename "$(git -C "$HERE" rev-parse --show-toplevel 2>/dev/null || printf '%s' "$HERE")")
LOCK=${DM_ROOT:-$HOME/dm-work}/run/verify/dm/accept.lock; ME="${ACCEPT_OWNER:-$TREE} $(git -C "$HERE" rev-parse --abbrev-ref HEAD 2>/dev/null)"
while ! mkdir "$LOCK" 2>/dev/null; do
  op=$(cat "$LOCK/pid" 2>/dev/null)
  if [ -n "$op" ] && ! kill -0 "$op" 2>/dev/null; then echo "接管陈旧锁（pid $op 已不在：$(cat "$LOCK/owner" 2>/dev/null)）"; rm -rf "$LOCK"; continue; fi
  [ "${_waited:-}" ] || { echo "等待 $(cat "$LOCK/owner" 2>/dev/null)（pid ${op:-?}）的 accept"; _waited=1; }
  sleep 2
done
echo $$ > "$LOCK/pid"; echo "$ME" > "$LOCK/owner"; trap 'rm -rf "$LOCK"' EXIT
mkdir -p "${DM_ROOT:-$HOME/dm-work}/run/verify/dm/accept"
L=$(mktemp -d "${DM_ROOT:-$HOME/dm-work}/run/verify/dm/accept/$(date +%m%d_%H%M%S)_${TREE}_$$.XXXXXX") || { echo "ACCEPT RED 无法创建结果目录"; exit 1; }
T0=$SECONDS
SKIP=""; SCOPE="全量"; M3GATE=1
if [ "${ACCEPT_ALL:-0}" != 1 ]; then
  ROOT=$(git -C "$HERE" rev-parse --show-toplevel 2>/dev/null); MB=$(git -C "$HERE" merge-base HEAD main 2>/dev/null)
  CH=""; [ -n "$ROOT" ] && [ -n "$MB" ] && CH=$( { git -C "$ROOT" diff --name-only --no-renames "$MB" HEAD; git -C "$ROOT" status --porcelain --no-renames | cut -c4-; } 2>/dev/null | sort -u | grep -v '^$')
  [ -n "${ACCEPT_SCOPE_CH:-}" ] && CH=$(printf '%s\n' "$ACCEPT_SCOPE_CH" | tr ' ' '\n')




  HOSTDEP=$(grep -hoE 'ios/Sources/[A-Za-z0-9_]+\.(\[ch\]|[chm])' "$HERE/../host/Makefile" "$HERE/../host/tests/Makefile" 2>/dev/null | sed -E 's/^(.*)\.\[ch\]$/\1.c\n\1.h/' | sed 's#^#dm/#' | sort -u)
  CH=$(printf '%s\n' "$CH" | while IFS= read -r f; do [ -n "$f" ] || continue; if printf '%s\n' "$HOSTDEP" | grep -qxF "$f"; then echo "dm/host/(宿主也编) $f"; else echo "$f"; fi; done)
  if [ -n "$CH" ]; then
    if ! printf '%s\n' "$CH" | grep -qvE '^(dm/ios/|docs/)|\.md$'; then SKIP="1 2 3 4 5 6 7 9 10"; SCOPE="改动只在 dm/ios 与文档"
    elif ! printf '%s\n' "$CH" | grep -qvE '^(dm/gfx/|dm/ios/|docs/)|\.md$'; then SKIP="1 2 5 6 7 9"; SCOPE="改动只在 dm/gfx（及 dm/ios、文档）"; fi
  fi
fi


if [ "${ACCEPT_ALL:-0}" != 1 ] && [ -n "${CH:-}" ] && ! printf '%s\n' "$CH" | grep -qE '^dm/gfx/|^dm/host/(front|app_mac|touch_mac|memstat)[^/]*$|^dm/host/Makefile$|^dm/verify/(shotgate|hostshot|accept)'; then M3GATE=0; fi
echo "SCOPE ${SCOPE}$([ -n "$SKIP" ] && echo "；跳过项序号 ${SKIP}（ACCEPT_ALL=1 强制全量）")；旧接口版画面闸门 $([ $M3GATE = 1 ] && echo 跑 || echo 本次改动不涉及，跳过)"
[ "${ACCEPT_SCOPE_ONLY:-0}" = 1 ] && exit 0


if make -s -C "$HERE/../host" > "$L/build.log" 2>&1; then
  echo "BUILD OK 宿主预构建完成，后续子项只做增量核对"
else
  echo "宿主预构建失败；运行子项未启动，现场保留" > "$L/KEEP"
  cat "$L/build.log"
  echo "ACCEPT RED 宿主预构建失败 日志 $L"
  exit 1
fi
item() {
  local k=$1 name=$2 want=$3 log=$4 kl=$5 kn=$6; shift 6; local s=$SECONDS
  case " $SKIP " in *" $k "*) printf '0\n| %s | %s | ⏭ 跳过：%s | 0 秒 |\n' "$name" "$want" "$SCOPE" > "$L/$k.row"; echo "⏭ ${name}（跳过：${SCOPE}）"; return 0;; esac
  "$@" > "$L/$log" 2>&1; local rc=$?
  local got=$(grep -E "$kl" "$L/$log" | tail -$kn | tr '\n' ' ') mark
  [ $rc = 0 ] && mark="✅" || mark="🔴"
  printf '%s\n| %s | %s | %s %s | %s 秒 |\n' "$rc" "$name" "$want" "$mark" "${got:-（无读数行，见 ${log}）}" "$((SECONDS - s))" > "$L/$k.row"
  echo "$mark ${name}（$((SECONDS - s)) 秒）"
}
item 0 "⓪ 启动块清单 bootlist.tsv" "存在、构建号与 main.cuts.tsv 一致、与仓库 dm/data/bootlist_<构建号>.tsv 逐字节相同、sha 自洽、块数 > 0、逐块与资源包一致" bootlist.log '^BOOTLIST' 1 "$HERE/bootchk.sh" "$OUT"


regress_both() {
  local rc=0 touch_out=${DM_TOUCH_OUT:-$(bash "$(dirname "$0")/../tools/product.sh" path touch)}
  "$HERE/regress.sh" "$OUT" || rc=1
  "$HERE/regress.sh" --struct-only "$touch_out" || rc=1
  return $rc
}
item 1 "① 改写等价回归 regress（+ 出包产物 touch 结构核对）" "被测产物三模块 STRUCT/EQUIV OK、GATEFN 全过；touch 三模块 STRUCT OK、GATEFN 全过；两行 REGRESS OK" regress.log '^(STRUCT|EQUIV|REGRESS|GATEFN)' 13 regress_both &
savecmp_gate() {
  bash "$HERE/../tools/host_savecmp.sh" "$TR" "$OUT"; local rc=$?
  local hd=$(sed -n 's/^局目录 //p' "$L/${1:-savecmp.log}" | head -1) r
  r=$("$HERE/logred.sh" "$hd/host.log"); local lrc=$?
  echo "HOSTLOG 宿主日志红线 $(echo "$r" | cut -f1) 行 $(echo "$r" | cut -f2)（$hd/host.log）"
  [ $lrc = 0 ] || { [ $lrc = 2 ] && echo "HOSTLOG 找不到宿主日志，判红"; rc=1; }




  local hit=$(grep -o 'DM_TACT_PREFETCH：命中 [0-9]*' "$hd/host.log" | tail -1 | grep -o '[0-9]*$')
  echo "PREFETCH 预取命中 ${hit:-无读数} 块（只报不判）"
  return $rc
}
item 2 "② 存档逐字节对拍 host_savecmp $(basename "$TR")" "逐拍 6 格一致、正对照一致、负对照不一致、宿主日志无红线" savecmp.log 'SELFCMP|^SAVECMP|结论|^HOSTLOG|^PREFETCH' 6 savecmp_gate savecmp.log &
gatebc() {
  local rc=0 rule_out=${DM_RULE_OUT:-$(bash "$(dirname "$0")/../tools/product.sh" path rules)} self_out=${DM_LOGICSELF_OUT:-$(bash "$(dirname "$0")/../tools/product.sh" path touch)}
  if [ ! -f "$HERE/rulegate.sh" ] || [ ! -f "$HERE/logicself.sh" ]; then
    echo "RULEGATE TEST FIXTURE 未装载脚本"
    echo "LOGICSELF TEST FIXTURE 未装载脚本"
    return 0
  fi
  bash "$HERE/rulegate.sh" 400 "$rule_out" || rc=1
  bash "$HERE/logicself.sh" "$self_out" 230 || rc=1
  return $rc
}
item 7 "②B/C 逻辑闸门 rulegate + logicself" "RULEGATE 400 逐拍六组一致；LOGICSELF 230 逐拍自身基线一致" logic_gate.log '^(RULEGATE|LOGICSELF)' 8 gatebc &


lag_gate() {
  savecmp_gate savecmp_lag.log; local rc=$?
  local hd=$(sed -n 's/^局目录 //p' "$L/savecmp_lag.log" | head -1)
  local inc=$(grep -o '下一拍时限 [0-9]* → [0-9]*' "$hd/host.log" | tail -1 | awk '{print $4 - $2}')
  if [ -n "$inc" ] && [ "$inc" -ge 200 ]; then echo "LAGRANGE 积压已造出：下一拍时限增量 ${inc}（≥ 200）"; else echo "LAGRANGE 空跑：下一拍时限增量 ${inc:-无读数}（须 ≥ 200）"; rc=1; fi
  return $rc
}
item 5 "⑤ 补拍积压下冻结拍确定性 DM_TEST_LAG=400 $(basename "$TR")" "冻结拍 401（目标 401）不带【红】、正对照一致、负对照不一致、宿主日志无红线、积压射程增量 ≥ 200" savecmp_lag.log 'SELFCMP|^SAVECMP|结论|^HOSTLOG|^LAGRANGE|^PREFETCH' 7 env DM_TEST_LAG=400 bash -c "$(declare -f savecmp_gate lag_gate); HERE=$HERE TR=$TR OUT=$OUT L=$L lag_gate" &
func_cases() {
  local case_dir name total green=0 status elapsed note log detail reported
  local cases=() pending=()
  while IFS= read -r -d '' case_dir; do
    [ -f "$case_dir/script.txt" ] && [ -f "$case_dir/assert.py" ] || continue


    if [ -f "$case_dir/pending" ]; then pending+=("$(basename "$case_dir")：$(head -1 "$case_dir/pending")"); continue; fi
    cases+=("$(basename "$case_dir")")
  done < <(find "$HERE/touchcases" -mindepth 1 -maxdepth 1 -type d -print0 | sort -z)
  total=${#cases[@]}
  if [ "$total" -eq 0 ]; then
    echo 'FUNC 0/0'
    return 1
  fi






  python3 "$HERE/../ios/tools/gen_romfs_overlay.py" "${DM_ROMFS_SRC:-${DM_ROOT:-$HOME/dm-work}/data/switch/3.3.94012/Update/RomFS}" "$L/romfs_overlay" > /dev/null 2>"$L/func_overlay.err" || { echo "FUNCFAIL 界面覆盖层生成失败：$(tail -1 "$L/func_overlay.err")"; echo 'FUNC 0/0'; return 1; }
  export DM_ROMFS_OVERLAY="$L/romfs_overlay"
  local sem="$L/func.sem.$$"; mkfifo "$sem" || return 1; exec 8<>"$sem"; rm -f "$sem"
  for _ in $(seq 1 "${FUNC_JOBS:-2}"); do echo >&8; done
  for name in "${cases[@]}"; do
    read -r -u 8
    (
      log="$L/func_${name}.log"
      local started=$SECONDS rc=0
      timeout 120s env DM_TOUCH_RUNROOT="$L/touchchain/$name" bash "$HERE/touchchain.sh" "$name" > "$log" 2>&1 || rc=$?
      reported=$(awk '/^TOUCHCHAIN / {print $(NF - 1)}' "$log" | tail -1)
      elapsed=${reported:-$((SECONDS - started))}
      status=RED
      [ "$rc" -eq 0 ] && [ "$elapsed" -le "${FUNC_LIMIT_S:-60}" ] && status=OK
      printf '%s %s\n' "$status" "$elapsed" > "$L/func_${name}.row"
      echo >&8
    ) &
  done
  wait; exec 8>&-
  for name in "${cases[@]}"; do
    read -r status elapsed < "$L/func_${name}.row"
    note=''
    [ "$elapsed" -gt 20 ] && note='（慢：> 20 s，本机负载高？）'; [ "$elapsed" -gt "${FUNC_LIMIT_S:-60}" ] && note="（超卡死兜底 ${FUNC_LIMIT_S:-60} s）"
    printf 'FUNCCASE %s %s %s 秒%s\n' "$name" "$status" "$elapsed" "$note"
    if [ "$status" = OK ]; then
      green=$((green + 1))
    else
      log="$L/func_${name}.log"
      detail=$(grep -E '局目录：|断言失败：' "$log" | tail -2 | tr '\n' ' ')
      [ -n "$detail" ] || detail="日志：$log"
      printf 'FUNCFAIL %s %s\n' "$name" "$detail"
    fi
  done
  printf 'FUNC %s/%s\n' "$green" "$total"
  [ ${#pending[@]} -eq 0 ] || { printf 'FUNCPENDING 待接入 %s 例：' "${#pending[@]}"; printf '%s；' "${pending[@]}"; echo; }
  [ "$green" -eq "$total" ]
}
item 9 "⑨ 功能用例 touchchain" "自动收集 script.txt + assert.py 用例；各例并行；断言通过（墙钟 60 s 只作卡死兜底）" func.log '^(FUNCCASE|FUNC|FUNCFAIL|FUNCPENDING)' 1000 func_cases &
PAGES=()
[ "$MODE" = fast ] && PAGES=(camp_e stash wild charselect skilltree)
PAGE_NAMES=${PAGES[*]:-$(cd "$HERE/shotgate_pages" && ls *.json | sed 's/\.json$//' | tr '\n' ' ')}


m3_shotgate() {
  local name="⑪ 旧接口版画面闸门 shotgate ${MODE}（DM_NVN_M3=1，${PAGE_NAMES}）" want="旧接口版宿主每页对真值绿"
  if [ $M3GATE != 1 ]; then printf '0\n| %s | %s | ⏭ 旧接口版画面闸门：本次改动不涉及，跳过 | 0 秒 |\n' "$name" "$want" > "$L/11.row"; echo "⏭ ${name}（旧接口版画面闸门：本次改动不涉及，跳过）"; return 0; fi
  case " $SKIP " in *" 3 "*) printf '0\n| %s | %s | ⏭ 跳过：%s | 0 秒 |\n' "$name" "$want" "$SCOPE" > "$L/11.row"; echo "⏭ ${name}（跳过：${SCOPE}）"; return 0;; esac
  local s=$SECONDS rc=0
  { make -s -C "$HERE/../host" dmhost_m3 && DM_HOST="$HERE/../host/dmhost_m3" "$HERE/shotgate.sh" "$OUT" ${PAGES[@]+"${PAGES[@]}"}; } > "$L/shotgate_m3.log" 2>&1 || rc=1
  local got=$(grep -E 'SHOTGATE_ALL' "$L/shotgate_m3.log" | tail -1) mark; [ $rc = 0 ] && mark="✅" || mark="🔴"
  printf '%s\n| %s | %s | %s %s | %s 秒 |\n' "$rc" "$name" "$want" "$mark" "${got:-（无读数行，见 shotgate_m3.log）}" "$((SECONDS - s))" > "$L/11.row"; echo "$mark ${name}（$((SECONDS - s)) 秒）"
}
{ item 3 "③ 画面闸门 shotgate ${MODE}（${PAGE_NAMES}）" "每页对真值绿（结构 / 非黑 / 网格 / 命名区域 / 应有元素 / 宿主日志无【红】）" shotgate.log 'SHOTGATE_ALL' 1 "$HERE/shotgate.sh" "$OUT" ${PAGES[@]+"${PAGES[@]}"}; m3_shotgate; } &
unit_tests() {
  local rc=0
  "${DM_ROOT:-$HOME/dm-work}/build/cache/dm/venv/bin/python" "$HERE/ryu/test_selfcmp.py" > "$L/unit_selfcmp.log" 2>&1 && echo "UNIT selfcmp 通过" || { echo "UNIT selfcmp 失败（见 unit_selfcmp.log）"; rc=1; }
  "${DM_ROOT:-$HOME/dm-work}/build/cache/dm/venv/bin/python" "$HERE/ryu/test_plan.py" > "$L/unit_plan.log" 2>&1 && echo "UNIT plan 通过" || { echo "UNIT plan 失败（见 unit_plan.log）"; rc=1; }
  make -j8 -C "$HERE/../host/tests" run > "$L/unit_host.log" 2>&1 && echo "UNIT host 通过" || { echo "UNIT host 失败：$(grep -m1 -iE 'error|fail|失败|红' "$L/unit_host.log" | cut -c1-140)"; rc=1; }
  make -j8 -C "$HERE/../gfx/tests" run > "$L/unit_gfx.log" 2>&1 && make -j8 -C "$HERE/../gfx/tests" run-m3 >> "$L/unit_gfx.log" 2>&1 && echo "UNIT gfx 通过" || { echo "UNIT gfx 失败：$(grep -m1 -iE 'error|fail|失败|红|undefined' "$L/unit_gfx.log" | cut -c1-140)"; rc=1; }


  make -C "$HERE/../ios/tests" test > "$L/unit_ios.log" 2>&1 && echo "UNIT ios 通过" || { echo "UNIT ios 失败：$(grep -m1 -E 'FAIL|Error|error' "$L/unit_ios.log" | cut -c1-140)"; rc=1; }


  (cd "$HERE" && python3 -m unittest -q test_accept) > "$L/unit_accept.log" 2>&1 && echo "UNIT accept 通过" || { echo "UNIT accept 失败：$(grep -m1 -E '^(FAIL|ERROR):' "$L/unit_accept.log" | cut -c1-140)"; rc=1; }
  return $rc
}
item 4 "④ 单测 selfcmp + plan + make -C dm/host/tests run + dm/gfx/tests run + dm/ios/tests test + test_accept" "六套全过、无构建 / 链接错误" unit.log '^UNIT' 6 unit_tests &
ios_build_gate() {
  local s=$SECONDS raw="$L/iosbuild-command.log"
  DM_NOINSTALL=1 DM_IOS_DD="$HERE/../../.iosdd" bash "$HERE/../ios/run.sh" build > "$raw" 2>&1
  local rc=$? elapsed=$((SECONDS - s))
  if [ "$rc" = 0 ]; then
    echo "IOSBUILD OK 用时 $elapsed 秒"
  else
    echo "IOSBUILD RED 用时 $elapsed 秒"
    grep -m1 -E 'error:' "$raw" || true
  fi
  return "$rc"
}
item 8 "⑧ iOS 构建（只编不装）" "DM_NOINSTALL=1 工作树派生目录构建成功；首个 error 可见" iosbuild.log '^IOSBUILD (OK|RED)' 2 ios_build_gate &


item 10 "⑩ 局内切运行模式 opmode" "切换前已进局；取走通知 30/31；活到 timeout 无 CRASH；切换后 ≥ 300 帧；宿主日志无红线" opmode.log '^OPMODE (OK|RED)' 1 bash "$HERE/opmode.sh" "$OUT" &


[ "$MODE" = full ] && item 6 "⑥ 存档事务 save_txn_regress" "4/4 注入命中；已提交文件同期望版；校验和自洽；残缺工作目录 / 拒绝日志负对照成立" save_txn.log '^✓|^✗|^结论|^SAVE_TXN' 6 bash "$HERE/../tools/save_txn_regress.sh" "$OUT" &
wait
allrc=0; rows=()
ITEMS=(0 1 2 3 11 4 5 7 8 9 10); [ "$MODE" = full ] && ITEMS+=(6)
for k in "${ITEMS[@]}"; do { read -r rc; read -r row; } < "$L/$k.row"; [ "$rc" = 0 ] || allrc=1; rows+=("$row"); done
[ "$allrc" = 0 ] || echo "ACCEPT $MODE 红；本目录保留，不删不覆盖" > "$L/KEEP"
echo; echo "| 项 | 期望 | 真结果 | 用时 |"; echo "|---|---|---|---|"; printf '%s\n' "${rows[@]}"
echo; echo "ACCEPT $([ $allrc = 0 ] && echo OK || echo RED) 档位 $MODE 产物 $(basename "$OUT") 总用时 $((SECONDS - T0)) 秒 日志 $L"
exit $allrc
