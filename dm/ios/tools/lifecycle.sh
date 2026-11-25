#!/bin/bash




set -u
cd "$(dirname "$0")/.."
source tools/local_env.sh && local_env_need DM_DEVICE DM_IOS_UDID || exit 1
D=$DM_DEVICE; UDID=$DM_IOS_UDID; B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
TREE=$(git rev-parse --show-toplevel); DD=$TREE/.iosdd_ui; R=${DM_ROOT:-$HOME/dm-work}/run/ios/lifecycle/$(date +%m%d_%H%M%S)_$$; mkdir -p "$R"
t(){ echo "[$(date +%T)] $*"; }
MODE=${1:-ui}
if [ "$MODE" = sim ]; then
  DM_IOS_DD="$TREE/.iosdd" bash run.sh build > "$R/build.log" 2>&1 || { t "🔴 构建失败"; grep -E "error:" "$R/build.log" | head -5; echo "LIFECYCLE RED"; exit 1; }
  source tools/devlock.sh; devlock_acquire "${SMOKE_OWNER:-未署名} 生命周期（应用内模拟）"; trap devlock_release EXIT
  pidof(){ timeout 30 xcrun devicectl device info processes --device $D 2>/dev/null | grep 'DmIOS.app/DmIOS' | awk '{print $1}' | tr '\n' ' '; }
  for p in $(pidof); do timeout 30 xcrun devicectl device process terminate --device $D --pid $p > /dev/null 2>&1; done; sleep 2
  DM_IOS_DD="$TREE/.iosdd" bash run.sh install > "$R/install.log" 2>&1 || { t "🔴 装机失败"; tail -3 "$R/install.log"; echo "LIFECYCLE RED"; exit 1; }
  KEYS='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,C0.84:0.208@OpenPanel:CharacterSelectPanel%1>UpdateCharacterInfo,MSG:CharacterSelect:LoadCharacter:0@UpdateCharacterInfo%0.3>FrontEndNavigation:ToGame'
  t "拉起（进局后第 25 秒起模拟后台 30 秒）"
  DEVICECTL_CHILD_DM_SAVE_FRESH=warlock DEVICECTL_CHILD_DM_AUDIO=1 DEVICECTL_CHILD_DM_TEST_BG=25,30 DEVICECTL_CHILD_DM_UIMSG=1 DEVICECTL_CHILD_DM_KEYS="$KEYS" timeout -k 5 30 xcrun devicectl device process launch --device $D $B -mode boot > "$R/launch.txt" 2>&1
  sleep 78; P0=$(pidof); trc=0; [ -n "$P0" ] || trc=1
