#!/bin/bash











set -euo pipefail

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd); DM=$(cd "$HERE/../.." && pwd)
KIND=${1:?usage: build_xlate.sh <android|host> <output folder>}; OUT=${2:?usage: build_xlate.sh <android|host> <output folder>}
case $KIND in android) RID=linux-bionic-arm64 ;; host) RID=linux-x64 ;; *) echo "build_xlate.sh: kind must be android or host"; exit 2 ;; esac
ROOT=${DM_ROOT:-$HOME/dm-work}
CACHE=${DM_XLATE_CACHE:-$ROOT/build/cache/android-xlate}
SDK=10.0.403
NDK=${ANDROID_HOME:-/nonexistent}/ndk/29.0.14206865
DOTNET=${DOTNET:-$(command -v dotnet || echo "${DOTNET_ROOT:-/nonexistent}/dotnet")}

say() { echo "[$(date +%T)] $*" >&2; }
die() { say "error: $*"; exit 1; }

mkdir -p "$CACHE"
exec 9> "$CACHE/.lock"
flock 9

SRCS=$(cd "$DM/.." && { git ls-files -co --exclude-standard dm/gfx/tools/dmsh_xlate dm/gfx/tools/dmsh_xlate_native dm/third_party/ryujinx 2> /dev/null \
  || find dm/gfx/tools/dmsh_xlate dm/gfx/tools/dmsh_xlate_native dm/third_party/ryujinx -type f ! -path '*/bin/*' ! -path '*/obj/*'; } | LC_ALL=C sort)
[ -n "$SRCS" ] || die "no translator sources found"
RECIPE=$( { echo "$KIND"; cat "$HERE/build_xlate.sh" "$HERE/xlate-packages.txt"; (cd "$DM/.." && for f in $SRCS; do echo "$f"; sha256sum < "$f"; done); } | sha256sum | cut -c1-16)
if [ -f "$OUT/RECIPE" ] && [ "$(cat "$OUT/RECIPE")" = "$RECIPE" ]; then say "translator $RECIPE is up to date in $OUT"; exit 0; fi
[ -x "$DOTNET" ] || die "dotnet not found (set DOTNET or DOTNET_ROOT)"
[ "$("$DOTNET" --version)" = "$SDK" ] || die "the .NET SDK must be $SDK, found $("$DOTNET" --version)"
W=$CACHE/work-$KIND; S=$W/src; N=$CACHE/nuget
rm -rf "$W"; mkdir -p "$S" "$W/out" "$W/home" "$N"
(cd "$DM/.." && for f in $SRCS; do mkdir -p "$S/$(dirname "$f")"; cp "$f" "$S/$f"; done)
printf '{ "sdk": { "version": "%s", "rollForward": "disable" } }\n' "$SDK" > "$S/global.json"

cat > "$S/NuGet.Config" <<'XML'
<?xml version="1.0" encoding="utf-8"?>
<configuration>
  <packageSources><clear /><add key="nuget.org" value="https://api.nuget.org/v3/index.json" /></packageSources>
</configuration>
XML
export DOTNET_CLI_HOME=$W/home DOTNET_CLI_TELEMETRY_OPTOUT=1 DOTNET_NOLOGO=1 DOTNET_SKIP_FIRST_TIME_EXPERIENCE=1 NUGET_PACKAGES=$N \
  NUGET_HTTP_CACHE_PATH=$CACHE/nuget-http NUGET_PLUGINS_CACHE_PATH=$CACHE/nuget-plugins MSBUILDDISABLENODEREUSE=1
args=(-c Release -r "$RID" -o "$W/out" -nologo -v quiet -p:ContinuousIntegrationBuild=true -p:Deterministic=true -p:PathMap="$S=/src"
      -p:DebugType=none -p:StripSymbols=true)
if [ "$KIND" = android ]; then
  [ -d "$NDK" ] || die "NDK 29.0.14206865 not found under \$ANDROID_HOME/ndk"

  export PATH=$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH
  args+=(-p:DisableUnsupportedError=true -p:PublishAotUsingRuntimePack=true)
