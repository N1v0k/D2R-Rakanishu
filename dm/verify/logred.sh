#!/bin/bash








LOGRED='【红】|FATAL|看门狗|MTLCommandBufferStatus(Error|Fault)|CommandBufferCallbackError|Discarded \(victim|GPU (hang|error|fault|recovery)|管线.*失败|[Pp]ipeline.*fail'
[ -f "$1" ] || { printf '0\t（日志不存在 %s）\n' "$1"; exit 2; }
n=$(grep -cE "$LOGRED" "$1"); n=${n:-0}
printf '%s\t%s\n' "$n" "$(grep -m1 -E "$LOGRED" "$1" | cut -c1-140)"
[ "$n" = 0 ]
