#!/bin/bash






set -u
HERE=$(cd "$(dirname "$0")" && pwd)
TR=${1:?war_summon13真值目录}; OUT=${2:-$(bash "$(dirname "$0")/product.sh" path keypage)}
PY=${DM_ROOT:-$HOME/dm-work}/build/cache/dm/venv/bin/python
export LC_ALL=C HSC_WALL=${HSC_WALL:-180}


FT=$($PY -c 'import json,sys; m=json.load(open(sys.argv[1])); t=m.get("fix_time"); print(int(t if t is not None else m["wall_start"]))' "$TR/meta.json") || exit 2
export FT
export KEYS_FRONT='X@OpenPanel:MainMenuButtonRibbon>OpenPanel:CharacterSelectPanel,DOWN@OpenPanel:CharacterSelectPanel>UpdateCharacterInfo,A@UpdateCharacterInfo%1>CharacterSelect:SelectCharacter,A@CharacterSelect:SelectCharacter>FrontEndNavigation:ToGame,PLUS@DM:Tick:2558#>OpenPanel:SkillsTreePanel,ZR@OpenPanel:SkillsTreePanel%0.5,ZR@%0.3,RIGHT@%0.3,UP@%0.3,UP@%0.3,Y@DM:Tick:2683#>OpenPanel:ButtonBindingModal,Y@OpenPanel:ButtonBindingModal%0.5>ClosePanel:ButtonBindingModal,B@ClosePanel:ButtonBindingModal%0.5>ClosePanel:SkillsTreePanel'
unset KEYS_EXIT


exec bash "$HERE/host_savecmp.sh" "$TR" "$OUT"
