



import os

HERE = os.path.dirname(os.path.abspath(__file__))


def gen_dir(*sub):
    root = os.environ.get('DM_GEN_ASSETS') or os.path.join(HERE, '..', 'Gen', 'assets')
    p = os.path.normpath(os.path.join(root, *sub)); os.makedirs(p, exist_ok=True); return p
