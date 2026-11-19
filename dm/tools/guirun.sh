#!/bin/bash














set -u
O=/dev/null; E=/dev/null; P=; ENV=()
while [ $# -gt 0 ]; do case $1 in -o) O=$2; shift 2;; -e) E=$2; shift 2;; -p) P=$2; shift 2;; --) shift; break;; *=*) ENV+=(--env "$1"); shift;; *) break;; esac; done
[ $# -ge 1 ] || { echo "用法：guirun.sh [-o f] [-e f] [-p pidf] [K=V…] -- <程序> [参数…]" >&2; exit 2; }
T=$(mktemp -d "${TMPDIR:-/tmp}/guirun.XXXXXX") || exit 2; A=$T/GuiRun.app; trap 'rm -rf "$T"' EXIT; mkdir -p "$A/Contents/MacOS"
PF=${P:-$T/pid}
{ printf '#!/bin/bash\necho $$ > %q\ncd %q\n' "$PF" "$PWD"



  printf 'n=$(sysctl -n kern.maxfilesperproc); ulimit -n "$n" || { echo "guirun：ulimit -n $n 失败（软上限仍是 $(ulimit -Sn)），不起目标" >&2; exit 97; }\n'
  printf '%q ' exec "$@"; echo; } > "$A/Contents/MacOS/run"; chmod +x "$A/Contents/MacOS/run"
cat > "$A/Contents/Info.plist" <<PL
<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict><key>CFBundleExecutable</key><string>run</string><key>CFBundleIdentifier</key><string>local.guirun.$$</string><key>CFBundlePackageType</key><string>APPL</string></dict></plist>
PL
open -n -W --stdout "$O" --stderr "$E" ${ENV[@]+"${ENV[@]}"} "$A" || exit 125
