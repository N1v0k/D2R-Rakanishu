#!/bin/bash










set -u
cd "$(dirname "$0")/../ios"
t(){ echo "[$(date +%T)] $*"; }
ROOT=${DM_ROOT:-$HOME/dm-work}; DD=${DM_TV_DD:-$ROOT/run/tvos/dd}; LOG=$ROOT/run/tvos/build.log; mkdir -p "$ROOT/run/tvos"
SIGN=(-allowProvisioningUpdates); [ "${DM_TV_NOSIGN:-0}" = 1 ] && SIGN=(CODE_SIGNING_ALLOWED=NO)
DM_GEN_ONLY=1 ./run.sh build || { t "🔴 生成失败（ios/run.sh build）"; exit 1; }
python3 ../tvos/build_shaders.py "${DM_MSL_MAC:-$ROOT/build/cache/metal_shader_cache}" "$ROOT/build/tvos/msl" || { t "🔴 tvOS 着色器没编齐"; exit 1; }
python3 ../tvos/gen_icon.py Assets.xcassets/AppIcon.appiconset/AppIcon1024.png Gen/tvos/Assets.xcassets || { t "🔴 tvOS 图标没生成（要 Pillow）"; exit 1; }
V=$(sed -n 's/^#define DMSH_CHAIN "\(.*\)"/\1/p' ../gfx/nvn/dmsh_chain.h)
rm -rf Gen/tvos/msl; mkdir -p Gen/tvos/msl && cp -cR "$ROOT/build/tvos/msl/b-$V" Gen/tvos/msl/ && find Gen/tvos/msl -name '*.tvos.sha256' -delete
xcodebuild -project DmIOS.xcodeproj -scheme DmTV -configuration Release -sdk appletvos -destination 'generic/platform=tvOS' -derivedDataPath "$DD" \
  "${SIGN[@]}" build > "$LOG" 2>&1 || { t "🔴 构建失败（${LOG}）"; grep -E "error:" "$LOG" | head -10; exit 1; }
t "构建完成：$DD/Build/Products/Release-appletvos/DmTV.app"
