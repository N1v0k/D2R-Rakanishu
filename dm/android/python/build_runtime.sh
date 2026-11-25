#!/usr/bin/env bash














set -euo pipefail

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
OUT=${1:?usage: build_runtime.sh <output folder>}
ROOT=${DM_ROOT:-$HOME/dm-work}
CACHE=${DM_PY_CACHE:-$ROOT/build/cache/android-python}
NDK=${ANDROID_HOME:?ANDROID_HOME is not set}/ndk/29.0.14206865
API=24

PY_VER=3.14
PY_REL=3.14.10
PY_TAG=cpython-314-aarch64-linux-android

SOURCES="
python-$PY_REL-aarch64-linux-android.tar.gz https://www.python.org/ftp/python/$PY_REL/python-$PY_REL-aarch64-linux-android.tar.gz 423c80b957330cf0ebb92b9b20f2294c292144a441bdb0ceb8c69d14928e569b
Python-$PY_REL.tar.xz https://www.python.org/ftp/python/$PY_REL/Python-$PY_REL.tar.xz de414a57119abef52784d8993e341d53aed635fc2b05c2607d9f98a2c5b72032
zlib-1.3.tar.gz https://zlib.net/fossils/zlib-1.3.tar.gz ff0ba4c292013dbc27530b3a81e1f9a813cd39de01ca5e0f8bf355702efa593e
libpng-1.6.53.tar.gz https://download.sourceforge.net/libpng/libpng-1.6.53.tar.gz da0b045cbb1d06a8fc9696f9441359f70645f280ff24ae453ccb7c722353654f
brotli-1.2.0.tar.gz https://github.com/google/brotli/archive/refs/tags/v1.2.0.tar.gz 816c96e8e8f193b40151dad7e8ff37b1221d019dbcb9c35cd3fadbfe6477dfec
libjpeg-turbo-3.1.3.tar.gz https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.1.3/libjpeg-turbo-3.1.3.tar.gz 075920b826834ac4ddf97661cc73491047855859affd671d52079c6867c1c6c0
libwebp-1.6.0.tar.gz https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-1.6.0.tar.gz e4ab7009bf0629fd11982d4c2aa83964cf244cffba7347ecd39019a9e38c4564
freetype-2.14.1.tar.gz https://download.savannah.gnu.org/releases/freetype/freetype-2.14.1.tar.gz 174d9e53402e1bf9ec7277e22ec199ba3e55a6be2c0740cb18c0ee9850fc8c34
harfbuzz-12.3.0.tar.xz https://github.com/harfbuzz/harfbuzz/releases/download/12.3.0/harfbuzz-12.3.0.tar.xz 8660ebd3c27d9407fc8433b5d172bafba5f0317cb0bb4339f28e5370c93d42b7
fribidi-1.0.13.tar.xz https://github.com/fribidi/fribidi/releases/download/v1.0.13/fribidi-1.0.13.tar.xz 7fa16c80c81bd622f7b198d31356da139cc318a63fc7761217af4130903f54a2
numpy-2.4.4.tar.gz https://files.pythonhosted.org/packages/source/n/numpy/numpy-2.4.4.tar.gz 2d390634c5182175533585cc89f3608a4682ccb173cc9bb940b2881c8d6f8fa0
pillow-12.1.1.tar.gz https://files.pythonhosted.org/packages/source/p/pillow/pillow-12.1.1.tar.gz 9ad8fa5937ab05218e2b6a4cff30295ad35afd2f83ac592e68c0d871bb0fdbc4
lz4-4.4.5.tar.gz https://files.pythonhosted.org/packages/source/l/lz4/lz4-4.4.5.tar.gz 5f0b9e53c1e82e88c10d7c180069363980136b9d7a8306c4dca4f760d60c39f0
capstone-5.0.9.tar.gz https://files.pythonhosted.org/packages/source/c/capstone/capstone-5.0.9.tar.gz 0429af292ddc604d3c9344696807e281d5e728db029f00e6a4ea9e3bff1aac9e
pip-25.3.pyz https://bootstrap.pypa.io/pip/zipapp/pip-25.3.pyz e6f15b8635aae29aa2aa3d5c0e5b19d8998f8012664ce091d79ef2ef0d6b25e9
"

say() { echo "[$(date +%T)] $*" >&2; }
die() { say "error: $*"; exit 1; }

