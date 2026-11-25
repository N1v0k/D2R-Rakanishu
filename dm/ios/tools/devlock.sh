#!/bin/bash

DEVLOCK_DIR=${DM_DEVLOCK_DIR:-${DM_ROOT:-$HOME/dm-work}/run/ios/device.lock}
DEVLOCK_DEPTH=${DEVLOCK_DEPTH:-0}
DEVLOCK_QUEUE=${DM_DEVLOCK_QUEUE:-${DEVLOCK_DIR%.lock}.queue}
DEVLOCK_TICKET=










DEVLOCK_WD_PID=
DEVLOCK_WD_LOG=${DM_DEVLOCK_WD_LOG:-${DM_ROOT:-$HOME/dm-work}/run/ios/devlock_watchdog.log}




DEVLOCK_HIST=${DM_DEVLOCK_HIST:-${DM_ROOT:-$HOME/dm-work}/run/ios/devlock_hist.tsv}
devlock_hist() { mkdir -p "$(dirname "$DEVLOCK_HIST")" 2>/dev/null; printf '%s\t%s\t%s\t%s\t%s\t%s\n' "$(date '+%F %T')" "$1" "$$" "$DEVLOCK_DIR" "$2" "$3" >> "$DEVLOCK_HIST" 2>/dev/null || true; }
devlock_default_max() {
  [ "$DEVLOCK_DIR" = "${DM_ROOT:-$HOME/dm-work}/run/ios/device.lock" ] || { echo 0; return; }
  [ "${DM_DEVLOCK_CLASS:-}" = measure ] && echo 1200 || echo 600
}


devlock_descendants() {
  local root=$1 kids k
  kids=$(ps -axo pid=,ppid= 2>/dev/null | awk -v p="$root" '$2==p{print $1}')
  for k in $kids; do devlock_descendants "$k"; echo "$k"; done
}
devlock_watchdog() {
  local holder=$1 lst=$2 max=$3 owner=$4 me=${BASHPID:-$$} now started held victims v
  trap '' HUP
  while sleep 2; do
    [ "$(cat "$DEVLOCK_DIR/pid" 2>/dev/null)" = "$holder" ] || exit 0
    [ "$(cat "$DEVLOCK_DIR/lstart" 2>/dev/null)" = "$lst" ] || exit 0
    kill -0 "$holder" 2>/dev/null || exit 0
    started=$(cat "$DEVLOCK_DIR/started" 2>/dev/null || echo 0); now=$(date +%s); held=$((now - started))
    [ "$held" -ge "$max" ] || continue
    local msg="【红】设备锁看门狗：持有者「${owner}」（pid ${holder}）已持有 ${held} 秒，超过上限 ${max} 秒 → 终止持锁进程树、释放锁"
    echo "$msg" >&2; mkdir -p "$(dirname "$DEVLOCK_WD_LOG")"; echo "$(date '+%F %T') ${msg}（锁 ${DEVLOCK_DIR}）" >> "$DEVLOCK_WD_LOG"
    victims=$(devlock_descendants "$holder" | grep -v -x -e "$me" $(devlock_descendants "$me" | sed 's/^/-e /'))
    for v in $victims "$holder"; do kill -TERM "$v" 2>/dev/null; done
    sleep 5
    for v in $victims "$holder"; do kill -0 "$v" 2>/dev/null && kill -KILL "$v" 2>/dev/null; done
    [ "$(cat "$DEVLOCK_DIR/pid" 2>/dev/null)" = "$holder" ] && rm -rf "$DEVLOCK_DIR"
    exit 0
  done
}









devlock_ticket_take() {
  mkdir -p "$DEVLOCK_QUEUE"
  local cls=0; [ "${DM_DEVLOCK_CLASS:-}" = measure ] && cls=1
  DEVLOCK_TICKET="$DEVLOCK_QUEUE/${cls}_$(python3 -c 'import time;print(time.time_ns())')_$$"
  mkdir "$DEVLOCK_TICKET"; printf "%s\n" "${1:-unknown}" > "$DEVLOCK_TICKET/owner"
}
devlock_ticket_drop() { [ -n "$DEVLOCK_TICKET" ] && rm -rf "$DEVLOCK_TICKET"; DEVLOCK_TICKET=; }


devlock_ahead() {
  local d n=0 first="" mine; mine=$(basename "$DEVLOCK_TICKET")




  local IFS=$' \t\n'
  for d in $(ls "$DEVLOCK_QUEUE" 2>/dev/null | LC_ALL=C sort); do
    kill -0 "${d##*_}" 2>/dev/null || { rm -rf "$DEVLOCK_QUEUE/$d"; continue; }
    [ "$d" != "$mine" ] || break
    n=$((n + 1)); [ -n "$first" ] || first=$(cat "$DEVLOCK_QUEUE/$d/owner" 2>/dev/null)
  done
  printf "%s\t%s\n" "$n" "$first"
}