elif [ "$MODE" = switcher ]; then
  DM_GEN_ONLY=1 DM_IOS_DD="$DD" bash run.sh build > "$R/gen.log" 2>&1 || { t "🔴 生成失败"; tail -3 "$R/gen.log"; echo "LIFECYCLE RED"; exit 1; }
  source tools/asc_env.sh; asc_auth
  xcodebuild build-for-testing -project DmIOS.xcodeproj -scheme DmIOS -configuration Release -destination "platform=iOS,id=$UDID" -derivedDataPath "$DD" -allowProvisioningUpdates ${ASC_AUTH[@]+"${ASC_AUTH[@]}"} > "$R/build.log" 2>&1 \
    || { t "🔴 构建失败"; grep -E "error:" "$R/build.log" | head -5; echo "LIFECYCLE RED"; exit 1; }
  source tools/devlock.sh; devlock_acquire "${SMOKE_OWNER:-未署名} 生命周期（多任务界面划掉）"; trap devlock_release EXIT
  pull(){ timeout 60 xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B --source "Documents/$1" --destination "$R/$2" > /dev/null 2>&1; }
  t "第 1 局：进局 → 捡 r30 → 多任务界面停 3 秒 → 结束进程"
  xcodebuild test-without-building -project DmIOS.xcodeproj -scheme DmIOS -configuration Release -destination "platform=iOS,id=$UDID" -derivedDataPath "$DD" -only-testing:TouchSmokeTests/LifecycleSmoke/testSwitcherKill > "$R/test1.log" 2>&1
  pull boot.txt kill_boot.txt || { t "🔴 拉第 1 局日志失败"; echo "LIFECYCLE RED"; exit 1; }
  t "第 2 局：重进同一角色、记物品表"
  xcodebuild test-without-building -project DmIOS.xcodeproj -scheme DmIOS -configuration Release -destination "platform=iOS,id=$UDID" -derivedDataPath "$DD" -only-testing:TouchSmokeTests/LifecycleSmoke/testReenter > "$R/test2.log" 2>&1
  pull boot.txt re_boot.txt; pull lc_state.jsonl re_state.jsonl
  devlock_release; trap - EXIT
  K="$R/kill_boot.txt"; grep -a "前端：scene\|前端：失活\|进后台等游戏存盘\|存档提交 #\|DM_TEST_GIVE" "$K" | sed 's#/[^ ]*/Documents/#Documents/#g' | cut -c1-160
  cls=$(grep -a "DM_TEST_GIVE：.*「r30」" "$K" | grep -o "物品类号 [0-9]*" | head -1 | grep -o "[0-9]*$")
  gt=$(grep -an "DM_TEST_GIVE：.*「r30」" "$K" | head -1 | cut -d: -f1); ct=$(grep -an "存档提交 #" "$K" | tail -1 | cut -d: -f1)
  have=$( [ -n "$cls" ] && [ -s "$R/re_state.jsonl" ] && tail -1 "$R/re_state.jsonl" | grep -c "\"cls\":$cls," || echo 0)
  echo "r30 物品类号 ${cls:-无}；第 1 局最后一次存档提交在 r30 之后：$([ -n "$gt" ] && [ -n "$ct" ] && [ "$ct" -gt "$gt" ] && echo 是 || echo 否)；第 2 局物品表里有它：$([ "${have:-0}" -gt 0 ] && echo 有 || echo 没有)"
  [ "${have:-0}" -gt 0 ] && echo "LIFECYCLE OK" || echo "LIFECYCLE RED"; exit 0
else
DM_GEN_ONLY=1 DM_IOS_DD="$DD" bash run.sh build > "$R/gen.log" 2>&1 || { t "🔴 生成失败"; tail -3 "$R/gen.log"; echo "LIFECYCLE RED"; exit 1; }
source tools/asc_env.sh; asc_auth
xcodebuild build-for-testing -project DmIOS.xcodeproj -scheme DmIOS -configuration Release -destination "platform=iOS,id=$UDID" -derivedDataPath "$DD" -allowProvisioningUpdates ${ASC_AUTH[@]+"${ASC_AUTH[@]}"} > "$R/build.log" 2>&1 \
  || { t "🔴 构建失败"; grep -E "error:" "$R/build.log" | head -5; echo "LIFECYCLE RED"; exit 1; }
source tools/devlock.sh; devlock_acquire "${SMOKE_OWNER:-未署名} 生命周期"; trap devlock_release EXIT
t "跑界面测试（约 90 秒）"
xcodebuild test-without-building -project DmIOS.xcodeproj -scheme DmIOS -configuration Release -destination "platform=iOS,id=$UDID" -derivedDataPath "$DD" -only-testing:TouchSmokeTests/LifecycleSmoke > "$R/test.log" 2>&1; trc=$?
grep -E "Test Case.*(passed|failed)|error:|XCTAssert" "$R/test.log" | tail -4
fi
timeout 60 xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B --source Documents/boot.txt --destination "$R/boot.txt" > "$R/pull.log" 2>&1 || { t "🔴 拉 boot.txt 失败"; echo "LIFECYCLE RED"; exit 1; }
L="$R/boot.txt"; bad=0
chk(){ if [ "$2" = 1 ]; then echo "✓ $1"; else echo "✗ $1"; bad=1; fi; }
bg=$(grep -n "前端：sceneDidEnterBackground" "$L" | head -1 | cut -d: -f1); fg=$(grep -n "前端：sceneWillEnterForeground" "$L" | tail -1 | cut -d: -f1)
chk "同一份日志里先进后台、后回前台（进程没重开）：行 ${bg:-无} → ${fg:-无}" $([ -n "$bg" ] && [ -n "$fg" ] && [ "$fg" -gt "$bg" ] && echo 1 || echo 0)
chk "进后台时图形已挂起" $(grep -q "图形挂起：在途 GPU 工作已清空" "$L" && echo 1 || echo 0)
cb=$(grep "前端：sceneDidEnterBackground" "$L" | head -1 | sed -E 's/.*客体时钟 ([0-9.]+) s.*/\1/'); cf=$(grep "前端：sceneWillEnterForeground" "$L" | tail -1 | sed -E 's/.*客体时钟 ([0-9.]+) s.*/\1/')
chk "后台那段没算进游戏时间：客体时钟 进后台 ${cb:-无} s → 回前台 ${cf:-无} s（差 < 1 s）" $(python3 -c "print(1 if abs(float('${cf:-1e9}')-float('${cb:-0}'))<1.0 else 0)" 2>/dev/null || echo 0)
ab=$(grep "前端：sceneWillEnterForeground" "$L" | tail -1 | sed -E 's/.*声卡回调累计 ([0-9]+).*/\1/'); ae=$(grep -o "声卡回调 [0-9]*\|音频回调 [0-9]*" "$L" | tail -1 | grep -o "[0-9]*$")
post=$([ -n "$fg" ] && tail -n +"$fg" "$L" || true)
chk "回前台后音频恢复行出现" $(echo "$post" | grep -q "audio 恢复" && echo 1 || echo 0)
up=$(echo "$post" | grep "上屏 至帧" | tail -1); chk "回前台后仍在上屏：${up:-无上屏行}" $([ -n "$up" ] && echo 1 || echo 0)















