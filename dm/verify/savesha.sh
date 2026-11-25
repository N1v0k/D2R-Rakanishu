#!/bin/bash






set -eu
D=${1%/}
( cd "$D" && find . -type f ! -name .lock | sed 's|^\./||' | LC_ALL=C sort | while read -r f; do printf '%s  0000000000000001/%s\n' "$(shasum -a 256 "$f" | cut -c1-64)" "$f"; done ) | shasum -a 256 | cut -c1-64
