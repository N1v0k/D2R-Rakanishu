







ASC_ENV=${DM_ASC_ENV:-$HOME/.appstoreconnect/dm.env}
asc_auth() {
  ASC_AUTH=()
  [ -f "$ASC_ENV" ] || return 0
  source "$ASC_ENV"
  ASC_KEY=$HOME/.appstoreconnect/private_keys/AuthKey_${ASC_KEY_ID}.p8
  [ -f "$ASC_KEY" ] || { echo "🔴 缺 API 密钥文件 $ASC_KEY"; return 1; }
  ASC_AUTH=(-authenticationKeyPath "$ASC_KEY" -authenticationKeyID "$ASC_KEY_ID" -authenticationKeyIssuerID "$ASC_ISSUER")
}
