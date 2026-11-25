#!/bin/bash



set -u
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
APK=${1:-}; VARIANT=${2:-}; shift 2 2> /dev/null || true
RELEASE=0; CERT=""; PKG=""; KS=${ANDROID_USER_HOME:-$HOME/.android}/debug.keystore; REPORT=""; NOSIG=0; PUBLIC=0
while [ $# -gt 0 ]; do case $1 in
  --public) PUBLIC=1; shift ;;
  --cert-sha256) CERT=$(printf '%s' "${2:-}" | tr -d ':' | tr 'A-F' 'a-f'); shift 2 ;;
  --no-signature) NOSIG=1; shift ;;
  --release) RELEASE=1; shift ;;
  --package) PKG=${2:-}; shift 2 ;;
  --keystore) KS=${2:-}; shift 2 ;;
  --report) REPORT=${2:-}; shift 2 ;;
  *) echo "check_variant: unknown argument $1"; exit 2 ;;
esac; done
[ -f "$APK" ] && { [ "$VARIANT" = qa ] || [ "$VARIANT" = player ]; } || { echo "usage: check_variant.sh <apk> <qa|player> [--cert-sha256 <hex>] [--no-signature] [--release] [--public] [--package <id>] [--keystore <file>] [--report <file>]"; exit 2; }
[ "$PUBLIC" = 0 ] || [ "$VARIANT" = player ] || { echo "check_variant: --public is for the player variant"; exit 2; }
BT=$(ls -d "${ANDROID_HOME:-/nonexistent}"/build-tools/* 2> /dev/null | tail -1)
[ -x "$BT/aapt2" ] && [ -x "$BT/apksigner" ] || { echo "check_variant: no aapt2 / apksigner under \$ANDROID_HOME/build-tools"; exit 2; }
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
unzip -q -o "$APK" 'lib/arm64-v8a/libdm.so' 'classes*.dex' -d "$T" || { echo "check_variant: cannot unpack $APK"; exit 2; }
L=$T/lib/arm64-v8a/libdm.so
fail=0
ok() { echo "  OK   $*"; }
bad() { echo "  FAIL $*"; fail=1; }
count() { LC_ALL=C grep -aoE -- "$1" "$2" | wc -l | tr -d ' '; }

echo "check_variant: $APK as $VARIANT"

badging=$("$BT/aapt2" dump badging "$APK" 2> /dev/null | head -1)
name=$(printf '%s' "$badging" | sed -n "s/.* versionName='\([^']*\)'.*/\1/p"); code=$(printf '%s' "$badging" | sed -n "s/.* versionCode='\([^']*\)'.*/\1/p"); pkg=$(printf '%s' "$badging" | sed -n "s/^package: name='\([^']*\)'.*/\1/p")
echo "  APK  package $pkg, versionCode $code, versionName $name"
debuggable=0
if "$BT/aapt2" dump xmltree --file AndroidManifest.xml "$APK" 2> /dev/null | grep -q 'android:debuggable.*=true\|android:debuggable(0x[0-9a-f]*)=(type 0x12)0xffffffff'; then debuggable=1; fi
expected_debug=0; if [ "$VARIANT" = qa ] && [ "$RELEASE" = 0 ]; then expected_debug=1; fi
[ "$debuggable" = "$expected_debug" ] && ok "G1 debuggable=$debuggable" || bad "G1 debuggable=$debuggable, expected $expected_debug"

got=$("$BT/apksigner" verify --print-certs "$APK" 2>&1 | sed -n 's/.*certificate SHA-256 digest: //p' | head -1)
if [ "$NOSIG" = 1 ]; then
  ok "G1 certificate check skipped (--no-signature: unsigned build)"
else
  if [ -z "$CERT" ]; then
    [ -f "$KS" ] || { bad "G1 no keystore $KS to compare the signature with"; KS=""; }
    [ -n "$KS" ] && CERT=$("${JAVA_HOME:+$JAVA_HOME/bin/}keytool" -list -v -keystore "$KS" -storepass android 2> /dev/null | sed -n 's/^.*SHA256: //p' | head -1 | tr -d ':' | tr 'A-F' 'a-f')
  fi
  if [ -n "$got" ] && [ "$got" = "$CERT" ]; then ok "G1 certificate SHA-256 $got"; else bad "G1 certificate SHA-256 ${got:-?}, expected ${CERT:-?}"; fi
fi
if [ -n "$PKG" ]; then [ "$pkg" = "$PKG" ] && ok "G1 package $pkg" || bad "G1 package $pkg, expected $PKG"; fi
case $VARIANT/$name in
  player/*-player) ok "G1 versionName $name ends in -player" ;;
  player/*) bad "G1 versionName $name lacks -player" ;;
  qa/*-player) bad "G1 versionName $name is a player name" ;;
  qa/*) ok "G1 versionName $name has no -player" ;;
esac

if [ "$VARIANT" = qa ]; then marks=('boot: variant qa' 'boot: 环境'); [ "$RELEASE" = 1 ] || marks+=('DM_TEST_RECIPE_STOCK' '\[qa\] '); else marks=('boot: variant player'); fi
for m in "${marks[@]}"; do n=$(count "$m" "$L"); [ "$n" -gt 0 ] && ok "G2 libdm.so has '$m' ($n)" || bad "G2 libdm.so lacks '$m'"; done
[ "$VARIANT" = player ] && [ "$(count 'boot: variant qa' "$L")" != 0 ] && bad "G2 player libdm.so carries 'boot: variant qa'"
[ "$VARIANT" = qa ] && [ "$(count 'boot: variant player' "$L")" != 0 ] && bad "G2 qa libdm.so carries 'boot: variant player'"
if [ "$VARIANT" = player ]; then

  for p in 'DM_[A-Z0-9_]*TEST' 'DM_KEYS' 'DM_UI_TAPS' 'DM_QA' '\[qa\]' 'dm-qa' 'state\.json' 'test hooks' 'boot: 环境'; do
    n=$(count "$p" "$L"); [ "$n" = 0 ] && ok "G3 libdm.so '$p' 0" || { bad "G3 libdm.so '$p' $n: $(LC_ALL=C grep -aoE -- "$p[A-Za-z0-9_]*" "$L" | sort -u | head -8 | tr '\n' ' ')"; }
  done

  ndex=$(ls "$T"/classes*.dex 2> /dev/null | wc -l | tr -d ' ')
  [ "$ndex" -gt 0 ] || bad "G4 no classes*.dex in the APK"
  for p in dmArtDump nativeOverlay nativeEnabled; do
    n=0; for d in "$T"/classes*.dex; do n=$((n + $(count "$p" "$d"))); done
    [ "$n" = 0 ] && ok "G4 $ndex dex files '$p' 0" || bad "G4 dex '$p' $n"
  done

  TSV=$HERE/player_markers.tsv
  unknown=$(LC_ALL=C grep -aoE 'gfx-[a-z0-9]+(-[a-z0-9]+)*' "$L" | sort -u | while read -r g; do awk -F'\t' -v g="$g" '$1 == g && ($2 == "allow" || $2 == "deny") {f = 1} END {exit !f}' "$TSV" || printf '%s ' "$g"; done)
  allow=$(awk -F'\t' '!/^#/ && $2 == "allow" {print $1}' "$TSV" | sort | tr '\n' ' ')
  ngfx=$(LC_ALL=C grep -aoE 'gfx-[a-z0-9]+(-[a-z0-9]+)*' "$L" | sort -u | wc -l | tr -d ' ')
  [ -z "$unknown" ] && ok "G5 all $ngfx gfx-* names classified in player_markers.tsv" || bad "G5 gfx-* names missing from player_markers.tsv: $unknown"
  [ "$allow" = "gfx-pipe-inflight gfx-pipe-skip gfx-pipe-suspect " ] && ok "G5 allow = crash guard state files only" || bad "G5 allow entries are '$allow' (player_guard.h lets through only gfx-pipe-inflight.<n>, gfx-pipe-suspect, gfx-pipe-skip)"


  entries=$(unzip -Z1 "$APK")
  for f in lib/arm64-v8a/libpackbuild.so lib/arm64-v8a/libdeswizzle.so; do
    printf '%s\n' "$entries" | grep -qxF "$f" && ok "G6 baseline $f" || bad "G6 baseline $f missing"
  done
  builder=(assets/py/VERSION assets/py/runtime.zip assets/py/tools.zip assets/py/queries.zip lib/arm64-v8a/libpylauncher.so 'lib/arm64-v8a/libpython3.*.so'
           lib/arm64-v8a/libdmsh_xlate.so lib/arm64-v8a/libdmsh_shaders.so lib/arm64-v8a/libmonpic.so lib/arm64-v8a/libfreetype.so lib/arm64-v8a/libharfbuzz.so
           lib/arm64-v8a/libfribidi.so lib/arm64-v8a/libwebp.so lib/arm64-v8a/libwebpmux.so lib/arm64-v8a/libwebpdemux.so lib/arm64-v8a/libsharpyuv.so)
  have=0; lack=""
  for f in "${builder[@]}"; do
    case $f in *'*'*) n=$(printf '%s\n' "$entries" | grep -cxE "$(printf '%s' "$f" | sed 's/\./\\./g; s/\*/[0-9]+/')") ;; *) n=$(printf '%s\n' "$entries" | grep -cxF "$f") ;; esac
    [ "$n" = 1 ] && have=$((have + 1)) || lack="$lack $f"
  done
  extra=$(printf '%s\n' "$entries" | grep -E '^assets/py/' | grep -vxE 'assets/py/(VERSION|runtime\.zip|tools\.zip|queries\.zip)' | head -3 | tr '\n' ' ')
  [ -z "$extra" ] || bad "G6 unknown files under assets/py/: $extra"
  if [ "$have" = "${#builder[@]}" ]; then
    BUILDER=1; ok "G6 pack builder complete (${#builder[@]} parts)"
    xt=$("$BT/aapt2" dump xmltree --file AndroidManifest.xml "$APK" 2> /dev/null)
    printf '%s\n' "$xt" | grep -qE 'android:extractNativeLibs\([^)]*\)=\(type 0x12\)0xffffffff|android:extractNativeLibs.*=true' \
      && ok "G6 native libraries are extracted at install (the launcher runs from there)" || bad "G6 the pack builder needs extractNativeLibs=true"
  elif [ "$have" = 0 ]; then
    BUILDER=0; ok "G6 no pack builder (imports computer packs only)"
  else
    BUILDER=partial; bad "G6 partial pack builder ($have of ${#builder[@]}), missing:$lack"
  fi


  QA_SRC=$HERE/../app/src/qa/java/com/xiaoboluo/d2r; PL_SRC=$HERE/../app/src/player/java/com/xiaoboluo/d2r
  marks=$( { for f in "$QA_SRC"/*.java; do n=$(basename "$f" .java); [ -f "$PL_SRC/$n.java" ] || echo "Lcom/xiaoboluo/d2r/$n;"; done
             cat "$QA_SRC"/*.java | grep -oE '"qa-[a-z0-9-]+"' | tr -d '"'; } | sort -u)
  [ -n "$marks" ] || bad "G7 no qa classes or marks found under $QA_SRC"
  hits=""
  for m in $marks; do n=0; for d in "$T"/classes*.dex; do n=$((n + $(LC_ALL=C grep -acF -- "$m" "$d"))); done; [ "$n" = 0 ] || hits="$hits $m"; done
  [ -z "$hits" ] && ok "G7 dex carries none of $(printf '%s\n' $marks | wc -l | tr -d ' ') qa classes and marks" || bad "G7 dex carries qa code:$hits"
  if [ "$BUILDER" = 1 ]; then
    mkdir -p "$T/tools" && unzip -q -o "$APK" assets/py/tools.zip -d "$T" && (cd "$T/tools" && unzip -q "$T/assets/py/tools.zip") || bad "G7 assets/py/tools.zip does not unpack"
    (cd "$T/tools" && find . -type f | sed 's|^\./||' | LC_ALL=C sort) > "$T/tools.have"
    if python3 -B -c 'import os, sys; sys.path.insert(0, sys.argv[1]); import pack_assets; print("\n".join(sorted(pack_assets.tool_files(sys.argv[2], variant="player"))))' \
        "$HERE/../python" "$HERE/../../.." 2> "$T/tools.err" | LC_ALL=C sort > "$T/tools.want"; then
      d=$(diff "$T/tools.want" "$T/tools.have" | grep '^[<>]' | head -4 | tr '\n' ' ')
      [ -z "$d" ] && ok "G7 tool tree = the player's list ($(wc -l < "$T/tools.have" | tr -d ' ') files)" || bad "G7 tool tree differs from the player's list: $d"
    else bad "G7 the player's tool list cannot be made: $(tail -1 "$T/tools.err")"; fi
    qa=$(grep -E '^qa ' "$HERE/../python/tools.txt" | sed 's/^qa *//' | while read -r q; do grep -qxF "$q" "$T/tools.have" && printf '%s ' "$q"; done)
    [ -z "$qa" ] && ok "G7 tool tree has no qa-only file" || bad "G7 tool tree carries qa-only files: $qa"

    sw=$(cd "$T/tools" && LC_ALL=C grep -rlE 'DM_(TEST|QA)[A-Z0-9_]*' . | sed 's|^\./||' | grep -vxF dm/host/dm_kills.c | head -5 | tr '\n' ' ')
    [ -z "$sw" ] && ok "G7 tool tree has no test switch (dm_kills.c excepted: a table source)" || bad "G7 tool tree carries test switches in: $sw"
  fi
  if [ "$PUBLIC" = 1 ]; then
    [ "$BUILDER" = 1 ] && ok "public: pack builder complete" || bad "public: the published APK must carry the complete pack builder"
    root=$("$BT/aapt2" dump xmltree --file AndroidManifest.xml "$APK" 2> /dev/null | grep -A2 'android:name.*"dmSharedRoot"' | sed -n 's/.*android:value[^"]*"\([^"]*\)".*/\1/p' | head -1)
    [ "$root" = "S2X/D2R Mobile" ] && ok "public: shared folder '$root'" || bad "public: shared folder '${root:-?}', expected 'S2X/D2R Mobile'"
    [ "$pkg" = io.r4k4n1shu.d2r ] && ok "public: application ID $pkg" || bad "public: application ID $pkg, expected io.r4k4n1shu.d2r"
  fi

  keep=$(sed -n 's/^ *X("\(DM_[A-Z0-9_]*\)").*/\1/p' "$HERE/../compat/include/dm_player_env.h" | sort -u)
  rest=$(LC_ALL=C grep -aoE 'DM_[A-Z][A-Z0-9_]*' "$L" | sort -u | grep -vxF -f <(printf '%s\n' "$keep") | tr '\n' ' ')
  echo "  INFO DM_* strings in libdm.so outside the getenv list (not readable at run time): ${rest:-none}"
  [ -n "$REPORT" ] && printf '%s\n' $rest > "$REPORT"
fi
[ $fail = 0 ] && echo "check_variant: $VARIANT OK" || echo "check_variant: $VARIANT FAILED"
exit $fail
