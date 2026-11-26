#!/bin/bash






set -u
cd "$(dirname "$0")"
source tools/devlock.sh
source tools/asc_env.sh
source tools/local_env.sh
D=${DM_DEVICE:-}; B=${DM_BUNDLE_ID:?先在本机配置里设 DM_BUNDLE_ID（App 包名，见 dm/ios/local.env.example）}
R=${DM_ROOT:-$HOME/dm-work}/run/ios; mkdir -p "$R/out"
DD=${DM_IOS_DD:-$R/dd}
APP=$DD/Build/Products/Release-iphoneos/DmIOS.app
t(){ echo "[$(date +%T)] $*"; }
OUT=${DM_OUT:-$(bash "../tools/product.sh" path touch)}; MODS="main sdk"

MOVIE_FFMPEG=${DM_MOVIE_FFMPEG:-0}
MOVIE_PREFIX=${DM_MOVIE_FFMPEG_PREFIX:-${DM_ROOT:-$HOME/dm-work}/build/cache/dm/ffmpeg-ios/out}
MOVIE_LDFLAGS=
if [ "$MOVIE_FFMPEG" = 1 ]; then
  case "$MOVIE_PREFIX" in *' '*) t "🔴 DM_MOVIE_FFMPEG_PREFIX 不能含空格：${MOVIE_PREFIX}"; exit 1;; esac
  for library in libavformat libavcodec libswscale libavutil; do
    [ -f "$MOVIE_PREFIX/lib/$library.a" ] || { t "🔴 缺 ${MOVIE_PREFIX}/lib/$library.a；先运行 tools/build_ffmpeg_ios.sh"; exit 1; }
  done
  [ -f "$MOVIE_PREFIX/include/libavcodec/avcodec.h" ] || { t "🔴 FFmpeg 头文件不在 ${MOVIE_PREFIX}/include；重跑 tools/build_ffmpeg_ios.sh"; exit 1; }
  MOVIE_LDFLAGS="$MOVIE_PREFIX/lib/libavformat.a $MOVIE_PREFIX/lib/libavcodec.a $MOVIE_PREFIX/lib/libswscale.a $MOVIE_PREFIX/lib/libavutil.a"
  export MOVIE_FFMPEG MOVIE_PREFIX
  t "过场解码：静态 FFmpeg（${MOVIE_PREFIX}）"
else
  export MOVIE_FFMPEG=0
  unset MOVIE_PREFIX
