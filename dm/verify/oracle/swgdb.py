





import socket, re

class Gdb:
    def __init__(self, host='127.0.0.1', port=55571, timeout=30):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.sock.settimeout(timeout)
        self.buf = b''
        self.pending = []

    def close(self):
        try: self.sock.close()
        except OSError: pass

    def _send(self, payload):
        self.sock.sendall(b'$' + payload + b'#' + b'%02x' % (sum(payload) & 0xFF))

    def _recv_packet(self):
        while True:
            while b'#' not in self.buf or len(self.buf.split(b'#', 1)[1]) < 2:
                chunk = self.sock.recv(1 << 16)
                if not chunk: raise IOError('gdb stub 断开了连接')
                self.buf += chunk
            if self.buf[:1] in (b'+', b'-'): self.buf = self.buf[1:]; continue
            if not self.buf.startswith(b'$'):
                i = self.buf.find(b'$')
                if i < 0: self.buf = b''; continue
                self.buf = self.buf[i:]
            body, rest = self.buf[1:].split(b'#', 1)
            self.buf = rest[2:]
            self.sock.sendall(b'+')
            return body

    def cmd(self, payload):
        self._send(payload)
        expect_stop = payload[:1] in (b'?', b's', b'c') or payload.startswith(b'vCont')
        while True:
            body = self._recv_packet()
            if body.startswith(b'O') and len(body) > 1 and not body.startswith(b'OK'): continue
            if not expect_stop and body[:1] in (b'T', b'S') and b'thread:' in body:
                self._stash(body); continue
            return body

    def monitor(self, text):
        r = self.cmd(b'qRcmd,' + text.encode().hex().encode())
        if r in (b'OK', b''): return ''
        try: return bytes.fromhex(r.decode()).decode('utf-8', 'replace')
        except ValueError: return r.decode('utf-8', 'replace')

    def read(self, addr, n):
        r = self.cmd(b'm%x,%x' % (addr, n))
        if r.startswith(b'E'): raise IOError(f'读 {addr:#x}+{n:#x} 失败：{r.decode()}')
        return bytes.fromhex(r.decode())

    def write(self, addr, data):
        return self.cmd(b'M%x,%x:%s' % (addr, len(data), data.hex().encode())) == b'OK'

    def reg(self, n):

        r = self.cmd(b'p%x' % n)
        try: return int.from_bytes(bytes.fromhex(r.decode()), 'little')
        except ValueError: raise IOError(f'读寄存器 {n} 回包不是十六进制：{r[:40]!r}')

    def set_reg(self, n, v):
        return self.cmd(b'P%x=%s' % (n, (v & 0xFFFFFFFFFFFFFFFF).to_bytes(8, 'little').hex().encode())) == b'OK'

    def set_watch(self, addr, n=8):

        return self.cmd(b'Z2,%x,%x' % (addr, n)) == b'OK'
    def clear_watch(self, addr, n=8): return self.cmd(b'z2,%x,%x' % (addr, n)) == b'OK'

    def set_bp(self, addr): return self.cmd(b'Z0,%x,4' % addr) == b'OK'
    def clear_bp(self, addr): return self.cmd(b'z0,%x,4' % addr) == b'OK'

    def interrupt(self):
        self.sock.sendall(b'\x03')
        while True:
            b = self._recv_packet()
            if b[:1] in (b'T', b'S'): return b

    def cont(self):

        self._send(b'c')
        while True:
            b = self._recv_packet()
            if b[:1] in (b'T', b'S') and b'thread:' in b: self._stash(b); continue
            return b

    @staticmethod
    def _tid(stop):
        import re as _re
        m = _re.search(rb'thread:([0-9a-f]+);', stop or b'')
        return m.group(1) if m else None

    def _stash(self, b):




        t = self._tid(b)
        if t is None or all(self._tid(p) != t for p in self.pending): self.pending.append(b)

    def select(self, stop):




        t = self._tid(stop)
        if t: self.cmd(b'Hg' + t)

    def wait_stop(self, timeout=None, select=True):
        if self.pending:
            b = self.pending.pop(0)
        else:
            old = self.sock.gettimeout()
            if timeout is not None: self.sock.settimeout(timeout)
            try:
                while True:
                    b = self._recv_packet()
                    if b[:1] in (b'T', b'S'): break
            finally: self.sock.settimeout(old)
        if select: self.select(b)
        return b

    def _wait_raw(self, timeout):
        old = self.sock.gettimeout(); self.sock.settimeout(timeout)
        try:
            while True:
                b = self._recv_packet()
                if b[:1] in (b'T', b'S'): return b
        finally: self.sock.settimeout(old)

    def step_over(self, addr, stop_packet):






        tid = self._tid(stop_packet)
        self.clear_bp(addr); self.set_bp(addr + 4); self.cont()
        same = False
        for _ in range(200):
            st = self._wait_raw(10)
            if tid is not None and self._tid(st) == tid:
                self.select(st)
                if self.reg(32) == addr + 4: same = True; break
                self.cont(); continue
            self._stash(st); self.cont()
        self.clear_bp(addr + 4); self.set_bp(addr)
        return same

    def modules(self):

        info = self.monitor('get info')
        return {n: (int(a, 16), int(b, 16)) for a, b, n in re.findall(r'0x([0-9a-fA-F]+)\s*-\s*0x([0-9a-fA-F]+)\s+(\S+)', info)}, info