mkdir -p "$CACHE"
exec 9> "$CACHE/.lock"
flock 9
RECIPE=$(cat "$HERE/build_runtime.sh" "$HERE/launcher.c" "$HERE/build-requirements.txt" "$HERE/build-wheel-requirements.txt" "$HERE/pack_zip.py" "$HERE/elf_clean.py" | sha256sum | cut -c1-16)
if [ -f "$OUT/RECIPE" ] && [ "$(cat "$OUT/RECIPE")" = "$RECIPE" ]; then say "runtime $RECIPE is up to date in $OUT"; exit 0; fi
[ -d "$NDK" ] || die "NDK 29.0.14206865 not found under $ANDROID_HOME/ndk"
TC=$NDK/toolchains/llvm/prebuilt/linux-x86_64
CC=$TC/bin/aarch64-linux-android$API-clang; CXX=$CC++; STRIP=$TC/bin/llvm-strip; READELF=$TC/bin/llvm-readelf
CACHE=$(cd "$CACHE" && pwd)


DL=$CACHE/dl; W=/tmp/dm-pyrt-$(printf '%s' "$CACHE" | sha256sum | cut -c1-12)-$RECIPE; P=$W/prefix; B=$W/build
mkdir -p "$DL"; rm -rf "$W"; mkdir -p "$P/lib/pkgconfig" "$P/include" "$B"

PFX="-ffile-prefix-map=$W=/build -ffile-prefix-map=$NDK=/ndk -ffile-prefix-map=$CACHE=/cache"


src() { echo "$SOURCES" | awk -v n="$1" '$1 == n { print $0 }'; }
fetch() {
  local line name url sum; line=$(src "$1"); [ -n "$line" ] || die "no source entry $1"
  read -r name url sum <<< "$line"
  if [ ! -f "$DL/$name" ] || ! echo "$sum  $DL/$name" | sha256sum -c --quiet - 2> /dev/null; then
    # a stalled transfer (under 1 KB/s for a minute) is broken off and tried again, so a slow mirror never holds the build forever
    say "download $name"; curl -fsSL --connect-timeout 30 --speed-limit 1024 --speed-time 60 --retry 4 --retry-all-errors -o "$DL/$name.part" "$url" \
      || die "download of $name failed"
    echo "$sum  $DL/$name.part" | sha256sum -c --quiet - || die "checksum of $name does not match"
    mv "$DL/$name.part" "$DL/$name"
  fi
}
for n in $(echo "$SOURCES" | awk 'NF { print $1 }'); do fetch "$n"; done
unpack() { mkdir -p "$B/$1"; tar -xf "$DL/$2" -C "$B/$1" --strip-components=1; }

# 2. Host tools in their own environment, every wheel pinned by hash
python3 -m venv --without-pip "$W/venv"
"$W/venv/bin/python" "$DL/pip-25.3.pyz" -q install --require-hashes --only-binary=:all: -r "$HERE/build-requirements.txt"
export PATH=$W/venv/bin:$PATH
# what the wheel builds install into their own environments, pinned by hash; those builds install from this wheelhouse and nothing else
PIP_CACHE_DIR="$CACHE/pip" "$W/venv/bin/python" "$DL/pip-25.3.pyz" -q download --require-hashes --no-deps --only-binary=:all: --python-version 3.14 \
  --implementation cp --abi cp314 --abi abi3 --abi none --platform manylinux_2_17_x86_64 --platform manylinux2014_x86_64 \
  --platform manylinux_2_28_x86_64 --platform manylinux_2_5_x86_64 --platform manylinux1_x86_64 --platform any \
  -d "$W/wheelhouse" -r "$HERE/build-wheel-requirements.txt" || die "the wheel builds' requirements do not download with their hashes"

# 3. The official runtime (its prefix holds libpython, the standard library and its extension modules)
mkdir -p "$W/cpy"; tar -xzf "$DL/python-$PY_REL-aarch64-linux-android.tar.gz" -C "$W/cpy"
CPY=$W/cpy/prefix

# 4. Pillow's C libraries at the versions the PC wheel of pillow 12.1.1 uses. FreeType, HarfBuzz, WebP and FriBiDi are shared
#    libraries as in the PC wheel; zlib, libpng, brotli and libjpeg-turbo are static. zlib is vanilla 1.3: the PC wheel loads
#    the system zlib 1.3 at run time, so 1.3 is what produces the PC's PNG bytes (never the Android system zlib).
CM=(-G Ninja -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=$API
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$P -DCMAKE_PREFIX_PATH=$P -DCMAKE_FIND_ROOT_PATH=$P
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF "-DCMAKE_C_FLAGS=-D__BIONIC_NO_PAGE_SIZE_MACRO $PFX" "-DCMAKE_CXX_FLAGS=$PFX"
    -DCMAKE_SHARED_LINKER_FLAGS=-Wl,-z,max-page-size=16384)
