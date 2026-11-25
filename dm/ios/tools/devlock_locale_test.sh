#!/bin/bash




set -u; H=$(cd "$(dirname "$0")" && pwd); T=$(mktemp -d); export DM_DEVLOCK_DIR=$T/device.lock DM_DEVLOCK_QUEUE=$T/device.queue; bad=0
ok(){ echo "  ✓ $1"; }; no(){ echo "  🔴 $1"; bad=1; }


( unset DM_DEVLOCK_HELD; export LC_ALL=zh_CN.UTF-8; source $H/devlock.sh; devlock_acquire 甲 2>/dev/null; sleep 4; date +%s > $T/a_rel; devlock_release ) &
sleep 1
( unset DM_DEVLOCK_HELD; export LC_ALL=C; source $H/devlock.sh; devlock_acquire 乙 2>/dev/null; date +%s > $T/b_got; devlock_release )
wait; [ "$(cat $T/b_got)" -ge "$(cat $T/a_rel)" ] && ok "① 中文环境持锁时 C 环境的等到放锁才拿到" || no "① C 环境的在中文环境持有者还活着时拿到了锁"


sleep 30 & P=$!; mkdir -p $DM_DEVLOCK_DIR; echo $P > $DM_DEVLOCK_DIR/pid; echo 旧锁 > $DM_DEVLOCK_DIR/owner; date +%s > $DM_DEVLOCK_DIR/started
LC_ALL=zh_CN.UTF-8 ps -o lstart= -p $P | sed 's/^[[:space:]]*//; s/[[:space:]]*$//' > $DM_DEVLOCK_DIR/lstart
( sleep 3; date +%s > $T/p_kill; kill $P ) &
( unset DM_DEVLOCK_HELD; export LC_ALL=C; source $H/devlock.sh; devlock_acquire 丙 2>/dev/null; date +%s > $T/c_got; devlock_release )
wait; [ "$(cat $T/c_got)" -ge "$(cat $T/p_kill)" ] && ok "② 中文串旧锁的持有者活着时没被拆，死了才收回" || no "② 中文串旧锁在持有者活着时被拆了"


mkdir -p $DM_DEVLOCK_DIR; echo 999999 > $DM_DEVLOCK_DIR/pid; echo 死锁 > $DM_DEVLOCK_DIR/owner; echo "Thu Jan  1 00:00:00 2026" > $DM_DEVLOCK_DIR/lstart
t0=$(date +%s); ( unset DM_DEVLOCK_HELD; source $H/devlock.sh; devlock_acquire 丁 2>/dev/null; devlock_release ); [ $(( $(date +%s) - t0 )) -le 3 ] && ok "③ 死持有者的锁 3 秒内收回" || no "③ 死持有者的锁没及时收回"
rm -rf $T; [ $bad = 0 ] && echo "devlock_locale_test 通过 3/3" || { echo "🔴 devlock_locale_test 有不过的"; exit 1; }
