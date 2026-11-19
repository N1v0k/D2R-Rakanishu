#!/bin/bash










set -eu
SRC=${1:-$(bash "$(dirname "$0")/../../tools/product.sh" path keypage)}; DST=$SRC-truth
OFF=$((0x8524c4)); OLD=d2a20808; NEW=d2a40008
le() { echo "$1" | sed -E 's/(..)(..)(..)(..)/\4\3\2\1/'; }
cur=$(xxd -s $OFF -l 4 -p "$SRC/main.patched.bin")
[ "$cur" = "$(le $OLD)" ] || { echo "【红】源产物 main+0x8524c4 不是预期指令 ${OLD}（读到小端 ${cur}），构建号变了？不改" >&2; exit 2; }
rm -rf "$DST"; cp -R "$SRC" "$DST"
printf "$(le $NEW | sed 's/../\\x&/g')" | dd of="$DST/main.patched.bin" bs=1 seek=$OFF conv=notrunc 2>/dev/null
[ "$(xxd -s $OFF -l 4 -p "$DST/main.patched.bin")" = "$(le $NEW)" ] || { echo "【红】写入核对失败" >&2; exit 1; }
cmp -l "$SRC/main.patched.bin" "$DST/main.patched.bin" | wc -l | awk '{ if ($1 > 4) { print "【红】改动字节数 " $1 " > 4" > "/dev/stderr"; exit 1 } }'
echo "$DST"
