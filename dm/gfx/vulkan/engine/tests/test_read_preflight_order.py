
import pathlib
import re
import unittest

SOURCE = pathlib.Path(__file__).resolve().parents[1] / "xbg_game.cpp"
ENTRIES = (
    "draw", "dispatch", "clear_attachment", "copy_texture_buffer",
    "copy_texture", "clear_texture", "resolve_texture", "execute_indirect",
    "xbg_present_texture_now", "xbg_legacy_read_texels", "gpu_buffer_copy",
)


def entry_prefix(source, name):
    match = re.search(r"(?:static )?(?:void|bool) " + name + r"\([^\n]*?\)\{", source)
    if not match:
        raise AssertionError("missing production entry: " + name)
    return match.end(), source[match.end():]




PREFLIGHT = "void preflight_cpu_reads(){if(read_preflight)resolve_pending();}"
DEFAULT_ON = "bool read_preflight=true;"
INVARIANT = 'need(!read_preflight || (pending_writebacks.empty() && pending_reports.empty()),"Unresolved CPU reads at descriptor entry");'


def check(source):

    if PREFLIGHT not in source or DEFAULT_ON not in source:
        raise AssertionError("preflight must resolve actual pending results")
    for name in ENTRIES:
        _, tail = entry_prefix(source, name)
        if name in ("xbg_present_texture_now", "xbg_legacy_read_texels"):
            lock = ("gfx_drain();" if name == "xbg_legacy_read_texels" else "") + "std::lock_guard<std::recursive_mutex> lock(mutex);"
            if not tail.startswith(lock):
                raise AssertionError("present preflight requires engine lock")
            tail = tail[len(lock):]
        if not tail.startswith("preflight_cpu_reads();"):
            raise AssertionError(name + ": preflight must precede all acquired references")
    _, tail = entry_prefix(source, "descriptors")
    if not tail.lstrip().startswith(INVARIANT):
        raise AssertionError("descriptor entry must reject unresolved results")
    if "s.recording=false;s.pending=true;" not in source or "release_descriptors(s);" not in source:
        raise AssertionError("retirement contract changed: re-audit required")


class ReadPreflightOrder(unittest.TestCase):
    def test_production(self):
        check(SOURCE.read_text(encoding="utf-8"))

    def test_missing_and_late_negative_controls(self):
        source = SOURCE.read_text(encoding="utf-8")
        for name in ENTRIES:
            offset, tail = entry_prefix(source, name)
            at = offset + tail.index("preflight_cpu_reads();")
            end = at + len("preflight_cpu_reads();")
            for replacement in ("", "Commands stale;preflight_cpu_reads();"):
                with self.subTest(entry=name, mutation=replacement):
                    with self.assertRaises(AssertionError):
                        check(source[:at] + replacement + source[end:])

    def test_noop_and_missing_invariant_negative_controls(self):
        source = SOURCE.read_text(encoding="utf-8")
        for mutated in (
            source.replace(PREFLIGHT, "void preflight_cpu_reads(){}"),
            source.replace(DEFAULT_ON, "bool read_preflight=false;"),
            source.replace(INVARIANT, ""),
        ):
            with self.assertRaises(AssertionError):
                check(mutated)


if __name__ == "__main__":
    unittest.main()
