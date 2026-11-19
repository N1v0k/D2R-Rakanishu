#!/bin/bash






export LC_ALL=C
ACTIVE=${DM_ROOT:-$HOME/dm-work}/run/verify/dm/merge/active
pid_start() { local loc=C; printf %s "${2:-}" | LC_ALL=C grep -q '[^ -~]' && loc=zh_CN.UTF-8; LC_ALL=$loc ps -o lstart= -p "$1" 2>/dev/null | tr -s ' '; }
live=1
for f in "$ACTIVE"/*; do
  [ -f "$f" ] || continue; p=${f##*/}; s=$(cat "$f" 2>/dev/null)
  case $p in *[!0-9]*|'') continue;; esac
  if kill -0 "$p" 2>/dev/null && [ "$(pid_start "$p" "$s")" = "$s" ]; then echo "合入在跑：pid ${p}（启动 ${s}）"; live=0
  else rm -f "$f"; fi
done
exit $live