cm() { local d=$1; shift; say "build $d"
  { cmake -S "$B/$d" -B "$B/$d/_b" "${CM[@]}" "$@" && cmake --build "$B/$d/_b" && cmake --install "$B/$d/_b"; } > "$B/$d.log" 2>&1 || die "$d failed, see $B/$d.log"; }
unpack zlib zlib-1.3.tar.gz
{ cmake -S "$B/zlib" -B "$B/zlib/_b" "${CM[@]}" && cmake --build "$B/zlib/_b" --target zlibstatic; } > "$B/zlib.log" 2>&1 || die "zlib failed"
cp "$B/zlib/_b/libz.a" "$P/lib/" && cp "$B/zlib/zlib.h" "$B/zlib/_b/zconf.h" "$P/include/"
printf 'prefix=%s\nlibdir=${prefix}/lib\nincludedir=${prefix}/include\nName: zlib\nDescription: zlib\nVersion: 1.3\nLibs: -L${libdir} -lz\nCflags: -I${includedir}\n' "$P" > "$P/lib/pkgconfig/zlib.pc"
ZL=(-DZLIB_ROOT=$P -DZLIB_LIBRARY=$P/lib/libz.a -DZLIB_INCLUDE_DIR=$P/include)
unpack libpng libpng-1.6.53.tar.gz; cm libpng -DPNG_SHARED=OFF -DPNG_TESTS=OFF -DPNG_TOOLS=OFF "${ZL[@]}"
unpack brotli brotli-1.2.0.tar.gz; cm brotli -DBROTLI_DISABLE_TESTS=ON -DBROTLI_BUILD_TOOLS=OFF
unpack jpeg libjpeg-turbo-3.1.3.tar.gz; cm jpeg -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_TURBOJPEG=OFF
unpack webp libwebp-1.6.0.tar.gz; cm webp -DBUILD_SHARED_LIBS=ON -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_CWEBP=OFF -DWEBP_BUILD_DWEBP=OFF \
  -DWEBP_BUILD_GIF2WEBP=OFF -DWEBP_BUILD_IMG2WEBP=OFF -DWEBP_BUILD_VWEBP=OFF -DWEBP_BUILD_WEBPINFO=OFF -DWEBP_BUILD_WEBPMUX=OFF \
  -DWEBP_BUILD_EXTRAS=OFF -DWEBP_BUILD_LIBWEBPMUX=ON
unpack freetype freetype-2.14.1.tar.gz; cm freetype -DBUILD_SHARED_LIBS=ON -DFT_REQUIRE_ZLIB=ON -DFT_REQUIRE_PNG=ON -DFT_REQUIRE_BROTLI=ON \
  -DFT_DISABLE_BZIP2=ON -DFT_DISABLE_HARFBUZZ=ON "${ZL[@]}" -DPNG_LIBRARY=$P/lib/libpng16.a -DPNG_PNG_INCLUDE_DIR=$P/include \
  "-DBROTLIDEC_LIBRARIES=$P/lib/libbrotlidec.a;$P/lib/libbrotlicommon.a"
