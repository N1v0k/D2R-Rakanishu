#!/bin/bash
















set -u
P=${1:?被测 pid}; F=${2:?记录文件}; NAME=${3:-dmhost}
kill -0 "$P" 2>/dev/null || { echo "CONC RED 被测进程 $P 不存在（起晚了或已退出）"; exit 1; }
: > "$F" || { echo "CONC RED 记录文件 $F 写不了"; exit 1; }
while kill -0 "$P" 2>/dev/null; do
  n=$(ps -A -o pid=,comm= | awk -v me="$P" -v nm="$NAME" '$1 != me { c = $2; sub(".*/", "", c); if (c == nm) k++ } END { print k + 0 }')
  echo "$(date +%s) $n $(sysctl -n vm.loadavg | awk '{print $2}')" >> "$F"
  sleep 1
done
awk -v f="$F" '{ n++; s += $2; if ($2 > m) m = $2 } END {
  if (!n) { print "CONC RED 没采到样本（被测进程不到 1 秒就退出）"; exit 1 }
  if (m > 0) { printf "CONC RED 采样 %d 秒 别的进程 最多 %d 均 %.1f → 本轮作废（%s）\n", n, m, s / n, f; exit 1 }
  printf "CONC OK 采样 %d 秒 别的进程 最多 0 → 本轮有效\n", n }' "$F"
