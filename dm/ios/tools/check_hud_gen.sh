#!/bin/bash






set -uo pipefail
I="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; W="$(cd "$I/../.." && pwd)"
BAD=$(git -C "$W" ls-files -- dm/ios/NativeHUD/v4 dm/ios/NativeHUD/mapgoal dm/ios/NativeHUD/widestash dm/ios/NativeHUD/pouch dm/ios/NativeHUD/touch/fe_rightpanel.* \
      'dm/ios/Sources/TouchArt/dm_native_*' dm/ios/Sources/dm_touch_native_art.c dm/host/dm_vault_tab.h dm/host/dm_hud_exp.h dm/host/dm_bright_levels.inc dm/ios/tools/touchrectv1_HUDPanelHD.json dm/ios/tools/native_art/skills.txt dm/ios/tools/native_art/skilldesc.txt)
[ -z "$BAD" ] || { echo "HUDGEN 红：原版衍生的生成物进了仓库（应该构建时从游戏文件生成）："; echo "$BAD" | sed 's/^/  /'; exit 1; }
R=${DM_ROMFS_SRC:-${DM_ROOT:-$HOME/dm-work}/data/switch/$(sed -n '1s/-.*//p' "$(dirname "$0")/../../PRODUCT")/Update/RomFS}
[ -f "$R/data/build_manifest.psv" ] || { echo "HUDGEN OK（入库检查）；生成检查跳过：这台机器没有游戏文件 $R"; exit 0; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
python3 "$I/tools/native_art/generate.py" "$R" > "$T.art.log" 2>&1 || { echo "HUDGEN 红：技能图集生成不通（底栏要用它）"; tail -3 "$T.art.log"; exit 1; }
python3 "$I/tools/gen_hud_v4.py" "$R" "$T" > "$T.log" 2>&1 || { echo "HUDGEN 红：gen_hud_v4.py 跑不通"; tail -3 "$T.log"; exit 1; }
n=$(find "$T" -type f | wc -l | tr -d ' '); [ "$n" -ge 79 ] || { echo "HUDGEN 红：底部 UI v4 只生成了 $n 个文件（应 79）"; exit 1; }
echo "HUDGEN OK：生成物没有入库；底部 UI v4 从游戏文件生成 $n 个文件$(grep -q '^没找到 PC' "$T.log" && echo '（Switch 素材放大版）')"
