









import builtins, io, os

if os.name == 'nt':
    _open = io.open

    def _lf_open(file, mode='r', buffering=-1, encoding=None, errors=None, newline=None, closefd=True, opener=None):
        if newline is None and 'b' not in mode and '+' not in mode and any(c in mode for c in 'wax'): newline = '\n'
        return _open(file, mode, buffering, encoding, errors, newline, closefd, opener)

    builtins.open = io.open = _lf_open
