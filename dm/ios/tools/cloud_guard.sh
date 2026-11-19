



















CG_ENV=; CG_REAL=; CG_LOCAL=
cloud_guard_env() {
  CG_ENV=; CG_REAL=; CG_LOCAL=
  if [ "${DM_REAL_ICLOUD:-}" = 1 ]; then
    [ "${DM_REAL_ICLOUD_CONFIRM:-}" = "写入真实云端" ] || { echo "【红】DM_REAL_ICLOUD=1 还要同时给 DM_REAL_ICLOUD_CONFIRM=写入真实云端 才放开；这一局不起" >&2; return 1; }
    echo "【红】本局连的是真实 iCloud（DM_REAL_ICLOUD=1）：Documents/save 里的改动会同步到这个账号的所有设备" >&2; CG_REAL=1; return 0
  fi
  case " ${2:-} " in
    *" DM_TEST_ICLOUD_SUB="*) echo "【红】DM_TEST_ICLOUD_SUB 走的是真实 iCloud 容器，没有显式放开（DM_REAL_ICLOUD）不许带；这一局不起" >&2; return 1;;
    *" DM_TEST_ICLOUD_DIR="*|*" DM_TEST_ICLOUD_ROOT="*) CG_LOCAL=1; return 0;;
  esac
  CG_ENV="DM_TEST_ICLOUD_OFF=1"
}
cloud_guard_check() {
  [ "$CG_REAL" = 1 ] || [ "$CG_LOCAL" = 1 ] && return 0
  local bad; bad=$(LC_ALL=C grep -a -E 'iCloud 同步：(启动 |.* 上传（|.* 下载（|.*记删除标记)' "$1" 2>/dev/null | head -1)
  [ -z "$bad" ] && return 0
  echo "【红】这一局本该不同步，却出现了 iCloud 同步动作：$(echo "$bad" | cut -c1-160)。可能已写进真实云端，立刻停下报告，别再起局" >&2; return 1
}
cloud_guard_clean() { return 0; }
