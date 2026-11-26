










import hashlib
import os
import sys

from unicorn import Uc, UcError, UC_ARCH_ARM64, UC_MODE_ARM, UC_PROT_NONE, UC_PROT_READ, UC_PROT_WRITE, UC_PROT_EXEC, UC_HOOK_CODE
from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X2, UC_ARM64_REG_X3, UC_ARM64_REG_SP, UC_ARM64_REG_LR, UC_ARM64_REG_PC

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'verify'))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools'))
import nsoimg
from gen_mon_pics import MAIN_SHA256
from game_addrs import GA

ENTRY = GA.GRANNY_DECOMPRESS
PAGE = 0x1000
CODE = (ENTRY & -PAGE, (ENTRY & -PAGE) + 3 * PAGE)
TABLE = (GA.GRANNY_QUAT_RANGES & -PAGE, (GA.GRANNY_QUAT_RANGES & -PAGE) + 2 * PAGE)
IMPORTS = (GA.IMPORT_MEMCPY & -PAGE, (GA.IMPORT_MEMCPY & -PAGE) + PAGE)
MEMCPY, MEMMOVE = GA.IMPORT_MEMCPY, GA.IMPORT_MEMMOVE
STACK = (0x40000000, 0x40010000)
SRC, DST, RET = 0x50000000, 0x60000000, 0x70000000
PAGE = 0x1000
GUARD = 0xA5
INSN_BASE, INSN_PER_BYTE = 2_000_000, 400
MAX_BYTES = 64 << 20


class DecodeError(Exception):
    """A section that the game's decompressor does not expand to exactly its stated size, or an executable that is not the verified one."""


def _page_up(n):
    return (n + PAGE - 1) & ~(PAGE - 1)


def fingerprint(main_path):
    with open(main_path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


class GameDecoder:
    def __init__(self, main_path):
        digest = fingerprint(main_path)
        if digest != MAIN_SHA256:
            raise DecodeError('the game executable %s has SHA-256 %s, not the verified 3.4.95035 build' % (main_path, digest))
        img = nsoimg.load(main_path)
        u = self._u = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
        for (lo, hi), prot in ((CODE, UC_PROT_READ | UC_PROT_EXEC), (TABLE, UC_PROT_READ), (IMPORTS, UC_PROT_READ | UC_PROT_EXEC)):
            u.mem_map(lo, hi - lo, prot)
            u.mem_write(lo, img[lo:hi])
        u.mem_map(STACK[0], STACK[1] - STACK[0], UC_PROT_READ | UC_PROT_WRITE)
        u.mem_map(RET, PAGE, UC_PROT_READ | UC_PROT_EXEC)
        u.hook_add(UC_HOOK_CODE, self._import, begin=IMPORTS[0], end=IMPORTS[1])
        self._src_cap = self._dst_cap = 0
        self._src_end = self._dst_end = 0
        self._fault = None

    def _writable(self, a, n):
        return (STACK[0] <= a and a + n <= STACK[1]) or (DST <= a and a + n <= DST + self._dst_end)

    def _readable(self, a, n):
        return self._writable(a, n) or (SRC <= a and a + n <= SRC + self._src_end)

    def _import(self, u, addr, size, _):
        if addr not in (MEMCPY, MEMMOVE):
            self._fault = 'called the import stub at %#x' % addr
            u.emu_stop()
            return
        d, s, n = (u.reg_read(r) for r in (UC_ARM64_REG_X0, UC_ARM64_REG_X1, UC_ARM64_REG_X2))
        if n > 1 << 30 or not (self._writable(d, n) and self._readable(s, n)):
            self._fault = 'copied %#x bytes from %#x to %#x outside its own buffers' % (n, s, d)
            u.emu_stop()
            return
        if n:
            u.mem_write(d, bytes(u.mem_read(s, n)))
        u.reg_write(UC_ARM64_REG_PC, u.reg_read(UC_ARM64_REG_LR))

    def _region(self, base, cap, end, prot):

        if end > cap:
            if cap:
                self._u.mem_unmap(base, cap)
            cap = _page_up(max(end, cap * 2, 1 << 20))
            self._u.mem_map(base, cap, prot)
        self._u.mem_protect(base, end, prot)
        if cap > end:
            self._u.mem_protect(base + end, cap - end, UC_PROT_NONE)
        return cap

    def decode(self, src, size):

        if not 0 < size <= MAX_BYTES or len(src) > MAX_BYTES:
            raise DecodeError('a section of %d bytes expanding to %d is beyond what this decoder takes' % (len(src), size))
        u = self._u
        self._src_end = _page_up(len(src) + 64)
        self._dst_end = _page_up(size + PAGE)
        self._src_cap = self._region(SRC, self._src_cap, self._src_end, UC_PROT_READ)
        self._dst_cap = self._region(DST, self._dst_cap, self._dst_end, UC_PROT_READ | UC_PROT_WRITE)
        u.mem_write(SRC, bytes(src) + bytes(self._src_end - len(src)))
        u.mem_write(DST, bytes([GUARD]) * self._dst_end)
        for reg, value in ((UC_ARM64_REG_X0, DST), (UC_ARM64_REG_X1, size), (UC_ARM64_REG_X2, SRC), (UC_ARM64_REG_X3, len(src)),
                           (UC_ARM64_REG_SP, STACK[1] - 0x100), (UC_ARM64_REG_LR, RET)):
            u.reg_write(reg, value)
        limit = INSN_BASE + INSN_PER_BYTE * (size + len(src))
        self._fault = None
        try:
            u.emu_start(ENTRY, RET, count=limit)
        except UcError as e:
            raise DecodeError('the decompressor faulted at %#x: %s' % (u.reg_read(UC_ARM64_REG_PC), e)) from e
        if self._fault:
            raise DecodeError('the decompressor ' + self._fault)
        if u.reg_read(UC_ARM64_REG_PC) != RET:
            raise DecodeError('the decompressor did not finish within %d instructions' % limit)
        rc = u.reg_read(UC_ARM64_REG_X0) & 0xffffffff
        if rc:
            raise DecodeError('the decompressor returned %#x for a section of %d bytes' % (rc, size))
        if bytes(u.mem_read(DST + size, self._dst_end - size)) != bytes([GUARD]) * (self._dst_end - size):
            raise DecodeError('the decompressor wrote past the %d bytes of its section' % size)
        return bytes(u.mem_read(DST, size))
