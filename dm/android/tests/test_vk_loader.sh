#!/bin/bash



set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd); and=$(cd "$here/.." && pwd)
ndk=${1:-${ANDROID_NDK_HOME:-${ANDROID_HOME:-$HOME/Library/Android/sdk}/ndk/29.0.14206865}}
inc=$(ls -d "$ndk"/toolchains/llvm/prebuilt/*/sysroot/usr/include | head -1)
[ -f "$inc/vulkan/vulkan_core.h" ] || { echo "test_vk_loader: no NDK Vulkan headers under $ndk"; exit 2; }
o=$(mktemp -d); trap 'rm -rf "$o"' EXIT
mkdir -p "$o/inc"; ln -s "$inc/vulkan" "$o/inc/vulkan"; [ -d "$inc/vk_video" ] && ln -s "$inc/vk_video" "$o/inc/vk_video"
python3 "$and/tools/gen_vk_shim.py" "$inc/vulkan" "$o/vk_shim.c"
CC=${CC:-cc}; F="-O1 -Wall -isystem $o/inc -I$and"
for id in 1 2; do $CC $F -fPIC -shared -DFAKE_ID=$id -o "$o/fake$id.so" "$here/test_vk_loader_fake.c"; done
$CC $F -o "$o/t" "$here/test_vk_loader.c" "$and/vk_loader.c" "$o/vk_shim.c" -ldl -lpthread
"$o/t" "$o/fake1.so" "$o/fake2.so"