fi
if [ "$1" = build ]; then


  mkdir -p Gen; for x in Gen/* Gen/.[!.]*; do [ -e "$x" ] && [ "$x" != Gen/assets ] && rm -rf "$x"; done; mkdir -p Gen/dm


  nl=$(grep -rn 'listen(' Sources ../host --include='*.c' --include='*.m' | grep -v '^[^:]*:[0-9]*:\s*//' | wc -l | tr -d ' ')
  gl=$(grep -c 'if (log_port && !pthread_create(&lp, NULL, log_port_main' Sources/main.m)
  [ "$nl" = 1 ] && [ "$gl" = 1 ] || { t "🔴 端口监听核对失败：listen( 共 $nl 处（只许 1），开关守卫 $gl 处（须 1）"; exit 1; }


  python3 tools/gen_assets.py "${DM_ROMFS_SRC:-${DM_ROOT:-$HOME/dm-work}/data/switch/$(sed -n '1s/-.*//p' ../PRODUCT)/Update/RomFS}" || { t "🔴 gen_assets 失败（见上）"; exit 1; }
  make -s -C ../host dm_vault_tab.h dm_hud_exp.h dm_bright_levels.inc ROMFS="${DM_ROMFS_SRC:-${DM_ROOT:-$HOME/dm-work}/data/switch/$(sed -n '1s/-.*//p' ../PRODUCT)/Update/RomFS}" || { t "🔴 宿主表头生成失败（见上）"; exit 1; }
  python3 tools/gen_build.py ../host Gen > Gen/dm.xcconfig || { t "🔴 gen_build 失败"; exit 1; }
  F=$(python3 tools/gen_embed.py $OUT Gen/embed.S $MODS) || { t "🔴 gen_embed 失败"; exit 1; }
  echo "OTHER_LDFLAGS = \$(inherited) \$(DM_FRAMEWORKS) $F $MOVIE_LDFLAGS" >> Gen/dm.xcconfig
  for m in $MODS; do cp $OUT/$m.fixups.tsv $OUT/$m.gates.tsv Gen/dm/; done


  for f in bootlist.tsv main.cuts.tsv; do [ -f $OUT/$f ] && cp $OUT/$f Gen/dm/; done


  cp ../gfx/nvn/shader_map.tsv Gen/dm/
  cp ../gfx/nvn/pso_manifest.tsv Gen/dm/
  cp ../gfx/nvn/c1_full.tsv Gen/dm/






  MSL=${DM_MSL_IOS:-${DM_ROOT:-$HOME/dm-work}/build/ios/dmsh_b}; V=$(sed -n 's/^#define DMSH_CHAIN "\(.*\)"/\1/p' ../gfx/nvn/dmsh_chain.h)
  [ -n "$V" ] && [ -d "$MSL/b-$V" ] && ls "$MSL/b-$V" | grep -q "_b.metallib$" || { t "🔴 缺 iOS 版 B 着色器 $MSL/b-${V}（见上一行出包命令；V=${V}）"; exit 1; }
  mkdir -p Gen/msl && cp -cR "$MSL/b-$V" Gen/msl/


  cp asset_pointers.txt Gen/dm/; if [ -n "${DM_ASSET_BASE:-}" ]; then echo "$DM_ASSET_BASE" > Gen/dm/asset_base.txt; else rm -f Gen/dm/asset_base.txt; fi


  rm -f Gen/dm/astc_pack_base.txt; [ -n "${DM_ASTC_PACK_BASE:-}" ] && echo "$DM_ASTC_PACK_BASE" > Gen/dm/astc_pack_base.txt
  rm -rf Gen/save_ref; mkdir -p Gen/save_ref; if [ -d "${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref/0000000000000001/0" ]; then cp -R "${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref/0000000000000001/0/." Gen/save_ref/ && chmod -R u+w Gen/save_ref; elif [ "${DM_REQUIRE_SAVES:-0}" = 1 ]; then t "🔴 正式出包要带标准存档（DM_REQUIRE_SAVES=1），但 ${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref/0000000000000001/0 不在"; exit 1; fi
  rm -rf Gen/save_ref_warlock; mkdir -p Gen/save_ref_warlock; if [ -d "${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref_warlock/0000000000000001/0" ]; then cp -R "${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref_warlock/0000000000000001/0/." Gen/save_ref_warlock/ && chmod -R u+w Gen/save_ref_warlock; elif [ "${DM_REQUIRE_SAVES:-0}" = 1 ]; then t "🔴 正式出包要带标准存档（DM_REQUIRE_SAVES=1），但 ${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref_warlock/0000000000000001/0 不在"; exit 1; fi




  python3 tools/gen_romfs_overlay.py "${DM_ROMFS_SRC:-${DM_ROOT:-$HOME/dm-work}/data/switch/$(sed -n '1s/-.*//p' ../PRODUCT)/Update/RomFS}" Gen/romfs_overlay > /dev/null || { t "🔴 gen_romfs_overlay 失败"; exit 1; }



  for o in Gen/romfs_overlay Gen/romfs_overlay_variants/* Gen/romfs_overlay_astc Gen/romfs_overlay_astc_variants/*; do
    [ -d "$o/data" ] || continue; grep -q '^vfs-root = ' "$o/data/.build.config" 2>/dev/null || { t "🔴 覆盖层 $o 里没有可用的配置文件 data/.build.config"; exit 1; }
  done
  [ -f Gen/romfs_overlay/data/.build.config ] || { t "🔴 没生成覆盖层 Gen/romfs_overlay"; exit 1; }






  SPEC=project.yml; FREE=
  if [ "${DM_FREE_ACCOUNT:-0}" = 1 ]; then
    python3 tools/project_free.py project.yml project_free.yml || { t "🔴 DM_FREE_ACCOUNT：免费档的工程描述没生成（见上）"; exit 1; }
    SPEC=project_free.yml; FREE=free
  fi
  if [ "${DM_IOS_M3:-0}" = 1 ]; then
    python3 tools/project_m3.py $SPEC project_m3.yml $FREE || { rm -f project_free.yml; t "🔴 DM_IOS_M3：旧接口版的工程描述没生成（见上）"; exit 1; }
    SPEC=project_m3.yml
  fi
  if [ $SPEC = project.yml ]; then
  xcodegen generate -q >/dev/null || { t "🔴 xcodegen 失败"; exit 1; }
  else
    xcodegen generate -q --spec $SPEC >/dev/null; XG=$?; rm -f project_free.yml project_m3.yml; [ $XG = 0 ] || { t "🔴 xcodegen 失败"; exit 1; }
    [ -z "$FREE" ] || t "按免费账号生成：不带 iCloud / 持续执行 / 增加内存上限 / 扩展虚拟寻址，也不带资源下载扩展（只占一个应用标识）"
    [ "${DM_IOS_M3:-0}" != 1 ] || t "按旧接口版生成：部署目标 iOS 18.0、图形后端不用 Metal 4"
  fi
  [ "${DM_GEN_ONLY:-}" = 1 ] && { t "生成完成（DM_GEN_ONLY）"; exit 0; }
  asc_auth










  BN=$(git log -1 --format=%ct 2>/dev/null); [ -n "$(git status --porcelain --untracked-files=no 2>/dev/null)" ] && BN=1; BN=${BN:-1}
  [ "$BN" = 1 ] && t "⚠ 工作区有未提交改动（或读不到提交）：开发包构建号 = 1，不能用于放行" || t "开发包构建号 ${BN}（提交 $(git log -1 --format=%h | cut -c1-9) 的提交时间戳）"






  DEST='generic/platform=iOS'
  if [ "${DM_FREE_ACCOUNT:-0}" = 1 ]; then
    if [ -n "${DM_IOS_UDID:-}" ]; then DEST="id=$DM_IOS_UDID"; t "免费账号：用已连接的设备做构建目标（先把手机用数据线连上并解锁）"
    else t "⚠ 免费账号第一次构建要把手机连上，并在本机配置里填 DM_IOS_UDID（见 local.env.example）；没填，先按通用目标试"; fi
  fi
  xcodebuild -project DmIOS.xcodeproj -scheme DmIOS -configuration Release -destination "$DEST" \
    -derivedDataPath "$DD" -allowProvisioningUpdates ${ASC_AUTH[@]+"${ASC_AUTH[@]}"} DM_BUILD_NUMBER=$BN ${DM_DEVICE_FAMILY:+DM_DEVICE_FAMILY=$DM_DEVICE_FAMILY} build > "$R/build.log" 2>&1 || { t "🔴 构建失败"; grep -E "error:" "$R/build.log" | head -5
      grep -q 'no devices from which to generate a provisioning profile' "$R/build.log" && t "原因：这个团队名下还没有设备。把手机用数据线连上并解锁，在本机配置里填 DM_IOS_UDID、DM_FREE_ACCOUNT=1，再构建一次"; exit 1; }
  codesign -d --entitlements :- $APP 2>/dev/null | plutil -p - ; t "构建完成"


  t "构建完成"; exit 0
fi
if [ "$1" = install ]; then
  local_env_need DM_DEVICE || exit 1


  [ "${DM_DEVBATTERY:-}" = 1 ] || DM_DEVBATTERY=0
  devlock_acquire "${SMOKE_OWNER:-${ACCEPT_OWNER:-run.sh install}}"
  trap devlock_release EXIT
  xcrun devicectl device install app --device $D "$APP" > $R/install.log 2>&1 || { t "🔴 装机失败"; tail -5 $R/install.log; exit 1; }
  t "装机完成"; exit 0
fi
local_env_need DM_DEVICE || exit 1
M=$1; CAP=${2:-60}; O=$R/out/$M.txt; rm -f $O
devlock_acquire "${SMOKE_OWNER:-${ACCEPT_OWNER:-run.sh $M}}"
trap devlock_release EXIT
xcrun devicectl device process launch --device $D --terminate-existing $B -mode $M > $R/launch_$M.log 2>&1
grep -q Launched $R/launch_$M.log || { t "🔴 启动失败"; tail -8 $R/launch_$M.log; exit 1; }
pull(){ xcrun devicectl device copy from --device $D --domain-type appDataContainer --domain-identifier $B --source Documents/$M.txt --destination $O >/dev/null 2>&1; }
alive(){ xcrun devicectl device info processes --device $D 2>/dev/null | grep -q "DmIOS.app/DmIOS"; }
s=$SECONDS; sleep 3
while :; do pull; grep -q DONE $O 2>/dev/null && { t "$M 完成"; cat $O; exit 0; }
  alive || { sleep 1; pull; t "$M 进程已退出"; cat $O 2>/dev/null; exit 0; }
  [ $((SECONDS - s)) -gt $CAP ] && { t "🔴 $M 超 ${CAP}s"; cat $O 2>/dev/null; exit 1; }; sleep 3; done