lg=$(tail -n +"${fg:-1}" "$L" | python3 -c '
import re, sys
P = sys.stdin.read().split("\n")
t = [tuple(map(int, m.groups())) for l in P for m in [re.search(r"显示节拍 至 (\d+)：节拍 \d+、出图 \d+、空节拍 (\d+)、合并丢弃旧帧 (\d+)", l)] if m]
u = [tuple(map(int, m.groups())) for l in P for m in [re.search(r"上屏 至帧 \d+：\d+ 帧，掉帧 (\d+)（丢弃 (\d+)）", l)] if m]
n = max(len(t) - 1, 0); de = t[-1][1] - t[0][1] if t else -1; dd = t[-1][2] - t[0][2] if t else -1
print(u[-1][0] - u[-1][1] if u else -1, u[-1][1] if u else -1, n, de, dd)')
set -- $lg
chk "回前台那一窗上了屏的帧没有间隔超 25 ms 的：${1:-无读数} 帧" $([ "${1:--1}" = 0 ] && echo 1 || echo 0)
chk "回前台后完整显示节拍窗 ≥ 2 个且空节拍、丢弃都不新增：${3:-0} 窗（每窗 300 节拍），空节拍 +${4:-?}，丢弃 +${5:-?}" $([ "${3:-0}" -ge 2 ] && [ "${4:--1}" = 0 ] && [ "${5:--1}" = 0 ] && echo 1 || echo 0)
chk "进后台那一刻丢弃的帧数在上限内（只防失控，≤ 12）：${2:-无读数} 帧" $([ "${2:--1}" -ge 0 ] && [ "${2:-99}" -le 12 ] && echo 1 || echo 0)
chk "回前台后 3D 比例没有因此降档" $(echo "$post" | grep -q "3D 渲染比例 0.7[02]" && echo 0 || echo 1)
chk "红行 0（实得 $(grep -c '【红】' "$L")）" $(grep -q '【红】' "$L" && echo 0 || echo 1)
chk "动作序列做完、进程还在（退出码 / 存活标志 ${trc}）" $([ "$trc" = 0 ] && echo 1 || echo 0)
chk "模拟回前台后 2 秒声卡回调有增量：$(grep -o '测试回前台后 2 s 声卡回调 +[0-9]*' "$L" | tail -1)" $([ "$MODE" != sim ] || grep -qE '测试回前台后 2 s 声卡回调 \+[1-9]' "$L" && echo 1 || echo 0)
[ "$MODE" = sim ] && for p in $(pidof); do timeout 30 xcrun devicectl device process terminate --device $D --pid $p > /dev/null 2>&1; done
echo "日志 $R"; [ $bad = 0 ] && echo "LIFECYCLE OK" || echo "LIFECYCLE RED"; exit $bad
