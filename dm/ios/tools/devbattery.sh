

















DEVBATTERY_LOW=${DEVBATTERY_LOW:-30}; DEVBATTERY_RESUME=${DEVBATTERY_RESUME:-60}; DEVBATTERY_CHARGING_MIN=${DEVBATTERY_CHARGING_MIN:-20}
DEVBATTERY_DIR=${DEVBATTERY_DIR:-${DM_ROOT:-$HOME/dm-work}/run/ios}
devbattery_read() {
  if [ -n "${DEVBATTERY_FAKE:-}" ]; then echo "$DEVBATTERY_FAKE"; return 0; fi
  local u=${DM_IOS_UDID:-} out
  [ -n "$u" ] || { source "$(dirname "${BASH_SOURCE[0]}")/local_env.sh" 2>/dev/null; u=${DM_IOS_UDID:-}; }
  [ -n "$u" ] || return 2


  local tb; tb=$(command -v timeout || command -v gtimeout) || return 3
  command -v ideviceinfo > /dev/null || return 3
  out=$("$tb" 20 ideviceinfo -u "$u" -q com.apple.mobile.battery 2>/dev/null) || out=$("$tb" 20 ideviceinfo -n -u "$u" -q com.apple.mobile.battery 2>/dev/null) || {




    local cf=$DEVBATTERY_DIR/app_battery_$u.txt p s
    if [ -s "$cf" ] && [ $(( $(date +%s) - $(stat -f %m "$cf") )) -le "${DM_DEVBATTERY_APP_MAX_AGE_S:-1800}" ] && read -r p s < "$cf" && [ "${p:--1}" -ge 0 ] 2>/dev/null; then
      echo "$p $([ "$s" = 充电中 ] && echo 1 || echo 0) $([ "$s" = 充电中 ] || [ "$s" = 已充满 ] && echo 1 || echo 0) $([ "$s" = 已充满 ] && echo 1 || echo 0)"; return 0; fi
    [ "${DM_DEVBATTERY_INRUN:-0}" = 1 ] && return 6; return 1; }
  awk '/^BatteryCurrentCapacity:/{p=$2} /^BatteryIsCharging:/{c=($2=="true")} /^ExternalConnected:/{e=($2=="true")} /^FullyCharged:/{f=($2=="true")} END{ if (p=="") exit 1; print p, c+0, e+0, f+0 }' <<<"$out"
}
devbattery_gate() {
  mkdir -p "$DEVBATTERY_DIR"; local flag=$DEVBATTERY_DIR/device_low_battery log=$DEVBATTERY_DIR/battery.log r p c e f now; now=$(date '+%m-%d %H:%M:%S')
  r=$(devbattery_read); local rc=$?
  if [ $rc = 2 ]; then
    echo "$now 没配 UDID" >> "$log"; DEVBATTERY_LINE="电量 没配 UDID"
    echo "【红】没配测试机 UDID（本机配置 ~/.config/dm-ios/local.env 的 DM_IOS_UDID 为空），读不了电量，这局不跑" >&2; devbattery_notify "没配测试机 UDID，真机局已拒绝"; return 1; fi
  if [ $rc = 3 ]; then
    echo "$now 本机缺工具" >> "$log"; DEVBATTERY_LINE="电量 读不了：本机缺 timeout 或 ideviceinfo"
    echo "【红】本机缺读电量要用的命令：timeout（brew install coreutils）$(command -v timeout gtimeout > /dev/null && echo 有 || echo 没有)，ideviceinfo（brew install libimobiledevice）$(command -v ideviceinfo > /dev/null && echo 有 || echo 没有)。与设备无关，装好再跑" >&2; return 3; fi
  if [ $rc = 6 ]; then
    echo "$now USB 读不到、无新鲜 App 读数，交局内判" >> "$log"; DEVBATTERY_LINE="电量 USB 读不到（网络连接），交执行器局内判：开局 ≥ ${DEVBATTERY_RESUME}%、局中 < ${DEVBATTERY_LOW}% 停局"
    [ -f "$flag" ] && { echo "【红】测试机电量暂停中（标记 ${flag}：$(head -1 "$flag")），没有新读数确认回到 ${DEVBATTERY_RESUME}%，这局不跑" >&2; return 1; }
    return 0; fi
  if [ $rc != 0 ]; then
    echo "$now 读不到" >> "$log"; DEVBATTERY_LINE="电量 读不到"
    echo "【红】测试机电量读不到（ideviceinfo 不通，设备多半离线或没电关机），这局不跑" >&2; devbattery_notify "测试机电量读不到，真机局已拒绝"; return 1; fi
  read -r p c e f <<<"$r"; f=${f:-0}
  DEVBATTERY_LINE="电量 ${p}%（$([ "$c" = 1 ] && echo 充电 || echo 未充电)，电源 $([ "$e" = 1 ] && echo 有 || echo 无)）"
  echo "$now $p% 充电=$c 电源=$e 充满=$f" >> "$log"
  if [ "$e" = 1 ] && [ "$c" = 1 ]; then   # 接电且在充：只看当场读数，不看暂停标记 | On external power and charging: only the current reading counts, not the pause marker
    if [ "$p" -ge "$DEVBATTERY_CHARGING_MIN" ]; then [ -f "$flag" ] && { rm -f "$flag"; echo "测试机接电充电中、电量 ${p}%（≥ ${DEVBATTERY_CHARGING_MIN}%）：撤掉暂停标记，真机局放行" >&2; }; return 0; fi
    echo "【红】测试机充电中但电量 ${p}% 低于 ${DEVBATTERY_CHARGING_MIN}%：这局不跑" >&2; return 1
  fi
  if [ -f "$flag" ]; then
    if [ "$p" -ge "$DEVBATTERY_RESUME" ]; then rm -f "$flag"; echo "测试机电量回到 ${p}%（≥ ${DEVBATTERY_RESUME}%），撤掉暂停标记，真机局放行" >&2
    else echo "【红】测试机电量暂停中：现 ${p}%，充到 ${DEVBATTERY_RESUME}% 才放行（标记 ${flag}：$(head -1 "$flag")）" >&2; return 1; fi
  elif [ "$p" -lt "$DEVBATTERY_LOW" ]; then
    echo "$now 电量 ${p}% 充电=$c 电源=$e" > "$flag"
    echo "【红】测试机电量 ${p}% 低于 ${DEVBATTERY_LOW}%：暂停全部真机局，充到 ${DEVBATTERY_RESUME}% 再放行（标记 ${flag}）" >&2
    devbattery_notify "测试机电量 ${p}%，真机局已暂停，充到 ${DEVBATTERY_RESUME}% 放行"; return 1
  fi
  [ "$e" = 1 ] && [ "$c" = 0 ] && [ "$f" = 0 ] && [ "$p" -lt "${DEVBATTERY_NOCHARGE:-90}" ] && echo "【红】测试机插着电源却没在充（${p}%）：供电跟不上，局照跑，请换充电头 / 线" >&2
  return 0
}
devbattery_notify() {   # 记进 battery.log 与 device_alert.txt（巡检读这个文件；不用系统通知：osascript 首次会弹权限框） | Record into battery.log and device_alert.txt (the patrol reads this file; system notifications are not used: osascript pops a permission dialog the first time)
  echo "$(date '+%m-%d %H:%M:%S') 通知：$1" | tee -a "$DEVBATTERY_DIR/battery.log" >> "$DEVBATTERY_DIR/device_alert.txt"
}
