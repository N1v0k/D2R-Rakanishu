#!/bin/bash








set -u
OUT=${1:?用法：charsel_retap.sh <结果目录>}; TOOLS=$(cd "$(dirname "$0")" && pwd); mkdir -p "$OUT"; WANT=(76 1 9 88); YS=(47 92 128 165); bad=0
for r in 0 1 2 3; do y=${YS[$r]}
  printf '4.0 down 0 803,%s\n4.1 up 0\n5.8 down 0 803,%s\n5.9 up 0\n7.6 down 0 803,%s\n7.7 up 0\n10.0 down 0 478,395\n10.1 up 0\n12.0 down 0 478,163\n12.1 up 0\n' $y $y $y > "$OUT/t$r.txt"
  E2E_REF=956x440 E2E_KEYS='B@OpenPanel:NoSuchPanelEver' E2E_ENV='{"DM_TOUCH_SCRIPT_START":"menu","DM_FE_AUTO":"1"}' bash "$TOOLS/touch_e2e_mac.sh" "$OUT/t$r.txt" "$OUT/r$r" 26 > "$OUT/r$r.out" 2>&1
  sel=$(LC_ALL=C grep -a -c 'UIMSG.*CharacterSelect SelectCharacter' "$OUT/r$r/boot.txt" 2>/dev/null); lv=$(LC_ALL=C grep -a 'dm_hud' "$OUT/r$r/boot.txt" 2>/dev/null | LC_ALL=C grep -a -o 'Lv [0-9]*' | awk '$2>0{print $2; exit}')
  if [ "${sel:-1}" = 0 ] && [ "${lv:-}" = "${WANT[$r]}" ]; then echo "第 $((r+1)) 行 OK：进局等级 ${lv}，SelectCharacter 0 条"; else echo "【红】第 $((r+1)) 行：进局等级「${lv:-没进局}」（应为 ${WANT[$r]}），SelectCharacter ${sel:-?} 条"; bad=1; fi
done
[ $bad = 0 ] && echo "CHARSEL-RETAP OK 4/4" || echo "CHARSEL-RETAP RED"; exit $bad
