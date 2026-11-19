















_sk_pub=_public
_sk_put_back() {
  local C=$1 own; own=$(cat "$C/save.test.owner" 2>/dev/null) || return 0; [ -n "$own" ] || { rm -f "$C/save.test.owner"; return 0; }
  rm -rf "$C/Documents/save.test.work" "$C/save.keep.$own"
  [ -d "$C/Documents/save.test" ] && mv "$C/Documents/save.test" "$C/save.keep.$own"
  [ -d "$C/save.keep.$_sk_pub" ] && mv "$C/save.keep.$_sk_pub" "$C/Documents/save.test"
  rm -f "$C/save.test.owner"
}
savekey_in() {
  local C=$1 env=" ${2:-} " key=${DM_SAVE_KEY:-}
  [ -f "$C/save.test.owner" ] && { echo "存档钥匙：上一局（钥匙「$(cat "$C/save.test.owner")」）没换回，先归位" >&2; _sk_put_back "$C"; }
  if [ -z "$key" ]; then
    case "$env" in *" DM_SAVE_TEST=1 "*) case "$env" in *" DM_SAVE_FRESH="*) ;; *)
      echo "【红】这一局带 DM_SAVE_TEST=1 接着用上一局的测试存档，却没带存档钥匙：本机容器是各线共用的，两步之间会被别的局换档。给这几步都带上同一个 DM_SAVE_KEY=<你这条线自己取的名字>；这一局不起" >&2; return 1;; esac;; esac
    return 0
  fi
  case "$key" in *[!A-Za-z0-9_.-]*|"$_sk_pub"|"") echo "【红】DM_SAVE_KEY 只能用字母、数字、_ . -，且不能叫 ${_sk_pub}；这一局不起" >&2; return 1;; esac
  rm -rf "$C/Documents/save.test.work" "$C/save.keep.$_sk_pub"
  [ -d "$C/Documents/save.test" ] && mv "$C/Documents/save.test" "$C/save.keep.$_sk_pub"
  echo "$key" > "$C/save.test.owner"
  if [ -d "$C/save.keep.$key" ]; then mv "$C/save.keep.$key" "$C/Documents/save.test"
  elif [ -d "$C/save.keep.$_sk_pub" ]; then cp -R "$C/save.keep.$_sk_pub" "$C/Documents/save.test"
  fi
  return 0
}
savekey_out() { [ -f "$1/save.test.owner" ] && _sk_put_back "$1"; return 0; }
