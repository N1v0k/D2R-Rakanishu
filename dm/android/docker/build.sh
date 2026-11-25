#!/usr/bin/env bash





















set -euo pipefail

image=${DM_ANDROID_BUILD_IMAGE:-dm-android-build:1}
src=$(readlink -f "${DM_SRC:-$(dirname "$0")/../../..}")
gen=$(readlink -f "${DM_GEN_DIR:?要 DM_GEN_DIR=<宿主表头目录> | needs DM_GEN_DIR=<host header directory>}")
user_home=${DM_ANDROID_USER:-$HOME/.android-dm}

for f in dm_vault_tab.h dm_hud_exp.h dm_bright_levels.inc; do
    [ -f "$gen/$f" ] || { echo "缺 $gen/$f | missing $gen/$f (dm/host/Makefile)" >&2; exit 1; }
done
mkdir -p "$user_home"
if [ ! -f "$user_home/debug.keystore" ] && command -v cmd.exe > /dev/null; then
    win_key=$(wslpath "$(cmd.exe /c 'echo %USERPROFILE%' 2> /dev/null | tr -d '\r')")/.android/debug.keystore
    [ -f "$win_key" ] && cp "$win_key" "$user_home/debug.keystore" && echo "debug key ← $win_key"
fi

if [ $# -eq 0 ]; then
    set -- gradle --no-daemon -p dm/android :app:assembleDebug :app:lintDebug -PdmGenDir=/gen -PdmPython=/usr/bin/python3
fi

args=(--rm --user "$(id -u):$(id -g)"
      -v "$src:/src"
      -v "$gen:/gen:ro"
      -v "$user_home:/android-user"
      -v dm-android-build-cache:/cache)
if [ -t 0 ] && [ -t 1 ]; then
    args+=(-it)
fi
exec docker run "${args[@]}" "$image" "$@"
