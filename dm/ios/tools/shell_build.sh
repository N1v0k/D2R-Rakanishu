#!/bin/bash


















set -u
KIND=${1:-ios}; case $KIND in ios|mac) ;; *) echo "用法：shell_build.sh [ios|mac]"; exit 2;; esac
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd); SRC=$(cd "$HERE/../.." && pwd)
ROOT=${DM_ROOT:-$HOME/dm-work}; O=$ROOT/build/shell/$KIND; W=$O/tree; DD=$O/dd
t() { echo "[$(date +%T)] $*"; }; die() { t "🔴 $*"; exit 1; }
mkdir -p "$O" && rm -rf "$W" "$O/ref" "$O/app" "$O/D2RMobile-shell-build-input.zip" "$O/shell_manifest.txt" && mkdir -p "$W" "$O/ref" || die "建不了 $O"
rsync -a --exclude '/ios/Gen' --exclude '/ios/DmIOS.xcodeproj' --exclude '.iosdd*' --exclude '/ios/local.env' "$SRC/" "$W/dm/" || die "拷源码树失败"


printf 'DM_TEAM_ID=0000000000\nDM_BUNDLE_ID=org.d2rmobile.app\nDM_FREE_ACCOUNT=1\n' > "$O/shell.env"; export DM_LOCAL_ENV=$O/shell.env DM_FREE_ACCOUNT=1 DM_ROOT=$ROOT
cd "$W/dm/ios" || die "副本里没有 dm/ios"
t "生成（与平时出包同一步：用本机游戏文件）"; DM_GEN_ONLY=1 DM_IOS_DD="$DD" bash run.sh build > "$O/gen.log" 2>&1 || { tail -5 "$O/gen.log"; die "生成失败，日志 $O/gen.log"; }


python3 tools/shell_stubs.py data "$W/dm" "$O/ref/derived.tsv" || die "拆表失败"
python3 tools/shell_stubs.py stubs "$W/dm" || die "写替身失败"


OUT=$(bash ../tools/product.sh path touch) || die "取不到产物目录"
python3 ../tools/guest_dylib.py "$OUT" "$O/ref" main sdk > /dev/null || die "对照用的动态库没写出来"
echo '// 壳：模块映像不在可执行文件里（tools/shell_build.sh）' > Gen/embed.S
sed -i '' -e 's/ -Wl,-segprot,__DM[A-Za-z0-9]*W,rw,rw//g' -e 's/^OTHER_CFLAGS = \$(inherited) /OTHER_CFLAGS = $(inherited) -DDM_SHELL /' Gen/dm.xcconfig
grep -q -- '-DDM_SHELL' Gen/dm.xcconfig && ! grep -q segprot Gen/dm.xcconfig || die "构建配置没改对：$(grep -n 'OTHER_' Gen/dm.xcconfig)"

case "$W$DD" in *' '*) die "工作目录带空格，编译参数里放不下：$W";; esac
WR=$(cd "$W" && pwd -P); sed -i '' -e "s#-DDM_SHELL #-DDM_SHELL -ffile-prefix-map=$W=src -ffile-prefix-map=$WR=src -ffile-prefix-map=$DD=build #" Gen/dm.xcconfig
grep -q -- '-ffile-prefix-map' Gen/dm.xcconfig || die "路径替换参数没加上：$(grep -n 'OTHER_CFLAGS' Gen/dm.xcconfig)"


if [ "$KIND" = mac ]; then DEST='platform=macOS,arch=arm64,variant=Designed for iPad'; PROD=Release-iphoneos; FAM=; else DEST='generic/platform=iOS'; PROD=Release-iphoneos; FAM=DM_DEVICE_FAMILY=1,2; fi
t "构建（不签名）"
xcodebuild -project DmIOS.xcodeproj -scheme DmIOS -configuration Release -destination "$DEST" -derivedDataPath "$DD" \
  CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY= $FAM build > "$O/build.log" 2>&1 || { grep -E 'error:' "$O/build.log" | head -5; die "构建失败，日志 $O/build.log"; }
APP=$(ls -d "$DD"/Build/Products/Release-*/DmIOS.app 2> /dev/null | head -1); [ -d "$APP" ] || die "没找到构建出的 App"
cp -R "$APP" "$O/app" || die "拷 App 失败"; A=$O/app




