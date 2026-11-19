import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1]))
import lib

r = lib.load(sys.argv[1])
rows = [(row.get("tick"), row.get("act"), row.get("arg")) for row in r._uimsg if row.get("cat") == "PanelManager"]
note_open = [t for t, act, arg in rows if act == "OpenPanel" and arg == "NotificationPanel"]
note_close = [t for t, act, arg in rows if act == "ClosePanel" and arg == "NotificationPanel"]
inv_open = [t for t, act, arg in rows if act == "OpenPanel" and arg == "PlayerInventoryExpansionLayout"]
lib.check(len(note_open) == 1, f"改难度后系统通知应弹一次，实际 {note_open}")
lib.check(len(inv_open) == 1, f"通知开着时点功能键应打开功能页一次，实际 {inv_open}")
if note_open and inv_open:
    lib.check(note_open[0] < inv_open[0], f"功能页应在通知弹出之后打开（通知拍 {note_open[0]}，功能页拍 {inv_open[0]}）")
    lib.check(not note_close or note_close[0] > inv_open[0], f"通知不应被这次按键关掉（通知关于拍 {note_close[:1]}，功能页开于拍 {inv_open[0]}）")


    lib.check(bool(note_close) and 120 <= note_close[0] - note_open[0] <= 175, f"通知应约 5 秒（实测 146 拍）后自己关，实际开 {note_open} 关 {note_close}")
sys.exit(lib.main())
