#!/bin/bash
set -u
export LC_ALL=C
ROOT=$(cd "$(dirname "$0")/.." && pwd)
CASE=${1:?用法：touchchain.sh <用例名>}
CASE_DIR="$ROOT/verify/touchcases/$CASE"
[ -f "$CASE_DIR/script.txt" ] && [ -f "$CASE_DIR/assert.py" ] || { echo "【红】未知或不完整用例：$CASE" >&2; exit 2; }
BIN="$ROOT/host/dmhost"
OUT=${DM_HOST_OUT:-"$(bash "$(dirname "$0")/../tools/product.sh" path touch)"}
REF=${DM_TOUCH_SAVE:-"${DM_ROOT:-$HOME/dm-work}/data/switch/save_ref_warlock/0000000000000001/0"}




PREFIX=${DM_TOUCH_PREFIX:-$( [ -f "$CASE_DIR/prefix" ] && head -1 "$CASE_DIR/prefix" )}
case "$PREFIX" in ""|/*) ;; *) PREFIX="${DM_ROOT:-$HOME/dm-work}/$PREFIX";; esac
STAMP=$(date +%s)
RUNROOT=${DM_TOUCH_RUNROOT:-${DM_ROOT:-$HOME/dm-work}/run/verify/dm/touchchain}
RUN="$RUNROOT/${CASE}_${STAMP}_$$"
START=$SECONDS
status=2
mkdir -p "$RUN/user" || exit 2
if [ -n "$PREFIX" ]; then
    [ -f "$PREFIX/manifest.json" ] && [ -f "$PREFIX/c2s.jsonl" ] || { echo "【红】前缀不完整：$PREFIX" >&2; exit 2; }
    sha=$(sed -n 's/.*"start_save_sha": "\([0-9a-f]*\)".*/\1/p' "$PREFIX/manifest.json")
    root=""
    for m in "${DM_ROOT:-$HOME/dm-work}"/data/switch/*/MANIFEST.sha256; do [ "$(shasum -a 256 "$m" | cut -c1-64)" = "$sha" ] && { root=$(dirname "$m"); break; }; done
    [ -n "$root" ] || { echo "【红】按指纹找不到前缀存档：$sha" >&2; exit 2; }
    REF="$root/0000000000000001/0"
    export DM_C2S_REPLAY="$PREFIX/c2s.jsonl" DM_TOUCH_PREFIX="$PREFIX"


    export DM_KEYS='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,C0.84:0.208@OpenPanel:CharacterSelectPanel%1>UpdateCharacterInfo,MSG:CharacterSelect:LoadCharacter:0@UpdateCharacterInfo%0.3>FrontEndNavigation:ToGame'
fi
cp -R "$REF" "$RUN/user/0" || { echo "【红】术士存档副本失败：$REF" >&2; exit 2; }
chmod -R u+w "$RUN/user/0" || exit 2
export DM_SAVE="$RUN/user/0" DM_GAME_SEED=305419896 DM_FIXED_TIME=1790895128
export DM_TOUCH_SCRIPT="$CASE_DIR/script.txt"




if [ -z "${DM_ROMFS_OVERLAY:-}" ]; then
    python3 "$ROOT/ios/tools/gen_romfs_overlay.py" "${DM_ROMFS_SRC:-${DM_ROOT:-$HOME/dm-work}/data/switch/3.3.94012/Update/RomFS}" "$RUN/romfs_overlay" > /dev/null || { echo "【红】界面覆盖层生成失败（gen_romfs_overlay.py）" >&2; exit 2; }
    export DM_ROMFS_OVERLAY="$RUN/romfs_overlay"
fi


export DM_KEYS=${DM_KEYS:-'X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,MSG:CharacterSelect:LoadCharacter:0@OpenPanel:CharacterSelectPanel%1>FrontEndNavigation:ToGame'}
export DM_TICKS="$RUN/ticks.jsonl" DM_TICKS_FULL=1 DM_TICKS_N=${DM_TICKS_N:-${PREFIX:+720}}
[ -f "$CASE_DIR/ticks" ] && export DM_TICKS_N=${DM_TICKS_N:-$(head -1 "$CASE_DIR/ticks")}
export DM_TICKS_N=${DM_TICKS_N:-900}
export DM_C2S_LOG="$RUN/c2s.jsonl" DM_UIMSG="$RUN/uimsg.jsonl" DM_STATE="$RUN/state.jsonl" DM_WATCH_STALL=15 DM_WATCH=90








export DM_PRESENT_HZ=0 DM_DETCLOCK=33333333
[ -x "$BIN" ] || timeout 75s make -C "$ROOT/host" dmhost >/dev/null || { echo "【红】宿主编译失败" >&2; exit 2; }
set +e
timeout 100s "$BIN" "$OUT" main sdk > "$RUN/host.log" 2>&1
rc=$?; [ "$rc" -eq 0 ] || { grep '【红】看门狗' "$RUN/host.log" | tail -1 >&2 || true; exit "$rc"; }
host_status=$?
timeout 5s python3 "$CASE_DIR/assert.py" "$RUN"
assert_status=$?


[ -s "$RUN/state.jsonl" ] || { echo "断言失败：局目录缺 state.jsonl（宿主未写 DM_STATE）" >&2; assert_status=1; }
set -e
if [ "$host_status" -eq 0 ] && [ "$assert_status" -eq 0 ]; then
    status=0
    [ "${DM_TOUCH_KEEP:-0}" = 1 ] || rm -rf "$RUN"
    echo "TOUCHCHAIN $CASE OK 用时 $((SECONDS - START)) 秒"
    exit 0
fi
echo "【红】touchchain ${CASE}：host=$host_status assert=${assert_status}；局目录：$RUN" >&2
echo "TOUCHCHAIN $CASE RED 用时 $((SECONDS - START)) 秒"
exit 1
