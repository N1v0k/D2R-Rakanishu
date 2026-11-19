import os, json
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parents[1]))
import lib





prefix = Path(os.environ["DM_TOUCH_PREFIX"])
last = max(json.loads(l)["tick"] for l in open(prefix / "c2s.jsonl") if l.strip())
r = lib.load(sys.argv[1])
lib.check(any(op in (0x05, 0x06) for _, op, _ in r.c2s_ops(t0=last + 1)), f"前缀末拍 {last} 之后没有主技能施放包 0x05 / 0x06")
sys.exit(lib.main())