fi
say "translator $RECIPE: publish for $RID"
(cd "$S" && "$DOTNET" publish dm/gfx/tools/dmsh_xlate_native/dmsh_xlate_native.csproj "${args[@]}") > "$W/publish.log" 2>&1 \
  || { tail -20 "$W/publish.log" >&2; die "publish failed, see $W/publish.log"; }

python3 - "$N" "$HERE/xlate-packages.txt" "$KIND" <<'PY' || die "the NuGet packages differ from xlate-packages.txt"
import os, sys
cache, listing, kind = sys.argv[1:]
pinned, need = {}, set()
for line in open(listing, encoding='utf-8'):
    if not line.strip() or line.startswith('#'):
        continue
    k, name, ver, sha = line.split()
    pinned[(name, ver)] = sha
    if k in ('all', kind):
        need.add((name, ver))
have = {}
for name in os.listdir(cache):
    for ver in os.listdir(os.path.join(cache, name)):
        f = os.path.join(cache, name, ver, '%s.%s.nupkg.sha512' % (name, ver))
        have[(name, ver)] = open(f).read().strip() if os.path.isfile(f) else '(no digest)'
bad = ['%s %s: %s, pinned %s' % (n, v, d, pinned.get((n, v), 'nothing')) for (n, v), d in sorted(have.items()) if pinned.get((n, v)) != d]
bad += ['%s %s is pinned for %s but was not restored' % (n, v, kind) for (n, v) in sorted(need - set(have))]
for b in bad:
    print(b, file=sys.stderr)
sys.exit(1 if bad else 0)
PY
L=$W/out/dmsh_xlate.so
[ -f "$L" ] || die "publish wrote no dmsh_xlate.so"
READELF=$(command -v llvm-readelf || echo "$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-readelf")
[ -x "$READELF" ] || READELF=readelf

syms=$("$READELF" --dyn-syms -W "$L" | awk '$4 == "FUNC" && $5 == "GLOBAL" && $7 != "UND" { print $8 }' | sed 's/@.*//' | LC_ALL=C sort | tr '\n' ' ')
[ "$syms" = "dmsh_xlate_free dmsh_xlate_one " ] || die "the library exports '$syms', expected dmsh_xlate_free dmsh_xlate_one"
if [ "$KIND" = android ]; then
  need=$("$READELF" -dW "$L" | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p' | LC_ALL=C sort | tr '\n' ' ')
  [ "$need" = "libc.so libdl.so libm.so " ] || die "the library needs '$need', expected libc.so libdl.so libm.so only"
  "$READELF" -lW "$L" | awk '$1 == "LOAD" && strtonum($NF) < 16384 { bad = 1 } END { exit bad }' || die "LOAD segments aligned below 16 KB"
fi
leak=$(LC_ALL=C grep -aoF -e "$HOME" -e "$W" -e "$CACHE" "$L" | head -3 || true)
[ -z "$leak" ] || die "the library names a folder of this machine: $leak"
rm -rf "$OUT.part"; mkdir -p "$OUT.part"
if [ "$KIND" = android ]; then mkdir -p "$OUT.part/jni/arm64-v8a" && cp "$L" "$OUT.part/jni/arm64-v8a/libdmsh_xlate.so"; else cp "$L" "$OUT.part/libdmsh_xlate.so"; fi

mkdir -p "$OUT.part/licenses"
cp "$S/dm/third_party/ryujinx/LICENSE.txt" "$OUT.part/licenses/ryujinx-LICENSE.txt"
if [ "$KIND" = android ]; then
  for f in LICENSE.TXT THIRD-PARTY-NOTICES.TXT; do
    src=$(ls "$N"/microsoft.netcore.app.runtime.nativeaot.linux-bionic-arm64/*/"$f" 2> /dev/null | head -1)
    [ -f "$src" ] || die "the NativeAOT runtime package has no $f"
    cp "$src" "$OUT.part/licenses/dotnet-$f"
  done
fi
echo "$RECIPE" > "$OUT.part/RECIPE"
rm -rf "$OUT" && mv "$OUT.part" "$OUT"
say "translator $RECIPE ready in $OUT ($(wc -c < "$L") bytes)"
