











LG_ARGS=(-ApplePersistenceIgnoreState YES)
launch_guard_pre() { local d="$1/Library/Saved Application State"; [ -d "$d" ] && find "$d" -maxdepth 1 -name '*.savedState' -exec rm -rf {} + 2>/dev/null; return 0; }
launch_guard_blocked() {
  local st; st=$(lsappinfo info -only pid,applicationState,isHidden -app "$1" 2>/dev/null | tr '\n' ' ')
  echo "【红】起包被挡：$2，启动日志没出现。系统登记的应用状态：${st:-查不到（进程没起来）}。多半是桌面上有系统弹框压着（恢复窗口 / 权限 / 损坏提示），到这台机器的桌面看一眼；不是被测代码的问题" >&2
  return 1
}
