#!/bin/bash














MACWIN_DIR=${MACWIN_DIR:-${DM_ROOT:-$HOME/dm-work}/run/ios}
macwin_alive() {
  local d=$1 p s; p=$(cat "$d/pid" 2>/dev/null); s=$(cat "$d/lstart" 2>/dev/null)
  [ -n "$p" ] && kill -0 "$p" 2>/dev/null && [ "$(devlock_lstart "$p" "$s")" = "$s" ]
}
macwin_excl_acquire() {
  local d=$MACWIN_DIR/macwin.excl s=$SECONDS busy n
  mkdir -p "$d" && printf '%s\n' "$$" > "$d/pid" && devlock_lstart "$$" > "$d/lstart" && printf '%s\n' "${1:-未署名}" > "$d/owner" || return 1
  MACWIN_MINE=1
  while :; do
    busy=; for n in 1 2 3; do macwin_alive "$MACWIN_DIR/macsmoke_reg$n.lock" && busy="$busy reg$n"; done
    [ -z "$busy" ] && return 0
    [ $((SECONDS - s)) -ge "${MACWIN_WAIT_MAX:-1800}" ] && { echo "【红】窗口独占：等了 $((SECONDS - s)) 秒槽锁仍在用（${busy# }），不开窗" >&2; macwin_excl_release; return 1; }
    [ $(((SECONDS - s) % 60)) -lt 5 ] && echo "窗口独占：等整表槽位让出（${busy# }，已等 $((SECONDS - s)) 秒）" >&2
    sleep 5
  done
}
macwin_excl_release() { [ "${MACWIN_MINE:-}" = 1 ] || return 0; [ "$(cat "$MACWIN_DIR/macwin.excl/pid" 2>/dev/null)" = "$$" ] && rm -rf "$MACWIN_DIR/macwin.excl"; MACWIN_MINE=; }
macwin_busy() { macwin_alive "$MACWIN_DIR/macwin.excl"; }
macwin_wait_free() { local s=$SECONDS; while macwin_busy; do [ $(((SECONDS - s) % 60)) -lt 5 ] && echo "槽位局：等开窗任务（$(cat "$MACWIN_DIR/macwin.excl/owner" 2>/dev/null)）结束，已等 $((SECONDS - s)) 秒" >&2; sleep 5; done; }
