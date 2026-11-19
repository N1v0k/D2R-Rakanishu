#!/bin/bash












mlock_acquire() {
  [ "${XT_LOCK_HELD:-}" = 1 ] && return 0
  export DM_DEVLOCK_DIR=${XT_LOCK_DIR:-${DM_ROOT:-$HOME/dm-work}/run/ios/macsmoke.lock}
  source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/devlock.sh" || { echo "🔴 mlock：同目录下没有 devlock.sh"; return 1; }
  devlock_acquire "$1" || return 1


  source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/macwin.sh" && macwin_excl_acquire "$1" || { devlock_release; return 1; }
  export XT_LOCK_HELD=1; MLOCK_MINE=1
}
mlock_release() { [ "${MLOCK_MINE:-}" = 1 ] || return 0; macwin_excl_release; devlock_release; MLOCK_MINE=; unset XT_LOCK_HELD; }