cat > "$B/cross.txt" << EOT
[binaries]
c = '$CC'
cpp = '$CXX'
ar = '$TC/bin/llvm-ar'
strip = '$STRIP'
pkg-config = 'pkg-config'
[properties]
pkg_config_libdir = '$P/lib/pkgconfig'
[built-in options]
c_args = ['-D__BIONIC_NO_PAGE_SIZE_MACRO', '-fPIC', '-I$P/include', $(printf "'%s', " $PFX | sed 's/, $//')]
cpp_args = ['-D__BIONIC_NO_PAGE_SIZE_MACRO', '-fPIC', '-I$P/include', $(printf "'%s', " $PFX | sed 's/, $//')]
c_link_args = ['-L$P/lib', '-Wl,-z,max-page-size=16384']
cpp_link_args = ['-L$P/lib', '-Wl,-z,max-page-size=16384', '-static-libstdc++']
[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
EOT
ms() { local d=$1; shift; say "build $d"
  (cd "$B/$d" && meson setup _b --cross-file "$B/cross.txt" --prefix="$P" --libdir=lib "$@" && ninja -C _b && ninja -C _b install) > "$B/$d.log" 2>&1 || die "$d failed, see $B/$d.log"; }
unpack harfbuzz harfbuzz-12.3.0.tar.xz; ms harfbuzz --buildtype=minsize -Dfreetype=enabled -Dglib=disabled -Dgobject=disabled -Dcairo=disabled \
  -Dicu=disabled -Dtests=disabled -Ddocs=disabled -Dutilities=disabled -Ddefault_library=shared
# FriBiDi (LGPL-2.1+) stays a separate shared library that pillow loads at run time, so it can be replaced on its own
unpack fribidi fribidi-1.0.13.tar.xz; ms fribidi --buildtype=release -Ddocs=false -Dbin=false -Dtests=false -Ddefault_library=shared

# 5. Python's zlib module against the same vanilla zlib 1.3 (the official build links the Android system zlib, a fork)
unpack cpysrc "Python-$PY_REL.tar.xz"
"$CC" -O2 -shared -fPIC -D__BIONIC_NO_PAGE_SIZE_MACRO $PFX -DPy_BUILD_CORE_MODULE -I"$CPY/include/python$PY_VER" -I"$CPY/include/python$PY_VER/internal" \
  -I"$P/include" "$B/cpysrc/Modules/zlibmodule.c" "$P/lib/libz.a" -L"$CPY/lib" -lpython$PY_VER -Wl,-z,max-page-size=16384 -Wl,--no-undefined \
  -o "$B/zlib.$PY_TAG.so" || die "zlib module failed"

# 6. The four packages as wheels, through cibuildwheel's Android platform. cibuildwheel's helper script for CPython asks for NDK
#    27.3; an own SDK view maps that name onto NDK r29, so every compiler call is r29 and nothing is installed into the SDK.
mkdir -p "$W/android-home/ndk" "$W/wheels" "$W/tmp"; ln -s "$NDK" "$W/android-home/ndk/27.3.13750724"
cat > "$B/numpy-cross.txt" << EOT
[binaries]
c = '$CC'
cpp = '$CXX'
ar = '$TC/bin/llvm-ar'
strip = '$STRIP'
[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
[properties]
needs_exe_wrapper = true
longdouble_format = 'IEEE_QUAD_LE'
[built-in options]
c_args = ['-D__BIONIC_NO_PAGE_SIZE_MACRO', $(printf "'%s', " $PFX | sed 's/, $//')]
cpp_args = ['-D__BIONIC_NO_PAGE_SIZE_MACRO', $(printf "'%s', " $PFX | sed 's/, $//')]
EOT
wheel() { local d=$1; shift; say "wheel $d"
  (cd "$B/$d" && env ANDROID_HOME="$W/android-home" ANDROID_SDK_ROOT="$W/android-home" TMPDIR="$W/tmp" CIBW_CACHE_PATH="$CACHE/cibw" \
     PIP_CACHE_DIR="$CACHE/pip" XDG_CACHE_HOME="$CACHE/xdg" PIP_NO_INDEX=1 PIP_FIND_LINKS="$W/wheelhouse" CIBW_BUILD=cp314-android_arm64_v8a CIBW_ARCHS=arm64_v8a CIBW_BUILD_VERBOSITY=1 "$@" \
     cibuildwheel --platform android --output-dir "$W/wheels" .) > "$B/wheel-$d.log" 2>&1 || die "wheel $d failed, see $B/wheel-$d.log"; }
unpack lz4 lz4-4.4.5.tar.gz; wheel lz4 CIBW_ENVIRONMENT="CFLAGS=\"\$CFLAGS $PFX\""
unpack capstone capstone-5.0.9.tar.gz; wheel capstone CIBW_ENVIRONMENT="CAPSTONE_ARCHS=aarch64 CFLAGS=\"\$CFLAGS $PFX\""
unpack numpy numpy-2.4.4.tar.gz
# numpy's own [tool.cibuildwheel] table serves its CI: its build-group name is rejected by this cibuildwheel, and its hooks
# install OpenBLAS and run the test suite; the build here has no BLAS (as measured in the probe) and runs no hooks
sed -i '/^enable = \[/d' "$B/numpy/pyproject.toml"
wheel numpy CIBW_BEFORE_BUILD='' CIBW_BEFORE_TEST='' CIBW_TEST_COMMAND='' CIBW_CONFIG_SETTINGS="setup-args=--cross-file=$B/numpy-cross.txt setup-args=-Dallow-noblas=true setup-args=-Dblas=none setup-args=-Dlapack=none build-dir=build"
unpack pillow pillow-12.1.1.tar.gz
# pillow's own table builds its dependencies (done above, at the PC wheel's versions) and runs its test suite: no hooks here
wheel pillow CIBW_BEFORE_ALL='' CIBW_TEST_COMMAND='' CIBW_TEST_EXTRAS='' CIBW_TEST_REQUIRES='' \
  CIBW_CONFIG_SETTINGS="raqm=vendor fribidi=vendor platform-guessing=disable tiff=disable lcms=disable jpeg2000=disable imagequant=disable xcb=disable avif=disable" \
  CIBW_ENVIRONMENT="ZLIB_ROOT=$P JPEG_ROOT=$P FREETYPE_ROOT=$P HARFBUZZ_ROOT=$P WEBP_ROOT=$P CFLAGS=\"\$CFLAGS $PFX -I$P/include -I$P/include/freetype2 -I$P/include/harfbuzz\" LDFLAGS=\"\$LDFLAGS -L$P/lib\"" \
  CIBW_REPAIR_WHEEL_COMMAND_ANDROID=''

echo "$(src python-$PY_REL-aarch64-linux-android.tar.gz | awk '{print $3}')  $CACHE/cibw/python-$PY_REL-aarch64-linux-android.tar.gz" | sha256sum -c --quiet - \
  || die "cibuildwheel compiled against another CPython build than the pinned one"

# 7. The launcher: an executable named like a library, so it is packaged into the native library folder, the one place an app
#    may execute files from. Child steps run through it (sys.executable points at it).
J=$W/stage/jni/arm64-v8a; R=$W/stage/py; mkdir -p "$J" "$R/lib" "$R/site-packages"
"$CC" -O2 -fPIE -pie -D__BIONIC_NO_PAGE_SIZE_MACRO $PFX -I"$CPY/include/python$PY_VER" "$HERE/launcher.c" -L"$CPY/lib" -lpython$PY_VER \
  -Wl,-z,max-page-size=16384 -Wl,--no-undefined -Wl,--build-id=none -o "$J/libpylauncher.so" || die "launcher failed"
cp "$CPY/lib/libpython$PY_VER.so" "$J/"
for f in libfreetype.so libharfbuzz.so libwebp.so libwebpmux.so libwebpdemux.so libsharpyuv.so libfribidi.so; do
  real=$(readlink -f "$P/lib/$f"); cp "$real" "$J/$f"
done
# Android packages and finds native libraries only by their plain lib*.so names
for f in "$J"/*.so; do
  if "$READELF" -d "$f" | grep -E '\((NEEDED|SONAME)\)' | grep -qE '\.so\.[0-9]'; then die "$f names a versioned library"; fi
done

# 8. The Python tree: standard library without tests, GUI, installers, TLS, SQLite, bzip2, LZMA and Zstandard (the pack tools use none of them),
#    precompiled and without sources; the packages without their tests and headers
rsync -a --exclude __pycache__ --exclude test --exclude tests --exclude idlelib --exclude tkinter --exclude turtledemo \
  --exclude ensurepip --exclude lib2to3 --exclude pydoc_data --exclude sqlite3 --exclude '__phello__' --exclude 'config-*' \
  --exclude '_test*.so' --exclude 'xxlimited*.so' --exclude '_xxtestfuzz*.so' --exclude '_ctypes_test*.so' --exclude '_ssl.*.so' \
  --exclude '_hashlib.*.so' --exclude '_sqlite3.*.so' --exclude '_tkinter*.so' --exclude '_bz2.*.so' --exclude '_lzma.*.so' --exclude '_zstd.*.so' \
  --exclude 'site-packages' \
  "$CPY/lib/python$PY_VER" "$R/lib/"
cp "$B/zlib.$PY_TAG.so" "$R/lib/python$PY_VER/lib-dynload/"
# the official build's record of its configuration (sysconfig) names the folders of python.org's build machine
sed -i 's|/Users/runner|/build|g' "$R/lib/python$PY_VER"/_sysconfigdata__android_*.py "$R/lib/python$PY_VER"/_sysconfig_vars__android_*.json
for w in "$W"/wheels/*.whl; do (cd "$R/site-packages" && "$W/venv/bin/python" -m zipfile -e "$w" .); done
S=$R/site-packages
mkdir -p "$W/stage/licenses"
for d in "$S"/*.dist-info; do n=$(basename "$d" .dist-info); mkdir -p "$W/stage/licenses/$n"
  (cd "$d" && find . -type f \( -iname '*licen*' -o -iname 'copying*' \) -exec cp --parents {} "$W/stage/licenses/$n/" \;); done
rm -rf "$S"/*.dist-info "$S"/numpy/_core/include "$S"/numpy/_core/lib "$S"/numpy/f2py "$S"/numpy/_pyinstaller "$S"/numpy/testing/_private/extbuild.py \
  "$S"/capstone/include "$S"/capstone/lib/*.a "$S"/PIL/_imagingtk.*.so
find "$S" \( -name tests -o -name testing -o -name __pycache__ \) -type d -prune -exec rm -rf {} +
find "$S" \( -name '*.pyi' -o -name '*.a' -o -name '*.h' -o -name '*.pxd' -o -name 'py.typed' \) -delete
# numpy's record of its own build (numpy.show_config) names the compilers and interpreter by their folders here
sed -i -e "s|$W|/build|g" -e "s|$NDK|/ndk|g" -e "s|$CACHE|/cache|g" -e "s|$HOME|~|g" -E -e "s|cibw-run-[A-Za-z0-9_]+|cibw-run|g" "$S/numpy/__config__.py"
cp "$B/cpysrc/LICENSE" "$W/stage/licenses/cpython-LICENSE"
for d in zlib libpng brotli jpeg webp freetype harfbuzz fribidi; do
  mkdir -p "$W/stage/licenses/$d"
  find "$B/$d" -maxdepth 1 -type f \( -iname 'LICENSE*' -o -iname 'COPYING*' -o -iname 'LICENCE*' -o -iname 'FTL.TXT' -o -iname 'README' \) -exec cp {} "$W/stage/licenses/$d/" \;
done
[ -f "$B/freetype/docs/FTL.TXT" ] && cp "$B/freetype/docs/FTL.TXT" "$W/stage/licenses/freetype/"
find "$R" "$J" -name '*.so' -exec "$STRIP" --strip-unneeded {} \;
find "$R" "$J" -name '*.so' -exec "$W/venv/bin/python" "$HERE/elf_clean.py" {} +
# sourceless bytecode for the standard library and the packages, with the build interpreter of the same Python version
BT=$(ls "$CACHE"/cibw/cpython-$PY_REL+*-x86_64-unknown-linux-gnu-install_only.tar.gz 2> /dev/null | head -1)
[ -f "$BT" ] || die "no Python $PY_REL build interpreter in the cross-build cache"
mkdir -p "$W/bpy" && tar -xzf "$BT" -C "$W/bpy"; BPY=$W/bpy/python/bin/python$PY_VER
[ -x "$BPY" ] || die "the build interpreter has no $BPY"
# (file names inside the bytecode start at the runtime root, never at this machine's build folder)
"$BPY" -m compileall -q -b -j 0 --invalidation-mode unchecked-hash -s "$R" -p py "$R" > "$W/compileall.log" 2>&1 || die "precompiling failed, see $W/compileall.log"
find "$R" -name '*.py' -delete
leak=$( { LC_ALL=C grep -rlaF -e "$HOME" -e "$W" -e "$CACHE" "$R" "$J"; LC_ALL=C grep -rlaE '/Users/[A-Za-z0-9._-]+' "$R" "$J"; } | head -3 || true)
[ -z "$leak" ] || die "the runtime holds a path of a build machine: $leak"

# 9. Checks: every ELF file loads with 16 KB pages, and nothing outside this runtime is needed
for f in $(find "$J" "$R" -name '*.so'); do
  "$READELF" -lW "$f" | awk '$1 == "LOAD" && strtonum($NF) < 16384 { bad = 1 } END { exit bad }' || die "$f has LOAD segments aligned below 16 KB"
done

# 10. runtime.zip with fixed order, times and modes, so the same inputs give the same bytes
"$W/venv/bin/python" "$HERE/pack_zip.py" "$R" "$W/stage/runtime.zip"
rm -rf "$R"
echo "$RECIPE" > "$W/stage/RECIPE"
rm -rf "$OUT.part" && mv "$W/stage" "$OUT.part" && rm -rf "$OUT" && mv "$OUT.part" "$OUT"
rm -rf "$W"
say "runtime $RECIPE ready in $OUT: $(du -sh "$OUT/jni" | cut -f1) native libraries, $(du -h "$OUT/runtime.zip" | cut -f1) runtime.zip"
