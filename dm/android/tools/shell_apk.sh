#!/bin/bash





set -u
KIND=debug; [ "${1:-}" = release ] && { KIND=release; shift; }
ALLOW_DEBUG_SIGNING=${DM_ALLOW_DEBUG_SIGNING:-}; for a in "$@"; do case $a in -PdmAllowDebugSigning=*) ALLOW_DEBUG_SIGNING=${a#-PdmAllowDebugSigning=} ;; esac; done
VARIANT=qa; for a in "$@"; do case $a in -PdmVariant=*) VARIANT=${a#-PdmVariant=} ;; esac; done

UNSIGNED=""; for a in "$@"; do case $a in -PdmUnsigned=true) UNSIGNED=1 ;; esac; done
case $VARIANT in qa) SFX="" ;; player) SFX=-player ;; *) echo "-PdmVariant must be qa or player, got '$VARIANT'"; exit 1 ;; esac
[ $VARIANT != player ] || KIND=release
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd); DM=$(cd "$HERE/../.." && pwd)
ROOT=${DM_ROOT:-$HOME/dm-work}; O=$ROOT/build/android_shell; G=$O/gen
ROMFS=${DM_ROMFS_SRC:-$ROOT/data/switch/3.3.94012/Update/RomFS}
PY=${DM_PYTHON:-$ROOT/build/cache/dm/venv/bin/python}
export JAVA_HOME=${JAVA_HOME:-$(brew --prefix openjdk@17 2> /dev/null)/libexec/openjdk.jdk/Contents/Home}
export ANDROID_HOME=${ANDROID_HOME:-$HOME/Library/Android/sdk}; export ANDROID_SDK_ROOT=$ANDROID_HOME
t() { echo "[$(date +%T)] $*"; }; die() { t "🔴 $*"; exit 1; }
[ -f "$ROMFS/data/build_manifest.psv" ] || die "没找到游戏文件 ${ROMFS}（DM_ROMFS_SRC）"
W=$O/tree$SFX
rm -rf "$G" "$O/ref" "$O/D2RMobile-android$SFX.apk" && mkdir -p "$G" "$O/ref" "$W" || die "建不了 $O"
python3 "$DM/android/tools/check_player_env.py" > "$O/check_player_env.log" 2>&1 || { cat "$O/check_player_env.log"; die "player 的 getenv 名单与源码不一致（dm/android/tools/check_player_env.py）"; }
if ! (cd "$DM/.." && git ls-files -z -co --exclude-standard dm THIRD-PARTY-NOTICES.md) > "$O/tree$SFX.lst" 2> /dev/null; then

  (cd "$DM/.." && find dm -type f ! -path "*/build/*" ! -path "*/.cxx/*" ! -path "*/.gradle/*" -print0; printf 'THIRD-PARTY-NOTICES.md\0') > "$O/tree$SFX.lst" || die "列不出源码文件"
fi






if [ -d "$W/dm" ]; then
  python3 -c '
import os, sys
w, keep = os.fsencode(sys.argv[1]), set(open(sys.argv[2], "rb").read().split(b"\0"))
out = {b"dm/android/app/build", b"dm/android/app/.cxx", b"dm/android/.gradle", b"dm/android/build"}
for top, dirs, files in os.walk(os.path.join(w, b"dm")):
    rel = os.path.relpath(top, w)
    for d in list(dirs):
        r = os.path.join(rel, d)
        if r in out: dirs.remove(d)
        elif os.path.islink(os.path.join(w, r)): dirs.remove(d); files.append(d)
    for f in files:
        r = os.path.join(rel, f)
        if r not in keep: os.remove(os.path.join(w, r))
' "$W" "$O/tree$SFX.lst" || die "清不掉副本里的旧文件"
fi

