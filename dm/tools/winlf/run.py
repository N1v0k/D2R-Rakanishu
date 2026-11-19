#!/usr/bin/env python3






from pathlib import Path
import runpy
import sys

if len(sys.argv) < 2:
    raise SystemExit("usage: run.py generator.py [arguments]")


runpy.run_path(str(Path(__file__).with_name('sitecustomize.py')))
script = str(Path(sys.argv[1]).resolve())
sys.argv = [script] + sys.argv[2:]
sys.path.insert(0, str(Path(script).parent))
runpy.run_path(script, run_name='__main__')
