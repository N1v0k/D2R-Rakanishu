#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1]))
import lib
r = lib.load(sys.argv[1])
rows = [(t, op) for t, op, _ in r.c2s_ops()]
ops = [op for _, op in rows]




TARGET = {0x02, 0x04, 0x13, 0x16, 0x2f, 0x40, 0x41}
stray = [(t, op) for t, op in rows if op in (0x01, 0x03) and not any(o in TARGET and abs(t2 - t) <= 40 for t2, o in rows)]
lib.check(not stray, "没有无目标的点地移动包（0x01 / 0x03 都伴随带单位的交互包）：%s" % stray[:3])
lib.check(0x42 not in ops, "没有摇杆移动包（0x42）")
ev = r._panel_events
opened = [t for t, a, p in ev if a == "OpenPanel" and p == "PlayerInventoryExpansionLayout"]
closed = [t for t, a, p in ev if a == "ClosePanel" and p == "PlayerInventoryExpansionLayout" and opened and t > opened[0]]
lib.check(bool(opened), "功能键开物品栏")
lib.check(bool(closed), "点面板外后物品栏关闭")
sys.exit(lib.main())