rsync -a --checksum --no-times -0 --ignore-missing-args --files-from="$O/tree$SFX.lst" "$DM/.." "$W/" || die "拷源码副本失败"
for f in host/dm_vault_tab.h host/dm_hud_exp.h host/dm_bright_levels.inc ios/Sources/dm_touch_native_art.c; do [ ! -e "$W/dm/$f" ] || die "副本里有生成文件 $f"; done
t "用本机游戏文件生成真表"
"$PY" "$DM/ios/tools/gen_vault_table.py" "$ROMFS" > "$G/dm_vault_tab.h" || die "生成 dm_vault_tab.h 失败"
"$PY" "$DM/ios/tools/gen_hud_exp.py" "$ROMFS" > "$G/dm_hud_exp.h" || die "生成 dm_hud_exp.h 失败"
"$PY" "$DM/tools/gen_bright_levels.py" "$ROMFS" > "$G/dm_bright_levels.inc" || die "生成 dm_bright_levels.inc 失败"
"$PY" "$DM/ios/tools/native_art/generate.py" "$ROMFS" --output-dir "$O/ref/native_art" > /dev/null || die "生成技能图标索引失败"
cp "$O/ref/native_art/dm_touch_native_art.c" "$G/" && cp "$G/dm_vault_tab.h" "$G/dm_hud_exp.h" "$G/dm_touch_native_art.c" "$O/ref/" || die "拷真表失败"
"$PY" "$DM/ios/tools/shell_stubs.py" data-gen "$G" "$O/ref/derived.tsv" || die "拆表失败"
"$PY" "$DM/ios/tools/shell_stubs.py" stubs-gen "$G" || die "写替身失败"
t "构建 APK（${VARIANT}）"
if [ $KIND = release ]; then
  TASK=:app:assembleRelease; OUT=$W/dm/android/app/build/outputs/apk/release/app-release.apk
  [ -z "$UNSIGNED" ] || OUT=$W/dm/android/app/build/outputs/apk/release/app-release-unsigned.apk
else TASK=:app:assembleDebug; OUT=$W/dm/android/app/build/outputs/apk/debug/app-debug.apk; fi
rm -f "$OUT"
rm -f "$W/dm/android/app/build/signing.mode" "$W/dm/android/app/build/signing.sha256"
if [ -z "${UNSIGNED:-}" ]; then
gradle --no-daemon -q -p "$W/dm/android" :app:writeSigningFingerprint -PdmGenDir="$G" -PdmPython="$PY" "$@" > "$O/signing$SFX.log" 2>&1 || { cat "$O/signing$SFX.log"; die "Signing check failed"; }
bash "$DM/android/tools/check_signing.sh" "$W/dm/android/app/build/signing.mode" "$KIND" "$ALLOW_DEBUG_SIGNING" || die "Release signing check failed"
fi
gradle --no-daemon -q -p "$W/dm/android" $TASK -PdmGenDir="$G" -PdmPython="$PY" "$@" > "$O/build$SFX.log" 2>&1 || { grep -E "error|FAIL" "$O/build$SFX.log" | head -8; die "构建失败，日志 $O/build$SFX.log"; }
cp "$OUT" "$O/D2RMobile-android$SFX.apk" || die "没找到构建出的 APK"




A=$O/D2RMobile-android$SFX.apk; L=$O/libdm$SFX.so; unzip -p "$A" lib/arm64-v8a/libdm.so > "$L" || die "APK 里没有 lib/arm64-v8a/libdm.so"


