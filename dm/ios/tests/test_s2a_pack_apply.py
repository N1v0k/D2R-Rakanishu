#!/usr/bin/env python3




import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import unittest

SRC = Path(__file__).resolve().parents[1] / "Sources" / "s2a_pack_apply.c"

PROGRESS = re.compile(r"进度：解开 \S+（(\d+)/(\d+)）")
FAILED = re.compile(r"解包失败：(.+)")
DRIVER = r'''
#include <stdio.h>
int dm_s2a_pack_apply(const char *docs);
int main(int argc, char **argv) { return dm_s2a_pack_apply(argv[1]) ? 1 : 0; }
'''


def done_line(sha):
    return re.compile(r"解开 \d+ 个包 .*\.applied = " + re.escape(sha))


class PackApplyTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        d = Path(cls.tmp.name)
        (d / "driver.c").write_text(DRIVER)
        cls.exe = d / "apply"
        subprocess.run(["clang", "-O1", "-Wall", "-Werror", "-o", str(cls.exe), str(SRC), str(d / "driver.c")], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def setUp(self):
        self.d = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.d)
        self.docs = self.d / "Documents"
        self.root = self.docs / "资源包"
        (self.root / ".incoming").mkdir(parents=True)

    def pack(self, files, batches):

        for i, rels in enumerate(batches):
            with tarfile.open(self.root / ".incoming" / f"20261112_{i:03d}.tar", "w", format=tarfile.PAX_FORMAT) as t:
                for rel in rels:
                    ti = tarfile.TarInfo(rel); ti.size = len(files[rel]); t.addfile(ti, io.BytesIO(files[rel]))
        index = "".join(f"{rel}\t{len(b)}\t{hashlib.sha256(b).hexdigest()}\n" for rel, b in files.items())
        (self.root / "index.tsv").write_text(index, encoding="utf-8")
        sha = hashlib.sha256(index.encode()).hexdigest()
        (self.root / "manifest.json").write_text(json.dumps({"version": 2, "index_sha256": sha}), encoding="utf-8")
        return sha

    def run_apply(self):
        r = subprocess.run([str(self.exe), str(self.docs)])
        log = (self.docs / "unpack.log").read_text(encoding="utf-8").splitlines()
        return r.returncode, log

    def test_unpacks_checks_and_marks_applied(self):
        long = "data/" + "x" * 120 + ".data"
        files = {"data/00-00000001.data": b"a" * 70000, "data/17-00000002.data": b"bc" * 513, long: b"long"}
        sha = self.pack(files, [["data/00-00000001.data"], ["data/17-00000002.data", long]])
        rc, log = self.run_apply()
        self.assertEqual(rc, 0, log)
        self.assertEqual([(m.group(1), m.group(2)) for m in map(PROGRESS.search, log) if m], [("1", "2"), ("2", "2")])
        self.assertTrue(any(done_line(sha).search(l) for l in log), log)
        self.assertEqual((self.root / ".applied").read_text(), sha)
        self.assertEqual((self.root / "index.applied.tsv").read_text(), (self.root / "index.tsv").read_text())
        for rel, b in files.items():
            self.assertEqual((self.root / rel).read_bytes(), b)
        self.assertEqual(list((self.root / ".incoming").iterdir()), [])

    def test_truncated_tar_fails_and_is_dropped_with_later_ones(self):
        files = {"data/a.data": b"a" * 5000, "data/b.data": b"b" * 10}
        self.pack(files, [["data/a.data"], ["data/b.data"]])
        first = self.root / ".incoming" / "20261112_000.tar"
        first.write_bytes(first.read_bytes()[:1536])
        (self.root / ".pushed").write_text("data/a.data\tx\n")
        rc, log = self.run_apply()
        self.assertEqual(rc, 1)
        self.assertTrue(any(FAILED.search(l) for l in log), log)
        self.assertEqual(list((self.root / ".incoming").iterdir()), [])
        self.assertFalse((self.root / ".pushed").exists())
        self.assertFalse((self.root / ".applied").exists())

    def test_unsafe_path_is_refused(self):
        files = {"../escape.data": b"x"}
        self.pack(files, [["../escape.data"]])
        rc, log = self.run_apply()
        self.assertEqual(rc, 1)
        self.assertTrue(any(FAILED.search(l) and "不安全" in l for l in log), log)
        self.assertFalse((self.docs / "escape.data").exists())

    def test_size_mismatch_after_unpack_fails(self):
        files = {"data/a.data": b"a" * 10}
        self.pack(files, [["data/a.data"]])
        idx = self.root / "index.tsv"
        idx.write_text(idx.read_text().replace("\t10\t", "\t11\t"))
        rc, log = self.run_apply()
        self.assertEqual(rc, 1)
        self.assertTrue(any(FAILED.search(l) and "大小不对" in l for l in log), log)
        self.assertFalse((self.root / ".applied").exists())


if __name__ == "__main__":
    unittest.main()
