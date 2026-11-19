#!/bin/bash






ROOT=${1:-${DM_ROOT:-$HOME/dm-work}/run/verify/dm/shotgate}; DAYS=${2:-${SHOTGATE_KEEP_DAYS:-3}}
case "$ROOT" in */shotgate|*/shotgate/) ;; *) echo "shotgate_gc：根目录不是 shotgate（${ROOT}），不动"; exit 2;; esac
case "$DAYS" in ''|*[!0-9]*) echo "shotgate_gc：天数不是整数（${DAYS}），不动"; exit 2;; esac
[ -d "$ROOT" ] || exit 0
n=0
while IFS= read -r d; do
  [ -d "$d" ] && [ ! -L "$d" ] && [ ! -e "$d/KEEP" ] && [ ! -e "$d/PIN" ] || continue
  rm -rf -- "$d" && n=$((n+1))
done < <(find "$ROOT" -mindepth 1 -maxdepth 1 -type d -name '[0-9][0-9][0-9][0-9]_*' -mtime +"$DAYS" 2>/dev/null)
[ -d "$ROOT/green" ] && find "$ROOT/green" -maxdepth 1 -type f -name '*.html' -mtime +"$DAYS" -delete 2>/dev/null
[ $n -gt 0 ] && echo "shotgate_gc：清掉 $n 个超过 $DAYS 天的旧现场"
exit 0