devlock_lstart() {
  local loc=C; printf %s "${2:-}" | LC_ALL=C grep -q '[^ -~]' && loc=zh_CN.UTF-8




  LC_ALL=$loc ps -o lstart= -p "$1" 2>/dev/null | sed 's/^[[:space:]]*//; s/[[:space:]]*$//' || true
}
devlock_is_descendant() {
  local child=$1 ancestor=$2 parent depth=0
  while [ "$depth" -lt 64 ]; do
    [ "$child" = "$ancestor" ] && return 0
    parent=$(ps -o ppid= -p "$child" 2>/dev/null | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')
    [ -n "$parent" ] || return 1
    [ "$parent" = "$child" ] && return 1
    child=$parent
    depth=$((depth + 1))
  done
  return 1
}





devlock_acquire() {
  local rc=0; _devlock_acquire "$@" || rc=$?
  if [ "$rc" = 0 ] && [ "$(cat "$DEVLOCK_DIR/pid" 2>/dev/null)" != "${DM_DEVLOCK_HELD:-}" ]; then rc=97; fi
  [ "$rc" = 0 ] && return 0
  echo "【红】设备锁：${1:-unknown} 没取到锁（返回 ${rc}；锁 $DEVLOCK_DIR 现在的持有者「$(cat "$DEVLOCK_DIR/owner" 2>/dev/null || echo 无)」pid $(cat "$DEVLOCK_DIR/pid" 2>/dev/null || echo 无)）" >&2
  return "$rc"
}
_devlock_acquire() {
  local owner=${1:-unknown} max=${2:-${DM_DEVLOCK_MAX_S:-}} held_lstart pid lock_owner started now held lock_lstart self_lstart lock_pid_lstart current_pid last_notice=0 lock_max over_notice=0
  [ -n "$max" ] || max=$(devlock_default_max)
  local q0=$SECONDS
  mkdir -p "$(dirname "$DEVLOCK_DIR")" 2>/dev/null && [ -w "$(dirname "$DEVLOCK_DIR")" ] || return 98
  current_pid=${BASHPID:-$$}
  self_lstart=$(devlock_lstart $$)
  if [ -n "${DM_DEVLOCK_HELD:-}" ] && [ -f "$DEVLOCK_DIR/pid" ] &&
     [ "$(cat "$DEVLOCK_DIR/pid" 2>/dev/null)" = "$DM_DEVLOCK_HELD" ] &&




     held_lstart=$(devlock_lstart "$DM_DEVLOCK_HELD" "$(cat "$DEVLOCK_DIR/lstart" 2>/dev/null)") &&
     [ -n "$held_lstart" ] &&
     [ "$held_lstart" = "$(cat "$DEVLOCK_DIR/lstart" 2>/dev/null)" ] &&
     kill -0 "$DM_DEVLOCK_HELD" 2>/dev/null &&
     devlock_is_descendant "$current_pid" "$DM_DEVLOCK_HELD"; then
    DEVLOCK_DEPTH=$((DEVLOCK_DEPTH + 1))
    return 0
  fi




  if { [[ " ${DM_DEVBATTERY_LOCKS:-${DM_ROOT:-$HOME/dm-work}/run/ios/device.lock} " == *" $DEVLOCK_DIR "* ]] && [ "${DM_DEVBATTERY:-1}" != 0 ]; } || [ "${DM_DEVBATTERY:-}" = 1 ]; then
    source "$(dirname "${BASH_SOURCE[0]}")/devbattery.sh"
    local bw0=$SECONDS bwmax=${DM_DEVBATTERY_WAIT_S:-3600}
    local brc
    until devbattery_gate; do
      brc=$?; [ $brc != 3 ] || { echo "【红】电量闸：本机缺命令（见上一行），等也没用，$owner 不取锁、退出" >&2; exit 4; }
      [ $((SECONDS - bw0)) -ge "$bwmax" ] && { echo "【红】电量闸等了 $((SECONDS - bw0)) 秒仍不放行（${DEVBATTERY_LINE:-}），$owner 不取锁、退出" >&2; exit 4; }
      sleep "${DM_DEVBATTERY_POLL_S:-60}"
    done
    echo "电量闸放行：${DEVBATTERY_LINE:-}（${owner}）" >&2
  fi
  devlock_ticket_take "$owner"
  local ahead first q_notice=0
  while :; do
    IFS=$'\t' read -r ahead first < <(devlock_ahead)
    [ "$ahead" = 0 ] && break
    now=$(date +%s); [ $((now - q_notice)) -ge 30 ] && { echo "排队等设备锁：前面还有 $ahead 个（下一个：${first:-?}）" >&2; q_notice=$now; }
    sleep 1
  done
  while :; do


    IFS=$'\t' read -r ahead first < <(devlock_ahead)
    if [ "$ahead" != 0 ]; then sleep 1; continue; fi
    mkdir "$DEVLOCK_DIR" 2>/dev/null && break
    pid=$(cat "$DEVLOCK_DIR/pid" 2>/dev/null || true)
    lock_owner=$(cat "$DEVLOCK_DIR/owner" 2>/dev/null || echo unknown)
    started=$(cat "$DEVLOCK_DIR/started" 2>/dev/null || date +%s)
    lock_lstart=$(cat "$DEVLOCK_DIR/lstart" 2>/dev/null || true)
    now=$(date +%s)
    held=$((now - started))
    lock_pid_lstart=$(devlock_lstart "$pid" "$lock_lstart")


    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null && { [ ! -f "$DEVLOCK_DIR/lstart" ] || [ "$lock_pid_lstart" = "$lock_lstart" ]; }; then
      lock_max=$(cat "$DEVLOCK_DIR/max" 2>/dev/null || echo 0)
      if [ "$last_notice" -eq 0 ] || [ $((now - last_notice)) -ge 30 ]; then
        if [ "${lock_max:-0}" -gt 0 ]; then echo "等待设备锁：持有者 ${lock_owner}，已持有 ${held} 秒 / 上限 ${lock_max} 秒" >&2
        else echo "等待设备锁：持有者 ${lock_owner}，已持有 ${held} 秒（未申报上限）" >&2; fi
        last_notice=$now
      fi


      if [ "${lock_max:-0}" -gt 0 ] && [ "$held" -gt $((lock_max + 30)) ] && [ $((now - over_notice)) -ge 60 ]; then
        echo "【红】设备锁：持有者 ${lock_owner}（pid ${pid}）已持有 ${held} 秒，超过上限 ${lock_max} 秒仍未释放（看门狗没起作用），不代为终止，请找持有者" >&2
        mkdir -p "$(dirname "$DEVLOCK_WD_LOG")"; echo "$(date '+%F %T') 等锁方：持有者「${lock_owner}」pid $pid 已持有 ${held} 秒、上限 ${lock_max} 秒仍未释放（锁 ${DEVLOCK_DIR}）" >> "$DEVLOCK_WD_LOG"
        over_notice=$now
      fi
      sleep 1
    elif [ -z "$pid" ] && [ $((now - $(stat -f %m "$DEVLOCK_DIR" 2>/dev/null || echo 0))) -lt 10 ]; then
      sleep 1
    else
      rm -rf "$DEVLOCK_DIR"
    fi
  done
  printf "%s\n" "$$" > "$DEVLOCK_DIR/pid"
  printf "%s\n" "$owner" > "$DEVLOCK_DIR/owner"
  date +%s > "$DEVLOCK_DIR/started"
  printf "%s\n" "$self_lstart" > "$DEVLOCK_DIR/lstart"
  printf "%s\n" "${max:-0}" > "$DEVLOCK_DIR/max"
  export DM_DEVLOCK_HELD=$$
  devlock_ticket_drop
  DEVLOCK_DEPTH=1
  devlock_hist 取 "$owner" $((SECONDS - q0))
  if [ "${max:-0}" -gt 0 ]; then
    devlock_watchdog "$$" "$self_lstart" "$max" "$owner" </dev/null >/dev/null &
    DEVLOCK_WD_PID=$!; disown "$DEVLOCK_WD_PID" 2>/dev/null || true
  fi
}

devlock_release() {
  [ "$DEVLOCK_DEPTH" -gt 0 ] || return 0
  if [ "$DEVLOCK_DEPTH" -gt 1 ]; then
    DEVLOCK_DEPTH=$((DEVLOCK_DEPTH - 1))
    return 0
  fi








  if [ -n "$DEVLOCK_WD_PID" ]; then local wk; wk=$(pgrep -P "$DEVLOCK_WD_PID" 2>/dev/null) || true; kill -KILL "$DEVLOCK_WD_PID" 2>/dev/null || true; [ -z "$wk" ] || kill -KILL $wk 2>/dev/null || true; DEVLOCK_WD_PID=; fi
  [ "$(cat "$DEVLOCK_DIR/pid" 2>/dev/null)" = "$$" ] || return 0
  devlock_hist 放 "$(cat "$DEVLOCK_DIR/owner" 2>/dev/null)" $(( $(date +%s) - $(cat "$DEVLOCK_DIR/started" 2>/dev/null || date +%s) ))
  rm -rf "$DEVLOCK_DIR"
  DEVLOCK_DEPTH=0
}
