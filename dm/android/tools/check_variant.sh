#!/bin/bash


set -u
HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
APK=${1:-}; VARIANT=${2:-}; shift 2 2> /dev/null || true
RELEASE=0; CERT=""; PKG=""; KS=${ANDROID_USER_HOME:-$HOME/.android}/debug.keystore; REPORT=""; NOSIG=0
while [ $# -gt 0 ]; do case $1 in
  --cert-sha256) CERT=$(printf '%s' "${2:-}" | tr -d ':' | tr 'A-F' 'a-f'); shift 2 ;;
  --no-signature) NOSIG=1; shift ;;
  --release) RELEASE=1; shift ;;
  --package) PKG=${2:-}; shift 2 ;;
  --keystore) KS=${2:-}; shift 2 ;;
  --report) REPORT=${2:-}; shift 2 ;;
  *) echo "check_variant: unknown argument $1"; exit 2 ;;
esac; done
[ -f "$APK" ] && { [ "$VARIANT" = qa ] || [ "$VARIANT" = player ]; } || { echo "usage: check_variant.sh <apk> <qa|player> [--cert-sha256 <hex>] [--no-signature] [--package <id>] [--keystore <file>] [--report <file>]"; exit 2; }
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

  keep=$(sed -n 's/^ *X("\(DM_[A-Z0-9_]*\)").*/\1/p' "$HERE/../compat/include/dm_player_env.h" | sort -u)
  rest=$(LC_ALL=C grep -aoE 'DM_[A-Z][A-Z0-9_]*' "$L" | sort -u | grep -vxF -f <(printf '%s\n' "$keep") | tr '\n' ' ')
  echo "  INFO DM_* strings in libdm.so outside the getenv list (not readable at run time): ${rest:-none}"
  [ -n "$REPORT" ] && printf '%s\n' $rest > "$REPORT"
fi
[ $fail = 0 ] && echo "check_variant: $VARIANT OK" || echo "check_variant: $VARIANT FAILED"
exit $fail
