#!/bin/bash






D=${1%/}; B=$D/bootlist.tsv
red() { echo "BOOTLIST RED $*"; exit 1; }
[ -s "$B" ] || red "缺 $B"
bid=$(sed -n '1s/^# BUILD_ID[[:space:]]*//p' "$B"); cid=$(grep -m1 '^# BUILD_ID' "$D/main.cuts.tsv" | awk '{print $NF}')
[ -n "$bid" ] && [ "$bid" = "$cid" ] || red "构建号不符：清单 ${bid:-无} / main.cuts.tsv ${cid:-无}"
RB=$(cd "$(dirname "$0")/../data" 2>/dev/null && pwd)/bootlist_$bid.tsv
[ -f "$RB" ] || red "仓库缺清单 dm/data/bootlist_$bid.tsv"
cmp -s "$B" "$RB" || red "与仓库 dm/data/bootlist_${bid:0:12}….tsv 不同（产物 $(grep -vc '^#' "$B") 块 / 仓库 $(grep -vc '^#' "$RB") 块）"
[ -f "$D/bootlist.sha256" ] && (cd "$D" && shasum -a 256 -c bootlist.sha256 >/dev/null 2>&1) || red "bootlist.sha256 缺或不符"
n=$(grep -vc '^#' "$B"); [ "$n" -gt 0 ] || red "清单 0 块"


st=$(python3 "$(dirname "$0")/bootlist_static.py" "$RB" ${DM_ROMFS:+"$DM_ROMFS/data"}) || red "$(echo "$st" | tail -1)；$(echo "$st" | head -1)"
echo "BOOTLIST OK $n 块 构建号 ${bid:0:12}… 与仓库清单逐字节相同，逐块与资源包一致"