python3 "$DM/android/tools/check_driver_libs.py" "$A" > "$O/check_driver_libs$SFX.log" 2>&1 || { cat "$O/check_driver_libs$SFX.log"; die "驱动载入库核对没过（check_driver_libs.py）"; }
n=0; hit=0
GITOK=0; git -C "$DM" rev-parse --git-dir > /dev/null 2>&1 && GITOK=1
while IFS= read -r k; do [ ${#k} -ge 6 ] || continue; n=$((n + 1))
  LC_ALL=C grep -qaF -- "$k" "$L" || continue
  if [ "$GITOK" = 1 ]; then
    git -C "$DM" grep -qF -- "$k" -- . && continue
  else
    grep -rqF -- "$k" --include='*.c' --include='*.cpp' --include='*.cxx' --include='*.h' --include='*.hpp' \
      --include='*.inc' --include='*.json' --include='*.py' --include='*.sh' --include='*.ts' \
      --include='*.java' --include='*.kt' --include='*.xml' "$DM" && continue
  fi
  hit=$((hit + 1)); echo "  原生库里有：$k"
done < <(awk -F'\t' '$1=="VTROW"{print $8; print $9}' "$O/ref/derived.tsv" | sort -u)
[ "$n" -ge 100 ] || die "能核的字符串只有 $n 个，核不了"
[ "$hit" = 0 ] || die "原生库里出现了真表里的字符串（$hit 处 / 核了 $n 个）：替身没生效"

X=$O/apk_x$SFX; rm -rf "$X" && mkdir -p "$X" && (cd "$X" && unzip -q "$A") || die "解不开 APK"
leak=$(LC_ALL=C grep -raoE -- "/Users/[A-Za-z0-9._-]+|$HOME" "$X" | head -3)
[ -z "$leak" ] || die "APK 里带着本机路径：$leak"


bad=$(unzip -l "$A" | awk '{print $4}' | grep -E '\.(xgs|patched\.bin|data)$|^assets/|overlay|derived\.tsv' | grep -vE '^assets/builds/[a-z0-9-]{3,40}\.json$|^assets/glossary/terms\.json$|^assets/licenses/THIRD-PARTY-NOTICES\.txt$' | head -3)
[ -z "$bad" ] || die "APK 里带了不该有的文件：$bad"
G2=$O/apk_guides$SFX; rm -rf "$G2" && mkdir -p "$G2"
for f in $(unzip -l "$A" | awk '{print $4}' | grep -E '^assets/builds/'); do gn=${f#assets/builds/}
  [ "$gn" != schema.json ] && [ -f "$DM/android/builds/$gn" ] || die "APK 里的指南 $gn 不是仓库里的"
  unzip -p "$A" "$f" > "$G2/$gn" && cmp -s "$G2/$gn" "$DM/android/builds/$gn" || die "APK 里的指南 $gn 与仓库里的不同"; done

for f in "$DM"/android/builds/*.json; do gn=${f##*/}; [ "$gn" = schema.json ] || [ -f "$G2/$gn" ] || die "仓库里的指南 $gn 不在 APK 里"; done
if ls "$G2"/*.json > /dev/null 2>&1; then python3 "$DM/android/tools/check_builds.py" "$G2"/*.json > "$O/check_builds$SFX.log" || { cat "$O/check_builds$SFX.log"; die "构建指南没过 check_builds.py"; }; fi



unzip -p "$A" assets/licenses/THIRD-PARTY-NOTICES.txt > "$G2/notices.txt" 2> /dev/null && cmp -s "$G2/notices.txt" "$DM/../THIRD-PARTY-NOTICES.md" || die "APK 里的许可说明与仓库里的不同或缺失"
grep -q 'Permission is hereby granted' "$G2/notices.txt" || die "APK 里的许可说明不含任何许可正文"

if unzip -l "$A" | awk '{print $4}' | grep -qx 'assets/glossary/terms.json'; then
  unzip -p "$A" assets/glossary/terms.json > "$G2/glossary-terms.json" && cmp -s "$G2/glossary-terms.json" "$DM/android/glossary/terms.json" || die "APK 里的术语表与仓库里的不同"
  python3 "$DM/android/tools/check_glossary.py" "$G2/glossary-terms.json" > "$O/check_glossary$SFX.log" || { cat "$O/check_glossary$SFX.log"; die "术语表没过 check_glossary.py"; }; fi



if [ -n "${UNSIGNED:-}" ]; then
  if [ "$KIND" = release ]; then set -- --release; else set --; fi
  bash "$DM/android/tools/check_variant.sh" "$A" "$VARIANT" --no-signature "$@" --report "$O/player_env_report$SFX.txt" > "$O/check_variant$SFX.log" 2>&1 || { cat "$O/check_variant$SFX.log"; die "Variant verification failed: $VARIANT (unsigned)"; }
  t "好了（${KIND}，${VARIANT}，未签名）：${A}"; exit 0
fi
CERT=$(cat "$W/dm/android/app/build/signing.sha256") || die "Missing signing fingerprint"
if [ "$KIND" = release ]; then set -- --release; else set --; fi
bash "$DM/android/tools/check_variant.sh" "$A" "$VARIANT" --cert-sha256 "$CERT" "$@" --report "$O/player_env_report$SFX.txt" > "$O/check_variant$SFX.log" 2>&1 || { cat "$O/check_variant$SFX.log"; die "Variant verification failed: $VARIANT"; }

t "好了（${KIND}，${VARIANT}）：${A}（$(($(wc -c < "$A") / 1048576)) MB）；核了真表里 $n 个源码里没有的字符串，原生库里 0 处；APK 里没有游戏程序 / 着色器包 / 覆盖层，也没有本机路径"
