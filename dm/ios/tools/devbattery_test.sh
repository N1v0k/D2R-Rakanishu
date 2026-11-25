#!/bin/bash


cd "$(dirname "$0")"; T=$(mktemp -d); export DEVBATTERY_DIR=$T;
: > $T/empty.env
 source ./devbattery.sh; bad=0
ck() { local want=$1 got; shift; DEVBATTERY_FAKE="$*" devbattery_gate 2>/dev/null; got=$?; [ "$got" = "$want" ] && echo "通 $* → $got" || { echo "不通 $* → ${got}（期望 ${want}）"; bad=1; }; }
ck 0 80 0 1 1
ck 1 25 0 1 0
[ -f $T/device_low_battery ] && echo "通 低电落标记" || { echo "不通 低电没落标记"; bad=1; }
ck 1 45 0 1 0
ck 0 60 0 1 0
[ ! -f $T/device_low_battery ] && echo "通 撤标记" || { echo "不通 标记没撤"; bad=1; }
ck 0 45 0 1 0
ck 1 25 0 0 0
ck 0 25 1 1 0
[ ! -f $T/device_low_battery ] && echo "通 充电中放行撤标记" || { echo "不通 充电中放行却没撤标记"; bad=1; }
ck 1 15 1 1 0
ck 1 15 0 0 0
ck 1 45 0 1 0
ck 0 20 1 1 0


E=$T/emptybin; mkdir -p $E; for c in date mkdir dirname cat head awk; do ln -s "$(command -v $c)" $E/$c; done
( PATH=$E DM_IOS_UDID=fake; unset DEVBATTERY_FAKE; devbattery_gate > $T/miss.out 2>&1; echo $? > $T/miss.rc )
[ "$(cat $T/miss.rc)" = 3 ] && grep -q "本机缺读电量要用的命令" $T/miss.out && ! grep -q "离线" $T/miss.out && echo "通 缺 timeout → 3，报缺命令不报离线" || { echo "不通 缺 timeout → $(cat $T/miss.rc)：$(head -1 $T/miss.out)"; bad=1; }
out=$(DEVBATTERY_FAKE="50 0 1 0" devbattery_gate 2>&1); case "$out" in *供电跟不上*) echo "通 插电不充记红";; *) echo "不通 插电不充没记红"; bad=1;; esac
out=$(DEVBATTERY_FAKE="99 0 1 0" devbattery_gate 2>&1); case "$out" in *供电跟不上*) echo "不通 99% 停充被当成供电跟不上"; bad=1;; *) echo "通 99% 停充不记红";; esac
DEVBATTERY_FAKE= DM_IOS_UDID=00000000-0000000000000000 devbattery_gate 2>/dev/null && { echo "不通 读不到却放行"; bad=1; } || echo "通 读不到拒"
grep -q '读不到' $T/battery.log && echo "通 读不到记「读不到」" || { echo "不通 读不到没记对"; bad=1; }
DEVBATTERY_FAKE= DM_IOS_UDID= DM_LOCAL_ENV=$T/empty.env HOME=$T devbattery_gate 2>/dev/null && { echo "不通 没配 UDID 却放行"; bad=1; } || echo "通 没配 UDID 拒"
grep -q '没配 UDID' $T/battery.log && echo "通 没配 UDID 记「没配 UDID」" || { echo "不通 没配 UDID 没分开记"; bad=1; }


F=$T/fbin; mkdir -p $F; printf '#!/bin/sh\nexit 1\n' > $F/ideviceinfo; chmod +x $F/ideviceinfo; U=00000000-1111; rm -f $T/device_low_battery
ab() { local want=$1 got; shift; ( PATH=$F:$PATH DM_IOS_UDID=$U; unset DEVBATTERY_FAKE; export "$@"; devbattery_gate > /dev/null 2>&1 ); got=$?; [ "$got" = "$want" ] && echo "通 App 读数 $* → $got" || { echo "不通 App 读数 $* → ${got}（期望 ${want}）"; bad=1; }; }
echo "75 放电" > $T/app_battery_$U.txt; ab 0 X=1
echo "25 放电" > $T/app_battery_$U.txt; ab 1 X=1
[ -f $T/device_low_battery ] && echo "通 App 读数低电落标记" || { echo "不通 App 读数低电没落标记"; bad=1; }
touch -t 202001010000 $T/app_battery_$U.txt; ab 1 DM_DEVBATTERY_INRUN=1
rm -f $T/device_low_battery; ab 0 DM_DEVBATTERY_INRUN=1
ab 1 X=1
grep -q 通知 $T/device_alert.txt && echo "通 告警文件有记录" || { echo "不通 告警文件没记录"; bad=1; }


L=$T/device.lock; export DEVBATTERY_DIR=$T
DM_DEVBATTERY=1 DEVBATTERY_FAKE="80 0 1 0" DM_DEVLOCK_DIR=$L bash -c 'source ./devlock.sh; devlock_acquire 单测 2>/dev/null; [ -d "$DEVLOCK_DIR" ] && devlock_release' && echo "通 devlock 电量够取到锁" || { echo "不通 devlock 电量够没取到"; bad=1; }
DM_DEVBATTERY=1 DEVBATTERY_FAKE="10 0 1 0" DM_DEVBATTERY_WAIT_S=2 DM_DEVBATTERY_POLL_S=1 DM_DEVLOCK_DIR=$L bash -c 'source ./devlock.sh; devlock_acquire 单测 2>/dev/null; echo 不该走到这' >/dev/null; r=$?
[ $r = 4 ] && [ ! -d $L ] && echo "通 devlock 电量低退出 4 且没取锁" || { echo "不通 devlock 电量低 → $r"; bad=1; }


L2=$T/device2.lock; rm -f $T/device_low_battery
( unset DM_DEVBATTERY; DM_DEVBATTERY_LOCKS="$T/x.lock $L2" DEVBATTERY_FAKE="25 0 1 0" DM_DEVBATTERY_WAIT_S=1 DM_DEVBATTERY_POLL_S=1 DM_DEVLOCK_DIR=$L2 bash -c 'source ./devlock.sh; devlock_acquire 单测 2>/dev/null; echo 不该走到这' >/dev/null ); r=$?
[ $r = 4 ] && [ ! -d $L2 ] && [ -f $T/device_low_battery ] && echo "通 名单内锁 不在充 25% → 暂停（退出 4、落标记）" || { echo "不通 名单内锁 不在充 25% → $r"; bad=1; }
( unset DM_DEVBATTERY; DM_DEVBATTERY_LOCKS="$T/x.lock" DEVBATTERY_FAKE="25 0 1 0" DM_DEVLOCK_DIR=$L2 bash -c 'source ./devlock.sh; devlock_acquire 单测 2>/dev/null; [ -d "$DEVLOCK_DIR" ] && devlock_release' ) && echo "通 名单外锁不过闸" || { echo "不通 名单外锁被闸住"; bad=1; }


DM_DEVBATTERY=0 DEVBATTERY_FAKE="10 0 1 0" DM_DEVBATTERY_WAIT_S=1 DM_DEVBATTERY_POLL_S=1 DM_DEVLOCK_DIR=$L bash -c 'source ./devlock.sh; devlock_acquire 单测 2>/dev/null; [ -d "$DEVLOCK_DIR" ] && devlock_release' && echo "通 DM_DEVBATTERY=0 电量低也取到锁" || { echo "不通 DM_DEVBATTERY=0 仍被闸住"; bad=1; }
rm -rf "$T"; [ $bad = 0 ] && echo "devbattery 单测全过" || { echo "devbattery 单测有不通"; exit 1; }
