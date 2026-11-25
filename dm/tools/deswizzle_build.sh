#!/bin/bash








set -euo pipefail
O=${1:?输出目录 | output dir}; H=$(cd "$(dirname "$0")/.." && pwd)
SRC=("$H/gfx/tex/nvn_deswizzle.c" "$H/gfx/nvn/nvn_tex.c"); INC=(-I"$H/gfx/nvn" -I"$H/gfx/tex")
LLVM=${DM_LLVM:-$(ls -d /opt/homebrew/opt/llvm@*/bin 2> /dev/null | while read -r d; do [ -x "$d/lld-link" ] && echo "$d"; done | head -1 || true)}
[ -n "$LLVM" ] && [ -x "$LLVM/lld-link" ] || { echo "[x] 找不到带 lld-link 的 LLVM（brew install llvm，或 DM_LLVM=<bin 目录>）| no LLVM with lld-link found" >&2; exit 1; }
CC=$LLVM/clang; [ -x "$CC" ] || CC=clang
rm -rf "$O" && mkdir -p "$O/macos-arm64" "$O/win-x64"
clang -O2 -shared -fPIC -arch arm64 "-ffile-prefix-map=$H=/_/" -Wl,-install_name,@rpath/deswizzle.dylib "${INC[@]}" -o "$O/macos-arm64/deswizzle.dylib" "${SRC[@]}"
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT; mkdir "$W/inc"
printf '#pragma once\n#include <stddef.h>\nvoid *memcpy(void *d, const void *s, size_t n);\nvoid *memset(void *d, int c, size_t n);\n' > "$W/inc/string.h"
printf '#include <stddef.h>\nvoid *memcpy(void *d, const void *s, size_t n) { unsigned char *a = d; const unsigned char *b = s; while (n--) *a++ = *b++; return d; }\nvoid *memset(void *d, int c, size_t n) { unsigned char *a = d; while (n--) *a++ = (unsigned char)c; return d; }\n' > "$W/crt.c"
for f in "${SRC[@]}" "$W/crt.c"; do "$CC" --target=x86_64-pc-windows-msvc -O2 "-ffile-prefix-map=$H=/_/" "-ffile-prefix-map=$W=/_/" -ffreestanding -fno-builtin -isystem "$W/inc" "${INC[@]}" -c "$f" -o "$W/$(basename "$f" .c).obj"; done
"$LLVM/lld-link" /DLL /NOENTRY /NODEFAULTLIB /MACHINE:X64 /EXPORT:nvn_tex_layout /EXPORT:nvn_deswizzle /EXPORT:nvn_swizzle /OUT:"$O/win-x64/deswizzle.dll" "$W"/*.obj > /dev/null
rm -f "$O/win-x64/deswizzle.lib" "$O/win-x64/deswizzle.exp"
cat "${SRC[@]}" "$H/gfx/tex/nvn_deswizzle.h" "$H/gfx/nvn/nvn_tex.h" | shasum -a 256 | cut -d' ' -f1 > "$O/source.sha256"
echo "贴图解排布库：$O（macos-arm64 $(stat -f %z "$O/macos-arm64/deswizzle.dylib") 字节，win-x64 $(stat -f %z "$O/win-x64/deswizzle.dll") 字节，源码 $(cut -c1-12 "$O/source.sha256")）"
