#!/bin/bash







set -eu
HERE=$(cd "$(dirname "$0")" && pwd); DM=$HERE/..; ROOT=${DM_ROOT:-$HOME/dm-work}; GV=$(sed -n '1s/-.*//p' "$DM/PRODUCT")
OUT=${1:-$ROOT/out/macapp}; KIND=${DM_MAC_PRODUCT:-touch}; BID=${DM_MAC_BUNDLE_ID:-org.d2rmobile.mac}
P=$(bash "$HERE/product.sh" path "$KIND"); [ -f "$P/main.patched.bin" ] || { echo "🔴 没有产物 ${P}（先跑 product.sh build）"; exit 2; }


M3=${DM_MAC_M3:-}; NAME="D2R Mobile${M3:+ M3}"; [ -z "$M3" ] || BID=${DM_MAC_BUNDLE_ID:-org.d2rmobile.mac.m3}




ICLOUD=${DM_MAC_ICLOUD:-}; PF=
if [ -n "$ICLOUD" ]; then
  SIGN_DIR=${DM_MAC_SIGN_DIR:-$ROOT/mac-sign}
  PF=$(ls -t "$SIGN_DIR"/*.provisionprofile "$SIGN_DIR"/*.mobileprovision 2>/dev/null | head -1)
  [ -n "$PF" ] || { echo "🔴 签名目录里没有 profile：把带 iCloud 能力的 .provisionprofile 放进 ${SIGN_DIR}（DM_MAC_SIGN_DIR 可换目录）"; exit 2; }
  TMPD=$(mktemp -d "${TMPDIR:-/tmp}/dm_mac_sign.XXXXXX") || { echo "🔴 建不了临时目录"; exit 3; }
  trap 'rm -rf "$TMPD"' EXIT
  security cms -D -i "$PF" > "$TMPD/p.plist" 2>/dev/null || { echo "🔴 解不开 $(basename "$PF")（不是有效的 provisioning profile）"; exit 3; }
  TEAM=$(/usr/libexec/PlistBuddy -c "Print :ApplicationIdentifierPrefix:0" "$TMPD/p.plist" 2>/dev/null || true)
  APPID=$(/usr/libexec/PlistBuddy -c "Print :Entitlements:com.apple.application-identifier" "$TMPD/p.plist" 2>/dev/null || true)
  [ -n "$APPID" ] || APPID=$(/usr/libexec/PlistBuddy -c "Print :Entitlements:application-identifier" "$TMPD/p.plist" 2>/dev/null || true)
  [ -n "$TEAM" ] && [ -n "$APPID" ] || { echo "🔴 读不出 $(basename "$PF") 的团队/包标识"; exit 3; }
  BID=${APPID#*.}

  plutil -extract Platform json -o - "$TMPD/p.plist" 2>/dev/null | grep -q '"OSX"' || { echo "🔴 $(basename "$PF") 不是 Mac 平台的 profile"; exit 3; }
  /usr/libexec/PlistBuddy -c "Print :Entitlements:com.apple.developer.icloud-container-identifiers:0" "$TMPD/p.plist" > /dev/null 2>&1 || { echo "🔴 $(basename "$PF") 不带 iCloud 容器"; exit 3; }
  EXP=$(/usr/libexec/PlistBuddy -c "Print :ExpirationDate" "$TMPD/p.plist" 2>/dev/null || true)
  EXPS=$(date -j -f "%a %b %d %T %Z %Y" "$EXP" "+%s" 2>/dev/null || echo 0)
  [ "$EXPS" -gt "$(date +%s)" ] || { echo "🔴 $(basename "$PF") 已过期（${EXP}）：重新下载一份放进签名目录"; exit 3; }
  echo "签名用 profile：$(basename "$PF")（包标识 ${BID}，到期 ${EXP}）"
fi
make -C "$DM/host" ${M3:+dmhost_m3} > /dev/null
MIN=$(make -s -C "$DM/host" print-macos-min${M3:+-m3})
APP="$OUT/$NAME.app"; rm -rf "$APP"; mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$DM/host/dmhost${M3:+_m3}" "$APP/Contents/MacOS/D2RMobile"
LINK=${DM_MAC_LINK-1}; [ "$LINK" != 0 ] || LINK=; RES="$APP/Contents/Resources"; : > "$RES/paths.txt"
if [ -n "$LINK" ]; then echo "product=$P" >> "$RES/paths.txt"; else cp -R "$P" "$RES/product"; fi
cp "$DM/gfx/nvn/shader_map.tsv" "$APP/Contents/Resources/shader_map.tsv"
cp "$DM/gfx/nvn/pso_manifest.tsv" "$APP/Contents/Resources/pso_manifest.tsv"
[ ! -f "$DM/gfx/nvn/c1_full.tsv" ] || cp "$DM/gfx/nvn/c1_full.tsv" "$APP/Contents/Resources/c1_full.tsv"


V=$(sed -n 's/.*DMSH_CHAIN "\(.*\)".*/\1/p' "$DM/gfx/nvn/dmsh_chain.h")


[ -z "$M3" ] || [ -n "${DM_MSL_DIR:-}" ] || { bash "$DM/gfx/tools/msl_retarget.sh" "$MIN" "$OUT/msl_min$MIN" > /dev/null || { echo "🔴 着色器库按 macOS ${MIN} 重编失败"; exit 2; }; DM_MSL_DIR=$OUT/msl_min$MIN; }
MSL=${DM_MSL_DIR:-$ROOT/build/cache/metal_shader_cache}/b-$V
[ -d "$MSL" ] || { echo "🔴 没有着色器库 ${MSL}（先跑 dm/gfx/tools/dmsh_build.py）"; exit 2; }
if [ -n "$LINK" ]; then echo "msl=$(dirname "$MSL")" >> "$RES/paths.txt"
else mkdir -p "$RES/msl/b-$V"; rsync -a --include='*.metallib' --include='*.json' --exclude='*' "$MSL/" "$RES/msl/b-$V/"; fi


if [ "${DM_MAC_OVERLAY:-}" != none ]; then
  OVD="$RES/romfs_overlay"; [ -z "$LINK" ] || { OVD="$OUT/romfs_overlay.local"; echo "romfs_overlay=$OVD" >> "$RES/paths.txt"; }


  if [ -n "${DM_MAC_OVERLAY:-}" ]; then
    [ "$(cd "$DM_MAC_OVERLAY" && pwd -P)" != "$(mkdir -p "$OVD" && cd "$OVD" && pwd -P)" ] || { echo "DM_MAC_OVERLAY must differ from the output overlay directory"; exit 2; }
  fi
  rm -rf "$OVD" "${OVD}_variants"
  if [ -n "${DM_MAC_OVERLAY:-}" ]; then
    cp -R "$DM_MAC_OVERLAY" "$OVD"
    [ ! -d "${DM_MAC_OVERLAY%/}_variants" ] || cp -R "${DM_MAC_OVERLAY%/}_variants" "${OVD}_variants"
  else DM_OVERLAY_MAC=1 python3 "$DM/ios/tools/gen_romfs_overlay.py" "${DM_ROMFS_SRC:-$ROOT/data/switch/$GV/Update/RomFS}" "$OVD" > /dev/null || { echo "🔴 界面覆盖层生成失败"; exit 2; }; fi
  [ -z "$LINK" ] || [ ! -d "${OVD}_variants" ] || echo "romfs_overlay_variants=${OVD}_variants" >> "$RES/paths.txt"
fi


if [ "${DM_MAC_ROMFS_COPY:-0}" = 1 ]; then
  ROM=${DM_ROMFS_SRC:-$ROOT/data/switch/$GV/Update/RomFS}
  [ -f "$ROM/data/build_manifest.psv" ] || { echo "🔴 DM_MAC_ROMFS_COPY=1 但资源里没有清单：$ROM/data/build_manifest.psv"; exit 2; }
  ditto "$ROM" "$RES/romfs"
fi




ICON_SRC=${DM_MAC_ICON_SOURCE:-$DM/ios/Assets.xcassets/AppIcon.appiconset/AppIcon1024.png}; ICON_KEY=


if [ -z "${DM_MAC_ICON_SOURCE:-}" ] && [ ! -f "$ICON_SRC" ]; then
  echo "提示：仓库里没有缺省图标 ${ICON_SRC}，这个包不带图标 | note: the repository's default icon ${ICON_SRC} is missing; building without an icon"; ICON_SRC=none
fi
if [ "$ICON_SRC" != none ]; then
  [ -f "$ICON_SRC" ] || { echo "🔴 DM_MAC_ICON_SOURCE 指的图不存在：$ICON_SRC"; exit 2; }
  ICONSET="$RES/AppIcon.iconset"; mkdir "$ICONSET"
  for size in 16 32 128 256 512; do
    sips -s format png -z "$size" "$size" "$ICON_SRC" --out "$ICONSET/icon_${size}x${size}.png" > /dev/null
    twice=$((size * 2))
    sips -s format png -z "$twice" "$twice" "$ICON_SRC" --out "$ICONSET/icon_${size}x${size}@2x.png" > /dev/null
  done
  iconutil -c icns "$ICONSET" -o "$RES/AppIcon.icns"
  rm -r "$ICONSET"
  ICON_KEY="<key>CFBundleIconFile</key><string>AppIcon</string>"
fi
if [ -z "$LINK" ] && [ -n "$(find "$RES" -type l -print -quit)" ]; then
  echo "🔴 自带资源的包里不许有符号链接（换到别的机器就断）"; exit 3
fi
[ -s "$RES/paths.txt" ] || rm -f "$RES/paths.txt"
VER=${DM_MAC_VERSION:-1.0}; BUILD=${DM_MAC_BUILD:-$(date +%m%d%H%M)}
echo "版本 ${VER} 构建 ${BUILD} 产物 $(basename "$P") 提交 $(git -C "$DM" rev-parse --short HEAD 2>/dev/null || echo 未知)" > "$RES/build.txt"
cat > "$APP/Contents/Info.plist" <<PL
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleName</key><string>$NAME</string>
<key>CFBundleDisplayName</key><string>$NAME</string>
<key>CFBundleIdentifier</key><string>$BID</string>
<key>CFBundleExecutable</key><string>D2RMobile</string>
${ICON_KEY}
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>$VER</string>
<key>CFBundleVersion</key><string>$BUILD</string>
<key>LSMinimumSystemVersion</key><string>$MIN</string>
${DM_MAC_UPDATE_URL:+<key>SWUpdateURL</key><string>$DM_MAC_UPDATE_URL</string>
}<key>NSHighResolutionCapable</key><true/>
<key>LSApplicationCategoryType</key><string>public.app-category.role-playing-games</string>
<key>GCSupportsControllerUserInteraction</key><true/>
</dict></plist>
PL


[ -s "$RES/pso_manifest.tsv" ] && [ -s "$RES/shader_map.tsv" ] || { echo "🔴 包里缺管线预热种子清单或着色器映射表（Resources/pso_manifest.tsv、shader_map.tsv）"; exit 3; }


[ -z "$M3" ] || { N4=$(nm "$APP/Contents/MacOS/D2RMobile" | grep -c MTL4 || true); [ "$N4" = 0 ] || { echo "🔴 旧接口版的程序本体里还有 ${N4} 个 Metal 4 符号"; exit 3; }; }


if [ -z "$LINK" ]; then ( cd "$RES/msl/b-$V" && for f in $(ls | grep '_b\.metallib$' | sed -n '1p;1000p;$p') $(ls | grep '_bf\.metallib$' | head -1); do
  LV=$(xcrun -sdk macosx metal-objdump -d "$f" 2>/dev/null | sed -n 's/.*target triple = ".*-apple-macosx\([0-9]*\.[0-9]*\).*/\1/p' | head -1)
  [ -n "$LV" ] && [ "$(printf '%s\n%s\n' "$LV" "$MIN" | sort -V | tail -1)" = "$MIN" ] || { echo "🔴 着色器库 ${f} 的目标系统是 macOS ${LV:-读不到}，高于包声明的最低系统 ${MIN}"; exit 3; }
done ) || exit 3; fi


BIN_MIN=$(otool -l "$APP/Contents/MacOS/D2RMobile" | awk '$1=="minos"{print $2; exit}'); PL_MIN=$(/usr/libexec/PlistBuddy -c "Print :LSMinimumSystemVersion" "$APP/Contents/Info.plist")
[ -n "$BIN_MIN" ] && [ "$BIN_MIN" = "$PL_MIN" ] || { echo "🔴 最低系统版本对不上：程序本体 ${BIN_MIN:-读不到}，Info.plist ${PL_MIN}"; exit 3; }
ARCHS=$(lipo -archs "$APP/Contents/MacOS/D2RMobile"); [ "$ARCHS" = arm64 ] || { echo "🔴 程序本体的架构不是只有 arm64：${ARCHS}"; exit 3; }
python3 "$HERE/mac_bundle.py" "$APP/Contents/MacOS/D2RMobile" "$DM/host/dmhost${M3:+_m3}"
if [ -z "$ICLOUD" ]; then
  codesign --force -s - "$APP" > /dev/null 2>&1 || { echo "🔴 本地签名失败"; exit 3; }
else

  cp "$PF" "$APP/Contents/embedded.provisionprofile"
  plutil -extract Entitlements xml1 -o "$TMPD/ent.plist" "$TMPD/p.plist" 2>/dev/null || { echo "🔴 读不出 profile 的 entitlements"; exit 3; }
  CERT_SHA=$(plutil -extract DeveloperCertificates.0 raw "$TMPD/p.plist" 2>/dev/null | tr -d '\n' | base64 -d 2>/dev/null | openssl x509 -noout -fingerprint -sha1 2>/dev/null | sed 's/.*=//; s/://g' | tr 'A-Z' 'a-z')
  [ -n "$CERT_SHA" ] || { echo "🔴 读不出 profile 内嵌证书的指纹"; exit 3; }
  ID=$(security find-identity -v -p codesigning 2>/dev/null | awk -v h="$CERT_SHA" 'tolower($2) == h { $1 = $2 = ""; sub(/^ */, ""); gsub(/^"|"$/, ""); print; exit }')
  [ -n "$ID" ] || { echo "🔴 钥匙串里没有与 profile 证书（指纹 ${CERT_SHA}）对应的签名身份：Xcode → Settings → Accounts 登录该团队会自动装"; exit 3; }

  codesign --force -s "$ID" --entitlements "$TMPD/ent.plist" "$APP" > /dev/null 2>&1 || { echo "🔴 团队证书签名失败"; exit 3; }
  codesign -d --entitlements :- "$APP" 2>/dev/null | plutil -p - 2>/dev/null | grep -q "icloud-container-identifiers" || { echo "🔴 签出来的包没有 iCloud 权限"; exit 3; }
  echo "签名：${ID}（iCloud 容器 $(/usr/libexec/PlistBuddy -c "Print :com.apple.developer.icloud-container-identifiers:0" "$TMPD/ent.plist" 2>/dev/null || echo "?")）"
fi
codesign --verify --deep --strict "$APP"
echo "MACAPP ${APP}（${M3:+旧接口版，}版本 ${VER} 构建 ${BUILD}，最低系统 macOS ${MIN}，产物 $(basename "$P")，$(du -sh "$APP" | cut -f1)${LINK:+，不带派生物：只能在本机跑}）"