: > "$O/shell_manifest.txt"
take() {
  local p=${1%/}; [ -e "$A/$p" ] || return 0
  mkdir -p "$O/ref/$(dirname "$p")" && mv "$A/$p" "$O/ref/$p" || die "搬不走 $p"; [ "${2:-}" = nolist ] || echo "$1" >> "$O/shell_manifest.txt"; }
V=$(sed -n 's/^#define DMSH_CHAIN "\(.*\)"/\1/p' ../gfx/nvn/dmsh_chain.h); [ -n "$V" ] && [ -d "$A/msl/b-$V" ] || die "着色器库目录不对（链版本「${V}」）"
take "msl/b-$V/"; rmdir "$A/msl" 2> /dev/null
for d in romfs_overlay romfs_overlay_variants hud_v4_ui mapgoal mercslots; do take "$d/"; done


for d in save_ref save_ref_warlock romfs_overlay_astc romfs_overlay_astc_variants; do take "$d/" nolist; done
for f in "$A"/dm/*.fixups.tsv "$A"/dm/*.gates.tsv "$A"/dm/bootlist.tsv "$A"/dm/main.cuts.tsv "$A"/dm/c1_full.tsv "$A"/dm_native_*.png "$A"/native_art_manifest.json; do [ -e "$f" ] && take "${f#$A/}"; done
echo "dm/derived.tsv" >> "$O/shell_manifest.txt"; echo "Frameworks/Guest.framework/" >> "$O/shell_manifest.txt"


cp "$O/shell_manifest.txt" "$A/dm/shell_manifest.txt"


SD=$(python3 ../tools/d2rm_pack.py --src-digest "$ROOT/build/cache/metal_shader_cache/b-$V") && [ -n "$SD" ] || die "算不出着色器源码摘要"
SCV=$(brew list --versions spirv-cross 2> /dev/null | awk '{print $2}')
printf 'PRODUCT=%s\nCHAIN=%s\nKIND=%s\nSHADER_SRC=%s\nSPIRV_CROSS=%s\n' "$(head -1 ../PRODUCT)" "$V" "$KIND" "$SD" "${SCV:-unknown}" > "$A/dm/shell_info.txt"


rm -rf "$A/Extensions" "$A/PlugIns"
for k in BAAppGroupID BAHasManagedAssetPacks BAUsesAppleHosting; do /usr/libexec/PlistBuddy -c "Delete :$k" "$A/Info.plist" 2> /dev/null; done
rm -rf "$A/_CodeSignature" "$A/embedded.mobileprovision"

strip -S "$A/DmIOS" || die "strip 失败"
leak=$(LC_ALL=C grep -raoE -- "/Users/[A-Za-z0-9._-]+|$HOME" "$A" | head -3); [ -z "$leak" ] || die "壳里带着本机路径：$leak"


n=$(otool -l "$A/DmIOS" | grep -c -E 'sectname __dm.*x[01]|segname __DM.*W'); [ "$n" = 0 ] || die "壳的可执行文件里还有模块映像节 $n 个"




python3 - "$A" "$O/ref" "$W/dm" <<'PYEOF' || die "壳里搜到了游戏内容（见上）"
import os, sys
app, ref, src = sys.argv[1:]
needles = set()
for l in open(os.path.join(ref, 'derived.tsv'), encoding='utf-8'):
    f = l.rstrip('\n').split('\t')
    if f[0] in ('VTROW', 'VTSET'): needles.update(x.encode() for x in f[1:] if len(x) >= 8 and not x.lstrip('-').isdigit())
g = open(os.path.join(ref, 'Guest.framework', 'Guest'), 'rb').read(); needles.add(g[0x4000:0x4040]); needles.add(g[0x200000:0x200040])
# 仓库源码里本来就写着的字符串不算（少数物品名是我们的功能代码里点名用的，随源码公开）：副本里三份生成文件此时已是替身，搜到的都是手写源码
blob = b'\0'.join(open(os.path.join(d, fn), 'rb').read() for top in ('host', 'gfx', 'ios/Sources') for d, _, fs in os.walk(os.path.join(src, top)) for fn in fs if fn.endswith(('.c', '.m', '.h')))
own = {n for n in needles if n in blob}; needles -= own
bad = 0
for d, _, fs in os.walk(app):
    for fn in fs:
        b = open(os.path.join(d, fn), 'rb').read()
        hit = [n for n in needles if n in b]
        if hit: bad += 1; print('  %s 里有 %d 处对照内容，如 %r' % (os.path.relpath(os.path.join(d, fn), app), len(hit), hit[0][:40]))
print('内容核对：%d 个特征串（另有 %d 个是源码里本来就有的，不算）× %d 个文件，命中 %d 个文件' % (len(needles), len(own), sum(len(fs) for _, _, fs in os.walk(app)), bad)); sys.exit(1 if bad else 0)
PYEOF
( cd "$A" && find . -type f | sort | while read -r f; do printf '%s\t%s\n' "$(stat -f %z "$f")" "$f"; done ) > "$O/shell_files.tsv"








rm -rf "$O/Tools" && dotnet publish ../gfx/tools/dmsh_xlate -c Release -r osx-arm64 -p:PublishAot=true -p:StripSymbols=true -o "$O/Tools/dmsh_xlate/macos-arm64" -v quiet \
  -p:DebugType=none -p:DebugSymbols=false "-p:PathMap=$(cd ../.. && pwd)=/_/" && rm -rf "$O/Tools/dmsh_xlate/macos-arm64/"*.dSYM || die "预编着色器翻译工具（macos-arm64）失败"
XS=$("$O/Tools/dmsh_xlate/macos-arm64/dmsh_xlate" --source-sha) || die "预编的翻译工具起不来"
WX=${DM_XLATE_WIN_AOT:?要 DM_XLATE_WIN_AOT=<Windows 上出好的 NativeAOT 翻译工具目录>（见上面的说明） | needs DM_XLATE_WIN_AOT=<dir of the NativeAOT translator built on Windows> (see above)}
[ "$(head -c 2 "$WX/dmsh_xlate.exe")" = MZ ] || die "$WX/dmsh_xlate.exe 不是 Windows 程序"
mkdir -p "$O/Tools/dmsh_xlate/win-x64" && cp "$WX/dmsh_xlate.exe" "$O/Tools/dmsh_xlate/win-x64/" || die "放不进 Windows 版翻译工具"
[ -z "${DM_XLATE_WIN_SHA:-}" ] || [ "$DM_XLATE_WIN_SHA" = "$XS" ] || die "Windows 版翻译工具的源码指纹 $DM_XLATE_WIN_SHA 和本机这份 $XS 不同：两边不是同一份源码"

bash ../tools/deswizzle_build.sh "$O/Tools/deswizzle" || die "预编贴图解排布库失败"






SC=${DM_SPIRV_CROSS_DIR:?要 DM_SPIRV_CROSS_DIR=<放 macos-arm64/spirv-cross 与 win-x64/spirv-cross.exe 的目录>（见上面的说明） | needs DM_SPIRV_CROSS_DIR (see above)}
[ "$(head -c 2 "$SC/win-x64/spirv-cross.exe")" = MZ ] || die "$SC/win-x64/spirv-cross.exe 不是 Windows 程序"
"$SC/macos-arm64/spirv-cross" --help > /dev/null 2>&1; [ $? -le 1 ] && file "$SC/macos-arm64/spirv-cross" | grep -q "Mach-O.*arm64" || die "$SC/macos-arm64/spirv-cross 不是能跑的 macOS arm64 程序"
mkdir -p "$O/Tools/spirv-cross/macos-arm64" "$O/Tools/spirv-cross/win-x64" && cp "$SC/macos-arm64/spirv-cross" "$O/Tools/spirv-cross/macos-arm64/" && cp "$SC/win-x64/spirv-cross.exe" "$O/Tools/spirv-cross/win-x64/" || die "放不进 spirv-cross"
leak=$(LC_ALL=C grep -raoE -- "/Users/[A-Za-z0-9._-]+|$HOME" "$O/Tools" | head -3); [ -z "$leak" ] || die "预编的工具里带着本机路径：$leak"
( cd "$O" && rm -rf Payload && mkdir Payload && cp -R app Payload/DmIOS.app && zip -qry D2RMobile-shell-build-input.zip Payload Tools && rm -rf Payload ) || die "打包失败"
t "壳：$O/D2RMobile-shell-build-input.zip（$(du -h "$O/D2RMobile-shell-build-input.zip" | cut -f1)，$(wc -l < "$O/shell_files.tsv" | tr -d ' ') 个文件）；缺的内容 $(wc -l < "$O/shell_manifest.txt" | tr -d ' ') 项，清单 $O/shell_manifest.txt"
