





_le_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
for _le_f in "${DM_LOCAL_ENV:-}" "$_le_dir/local.env" "$HOME/.config/dm-ios/local.env"; do
  [ -n "$_le_f" ] && [ -f "$_le_f" ] && { set -a; source "$_le_f"; set +a; DM_LOCAL_ENV_FILE=$_le_f; break; }
done
local_env_need() {
  local v miss=
  for v in "$@"; do [ -n "${!v:-}" ] || miss="$miss $v"; done
  [ -z "$miss" ] || { echo "🔴 缺本机配置：${miss}（把 dm/ios/local.env.example 复制成 dm/ios/local.env 填好；本次读的是：${DM_LOCAL_ENV_FILE:-没找到配置文件}）" >&2; return 1; }
}
